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
#include <linux/proc_fs.h>

#include "init.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shahbaz Youssefi");

extern skink_sensor_layer_size		the_layers_count;
extern layer_data			*the_layers;
extern module_data			*the_modules;
extern patch_data			*the_patches;

static struct task_struct		*_initialization_task;
extern bool				the_proc_file_deleted;
extern char				*the_read_buffer;

static bool				_busy						= false;
static bool				_initialization_task_exited			= false;
static bool				_must_exit					= false;

static int				_device_id					= -1;

static void details(skink_sensor_layer *layer)
{
	uint32_t i;
	layer_data *layer_here = the_layers + layer->id_in_device;
	if (the_layers == NULL || the_modules == NULL || the_patches == NULL)
	{
		SKINK_LOG("details called while the device is not properly initialized");
		return;
	}
	strncpy(layer->name, layer_here->name, SKINK_MAX_NAME_LENGTH);
	layer->name[SKINK_MAX_NAME_LENGTH] = '\0';
	layer->sensors_count = layer_here->sensors_count;
	layer->modules_count = layer_here->modules_count;
	layer->patches_count = layer_here->patches_count;
	layer->acquisition_rate = layer_here->acquisition_rate;
	for (i = 0; i < layer_here->modules_count; ++i)
		layer->modules[i].sensors_count = the_modules[layer_here->module_data_begin + i].sensors_count;
	for (i = 0; i < layer_here->patches_count; ++i)
		layer->patches[i].modules_count = the_patches[layer_here->patch_data_begin + i].modules_count;
}

static void busy(bool b)
{
	_busy = b;
}

static int acquire(skink_sensor_layer *layer, skink_sensor_id *skinmap, uint8_t buffer)
{
	skink_sensor_id i;
	skink_sensor_response skin_val;
	skin_rt_time t = skin_rt_get_time();
	char turn_off = 0;
#if BITS_PER_LONG != 64
	skin_rt_time temp;
#endif
	t = (t>>15) + layer->id_in_device * 0x3fff;	// make t & 0xffff about 2 seconds (with an offset to distinguish layers)
#if BITS_PER_LONG != 64
	temp = t;
	skin_val = do_div(temp, SKINK_SENSOR_RESPONSE_MAX);	// changes temp and returns temp % SKINK_SENSOR_RESPONSE_MAX
	turn_off = temp & 1;
#else
	skin_val = t % SKINK_SENSOR_RESPONSE_MAX;
	turn_off = (t / SKINK_SENSOR_RESPONSE_MAX) & 1;
#endif
	if (turn_off)				// make an effect of turning on gradually in 2 seconds and off in another 2 seconds (and in a cycle)
		skin_val = SKINK_SENSOR_RESPONSE_MAX - skin_val;
	for (i = 0; i < layer->sensors_count; ++i)
		layer->sensors[skinmap[i]].response[buffer] = skin_val;
	return SKINK_SUCCESS;
}

static int init_thread(void *arg)
{
	skink_registration_data data;
	int ret;
	uint32_t i;
	skink_sensor_size sensors_count = 0;
	skink_module_size modules_count = 0;
	skink_patch_size patches_count = 0;
	SKINK_LOG("Init thread: Stage 1...");
	// Stage 1: Data initialization
	if (init_data())
	{
		SKINK_LOG("Stage 1: Skin virtual device module stopping due to error in data initialization!");
		_initialization_task_exited = true;
		do_exit(0);
		return 0;
	}
	for (i = 0; i < the_layers_count; ++i)
	{
		modules_count += the_layers[i].modules_count;
		patches_count += the_layers[i].patches_count;
		sensors_count += the_layers[i].sensors_count;
	}
	SKINK_LOG("Stage 1: Detected %u sensors in %u modules in %u patches", (unsigned int)sensors_count, (unsigned int)modules_count, (unsigned int)patches_count);
	SKINK_LOG("Init thread: Stage 1...done");
	SKINK_LOG("Init thread: Stage 2...");
	// Stage 2: registering device
	data.sensor_layers_count = the_layers_count;
	data.sensors_count = sensors_count;
	data.modules_count = modules_count;
	data.patches_count = patches_count;
	data.details_callback = details;
	data.busy_callback = busy;
	data.acquire_callback = acquire;
	data.device_name = SKINK_MODULE_NAME;
	while (!_must_exit)
	{
		bool revived;
		ret = skink_device_register_nonblocking(&data, &revived);
		if (ret >= 0)
		{
			if (revived)
				SKINK_LOG("Stage 2: Revived!");
			_device_id = ret;
			break;
		}
		else if (ret == SKINK_IN_USE)
		{
			SKINK_LOG("Stage 2: Skin virtual device module stopping because another device with same name is in use!");
			goto exit_done;
		}
		else if (ret == SKINK_TOO_LATE)
		{
			SKINK_LOG("Stage 2: Skin virtual device module stopping because it is introduced to the skin kernel too late!");
			goto exit_done;
		}
		else if (ret == SKINK_BAD_DATA)
		{
			SKINK_LOG("Stage 3: Skin virtual device module stopping because Skinware rejected its registration data!");
			goto exit_done;
		}
		else if (ret == SKINK_TOO_EARLY)
		{
			set_current_state(TASK_INTERRUPTIBLE);
			msleep(1);
			// And retry
		}
		else
		{
			SKINK_LOG("Stage 2: Got unknown return value from Skinware: %d!", ret);
			goto exit_done;
		}
	}
	if (!_must_exit && _device_id >= 0)
	{
		if (skink_device_resume(_device_id) != SKINK_SUCCESS)
			SKINK_LOG("Stage 2: Error - Failed to resume network");
		else
			SKINK_LOG("Init thread: Stage 2...done");
	}
exit_done:
	_initialization_task_exited = true;
	do_exit(0);
	return 0;		// should never reach
}

static int __init skin_driver_init(void)
{
	SKINK_LOG("Skin virtual device module initializing...");
	SKINK_LOG("Using skin kernel version %s", SKINK_VERSION);
	_busy = false;
	_initialization_task_exited = false;
	_must_exit = false;
	the_proc_file_deleted = true;
	if ((_initialization_task = kthread_run(init_thread, NULL, "skin_virtual_init_thread")) == ERR_PTR(-ENOMEM))
	{
		_initialization_task = NULL;
		SKINK_LOG("Error creating the initial thread!");
		SKINK_LOG("Stopping functionality!");
		_initialization_task_exited = true;
		return -ENOMEM;
	}
	SKINK_LOG("Skin virtual device module initializing...done!");
	return 0;
}

static void __exit skin_driver_exit(void)
{
	SKINK_LOG("Skin virtual device module exiting...");
	_must_exit = true;
	while (!_initialization_task_exited)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (_device_id >= 0)
		skink_device_pause(_device_id);
	if (!the_proc_file_deleted)
	{
		remove_proc_entry(PROCFS_ENTRY, NULL);
		the_proc_file_deleted = true;
	}
	if (the_read_buffer)
		vfree(the_read_buffer);
	if (_busy)
		SKINK_LOG("Still in use by Skinware.  Waiting...");
	while (_busy)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	free_data();
	SKINK_LOG("Skin virtual device module exiting...done!");
}

module_init(skin_driver_init);
module_exit(skin_driver_exit);
