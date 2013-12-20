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

#ifndef SKIN_SERVICES_INTERNAL_H
#define SKIN_SERVICES_INTERNAL_H

#include "skin_services.h"

struct skin_service_info;

typedef struct skin_service_data
{
	skin_service_function		function;		/* callback function to let the service write its data */
	bool				must_stop;		/* if 1, task will quit */
	bool				running;		/* if 1, then task is still running */
	bool				paused;			/* if 1, then task is paused */
	bool				must_pause;		/* if 1, then task should pause */
	struct skin_service_info	*service;		/* the service to which this task relates to */
	skin_rt_task			*task;			/* the RT task identifier of this task */
	skin_rt_task_id			task_id;		/* id of the task */
	bool				created;		/* if 1, then task is created and should be deleted on cleanup */
	void				*user_data;
} skin_service_data;

typedef enum skin_service_state
{
					SKIN_SERVICE_STATE_UNUSED		= 0,	/* Not used */
					SKIN_SERVICE_STATE_INITIALIZING		= 1,	/* Still initializing */
					SKIN_SERVICE_STATE_INITIALIZED		= 2,	/* Initialized, but not yet working */
					SKIN_SERVICE_STATE_BAD			= 3,	/* (currently not used) Bad service.  Will not be used */
					SKIN_SERVICE_STATE_PAUSED		= 4,	/* Paused service */
					SKIN_SERVICE_STATE_WORKING		= 5	/* Working service */
} skin_service_state;

typedef struct skin_service_info
{
	uint32_t			id;			/* id of the service (equal to index in array of services) */
	skin_service_manager		*manager;
	skin_rt_mutex			*mutex;			/* the mutex used to access the service. TODO: is this needed? */
	skin_rt_rwlock			*rwlock;		/* the reader-writer lock used if service is periodic */
	skin_rt_semaphore		*request,		/* the semaphores used if the service is sporadic */
					*response;
	void				*memory;		/* working memory of service */
	char				memory_name[SKIN_RT_MAX_NAME_LENGTH + 1];	/* names of the service memory for cleanup */
	skin_service_data		task_data;		/* data specific to the service task */
	skin_service_state		state;			/* state of the service */
} skin_service_info;

#endif
