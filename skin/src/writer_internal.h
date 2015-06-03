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

#ifndef WRITER_INTERNAL_H
#define WRITER_INTERNAL_H

#include <skin_writer.h>
#include "config.h"

struct skin_driver;

struct skin_writer_attr_internal
{
	size_t buffer_size;
	uint8_t buffer_count;
	char prefix[URT_NAME_LEN - 3 + 1];
};

/* data shared with readers */
struct skin_writer_info
{
	/* information */
	struct skin_writer_attr_internal attr;
	urt_time period;			/* period, if periodic */
	unsigned int readers_attached;		/* number of readers still attached */
	bool bad;				/* the writer callback has declared failure */
	uint16_t driver_index;			/*
						 * if writer of a driver, the driver's index,
						 * otherwise an invalidly large value
						 */
	/* synchronization */
	urt_time write_times[SKIN_CONFIG_MAX_BUFFERS];
						/* last write time on each buffer */
	uint8_t last_written_buffer;		/* the buffer with the latest data */
	uint8_t buffer_being_written;		/* the buffer currently being filled */
	bool active;				/* whether writer is active */
	bool paused;				/* whether writer is paused */
	urt_time next_predicted_swap;		/* when the next swap is expected to happen */
};

/* internal information on writers */
struct skin_writer
{
	/* task control */
	volatile sig_atomic_t must_stop;	/* if true, task will quit */
	bool running;				/* if true, task is not yet terminated */
	bool must_pause;			/* if true, task will pause */
	bool paused;				/* if true, task is paused */
	urt_task *task;				/* the real-time task for this writer */
	/* synchronization */
	urt_rwlock *rwls[SKIN_CONFIG_MAX_BUFFERS];
						/* rwlocks for synchronization */
	urt_sem *request,			/* request and response */
		*response;			/* semaphores for sporadic tasks */
	void *mem;				/* shared memory for writer */
	/* acquisition */
	struct skin_writer_callbacks callbacks;
	/* references */
	struct skin *skin;			/* reference back to the skin object */
	uint16_t info_index;			/* index to writer_info in skin kernel */
	struct skin_driver *driver;		/* if a driver writer, reference to the driver */
	uint16_t index;				/* index to skin's list of writers */
	/* statistics */
	struct skin_writer_statistics stats;
	urt_mutex *stats_lock;
};

#endif
