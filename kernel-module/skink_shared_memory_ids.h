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

#ifndef SKINK_SHARED_MEMORY_IDS_H
#define SKINK_SHARED_MEMORY_IDS_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define					SKINK_RT_SHARED_MEMORY_NAME_SENSOR_LAYERS	"SKNSL"
#define					SKINK_RT_SHARED_MEMORY_NAME_SENSORS		"SKNS"
#define					SKINK_RT_SHARED_MEMORY_NAME_SUB_REGIONS		"SKNSR"
#define					SKINK_RT_SHARED_MEMORY_NAME_REGIONS		"SKNR"
#define					SKINK_RT_SHARED_MEMORY_NAME_MODULES		"SKNM"
#define					SKINK_RT_SHARED_MEMORY_NAME_PATCHES		"SKNP"
#define					SKINK_RT_SHARED_MEMORY_NAME_SUB_REGION_INDICES	"SKNSRI"
#define					SKINK_RT_SHARED_MEMORY_NAME_REGION_INDICES	"SKNRI"
#define					SKINK_RT_SHARED_MEMORY_NAME_SENSOR_NEIGHBORS	"SKNSN"
#define					SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS	"SKNX"
#define					SKINK_RT_DATA_RWL_NAME				"SKNL00"

// To get the lock name for data_buffer[buffer] in sensor_layer[layer], call this function and after, name shall contain the desired rwlock name
// IMPORTANT NOTE: For now, the name would be 'S' 'K' 'N' 'L' (layer) (buffer) where (i) is
// i = 0..9 -> '0'..'9'
// i = 10..35 -> 'A'..'Z'
// i = 36 -> '_'
// otherwise -> '$'
// This means that with this method, you can't have more than 37 layers of data (and 37 buffers for each, but that is highly unlikely)
static inline void skink_get_rwlock_name(char *name, uint32_t layer, uint32_t buffer)
{
	name[0] = SKINK_RT_DATA_RWL_NAME[0];
	name[1] = SKINK_RT_DATA_RWL_NAME[1];
	name[2] = SKINK_RT_DATA_RWL_NAME[2];
	name[3] = SKINK_RT_DATA_RWL_NAME[3];
	if (layer < 10)		name[4] = '0'+layer;
	else if (layer < 36)	name[4] = 'A'+layer-10;
	else if (layer == 36)	name[4] = '_';
	else			name[4] = '$';
	if (buffer < 10)	name[5] = '0'+buffer;
	else if (buffer < 36)	name[5] = 'A'+buffer-10;
	else if (buffer == 36)	name[5] = '_';
	else			name[5] = '$';
	name[6] = '\0';
}

// The following are indices to the shared misc memory (with name SKINK_RT_SHARED_MEMORY_NAME_MISCELLANEOUS) to access data which are:
// booleans indicating whether the main module is loaded/initialized properly/in calibration phase/in regionalizing phase
// number of sensor types, sensor layers array size
// sensors array size and sensor neighbors array size
// sub-regions array size
// regions array size
// modules array size
// patches array size
// sub-region indices array size
// region indices array size
// sensor neighbors array size
// flags to guide the calibrator and regionalizer programs
// sizeofs of structures.  The top layer compares its own sizeofs against the kernels to make sure they have the same size
// All of these data are of type skink_misc_datum, though!
#define					SKINK_MISC_MEMORY_SIZE				30	// as in number of elements, not bytes
#define					SKINK_MODULE_LOADED_INDEX			0
#define					SKINK_INITIALIZED_INDEX				1
#define					SKINK_INITIALIZATION_FAILED_INDEX		2
#define					SKINK_SENSOR_LAYERS_COUNT_INDEX			3
#define					SKINK_SENSORS_COUNT_INDEX			4
#define					SKINK_SUB_REGIONS_COUNT_INDEX			5
#define					SKINK_REGIONS_COUNT_INDEX			6
#define					SKINK_MODULES_COUNT_INDEX			7
#define					SKINK_PATCHES_COUNT_INDEX			8
#define					SKINK_SUB_REGION_INDICES_COUNT_INDEX		9
#define					SKINK_REGION_INDICES_COUNT_INDEX		9	// region_indices and sub-region_indices have the same size
#define					SKINK_SENSOR_NEIGHBORS_COUNT_INDEX		10

// Setup phase flags for calibrator and regionalizer
#define					SKINK_CALIBRATION_POSSIBLE_INDEX		11
#define					SKINK_CALIBRATION_TOO_EARLY_INDEX		12
#define					SKINK_REGIONALIZATION_POSSIBLE_INDEX		13
#define					SKINK_REGIONALIZATION_TOO_EARLY_INDEX		14
#define					SKINK_REGIONALIZATION_SYNC_INDEX		15

// For debug purposes:
#define					SKINK_REQUEST_INDEX				16
#define					SKINK_RESPONSE_INDEX				17

// The following are to ensure data structures have same size in kernel and user space
// Otherwise the shared arrays would be looked at differently!
#define					SKINK_SIZEOF_SENSOR_LAYER_INDEX			18
#define					SKINK_SIZEOF_SENSOR_INDEX			19
#define					SKINK_SIZEOF_MODULE_INDEX			20
#define					SKINK_SIZEOF_PATCH_INDEX			21
#define					SKINK_SIZEOF_SUB_REGION_INDEX			22
#define					SKINK_SIZEOF_REGION_INDEX			23

#endif
