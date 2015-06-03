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

#define URT_LOG_PREFIX "calibrator: "

#include "calib_internal.h"
#include <config.h>
#ifdef __KERNEL__
# if HAVE_KIO_H
#  include <kio.h>
#  define USE_KIO
#  define USE_DATABASE
# endif
#else
# define USE_DATABASE
#endif

/*
 * the calibrator is a program that provides a service where position information of sensors
 * are provided.  The program can either read these information from a database, or fake it
 * for debug purposes.  The information provided include 3d position, 3d orientation,
 * robot link the sensor is located on and sensor radius.
 *
 * The calibrator provides a library file for dealing with the shared memory.
 */

URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("Skin Calibrator");

#ifdef USE_DATABASE
static bool fake_fill = false;
static char *database = NULL;
#endif
static char *name = NULL;

URT_MODULE_PARAM_START()
#ifdef USE_DATABASE
URT_MODULE_PARAM(fake_fill, bool, "Generate fake positions for unknown sensors.  Default value is false")
URT_MODULE_PARAM(database, charp, "Database file where calibration data are located.  Default value is 'skin.calib'")
#endif
URT_MODULE_PARAM(name, charp, "Calibration service name.  Default value is 'CAL'")
URT_MODULE_PARAM_END()

#ifdef USE_KIO
# define FILE KFILE
# define fopen(f, m) kopen(f, m"p")
# define fclose kclose
# define fscanf kscanf
#endif

struct data
{
	struct skin *skin;

	/* sanitized arguments */
	const char *database;

	/* calibration data read from file */
	skin_sensor_size sensor_count;
	struct calib_map_element *calib_data;

#ifdef USE_DATABASE
	/* keep file here so if in kernel and using KIO, it could be asynchronously closed */
	FILE *calib_file;
#endif

	/* book-keeping */
	bool already_copied;
};

static int start(struct data *d);
static void body(struct data *d);
static void stop(struct data *d);

URT_GLUE(start, body, stop, struct data, interrupted, done)

static int fill_memory(struct skin_writer *writer, void *mem, size_t size, void *d)
{
	struct data *data = d;
	struct calib_data *c = mem;

	if (data->already_copied)
		return 0;

	c->sensor_count = data->sensor_count;
	memcpy(c->map, data->calib_data, data->sensor_count * sizeof *data->calib_data);

	data->already_copied = true;

	return 0;
}

static int _calib_data_cmp(const void *d1, const void *d2)
{
	const struct calib_map_element *e1 = d1;
	const struct calib_map_element *e2 = d2;
	int ret;

	ret = (e1->type > e2->type) - (e1->type < e2->type);
	if (ret)
		return ret;

	return (e1->uid > e2->uid) - (e1->uid < e2->uid);
}

#ifdef USE_DATABASE
static int read_calib_data(struct data *d)
{
	unsigned long long sensor_count, i;

	d->calib_file = fopen(d->database, "r");
	if (d->calib_file == NULL)
		return -1;

	if (fscanf(d->calib_file, "%llu", &sensor_count) != 1)
		goto exit_bad_file;

	if (sensor_count == 0)
		goto exit_bad_format;

	d->sensor_count = sensor_count;
	d->calib_data = urt_mem_new(sensor_count * sizeof *d->calib_data);
	if (d->calib_data == NULL)
		goto exit_no_mem;

	for (i = 0; i < sensor_count; ++i)
	{
		unsigned long long uid;
		long long position[3], orientation[3];
		unsigned long long radius;
		unsigned int link;

		struct calib_map_element *e = &d->calib_data[i];

		if (fscanf(d->calib_file, "%u %llu %lld %lld %lld %lld %lld %lld %llu %u",
					&e->type, &uid, &position[0], &position[1], &position[2],
					&orientation[0], &orientation[1], &orientation[2], &radius, &link) != 10)
			goto exit_bad_format;
		e->uid = uid;
		e->info = (struct skin_sensor_calibration_info){
			.position_nm = {
				[0] = position[0],
				[1] = position[1],
				[2] = position[2],
			},
			.orientation_nm = {
				[0] = orientation[0],
				[1] = orientation[1],
				[2] = orientation[2],
			},
			.radius_nm = radius,
			.robot_link = link,
		};
	}

	fclose(d->calib_file);
	d->calib_file = NULL;

	qsort(d->calib_data, d->sensor_count, sizeof *d->calib_data, _calib_data_cmp);

	return 0;
exit_bad_file:
	urt_err("error: unexpected end of file or read error\n");
	goto exit_cleanup;
exit_bad_format:
	urt_err("error: bad file format\n");
	goto exit_cleanup;
exit_no_mem:
	urt_err("error: not enough memory\n");
	goto exit_cleanup;
exit_cleanup:
	d->sensor_count = 0;
	urt_mem_delete(d->calib_data);
	fclose(d->calib_file);
	d->calib_file = NULL;
	return -1;
}
#endif

static void cleanup(struct data *d)
{
	skin_free(d->skin);
#ifdef USE_DATABASE
	if (d->calib_file)
		fclose(d->calib_file);
	d->calib_file = NULL;
#endif
	urt_exit();
}

static int start(struct data *d)
{
	*d = (struct data){0};

	if (urt_init())
		return EXIT_FAILURE;

#ifdef USE_DATABASE
	d->database = database?database:"skin.calib";
#endif

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
#ifdef USE_DATABASE
	if (!fake_fill && read_calib_data(d))
		urt_err("error: failed to read sensor data from database\n");
	else
#endif
		if (skin_service_add(d->skin, &(struct skin_writer_attr){
					.buffer_size = sizeof(struct calib_data) + sizeof(struct calib_map_element[d->sensor_count]),
					.buffer_count = 1,
					.name = name?name:"CAL",
				}, &(urt_task_attr){0},
				&(struct skin_writer_callbacks){ .write = fill_memory, .user_data = d}) == NULL)
			urt_err("error: could not start service\n");

	skin_resume(d->skin);

	done = 1;
}

static void stop(struct data *d)
{
	while (!interrupted)
		urt_sleep(10000000);
	cleanup(d);
}
