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

#ifndef SKIN_SENSOR_H
#define SKIN_SENSOR_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#include "skin_callbacks.h"

struct skin_object;
struct skin_module;
struct skin_patch;
struct skin_sub_region;
struct skin_region;
struct skin_sensor_type;

SKIN_DECL_BEGIN

typedef struct skin_sensor skin_sensor;

/*
 * skin_sensor is a sensor in the skin.
 *
 * init				initialize to an invalid sensor
 * free				release any resources and make invalid
 * get_response			get the current response of the sensor
 * sensor_type			get a reference to the sensor type the sensor belongs to
 * sub_region			get a reference to the sub-region the sensor belongs to
 * module			get a reference to the module the sensor belongs to
 * patch			get a reference to the patch the sensor belongs to
 * sensor_type			get a reference to the sensor type the sensor belongs to
 * for_each_region		call a callback for each region the sensor belongs to
 */
void skin_sensor_init(skin_sensor *sensor);
void skin_sensor_free(skin_sensor *sensor);
static inline skin_sensor_response skin_sensor_get_response(skin_sensor *sensor);
struct skin_sensor_type *skin_sensor_sensor_type(skin_sensor *sensor);
struct skin_sub_region *skin_sensor_sub_region(skin_sensor *sensor);
struct skin_module *skin_sensor_module(skin_sensor *sensor);
struct skin_patch *skin_sensor_patch(skin_sensor *sensor);
void skin_sensor_for_each_region(skin_sensor *sensor, skin_callback_region c, void *data);

struct skin_sensor
{
	skin_sensor_id			id;				/* id of the sensor itself.  Is equal to its index in the sensor list */
	skin_sub_region_id		sub_region_id;			/* id of sub-region this sensor belongs to */
	skin_module_id			module_id;			/* id of module this sensor belongs to. */
	skin_sensor_type_id		sensor_type_id;			/* id of sensor type this sensor belongs to */
	skin_sensor_response		response;			/* response of the sensor.  Has a value between 0 and 65535 */
	float				relative_position[3];		/* 3d position of the sensor (local to robot_link reference frame) */
	float				relative_orientation[3];	/* normal axis at this sensor.  Since skin module is deformed, could be different for */
									/* every sensor (local to robot_link reference frame).  If non-zero, */
									/* then it is already made normal (norm == 1) */
	float				global_position[3];
	float				global_orientation[3];
	float				flattened_position[2];		/* the position in the flattened image of the skin */
	float				radius;				/* the radius of the sensor */
	skin_sensor_id			*neighbors;			/* list of sensors connected to this neighbor in the flattened image */
	skin_sensor_size		neighbors_count;		/* size of neighbors */
	uint32_t			robot_link;			/* robot link this sensor is located on */

	/* internal */
	struct skin_object		*p_object;

#ifdef SKIN_CPP
	skin_sensor() { skin_sensor_init(this); }
	~skin_sensor() { skin_sensor_free(this); }
	skin_sensor_response get_response() { return skin_sensor_get_response(this); }
	skin_sensor_type *sensor_type() { return skin_sensor_sensor_type(this); }
	skin_sub_region *sub_region() { return skin_sensor_sub_region(this); }
	skin_module *module() { return skin_sensor_module(this); }
	skin_patch *patch() { return skin_sensor_patch(this); }

	class skin_region_iterator
	{
	private:
		skin_region_id current;
		skin_object *object;
		skin_region_iterator(skin_sensor *sensor, skin_region_id c);
	public:
		skin_region_iterator();
		skin_region_iterator &operator ++();
		skin_region_iterator operator ++(int);
		bool operator ==(const skin_region_iterator &rhs) const;
		bool operator !=(const skin_region_iterator &rhs) const;
		skin_region &operator *() const;
		skin_region *operator ->() const;
		operator skin_region *() const;
		friend struct skin_sensor;
	};
	skin_region_iterator regions_iter_begin();
	const skin_region_iterator regions_iter_end();
#endif
};

static inline skin_sensor_response skin_sensor_get_response(skin_sensor *sensor) { return sensor->response; };

SKIN_DECL_END

#endif
