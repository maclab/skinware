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

#ifndef READER_INTERNAL_H
#define READER_INTERNAL_H

#include <skin_reader.h>
#include "config.h"

struct skin_user;

/* internal information on readers */
struct skin_reader
{
	/* task control */
	volatile sig_atomic_t must_stop;	/* if true, task will quit */
	bool running;				/* if true, task is not yet terminated */
	bool must_pause;			/* if true, task will pause */
	bool paused;				/* if true, task is paused */
	bool soft;				/* whether its a soft real-time reader */
	urt_time period;			/* period, if periodic */
	urt_task *task;				/* the real-time task for this reader */
	/* synchronization */
	urt_rwlock *rwls[SKIN_CONFIG_MAX_BUFFERS];
						/* rwlocks for synchronization */
	urt_sem *writer_request,		/* request and response */
		*writer_response;		/* semaphores for sporadic writers */
	urt_sem *request,			/* request and response */
		*response;			/* semaphores for sporadic tasks */
	void *mem;				/* shared memory for reader */
	/* acquisition */
	struct skin_reader_callbacks callbacks;
	/* references */
	struct skin *skin;			/* reference back to the skin object */
	uint16_t writer_index;			/* index to writer_info in skin kernel */
	struct skin_user *user;			/* if a user reader, reference to the user */
	uint16_t index;				/* index to skin's list of readers */
	/* statistics */
	struct skin_reader_statistics stats;
	urt_mutex *stats_lock;
};

#endif
