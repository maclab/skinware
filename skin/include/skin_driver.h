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

#ifndef SKIN_DRIVER_H
#define SKIN_DRIVER_H

#include "skin_types.h"
#include "skin_writer.h"

URT_DECL_BEGIN

struct skin_driver;
struct skin_writer;
struct skin_patch;
struct skin_module;
struct skin_sensor;

struct skin_driver_attr
{
	/* the dimensions of data provided by the driver */
	skin_patch_size patch_count;
	skin_module_size module_count;
	skin_sensor_size sensor_count;
};

struct skin_patch_decl
{
	skin_module_size module_count;
};

struct skin_module_decl
{
	skin_sensor_size sensor_count;
};

struct skin_sensor_decl
{
	skin_sensor_unique_id uid;
	skin_sensor_type_id type;
};

struct skin_driver_details
{
	struct skin_driver_attr overall;
	struct skin_patch_decl *patches;
	struct skin_module_decl *modules;
	struct skin_sensor_decl *sensors;
};

struct skin_driver_callbacks
{
	int (*details)(struct skin_driver *driver, bool revived, struct skin_driver_details *details, void *user_data);
						/*
						 * a function that has two behaviors based on `revived`.
						 * If revived, then details of the skin piece managed by this
						 * driver are already filled.  Therefore, this function gives
						 * a chance to the driver to verify that the structure is
						 * correct and possibly adjust its own internal structures.
						 * If not revived, i.e. the driver has just started, this
						 * function needs to fill in the information on the skin
						 * structure.  The `overall` part of the details in this
						 * case is filled, since that much information is already
						 * provided to the driver.  `patches`, `modules` and `sensors`
						 * point to memory allocated according to that overall
						 * information.
						 */
	int (*acquire)(struct skin_driver *driver, skin_sensor_response *responses, skin_sensor_size sensor_count, void *user_data);
						/*
						 * a function that will be called by the driver's writer thread.
						 * It is given an array of sensors responses that needs to be
						 * filled by this function and the size of that array, even though
						 * the driver should already know this value.
						 */
	void (*init)(struct skin_driver *driver, void *user_data);
						/* a function to be called after a driver is created */
	void (*clean)(struct skin_driver *driver, void *user_data);
						/* a function to be called before a driver is removed */
	void (*writer_init)(struct skin_writer *writer, void *user_data);
						/* a function to be called after a driver's writer is created */
	void (*writer_clean)(struct skin_writer *writer, void *user_data);
						/* a function to be called before a driver's writer is removed */
	void *user_data;
};

/*
 * drivers are created and removed with skin_driver_add/remove.  They are controlled with
 * skin_driver_* functions.  The writer associated with the driver is controlled by
 * skin_writer_* functions, taken the handle returned from skin_driver_get_writer.
 *
 * pause		pause the driver's writer.
 * resume		resume the driver's writer.  A driver that has been just created is initially
 *			in a paused state and should be resumed to actually start working.
 * is_paused		whether driver's writer is paused
 * is_active		whether driver is active
 *
 * get_writer		get writer associated with driver
 * get_attr		get the attributes with which the driver is initialized.
 *
 * copy_last_buffer	copy data of last buffer in current buffer
 */
struct skin_writer *skin_driver_get_writer(struct skin_driver *driver);
URT_INLINE int skin_driver_pause(struct skin_driver *driver) { return skin_writer_pause(skin_driver_get_writer(driver)); }
URT_INLINE int skin_driver_resume(struct skin_driver *driver) { return skin_writer_resume(skin_driver_get_writer(driver)); }
URT_INLINE bool skin_driver_is_paused(struct skin_driver *driver) { return skin_writer_is_paused(skin_driver_get_writer(driver)); }
bool skin_driver_is_active(struct skin_driver *driver);

int skin_driver_get_attr(struct skin_driver *driver, struct skin_driver_attr *attr);

URT_INLINE void skin_driver_copy_last_buffer(struct skin_driver *driver) { skin_writer_copy_last_buffer(skin_driver_get_writer(driver)); }

URT_DECL_END

#endif
