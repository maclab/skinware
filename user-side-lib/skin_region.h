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

#ifndef SKIN_REGION_H
#define SKIN_REGION_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#include "skin_callbacks.h"

struct skin_object;
struct skin_sub_region;

SKIN_DECL_BEGIN

typedef struct skin_region skin_region;

/*
 * skin_region is a region in the skin, consisting of sensors.  Regions may overlap and therefore are implemented using sub-regions.
 *
 * init				initialize to an invalid region
 * free				release any resources and make invalid
 * sensors			get the array of array of sensors of the region for each sensor type and their counts.
 *				If counts is NULL, it will be ignored.  It returns the number of sensor types, which is the
 *				size of the array and the same as sensor_types_count()
 * sensor_types_count		get the number of sensor types in the skin, which is the same as skin_object_sensor_types_count().
 *				This is useful for acquiring memory before calling sensors()
 * sub_region_indices		get the array of sub-region indices that make the region and its count.  Note that this array contains
 *				indices to the real sub-regions array (retrievable from skin_object_sub_regions()).  If count is NULL,
 *				it will be ignored
 * sub_region_indices_count	get the number of sub-regions that make the region
 * for_each_sensor		call a callback for each sensor of the region.  Note that the callback is called for sensors with an
 *				ordering over sensor types.  That is, sensors of each sensor type are given contiguously
 * for_each_sub_region		call a callback for each sub-region of the region
 */
void skin_region_init(skin_region *region);
void skin_region_free(skin_region *region);
skin_sub_region_id *skin_region_sub_region_indices(skin_region *region, skin_sub_region_index_size *count);
static inline skin_sub_region_index_size skin_region_sub_region_indices_count(skin_region *region);
void skin_region_for_each_sensor(skin_region *region, skin_callback_sensor c, void *data);
void skin_region_for_each_sub_region(skin_region *region, skin_callback_sub_region c, void *data);

struct skin_region
{
	skin_region_id			id;				/* id of the region itself.  Is equal to its index in the region list */
	skin_sub_region_index_id	sub_region_indices_begin,	/* [begin end) are indices to sub-regions_indices of this region */
					sub_region_indices_end;		/* the actual sub-regions are sub_regions[sub_region_indices[begin]], ... */

	/* internal */
	struct skin_object		*p_object;

#ifdef SKIN_CPP
	skin_region() { skin_region_init(this); }
	~skin_region() { skin_region_free(this); }
	skin_sub_region_id *sub_region_indices(skin_sub_region_index_size *count = NULL) { return skin_region_sub_region_indices(this, count); }
	skin_sub_region_index_size sub_region_indices_count() { return skin_region_sub_region_indices_count(this); }

	class skin_sensor_iterator
	{
	private:
		skin_sensor_id current_sensor;
		skin_sub_region_id current_sub_region;
		skin_sensor_type_id current_sensor_type;
		skin_region *region;
		skin_sensor_iterator(skin_region *region, skin_sensor_type_id cst, skin_sub_region_id csr, skin_sensor_id cs);
	public:
		skin_sensor_iterator();
		skin_sensor_iterator &operator ++();
		skin_sensor_iterator operator ++(int);
		bool operator ==(const skin_sensor_iterator &rhs) const;
		bool operator !=(const skin_sensor_iterator &rhs) const;
		skin_sensor &operator *() const;
		skin_sensor *operator ->() const;
		operator skin_sensor *() const;
		friend struct skin_region;
	};
	skin_sensor_iterator sensors_iter_begin();
	const skin_sensor_iterator sensors_iter_end();

	class skin_sub_region_iterator
	{
	private:
		skin_sub_region_index_id current;
		skin_object *object;
		skin_sub_region_iterator(skin_region *region, skin_sub_region_index_id c);
	public:
		skin_sub_region_iterator();
		skin_sub_region_iterator &operator ++();
		skin_sub_region_iterator operator ++(int);
		bool operator ==(const skin_sub_region_iterator &rhs) const;
		bool operator !=(const skin_sub_region_iterator &rhs) const;
		skin_sub_region &operator *() const;
		skin_sub_region *operator ->() const;
		operator skin_sub_region *() const;
		friend struct skin_region;
	};
	skin_sub_region_iterator sub_regions_iter_begin();
	const skin_sub_region_iterator sub_regions_iter_end();
#endif
};

static inline skin_sub_region_index_size skin_region_sub_region_indices_count(skin_region *region)
				{ return region->sub_region_indices_end - region->sub_region_indices_begin; }

SKIN_DECL_END

#endif
