/*
 * Copyright (C) 2015  Maclab, DIBRIS, Universita di Genova <info@cyskin.com>
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

#ifndef __KERNEL__

#include <skin.h>

extern inline int skin_driver_pause(struct skin_driver *driver);
URT_EXPORT_SYMBOL(skin_driver_pause);
extern inline int skin_driver_resume(struct skin_driver *driver);
URT_EXPORT_SYMBOL(skin_driver_resume);
extern inline bool skin_driver_is_paused(struct skin_driver *driver);
URT_EXPORT_SYMBOL(skin_driver_is_paused);
extern inline void skin_driver_copy_last_buffer(struct skin_driver *driver);
URT_EXPORT_SYMBOL(skin_driver_copy_last_buffer);

extern inline int skin_user_pause(struct skin_user *user);
URT_EXPORT_SYMBOL(skin_user_pause);
extern inline int skin_user_resume(struct skin_user *user);
URT_EXPORT_SYMBOL(skin_user_resume);
extern inline bool skin_user_is_paused(struct skin_user *user);
URT_EXPORT_SYMBOL(skin_user_is_paused);

extern inline skin_sensor_response skin_sensor_get_response(struct skin_sensor *s);
URT_EXPORT_SYMBOL(skin_sensor_get_response);

extern inline skin_sensor_size skin_module_sensor_count(struct skin_module *module);
URT_EXPORT_SYMBOL(skin_module_sensor_count);

extern inline skin_module_size skin_patch_module_count(struct skin_patch *patch);
URT_EXPORT_SYMBOL(skin_patch_module_count);

#endif
