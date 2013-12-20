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

#ifndef SKIN_KERNEL_H
#define SKIN_KERNEL_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

/*
 * This file exists to provide data types and shared memory names, as well as skin RT interface (currently to RTAI)
 * to user-size LXRT counterpart of the kernel module, which is the user of those data.
 *
 * In case of adding more .h files relevant to the user-side need, include them in this file.
 */

#include <skink_common.h>
#include <skink_rt.h>
#ifndef SKIN_DS_ONLY
# include <skink_sensor_layer.h>
# include <skink_sensor.h>
# include <skink_sub_region.h>
# include <skink_region.h>
# include <skink_module.h>
# include <skink_patch.h>
# include <skink_shared_memory_ids.h>
# include <skink_services.h>
#endif

#endif
