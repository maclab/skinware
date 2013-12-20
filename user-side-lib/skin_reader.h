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

#ifndef SKIN_DATA_ACQUISITION_H
#define SKIN_DATA_ACQUISITION_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include <stdbool.h>
#include "skin_common.h"
#include "skin_kernel.h"			/* kernel module specific data types, definitions and shared memory names */

#define SKIN_ALL_SENSOR_TYPES ((skin_sensor_type_id)SKIN_INVALID_ID)

SKIN_DECL_BEGIN

typedef enum skin_acquisition_type
{
	SKIN_ACQUISITION_SPORADIC = 0,		/* data acquired only upon request (through skin_reader_request) */
	SKIN_ACQUISITION_PERIODIC,		/* data acquired at certain periods */
	SKIN_ACQUISITION_ASAP			/* data acquired as soon as possible */
} skin_acquisition_type;

struct skin_object;
struct skin_reader_data;

typedef struct skin_task_statistics
{
	skin_rt_time			worst_read_time;
	uint64_t			number_of_reads;
	skin_rt_time			best_read_time;
	skin_rt_time			accumulated_read_time;

	/* internal */
	skin_rt_time			p_swap_protection_time;
} skin_task_statistics;

typedef struct skin_reader skin_reader;

/*
 * skin_reader manages the readers.  In all these functions, unless stated otherwise, SKIN_SUCCESS would be returned if everything is ok,
 * SKIN_PARTIAL if SKIN_ALL_SENSOR_TYPES given and the operation for some of those failed, SKIN_BAD_ID if invalid id is given or
 * SKIN_FAIL if other failures.
 *
 * init				initialize the reader, but it wouldn't still work until initialized by skin_object
 * free				stop the readers and release resources
 * start			start acquisition of a layer in a given mode.  This function is either given SKIN_ALL_SENSOR_TYPES
 *				to start acquisition from all sensor types or a specific sensor type id.  The acquisition mode is
 *				a choice between ASAP, PERIODIC and SPORADIC.  If periodic, the period will also be taken into account,
 *				which is a value in nanoseconds
 *				More detail on why it failed would be logged.
 *				Note: There are some stuff inside that need to be done in real-time mode.  Thus, if the thread
 *				calling it is not already real-time, it would become real-time temporarily and turned back
 *				to normal before this function returns
 * request			if in sporadic mode, call this function to request acquisition for one or all sensor types
 * request_nonblocking		similar to request, but only sends the request and doesn't wait on response.  After a call to this
 *				function, await_response needs to be called
 * await_response		complete the request after a call to request_nonblocking.  If must_stop is NULL it will ignored.
 *				Otherwise, it can be used to cancel the await_response operation (from another thread)
 * stop				stop data acquisition for one or all sensor types
 *				More detail on why it failed would be logged
 * pause			pause acquisition of one or all sensor types
 * resume			resume acquisition of one or all sensor types
 * is_paused			if SKIN_ALL_SENSOR_TYPES is given, returns true if all sensor types are being acquired, or false if at
 *				least one is not being acquired.  If a specific sensor type is given, it would check whether that
 *				sensor type is being acquired.  SKIN_FAIL or SKIN_BAD_ID would be returned otherwise
 * register_user		register a mutex for one or all sensor types so that whenever a read is completed, the user waiting for
 *				the read could be awakened.  The user would call wait_read and wait until awakened.  This way, it can be
 *				guaranteed that the user of data always works _after_ the data are read.  Note that this doesn't mean
 *				_another_ read could not have been started.
 *				Note: these mutexes are only worked with in ASAP and PERIODIC modes, since in SPORADIC mode the user would
 *				already know when the read is done.
 * wait_read			helper function which can wait on a registered mutex (with the possibility to cancel the wait), try-wait it,
 *				or timed-wait it.  If time parameter is (skin_rt_time)-1, then it will wait until mutex is unlocked.
 *				If time is 0, try-locks the mutex and if time > 0, timed-locks the mutex.
 *				If must_stop is NULL, it will be ignored
 * unregister_user		removes the mutex from the list of registered mutexes
 * enable_swap_skip_prediction	a function used in analysis.  Enables swap skip prediction and avoidance (even for running threads)
 * disable_swap_skip_prediction	a function used in analysis.  Disables swap skip prediction and avoidance (even for running threads)
 */
void skin_reader_init(skin_reader *reader);
void skin_reader_free(skin_reader *reader);
int skin_reader_start(skin_reader *reader, skin_sensor_type_id sensor_type, skin_acquisition_type t, skin_rt_time period);
int skin_reader_request(skin_reader *reader, skin_sensor_type_id sensor_type, bool *must_stop);
int skin_reader_request_nonblocking(skin_reader *reader, skin_sensor_type_id sensor_type);
int skin_reader_await_response(skin_reader *reader, skin_sensor_type_id sensor_type, bool *must_stop);
int skin_reader_stop(skin_reader *reader, skin_sensor_type_id sensor_type);
int skin_reader_pause(skin_reader *reader, skin_sensor_type_id sensor_type);
int skin_reader_resume(skin_reader *reader, skin_sensor_type_id sensor_type);
int skin_reader_is_paused(skin_reader *reader, skin_sensor_type_id sensor_type);
int skin_reader_register_user(skin_reader *reader, skin_rt_mutex *mutex, skin_sensor_type_id sensor_type);
int skin_reader_wait_read(skin_reader *reader, skin_rt_mutex *mutex, skin_rt_time wait_time, bool *must_stop);
int skin_reader_unregister_user(skin_reader *reader, skin_rt_mutex *mutex, skin_sensor_type_id sensor_type);
void skin_reader_enable_swap_skip_prediction(skin_reader *reader);
void skin_reader_disable_swap_skip_prediction(skin_reader *reader);

struct skin_reader
{
	skin_task_statistics		*tasks_statistics;			/* array of statistics, one for each task */
	skin_sensor_type_size		tasks_count;				/*
										 * number of running tasks, one for each layer.  This should be
										 * the same as count after a call to skin_object_sensor_types(&count)
										 */

	/* internal */
	skink_sensor_layer		*p_kernel_sensor_layers;		/* shared memory with kernel module */
	skink_sensor			*p_kernel_sensors;			/* shared memory with kernel module */
	skink_misc_datum		*p_kernel_misc_data;			/* shared memory with kernel module */
	struct skin_reader_data		*p_readers_data;			/* data on reader task.  Structure defined in skin_reader.cpp */
	struct skin_object		*p_object;
	void				*p_registered_users;
	skin_rt_mutex			*p_registered_users_mutex;		/* mutex for handling work with p_registered_users */
	bool				p_enable_swap_skip_prediction;

#ifdef SKIN_CPP
	skin_reader() { skin_reader_init(this); }
	~skin_reader() { skin_reader_free(this); }
	int start(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES, skin_acquisition_type t = SKIN_ACQUISITION_ASAP, skin_rt_time period = 0)
			{ return skin_reader_start(this, sensor_type, t, period); }
	int request(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES, bool *must_stop = NULL)
			{ return skin_reader_request(this, sensor_type, must_stop); }
	int request_nonblocking(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES)
			{ return skin_reader_request_nonblocking(this, sensor_type); }
	int await_response(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES, bool *must_stop = NULL)
			{ return skin_reader_await_response(this, sensor_type, must_stop); }
	int stop(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES) { return skin_reader_stop(this, sensor_type); }
	int pause(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES) { return skin_reader_pause(this, sensor_type); }
	int resume(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES) { return skin_reader_resume(this, sensor_type); }
	int is_paused(skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES) { return skin_reader_is_paused(this, sensor_type); }
	int register_user(skin_rt_mutex *mutex, skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES)
			{ return skin_reader_register_user(this, mutex, sensor_type); }
	int wait_read(skin_rt_mutex *mutex, skin_rt_time wait_time = (skin_rt_time)-1, bool *must_stop = NULL)
			{ return skin_reader_wait_read(this, mutex, wait_time, must_stop); }
	int unregister_user(skin_rt_mutex *mutex, skin_sensor_type_id sensor_type = SKIN_ALL_SENSOR_TYPES)
			{ return skin_reader_unregister_user(this, mutex, sensor_type); }
	void enable_swap_skip_prediction() { skin_reader_enable_swap_skip_prediction(this); }
	void disable_swap_skip_prediction() { skin_reader_disable_swap_skip_prediction(this); }
#endif
};

SKIN_DECL_END

#endif
