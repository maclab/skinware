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

#include "internal.h"
#include <skin_sensor_types.h>

static const char *get_sensor_type_name(skin_sensor_type_id id, char *buffer)
{
	const char *sensor_type = skin_get_sensor_type_name(id);
	if (sensor_type == NULL)
	{
		sprintf(buffer, "<unknown> (%10u)", id);
		sensor_type = buffer;
	}
	return sensor_type;
}

static void print_header(int name_len, int name_print_len, bool empty_before_header)
{
	int s;

	if (empty_before_header)
		urt_out_cont("       |        |\n");

	urt_out_cont(" index |  type  | ");
	for (s = 0; s < name_len / 2 - 2; ++s)
		urt_out_cont(" ");
	urt_out_cont("name");
	for (s = 0; s < name_len - name_len / 2 - 2; ++s)
		urt_out_cont(" ");
	urt_out_cont(" | data (bytes) | buffers |    period    | readers | bad\n");

	urt_out_cont("-------+--------+-");
	for (s = 0; s < name_print_len; ++s)
		urt_out_cont("-");
	urt_out_cont("-+--------------+---------+--------------+---------+-----\n");
}

void skin_internal_print_info(struct skin *skin)
{
	size_t i;
	struct skin_kernel *sk = skin->kernel;
	int name_len = URT_NAME_LEN - 3;
	int name_print_len = 4;
	bool reprint_header = false;

	if (name_len / 2 - 2 > 0)
		name_print_len += name_len / 2 - 2;
	if (name_len - name_len / 2 - 2)
		name_print_len += name_len - name_len / 2 - 2;

	/* lock the kernel */
	if (skin_internal_driver_read_lock(&skin->kernel_locks))
		goto exit_no_drivers_lock;
	if (skin_internal_global_read_lock(&skin->kernel_locks))
		goto exit_no_global_lock;

	print_header(name_len, name_print_len, false);

	/* output information on everything! */
	for (i = 0; i < sk->max_writer_count; ++i)
	{
		struct skin_writer_info *w = &sk->writers[i];
		char period[20];
		bool is_driver;

		if (!w->active && w->readers_attached == 0)
			continue;
		if (w->period)
			sprintf(period, "%12llu", w->period);
		else
			strcpy(period, "  Sporadic  ");
		is_driver = w->driver_index < sk->max_driver_count;

		if (reprint_header)
		{
			reprint_header = false;
			print_header(name_len, name_print_len, true);
		}

		urt_out_cont(" %5zu | %6s | %*s | %12zu | %7u | %12s | %7u | %3s\n",
				i, is_driver?"Driver":"Writer", name_print_len, w->attr.prefix,
				w->attr.buffer_size, w->attr.buffer_count, period,
				w->readers_attached, w->bad?"Yes":"No");

		if (is_driver)
		{
			struct skin_driver_info *d = &sk->drivers[w->driver_index];
			const char *sensor_type = NULL;
			char sensor_type_unknown[30];
			skin_sensor_type_id t;

			urt_out_cont("       |    \\   |\n");
			urt_out_cont("       |     -> |");
			urt_out_cont(" index | users | patches | modules | sensors |       sensor types      \n");

			urt_out_cont("       |        |-------+-------+---------+---------+---------+--------------------------\n");

			if (d->sensor_type_count < 1)
				sensor_type = "<NONE> <-- this is an internal error";
			else
				sensor_type = get_sensor_type_name(d->sensor_types[0], sensor_type_unknown);

			urt_out_cont("       |        | %5u | %5u | %7u | %7u | %7u | %s\n",
					w->driver_index, d->users_attached, d->attr.patch_count,
					d->attr.module_count, d->attr.sensor_count, sensor_type);

			for (t = 1; t < d->sensor_type_count; ++t)
				urt_out_cont("       |        |       |       |         |         |         | %s\n",
						get_sensor_type_name(d->sensor_types[t], sensor_type_unknown));

			/* reprint the header to make it clear what's going on */
			reprint_header = true;
		}
	}

	urt_out_cont("\n");
	urt_out_cont("Max number of drivers: %u\n", sk->max_driver_count);
	urt_out_cont("Max number of writers: %u\n", sk->max_writer_count);

	skin_internal_global_read_unlock(&skin->kernel_locks);
	skin_internal_driver_read_unlock(&skin->kernel_locks);
	return;
exit_no_global_lock:
	skin_internal_driver_read_unlock(&skin->kernel_locks);
exit_no_drivers_lock:
	urt_err("Error getting locks");
}
URT_EXPORT_SYMBOL(skin_internal_print_info);
