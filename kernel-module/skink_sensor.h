/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#ifndef SKINK_SENSOR_H
#define SKINK_SENSOR_H

#include "skink_common.h"

typedef struct skink_sensor
{
	skink_sensor_id			id;				// Relative to its layer
	skink_sensor_response		response[SKINK_MAX_BUFFERS];	// possible multiple buffers for sensor responses
	skink_sub_region_id		sub_region;
	skink_module_id			module;				// Relative to its layer
	skink_sensor_layer_id		layer;				// which layer this sensor is in
	int64_t				position_nm[3],			// information provided by the calibrator.  Not used by the kernel module,
					orientation_nm[3],		// but simply there for the user programs to take it.  These values are
					flattened_position_nm[2];	// position and orientation relative to the robot link as well the sensors
									// flattened position
	uint64_t			radius_nm;			// radius of the sensor, assuming it's a circle (all values in nanometer (to avoid float and FPU in kernel))
	uint32_t			neighbors_begin;		// information provided by the calibrator.  [begin, begin+count) in sensor
	skink_sensor_size		neighbors_count;		// neighbors list show the neighbors of of this sensor.  These values are absolute
									// and not relative to layer.
	uint32_t			robot_link;			// information provided by the calibrator.  Not used by the kernel module,
									// but simply there for the user programs to take it
} skink_sensor;

#endif
