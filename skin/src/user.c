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
#include <skin_patch.h>
#include <skin_module.h>
#include <skin_sensor.h>
#include "internal.h"
#include "user_internal.h"
#include "reader_internal.h"
#include "data_internal.h"
#include "names.h"

static int _sanity_check_skin(struct skin *skin)
{
	return skin == NULL || skin->kernel == NULL?-1:0;
}

static int _sanity_check_user(struct skin_user *user)
{
	return user == NULL || user->skin == NULL || user->skin->kernel == NULL
		|| user->driver_index >= user->skin->kernel->max_driver_count?-1:0;
}

static int _find_driver_by_sensor_type(struct skin *skin, char *reader_prefix, skin_sensor_type_id sensor_type, int *error)
{
	unsigned int i, j, k;
	struct skin_kernel *sk = skin->kernel;
	int ret = -1;
	bool found = false;

	if ((*error = skin_internal_driver_read_lock(&skin->kernel_locks)))
		goto exit_no_drivers_lock;

	for (i = 0; i < sk->max_driver_count && !found; ++i)
	{
		struct skin_driver_info *d = &sk->drivers[i];

		if (!d->active)
			continue;

		for (j = 0; j < d->sensor_type_count; ++j)
			if (d->sensor_types[j] == sensor_type)
			{
				struct skin_writer_info *w = &sk->writers[d->writer_index];
				bool exists = false;

				/* make sure we aren't already attached to this driver */
				for (k = 0; k < skin->users_mem_size; ++k)
					if (skin->users[k] != NULL)
					{
						struct skin_writer_info *uw = &sk->writers[skin->users[k]->reader->writer_index];

						if (skin_internal_name_cmp_prefix(w->attr.prefix, uw->attr.prefix) == 0)
						{
							exists = true;
							break;
						}
					}

				if (!exists)
				{
					skin_internal_name_cpy_prefix(reader_prefix, w->attr.prefix);
					found = true;
					ret = 0;
					break;
				}
			}
	}

	if (ret)
		*error = ENOENT;

	skin_internal_driver_read_unlock(&skin->kernel_locks);
exit_no_drivers_lock:
	return ret;
}

static struct skin_user *_attach_user(struct skin *skin, const struct skin_user_attr *attr, const struct skin_reader *reader, int *error)
{
	unsigned int i;
	struct skin_kernel *sk = skin->kernel;
	struct skin_user *user = NULL;
	struct skin_writer_info *writer_info = &sk->writers[reader->writer_index];
	char name[URT_NAME_LEN + 1];

	for (i = 0; i < sk->max_driver_count; ++i)
	{
		struct skin_driver_info *d = &sk->drivers[i];

		if (d->writer_index == reader->writer_index)
		{
			/* attach to the data structure memory of the driver */
			skin_internal_name_set(name, writer_info->attr.prefix, "DS");
			SKIN_DEFINE_STRUCTURE(d->attr);
			skin_structure *ds = urt_shmem_attach(name, error);
			if (ds == NULL)
				goto exit_fail;

			user = urt_mem_new(sizeof *user, error);
			if (user == NULL)
			{
				urt_shmem_detach(ds);
				goto exit_fail;
			}

			++d->users_attached;

			*user = (struct skin_user){
				.driver_attr = d->attr,
				.data_structure = ds,
				.driver_index = i,
			};

			break;
		}
	}

	if (user == NULL)
		*error = ENOENT;

exit_fail:
	return user;
}

static int _construct_structure(struct skin_user *user)
{
	int err;
	skin_patch_id p;
	skin_module_id m;
	skin_sensor_id s;
	skin_module_size m_so_far;
	skin_sensor_size s_so_far;
	SKIN_DEFINE_STRUCTURE(user->driver_attr);
	skin_structure *ds = user->data_structure;

	user->patches = urt_mem_new(user->driver_attr.patch_count * sizeof *user->patches, &err);
	user->modules = urt_mem_new(user->driver_attr.module_count * sizeof *user->modules, &err);
	user->sensors = urt_mem_new(user->driver_attr.sensor_count * sizeof *user->sensors, &err);

	if (user->patches == NULL || user->modules == NULL || user->sensors == NULL)
		goto exit_no_mem;

	/* initialize sensors */
	for (s = 0; s < user->driver_attr.sensor_count; ++s)
	{
		user->sensors[s] = (struct skin_sensor){
			.id = s,
			.uid = ds->sensors[s].uid,
			/* .module is filled later */
			.type = ds->sensors[s].type,
			.user = user,
		};
	}

	/* initialize modules */
	s_so_far = 0;
	for (m = 0; m < user->driver_attr.module_count; ++m)
	{
		user->modules[m] = (struct skin_module){
			.id = m,
			/* .patch is filled later */
			.user = user,
			.sensors = user->sensors + s_so_far,
			.sensor_count = ds->modules[m].sensor_count,
		};
		s_so_far += user->modules[m].sensor_count;

		/* set back reference from sensors to module */
		for (s = 0; s < user->modules[m].sensor_count; ++s)
			user->modules[m].sensors[s].module = m;
	}

	/* initialize patches */
	m_so_far = 0;
	for (p = 0; p < user->driver_attr.patch_count; ++p)
	{
		user->patches[p] = (struct skin_patch){
			.id = p,
			.user = user,
			.modules = user->modules + m_so_far,
			.module_count = ds->patches[p].module_count,
		};
		m_so_far += user->patches[p].module_count;

		/* set back reference from modules to patch */
		for (m = 0; m < user->patches[p].module_count; ++m)
			user->patches[p].modules[m].patch = p;
	}

	return 0;
exit_no_mem:
	urt_mem_delete(user->patches);
	urt_mem_delete(user->modules);
	urt_mem_delete(user->sensors);
	return err;
}

static void _build_sensor_type_indices(struct skin_user *user, struct skin_driver_info *driver_info)
{
	skin_sensor_id s;
	skin_sensor_type_id i;
	skin_sensor_type_id last_sensor_of_type[SKIN_CONFIG_MAX_SENSOR_TYPES];

	/* mark all as invalid */
	user->sensor_type_count = driver_info->sensor_type_count;
	for (i = 0; i < user->sensor_type_count; ++i)
	{
		user->sensor_types[i].type = driver_info->sensor_types[i];
		user->sensor_types[i].first_sensor = SKIN_INVALID_ID;
		last_sensor_of_type[i] = SKIN_INVALID_ID;
	}

	/* find the first of each sensor type */
	for (s = 0; s < user->driver_attr.sensor_count; ++s)
	{
		struct skin_sensor *sensor = &user->sensors[s];

		for (i = 0; i < user->sensor_type_count; ++i)
			if (user->sensor_types[i].type == sensor->type)
				break;

		if (i >= user->sensor_type_count)
		{
			internal_error("user detected sensor type (%u) that was undetected by driver ('%s')\n",
					sensor->type, user->skin->kernel->writers[driver_info->writer_index].attr.prefix);
			continue;
		}

		/* if not already set, this is the first of this type */
		if (user->sensor_types[i].first_sensor >= user->driver_attr.sensor_count)
			user->sensor_types[i].first_sensor = s;
		/* otherwise, there was a sensor before this, so set that sensor's next to this sensor */
		else
			user->sensors[last_sensor_of_type[i]].next_of_type = s;

		last_sensor_of_type[i] = s;
	}

	/* set the next of the last visited sensor types to invalid */
	for (i = 0; i < user->sensor_type_count; ++i)
		if (last_sensor_of_type[i] < user->driver_attr.sensor_count)
			user->sensors[last_sensor_of_type[i]].next_of_type = SKIN_INVALID_ID;
}

SKIN_DEFINE_STORE_FUNCTION(user);

static void _copy_sensor_responses(struct skin_user *user, skin_sensor_response *responses,
		skin_sensor_size sensor_count, void *user_data)
{
	skin_sensor_id s;

/* TODO: remove this check after test */
if (sensor_count != user->driver_attr.sensor_count)
urt_err("internal error: mismatch between acq sensor count and creation-time sensor count\n");

	for (s = 0; s < sensor_count; ++s)
		user->sensors[s].response = responses[s];
}

static void _user_reader_callback(struct skin_reader *reader, void *mem, size_t size, void *user_data)
{
	struct skin_user *user = user_data;
	user->callbacks.peek(user, mem, user->driver_attr.sensor_count, user->callbacks.user_data);
}

static void _user_reader_init(struct skin_reader *reader, void *user_data)
{
	struct skin_user *user = user_data;
	user->callbacks.reader_init(reader, user->callbacks.user_data);
}

static void _user_reader_clean(struct skin_reader *reader, void *user_data)
{
	struct skin_user *user = user_data;
	user->callbacks.reader_clean(reader, user->callbacks.user_data);
}

struct skin_user *(skin_driver_attach)(struct skin *skin, const struct skin_user_attr *attr_,
		const struct skin_reader_attr *reader_attr_, const urt_task_attr *task_attr_,
		const struct skin_user_callbacks *callbacks_, int *error, ...)
{
	struct skin_user *user = NULL;
	struct skin_reader *reader = NULL;
	char reader_prefix[URT_NAME_LEN - 3 + 1];
	int err = 0;
	skin_patch_id p;
	skin_module_id m;
	skin_sensor_id s;
	struct skin_user_attr attr;
	struct skin_reader_attr reader_attr;
	struct urt_task_attr task_attr;
	struct skin_user_callbacks callbacks;

	/*
	 * Note: users find drivers by prefix like readers do writers, except an empty prefix
	 * is allowed for users, in which case a driver handling the given sensor type is
	 * found.
	 */
	if (_sanity_check_skin(skin) || attr_ == NULL || task_attr_ == NULL || callbacks_ == NULL)
		goto exit_bad_param;

	attr = *attr_;
	reader_attr = reader_attr_?*reader_attr_:(struct skin_reader_attr){0};
	task_attr = *task_attr_;
	callbacks = *callbacks_;

	/* default values */
	if (callbacks.peek == NULL)
		callbacks.peek = _copy_sensor_responses;
	if (!urt_priority_is_valid(task_attr.priority))
		task_attr.priority = urt_priority(SKIN_CONFIG_PRIORITY_USER);

	/*
	 * if reader name is not given, search the drivers for one that has sensor type
	 * as given and not previously attached.  If found, write its writer's prefix
	 * as reader_prefix.  In end, when the reader is attached, gets its attribute and
	 * overwrite the given reader attribute.
	 */
	if (reader_attr.name == NULL || reader_attr.name[0] == '\0')
	{
		if (_find_driver_by_sensor_type(skin, reader_prefix, attr.sensor_type, &err))
			goto exit_no_driver_of_sensor_type;
		reader_attr.name = reader_prefix;
	}

	/*
	 * Now reader prefix is definitely given, so simply attach to the writer.  Then
	 * check if it actually corresponded to a driver.  This step is necessary if the reader
	 * prefix was originally given, in which case we don't know yet if the reader comes from
	 * a driver, or if the driver found has been removed in the meantime.
	 */
	reader = skin_service_attach(skin, &reader_attr, &task_attr, &(struct skin_reader_callbacks){
				.read = _user_reader_callback,
				.init = callbacks.reader_init?_user_reader_init:NULL,
				.clean = callbacks.reader_clean?_user_reader_clean:NULL
			}, &err);
	if (reader == NULL)
		goto exit_no_reader;

	/*
	 * the reader attribute may now be pointing to reader_prefix inside this function.
	 * Try to get it to point to a persistent memory (if possible)!
	 */
	if (reader_attr.name == reader_prefix && skin_reader_get_attr(reader, &reader_attr))
		reader_attr.name = NULL;

	if ((err = skin_internal_driver_write_lock(&skin->kernel_locks)))
		goto exit_no_drivers_lock;

	user = _attach_user(skin, &attr, reader, &err);

	skin_internal_driver_write_unlock(&skin->kernel_locks);

	if (user == NULL)
		goto exit_no_user;

	*user = (struct skin_user){
		.callbacks = callbacks,
		.driver_attr = user->driver_attr,
		.data_structure = user->data_structure,
		.skin = skin,
		.reader = reader,
		.driver_index = user->driver_index,
	};
	reader->user = user;

	/* update the readers's user_data to point to the recently created user */
	reader->callbacks.user_data = user;

	/* construct data structures out of the basic structure taken from the driver */
	if ((err = _construct_structure(user)))
		goto exit_no_structure;

	/* build sensor type indices */
	_build_sensor_type_indices(user, &skin->kernel->drivers[user->driver_index]);

	/* store the pointer in internal memory */
	_store_user(skin, user);

	/* call the object-specific init hooks */
	if (callbacks.init)
		callbacks.init(user, callbacks.user_data);

	/* call the generic init hooks */
	if (skin->user_init_hook)
		skin->user_init_hook(user, skin->user_init_user_data);
	if (skin->sensor_init_hook)
		for (s = 0; s < user->driver_attr.sensor_count; ++s)
			skin->sensor_init_hook(&user->sensors[s], skin->sensor_init_user_data);
	if (skin->module_init_hook)
		for (m = 0; m < user->driver_attr.module_count; ++m)
			skin->module_init_hook(&user->modules[m], skin->module_init_user_data);
	if (skin->patch_init_hook)
		for (p = 0; p < user->driver_attr.patch_count; ++p)
			skin->patch_init_hook(&user->patches[p], skin->patch_init_user_data);

	return user;
exit_no_structure:
	skin_driver_detach(user);
	reader = NULL;
exit_no_user:
exit_no_drivers_lock:
	skin_service_detach(reader);
exit_no_reader:
exit_no_driver_of_sensor_type:
exit_fail:
	/* if reader is NULL, call the clean hook anyway, in case `user_data` requires cleanup */
	if (reader == NULL && callbacks_ && callbacks_->reader_clean)
		callbacks_->reader_clean(reader, callbacks_->user_data);
	/* if user is NULL, call the clean hook anyway, in case `user_data` requires cleanup */
	if (user == NULL && callbacks_ && callbacks_->clean)
		callbacks_->clean(user, callbacks_->user_data);
	if (error)
		*error = err;
	return NULL;
exit_bad_param:
	err = EINVAL;
	goto exit_fail;
}
URT_EXPORT_SYMBOL(skin_driver_attach);

void skin_driver_detach(struct skin_user *user)
{
	bool locked;
	skin_patch_id p;
	skin_module_id m;
	skin_sensor_id s;

	if (user == NULL || _sanity_check_skin(user->skin) || _sanity_check_user(user))
		return;

	/* remove it from local book-keeping, making sure it had been added for book-keeping */
	if (user->index < user->skin->users_mem_size)
		if (user == user->skin->users[user->index])
			user->skin->users[user->index] = NULL;

	locked = skin_internal_driver_write_lock(&user->skin->kernel_locks) == 0;

	/* reduce its user count */
	--user->skin->kernel->drivers[user->driver_index].users_attached;

	if (locked)
		skin_internal_driver_write_unlock(&user->skin->kernel_locks);

	/* remove the reader associated with this driver */
	skin_service_detach(user->reader);
	user->reader = NULL;

	/* detach from memory */
	urt_shmem_detach(user->data_structure);

	/* call the generic clean hooks */
	if (user->skin->user_clean_hook)
		user->skin->user_clean_hook(user, user->skin->user_clean_user_data);
	if (user->skin->patch_clean_hook)
		for (p = 0; p < user->driver_attr.patch_count; ++p)
			user->skin->patch_clean_hook(&user->patches[p], user->skin->patch_clean_user_data);
	if (user->skin->module_clean_hook)
		for (m = 0; m < user->driver_attr.module_count; ++m)
			user->skin->module_clean_hook(&user->modules[m], user->skin->module_clean_user_data);
	if (user->skin->sensor_clean_hook)
		for (s = 0; s < user->driver_attr.sensor_count; ++s)
			user->skin->sensor_clean_hook(&user->sensors[s], user->skin->sensor_clean_user_data);

	/* call the object-specific clean hooks */
	if (user->callbacks.clean)
		user->callbacks.clean(user, user->callbacks.user_data);

	/* final cleanup */
	urt_mem_delete(user->patches);
	urt_mem_delete(user->modules);
	urt_mem_delete(user->sensors);
	urt_mem_delete(user);
}
URT_EXPORT_SYMBOL(skin_driver_detach);

bool skin_user_is_active(struct skin_user *user)
{
	if (_sanity_check_user(user))
		return false;

	return skin_internal_driver_is_active(user->skin, user->driver_index);
}
URT_EXPORT_SYMBOL(skin_user_is_active);

struct skin_reader *skin_user_get_reader(struct skin_user *user)
{
	if (_sanity_check_user(user))
		return NULL;
	return user->reader;
}
URT_EXPORT_SYMBOL(skin_user_get_reader);

skin_sensor_size skin_user_sensor_count(struct skin_user *user)
{
	return user->driver_attr.sensor_count;
}
URT_EXPORT_SYMBOL(skin_user_sensor_count);

skin_module_size skin_user_module_count(struct skin_user *user)
{
	return user->driver_attr.module_count;
}
URT_EXPORT_SYMBOL(skin_user_module_count);

skin_patch_size skin_user_patch_count(struct skin_user *user)
{
	return user->driver_attr.patch_count;
}
URT_EXPORT_SYMBOL(skin_user_patch_count);

skin_sensor_type_size skin_user_sensor_type_count(struct skin_user *user)
{
	return user->sensor_type_count;
}
URT_EXPORT_SYMBOL(skin_user_sensor_type_count);
