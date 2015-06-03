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

#define URT_LOG_PREFIX "skin_info: "
#include <skin.h>

URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("Skin Information Retriever");

URT_MODULE_PARAM_START()
URT_MODULE_PARAM_END()

struct data
{
	struct skin *skin;
};

static int start(struct data *d);
static void body(struct data *d);
static void stop(struct data *d);

URT_GLUE(start, body, stop, struct data, interrupted, done)

static void cleanup(struct data *d)
{
	skin_free(d->skin);
	urt_exit();
}

static int start(struct data *d)
{
	*d = (struct data){0};

	if (urt_init())
		return EXIT_FAILURE;

	d->skin = skin_init();
	if (d->skin == NULL)
		goto exit_no_skin;

	return 0;
exit_no_skin:
	urt_err("init failed\n");
	cleanup(d);
	return EXIT_FAILURE;
}

static void body(struct data *d)
{
	skin_internal_print_info(d->skin);
	done = 1;
}

static void stop(struct data *d)
{
	cleanup(d);
}
