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

#ifndef SKINK_SERVICES_H
#define SKINK_SERVICES_H

#include "skink_common.h"
#include "skink_rt.h"
#include "skink_sensor_layer.h"
#include "skink_sensor.h"
#include "skink_module.h"
#include "skink_patch.h"
#include "skink_sub_region.h"
#include "skink_region.h"

// The following are information about the service itself and are internal to Skinware
#define					SKINK_SERVICE_HEADER_SIZE			6
#define					SKINK_SERVICE_PERIOD(result)			(*(((uint64_t *)(result)) - 1))		// if periodic
#define					SKINK_SERVICE_TIMESTAMP(result)			(*(((uint64_t *)(result)) - 2))
#define					SKINK_SERVICE_PERIODIC				1
#define					SKINK_SERVICE_SPORADIC				0
#define					SKINK_SERVICE_MODE(result)			(*(((uint64_t *)(result)) - 3))
#define					SKINK_SERVICE_IS_PERIODIC(result)		(SKINK_SERVICE_MODE(result) == SKINK_SERVICE_PERIODIC)
#define					SKINK_SERVICE_IS_SPORADIC(result)		(SKINK_SERVICE_MODE(result) == SKINK_SERVICE_SPORADIC)
#define					SKINK_SERVICE_RESULT_COUNT(result)		(*(((uint64_t *)(result)) - 4))
#define					SKINK_SERVICE_MEM_SIZE(result)			(*(((uint64_t *)(result)) - 5))
#define					SKINK_SERVICE_ALIVE				1
#define					SKINK_SERVICE_DEAD				0
#define					SKINK_SERVICE_STATUS(result)			(*(((uint64_t *)(result)) - 6))
#define					SKINK_SERVICE_IS_ALIVE(result)			(SKINK_SERVICE_STATUS(result) == SKINK_SERVICE_ALIVE)
#define					SKINK_SERVICE_IS_DEAD(result)			(SKINK_SERVICE_STATUS(result) == SKINK_SERVICE_DEAD)

#ifdef __KERNEL__

// API for creating services

#define					SKINK_ALL_LAYERS				((skink_sensor_layer_id)SKINK_INVALID_ID)

typedef struct skink_data_structures
{
	skink_sensor_layer		*sensor_layers;
	skink_sub_region		*sub_regions;
	skink_region			*regions;
	skink_sub_region_id		*sub_region_indices;
	skink_region_id			*region_indices;
	skink_sensor_id			*sensor_neighbors;
	skink_sensor			*sensors;			// This array is only for use by indices provided by sensor_neighbors_count
									// All other indices are relative to layer and should go through
									// sensor_layers[*].sensors
	skink_sensor_layer_size		sensor_layers_count;
	skink_sub_region_size		sub_regions_count;
	skink_region_size		regions_count;
	skink_sub_region_index_size	sub_region_indices_count;	// These three array
	skink_region_index_size		region_indices_count;		// counts are useless.
	uint32_t			sensor_neighbors_count;		// Remove them?
} skink_data_structures;

typedef enum skink_acquisition_type
{
	SKINK_ACQUISITION_SPORADIC = 0,			// Which means read is only done when requested (through skink_request_read)
	SKINK_ACQUISITION_PERIODIC,
	SKINK_ACQUISITION_ASAP
} skink_acquisition_type;

typedef void (*skink_service_acquire)(skink_sensor_layer *layer, uint8_t buffer);		// A function assigned to a sensor layer that would be called to feed the
												// service with data of a layer.  At most one function can be assigned to
												// each layer.
typedef void (*skink_service_function)(void *mem, void *user_data);				// A function that will be called by the service thread.  It should fill
												// in the memory in whatever way it sees fit.

int skink_get_data_structures(skink_data_structures *data_structures);				// This function retrieves the data structures used in the skin.
int skink_get_data_structures_nonblocking(skink_data_structures *data_structures);		// It can be used by a service to first inspect the skin, decide how much
												// memory it needs and possibly build some internal structures.
												// Returns either SKINK_SUCCESS or SKINK_FAIL.  This function would block
												// if the skin kernel is still initializing or returns SKINK_TOO_EARLY
												// if nonblocking version of function is called

int skink_initialize_periodic_service(const char *service_name, size_t elem_size, size_t count,	// Initialize either a periodic or sporadic service by allocating shared
					const char *rwl_name, skin_rt_time period);		// memory (of size mem_size and with name service_name).  If the service is
int skink_initialize_sporadic_service(const char *service_name, size_t elem_size, size_t count,	// periodic, a reader-writer lock is created for synchronization.  If the
					const char *request_name, const char *response_name);	// service is sporadic, two semaphores are created.  This function may return
												// a negative value indicating error or a non-negative value which is the
												// service id.  This service id should be used with the rest of the functions.

int skink_acquire_layer(unsigned int service_id, skink_sensor_layer_id layer,			// service_id: The service id returned by skink_initialize_*_service
			skink_service_acquire acquire, skink_acquisition_type mode,		// layer: layer for which acquire is being assigned.  If SKINK_ALL_LAYERS
			skin_rt_time period);							//   is given, the acquire function would be assigned to all layers.
												// acquire: A function called on each cycle
												// mode: One of SKINK_ACQUISITION_*, similar to acquisition methods in
												//   user-space library
												// period: If mode was SKINK_ACQUISITION_PERIODIC then this would be the
												//   task period.  Otherwise it is ignored
												// This function may return SKINK_SUCCESS or SKINK_FAIL if input is invalid

int skink_start_service(unsigned int service_id, skink_service_function function, void *data);	// Spawn reader threads and a service thread calling function
												// either periodically or upon request based on whichever mode the service
												// was initialized with
												// service_id: The service id returned by skink_initialize_*_service
												// function: A function that would be called whenever the service needs to
												//   update its output
												// data: A pointer to be passed to function

int skink_pause_service(unsigned int service_id);						// Pauses reader threads as well as service thread

int skink_resume_service(unsigned int service_id);						// Resumes a paused service

int skink_stop_service(unsigned int service_id);						// Stop the service, removing the service and reader threads.
												// This function blocks until the service is stopped, after which the service
												// module can be removed (if SKINK_SUCCESS is returned).

int skink_request_read(unsigned int service_id, skink_sensor_layer_id layer, bool *must_stop);	// if any of the readers are in sporadic mode, a call to this function would
												// make them read the data (and call their respective "acquire" function once)
												// and it would block until read is complete.
												// if layer is SKINK_ALL_LAYERS, then all layers would be requested at once
												// if must_stop is not NULL, this call will fail if must_stop becomes true

int skink_request_read_nonblocking(unsigned int service_id, skink_sensor_layer_id layer);	// similar to skink_request_read, except only the request is sent.
												// if layer is SKINK_ALL_LAYERS, then all layers would be requested at once

int skink_await_read_response(unsigned int service_id, skink_sensor_layer_id layer, bool *must_stop);
												// this is counter part of skink_request_read_nonblocking which awaits on
												// responses from reader thread.
												// if layer is SKINK_ALL_LAYERS, then waits until responses from all layers
												// arrive.
												// if must_stop is not NULL, this call will fail if must_stop becomes true

// API for using services

typedef struct skink_service
{
	uint32_t			id;
	void				*memory;			// the shared memory with the service
} skink_service;

int skink_lock_service(skink_service *service, bool *must_stop);				// If the service is periodic, a call to this function would lock the
												// service.  skink_unlock_service needs to be called to let the service
												// continue its operation.
												// if must_stop is not NULL, this call will fail if must_stop becomes true

int skink_unlock_service(skink_service *service);						// If skink_lock_service has succeeded, this function needs to be called
												// to allow the service to continue its operation

int skink_request_service(skink_service *service, bool *must_stop);				// If the service is sporadic, a call to this function would request the
												// execution of the service once, and blocks until the service is done
												// if must_stop is not NULL, this call will fail if must_stop becomes true
												// Returns SKINK_FAIL or SKINK_SUCCESS

int skink_request_service_nonblocking(skink_service *service);					// Similar to skink_request_service but doesn't wait on response

int skink_await_service_response(skink_service *service, bool *must_stop);			// The counter part of skink_request_service_nonblocking which awaits on
												// response from service
												// if must_stop is not NULL, this call will fail if must_stop becomes true

int skink_connect_to_service(const char *service_name, skink_service *service);			// Connect to a periodic service with name given as service name.
												// The shared memory of this service will be in service->memory

void skink_disconnect_from_service(skink_service *service);					// Once the need for the service is lifted, call this function to disconnect
												// from it.

/* The structure of a service is basically like this:
init_module:
- Get data structures
- Inspect data structures and figure out needed service memory
- Call skink_initialize_periodic/sporadic_service
  * If a negative value was returned, then fail
- For each layer, if decided so, call skink_acquire_layer
- Call skink_start_service

cleanup_module:
- First, make sure the call to skink_start_service in init phase has returned.  Note that otherwise a race condition happens and the kernel may crash.
- After you are sure your call above to skink_start_service has finished, Call skink_stop_service.
  * If returned SKINK_FAIL, then there has been a bug and probably memory corruption.  It is suggested to go in an infinite loop and demand a restart
- Unload module

## Method 1: Fast services:

variable: service_res

acquire callback:
- Scan the layer and compute relevant part of service_res

service callback:
- Copy service_res to service memory

## Method 2: Slow services:

variable: responses_copy

acquire callback:
- Copy sensor values to responses_copy

service callback:
- Process responses_copy and put the results in service memory
 */

#endif /* __KERNEL__ */

#endif
