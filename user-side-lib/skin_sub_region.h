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

#ifndef SKIN_SUB_REGION_H
#define SKIN_SUB_REGION_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#include "skin_callbacks.h"

struct skin_object;
struct skin_sensor;
struct skin_region;

SKIN_DECL_BEGIN

typedef struct skin_sub_region skin_sub_region;

/*
 * skin_sub_region is a sub-region in the skin, consisting of sensors.  Sub-regions do not overlap.
 *
 * init				initialize to an invalid sub-region
 * free				release any resources and make invalid
 * sensors			get the array of array of sensors of the sub-region for each sensor type and their counts.
 *				If counts is NULL, it will be ignored.  It returns the number of sensor types, which is the
 *				size of the array and the same as sensor_types_count()
 * sensor_types_count		get the number of sensor types in the skin, which is the same as skin_object_sensor_types_count().
 *				This is useful for acquiring memory before calling sensors()
 * region_indices		get the array of region indices that contain the sub-region and its count.  Note that this array contains
 *				indices to the real regions array (retrievable from skin_object_regions()).  If count is NULL,
 *				it will be ignored
 * region_indices_count		get the number of regions that containt the sub-region
 * for_each_sensor		call a callback for each sensor of the sub-region.  Note that the callback is called for sensors with an
 *				ordering over sensor types.  That is, sensors of each sensor type are given contiguously
 * for_each_region		call a callback for each region of the sub-region
 */
void skin_sub_region_init(skin_sub_region *sub_region);
void skin_sub_region_free(skin_sub_region *sub_region);
skin_sensor_type_size skin_sub_region_sensors(skin_sub_region *sub_region, skin_sensor **sensors, skin_sensor_size *counts);
skin_sensor_type_size skin_sub_region_sensor_types_count(skin_sub_region *sub_region);
skin_region_id *skin_sub_region_region_indices(skin_sub_region *sub_region, skin_region_index_size *count);
static inline skin_region_index_size skin_sub_region_region_indices_count(skin_sub_region *sub_region);
void skin_sub_region_for_each_sensor(skin_sub_region *sub_region, skin_callback_sensor c, void *data);
void skin_sub_region_for_each_region(skin_sub_region *sub_region, skin_callback_region c, void *data);

struct skin_sub_region
{
	skin_sub_region_id		id;			/* id of the sub_region itself.  Is equal to its index in the sub_region list */
	skin_sensor_id			*sensors_begins,	/* [begins[i] ends[i]) are ids of sensors of this sub-region of type i */
					*sensors_ends;		/* The size of these arrays is number of sensor types in the skin */
	skin_region_index_id		region_indices_begin,	/* [begin end) are indices to regions indices array.  Those values are indices */
					region_indices_end;	/* to the actual regions (this is similar to sub_region_indices_begin/end) */

	/* internal */
	struct skin_object		*p_object;

#ifdef SKIN_CPP
	skin_sub_region() { skin_sub_region_init(this); }
	~skin_sub_region() { skin_sub_region_free(this); }
	skin_sensor_type_size sensors(skin_sensor **sensors, skin_sensor_size *counts = NULL)
					{ return skin_sub_region_sensors(this, sensors, counts); }
	skin_sensor_type_size sensor_types_count() { return skin_sub_region_sensor_types_count(this); }
	skin_region_id *region_indices(skin_region_index_size *count = NULL) { return skin_sub_region_region_indices(this, count); }
	skin_region_index_size region_indices_count() { return skin_sub_region_region_indices_count(this); }

	class skin_sensor_iterator
	{
	private:
		skin_sensor_id current_sensor;
		skin_sensor_type_id current_sensor_type;
		skin_sub_region *sub_region;
		skin_sensor_iterator(skin_sub_region *sub_region, skin_sensor_type_id cst, skin_sensor_id cs);
	public:
		skin_sensor_iterator();
		skin_sensor_iterator &operator ++();
		skin_sensor_iterator operator ++(int);
		bool operator ==(const skin_sensor_iterator &rhs) const;
		bool operator !=(const skin_sensor_iterator &rhs) const;
		skin_sensor &operator *() const;
		skin_sensor *operator ->() const;
		operator skin_sensor *() const;
		friend struct skin_sub_region;
	};
	skin_sensor_iterator sensors_iter_begin();
	const skin_sensor_iterator sensors_iter_end();

	class skin_region_iterator
	{
	private:
		skin_region_index_id current;
		skin_object *object;
		skin_region_iterator(skin_sub_region *sub_region, skin_region_index_id c);
	public:
		skin_region_iterator();
		skin_region_iterator &operator ++();
		skin_region_iterator operator ++(int);
		bool operator ==(const skin_region_iterator &rhs) const;
		bool operator !=(const skin_region_iterator &rhs) const;
		skin_region &operator *() const;
		skin_region *operator ->() const;
		operator skin_region *() const;
		friend struct skin_sub_region;
	};
	skin_region_iterator regions_iter_begin();
	const skin_region_iterator regions_iter_end();
#endif
};

static inline skin_region_index_size skin_sub_region_region_indices_count(skin_sub_region *sub_region)
				{ return sub_region->region_indices_end - sub_region->region_indices_begin; }

SKIN_DECL_END

#endif
