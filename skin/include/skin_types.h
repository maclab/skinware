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

#ifndef SKIN_TYPES_H
#define SKIN_TYPES_H

#include <urt.h>

#define SKIN_INVALID_ID 0xffffffffu

/* id types */
typedef uint32_t skin_sensor_type_id;
typedef uint32_t skin_sensor_id;
typedef uint32_t skin_module_id;
typedef uint32_t skin_patch_id;
/*
 * note: when checking to see if adding more sensors/modules/etc gets above SKIN_*_MAX,
 * make sure you cast them all to uint64_t to avoid overflow
 */
# define SKIN_SENSOR_TYPE_MAX ((skin_sensor_type_size)0xfffffffe)
# define SKIN_SENSOR_MAX ((skin_sensor_size)0xfffffffe)
# define SKIN_MODULE_MAX ((skin_module_size)0xfffffffe)
# define SKIN_PATCH_MAX ((skin_patch_size)0xfffffffe)

/* size types */
typedef skin_sensor_type_id skin_sensor_type_size;
typedef skin_sensor_id skin_sensor_size;
typedef skin_module_id skin_module_size;
typedef skin_patch_id skin_patch_size;

/* other */
typedef uint64_t skin_sensor_unique_id;
typedef uint16_t skin_sensor_response;

#define SKIN_SENSOR_RESPONSE_MAX ((skin_sensor_response)0xffff)

#endif
