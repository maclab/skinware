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

#ifndef SKIN_OBJECT_H
#define SKIN_OBJECT_H

#include "skin_types.h"
#include "skin_callbacks.h"
#include "skin_hooks.h"
#include "skin_writer.h"
#include "skin_reader.h"
#include "skin_driver.h"
#include "skin_user.h"

struct skin;

URT_DECL_BEGIN

/*
 * skin is the main data structure of the skin.  It handles all requests for creating and accessing
 * drivers, services etc.  The services and drivers have similar interfaces.  There is however
 * a simpler, more abstract interface for driver users as well.  Services and drivers result in writer,
 * reader, driver or user objects to be returned.  These objects are used to control the services and drivers.
 *
 * A typical user on the other hand, most likely wants to attach to all drivers and more importantly be
 * independent of knowing which exist or not.  So a simple load/unload/update interface is created for users
 * to attach to the skin and run with minimal effort.
 *
 * Initialization and termination:
 * init				initialize the skin.  If Skinware is already running, it will attach itself to it,
 *				otherwise it will create a new Skinware environment.
 *				If the optional argument name is given, then the Skinware shared resources are
 *				created with names with that name as prefix.  At most URT_NAME_LEN - 3 characters
 *				are taken from the name.  If NULL, the configured default is used.
 * free				release resources and make invalid
 *
 * Service management:
 *   Writer side:
 * service_add			add a service writer to skin kernel.  All fields of the skin_writer_attr given must be filled.
 *				The writer functionality is through skin_writer_* functions that are given the handle
 *				that is returned by this function.  task_attr contains the real-time task attributes.
 *				The type of writer is determined from its task_attr.  If period is set, it becomes
 *				periodic, otherwise it becomes sporadic.
 *				If the service (identified by it's name attribute) is removed but still has readers attached
 *				to it, and it tries to revive itself, it will be successful but only if the rest of the attributes
 *				are identical.  In that case, if `error` is given, it will be set to EALREADY.
 * service_remove		stop a service writer and remove it from skin kernel, unless there are still readers attached to it.
 *   Reader side:
 * service_attach		attach a service reader to a writer.  All fields of the skin_reader_attr given must be filled.
 *				task_attr contains the real-time task attributes.  The type of reader is determined from
 *				its task_attr.  If period is set, it becomes periodic, otherwise if soft is not set it
 *				becomes sporadic, otherwise it will be soft real-time and triggered by writer.
 * service_detach		stop a service reader and detach it from the writer.
 *
 * Driver management:
 *   Driver side:
 * driver_add			add a driver writer to skin kernel.  All fields of the skin_driver_attr given must be filled.
 *				The other parameters are similar to service_add.  The buffer_size attribute of writer is unnecessary
 *				as it will get recomputed from driver attributes.  The driver functionality is through
 *				skin_driver_* functions that are given the handle that is returned by the function, and it's
 *				writer's functionality is through skin_writer_* functions given the handle returned from
 *				skin_driver_get_writer.  Similar to skin_service_add, if driver is revived and `error` is provided,
 *				it will be set to EALREADY.
 * driver_remove		stop a driver writer and remove it from skin kernel, unless there are still readers attached to it.
 *
 *   User side:
 * driver_attach		attach to a single driver.  This function can be used to not only have different acquisition modes
 *				for different drivers, but also to be able to have a custom function on read (rather than sensor data copy).
 *				If the reader attribute has an empty name, then a new driver that supports a sensor type as given in the
 *				user attribute is attached to.  Otherwise, the sensor type given in user attribute is ignored because the
 *				target driver is immediately identified by the reader attribute.
 * driver_detach		detach from a driver.
 * driver_update		update by detaching from a driver if it is removed or attaching to it if it is created/revived.
 *				Similar to driver_attach, this function can be used to create a specific acquisition mode and have a custom
 *				function on read in case a new reader is created.
 *
 * User management:
 * 				NOTE: the following functions attaching and detaching to drivers en-mass assume mutual exclusion
 *				from all attach/detach functions.  If you happen to be attaching/detaching in different threads
 *				while also attaching/detaching en-masse (which doesn't have much logical sense), you need to
 *				use a mutex.
 * load				load the skin by attaching to all currently functional drivers.  All readers are created
 *				similarly, i.e. with the same urt_task attribute.  Furthermore, the skin data is automatically
 *				copied from the drivers.
 * unload			unload the skin by detaching from all drivers.
 * update			update the skin by detaching from removed drivers and attaching to new/revived ones.
 *				Similar to load, newly created readers are created similarly and automatically copy data from drivers.
 *				If a driver is already attached but uses a different task_attr, its reader is recreated.
 *
 * Running:
 * pause			pause all writers and readers of the skin, from both services and drivers.
 * resume			resume all writers and readers of the skin, from both services and drivers.
 * request			request all sporadic users of skin for one read.
 *
 * Info:
 * *_count			return number of objects and entities.
 *
 * Access:
 * for_each_*			iterators over writers, readers, drivers, users, sensors, modules and patches.
 *				The return value is 0 if all were iterated and non-zero if the callback had terminated
 *				the iteration prematurely, or if there was an error.
 * for_each_sensor_of_type	iterate over sensors of a given type only.  The return value and behavior is similar
 *				to other for_each functions
 *
 * Hooks:
 * set_*_*_hook			set a hook to be called on event for object and data.  The objects are writers, readers, drivers,
 *				users, sensors, modules and patches, while the events are init and clean.  When a new
 *				object is created, the init hook is called for it.  Before deleting an object, the clean
 *				hook is called for it.
 */
/* a helper macro is used for `, ##__VA_ARGS__` to work */
#define skin_init(...) skin_init_(unused, ##__VA_ARGS__, NULL, NULL)
#define skin_init_(d, ...) skin_init(__VA_ARGS__)
URT_ATTR_WARN_UNUSED struct skin *(skin_init)(const char *name, int *error, ...);
void skin_free(struct skin *skin);

/* services */

#define skin_service_add(...) skin_service_add(__VA_ARGS__, NULL)
struct skin_writer *(skin_service_add)(struct skin *skin, const struct skin_writer_attr *attr,
		const urt_task_attr *task_attr, const struct skin_writer_callbacks *callbacks, int *error, ...);
void skin_service_remove(struct skin_writer *writer);

#define skin_service_attach(...) skin_service_attach(__VA_ARGS__, NULL)
struct skin_reader *(skin_service_attach)(struct skin *skin, const struct skin_reader_attr *attr,
		const urt_task_attr *task_attr, const struct skin_reader_callbacks *callbacks, int *error, ...);
void skin_service_detach(struct skin_reader *reader);

/* drivers */

#define skin_driver_add(...) skin_driver_add(__VA_ARGS__, NULL)
struct skin_driver *(skin_driver_add)(struct skin *skin, const struct skin_driver_attr *attr,
		const struct skin_writer_attr *writer_attr, const urt_task_attr *task_attr,
		const struct skin_driver_callbacks *callbacks, int *error, ...);
void skin_driver_remove(struct skin_driver *driver);

#define skin_driver_attach(...) skin_driver_attach(__VA_ARGS__, NULL)
struct skin_user *(skin_driver_attach)(struct skin *skin, const struct skin_user_attr *attr,
		const struct skin_reader_attr *reader_attr, const urt_task_attr *task_attr,
		const struct skin_user_callbacks *callbacks, int *error, ...);
void skin_driver_detach(struct skin_user *user);

/* helpers */

int skin_load(struct skin *skin, const urt_task_attr *task_attr);
void skin_unload(struct skin *skin);
int skin_update(struct skin *skin, const urt_task_attr *task_attr);

void skin_pause(struct skin *skin);
void skin_resume(struct skin *skin);

#define skin_request(...) skin_request(__VA_ARGS__, NULL)
void (skin_request)(struct skin *skin, volatile sig_atomic_t *stop, ...);

/* info */

size_t skin_writer_count(struct skin *skin);
size_t skin_reader_count(struct skin *skin);
size_t skin_driver_count(struct skin *skin);
size_t skin_user_count(struct skin *skin);
skin_sensor_size skin_sensor_count(struct skin *skin);
skin_module_size skin_module_count(struct skin *skin);
skin_patch_size skin_patch_count(struct skin *skin);
skin_sensor_type_size skin_sensor_type_count(struct skin *skin);

/* object access */

#define skin_for_each_writer(...) skin_for_each_writer(__VA_ARGS__, NULL)
#define skin_for_each_reader(...) skin_for_each_reader(__VA_ARGS__, NULL)
#define skin_for_each_driver(...) skin_for_each_driver(__VA_ARGS__, NULL)
#define skin_for_each_user(...) skin_for_each_user(__VA_ARGS__, NULL)
int (skin_for_each_writer)(struct skin *skin, skin_callback_writer callback, void *user_data, ...);
int (skin_for_each_reader)(struct skin *skin, skin_callback_reader callback, void *user_data, ...);
int (skin_for_each_driver)(struct skin *skin, skin_callback_driver callback, void *user_data, ...);
int (skin_for_each_user)(struct skin *skin, skin_callback_user callback, void *user_data, ...);

/* data access */

#define skin_for_each_sensor(...) skin_for_each_sensor(__VA_ARGS__, NULL)
#define skin_for_each_module(...) skin_for_each_module(__VA_ARGS__, NULL)
#define skin_for_each_patch(...) skin_for_each_patch(__VA_ARGS__, NULL)
#define skin_for_each_sensor_type(...) skin_for_each_sensor_type(__VA_ARGS__, NULL)
int (skin_for_each_sensor)(struct skin *skin, skin_callback_sensor callback, void *user_data, ...);
int (skin_for_each_module)(struct skin *skin, skin_callback_module callback, void *user_data, ...);
int (skin_for_each_patch)(struct skin *skin, skin_callback_patch callback, void *user_data, ...);
int (skin_for_each_sensor_type)(struct skin *skin, skin_callback_sensor_type callback, void *user_data, ...);

#define skin_for_each_sensor_of_type(...) skin_for_each_sensor_of_type(__VA_ARGS__, NULL)
int (skin_for_each_sensor_of_type)(struct skin *skin, skin_sensor_type_id type,
		skin_callback_sensor callback, void *user_data, ...);

/* hooks */

#define skin_set_writer_init_hook(...) skin_set_writer_init_hook(__VA_ARGS__, NULL)
#define skin_set_writer_clean_hook(...) skin_set_writer_clean_hook(__VA_ARGS__, NULL)
#define skin_set_reader_init_hook(...) skin_set_reader_init_hook(__VA_ARGS__, NULL)
#define skin_set_reader_clean_hook(...) skin_set_reader_clean_hook(__VA_ARGS__, NULL)
#define skin_set_driver_init_hook(...) skin_set_driver_init_hook(__VA_ARGS__, NULL)
#define skin_set_driver_clean_hook(...) skin_set_driver_clean_hook(__VA_ARGS__, NULL)
#define skin_set_user_init_hook(...) skin_set_user_init_hook(__VA_ARGS__, NULL)
#define skin_set_user_clean_hook(...) skin_set_user_clean_hook(__VA_ARGS__, NULL)
#define skin_set_sensor_init_hook(...) skin_set_sensor_init_hook(__VA_ARGS__, NULL)
#define skin_set_sensor_clean_hook(...) skin_set_sensor_clean_hook(__VA_ARGS__, NULL)
#define skin_set_module_init_hook(...) skin_set_module_init_hook(__VA_ARGS__, NULL)
#define skin_set_module_clean_hook(...) skin_set_module_clean_hook(__VA_ARGS__, NULL)
#define skin_set_patch_init_hook(...) skin_set_patch_init_hook(__VA_ARGS__, NULL)
#define skin_set_patch_clean_hook(...) skin_set_patch_clean_hook(__VA_ARGS__, NULL)
void (skin_set_writer_init_hook)(struct skin *skin, skin_hook_writer hook, void *user_data, ...);
void (skin_set_writer_clean_hook)(struct skin *skin, skin_hook_writer hook, void *user_data, ...);
void (skin_set_reader_init_hook)(struct skin *skin, skin_hook_reader hook, void *user_data, ...);
void (skin_set_reader_clean_hook)(struct skin *skin, skin_hook_reader hook, void *user_data, ...);
void (skin_set_driver_init_hook)(struct skin *skin, skin_hook_driver hook, void *user_data, ...);
void (skin_set_driver_clean_hook)(struct skin *skin, skin_hook_driver hook, void *user_data, ...);
void (skin_set_user_init_hook)(struct skin *skin, skin_hook_user hook, void *user_data, ...);
void (skin_set_user_clean_hook)(struct skin *skin, skin_hook_user hook, void *user_data, ...);
void (skin_set_sensor_init_hook)(struct skin *skin, skin_hook_sensor hook, void *user_data, ...);
void (skin_set_sensor_clean_hook)(struct skin *skin, skin_hook_sensor hook, void *user_data, ...);
void (skin_set_module_init_hook)(struct skin *skin, skin_hook_module hook, void *user_data, ...);
void (skin_set_module_clean_hook)(struct skin *skin, skin_hook_module hook, void *user_data, ...);
void (skin_set_patch_init_hook)(struct skin *skin, skin_hook_patch hook, void *user_data, ...);
void (skin_set_patch_clean_hook)(struct skin *skin, skin_hook_patch hook, void *user_data, ...);

/* internal functions for tools */
void skin_internal_print_info(struct skin *skin);

URT_DECL_END

#endif
