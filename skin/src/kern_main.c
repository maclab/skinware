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
#include <urt.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include "internal.h"

URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("Skinware");

#if SKIN_CONFIG_SUPPORT_USER_SPACE
/* setup sysfs for user-space rwlock lock/unlock */
static struct kobject *_kobj = NULL;

typedef int (*lock_func_type)(struct skin_kernel_locks *);
static lock_func_type lock_funcs[3][5] = {
	[SKIN_KERNEL_GLOBAL_LOCK] = {
		[SKIN_KERNEL_LOCK_WRITE_LOCK] = skin_internal_global_write_lock,
		[SKIN_KERNEL_LOCK_WRITE_UNLOCK] = skin_internal_global_write_unlock,
		[SKIN_KERNEL_LOCK_READ_LOCK] = skin_internal_global_read_lock,
		[SKIN_KERNEL_LOCK_READ_UNLOCK] = skin_internal_global_read_unlock,
	},
	[SKIN_KERNEL_DRIVER_LOCK] = {
		[SKIN_KERNEL_LOCK_WRITE_LOCK] = skin_internal_driver_write_lock,
		[SKIN_KERNEL_LOCK_WRITE_UNLOCK] = skin_internal_driver_write_unlock,
		[SKIN_KERNEL_LOCK_READ_LOCK] = skin_internal_driver_read_lock,
		[SKIN_KERNEL_LOCK_READ_UNLOCK] = skin_internal_driver_read_unlock,
	},
};

static ssize_t _lock_op(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = -EINVAL;

	if (buf[0] < SKIN_KERNEL_GLOBAL_LOCK || buf[0] > SKIN_KERNEL_DRIVER_LOCK
			|| buf[1] < SKIN_KERNEL_LOCK_WRITE_LOCK || buf[1] > SKIN_KERNEL_LOCK_READ_UNLOCK)
		urt_err("error: bad global lock command %d %d\n", buf[0], buf[1]);
	else
		if (lock_funcs[(unsigned int)buf[0]][(unsigned int)buf[1]](NULL) == 0)
			ret = count;

	return ret;
}

static struct kobj_attribute _lock_attr = __ATTR(global_locks, 0222, NULL, _lock_op);
#endif

/* setup skin kernel locks for other kernel modules or user applications to use */
rwlock_t skin_internal_global_lock;
rwlock_t skin_internal_driver_lock;
URT_EXPORT_SYMBOL(skin_internal_global_lock);
URT_EXPORT_SYMBOL(skin_internal_driver_lock);

static int __init _skin_init(void)
{
	rwlock_init(&skin_internal_global_lock);
	rwlock_init(&skin_internal_driver_lock);

#if SKIN_CONFIG_SUPPORT_USER_SPACE
	/* create sysfs file */
	_kobj = kobject_create_and_add("skin"SKIN_CONFIG_SUFFIX, NULL);
	if (!_kobj)
	{
		urt_err("error: failed to create /sys directory for skin.  User-space applications won't run\n");
		goto no_sysfs;
	}
	if (sysfs_create_file(_kobj, &_lock_attr.attr))
		urt_err("error: could not create /sys file for locks.  User-space applications won't run\n");
no_sysfs:
#endif

	urt_out("Loaded\n");
	return 0;
}

static void __exit _skin_exit(void)
{
#if SKIN_CONFIG_SUPPORT_USER_SPACE
	if (_kobj)
		kobject_put(_kobj);
#endif
	urt_out("Unloaded\n");
}

module_init(_skin_init);
module_exit(_skin_exit);
