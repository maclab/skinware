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

#ifndef SKINK_DATA_ACQUISITION_H
#define SKINK_DATA_ACQUISITION_H

#ifdef __KERNEL__

#include "skink_common.h"

struct skink_device_info;

typedef struct skink_writer_data
{
	bool				must_stop;		// if 1, task will quit
	bool				running;		// if 1, then task is still running
	bool				paused;			// if 1, then task is paused
	bool				must_pause;		// if 1, then task should pause
	struct skink_device_info	*device;		// the device to which this task relates to
	skink_sensor_layer_id		layer;			// the layer in the device to which this task relates to
	skin_rt_task			task;			// the RT task identifier of this task
	bool				created;		// if 1, then task is created and should be deleted on cleanup
} skink_writer_data;

int skink_initialize_writers(void);
void skink_free_writers(void);
void skink_pause_threads(void);		// pause is blocking (except until the_skink_state changes to EXITING in which case it returns)
void skink_resume_threads(void);

#endif /* __KERNEL__ */

#endif
