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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "skin_internal.h"
#include "skin_common.h"
#include "skin_services.h"
#include "skin_object.h"
#include "skin_services_internal.h"

#define SKIN_CONVERTED_TASK_PRIORITY		(SKIN_RT_MAX_PRIORITY - SKIN_SERVICE_PRIORITY * SKIN_RT_MORE_PRIORITY)

static void _signal_all_waiting_users(skin_service_info *service)
{
	int i;
	int tasks_waiting = 1;		/* one task responsible for waking this service up */

	/* for each successful try lock, one user is waiting */
	while (skin_rt_sem_try_wait(service->request) == SKIN_RT_SUCCESS)
		++tasks_waiting;

	/* Unblock as many users as there were requests */
	for (i = 0; i < tasks_waiting; ++i)
		skin_rt_sem_post(service->response);
}

/* Note: updates to service function in kernel, should be reflected here too (and vice versa) */
void *the_skin_service_thread(void *arg)
{
	skin_service_data		*task;
	skin_service_info		*service;
	skin_service_manager		*manager;
	skin_object			*so;
	void				*service_memory;
	char				temp[50]		= "";

	task = (skin_service_data *)arg;
	service = task->service;
	if (service == NULL)
		return NULL;
	manager = service->manager;
	if (manager == NULL)
		return NULL;
	so = manager->p_object;
	if (so == NULL)
		return NULL;

	if (service->id >= SKIN_MAX_SERVICES)
	{
		skin_log_set_error(so, "Internal error: service thread created with wrong data");
		return NULL;
	}
	service_memory = service->memory;
	if (service_memory == NULL)
	{
		skin_log_set_error(so, "Internal error: service thread created with wrong data");
		return NULL;
	}

	if (SKIN_SERVICE_IS_PERIODIC(service_memory))
		snprintf(temp, sizeof(temp) - 1, " (period: %llu)", (unsigned long long)SKIN_SERVICE_PERIOD(service_memory));
	skin_log("Service thread starting (service id %u) (PID: %ld - TID: %ld)%s",
			service->id, (long)getpid(), (long)(pid_t)syscall(SYS_gettid), temp);

	/* make task real-time */
	task->task = skin_rt_user_task_on_start(SKIN_CONVERTED_TASK_PRIORITY);
	if (task->task == NULL)
	{
		skin_log_set_error(so, "Could not get buddy task of service thread (for service %u)!", service->id);
		return NULL;
	}

	skin_rt_time kernel_time_to_user = skin_object_time_diff_with_kernel(so);
	skin_rt_user_task_make_hard_real_time();
	if (SKIN_SERVICE_IS_PERIODIC(service_memory))
	{
		skin_rt_time period = SKIN_SERVICE_PERIOD(service_memory);
		skin_rt_time start_time = skin_rt_get_time() + 2 * period;
		/* align the start time with multiples of period.  Users will also do it, so kind of already synchronized! */
		start_time -= (start_time - kernel_time_to_user) % period;
		skin_rt_user_task_make_periodic(task->task, start_time, period);
		skin_rt_user_task_wait_period();
	}

	SKIN_SERVICE_TIMESTAMP(service_memory)	= 0;
	task->running				= true;
	task->paused				= false;
	while (!task->must_stop)
	{
		if (task->must_pause)
		{
			if (SKIN_SERVICE_IS_PERIODIC(service_memory))
				skin_rt_user_task_wait_period();
			else
			{
				if (skin_rt_sem_try_wait(service->request) == SKIN_RT_SUCCESS)
					_signal_all_waiting_users(service);
				skin_rt_sleep(SKIN_MAX_THREAD_DELAY);
			}
			task->paused = true;
			continue;
		}
		task->paused = false;

		/* if sporadic, wait for request */
		if (SKIN_SERVICE_IS_SPORADIC(service_memory))
		{
			int ret = 0;
			while ((ret = skin_rt_sem_timed_wait(service->request, SKIN_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
			{
				if (ret == SKIN_RT_FAIL)
				{
					skin_log_set_error(so, "Acquiring service sporadic lock failed, quitting!");
					task->must_stop = true;
				}
				/*
				 * if not, then it must be timeout, which is fine.  Try again, but just test
				 * to see if you are supposed to stop
				 */
				if (task->must_stop || task->must_pause)
					break;
			}
			if (task->must_stop || task->must_pause)
			{
				if (ret == SKIN_RT_SUCCESS)
					_signal_all_waiting_users(service);
				continue;
			}
		}
		else		/* then task is periodic */
		{
			int ret = 0;
			while ((ret = skin_rt_rwlock_timed_write_lock(service->rwlock, SKIN_SERVICE_PERIOD(service_memory))) != SKIN_RT_SUCCESS)
			{
				if (ret == SKIN_RT_FAIL)
				{
					skin_log_set_error(so, "Acquiring service periodic lock failed, quitting!");
					task->must_stop = true;
				}
				/*
				 * if not, then it must be timeout, which is fine.  Try again, but just test
				 * to see if you are supposed to stop
				 */
				if (task->must_stop || task->must_pause)
					break;
			}
			if (task->must_stop || task->must_pause)
			{
				if (ret == SKIN_RT_SUCCESS)
					skin_rt_rwlock_write_unlock(service->rwlock);
				continue;
			}
		}

		/* call the service function */
		task->function(service_memory, task->user_data);

		/* keep timing information */
		SKIN_SERVICE_TIMESTAMP(service_memory) = skin_rt_get_time() - kernel_time_to_user;

		/* release sync locks */
		if (SKIN_SERVICE_IS_SPORADIC(service_memory))
			_signal_all_waiting_users(service);
		else
		{
			skin_rt_rwlock_write_unlock(service->rwlock);
			skin_rt_user_task_wait_period();
		}
	}

	task->running = false;
	skin_rt_user_task_on_stop();
	skin_log("Service thread exiting (service id %u) (PID: %ld - TID: %ld)",
			service->id, (long)getpid(), (long)(pid_t)syscall(SYS_gettid));
	return NULL;
}

static void _make_invalid(skin_service *service)
{
	*service = (skin_service){0};
}

void skin_service_init(skin_service *service)
{
	_make_invalid(service);
}

void skin_service_free(skin_service *service)
{
	skin_service_disconnect(service);
}

static int _check_lock_ret_value(skin_service *service, bool *must_stop, int ret, const char *lock_name)
{
	if (ret == SKIN_RT_FAIL)
	{
		skin_log_set_error(service->p_object, "Acquiring %s lock failed.  Ignoring lock request", lock_name);
		return SKIN_FAIL;
	}
	if (must_stop && *must_stop)
	{
		skin_log_set_error(service->p_object, "Warning: The program is asked to stop.  Ignoring lock request");
		return SKIN_LOCK_NOT_ACQUIRED;
	}
	if (!SKIN_SERVICE_IS_ALIVE(service->memory))
	{
		skin_log_set_error(service->p_object, "Warning: The service has been disabled.  Ignoring lock request!");
		return SKIN_LOCK_NOT_ACQUIRED;
	}
	return SKIN_SUCCESS;
}

int skin_service_lock(skin_service *service, bool *must_stop)
{
	int ret_value = SKIN_SUCCESS;
	bool fromlinux;

	if (service->memory == NULL || SKIN_SERVICE_IS_DEAD(service->memory)
		|| !SKIN_SERVICE_IS_PERIODIC(service->memory))
		return SKIN_FAIL;
	if (service->p_rwl == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() == SKIN_RT_FAIL)
	{
		skin_log_set_error(service->p_object, "Could not make thread calling service lock a real-time thread!");
		return SKIN_FAIL;
	}

	int ret = 0;
	while ((ret = skin_rt_rwlock_timed_read_lock(service->p_rwl, SKIN_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
	{
		ret = _check_lock_ret_value(service, must_stop, ret, "periodic");
		if (ret != SKIN_SUCCESS)
			break;
	}
	if (fromlinux)
		skin_rt_stop();
	return ret_value;
}

int skin_service_unlock(skin_service *service)
{
	bool fromlinux;

	if (service->memory == NULL || SKIN_SERVICE_IS_DEAD(service->memory)
		|| !SKIN_SERVICE_IS_PERIODIC(service->memory) || service->p_rwl == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() == SKIN_RT_FAIL)
	{
		skin_log_set_error(service->p_object, "Could not make thread calling service unlock a real-time thread!");
		return SKIN_FAIL;
	}

	skin_rt_rwlock_read_unlock(service->p_rwl);

	if (fromlinux)
		skin_rt_stop();
	return SKIN_SUCCESS;
}

static void _request_nonblocking(skin_service *service)
{
	skin_rt_sem_post(service->p_request);
}

static int _await_response(skin_service *service, bool *must_stop)
{
	int ret = SKIN_SUCCESS;
	while ((ret = skin_rt_sem_timed_wait(service->p_response, SKIN_MAX_THREAD_DELAY)) != SKIN_RT_SUCCESS)
	{
		ret = _check_lock_ret_value(service, must_stop, ret, "response");
		if (ret != SKIN_SUCCESS)
			break;
	}
	return ret;
}

int skin_service_request(skin_service *service, bool *must_stop)
{
	int ret;
	bool fromlinux;

	if (service->memory == NULL || SKIN_SERVICE_IS_DEAD(service->memory)
		|| !SKIN_SERVICE_IS_SPORADIC(service->memory))
		return SKIN_FAIL;
	if (service->p_request == NULL || service->p_response == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() == SKIN_RT_FAIL)
	{
		skin_log_set_error(service->p_object, "Could not make thread calling service request a real-time thread!");
		return SKIN_FAIL;
	}

	_request_nonblocking(service);
	ret = _await_response(service, must_stop);

	if (fromlinux)
		skin_rt_stop();
	return ret;
}

int skin_service_request_nonblocking(skin_service *service)
{
	bool fromlinux;

	if (service->memory == NULL || SKIN_SERVICE_IS_DEAD(service->memory)
		|| !SKIN_SERVICE_IS_SPORADIC(service->memory))
		return SKIN_FAIL;
	if (service->p_request == NULL || service->p_response == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() == SKIN_RT_FAIL)
	{
		skin_log_set_error(service->p_object, "Could not make thread calling service request nonblocking a real-time thread!");
		return SKIN_FAIL;
	}

	_request_nonblocking(service);

	if (fromlinux)
		skin_rt_stop();
	return SKIN_SUCCESS;
}

int skin_service_await_response(skin_service *service, bool *must_stop)
{
	int ret;
	bool fromlinux;

	if (service->memory == NULL || SKIN_SERVICE_IS_DEAD(service->memory)
		|| !SKIN_SERVICE_IS_SPORADIC(service->memory))
		return SKIN_FAIL;
	if (service->p_request == NULL || service->p_response == NULL)
		return SKIN_FAIL;

	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() == SKIN_RT_FAIL)
	{
		skin_log_set_error(service->p_object, "Could not make thread calling service await response a real-time thread!");
		return SKIN_FAIL;
	}

	ret = _await_response(service, must_stop);

	if (fromlinux)
		skin_rt_stop();
	return ret;
}

void skin_service_disconnect(skin_service *service)
{
	if (service->memory != NULL && service->p_mem_name != NULL)
	{
		skin_rt_shared_memory_detach(service->p_mem_name);
		skin_log("Disconnected from service %s", service->p_mem_name);
	}
	if (service->p_mem_name)
		free(service->p_mem_name);
	_make_invalid(service);
}

static void _service_info_init(skin_service_info *info, skin_service_manager *sm, uint32_t id)
{
	*info = (skin_service_info){
		.id = id,
		.state = SKIN_SERVICE_STATE_UNUSED,
		.manager = sm,
		.task_data.must_stop = true,
		.task_data.service = info
	};
	info->memory_name[0] = '\0';
}

void skin_service_manager_init(skin_service_manager *sm)
{
	*sm = (skin_service_manager){0};
}

void skin_service_manager_free(skin_service_manager *sm)
{
	skin_service_manager_internal_terminate(sm);
}

int skin_service_manager_internal_initialize(skin_service_manager *sm)
{
	skin_service_info *services;
	uint32_t i;

	sm->p_add_remove_mutex = skin_rt_mutex_init(NULL, 1);
	if (sm->p_add_remove_mutex == NULL)
		return SKIN_FAIL;

	services = malloc(SKIN_MAX_SERVICES * sizeof(*services));
	sm->p_services = services;
	if (sm->p_services == NULL)
		return SKIN_FAIL;

	for (i = 0; i < SKIN_MAX_SERVICES; ++i)
		_service_info_init(&services[i], sm, i);

	return SKIN_SUCCESS;
}

int skin_service_manager_internal_terminate(skin_service_manager *sm)
{
	if (sm->p_add_remove_mutex)
		skin_rt_mutex_delete(sm->p_add_remove_mutex);
	if (sm->p_services)
	{
		unsigned int i;
		for (i = 0; i < SKIN_MAX_SERVICES; ++i)
			skin_service_manager_stop_service(sm, i);
		free(sm->p_services);
	}
	sm->p_add_remove_mutex = NULL;
	sm->p_services = NULL;
	return SKIN_SUCCESS;
}

#define SKIN_SHARED_MEMORY_ALLOC(result, name, size)\
	do {\
		bool _is_contiguous;\
		result = skin_rt_shared_memory_alloc(name, size, &_is_contiguous);\
		if (result == NULL)\
			skin_log_set_error(sm->p_object, "Could not acquire memory for %s", #result);\
		else if (!_is_contiguous)\
			skin_log_set_error(sm->p_object, "Warning - Using discontiguous memory");	/* Not really a problem.  But if you see this,*/\
													/* you are most likely almost running out of memory */\
	} while (0)

static void _service_info_cleanup(skin_service_info *info)
{
	if (info->state != SKIN_SERVICE_STATE_BAD)
		skin_rt_mutex_delete(info->mutex);
	if (info->memory_name[0] != '\0')
		skin_rt_shared_memory_free(info->memory_name);
	if (info->request != NULL)
		skin_rt_sem_unshare_and_delete(info->request);
	if (info->response != NULL)
		skin_rt_sem_unshare_and_delete(info->response);
	if (info->rwlock != NULL)
		skin_rt_rwlock_unshare_and_delete(info->rwlock);

	info->mutex = NULL;
	info->memory = NULL;
	info->memory_name[0] = '\0';
	info->request = NULL;
	info->response = NULL;
	info->rwlock =  NULL;
	info->state = SKIN_SERVICE_STATE_UNUSED;
}

static int _initialize_common(skin_service_manager *sm,
		const char *service_name, size_t elem_size, size_t count)
{
	uint32_t i;
	int sid = -1;
	skin_service_info *services;

	if (sm->p_add_remove_mutex == NULL || sm->p_services == NULL)
		return SKIN_FAIL;

	services = sm->p_services;

	/* find an available slot */
	if (skin_rt_mutex_lock(sm->p_add_remove_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not acquire service creation lock");
		return SKIN_FAIL;
	}
	for (i = 0; i < SKIN_MAX_SERVICES; ++i)
		if (services[i].state == SKIN_SERVICE_STATE_UNUSED)
		{
			services[i].state = SKIN_SERVICE_STATE_INITIALIZING;
			sid = i;
			break;
		}
	skin_rt_mutex_unlock(sm->p_add_remove_mutex);

	/* allocate resources for this service */
	if (sid < 0)
	{
		skin_log_set_error(sm->p_object, "Too many services");
		return SKIN_FAIL;
	}
	if ((services[sid].mutex = skin_rt_mutex_init(NULL, 1)) == NULL)
	{
		skin_log_set_error(sm->p_object, "Could not create mutex for service");
		services[sid].state = SKIN_SERVICE_STATE_UNUSED;
		return SKIN_FAIL;
	}
	SKIN_SHARED_MEMORY_ALLOC(services[sid].memory, service_name, elem_size * count + SKIN_SERVICE_HEADER_SIZE * sizeof(uint64_t));
	if (services[sid].memory == NULL)
	{
		skin_log_set_error(sm->p_object, "Out of memory when creating service");
		_service_info_cleanup(&services[sid]);
		return SKIN_NO_MEM;
	}
	strncpy(services[sid].memory_name, service_name, SKIN_RT_MAX_NAME_LENGTH);
	services[sid].memory_name[SKIN_RT_MAX_NAME_LENGTH]	= '\0';
	services[sid].memory = ((uint64_t *)services[sid].memory) + SKIN_SERVICE_HEADER_SIZE;

	/* initial service data */
	SKIN_SERVICE_TIMESTAMP(services[sid].memory)		= 0;
	SKIN_SERVICE_STATUS(services[sid].memory)		= SKIN_SERVICE_DEAD;
	SKIN_SERVICE_MEM_SIZE(services[sid].memory)		= elem_size * count;
	SKIN_SERVICE_RESULT_COUNT(services[sid].memory)		= count;
	return sid;
}

int skin_service_manager_initialize_periodic_service(skin_service_manager *sm, const char *service_name,
		size_t elem_size, size_t count, const char *rwl_name, skin_rt_time period)
{
	int sid = -1;
	bool fromlinux;
	skin_service_info *services;

	if (service_name == NULL || service_name[0] == '\0' || rwl_name == NULL || rwl_name[0] == '\0' || elem_size == 0 || count == 0)
		return SKIN_BAD_DATA;
	if (sm->p_object == NULL)
		return SKIN_FAIL;
	if (!skin_rt_name_available(service_name) || !skin_rt_name_available(rwl_name))
		return SKIN_BAD_NAME;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not make service creation thread work with realtime functions");
		return SKIN_FAIL;
	}

	sid = _initialize_common(sm, service_name, elem_size, count);
	if (sid < 0)
		goto exit_normal;

	services = sm->p_services;
	if ((services[sid].rwlock = skin_rt_rwlock_init_and_share(NULL, rwl_name)) == NULL)
		goto exit_fail;
	services[sid].state				= SKIN_SERVICE_STATE_INITIALIZED;
	SKIN_SERVICE_PERIOD(services[sid].memory)	= period;
	SKIN_SERVICE_MODE(services[sid].memory)		= SKIN_SERVICE_PERIODIC;
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return sid;
exit_fail:
	_service_info_cleanup(&services[sid]);
	sid = SKIN_FAIL;
	goto exit_normal;
}

int skin_service_manager_initialize_sporadic_service(skin_service_manager *sm, const char *service_name,
		size_t elem_size, size_t count, const char *request_name, const char *response_name)
{
	int sid = -1;
	bool fromlinux;
	skin_service_info *services;

	if (service_name == NULL || service_name[0] == '\0' || request_name == NULL || request_name[0] == '\0'
		|| response_name == NULL || response_name[0] == '\0' || elem_size == 0 || count == 0)
		return SKIN_BAD_DATA;
	if (sm->p_object == NULL)
		return SKIN_FAIL;
	if (!skin_rt_name_available(service_name) || !skin_rt_name_available(request_name) || !skin_rt_name_available(response_name))
		return SKIN_BAD_NAME;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not make service creation thread work with realtime functions");
		return SKIN_FAIL;
	}

	sid = _initialize_common(sm, service_name, elem_size, count);
	if (sid < 0)
		goto exit_normal;

	services = sm->p_services;
	if ((services[sid].request = skin_rt_sem_init_and_share(NULL, 0, request_name)) == NULL)
		goto exit_fail;
	if ((services[sid].response = skin_rt_sem_init_and_share(NULL, 0, response_name)) == NULL)
		goto exit_fail;
	services[sid].state				= SKIN_SERVICE_STATE_INITIALIZED;
	SKIN_SERVICE_MODE(services[sid].memory)		= SKIN_SERVICE_SPORADIC;
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return sid;
exit_fail:
	_service_info_cleanup(&services[sid]);
	sid = SKIN_FAIL;
	goto exit_normal;
}

static void _remove_task(skin_service_manager *sm, unsigned int service_id)
{
	skin_service_info	*si;
	bool			all_quit = false;
	struct timespec		start_time_,
				now_time_;
	uint64_t		start_time;
        uint64_t		now_time;
	uint64_t		max_wait_time = 0;
	bool			fromlinux;

	if (service_id >= SKIN_MAX_SERVICES)
		return;
	si = (skin_service_info *)sm->p_services + service_id;
	if (si->state == SKIN_SERVICE_STATE_UNUSED)
		return;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not make service task remove work with realtime functions");
		return;
	}

	if (skin_rt_mutex_lock(sm->p_add_remove_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Stopping service task couldn't get lock");
		goto exit_normal;
	}
	if (si->memory == NULL)
		goto exit_unlock;

	SKIN_SERVICE_STATUS(si->memory) = SKIN_SERVICE_DEAD;
	if (si->task_data.task_id > 0)
	{
		/* give the task its period + 1.5 seconds to stop, or just 1.5 seconds if sporadic */
		si->task_data.must_stop = true;
		if (SKIN_SERVICE_IS_PERIODIC(si->memory))
			max_wait_time = SKIN_SERVICE_PERIOD(si->memory);
		max_wait_time += 1500000000ull;
		all_quit = false;
		clock_gettime(CLOCK_MONOTONIC, &start_time_);
		start_time = start_time_.tv_sec * 1000000000ull + start_time_.tv_nsec;

		/* wiat for task to stop */
		do
		{
			usleep(1000);
			all_quit = !si->task_data.running;
			clock_gettime(CLOCK_MONOTONIC, &now_time_);
			now_time = now_time_.tv_sec * 1000000000ull + now_time_.tv_nsec;
		} while (!all_quit && now_time-start_time < max_wait_time);

		/* if not stopped, kill it anyway */
		if (si->task_data.created && si->task_data.running)
			skin_log_set_error(sm->p_object, "Waited for service %u to exit for %fs, but it didn't stop.  Killing it.",
					service_id, max_wait_time / 1000000000.0f);
		skin_rt_user_task_delete(si->task_data.task);
		skin_rt_user_task_join(si->task_data.task_id);
	}
	/* TODO: use C-style */
	si->task_data.function		= NULL;
	si->task_data.created		= false;
	si->task_data.task		= NULL;
	si->task_data.task_id		= 0;
	si->state			= SKIN_SERVICE_STATE_INITIALIZED;
exit_unlock:
	skin_rt_mutex_unlock(sm->p_add_remove_mutex);
exit_normal:
	if (fromlinux)
		skin_rt_stop();
}

int skin_service_manager_start_service(skin_service_manager *sm,
		unsigned int service_id, skin_service_function function, void *data)
{
	int ret = SKIN_FAIL;
	bool fromlinux;
	skin_service_info *services;

	if (service_id >= SKIN_MAX_SERVICES)
		return SKIN_BAD_ID;
	if (function == NULL)
		return SKIN_BAD_DATA;
	if (sm->p_object == NULL || sm->p_services == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not make service start work with realtime functions");
		return SKIN_FAIL;
	}

	if (skin_rt_mutex_lock(sm->p_add_remove_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Service start couldn't get lock");
		goto exit_normal;
	}

	services = sm->p_services;
	if (services[service_id].state == SKIN_SERVICE_STATE_UNUSED)
	{
		ret = SKIN_BAD_ID;
		goto exit_unlock;
	}
	if (services[service_id].state == SKIN_SERVICE_STATE_BAD || services[service_id].state == SKIN_SERVICE_STATE_WORKING)
		goto exit_unlock;

	/* create the service thread */
	/* TODO: use C-style (?) */
	services[service_id].task_data.must_stop		= false;
	services[service_id].task_data.must_pause		= false;
	services[service_id].task_data.running			= false;
	services[service_id].task_data.function			= function;
	services[service_id].task_data.user_data		= data;
	services[service_id].task_data.service			= services + service_id;
	services[service_id].task_data.task_id			= skin_rt_user_task_init(the_skin_service_thread,
										&services[service_id].task_data, SKIN_TASK_STACK_SIZE);
	if (services[service_id].task_data.task_id == 0)
	{
		skin_log_set_error(sm->p_object, "Could not create thread for service %u", service_id);
		skin_rt_mutex_unlock(sm->p_add_remove_mutex);
		skin_service_manager_stop_service(sm, service_id);
		goto exit_normal;
	}
	services[service_id].task_data.created			= true;
	services[service_id].state				= SKIN_SERVICE_STATE_WORKING;
	SKIN_SERVICE_STATUS(services[service_id].memory)	= SKIN_SERVICE_ALIVE;
	ret = SKIN_SUCCESS;
exit_unlock:
	skin_rt_mutex_unlock(sm->p_add_remove_mutex);
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return ret;
}

static int _pause_resume(skin_service_manager *sm, unsigned int service_id, bool pause)
{
	int ret = SKIN_FAIL;
	bool fromlinux;
	skin_service_info *services;

	if (service_id >= SKIN_MAX_SERVICES)
		return SKIN_BAD_ID;
	if (sm->p_object == NULL || sm->p_services == NULL)
		return SKIN_FAIL;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not make service %s work with realtime functions", pause?"pause":"resume");
		return SKIN_FAIL;
	}

	if (skin_rt_mutex_lock(sm->p_add_remove_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Service %s couldn't get lock", pause?"pause":"resume");
		goto exit_normal;
	}

	services = sm->p_services;
	if (services[service_id].state == SKIN_SERVICE_STATE_UNUSED)
	{
		ret = SKIN_BAD_ID;
		goto exit_unlock;
	}
	if (services[service_id].state == SKIN_SERVICE_STATE_BAD)
		goto exit_unlock;
	services[service_id].task_data.must_pause = pause;
	ret = SKIN_SUCCESS;
exit_unlock:
	skin_rt_mutex_unlock(sm->p_add_remove_mutex);
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return ret;
}

int skin_service_manager_pause_service(skin_service_manager *sm, unsigned int service_id)
{
	return _pause_resume(sm, service_id, true);
}

int skin_service_manager_resume_service(skin_service_manager *sm, unsigned int service_id)
{
	return _pause_resume(sm, service_id, false);
}

int skin_service_manager_stop_service(skin_service_manager *sm, unsigned int service_id)
{
	bool fromlinux;
	skin_service_info *si;
	int ret = SKIN_FAIL;

	if (service_id >= SKIN_MAX_SERVICES)
		return SKIN_BAD_ID;
	if (sm->p_object == NULL || sm->p_services == NULL)
		return SKIN_FAIL;

	si = (skin_service_info *)sm->p_services + service_id;
	if (si->state == SKIN_SERVICE_STATE_UNUSED)
		return SKIN_SUCCESS;

	/* make thread real-time if not already */
	fromlinux = !skin_rt_is_rt_context();
	if (fromlinux && skin_rt_init() != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Could not make service stop work with realtime functions");
		return SKIN_FAIL;
	}

	_remove_task(sm, service_id);

	if (skin_rt_mutex_lock(sm->p_add_remove_mutex) != SKIN_RT_SUCCESS)
	{
		skin_log_set_error(sm->p_object, "Service stop couldn't get lock");
		goto exit_normal;
	}
	_service_info_cleanup(si);
	skin_rt_mutex_unlock(sm->p_add_remove_mutex);
	ret = SKIN_SUCCESS;
exit_normal:
	if (fromlinux)
		skin_rt_stop();
	return ret;
}

static int _connect_common(skin_service_manager *sm, const char *service_name, skin_service *service)
{
	service->p_mem_name = strdup(service_name);
	if (service->p_mem_name == NULL)
	{
		skin_log_set_error(sm->p_object, "Out of memory when initializing service %s", service_name);
		return SKIN_FAIL;
	}
	service->memory = skin_rt_shared_memory_attach(service_name);
	if (service->memory == NULL)
	{
		skin_log_set_error(sm->p_object, "Could not get results of service %s", service_name);
		return SKIN_FAIL;
	}
	service->memory = (uint64_t *)service->memory + SKIN_SERVICE_HEADER_SIZE;
	return SKIN_SUCCESS;
}

int skin_service_manager_connect_to_periodic_service(skin_service_manager *sm, const char *service_name,
		const char *rwl_name, skin_service *service)
{
	int res;

	if (sm->p_object == NULL || service_name == NULL || rwl_name == NULL || service == NULL)
		return SKIN_FAIL;

	res = _connect_common(sm, service_name, service);
	if (res != SKIN_SUCCESS)
		return res;

	if (!SKIN_SERVICE_IS_PERIODIC(service->memory))
	{
		skin_log_set_error(sm->p_object, "Request to connect to non-periodic service %s through connect_to_periodic_service", service_name);
		goto exit_fail;
	}
	service->p_rwl = skin_rt_get_shared_rwlock(rwl_name);
	if (service->p_rwl == NULL)
	{
		skin_log_set_error(sm->p_object, "Could not acquire shared lock %s from service %s", rwl_name, service_name);
		goto exit_fail;
	}
	service->p_object = sm->p_object;
	skin_log("Connected to service %s (periodic)", service_name);
	return SKIN_SUCCESS;
exit_fail:
	skin_rt_shared_memory_detach(service_name);
	skin_service_disconnect(service);
	return SKIN_FAIL;
}

int skin_service_manager_connect_to_sporadic_service(skin_service_manager *sm, const char *service_name,
		const char *request_name, const char *response_name, skin_service *service)
{
	int res;

	if (sm->p_object == NULL || service_name == NULL || request_name == NULL || response_name == NULL || service == NULL)
		return SKIN_FAIL;

	res = _connect_common(sm, service_name, service);
	if (res != SKIN_SUCCESS)
		return res;

/*	skin_log("Sporadic service header:\n  %llu\n  %llu\n  %llu\n  %llu\n  %llu",
			(unsigned long long)SKIN_SERVICE_TIMESTAMP(service->memory),
			(unsigned long long)SKIN_SERVICE_MODE(service->memory),
			(unsigned long long)SKIN_SERVICE_RESULT_COUNT(service->memory),
			(unsigned long long)SKIN_SERVICE_MEM_SIZE(service->memory),
			(unsigned long long)SKIN_SERVICE_STATUS(service->memory));
*/
	if (!SKIN_SERVICE_IS_SPORADIC(service->memory))
	{
		skin_log_set_error(sm->p_object, "Request to connect to non-sporadic service %s through connect_to_sporadic_service", service_name);
		goto exit_fail;
	}
	service->p_request = skin_rt_get_shared_mutex(request_name);
	service->p_response = skin_rt_get_shared_mutex(response_name);
	if (service->p_request == NULL || service->p_response == NULL)
	{
		skin_log_set_error(sm->p_object, "Could not acquire shared locks %s and %s from service %s", request_name, response_name, service_name);
		goto exit_fail;
	}
	service->p_object = sm->p_object;
	skin_log("Connected to service %s (sporadic)", service_name);
	return SKIN_SUCCESS;
exit_fail:
	skin_rt_shared_memory_detach(service_name);
	skin_service_disconnect(service);
	return SKIN_FAIL;
}
