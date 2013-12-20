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
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/vmalloc.h>

#define					SKINK_MODULE_NAME				"skin_kernel"

#include "skink_internal.h"
#include "skink_common.h"
#include "skink_sysfs.h"
#include "skink_rt.h"

extern skink_misc_datum			*the_misc_data;

static struct kobject			*_kobject					= NULL;

// This will be filled by the data acquisition thread.  When dumping, reading time and data maybe not be from the same time, but the difference is small.
// Better than using a lock and keep the data acquisition thread waiting.
skin_rt_time				the_time					= 0;

// Definition, initialization and deletion of data-independent outputs:
// typechar is for sprintf.  That is for example d for int
#define					DEFINE_STATIC_OBJECT(type, typechar, name, num)\
type					the_##name[num];\
\
static ssize_t _show_##name(struct kobject *kobj, struct kobj_attribute *attr, char *buf)\
{\
	uint32_t i;\
	ssize_t so_far = 0;\
	for (i = 0; i < (num) && so_far < (ssize_t)PAGE_SIZE; ++i)\
	{\
		int ret = snprintf(buf + so_far, PAGE_SIZE - so_far - 1, "%"#typechar, the_##name[i]);\
		so_far += ret;\
		if (ret > 0) /* only write \n if there was actual data to write */\
			so_far += snprintf(buf + so_far, PAGE_SIZE - so_far, "\n");\
	}\
	return so_far;\
}\
\
static struct kobj_attribute		_attr_##name					= __ATTR(name, 0444, _show_##name, NULL);

#define					INIT_STATIC_OBJECT(name, defval, num)\
do {\
	uint32_t i;\
	if (sysfs_create_file(_kobject, &_attr_##name.attr))\
		SKINK_LOG("Stage 2: Could not create sysfs file for "#name);\
	else\
	{\
		for (i = 0; i < (num); ++i)\
			the_##name[i] = defval;\
		SKINK_LOG("Stage 2: Successfuly created sysfs file for "#name);\
	}\
} while (0)

#define					DELETE_STATIC_OBJECT(name)			((void)0)

// Definition, initialization and deletion of data-dependent outputs:
#define					DEFINE_DYNAMIC_OBJECT(type, typechar, name, num)\
type					*the_##name					= NULL;		/* every variable is an array, one value for each layer */\
\
static ssize_t _show_##name(struct kobject *kobj, struct kobj_attribute *attr, char *buf)\
{\
	uint32_t i;\
	ssize_t so_far;\
	so_far = snprintf(buf, PAGE_SIZE, "%llu\n", the_time);\
	for (i = 0; i < (num) && so_far < (ssize_t)PAGE_SIZE; ++i)\
		so_far += snprintf(buf + so_far, PAGE_SIZE - so_far, "%"#typechar"\n", the_##name[i]);\
	return so_far;\
}\
\
static struct kobj_attribute		_attr_##name					= __ATTR(name, 0444, _show_##name, NULL);

#define					INIT_DYNAMIC_OBJECT(name, defval, num)\
do {\
	uint32_t i;\
	if (sysfs_create_file(_kobject, &_attr_##name.attr))\
		SKINK_LOG("Stage 2: Could not create sysfs file for "#name);\
	else\
	{\
		the_##name = vmalloc((num) * sizeof(*the_##name));\
		if (!the_##name)\
			SKINK_LOG("Stage 2: Could not acquire memory for output data (in sysfs) for "#name);\
		else\
		{\
			for (i = 0; i < (num); ++i)\
				the_##name[i] = defval;\
			SKINK_LOG("Stage 2: Successfuly created sysfs file for "#name);\
		}\
	}\
} while (0)

#define					DELETE_DYNAMIC_OBJECT(name)\
do {\
	if (the_##name)\
		vfree((void *)the_##name);\
} while (0)

// Here are the variable for each of which one sysfs file would be created:
//DEFINE_STATIC_OBJECT(skin_rt_time, llu, current_time, 1);					// Specialized below
DEFINE_DYNAMIC_OBJECT(uint64_t, llu, swap_skips, SKINK_SENSOR_LAYERS_COUNT);
DEFINE_DYNAMIC_OBJECT(uint64_t, llu, frame_writes, SKINK_SENSOR_LAYERS_COUNT);
DEFINE_DYNAMIC_OBJECT(skin_rt_time, llu, worst_write_time, SKINK_SENSOR_LAYERS_COUNT);
DEFINE_DYNAMIC_OBJECT(skin_rt_time, llu, best_write_time, SKINK_SENSOR_LAYERS_COUNT);
//DEFINE_DYNAMIC_OBJECT(skin_rt_time, llu, accumulated_write_time, SKINK_SENSOR_LAYERS_COUNT);	// Specialized below
DEFINE_STATIC_OBJECT(const char *, s, registered_devices, SKINK_MAX_DEVICES);
DEFINE_STATIC_OBJECT(const char *, s, bad_devices, SKINK_MAX_DEVICES);
typedef char phase_str[10];
DEFINE_STATIC_OBJECT(phase_str, s, phase, 1);

// current_time
static ssize_t _show_current_time(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%llu\n", skin_rt_get_time());
}

static struct kobj_attribute		_attr_current_time				= __ATTR(current_time, 0444, _show_current_time, NULL);

// accumulated_write_time:
skin_rt_time				*the_accumulated_write_time			= NULL;		/* every variable is an array, one value for each layer */

static ssize_t _show_accumulated_write_time(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	uint32_t i;
	ssize_t so_far;
	so_far = snprintf(buf, PAGE_SIZE, "%llu\n", the_time);
	for (i = 0; i < SKINK_SENSOR_LAYERS_COUNT && so_far < (ssize_t)PAGE_SIZE - 1; ++i)
		so_far += snprintf(buf + so_far, PAGE_SIZE - so_far, "%llu %llu\n", the_accumulated_write_time[i], the_frame_writes[i]);
	return so_far;
}

static struct kobj_attribute		_attr_accumulated_write_time			= __ATTR(accumulated_write_time, 0444, _show_accumulated_write_time, NULL);

void skink_sysfs_init(void)
{
	if (_kobject)
		kobject_put(_kobject);
	_kobject = kobject_create_and_add("skin_kernel", NULL);	// TODO: anyone knows how I can get the already created kobject in /sys/module/skin_kernel
									// instead of creating this kobject (/sys/skin_kernel) myself?
	if (!_kobject)
		SKINK_LOG("Stage 0: Could not create kobject for this module");
	INIT_STATIC_OBJECT(registered_devices, "", SKINK_MAX_DEVICES);
//	INIT_STATIC_OBJECT(phase, "init", 1);				// Specialized below
	// phase
	if (sysfs_create_file(_kobject, &_attr_phase.attr))
		SKINK_LOG("Stage 0: Could not create sysfs file for phase");
	else
	{
		strncpy(the_phase[0], "init", sizeof(the_phase[0]) - 1);
		the_phase[0][sizeof(the_phase[0]) - 1] = '\0';	// never touched
		SKINK_LOG("Stage 0: Successfuly created sysfs file for phase");
	}
}

void skink_sysfs_post_devs(void)
{
	if (!_kobject)
		return;
	INIT_DYNAMIC_OBJECT(swap_skips, 0, SKINK_SENSOR_LAYERS_COUNT);
	INIT_DYNAMIC_OBJECT(frame_writes, 0, SKINK_SENSOR_LAYERS_COUNT);
	INIT_DYNAMIC_OBJECT(worst_write_time, 0, SKINK_SENSOR_LAYERS_COUNT);
	INIT_DYNAMIC_OBJECT(best_write_time, 2000000000, SKINK_SENSOR_LAYERS_COUNT);
	INIT_DYNAMIC_OBJECT(accumulated_write_time, 0, SKINK_SENSOR_LAYERS_COUNT);
	INIT_STATIC_OBJECT(bad_devices, "", SKINK_MAX_DEVICES);
}

void skink_sysfs_post_rt(void)
{
	if (!_kobject)
		return;
	//INIT_STATIC_OBJECT(current_time, 0, 1);			// Specialized below
	// current_time
	if (sysfs_create_file(_kobject, &_attr_current_time.attr))\
		SKINK_LOG("Stage 3: Could not create sysfs file for current_time");\
	else
		SKINK_LOG("Stage 3: Successfuly created sysfs file for current_time");\
}

void skink_sysfs_remove(void)
{
	if (_kobject)
		kobject_put(_kobject);
	DELETE_STATIC_OBJECT(current_time);
	DELETE_DYNAMIC_OBJECT(swap_skips);
	DELETE_DYNAMIC_OBJECT(frame_writes);
	DELETE_DYNAMIC_OBJECT(worst_write_time);
	DELETE_DYNAMIC_OBJECT(best_write_time);
	DELETE_DYNAMIC_OBJECT(accumulated_write_time);
	DELETE_STATIC_OBJECT(registered_devices);
	DELETE_STATIC_OBJECT(bad_devices);
	DELETE_STATIC_OBJECT(phase);
}
