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
#include "internal.h"
#ifndef __KERNEL__
# include <pthread.h>
# include <unistd.h>
# include <errno.h>
#endif

#ifdef __KERNEL__
/* in kernel space, the locks are accessible globally */
int skin_internal_global_write_lock(struct skin_kernel_locks *locks)
{
	write_lock(&skin_internal_global_lock);
	return 0;
}

int skin_internal_global_read_lock(struct skin_kernel_locks *locks)
{
	read_lock(&skin_internal_global_lock);
	return 0;
}

int skin_internal_global_write_unlock(struct skin_kernel_locks *locks)
{
	write_unlock(&skin_internal_global_lock);
	return 0;
}

int skin_internal_global_read_unlock(struct skin_kernel_locks *locks)
{
	read_unlock(&skin_internal_global_lock);
	return 0;
}

int skin_internal_driver_write_lock(struct skin_kernel_locks *locks)
{
	write_lock(&skin_internal_driver_lock);
	return 0;
}

int skin_internal_driver_read_lock(struct skin_kernel_locks *locks)
{
	read_lock(&skin_internal_driver_lock);
	return 0;
}

int skin_internal_driver_write_unlock(struct skin_kernel_locks *locks)
{
	write_unlock(&skin_internal_driver_lock);
	return 0;
}

int skin_internal_driver_read_unlock(struct skin_kernel_locks *locks)
{
	read_unlock(&skin_internal_driver_lock);
	return 0;
}
#elif SKIN_CONFIG_SUPPORT_KERNEL_SPACE
/* in user space, if kernel space also exists, do the (un)locking through sysfs file */

/*
 * Note: see note on URT's sysfs_lock definition.  In short, multiple threads of the same
 * application cannot ask for the lock through sysfs, because the kernel would block
 * the whole process, instead of just the calling thread.  Therefore, the writes to the
 * sysfs file among the threads of the same process are made mutually exclusive.
 *
 * This mutex is recursive, because the same thread is allowed to lock both the global
 * and driver locks (for example skin_internal_print_info).
 */
static pthread_mutex_t sysfs_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static inline int _lock_op(int fid, int lock, int op)
{
	char cmd[2] = { lock, op };
	int ret = 0;

	if (op == SKIN_KERNEL_LOCK_WRITE_LOCK || op == SKIN_KERNEL_LOCK_READ_LOCK)
		pthread_mutex_lock(&sysfs_lock);

	if (write(fid, cmd, 2) < 0)
	{
		urt_err("error: skin kernel lock operation (%d %d) error %d: '%s'\n", lock, op, errno, strerror(errno));
		ret = errno;
	}

	if (op == SKIN_KERNEL_LOCK_WRITE_UNLOCK || op == SKIN_KERNEL_LOCK_READ_UNLOCK)
		pthread_mutex_unlock(&sysfs_lock);

	return ret;
}

int skin_internal_global_write_lock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_GLOBAL_LOCK, SKIN_KERNEL_LOCK_WRITE_LOCK);
}

int skin_internal_global_read_lock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_GLOBAL_LOCK, SKIN_KERNEL_LOCK_READ_LOCK);
}

int skin_internal_global_write_unlock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_GLOBAL_LOCK, SKIN_KERNEL_LOCK_WRITE_UNLOCK);
}

int skin_internal_global_read_unlock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_GLOBAL_LOCK, SKIN_KERNEL_LOCK_READ_UNLOCK);
}

int skin_internal_driver_write_lock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_DRIVER_LOCK, SKIN_KERNEL_LOCK_WRITE_LOCK);
}

int skin_internal_driver_read_lock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_DRIVER_LOCK, SKIN_KERNEL_LOCK_READ_LOCK);
}

int skin_internal_driver_write_unlock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_DRIVER_LOCK, SKIN_KERNEL_LOCK_WRITE_UNLOCK);
}

int skin_internal_driver_read_unlock(struct skin_kernel_locks *locks)
{
	return _lock_op(locks->fid, SKIN_KERNEL_DRIVER_LOCK, SKIN_KERNEL_LOCK_READ_UNLOCK);
}
#else
/* in user space, if only user space is supported, use the shared rwlocks */
int skin_internal_global_write_lock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_wrlock(locks->global);
}

int skin_internal_global_read_lock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_rdlock(locks->global);
}

int skin_internal_global_write_unlock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_unlock(locks->global);
}

int skin_internal_global_read_unlock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_unlock(locks->global);
}

int skin_internal_driver_write_lock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_wrlock(locks->drivers);
}

int skin_internal_driver_read_lock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_rdlock(locks->drivers);
}

int skin_internal_driver_write_unlock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_unlock(locks->drivers);
}

int skin_internal_driver_read_unlock(struct skin_kernel_locks *locks)
{
	return pthread_rwlock_unlock(locks->drivers);
}
#endif

void skin_internal_wait_termination(bool *running)
{
#if SKIN_CONFIG_STOP_MIN_WAIT > 0
	urt_time start = urt_get_time();

	while (*running && urt_get_time() - start < SKIN_CONFIG_STOP_MIN_WAIT * (urt_time)1000000)
#else
	while (*running)
#endif
		urt_sleep(SKIN_CONFIG_EVENT_MAX_DELAY);
}

void skin_internal_signal_all_requests(urt_sem *req, urt_sem *res)
{
	unsigned int i, tasks_waiting = 1;	/* we know that at least one task is waiting */

	/* see how many have requested the same read in the meantime */
	while (urt_sem_try_wait(req) == 0)
		++tasks_waiting;

	/* wake up as many as there were requests */
	for (i = 0; i < tasks_waiting; ++i)
		urt_sem_post(res);
}

bool skin_internal_writer_is_active(struct skin *skin, uint16_t writer_index)
{
	struct skin_writer_info *writer_info;
	bool is_active;

	if (skin_internal_global_read_lock(&skin->kernel_locks))
		return false;

	writer_info = &skin->kernel->writers[writer_index];
	is_active = writer_info->active && !writer_info->bad;

	skin_internal_global_read_unlock(&skin->kernel_locks);

	return is_active;
}

bool skin_internal_driver_is_active(struct skin *skin, uint16_t driver_index)
{
	struct skin_driver_info *driver_info;
	struct skin_writer_info *writer_info;
	uint16_t writer_index;
	bool is_active;

	/* check if driver is active */
	if (skin_internal_driver_read_lock(&skin->kernel_locks))
		return false;

	driver_info = &skin->kernel->drivers[driver_index];
	is_active = driver_info->active;

	writer_index = driver_info->writer_index;
	if (writer_index >= skin->kernel->max_writer_count)
		is_active = false;

	skin_internal_driver_read_unlock(&skin->kernel_locks);

	if (!is_active)
		return false;

	/* check if its writer is also in a good condition */
	if (skin_internal_global_read_lock(&skin->kernel_locks))
		return false;

	writer_info = &skin->kernel->writers[writer_index];
	is_active = writer_info->active && !writer_info->bad;

	skin_internal_global_read_unlock(&skin->kernel_locks);

	return is_active;
}
