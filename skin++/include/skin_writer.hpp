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

#ifndef SKIN_WRITER_HPP
#define SKIN_WRITER_HPP

#include <functional>
#include <skin_writer.h>
#include "skin_types.hpp"

class SkinWriter;
class SkinDriver;

class SkinWriterAttr
{
public:
	SkinWriterAttr(size_t bufferSize, uint8_t bufferCount, const char *name)
	{
		attr.buffer_size = bufferSize;
		attr.buffer_count = bufferCount;
		attr.name = name;
	}
	SkinWriterAttr(const struct skin_writer_attr &a)
	{
		attr = a;
	}

	size_t getBufferSize() { return attr.buffer_size; }
	uint8_t getBufferCount() { return attr.buffer_count; }
	const char *getName() { return attr.name; }

	/* internal */
	struct skin_writer_attr attr;
};

class SkinWriterCallbacks
{
public:
	typedef std::function<int (SkinWriter &, void *, size_t)> writeCallback;
	typedef std::function<void (SkinWriter &)> initCallback;
	typedef std::function<void (SkinWriter &)> cleanCallback;
	SkinWriterCallbacks(writeCallback wc, initCallback ic = NULL, cleanCallback cc = NULL):
		write(wc), init(ic), clean(cc) {}

	/* internal */
	writeCallback write;
	initCallback init;
	cleanCallback clean;
};

class SkinWriterStatistics
{
public:
	urt_time startTime;
	urt_time worstWriteTime;
	urt_time bestWriteTime;
	urt_time accumulatedWriteTime;
	uint64_t writeCount;
	uint64_t swapSkips;

	SkinWriterStatistics() = default;
	SkinWriterStatistics(const SkinWriterStatistics &) = default;
	SkinWriterStatistics &operator =(const SkinWriterStatistics &) = default;
	SkinWriterStatistics(const struct skin_writer_statistics &stats)
	{
		*this = stats;
	}
	SkinWriterStatistics &operator =(const struct skin_writer_statistics &stats)
	{
		startTime = stats.start_time;
		worstWriteTime = stats.worst_write_time;
		bestWriteTime = stats.best_write_time;
		accumulatedWriteTime = stats.accumulated_write_time;
		writeCount = stats.write_count;
		swapSkips = stats.swap_skips;
		return *this;
	}
};

class Skin;
class SkinDriver;

class SkinWriter
{
public:
	SkinWriter(): writer(NULL), skin(NULL) {}
	SkinWriter(const SkinWriter &) = default;
	SkinWriter &operator =(const SkinWriter &) = default;

	bool isValid() { return writer != NULL && skin != NULL; }

	int pause() { return skin_writer_pause(writer); }
	int resume() { return skin_writer_resume(writer); }
	bool isPaused() { return skin_writer_is_paused(writer); }
	bool isActive() { return skin_writer_is_active(writer); }
	int request(volatile sig_atomic_t *stop = NULL) { return skin_writer_request(writer, stop); }
	int requestNonblocking() { return skin_writer_request_nonblocking(writer); }
	int awaitResponse(volatile sig_atomic_t *stop = NULL) { return skin_writer_await_response(writer, stop); }

	SkinDriver getDriver();
	Skin &getSkin() { return *skin; }
	SkinWriterAttr getAttr()
	{
		struct skin_writer_attr attr;
		skin_writer_get_attr(writer, &attr);
		return attr;
	}
	SkinWriterStatistics getStatistics()
	{
		struct skin_writer_statistics stats;
		skin_writer_get_statistics(writer, &stats);
		return stats;
	}

	void copyLastBuffer() { skin_writer_copy_last_buffer(writer); }

	/* internal */
	SkinWriter(struct skin_writer *w, Skin *s): writer(w), skin(s) {}

	struct skin_writer *writer;
	Skin *skin;
};

#endif
