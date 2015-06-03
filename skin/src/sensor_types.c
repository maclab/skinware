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

#define URT_LOG_PREFIX "skin: "
#include <skin_sensor_types.h>
#include "internal.h"

struct type_name
{
	skin_sensor_type_id type_id;
	const char *name;
};

/* Note: keep this array sorted on type_id */
static struct type_name names[] = {
	{
		.type_id = SKIN_SENSOR_TYPE_CYSKIN_TAXEL,
		.name = "CySkin Taxel",
	},
	{
		.type_id = SKIN_SENSOR_TYPE_CYSKIN_TEMPERATURE,
		.name = "CySkin Temperature",
	},
	{
		.type_id = SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL,
		.name = "MacLab ROBOSKIN Taxel",
	},
	{
		.type_id = SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL,
		.name = "ROBOSKIN Taxel",
	},
};

#ifndef NDEBUG
/* in debug mode, perform a sanity check on the names array, to make sure it's not messed up */
static void check_names_array(void)
{
	static bool first_time = true;
	size_t i = 0;
	size_t size = sizeof names / sizeof *names;

	if (!first_time)
		return;
	first_time = false;

	if (names[0].type_id == 0 || names[0].name == NULL)
		goto exit_fail;

	for (i = 1; i < size; ++i)
		if (names[i].type_id <= names[i - 1].type_id || names[i].name == NULL)
			goto exit_fail;

	return;
exit_fail:
	internal_error("bad type name map at index %zu\n", i);
}
#endif

static int compare(const void *a, const void *b)
{
	const struct type_name *first = a;
	const struct type_name *second = b;

	return (first->type_id > second->type_id) - (first->type_id < second->type_id);
}

const char *skin_get_sensor_type_name(skin_sensor_type_id id)
{
	struct type_name find = { .type_id = id };
	struct type_name *res;

#ifndef NDEBUG
	check_names_array();
#endif

	res = bsearch(&find, names, sizeof names / sizeof *names, sizeof *names, compare);
	return res?res->name:NULL;
}
URT_EXPORT_SYMBOL(skin_get_sensor_type_name);
