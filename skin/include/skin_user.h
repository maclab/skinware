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

#ifndef SKIN_USER_H
#define SKIN_USER_H

#include "skin_types.h"
#include "skin_callbacks.h"
#include "skin_reader.h"

URT_DECL_BEGIN

struct skin_user;

struct skin_user_attr
{
	skin_sensor_type_id sensor_type;	/*
						 * type of a sensor handled by a driver.  If multiple drivers
						 * provide sensors of the same type, the skin_driver_attach
						 * function can still be called which will attach to another driver
						 * providing sensors of this sensor type but which has not yet been
						 * attached to.  Once skin_driver_attach returns zero, there are no
						 * more drivers that provide sensor types of given name.
						 *
						 * This is only used if the reader attribute is "".  Otherwise that
						 * attribute directly identifies a single driver.
						 */
};

struct skin_user_callbacks
{
	void (*peek)(struct skin_user *user, skin_sensor_response *responses, skin_sensor_size sensor_count, void *user_data);
						/*
						 * a function that will be called by the reader thread.
						 * It should read in the sensor responses.
						 *
						 * If NULL, the default function will be used, which simply
						 * copies sensor responses to their respective skin_sensor
						 * structure.
						 */
	void (*init)(struct skin_user *user, void *user_data);
						/* a function to be called after a user is created */
	void (*clean)(struct skin_user *user, void *user_data);
						/* a function to be called before a user is removed */
	void (*reader_init)(struct skin_reader *reader, void *user_data);
						/* a function to be called after a user's reader is created */
	void (*reader_clean)(struct skin_reader *reader, void *user_data);
						/* a function to be called before a user's reader is removed */
	void *user_data;
};

/*
 * users are created and removed with skin_driver_attach/detach.  They are controlled with
 * skin_user_* functions.  The reader associated with the user is controlled by
 * skin_reader_* functions, taken the handle returned from skin_user_get_reader.
 *
 * pause			pause the user's reader.
 * resume			resume the user's reader.  A user that has been just created is initially
 *				in a paused state and should be resumed to actually start working.
 * is_paused			whether user's reader is paused
 * is_active			whether driver this user is attached to is still active
 *
 * get_reader			get reader associated with user
 *
 * *_count			number of sensors, modules and patches
 * for_each_*			iterators over sensors, modules and patches.  The return value is 0 if all were
 *				iterated and non-zero if the callback had terminated the iteration prematurely,
 *				or if there was an error.
 * for_each_sensor_of_type	iterate over sensors of a given type only.  The return value and behavior is similar
 *				to other for_each functions
 */
struct skin_reader *skin_user_get_reader(struct skin_user *user);
URT_INLINE int skin_user_pause(struct skin_user *user) { return skin_reader_pause(skin_user_get_reader(user)); }
URT_INLINE int skin_user_resume(struct skin_user *user) { return skin_reader_resume(skin_user_get_reader(user)); }
URT_INLINE bool skin_user_is_paused(struct skin_user *user) { return skin_reader_is_paused(skin_user_get_reader(user)); }
bool skin_user_is_active(struct skin_user *user);

skin_sensor_size skin_user_sensor_count(struct skin_user *user);
skin_module_size skin_user_module_count(struct skin_user *user);
skin_patch_size skin_user_patch_count(struct skin_user *user);
skin_sensor_type_size skin_user_sensor_type_count(struct skin_user *user);

#define skin_user_for_each_sensor(...) skin_user_for_each_sensor(__VA_ARGS__, NULL)
#define skin_user_for_each_module(...) skin_user_for_each_module(__VA_ARGS__, NULL)
#define skin_user_for_each_patch(...) skin_user_for_each_patch(__VA_ARGS__, NULL)
#define skin_user_for_each_sensor_type(...) skin_user_for_each_sensor_type(__VA_ARGS__, NULL)
int (skin_user_for_each_sensor)(struct skin_user *user, skin_callback_sensor callback, void *user_data, ...);
int (skin_user_for_each_module)(struct skin_user *user, skin_callback_module callback, void *user_data, ...);
int (skin_user_for_each_patch)(struct skin_user *user, skin_callback_patch callback, void *user_data, ...);
int (skin_user_for_each_sensor_type)(struct skin_user *user, skin_callback_sensor_type callback, void *user_data, ...);

#define skin_user_for_each_sensor_of_type(...) skin_user_for_each_sensor_of_type(__VA_ARGS__, NULL)
int (skin_user_for_each_sensor_of_type)(struct skin_user *user, skin_sensor_type_id type,
		skin_callback_sensor callback, void *user_data, ...);

URT_DECL_END

#endif
