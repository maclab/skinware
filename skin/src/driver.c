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
#include "data_internal.h"
#include "names.h"

static int _sanity_check_skin(struct skin *skin)
{
	return skin == NULL || skin->kernel == NULL?-1:0;
}

static int _sanity_check_driver(struct skin_driver *driver)
{
	return driver == NULL || driver->skin == NULL || driver->skin->kernel == NULL
		|| driver->info_index >= driver->skin->kernel->max_driver_count?-1:0;
}

static struct skin_driver *_revive_driver(struct skin *skin, const struct skin_driver_attr *attr, const struct skin_writer *writer, int *error)
{
	struct skin_kernel *sk = skin->kernel;
	struct skin_driver *driver = NULL;
	struct skin_writer_info *writer_info = &sk->writers[writer->info_index];
	struct skin_driver_info *d = &sk->drivers[writer_info->driver_index];
	char name[URT_NAME_LEN + 1];

	/* attach to the data structure memory of the reviving driver */
	skin_internal_name_set(name, writer_info->attr.prefix, "DS");
	SKIN_DEFINE_STRUCTURE(*attr);
	skin_structure *ds = urt_shmem_attach(name, error);
	if (ds == NULL)
		goto exit_no_ds;

	*error = EEXIST;

	/* make sure the new and old attributes match */
	if (d->attr.patch_count != attr->patch_count || d->attr.module_count != attr->module_count
			|| d->attr.sensor_count != attr->sensor_count)
		goto exit_fail;
	/* make sure the data structure layout is the same.  If not, it could be because of different compiler versions */
	if (ds->data_structure_size != sizeof(skin_structure)
			|| ds->modules_offset != offsetof(skin_structure, modules)
			|| ds->sensors_offset != offsetof(skin_structure, sensors))
	{
		urt_err("warning: driver '%*s' data structure layout mismatch\n",
				URT_NAME_LEN - 3, writer_info->attr.prefix);
		urt_err("   note: this could be due to different skin version or configuration\n");
		urt_err("   note: ... or due to different compiler, compiler version or options\n");
		goto exit_fail;
	}

	*error = EALREADY;

	driver = urt_mem_new(sizeof *driver, error);
	if (driver == NULL)
		goto exit_fail;

	d->active = true;

	*driver = (struct skin_driver){
		.data_structure = ds,
		.info_index = writer_info->driver_index,
	};

exit_no_ds:
	return driver;
exit_fail:
	urt_shmem_detach(ds);
	return NULL;
}

static struct skin_driver *_add_driver(struct skin *skin, const struct skin_driver_attr *attr, const struct skin_writer *writer, int *error)
{
	unsigned int i;
	struct skin_kernel *sk = skin->kernel;
	struct skin_driver *driver = NULL;
	struct skin_writer_info *writer_info = &sk->writers[writer->info_index];
	char name[URT_NAME_LEN + 1];

	/* allocate memory for the driver data structure */
	skin_internal_name_set(name, writer_info->attr.prefix, "DS");
	SKIN_DEFINE_STRUCTURE(*attr);
	skin_structure *ds = urt_shmem_new(name, sizeof(skin_structure), error);
	if (ds == NULL)
		goto exit_no_ds;
	ds->data_structure_size = sizeof(skin_structure);
	ds->modules_offset = offsetof(skin_structure, modules);
	ds->sensors_offset = offsetof(skin_structure, sensors);

	for (i = 0; i < sk->max_driver_count; ++i)
	{
		struct skin_driver_info *d = &sk->drivers[i];

		if (!d->active && d->users_attached == 0)
		{
			driver = urt_mem_new(sizeof *driver, error);
			if (driver == NULL)
				goto exit_fail;

			*d = (struct skin_driver_info){
				.attr = *attr,
				.writer_index = writer->info_index,
				.active = true,
			};
			*driver = (struct skin_driver){
				.data_structure = ds,
				.info_index = i,
			};
			writer_info->driver_index = i;

			break;
		}
	}

	if (driver == NULL)
	{
		*error = ENOSPC;
		goto exit_fail;
	}

exit_no_ds:
	return driver;
exit_fail:
	urt_shmem_detach(ds);
	return NULL;
}

static int _check_data_structure_consistency(struct skin_driver_details *details)
{
	skin_module_size total_modules = 0;
	skin_sensor_size total_sensors = 0;
	skin_patch_id p;
	skin_module_id m;

	for (p = 0; p < details->overall.patch_count; ++p)
		total_modules += details->patches[p].module_count;
	if (total_modules != details->overall.module_count)
		return -1;

	for (m = 0; m < details->overall.module_count; ++m)
		total_sensors += details->modules[m].sensor_count;
	if (total_sensors != details->overall.sensor_count)
		return -1;

	return 0;
}

static int _cache_sensor_types(struct skin_driver_info *d, struct skin_driver_details *details)
{
	skin_sensor_id s;
	skin_sensor_type_id i;

	d->sensor_type_count = 0;
	for (s = 0; s < details->overall.sensor_count; ++s)
	{
		bool already = false;
		struct skin_sensor_decl *sensor = &details->sensors[s];

		for (i = 0; i < d->sensor_type_count; ++i)
			if (d->sensor_types[i] == sensor->type)
			{
				already = true;
				break;
			}

		if (already)
			continue;

		if (d->sensor_type_count >= SKIN_CONFIG_MAX_SENSOR_TYPES)
			return -1;

		d->sensor_types[d->sensor_type_count++] = sensor->type;
	}
	return 0;
}

SKIN_DEFINE_STORE_FUNCTION(driver);

static int _driver_writer_callback(struct skin_writer *writer, void *mem, size_t size, void *user_data)
{
	struct skin_driver *driver = user_data;
	struct skin_driver_info *info = &driver->skin->kernel->drivers[driver->info_index];
	return driver->callbacks.acquire(driver, mem, info->attr.sensor_count, driver->callbacks.user_data);
}

static void _driver_writer_init(struct skin_writer *writer, void *user_data)
{
	struct skin_driver *driver = user_data;
	driver->callbacks.writer_init(writer, driver->callbacks.user_data);
}

static void _driver_writer_clean(struct skin_writer *writer, void *user_data)
{
	struct skin_driver *driver = user_data;
	driver->callbacks.writer_clean(writer, driver->callbacks.user_data);
}

struct skin_driver *(skin_driver_add)(struct skin *skin, const struct skin_driver_attr *attr_,
		const struct skin_writer_attr *writer_attr_, const urt_task_attr *task_attr_,
		const struct skin_driver_callbacks *callbacks, int *error, ...)
{
	struct skin_driver *driver = NULL;
	struct skin_writer *writer = NULL;
	int err = 0;
	bool revived = false;
	struct skin_driver_attr attr;
	struct skin_writer_attr writer_attr;
	struct urt_task_attr task_attr;

	if (_sanity_check_skin(skin) || attr_ == NULL || writer_attr_ == NULL || task_attr_ == NULL
			|| callbacks == NULL || callbacks->acquire == NULL || callbacks->details == NULL
			|| writer_attr_->name == NULL || writer_attr_->name[0] == '\0')
		goto exit_bad_param;

	attr = *attr_;
	writer_attr = *writer_attr_;
	task_attr = *task_attr_;

	/* do sanity checks */
	if (attr.patch_count == 0 || attr.module_count == 0 || attr.sensor_count == 0)
		goto exit_bad_param;
	writer_attr.buffer_size = attr.sensor_count * sizeof(skin_sensor_response);

	/* default values */
	if (!urt_priority_is_valid(task_attr.priority))
		task_attr.priority = urt_priority(SKIN_CONFIG_PRIORITY_DRIVER);

	/* try to start the writer, and check if it is revived */
	writer = skin_service_add(skin, &writer_attr, &task_attr, &(struct skin_writer_callbacks){
				.write = _driver_writer_callback,
				.init = callbacks->writer_init?_driver_writer_init:NULL,
				.clean = callbacks->writer_clean?_driver_writer_clean:NULL
			}, &err);
	if (writer == NULL)
		goto exit_no_writer;
	revived = err == EALREADY;

	/* if the writer is revived and doesn't belong to a driver, it's an error */
	if (revived && skin->kernel->writers[writer->info_index].driver_index >= skin->kernel->max_driver_count)
		goto exit_revived_but_not_driver;

	if ((err = skin_internal_driver_write_lock(&skin->kernel_locks)))
		goto exit_no_drivers_lock;

	/*
	 * if revived, find the driver_info object and check to see if the overall structure matches.
	 * Furthermore, attach to the data structure memory of the driver.  If there is a mismatch, or
	 * no driver associated with the revived writer is found, it's an EEXIST error.  Similar to
	 * skin_service_add, if revived, set error to EALREADY.
	 *
	 * if not revived, add a new driver.
	 */
	if (revived)
		driver = _revive_driver(skin, &attr, writer, &err);
	else
		driver = _add_driver(skin, &attr, writer, &err);

	skin_internal_driver_write_unlock(&skin->kernel_locks);

	if (driver == NULL)
		goto exit_no_driver;

	if (revived && error)
		*error = err;

	*driver = (struct skin_driver){
		.callbacks = *callbacks,
		.data_structure = driver->data_structure,
		.skin = skin,
		.writer = writer,
		.info_index = driver->info_index,
	};
	writer->driver = driver;

	/* update the writer's user_data to point to the recently created driver */
	writer->callbacks.user_data = driver;

	/* call the details function to fill in data structure (if not revived), or verify structure (if revived) */
	/* Note: scope is created because inside there is an identifier with variable size and there cannot be a goto over it */
	{
		SKIN_DEFINE_STRUCTURE(attr);
		skin_structure *ds = driver->data_structure;
		struct skin_driver_details details = {
			.overall = attr,
			.patches = ds->patches,
			.modules = ds->modules,
			.sensors = ds->sensors,
		};
		if (callbacks->details(driver, revived, &details, callbacks->user_data))
		{
			err = ECANCELED;
			goto exit_no_details;
		}

		/* check the consistency of the data structure memory (even if revived, since a bad user may have changed it) */
		details.overall = attr;
		if (_check_data_structure_consistency(&details))
		{
			err = EINVAL;
			goto exit_bad_details;
		}

		/* cache used sensor types */
		if (_cache_sensor_types(&skin->kernel->drivers[driver->info_index], &details))
		{
			err = EINVAL;
			goto exit_bad_details;
		}
	}

	/* store the pointer in internal memory */
	_store_driver(skin, driver);

	/* call the object-specific init hook */
	if (callbacks->init)
		callbacks->init(driver, callbacks->user_data);

	/* call the generic init hook */
	if (skin->driver_init_hook)
		skin->driver_init_hook(driver, skin->driver_init_user_data);

	return driver;
exit_bad_details:
exit_no_details:
	skin_driver_remove(driver);
	writer = NULL;
exit_no_driver:
	skin_service_remove(writer);
exit_no_writer:
exit_no_drivers_lock:
exit_fail:
	/* if writer is NULL, call the clean hook anyway, in case `user_data` requires cleanup */
	if (writer == NULL && callbacks && callbacks->writer_clean)
		callbacks->writer_clean(writer, callbacks->user_data);
	/* if driver is NULL, call the clean hook anyway, in case `user_data` requires cleanup */
	if (driver == NULL && callbacks && callbacks->clean)
		callbacks->clean(driver, callbacks->user_data);
	if (error)
		*error = err;
	return NULL;
exit_bad_param:
	err = EINVAL;
	goto exit_fail;
exit_revived_but_not_driver:
	err = EEXIST;
	goto exit_no_driver;
}
URT_EXPORT_SYMBOL(skin_driver_add);

void skin_driver_remove(struct skin_driver *driver)
{
	bool locked;

	if (driver == NULL || _sanity_check_skin(driver->skin) || _sanity_check_driver(driver))
		return;

	/* remove it from local book-keeping, making sure it had been added for book-keeping */
	if (driver->index < driver->skin->drivers_mem_size)
		if (driver == driver->skin->drivers[driver->index])
			driver->skin->drivers[driver->index] = NULL;

	locked = skin_internal_driver_write_lock(&driver->skin->kernel_locks) == 0;

	/* mark it as inactive */
	driver->skin->kernel->drivers[driver->info_index].active = false;

	if (locked)
		skin_internal_driver_write_unlock(&driver->skin->kernel_locks);

	/* remove the writer associated with this driver */
	skin_service_remove(driver->writer);
	driver->writer = NULL;

	/* detach from memory */
	urt_shmem_detach(driver->data_structure);

	/* call the generic clean hook */
	if (driver->skin->driver_clean_hook)
		driver->skin->driver_clean_hook(driver, driver->skin->driver_clean_user_data);

	/* call the object-specific clean hook */
	if (driver->callbacks.clean)
		driver->callbacks.clean(driver, driver->callbacks.user_data);

	/* final cleanup */
	urt_mem_delete(driver);
}
URT_EXPORT_SYMBOL(skin_driver_remove);

bool skin_driver_is_active(struct skin_driver *driver)
{
	if (_sanity_check_driver(driver))
		return false;

	return skin_internal_driver_is_active(driver->skin, driver->info_index);
}
URT_EXPORT_SYMBOL(skin_driver_is_active);

struct skin_writer *skin_driver_get_writer(struct skin_driver *driver)
{
	if (_sanity_check_driver(driver))
		return NULL;
	return driver->writer;
}
URT_EXPORT_SYMBOL(skin_driver_get_writer);

int skin_driver_get_attr(struct skin_driver *driver, struct skin_driver_attr *attr)
{
	int err = 0;

	if (_sanity_check_driver(driver) || attr == NULL)
		return EINVAL;

	if ((err = skin_internal_driver_read_lock(&driver->skin->kernel_locks)))
		return err;

	*attr = driver->skin->kernel->drivers[driver->info_index].attr;

	skin_internal_driver_read_unlock(&driver->skin->kernel_locks);

	return 0;
}
URT_EXPORT_SYMBOL(skin_driver_get_attr);
