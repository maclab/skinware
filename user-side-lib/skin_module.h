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

#ifndef SKIN_MODULE_H
#define SKIN_MODULE_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#include "skin_callbacks.h"

struct skin_object;
struct skin_sensor;
struct skin_patch;
struct skin_sensor_type;

SKIN_DECL_BEGIN

typedef struct skin_module skin_module;

/*
 * skin_module is a module in the skin, consisting of sensors.
 *
 * init				initialize to an invalid module
 * free				release any resources and make invalid
 * sensors			get the array of sensors of the module and its count.  If count is NULL, it will be ignored
 * sensors_count		get the number of sensors in the module
 * patch			get a reference to the patch the module belongs to
 * sensor_type			get a reference to the sensor type the module belongs to
 * for_each_sensor		call a callback for each sensor of the module
 */
void skin_module_init(skin_module *module);
void skin_module_free(skin_module *module);
struct skin_sensor *skin_module_sensors(skin_module *module, skin_sensor_size *count);
static inline skin_sensor_size skin_module_sensors_count(skin_module *module);
struct skin_patch *skin_module_patch(skin_module *module);
struct skin_sensor_type *skin_module_sensor_type(skin_module *module);
void skin_module_for_each_sensor(skin_module *module, skin_callback_sensor c, void *data);

struct skin_module
{
	skin_module_id			id;			/* id of the module itself.  Is equal to its index in the module list */
	skin_sensor_id			sensors_begin,		/* [begin end) are ids of sensors of this module */
					sensors_end;
	skin_sensor_type_id		sensor_type_id;		/* id of sensor type this module belongs to */
	skin_patch_id			patch_id;		/* patch this module belongs to */

	/* internal */
	struct skin_object		*p_object;

#ifdef SKIN_CPP
	skin_module() { skin_module_init(this); }
	~skin_module() { skin_module_free(this); }
	skin_sensor *sensors(skin_sensor_size *count = NULL) { return skin_module_sensors(this, count); }
	skin_sensor_size sensors_count() { return skin_module_sensors_count(this); }
	skin_patch *patch() { return skin_module_patch(this); }
	skin_sensor_type *sensor_type() { return skin_module_sensor_type(this); }

	class skin_sensor_iterator
	{
	private:
		skin_sensor_id current;
		skin_object *object;
		skin_sensor_iterator(skin_module *module, skin_sensor_id c);
	public:
		skin_sensor_iterator();
		skin_sensor_iterator &operator ++();
		skin_sensor_iterator operator ++(int);
		bool operator ==(const skin_sensor_iterator &rhs) const;
		bool operator !=(const skin_sensor_iterator &rhs) const;
		skin_sensor &operator *() const;
		skin_sensor *operator ->() const;
		operator skin_sensor *() const;
		friend struct skin_module;
	};
	skin_sensor_iterator sensors_iter_begin();
	const skin_sensor_iterator sensors_iter_end();
#endif
};

static inline skin_sensor_size skin_module_sensors_count(skin_module *module) { return module->sensors_end - module->sensors_begin; }

SKIN_DECL_END


#endif
