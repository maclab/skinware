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

#ifndef SKIN_READER_HPP
#define SKIN_READER_HPP

#include <functional>
#include <skin_reader.h>
#include "skin_types.hpp"

class SkinReader;
class SkinUser;

class SkinReaderAttr
{
public:
	SkinReaderAttr(const char *name)
	{
		attr.name = name;
	}
	SkinReaderAttr(const struct skin_reader_attr &a)
	{
		attr = a;
	}

	const char *getName() { return attr.name; }

	/* internal */
	struct skin_reader_attr attr;
};

class SkinReaderCallbacks
{
public:
	typedef std::function<void (SkinReader &, void *, size_t)> readCallback;
	typedef std::function<void (SkinReader &)> initCallback;
	typedef std::function<void (SkinReader &)> cleanCallback;
	SkinReaderCallbacks(readCallback rc, initCallback ic = NULL, cleanCallback cc = NULL):
		read(rc), init(ic), clean(cc) {}

	/* internal */
	readCallback read;
	initCallback init;
	cleanCallback clean;
};

class SkinReaderStatistics
{
public:
	urt_time startTime;
	urt_time worstWriteTime;
	urt_time bestWriteTime;
	urt_time accumulatedWriteTime;
	uint64_t readCount;

	SkinReaderStatistics() = default;
	SkinReaderStatistics(const SkinReaderStatistics &) = default;
	SkinReaderStatistics &operator =(const SkinReaderStatistics &) = default;
	SkinReaderStatistics(const struct skin_reader_statistics &stats)
	{
		*this = stats;
	}
	SkinReaderStatistics &operator =(const struct skin_reader_statistics &stats)
	{
		startTime = stats.start_time;
		worstWriteTime = stats.worst_read_time;
		bestWriteTime = stats.best_read_time;
		accumulatedWriteTime = stats.accumulated_read_time;
		readCount = stats.read_count;
		return *this;
	}
};

class Skin;
class SkinUser;

class SkinReader
{
public:
	SkinReader(): reader(NULL), skin(NULL) {}
	SkinReader(const SkinReader &) = default;
	SkinReader &operator =(const SkinReader &) = default;

	bool isValid() { return reader != NULL && skin != NULL; }

	int pause() { return skin_reader_pause(reader); }
	int resume() { return skin_reader_resume(reader); }
	bool isPaused() { return skin_reader_is_paused(reader); }
	bool isActive() { return skin_reader_is_active(reader); }
	int request(volatile sig_atomic_t *stop) { return skin_reader_request(reader, stop); }
	int requestNonblocking() { return skin_reader_request_nonblocking(reader); }
	int awaitResponse(volatile sig_atomic_t *stop) { return skin_reader_await_response(reader, stop); }

	SkinUser getUser();
	Skin &getSkin() { return *skin; }
	SkinReaderAttr getAttr()
	{
		struct skin_reader_attr attr;
		skin_reader_get_attr(reader, &attr);
		return attr;
	}
	SkinReaderStatistics getStatistics()
	{
		struct skin_reader_statistics stats;
		skin_reader_get_statistics(reader, &stats);
		return stats;
	}

	/* internal */
	SkinReader(struct skin_reader *r, Skin *s): reader(r), skin(s) {}

	struct skin_reader *reader;
	Skin *skin;
};

#endif
