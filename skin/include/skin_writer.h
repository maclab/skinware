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

#ifndef SKIN_WRITER_H
#define SKIN_WRITER_H

#include "skin_types.h"

URT_DECL_BEGIN

struct skin_writer;
struct skin_driver;

struct skin_writer_attr
{
	size_t buffer_size;			/* writer's memory size (of a single buffer) */
	uint8_t buffer_count;			/* how many buffers it should have (0 for default) */
	const char *name;			/*
						 * name prefix of writer (max URT_NAME_LEN - 3 characters).
						 * The names used by the writer will have this as prefix.
						 */
};

struct skin_writer_callbacks
{
	int (*write)(struct skin_writer *writer, void *mem, size_t size, void *user_data);
						/*
						 * a function that will be called by the writer thread.
						 * It should fill in the memory in whatever way it sees fit.
						 */
	void (*init)(struct skin_writer *writer, void *user_data);
						/* a function to be called after a writer is created */
	void (*clean)(struct skin_writer *writer, void *user_data);
						/* a function to be called before a writer is removed */
	void *user_data;
};

struct skin_writer_statistics
{
	urt_time start_time;			/* start time of writer */
	urt_time worst_write_time;		/* worst, best and accumulated */
	urt_time best_write_time;		/* execution time */
	urt_time accumulated_write_time;	/* of the writers */
	uint64_t write_count;			/* number of frame writes since spawn */
	uint64_t swap_skips;			/* number of times swap was skipped */
};

/*
 * writers are created and removed with skin_service_add/remove.  They are controlled with
 * skin_writer_* functions.
 *
 * pause		pause a writer.  The writer is still available and the readers get notified
 *			of the fact that the writer is paused.
 * resume		resume a writer.  A writer that has been just created is initially in a paused
 *			state and should be resumed to actually start working.
 * is_paused		whether the writer is paused or not.
 * request		for sporadic writers, requests a write to be done and wait for it to finish,
 *			which is equivalent to a call to request_nonblocking followed by await_response
 * request_nonblocking	send a request to the writer but doesn't await its response.  The request must
 *			be completed with await_response.
 * await_response	await a response from the writer.  This is the complementary function of
 *			request_nonblocking.
 *
 * get_driver		if the writer belongs to a driver, this would return the driver object
 * get_attr		get the attributes with which the writer is initialized.  The name attribute
 *			is valid only while the writer is alive.
 * get_statistics	return writer statistics
 *
 * copy_last_buffer	copy data of last buffer in current buffer
 */
int skin_writer_pause(struct skin_writer *writer);
int skin_writer_resume(struct skin_writer *writer);
bool skin_writer_is_paused(struct skin_writer *writer);
bool skin_writer_is_active(struct skin_writer *writer);
#define skin_writer_request(...) skin_writer_request(__VA_ARGS__, NULL)
int (skin_writer_request)(struct skin_writer *writer, volatile sig_atomic_t *stop, ...);
int skin_writer_request_nonblocking(struct skin_writer *writer);
#define skin_writer_await_response(...) skin_writer_await_response(__VA_ARGS__, NULL)
int (skin_writer_await_response)(struct skin_writer *writer, volatile sig_atomic_t *stop, ...);

struct skin_driver *skin_writer_get_driver(struct skin_writer *writer);
int skin_writer_get_attr(struct skin_writer *writer, struct skin_writer_attr *attr);
int skin_writer_get_statistics(struct skin_writer *writer, struct skin_writer_statistics *stats);

void skin_writer_copy_last_buffer(struct skin_writer *writer);

/* internal */
void skin_writer_acquisition_task(urt_task *task, void *data);

URT_DECL_END

#endif
