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

#ifndef SKINK_SERVICES_INTERNAL_H
#define SKINK_SERVICES_INTERNAL_H

#ifdef __KERNEL__

#include "skink_services.h"

struct skink_service_info;

typedef struct skink_reader_data
{
	skink_service_acquire		acquire;		// acquire callback for feeding the service with data
	int				acq_mode;		// one of SKINK_ACQUISITION_*
	skin_rt_time			period;			// task period if mode == SKINK_ACQUISITION_PERIODIC
	bool				must_stop;		// if 1, task will quit
	bool				running;		// if 1, then task is still running
	bool				paused;			// if 1, then task is paused
	bool				must_pause;		// if 1, then task should pause
	struct skink_service_info	*service;		// the service to which this task relates to
	skin_rt_semaphore		read_request,		// the semaphores used by users to ask
					read_response;		// reader for data
	bool				read_sems_created;	// whether read_* is initialized (and therefore needs cleanup)
	skink_sensor_layer_id		layer;			// the layer to which this task relates to
	skin_rt_task			task;			// the RT task identifier of this task
	bool				created;		// if 1, then task is created and should be deleted on cleanup
} skink_reader_data;

typedef struct skink_service_data
{
	skink_service_function		function;		// callback function to let the service write its data
	bool				must_stop;		// if 1, task will quit
	bool				running;		// if 1, then task is still running
	bool				paused;			// if 1, then task is paused
	bool				must_pause;		// if 1, then task should pause
	struct skink_service_info	*service;		// the service to which this task relates to
	skin_rt_task			task;			// the RT task identifier of this task
	bool				created;		// if 1, then task is created and should be deleted on cleanup
	void				*user_data;
} skink_service_data;

typedef struct skink_service_info
{
	uint32_t			id;			// id of the service (equal to index in array of services)
	skin_rt_mutex			mutex;			// the mutex used to access the service. TODO: is this needed?
	skin_rt_rwlock			rwlock;			// the reader-writer lock used if service is periodic
	skin_rt_semaphore		service_request,	// the semaphores used if the service is sporadic
					service_response;
	void				*memory;		// working memory of service
	char				memory_name[SKIN_RT_MAX_NAME_LENGTH + 1];	// names of the service memory
	char				request_name[SKIN_RT_MAX_NAME_LENGTH + 1];	// semaphores
	char				response_name[SKIN_RT_MAX_NAME_LENGTH + 1];	// and
	char				rwlock_name[SKIN_RT_MAX_NAME_LENGTH + 1];	// rwlock to use for cleanup
	enum skink_service_state
	{
					SKINK_SERVICE_STATE_UNUSED		= 0,	// Not used
					SKINK_SERVICE_STATE_INITIALIZING	= 1,	// Still initializing
					SKINK_SERVICE_STATE_INITIALIZED		= 2,	// Initialized, but not yet working
					SKINK_SERVICE_STATE_BAD			= 3,	// (currently not used) Bad service.  Will not be used
					SKINK_SERVICE_STATE_PAUSED		= 4,	// Paused service.  Unlike drivers, services should not `rmmod` when paused
					SKINK_SERVICE_STATE_WORKING		= 5	// Working service.  Service should NOT `rmmod` no matter what
	}				state;			// state of the service
} skink_service_info;

void skink_initialize_services(void);
void skink_free_services(void);

#endif /* __KERNEL__ */

#endif
