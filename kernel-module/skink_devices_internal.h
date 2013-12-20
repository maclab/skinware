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

#ifndef SKINK_DEVICES_INTERNAL_H
#define SKINK_DEVICES_INTERNAL_H

#ifdef __KERNEL__

#include "skink_devices.h"
#include "skink_writer_internal.h"

typedef struct skink_device_info
{
	uint8_t				id;		// id of the device
	skink_sensor_layer_size		layers_count;	// number of sensor layers introduced by this device
	skink_sensor_layer		*layers;	// the sensor layers themselves
	skink_writer_data		*writers;	// corresponding acquisition tasks for each layer
	skink_device_details		details;	// details callback for obtaining details of each layer
	skink_device_busy		busy;		// busy callback to tell the driver if the device is busy or not
	skink_device_acquire		acquire;	// acquire callback to actually get the data from the device
	skin_rt_mutex			mutex;		// the mutex used to access the device
	skink_sensor_layer_size		busy_count;	// number of layers that are busy at every instance.  Once this reaches 0, busy(0) is called
	char				name[SKINK_MAX_NAME_LENGTH + 1];
							// name of the device.  This name is used to uniquely identify the device
	enum skink_device_state
	{
					SKINK_DEVICE_STATE_UNUSED		= 0,	// Not used
					SKINK_DEVICE_STATE_INITIALIZING		= 1,	// Still initializing
					SKINK_DEVICE_STATE_INITIALIZED		= 2,	// Initialized, but not yet working
					SKINK_DEVICE_STATE_BAD			= 3,	// Bad device.  Will not be used
					SKINK_DEVICE_STATE_PAUSED		= 4,	// Paused device.  Driver could be `rmmod`ed
					SKINK_DEVICE_STATE_WORKING		= 5	// Working device.  Driver must remain in kernel until busy(0) is called
	}				state;		// state of the device
} skink_device_info;

void skink_initialize_devices(void);
void skink_no_more_devices(void);
int skink_build_structures(void);
void skink_free_devices(void);

#endif /* __KERNEL__ */

#endif
