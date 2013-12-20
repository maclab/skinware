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
#include "skink_devices_internal.h"
#include "skink_shared_memory_ids.h"

extern int				skink_buffer_count;

extern skink_misc_datum			*the_misc_data;
extern skink_sensor_layer		*the_sensor_layers;
extern skink_sensor			*the_sensors;
extern skink_module			*the_modules;
extern skink_patch			*the_patches;
extern skink_sub_region_id		*the_sub_region_indices;
extern skink_region_id			*the_region_indices;
extern skink_sensor_id			*the_sensor_neighbors;
extern skin_rt_rwlock			*the_sync_locks;
extern skink_state			the_state;
extern skink_writer_data		*the_writer_thread_data;

static skink_device_info		_devices[SKINK_MAX_DEVICES];
struct semaphore			the_devices_mutex;
static unsigned int			_devices_registering_count			= 0;
extern bool				the_devices_resume_possible;

extern const char			*the_registered_devices[];		// sysfs output
extern const char			*the_bad_devices[];			// sysfs output

extern void the_clean_free(void);

EXPORT_SYMBOL_GPL(skink_device_register);
EXPORT_SYMBOL_GPL(skink_device_register_nonblocking);
EXPORT_SYMBOL_GPL(skink_device_pause);
EXPORT_SYMBOL_GPL(skink_device_resume);
EXPORT_SYMBOL_GPL(skink_device_pause_nonblocking);
EXPORT_SYMBOL_GPL(skink_device_resume_nonblocking);
EXPORT_SYMBOL_GPL(skink_copy_from_previous_buffer);

static int _device_register_common(skink_registration_data *data, bool *revived)
{
	uint32_t i;
	int did = -1;
	bool is_new = true;
	if (revived)
		*revived = false;
	if (data == NULL)
	{
		SKINK_LOG("Cannot register device without providing data for it...aborting");
		return SKINK_BAD_DATA;
	}
	if (data->sensor_layers_count == 0 || data->sensors_count == 0 || data->modules_count == 0 || data->patches_count == 0)
	{
		SKINK_LOG("Cannot register device with 0 layers, sensors, modules or patches...aborting");
		return SKINK_BAD_DATA;
	}
	if (data->details_callback == NULL || data->busy_callback == NULL || data->acquire_callback == NULL)
	{
		SKINK_LOG("Cannot register device with incomplete callbacks...aborting");
		return SKINK_BAD_DATA;
	}
	if (data->device_name == NULL)
	{
		SKINK_LOG("Cannot register device without a name...aborting");
		return SKINK_BAD_DATA;
	}
	data->busy_callback(1);		// register must immediately call busy(true) until at least data structures are built
	if (down_interruptible(&the_devices_mutex))
	{
		SKINK_LOG("Register device interrupted by signal...aborting");
		data->busy_callback(0);
		return SKINK_FAIL;
	}
	is_new = true;
	for (i = 0; i < SKINK_MAX_DEVICES; ++i)
		if (_devices[i].state == SKINK_DEVICE_STATE_UNUSED)	// all the rest of this array will also be unused
		{
			did = i;
			break;
		}
		else if (strncmp(_devices[i].name, data->device_name, SKINK_MAX_NAME_LENGTH) == 0)
		{
			did = i;
			is_new = false;
			break;
		}
	if (did < 0)
	{
		SKINK_LOG("Too many devices");
		data->busy_callback(0);
		up(&the_devices_mutex);
		return SKINK_TOO_LATE;
	}
	switch (the_state)
	{
	case SKINK_STATE_INIT:			// shouldn't happen!
		data->busy_callback(0);
		up(&the_devices_mutex);
		return SKINK_TOO_EARLY;
	case SKINK_STATE_READY_FOR_DEVICES:
		if (!is_new)
		{
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_IN_USE;
		}
		if ((uint64_t)SKINK_SENSOR_LAYERS_COUNT + data->sensor_layers_count > (uint64_t)SKINK_SENSOR_LAYER_MAX + 1)
		{
			SKINK_LOG("Too many sensor layers are being registered");
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_TOO_LATE;
		}
		if ((uint64_t)SKINK_SENSORS_COUNT + data->sensors_count > (uint64_t)SKINK_SENSOR_MAX + 1)
		{
			SKINK_LOG("Too many sensors are being registered");
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_TOO_LATE;
		}
		if ((uint64_t)SKINK_MODULES_COUNT + data->modules_count > (uint64_t)SKINK_MODULE_MAX + 1)
		{
			SKINK_LOG("Too many modules are being registered");
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_TOO_LATE;
		}
		if ((uint64_t)SKINK_PATCHES_COUNT + data->patches_count > (uint64_t)SKINK_PATCH_MAX + 1)
		{
			SKINK_LOG("Too many patches are being registered");
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_TOO_LATE;
		}
		++_devices_registering_count;
		break;	// Note: Here is the only case where the switch breaks and the rest of the function executes
	case SKINK_STATE_NO_MORE_DEVICES:
	case SKINK_STATE_DEVICES_INTRODUCED:
	case SKINK_STATE_STRUCTURES_BUILT:
		if (!is_new)
		{
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_IN_USE;
		}
		up(&the_devices_mutex);
		return SKINK_TOO_LATE;
	case SKINK_STATE_THREAD_SPAWNED:
	case SKINK_STATE_CALIBRATING:
	case SKINK_STATE_REGIONALIZING:
	case SKINK_STATE_OPERATIONAL:
		if (is_new)
		{
			data->busy_callback(0);
			up(&the_devices_mutex);
			return SKINK_TOO_LATE;
		}
		if (_devices[did].state == SKINK_DEVICE_STATE_PAUSED)
		{
			// Make sure its the same device being revived (not another device with the same name!)
			skink_sensor_layer_id	lid;
			skink_sensor_size	sensors_count = 0;
			skink_module_size	modules_count = 0;
			skink_patch_size	patches_count = 0;
			if (_devices[did].layers_count != data->sensor_layers_count)
			{
				data->busy_callback(0);
				up(&the_devices_mutex);
				return SKINK_IN_USE;		// That is, it is trying to resume another device!
			}
			for (lid = 0; lid < _devices[did].layers_count; ++lid)
			{
				sensors_count += _devices[did].layers[lid].sensors_count;
				modules_count += _devices[did].layers[lid].modules_count;
				patches_count += _devices[did].layers[lid].patches_count;
			}
			if (sensors_count != data->sensors_count
				|| modules_count != data->modules_count
				|| patches_count != data->patches_count)
			{
				data->busy_callback(0);
				up(&the_devices_mutex);
				return SKINK_IN_USE;		// That is, it is trying to resume another device!
			}
			// Commented the three lines below, because register shouldn't resume
/*			_devices[did].state = SKINK_DEVICE_STATE_WORKING;
			for (lid = 0; lid < _devices[did].layers_count; ++lid)
				_devices[did].writers[lid].must_pause = false;*/
			// Reassign the callback pointers because the driver module may have been loaded in another part of the memory
			_devices[did].details	= data->details_callback;
			_devices[did].busy	= data->busy_callback;
			_devices[did].acquire	= data->acquire_callback;
			if (revived)
				*revived = true;
			data->busy_callback(0);
			the_bad_devices[did] = "";
			up(&the_devices_mutex);
			// TODO: Check to see if the threads are still alive.  If not, recreate them! Should I do it within the lock or outside?
			// If fast, then within just to be sure
			SKINK_LOG("Successfully revived device with name \"%s\"", _devices[did].name);
			return did;
		}
		data->busy_callback(0);
		up(&the_devices_mutex);
		return SKINK_IN_USE;
	case SKINK_STATE_EXITING:
	case SKINK_STATE_FAILED:
		data->busy_callback(0);
		up(&the_devices_mutex);
		return SKINK_TOO_LATE;
	default:
		data->busy_callback(0);
		SKINK_LOG("Register device encountered unknown kernel state: %d", the_state);
		up(&the_devices_mutex);
		return SKINK_TOO_LATE;
	}
	_devices[did].state				= SKINK_DEVICE_STATE_INITIALIZING;
	_devices[did].id				= did;
	_devices[did].layers_count			= data->sensor_layers_count;
	_devices[did].details				= data->details_callback;
	_devices[did].busy				= data->busy_callback;
	_devices[did].acquire				= data->acquire_callback;
	_devices[did].layers				= NULL;
	_devices[did].writers				= NULL;
	strncpy(_devices[did].name, data->device_name, SKINK_MAX_NAME_LENGTH);
	_devices[did].name[SKINK_MAX_NAME_LENGTH]	= '\0';
	SKINK_SENSOR_LAYERS_COUNT			+= data->sensor_layers_count;
	SKINK_SENSORS_COUNT				+= data->sensors_count;
	SKINK_MODULES_COUNT				+= data->modules_count;
	SKINK_PATCHES_COUNT				+= data->patches_count;
	the_registered_devices[did] = _devices[did].name;
	--_devices_registering_count;
	if (the_state == SKINK_STATE_NO_MORE_DEVICES && _devices_registering_count == 0)
		the_state = SKINK_STATE_DEVICES_INTRODUCED;
	up(&the_devices_mutex);
	SKINK_LOG("Successfully registered device with name \"%s\"", _devices[did].name);
	return did;
}

int skink_device_register(skink_registration_data *data, bool *revived)
{
	while (the_state == SKINK_STATE_INIT)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	return _device_register_common(data, revived);
}

int skink_device_register_nonblocking(skink_registration_data *data, bool *revived)
{
	if (the_state == SKINK_STATE_INIT)
		return SKINK_TOO_EARLY;
	return _device_register_common(data, revived);
}

void skink_no_more_devices(void)
{
	if (down_interruptible(&the_devices_mutex))
	{
		SKINK_LOG("Register device finalization interrupted by signal...aborting");
		return;
	}
	the_state = SKINK_STATE_NO_MORE_DEVICES;
	if (_devices_registering_count == 0)
		the_state = SKINK_STATE_DEVICES_INTRODUCED;
	// If count is not zero, then there are one or many registrations taking place.  Register itself will also check for this condition
	// and therefore, as soon as it reaches 0, the state will be set to DEVICES_INTRODUCED
	up(&the_devices_mutex);
}

static int _device_pause(unsigned int dev_id)
{
	uint32_t i;
	if (dev_id >= SKINK_MAX_DEVICES)
		return SKINK_BAD_ID;
	if (_devices[dev_id].state == SKINK_DEVICE_STATE_BAD)
		return SKINK_FAIL;
	if (down_interruptible(&the_devices_mutex))
	{
		SKINK_LOG("pause_me interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	if (_devices[dev_id].state != SKINK_DEVICE_STATE_WORKING
		&& _devices[dev_id].state != SKINK_DEVICE_STATE_INITIALIZED)
	{
		up(&the_devices_mutex);
		return SKINK_FAIL;
	}
	if (_devices[dev_id].writers)		// If failed to initialize, this could be NULL
		for (i = 0; i < _devices[dev_id].layers_count; ++i)
			_devices[dev_id].writers[i].must_pause = true;
	_devices[dev_id].state = SKINK_DEVICE_STATE_PAUSED;
	up(&the_devices_mutex);
	return SKINK_SUCCESS;
}

int skink_device_pause(unsigned int dev_id)
{
	while (the_state < SKINK_STATE_THREAD_SPAWNED && the_state != SKINK_STATE_FAILED)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	return _device_pause(dev_id);
}

int skink_device_pause_nonblocking(unsigned int dev_id)
{
	if (the_state < SKINK_STATE_THREAD_SPAWNED && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	return _device_pause(dev_id);
}

static int _device_resume(unsigned int dev_id)
{
	uint32_t i;
	if (dev_id >= SKINK_MAX_DEVICES)
		return SKINK_BAD_ID;
	if (_devices[dev_id].state == SKINK_DEVICE_STATE_BAD)
		return SKINK_FAIL;
	if (down_interruptible(&the_devices_mutex))
	{
		SKINK_LOG("resume_me interrupted by signal...aborting");
		return SKINK_FAIL;
	}
	if (_devices[dev_id].state != SKINK_DEVICE_STATE_PAUSED
		&& _devices[dev_id].state != SKINK_DEVICE_STATE_INITIALIZED)
	{
		up(&the_devices_mutex);
		return SKINK_FAIL;
	}
	_devices[dev_id].state = SKINK_DEVICE_STATE_WORKING;
	if (the_devices_resume_possible)	// If in the middle of certain operations, such as regionalization, the threads cannot resume
		if (_devices[dev_id].writers)	// If failed to initialize, this could be NULL
			for (i = 0; i < _devices[dev_id].layers_count; ++i)
				_devices[dev_id].writers[i].must_pause = false;
	the_bad_devices[dev_id] = "";
	up(&the_devices_mutex);
	return SKINK_SUCCESS;
}

int skink_device_resume(unsigned int dev_id)
{
	while (the_state < SKINK_STATE_THREAD_SPAWNED && the_state != SKINK_STATE_FAILED)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	return _device_resume(dev_id);
}

int skink_device_resume_nonblocking(unsigned int dev_id)
{
	if (the_state < SKINK_STATE_THREAD_SPAWNED && the_state != SKINK_STATE_FAILED)
		return SKINK_TOO_EARLY;
	if (the_state == SKINK_STATE_FAILED)		// STATE_EXITING cannot happen because Linux doesn't let skin kernel unload while someone is using it
		return SKINK_TOO_LATE;
	return _device_resume(dev_id);
}

void skink_initialize_devices(void)
{
	uint32_t i;
	for (i = 0; i < SKINK_MAX_DEVICES; ++i)
	{
		_devices[i].id			= i;
		_devices[i].details		= NULL;
		_devices[i].busy		= NULL;
		_devices[i].acquire		= NULL;
		_devices[i].layers_count	= 0;
		_devices[i].layers		= NULL;
		_devices[i].writers		= NULL;
		_devices[i].busy_count		= 0;
		_devices[i].name[0]		= '\0';
		_devices[i].state		= SKINK_DEVICE_STATE_UNUSED;
	}
}

#define UNBUSY_ALL \
	do {\
		for (i = 0; i < SKINK_MAX_DEVICES; ++i)\
			if (_devices[i].state == SKINK_DEVICE_STATE_INITIALIZING\
				|| _devices[i].state == SKINK_DEVICE_STATE_INITIALIZED)\
				_devices[i].busy(0);\
	} while (0)

#define SKINK_SHARED_MEMORY_ALLOC(name_c, name_s)\
	do {\
		bool _is_contiguous;\
		the_##name_s = skin_rt_shared_memory_alloc(SKINK_RT_SHARED_MEMORY_NAME_##name_c,\
								SKINK_##name_c##_COUNT * sizeof(*the_##name_s),\
								&_is_contiguous);\
		if (the_##name_s == NULL)\
		{\
			SKINK_LOG("Stage 2: Could not acquire memory for "#name_s);\
			the_clean_free();\
			SKINK_##name_c##_COUNT = 0;\
			UNBUSY_ALL;\
			return SKINK_NO_MEM;\
		}\
		else if (!_is_contiguous)\
			SKINK_LOG("Stage 2: Warning - Using discontiguous memory");	/* Not really a problem.  But if you see this,*/\
											/* you are most likely almost running out of memory */\
	} while (0)

int skink_build_structures(void)
{
	uint32_t			did, i, j, k;
	skink_sensor_layer_size		sensor_layers_so_far	= 0;
	skink_sensor_size		sensors_so_far		= 0;
	skink_module_size		modules_so_far		= 0;
	skink_patch_size		patches_so_far		= 0;
	if (SKINK_SENSOR_LAYERS_COUNT == 0)
	{
		SKINK_LOG("Stage 2: No working devices are attached to the kernel");
		UNBUSY_ALL;
		return SKINK_BAD_DATA;
	}
	SKINK_SHARED_MEMORY_ALLOC(SENSOR_LAYERS, sensor_layers);
	SKINK_SHARED_MEMORY_ALLOC(SENSORS, sensors);
	SKINK_SHARED_MEMORY_ALLOC(MODULES, modules);
	SKINK_SHARED_MEMORY_ALLOC(PATCHES, patches);
	the_writer_thread_data = vmalloc(SKINK_SENSOR_LAYERS_COUNT * sizeof(*the_writer_thread_data));
	if (!the_writer_thread_data)
	{
		SKINK_LOG("Stage 2: Insufficient memory");
		UNBUSY_ALL;
		return SKINK_NO_MEM;
	}
	for (did = 0; did < SKINK_MAX_DEVICES; ++did)
		if (_devices[did].state == SKINK_DEVICE_STATE_INITIALIZING)		// For every registered device
		{
			_devices[did].layers	= the_sensor_layers + sensor_layers_so_far;
			_devices[did].writers	= the_writer_thread_data + sensor_layers_so_far;
			for (i = 0; i < _devices[did].layers_count; ++i)			// For every layer in that device
			{
				skink_sensor_size	local_sensors_so_far	= 0;
				skink_module_size	local_modules_so_far	= 0;
				skink_sensor_layer	*layer			= the_sensor_layers + sensor_layers_so_far;
				layer->id			= sensor_layers_so_far;
				layer->id_in_device		= i;
				layer->number_of_buffers	= skink_buffer_count;
				layer->sensor_map		= NULL;
				layer->sensors			= the_sensors + sensors_so_far;
				layer->modules			= the_modules + modules_so_far;
				layer->patches			= the_patches + patches_so_far;
				_devices[did].details(layer);				// Get the details of sensor/module/patch structures
				if (layer->acquisition_rate == 0)
				{
					SKINK_LOG("Stage 2: Cannot have acquisition rate of 0 for a sensor layer.  Defaulting to 1Hz");
					layer->acquisition_rate = 1;
				}
				if (layer->number_of_buffers == 0)
				{
					SKINK_LOG("Stage 2: Cannot have 0 buffers.  Setting to default value");
					layer->number_of_buffers = skink_buffer_count;
				}
				if (layer->number_of_buffers > SKINK_MAX_BUFFERS)
				{
					SKINK_LOG("Stage 2: Cannot have more than %d buffers.  Capping to the maximum value", SKINK_MAX_BUFFERS);
					layer->number_of_buffers = SKINK_MAX_BUFFERS;
				}
				if (layer->sensors_count == 0 || layer->modules_count == 0 || layer->patches_count == 0)
				{
					SKINK_LOG("Stage 2: Cannot have a layer with 0 sensors, modules or patches");
					_devices[did].busy(0);
					// make the device state bad, so that data acquisition wouldn't start on it, yet clean up would clean it!
					_devices[did].state = SKINK_DEVICE_STATE_BAD;
					break;
				}
				local_sensors_so_far = 0;
				for (j = 0; j < layer->modules_count; ++j)			// Update some indices
				{
					layer->modules[j].layer		= layer->id;
					layer->modules[j].sensors_begin	= local_sensors_so_far;
					local_sensors_so_far		+= layer->modules[j].sensors_count;
					for (k = 0; k < layer->modules[j].sensors_count; ++k)
						layer->sensors[layer->modules[j].sensors_begin + k].module = j;
				}
				local_modules_so_far = 0;
				for (j = 0; j < layer->patches_count; ++j)
				{
					layer->patches[j].layer		= layer->id;
					layer->patches[j].modules_begin	= local_modules_so_far;
					local_modules_so_far		+= layer->patches[j].modules_count;
					for (k = 0; k < layer->patches[j].modules_count; ++k)
						layer->modules[layer->patches[j].modules_begin + k].patch = j;
				}
				the_writer_thread_data[sensor_layers_so_far].device	= &_devices[did];
				the_writer_thread_data[sensor_layers_so_far].layer	= i;
				the_writer_thread_data[sensor_layers_so_far].created	= false;
				++sensor_layers_so_far;
				sensors_so_far += layer->sensors_count;
				modules_so_far += layer->modules_count;
				patches_so_far += layer->patches_count;
			}
			if (!skin_rt_mutex_init(&_devices[did].mutex, 1))			// Get a mutex for the device also
			{
				SKINK_LOG("Stage 2: Could not acquire internal lock for device with name \"%s\"", _devices[did].name);
				_devices[did].busy(0);
				// make the device state bad, so that data acquisition wouldn't start on it, yet clean up would clean it!
				_devices[did].state = SKINK_DEVICE_STATE_BAD;
			}
			else
				_devices[did].state = SKINK_DEVICE_STATE_INITIALIZED;
		}
	// Update any other possible indices
	SKINK_SENSOR_NEIGHBORS_COUNT = 0;
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		skink_sensor_layer *layer = the_sensor_layers + i;
		for (j = 0; j < layer->sensors_count; ++j)
		{
			layer->sensors[j].id = j;
			layer->sensors[j].layer = i;
		}
		for (j = 0; j < layer->modules_count; ++j)
			layer->modules[j].id = j;
		for (j = 0; j < layer->patches_count; ++j)
			layer->patches[j].id = j;
	}
	for (i = 0; i < SKINK_SENSORS_COUNT; ++i)
	{
		the_sensors[i].sub_region		= 0;
		the_sensors[i].neighbors_begin		= 0;
		the_sensors[i].neighbors_count		= 0;
		for (j = 0; j < SKINK_MAX_BUFFERS; ++j)
			the_sensors[i].response[j]	= 0;
	}
	// Allocate internal memories
	the_sync_locks = vmalloc(SKINK_SENSOR_LAYERS_COUNT * SKINK_MAX_BUFFERS * sizeof(*the_sync_locks));
	if (the_sync_locks == NULL)						// The space allocated is possibly bigger than the number of buffers
	{									// in all layers, but allocated like this gives the benefit of
		SKINK_LOG("Stage 2: Could not acquire sync lock memory");	// aligning each layer's locks, making it much easier to locate them
		the_clean_free();
		UNBUSY_ALL;
		return SKINK_NO_MEM;
	}
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		skink_sensor_layer *layer = the_sensor_layers + i;
		if (layer->sensors_count == 0)		// then the layer corresponds to a bad device
			continue;
		layer->sensor_map = vmalloc(layer->sensors_count * sizeof(*layer->sensor_map));
		if (layer->sensor_map == NULL)
		{
			SKINK_LOG("Stage 2: Insufficient memory");
			the_clean_free();
			UNBUSY_ALL;
			return SKINK_NO_MEM;
		}
		for (j = 0; j < layer->sensors_count; ++j)
			layer->sensor_map[j] = j;
	}
	// Get and share synchronization locks
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		skin_rt_rwlock *sync_locks = the_sync_locks + SKINK_MAX_BUFFERS * i;
		uint8_t number_of_buffers = the_sensor_layers[i].number_of_buffers;
		if (number_of_buffers > 1)
			for (j = 0; j < number_of_buffers; ++j)
			{
				char rwl_name[10];
				skink_get_rwlock_name(rwl_name, i, j);
				if (skin_rt_rwlock_init_and_share(sync_locks + j, rwl_name) == NULL)
				{
					SKINK_RT_LOG("Stage 2: Failed to acquire or share readers-writers lock for synchronization");
					for (k = 0; k < j; ++k)
					{
						skink_get_rwlock_name(rwl_name, i, k);
						skin_rt_rwlock_delete(sync_locks + k);
						skin_rt_unshare_rwlock(rwl_name);
					}
					the_clean_free();
					UNBUSY_ALL;
					return SKINK_FAIL;
				}
			}
	}
	UNBUSY_ALL;		// From now on, the devices can unload and load again
	return SKINK_SUCCESS;
}

void skink_free_devices(void)
{
	uint32_t i;
	for (i = 0; i < SKINK_MAX_DEVICES; ++i)
		if (_devices[i].state != SKINK_DEVICE_STATE_UNUSED)
		{
			if (_devices[i].state != SKINK_DEVICE_STATE_BAD
				&& _devices[i].state != SKINK_DEVICE_STATE_PAUSED)
				_devices[i].busy(0);
			if (_devices[i].state != SKINK_DEVICE_STATE_BAD)
				skin_rt_mutex_delete(&_devices[i].mutex);
			_devices[i].state		= SKINK_DEVICE_STATE_UNUSED;
			_devices[i].layers_count	= 0;
			_devices[i].layers		= NULL;
			_devices[i].details		= NULL;
			_devices[i].busy		= NULL;
			_devices[i].busy_count		= 0;
			_devices[i].acquire		= NULL;
			_devices[i].writers		= NULL;
			_devices[i].name[0]		= '\0';
		}
}

void skink_copy_from_previous_buffer(skink_sensor_layer *layer)
{
	skink_sensor_id i;
	uint8_t cur_buf, prev_buf;
	skink_sensor *sensors;
	skink_sensor_size count;
	if (layer == NULL || layer->sensors == NULL)
		return;
	// if a swap skip, then shouldn't copy, because the previous frame has already updated current buffer
	// a swap skip happens if write time of previous buffer + period is smaller than current time
	// TODO: perhaps, add a threshold?
	if (layer->write_time[layer->last_written_buffer] + 1000000000 / layer->acquisition_rate > skin_rt_get_time())
		return;
	cur_buf = layer->buffer_being_written;
	prev_buf = layer->last_written_buffer;
	count = layer->sensors_count;
	sensors = layer->sensors;
	for (i = 0; i < count; ++i)
		sensors[i].response[cur_buf] = sensors[i].response[prev_buf];
}
