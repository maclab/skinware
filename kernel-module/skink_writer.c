/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n. 231500 (ROBOSKIN).
 *
 * This file is part of Skinware.
 *
 * Skinware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Skinware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Skinware.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/div64.h>

#define					SKINK_MODULE_NAME				"skin_kernel"

#include "skink_internal.h"
#include "skink_common.h"
#include "skink_sensor_layer.h"
#include "skink_sensor.h"
#include "skink_module.h"
#include "skink_rt.h"
#include "skink_shared_memory_ids.h"
#include "skink_writer_internal.h"
#include "skink_devices_internal.h"

extern skink_misc_datum			*the_misc_data;
extern skink_sensor_layer		*the_sensor_layers;
extern skink_module			*the_modules;
extern bool				the_devices_resume_possible;

skin_rt_rwlock				*the_sync_locks					= NULL;		// can't put it in skink_sensor_layer
													// because its size is different in kernel
													// and user space and sensor_layers is
													// shared memory
extern skink_state			the_state;

skink_writer_data			*the_writer_thread_data				= NULL;

extern uint64_t				*the_swap_skips;
extern uint64_t				*the_frame_writes;
extern skin_rt_time			*the_worst_write_time;
extern skin_rt_time			*the_best_write_time;
extern skin_rt_time			*the_accumulated_write_time;
extern skin_rt_time			the_time;
static skin_rt_time			_starting_time					= 0;

extern const char			*the_bad_devices[];				// sysfs output

#define MUTEX_TIMED_LOCK(mutex) \
	do {\
		start_wait_time = skin_rt_get_time();\
		could_lock = false;\
		while (skin_rt_get_time() - start_wait_time < lock_timeout)\
		{\
			int temp = skin_rt_mutex_timed_lock(mutex, 100000);	/* try locking for 100 micro seconds*/\
			if (temp == SKIN_RT_SUCCESS)\
			{\
				could_lock = true;\
				error_in_lock = false;\
				break;\
			}\
			else if (temp == SKIN_RT_FAIL)			/* something wrong with the lock */\
			{\
				if (!error_in_lock)			/* to prevent too many messages, only print the first fail after successes */\
					SKINK_RT_LOG("Internal mutex error");\
				error_in_lock = true;			/* keep could_lock 0 so it won't be unlocked */\
				break;\
			}\
			/* else nothing!  try locking again */\
			if (task->must_stop)\
				break;\
		}\
	} while (0)

#define MUTEX_UNLOCK(mutex) \
	do {\
		if (could_lock)						/* if couldn't lock because of error or timeout, then forget about locking! */\
			skin_rt_mutex_unlock(mutex);\
	} while (0)

#define SWAP_BUFFERS \
	do {\
		layer->last_written_buffer		= current_buffer;\
		layer->write_time[current_buffer]	= timestamp;\
		skin_rt_rwlock_write_unlock(sync_locks + current_buffer);\
		current_buffer				= other_buffer;\
		layer->buffer_being_written		= current_buffer;\
		swap_done = true;\
	} while (0)

static void _writer_thread(long int param)
{
	uint32_t			i;
	uint8_t				current_buffer		= 0;
	uint32_t			this_thread		= (uint32_t)param;
	skink_sensor_layer_id		this_layer		= (skink_sensor_layer_id)param;
	skink_writer_data		*task;
	skink_device_info		*device;
	skink_sensor_layer		*layer;					// which layer this thread is running for
	uint8_t				number_of_buffers;			// cache this value
	skin_rt_time			task_period;
	skin_rt_time			best_write_time		= 2000000000;	// used for swap prediction.  Same value as best_write_time
	skin_rt_time			lock_timeout;
	skin_rt_time			start_wait_time;
	bool				could_lock		= false,
					error_in_lock		= false;
	bool				swap_done		= true;
	skin_rt_time			timestamp		= 0;
	skin_rt_time			exec_time;
	skin_rt_time			passed_time;
	skin_rt_rwlock			*sync_locks;				// Refer to part of the array where this thread's locks are
	if (this_layer >= SKINK_SENSOR_LAYERS_COUNT)
	{
		SKINK_LOG("Internal error: writer thread created with wrong data");
		return;
	}
	task			= the_writer_thread_data + this_thread;
	if (task == NULL)
	{
		SKINK_LOG("Internal error: writer thread created with wrong data");
		return;
	}
	device			= task->device;
	if (device == NULL)
	{
		SKINK_LOG("Internal error: writer thread created with wrong data");
		return;
	}
	layer			= the_sensor_layers + this_layer;		// which layer this thread is running for
	if (layer == NULL)
	{
		SKINK_LOG("Internal error: writer thread created with wrong data");
		return;
	}
	number_of_buffers	= layer->number_of_buffers;		// cache this value
	if (number_of_buffers == 0)
	{
		SKINK_LOG("Internal error: writer thread created with wrong data");
		return;
	}
	sync_locks		= the_sync_locks + SKINK_MAX_BUFFERS * this_layer;
	task_period		= 1000000000 / layer->acquisition_rate;
	lock_timeout		= task_period / 4;	// quarter period try lock
	skin_rt_kernel_task_on_start();			// if using rtai, this function is empty
	for (i = 0; i < number_of_buffers; ++i)
		layer->write_time[i] = 0;
	if (number_of_buffers > 1)			// if single-buffer, no locking is done
		skin_rt_rwlock_write_lock(sync_locks + 0);
	current_buffer			= 0;
	layer->last_written_buffer	= 0;
	layer->buffer_being_written	= 0;
	layer->paused			= false;
	task->running			= true;
	task->paused			= false;
	if (_starting_time == 0)			// shared between all threads
		_starting_time = skin_rt_get_time();
	SKINK_RT_LOG("Stage 3: Writer thread for layer %u started (period: %llu)", this_layer, task_period);
	while (!task->must_stop)
	{
		exec_time = skin_rt_get_exectime();
		if (!swap_done)				// if !swap_done, then try swapping buffers again, last chance!
			if (number_of_buffers > 1)
			{
				for (i = 1; i < number_of_buffers; ++i)
				{
					uint8_t other_buffer = (current_buffer + i) % number_of_buffers;	// go to next buffer
					if (skin_rt_rwlock_try_write_lock(sync_locks + other_buffer) == SKIN_RT_SUCCESS)
						// if no readers on other buffer, go write on that
					{
						SWAP_BUFFERS;
						break;
					}
				}
				if (i == number_of_buffers)	// then there are readers still reading the other buffers.  Oh well, continue writing on the same buffer
					++the_swap_skips[this_thread];
				layer->next_predicted_swap = skin_rt_get_time() + best_write_time;
								// next predicted swap is ideally slightly after best_write_time nanoseconds from now
			}
		swap_done = true;

		timestamp = skin_rt_get_time();
		passed_time = skin_rt_get_time();

		// Note: this next line is for debugging purposes.  Useful for computing delay between user program and this thread
		the_misc_data[SKINK_RESPONSE_INDEX] = the_misc_data[SKINK_REQUEST_INDEX];

		/****** start the actual acquisition ******/
		MUTEX_TIMED_LOCK(&device->mutex);
		if (task->must_pause)			// TODO: If between this if and the call to busy the module unloads, there would be a crash
		{					// this shouldn't be a problem though, as the thread has just woken up.  Normally, schedulers give the thread
			MUTEX_UNLOCK(&device->mutex);	// a minimum amount of time even if a higher priority task arrives (right? RIGHT?)
			layer->paused = true;
			task->paused = true;
			skin_rt_kernel_task_wait_period();	// If paused, then check again in the next period
			continue;
		}
		device->busy(1);
		++device->busy_count;
		layer->paused = false;
		task->paused = false;
		MUTEX_UNLOCK(&device->mutex);
		if (device->acquire(layer, layer->sensor_map, current_buffer) != SKINK_SUCCESS)
		{
			// Add it to list of bad devices so an outside process could do something about it
			// If revived or resumed, this will be cleared
			the_bad_devices[device->id] = device->name;
			task->must_pause = true;	// pause the thread, so it won't call acquire again
		}
		MUTEX_TIMED_LOCK(&device->mutex);
		--device->busy_count;
		device->busy(device->busy_count != 0);
		MUTEX_UNLOCK(&device->mutex);
		/****** end of acquisition ******/

		passed_time = skin_rt_get_time() - passed_time;
		if (passed_time < best_write_time)
			best_write_time = passed_time;

		swap_done = false;
		if (number_of_buffers > 1)							// if more than 1 buffers
		{
			do
			{
				uint8_t other_buffer;
				for (i = 1; i < number_of_buffers; ++i)
				{
					other_buffer = (current_buffer + i) % number_of_buffers;	// go to next buffer
					if (skin_rt_rwlock_try_write_lock(sync_locks + other_buffer) == SKIN_RT_SUCCESS)
						// if no readers on other buffer, go write on that
					{
						SWAP_BUFFERS;
						break;
					}
				}
				if (!swap_done)			// If not, try timed-lock on one of the buffers, that is sleeping (like wait_period)
								// unless the lock could be acquired, which means the swap is done immediately after
								// Since sleeping would be done on one of the buffers, while others could be unlocked in
								// the meantime, sleep just a little (200us) and check for unlocked buffers again.
				{
					skin_rt_time time_left = skin_rt_period_time_left();
					skin_rt_time sleep_time;
					if (time_left < 200000)					// not much time left (200us), just wait_period and try swapping after
						break;
					other_buffer = (current_buffer + 1) % number_of_buffers;// Note: The choice of buffer to wait on is not so clear
												// Current method is to wait on the next buffer in the buffer list
												// Other choices include:
												// - Wait on oldest updated buffer
												//     Although this seems logical, in fact it could be the worst choice
												//     because if a buffer is locked for a long time, the owner probably
												//     has died and it is even less likely to free the lock
												// - Wait on the most recent updated buffer
												//     Although this doesn't have the problem of the previous choice, it
												//     is likely that the users have just started reading this last buffer
												//     and therefore the lock wouldn't be taken.
					sleep_time = time_left - 200000;			// Don't sleep past the period start (200us margin considered)
					if (sleep_time > 200000)				// try waiting for the lock for 200us.  If couldn't acquire lock, go back up
						sleep_time = 200000;				// and check again to see if there are any buffers just unlocked
					if (skin_rt_rwlock_timed_write_lock(sync_locks + other_buffer, sleep_time) == SKIN_RT_SUCCESS)
						SWAP_BUFFERS;
				}
			} while (!swap_done);
			layer->next_predicted_swap = skin_rt_get_time() + task_period;	// next predicted swap is ideally same time, next period!
		}	// if in single buffer mode, no locking (TODO: really?), because either way, the writer can't afford to get blocked
			// TODO: However, maybe it would be good to lock the user-lib and have it signaled from here as soon as write is complete
			// so that reading would be more synced with writing.  Problem is though, that the kernel writes probably faster and signals would
			// be generated (in the kernel module) sooner than the user-lib waiting on them, so eventually it would be the same as no locks.
		skin_rt_kernel_task_wait_period();

		// sysfs output
		++the_frame_writes[this_thread];
		the_time = skin_rt_get_time() - _starting_time;
		exec_time = skin_rt_get_exectime() - exec_time;
		if (exec_time == 0)		// I expected wait_period to update this, but apparently it's possible for it not to be updated
			exec_time = passed_time;
		if (exec_time > the_worst_write_time[this_thread])
			the_worst_write_time[this_thread] = exec_time;
		if (exec_time < the_best_write_time[this_thread])
			the_best_write_time[this_thread] = exec_time;
		the_accumulated_write_time[this_thread] += exec_time;
	}
	skin_rt_rwlock_write_unlock(sync_locks + current_buffer);
	task->running = false;
	skin_rt_kernel_task_on_stop();		// if using rtai, this function is empty
	SKINK_RT_LOG("Stage 7: Writer thread for layer %u stopped", this_layer);
}

int skink_initialize_writers(void)
{
	uint32_t		i;
	skin_rt_time		start_time;
	uint32_t		period;
	int			num_success = 0;
#if BITS_PER_LONG != 64
	skin_rt_time		temp;
#endif
	// Create all threads
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		the_writer_thread_data[i].must_stop = false;
		the_writer_thread_data[i].must_pause = true;
		the_writer_thread_data[i].running = false;
		if (skin_rt_kernel_task_init(&the_writer_thread_data[i].task, _writer_thread, i, SKINK_TASK_STACK_SIZE,
					SKINK_CONVERTED_WRITER_PRIORITY) != SKIN_RT_SUCCESS)
			SKINK_LOG("Stage 3: Could not create realtime writer task %d", i);
		else
			the_writer_thread_data[i].created = true;
	}
	// Make the threads periodic and start them
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		if (!the_writer_thread_data[i].created)
			continue;
		period = 1000000000 / the_sensor_layers[i].acquisition_rate;
		start_time = skin_rt_get_time() + 2 * period;
#if BITS_PER_LONG != 64
		temp = start_time;
		start_time -= do_div(temp, period);	// changes temp and returns temp % period
#else
		start_time -= start_time % period;	// Align the start time with multiples of period.  Users will also do it, so kind of already synchronized!
#endif
		start_time += period / SKINK_SENSOR_LAYERS_COUNT * i;		// To make the equal period tasks have different start times
		if (skin_rt_kernel_task_make_periodic(&the_writer_thread_data[i].task, start_time, period) != SKIN_RT_SUCCESS)
			SKINK_LOG("Stage 3: Could not make realtime writer task %d periodic", i);
		else
			++num_success;
	}
	if (num_success == 0)
	{
		skink_free_writers();
		return SKINK_FAIL;
	}
	else if (num_success < SKINK_SENSOR_LAYERS_COUNT)
		return SKINK_PARTIAL;
	return SKINK_SUCCESS;
}

void skink_free_writers(void)
{
	uint32_t	i, j;
	bool		all_quit;
	if (the_writer_thread_data)
	{
		struct timespec		start_, now_;
		uint64_t		start, now;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)			// Tell the threads they should stop
			the_writer_thread_data[i].must_stop = true;
		all_quit = false;
		// Give the threads 1.5 seconds to stop
		getrawmonotonic(&start_);
		start = start_.tv_sec * 1000000000ull + start_.tv_nsec;
		do
		{
			set_current_state(TASK_INTERRUPTIBLE);
			msleep(1);
			all_quit = true;
			for (j = 0; j < SKINK_SENSOR_LAYERS_COUNT; ++j)
				if (the_writer_thread_data[j].created
					&& the_writer_thread_data[j].running)
				{
					all_quit = false;
					break;
				}
			getrawmonotonic(&now_);
			now = now_.tv_sec * 1000000000ull + now_.tv_nsec;
		} while (!all_quit && now - start < 1500000000ull);
		// Delete the tasks, even if they didn't stop
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		{
			if (!the_writer_thread_data[i].created)
				continue;
			if (the_writer_thread_data[i].running)
				SKINK_LOG("Stage %s: Waited for writer thread %d to exit for 1.5s, but it didn't stop.  Killing it",
						(the_state == SKINK_STATE_EXITING?"7":
						the_state == SKINK_STATE_STRUCTURES_BUILT?"4":"?"), i);
			skin_rt_kernel_task_delete(&the_writer_thread_data[i].task);
		}
		vfree(the_writer_thread_data);
		the_writer_thread_data = NULL;
	}
}

void skink_pause_threads(void)
{
	// Note that although the threads are paused, from the devices point of view they are working
	// This way, they can successfully issue a pause command (which has no effect but to change device state)
	uint32_t		i;
	bool			allpaused = false;
	the_devices_resume_possible = false;
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)			// Tell the threads they should pause
		the_writer_thread_data[i].must_pause = true;
	allpaused = false;
	while (!allpaused)						// Wait until threads are paused
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
		allpaused = true;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			if (the_writer_thread_data[i].created
				&& !the_writer_thread_data[i].paused)
			{
				allpaused = false;
				break;
			}
		if (the_state == SKINK_STATE_EXITING)
			return;
	}
}

void skink_resume_threads(void)
{
	// Resume only those threads whose devices are not paused (because while paused (for example for regionalization), some devices may have decided to pause)
	uint32_t		i;
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		if (the_writer_thread_data[i].device->state == SKINK_DEVICE_STATE_WORKING)
			the_writer_thread_data[i].must_pause = false;
	the_devices_resume_possible = true;
}
