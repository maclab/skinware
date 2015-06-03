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
#ifndef __KERNEL__
# include <pthread.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
#endif
#include <skin_base.h>
#include <skin_sensor.h>
#include <skin_module.h>
#include <skin_patch.h>
#include "internal.h"
#include "reader_internal.h"
#include "names.h"

static void _init_kernel(struct skin_kernel *sk)
{
	/* initialize the kernel */
	sk->initialized = false;
	sk->initialization_failed = false;

	*sk = (struct skin_kernel){
		.max_driver_count = SKIN_CONFIG_MAX_DRIVERS,
		.max_writer_count = SKIN_CONFIG_MAX_DRIVERS + SKIN_CONFIG_MAX_SERVICES,
		.sizeof_skin_kernel = sizeof(struct skin_kernel),
		.sizeof_sensor = sizeof(struct skin_sensor),
		.sizeof_module = sizeof(struct skin_module),
		.sizeof_patch = sizeof(struct skin_patch),
		.sizeof_writer_info = sizeof(struct skin_writer_info),
		.sizeof_driver_info = sizeof(struct skin_driver_info),
	};

	memset(sk->writers, 0, sizeof(struct skin_writer_info[sk->max_writer_count]));
}

static int _sanity_check_kernel(struct skin_kernel *sk)
{
	/* wait until initialization is done or failed */
	while (!sk->initialized && !sk->initialization_failed)
		urt_sleep(10000);
	if (sk->initialization_failed)
		return -1;

	return sk->sizeof_skin_kernel != sizeof(struct skin_kernel)
		|| sk->sizeof_sensor != sizeof(struct skin_sensor)
		|| sk->sizeof_module != sizeof(struct skin_module)
		|| sk->sizeof_patch != sizeof(struct skin_patch)
		|| sk->sizeof_writer_info != sizeof(struct skin_writer_info)
		|| sk->sizeof_driver_info != sizeof(struct skin_driver_info)
		?-1:0;
}

#if !defined(__KERNEL__) && !SKIN_CONFIG_SUPPORT_KERNEL_SPACE
static void *_get_kernel_lock(char *name, char *suffix, char *sfx, bool create, int *error)
{
# if !defined(_POSIX_THREAD_PROCESS_SHARED) || _POSIX_THREAD_PROCESS_SHARED <= 0
	*error = ENOTSUP;
	return NULL;
# else
	void *lock;

	skin_internal_name_set_suffix(suffix, sfx);
	if (create)
	{
		lock = urt_shmem_new(name, sizeof(pthread_rwlock_t), error);
		if (lock)
		{
			pthread_rwlockattr_t attr;

			pthread_rwlockattr_init(&attr);
			pthread_rwlockattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

			*error = pthread_rwlock_init(lock, &attr);

			pthread_rwlockattr_destroy(&attr);
		}
	}
	else
		lock = urt_shmem_attach(name, error);

	return lock;
# endif
}
#endif

static int _get_kernel_locks(struct skin_kernel_locks *kernel_locks, char *name, char *suffix, bool create)
{
	int error = 0;

#ifndef __KERNEL__
	/* in kernel space, global locks are linked from the main module and are accessible globally, so there is nothing to do */

# if SKIN_CONFIG_SUPPORT_KERNEL_SPACE
	/* if kernel space is supported, the global locks are accessed through a sysfs file */
	kernel_locks->fid = open("/sys/skin"SKIN_CONFIG_SUFFIX"/global_locks", O_WRONLY);
	if (kernel_locks->fid < 0)
		error = errno;
# else
	*kernel_locks = (struct skin_kernel_locks){
		.global = _get_kernel_lock(name, suffix, "GL", create, &error),
		.drivers = _get_kernel_lock(name, suffix, "DL", create, &error),
	};
# endif
#endif

	return error;
}

static int _get_memories(struct skin *skin)
{
	int error = 0;
	unsigned int i;

	skin->writers_mem_size = SKIN_CONFIG_MAX_DRIVERS + SKIN_CONFIG_MAX_SERVICES;
	skin->readers_mem_size = SKIN_CONFIG_MAX_DRIVERS + SKIN_CONFIG_MAX_SERVICES;
	skin->drivers_mem_size = SKIN_CONFIG_MAX_DRIVERS;
	skin->users_mem_size = SKIN_CONFIG_MAX_SERVICES;

	skin->writers = urt_mem_new(skin->writers_mem_size * sizeof *skin->writers, &error);
	skin->readers = urt_mem_new(skin->readers_mem_size * sizeof *skin->readers, &error);
	skin->drivers = urt_mem_new(skin->drivers_mem_size * sizeof *skin->drivers, &error);
	skin->users = urt_mem_new(skin->users_mem_size * sizeof *skin->users, &error);

	for (i = 0; i < skin->writers_mem_size; ++i)
		skin->writers[i] = NULL;
	for (i = 0; i < skin->readers_mem_size; ++i)
		skin->readers[i] = NULL;
	for (i = 0; i < skin->drivers_mem_size; ++i)
		skin->drivers[i] = NULL;
	for (i = 0; i < skin->users_mem_size; ++i)
		skin->users[i] = NULL;

	return error;
}

static void _free_memories(struct skin *skin)
{
	urt_mem_delete(skin->writers);
	urt_mem_delete(skin->readers);
	urt_mem_delete(skin->drivers);
	urt_mem_delete(skin->users);
}

static void _detach_from_kernel_locks(struct skin_kernel_locks *kernel_locks)
{
#ifndef __KERNEL__
# if SKIN_CONFIG_SUPPORT_KERNEL_SPACE
	if (kernel_locks->fid >= 0)
		close(kernel_locks->fid);
	kernel_locks->fid = -1;
# else
	if (kernel_locks->global)
		pthread_rwlock_destroy(kernel_locks->global);
	if (kernel_locks->drivers)
		pthread_rwlock_destroy(kernel_locks->drivers);
	urt_shmem_detach(kernel_locks->global);
	urt_shmem_detach(kernel_locks->drivers);
# endif
#endif
}

static void _detach_from_kernel(struct skin_kernel *sk)
{
	urt_shmem_detach(sk);
}

struct skin *(skin_init)(const char *prefix, int *error, ...)
{
	char name[URT_NAME_LEN + 1];
	char *suffix;
	int err;
	bool is_new = false;
	struct skin *skin;

	skin = urt_mem_new(sizeof *skin, &err);
	if (skin == NULL)
		goto exit_no_mem;
	*skin = (struct skin){
#ifdef __KERNEL__
		0,
#else
		.kernel_locks = {
			.fid = -1,
		},
#endif
	};
	if (_get_memories(skin))
		goto exit_no_mem;

	suffix = skin_internal_name_set_prefix(name, prefix, SKIN_CONFIG_DEFAULT_PREFIX);

	/* try creating a new environment, if failed, Skinware is already up, so attach to it */
	skin_internal_name_set_suffix(suffix, "MEM");
	skin->kernel = urt_shmem_new(name, sizeof(struct skin_kernel)
			+ sizeof(struct skin_writer_info[SKIN_CONFIG_MAX_DRIVERS + SKIN_CONFIG_MAX_SERVICES]));
	if (skin->kernel == NULL)
		skin->kernel = urt_shmem_attach(name, &err);
	else
		is_new = true;
	/* if couldn't create or attach, either there is not enough memory, or Skinware was just removed */
	if (skin->kernel == NULL)
		goto exit_no_attach;

	if (is_new)
		_init_kernel(skin->kernel);
	else
		if (_sanity_check_kernel(skin->kernel))
		{
			/*
			 * the layouts between Skinware already being run and the current one trying to attach to it don't match.
			 * This means that one of them has been compiled differently from the other, perhaps with different
			 * compiler options.
			 */
			err = EEXIST;
			goto exit_sanity_failed;
		}

	if ((err = _get_kernel_locks(&skin->kernel_locks, name, suffix, is_new)))
		goto exit_no_kernel_locks;

	if (is_new)
		skin->kernel->initialized = true;

#ifndef __KERNEL__
# ifndef NDEBUG
	skin->log_file = fopen("skin-debug.log", "w");
# endif
#endif

	return skin;
exit_no_kernel_locks:
	if (is_new)
		skin->kernel->initialization_failed = true;
	_detach_from_kernel_locks(&skin->kernel_locks);
exit_sanity_failed:
	_detach_from_kernel(skin->kernel);
exit_no_attach:
	urt_mem_delete(skin);
exit_no_mem:
	if (skin)
		_free_memories(skin);
	if (error)
		*error = err;
	return NULL;
}
URT_EXPORT_SYMBOL(skin_init);

void skin_free(struct skin *skin)
{
	if (skin == NULL)
		return;

	skin_unload(skin);

	_free_memories(skin);
	_detach_from_kernel_locks(&skin->kernel_locks);
	_detach_from_kernel(skin->kernel);

#ifndef __KERNEL__
# ifndef NDEBUG
	if (skin->log_file)
		fclose(skin->log_file);
# endif
#endif

	urt_mem_delete(skin);
}
URT_EXPORT_SYMBOL(skin_free);

#define DEFINE_OBJECT_COUNT_FUNC(object)				\
static int _##object##_count(struct skin_##object *obj, void *d)	\
{									\
	++*(size_t *)d;							\
	return 0;							\
}									\
size_t skin_##object##_count(struct skin *skin)				\
{									\
	size_t res = 0;							\
	skin_for_each_##object(skin, _##object##_count, &res);		\
	return res;							\
}									\
URT_EXPORT_SYMBOL(skin_##object##_count);

#define DEFINE_ENTITY_COUNT_FUNC(entity)				\
static int _##entity##_count(struct skin_user *user, void *d)		\
{									\
	*(skin_##entity##_size *)d += skin_user_##entity##_count(user);	\
	return 0;							\
}									\
skin_##entity##_size skin_##entity##_count(struct skin *skin)		\
{									\
	skin_##entity##_size res = 0;					\
	skin_for_each_user(skin, _##entity##_count, &res);		\
	return res;							\
}									\
URT_EXPORT_SYMBOL(skin_##entity##_count);

DEFINE_OBJECT_COUNT_FUNC(writer)
DEFINE_OBJECT_COUNT_FUNC(reader)
DEFINE_OBJECT_COUNT_FUNC(driver)
DEFINE_OBJECT_COUNT_FUNC(user)

DEFINE_ENTITY_COUNT_FUNC(sensor)
DEFINE_ENTITY_COUNT_FUNC(module)
DEFINE_ENTITY_COUNT_FUNC(patch)

static int _sensor_type_count(struct skin_sensor_type *st, void *d)
{
	++*(skin_sensor_type_size *)d;
	return 0;
}

skin_sensor_type_size skin_sensor_type_count(struct skin *skin)
{
	skin_sensor_type_size res = 0;
	skin_for_each_sensor_type(skin, _sensor_type_count, &res);
	return res;
}
URT_EXPORT_SYMBOL(skin_sensor_type_count);
