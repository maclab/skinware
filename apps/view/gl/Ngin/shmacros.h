/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of Ngin.
 *
 * Ngin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ngin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ngin.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MACROS_H_BY_CYCLOPS
#define MACROS_H_BY_CYCLOPS

#include <stdio.h>
#include <math.h>

#ifndef NDEBUG
#define SH_DEBUG_MODE
#endif

#ifndef SHMACROS_LOG_PREFIX
// Define this macro before including shmacros.h to have this prefix in the log file. Otherwise, "Unknown Module" will be used.
#warning Define SHMACROS_LOG_PREFIX to a string before including shmacros.h file to have that prefix in the logs.
#define SHMACROS_LOG_PREFIX		"Unknown Module"
#endif

// LOG is an fprintf with i arguments apart from file name and format string, includes \n
#define LOG(x, ...)			fprintf(stderr, SHMACROS_LOG_PREFIX ": " x "\n", ##__VA_ARGS__)

// Vector operations
#define DOT(x, y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define NORM2(x)			DOT((x), (x))
#define NORM(x)				(sqrt(NORM2(x)))
#define CROSS_X(x, y)			((x)[1]*(y)[2]-(x)[2]*(y)[1])
#define CROSS_Y(x, y)			((x)[2]*(y)[0]-(x)[0]*(y)[2])
#define CROSS_Z(x, y)			((x)[0]*(y)[1]-(x)[1]*(y)[0])
#define CROSS(x, y)			(CROSS_X(x,y)),(CROSS_Y(x,y)),(CROSS_Z(x,y))
#define DISTANCE2(x, y)			(((x)[0]-(y)[0])*((x)[0]-(y)[0])\
					+((x)[1]-(y)[1])*((x)[1]-(y)[1])\
					+((x)[2]-(y)[2])*((x)[2]-(y)[2]))
#define DISTANCE(x, y)			(sqrt(DISTANCE2((x), (y))))
#define NORMALIZED_CROSS(x, y, d)	(CROSS_X(x,y))/(d),(CROSS_Y(x,y))/(d),(CROSS_Z(x,y))/(d)

// Scalar operations
#define MAX(x, y)			(((x)>(y))?(x):(y))
#define MIN(x, y)			(((x)<(y))?(x):(y))
#define EQUAL(x, y)			((x)-(y) < 1e-6 && (y)-(x) < 1e-6)
#define IS_ZERO(x)			((x) < 1e-6 && (x) > 1e-6)

#endif
