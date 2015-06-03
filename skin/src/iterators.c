/*
 * Copyright (C) 2011-2015  Maclab, DIBRIS, Universita di Genova <info@cyskin.com>
 * Authored by Shahbaz Youssefi <ShabbyX@gmail.com>
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

#define URT_LOG_PREFIX "skin: "
#include <skin_base.h>
#include <skin_sensor.h>
#include <skin_module.h>
#include <skin_patch.h>
#include <skin_sensor_type.h>
#include "internal.h"
#include "user_internal.h"

static int _sanity_check_skin(struct skin *skin)
{
	return skin == NULL?-1:0;
}

static int _sanity_check_user(struct skin_user *user)
{
	return user == NULL || user->sensors == NULL || user->modules == NULL || user->patches == NULL
		|| user->sensor_type_count > SKIN_CONFIG_MAX_SENSOR_TYPES?-1:0;
}

static int _sanity_check_patch(struct skin_patch *patch)
{
	return patch == NULL || patch->modules == NULL?-1:0;
}

static int _sanity_check_module(struct skin_module *module)
{
	return module == NULL || module->sensors == NULL?-1:0;
}

#define OBJECT_ITERATE(objects)								\
do {											\
	size_t i;									\
	int ret = 0;									\
											\
	if (_sanity_check_skin(skin) || callback == NULL)				\
		return -1;								\
											\
	for (i = 0; i < skin->objects##_mem_size; ++i)					\
	{										\
		if (skin->objects[i] == NULL)						\
			continue;							\
		if (callback(skin->objects[i], user_data))				\
		{									\
			ret = -1;							\
			break;								\
		}									\
	}										\
											\
	return ret;									\
} while (0)

#define SKIN_DATA_ITERATE(datum)							\
do {											\
	size_t i;									\
	int ret = 0;									\
											\
	if (_sanity_check_skin(skin) || callback == NULL)				\
		return -1;								\
											\
	for (i = 0; i < skin->users_mem_size; ++i)					\
	{										\
		if (skin->users[i] == NULL)						\
			continue;							\
		if (skin_user_for_each_##datum(skin->users[i], callback, user_data))	\
		{									\
			ret = -1;							\
			break;								\
		}									\
	}										\
											\
	return ret;									\
} while (0)

#define USER_DATA_ITERATE(datum, data)							\
do {											\
	skin_##datum##_id i;								\
											\
	if (_sanity_check_user(user) || callback == NULL)				\
		return -1;								\
											\
	for (i = 0; i < user->driver_attr.datum##_count; ++i)				\
		if (callback(&user->data[i], user_data))				\
			return -1;							\
											\
	return 0;									\
} while (0)

#define ENTITY_DATA_ITERATE(entity, datum, data)					\
do {											\
	skin_##datum##_id i;								\
											\
	if (_sanity_check_##entity(entity) || callback == NULL)				\
		return -1;								\
											\
	for (i = 0; i < entity->datum##_count; ++i)					\
		if (callback(&entity->data[i], user_data))				\
			return -1;							\
											\
	return 0;									\
} while (0)

int (skin_for_each_writer)(struct skin *skin, skin_callback_writer callback, void *user_data, ...)
{
	OBJECT_ITERATE(writers);
}
URT_EXPORT_SYMBOL(skin_for_each_writer);

int (skin_for_each_reader)(struct skin *skin, skin_callback_reader callback, void *user_data, ...)
{
	OBJECT_ITERATE(readers);
}
URT_EXPORT_SYMBOL(skin_for_each_reader);

int (skin_for_each_driver)(struct skin *skin, skin_callback_driver callback, void *user_data, ...)
{
	OBJECT_ITERATE(drivers);
}
URT_EXPORT_SYMBOL(skin_for_each_driver);

int (skin_for_each_user)(struct skin *skin, skin_callback_user callback, void *user_data, ...)
{
	OBJECT_ITERATE(users);
}
URT_EXPORT_SYMBOL(skin_for_each_user);

int (skin_for_each_sensor)(struct skin *skin, skin_callback_sensor callback, void *user_data, ...)
{
	SKIN_DATA_ITERATE(sensor);
}
URT_EXPORT_SYMBOL(skin_for_each_sensor);

int (skin_for_each_module)(struct skin *skin, skin_callback_module callback, void *user_data, ...)
{
	SKIN_DATA_ITERATE(module);
}
URT_EXPORT_SYMBOL(skin_for_each_module);

int (skin_for_each_patch)(struct skin *skin, skin_callback_patch callback, void *user_data, ...)
{
	SKIN_DATA_ITERATE(patch);
}
URT_EXPORT_SYMBOL(skin_for_each_patch);

static bool _sensor_type_is_new(struct skin *skin, size_t cur_user, skin_sensor_type_id cur_type)
{
	size_t i;
	skin_sensor_type_id t;
	for (i = 0; i < cur_user; ++i)
	{
		if (skin->users[i] == NULL)
			continue;
		for (t = 0; t < skin->users[i]->sensor_type_count; ++t)
			if (skin->users[cur_user]->sensor_types[cur_type].type
					== skin->users[i]->sensor_types[t].type)
				return false;
	}
	return true;
}

int (skin_for_each_sensor_type)(struct skin *skin, skin_callback_sensor_type callback, void *user_data, ...)
{
	size_t i;
	skin_sensor_type_id t;
	int ret = 0;

	if (_sanity_check_skin(skin) || callback == NULL)
		return -1;

	for (i = 0; i < skin->users_mem_size; ++i)
	{
		if (_sanity_check_user(skin->users[i]))
			continue;
		for (t = 0; t < skin->users[i]->sensor_type_count; ++t)
		{
			/* make sure this sensor type is new */
			if (!_sensor_type_is_new(skin, i, t))
				continue;

			/* construct a sensor type and call the callback for it */
			if (callback(&(struct skin_sensor_type){
						.id = skin->users[i]->sensor_types[t].type,
						.user = skin->users[i],
					}, user_data))
				return -1;
		}
	}

	return ret;
}
URT_EXPORT_SYMBOL(skin_for_each_sensor_type);

int (skin_for_each_sensor_of_type)(struct skin *skin, skin_sensor_type_id type,
		skin_callback_sensor callback, void *user_data, ...)
{
	size_t i;
	int ret = 0;

	if (_sanity_check_skin(skin) || callback == NULL)
		return -1;

	for (i = 0; i < skin->users_mem_size; ++i)
	{
		if (skin->users[i] == NULL)
			continue;
		if (skin_user_for_each_sensor_of_type(skin->users[i], type, callback, user_data))
		{
			ret = -1;
			break;
		}
	}

	return ret;
}
URT_EXPORT_SYMBOL(skin_for_each_sensor_of_type);

int (skin_user_for_each_sensor)(struct skin_user *user, skin_callback_sensor callback, void *user_data, ...)
{
	USER_DATA_ITERATE(sensor, sensors);
}
URT_EXPORT_SYMBOL(skin_user_for_each_sensor);

int (skin_user_for_each_module)(struct skin_user *user, skin_callback_module callback, void *user_data, ...)
{
	USER_DATA_ITERATE(module, modules);
}
URT_EXPORT_SYMBOL(skin_user_for_each_module);

int (skin_user_for_each_patch)(struct skin_user *user, skin_callback_patch callback, void *user_data, ...)
{
	USER_DATA_ITERATE(patch, patches);
}
URT_EXPORT_SYMBOL(skin_user_for_each_patch);

int (skin_user_for_each_sensor_type)(struct skin_user *user, skin_callback_sensor_type callback, void *user_data, ...)
{
	skin_sensor_type_id t;

	if (_sanity_check_user(user) || callback == NULL)
		return -1;

	for (t = 0; t < user->sensor_type_count; ++t)
		/* construct a sensor type and call the callback for it */
		if (callback(&(struct skin_sensor_type){
					.id = user->sensor_types[t].type,
					.user = user,
				}, user_data))
			return -1;

	return 0;
}
URT_EXPORT_SYMBOL(skin_user_for_each_sensor_type);

int (skin_user_for_each_sensor_of_type)(struct skin_user *user, skin_sensor_type_id type,
		skin_callback_sensor callback, void *user_data, ...)
{
	skin_sensor_type_id t;
	skin_sensor_id i;

	if (_sanity_check_user(user) || callback == NULL)
		return -1;

	/* find the type */
	for (t = 0; t < user->sensor_type_count; ++t)
		if (user->sensor_types[t].type == type)
			break;

	if (t >= user->sensor_type_count)
		return -1;

	for (i = user->sensor_types[t].first_sensor; i < user->driver_attr.sensor_count; i = user->sensors[i].next_of_type)
		if (callback(&user->sensors[i], user_data))
			return -1;

	return 0;
}
URT_EXPORT_SYMBOL(skin_user_for_each_sensor_of_type);

struct for_each_sensor_data
{
	skin_callback_sensor callback;
	void *user_data;
};

static int _module_for_each_sensor(struct skin_module *module, void *d)
{
	struct for_each_sensor_data *data = d;
	return skin_module_for_each_sensor(module, data->callback, data->user_data);
}

int (skin_patch_for_each_sensor)(struct skin_patch *patch, skin_callback_sensor callback, void *user_data, ...)
{
	return skin_patch_for_each_module(patch, _module_for_each_sensor, &(struct for_each_sensor_data){
				.callback = callback,
				.user_data = user_data,
			});
}
URT_EXPORT_SYMBOL(skin_patch_for_each_sensor);

int (skin_patch_for_each_module)(struct skin_patch *patch, skin_callback_module callback, void *user_data, ...)
{
	ENTITY_DATA_ITERATE(patch, module, modules);
}
URT_EXPORT_SYMBOL(skin_patch_for_each_module);

int (skin_module_for_each_sensor)(struct skin_module *module, skin_callback_sensor callback, void *user_data, ...)
{
	ENTITY_DATA_ITERATE(module, sensor, sensors);
}
URT_EXPORT_SYMBOL(skin_module_for_each_sensor);
