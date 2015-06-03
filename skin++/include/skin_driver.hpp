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

#ifndef SKIN_DRIVER_HPP
#define SKIN_DRIVER_HPP

#include <functional>
#include <skin_driver.h>
#include "skin_types.hpp"
#include "skin_writer.hpp"

class SkinDriver;

class SkinDriverAttr
{
public:
	SkinDriverAttr(SkinPatchSize patchCount, SkinModuleSize moduleCount, SkinSensorSize sensorCount)
	{
		attr.patch_count = patchCount;
		attr.module_count = moduleCount;
		attr.sensor_count = sensorCount;
	}
	SkinDriverAttr(const struct skin_driver_attr &a)
	{
		attr = a;
	}
	SkinDriverAttr(): SkinDriverAttr(0, 0, 0) {}

	SkinPatchSize getPatchCount() { return attr.patch_count; }
	SkinModuleSize getModuleCount() { return attr.module_count; }
	SkinSensorSize getSensorCount() { return attr.sensor_count; }

	/* internal */
	struct skin_driver_attr attr;
};

class SkinPatchDecl
{
public:
	SkinModuleSize getModuleCount() { return decl.module_count; }
	void setModuleCount(SkinModuleSize count) { decl.module_count = count; }

	/* internal */
	struct skin_patch_decl decl;
};

class SkinModuleDecl
{
public:
	SkinSensorSize getSensorCount() { return decl.sensor_count; }
	void setSensorCount(SkinSensorSize count) { decl.sensor_count = count; }

	/* internal */
	struct skin_module_decl decl;
};

class SkinSensorDecl
{
public:
	SkinSensorUniqueId getUniqueId() { return decl.uid; }
	SkinSensorTypeId getType() { return decl.type; }
	void setUniqueId(SkinSensorUniqueId uid) { decl.uid = uid; }
	void setType(SkinSensorTypeId type) { decl.type = type; }

	/* internal */
	struct skin_sensor_decl decl;
};

/*
 * Note: The driver details is a shared memory created in C with skin_*_decl structs, but is being viewed by
 * the C++ user as if it consisted of Skin*Decl classes.  The following assertions make sure the memory
 * layout is the same.
 */
static_assert(sizeof(struct skin_patch_decl) == sizeof(SkinPatchDecl), "Memory layout would mismatch that of C's");
static_assert(sizeof(struct skin_module_decl) == sizeof(SkinModuleDecl), "Memory layout would mismatch that of C's");
static_assert(sizeof(struct skin_sensor_decl) == sizeof(SkinSensorDecl), "Memory layout would mismatch that of C's");

class SkinDriverDetails
{
public:
	SkinDriverAttr overall;
	SkinPatchDecl *patches;
	SkinModuleDecl *modules;
	SkinSensorDecl *sensors;
	SkinDriverDetails(): overall(), patches(NULL), modules(NULL), sensors(NULL) {}
	SkinDriverDetails(struct skin_driver_details *details): overall(details->overall),
		patches((SkinPatchDecl *)details->patches),
		modules((SkinModuleDecl *)details->modules),
		sensors((SkinSensorDecl *)details->sensors) {}
};

class SkinDriverCallbacks
{
public:
	typedef std::function<int (SkinDriver &, bool, SkinDriverDetails &)> detailsCallback;
	typedef std::function<int (SkinDriver &, SkinSensorResponse *, SkinSensorSize)> acquireCallback;
	typedef std::function<void (SkinDriver &)> initCallback;
	typedef std::function<void (SkinDriver &)> cleanCallback;
	typedef std::function<void (SkinWriter &)> writerInitCallback;
	typedef std::function<void (SkinWriter &)> writerCleanCallback;
	SkinDriverCallbacks(detailsCallback dc, acquireCallback ac, initCallback ic = NULL, cleanCallback cc = NULL,
			writerInitCallback wic = NULL, writerCleanCallback wcc = NULL):
		details(dc), acquire(ac), init(ic), clean(cc), writerInit(wic), writerClean(wcc) {}

	/* internal */
	detailsCallback details;
	acquireCallback acquire;
	initCallback init;
	cleanCallback clean;
	writerInitCallback writerInit;
	writerCleanCallback writerClean;
};

class Skin;
class SkinWriter;

class SkinDriver
{
public:
	SkinDriver(): driver(NULL), skin(NULL) {}
	SkinDriver(const SkinDriver &) = default;
	SkinDriver &operator =(const SkinDriver &) = default;

	bool isValid() { return driver != NULL && skin != NULL; }

	int pause() { return skin_driver_pause(driver); }
	int resume() { return skin_driver_resume(driver); }
	bool isPaused() { return skin_driver_is_paused(driver); }
	bool isActive() { return skin_driver_is_active(driver); }

	SkinWriter getWriter() { return SkinWriter(skin_driver_get_writer(driver), skin); }
	Skin &getSkin() { return *skin; }
	SkinDriverAttr getAttr()
	{
		struct skin_driver_attr attr;
		skin_driver_get_attr(driver, &attr);
		return attr;
	}

	void copyLastBuffer() { skin_driver_copy_last_buffer(driver); }

	/* internal */
	SkinDriver(struct skin_driver *d, Skin *s): driver(d), skin(s) {}

	struct skin_driver *driver;
	Skin *skin;
};

#endif
