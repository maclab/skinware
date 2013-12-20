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

#include <stdlib.h>
#include <time.h>
#include "skin_internal.h"
#include "skin_common.h"
#include "skin_reader.h"
#include "skin_sensor.h"
#include "skin_sensor_type.h"
#include "skin_object.h"

#define SKIN_CONVERTED_TASK_PRIORITY		(SKIN_RT_MAX_PRIORITY - SKIN_USER_ACQ_PRIORITY * SKIN_RT_MORE_PRIORITY)

#define SKINK_INITIALIZED			reader->p_kernel_misc_data[SKINK_INITIALIZED_INDEX]
#define SKINK_SUB_REGIONS_COUNT			reader->p_kernel_misc_data[SKINK_SUB_REGIONS_COUNT_INDEX]
#define SKINK_REGIONS_COUNT			reader->p_kernel_misc_data[SKINK_REGIONS_COUNT_INDEX]

typedef struct internal_list
{
	skin_rt_mutex *mutex;
	struct internal_list *next;
} internal_list;

static int _list_add(internal_list **list, skin_rt_mutex *m)
{
	internal_list *node;
	internal_list *tail;

	tail = *list;
	if (tail != NULL)
	{
		while (tail->next != NULL)
		{
			/* make sure it's not a duplicate */
			if (tail->mutex == m)
				return 0;
			tail = tail->next;
		}
		/* same check for actual tail */
		if (tail->mutex == m)
			return 0;
	}

	node = malloc(sizeof(*node));
	if (node == NULL)
		return -1;

	*node = (internal_list){
		.mutex = m,
		.next = NULL
	};

	if (tail == NULL)
		*list = node;
	else
		tail->next = node;

	return 0;
}

static void _list_remove(internal_list **list, skin_rt_mutex *m)
{
	internal_list *prev;
	internal_list *cur;
	internal_list *next;

	prev = NULL;
	cur = *list;
	while (cur != NULL)
	{
		next = cur->next;
		/* check if node should be removed */
		if (cur->mutex == m)
		{
			/* unlink from list */
			if (prev == NULL)
				*list = next;
			else
				prev->next = next;
			/* remove */
			free(cur);
			/* there are no duplicates */
			return;
		}
		prev = cur;
		cur = next;
	}
}

static void _list_op(internal_list *list, int (*op)(skin_rt_mutex *))
{
	internal_list *cur;

	cur = list;
	while (cur != NULL)
	{
		op(cur->mutex);
		cur = cur->next;
	}
}

static void _list_clear(internal_list **list)
{
	internal_list *cur;
	internal_list *next;

	cur = *list;
	while (cur != NULL)
	{
		next = cur->next;
		free(cur);
		cur = next;
	}

	*list = NULL;
}

struct skin_reader_data
{
	skin_rt_task_id			task_id;
	skin_rt_task			*task;
	skin_acquisition_type		acq_mode;		/* one of SKIN_ACQUISITION_* */
	skin_rt_semaphore		*read_request;		/* sporadic request/response locks */
	skin_rt_semaphore		*read_response;
	skin_rt_time			period;			/* period if periodic */
	bool				running;
	bool				must_stop;
	bool				enabled;
	skin_sensor_type_id		task_index;		/* This and acq_mode are used to avoid */
	skin_reader			*reader;		/* creating (and storing) arguments to reader threads */
};

struct shared_layer_data
{
	skink_sensor			*skin_sensors;
	skin_rt_rwlock			*skin_sync_locks[SKINK_MAX_BUFFERS];
};

static void _signal_all_waiting_users(struct skin_reader_data *task)
{
	int i;
	int tasks_waiting = 1;		/* one task responsible for waking this thread up */

	/* for each successful try lock, one user is waiting */
	while (skin_rt_sem_try_wait(task->read_request) == SKIN_RT_SUCCESS)
		++tasks_waiting;

	/* Unblock as many users as there were requests */
	for (i = 0; i < tasks_waiting; ++i)
		skin_rt_sem_post(task->read_response);
}

/* Note: updates to service reader, should be reflected here too (and vice versa) */
void *the_skin_reader_thread(void *arg)
{
	/* arguments */
	struct skin_reader_data		*task;		/* data of this task */
	skin_reader			*sc;		/* skin_reader */
	skin_sensor_type_id		st;		/* sensor type */
	skin_object			*so;		/* skin_object reference */
	skin_task_statistics		*stat;

	/* other stuff to cache */
	skin_rt_time			task_period;
	skink_sensor_layer		*kernel_layer;
	skin_sensor_type		*this_layer;
	bool				single_buffer;
	skin_rt_time			kernel_period;
	struct shared_layer_data	layer;
	skin_sensor			*sensors_here;	/* sensors in skin library */
	skink_sensor			*sensors_there;	/* sensors in skin kernel */
	skin_sensor_size		scount;		/* number of sensors */

	/* other variables */
	uint8_t				buffer;
	uint8_t				last_buffer;
	skin_rt_time			last_timestamp;
	skin_rt_time			new_timestamp;
	skin_rt_time			kernel_time_to_user;
	bool				swap_prediction_possible;
	skin_acquisition_type		acquisition_type;
	skin_sensor_id			i;
	char				temp[50] = "";

	/* arguments */
	task = arg;
	sc = task->reader;
	if (sc == NULL)
		return NULL;
	st = task->task_index;
	so = sc->p_object;
	if (so == NULL)
		return NULL;
	if (st >= sc->tasks_count)
	{
		skin_log_set_error(so, "Internal error: Task being created for sensor type %u which is non-existent", st);
		return NULL;
	}
	if (sc->tasks_statistics == NULL)
	{
		skin_log_set_error(so, "Statistics memory not initialized (for sensor type %u)", st);
		return NULL;
	}
	stat = sc->tasks_statistics + st;

	/* cache */
	task_period = task->period;
	stat->p_swap_protection_time = 1000;		/* some value I know is definitely smaller than the time needed */
	kernel_layer = sc->p_kernel_sensor_layers + st;
	this_layer = so->p_sensor_types + st;
	single_buffer = kernel_layer->number_of_buffers == 1;
	kernel_period = 1000000000 / kernel_layer->acquisition_rate;
	layer.skin_sensors = sc->p_kernel_sensors + so->p_sensor_types[st].sensors_begin;
				/*
				 * Note: this index is also valid in the skin kernel, even though
				 * it is not stored there.  This is computed in load_from_kernel and (for
				 * now) it is guaranteed that the order of sensors in the kernel and library
				 * structures is the same.  Otherwise, this couldn't have been used
				 */

	if (layer.skin_sensors == NULL)
	{
		skin_log_set_error(so, "Could not attach to kernel module's memories");
		skin_rt_user_task_on_stop();
		return NULL;
	}
	sensors_here = so->p_sensors + so->p_sensor_types[st].sensors_begin;
	sensors_there = layer.skin_sensors;
	scount = kernel_layer->sensors_count;

	/* variables */
	buffer = 0;
	last_buffer = buffer;
	last_timestamp = 0;
	new_timestamp = 0;
	kernel_time_to_user = 0;
	swap_prediction_possible = true;
	acquisition_type = task->acq_mode;

	if (acquisition_type == SKIN_ACQUISITION_SPORADIC && (!task->read_request || !task->read_response))
	{
		skin_log_set_error(so, "Sporadic locks have not been correctly created");
		skin_rt_user_task_on_stop();
		return NULL;
	}

	if ((single_buffer && acquisition_type != SKIN_ACQUISITION_SPORADIC) || acquisition_type == SKIN_ACQUISITION_PERIODIC)
		snprintf(temp, sizeof(temp) - 1, " (period: %llu)", task_period);
	skin_log("Thread starting (sensor type %u) (PID: %ld - TID: %ld)%s", st,
			(long)getpid(), (long)(pid_t)syscall(SYS_gettid), temp);

	/* make task real-time */
	task->task = skin_rt_user_task_on_start(SKIN_CONVERTED_TASK_PRIORITY);
	if (task->task == NULL)
	{
		skin_log_set_error(so, "Could not get buddy task of reader thread (for sensor type %u)!", st);
		return NULL;
	}

	/* get shared locks (only if multiple buffers exist) */
	if (!single_buffer)
		for (i = 0; i < kernel_layer->number_of_buffers; ++i)
		{
			char lock_name[SKIN_RT_MAX_NAME_LENGTH + 1];

			skink_get_rwlock_name(lock_name, st, i);
			layer.skin_sync_locks[i] = skin_rt_get_shared_rwlock(lock_name);
			if (layer.skin_sync_locks[i] == NULL)
			{
				skin_log_set_error(so, "Could not get shared synchronization locks (for sensor type %u)!", st);
				skin_rt_user_task_on_stop();
				return NULL;
			}
		}

	kernel_time_to_user = skin_object_time_diff_with_kernel(so);
	if (kernel_time_to_user == 0)			/* the kernel could not create the sysfs file.  Problem is more serious than time-synchronization! */
		swap_prediction_possible = false;	/* anyway, disable swap prediction */

	skin_rt_user_task_make_hard_real_time();
	if ((single_buffer && acquisition_type != SKIN_ACQUISITION_SPORADIC)
		/* if single buffer and not sporadic, then it's made periodic */
		|| acquisition_type == SKIN_ACQUISITION_PERIODIC)
	{
		/*
		 * align the start time with multiples of kernel period.
		 * This way, the user and kernel threads are already more or less synchronized
		 */
		skin_rt_time start_time = skin_rt_get_time() + 2 * kernel_period;
		start_time -= (start_time - kernel_time_to_user) % kernel_period;
		start_time += kernel_period / so->p_sensor_types_count * st;
		/*
		 * it has been observed that the kernel is not faster than 10us.
		 * This next line is to fire user tasks shortly after kernel tasks
		 */
		start_time += 10000;
		skin_rt_user_task_make_periodic(task->task, start_time, task_period);
		skin_rt_user_task_wait_period();	/* wait for it to start */
	}

	task->running = true;
	while (!task->must_stop)
	{
		skin_rt_time exec_time;
		skin_rt_time passed_time;
		bool kernel_paused;

		kernel_paused = kernel_layer->paused;
		this_layer->is_active = !kernel_paused;

		/* if thread is disabled or kernel thread not working, sleep and try again */
		if (!task->enabled || kernel_layer->paused)
		{
			if (acquisition_type == SKIN_ACQUISITION_PERIODIC
				|| (single_buffer && acquisition_type != SKIN_ACQUISITION_SPORADIC))
				skin_rt_user_task_wait_period();
			else
			{
				/* if this thread is paused, skip sporadic requests */
				if (acquisition_type == SKIN_ACQUISITION_SPORADIC
					&& skin_rt_sem_try_wait(task->read_request) == SKIN_RT_SUCCESS)
					_signal_all_waiting_users(task);
				skin_rt_sleep(SKIN_MAX_THREAD_DELAY);
			}
			continue;
		}

		exec_time = skin_rt_get_exectime();
		passed_time = 0;

		/* if sporadic, wait for request */
		if (acquisition_type == SKIN_ACQUISITION_SPORADIC)
		{
			int ret = 0;
			while ((ret = skin_rt_sem_timed_wait(task->read_request, SKIN_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
			{
				if (ret == SKIN_RT_FAIL)
				{
					skin_log_set_error(so, "Acquiring sporadic lock failed, quitting!");
					task->must_stop = true;
				}
				/*
				 * if not, then it must be timeout, which is fine.  Try again, but just test
				 * to see if you are supposed to stop
				 */
				if (task->must_stop || !task->enabled || kernel_layer->paused)
					break;
			}
			if (task->must_stop || !task->enabled || kernel_layer->paused)
			{
				if (ret == SKIN_RT_SUCCESS)
					_signal_all_waiting_users(task);
				continue;
			}
		}

		/* if not single buffer, synchronize with the writer, otherwise read without locking */
		if (!single_buffer)
		{
			buffer = kernel_layer->last_written_buffer;
			new_timestamp = kernel_layer->write_time[buffer];
			if ((buffer == last_buffer && last_timestamp >= new_timestamp)	/* then already has the last updated version */
				|| (swap_prediction_possible				/* this could be false in case couldn't open kernel's sysfs files */
					&& sc->p_enable_swap_skip_prediction		/* and if user hasn't disabled it */
					&& skin_rt_get_time() + stat->p_swap_protection_time > kernel_layer->next_predicted_swap + kernel_time_to_user))
					/* then it is guessed that I can't make the read in time, so sleep on the buffer being written */
			{
				int ret = 0;
				buffer = kernel_layer->buffer_being_written;
				while ((ret = skin_rt_rwlock_timed_read_lock(layer.skin_sync_locks[buffer], task_period)) != SKIN_RT_SUCCESS)
				{
					if (ret == SKIN_RT_FAIL)
					{
						skin_log_set_error(so, "Acquiring read lock failed, quitting!");
						task->must_stop = true;
					}
					/*
					 * if not, then it must be timeout, which is fine.  Try again, but just test
					 * to see if task is supposed to stop
					 */
					if (task->must_stop || !task->enabled || kernel_layer->paused)
						break;
				}
				if (task->must_stop || !task->enabled || kernel_layer->paused)
				{
					if (ret == SKIN_RT_SUCCESS)
						skin_rt_rwlock_read_unlock(layer.skin_sync_locks[buffer]);
					if (acquisition_type == SKIN_ACQUISITION_SPORADIC)
						_signal_all_waiting_users(task);
					continue;
				}
				new_timestamp = kernel_layer->write_time[buffer];
			}
			else if (skin_rt_rwlock_try_read_lock(layer.skin_sync_locks[buffer]) == SKIN_RT_LOCK_NOT_ACQUIRED)
			{
				/*
				 * lock could not be acquired, which means from a few lines ago till now,
				 * the kernel finished updating buffers and has switched to this buffer
				 */
				if (acquisition_type == SKIN_ACQUISITION_SPORADIC)
					skin_rt_sem_post(task->read_request);	/* allow reallocation of request lock */
				continue;
			}
			/* the lock is now successfully acquired */
			last_buffer = buffer;
			last_timestamp = new_timestamp;
		}

		/*
		 * try-lock all mutexes registered for read, so that if they are free, they would be locked.
		 * You don't want a user to arrive while read is in progress.
		 */
		skin_rt_mutex_lock(sc->p_registered_users_mutex);
		_list_op(((internal_list **)sc->p_registered_users)[st], skin_rt_mutex_try_lock);
		skin_rt_mutex_unlock(sc->p_registered_users_mutex);

		passed_time = skin_rt_get_time();

		/* data copy from skin kernel */
		for (i = 0; i < scount; ++i)
			sensors_here[i].response = sensors_there[i].response[buffer];

		/* update run time statistics for swap skip protection */
		passed_time = skin_rt_get_time() - passed_time;
		if (passed_time > stat->p_swap_protection_time)
			stat->p_swap_protection_time = (stat->p_swap_protection_time * 7 + passed_time) >> 3;     /* converge to passed time with a factor of 1/8 */

		/* release sync locks */
		if (!single_buffer)
			skin_rt_rwlock_read_unlock(layer.skin_sync_locks[buffer]);
		if (acquisition_type == SKIN_ACQUISITION_SPORADIC)
			_signal_all_waiting_users(task);

		/*
		 * unlock all mutexes registered for read, so that
		 * if there are users waiting on it, they would be unlocked
		 */
		skin_rt_mutex_lock(sc->p_registered_users_mutex);
		_list_op(((internal_list **)sc->p_registered_users)[st], skin_rt_mutex_unlock);
		skin_rt_mutex_unlock(sc->p_registered_users_mutex);

		if ((single_buffer && acquisition_type == SKIN_ACQUISITION_ASAP) || acquisition_type == SKIN_ACQUISITION_PERIODIC)
			/* If single buffer and ASAP, then it's considered periodic */
			skin_rt_user_task_wait_period();

		/* update statistics */
		exec_time = skin_rt_get_exectime() - exec_time;
		/*
		 * if 0, then no context switch has happened since last call, so this value is not updated
		 * use passed_time instead.  This needs to be fixed by RTAI and I have sent a patch for it
		 */
		if (exec_time == 0)
			exec_time = passed_time;
		++stat->number_of_reads;
		if (exec_time > stat->worst_read_time)
			stat->worst_read_time = exec_time;
		if (exec_time < stat->best_read_time)
			stat->best_read_time = exec_time;
		stat->accumulated_read_time += exec_time;
	}

	skin_rt_user_task_on_stop();
	skin_log("Thread exiting (sensor type %u) (PID: %ld - TID: %ld)",
			st, (long)getpid(), (long)(pid_t)syscall(SYS_gettid));
	task->running = false;
	return NULL;
}

void skin_reader_init(skin_reader *reader)
{
	*reader = (skin_reader){
		.p_enable_swap_skip_prediction = true
	};
}

void skin_reader_free(skin_reader *reader)
{
	skin_reader_internal_terminate(reader);
}

int skin_reader_internal_initialize(skin_reader *reader, skin_sensor_type_size task_count, bool for_setup)
{
	bool fromlinux;
	int ret;
	skin_sensor_type_id i;

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (task_count == 0)
	{
		skin_log_set_error(reader->p_object, "Cannot initialize reader with 0 tasks!");
		return SKIN_FAIL;
	}
	if (!reader->p_object->p_sensors)
	{
		skin_log_set_error(reader->p_object, "The main object must be initialized before initialize could be called.  This is an internal error.");
		return SKIN_FAIL;
	}
	if (!skin_rt_priority_is_valid(SKIN_CONVERTED_TASK_PRIORITY))
	{
		skin_log_set_error(reader->p_object, "The assigned task priority (%u) is not supported.  I am afraid you need to reconfigure Skinware",
				SKIN_CONVERTED_TASK_PRIORITY);
		return SKIN_FAIL;
	}

	/* initialize memory */
	reader->p_readers_data		= malloc(task_count * sizeof(*reader->p_readers_data));
	reader->tasks_statistics	= malloc(task_count * sizeof(*reader->tasks_statistics));
	reader->p_registered_users	= malloc(task_count * sizeof(internal_list *));
	if (!reader->p_readers_data || !reader->tasks_statistics || !reader->p_registered_users)
	{
		skin_log_set_error(reader->p_object, "Cannot obtain memory for task creation!");
		return SKIN_NO_MEM;
	}
	for (i = 0; i < task_count; ++i)
	{
		reader->p_readers_data[i] = (struct skin_reader_data){0};
		((internal_list **)reader->p_registered_users)[i] = NULL;
	}
	reader->tasks_count = task_count;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make initialization thread work with realtime functions");
		fromlinux = false;
		goto exit_fail;
	}

	/* attach to skin kernel's memory */
	reader->p_kernel_misc_data = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	if (reader->p_kernel_misc_data == NULL)
	{
		skin_log_set_error(reader->p_object, "Could not attach to misc shared memory");
		goto exit_fail;
	}
	if (!for_setup && !SKINK_INITIALIZED)
	{
		skin_log_set_error(reader->p_object, "According to the misc memory, the kernel module is either not initialized properly or unloaded");
		goto exit_fail;
	}
	reader->p_kernel_sensor_layers = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	reader->p_kernel_sensors = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	if (!reader->p_kernel_sensor_layers || !reader->p_kernel_sensors)
	{
		skin_log_set_error(reader->p_object, "Could not attach to kernel shared memories");
		goto exit_fail;
	}
	reader->p_registered_users_mutex = skin_rt_mutex_init(NULL, 1);
	if (!reader->p_registered_users_mutex)
	{
		skin_log_set_error(reader->p_object, "Could not acquire necessary locks");
		goto exit_fail;
	}
	/* initialize tasks data */
	for (i = 0; i < task_count; ++i)
	{
		reader->tasks_statistics[i] = (skin_task_statistics){
			.best_read_time = 2000000000
		};
		reader->p_readers_data[i] = (struct skin_reader_data){
			.acq_mode = SKIN_ACQUISITION_ASAP,
			.must_stop = true
		};
	}
	ret = SKIN_SUCCESS;
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return ret;
exit_fail:
	skin_reader_internal_terminate(reader);
	ret = SKIN_FAIL;
	goto exit_normal;
}

static void _remove_task(skin_reader *reader, skin_sensor_type_id sensor_type, skin_rt_time wait_time)
{
	if (reader->p_readers_data[sensor_type].task)
	{
		char *type_name = reader->p_object->p_sensor_types[sensor_type].name;
		if (reader->p_readers_data[sensor_type].running)
			skin_log_set_error(reader->p_object, "Waited for reader thread for sensor type %u (%s) to exit for %fs, but it didn't stop."
					"  Killing it.", sensor_type, type_name?type_name:"<no name>", wait_time / 1000000000.0f);
		skin_rt_user_task_delete(reader->p_readers_data[sensor_type].task);
		skin_rt_user_task_join(reader->p_readers_data[sensor_type].task_id);
	}
	reader->p_readers_data[sensor_type].task = NULL;
}

int skin_reader_internal_terminate(skin_reader *reader)
{
	skin_sensor_type_id	i;
	bool			all_quit = false;
	struct timespec		start_time_,
				now_time_;
	uint64_t		start_time;
        uint64_t		now_time;
	uint64_t		max_wait_time = 0;

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
/*		skin_log_set_error(reader->p_object, "Cannot terminate reader because it is not initialized properly."); */
				/*
				 * don't give error because it could be that the skin_object object is not initialized and
				 * is simply being destroyed during termination of program because of some unrelated error
				 */
		return SKIN_FAIL;
	}
	for (i = 0; i < reader->tasks_count; ++i)
		reader->p_readers_data[i].must_stop = true;

	/* compute a wait time: max(task periods) + 1.5 seconds, or 1.5 seconds if none are periodic */
	for (i = 0; i < reader->tasks_count; ++i)
		if (reader->p_readers_data[i].acq_mode == SKIN_ACQUISITION_PERIODIC
			&& reader->p_readers_data[i].period > (skin_rt_time)max_wait_time)
			max_wait_time = reader->p_readers_data[i].period;
	max_wait_time += 1500000000ull;
	clock_gettime(CLOCK_MONOTONIC, &start_time_);
	start_time = start_time_.tv_sec * 1000000000ull + start_time_.tv_nsec;

	/* wait for tasks to stop */
	do
	{
		usleep(1000);
		all_quit = true;
		for (i = 0; i < reader->tasks_count; ++i)
			if (reader->p_readers_data[i].running)
			{
				all_quit = false;
				break;
			}
		clock_gettime(CLOCK_MONOTONIC, &now_time_);
		now_time = now_time_.tv_sec * 1000000000ull + now_time_.tv_nsec;
	} while (!all_quit && now_time-start_time < max_wait_time);

	/* if not stopped, kill them anyway */
	for (i = 0; i < reader->tasks_count; ++i)
		_remove_task(reader, i, max_wait_time);

	/* sporadic request semaphores clean up */
	for (i = 0; i < reader->tasks_count; ++i)
	{
		if (reader->p_readers_data[i].read_request)
			skin_rt_sem_delete(reader->p_readers_data[i].read_request);
		if (reader->p_readers_data[i].read_response)
			skin_rt_sem_delete(reader->p_readers_data[i].read_response);
	}
	free(reader->p_readers_data);

	/* detach from skin kernel */
	if (reader->p_kernel_sensor_layers)
		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	if (reader->p_kernel_sensors)
		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	if (reader->p_kernel_misc_data)
		skin_rt_shared_memory_detach(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);

	/* clean up of user synchronization data */
	if (reader->p_registered_users)
		for (i = 0; i < reader->tasks_count; ++i)
			_list_clear(&((internal_list **)reader->p_registered_users)[i]);
	free(reader->p_registered_users);
	if (reader->p_registered_users_mutex)
		skin_rt_mutex_delete(reader->p_registered_users_mutex);

	/* other clean up */
	free(reader->tasks_statistics);
	skin_reader_init(reader);

	return SKIN_SUCCESS;
}

static int _start(skin_reader *reader, skin_sensor_type_id sensor_type, skin_acquisition_type t, skin_rt_time period)
{
	skin_rt_time kernel_period;

	/* set data for readers */
	kernel_period = 1000000000 / reader->p_kernel_sensor_layers[sensor_type].acquisition_rate;
	if (period <= kernel_period || t == SKIN_ACQUISITION_ASAP)
		period = kernel_period;
	reader->p_readers_data[sensor_type] = (struct skin_reader_data){
		.enabled = true,
		.acq_mode = t,
		.period = period,
		.task_index = sensor_type,
		.reader = reader,
	};

	/* if starting thread in sporadic mode, get sporadic request locks */
	if (t == SKIN_ACQUISITION_SPORADIC)
	{
		reader->p_readers_data[sensor_type].read_request = skin_rt_sem_init(NULL, 0);
		reader->p_readers_data[sensor_type].read_response = skin_rt_sem_init(NULL, 0);
		if (!reader->p_readers_data[sensor_type].read_request || !reader->p_readers_data[sensor_type].read_response)
		{
			skin_log_set_error(reader->p_object, "Could not acquire sporadic acquisition locks!");
			reader->p_readers_data[sensor_type].must_stop = true;
			return SKIN_FAIL;
		}
	}

	reader->p_readers_data[sensor_type].task_id = skin_rt_user_task_init(the_skin_reader_thread,
							reader->p_readers_data + sensor_type, SKIN_TASK_STACK_SIZE);
	if (reader->p_readers_data[sensor_type].task_id == 0)	/* invalid */
	{
		skin_log_set_error(reader->p_object, "Could not create reader thread for sensor type %u", sensor_type);
		skin_reader_stop(reader, sensor_type);
		return SKIN_FAIL;
	}

	return SKIN_SUCCESS;
}

int skin_reader_start(skin_reader *reader, skin_sensor_type_id sensor_type, skin_acquisition_type t, skin_rt_time period)
{
	bool fromlinux;
	int ret;

	skin_reader_stop(reader, sensor_type);

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_object->p_sensor_types)
	{
		skin_log_set_error(reader->p_object, "The main object must be initialized before start could be called");
		return SKIN_FAIL;
	}
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before start could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}
	if (sensor_type != SKIN_ALL_SENSOR_TYPES && sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u", sensor_type);
		return SKIN_BAD_ID;
	}

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make initialization thread work with realtime functions");
		return SKIN_FAIL;
	}

	/* if all sensor types requested, call this same function with each sensor type id */
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		bool failure = false;
		bool success = false;
		skin_sensor_type_id i;

		for (i = 0; i < reader->tasks_count; ++i)
		{
			if (_start(reader, i, t, period) == SKIN_SUCCESS)
				success = true;
			else
				failure = true;
		}
		if (success && !failure)
			ret = SKIN_SUCCESS;
		else if (failure && !success)
			ret = SKIN_FAIL;
		else
			ret = SKIN_PARTIAL;
	}
	else
		ret = _start(reader, sensor_type, t, period);
	if (fromlinux)
		skin_rt_stop();
	return ret;
}

static int _check_request(skin_reader *reader, skin_sensor_type_id sensor_type, const char *func)
{
	if (sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to %s", sensor_type, func);
		return SKIN_BAD_ID;
	}
	if (reader->p_readers_data[sensor_type].acq_mode != SKIN_ACQUISITION_SPORADIC)
	{
		skin_log_set_error(reader->p_object, "%s must only be called only if acquiring data in sporadic mode", func);
		return SKIN_FAIL;
	}
	if (!reader->p_readers_data[sensor_type].read_request || !reader->p_readers_data[sensor_type].read_response)
	{
		skin_log_set_error(reader->p_object, "Sporadic acquisition locks have not been acquired properly (Check previous logs)");
		return SKIN_FAIL;
	}
	return SKIN_SUCCESS;
}

static void _request_nonblocking(skin_reader *reader, skin_sensor_type_id sensor_type)
{
	/* signal reader thread to acquire data */
	skin_rt_sem_post(reader->p_readers_data[sensor_type].read_request);
}

static int _await_response(skin_reader *reader, skin_sensor_type_id sensor_type, bool *must_stop)
{
	int ret = 0;

	/* wait for the reader thread to finish reading */
	while ((ret = skin_rt_sem_timed_wait(reader->p_readers_data[sensor_type].read_response,
			SKIN_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
	{
		if (ret == SKIN_RT_FAIL)
		{
			skin_log_set_error(reader->p_object, "Acquiring sporadic lock failed.  Ignoring response wait!");
			return SKIN_FAIL;
		}
		/* if not failed, it's fine.  Try again, but just test to see if you're supposed to stop */
		if (must_stop && *must_stop)
		{
			skin_log_set_error(reader->p_object, "Warning: The program is asked to stop.  Ignoring response wait!");
			return SKIN_FAIL;
		}
		if (reader->p_readers_data[sensor_type].must_stop)
		{
			skin_log_set_error(reader->p_object, "Warning: The reader thread is asked to stop.  Ignoring response wait!");
			return SKIN_FAIL;
		}
	}
	return SKIN_SUCCESS;
}

int skin_reader_request(skin_reader *reader, skin_sensor_type_id sensor_type, bool *must_stop)
{
	bool fromlinux;
	int ret;

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before request could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make thread calling request a real-time thread!");
		return SKIN_FAIL;
	}

	/* if all sensor types requested, call request_nonblocking for all sporadic layers and then await_response */
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		bool *requested = malloc(reader->tasks_count * sizeof(*requested));
		int should_request = 0;
		int actual_request = 0;
		skin_sensor_type_id i;

		if (requested == NULL)
			goto exit_fail;

		memset(requested, false, reader->tasks_count * sizeof(*requested));
		for (i = 0; i < reader->tasks_count; ++i)
			if (reader->p_readers_data[i].acq_mode == SKIN_ACQUISITION_SPORADIC
				&& _check_request(reader, i, "request") == SKIN_SUCCESS)
			{
				++should_request;
				++actual_request;
				_request_nonblocking(reader, i);
				requested[i] = true;
			}
		for (i = 0; i < reader->tasks_count; ++i)
			if (requested[i])
				if (_await_response(reader, i, must_stop) != SKIN_SUCCESS)
					--actual_request;
		free(requested);

		if (actual_request == 0 && should_request > 0)
			ret = SKIN_FAIL;
		else if (actual_request < should_request)
			ret = SKIN_PARTIAL;
		else
			ret = SKIN_SUCCESS;
	}
	else
	{
		ret = _check_request(reader, sensor_type, "request");
		if (ret != SKIN_SUCCESS)
			goto exit_normal;
		_request_nonblocking(reader, sensor_type);
		ret = _await_response(reader, sensor_type, must_stop);
	}
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return ret;
exit_fail:
	ret = SKIN_FAIL;
	goto exit_normal;
}

int skin_reader_request_nonblocking(skin_reader *reader, skin_sensor_type_id sensor_type)
{
	bool fromlinux;
	int ret;

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before request_nonblocking could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make thread calling request_nonblocking a real-time thread!");
		return SKIN_FAIL;
	}

	/* if all sensor types requested, call request_nonblocking for all sporadic layers */
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		int requested = 0;
		skin_sensor_type_id i;

		for (i = 0; i < reader->tasks_count; ++i)
			if (reader->p_readers_data[i].acq_mode == SKIN_ACQUISITION_SPORADIC
				&& _check_request(reader, i, "request_nonblocking") == SKIN_SUCCESS)
			{
				++requested;
				_request_nonblocking(reader, i);
			}
		if (requested == 0)
			ret = SKIN_FAIL;
		else
			ret = SKIN_SUCCESS;
	}
	else
	{
		ret = _check_request(reader, sensor_type, "request_nonblocking");
		if (ret == SKIN_SUCCESS)
			_request_nonblocking(reader, sensor_type);
	}
	if (fromlinux)
		skin_rt_stop();
	return SKIN_SUCCESS;
}

int skin_reader_await_response(skin_reader *reader, skin_sensor_type_id sensor_type, bool *must_stop)
{
	bool fromlinux;
	int ret;

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before await_response could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make thread calling await_response a real-time thread!");
		return SKIN_FAIL;
	}

	/* if all sensor types requested, call await_response for all sporadic layers */
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		int should_respond = 0;
		int actual_respond = 0;
		skin_sensor_type_id i;

		for (i = 0; i < reader->tasks_count; ++i)
			if (reader->p_readers_data[i].acq_mode == SKIN_ACQUISITION_SPORADIC
				&& _check_request(reader, i, "await_response") == SKIN_SUCCESS)
			{
				++should_respond;
				if (_await_response(reader, i, must_stop) == SKIN_SUCCESS)
					++actual_respond;
			}
		if (actual_respond == 0 && should_respond > 0)
			ret = SKIN_FAIL;
		else if (actual_respond < should_respond)
			ret = SKIN_PARTIAL;
		else
			ret = SKIN_SUCCESS;
	}
	else
	{
		ret = _check_request(reader, sensor_type, "request_nonblocking");
		if (ret != SKIN_SUCCESS)
			goto exit_normal;
		ret = _await_response(reader, sensor_type, must_stop);
	}
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return ret;
}

int skin_reader_stop(skin_reader *reader, skin_sensor_type_id sensor_type)
{
	struct timespec		start_time_,
				now_time_;
	uint64_t		start_time;
        uint64_t		now_time;
	uint64_t		wait_time = 0;

	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before request could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}
	if (sensor_type != SKIN_ALL_SENSOR_TYPES && sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to stop", sensor_type);
		return SKIN_BAD_ID;
	}

	/* if all sensor types requested, call this same function with each sensor type id */
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		bool failure = false;
		bool success = false;
		skin_sensor_type_id i;

		for (i = 0; i < reader->tasks_count; ++i)
		{
			int ret = skin_reader_stop(reader, i);
			if (ret == SKIN_SUCCESS)
				success = true;
			else
				failure = true;
		}
		if (success && !failure)
			return SKIN_SUCCESS;
		if (failure && !success)
			return SKIN_FAIL;
		return SKIN_PARTIAL;
	}

	/* tell the reader it should stop */
	reader->p_readers_data[sensor_type].must_stop = true;

	/* give it its period + 1.5 seconds, or just 1.5 seconds if not periodic */
	if (reader->p_readers_data[sensor_type].acq_mode == SKIN_ACQUISITION_PERIODIC
		&& reader->p_readers_data[sensor_type].period > (skin_rt_time)wait_time)
		wait_time = reader->p_readers_data[sensor_type].period;
	wait_time += 1500000000ull;
	clock_gettime(CLOCK_MONOTONIC, &start_time_);
	start_time = start_time_.tv_sec * 1000000000ull + start_time_.tv_nsec;

	/* wait for the task to stop */
	do
	{
		usleep(1000);
		if (!reader->p_readers_data[sensor_type].running)
			break;
		clock_gettime(CLOCK_MONOTONIC, &now_time_);
		now_time = now_time_.tv_sec * 1000000000ull + now_time_.tv_nsec;
	} while (now_time - start_time < wait_time);

	/* if not stopped, kill it anyway */
	_remove_task(reader, sensor_type, wait_time);

	/* sporadic request semaphores clean up */
	if (reader->p_readers_data[sensor_type].read_request)
		skin_rt_sem_delete(reader->p_readers_data[sensor_type].read_request);
	if (reader->p_readers_data[sensor_type].read_response)
		skin_rt_sem_delete(reader->p_readers_data[sensor_type].read_response);
	reader->p_readers_data[sensor_type].read_request = NULL;
	reader->p_readers_data[sensor_type].read_response = NULL;

	/* clean up of user synchronization data */
	_list_clear(&((internal_list **)reader->p_registered_users)[sensor_type]);

	/* other clean up */
	reader->tasks_statistics[sensor_type] = (skin_task_statistics){
		.best_read_time = 2000000000
	};

	return SKIN_SUCCESS;
}

int skin_reader_pause(skin_reader *reader, skin_sensor_type_id sensor_type)
{
	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before pause could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		skin_sensor_type_id i;
		for (i = 0; i < reader->tasks_count; ++i)
			reader->p_readers_data[i].enabled = false;
		return SKIN_SUCCESS;
	}
	if (sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to pause", sensor_type);
		return SKIN_BAD_ID;
	}
	reader->p_readers_data[sensor_type].enabled = false;
	return SKIN_SUCCESS;
}

int skin_reader_resume(skin_reader *reader, skin_sensor_type_id sensor_type)
{
	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before resume could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		skin_sensor_type_id i;
		for (i = 0; i < reader->tasks_count; ++i)
			reader->p_readers_data[i].enabled = true;
		return SKIN_SUCCESS;
	}
	if (sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to resume", sensor_type);
		return SKIN_BAD_ID;
	}
	reader->p_readers_data[sensor_type].enabled = true;
	return SKIN_SUCCESS;
}

int skin_reader_is_paused(skin_reader *reader, skin_sensor_type_id sensor_type)
{
	if (reader->p_object == NULL)
		return SKIN_FAIL;
	if (!reader->p_readers_data)
	{
		skin_log_set_error(reader->p_object, "The reader must be initialized before is_paused could be called.\n"
				"\tThis initialization is done by the main object.");
		return SKIN_FAIL;
	}
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
	{
		skin_sensor_type_id i;
		for (i = 0; i < reader->tasks_count; ++i)
			if (reader->p_readers_data[i].enabled)
				return false;
		return true;
	}
	if (sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to is_paused", sensor_type);
		return SKIN_BAD_ID;
	}
	return !reader->p_readers_data[sensor_type].enabled;
}

int skin_reader_register_user(skin_reader *reader, skin_rt_mutex *mutex, skin_sensor_type_id sensor_type)
{
	skin_sensor_type_id i;
	bool fromlinux = false;

	if (sensor_type != SKIN_ALL_SENSOR_TYPES && sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to register_user", sensor_type);
		return SKIN_BAD_ID;
	}

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make register_user work with realtime functions");
		fromlinux = false;
		goto exit_fail;
	}

	if (!reader->p_registered_users_mutex || skin_rt_mutex_lock(reader->p_registered_users_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not register user due to failure in locking");
		goto exit_fail;
	}
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
		for (i = 0; i < reader->p_object->p_sensor_types_count; ++i)
			_list_add(&((internal_list **)reader->p_registered_users)[i], mutex);
	else
		_list_add(&((internal_list **)reader->p_registered_users)[sensor_type], mutex);
	skin_rt_mutex_unlock(reader->p_registered_users_mutex);

	if (fromlinux)
		skin_rt_stop();
	return SKIN_SUCCESS;
exit_fail:
	if (fromlinux)
		skin_rt_stop();
	return SKIN_FAIL;
}

int skin_reader_wait_read(skin_reader *reader, skin_rt_mutex *mutex, skin_rt_time wait_time, bool *must_stop)
{
	bool fromlinux;
	int ret;
	int ret_value = SKIN_SUCCESS;

	if (reader->p_object == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make thread calling wait_read a real-time thread!");
		return SKIN_FAIL;
	}
	if (wait_time == (skin_rt_time)-1)
	{
		while ((ret = skin_rt_mutex_timed_lock(mutex, SKIN_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
		{
			if (ret == SKIN_RT_FAIL)
			{
				ret_value = SKIN_FAIL;
				break;
			}
			if (must_stop && *must_stop)
			{
				ret_value = SKIN_LOCK_NOT_ACQUIRED;
				break;
			}
		}
	}
	else if (wait_time == 0)
	{
		ret = skin_rt_mutex_try_lock(mutex);
		if (ret == SKIN_RT_SUCCESS)
			ret_value = SKIN_SUCCESS;
		else if (ret == SKIN_RT_LOCK_NOT_ACQUIRED)
			ret_value = SKIN_LOCK_NOT_ACQUIRED;
		else
			ret_value = SKIN_FAIL;
	}
	else
	{
		ret = skin_rt_mutex_timed_lock(mutex, wait_time);
		if (ret == SKIN_RT_SUCCESS)
			ret_value = SKIN_SUCCESS;
		else if (ret == SKIN_RT_TIMEOUT)
			ret_value = SKIN_LOCK_TIMEOUT;
		else
			ret_value = SKIN_FAIL;
	}
	if (fromlinux)
		skin_rt_stop();
	return ret_value;
}

int skin_reader_unregister_user(skin_reader *reader, skin_rt_mutex *mutex, skin_sensor_type_id sensor_type)
{
	skin_sensor_type_id i;
	bool fromlinux = false;

	if (sensor_type != SKIN_ALL_SENSOR_TYPES && sensor_type >= reader->tasks_count)
	{
		skin_log_set_error(reader->p_object, "Invalid sensor type %u given to unregister_user", sensor_type);
		return SKIN_BAD_ID;
	}

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not make unregister_user work with realtime functions");
		fromlinux = false;
		goto exit_fail;
	}

	if (!reader->p_registered_users_mutex || skin_rt_mutex_lock(reader->p_registered_users_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(reader->p_object, "Could not unregister user due to failure in locking");
		return SKIN_FAIL;
	}
	if (sensor_type == SKIN_ALL_SENSOR_TYPES)
		for (i = 0; i < reader->p_object->p_sensor_types_count; ++i)
			_list_remove(&((internal_list **)reader->p_registered_users)[i], mutex);
	else
		_list_remove(&((internal_list **)reader->p_registered_users)[sensor_type], mutex);
	skin_rt_mutex_unlock(reader->p_registered_users_mutex);

	if (fromlinux)
		skin_rt_stop();
	return SKIN_SUCCESS;
exit_fail:
	if (fromlinux)
		skin_rt_stop();
	return SKIN_FAIL;
}

void skin_reader_enable_swap_skip_prediction(skin_reader *reader)
{
	reader->p_enable_swap_skip_prediction = true;
}

void skin_reader_disable_swap_skip_prediction(skin_reader *reader)
{
	reader->p_enable_swap_skip_prediction = false;
}
