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

#ifndef SKIN_PATCH_H
#define SKIN_PATCH_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#include "skin_callbacks.h"

struct skin_object;
struct skin_sensor;
struct skin_module;
struct skin_sensor_type;

SKIN_DECL_BEGIN

typedef struct skin_patch skin_patch;

/*
 * skin_patch is a patch in the skin, consisting of modules.
 *
 * init				initialize to an invalid patch
 * free				release any resources and make invalid
 * modules			get the array of modules of the patch and its count.  If count is NULL, it will be ignored
 * modules_count		get the number of modules in the patch
 * sensors_count		get the number of sensors in all modules of the patch
 * sensor_type			get a reference to the sensor type the patch belongs to
 * for_each_sensor		call a callback for each sensor of the patch
 * for_each_module		call a callback for each module of the patch
 */
void skin_patch_init(skin_patch *patch);
void skin_patch_free(skin_patch *patch);
struct skin_module *skin_patch_modules(skin_patch *patch, skin_module_size *count);
static inline skin_module_size skin_patch_modules_count(skin_patch *patch);
static inline skin_sensor_size skin_patch_sensors_count(skin_patch *patch);
struct skin_sensor_type *skin_patch_sensor_type(skin_patch *patch);
void skin_patch_for_each_sensor(skin_patch *patch, skin_callback_sensor c, void *data);
void skin_patch_for_each_module(skin_patch *patch, skin_callback_module c, void *data);

struct skin_patch
{
	skin_patch_id			id;				/* id of the patch itself.  Is equal to its index in the patch list */
	skin_module_id			modules_begin, modules_end;	/* [begin end) are ids of modules of this patch */
	skin_sensor_type_id		sensor_type_id;

	/* internal */
	struct skin_object		*p_object;
	skin_sensor_size		p_sensors_count;		/* this is a cached value of number of sensors */

#ifdef SKIN_CPP
	skin_patch() { skin_patch_init(this); }
	~skin_patch() { skin_patch_free(this); }
	skin_module *modules(skin_module_size *count = NULL) { return skin_patch_modules(this, count); }
	skin_module_size modules_count() { return skin_patch_modules_count(this); }
	skin_sensor_size sensors_count() { return skin_patch_sensors_count(this); }
	skin_sensor_type *sensor_type() { return skin_patch_sensor_type(this); }

	class skin_sensor_iterator
	{
	private:
		skin_sensor_id current_sensor;
		skin_module_id current_module;
		skin_patch *patch;
		skin_sensor_iterator(skin_patch *patch, skin_module_id cm, skin_sensor_id cs);
	public:
		skin_sensor_iterator();
		skin_sensor_iterator &operator ++();
		skin_sensor_iterator operator ++(int);
		bool operator ==(const skin_sensor_iterator &rhs) const;
		bool operator !=(const skin_sensor_iterator &rhs) const;
		skin_sensor &operator *() const;
		skin_sensor *operator ->() const;
		operator skin_sensor *() const;
		friend struct skin_patch;
	};
	skin_sensor_iterator sensors_iter_begin();
	const skin_sensor_iterator sensors_iter_end();

	class skin_module_iterator
	{
	private:
		skin_module_id current;
		skin_object *object;
		skin_module_iterator(skin_patch *patch, skin_module_id c);
	public:
		skin_module_iterator();
		skin_module_iterator &operator ++();
		skin_module_iterator operator ++(int);
		bool operator ==(const skin_module_iterator &rhs) const;
		bool operator !=(const skin_module_iterator &rhs) const;
		skin_module &operator *() const;
		skin_module *operator ->() const;
		operator skin_module *() const;
		friend struct skin_patch;
	};
	skin_module_iterator modules_iter_begin();
	const skin_module_iterator modules_iter_end();
#endif
};

static inline skin_module_size skin_patch_modules_count(skin_patch *patch) { return patch->modules_end - patch->modules_begin; }
static inline skin_sensor_size skin_patch_sensors_count(skin_patch *patch) { return patch->p_sensors_count; }

SKIN_DECL_END

#endif
