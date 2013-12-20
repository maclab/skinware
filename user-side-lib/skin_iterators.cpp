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

#include "skin_internal.h"
#include "skin_common.h"
#include "skin_object.h"
#include "skin_sensor.h"
#include "skin_sub_region.h"
#include "skin_region.h"
#include "skin_module.h"
#include "skin_patch.h"
#include "skin_sensor_type.h"

/* skin_object */
skin_object::skin_sensor_iterator skin_object::sensors_iter_begin()
{
	return skin_sensor_iterator(this, 0);
}

const skin_object::skin_sensor_iterator skin_object::sensors_iter_end()
{
	return skin_sensor_iterator(this, p_sensors_count);
}

skin_object::skin_sensor_type_iterator skin_object::sensor_types_iter_begin()
{
	return skin_sensor_type_iterator(this, 0);
}

const skin_object::skin_sensor_type_iterator skin_object::sensor_types_iter_end()
{
	return skin_sensor_type_iterator(this, p_sensor_types_count);
}

skin_object::skin_module_iterator skin_object::modules_iter_begin()
{
	return skin_module_iterator(this, 0);
}

const skin_object::skin_module_iterator skin_object::modules_iter_end()
{
	return skin_module_iterator(this, p_modules_count);
}

skin_object::skin_patch_iterator skin_object::patches_iter_begin()
{
	return skin_patch_iterator(this, 0);
}

const skin_object::skin_patch_iterator skin_object::patches_iter_end()
{
	return skin_patch_iterator(this, p_patches_count);
}

skin_object::skin_sub_region_iterator skin_object::sub_regions_iter_begin()
{
	return skin_sub_region_iterator(this, 0);
}

const skin_object::skin_sub_region_iterator skin_object::sub_regions_iter_end()
{
	return skin_sub_region_iterator(this, p_sub_regions_count);
}

skin_object::skin_region_iterator skin_object::regions_iter_begin()
{
	return skin_region_iterator(this, 0);
}

const skin_object::skin_region_iterator skin_object::regions_iter_end()
{
	return skin_region_iterator(this, p_regions_count);
}

skin_object::skin_sensor_iterator::skin_sensor_iterator()
{
	object = NULL;
	current = 0;
}

skin_object::skin_sensor_iterator::skin_sensor_iterator(skin_object *obj, skin_sensor_id c)
{
	object = obj;
	current = c;
}

skin_object::skin_sensor_iterator &skin_object::skin_sensor_iterator::operator ++()
{
	++current;
	return *this;
}

skin_object::skin_sensor_iterator skin_object::skin_sensor_iterator::operator ++(int)
{
	skin_sensor_iterator i = *this;
	++*this;
	return i;
}

bool skin_object::skin_sensor_iterator::operator ==(const skin_sensor_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_object::skin_sensor_iterator::operator !=(const skin_sensor_iterator &rhs) const
{
	return current != rhs.current;
}

skin_sensor &skin_object::skin_sensor_iterator::operator *() const
{
	return object->p_sensors[current];
}

skin_sensor *skin_object::skin_sensor_iterator::operator ->() const
{
	return &object->p_sensors[current];
}

skin_object::skin_sensor_iterator::operator skin_sensor *() const
{
	if (!object || !object->p_sensors)
		return NULL;
	return object->p_sensors + current;
}

skin_object::skin_sensor_type_iterator::skin_sensor_type_iterator()
{
	object = NULL;
	current = 0;
}

skin_object::skin_sensor_type_iterator::skin_sensor_type_iterator(skin_object *obj, skin_sensor_type_id c)
{
	object = obj;
	current = c;
}

skin_object::skin_sensor_type_iterator &skin_object::skin_sensor_type_iterator::operator ++()
{
	++current;
	return *this;
}

skin_object::skin_sensor_type_iterator skin_object::skin_sensor_type_iterator::operator ++(int)
{
	skin_sensor_type_iterator i = *this;
	++*this;
	return i;
}

bool skin_object::skin_sensor_type_iterator::operator ==(const skin_sensor_type_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_object::skin_sensor_type_iterator::operator !=(const skin_sensor_type_iterator &rhs) const
{
	return current != rhs.current;
}

skin_sensor_type &skin_object::skin_sensor_type_iterator::operator *() const
{
	return object->p_sensor_types[current];
}

skin_sensor_type *skin_object::skin_sensor_type_iterator::operator ->() const
{
	return &object->p_sensor_types[current];
}

skin_object::skin_sensor_type_iterator::operator skin_sensor_type *() const
{
	if (!object || !object->p_sensor_types)
		return NULL;
	return object->p_sensor_types + current;
}

skin_object::skin_module_iterator::skin_module_iterator()
{
	object = NULL;
	current = 0;
}

skin_object::skin_module_iterator::skin_module_iterator(skin_object *obj, skin_module_id c)
{
	object = obj;
	current = c;
}

skin_object::skin_module_iterator &skin_object::skin_module_iterator::operator ++()
{
	++current;
	return *this;
}

skin_object::skin_module_iterator skin_object::skin_module_iterator::operator ++(int)
{
	skin_module_iterator i = *this;
	++*this;
	return i;
}

bool skin_object::skin_module_iterator::operator ==(const skin_module_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_object::skin_module_iterator::operator !=(const skin_module_iterator &rhs) const
{
	return current != rhs.current;
}

skin_module &skin_object::skin_module_iterator::operator *() const
{
	return object->p_modules[current];
}

skin_module *skin_object::skin_module_iterator::operator ->() const
{
	return &object->p_modules[current];
}

skin_object::skin_module_iterator::operator skin_module *() const
{
	if (!object || !object->p_modules)
		return NULL;
	return object->p_modules + current;
}

skin_object::skin_patch_iterator::skin_patch_iterator()
{
	object = NULL;
	current = 0;
}

skin_object::skin_patch_iterator::skin_patch_iterator(skin_object *obj, skin_patch_id c)
{
	object = obj;
	current = c;
}

skin_object::skin_patch_iterator &skin_object::skin_patch_iterator::operator ++()
{
	++current;
	return *this;
}

skin_object::skin_patch_iterator skin_object::skin_patch_iterator::operator ++(int)
{
	skin_patch_iterator i = *this;
	++*this;
	return i;
}

bool skin_object::skin_patch_iterator::operator ==(const skin_patch_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_object::skin_patch_iterator::operator !=(const skin_patch_iterator &rhs) const
{
	return current != rhs.current;
}

skin_patch &skin_object::skin_patch_iterator::operator *() const
{
	return object->p_patches[current];
}

skin_patch *skin_object::skin_patch_iterator::operator ->() const
{
	return &object->p_patches[current];
}

skin_object::skin_patch_iterator::operator skin_patch *() const
{
	if (!object || !object->p_patches)
		return NULL;
	return object->p_patches + current;
}

skin_object::skin_sub_region_iterator::skin_sub_region_iterator()
{
	object = NULL;
	current = 0;
}

skin_object::skin_sub_region_iterator::skin_sub_region_iterator(skin_object *obj, skin_sub_region_id c)
{
	object = obj;
	current = c;
}

skin_object::skin_sub_region_iterator &skin_object::skin_sub_region_iterator::operator ++()
{
	++current;
	return *this;
}

skin_object::skin_sub_region_iterator skin_object::skin_sub_region_iterator::operator ++(int)
{
	skin_sub_region_iterator i = *this;
	++*this;
	return i;
}

bool skin_object::skin_sub_region_iterator::operator ==(const skin_sub_region_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_object::skin_sub_region_iterator::operator !=(const skin_sub_region_iterator &rhs) const
{
	return current != rhs.current;
}

skin_sub_region &skin_object::skin_sub_region_iterator::operator *() const
{
	return object->p_sub_regions[current];
}

skin_sub_region *skin_object::skin_sub_region_iterator::operator ->() const
{
	return &object->p_sub_regions[current];
}

skin_object::skin_sub_region_iterator::operator skin_sub_region *() const
{
	if (!object || !object->p_sub_regions)
		return NULL;
	return object->p_sub_regions + current;
}

skin_object::skin_region_iterator::skin_region_iterator()
{
	object = NULL;
	current = 0;
}

skin_object::skin_region_iterator::skin_region_iterator(skin_object *obj, skin_region_id c)
{
	object = obj;
	current = c;
}

skin_object::skin_region_iterator &skin_object::skin_region_iterator::operator ++()
{
	++current;
	return *this;
}

skin_object::skin_region_iterator skin_object::skin_region_iterator::operator ++(int)
{
	skin_region_iterator i = *this;
	++*this;
	return i;
}

bool skin_object::skin_region_iterator::operator ==(const skin_region_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_object::skin_region_iterator::operator !=(const skin_region_iterator &rhs) const
{
	return current != rhs.current;
}

skin_region &skin_object::skin_region_iterator::operator *() const
{
	return object->p_regions[current];
}

skin_region *skin_object::skin_region_iterator::operator ->() const
{
	return &object->p_regions[current];
}

skin_object::skin_region_iterator::operator skin_region *() const
{
	if (!object || !object->p_regions)
		return NULL;
	return object->p_regions + current;
}

/* skin_sensor_type */
skin_sensor_type::skin_sensor_iterator skin_sensor_type::sensors_iter_begin()
{
	return skin_sensor_iterator(this, sensors_begin);
}

const skin_sensor_type::skin_sensor_iterator skin_sensor_type::sensors_iter_end()
{
	return skin_sensor_iterator(this, sensors_end);
}

skin_sensor_type::skin_sensor_iterator::skin_sensor_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_sensor_type::skin_sensor_iterator::skin_sensor_iterator(skin_sensor_type *sensorType, skin_sensor_id c)
{
	object	= sensorType->p_object;
	current	= c;
}

skin_sensor_type::skin_sensor_iterator &skin_sensor_type::skin_sensor_iterator::operator ++()
{
	++current;
	return *this;
}

skin_sensor_type::skin_sensor_iterator skin_sensor_type::skin_sensor_iterator::operator ++(int)
{
	skin_sensor_iterator i = *this;
	++current;
	return i;
}

bool skin_sensor_type::skin_sensor_iterator::operator ==(const skin_sensor_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_sensor_type::skin_sensor_iterator::operator !=(const skin_sensor_iterator &rhs) const
{
	return current != rhs.current;
}

skin_sensor &skin_sensor_type::skin_sensor_iterator::operator *() const
{
	return object->p_sensors[current];
}

skin_sensor *skin_sensor_type::skin_sensor_iterator::operator ->() const
{
	return &object->p_sensors[current];
}

skin_sensor_type::skin_sensor_iterator::operator skin_sensor *() const
{
	if (!object || !object->p_sensors)
		return NULL;
	return object->p_sensors + current;
}

skin_sensor_type::skin_module_iterator skin_sensor_type::modules_iter_begin()
{
	return skin_module_iterator(this, modules_begin);
}

const skin_sensor_type::skin_module_iterator skin_sensor_type::modules_iter_end()
{
	return skin_module_iterator(this, modules_end);
}

skin_sensor_type::skin_module_iterator::skin_module_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_sensor_type::skin_module_iterator::skin_module_iterator(skin_sensor_type *moduleType, skin_module_id c)
{
	object	= moduleType->p_object;
	current	= c;
}

skin_sensor_type::skin_module_iterator &skin_sensor_type::skin_module_iterator::operator ++()
{
	++current;
	return *this;
}

skin_sensor_type::skin_module_iterator skin_sensor_type::skin_module_iterator::operator ++(int)
{
	skin_module_iterator i = *this;
	++current;
	return i;
}

bool skin_sensor_type::skin_module_iterator::operator ==(const skin_module_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_sensor_type::skin_module_iterator::operator !=(const skin_module_iterator &rhs) const
{
	return current != rhs.current;
}

skin_module &skin_sensor_type::skin_module_iterator::operator *() const
{
	return object->p_modules[current];
}

skin_module *skin_sensor_type::skin_module_iterator::operator ->() const
{
	return &object->p_modules[current];
}

skin_sensor_type::skin_module_iterator::operator skin_module *() const
{
	if (!object || !object->p_modules)
		return NULL;
	return object->p_modules + current;
}

skin_sensor_type::skin_patch_iterator skin_sensor_type::patches_iter_begin()
{
	return skin_patch_iterator(this, patches_begin);
}

const skin_sensor_type::skin_patch_iterator skin_sensor_type::patches_iter_end()
{
	return skin_patch_iterator(this, patches_end);
}

skin_sensor_type::skin_patch_iterator::skin_patch_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_sensor_type::skin_patch_iterator::skin_patch_iterator(skin_sensor_type *patchType, skin_patch_id c)
{
	object	= patchType->p_object;
	current	= c;
}

skin_sensor_type::skin_patch_iterator &skin_sensor_type::skin_patch_iterator::operator ++()
{
	++current;
	return *this;
}

skin_sensor_type::skin_patch_iterator skin_sensor_type::skin_patch_iterator::operator ++(int)
{
	skin_patch_iterator i = *this;
	++current;
	return i;
}

bool skin_sensor_type::skin_patch_iterator::operator ==(const skin_patch_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_sensor_type::skin_patch_iterator::operator !=(const skin_patch_iterator &rhs) const
{
	return current != rhs.current;
}

skin_patch &skin_sensor_type::skin_patch_iterator::operator *() const
{
	return object->p_patches[current];
}

skin_patch *skin_sensor_type::skin_patch_iterator::operator ->() const
{
	return &object->p_patches[current];
}

skin_sensor_type::skin_patch_iterator::operator skin_patch *() const
{
	if (!object || !object->p_patches)
		return NULL;
	return object->p_patches + current;
}

/* skin_region */
skin_region::skin_sensor_iterator skin_region::sensors_iter_begin()
{
	return skin_sensor_iterator(this, 0, sub_region_indices_begin, p_object->
			p_sub_regions[p_object->p_sub_region_indices[sub_region_indices_begin]].sensors_begins[0]);
}

const skin_region::skin_sensor_iterator skin_region::sensors_iter_end()
{
	return skin_sensor_iterator(this, p_object->p_sensor_types_count, 0, 0);
}

skin_region::skin_sub_region_iterator skin_region::sub_regions_iter_begin()
{
	return skin_sub_region_iterator(this, sub_region_indices_begin);
}

const skin_region::skin_sub_region_iterator skin_region::sub_regions_iter_end()
{
	return skin_sub_region_iterator(this, sub_region_indices_end);
}

skin_region::skin_sensor_iterator::skin_sensor_iterator()
{
	region			= NULL;
	current_sensor_type	= 0;
	current_sub_region	= 0;
	current_sensor		= 0;
}

skin_region::skin_sensor_iterator::skin_sensor_iterator(skin_region *rgn, skin_sensor_type_id cst, skin_sub_region_id csr, skin_sensor_id cs)
{
	region			= rgn;
	current_sensor_type	= cst;
	current_sub_region	= csr;
	current_sensor		= cs;
}

skin_region::skin_sensor_iterator &skin_region::skin_sensor_iterator::operator ++()
{
	++current_sensor;
	skin_object			*obj			= region->p_object;
	skin_sensor_type_size		sensor_type_count	= obj->p_sensor_types_count;
	skin_sub_region			*sub_regions		= obj->p_sub_regions;
	skin_sub_region_index_id	sub_region_end		= region->sub_region_indices_end;
	skin_sub_region_id		*sub_region_indices	= obj->p_sub_region_indices;
	while (current_sensor_type < sensor_type_count
		&& current_sub_region < sub_region_end
		&& current_sensor >= sub_regions[sub_region_indices[current_sub_region]].sensors_ends[current_sensor_type])
	{
		++current_sub_region;
		while (current_sensor_type < sensor_type_count
			&& current_sub_region >= sub_region_end)	/* should only happen once, unless the region has no sub-regions!! */
		{
			++current_sensor_type;
			current_sub_region = (current_sensor_type == sensor_type_count)?0:region->sub_region_indices_begin;
		}
		current_sensor = (current_sensor_type == sensor_type_count)?0:
			sub_regions[sub_region_indices[current_sub_region]].sensors_begins[current_sensor_type];
	}
	return *this;
}

skin_region::skin_sensor_iterator skin_region::skin_sensor_iterator::operator ++(int)
{
	skin_sensor_iterator i = *this;
	++*this;
	return i;
}

bool skin_region::skin_sensor_iterator::operator ==(const skin_sensor_iterator &rhs) const
{
	return current_sub_region == rhs.current_sub_region && current_sensor == rhs.current_sensor
		&& current_sensor_type == rhs.current_sensor_type;
}

bool skin_region::skin_sensor_iterator::operator !=(const skin_sensor_iterator &rhs) const
{
	return current_sub_region != rhs.current_sub_region || current_sensor != rhs.current_sensor
		|| current_sensor_type != rhs.current_sensor_type;
}

skin_sensor &skin_region::skin_sensor_iterator::operator *() const
{
	return region->p_object->p_sensors[current_sensor];
}

skin_sensor *skin_region::skin_sensor_iterator::operator ->() const
{
	return &region->p_object->p_sensors[current_sensor];
}

skin_region::skin_sensor_iterator::operator skin_sensor *() const
{
	if (!region || !region->p_object || !region->p_object->p_sensors)
		return NULL;
	return region->p_object->p_sensors + current_sensor;
}

skin_region::skin_sub_region_iterator::skin_sub_region_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_region::skin_sub_region_iterator::skin_sub_region_iterator(skin_region *region, skin_sub_region_index_id c)
{
	object	= region->p_object;
	current	= c;
}

skin_region::skin_sub_region_iterator &skin_region::skin_sub_region_iterator::operator ++()
{
	++current;
	return *this;
}

skin_region::skin_sub_region_iterator skin_region::skin_sub_region_iterator::operator ++(int)
{
	skin_sub_region_iterator i = *this;
	++*this;
	return i;
}

bool skin_region::skin_sub_region_iterator::operator ==(const skin_sub_region_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_region::skin_sub_region_iterator::operator !=(const skin_sub_region_iterator &rhs) const
{
	return current != rhs.current;
}

skin_sub_region &skin_region::skin_sub_region_iterator::operator *() const
{
	return object->p_sub_regions[object->p_sub_region_indices[current]];
}

skin_sub_region *skin_region::skin_sub_region_iterator::operator ->() const
{
	return &object->p_sub_regions[object->p_sub_region_indices[current]];
}

skin_region::skin_sub_region_iterator::operator skin_sub_region *() const
{
	if (!object || !object->p_sub_regions || !object->p_sub_region_indices)
		return NULL;
	return object->p_sub_regions + object->p_sub_region_indices[current];
}

/* skin_sub_region */
skin_sub_region::skin_sensor_iterator skin_sub_region::sensors_iter_begin()
{
	return skin_sensor_iterator(this, 0, sensors_begins[0]);
}

const skin_sub_region::skin_sensor_iterator skin_sub_region::sensors_iter_end()
{
	return skin_sensor_iterator(this, p_object->p_sensor_types_count, 0);
}

skin_sub_region::skin_region_iterator skin_sub_region::regions_iter_begin()
{
	return skin_region_iterator(this, region_indices_begin);
}

const skin_sub_region::skin_region_iterator skin_sub_region::regions_iter_end()
{
	return skin_region_iterator(this, region_indices_end);
}

skin_sub_region::skin_sensor_iterator::skin_sensor_iterator()
{
	sub_region		= NULL;
	current_sensor_type	= 0;
	current_sensor		= 0;
}

skin_sub_region::skin_sensor_iterator::skin_sensor_iterator(skin_sub_region *sub_rgn, skin_sensor_type_id cst, skin_sensor_id cs)
{
	sub_region		= sub_rgn;
	current_sensor_type	= cst;
	current_sensor		= cs;
}

skin_sub_region::skin_sensor_iterator &skin_sub_region::skin_sensor_iterator::operator ++()
{
	++current_sensor;
	skin_sensor_type_size sensorTypeCount = sub_region->p_object->p_sensor_types_count;
	while (current_sensor_type < sensorTypeCount
		&& current_sensor >= sub_region->sensors_ends[current_sensor_type])
	{
		++current_sensor_type;
		current_sensor = (current_sensor_type == sensorTypeCount)?0:
			sub_region->sensors_begins[current_sensor_type];
	}
	return *this;
}

skin_sub_region::skin_sensor_iterator skin_sub_region::skin_sensor_iterator::operator ++(int)
{
	skin_sensor_iterator i = *this;
	++*this;
	return i;
}

bool skin_sub_region::skin_sensor_iterator::operator ==(const skin_sensor_iterator &rhs) const
{
	return current_sensor == rhs.current_sensor && current_sensor_type == rhs.current_sensor_type;
}

bool skin_sub_region::skin_sensor_iterator::operator !=(const skin_sensor_iterator &rhs) const
{
	return current_sensor != rhs.current_sensor || current_sensor_type != rhs.current_sensor_type;
}

skin_sensor &skin_sub_region::skin_sensor_iterator::operator *() const
{
	return sub_region->p_object->p_sensors[current_sensor];
}

skin_sensor *skin_sub_region::skin_sensor_iterator::operator ->() const
{
	return &sub_region->p_object->p_sensors[current_sensor];
}

skin_sub_region::skin_sensor_iterator::operator skin_sensor *() const
{
	if (!sub_region || !sub_region->p_object || !sub_region->p_object->p_sensors)
		return NULL;
	return sub_region->p_object->p_sensors + current_sensor;
}

skin_sub_region::skin_region_iterator::skin_region_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_sub_region::skin_region_iterator::skin_region_iterator(skin_sub_region *sub_region, skin_region_index_id c)
{
	object	= sub_region->p_object;
	current	= c;
}

skin_sub_region::skin_region_iterator &skin_sub_region::skin_region_iterator::operator ++()
{
	++current;
	return *this;
}

skin_sub_region::skin_region_iterator skin_sub_region::skin_region_iterator::operator ++(int)
{
	skin_region_iterator i = *this;
	++*this;
	return i;
}

bool skin_sub_region::skin_region_iterator::operator ==(const skin_region_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_sub_region::skin_region_iterator::operator !=(const skin_region_iterator &rhs) const
{
	return current != rhs.current;
}

skin_region &skin_sub_region::skin_region_iterator::operator *() const
{
	return object->p_regions[object->p_region_indices[current]];
}

skin_region *skin_sub_region::skin_region_iterator::operator ->() const
{
	return &object->p_regions[object->p_region_indices[current]];
}

skin_sub_region::skin_region_iterator::operator skin_region *() const
{
	if (!object || !object->p_regions || !object->p_region_indices)
		return NULL;
	return object->p_regions + object->p_region_indices[current];
}

/* skin_patch */
skin_patch::skin_sensor_iterator skin_patch::sensors_iter_begin()
{
	return skin_sensor_iterator(this, modules_begin, p_object->p_modules[modules_begin].sensors_begin);
}

const skin_patch::skin_sensor_iterator skin_patch::sensors_iter_end()
{
	return skin_sensor_iterator(this, modules_end, 0);
}

skin_patch::skin_module_iterator skin_patch::modules_iter_begin()
{
	return skin_module_iterator(this, modules_begin);
}

const skin_patch::skin_module_iterator skin_patch::modules_iter_end()
{
	return skin_module_iterator(this, modules_end);
}

skin_patch::skin_sensor_iterator::skin_sensor_iterator()
{
	patch		= NULL;
	current_module	= 0;
	current_sensor	= 0;
}

skin_patch::skin_sensor_iterator::skin_sensor_iterator(skin_patch *p, skin_module_id cm, skin_sensor_id cs)
{
	patch		= p;
	current_module	= cm;
	current_sensor	= cs;
}

skin_patch::skin_sensor_iterator &skin_patch::skin_sensor_iterator::operator ++()
{
	++current_sensor;
	while (current_module < patch->modules_end
		&& current_sensor >= patch->p_object->p_modules[current_module].sensors_end)
	{
		++current_module;
		current_sensor = (current_module == patch->modules_end)?0:
			patch->p_object->p_modules[current_module].sensors_begin;

	}
	return *this;
}

skin_patch::skin_sensor_iterator skin_patch::skin_sensor_iterator::operator ++(int)
{
	skin_sensor_iterator i = *this;
	++*this;
	return i;
}

bool skin_patch::skin_sensor_iterator::operator ==(const skin_sensor_iterator &rhs) const
{
	return current_module == rhs.current_module && current_sensor == rhs.current_sensor;
}

bool skin_patch::skin_sensor_iterator::operator !=(const skin_sensor_iterator &rhs) const
{
	return current_module != rhs.current_module || current_sensor != rhs.current_sensor;
}

skin_sensor &skin_patch::skin_sensor_iterator::operator *() const
{
	return patch->p_object->p_sensors[current_sensor];
}

skin_sensor *skin_patch::skin_sensor_iterator::operator ->() const
{
	return &patch->p_object->p_sensors[current_sensor];
}

skin_patch::skin_sensor_iterator::operator skin_sensor *() const
{
	if (!patch || !patch->p_object || !patch->p_object->p_sensors)
		return NULL;
	return patch->p_object->p_sensors + current_sensor;
}

skin_patch::skin_module_iterator::skin_module_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_patch::skin_module_iterator::skin_module_iterator(skin_patch *patch, skin_module_id c)
{
	object	= patch->p_object;
	current	= c;
}

skin_patch::skin_module_iterator &skin_patch::skin_module_iterator::operator ++()
{
	++current;
	return *this;
}

skin_patch::skin_module_iterator skin_patch::skin_module_iterator::operator ++(int)
{
	skin_module_iterator i = *this;
	++*this;
	return i;
}

bool skin_patch::skin_module_iterator::operator ==(const skin_module_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_patch::skin_module_iterator::operator !=(const skin_module_iterator &rhs) const
{
	return current != rhs.current;
}

skin_module &skin_patch::skin_module_iterator::operator *() const
{
	return object->p_modules[current];
}

skin_module *skin_patch::skin_module_iterator::operator ->() const
{
	return &object->p_modules[current];
}

skin_patch::skin_module_iterator::operator skin_module *() const
{
	if (!object || !object->p_modules)
		return NULL;
	return object->p_modules + current;
}

/* skin_module */
skin_module::skin_sensor_iterator skin_module::sensors_iter_begin()
{
	return skin_sensor_iterator(this, sensors_begin);
}

const skin_module::skin_sensor_iterator skin_module::sensors_iter_end()
{
	return skin_sensor_iterator(this, sensors_end);
}

skin_module::skin_sensor_iterator::skin_sensor_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_module::skin_sensor_iterator::skin_sensor_iterator(skin_module *module, skin_sensor_id c)
{
	object	= module->p_object;
	current	= c;
}

skin_module::skin_sensor_iterator &skin_module::skin_sensor_iterator::operator ++()
{
	++current;
	return *this;
}

skin_module::skin_sensor_iterator skin_module::skin_sensor_iterator::operator ++(int)
{
	skin_sensor_iterator i = *this;
	++*this;
	return i;
}

bool skin_module::skin_sensor_iterator::operator ==(const skin_sensor_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_module::skin_sensor_iterator::operator !=(const skin_sensor_iterator &rhs) const
{
	return current != rhs.current;
}

skin_sensor &skin_module::skin_sensor_iterator::operator *() const
{
	return object->p_sensors[current];
}

skin_sensor *skin_module::skin_sensor_iterator::operator ->() const
{
	return &object->p_sensors[current];
}

skin_module::skin_sensor_iterator::operator skin_sensor *() const
{
	if (!object || !object->p_sensors)
		return NULL;
	return object->p_sensors + current;
}

/* skin_sensor */
skin_sensor::skin_region_iterator skin_sensor::regions_iter_begin()
{
	return skin_region_iterator(this, p_object->p_sub_regions[sub_region_id].region_indices_begin);
}

const skin_sensor::skin_region_iterator skin_sensor::regions_iter_end()
{
	return skin_region_iterator(this, p_object->p_sub_regions[sub_region_id].region_indices_end);
}

skin_sensor::skin_region_iterator::skin_region_iterator()
{
	object	= NULL;
	current	= 0;
}

skin_sensor::skin_region_iterator::skin_region_iterator(skin_sensor *sensor, skin_region_id c)
{
	object	= sensor->p_object;
	current	= c;
}

skin_sensor::skin_region_iterator &skin_sensor::skin_region_iterator::operator ++()
{
	++current;
	return *this;
}

skin_sensor::skin_region_iterator skin_sensor::skin_region_iterator::operator ++(int)
{
	skin_region_iterator i = *this;
	++*this;
	return i;
}

bool skin_sensor::skin_region_iterator::operator ==(const skin_region_iterator &rhs) const
{
	return current == rhs.current;
}

bool skin_sensor::skin_region_iterator::operator !=(const skin_region_iterator &rhs) const
{
	return current != rhs.current;
}

skin_region &skin_sensor::skin_region_iterator::operator *() const
{
	return object->p_regions[object->p_region_indices[current]];
}

skin_region *skin_sensor::skin_region_iterator::operator ->() const
{
	return &object->p_regions[object->p_region_indices[current]];
}

skin_sensor::skin_region_iterator::operator skin_region *() const
{
	if (!object || !object->p_regions || !object->p_region_indices)
		return NULL;
	return object->p_regions + object->p_region_indices[current];
}
