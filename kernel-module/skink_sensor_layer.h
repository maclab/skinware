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

#ifndef SKINK_SENSOR_LAYER_H
#define SKINK_SENSOR_LAYER_H

#include "skink_common.h"
#include "skink_rt.h"

struct skink_sensor;
struct skink_module;
struct skink_patch;

typedef struct skink_sensor_layer				// Supports multiple buffers, but the buffers are in response
{
	skink_sensor_layer_id		id;			// The id of this layer.  Same as type-code for name
								// could be used to index desired layers of sub-regions
	skink_sensor_layer_id		id_in_device;		// The id of this layer in its device
	struct skink_sensor		*sensors;		// A pointer to the layer sensor data
	struct skink_module		*modules;		// A pointer to the layer module data
	struct skink_patch		*patches;		// A pointer to the layer patch data
	skink_sensor_size		sensors_count;		// number of elements in sensors array
	skink_module_size		modules_count;		// number of elements in modules array
	skink_patch_size		patches_count;		// number of elements in patches array
	skin_rt_time			write_time[SKINK_MAX_BUFFERS];
								// write time of each buffer
	uint8_t				paused;			// for the users and services to know if the layer is paused
	uint8_t				last_written_buffer;	// which buffer is last written to
	uint8_t				buffer_being_written;	// which buffer is being written on.  Top layer can wait on this buffer if up-to-date
	uint8_t 			number_of_buffers;	// the number of buffers these sensor layer has, for example 2 for double buffer
	skin_rt_time			next_predicted_swap;	// time next swap is expected to happen.  Used by top layer for buffer swap protection
	uint32_t			acquisition_rate;	// the rate at which acquisition takes place
	char				name[SKINK_MAX_NAME_LENGTH + 1];
								// name of the sensor type, for example "Taxel"
	skink_sensor_id			*sensor_map;		// this is a map from sensor IDs before they got sorted and after it.  It is for the use
								// of device driver.  This field is passed as parameter to the device driver's acquire
								// function even though the layer is being passed, just to stress the fact that they must use it
} skink_sensor_layer;

#endif
