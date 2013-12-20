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

#ifndef SKIN_SERVICES_H
#define SKIN_SERVICES_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include <stdbool.h>
#include "skin_common.h"
#include "skin_kernel.h"

#define					SKIN_SERVICE_HEADER_SIZE			SKINK_SERVICE_HEADER_SIZE
#define					SKIN_SERVICE_PERIOD(result)			SKINK_SERVICE_PERIOD(result)
#define					SKIN_SERVICE_TIMESTAMP(result)			SKINK_SERVICE_TIMESTAMP(result)
#define					SKIN_SERVICE_PERIODIC				SKINK_SERVICE_PERIODIC
#define					SKIN_SERVICE_SPORADIC				SKINK_SERVICE_SPORADIC
#define					SKIN_SERVICE_MODE(result)			SKINK_SERVICE_MODE(result)
#define					SKIN_SERVICE_IS_PERIODIC(result)		SKINK_SERVICE_IS_PERIODIC(result)
#define					SKIN_SERVICE_IS_SPORADIC(result)		SKINK_SERVICE_IS_SPORADIC(result)
#define					SKIN_SERVICE_RESULT_COUNT(result)		SKINK_SERVICE_RESULT_COUNT(result)
#define					SKIN_SERVICE_MEM_SIZE(result)			SKINK_SERVICE_MEM_SIZE(result)
#define					SKIN_SERVICE_ALIVE				SKINK_SERVICE_ALIVE
#define					SKIN_SERVICE_DEAD				SKINK_SERVICE_DEAD
#define					SKIN_SERVICE_STATUS(result)			SKINK_SERVICE_STATUS(result)
#define					SKIN_SERVICE_IS_ALIVE(result)			SKINK_SERVICE_IS_ALIVE(result)
#define					SKIN_SERVICE_IS_DEAD(result)			SKINK_SERVICE_IS_DEAD(result)

struct skin_object;
struct skin_service_manager;

SKIN_DECL_BEGIN

typedef void (*skin_service_function)(void *mem, void *user_data);

typedef struct skin_service skin_service;

/*
 * skin_service is a user interface for interacting with the service.  In all functions, unless stated otherwise, SKIN_SUCCESS would be
 * returned if everything is ok or SKIN_FAIL if failed.
 *
 * init				initialize the service, but it would be an invalid one
 * free				disconnect the service and release resources
 * lock				only used with PERIODIC services.  Locks the service memory.  If must_stop is NULL it will ignored.
 *				Otherwise, it can be used to cancel the lock operation (from another thread).
 *				Returns SKIN_LOCK_NOT_ACQUIRED if must stop or service stops
 * unlock			only used with PERIODIC services.  Unlock the service memory
 * request			only used with SPORADIC services.  Requests execution of service and awaits its response.
 *				Returns SKIN_LOCK_NOT_ACQUIRED if must stop or service stops
 * request_nonblocking		similar to request, but only sends the request and doesn't wait on response.  After a call to this
 *				function, await_response needs to be called
 * await_response		complete the request after a call to request_nonblocking.  If must_stop is NULL it will ignored.
 *				Otherwise, it can be used to cancel the await_response operation (from another thread).
 *				Returns SKIN_LOCK_NOT_ACQUIRED if must stop or service stops
 * disconnect			disconnect and invalidate service object
 */
void skin_service_init(skin_service *service);
void skin_service_free(skin_service *service);
int skin_service_lock(skin_service *service, bool *must_stop);
int skin_service_unlock(skin_service *service);
int skin_service_request(skin_service *service, bool *must_stop);
int skin_service_request_nonblocking(skin_service *service);
int skin_service_await_response(skin_service *service, bool *must_stop);
void skin_service_disconnect(skin_service *service);

struct skin_service
{
	void				*memory;		/* the shared memory with the service */

	/* internal */
	struct skin_object		*p_object;
	skin_rt_rwlock			*p_rwl;
	skin_rt_semaphore		*p_request,
					*p_response;
	bool				p_locked;
	char				*p_mem_name;

#ifdef SKIN_CPP
	skin_service() { skin_service_init(this); }
	~skin_service() { skin_service_free(this); }
	int lock(bool *must_stop = NULL) { return skin_service_lock(this, must_stop); }
	int unlock() { return skin_service_unlock(this); }
	int request(bool *must_stop = NULL) { return skin_service_request(this, must_stop); }
	int request_nonblocking() { return skin_service_request_nonblocking(this); }
	int await_response(bool *must_stop = NULL) { return skin_service_await_response(this, must_stop); }
	void disconnect() { skin_service_disconnect(this); }
private:
	/* note: skin_service objects should not be copied, as their destructor disconnects from the service */
	skin_service &operator =(skin_service &);
	skin_service(skin_service &);
#endif
};

typedef struct skin_service_manager skin_service_manager;

/*
 * skin_service_manager is what manages creation of services as well as connection to other services.  In all functions, unless stated
 * otherwise, SKIN_SUCCESS would be returned if everything is ok, SKIN_BAD_ID if a bad service id is given, SKIN_BAD_DATA if data given
 * to create a service are invalid or SKIN_FAIL if other errors.
 *
 * init				initialize the manager, but it wouldn't still work until initialized by skin_object
 * free				stop all services created by this manager.  Note that this function doesn't disconnect from any
 *				services the user may have been connected to.  Those services need to be disconnected by
 *				skin_service_disconnect.
 *
 * For service creation:
 * initialize_periodic_service	initialize a periodic or sporadic service by allocating shared memory (of size mem_size and
 *				with name service_name).  A reader-writer lock is created for synchronization.  This function may
 *				return a negative value indicating error or a non-negative value which is the service id.
 *				This service id should be used with the rest of the functions, such as start_service and stop_service
 * initialize_sporadic_service	similar to initialize_periodic_service, but initializes a sporadic service and creating two semaphores
 *				(request and response) instead of the reader-writer lock
 * start_service		spawn a service thread calling function either periodically or upon request based on whichever mode the
 *				service was initialized with
 * pause_service		pause service thread
 * resume_service		resume a paused service
 * stop_service			stop the service, removing the service thread.  This function blocks until the service is stopped
 *
 * For service usage:
 * connect_to_periodic_service	connect to a periodic service having its results stored in shared memory with name service_name and
 *				that uses a readers-writers lock with name rwl_name.  On success, service->memory will contain
 *				the address to the shared memory
 * connect_to_sporadic_service	similar to connect_to_periodic_service, but connects to a sporadic one.  Instead of a readers-writers lock,
 *				it would attach to two semaphores with names request_name and response_name
 */
void skin_service_manager_init(skin_service_manager *sm);
void skin_service_manager_free(skin_service_manager *sm);
int skin_service_manager_initialize_periodic_service(skin_service_manager *sm,
					const char *service_name,
					size_t elem_size, size_t count,
					const char *rwl_name,
					skin_rt_time period);
int skin_service_manager_initialize_sporadic_service(skin_service_manager *sm,
					const char *service_name,
					size_t elem_size, size_t count,
					const char *request_name, const char *response_name);
int skin_service_manager_start_service(skin_service_manager *sm,
					unsigned int service_id,
					skin_service_function function,
					void *data);
int skin_service_manager_pause_service(skin_service_manager *sm, unsigned int service_id);
int skin_service_manager_resume_service(skin_service_manager *sm, unsigned int service_id);
int skin_service_manager_stop_service(skin_service_manager *sm, unsigned int service_id);
int skin_service_manager_connect_to_periodic_service(skin_service_manager *sm,
					const char *service_name,
					const char *rwl_name,
					skin_service *service);
int skin_service_manager_connect_to_sporadic_service(skin_service_manager *sm,
					const char *service_name,
					const char *request_name, const char *response_name,
					skin_service *service);

struct skin_service_manager
{
	/* internal */
	struct skin_object		*p_object;
	skin_rt_mutex			*p_add_remove_mutex;
	void				*p_services;			/*
									 * The services.  There are maximum SKIN_MAX_SERVICES possible
									 * Note: I can't dynamically resize the list because the threads have pointers
									 * to inside it.  Resizing could invalidate those pointers.
									 */

#ifdef SKIN_CPP
	skin_service_manager() { skin_service_manager_init(this); }
	~skin_service_manager() { skin_service_manager_free(this); }
	int initialize_periodic_service(const char *service_name, size_t elem_size, size_t count,
					const char *rwl_name, skin_rt_time period)
			{ return skin_service_manager_initialize_periodic_service(this,
					service_name, elem_size, count, rwl_name, period); }
	int initialize_sporadic_service(const char *service_name, size_t elem_size, size_t count,
					const char *request_name, const char *response_name)
			{ return skin_service_manager_initialize_sporadic_service(this,
					service_name, elem_size, count, request_name, response_name); }
	int start_service(unsigned int service_id, skin_service_function function, void *data)
			{ return skin_service_manager_start_service(this, service_id, function, data); }
	int pause_service(unsigned int service_id) { return skin_service_manager_pause_service(this, service_id); }
	int resume_service(unsigned int service_id) { return skin_service_manager_resume_service(this, service_id); }
	int stop_service(unsigned int service_id) { return skin_service_manager_stop_service(this, service_id); }
	int connect_to_periodic_service(const char *service_name, const char *rwl_name, skin_service *service)
			{ return skin_service_manager_connect_to_periodic_service(this, service_name, rwl_name, service); }
	int connect_to_sporadic_service(const char *service_name, const char *request_name, const char *response_name, skin_service *service)
			{ return skin_service_manager_connect_to_sporadic_service(this,
					service_name, request_name, response_name, service); }
#endif
};

SKIN_DECL_END

#endif
