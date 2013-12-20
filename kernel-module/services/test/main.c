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

#define					SKINK_MODULE_NAME				"skin_test_service"
#include <skink.h>

#include "skin_test_service.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shahbaz Youssefi");

static skink_sensor_layer_size		_sensor_layers_count				= 0;
// 8 services * layers_count readers.  Each increments _reader_responses[*][*][0]
// the service copies _reader_responses[*][*][0] to [*][*][1], but before checks if they are different
// if they are the same, the reader threads are dead, if not they are alive
static int				_reader_responses[8][SKINK_MAX_SENSOR_LAYERS][2];
// if the service decided that the readers are alive, it increments the number in _service_responses
// the main service then copies these values to the service memory
static int				_service_responses[8];

static struct task_struct		*_initialization_task;
static bool				_initialization_task_exited			= false;
static bool				_must_exit					= false;

static int				_service_ids[9]					= {-1, -1, -1, -1, -1, -1, -1, -1, -1};	// service ids returned by the skin kernel
static skink_service			_services[8]					= {{0}};

void acquire_asap_periodic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[0][layer->id][0];
}

void service_asap_periodic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[0][i][0] == _reader_responses[0][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[0];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[0][i][1] = _reader_responses[0][i][0];
}

void acquire_asap_sporadic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[1][layer->id][0];
}

void service_asap_sporadic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[1][i][0] == _reader_responses[1][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[1];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[1][i][1] = _reader_responses[1][i][0];
}

void acquire_periodic_periodic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[2][layer->id][0];
}

void service_periodic_periodic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[2][i][0] == _reader_responses[2][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[2];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[2][i][1] = _reader_responses[2][i][0];
}

void acquire_periodic_sporadic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[3][layer->id][0];
}

void service_periodic_sporadic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[3][i][0] == _reader_responses[3][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[3];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[3][i][1] = _reader_responses[3][i][0];
}

void acquire_sporadic_periodic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[4][layer->id][0];
}

void service_sporadic_periodic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	skink_request_read(_service_ids[4], SKINK_ALL_LAYERS, &_must_exit);
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[4][i][0] == _reader_responses[4][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[4];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[4][i][1] = _reader_responses[4][i][0];
}

void acquire_sporadic_sporadic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[5][layer->id][0];
}

void service_sporadic_sporadic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	skink_request_read(_service_ids[5], SKINK_ALL_LAYERS, &_must_exit);
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[5][i][0] == _reader_responses[5][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[5];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[5][i][1] = _reader_responses[5][i][0];
}

void acquire_mixed_periodic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[6][layer->id][0];
}

void service_mixed_periodic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	skink_request_read(_service_ids[6], SKINK_ALL_LAYERS, &_must_exit);
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[6][i][0] == _reader_responses[6][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[6];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[6][i][1] = _reader_responses[6][i][0];
}

void acquire_mixed_sporadic(skink_sensor_layer *layer, uint8_t buffer)
{
	++_reader_responses[7][layer->id][0];
}

void service_mixed_sporadic(void *mem, void *data)
{
	uint32_t i;
	bool all_alive = true;
	skink_request_read(_service_ids[7], SKINK_ALL_LAYERS, &_must_exit);
	for (i = 0; i < _sensor_layers_count; ++i)
		if (_reader_responses[7][i][0] == _reader_responses[7][i][1])
			all_alive = false;
	if (all_alive)
		++_service_responses[7];
	for (i = 0; i < _sensor_layers_count; ++i)
		_reader_responses[7][i][1] = _reader_responses[7][i][0];
}

// no acquire thread for the main service

void service_test(void *mem, void *data)
{
	int *res = mem;
	uint32_t i;
	int ret[4];
	int ret2[4];
	// get the sporadic services
	ret[0] = skink_request_service_nonblocking(&_services[1]);
	ret[1] = skink_request_service_nonblocking(&_services[3]);
	ret[2] = skink_request_service_nonblocking(&_services[5]);
	ret[3] = skink_request_service_nonblocking(&_services[7]);
	// note, by using request_nonblocking/await_response the services can be executed in parallel
	if (ret[0] == SKINK_SUCCESS) ret2[0] = skink_await_service_response(&_services[1], &_must_exit);
	if (ret[1] == SKINK_SUCCESS) ret2[1] = skink_await_service_response(&_services[3], &_must_exit);
	if (ret[2] == SKINK_SUCCESS) ret2[2] = skink_await_service_response(&_services[5], &_must_exit);
	if (ret[3] == SKINK_SUCCESS) ret2[3] = skink_await_service_response(&_services[7], &_must_exit);
//	skink_request_service(_service_ids[1], &_must_exit);
//	SKINK_RT_LOG("%d %d %d %d -> %d %d %d %d", ret[0], ret[1], ret[2], ret[3], ret2[0], ret2[1], ret2[2], ret2[3]);
	for (i = 0; i < 8; ++i)
		res[i] = _service_responses[i];
	SKINK_RT_LOG("%d %d %d %d %d %d %d %d", _service_responses[0], _service_responses[1], _service_responses[2], _service_responses[3],
						_service_responses[4], _service_responses[5], _service_responses[6], _service_responses[7]);
}

#define CLEANUP ((void)0)	// TODO: once done, check to see what cleanups need to be done

#define CHECK_EXIT \
	do {\
		if (_must_exit)\
		{\
			CLEANUP;\
			_initialization_task_exited = true;\
			do_exit(0);\
		}\
	} while (0)

#define START_SERVICE(id) \
	do {\
		char mem_name[SKIN_RT_MAX_NAME_LENGTH + 1];\
		char lock1_name[SKIN_RT_MAX_NAME_LENGTH + 1];\
		char lock2_name[SKIN_RT_MAX_NAME_LENGTH + 1];\
		skin_rt_get_free_name(mem_name);\
		skin_rt_get_free_name(lock1_name);\
		if (id & 1) /* then is sporadic */\
		{\
			skin_rt_get_free_name(lock2_name);\
			ret = skink_initialize_sporadic_service(mem_name, 1, 1, lock1_name, lock2_name);\
		}\
		else\
			ret = skink_initialize_periodic_service(mem_name, 1, 1, lock1_name, 40000000);	/* 25Hz */\
		if (ret < 0)\
		{\
			SKINK_LOG("Service %d could not be initialized (initialize_*_service returned: %d)", id, ret);\
			_must_exit = true;\
		}\
		else\
			_service_ids[id] = ret;\
		CHECK_EXIT;\
		for (i = 0; i < _sensor_layers_count; ++i)\
			if (id < 2) /* asap readers */\
			{\
				if ((ret = skink_acquire_layer(_service_ids[id], i, id & 1?acquire_asap_sporadic:acquire_asap_periodic,\
								SKINK_ACQUISITION_ASAP, 0)) != SKINK_SUCCESS)\
					SKINK_LOG("Could not register acquire callback for service %d: error %d", id, ret);\
			}\
			else if (id < 4) /* periodic readers */\
			{\
				if ((ret = skink_acquire_layer(_service_ids[id], i, id & 1?acquire_periodic_sporadic:acquire_periodic_periodic,\
								SKINK_ACQUISITION_PERIODIC, 0)) != SKINK_SUCCESS)\
					SKINK_LOG("Could not register acquire callback for service %d: error %d", id, ret);\
			}\
			else if (id < 6) /* sporadic readers */\
			{\
				if ((ret = skink_acquire_layer(_service_ids[id], i, id & 1?acquire_sporadic_sporadic:acquire_sporadic_periodic,\
								SKINK_ACQUISITION_SPORADIC, 0)) != SKINK_SUCCESS)\
					SKINK_LOG("Could not register acquire callback for service %d: error %d", id, ret);\
			}\
			else /* mixed readers */\
			{\
				if ((ret = skink_acquire_layer(_service_ids[id], i, id & 1?acquire_mixed_sporadic:acquire_mixed_periodic,\
								i % 3 == 0?SKINK_ACQUISITION_ASAP:\
								i % 3 == 1?SKINK_ACQUISITION_PERIODIC:\
									SKINK_ACQUISITION_SPORADIC, 0)) != SKINK_SUCCESS)\
					SKINK_LOG("Could not register acquire callback for service %d: error %d", id, ret);\
			}\
		CHECK_EXIT;\
		switch (id)\
		{\
			case 0: ret = skink_start_service(_service_ids[id], service_asap_periodic, NULL); break;\
			case 1: ret = skink_start_service(_service_ids[id], service_asap_sporadic, NULL); break;\
			case 2: ret = skink_start_service(_service_ids[id], service_periodic_periodic, NULL); break;\
			case 3: ret = skink_start_service(_service_ids[id], service_periodic_sporadic, NULL); break;\
			case 4: ret = skink_start_service(_service_ids[id], service_sporadic_periodic, NULL); break;\
			case 5: ret = skink_start_service(_service_ids[id], service_sporadic_sporadic, NULL); break;\
			case 6: ret = skink_start_service(_service_ids[id], service_mixed_periodic, NULL); break;\
			case 7: ret = skink_start_service(_service_ids[id], service_mixed_sporadic, NULL); break;\
			default: SKINK_LOG("Internal error: I gave myself an invalid id!");\
		}\
		if (ret < 0)\
		{\
			SKINK_LOG("Could not start service %d: error %d", id, ret);\
			_must_exit = true;\
		}\
		else if (ret == SKINK_PARTIAL)\
			SKINK_LOG("Service %d is only partially working", id);\
		else\
			SKINK_LOG("Service %d has successfully started", id);\
		if ((ret = skink_connect_to_service(mem_name, &_services[id])) != SKINK_SUCCESS)\
			SKINK_LOG("Could not connect to service %d: error %d", id, ret);\
	} while (0)

static int init_thread(void *arg)
{
	skink_data_structures skin_ds;
	int ret;
	uint32_t i;
	while ((ret = skink_get_data_structures_nonblocking(&skin_ds)) == SKINK_TOO_EARLY && !_must_exit)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (!_must_exit && ret != SKINK_SUCCESS)
	{
		SKINK_LOG("Skin test service module rejected by the skin kernel!");
		_must_exit = true;
	}
	CHECK_EXIT;
	_sensor_layers_count = skin_ds.sensor_layers_count;
	START_SERVICE(0);
	START_SERVICE(1);
	START_SERVICE(2);
	START_SERVICE(3);
	START_SERVICE(4);
	START_SERVICE(5);
	START_SERVICE(6);
	START_SERVICE(7);
	ret = skink_initialize_periodic_service(SKIN_TEST_SERVICE_SHMEM, 10, 1, SKIN_TEST_SERVICE_RWLOCK, 1000000000/*400000000*/);	// 2.5Hz
	if (ret < 0)
	{
		SKINK_LOG("Main service could not be initialized (initialize_periodic_service returned: %d)", ret);
		_must_exit = true;
	}
	else
		_service_ids[8] = ret;
	CHECK_EXIT;
	ret = skink_start_service(_service_ids[8], service_test, NULL);
	if (ret < 0)
	{
		SKINK_LOG("Could not start main service: error %d", ret);
		_must_exit = true;
	}
	else if (ret == SKINK_PARTIAL)
		SKINK_LOG("The main service is only partially working");
	else
		SKINK_LOG("Main service has successfully started");
	_initialization_task_exited = true;
	do_exit(0);
	return 0;		// should never reach
}

static int __init skin_service_init(void)
{
	SKINK_LOG("Skin test service module initializing...");
	SKINK_LOG("Using skin kernel version %s", SKINK_VERSION);
	_initialization_task_exited = false;
	_must_exit = false;
	if ((_initialization_task = kthread_run(init_thread, NULL, "skin_test_skin_service_init_thread")) == ERR_PTR(-ENOMEM))
	{
		_initialization_task = NULL;
		SKINK_LOG("Error creating the initial thread!");
		SKINK_LOG("Stopping functionality!");
		_initialization_task_exited = true;
		return -ENOMEM;
	}
	SKINK_LOG("Skin test service module initializing...done!");
	return 0;
}

static void __exit skin_service_exit(void)
{
	int ret;
	uint32_t i;
	SKINK_LOG("Skin test service module exiting...");
	_must_exit = true;
	while (!_initialization_task_exited)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	for (i = 0; i < 9; ++i)
		if (_service_ids[i] >= 0)
		{
			skink_disconnect_from_service(&_services[i]);
			if ((ret = skink_stop_service(_service_ids[i])) != SKINK_SUCCESS)
			{
				SKINK_LOG("Service %u could not be stopped! error %d", i, ret);
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
		}
	SKINK_LOG("Skin test service module exiting...done!");
}

module_init(skin_service_init);
module_exit(skin_service_exit);
