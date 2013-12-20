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
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>

#define					SKINK_MODULE_NAME				"skin_kernel"

#include "skink_internal.h"
#include "skink_common.h"
#include "skink_sensor_layer.h"
#include "skink_sensor.h"
#include "skink_region.h"
#include "skink_sub_region.h"
#include "skink_module.h"
#include "skink_patch.h"
#include "skink_rt.h"
#include "skink_shared_memory_ids.h"
#include "skink_writer_internal.h"
#include "skink_devices_internal.h"
#include "skink_services_internal.h"
#include "skink_sysfs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shahbaz Youssefi");

int					skink_buffer_count				= 3;
module_param(skink_buffer_count, int, S_IRUGO);
MODULE_PARM_DESC(skink_buffer_count, " If this value is set to 1, the kernel module will work in single buffer mode, not using synchronization.\n"
		"\t\t\tIf bigger than zero, the kernel will use skink_buffer_count buffers to write sensor data (not necessarily alternatively."
		" See documentations).\n"
		"\t\t\tDefault value is 3.  Minimum value is 1.  Maximum value is "SKINK_STRINGIFY(SKINK_MAX_BUFFERS));

// Shared memory
skink_misc_datum			*the_misc_data					= NULL;
skink_sensor_layer			*the_sensor_layers				= NULL;
skink_sensor				*the_sensors					= NULL;
skink_module				*the_modules					= NULL;
skink_patch				*the_patches					= NULL;
skink_sub_region			*the_sub_regions				= NULL;
skink_region				*the_regions					= NULL;
skink_sub_region_id			*the_sub_region_indices				= NULL;
skink_region_id				*the_region_indices				= NULL;
skink_sensor_id				*the_sensor_neighbors				= NULL;
// yromem derahS

// Internal variables
static struct task_struct		*_init_thread;
enum skink_state			the_state					= SKINK_STATE_INIT;
extern struct semaphore			the_devices_mutex;
extern struct semaphore			the_services_mutex;
extern skin_rt_rwlock			*the_sync_locks;
bool					the_devices_resume_possible;
// selbairav lanretnI

// Module internal cleanup flags
static int				_rt_stop_flags					= 0;
static int				_init_thread_exited				= 1;
// sgalf punaelc lanretni eludoM

extern char				the_phase[][10];		// sysfs output.  Roughly corresponds to the_state

static void _pre_cleanup(void)
{
	// This function makes sure no shared memory or lock reserved ids are in use.  If they are, it attempts to remove them and hope nothing breaks.
	// This is to be able to recover from a crash or unpredicted error in cleanup where residue remains in the system.
	uint32_t i, j;
#define CHECK_NAME(x) \
	do {\
		while (!skin_rt_name_available(x))\
		{\
			SKINK_LOG("Stage 0: Bad clean up or abuse of reserved name %s", x);\
			skin_rt_make_name_available(x);		/* I'm not sure if this frees memory (if object was memory) */\
		}\
	} while (0)
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_SENSORS);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGIONS);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_REGIONS);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_MODULES);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_PATCHES);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGION_INDICES);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_REGION_INDICES);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
	CHECK_NAME(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	for (i = 0; i < SKINK_MAX_SENSOR_LAYERS; ++i)
		for (j = 0; j < skink_buffer_count; ++j)
		{
			char rwl_name[SKIN_RT_MAX_NAME_LENGTH + 1];
			skink_get_rwlock_name(rwl_name, i, j);
			CHECK_NAME(rwl_name);
		}
#undef CHECK_NAME
}

void the_clean_free(void)
{
// CLEAN_(V)FREE: name_c, name_s the same name in first capital, second small
#define CLEAN_FREE(name_c, name_s)\
	do {\
		if (the_##name_s)\
			skin_rt_shared_memory_free(SKINK_RT_SHARED_MEMORY_NAME_##name_c);\
		the_##name_s = NULL;\
		SKINK_##name_c##_COUNT = 0;\
	} while (0)
#define CLEAN_VFREE(name_s)\
	do {\
		if (the_##name_s)\
			vfree(the_##name_s);\
		the_##name_s = NULL;\
	} while (0)
	if (the_sensor_layers)
	{
		uint32_t i;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
			CLEAN_VFREE(sensor_layers[i].sensor_map);
	}
	CLEAN_FREE(SENSOR_LAYERS, sensor_layers);
	CLEAN_FREE(SENSORS, sensors);
	CLEAN_FREE(SUB_REGIONS, sub_regions);
	CLEAN_FREE(REGIONS, regions);
	CLEAN_FREE(MODULES, modules);
	CLEAN_FREE(PATCHES, patches);
	CLEAN_FREE(SUB_REGION_INDICES, sub_region_indices);
	CLEAN_FREE(REGION_INDICES, region_indices);
	CLEAN_FREE(SENSOR_NEIGHBORS, sensor_neighbors);
#undef CLEAN_FREE
#undef CLEAN_VFREE
}

static int _command_write(struct file *file, const char *buf, unsigned long count, void *data)
{
	/*
	 * Possible commands to the skin kernel are:
	 * devs: This command tells the skin kernel not to accept anymore devices in skink_device_register
	 * clbr: This command tells the skin kernel that calibration has been finished
	 * rgn:  This command tells the skin kernel that regionalization has been finished
	 */
#define READ_CHUNK 100u
	char		command[READ_CHUNK];
	unsigned int	state			= 0;
	unsigned long	left			= count;
	unsigned int	i;
	while (left > 0)
	{
		unsigned int amount = left<READ_CHUNK?left:READ_CHUNK;
		if (copy_from_user(command, buf, amount))
		{
			SKINK_LOG("Could not get command from user space");
			return -EFAULT;
		}
		buf += amount;
		left -= amount;
		for (i = 0; i < READ_CHUNK; ++i)
			switch (state)
			{
			case 0:		// beginning of command
				if	(command[i] == 'd')	state = 1;
				else if	(command[i] == 'c')	state = 4;
				else if	(command[i] == 'r')	state = 7;
				break;
			case 1:		// d seen
				if	(command[i] == 'e')	state = 2;
				else				state = 0;
				break;
			case 2:		// de seen
				if	(command[i] == 'v')	state = 3;
				else				state = 0;
				break;
			case 3:		// dev seen
				if	(command[i] == 's')
				{
					if (the_state == SKINK_STATE_READY_FOR_DEVICES)
					{
						skink_no_more_devices();
						strncpy(the_phase[0], "nodevs", sizeof(the_phase[0]) - 1);
					}
					else
						SKINK_LOG("Invalid command \"devs\" at this stage");
				}
				state = 0;
				break;
			case 4:		// c seen
				if	(command[i] == 'l')	state = 5;
				else				state = 0;
				break;
			case 5:		// cl seen
				if	(command[i] == 'b')	state = 6;
				else				state = 0;
				break;
			case 6:		// clb seen
				if	(command[i] == 'r')
				{
					if (the_state == SKINK_STATE_CALIBRATING)
					{
						the_state = SKINK_STATE_REGIONALIZING;
						strncpy(the_phase[0], "rgn", sizeof(the_phase[0]) - 1);
					}
					else
						SKINK_LOG("Invalid command \"clbr\" at this stage");
				}
				state = 0;
				break;
			case 7:		// r seen
				if	(command[i] == 'g')	state = 8;
				else				state = 0;
				break;
			case 8:		// rg seen
				if	(command[i] == 'n')
				{
					if (the_state == SKINK_STATE_REGIONALIZING)
					{
						the_state = SKINK_STATE_REGIONALIZATION_DONE;
						strncpy(the_phase[0], "preop", sizeof(the_phase[0]) - 1);
					}
					else
						SKINK_LOG("Invalid command \"rgn\" at this stage");
				}
				state = 0;
				break;
			default:
				SKINK_LOG("Internal error: command state machine ended in invalid state %u", state);
				state = 0;
			}
	}
	return count;
}

// CHECK_EXIT is responsible for checking whether the module is being removed.  If so, it does some clean up and exits
#define CHECK_EXIT \
	do {\
		if (the_state == SKINK_STATE_EXITING)\
		{\
			CLEANUP;\
			_init_thread_exited = 1;\
			do_exit(0);\
			return SKINK_FAIL;		/* should never reach */\
		}\
	} while (0)

// The clean up is different at different stages, that's why CLEANUP_HELPER is used, which is redefined at every needed location
#define CLEANUP \
	do {\
		CLEANUP_HELPER;\
		the_state = SKINK_STATE_FAILED;\
		strncpy(the_phase[0], "fail", sizeof(the_phase[0]) - 1);\
		SKINK_INITIALIZATION_FAILED = 1;\
	} while (0)

static int skink_init_thread(void *arg)
{
	uint32_t		i, j;
	struct proc_dir_entry	*proc_entry;
	bool			misc_is_contiguous;
	int			ret;
	_init_thread_exited = 0;
	/******************************************/
	/*    Stage 0: Minimal initialization     */
	/******************************************/
	/*  This includes initialization of misc  */
	/*  memory, proc_fs file and other        */
	/*  initializations                       */
	/******************************************/
	_pre_cleanup();
	skink_sysfs_init();
	the_misc_data = skin_rt_shared_memory_alloc(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS,
			SKINK_MISC_MEMORY_SIZE * sizeof(*the_misc_data), &misc_is_contiguous);
	if (the_misc_data == NULL)
	{
		SKINK_LOG("Stage 0: Could not acquire memory");
		the_state = SKINK_STATE_FAILED;
		strncpy(the_phase[0], "fail", sizeof(the_phase[0]) - 1);
		_init_thread_exited = 1;
		do_exit(0);
		return SKINK_FAIL;
	}
	else if (!misc_is_contiguous)
			SKINK_LOG("Stage 0: Warning - Using discontiguous memory");	// No biggie, really, but if you see this, chances are you have almost
											// depleted your memory
	for (i = 0; i < SKINK_MISC_MEMORY_SIZE; ++i)
		the_misc_data[i]	= 0;
	// SKINK_INITIALIZED		= 0;			// implicit in
	// SKINK_INITIALIZATION_FAILED	= 0;			// previous for loop
	SKINK_CALIBRATION_TOO_EARLY	= 1;
	SKINK_REGIONALIZATION_TOO_EARLY	= 1;
	SKINK_MODULE_LOADED		= 1;
	SKINK_SIZEOF_SENSOR_LAYER	= sizeof(skink_sensor_layer);
	SKINK_SIZEOF_SENSOR		= sizeof(skink_sensor);
	SKINK_SIZEOF_MODULE		= sizeof(skink_module);
	SKINK_SIZEOF_PATCH		= sizeof(skink_patch);
	SKINK_SIZEOF_SUB_REGION		= sizeof(skink_sub_region);
	SKINK_SIZEOF_REGION		= sizeof(skink_region);
	SKINK_SENSOR_LAYERS_COUNT	= 0;
	SKINK_SENSORS_COUNT		= 0;
	SKINK_MODULES_COUNT		= 0;
	SKINK_PATCHES_COUNT		= 0;
#define CLEANUP_HELPER ((void)0)
	proc_entry = create_proc_entry(SKINK_COMMAND_PROCFS_ENTRY, 0200, NULL);
	if (proc_entry == NULL)
	{
		SKINK_LOG("Stage 0: Could not create /proc entry");
		remove_proc_entry(SKINK_COMMAND_PROCFS_ENTRY, NULL);
		CLEANUP;
		_init_thread_exited = 1;
		do_exit(0);
		return SKINK_FAIL;
	}
	proc_entry->write_proc	= _command_write;
	proc_entry->mode	= S_IFREG | S_IWUGO;
	proc_entry->uid		= 0;
	proc_entry->gid		= 0;
	proc_entry->size	= 0;
	sema_init(&the_devices_mutex, 1);
	sema_init(&the_services_mutex, 1);
#undef CLEANUP_HELPER
#define CLEANUP_HELPER the_clean_free()
	CHECK_EXIT;
	skink_initialize_devices();
	skink_initialize_services();
	the_state = SKINK_STATE_READY_FOR_DEVICES;
	strncpy(the_phase[0], "devs", sizeof(the_phase[0]) - 1);
	SKINK_LOG("Stage 1...");
	/******************************************/
	/*    Stage 1: Introduction of devices    */
	/******************************************/
	/*  At this stage, devices can register   */
	/*  themselves with the skin kernel       */
	/*  through skink_register                */
	/******************************************/
	while (the_state != SKINK_STATE_DEVICES_INTRODUCED && the_state != SKINK_STATE_EXITING)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	CHECK_EXIT;
	the_devices_resume_possible = true;
	SKINK_LOG("Stage 1...done");
	SKINK_LOG("Stage 2...");
	/******************************************/
	/*      Stage 2: Building structures      */
	/******************************************/
	/*  At this stage, the build_structures   */
	/*  callback of all registered devices    */
	/*  will be called                        */
	/******************************************/
	if (skink_build_structures() != SKINK_SUCCESS)
	{
		SKINK_LOG("Stage 2: Skin kernel stopping due to error in initialization of data structures");
		CLEANUP;
		_init_thread_exited = 1;
		do_exit(0);
		return SKINK_FAIL;		// should never reach
	}
	CHECK_EXIT;
	skink_sysfs_post_devs();
	CHECK_EXIT;
	the_state = SKINK_STATE_STRUCTURES_BUILT;
	SKINK_LOG("Stage 2...done");
	SKINK_LOG("Stage 3...");
	/******************************************/
	/*       Stage 3: Data acquisition        */
	/******************************************/
	/*  Create rt threads to acquire data     */
	/******************************************/
	skin_rt_init(SKINK_TICK_PERIOD, &_rt_stop_flags);
	skink_sysfs_post_rt();
	ret = skink_initialize_writers();
	if (ret == SKINK_PARTIAL)
		SKINK_LOG("Warning: the skin is only partially operational");
	else if (ret != SKINK_SUCCESS)
	{
		CLEANUP;
		_init_thread_exited = 1;
		do_exit(0);
		return SKINK_FAIL;		// should never reach
	}
#undef CLEANUP_HELPER
#define CLEANUP_HELPER \
	skink_free_writers();\
	skink_free_devices();\
	the_clean_free()
	CHECK_EXIT;
	//the_state = SKINK_STATE_THREAD_SPAWNED;		// there is nothing between this state and next
	the_state = SKINK_STATE_CALIBRATING;
	strncpy(the_phase[0], "clbr", sizeof(the_phase[0]) - 1);
	SKINK_LOG("Stage 3...done");
	SKINK_LOG("Stage 4...");
	/******************************************/
	/*         Stage 4: Calibration           */
	/******************************************/
	/*  Wait for calibrator to do its job     */
	/*  and send calibration data to the      */
	/*  skin kernel                           */
	/******************************************/
	SKINK_CALIBRATION_POSSIBLE = 1;
	SKINK_CALIBRATION_TOO_EARLY = 0;
	while (the_state == SKINK_STATE_CALIBRATING)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	CHECK_EXIT;
	the_sensor_neighbors = skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS);
	if (the_sensor_neighbors == NULL)
		SKINK_LOG("Stage 4: Could not attach to sensor neighbors memory.  These data will not be available to the users");
	// Nothing else to do.  At this stage, the top layer has already transfered the position information etc to the bottom layer.
	// Note that, calibration can be skipped by sending the right command to the skin kernel.  The kernel would move on and the skin
	// data would be usable, but without the sensor position, I doubt that would be of much use.  Anyway, know that you DO have the option!
	SKINK_CALIBRATION_POSSIBLE = 0;
	CHECK_EXIT;
	SKINK_LOG("Stage 4...done");
	SKINK_LOG("Stage 5...");
	/******************************************/
	/*        Stage 5: Regionalization        */
	/******************************************/
	/*  Wait for regionalizer to do its job   */
	/*  and send regionalization data to the  */
	/*  skin kernel.  Stop the acquisition     */
	/*  threads momentarily while the kernel  */
	/*  rearranges the sensors and creates    */
	/*  region/sub-region indices             */
	/******************************************/
	SKINK_REGIONALIZATION_SYNC = 0;
	SKINK_REGIONALIZATION_POSSIBLE = 1;
	SKINK_REGIONALIZATION_TOO_EARLY = 0;
	while (the_state == SKINK_STATE_REGIONALIZING && SKINK_REGIONALIZATION_SYNC == 0)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	CHECK_EXIT;
#define LOCK_DEVICES \
	do {\
		if (down_interruptible(&the_devices_mutex))\
		{\
			SKINK_LOG("Stage 5: Init thread interrupted by signal.  Aborting");\
			CLEANUP;\
			_init_thread_exited = 1;\
			do_exit(0);\
			return SKINK_FAIL;	/* should never reach */\
		}\
	} while (0)
#define UNLOCK_DEVICES up(&the_devices_mutex)
	// lock the device mutex
	LOCK_DEVICES;
#undef CLEANUP_HELPER
#define CLEANUP_HELPER \
	skink_free_writers();\
	UNLOCK_DEVICES;\
	skink_free_devices();\
	the_clean_free()
	// Pause all threads because skink_sensor_layer's sensor_map is going to change
	skink_pause_threads();
	CHECK_EXIT;
	if (SKINK_REGIONALIZATION_SYNC != 0)	// otherwise, state has changed which means regionalization part is skipped!
	{
		skink_sensor_id layer_sensors_begin;
		UNLOCK_DEVICES;
		// Set SYNC to 0 and wait for it to become 1 again
		SKINK_REGIONALIZATION_SYNC = 0;
		while (the_state != SKINK_STATE_EXITING && SKINK_REGIONALIZATION_SYNC == 0)
		{
			set_current_state(TASK_INTERRUPTIBLE);
			msleep(1);
		}
		CHECK_EXIT;
		LOCK_DEVICES;
		layer_sensors_begin = 0;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		{
			skink_sensor_layer *layer = the_sensor_layers + i;
			for (j = 0; j < layer->modules_count; ++j)
				layer->modules[j].sensors_begin -= layer_sensors_begin;
			for (j = 0; j < layer->sensors_count; ++j)
			{
				layer->sensor_map[j]		= layer->sensors[j].id;
				layer->sensors[j].id		= j;
			}
			layer_sensors_begin += layer->sensors_count;
		}
		the_sub_regions		= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGIONS);
		the_regions		= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_REGIONS);
		the_sub_region_indices	= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_SUB_REGION_INDICES);
		the_region_indices	= skin_rt_shared_memory_attach(SKINK_RT_SHARED_MEMORY_NAME_REGION_INDICES);
		if (the_sub_regions == NULL || the_regions == NULL
			|| the_sub_region_indices == NULL || the_region_indices == NULL)
		{
			SKINK_LOG("Stage 5: Could not attach to region/sub-region memories");
			CLEANUP;
			_init_thread_exited = 1;
			do_exit(0);
			return SKINK_FAIL;	// should never reach
		}
	}
	else if (the_state != SKINK_STATE_EXITING)		// Again, this means that the regionalization part is skipped
	{
#define SKINK_SHARED_MEMORY_ALLOC(name_c, name_s)\
	do {\
		bool _is_contiguous;\
		the_##name_s = skin_rt_shared_memory_alloc(SKINK_RT_SHARED_MEMORY_NAME_##name_c,\
								SKINK_##name_c##_COUNT * sizeof(*the_##name_s),\
								&_is_contiguous);\
		if (the_##name_s == NULL)\
		{\
			SKINK_LOG("Stage 5: Could not acquire memory for "#name_s);\
			CLEANUP;\
			_init_thread_exited = 1;\
			do_exit(0);\
			return SKINK_FAIL;	/* should never reach */\
		}\
		else if (!_is_contiguous)\
			SKINK_LOG("Stage 5: Warning - Using discontiguous memory");	/* Not really a problem.  But if you see this,*/\
											/* you are most likely almost running out of memory */\
	} while (0)
		// Create a single region for the whole skin
		SKINK_REGIONS_COUNT			= 1;	// all skin is one region
		SKINK_SHARED_MEMORY_ALLOC(REGIONS, regions);
		SKINK_SUB_REGIONS_COUNT			= 1;	// all skin is one sub-region
		SKINK_SHARED_MEMORY_ALLOC(SUB_REGIONS, sub_regions);
		SKINK_SUB_REGION_INDICES_COUNT		= 1;
		SKINK_REGION_INDICES_COUNT		= 1;
		SKINK_SHARED_MEMORY_ALLOC(SUB_REGION_INDICES, sub_region_indices);
		SKINK_SHARED_MEMORY_ALLOC(REGION_INDICES, region_indices);
		the_sub_regions[0].id			= 0;
		for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
		{
			the_sub_regions[0].layers[i].sensors_begin = 0;
			the_sub_regions[0].layers[i].sensors_count = the_sensor_layers[i].sensors_count;
		}
		the_sub_regions[0].region_indices_begin	= 0;
		the_sub_regions[0].region_indices_count	= 1;
		the_regions[0].id			= 0;
		the_regions[0].sub_region_indices_begin	= 0;
		the_regions[0].sub_region_indices_count	= 1;
		the_sub_region_indices[0]		= 0;
		the_region_indices[0]			= 0;
	}
	CHECK_EXIT;
	skink_resume_threads();
	UNLOCK_DEVICES;
#undef CLEANUP_HELPER
#define CLEANUP_HELPER \
	skink_free_writers();\
	skink_free_devices();\
	the_clean_free()
	while (the_state == SKINK_STATE_REGIONALIZING)
	{
		set_current_state(TASK_INTERRUPTIBLE);
		msleep(1);
	}
	SKINK_REGIONALIZATION_POSSIBLE = 0;
	CHECK_EXIT;
	the_state = SKINK_STATE_OPERATIONAL;
	strncpy(the_phase[0], "op", sizeof(the_phase[0]) - 1);
	SKINK_LOG("Stage 5...done");
#if 0	// Debug: Dump the structures
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT; ++i)
	{
		uint32_t j;
		SKINK_LOG("Sensor layer %u (%u in device): id: %u, sensor count: %u, module count: %u, patches count: %u, buffer count: %u, acquisition rate: %u, name: %s", i,
			(uint32_t)the_sensor_layers[i].id, (uint32_t)the_sensor_layers[i].id_in_device,
			(uint32_t)the_sensor_layers[i].sensors_count, (uint32_t)the_sensor_layers[i].modules_count,
			(uint32_t)the_sensor_layers[i].patches_count, (uint32_t)the_sensor_layers[i].number_of_buffers,
			(uint32_t)the_sensor_layers[i].acquisition_rate, the_sensor_layers[i].name);
#if 0
		for (j = 0; j < the_sensor_layers[i].sensors_count; ++j)
			SKINK_LOG("  Sensor %u: id: %u, sub-region: %u, module: %u", j,
				(uint32_t)the_sensor_layers[i].sensors[j].id,
				(uint32_t)the_sensor_layers[i].sensors[j].sub_region,
				(uint32_t)the_sensor_layers[i].sensors[j].module);
#endif
		for (j = 0; j < the_sensor_layers[i].modules_count; ++j)
			SKINK_LOG("  Module %u: id: %u, patch: %u, sensors: [%u, %u)", j,
				(uint32_t)the_sensor_layers[i].modules[j].id, (uint32_t)the_sensor_layers[i].modules[j].patch,
				(uint32_t)the_sensor_layers[i].modules[j].sensors_begin,
				(uint32_t)(the_sensor_layers[i].modules[j].sensors_begin
					+ the_sensor_layers[i].modules[j].sensors_count));
		for (j = 0; j < the_sensor_layers[i].patches_count; ++j)
			SKINK_LOG("  Patch %u: id: %u, layer: %u, modules: [%u, %u)", j,
				(uint32_t)the_sensor_layers[i].patches[j].id, (uint32_t)the_sensor_layers[i].patches[j].layer,
				(uint32_t)the_sensor_layers[i].patches[j].modules_begin,
				(uint32_t)(the_sensor_layers[i].patches[j].modules_begin
					+ the_sensor_layers[i].patches[j].modules_count));
	}
	for (i = 0; i < SKINK_SUB_REGIONS_COUNT; ++i)
	{
		uint32_t j;
		SKINK_LOG("Sub-region %u: id: %u, region indices: [%u, %u)", i,
			(uint32_t)the_sub_regions[i].id, (uint32_t)the_sub_regions[i].region_indices_begin,
			(uint32_t)(the_sub_regions[i].region_indices_begin
				+ the_sub_regions[i].region_indices_count));
		for (j = 0; j < SKINK_SENSOR_LAYERS_COUNT; ++j)
			SKINK_LOG("  Sensors in layer %u: [%u, %u)", j,
				(uint32_t)the_sub_regions[i].layers[j].sensors_begin,
				(uint32_t)(the_sub_regions[i].layers[j].sensors_begin
					+ the_sub_regions[i].layers[j].sensors_count));
	}
	for (i = 0; i < SKINK_REGIONS_COUNT; ++i)
		SKINK_LOG("Region %u: id: %u, subregion indices: [%u, %u)", i,
			(uint32_t)the_regions[i].id, (uint32_t)the_regions[i].sub_region_indices_begin,
			(uint32_t)(the_regions[i].sub_region_indices_begin
				+ the_regions[i].sub_region_indices_count));
	for (i = 0; i < SKINK_SUB_REGION_INDICES_COUNT; ++i)
		SKINK_LOG("Sub-region index %u: %u", i, (uint32_t)the_sub_region_indices[i]);
	for (i = 0; i < SKINK_REGION_INDICES_COUNT; ++i)
		SKINK_LOG("Region index %u: %u", i, (uint32_t)the_region_indices[i]);
#endif
	SKINK_LOG("Stage 6...");
	// Finally, now other top layers can start working!
	SKINK_INITIALIZED = 1;
	_init_thread_exited = 1;
	do_exit(0);		// what happens if the module is rmmoded but the thread has not finished working? Does it become a zombie?
				// How do I kill it from cleanup_module? kthread_stop doesn't work that way.
				// Wait, what is a zombie? A process with a dead parent or a process which is finished but not waited by the parent?
#undef CLEANUP_HELPER
	return SKINK_SUCCESS;			// should never reach
}

static int __init skin_kernel_init(void)
{
	bool should_reconfigure = false;
	SKINK_LOG("Skin kernel initializing...");
	SKINK_LOG("Version %s", SKINK_VERSION);
	if (skink_buffer_count < 1)
	{
		SKINK_LOG("The number of buffers cannot be smaller than 1.  Setting this number to 1");
		skink_buffer_count = 1;
	}
	if (skink_buffer_count > SKINK_MAX_BUFFERS)
	{
		SKINK_LOG("The number of buffers cannot be bigger than "SKINK_STRINGIFY(SKINK_MAX_BUFFERS)
				".  Setting this number to "SKINK_STRINGIFY(SKINK_MAX_BUFFERS));
		skink_buffer_count = SKINK_MAX_BUFFERS;
	}
	if (!skin_rt_priority_is_valid(SKINK_CONVERTED_WRITER_PRIORITY))
	{
		should_reconfigure = true;
		SKINK_LOG("The assigned writer priority (%u) is not valid", (uint32_t)SKINK_CONVERTED_WRITER_PRIORITY);
	}
	if (!skin_rt_priority_is_valid(SKINK_CONVERTED_READER_PRIORITY))
	{
		should_reconfigure = true;
		SKINK_LOG("The assigned service reader priority (%u) is not valid", (uint32_t)SKINK_CONVERTED_READER_PRIORITY);
	}
	if (!skin_rt_priority_is_valid(SKINK_CONVERTED_SERVICE_PRIORITY))
	{
		should_reconfigure = 1;
		SKINK_LOG("The assigned service priority (%u) is not valid", (uint32_t)SKINK_CONVERTED_SERVICE_PRIORITY);
	}
	if (should_reconfigure)
	{
		SKINK_LOG("You need to reconfigure Skinware");
		return -EINVAL;
	}
	the_state = SKINK_STATE_INIT;
	if ((_init_thread = kthread_run(skink_init_thread, NULL, "skink_init_thread")) == ERR_PTR(-ENOMEM))
	{
		_init_thread = NULL;
		SKINK_LOG("Error creating the initial thread");
		SKINK_LOG("Stopping the skin kernel");
		return -ENOMEM;
	}
	SKINK_LOG("Skin kernel initializing...done");
	return 0;
}

static void __exit skin_kernel_exit(void)
{
	uint32_t i, j;
	bool locks_shared;
	skink_sensor_layer_size sensor_layers_count = SKINK_SENSOR_LAYERS_COUNT;
	if (the_state == SKINK_STATE_OPERATIONAL)
		SKINK_LOG("Init thread: Stage 6...done");
	SKINK_LOG("Skin kernel exiting...");
	locks_shared = the_state >= SKINK_STATE_STRUCTURES_BUILT;
	the_state = SKINK_STATE_EXITING;
	strncpy(the_phase[0], "exit", sizeof(the_phase[0]) - 1);
	while (!_init_thread_exited)				// Wait until init thread exits
	{							// Note: if it hasn't already exited,
		set_current_state(TASK_INTERRUPTIBLE);		// then it might call clean_free
		msleep(1);					// so misc variables cannot be used
	}							// neither can data structures of skin
	// TODO: the init thread needs to be cleaned up.  Take a look at release_task, but I'm not sure if it's the right one
	SKINK_INITIALIZED = 0;
	skink_free_services();					// Free services
	skink_free_writers();					// Delete the acquisition tasks
	skin_rt_stop(_rt_stop_flags);
	skink_sysfs_remove();					// Remove sysfs entries
	remove_proc_entry(SKINK_COMMAND_PROCFS_ENTRY, NULL);	// Remove procfs entry
	if (the_sync_locks)
	{
		if (locks_shared && skink_buffer_count > 1)	// Remove synchronization locks
			for (i = 0; i < sensor_layers_count; ++i)
			{
				skin_rt_rwlock *sync_locks = the_sync_locks + SKINK_MAX_BUFFERS * i;
				for (j = 0; j < skink_buffer_count; ++j)
				{
					char rwl_name[SKIN_RT_MAX_NAME_LENGTH + 1];
					skink_get_rwlock_name(rwl_name, i, j);
					skin_rt_unshare_rwlock(rwl_name);
					skin_rt_rwlock_delete(sync_locks + j);
				}
			}
		vfree(the_sync_locks);
		the_sync_locks = NULL;
	}
	skink_free_devices();					// Free network devices
	the_clean_free();					// Free internal data structures
	if (the_misc_data)					// Make the misc memory tell everyone that the skin is not usable anymore.
	{							// Note: shared memory resides until everyone detaches
		SKINK_MODULE_LOADED		= 0;
		SKINK_INITIALIZED		= 0;
		SKINK_INITIALIZATION_FAILED	= 0;
		SKINK_SENSOR_LAYERS_COUNT	= 0;
		SKINK_SENSORS_COUNT		= 0;
		SKINK_SUB_REGIONS_COUNT		= 0;
		SKINK_REGIONS_COUNT		= 0;
		SKINK_MODULES_COUNT		= 0;
		SKINK_PATCHES_COUNT		= 0;
		SKINK_SUB_REGION_INDICES_COUNT	= 0;
		SKINK_REGION_INDICES_COUNT	= 0;
		SKINK_SENSOR_NEIGHBORS_COUNT	= 0;
		SKINK_CALIBRATION_POSSIBLE	= 0;
		SKINK_CALIBRATION_TOO_EARLY	= 0;
		SKINK_REGIONALIZATION_POSSIBLE	= 0;
		SKINK_REGIONALIZATION_TOO_EARLY	= 0;
		skin_rt_shared_memory_free(SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS);
	}
	SKINK_LOG("Skin kernel exiting...done");
}

module_init(skin_kernel_init);
module_exit(skin_kernel_exit);
