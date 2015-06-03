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

#define URT_LOG_PREFIX "skin: "
#include <skin_reader.h>
#include "internal.h"
#include "reader_internal.h"
#include "names.h"

static int _sanity_check_reader(struct skin_reader *reader, bool is_sporadic, bool writer_is_sporadic)
{
	return reader == NULL || reader->task == NULL || reader->skin == NULL || reader->skin->kernel == NULL
		|| (is_sporadic && (reader->request == NULL || reader->response == NULL))
		|| (writer_is_sporadic && (reader->writer_request == NULL || reader->writer_response == NULL))?-1:0;
}

static inline bool _buffer_data_is_new(uint8_t cur_buf, urt_time cur_timestamp,
		uint8_t last_buf, urt_time last_timestamp,
		struct skin_reader *reader, struct skin_writer_info *writer_info,
		bool do_swap_prediction, urt_time swap_protection_time)
{
	/*
	 * the buffer is new if either it's a different buffer or has a new timestamp.  If the timestamp is 0, the
	 * buffer has never been written to.  Still, if the buffer is new, but the reader suspects it cannot finish
	 * the read in time, then pretend that the buffer is not new, so the reader waits for the next swap.
	 */
	return cur_timestamp != 0 && (cur_buf != last_buf || cur_timestamp != last_timestamp)
		&& !(do_swap_prediction && urt_get_time() + swap_protection_time > writer_info->next_predicted_swap);
}

/*
 * the synchronization mechanism in the reader with the writer is as follows:
 *
 * Single Buffer:
 *
 *		Periodic Reader			Sporadic Reader			Soft Reader
 *
 * Periodic	S1. loop {			S2. loop {			S3. loop {
 * Writer						wait request
 *							loop {
 *			lock					lock			lock
 *			if new					if new			if new
 *									break
 *								unlock
 *								sleep
 *							}
 *				func			func					func
 *			unlock				unlock				unlock
 *							respond
 *			wait period			sleep				sleep
 *		}				}				}
 *
 * Sporadic	S4. loop {			S5. loop {			S6. loop {
 * Writer						wait request
 *			send writer request		send writer request		send writer_request
 *			wait writer response		wait writer response		wait_writer_response
 *			lock				lock				lock
 *			func				func				func
 *			unlock				unlock				unlock
 *							respond
 *			wait period			sleep				sleep
 *		}				}				}
 *
 * Multi Buffer:
 *
 *		Periodic Reader			Sporadic Reader			Soft Reader
 *
 * Periodic	M1. loop {			M2. loop {			M3. loop {
 * Writer						wait request
 *			if last is new			if last is new			if last is new
 *				b = last			b = last			b = last
 *			else				else				else
 *				b = cur				b = cur				b = cur
 *			lock(b)				lock(b)				lock(b)
 *			func				func				func
 *			unlock(b)			unlock(b)			unlock(b)
 *							respond
 *			wait period			sleep				sleep
 *		}				}				}
 *
 * Sporadic	M4. loop {			M5. loop {			M6. loop {
 * Writer						wait request
 *			send writer request		send writer request		send writer_request
 *			wait writer response		wait writer response		wait_writer_response
 *			lock(cur)			lock(cur)			lock(cur)
 *			func				func				func
 *			unlock(cur)			unlock(cur)			unlock(cur)
 *							respond
 *			wait period			sleep				sleep
 *		}				}				}
 *
 * Note: the combination of sporadic writer and soft reader could be deadly!  The combination of single buffer
 * and soft reader would not be very wise either.
 *
 * In the function, the specific code that belongs to either of these 12 cases is marked as such.
 */
void skin_reader_acquisition_task(urt_task *task, void *data)
{
	struct skin_reader *reader = data;
	struct skin_writer_info *writer_info;
	urt_time swap_protection_time = 0;
	uint8_t last_buffer = 0;
	urt_time last_timestamp = 0;
	bool multi_buffer;
	bool sporadic;
	bool soft;
	bool writer_periodic;

	if (_sanity_check_reader(reader, false, false))
		goto exit_bad_argument;
	writer_info = &reader->skin->kernel->writers[reader->writer_index];

	reader->stats.start_time = urt_get_time();
	multi_buffer = writer_info->attr.buffer_count > 1;
	writer_periodic = writer_info->period > 0;
	soft = reader->soft;
	sporadic = reader->period <= 0 && !soft;

	/* check more if sporadic */
	if (_sanity_check_reader(reader, sporadic, !writer_periodic))
		goto exit_bad_argument;

	urt_dbg(reader->skin->log_file, "reader corresponding to writer %u started (period: %lld, soft: %s) (name: %s)\n",
			reader->writer_index, reader->period, reader->soft?"Yes":"No",
			reader->skin->kernel->writers[reader->writer_index].attr.prefix);

	while (!reader->must_stop)
	{
		urt_time exec_time = urt_get_exec_time(), passed_time;
		uint8_t current_buffer = 0;
		bool must_pause;
		bool locked;

		/* if paused or writer is paused, sleep and retry */
		must_pause = reader->must_pause || writer_info->paused || !writer_info->active;
		reader->paused = must_pause;
		if (must_pause)
			goto skip_read;

		/* cases S2, S5, M2 and M5: wait for request for sporadic reads */
		if (sporadic)
			if (urt_sem_wait(reader->request, &reader->must_stop))
				goto skip_read;

		/* cases S4-6 and M4-6: send request and await response for sporadic writers */
		if (!writer_periodic)
		{
			if (urt_sem_post(reader->writer_request))
				goto skip_read_respond_users;
			if (urt_sem_wait(reader->writer_response, &reader->must_stop))
				goto skip_read_respond_users;
		}

		/* this loop is for retry of synchronization with multi-buffer writers */
		while (1)
		{
			/*
			 * try_lock indicates whether current_buffer is expected to be unlocked and so the lock is only tried.
			 * If the lock couldn't get acquired, it means that a buffer swap has just happened and current_buffer
			 * needs to be recalculated.  In case of periodic writers, if last buffer is not new, then we should
			 * wait on the buffer being written and therefore in that case try_lock is false
			 */
			bool try_lock = multi_buffer;

			/* cases M1-M3: see which buffer to lock; the one already written if it has new data or the one being written */
			if (multi_buffer && writer_periodic)
			{
				urt_time timestamp;

				current_buffer = writer_info->last_written_buffer;
				timestamp = writer_info->write_times[current_buffer];
				if (!_buffer_data_is_new(current_buffer, timestamp, last_buffer, last_timestamp,
							reader, writer_info, true, swap_protection_time))
				{
					current_buffer = writer_info->buffer_being_written;
					try_lock = false;
				}
			}
			/* cases M4-M6: last written buffer should be available */
			else if (multi_buffer)
				current_buffer = writer_info->last_written_buffer;

			/* case S2: wait until the periodic writer has new data, because sporadic reader shouldn't return with no results */
			if (writer_periodic && sporadic && !multi_buffer)
			{
				while (true)
				{
					if (urt_rwlock_read_lock(reader->rwls[0], &reader->must_stop))
						goto skip_read_respond_users;
					if (_buffer_data_is_new(0, writer_info->write_times[0], 0, last_timestamp, reader, writer_info, false, 0))
						break;
					urt_rwlock_read_unlock(reader->rwls[0]);
					urt_sleep(SKIN_CONFIG_EVENT_MAX_DELAY);
				}
			}
			/* cases S1, S3-6: wait until the buffer becomes available */
			/* cases M1-3: if last buffer is not new, wait until the buffer being written becomes available */
			else if (!try_lock)
			{
				if (urt_rwlock_read_lock(reader->rwls[current_buffer], &reader->must_stop))
					goto skip_read_respond_users;
			}
			/* cases M1-6: try lock the buffer.  If it fails, a buffer swap has happened in the meantime, so try again */
			else
				if (urt_rwlock_try_read_lock(reader->rwls[current_buffer]))
					continue;

			/* except the sole case where a buffer swap has happened during calculation, the lock should be acquired now */
			break;
		}

		/* cases S1 and S3: if there is nothing new to read, skip it.  Other S* cases should always result in a new buffer */
		if (!multi_buffer && !_buffer_data_is_new(0, writer_info->write_times[0], 0, last_timestamp, reader, writer_info, false, 0))
		{
			urt_rwlock_read_unlock(reader->rwls[0]);
			goto skip_read_respond_users;
		}

		/* if unlocked because the writer is removed, the buffer is unlocked, but there would be no new data */
		if (!writer_info->active)
		{
			urt_rwlock_read_unlock(reader->rwls[current_buffer]);
			goto skip_read_respond_users;
		}

		passed_time = urt_get_time();

		/* call the reader callback with the current buffer */
		last_timestamp = writer_info->write_times[current_buffer];
		last_buffer = current_buffer;
		reader->callbacks.read(reader,
				(char *)reader->mem + current_buffer * writer_info->attr.buffer_size,
				writer_info->attr.buffer_size,
				reader->callbacks.user_data);

		/* unlock the buffer */
		urt_rwlock_read_unlock(reader->rwls[current_buffer]);

		/* update swap skip protection time to converge to passed time with a factor of 1/8 */
		passed_time = urt_get_time() - passed_time;
		swap_protection_time = (swap_protection_time * 7 + passed_time) / 8;

		/* statistics */
		locked = urt_mutex_lock(reader->stats_lock, &reader->must_stop) == 0;

		++reader->stats.read_count;
		exec_time = urt_get_exec_time() - exec_time;
		/* some subsystems such as RTAI lack enough precision for execution time */
		/* Note: RTAI at https://github.com/ShabbyX/RTAI/commits/master has fixed this error */
		if (exec_time == 0)
			exec_time = passed_time;
		if (exec_time > reader->stats.worst_read_time)
			reader->stats.worst_read_time = exec_time;
		if (exec_time < reader->stats.best_read_time || reader->stats.best_read_time == 0)
			reader->stats.best_read_time = exec_time;
		reader->stats.accumulated_read_time += exec_time;

		if (locked)
			urt_mutex_unlock(reader->stats_lock);

skip_read_respond_users:
		/* cases S2, S5, M2 and M5: if sporadic, signal your requesters that read has been done */
		if (sporadic)
			skin_internal_signal_all_requests(reader->request, reader->response);
skip_read:
		/* cases S1, S4, M1 and M4: if periodic, wait your period */
		if (!sporadic && !soft)
			urt_task_wait_period(task);
		/*
		 * cases S3, S6, M3 and M6: if soft, sleep a little to avoid busy waiting (if single buffer),
		 * crazily invoking writer (if sporadic writer) or a race condition where a reader requests
		 * so fast while the writer is responding, and the same reader wakes up a second time instead
		 * of another reader who had been waiting before.
		 *
		 * While this doesn't theoretically solve the race condition, it is ok because first of all,
		 * with the added sleep, that race condition would happen if the system is very heavily loaded.
		 * It is likely that the older reader would wake up after another round of writer (because the
		 * reader who stole the response has already sent a request.  Since the reader is soft, there
		 * is no need to guarantee a deadline, so it's ok if in that case (heavily loaded system),
		 * some soft readers get mistreated by other soft readers.
		 *
		 * That is true only if the priorities of the soft readers are all smaller than the priorities
		 * of the other readers (otherwise the soft reader could steal a response from a hard reader).
		 * This is a valid assumption since in a system, soft real-time threads should have smaller
		 * priority with respect to hard real-time threads.
		 *
		 * cases S2, S5, M2, M5: if sporadic, sleep is unnecessary.  However, in the unlikely event
		 * that a lock failed to acquire, this sleep would prevent retrying immediately, effectively
		 * preventing a busy loop.
		 */
		else /* soft || sporadic */
			urt_sleep(SKIN_CONFIG_EVENT_MAX_DELAY);
	}

	reader->running = false;
	urt_dbg(reader->skin->log_file, "reader corresponding to writer %u stopped\n", reader->writer_index);

	return;
exit_bad_argument:
	internal_error("reader's data is invalid\n");
	reader->running = false;
}

int skin_reader_pause(struct skin_reader *reader)
{
	if (_sanity_check_reader(reader, false, false))
		return EINVAL;
	reader->must_pause = true;
	return 0;
}
URT_EXPORT_SYMBOL(skin_reader_pause);

int skin_reader_resume(struct skin_reader *reader)
{
	if (_sanity_check_reader(reader, false, false))
		return EINVAL;
	reader->must_pause = false;
	return 0;
}
URT_EXPORT_SYMBOL(skin_reader_resume);

bool skin_reader_is_paused(struct skin_reader *reader)
{
	if (_sanity_check_reader(reader, false, false))
		return false;
	return reader->paused;
}
URT_EXPORT_SYMBOL(skin_reader_is_paused);

bool skin_reader_is_active(struct skin_reader *reader)
{
	if (_sanity_check_reader(reader, false, false))
		return false;

	return skin_internal_writer_is_active(reader->skin, reader->writer_index);
}
URT_EXPORT_SYMBOL(skin_reader_is_active);

static inline void _request_nonblocking(struct skin_reader *reader)
{
	urt_sem_post(reader->request);
}

static inline int _await_response(struct skin_reader *reader, volatile sig_atomic_t *stop)
{
	return urt_sem_wait(reader->response, stop);
}

int (skin_reader_request)(struct skin_reader *reader, volatile sig_atomic_t *stop, ...)
{
	if (_sanity_check_reader(reader, true, false))
		return EINVAL;
	_request_nonblocking(reader);
	return _await_response(reader, stop);
}
URT_EXPORT_SYMBOL(skin_reader_request);

int skin_reader_request_nonblocking(struct skin_reader *reader)
{
	if (_sanity_check_reader(reader, true, false))
		return EINVAL;
	_request_nonblocking(reader);
	return 0;
}
URT_EXPORT_SYMBOL(skin_reader_request_nonblocking);

int (skin_reader_await_response)(struct skin_reader *reader, volatile sig_atomic_t *stop, ...)
{
	if (_sanity_check_reader(reader, true, false))
		return EINVAL;
	return _await_response(reader, stop);
}
URT_EXPORT_SYMBOL(skin_reader_await_response);

struct skin_user *skin_reader_get_user(struct skin_reader *reader)
{
	if (_sanity_check_reader(reader, false, false))
		return NULL;
	return reader->user;
}
URT_EXPORT_SYMBOL(skin_reader_get_user);

int skin_reader_get_attr(struct skin_reader *reader, struct skin_reader_attr *attr)
{
	int err = 0;

	if (_sanity_check_reader(reader, false, false) || attr == NULL)
		return EINVAL;

	if ((err = skin_internal_global_read_lock(&reader->skin->kernel_locks)))
		return err;

	/*
	 * Note: giving away this point is fine, because the writer is supposed to remain there until
	 * all readers are detached, so this pointer will remain live until at least after this reader
	 * is destroyed.  This behavior is documented.
	 */
	attr->name = reader->skin->kernel->writers[reader->writer_index].attr.prefix;

	skin_internal_global_read_unlock(&reader->skin->kernel_locks);

	return 0;
}
URT_EXPORT_SYMBOL(skin_reader_get_attr);

int skin_reader_get_statistics(struct skin_reader *reader, struct skin_reader_statistics *stats)
{
	bool locked = false;

	if (_sanity_check_reader(reader, false, false) || stats == NULL)
		return EINVAL;

	/*
	 * if calling from real-time context, locking is possible.  Otherwise,
	 * even if task is running, synchronization is impossible.  However,
	 * if not in real-time context, it is likely that this function is being
	 * called from cleanup hook.
	 */
	if (urt_is_rt_context())
		locked = urt_mutex_lock(reader->stats_lock, &reader->must_stop) == 0;

	*stats = reader->stats;

	if (locked)
		urt_mutex_unlock(reader->stats_lock);

	return 0;
}
URT_EXPORT_SYMBOL(skin_reader_get_statistics);
