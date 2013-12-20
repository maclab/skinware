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
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <asm/div64.h>

#define					SKINK_MODULE_NAME				"skin_motion"
#include "skin_motion.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shahbaz Youssefi");

unsigned int				frequency					= 5;
module_param(frequency, uint, S_IRUGO);
MODULE_PARM_DESC(frequency, " Frequency at which the motion detection is done.  Default value is 5 (Hz)");
int					threshold					= 500;
module_param(threshold, int, S_IRUGO);
MODULE_PARM_DESC(threshold, " The motion detection threshold.  If the difference in sensor values in different times\n"
		"\t\t\tare bigger than this value, a motion is assumed.\n"
		"\t\t\tDefault value is 500");

static skink_data_structures		_skin_data_structures;
static skink_sensor_response		*_previous_responses				= NULL;
static skin_motion			_motions[SKINK_MAX_SENSOR_LAYERS];
static skink_sensor_id			_layer_sensors_start[SKINK_MAX_SENSOR_LAYERS];	// in _previous_responses array, where each layer's sensors start
static skin_rt_mutex			_mutex;

static struct task_struct		*_initialization_task;
static bool				_initialization_task_exited			= false;
static bool				_must_exit					= false;

static int				_service_id					= -1;

// do_div is unsigned.  SIGNED_DIV takes care of negative values too
#if BITS_PER_LONG != 64
#define					SIGNED_DIV(x, y)				({ int64_t _x = (x); int64_t _y = (y); bool neg = false;\
											   if (_x < 0) { _x = -_x; neg = !neg; }\
											   if (_y < 0) { _y = -_y; neg = !neg; }\
											   do_div(_x, _y);\
											   if (neg) _x = -_x;\
											   _x; })
#else
#define					SIGNED_DIV(x, y)				((x) / (y))
#endif

#define ASSIGN(x, y)	do { x[0]  = y[0]; x[1]  = y[1]; x[2]  = y[2]; } while (0)
#define ADD(x, y)	do { x[0] += y[0]; x[1] += y[1]; x[2] += y[2]; } while (0)
#define DIV(x, y)	do { x[0] = SIGNED_DIV(x[0], y); x[1] = SIGNED_DIV(x[1], y); x[2] = SIGNED_DIV(x[2], y); } while (0)

static void acquire(skink_sensor_layer *layer, uint8_t buffer)
{
	skink_sensor_id		i;
	skink_sensor_size	sensors_count;
	int64_t			from[3]		= {0, 0, 0},
				to[3]		= {0, 0, 0};
	int			from_count	= 0,
				to_count	= 0;
	skink_sensor		*sensors	= layer->sensors;
	skink_sensor_response	*prev		= _previous_responses + _layer_sensors_start[layer->id];
	for (i = 0, sensors_count = layer->sensors_count; i < sensors_count; ++i)
	{
		skink_sensor_response r = sensors[i].response[buffer];
		if (r < prev[i] && r < prev[i] - threshold)
		{
			++from_count;
			ADD(from, sensors[i].position_nm);
		}
		else if (r > prev[i] && r > prev[i] + threshold)
		{
			++to_count;
			ADD(to, sensors[i].position_nm);
		}
		prev[i] = r;
	}
	skin_rt_mutex_lock(&_mutex);		// TODO: this mutex is not really needed
	if (to_count == 0 || from_count == 0)
		_motions[layer->id].detected = 0;	// no motion
	else
	{
		DIV(from, from_count);
		DIV(to, to_count);
		ASSIGN(_motions[layer->id].from, from);
		ASSIGN(_motions[layer->id].to, to);
		_motions[layer->id].detected = 1;
	}
	skin_rt_mutex_unlock(&_mutex);
}

static void service(void *mem, void *data)
{
	skin_motion			*res		= mem;
	skink_sensor_layer_id		i;
	skink_sensor_layer_size		layers_count;
	for (i = 0, layers_count = _skin_data_structures.sensor_layers_count; i < layers_count; ++i)
		res[i] = _motions[i];
//	SKINK_RT_LOG("From: %lld %lld %lld", _motions[0].from[0], _motions[0].from[1], _motions[0].from[2]);
//	SKINK_RT_LOG(" To : %lld %lld %lld", _motions[0].to[0],   _motions[0].to[1],   _motions[0].to[2]);
}

#define CLEANUP ((void)0)

#define CHECK_EXIT \
	do {\
		if (_must_exit)\
		{\
			CLEANUP;\
			_initialization_task_exited = true;\
			do_exit(0);\
		}\
	} while (0)

static int init_thread(void *arg)
{
	int ret;
	skink_sensor_layer_id i;
	skink_sensor_size sensors_count;
	while ((ret = skink_get_data_structures_nonblocking(&_skin_data_structures)) == SKINK_TOO_EARLY && !_must_exit)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (!_must_exit && ret != SKINK_SUCCESS)
	{
		SKINK_LOG("Skin motion service module rejected by the skin kernel!");
		_must_exit = true;
	}
	CHECK_EXIT;
	sensors_count = 0;
	for (i = 0; i < _skin_data_structures.sensor_layers_count; ++i)
	{
		_layer_sensors_start[i] = sensors_count;
		sensors_count += _skin_data_structures.sensor_layers[i].sensors_count;
	}
	_previous_responses = vmalloc(sensors_count * sizeof(*_previous_responses));
	if (_previous_responses == NULL)
	{
		SKINK_LOG("Could not allocate internal memory");
		_must_exit = true;
	}
	CHECK_EXIT;
	ret = skink_initialize_periodic_service(SKIN_MOTION_SHMEM, sizeof(skin_motion), _skin_data_structures.sensor_layers_count,
						SKIN_MOTION_RWLOCK, 1000000000 / frequency);
	if (ret < 0)
	{
		SKINK_LOG("Motion service could not be initialized (initialize_periodic_service returned: %d)", ret);
		_must_exit = true;
	}
	else
		_service_id = ret;
	CHECK_EXIT;
	if ((ret = skink_acquire_layer(_service_id, SKINK_ALL_LAYERS, acquire, SKINK_ACQUISITION_PERIODIC, 200000000)) != SKINK_SUCCESS)
		SKINK_LOG("Could not register acquire callbacks: error %d", ret);
	ret = skink_start_service(_service_id, service, NULL);
	if (ret < 0)
	{
		SKINK_LOG("Could not start service: error %d", ret);
		_must_exit = true;
	}
	else if (ret == SKINK_PARTIAL)
		SKINK_LOG("The service is only partially working");
	else
		SKINK_LOG("Service has successfully started");
	_initialization_task_exited = true;
	do_exit(0);
	return 0;		// should never reach
}

static int __init skin_service_init(void)
{
	SKINK_LOG("Skin motion detection module initializing...");
	SKINK_LOG("Using skin kernel version %s", SKINK_VERSION);
	_initialization_task_exited = false;
	_must_exit = false;
	if (skin_rt_mutex_init(&_mutex, 1) == NULL)
	{
		SKINK_LOG("Error initializing internal mutex");
		return -ENOMEM;
	}
	if ((_initialization_task = kthread_run(init_thread, NULL, "skin_motion_init_thread")) == ERR_PTR(-ENOMEM))
	{
		_initialization_task = NULL;
		SKINK_LOG("Error creating the initial thread!");
		SKINK_LOG("Stopping functionality!");
		skin_rt_mutex_delete(&_mutex);
		_initialization_task_exited = true;
		return -ENOMEM;
	}
	SKINK_LOG("Skin motion detection module initializing...done!");
	return 0;
}

static void __exit skin_service_exit(void)
{
	int ret;
	SKINK_LOG("Skin motion detection module exiting...");
	_must_exit = true;
	if (frequency < 1)
	{
		SKINK_LOG("Invalid frequency %u.  Setting it to 1", frequency);
		frequency = 1;
	}
	while (!_initialization_task_exited)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (_service_id >= 0)
		while ((ret = skink_stop_service(_service_id)) != SKINK_SUCCESS)
		{
			SKINK_LOG("Service could not be stopped! error %d", ret);
			if (ret == SKINK_BAD_ID)
			{
				SKINK_LOG("Something must be terribly wrong.  I am going to put cleanup in infinite loop so this message could be seen");
				while (1)
				{
					set_current_state(TASK_INTERRUPTIBLE);
					msleep(1000);
				}
			}
			else	// sleep and retry
			{
				set_current_state(TASK_INTERRUPTIBLE);
				msleep(10);
			}
		}
	skin_rt_mutex_delete(&_mutex);
	if (_previous_responses)
		vfree(_previous_responses);
	SKINK_LOG("Skin motion detection module exiting...done!");
}

module_init(skin_service_init);
module_exit(skin_service_exit);
