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

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <skin.h>

using namespace std;

ofstream fout("log.out");

skin_object skin;

string cachefile = "unnamed.cache";

int calibrate()
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

int main(int argc, char *argv[])
{
	bool rebuild = false;
	bool ignore_options = false;
	for (int i = 1; i < argc; ++i)
		if (ignore_options)
			cachefile = argv[i];
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			cout << "Usage: " << argv[0] << " [options] [cachefile]\n\n"
				"Options:\n"
				"--help or -h\n"
				"\tShow this help message\n"
				"--recalibrate or -r\n"
				"\tForce calibration even if cache exists\n"
				"--\n"
				"\tThreat the rest of the arguments as though they are not options.\n\n"
				"If cachefile is not given, the name would default to unnamed.cache\n"
				"If --recalibrate is not given, the program tries to calibrate by using data\n"
				"from cache file. If no cache file exists or cache file produces error, it will\n"
				"proceed with calibration. If multiple cache file names are given, the last one\n"
				"is taken.\n\n"
				"If you want to name your file one that clashes with the calibrator options,\n"
				"use -- to stop threating the rest of the arguments as options. For example:\n\n"
				"\t" << argv[0] << " --recalibrate -- --help\n\n"
				"will use --help as the name of the cache file" << endl << endl;
			return 0;
		}
		else if (strcmp(argv[i], "--recalibrate") == 0 || strcmp(argv[i], "-r") == 0)
			rebuild = true;
		else if (strcmp(argv[i], "--") == 0)
			ignore_options = true;
		else
			cachefile = argv[i];
	skin_object_init(&skin);
	if (!rebuild)
	{
		int ret;
		if ((ret = skin_object_calibration_reload(&skin, cachefile.c_str())) != SKIN_SUCCESS)
		{
			if (ret == SKIN_NO_FILE)
			{
				cout << "Calibration cache does not exist. Proceeding with calibration..." << endl;
				fout << "Calibration cache does not exist. Proceeding with calibration..." << endl;
			}
			else
			{
				cout << "Skin calibration cache reload error: " << skin_object_last_error(&skin) << endl;
				fout << "Skin calibration cache reload error: " << skin_object_last_error(&skin) << endl;
			}
			rebuild = true;
		}
	}
	if (rebuild)
	{
		if (skin_object_calibration_begin(&skin) != SKIN_SUCCESS)
		{
			cout << "Skin calibration initialization error: " << skin_object_last_error(&skin) << endl;
			fout << "Skin calibration initialization error: " << skin_object_last_error(&skin) << endl;
			skin_object_free(&skin);
			return EXIT_FAILURE;
		}
		if (calibrate())
		{
			skin_object_free(&skin);
			return EXIT_FAILURE;
		}
		if (skin_object_calibration_end(&skin, cachefile.c_str()) != SKIN_SUCCESS)
		{
			cout << "Skin calibration error: " << skin_object_last_error(&skin) << endl;
			fout << "Skin calibration error: " << skin_object_last_error(&skin) << endl;
			skin_object_free(&skin);
			return EXIT_FAILURE;
		}
	}
	skin_object_free(&skin);
	return EXIT_SUCCESS;
}
