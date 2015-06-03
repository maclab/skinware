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

#ifndef DRIVER_INTERNAL_H
#define DRIVER_INTERNAL_H

#include <skin_driver.h>
#include "config.h"

/* data shared with users */
struct skin_driver_info
{
	/* information */
	struct skin_driver_attr attr;
	skin_sensor_type_size sensor_type_count;
	skin_sensor_type_id sensor_types[SKIN_CONFIG_MAX_SENSOR_TYPES];
						/* sensor types handled by this driver */
	unsigned int users_attached;		/* number of users still attached */
	uint16_t writer_index;			/* index to writer_info in skin kernel */
	/* book keeping */
	bool active;				/* whether driver is active */
};

/* internal information on writers */
struct skin_driver
{
	/* acquisition */
	struct skin_driver_callbacks callbacks;
	/* data memory */
	void *data_structure;			/* the data structure of the piece of skin handled by this driver */
	/* references */
	struct skin *skin;			/* reference back to the skin object */
	struct skin_writer *writer;		/* the writer of the driver */
	uint16_t info_index;			/* index to driver_info in skin kernel */
	uint16_t index;				/* index to skin's list of drivers */
};

#endif
