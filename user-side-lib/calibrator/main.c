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

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <skin.h>

FILE *fout = NULL;
skin_object skin;
const char *cachefile = "unnamed.cache";

#define log(...)				\
do {						\
	if (fout)				\
		fprintf(fout, __VA_ARGS__);	\
	fprintf(stderr, __VA_ARGS__);		\
} while (0)

int calibrate(void)
{
	/*
	 * To get the sensors of the skin:
	 *	skin_sensor_size scount;
	 *	skin_sensor *sensors = skin.sensors(&scount);
	 * If you are interested in sensors of a single layer, you can similarly get the sensor types and
	 * search for the one you want.  You can see the documentations for that.
	 * To iterate over the sensors, you can either use the for each function:
	 *	skin_object_for_each_sensor(&skin, function, data);
	 * or with the array you got above:
	 *	for (skin_sensor_id i = 0; i < scount; ++i)
	 * Either way, you get the sensor values with:
	 *   with for_each:
	 *	s->get_response()
	 *   or with array:
	 *	sensors[i].get_response()
	 *
	 * What you need to fill in are the following fields in the sensors (I'll write using the array method, but the for_each is similar):
	 *	sensors[i].relative_position[0] = pos_x;
	 *	sensors[i].relative_position[1] = pos_y;
	 *	sensors[i].relative_position[2] = pos_z;
	 *	sensors[i].relative_orientation[0] = norm_x;
	 *	sensors[i].relative_orientation[1] = norm_y;
	 *	sensors[i].relative_orientation[2] = norm_z;
	 *	sensors[i].radius = radius;
	 *	sensors[i].robot_link = id_of_link_this_sensor_is_located_on;
	 *
	 * The position and orientation values are relative to the robot link.  The id you give them should be known to the module that would handle
	 * the kinematics in the future.
	 * Once you have filled these values, simply
	 *	return 0;
	 * If there were errors,
	 *	return -1;
	 */
	return -1;
}

static int get_max_sensor_count(struct skin_module *m, void *data)
{
	skin_sensor_size *max = data;
	skin_sensor_size cur = skin_module_sensors_count(m);
	if (cur > *max)
		*max = cur;
	return SKIN_CALLBACK_CONTINUE;
}

static void get_rectangle_sizes(unsigned int count, unsigned int *rows, unsigned int *cols)
{
	*cols = sqrt(count * 1.618);
	*rows = count / *cols;
	if (*rows * *cols < count)
		++*rows;
}

void auto_generate(void)
{
	skin_module_size mcount;
	skin_module *modules = skin_object_modules(&skin, &mcount);
	skin_sensor_size scount = 0;
	unsigned int mrow, mcol, srow, scol;
	float mwidth, mheight;
	skin_module_id i;
	skin_sensor_id j;

	skin_object_for_each_module(&skin, get_max_sensor_count, &scount);

	/*
	 * put the modules on a grid, as close to a rectangle with golden ratio as possible.
	 * For each module, put the sensors on a grid with golden ratio too.  However, for
	 * a better regular pattern, assume the same grid size for all modules (taking the
	 * module with the most number of sensors as reference).  This way, some modules
	 * may lack sensors and leave empty space, but sensor radii and structure should
	 * look more uniform.
	 */

	get_rectangle_sizes(mcount, &mrow, &mcol);
	get_rectangle_sizes(scount, &srow, &scol);

	/*
	 * let's say each sensor has a radius of 0.01 and is spaced at 0.03 center-to-center distance.
	 * Let's also say that the modules have an additional border of 0.03
	 */
#define SENSOR_RADIUS 0.01f
#define CENTER_TO_CENTER 0.03f
#define MODULE_BORDER 0.03f
	mwidth = scol * CENTER_TO_CENTER + MODULE_BORDER;
	mheight = srow * CENTER_TO_CENTER + MODULE_BORDER;

	for (i = 0; i < mcount; ++i)
	{
		skin_sensor_size mscount;
		skin_sensor *sensors = skin_module_sensors(&modules[i], &mscount);
		float mr = i / mcol * mheight;
		float mc = i % mcol * mwidth;

		for (j = 0; j < mscount; ++j)
		{
			float sr = mr + j / scol * CENTER_TO_CENTER;
			float sc = mc + j % scol * CENTER_TO_CENTER;

			sensors[j].relative_position[0] = sc;
			sensors[j].relative_position[1] = sr;
			sensors[j].relative_position[2] = 0;
			sensors[j].relative_orientation[0] = 0;
			sensors[j].relative_orientation[1] = 0;
			sensors[j].relative_orientation[2] = 1;
			sensors[j].radius = SENSOR_RADIUS;
			sensors[j].robot_link = 0;
		}
	}
}

int main(int argc, char *argv[])
{
	bool rebuild = false;
	bool ignore_options = false;
	bool fake_fill = false;
	int ret = EXIT_FAILURE;
	int i;

	fout = fopen("log.out", "w");

	for (i = 1; i < argc; ++i)
		if (ignore_options)
			cachefile = argv[i];
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			printf("Usage: %s [options] [cachefile]\n\n"
				"Options:\n"
				"--help or -h\n"
				"\tShow this help message\n"
				"--recalibrate or -r\n"
				"\tForce calibration even if cache exists\n"
				"--fake or -f\n"
				"\tGenerate fake positions for current skin configuration\n"
				"--\n"
				"\tThreat the rest of the arguments as though they are not options.\n\n"
				"If cachefile is not given, the name would default to unnamed.cache\n"
				"If --recalibrate is not given, the program tries to calibrate by using data\n"
				"from cache file. If no cache file exists or cache file produces error, it will\n"
				"proceed with calibration. If multiple cache file names are given, the last one\n"
				"is taken.\n\n"
				"If faking calibration values (for test purposes), the calibration data would\n"
				"not be cached.\n\n"
				"If you want to name your file one that clashes with the calibrator options,\n"
				"use -- to stop threating the rest of the arguments as options. For example:\n\n"
				"\t%s --recalibrate -- --help\n\n"
				"will use --help as the name of the cache file\n\n", argv[0], argv[0]);
			return 0;
		}
		else if (strcmp(argv[i], "--recalibrate") == 0 || strcmp(argv[i], "-r") == 0)
			rebuild = true;
		else if (strcmp(argv[i], "--fake") == 0 || strcmp(argv[i], "-f") == 0)
			fake_fill = true;
		else if (strcmp(argv[i], "--") == 0)
			ignore_options = true;
		else
			cachefile = argv[i];
	skin_object_init(&skin);
	if (!rebuild && !fake_fill)
	{
		int ret;
		if ((ret = skin_object_calibration_reload(&skin, cachefile)) != SKIN_SUCCESS)
		{
			if (ret == SKIN_NO_FILE)
				log("Calibration cache does not exist. Proceeding with calibration...\n");
			else
				log("Skin calibration cache reload error: %s\n", skin_object_last_error(&skin));
			rebuild = true;
		}
	}
	if (rebuild || fake_fill)
	{
		if (skin_object_calibration_begin(&skin) != SKIN_SUCCESS)
		{
			log("Skin calibration initialization error: %s\n", skin_object_last_error(&skin));
			goto exit_fail;
		}
		if (!fake_fill)
		{
			if (calibrate())
				goto exit_fail;
		}
		else
			auto_generate();
		if (skin_object_calibration_end(&skin, fake_fill?NULL:cachefile) != SKIN_SUCCESS)
		{
			log("Skin calibration error: %s\n", skin_object_last_error(&skin));
			goto exit_fail;
		}
	}
	ret = EXIT_SUCCESS;
exit_fail:
	skin_object_free(&skin);
	if (fout)
		fclose(fout);
	return ret;
}
