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

#ifndef SKIN_MOTION_H
#define SKIN_MOTION_H

#ifdef __KERNEL__
#include <skink.h>
#else
#include <skin.h>
#endif

/*
 * For now, only one motion per layer is implemented.  In the array of service results,
 * element i is the respective motion in layer i.
 */
typedef struct skin_motion
{
	int64_t			from[3],		// starting point of motion (in nanometers)
				to[3];			// end point of motion (in nanometers)
	char			detected;		// if 1, then there is motion.  If 0, then there is no motion
} skin_motion;

#define					SKIN_MOTION_SHMEM				"SKNMSM"
#define					SKIN_MOTION_RWLOCK				"SKNMSL"

#endif
