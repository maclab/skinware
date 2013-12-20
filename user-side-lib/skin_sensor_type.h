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

#ifndef SKIN_SENSOR_TYPE_H
#define SKIN_SENSOR_TYPE_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#include "skin_callbacks.h"

struct skin_object;
struct skin_sensor;
struct skin_module;
struct skin_patch;

SKIN_DECL_BEGIN

typedef struct skin_sensor_type skin_sensor_type;

/*
 * skin_module is a module in the skin, consisting of sensors.
 *
 * init				initialize to an invalid sensor type
 * free				release any resources and make invalid
 * sensors			get the array of sensors of this type and its count.  If count is NULL, it will be ignored
 * modules			get the array of modules with sensors of this type and its count.  If count is NULL, it will be ignored
 * patches			get the array of patches with sensors of this type and its count.  If count is NULL, it will be ignored
 * sensors_count		get the number of sensors of this type
 * modules_count		get the number of modules with sensors of this type
 * patches_count		get the number of patches with sensors of this type
 * for_each_sensor		call a callback for each sensor of this type
 * for_each_module		call a callback for each module with sensors of this type
 * for_each_patch		call a callback for each patch with sensors of this type
 */
void skin_sensor_type_init(skin_sensor_type *st);
void skin_sensor_type_free(skin_sensor_type *st);
struct skin_sensor *skin_sensor_type_sensors(skin_sensor_type *st, skin_sensor_size *count);
struct skin_module *skin_sensor_type_modules(skin_sensor_type *st, skin_module_size *count);
struct skin_patch *skin_sensor_type_patches(skin_sensor_type *st, skin_patch_size *count);
static inline skin_sensor_size skin_sensor_type_sensors_count(skin_sensor_type *st);
static inline skin_module_size skin_sensor_type_modules_count(skin_sensor_type *st);
static inline skin_patch_size skin_sensor_type_patches_count(skin_sensor_type *st);
void skin_sensor_type_for_each_sensor(skin_sensor_type *st, skin_callback_sensor c, void *data);
void skin_sensor_type_for_each_module(skin_sensor_type *st, skin_callback_module c, void *data);
void skin_sensor_type_for_each_patch(skin_sensor_type *st, skin_callback_patch c, void *data);

struct skin_sensor_type
{
	char				*name;			/* name of the sensor type. */
	skin_sensor_type_id		id;			/* the id of the sensor type.  Could be used to index sub-region layers */
	skin_sensor_id			sensors_begin,		/* [begin end) are ids of sensors of this type */
					sensors_end;
	skin_module_id			modules_begin,		/* [begin end) are ids of modules of this type */
					modules_end;
	skin_patch_id			patches_begin,		/* [begin end) are ids of patches of this type */
					patches_end;
	bool				is_active;		/* would be false if this sensor type is not active (its driver is unloaded/paused) */

	/* internal */
	struct skin_object		*p_object;

#ifdef SKIN_CPP
	skin_sensor_type() { skin_sensor_type_init(this); }
	~skin_sensor_type() { skin_sensor_type_free(this); }
	skin_sensor *sensors(skin_sensor_size *count = NULL) { return skin_sensor_type_sensors(this, count); }
	skin_module *modules(skin_module_size *count = NULL) { return skin_sensor_type_modules(this, count); }
	skin_patch *patches(skin_patch_size *count = NULL) { return skin_sensor_type_patches(this, count); }
	skin_sensor_size sensors_count() { return skin_sensor_type_sensors_count(this); }
	skin_module_size modules_count() { return skin_sensor_type_modules_count(this); }
	skin_patch_size patches_count() { return skin_sensor_type_patches_count(this); }

	class skin_sensor_iterator
	{
	private:
		skin_sensor_id current;
		skin_object *object;
		skin_sensor_iterator(skin_sensor_type *sensorType, skin_sensor_id c);
	public:
		skin_sensor_iterator();
		skin_sensor_iterator &operator ++();
		skin_sensor_iterator operator ++(int);
		bool operator ==(const skin_sensor_iterator &rhs) const;
		bool operator !=(const skin_sensor_iterator &rhs) const;
		skin_sensor &operator *() const;
		skin_sensor *operator ->() const;
		operator skin_sensor *() const;
		friend struct skin_sensor_type;
	};
	skin_sensor_iterator sensors_iter_begin();
	const skin_sensor_iterator sensors_iter_end();

	class skin_module_iterator
	{
	private:
		skin_module_id current;
		skin_object *object;
		skin_module_iterator(skin_sensor_type *sensorType, skin_module_id c);
	public:
		skin_module_iterator();
		skin_module_iterator &operator ++();
		skin_module_iterator operator ++(int);
		bool operator ==(const skin_module_iterator &rhs) const;
		bool operator !=(const skin_module_iterator &rhs) const;
		skin_module &operator *() const;
		skin_module *operator ->() const;
		operator skin_module *() const;
		friend struct skin_sensor_type;
	};
	skin_module_iterator modules_iter_begin();
	const skin_module_iterator modules_iter_end();

	class skin_patch_iterator
	{
	private:
		skin_patch_id current;
		skin_object *object;
		skin_patch_iterator(skin_sensor_type *sensorType, skin_patch_id c);
	public:
		skin_patch_iterator();
		skin_patch_iterator &operator ++();
		skin_patch_iterator operator ++(int);
		bool operator ==(const skin_patch_iterator &rhs) const;
		bool operator !=(const skin_patch_iterator &rhs) const;
		skin_patch &operator *() const;
		skin_patch *operator ->() const;
		operator skin_patch *() const;
		friend struct skin_sensor_type;
	};
	skin_patch_iterator patches_iter_begin();
	const skin_patch_iterator patches_iter_end();
#endif
};

static inline skin_sensor_size skin_sensor_type_sensors_count(skin_sensor_type *st)	{ return st->sensors_end - st->sensors_begin; }
static inline skin_module_size skin_sensor_type_modules_count(skin_sensor_type *st)	{ return st->modules_end - st->modules_begin; }
static inline skin_patch_size skin_sensor_type_patches_count(skin_sensor_type *st)	{ return st->patches_end - st->patches_begin; }

SKIN_DECL_END

#endif
