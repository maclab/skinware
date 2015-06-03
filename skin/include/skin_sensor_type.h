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

#ifndef SKIN_SENSOR_TYPE_H
#define SKIN_SENSOR_TYPE_H

#include "skin_types.h"

URT_DECL_BEGIN

struct skin_sensor_type
{
	skin_sensor_type_id	id;		/* id of the type */
	struct skin_user	*user;		/* user it belongs to */
	/*
	 * TODO: what if it belongs to more than one user?  Add skin_sensor_type_for_each_user!
	 * While at it, why not skin_sensor_type_for_each_sensor/module/patch also!
	 */
};

URT_DECL_END

#endif
