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

#ifndef SKIN_TEST_SERVICE_H
#define SKIN_TEST_SERVICE_H

typedef struct skin_test_result		// at the risk of sounding like a doctor
{
	int asap_periodic;		// each <reader_type>_<service_type> is an integer
	int asap_sporadic;		// that gets incremented by the service.  When the
	int periodic_periodic;		// service function is called, it checks internal
	int periodic_sporadic;		// variables, to see if readers are working.  If
	int sporadic_periodic;		// everything is fine, it increments its
	int sporadic_sporadic;		// corresponding variable here.  Continuous change
	int mixed_periodic;		// of these variables shows the services are alive
	int mixed_sporadic;
} skin_test_result;

#define					SKIN_TEST_SERVICE_SHMEM				"SKNTSM"
#define					SKIN_TEST_SERVICE_RWLOCK			"SKNTSL"

#endif
