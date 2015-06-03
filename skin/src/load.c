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
#include <skin_sensor.h>
#include "internal.h"
#include "user_internal.h"
#include "reader_internal.h"

static int _sanity_check_skin(struct skin *skin)
{
	return skin == NULL || skin->kernel == NULL
		|| skin->writers == NULL || skin->readers == NULL
		|| skin->drivers == NULL || skin->users == NULL?-1:0;
}

int skin_load(struct skin *skin, const urt_task_attr *task_attr)
{
	size_t i, j;
	struct skin_driver_info *drivers;
	struct skin_writer_info *writers;
	int error = 0;
	unsigned int attached = 0;

	if (_sanity_check_skin(skin) || task_attr == NULL)
		return EINVAL;

	drivers = skin->kernel->drivers;
	writers = skin->kernel->writers;

	/* go over all present drivers and attach to the active ones */
	for (i = 0; i < skin->kernel->max_driver_count; ++i)
	{
		/* check to see if driver is active and if so get its writer prefix */
		bool active;
		bool already_attached = false;
		char reader_prefix[URT_NAME_LEN - 3 + 1];

		if ((error = skin_internal_driver_read_lock(&skin->kernel_locks)))
			continue;
		active = drivers[i].active;
		if (active)
		{
			strncpy(reader_prefix, writers[drivers[i].writer_index].attr.prefix, URT_NAME_LEN - 3);
			reader_prefix[URT_NAME_LEN - 3] = '\0';
		}
		skin_internal_driver_read_unlock(&skin->kernel_locks);

		if (!active)
			continue;

		/* make sure we aren't already attached to this driver */
		for (j = 0; j < skin->users_mem_size; ++j)
			if (skin->users[j] && skin->users[j]->driver_index == i)
			{
				already_attached = true;
				break;
			}

		if (already_attached)
			continue;

		/* try to attach to that prefix */
		if (skin_driver_attach(skin, &(struct skin_user_attr){0}, &(struct skin_reader_attr) { .name = reader_prefix },
					task_attr, &(struct skin_user_callbacks){0}, &error) != NULL)
			++attached;
	}

	/* if at least one has been attached, call it a success */
	return attached?0:error?error:ENOENT;
}
URT_EXPORT_SYMBOL(skin_load);

void skin_unload(struct skin *skin)
{
	size_t i;

	if (_sanity_check_skin(skin))
		return;

	/*
	 * The drivers and users should be stopped first because they remove
	 * their respecting writers/readers, which shouldn't be stolen from
	 * underneath them.
	 *
	 * However, to speed things up, first tell all tasks that they should
	 * stop, so their delay in responding would be spent in parallel.
	 *
	 * Furthermore, to make sure readers don't falsely read because the
	 * buffers get unlocked, set the writers also as inactive in advance.
	 */
	for (i = 0; i < skin->writers_mem_size; ++i)
		if (skin->writers[i])
		{
			skin->kernel->writers[skin->writers[i]->info_index].active = false;
			skin->writers[i]->must_stop = 1;
		}
	for (i = 0; i < skin->readers_mem_size; ++i)
		if (skin->readers[i])
			skin->readers[i]->must_stop = 1;

	for (i = 0; i < skin->drivers_mem_size; ++i)
		if (skin->drivers[i])
			skin_driver_remove(skin->drivers[i]);
	for (i = 0; i < skin->users_mem_size; ++i)
		if (skin->users[i])
			skin_driver_detach(skin->users[i]);
	for (i = 0; i < skin->writers_mem_size; ++i)
		if (skin->writers[i])
			skin_service_remove(skin->writers[i]);
	for (i = 0; i < skin->readers_mem_size; ++i)
		if (skin->readers[i])
			skin_service_detach(skin->readers[i]);
}
URT_EXPORT_SYMBOL(skin_unload);

int skin_update(struct skin *skin, const urt_task_attr *task_attr_)
{
	size_t i;
	struct skin_driver_info *drivers;
	int error = 0;
	unsigned int updated = 0;
	urt_task_attr task_attr;

	if (_sanity_check_skin(skin) || task_attr_ == NULL)
		return EINVAL;

	task_attr = *task_attr_;

	/* do sanity checks */
	if (task_attr.period < 0)
		task_attr.period = 0;
	if (task_attr.soft)
		task_attr.period = 0;

	drivers = skin->kernel->drivers;

	/* go over the users and detach from the ones with inactive drivers, or the ones with a different task_attr */
	for (i = 0; i < skin->users_mem_size; ++i)
	{
		bool active = true, same;
		struct skin_user *user = skin->users[i];

		if (user == NULL)
			continue;

		/* check to see if reader's task attribute is different from task_attr */
		same = user->reader->period == task_attr.period && user->reader->soft == task_attr.soft;

		/* check to see if driver is active (only if same, because otherwise the driver should be detached from anyway) */
		if (same)
		{
			if ((error = skin_internal_driver_read_lock(&skin->kernel_locks)))
				continue;
			active = drivers[user->driver_index].active;
			skin_internal_driver_read_unlock(&skin->kernel_locks);
		}

		if (same && active)
			continue;

		/* similar to skin_unload, tell readers to stop and delete later so the delays are done in parallel */
		user->mark_for_removal = true;
		user->reader->must_stop = 1;
		++updated;
	}

	for (i = 0; i < skin->users_mem_size; ++i)
		if (skin->users[i] && skin->users[i]->mark_for_removal)
			skin_driver_detach(skin->users[i]);

	/* attach to any driver not already attached to */
	error = skin_load(skin, &task_attr);

	/* if at least one has been updated, call it a success */
	return updated?0:error;
}
URT_EXPORT_SYMBOL(skin_update);

static void pause_resume(struct skin *skin, bool pause)
{
	size_t i;
	int (*writer_pause)(struct skin_writer *) = pause?skin_writer_pause:skin_writer_resume;
	int (*reader_pause)(struct skin_reader *) = pause?skin_reader_pause:skin_reader_resume;

	for (i = 0; i < skin->writers_mem_size; ++i)
		writer_pause(skin->writers[i]);

	for (i = 0; i < skin->readers_mem_size; ++i)
		reader_pause(skin->readers[i]);
}

void skin_pause(struct skin *skin)
{
	pause_resume(skin, true);
}
URT_EXPORT_SYMBOL(skin_pause);

void skin_resume(struct skin *skin)
{
	pause_resume(skin, false);
}
URT_EXPORT_SYMBOL(skin_resume);

void (skin_request)(struct skin *skin, volatile sig_atomic_t *stop, ...)
{
	size_t i;

	/* note: the caller must make sure no users are added or removed in the meantime */

	/* send a request to all sporadic users (nonsporadic users automatically fail) */
	for (i = 0; i < skin->users_mem_size; ++i)
		if (skin->users[i])
			skin_reader_request_nonblocking(skin->users[i]->reader);

	/* wait for their responses */
	for (i = 0; i < skin->users_mem_size; ++i)
		if (skin->users[i])
			skin_reader_await_response(skin->users[i]->reader, stop);
}
URT_EXPORT_SYMBOL(skin_request);
