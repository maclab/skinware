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

#ifndef SKIN_OBJECT_HPP
#define SKIN_OBJECT_HPP

#include <map>
#include <mutex>
#include <skin_base.h>
#include "skin_types.hpp"
#include "skin_hooks.hpp"
#include "skin_callbacks.hpp"
#include "skin_writer.hpp"
#include "skin_reader.hpp"
#include "skin_driver.hpp"
#include "skin_user.hpp"

class SkinSensor;
class SkinModule;
class SkinPatch;
class SkinSensorType;

class Skin
{
public:
	Skin(): skin(NULL) {}
	Skin(const Skin &) = delete;
	Skin(Skin &&) = default;
	Skin &operator =(const Skin &) = delete;
	Skin &operator =(Skin &&) = default;
	~Skin() {}

	struct skin *getSkin() { return skin; }

	/*
	 * Note: the constructor/destructor don't do anything because urt_init/exit need to be
	 * called before/after it.  If the object is global or static, or ends in the same scope where
	 * URT is initialized/exited, this is simply impossible.  That is why the init/free functions
	 * exist.
	 */
	int init(const char *name = NULL) { int err = 0; skin = skin_init(name, &err); return err; }
	void free() { skin_free(skin); skin = NULL; }

	/* services */
	SkinWriter add(const SkinWriterAttr &attr, const urt_task_attr &taskAttr,
			const SkinWriterCallbacks &callbacks, int *error = NULL);
	void remove(SkinWriter &writer) { skin_service_remove(writer.writer); }
	SkinReader attach(const SkinReaderAttr &attr, const urt_task_attr &taskAttr,
			const SkinReaderCallbacks &callbacks, int *error = NULL);
	void detach(SkinReader &reader) { skin_service_detach(reader.reader); }

	/* drivers */
	SkinDriver add(const SkinDriverAttr &attr, const SkinWriterAttr &writerAttr, const urt_task_attr &taskAttr,
			const SkinDriverCallbacks &callbacks, int *error = NULL);
	void remove(SkinDriver &driver) { skin_driver_remove(driver.driver); }
	SkinUser attach(const SkinUserAttr &attr, const SkinReaderAttr &readerAttr, const urt_task_attr &taskAttr,
			const SkinUserCallbacks &callbacks, int *error = NULL);
	void detach(SkinUser &user) { skin_driver_detach(user.user); }

	/* helpers */
	int load(const urt_task_attr &taskAttr) { return skin_load(skin, &taskAttr); }
	void unload() { skin_unload(skin); }
	int update(const urt_task_attr &taskAttr) { return skin_update(skin, &taskAttr); }

	void pause() { skin_pause(skin); }
	void resume() { skin_resume(skin); }

	void request(volatile sig_atomic_t *stop) { skin_request(skin, stop); }

	/* info */
	size_t writerCount() { return skin_writer_count(skin); }
	size_t readerCount() { return skin_reader_count(skin); }
	size_t driverCount() { return skin_driver_count(skin); }
	size_t userCount() { return skin_user_count(skin); }
	SkinSensorSize sensorCount() { return skin_sensor_count(skin); }
	SkinModuleSize moduleCount() { return skin_module_count(skin); }
	SkinPatchSize patchCount() { return skin_patch_count(skin); }
	SkinSensorTypeSize sensorTypeCount() { return skin_sensor_type_count(skin); }

	/* hooks */
	void setWriterInitHook(SkinHook<SkinWriter> hook);
	void setWriterCleanHook(SkinHook<SkinWriter> hook);
	void setReaderInitHook(SkinHook<SkinReader> hook);
	void setReaderCleanHook(SkinHook<SkinReader> hook);
	void setDriverInitHook(SkinHook<SkinDriver> hook);
	void setDriverCleanHook(SkinHook<SkinDriver> hook);
	void setUserInitHook(SkinHook<SkinUser> hook);
	void setUserCleanHook(SkinHook<SkinUser> hook);
	void setSensorInitHook(SkinHook<SkinSensor> hook);
	void setSensorCleanHook(SkinHook<SkinSensor> hook);
	void setModuleInitHook(SkinHook<SkinModule> hook);
	void setModuleCleanHook(SkinHook<SkinModule> hook);
	void setPatchInitHook(SkinHook<SkinPatch> hook);
	void setPatchCleanHook(SkinHook<SkinPatch> hook);

	int forEachWriter(SkinCallback<SkinWriter> callback);
	int forEachReader(SkinCallback<SkinReader> callback);
	int forEachDriver(SkinCallback<SkinDriver> callback);
	int forEachUser(SkinCallback<SkinUser> callback);

	int forEachSensor(SkinCallback<SkinSensor> callback);
	int forEachModule(SkinCallback<SkinModule> callback);
	int forEachPatch(SkinCallback<SkinPatch> callback);
	int forEachSensorType(SkinCallback<SkinSensorType> callback);
	int forEachSensorOfType(SkinSensorTypeId type, SkinCallback<SkinSensor> callback);

	/* internal */

	/* skin handler */
	struct skin *skin;

	SkinHook<SkinWriter> writerInitHook;
	SkinHook<SkinWriter> writerCleanHook;
	SkinHook<SkinReader> readerInitHook;
	SkinHook<SkinReader> readerCleanHook;
	SkinHook<SkinDriver> driverInitHook;
	SkinHook<SkinDriver> driverCleanHook;
	SkinHook<SkinUser> userInitHook;
	SkinHook<SkinUser> userCleanHook;
	SkinHook<SkinSensor> sensorInitHook;
	SkinHook<SkinSensor> sensorCleanHook;
	SkinHook<SkinModule> moduleInitHook;
	SkinHook<SkinModule> moduleCleanHook;
	SkinHook<SkinPatch> patchInitHook;
	SkinHook<SkinPatch> patchCleanHook;
};

#endif
