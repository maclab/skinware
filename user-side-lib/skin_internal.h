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

#ifndef SKIN_INTERNAL_H
#define SKIN_INTERNAL_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_config.h"
#include "skin_common.h"
#include "skin_log.h"

/*
 * here go all definitions, such as macros, functions etc that are to be used internal
 * to the library, without getting exposed to the user.
 */

/*
 * internal functions of skin_reader
 *
 * initialize			called by skin_object.  Returns SKIN_SUCCESS, SKIN_FAIL or SKIN_NO_MEM
 * terminate			called skin_object.  Terminates all reader threads
 */
struct skin_reader;
int skin_reader_internal_initialize(struct skin_reader *reader, skin_sensor_type_size task_count, bool for_setup);
int skin_reader_internal_terminate(struct skin_reader *reader);

/*
 * internal functions of skin_service_manager
 *
 * initialize			called by skin_object.  Returns SKIN_SUCCESS, SKIN_FAIL or SKIN_NO_MEM
 * terminate			called skin_object.  Terminates all service threads
 */
struct skin_service_manager;
int skin_service_manager_internal_initialize(struct skin_service_manager *sm);
int skin_service_manager_internal_terminate(struct skin_service_manager *sm);


#endif
