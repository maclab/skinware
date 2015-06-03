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
#include <skin_writer.h>
#include "internal.h"

static int _sanity_check_writer(struct skin_writer *writer, bool is_sporadic)
{
	return writer == NULL || writer->task == NULL || writer->skin == NULL || writer->skin->kernel == NULL
		|| (is_sporadic && (writer->request == NULL || writer->response == NULL))?-1:0;
}

static bool _swap_buffers(struct skin_writer *writer, struct skin_writer_info *info, uint8_t *cur_buf, urt_time wait_time, bool before_write)
{
	uint8_t i;
	uint8_t buf_count = info->attr.buffer_count;

	/*
	 * divide wait time equally among other buffers, so in total there would be `wait_time` waiting time!
	 * This function should be called only in multi_buffer, so (buffer_count - 1) is definitely non-zero
	 */
#if !defined(__KERNEL__) || BITS_PER_LONG == 64
	wait_time /= (info->attr.buffer_count - 1);
#else
	/* in kernel space, for 32 bit architectures, 64 bit division is done with do_div */
	do_div(wait_time, info->attr.buffer_count - 1);
#endif

	for (i = 1; i < info->attr.buffer_count; ++i)
	{
		/* go to i buffers ahead */
		uint8_t next_buf = (*cur_buf + i) % buf_count;
		/* see if you can lock it */
		int ret = wait_time == 0?
			urt_rwlock_try_write_lock(writer->rwls[next_buf]):
			urt_rwlock_timed_write_lock(writer->rwls[next_buf], wait_time);
		if (ret == 0)
		{
			/*
			 * if swapped just before next write, then next predicted swap is just after the write finishes,
			 * otherwise, next predicted swap is about same time next period
			 */
			urt_time next_swap = urt_get_time() + (before_write?writer->stats.best_write_time:info->period);

			info->buffer_being_written = next_buf;
			info->next_predicted_swap = next_swap;
			info->last_written_buffer = *cur_buf;
			urt_rwlock_write_unlock(writer->rwls[*cur_buf]);

			*cur_buf = next_buf;
			return true;
		}
	}
	return false;
}

/*
 * the synchronization mechanism in the writer is as follows:
 *
 *		Periodic			Sporadic
 *
 * Single	1. loop {			2. loop {
 * Buffer						wait request
 *			lock				lock
 *			func				func
 *			unlock				unlock
 *							respond
 *			wait period
 *		}				}
 *
 * Multi	3. lock(cur)			4. lock(cur)
 * Buffer	loop {				loop {
 *			if last period no swap
 *				try swap again
 *							wait request
 *			func				func
 *			try while period left		try until
 *				lock(next)			lock(next)
 *				unlock(cur)			unlock(cur)
 *				cur = next			cur = next
 *							respond
 *			wait period
 *		}				}
 *		unlock(cur)			unlock(cur)
 *
 * In the function, the specific code that belongs to either of these four cases is marked as such.
 */
void skin_writer_acquisition_task(urt_task *task, void *data)
{
	struct skin_writer *writer = data;
	struct skin_writer_info *writer_info;
	uint8_t current_buffer;
	urt_time timestamp = 0;
	bool multi_buffer;
	bool periodic;
	bool swap_done = true;

	if (_sanity_check_writer(writer, false))
		goto exit_bad_argument;
	writer_info = &writer->skin->kernel->writers[writer->info_index];

	current_buffer = writer_info->buffer_being_written;
	writer->stats.start_time = urt_get_time();
	multi_buffer = writer_info->attr.buffer_count > 1;
	periodic = writer_info->period > 0;

	/* check more if sporadic */
	if (_sanity_check_writer(writer, !periodic))
		goto exit_bad_argument;

	/* cases 3 and 4: initial lock for multi-buffers */
	if (multi_buffer)
		urt_rwlock_write_lock(writer->rwls[current_buffer], &writer->must_stop);

	urt_dbg(writer->skin->log_file, "writer %u started (period: %lld) (name: %s)\n", writer->info_index, writer_info->period,
			writer_info->attr.prefix);

	while (!writer->must_stop)
	{
		urt_time exec_time = urt_get_exec_time(), passed_time = urt_get_time();
		bool must_pause;
		bool locked;
		bool swap_skipped = false;

		/* cases 3 and 4: if multi-buffer, try swapping buffers if not yet done */
		if (multi_buffer)
			if (!swap_done)
			{
				swap_done = _swap_buffers(writer, writer_info, &current_buffer, 0, true);
				if (!swap_done)
					swap_skipped = true;
			}

		/* if paused, sleep and retry */
		must_pause = writer->must_pause;
		writer_info->paused = must_pause;
		writer->paused = must_pause;
		if (must_pause)
			goto skip_write;

		/* cases 2 and 4: wait for request for sporadic writes */
		if (!periodic)
			if (urt_sem_wait(writer->request, &writer->must_stop))
				goto skip_write;

		/* cases 1 and 2: if single-buffer, lock the buffer */
		if (!multi_buffer)
			if (urt_rwlock_write_lock(writer->rwls[0], &writer->must_stop))
				goto skip_write;

		/* call the writer callback with the current buffer */
		timestamp = urt_get_time();
		writer_info->bad = writer->callbacks.write(writer,
				(char *)writer->mem + current_buffer * writer_info->attr.buffer_size,
				writer_info->attr.buffer_size,
				writer->callbacks.user_data);
		writer_info->write_times[current_buffer] = timestamp;

		if (multi_buffer)
		{
			swap_done = false;
			while (!swap_done && !writer->must_stop)
			{
				/* case 3: if periodic, then try swapping buffers only until period expires */
				/* case 4: if sporadic, try this forever */
				if (periodic && urt_get_time() < 200000)
					/* if not enough time left, wait period and try swapping in new period */
					break;

				swap_done = _swap_buffers(writer, writer_info, &current_buffer, 200000, false);
			}
		}
		else
			/* cases 1 and 2: unlock the buffer */
			urt_rwlock_write_unlock(writer->rwls[0]);

		/* statistics */
		locked = urt_mutex_lock(writer->stats_lock, &writer->must_stop) == 0;

		++writer->stats.write_count;
		exec_time = urt_get_exec_time() - exec_time;
		/* some subsystems such as RTAI lack enough precision for execution time */
		/* Note: RTAI at https://github.com/ShabbyX/RTAI has fixed this error */
		if (exec_time == 0)
			exec_time = urt_get_time() - passed_time;
		if (exec_time > writer->stats.worst_write_time)
			writer->stats.worst_write_time = exec_time;
		if (exec_time < writer->stats.best_write_time || writer->stats.best_write_time == 0)
			writer->stats.best_write_time = exec_time;
		writer->stats.accumulated_write_time += exec_time;
		if (swap_skipped)
			++writer->stats.swap_skips;

		if (locked)
			urt_mutex_unlock(writer->stats_lock);

		/* cases 2 and 4: if sporadic, signal your requesters that write has been done */
		if (!periodic)
			skin_internal_signal_all_requests(writer->request, writer->response);
skip_write:
		/* cases 1 and 3: if periodic, wait your period */
		if (periodic)
			urt_task_wait_period(task);
		else
			urt_sleep(SKIN_CONFIG_EVENT_MAX_DELAY);
	}

	/* cases 3 and 4: if multi-buffer, unlock the buffer currently being held */
	if (multi_buffer)
		urt_rwlock_write_unlock(writer->rwls[current_buffer]);

	writer->running = false;
	urt_dbg(writer->skin->log_file, "writer %u stopped\n", writer->info_index);

	return;
exit_bad_argument:
	internal_error("writer's data is invalid\n");
	writer->running = false;
}

int skin_writer_pause(struct skin_writer *writer)
{
	if (_sanity_check_writer(writer, false))
		return EINVAL;
	writer->must_pause = true;
	return 0;
}
URT_EXPORT_SYMBOL(skin_writer_pause);

int skin_writer_resume(struct skin_writer *writer)
{
	if (_sanity_check_writer(writer, false))
		return EINVAL;
	writer->must_pause = false;
	return 0;
}
URT_EXPORT_SYMBOL(skin_writer_resume);

bool skin_writer_is_paused(struct skin_writer *writer)
{
	if (_sanity_check_writer(writer, false))
		return false;
	return writer->paused;
}
URT_EXPORT_SYMBOL(skin_writer_is_paused);

bool skin_writer_is_active(struct skin_writer *writer)
{
	if (_sanity_check_writer(writer, false))
		return false;

	return skin_internal_writer_is_active(writer->skin, writer->info_index);
}
URT_EXPORT_SYMBOL(skin_writer_is_active);

static inline void _request_nonblocking(struct skin_writer *writer)
{
	urt_sem_post(writer->request);
}

static inline int _await_response(struct skin_writer *writer, volatile sig_atomic_t *stop)
{
	return urt_sem_wait(writer->response, stop);
}

int (skin_writer_request)(struct skin_writer *writer, volatile sig_atomic_t *stop, ...)
{
	if (_sanity_check_writer(writer, true))
		return EINVAL;
	_request_nonblocking(writer);
	return _await_response(writer, stop);
}
URT_EXPORT_SYMBOL(skin_writer_request);

int skin_writer_request_nonblocking(struct skin_writer *writer)
{
	if (_sanity_check_writer(writer, true))
		return EINVAL;
	_request_nonblocking(writer);
	return 0;
}
URT_EXPORT_SYMBOL(skin_writer_request_nonblocking);

int (skin_writer_await_response)(struct skin_writer *writer, volatile sig_atomic_t *stop, ...)
{
	if (_sanity_check_writer(writer, true))
		return EINVAL;
	return _await_response(writer, stop);
}
URT_EXPORT_SYMBOL(skin_writer_await_response);

struct skin_driver *skin_writer_get_driver(struct skin_writer *writer)
{
	if (_sanity_check_writer(writer, false))
		return NULL;
	return writer->driver;
}
URT_EXPORT_SYMBOL(skin_writer_get_driver);

int skin_writer_get_attr(struct skin_writer *writer, struct skin_writer_attr *attr)
{
	int err = 0;
	struct skin_writer_attr_internal *attr_internal;

	if (_sanity_check_writer(writer, false) || attr == NULL)
		return EINVAL;

	if ((err = skin_internal_global_read_lock(&writer->skin->kernel_locks)))
		return err;

	attr_internal = &writer->skin->kernel->writers[writer->info_index].attr;
	*attr = (struct skin_writer_attr){
		.buffer_size = attr_internal->buffer_size,
		.buffer_count = attr_internal->buffer_count,
		.name = attr_internal->prefix,
	};

	skin_internal_global_read_unlock(&writer->skin->kernel_locks);

	return 0;
}
URT_EXPORT_SYMBOL(skin_writer_get_attr);

int skin_writer_get_statistics(struct skin_writer *writer, struct skin_writer_statistics *stats)
{
	bool locked = false;

	if (_sanity_check_writer(writer, false) || stats == NULL)
		return EINVAL;

	/*
	 * if calling from real-time context, locking is possible.  Otherwise,
	 * even if task is running, synchronization is impossible.  However,
	 * if not in real-time context, it is likely that this function is being
	 * called from cleanup hook.
	 */
	if (urt_is_rt_context())
		locked = urt_mutex_lock(writer->stats_lock, &writer->must_stop) == 0;

	*stats = writer->stats;

	if (locked)
		urt_mutex_unlock(writer->stats_lock);

	return 0;
}
URT_EXPORT_SYMBOL(skin_writer_get_statistics);

void skin_writer_copy_last_buffer(struct skin_writer *writer)
{
	void *cur, *last;
	struct skin_writer_info *writer_info;

	if (_sanity_check_writer(writer, false) || writer->mem == NULL)
		return;

	writer_info = &writer->skin->kernel->writers[writer->info_index];
	last = (char *)writer->mem + writer_info->last_written_buffer * writer_info->attr.buffer_size;
	cur = (char *)writer->mem + writer_info->buffer_being_written * writer_info->attr.buffer_size;

	memmove(cur, last, writer_info->attr.buffer_size);
}
URT_EXPORT_SYMBOL(skin_writer_copy_last_buffer);
