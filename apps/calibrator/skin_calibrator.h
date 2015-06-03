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

#ifndef SKIN_CALIBRATOR_H
#define SKIN_CALIBRATOR_H

#include <skin.h>

URT_DECL_BEGIN

struct skin_sensor_calibration_info
{
	int64_t			position_nm[3];		/* 3d position of the sensor (local to robot link reference frame in nanometers) */
	int64_t			orientation_nm[3];	/* 3d orientation of the sensor (local to robot link reference frame in nanometers) */
	uint64_t		radius_nm;		/* radius of sensor in nanometers */
	uint32_t		robot_link;		/* the robot link this sensor is located on */
	bool			calibrated;		/* whether the values here are valid, i.e. the sensor is calibrated */
};

/*
 * the following function can be called given the calibration memory and the skin structure to copy calibration data over.
 * It additionally takes a function that given a sensor, would tell where its calibration data should be placed.  Most likely,
 * the user_data field of the sensor would be set before hand and this function would return a location in that user_data.
 *
 * The function returns 0 if all sensors of skin have been calibrated.  Otherwise, a negative value is returned.
 *
 * Example:
 *
 * struct sensor_additional_info
 * {
 *	struct skin_sensor_calibration_info calib_info;
 *	struct other_additional_info other_info;
 * };
 *
 * static struct skin_sensor_calibration_info *get_calib_info(struct skin_sensor *s, void *)
 * {
 *	return &((struct sensor_additional_info *)s->user_data)->calib_info;
 * }
 *
 * for_each_sensor_set_user_data(skin);
 * calib_service = attach_to_calibration(skin);
 * send_service_request(calib_service);
 *
 * // inside calibration service reader:
 * skin_calibrate(skin, mem, get_calib_info);
 *
 * and now calibration data can be used.
 */
#define skin_calibrate(...) skin_calibrate(__VA_ARGS__, NULL)
int (skin_calibrate)(struct skin *skin, void *calibration_memory,
		struct skin_sensor_calibration_info *(*get_calib_info)(struct skin_sensor *s, void *user_data),
		void *user_data, ...);

URT_DECL_END

#endif
