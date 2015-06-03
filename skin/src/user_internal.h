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

#ifndef USER_INTERNAL_H
#define USER_INTERNAL_H

#include <skin_driver.h>
#include <skin_user.h>
#include "config.h"

struct skin_patch;
struct skin_module;
struct skin_sensor;

/* internal information on users */
struct skin_user
{
	/* acquisition */
	struct skin_user_callbacks callbacks;
	/* data memory */
	struct skin_driver_attr driver_attr;	/* attributes of the driver, saved in case driver is removed */
	skin_sensor_type_size sensor_type_count;
	struct
	{
		skin_sensor_type_id type;
		skin_sensor_id first_sensor;
	} sensor_types[SKIN_CONFIG_MAX_SENSOR_TYPES];
						/*
						 * an array of sensor types that also tells what is the index
						 * of the first sensor of each type.
						 */
	void *data_structure;			/*
						 * the data structure of the piece of skin handled
						 * by the driver the user is attached to
						 */
	struct skin_patch *patches;		/* built from the basic data structure taken from the driver, ... */
	struct skin_module *modules;		/* the complete data structure with internal references and ... */
	struct skin_sensor *sensors;		/* computed information ready to be used are stored here ... */
	/* references */
	struct skin *skin;			/* reference back to the skin object */
	struct skin_reader *reader;		/* the reader of the user */
	uint16_t driver_index;			/* index to driver_info in skin kernel */
	uint16_t index;				/* index to skin's list of users */
	/* bookkeeping */
	bool mark_for_removal;			/* helper flag for skin_update */
};

#endif
