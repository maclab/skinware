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
#include "names.h"

static int _sanity_check_skin(struct skin *skin)
{
	return skin == NULL || skin->kernel == NULL?-1:0;
}

static struct skin_writer *_revive_writer(struct skin *skin, const struct skin_writer_attr *attr, const urt_task_attr *task_attr, int *error)
{
	unsigned int i;
	struct skin_kernel *sk = skin->kernel;
	struct skin_writer *writer = NULL;

	for (i = 0; i < sk->max_writer_count; ++i)
	{
		struct skin_writer_info *w = &sk->writers[i];

		if (!w->active && w->readers_attached > 0
			&& skin_internal_name_cmp_prefix(w->attr.prefix, attr->name) == 0)
		{
			*error = EEXIST;

			/* make sure the new and old attributes match */
			if (w->attr.buffer_size != attr->buffer_size || w->attr.buffer_count != attr->buffer_count
					|| w->period != task_attr->period)
				goto exit_fail;

			*error = EALREADY;

			writer = urt_mem_new(sizeof *writer, error);
			if (writer == NULL)
				goto exit_fail;

			w->active = true;

			*writer = (struct skin_writer){
				.info_index = i,
			};

			break;
		}
	}

exit_fail:
	return writer;
}

static struct skin_writer *_get_free_writer(struct skin *skin, const struct skin_writer_attr *attr, const urt_task_attr *task_attr, int *error)
{
	unsigned int i;
	struct skin_kernel *sk = skin->kernel;
	struct skin_writer *writer = NULL;

	for (i = 0; i < sk->max_writer_count; ++i)
	{
		struct skin_writer_info *w = &sk->writers[i];

		if (!w->active && w->readers_attached == 0)
		{
			writer = urt_mem_new(sizeof *writer, error);
			if (writer == NULL)
				goto exit_fail;

			*w = (struct skin_writer_info){
				.attr = {
					.buffer_size = attr->buffer_size,
					.buffer_count = attr->buffer_count,
				},
				.period = task_attr->period,
				.driver_index = sk->max_driver_count,
				.active = true,
			};
			skin_internal_name_set_prefix(w->attr.prefix, attr->name, NULL);
			*writer = (struct skin_writer){
				.info_index = i,
			};

			break;
		}
	}

	if (writer == NULL)
		*error = ENOSPC;
exit_fail:
	return writer;
}

SKIN_DEFINE_STORE_FUNCTION(writer);

struct skin_writer *(skin_service_add)(struct skin *skin, const struct skin_writer_attr *attr_,
		const urt_task_attr *task_attr_, const struct skin_writer_callbacks *callbacks, int *error, ...)
{
	struct skin_writer *writer = NULL;
	int err = 0;
	char name[URT_NAME_LEN + 1];
	uint8_t b;
	bool revived = false;
	struct skin_writer_attr attr;
	struct urt_task_attr task_attr;

	if (_sanity_check_skin(skin) || attr_ == NULL || task_attr_ == NULL || callbacks == NULL
			|| callbacks->write == NULL || attr_->name == NULL || attr_->name[0] == '\0')
		goto exit_bad_param;

	attr = *attr_;
	task_attr = *task_attr_;

	/* do sanity checks */
	if (attr.buffer_size == 0)
		goto exit_bad_param;
	if (attr.buffer_count == 0 || attr.buffer_count > SKIN_CONFIG_MAX_BUFFERS)
		attr.buffer_count = SKIN_CONFIG_MAX_BUFFERS;
	if (task_attr.period < 0)
		task_attr.period = 0;

	/* default values */
	if (!urt_priority_is_valid(task_attr.priority))
		task_attr.priority = urt_priority(SKIN_CONFIG_PRIORITY_WRITER);

	if ((err = skin_internal_global_write_lock(&skin->kernel_locks)))
		goto exit_no_global_lock;

	/* try to see if the prefix already exists and if so, revive it */
	writer = _revive_writer(skin, &attr, &task_attr, &err);
	revived = writer != NULL && err == EALREADY;

	/* otherwise, if there were no writers with that prefix, see if there are any free writers */
	if (writer == NULL && err != EEXIST)
		writer = _get_free_writer(skin, &attr, &task_attr, &err);

	skin_internal_global_write_unlock(&skin->kernel_locks);

	/* if neither revived nor any free space available, fail */
	if (writer == NULL)
		goto exit_no_writer;

	/* update err in case revived, because it's no an error, but we are still reporting it through this variable */
	if (revived && error)
		*error = err;

	/* create locks and shared memory */
	*writer = (struct skin_writer){
		.running = true,
		.must_pause = true,
		.paused = true,
		.callbacks = *callbacks,
		.skin = skin,
		.info_index = writer->info_index,
	};
	if (task_attr.period == 0)
	{
		skin_internal_name_set(name, attr.name, "REQ");
		if (revived)
			writer->request = urt_shsem_attach(name, &err);
		else
			writer->request = urt_shsem_new(name, 0, &err);
		skin_internal_name_set(name, attr.name, "RES");
		if (revived)
			writer->response = urt_shsem_attach(name, &err);
		else
			writer->response = urt_shsem_new(name, 0, &err);
		if (writer->request == NULL || writer->response == NULL)
			goto exit_no_lock;
	}
	for (b = 0; b < attr.buffer_count; ++b)
	{
		skin_internal_name_set_indexed(name, attr.name, "RW", b);
		if (revived)
			writer->rwls[b] = urt_shrwlock_attach(name, &err);
		else
			writer->rwls[b] = urt_shrwlock_new(name, &err);
		if (writer->rwls[b] == NULL)
			goto exit_no_lock;
	}
	skin_internal_name_set(name, attr.name, "MEM");
	if (revived)
		writer->mem = urt_shmem_attach(name, &err);
	else
		writer->mem = urt_shmem_new(name, attr.buffer_count * attr.buffer_size, &err);
	if (writer->mem == NULL)
		goto exit_no_mem;

	/* create lock for stats, but failure doesn't matter */
	writer->stats_lock = urt_mutex_new();

	/* create the task itself */
	writer->task = urt_task_new(skin_writer_acquisition_task, writer, &task_attr, &err);
	if (writer->task == NULL)
		goto exit_no_task;
	if ((err = urt_task_start(writer->task)))
		goto exit_no_task;

	/* store the pointer in internal memory */
	_store_writer(skin, writer);

	/* call the object-specific init hook */
	if (callbacks->init)
		callbacks->init(writer, callbacks->user_data);

	/* call the generic init hook */
	if (skin->writer_init_hook)
		skin->writer_init_hook(writer, skin->writer_init_user_data);

	return writer;
exit_no_task:
exit_no_mem:
exit_no_lock:
	skin_service_remove(writer);
exit_no_writer:
exit_no_global_lock:
exit_fail:
	/* if writer is NULL, call the clean hook anyway, in case `user_data` requires cleanup */
	if (writer == NULL && callbacks && callbacks->clean)
		callbacks->clean(writer, callbacks->user_data);
	if (error)
		*error = err;
	return NULL;
exit_bad_param:
	err = EINVAL;
	goto exit_fail;
}
URT_EXPORT_SYMBOL(skin_service_add);

void skin_service_remove(struct skin_writer *writer)
{
	bool locked;
	uint8_t b;

	if (writer == NULL || _sanity_check_skin(writer->skin)
			|| writer->info_index >= writer->skin->kernel->max_writer_count)
		return;

	/* remove it from local book-keeping, making sure it had been added for book-keeping */
	if (writer->index < writer->skin->writers_mem_size)
		if (writer == writer->skin->writers[writer->index])
			writer->skin->writers[writer->index] = NULL;

	locked = skin_internal_global_write_lock(&writer->skin->kernel_locks) == 0;

	/* mark it as inactive */
	writer->skin->kernel->writers[writer->info_index].active = false;

	if (locked)
		skin_internal_global_write_unlock(&writer->skin->kernel_locks);

	/* stop the task */
	writer->must_stop = 1;
	skin_internal_wait_termination(&writer->running);
	urt_task_delete(writer->task);

	/* detach from locks and memory */
	urt_shsem_detach(writer->request);
	urt_shsem_detach(writer->response);
	for (b = 0; b < SKIN_CONFIG_MAX_BUFFERS; ++b)
		urt_shrwlock_detach(writer->rwls[b]);
	urt_shmem_detach(writer->mem);
	writer->request = NULL;
	writer->response = NULL;

	/* call the generic clean hook */
	if (writer->skin->writer_clean_hook)
		writer->skin->writer_clean_hook(writer, writer->skin->writer_clean_user_data);

	/* call the object-specific clean hook */
	if (writer->callbacks.clean)
		writer->callbacks.clean(writer, writer->callbacks.user_data);

	/* final cleanup */
	urt_mutex_delete(writer->stats_lock);
	urt_mem_delete(writer);
}
URT_EXPORT_SYMBOL(skin_service_remove);
