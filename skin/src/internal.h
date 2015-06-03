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

#ifndef INTERNAL_H
#define INTERNAL_H

#include <skin_types.h>
#include <skin_hooks.h>
#include "config.h"
#include "writer_internal.h"
#include "driver_internal.h"

struct skin_writer;
struct skin_reader;
struct skin_driver;
struct skin_user;

/* shared memory between all instances */
struct skin_kernel
{
	/* data for environment sanity check */
	size_t sizeof_skin_kernel;
	size_t sizeof_sensor;
	size_t sizeof_module;
	size_t sizeof_patch;
	size_t sizeof_writer_info;
	size_t sizeof_driver_info;

	bool initialized;		/* whether this structure is initialized */
	bool initialization_failed;	/* whether initialization has failed */

	uint16_t max_driver_count;	/* maximum number of drivers (same as SKIN_CONFIG_MAX_DRIVERS) */
	uint16_t max_writer_count;	/* maximum total number of writers (sum of SKIN_CONFIG_MAX_DRIVERS and SERVICES */
	struct skin_driver_info drivers[SKIN_CONFIG_MAX_DRIVERS];
					/*
					 * driver information.  Used by users to discover the skin and
					 * read the skin structure as well as sensor responses
					 */
	struct skin_writer_info writers[];
					/*
					 * writer information.  Driver and service writers may be interleaved.
					 * Used by readers for synchronization
					 */
};

/* shared locks related to skin_kernel */
struct skin_kernel_locks
{
#ifndef __KERNEL__
	int fid;
#endif
	void *global;		/* lock for modifying skin_kernel data (except data with specific locks below) */
	void *drivers;		/* lock for modifying drivers */
};

struct skin
{
	/* shared skin data */
	struct skin_kernel *kernel;
	struct skin_kernel_locks kernel_locks;

	/* individual book-keeping */

	/*
	 * a list of every object created (writers, readers, drivers and users), so they could be
	 * cleaned up automatically on skin_free.  These lists are not protected by locks, therefore
	 * object creation, cleanup and access must be mutually exclusive.  If that cannot be
	 * guaranteed, the user must use locks herself.
	 */
	struct skin_writer **writers;
	struct skin_reader **readers;
	struct skin_driver **drivers;
	struct skin_user **users;
	size_t writers_mem_size;
	size_t readers_mem_size;
	size_t drivers_mem_size;
	size_t users_mem_size;

	/* hooks */
	skin_hook_writer writer_init_hook;	void *writer_init_user_data;
	skin_hook_writer writer_clean_hook;	void *writer_clean_user_data;
	skin_hook_reader reader_init_hook;	void *reader_init_user_data;
	skin_hook_reader reader_clean_hook;	void *reader_clean_user_data;
	skin_hook_driver driver_init_hook;	void *driver_init_user_data;
	skin_hook_driver driver_clean_hook;	void *driver_clean_user_data;
	skin_hook_user user_init_hook;		void *user_init_user_data;
	skin_hook_user user_clean_hook;		void *user_clean_user_data;
	skin_hook_sensor sensor_init_hook;	void *sensor_init_user_data;
	skin_hook_sensor sensor_clean_hook;	void *sensor_clean_user_data;
	skin_hook_module module_init_hook;	void *module_init_user_data;
	skin_hook_module module_clean_hook;	void *module_clean_user_data;
	skin_hook_patch patch_init_hook;	void *patch_init_user_data;
	skin_hook_patch patch_clean_hook;	void *patch_clean_user_data;

#ifndef NDEBUG
	void *log_file;
#endif
};

/* regarding locking of skin kernel */
#define SKIN_KERNEL_GLOBAL_LOCK 1
#define SKIN_KERNEL_DRIVER_LOCK 2

#define SKIN_KERNEL_LOCK_WRITE_LOCK 1
#define SKIN_KERNEL_LOCK_WRITE_UNLOCK 2
#define SKIN_KERNEL_LOCK_READ_LOCK 3
#define SKIN_KERNEL_LOCK_READ_UNLOCK 4

int skin_internal_global_write_lock(struct skin_kernel_locks *locks);
int skin_internal_global_read_lock(struct skin_kernel_locks *locks);
int skin_internal_global_write_unlock(struct skin_kernel_locks *locks);
int skin_internal_global_read_unlock(struct skin_kernel_locks *locks);

int skin_internal_driver_write_lock(struct skin_kernel_locks *locks);
int skin_internal_driver_read_lock(struct skin_kernel_locks *locks);
int skin_internal_driver_write_unlock(struct skin_kernel_locks *locks);
int skin_internal_driver_read_unlock(struct skin_kernel_locks *locks);

#ifdef __KERNEL__
# include <linux/spinlock.h>
extern rwlock_t skin_internal_global_lock;
extern rwlock_t skin_internal_driver_lock;
#endif

/* some functionality used by more than one module */
void skin_internal_wait_termination(bool *running);
void skin_internal_signal_all_requests(urt_sem *req, urt_sem *res);
#define SKIN_DEFINE_STORE_FUNCTION(object)					\
static void _store_##object(struct skin *skin, struct skin_##object *object)	\
{										\
	size_t i;								\
	void *enlarged;								\
	size_t size = skin->object##s_mem_size, new_size;			\
										\
	for (i = 0; i < size; ++i)						\
		if (skin->object##s[i] == NULL)					\
		{								\
			skin->object##s[i] = object;				\
			object->index = i;					\
			return;							\
		}								\
										\
	/* if not found, try to increase memory size */				\
	new_size = 2 * size;							\
	enlarged = urt_mem_resize(skin->object##s,				\
			size * sizeof *skin->object##s,				\
			new_size * sizeof *skin->object##s);			\
										\
	/*									\
	 * if not enough memory, then let them continue with			\
	 * having the object, but it won't get cleaned up			\
	 */									\
	if (enlarged == NULL)							\
		return;								\
										\
	skin->object##s = enlarged;						\
	skin->object##s_mem_size = new_size;					\
	for (i = size; i < new_size; ++i)					\
		skin->object##s[i] = NULL;					\
										\
	/* place the object in the beginning of the empty space just created */	\
	skin->object##s[size] = object;						\
	object->index = size;							\
}
bool skin_internal_writer_is_active(struct skin *skin, uint16_t writer_index);
bool skin_internal_driver_is_active(struct skin *skin, uint16_t driver_index);

#define internal_error(...)							\
do {										\
	urt_err("internal error: "__VA_ARGS__);					\
	urt_err("          note: please report this bug\n");			\
	urt_err("          note: https://github.com/maclab/skinware/issues\n");	\
} while (0)

#endif
