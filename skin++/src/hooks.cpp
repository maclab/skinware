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

#define DEFINE_HOOK_FUNCS(object, Object)				\
static void object##Init(struct skin_##object *object, void *userData)	\
{									\
	Skin *skin = (Skin *)userData;					\
	if (skin->object##InitHook)					\
		skin->object##InitHook(Skin##Object(object, skin));	\
}									\
static void object##Clean(struct skin_##object *object, void *userData)	\
{									\
	Skin *skin = (Skin *)userData;					\
	if (skin->object##CleanHook)					\
		skin->object##CleanHook(Skin##Object(object, skin));	\
}

DEFINE_HOOK_FUNCS(writer, Writer)
DEFINE_HOOK_FUNCS(reader, Reader)
DEFINE_HOOK_FUNCS(driver, Driver)
DEFINE_HOOK_FUNCS(user, User)
DEFINE_HOOK_FUNCS(sensor, Sensor)
DEFINE_HOOK_FUNCS(module, Module)
DEFINE_HOOK_FUNCS(patch, Patch)

void Skin::setWriterInitHook(SkinHook<SkinWriter> hook)
{
	writerInitHook = hook;
	skin_set_writer_init_hook(skin, writerInit, this);
}

void Skin::setWriterCleanHook(SkinHook<SkinWriter> hook)
{
	writerCleanHook = hook;
	skin_set_writer_clean_hook(skin, writerClean, this);
}

void Skin::setReaderInitHook(SkinHook<SkinReader> hook)
{
	readerInitHook = hook;
	skin_set_reader_init_hook(skin, readerInit, this);
}

void Skin::setReaderCleanHook(SkinHook<SkinReader> hook)
{
	readerCleanHook = hook;
	skin_set_reader_clean_hook(skin, readerClean, this);
}

void Skin::setDriverInitHook(SkinHook<SkinDriver> hook)
{
	driverInitHook = hook;
	skin_set_driver_init_hook(skin, driverInit, this);
}

void Skin::setDriverCleanHook(SkinHook<SkinDriver> hook)
{
	driverCleanHook = hook;
	skin_set_driver_clean_hook(skin, driverClean, this);
}

void Skin::setUserInitHook(SkinHook<SkinUser> hook)
{
	userInitHook = hook;
	skin_set_user_init_hook(skin, userInit, this);
}

void Skin::setUserCleanHook(SkinHook<SkinUser> hook)
{
	userCleanHook = hook;
	skin_set_user_clean_hook(skin, userClean, this);
}

void Skin::setSensorInitHook(SkinHook<SkinSensor> hook)
{
	sensorInitHook = hook;
	skin_set_sensor_init_hook(skin, sensorInit, this);
}

void Skin::setSensorCleanHook(SkinHook<SkinSensor> hook)
{
	sensorCleanHook = hook;
	skin_set_sensor_clean_hook(skin, sensorClean, this);
}

void Skin::setModuleInitHook(SkinHook<SkinModule> hook)
{
	moduleInitHook = hook;
	skin_set_module_init_hook(skin, moduleInit, this);
}

void Skin::setModuleCleanHook(SkinHook<SkinModule> hook)
{
	moduleCleanHook = hook;
	skin_set_module_clean_hook(skin, moduleClean, this);
}

void Skin::setPatchInitHook(SkinHook<SkinPatch> hook)
{
	patchInitHook = hook;
	skin_set_patch_init_hook(skin, patchInit, this);
}

void Skin::setPatchCleanHook(SkinHook<SkinPatch> hook)
{
	patchCleanHook = hook;
	skin_set_patch_clean_hook(skin, patchClean, this);
}
