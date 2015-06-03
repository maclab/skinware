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

#ifndef SKIN_SENSOR_TYPES_H
#define SKIN_SENSOR_TYPES_H

#include "skin_types.h"

/*
 * this file contains the list of known sensor types.  These are magic numbers.  Currently,
 * the higher 20-bit value is for the vendor and the lower 12-bit value is for the type.
 *
 * Rationale: having the sensor type defined by the drivers were considered (and implemented)
 * but the end result was little to no benefit compared to this approach.  That only
 * complicated the task of driver initialization as well as tracking type ids on different
 * users.
 *
 * With this approach, a driver can define a new type and give it a name simply by picking
 * an unused number and expose the name through an external header.  Newly developed sensor
 * types can be officially added with a patch.  The sensor types are currently 32-bit values,
 * so there is little risk of them running out!
 */

#define SKIN_SENSOR_TYPE_INVALID 0
#define SKIN_SENSOR_TYPE_CYSKIN_TAXEL 0x00001001
#define SKIN_SENSOR_TYPE_CYSKIN_TEMPERATURE 0x00001002
#define SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL 0x00002001
#define SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL 0x00003001

URT_DECL_BEGIN

/* get the name associated with the type id.  If not found, NULL is returned */
const char *skin_get_sensor_type_name(skin_sensor_type_id id);

URT_DECL_END

#endif
