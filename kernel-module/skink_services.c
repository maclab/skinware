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
#include <linux/semaphore.h>

#define					SKINK_MODULE_NAME				"skin_kernel"

#include "skink_internal.h"
#include "skink_common.h"
#include "skink_services_internal.h"

extern skink_misc_datum			*the_misc_data;
extern skink_sensor_layer		*the_sensor_layers;
extern skink_sensor			*the_sensors;
extern skink_sub_region			*the_sub_regions;
extern skink_region			*the_regions;
extern skink_sub_region_id		*the_sub_region_indices;
extern skink_region_id			*the_region_indices;
extern skink_sensor_id			*the_sensor_neighbors;
extern skink_state			the_state;

extern skin_rt_rwlock			*the_sync_locks;

static skink_service_info		_services[SKINK_MAX_SERVICES];
static skink_service_data		_service_thread_data[SKINK_MAX_SERVICES];
static skink_reader_data		_reader_thread_data[SKINK_MAX_SERVICES * SKINK_MAX_SENSOR_LAYERS];
											// data for reader thread of service i, layer j is in
											// reader_data[i * SKINK_SENSOR_LAYERS + j]
struct semaphore			the_services_mutex;

EXPORT_SYMBOL_GPL(skink_get_data_structures);
EXPORT_SYMBOL_GPL(skink_get_data_structures_nonblocking);
EXPORT_SYMBOL_GPL(skink_initialize_periodic_service);
EXPORT_SYMBOL_GPL(skink_initialize_sporadic_service);
EXPORT_SYMBOL_GPL(skink_acquire_layer);
EXPORT_SYMBOL_GPL(skink_request_read);
EXPORT_SYMBOL_GPL(skink_request_read_nonblocking);
EXPORT_SYMBOL_GPL(skink_await_read_response);
EXPORT_SYMBOL_GPL(skink_start_service);
EXPORT_SYMBOL_GPL(skink_pause_service);
EXPORT_SYMBOL_GPL(skink_resume_service);
EXPORT_SYMBOL_GPL(skink_stop_service);
EXPORT_SYMBOL_GPL(skink_lock_service);
EXPORT_SYMBOL_GPL(skink_unlock_service);
EXPORT_SYMBOL_GPL(skink_request_service);
EXPORT_SYMBOL_GPL(skink_request_service_nonblocking);
EXPORT_SYMBOL_GPL(skink_await_service_response);
EXPORT_SYMBOL_GPL(skink_connect_to_service);
EXPORT_SYMBOL_GPL(skink_disconnect_from_service);

/***************
 * Service API *
 ***************/

static void _signal_all_waiting_users_of_reader(skink_reader_data *task)
{
	int i;
	int tasks_waiting = 1;		/* one task responsible for waking this thread up */

	/* for each successful try lock, one user is waiting */
	while (skin_rt_sem_try_wait(&task->read_request) == SKIN_RT_SUCCESS)
		++tasks_waiting;

	/* Unblock as many users as there were requests program */
	for (i = 0; i < tasks_waiting; ++i)
		skin_rt_sem_post(&task->read_response);
}

static void _signal_all_waiting_users_of_service(skink_service_info *service)
{
	int i;
	int tasks_waiting = 1;		/* one task responsible for waking this service up */

	/* for each successful try lock, one user is waiting */
	while (skin_rt_sem_try_wait(&service->service_request) == SKIN_RT_SUCCESS)
		++tasks_waiting;

	/* Unblock as many users as there were requests */
	for (i = 0; i < tasks_waiting; ++i)
		skin_rt_sem_post(&service->service_response);
}

// Note: updates to reader in user-side library should be reflected here too (and vice versa)
static void _reader_thread(long int param)
{
	uint8_t			buffer		= 0;
	uint8_t			last_buffer	= 0;
	skin_rt_time		last_timestamp	= 0;
	skin_rt_time		new_timestamp	= 0;
	uint32_t		this_thread	= (uint32_t)param;
	skink_sensor_layer_id	this_layer	= (skink_sensor_layer_id)(this_thread % SKINK_SENSOR_LAYERS_COUNT);
	skink_reader_data	*task;
	skink_sensor_layer	*layer;
	skink_service_info	*service;
	skin_rt_rwlock		*sync_locks;
	uint8_t			number_of_buffers;
	bool			single_buffer;
	skin_rt_time		exec_time;
	skin_rt_time		passed_time;
	skin_rt_time		swap_protection_time = 1000;		// some value I know is definitely smaller than the time needed
	task			= _reader_thread_data + this_thread;
	if (task == NULL)
	{
		SKINK_RT_LOG("Internal error: reader thread created with wrong data");
		return;
	}
	service			= task->service;
	if (service == NULL)
	{
		SKINK_RT_LOG("Internal error: reader thread created with wrong data");
		return;
	}
	skin_rt_kernel_task_on_start();
	layer			= the_sensor_layers + this_layer;
	sync_locks		= the_sync_locks + SKINK_MAX_BUFFERS * this_layer;
	number_of_buffers	= layer->number_of_buffers;
	single_buffer		= number_of_buffers == 1;
	task->running		= true;
	task->paused		= false;
	SKINK_RT_LOG("Stage 6: Service reader thread for service %u, layer %u started", service->id, (unsigned int)this_layer);
	while (!task->must_stop)
	{
		bool do_sync = true;
		exec_time = skin_rt_get_exectime();
		// in the readers in user_space, here if layer->paused then it is threated as task->must_pause.  However, here
		// we still want the service reader callbacks to be called (so the service doesn't think the readers are dead)
		if (task->must_pause)
		{
			if (task->acq_mode == SKINK_ACQUISITION_PERIODIC
				|| (single_buffer && task->acq_mode != SKINK_ACQUISITION_SPORADIC))
				skin_rt_kernel_task_wait_period();
			else
			{
				if (task->acq_mode == SKINK_ACQUISITION_SPORADIC	// if this thread is paused, skip sporadic requests
					&& skin_rt_sem_try_wait(&task->read_request) == SKIN_RT_SUCCESS)
					_signal_all_waiting_users_of_reader(task);
				skin_rt_sleep(SKINK_MAX_THREAD_DELAY);
			}
			task->paused = true;
			continue;
		}
		task->paused = false;
		if (task->acq_mode == SKINK_ACQUISITION_SPORADIC)	// If in sporadic mode, wait for request
		{
			int ret = 0;
			while ((ret = skin_rt_sem_timed_wait(&task->read_request, SKINK_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
			{
				if (ret == SKIN_RT_FAIL)
				{
					SKINK_RT_LOG("Acquiring reader sporadic lock failed, quitting!");
					task->must_stop = true;
				}	// if not, then it must be timeout, which is fine, try again, but just test to see
					// if you are supposed to stop
				if (task->must_stop || task->must_pause)
					break;
			}
			if (task->must_stop || task->must_pause)
			{
				if (ret == SKIN_RT_SUCCESS)
					_signal_all_waiting_users_of_reader(task);
				continue;
			}
		}
		do_sync = !single_buffer && !layer->paused;		// in single_buffer, synchronization is skipped.
									// If the layer is paused also, synchronization is impossible
		if (do_sync)
		{
			buffer = layer->last_written_buffer;
			new_timestamp = layer->write_time[buffer];
			if ((buffer == last_buffer && last_timestamp >= new_timestamp)
				// Then already has the last updated version
				|| (skin_rt_get_time() + swap_protection_time > layer->next_predicted_swap))
				// Then it is guessed that I can't make the read in time, so sleep on the buffer being written
			{
				int ret = 0;
				buffer = layer->buffer_being_written;
				while ((ret = skin_rt_rwlock_timed_read_lock(&sync_locks[buffer], task->period)) != SKIN_RT_SUCCESS)
				{
					if (ret == SKIN_RT_FAIL)
					{
						SKINK_RT_LOG("Acquiring read lock failed, quitting!");
						task->must_stop = true;
					}	// if not, then it must be timeout, which is fine, try again, but just test to see
						// if task is supposed to stop
					if (task->must_stop || task->must_pause)
						break;
				}
				if (task->must_stop || task->must_pause)
				{
					if (ret == SKIN_RT_SUCCESS)
						skin_rt_rwlock_read_unlock(&sync_locks[buffer]);
					continue;
				}
				new_timestamp = layer->write_time[buffer];
			}
			else if (skin_rt_rwlock_try_read_lock(&sync_locks[buffer]) == SKIN_RT_LOCK_NOT_ACQUIRED)
				// Then lock could not be acquired, which means from a few lines ago till now, the kernel finished updating buffers
				// and is switched to this buffer
			{
				if (task->acq_mode == SKINK_ACQUISITION_SPORADIC)
					skin_rt_sem_post(&task->read_request);	// allow reallocation of lock
				continue;
			}
			// else	// then the lock is successfully acquired
			last_buffer = buffer;
			last_timestamp = new_timestamp;
		}
		// else // else, there is nothing to do, just read the buffer without locking
		passed_time = skin_rt_get_time();
		task->acquire(layer, buffer);
		passed_time = skin_rt_get_time() - passed_time;
		if (do_sync)
			skin_rt_rwlock_read_unlock(&sync_locks[buffer]);
		if (task->acq_mode == SKINK_ACQUISITION_SPORADIC)
			_signal_all_waiting_users_of_reader(task);
		else if (single_buffer || task->acq_mode == SKINK_ACQUISITION_PERIODIC)
				// If single buffer and not sporadic, then it's considered periodic
				skin_rt_kernel_task_wait_period();

		exec_time = skin_rt_get_time() - exec_time;
		if (exec_time == 0)	// there was no context switches, so exectime is not updated.  Use passed_time
			exec_time = passed_time;	// TODO: make sure the above statement is correct
		if (exec_time > swap_protection_time)
			swap_protection_time = (swap_protection_time * 7 + passed_time) >> 3;	// converge to exec time with a factor of 1/8
		// TODO: provide statistics to the service similar to user space
	}
	task->running = false;
	skin_rt_kernel_task_on_stop();
	SKINK_RT_LOG("Stage %s: Service reader thread for service %u, layer %u stopped",
				(the_state == SKINK_STATE_EXITING?"7":
				the_state == SKINK_STATE_OPERATIONAL?"6":"?"), service->id, (unsigned int)this_layer);
}

static void _service_thread(long int param)
{
	uint32_t			this_thread		= (uint32_t)param;
	skink_service_data		*task;
	skink_service_info		*service;
	void				*service_memory;
	if (this_thread >= SKINK_MAX_SERVICES)
	{
		SKINK_RT_LOG("Internal error: service thread created with wrong data");
		return;
	}
	task		= _service_thread_data + this_thread;
	if (task == NULL)
	{
		SKINK_RT_LOG("Internal error: service thread created with wrong data");
		return;
	}
	service		= task->service;
	if (service == NULL)
	{
		SKINK_RT_LOG("Internal error: service thread created with wrong data");
		return;
	}
	service_memory	= service->memory;
	if (service_memory == NULL)
	{
		SKINK_RT_LOG("Internal error: service thread created with wrong data");
		return;
	}
	skin_rt_kernel_task_on_start();			// if using rtai, this function is empty
	SKINK_SERVICE_TIMESTAMP(service_memory)	= 0;
	task->running				= true;
	task->paused				= false;
	SKINK_RT_LOG("Stage 6: Service thread for service %u started", this_thread);
	while (!task->must_stop)
	{
		if (task->must_pause)
		{
			if (SKINK_SERVICE_IS_PERIODIC(service_memory))
				skin_rt_kernel_task_wait_period();
			else
			{
				if (skin_rt_sem_try_wait(&service->service_request) == SKIN_RT_SUCCESS)
					_signal_all_waiting_users_of_service(service);
				skin_rt_sleep(SKINK_MAX_THREAD_DELAY);
			}
			task->paused = true;
			continue;
		}
		task->paused = false;
		if (SKINK_SERVICE_IS_SPORADIC(service_memory))	// If in sporadic mode, wait for request
		{
			int ret = 0;
			while ((ret = skin_rt_sem_timed_wait(&service->service_request, SKINK_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
			{
				if (ret == SKIN_RT_FAIL)
				{
					SKINK_RT_LOG("Acquiring service sporadic lock failed, quitting!");
					task->must_stop = true;
				}	// if not, then it must be timeout, which is fine, try again, but just test to see
					// if you are supposed to stop
				if (task->must_stop || task->must_pause)
					break;
			}
			if (task->must_stop || task->must_pause)
			{
				if (ret == SKIN_RT_SUCCESS)
					_signal_all_waiting_users_of_service(service);
				continue;
			}
		}
		else		// then task is periodic
		{
			int ret = 0;
			while ((ret = skin_rt_rwlock_timed_write_lock(&service->rwlock, SKINK_SERVICE_PERIOD(service_memory))) != SKIN_RT_SUCCESS)
			{
				if (ret == SKIN_RT_FAIL)
				{
					SKINK_RT_LOG("Acquiring service periodic lock failed, quitting!");
					task->must_stop = true;
				}	// if not, then it must be timeout, which is fine, try again, but just test to see
					// if you are supposed to stop
				if (task->must_stop || task->must_pause)
					break;
			}
			if (task->must_stop || task->must_pause)
			{
				if (ret == SKIN_RT_SUCCESS)
					skin_rt_rwlock_write_unlock(&service->rwlock);
				continue;
			}
		}
		task->function(service_memory, task->user_data);
		SKINK_SERVICE_TIMESTAMP(service_memory) = skin_rt_get_time();
		if (SKINK_SERVICE_IS_SPORADIC(service_memory))
			_signal_all_waiting_users_of_service(service);
		else
		{
			skin_rt_rwlock_write_unlock(&service->rwlock);
			skin_rt_kernel_task_wait_period();
		}
	}
	task->running = false;
	skin_rt_kernel_task_on_stop();
	SKINK_RT_LOG("Stage %s: Service thread for service %u stopped",
				(the_state == SKINK_STATE_EXITING?"7":
				the_state == SKINK_STATE_OPERATIONAL?"6":"?"), this_thread);
}

static int _get_data_structures(skink_data_structures *data_structures)
{
	if (data_structures == NULL)
		return SKINK_FAIL;
	data_structures->sensor_layers			= the_sensor_layers;
	data_structures->sub_regions			= the_sub_regions;
	data_structures->regions			= the_regions;
	data_structures->sub_region_indices		= the_sub_region_indices;
	data_structures->region_indices			= the_region_indices;
	data_structures->sensor_neighbors		= the_sensor_neighbors;
	data_structures->sensors			= the_sensors;
	data_structures->sensor_layers_count		= SKINK_SENSOR_LAYERS_COUNT;
	data_structures->sub_regions_count		= SKINK_SUB_REGIONS_COUNT;
	data_structures->regions_count			= SKINK_REGIONS_COUNT;
	data_structures->sub_region_indices_count	= SKINK_SUB_REGION_INDICES_COUNT;
	data_structures->region_indices_count		= SKINK_REGION_INDICES_COUNT;
	data_structures->sensor_neighbors_count		= SKINK_SENSOR_NEIGHBORS_COUNT;
	return SKINK_SUCCESS;
}

int skink_get_data_structures(skink_data_structures *data_structures)
{
	while (the_state != SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED && the_state != SKINK_STATE_EXITING)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (the_state == SKINK_STATE_FAILED || the_state == SKINK_STATE_EXITING)
		return SKINK_FAIL;
	return _get_data_structures(data_structures);
}

int skink_get_data_structures_nonblocking(skink_data_structures *data_structures)
{
	if (the_state == SKINK_STATE_FAILED || the_state == SKINK_STATE_EXITING)
		return SKINK_FAIL;
	if (the_state != SKINK_STATE_OPERATIONAL)
		return SKINK_TOO_EARLY;
	return _get_data_structures(data_structures);
}

static inline void _custom_strncpy(char *to, const char *from, unsigned int max_length)		// max_length does not include NUL, but `to` should have space for it
{
	--max_length;
	strncpy(to, from, max_length);
	to[max_length] = '\0';
}

#define SKINK_SHARED_MEMORY_ALLOC(result, name, size)\
	do {\
		bool _is_contiguous;\
		result = skin_rt_shared_memory_alloc(name, size, &_is_contiguous);\
		if (result == NULL)\
			SKINK_LOG("Stage 6: Could not acquire memory for "#result);\
		else if (!_is_contiguous)\
			SKINK_LOG("Stage 6: Warning - Using discontiguous memory");	/* Not really a problem.  But if you see this,*/\
											/* you are most likely almost running out of memory */\
	} while (0)

static void _service_cleanup(int sid)
{
	skin_rt_mutex_delete(&_services[sid].mutex);
	if (_services[sid].memory_name[0] != '\0')
	{
		skin_rt_shared_memory_free(_services[sid].memory_name);
		_services[sid].memory = NULL;
		_services[sid].memory_name[0] = '\0';
	}
	if (_services[sid].request_name[0] != '\0')
	{
		skin_rt_unshare_semaphore(_services[sid].request_name);
		skin_rt_sem_delete(&_services[sid].service_request);
		_services[sid].request_name[0] = '\0';
	}
	if (_services[sid].response_name[0] != '\0')
	{
		skin_rt_unshare_semaphore(_services[sid].response_name);
		skin_rt_sem_delete(&_services[sid].service_response);
		_services[sid].response_name[0] = '\0';
	}
	if (_services[sid].rwlock_name[0] != '\0')
	{
		skin_rt_unshare_rwlock(_services[sid].rwlock_name);
		skin_rt_rwlock_delete(&_services[sid].rwlock);
		_services[sid].rwlock_name[0] = '\0';
	}
	_services[sid].state = SKINK_SERVICE_STATE_UNUSED;
}

int _initialize_service_common(const char *service_name, size_t elem_size, size_t count)
{
	uint32_t i;
	int sid = -1;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Initialize service interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	for (i = 0; i < SKINK_MAX_SERVICES; ++i)
		if (_services[i].state == SKINK_SERVICE_STATE_UNUSED)
		{
			_services[i].state = SKINK_SERVICE_STATE_INITIALIZING;
			sid = i;
			break;
		}
	up(&the_services_mutex);
	if (sid < 0)
	{
		SKINK_LOG("Too many services");
		return SKINK_TOO_LATE;
	}
	if (skin_rt_mutex_init(&_services[sid].mutex, 1) == NULL)
	{
		_services[sid].state = SKINK_SERVICE_STATE_UNUSED;
		return SKINK_FAIL;
	}
	SKINK_SHARED_MEMORY_ALLOC(_services[sid].memory, service_name, elem_size * count + SKINK_SERVICE_HEADER_SIZE * sizeof(uint64_t));
	if (_services[sid].memory == NULL)
	{
		_service_cleanup(sid);
		return SKINK_NO_MEM;
	}
	_custom_strncpy(_services[sid].memory_name, service_name, SKIN_RT_MAX_NAME_LENGTH + 1);
	_services[sid].memory = ((uint64_t *)_services[sid].memory) + SKINK_SERVICE_HEADER_SIZE;
	SKINK_SERVICE_TIMESTAMP(_services[sid].memory)		= 0;
	SKINK_SERVICE_STATUS(_services[sid].memory)		= SKINK_SERVICE_DEAD;
	SKINK_SERVICE_MEM_SIZE(_services[sid].memory)		= elem_size * count;
	SKINK_SERVICE_RESULT_COUNT(_services[sid].memory)	= count;
	return sid;
}

int skink_initialize_periodic_service(const char *service_name, size_t elem_size, size_t count, const char *rwl_name, skin_rt_time period)
{
	int sid = -1;
	if (service_name == NULL || service_name[0] == '\0' || rwl_name == NULL || rwl_name[0] == '\0' || elem_size == 0 || count == 0)
		return SKINK_BAD_DATA;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (!skin_rt_name_available(service_name) || !skin_rt_name_available(rwl_name))
		return SKINK_BAD_NAME;
	if (skin_rt_is_rt_context())		// cannot be called from real-time context
		return SKINK_BAD_CONTEXT;
	sid = _initialize_service_common(service_name, elem_size, count);
	if (sid < 0)
		return sid;
	if (skin_rt_rwlock_init_and_share(&_services[sid].rwlock, rwl_name) == NULL)
	{
		_service_cleanup(sid);
		return SKINK_FAIL;
	}
	_custom_strncpy(_services[sid].rwlock_name, rwl_name, SKIN_RT_MAX_NAME_LENGTH + 1);
	_services[sid].state				= SKINK_SERVICE_STATE_INITIALIZED;
	SKINK_SERVICE_PERIOD(_services[sid].memory)	= period;
	SKINK_SERVICE_MODE(_services[sid].memory)	= SKINK_SERVICE_PERIODIC;
	return sid;
}

int skink_initialize_sporadic_service(const char *service_name, size_t elem_size, size_t count, const char *request_name, const char *response_name)
{
	int sid = -1;
	if (service_name == NULL || service_name[0] == '\0' || request_name == NULL || request_name[0] == '\0'
		|| response_name == NULL || response_name[0] == '\0' || elem_size == 0 || count == 0)
		return SKINK_BAD_DATA;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (!skin_rt_name_available(service_name) || !skin_rt_name_available(request_name) || !skin_rt_name_available(response_name))
		return SKINK_BAD_NAME;
	if (skin_rt_is_rt_context())		// cannot be called from real-time context
		return SKINK_BAD_CONTEXT;
	sid = _initialize_service_common(service_name, elem_size, count);
	if (sid < 0)
		return sid;
	if (skin_rt_sem_init_and_share(&_services[sid].service_request, 0, request_name) == NULL)
	{
		_service_cleanup(sid);
		return SKINK_FAIL;
	}
	_custom_strncpy(_services[sid].request_name, request_name, SKIN_RT_MAX_NAME_LENGTH + 1);
	if (skin_rt_sem_init_and_share(&_services[sid].service_response, 0, response_name) == NULL)
	{
		_service_cleanup(sid);
		return SKINK_FAIL;
	}
	_custom_strncpy(_services[sid].response_name, response_name, SKIN_RT_MAX_NAME_LENGTH + 1);
	_services[sid].state				= SKINK_SERVICE_STATE_INITIALIZED;
	SKINK_SERVICE_MODE(_services[sid].memory)	= SKINK_SERVICE_SPORADIC;
	return sid;
}

static int _acquire_layer(unsigned int service_id, skink_sensor_layer_id layer, skink_service_acquire acquire,
		skink_acquisition_type mode, skin_rt_time period)
{
	int data_pos;

	data_pos = service_id * SKINK_SENSOR_LAYERS_COUNT + layer;
	if (!_reader_thread_data[data_pos].read_sems_created)
	{
		if (skin_rt_sem_init(&_reader_thread_data[data_pos].read_request, 0) == NULL)
			return SKINK_FAIL;
		if (skin_rt_sem_init(&_reader_thread_data[data_pos].read_response, 0) == NULL)
		{
			skin_rt_sem_delete(&_reader_thread_data[data_pos].read_request);
			return SKINK_FAIL;
		}
		_reader_thread_data[data_pos].read_sems_created = true;
	}
	if (1000000000 / the_sensor_layers[layer].acquisition_rate > period)	// can't get faster than kernel!
		period = 1000000000 / the_sensor_layers[layer].acquisition_rate;
	_reader_thread_data[data_pos].acquire		= acquire;
	_reader_thread_data[data_pos].acq_mode		= mode;
	if (mode == SKINK_ACQUISITION_PERIODIC)
		_reader_thread_data[data_pos].period	= period;
	else	// if not periodic, the task period is helpful (and is used) to sleep for a sufficient amount until new data has arrived
		_reader_thread_data[data_pos].period	= 1000000000 / the_sensor_layers[layer].acquisition_rate;
	return SKINK_SUCCESS;
}

int skink_acquire_layer(unsigned int service_id, skink_sensor_layer_id layer, skink_service_acquire acquire,
		skink_acquisition_type mode, skin_rt_time period)
{
	int ret;
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	if (acquire == NULL || (layer != SKINK_ALL_LAYERS && layer >= SKINK_SENSOR_LAYERS_COUNT)
		|| (mode != SKINK_ACQUISITION_ASAP && mode != SKINK_ACQUISITION_PERIODIC && mode != SKINK_ACQUISITION_SPORADIC))
		return SKINK_BAD_DATA;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Service acquire registration interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_UNUSED)
	{
		up(&the_services_mutex);
		return SKINK_BAD_ID;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_BAD ||
		_services[service_id].state == SKINK_SERVICE_STATE_WORKING)
	{
		up(&the_services_mutex);
		return SKINK_FAIL;
	}
	if (layer == SKINK_ALL_LAYERS)
	{
		bool failure = false;
		bool success = false;
		skink_sensor_layer_id i;

		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		{
			if (_acquire_layer(service_id, i, acquire, mode, period) == SKINK_SUCCESS)
				success = true;
			else
				failure = true;
		}
		if (success && !failure)
			ret = SKINK_SUCCESS;
		else if (failure && !success)
			ret = SKINK_FAIL;
		else
			ret = SKINK_PARTIAL;
	}
	else
		ret = _acquire_layer(service_id, layer, acquire, mode, period);
	up(&the_services_mutex);
	return ret;
}

static int _check_read_request(unsigned int service_id)
{
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)	/* STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it */
		return SKINK_TOO_LATE;
	if (!skin_rt_is_rt_context())
		return SKINK_BAD_CONTEXT;
	if (_services[service_id].state != SKINK_SERVICE_STATE_WORKING)
		return SKINK_FAIL;
	return SKINK_SUCCESS;
}

static int _await_read_response_common(unsigned int service_id, skink_reader_data *rd, bool *must_stop)
{
	int ret = 0;
	while ((ret = skin_rt_sem_timed_wait(&rd->read_response, SKINK_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
											// Wait for the reader thread to finish reading
	{
		if (ret == SKIN_RT_FAIL)
		{
			SKINK_RT_LOG("Acquiring sporadic lock failed.  Ignoring read request!");
			return SKINK_FAIL;
		}
		if (must_stop && *must_stop)
		{
			SKINK_RT_LOG("Warning: The service is asked to stop.  Ignoring read request!");
			break;
		}
		if (_service_thread_data[service_id].must_stop)
		{
			SKINK_RT_LOG("Warning: The reader threads are asked to stop.  Ignoring read request!");
			break;
		}
	}
	return SKINK_SUCCESS;
}

int skink_request_read(unsigned int service_id, skink_sensor_layer_id layer, bool *must_stop)
{
	skink_reader_data *rd;
	int ret;
	ret = _check_read_request(service_id);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (layer == SKINK_ALL_LAYERS)
	{
		bool requested[SKINK_MAX_SENSOR_LAYERS] = {0};
		int should_request = 0;
		int actual_request = 0;
		skink_sensor_layer_id i;
		rd = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			if (rd[i].acq_mode == SKINK_ACQUISITION_SPORADIC)
			{
				++should_request;
				if (skink_request_read_nonblocking(service_id, i) == SKINK_SUCCESS)
				{
					requested[i] = true;
					++actual_request;
				}
			}
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			if (requested[i])
				if (skink_await_read_response(service_id, i, must_stop) != SKINK_SUCCESS)
					--actual_request;
		if (actual_request == 0 && should_request > 0)
			return SKINK_FAIL;
		if (actual_request < should_request)
			return SKINK_PARTIAL;
		return SKINK_SUCCESS;
	}
	if (layer >= SKINK_SENSOR_LAYERS_COUNT)
		return SKINK_BAD_ID;
	rd = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT + layer;
	if (!rd->read_sems_created)
		return SKINK_FAIL;
	skin_rt_sem_post(&rd->read_request);
	return _await_read_response_common(service_id, rd, must_stop);
}

int skink_request_read_nonblocking(unsigned int service_id, skink_sensor_layer_id layer)
{
	skink_reader_data *rd;
	int ret;
	ret = _check_read_request(service_id);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (layer == SKINK_ALL_LAYERS)
	{
		int should_request = 0;
		int actual_request = 0;
		skink_sensor_layer_id i;
		rd = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			if (rd[i].acq_mode == SKINK_ACQUISITION_SPORADIC)
			{
				++should_request;
				if (skink_request_read_nonblocking(service_id, i) == SKINK_SUCCESS)
					++actual_request;
				else if (rd[i].read_sems_created)
					skin_rt_sem_post(&rd[i].read_response);	// a request could not be made, so signal the response so users won't be blocked
			}
		if (actual_request == 0 && should_request > 0)
			return SKINK_FAIL;
		if (actual_request < should_request)
			return SKINK_PARTIAL;
		return SKINK_SUCCESS;
	}
	if (layer >= SKINK_SENSOR_LAYERS_COUNT)
		return SKINK_BAD_ID;
	rd = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT + layer;
	if (!rd->read_sems_created)
		return SKINK_FAIL;
	skin_rt_sem_post(&rd->read_request);
	return SKINK_SUCCESS;
}

int skink_await_read_response(unsigned int service_id, skink_sensor_layer_id layer, bool *must_stop)
{
	skink_reader_data *rd;
	int ret;
	ret = _check_read_request(service_id);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (layer == SKINK_ALL_LAYERS)
	{
		int should_respond = 0;
		int actual_respond = 0;
		skink_sensor_layer_id i;
		rd = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			if (rd[i].acq_mode == SKINK_ACQUISITION_SPORADIC)
			{
				++should_respond;
				if (skink_await_read_response(service_id, i, must_stop) == SKINK_SUCCESS)
					++actual_respond;
			}
		if (actual_respond == 0 && should_respond > 0)
			return SKINK_FAIL;
		if (actual_respond < should_respond)
			return SKINK_PARTIAL;
		return SKINK_SUCCESS;
	}
	if (layer >= SKINK_SENSOR_LAYERS_COUNT)
		return SKINK_BAD_ID;
	rd = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT + layer;
	if (!rd->read_sems_created)
		return SKINK_FAIL;
	return _await_read_response_common(service_id, rd, must_stop);
}

static void _remove_tasks(unsigned int service_id)
{
	uint32_t		i;
	skink_service_info	*si;
	skink_reader_data	*this_service_reader_data;
	bool			all_quit;
	struct timespec		start_, now_;
	uint64_t		start, now;
	uint64_t		max_wait_time = 0;
	uint64_t		max_wait_time_seconds = 0;
	uint32_t		max_wait_time_milliseconds = 0;
	if (service_id >= SKINK_MAX_SERVICES)
		return;
	si = _services + service_id;
	if (si->state == SKINK_SERVICE_STATE_UNUSED)
		return;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Stopping service tasks interrupted by signal...aborting");
		return;
	}
	this_service_reader_data = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT;
	if (si->memory == NULL)
	{
		up(&the_services_mutex);
		return;
	}
	SKINK_SERVICE_STATUS(si->memory) = SKINK_SERVICE_DEAD;
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)			// Tell the threads they should stop
	{
		this_service_reader_data[i].must_stop = true;
		if (this_service_reader_data[i].created && this_service_reader_data[i].acq_mode == SKINK_ACQUISITION_PERIODIC
			&& this_service_reader_data[i].period > (skin_rt_time)max_wait_time)
			max_wait_time = this_service_reader_data[i].period;
	}
	_service_thread_data[service_id].must_stop = true;
	if (SKINK_SERVICE_IS_PERIODIC(si->memory) && SKINK_SERVICE_PERIOD(si->memory) > (skin_rt_time)max_wait_time)
		max_wait_time = SKINK_SERVICE_PERIOD(si->memory);
	max_wait_time += 1500000000ull;	// Give the threads max(their periods)+1.5 seconds to stop (1.5 seconds if they are not periodic)
	all_quit = false;
	getrawmonotonic(&start_);
	start = start_.tv_sec * 1000000000ull + start_.tv_nsec;
	do
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
		all_quit = true;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			if (this_service_reader_data[i].created
				&& this_service_reader_data[i].running)
			{
				all_quit = false;
				break;
			}
		if (_service_thread_data[service_id].created
			&&_service_thread_data[service_id].running)
			all_quit = false;
		getrawmonotonic(&now_);
		now = now_.tv_sec * 1000000000ull + now_.tv_nsec;
	} while (!all_quit && now - start < max_wait_time);
	// Delete the tasks, even if they didn't stop
#if BITS_PER_LONG != 64
	max_wait_time_seconds = max_wait_time;
	max_wait_time_milliseconds = do_div(max_wait_time_seconds, 1000000000);	// changes max_wait_time_seconds and returns %1000000000
#else
	max_wait_time_seconds = max_wait_time / 1000000000;
	max_wait_time_milliseconds = max_wait_time % 1000000000;
#endif
	max_wait_time_milliseconds /= 1000000;
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		if (!this_service_reader_data[i].created)
			continue;
		if (this_service_reader_data[i].running)
			SKINK_LOG("Stage %s: Waited for service %u reader thread %u to exit for %llu.%03us, but it didn't stop.  Killing it",
					(the_state == SKINK_STATE_EXITING?"7":
					the_state == SKINK_STATE_OPERATIONAL?"6":"?"), service_id, (unsigned int)i,
					max_wait_time_seconds, max_wait_time_milliseconds);
		skin_rt_kernel_task_delete(&this_service_reader_data[i].task);
		this_service_reader_data[i].acquire = NULL;
		this_service_reader_data[i].created = false;
	}
	if (_service_thread_data[service_id].created
		&& _service_thread_data[service_id].running)
		SKINK_LOG("Stage %s: Waited for service %u to exit for %llu.%03us, but it didn't stop.  Killing it",
				(the_state == SKINK_STATE_EXITING?"7":
				the_state == SKINK_STATE_OPERATIONAL?"6":"?"), service_id,
				max_wait_time_seconds, max_wait_time_milliseconds);
	skin_rt_kernel_task_delete(&_service_thread_data[service_id].task);
	_service_thread_data[service_id].function	= NULL;
	_service_thread_data[service_id].created	= false;
	si->state					= SKINK_SERVICE_STATE_INITIALIZED;
	up(&the_services_mutex);
}

int skink_start_service(unsigned int service_id, skink_service_function function, void *data)
{
	uint32_t	i;
	int		num_success	= 0;
	int		num_readers	= 0;
	skin_rt_time	start_time;
#if BITS_PER_LONG != 64
	skin_rt_time	temp;
#endif
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	if (function == NULL)
		return SKINK_BAD_DATA;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Service start interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_UNUSED)
	{
		up(&the_services_mutex);
		return SKINK_BAD_ID;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_BAD ||
		_services[service_id].state == SKINK_SERVICE_STATE_WORKING)
	{
		up(&the_services_mutex);
		return SKINK_FAIL;
	}
	// cteate the reader threads
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		long int data_pos = service_id * SKINK_SENSOR_LAYERS_COUNT + i;
		if (_reader_thread_data[data_pos].acquire == NULL)
			continue;
		++num_readers;
		_reader_thread_data[data_pos].must_stop		= false;
		_reader_thread_data[data_pos].must_pause	= false;
		_reader_thread_data[data_pos].running		= false;
		_reader_thread_data[data_pos].layer		= i;
		_reader_thread_data[data_pos].service		= _services + service_id;
		if (skin_rt_kernel_task_init(&_reader_thread_data[data_pos].task, _reader_thread,
				data_pos, SKINK_TASK_STACK_SIZE, SKINK_CONVERTED_READER_PRIORITY) != SKIN_RT_SUCCESS)
			SKINK_LOG("Stage 3: Could not create reader thread (layer: %u) for service %u", i, service_id);
		else
			_reader_thread_data[data_pos].created = true;
	}
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		long int data_pos = service_id * SKINK_SENSOR_LAYERS_COUNT + i;
		if (_reader_thread_data[data_pos].acquire == NULL
			|| !_reader_thread_data[data_pos].created)
			continue;
		if (_reader_thread_data[data_pos].acq_mode == SKINK_ACQUISITION_PERIODIC)
		{
			skin_rt_time period = _reader_thread_data[data_pos].period;
			start_time = skin_rt_get_time() + 2 * period;
#if BITS_PER_LONG != 64
			temp = start_time;
			start_time -= do_div(temp, period);	// changes temp and returns temp % period
			temp = period;
			do_div(temp, SKINK_SENSOR_LAYERS_COUNT);
			start_time += temp * i;
#else
			start_time -= start_time % period;	// Align the start time with multiples of period.  Writes also do it, so kind of already synchronized!
			start_time += period / SKINK_SENSOR_LAYERS_COUNT * i;	// To make the equal period tasks have different start times
#endif
			start_time += 5000;			// This is to start service readers after writers, but before user-space readers
			if (skin_rt_kernel_task_make_periodic(&_reader_thread_data[data_pos].task, start_time, period) != SKIN_RT_SUCCESS)
				SKINK_LOG("Stage 3: Could not make reader (thread: %u, service: %u) periodic", i, service_id);
			else
				++num_success;
		}
		else
		{
			if (skin_rt_kernel_task_resume(&_reader_thread_data[data_pos].task) != SKIN_RT_SUCCESS)
			{
				SKINK_LOG("Stage 3: Could not start reader (thread: %u, service: %u)", i, service_id);
				if (_reader_thread_data[data_pos].read_sems_created)	// delete its semaphores so skink_request_read could not be called
				{
					skin_rt_sem_delete(&_reader_thread_data[data_pos].read_request);
					skin_rt_sem_delete(&_reader_thread_data[data_pos].read_response);
				}
				_reader_thread_data[data_pos].read_sems_created = false;
			}
			else
				++num_success;
		}
	}
	if (num_success == 0 && num_readers != 0)	// it is legitimate to have no readers (for example a service overlooking other services)
	{
		up(&the_services_mutex);
		_remove_tasks(service_id);
		return SKINK_FAIL;
	}
	// create the service thread itself
	_service_thread_data[service_id].must_stop	= false;
	_service_thread_data[service_id].must_pause	= false;
	_service_thread_data[service_id].running	= false;
	_service_thread_data[service_id].function	= function;
	_service_thread_data[service_id].user_data	= data;
	_service_thread_data[service_id].service	= _services + service_id;
	if (skin_rt_kernel_task_init(&_service_thread_data[service_id].task, _service_thread,
			service_id, SKINK_TASK_STACK_SIZE, SKINK_CONVERTED_SERVICE_PRIORITY) != SKIN_RT_SUCCESS)
	{
		SKINK_LOG("Stage 3: Could not create thread for service %u", service_id);
		up(&the_services_mutex);
		_remove_tasks(service_id);
		return SKINK_FAIL;
	}
	if (SKINK_SERVICE_IS_PERIODIC(_services[service_id].memory))
	{
		skin_rt_time period = SKINK_SERVICE_PERIOD(_services[service_id].memory);
		start_time = skin_rt_get_time() + 2 * period;
#if BITS_PER_LONG != 64
		temp = start_time;
		start_time -= do_div(temp, period);	// changes temp and returns temp % period
#else
		start_time -= start_time % period;	// Align the start time with multiples of period.  Users will also do it, so kind of already synchronized!
#endif
		if (skin_rt_kernel_task_make_periodic(&_service_thread_data[service_id].task, start_time, period) != SKIN_RT_SUCCESS)
		{
			SKINK_LOG("Stage 3: Could not make service (id: %u) periodic", service_id);
			up(&the_services_mutex);
			_remove_tasks(service_id);
			return SKINK_FAIL;
		}
	}
	else
		if (skin_rt_kernel_task_resume(&_service_thread_data[service_id].task) != SKIN_RT_SUCCESS)
		{
			SKINK_LOG("Stage 3: Could not start service (id: %u)", service_id);
			up(&the_services_mutex);
			_remove_tasks(service_id);
			return SKINK_FAIL;
		}
	_service_thread_data[service_id].created		= true;
	_services[service_id].state				= SKINK_SERVICE_STATE_WORKING;
	SKINK_SERVICE_STATUS(_services[service_id].memory)	= SKINK_SERVICE_ALIVE;
	up(&the_services_mutex);
	if (num_success < num_readers)
		return SKINK_PARTIAL;
	return SKINK_SUCCESS;
}

int skink_pause_service(unsigned int service_id)
{
	uint32_t i;
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Pause start interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_UNUSED)
	{
		up(&the_services_mutex);
		return SKINK_BAD_ID;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_BAD)
	{
		up(&the_services_mutex);
		return SKINK_FAIL;
	}
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		_reader_thread_data[service_id * SKINK_SENSOR_LAYERS_COUNT + i].must_pause = true;
	_service_thread_data[service_id].must_pause = true;
	up(&the_services_mutex);
	return SKINK_SUCCESS;
}

int skink_resume_service(unsigned int service_id)
{
	uint32_t i;
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Pause start interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_UNUSED)
	{
		up(&the_services_mutex);
		return SKINK_BAD_ID;
	}
	if (_services[service_id].state == SKINK_SERVICE_STATE_BAD)
	{
		up(&the_services_mutex);
		return SKINK_FAIL;
	}
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		_reader_thread_data[service_id * SKINK_SENSOR_LAYERS_COUNT + i].must_pause = false;
	_service_thread_data[service_id].must_pause = false;
	up(&the_services_mutex);
	return SKINK_SUCCESS;
}

int skink_stop_service(unsigned int service_id)
{
	uint32_t i;
	skink_service_info *si;
	skink_reader_data *this_service_reader_data;
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	si = _services + service_id;
	if (si->state == SKINK_SERVICE_STATE_UNUSED)
		return SKINK_SUCCESS;
	_remove_tasks(service_id);
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Stop service interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	this_service_reader_data = _reader_thread_data + service_id * SKINK_SENSOR_LAYERS_COUNT;
	if (si->memory != NULL)
		skin_rt_shared_memory_free(si->memory_name);
	if (si->state != SKINK_SERVICE_STATE_BAD)
		skin_rt_mutex_delete(&si->mutex);
	if (si->request_name[0] != '\0')
	{
		skin_rt_unshare_semaphore(si->request_name);
		skin_rt_sem_delete(&si->service_request);
	}
	if (si->response_name[0] != '\0')
	{
		skin_rt_unshare_semaphore(si->response_name);
		skin_rt_sem_delete(&si->service_response);
	}
	if (si->rwlock_name[0] != '\0')
	{
		skin_rt_unshare_rwlock(si->rwlock_name);
		skin_rt_rwlock_delete(&si->rwlock);
	}
	si->memory						= NULL;
	si->memory_name[0]					= '\0';
	si->request_name[0]					= '\0';
	si->response_name[0]					= '\0';
	si->rwlock_name[0]					= '\0';
	_service_thread_data[service_id].created		= false;
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		this_service_reader_data[i].acquire		= NULL;
		this_service_reader_data[i].acq_mode		= SKINK_ACQUISITION_ASAP;
		this_service_reader_data[i].created		= false;
		if (this_service_reader_data->read_sems_created)
		{
			skin_rt_sem_delete(&this_service_reader_data->read_request);
			skin_rt_sem_delete(&this_service_reader_data->read_response);
		}
		this_service_reader_data->read_sems_created	= false;
	}
	si->state						= SKINK_SERVICE_STATE_UNUSED;
	up(&the_services_mutex);
	return SKINK_SUCCESS;
}

void skink_initialize_services(void)
{
	uint32_t i, size;
	for (i = 0; i < SKINK_MAX_SERVICES; ++i)
	{
		skink_service_info *si = _services + i;
		si->id						= i;
		si->state					= SKINK_SERVICE_STATE_UNUSED;
		si->memory					= NULL;
		si->memory_name[0]				= '\0';
		si->request_name[0]				= '\0';
		si->response_name[0]				= '\0';
		si->rwlock_name[0]				= '\0';
		_service_thread_data[i].function		= NULL;
		_service_thread_data[i].created			= false;
	}
	for (i = 0, size = SKINK_MAX_SERVICES * SKINK_MAX_SENSOR_LAYERS; i < size; ++i)
	{
		_reader_thread_data[i].acquire			= NULL;
		_reader_thread_data[i].acq_mode			= SKINK_ACQUISITION_ASAP;
		_reader_thread_data[i].created			= false;
		_reader_thread_data[i].read_sems_created	= false;
	}
}

void skink_free_services(void)
{
	uint32_t i;
	for (i = 0; i < SKINK_MAX_SERVICES; ++i)
		skink_stop_service(i);
}

/***********************
 * Service Manager API *
 ***********************/

static int _check_service_request(skink_service *service)
{
	uint32_t service_id;
	if (service == NULL)
		return SKINK_BAD_ID;
	service_id = service->id;
	if (service_id >= SKINK_MAX_SERVICES)
		return SKINK_BAD_ID;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)	/* STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it */
		return SKINK_TOO_LATE;
	if (_services[service_id].state != SKINK_SERVICE_STATE_WORKING
		|| _services[service_id].memory == NULL)
		return SKINK_FAIL;
	if (!skin_rt_is_rt_context())		/* TODO: is there any way to make this thread temporarily real-time like is done in user-space? */
		return SKINK_BAD_CONTEXT;
	return SKINK_SUCCESS;
}

// TODO: do I need to use _services[service->id].mutex to lock service deletion and calls to these functions?
// Or should I just say: you have to remove all users before stopping the service?
int skink_lock_service(skink_service *service, bool *must_stop)
{
	int ret = 0;
	ret = _check_service_request(service);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (!SKINK_SERVICE_IS_PERIODIC(_services[service->id].memory))
		return SKINK_FAIL;
	while ((ret = skin_rt_rwlock_timed_read_lock(&_services[service->id].rwlock, SKINK_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
											// Wait for the service thread to finish executing
	{
		if (ret == SKIN_RT_FAIL)
		{
			SKINK_RT_LOG("Acquiring periodic lock failed.  Ignoring lock request!");
			return SKINK_FAIL;
		}
		if (must_stop && *must_stop)
		{
			SKINK_RT_LOG("Warning: The calling service is asked to stop.  Ignoring lock request!");
			break;
		}
		if (_service_thread_data[service->id].must_stop)
		{
			SKINK_RT_LOG("Warning: The service is asked to stop.  Ignoring lock request!");
			break;
		}
	}
	return SKINK_SUCCESS;
}

int skink_unlock_service(skink_service *service)
{
	int ret;
	ret = _check_service_request(service);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (!SKINK_SERVICE_IS_PERIODIC(_services[service->id].memory))
		return SKINK_FAIL;
	skin_rt_rwlock_read_unlock(&_services[service->id].rwlock);
	return SKINK_SUCCESS;
}

static int _await_response_common(unsigned int service_id, bool *must_stop)
{
	int ret = 0;
	while ((ret = skin_rt_sem_timed_wait(&_services[service_id].service_response, SKINK_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
											// Wait for the service thread to finish executing
	{
		if (ret == SKIN_RT_FAIL)
		{
			SKINK_RT_LOG("Acquiring sporadic lock failed.  Ignoring service request!");
			return SKINK_FAIL;
		}
		if (must_stop && *must_stop)
		{
			SKINK_RT_LOG("Warning: The calling service is asked to stop.  Ignoring service request!");
			break;
		}
		if (_service_thread_data[service_id].must_stop)
		{
			SKINK_RT_LOG("Warning: The service is asked to stop.  Ignoring service request!");
			break;
		}
	}
	return SKINK_SUCCESS;
}

int skink_request_service(skink_service *service, bool *must_stop)
{
	int ret;
	ret = _check_service_request(service);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (!SKINK_SERVICE_IS_SPORADIC(_services[service->id].memory))
		return SKINK_FAIL;
	skin_rt_sem_post(&_services[service->id].service_request);
	return _await_response_common(service->id, must_stop);
}

int skink_request_service_nonblocking(skink_service *service)
{
	int ret;
	ret = _check_service_request(service);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (!SKINK_SERVICE_IS_SPORADIC(_services[service->id].memory))
		return SKINK_FAIL;
	skin_rt_sem_post(&_services[service->id].service_request);
	return SKINK_SUCCESS;
}

int skink_await_service_response(skink_service *service, bool *must_stop)
{
	int ret;
	ret = _check_service_request(service);
	if (ret != SKINK_SUCCESS)
		return ret;
	if (!SKINK_SERVICE_IS_SPORADIC(_services[service->id].memory))
		return SKINK_FAIL;
	return _await_response_common(service->id, must_stop);
}

int skink_connect_to_service(const char *service_name, skink_service *service)
{
	uint32_t i;
	int sid = -1;
	if (the_state < SKINK_STATE_OPERATIONAL && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)	// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	if (service_name == NULL || service == NULL)
		return SKINK_FAIL;
	if (down_interruptible(&the_services_mutex))
	{
		SKINK_LOG("Connect to service interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	for (i = 0; i < SKINK_MAX_SERVICES; ++i)
		if (_services[i].state != SKINK_SERVICE_STATE_UNUSED)
			if (strncmp(_services[i].memory_name, service_name, SKIN_RT_MAX_NAME_LENGTH) == 0)
			{
				sid = i;
				break;
			}
	up(&the_services_mutex);
	if (sid < 0)
		return SKINK_FAIL;
	service->id = sid;
	service->memory = _services[sid].memory;
	return SKINK_SUCCESS;
}

void skink_disconnect_from_service(skink_service *service)
{
	service->id = (uint32_t)SKINK_INVALID_ID;
	service->memory = NULL;
}
