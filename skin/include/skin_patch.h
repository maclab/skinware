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

#ifndef SKIN_PATCH_H
#define SKIN_PATCH_H

#include "skin_types.h"
#include "skin_callbacks.h"

URT_DECL_BEGIN

struct skin_sensor;
struct skin_user;

/*
 * *_count			number of sensors and modules
 * for_each_*			iterators over sensors and modules.  The return value is 0 if all were
 *				iterated and non-zero if the callback had terminated the iteration prematurely,
 *				or if there was an error.
 */
struct skin_patch
{
	skin_patch_id		id;		/* index in its user's patch array */
	struct skin_user	*user;		/* user it belongs to */
	struct skin_module	*modules;	/* a pointer to the patch module data */
	skin_module_size	module_count;	/* number of elements in modules array */

	void			*user_data;	/* arbitrary user data */
};

skin_sensor_size skin_patch_sensor_count(struct skin_patch *patch);
URT_INLINE skin_module_size skin_patch_module_count(struct skin_patch *patch) { return patch->module_count; }

#define skin_patch_for_each_sensor(...) skin_patch_for_each_sensor(__VA_ARGS__, NULL)
#define skin_patch_for_each_module(...) skin_patch_for_each_module(__VA_ARGS__, NULL)
int (skin_patch_for_each_sensor)(struct skin_patch *patch, skin_callback_sensor callback, void *user_data, ...);
int (skin_patch_for_each_module)(struct skin_patch *patch, skin_callback_module callback, void *user_data, ...);

URT_DECL_END

#endif
