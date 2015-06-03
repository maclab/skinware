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

#ifndef SKIN_READER_H
#define SKIN_READER_H

#include "skin_types.h"

URT_DECL_BEGIN

struct skin_reader;
struct skin_user;

struct skin_reader_attr
{
	const char *name;			/*
						 * name prefix of writer (max URT_NAME_LEN - 3 characters).
						 * The reader tries to attach to the memory and locks created
						 * by the writer with this prefix.
						 */
};

struct skin_reader_callbacks
{
	void (*read)(struct skin_reader *reader, void *mem, size_t size, void *user_data);
						/*
						 * a function that will be called by the reader thread.
						 * It should read the memory in whatever way the writer
						 * is expected to have filled it.
						 */
	void (*init)(struct skin_reader *reader, void *user_data);
						/* a function to be called after a reader is created */
	void (*clean)(struct skin_reader *reader, void *user_data);
						/* a function to be called before a reader is removed */
	void *user_data;
};

struct skin_reader_statistics
{
	urt_time start_time;			/* start time of reader */
	urt_time worst_read_time;		/* worst, best and accumulated */
	urt_time best_read_time;		/* execution time */
	urt_time accumulated_read_time;		/* of the readers */
	uint64_t read_count;			/* number of frame reads since spawn */
};

/*
 * readers are created and removed with skin_service_attach/detach.  They are controlled with
 * skin_reader_* functions.
 *
 * pause		pause a reader.  The reader is still available and the readers get notified
 *			of the fact that the reader is paused.
 * resume		resume a reader.  A reader that has been just created is initially in a paused
 *			state and should be resumed to actually start working.
 * is_paused		whether the reader is paused or not.
 * is_active		whether the writer of this reader is active.
 * request		for sporadic readers, requests a write to be done and wait for it to finish,
 *			which is equivalent to a call to request_nonblocking followed by await_response
 * request_nonblocking	send a request to the reader but doesn't await its response.  The request must
 *			be completed with await_response.
 * await_response	await a response from the reader.  This is the complementary function of
 *			request_nonblocking.
 *
 * get_user		if the reader belongs to a user, this would return the user object
 * get_attr		get the attributes with which the reader is initialized.  The name attribute
 *			is valid only while the reader is alive.
 * get_statistics	return reader statistics
 */
int skin_reader_pause(struct skin_reader *reader);
int skin_reader_resume(struct skin_reader *reader);
bool skin_reader_is_paused(struct skin_reader *reader);
bool skin_reader_is_active(struct skin_reader *reader);
#define skin_reader_request(...) skin_reader_request(__VA_ARGS__, NULL)
int (skin_reader_request)(struct skin_reader *reader, volatile sig_atomic_t *stop, ...);
int skin_reader_request_nonblocking(struct skin_reader *reader);
#define skin_reader_await_response(...) skin_reader_await_response(__VA_ARGS__, NULL)
int (skin_reader_await_response)(struct skin_reader *reader, volatile sig_atomic_t *stop, ...);

struct skin_user *skin_reader_get_user(struct skin_reader *reader);
int skin_reader_get_attr(struct skin_reader *reader, struct skin_reader_attr *attr);
int skin_reader_get_statistics(struct skin_reader *reader, struct skin_reader_statistics *stats);

/* internal */
void skin_reader_acquisition_task(urt_task *task, void *data);

URT_DECL_END

#endif
