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

#include "calib_internal.h"
#ifndef __KERNEL__
# include <math.h>
#endif

struct calib_extra_data
{
	struct calib_data *calibration_memory;
	struct skin_sensor_calibration_info *(*get_calib_info)(struct skin_sensor *s, void *user_data);
	void *user_data;
	bool all_calibrated;

	/* fake calibration variables */
	skin_sensor_id current_sensor;
	skin_module_id current_module;
	skin_sensor_id max_sensor_count;
	unsigned int mrow, mcol, srow, scol;
	uint64_t mwidth_nm, mheight_nm;
};

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

static int _calibrate_sensor(struct skin_sensor *s, void *d)
{
	struct calib_extra_data *data = d;
	struct calib_map_element find;
	struct calib_map_element *found;
	struct skin_sensor_calibration_info *calib_info;

	calib_info = data->get_calib_info(s, data->user_data);
	if (calib_info == NULL)
		goto exit_failed;

	/* binary search for info on this sensor */
	find.type = s->type;
	find.uid = s->uid;

	found = bsearch(&find, data->calibration_memory->map, data->calibration_memory->sensor_count,
			sizeof *data->calibration_memory->map, _calib_data_cmp);
	if (found == NULL)
		goto exit_failed;

	/* calibrate the sensor */
	*calib_info = found->info;
	calib_info->calibrated = true;

exit_normal:
	return SKIN_CALLBACK_CONTINUE;
exit_failed:
	data->all_calibrated = false;
	goto exit_normal;
}

static int _max_sensor_count(struct skin_module *m, void *data)
{
	skin_sensor_size *max = data;
	skin_sensor_size cur = skin_module_sensor_count(m);
	if (cur > *max)
		*max = cur;
	return SKIN_CALLBACK_CONTINUE;
}

static void get_rectangle_sizes(unsigned int count, unsigned int *rows, unsigned int *cols)
{
#ifdef __KERNEL__
	*cols = count / 4;
#else
	*cols = sqrt(count * 1.618);
#endif
	if (*cols == 0)
		*cols = 1;

	*rows = count / *cols;
	if (*rows * *cols < count)
		++*rows;
}

static int _fake_sensor(struct skin_sensor *s, void *d)
{
	/*
	 * let's say each sensor has a radius of 0.01 and is spaced at 0.03 center-to-center distance.
	 * Let's also say that the modules have an additional border of 0.03
	 */
#define SENSOR_RADIUS 10000000llu
#define CENTER_TO_CENTER 30000000llu
#define MODULE_BORDER 30000000llu

	struct calib_extra_data *data = d;

	uint64_t mr = data->current_module / data->mcol * data->mheight_nm;
	uint64_t mc = data->current_module % data->mcol * data->mwidth_nm;

	uint64_t sr = mr + data->current_sensor / data->scol * CENTER_TO_CENTER;
	uint64_t sc = mc + data->current_sensor % data->scol * CENTER_TO_CENTER;

	struct skin_sensor_calibration_info *calib_info = data->get_calib_info(s, data->user_data);
	if (calib_info)
		*calib_info = (struct skin_sensor_calibration_info){
			.position_nm[0] = sc,
			.position_nm[1] = sr,
			.position_nm[2] = 0,
			.orientation_nm[0] = 0,
			.orientation_nm[1] = 0,
			.orientation_nm[2] = 1000000000,
			.radius_nm = SENSOR_RADIUS,
			.robot_link = 0,
			.calibrated = true,
		};
	else
		data->all_calibrated = false;

	++data->current_sensor;

	return SKIN_CALLBACK_CONTINUE;
}

static int _fake_module(struct skin_module *m, void *d)
{
	struct calib_extra_data *data = d;

	data->current_sensor = 0;
	skin_module_for_each_sensor(m, _fake_sensor, data);
	++data->current_module;

	return SKIN_CALLBACK_CONTINUE;
}

int (skin_calibrate)(struct skin *skin, void *calibration_memory,
		struct skin_sensor_calibration_info *(*get_calib_info)(struct skin_sensor *s, void *user_data),
		void *user_data, ...)
{
	struct calib_extra_data extra = {
		.calibration_memory = calibration_memory,
		.get_calib_info = get_calib_info,
		.user_data = user_data,
		.all_calibrated = true,
	};

	/* if sensor_count is zero, the calibration is supposed to be faked */
	if (((struct calib_data *)calibration_memory)->sensor_count > 0)
		skin_for_each_sensor(skin, _calibrate_sensor, &extra);
	else
	{
		/*
		 * put the modules on a grid, as close to a rectangle with golden ratio as possible.
		 * For each module, put the sensors on a grid with golden ratio too.  However, for
		 * a better regular pattern, assume the same grid size for all modules (taking the
		 * module with the most number of sensors as reference).  This way, some modules
		 * may lack sensors and leave empty space, but sensor radii and structure should
		 * look more uniform.
		 */
		skin_module_size mcount = skin_module_count(skin);
		skin_sensor_size scount = 0;

		skin_for_each_module(skin, _max_sensor_count, &scount);
		get_rectangle_sizes(mcount, &extra.mrow, &extra.mcol);
		get_rectangle_sizes(scount, &extra.srow, &extra.scol);

		extra.mwidth_nm = extra.scol * CENTER_TO_CENTER + MODULE_BORDER;
		extra.mheight_nm = extra.srow * CENTER_TO_CENTER + MODULE_BORDER;

		skin_for_each_module(skin, _fake_module, &extra);
	}

	return extra.all_calibrated?0:-1;
}
URT_EXPORT_SYMBOL(skin_calibrate);

#ifdef __KERNEL__
URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("Skinware");

static int __init _calib_init(void)
{
	urt_out("Loaded\n");
	return 0;
}

static void __exit _calib_exit(void)
{
	urt_out("Unloaded\n");
}

module_init(_calib_init);
module_exit(_calib_exit);
#endif
