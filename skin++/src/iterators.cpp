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

#define URT_LOG_PREFIX "skin++: "
#include <skin_base.hpp>
#include <skin_sensor.hpp>
#include <skin_module.hpp>
#include <skin_patch.hpp>
#include <skin_sensor_type.hpp>

template<class Object>
class data
{
public:
	data(SkinCallback<Object> c, Skin *s): callback(c), skin(s) {}
	SkinCallback<Object> callback;
	Skin *skin;
};

template<class object, class Object>
static int callbackFunc(object *obj, void *userData)
{
	auto d = (data<Object> *)userData;
	Object o(obj, d->skin);
	return d->callback(o);
}

#define DEFINE_ITERATOR(o, owner_, Owner, object, Object, skin)							\
int Skin##Owner::forEach##Object(SkinCallback<Skin##Object> callback)						\
{														\
	data<Skin##Object> d(callback, skin);									\
	return skin_##owner_##for_each_##object(o, callbackFunc<struct skin_##object, Skin##Object>, &d);	\
}

#define DEFINE_ITERATOR_SENSOR_OF_TYPE(o, owner_, Owner, skin)								\
int Skin##Owner::forEachSensorOfType(SkinSensorTypeId type, SkinCallback<SkinSensor> callback)				\
{															\
	data<SkinSensor> d(callback, skin);										\
	return skin_##owner_##for_each_sensor_of_type(o, type, callbackFunc<struct skin_sensor, SkinSensor>, &d);	\
}

DEFINE_ITERATOR(module, module_, Module, sensor, Sensor, skin)
DEFINE_ITERATOR(patch, patch_, Patch, sensor, Sensor, skin)
DEFINE_ITERATOR(patch, patch_, Patch, module, Module, skin)
DEFINE_ITERATOR(user, user_, User, sensor, Sensor, skin)
DEFINE_ITERATOR(user, user_, User, module, Module, skin)
DEFINE_ITERATOR(user, user_, User, patch, Patch, skin)
DEFINE_ITERATOR(user, user_, User, sensor_type, SensorType, skin)
DEFINE_ITERATOR_SENSOR_OF_TYPE(user, user_, User, skin)
DEFINE_ITERATOR(skin,,, sensor, Sensor, this)
DEFINE_ITERATOR(skin,,, module, Module, this)
DEFINE_ITERATOR(skin,,, patch, Patch, this)
DEFINE_ITERATOR(skin,,, sensor_type, SensorType, this)
DEFINE_ITERATOR_SENSOR_OF_TYPE(skin,,, this)
DEFINE_ITERATOR(skin,,, writer, Writer, this)
DEFINE_ITERATOR(skin,,, reader, Reader, this)
DEFINE_ITERATOR(skin,,, driver, Driver, this)
DEFINE_ITERATOR(skin,,, user, User, this)
