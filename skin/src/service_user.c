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
#include <skin_base.h>
#include "internal.h"
#include "reader_internal.h"
#include "names.h"

static int _sanity_check_skin(struct skin *skin)
{
	return skin == NULL || skin->kernel == NULL?-1:0;
}

static struct skin_reader *_attach_reader(struct skin *skin, const char *prefix, int *error)
{
	unsigned int i;
	struct skin_kernel *sk = skin->kernel;
	struct skin_reader *reader = NULL;

	for (i = 0; i < sk->max_writer_count; ++i)
	{
		struct skin_writer_info *w = &sk->writers[i];

		if (w->active && skin_internal_name_cmp_prefix(prefix, w->attr.prefix) == 0)
		{
			reader = urt_mem_new(sizeof *reader, error);
			if (reader == NULL)
				goto exit_fail;

			++w->readers_attached;

			*reader = (struct skin_reader){
				.writer_index = i,
			};

			break;
		}
	}

	if (reader == NULL)
		*error = ENOENT;
exit_fail:
	return reader;
}

SKIN_DEFINE_STORE_FUNCTION(reader);

struct skin_reader *(skin_service_attach)(struct skin *skin, const struct skin_reader_attr *attr_,
		const urt_task_attr *task_attr_, const struct skin_reader_callbacks *callbacks, int *error, ...)
{
	struct skin_reader *reader = NULL;
	int err = 0;
	char name[URT_NAME_LEN + 1];
	uint8_t b;
	struct skin_writer_info *writer_info;
	struct skin_reader_attr attr;
	struct urt_task_attr task_attr;

	if (_sanity_check_skin(skin) || attr_ == NULL || task_attr_ == NULL || callbacks == NULL
			|| callbacks->read == NULL || attr_->name == NULL || attr_->name[0] == '\0')
		goto exit_bad_param;

	attr = *attr_;
	task_attr = *task_attr_;

	/* do sanity checks */
	if (task_attr.period < 0)
		task_attr.period = 0;
	if (task_attr.soft)
		task_attr.period = 0;

	/* default values */
	if (!urt_priority_is_valid(task_attr.priority))
		task_attr.priority = urt_priority(SKIN_CONFIG_PRIORITY_READER);

	if ((err = skin_internal_global_write_lock(&skin->kernel_locks)))
		goto exit_no_global_lock;

	/* see if there are any writers with this prefix at all */
	reader = _attach_reader(skin, attr.name, &err);

	skin_internal_global_write_unlock(&skin->kernel_locks);

	if (reader == NULL)
		goto exit_no_reader;

	/* attach to its locks and shared memory */
	*reader = (struct skin_reader){
		.running = true,
		.must_pause = true,
		.paused = true,
		.callbacks = *callbacks,
		.skin = skin,
		.soft = task_attr.soft,
		.period = task_attr.period,
		.writer_index = reader->writer_index,
	};
	writer_info = &skin->kernel->writers[reader->writer_index];
	if (task_attr.period == 0 && !task_attr.soft)
	{
		reader->request = urt_sem_new(0, &err);
		reader->response = urt_sem_new(0, &err);
		if (reader->request == NULL || reader->response == NULL)
			goto exit_no_lock;
	}
	if (writer_info->period == 0)
	{
		skin_internal_name_set(name, attr.name, "REQ");
		reader->writer_request = urt_shsem_attach(name, &err);
		skin_internal_name_set(name, attr.name, "RES");
		reader->writer_response = urt_shsem_attach(name, &err);
		if (reader->writer_request == NULL || reader->writer_response == NULL)
			goto exit_no_lock;
	}
	for (b = 0; b < writer_info->attr.buffer_count; ++b)
	{
		skin_internal_name_set_indexed(name, attr.name, "RW", b);
		reader->rwls[b] = urt_shrwlock_attach(name, &err);
		if (reader->rwls[b] == NULL)
			goto exit_no_lock;
	}
	skin_internal_name_set(name, attr.name, "MEM");
	reader->mem = urt_shmem_attach(name, &err);
	if (reader->mem == NULL)
		goto exit_no_mem;

	/* cap period of periodic readers to that of writer if periodic */
	if (writer_info->period > 0 && reader->period > 0 && reader->period < writer_info->period)
	{
		reader->period = writer_info->period;
		task_attr.period = reader->period;
	}

	/* create lock for stats, but failure doesn't matter */
	reader->stats_lock = urt_mutex_new();

	/* create the task itself */
	reader->task = urt_task_new(skin_reader_acquisition_task, reader, &task_attr, &err);
	if (reader->task == NULL)
		goto exit_no_task;
	if ((err = urt_task_start(reader->task)))
		goto exit_no_task;

	/* store the pointer in internal memory */
	_store_reader(skin, reader);

	/* call the object-specific init hook */
	if (callbacks->init)
		callbacks->init(reader, callbacks->user_data);

	/* call the generic init hook */
	if (skin->reader_init_hook)
		skin->reader_init_hook(reader, skin->reader_init_user_data);

	return reader;
exit_no_task:
exit_no_mem:
exit_no_lock:
	skin_service_detach(reader);
exit_no_reader:
exit_no_global_lock:
exit_fail:
	/* if reader is NULL, call the clean hook anyway, in case `user_data` requires cleanup */
	if (reader == NULL && callbacks && callbacks->clean)
		callbacks->clean(reader, callbacks->user_data);
	if (error)
		*error = err;
	return NULL;
exit_bad_param:
	err = EINVAL;
	goto exit_fail;
}
URT_EXPORT_SYMBOL(skin_service_attach);

void skin_service_detach(struct skin_reader *reader)
{
	bool locked;
	uint8_t b;

	if (reader == NULL || _sanity_check_skin(reader->skin)
			|| reader->writer_index >= reader->skin->kernel->max_writer_count)
		return;

	/* remove it from local book-keeping, making sure it had been added for book-keeping */
	if (reader->index < reader->skin->readers_mem_size)
		if (reader == reader->skin->readers[reader->index])
			reader->skin->readers[reader->index] = NULL;

	locked = skin_internal_global_write_lock(&reader->skin->kernel_locks) == 0;

	/* reduce its user count */
	--reader->skin->kernel->writers[reader->writer_index].readers_attached;

	if (locked)
		skin_internal_global_write_unlock(&reader->skin->kernel_locks);

	/* stop the task */
	reader->must_stop = 1;
	skin_internal_wait_termination(&reader->running);
	urt_task_delete(reader->task);

	/* detach from locks and memory */
	urt_shsem_detach(reader->writer_request);
	urt_shsem_detach(reader->writer_response);
	for (b = 0; b < SKIN_CONFIG_MAX_BUFFERS; ++b)
		urt_shrwlock_detach(reader->rwls[b]);
	urt_shmem_detach(reader->mem);

	/* remove local locks */
	urt_sem_delete(reader->request);
	urt_sem_delete(reader->response);
	reader->request = NULL;
	reader->response = NULL;

	/* call the generic clean hook */
	if (reader->skin->reader_clean_hook)
		reader->skin->reader_clean_hook(reader, reader->skin->reader_clean_user_data);

	/* call the object-specific clean hook */
	if (reader->callbacks.clean)
		reader->callbacks.clean(reader, reader->callbacks.user_data);

	/* final cleanup */
	urt_mutex_delete(reader->stats_lock);
	urt_mem_delete(reader);
}
URT_EXPORT_SYMBOL(skin_service_detach);
