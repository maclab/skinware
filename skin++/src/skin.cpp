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
#include <stdexcept>
#include <skin_base.hpp>

class WriterCallbacksWithSkin
{
public:
	WriterCallbacksWithSkin(const SkinWriterCallbacks &c, Skin *s): callbacks(c), skin(s) {}
	SkinWriterCallbacks callbacks;
	Skin *skin;
};

class ReaderCallbacksWithSkin
{
public:
	ReaderCallbacksWithSkin(const SkinReaderCallbacks &c, Skin *s): callbacks(c), skin(s) {}
	SkinReaderCallbacks callbacks;
	Skin *skin;
};

class DriverCallbacksWithSkin
{
public:
	DriverCallbacksWithSkin(const SkinDriverCallbacks &c, Skin *s): callbacks(c), skin(s) {}
	SkinDriverCallbacks callbacks;
	Skin *skin;
};

class UserCallbacksWithSkin
{
public:
	UserCallbacksWithSkin(const SkinUserCallbacks &c, Skin *s): callbacks(c), skin(s) {}
	SkinUserCallbacks callbacks;
	Skin *skin;
};

static int serviceWrite(struct skin_writer *writer, void *mem, size_t size, void *userData)
{
	auto callbacks = (WriterCallbacksWithSkin *)userData;
	SkinWriter w(writer, callbacks->skin);
	return callbacks->callbacks.write(w, mem, size);
}

static void writerInit(struct skin_writer *writer, void *userData)
{
	auto callbacks = (WriterCallbacksWithSkin *)userData;
	SkinWriter w(writer, callbacks->skin);
	if (callbacks->callbacks.init)
		callbacks->callbacks.init(w);
}

static void writerClean(struct skin_writer *writer, void *userData)
{
	auto callbacks = (WriterCallbacksWithSkin *)userData;
	SkinWriter w(writer, callbacks->skin);
	if (callbacks->callbacks.clean)
		callbacks->callbacks.clean(w);
	delete callbacks;
}

static void serviceRead(struct skin_reader *reader, void *mem, size_t size, void *userData)
{
	auto callbacks = (ReaderCallbacksWithSkin *)userData;
	SkinReader r(reader, callbacks->skin);
	callbacks->callbacks.read(r, mem, size);
}

static void readerInit(struct skin_reader *reader, void *userData)
{
	auto callbacks = (ReaderCallbacksWithSkin *)userData;
	SkinReader r(reader, callbacks->skin);
	if (callbacks->callbacks.init)
		callbacks->callbacks.init(r);
}

static void readerClean(struct skin_reader *reader, void *userData)
{
	auto callbacks = (ReaderCallbacksWithSkin *)userData;
	SkinReader r(reader, callbacks->skin);
	if (callbacks->callbacks.clean)
		callbacks->callbacks.clean(r);
	delete callbacks;
}

static int driverDetails(struct skin_driver *driver, bool revived, struct skin_driver_details *details, void *userData)
{
	auto callbacks = (DriverCallbacksWithSkin *)userData;
	SkinDriver d(driver, callbacks->skin);
	SkinDriverDetails det(details);
	return callbacks->callbacks.details(d, revived, det);
}

static int driverAcquire(skin_driver *driver, skin_sensor_response *responses, skin_sensor_size sensorCount, void *userData)
{
	auto callbacks = (DriverCallbacksWithSkin *)userData;
	SkinDriver d(driver, callbacks->skin);
	return callbacks->callbacks.acquire(d, responses, sensorCount);
}

static void driverInit(struct skin_driver *driver, void *userData)
{
	auto callbacks = (DriverCallbacksWithSkin *)userData;
	SkinDriver d(driver, callbacks->skin);
	if (callbacks->callbacks.init)
		callbacks->callbacks.init(d);
}

static void driverClean(struct skin_driver *driver, void *userData)
{
	auto callbacks = (DriverCallbacksWithSkin *)userData;
	SkinDriver d(driver, callbacks->skin);
	if (callbacks->callbacks.clean)
		callbacks->callbacks.clean(d);
	delete callbacks;
}

static void driverWriterInit(struct skin_writer *writer, void *userData)
{
	auto callbacks = (DriverCallbacksWithSkin *)userData;
	SkinWriter w(writer, callbacks->skin);
	if (callbacks->callbacks.writerInit)
		callbacks->callbacks.writerInit(w);
}

static void driverWriterClean(struct skin_writer *writer, void *userData)
{
	auto callbacks = (DriverCallbacksWithSkin *)userData;
	SkinWriter w(writer, callbacks->skin);
	if (callbacks->callbacks.writerClean)
		callbacks->callbacks.writerClean(w);
}

static void userPeek(skin_user *user, skin_sensor_response *responses, skin_sensor_size sensorCount, void *userData)
{
	auto callbacks = (UserCallbacksWithSkin *)userData;
	SkinUser d(user, callbacks->skin);
	callbacks->callbacks.peek(d, responses, sensorCount);
}

static void userInit(struct skin_user *user, void *userData)
{
	auto callbacks = (UserCallbacksWithSkin *)userData;
	SkinUser u(user, callbacks->skin);
	if (callbacks->callbacks.init)
		callbacks->callbacks.init(u);
}

static void userClean(struct skin_user *user, void *userData)
{
	auto callbacks = (UserCallbacksWithSkin *)userData;
	SkinUser u(user, callbacks->skin);
	if (callbacks->callbacks.clean)
		callbacks->callbacks.clean(u);
	delete callbacks;
}

static void userReaderInit(struct skin_reader *reader, void *userData)
{
	auto callbacks = (UserCallbacksWithSkin *)userData;
	SkinReader r(reader, callbacks->skin);
	if (callbacks->callbacks.readerInit)
		callbacks->callbacks.readerInit(r);
}

static void userReaderClean(struct skin_reader *reader, void *userData)
{
	auto callbacks = (UserCallbacksWithSkin *)userData;
	SkinReader r(reader, callbacks->skin);
	if (callbacks->callbacks.readerClean)
		callbacks->callbacks.readerClean(r);
}

SkinWriter Skin::add(const SkinWriterAttr &attr, const urt_task_attr &taskAttr,
		const SkinWriterCallbacks &callbacks, int *error)
{
	auto extra = new WriterCallbacksWithSkin(callbacks, this);
	struct skin_writer_callbacks c = {
		serviceWrite,
		writerInit,
		writerClean,
		extra,
	};

	struct skin_writer *writer = skin_service_add(skin, &attr.attr, &taskAttr, &c, error);
	if (writer == NULL)
		return SkinWriter();

	return SkinWriter(writer, this);
}

SkinReader Skin::attach(const SkinReaderAttr &attr, const urt_task_attr &taskAttr,
		const SkinReaderCallbacks &callbacks, int *error)
{
	auto extra = new ReaderCallbacksWithSkin(callbacks, this);
	struct skin_reader_callbacks c = {
		serviceRead,
		readerInit,
		readerClean,
		extra,
	};

	struct skin_reader *reader = skin_service_attach(skin, &attr.attr, &taskAttr, &c, error);
	if (reader == NULL)
		return SkinReader();

	return SkinReader(reader, this);
}

SkinDriver Skin::add(const SkinDriverAttr &attr, const SkinWriterAttr &writerAttr, const urt_task_attr &taskAttr,
		const SkinDriverCallbacks &callbacks, int *error)
{
	auto extra = new DriverCallbacksWithSkin(callbacks, this);
	struct skin_driver_callbacks c = {
		driverDetails,
		driverAcquire,
		driverInit,
		driverClean,
		driverWriterInit,
		driverWriterClean,
		extra,
	};

	struct skin_driver *driver = skin_driver_add(skin, &attr.attr, &writerAttr.attr, &taskAttr, &c, error);
	if (driver == NULL)
		return SkinDriver();

	return SkinDriver(driver, this);
}

SkinUser Skin::attach(const SkinUserAttr &attr, const SkinReaderAttr &readerAttr, const urt_task_attr &taskAttr,
		const SkinUserCallbacks &callbacks, int *error)
{
	auto extra = new UserCallbacksWithSkin(callbacks, this);
	struct skin_user_callbacks c = {
		userPeek,
		userInit,
		userClean,
		userReaderInit,
		userReaderClean,
		extra,
	};

	struct skin_user *user = skin_driver_attach(skin, &attr.attr, &readerAttr.attr, &taskAttr, &c, error);
	if (user == NULL)
		return SkinUser();

	return SkinUser(user, this);
}
