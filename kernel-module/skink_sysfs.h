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

#ifndef SKINK_SYSFS_H
#define SKINK_SYSFS_H

void skink_sysfs_init(void);		// create sysfs objects such as phase or registered_devices
void skink_sysfs_post_devs(void);	// create sysfs files that depend on the data, so they are created after "devs" command is received
void skink_sysfs_post_rt(void);		// create sysfs files that depend on real-time subsystem being initialized such current_time
void skink_sysfs_remove(void);		// remove all sysfs files

#endif
