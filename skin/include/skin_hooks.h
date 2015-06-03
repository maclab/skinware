/*
 * Copyright (C) 2011-2015  Maclab, DIBRIS, Universita di Genova <info@cyskin.com>
 * Authored by Shahbaz Youssefi <ShabbyX@gmail.com>
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

#ifndef SKIN_HOOKS_H
#define SKIN_HOOKS_H

/*
 * this file contains callback definitions for skin_set_*_*_hook functions.
 *
 * Each of these callbacks gets a pointer to an object, as well as
 * a user provided data.
 */

struct skin_writer;
struct skin_reader;
struct skin_driver;
struct skin_user;

struct skin_sensor;
struct skin_module;
struct skin_patch;

typedef void (*skin_hook_writer)(struct skin_writer *w, void *data);
typedef void (*skin_hook_reader)(struct skin_reader *r, void *data);
typedef void (*skin_hook_driver)(struct skin_driver *d, void *data);
typedef void (*skin_hook_user)(struct skin_user *u, void *data);

typedef void (*skin_hook_sensor)(struct skin_sensor *s, void *data);
typedef void (*skin_hook_module)(struct skin_module *m, void *data);
typedef void (*skin_hook_patch)(struct skin_patch *p, void *data);

#endif
