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

#ifndef SKIN_OBJECT_H
#define SKIN_OBJECT_H

#ifdef __KERNEL__
#error this file is not supposed to be included in kernel
#endif

#include "skin_common.h"
#ifndef SKIN_DS_ONLY
# include "skin_reader.h"
# include "skin_services.h"
#endif
#include "skin_callbacks.h"

struct skin_object;
struct skin_sensor;
struct skin_sub_region;
struct skin_region;
struct skin_module;
struct skin_patch;
struct skin_sensor_type;

SKIN_DECL_BEGIN

#ifndef SKIN_DS_ONLY
/* helper struct for regionalization.  Containts information on one region */
typedef struct skin_regionalization_data
{
	skin_module_id *modules;	/* array of ids of modules in the region */
	skin_module_size size;		/* size of the modules array */
} skin_regionalization_data;
#endif

typedef struct skin_object skin_object;

/*
 * skin_object is the main data structure of the skin.  It holds all structures of the skin, as well as
 * the reader and the service manager.  The functions here are used for accessing these structures, as well
 * as initializing as a user, calibrator or regionalizer and performing the actual calibration or
 * regionalization.  The functions, unless stated otherwise, may return SKIN_SUCCESS if operation was
 * successful, SKIN_NO_MEM if out of memory or SKIN_FAIL if any other error.  Additionally, functions that
 * load calibration or regionalization data from file may return SKIN_NO_FILE if the file couldn't be opened,
 * SKIN_FILE_INCOMPLETE if the cache file was not complete, SKIN_FILE_PARSE_ERROR if there was an error
 * reading the cache file or SKIN_FILE_INVALID if the file didn't match the current skin.
 *
 * Initialization and termination:
 * init				initialize to an invalid object
 * free				release any resources and make invalid
 * load				load the skin from the skin kernel.  init must have been called before this function.  Before using
 *				the skin, this function should be called.  Not to be called if calibrating or regionalizing
 * calibration_begin		if calibrating, don't call skin_object_load, but this one
 * calibration_end		fill the calibration data in relative_position/orientation of skin_sensor and call this function.
 *				It would create the flattened image, set neighbors(_count) and transfer these data to the kernel module.
 *				The result would be saved in the cache file so that it could be reloaded
 * calibration_reload		if the calibration is already done and the robot is restarting, instead of calibration_begin/end,
 *				calibration_reload can be called.  Note that this function shouldn't be called by every user program,
 *				but just by the calibrator
 * regionalization_begin	if regionalizing, don't call skin_object_load, but this one
 * regionalization_end		fill an array of sets of modules as regions.  Each set of modules would be one region.  Modules can appear
 *				in many regions (that is, regions can overlap).  This function would then create region/sub-region associations
 *				and transfers these data to the kernel module.  The regionalization data would be saved in the cache file so
 *				that it can be reloaded
 * regionalization_reload	if the regionalization is already done and the robot is restarting, instead of regionalization_begin/end,
 *				regionalization_reload can be called.  Note that this function shouldn't be called by every user program,
 *				but just by the regionalizer
 *
 * Data structures access:
 * reader			get a reference to the skin_reader object
 * service_manager		get a reference to the skin_service_manager object
 * sensor_types			get the array of all sensor types in the skin and its count.  If count is NULL, it will be ignored
 * sensors			get the array of all sensors in the skin and its count.  If count is NULL, it will be ignored
 * modules			get the array of all modules in the skin and its count.  If count is NULL, it will be ignored
 * patches			get the array of all patches in the skin and its count.  If count is NULL, it will be ignored
 * sub_regions			get the array of all sub-regions in the skin and its count.  If count is NULL, it will be ignored
 * regions			get the array of all regions in the skin and its count.  If count is NULL, it will be ignored
 * sub_region_indices		get the array of region sub-region indices and its count.  Each region's own sub_region_indices_begin/end
 *				must be used to index this array and get the region's sub-region indices.  If count is NULL, it will be ignored
 * region_indices		get the array of sub-region region indices and its count.  Each sub-region's own region_indices_begin/end
 *				must be used to index this array and get the sub-region's region indices.  If count is NULL, it will be ignored
 * sensor_types_count		get the number of all sensor types in the skin
 * sensors_count		get the number of all sensors in the skin
 * modules_count		get the number of all modules in the skin
 * patches_count		get the number of all patches in the skin
 * sub_regions_count		get the number of all sub-regions in the skin
 * regions_count		get the number of all regions in the skin
 * sub_region_indices_count	get the total number of sub-region indices of all regions
 * region_indices_count		get the total number of region indices of all sub-regions
 * for_each_sensor_type		call a callback for each sensor type in the skin
 * for_each_sensor		call a callback for each sensor in the skin.  Note that the callback is called for sensors with an
 *				ordering over sensor types.  That is, sensors of each sensor type are given contiguously
 * for_each_module		call a callback for each module in the skin
 * for_each_patch		call a callback for each patch in the skin
 * for_each_sub_region		call a callback for each sub-region in the skin
 * for_each_region		call a callback for each region in the skin
 *
 * Miscellaneous:
 * last_error			get the last error (string) that was set by a failed operation.
 * time_diff_with_kernel	get an estimate of difference between skin_rt_get_time() in kernel and user spaces.  Normally, they
 *				should be the same, so this value should be near-zero.  However, it has been observed that this is
 *				not always the case
 */
void skin_object_init(skin_object *so);
void skin_object_free(skin_object *so);
#ifndef SKIN_DS_ONLY
int skin_object_load(skin_object *so);
int skin_object_calibration_begin(skin_object *so);
int skin_object_calibration_end(skin_object *so, const char *cache);
int skin_object_calibration_reload(skin_object *so, const char *cache);
int skin_object_regionalization_begin(skin_object *so);
int skin_object_regionalization_end(skin_object *so, skin_regionalization_data *regions,
		skin_region_size regions_count, const char *cache);
int skin_object_regionalization_reload(skin_object *so, const char *cache);
static inline skin_reader *skin_object_reader(skin_object *so);
static inline skin_service_manager *skin_object_service_manager(skin_object *so);
#endif
static inline struct skin_sensor_type *skin_object_sensor_types(skin_object *so, skin_sensor_type_size *count);
static inline struct skin_sensor *skin_object_sensors(skin_object *so, skin_sensor_size *count);
static inline struct skin_module *skin_object_modules(skin_object *so, skin_module_size *count);
static inline struct skin_patch *skin_object_patches(skin_object *so, skin_patch_size *count);
static inline struct skin_sub_region *skin_object_sub_regions(skin_object *so, skin_sub_region_size *count);
static inline struct skin_region *skin_object_regions(skin_object *so, skin_region_size *count);
static inline skin_sub_region_id *skin_object_sub_region_indices(skin_object *so, skin_sub_region_index_size *count);
static inline skin_region_id *skin_object_region_indices(skin_object *so, skin_region_index_size *count);
static inline skin_sensor_type_size skin_object_sensor_types_count(skin_object *so);
static inline skin_sensor_size skin_object_sensors_count(skin_object *so);
static inline skin_module_size skin_object_modules_count(skin_object *so);
static inline skin_patch_size skin_object_patches_count(skin_object *so);
static inline skin_sub_region_size skin_object_sub_regions_count(skin_object *so);
static inline skin_region_size skin_object_regions_count(skin_object *so);
static inline skin_sub_region_index_size skin_object_sub_region_indices_count(skin_object *so);
static inline skin_region_index_size skin_object_region_indices_count(skin_object *so);
void skin_object_for_each_sensor_type(skin_object *so, skin_callback_sensor_type c, void *data);
void skin_object_for_each_sensor(skin_object *so, skin_callback_sensor c, void *data);
void skin_object_for_each_module(skin_object *so, skin_callback_module c, void *data);
void skin_object_for_each_patch(skin_object *so, skin_callback_patch c, void *data);
void skin_object_for_each_sub_region(skin_object *so, skin_callback_sub_region c, void *data);
void skin_object_for_each_region(skin_object *so, skin_callback_region c, void *data);
#ifndef SKIN_DS_ONLY
static inline const char *skin_object_last_error(skin_object *so);
skin_rt_time skin_object_time_diff_with_kernel(skin_object *so);
#endif

/* normally, there would be only one object from the following struct */
struct skin_object
{
	/* internal */
	struct skin_sensor		*p_sensors;
	struct skin_sub_region		*p_sub_regions;
	struct skin_region		*p_regions;
	skin_sub_region_id		*p_sub_region_indices;
	skin_region_id			*p_region_indices;
	struct skin_module		*p_modules;
	struct skin_patch		*p_patches;
	struct skin_sensor_type		*p_sensor_types;
	skin_sensor_id			*p_sub_region_sensors_begins;	/* pack sensors_begin and sensors_end */
	skin_sensor_id			*p_sub_region_sensors_ends;	/* in one array to reduce wasted memory */
	skin_sensor_id			*p_sensor_neighbors;		/* TODO: Do I really need to keep the size of these three arrays? */

	skin_sensor_size		p_sensors_count;
	skin_sub_region_size		p_sub_regions_count;
	skin_region_size		p_regions_count;
	skin_sub_region_index_size	p_sub_region_indices_count;
	skin_region_index_size		p_region_indices_count;		/* should be equal to sub_region_indices_count */
	skin_module_size		p_modules_count;
	skin_patch_size			p_patches_count;
	skin_sensor_type_size		p_sensor_types_count;
	uint32_t			p_sub_region_sensors_begins_count;
	uint32_t			p_sub_region_sensors_ends_count; /* should be equal to sub_region_sensors_begins_count */
	uint32_t			p_sensor_neighbors_count;

#ifndef SKIN_DS_ONLY
	skin_reader			p_reader;
	skin_service_manager		p_service_manager;

	char				*p_error_message;
	size_t				p_error_message_mem_size;

	skin_rt_time			p_time_diff_with_kernel;
#endif

#ifdef SKIN_CPP
	skin_object() { skin_object_init(this); }
	~skin_object() { skin_object_free(this); }
	void unload() { skin_object_free(this); }
#ifndef SKIN_DS_ONLY
	int load() { return skin_object_load(this); }
	int calibration_begin() { return skin_object_calibration_begin(this); }
	int calibration_end(const char *cache) { return skin_object_calibration_end(this, cache); }
	int calibration_reload(const char *cache) { return skin_object_calibration_reload(this, cache); }
	int regionalization_begin() { return skin_object_regionalization_begin(this); }
	int regionalization_end(skin_regionalization_data *regions, skin_region_size regions_count, const char *cache)
			{ return skin_object_regionalization_end(this, regions, regions_count, cache); }
	int regionalization_reload(const char *cache) { return skin_object_regionalization_reload(this, cache); }
	skin_reader *reader() { return skin_object_reader(this); }
	skin_service_manager *service_manager() { return skin_object_service_manager(this); }
#endif
	struct skin_sensor_type *sensor_types(skin_sensor_type_size *count = NULL) { return skin_object_sensor_types(this, count); }
	struct skin_sensor *sensors(skin_sensor_size *count = NULL) { return skin_object_sensors(this, count); }
	struct skin_module *modules(skin_module_size *count = NULL) { return skin_object_modules(this, count); }
	struct skin_patch *patches(skin_patch_size *count = NULL) { return skin_object_patches(this, count); }
	struct skin_sub_region *sub_regions(skin_sub_region_size *count = NULL) { return skin_object_sub_regions(this, count); }
	struct skin_region *regions(skin_region_size *count = NULL) { return skin_object_regions(this, count); }
	skin_sub_region_id *sub_region_indices(skin_sub_region_index_size *count = NULL) { return skin_object_sub_region_indices(this, count); }
	skin_region_id *region_indices(skin_region_index_size *count = NULL) { return skin_object_region_indices(this, count); }
	skin_sensor_type_size sensor_types_count() { return skin_object_sensor_types_count(this); }
	skin_sensor_size sensors_count() { return skin_object_sensors_count(this); }
	skin_module_size modules_count() { return skin_object_modules_count(this); }
	skin_patch_size patches_count() { return skin_object_patches_count(this); }
	skin_sub_region_size sub_regions_count() { return skin_object_sub_regions_count(this); }
	skin_region_size regions_count() { return skin_object_regions_count(this); }
	skin_sub_region_index_size sub_region_indices_count() { return skin_object_sub_region_indices_count(this); }
	skin_region_index_size region_indices_count() { return skin_object_region_indices_count(this); }
#ifndef SKIN_DS_ONLY
	const char *last_error() { return skin_object_last_error(this); }
	skin_rt_time time_diff_with_kernel() { return skin_object_time_diff_with_kernel(this); }
#endif

	class skin_sensor_iterator
	{
	private:
		skin_sensor_id current;
		skin_object *object;
		skin_sensor_iterator(skin_object *object, skin_sensor_id c);
	public:
		skin_sensor_iterator();
		skin_sensor_iterator &operator ++();
		skin_sensor_iterator operator ++(int);
		bool operator ==(const skin_sensor_iterator &rhs) const;
		bool operator !=(const skin_sensor_iterator &rhs) const;
		skin_sensor &operator *() const;
		skin_sensor *operator ->() const;
		operator skin_sensor *() const;
		friend struct skin_object;
	};
	skin_sensor_iterator sensors_iter_begin();
	const skin_sensor_iterator sensors_iter_end();

	class skin_sensor_type_iterator
	{
	private:
		skin_sensor_type_id current;
		skin_object *object;
		skin_sensor_type_iterator(skin_object *object, skin_sensor_type_id c);
	public:
		skin_sensor_type_iterator();
		skin_sensor_type_iterator &operator ++();
		skin_sensor_type_iterator operator ++(int);
		bool operator ==(const skin_sensor_type_iterator &rhs) const;
		bool operator !=(const skin_sensor_type_iterator &rhs) const;
		skin_sensor_type &operator *() const;
		skin_sensor_type *operator ->() const;
		operator skin_sensor_type *() const;
		friend struct skin_object;
	};
	skin_sensor_type_iterator sensor_types_iter_begin();
	const skin_sensor_type_iterator sensor_types_iter_end();

	class skin_module_iterator
	{
	private:
		skin_module_id current;
		skin_object *object;
		skin_module_iterator(skin_object *object, skin_module_id c);
	public:
		skin_module_iterator();
		skin_module_iterator &operator ++();
		skin_module_iterator operator ++(int);
		bool operator ==(const skin_module_iterator &rhs) const;
		bool operator !=(const skin_module_iterator &rhs) const;
		skin_module &operator *() const;
		skin_module *operator ->() const;
		operator skin_module *() const;
		friend struct skin_object;
	};
	skin_module_iterator modules_iter_begin();
	const skin_module_iterator modules_iter_end();

	class skin_patch_iterator
	{
	private:
		skin_patch_id current;
		skin_object *object;
		skin_patch_iterator(skin_object *object, skin_patch_id c);
	public:
		skin_patch_iterator();
		skin_patch_iterator &operator ++();
		skin_patch_iterator operator ++(int);
		bool operator ==(const skin_patch_iterator &rhs) const;
		bool operator !=(const skin_patch_iterator &rhs) const;
		skin_patch &operator *() const;
		skin_patch *operator ->() const;
		operator skin_patch *() const;
		friend struct skin_object;
	};
	skin_patch_iterator patches_iter_begin();
	const skin_patch_iterator patches_iter_end();

	class skin_sub_region_iterator
	{
	private:
		skin_sub_region_id current;
		skin_object *object;
		skin_sub_region_iterator(skin_object *object, skin_sub_region_id c);
	public:
		skin_sub_region_iterator();
		skin_sub_region_iterator &operator ++();
		skin_sub_region_iterator operator ++(int);
		bool operator ==(const skin_sub_region_iterator &rhs) const;
		bool operator !=(const skin_sub_region_iterator &rhs) const;
		skin_sub_region &operator *() const;
		skin_sub_region *operator ->() const;
		operator skin_sub_region *() const;
		friend struct skin_object;
	};
	skin_sub_region_iterator sub_regions_iter_begin();
	const skin_sub_region_iterator sub_regions_iter_end();

	class skin_region_iterator
	{
	private:
		skin_region_id current;
		skin_object *object;
		skin_region_iterator(skin_object *object, skin_region_id c);
	public:
		skin_region_iterator();
		skin_region_iterator &operator ++();
		skin_region_iterator operator ++(int);
		bool operator ==(const skin_region_iterator &rhs) const;
		bool operator !=(const skin_region_iterator &rhs) const;
		skin_region &operator *() const;
		skin_region *operator ->() const;
		operator skin_region *() const;
		friend struct skin_object;
	};
	skin_region_iterator regions_iter_begin();
	const skin_region_iterator regions_iter_end();
#endif
};

#ifndef SKIN_DS_ONLY
static inline skin_reader *skin_object_reader(skin_object *so)					{ return &so->p_reader; }
static inline skin_service_manager *skin_object_service_manager(skin_object *so)		{ return &so->p_service_manager; }
#endif
static inline struct skin_sensor_type *skin_object_sensor_types(skin_object *so, skin_sensor_type_size *count)
{
	if (count)
		*count = so->p_sensor_types_count;
	return so->p_sensor_types;
}
static inline struct skin_sensor *skin_object_sensors(skin_object *so, skin_sensor_size *count)
{
	if (count)
		*count = so->p_sensors_count;
	return so->p_sensors;
}
static inline struct skin_module *skin_object_modules(skin_object *so, skin_module_size *count)
{
	if (count)
		*count = so->p_modules_count;
	return so->p_modules;
}
static inline struct skin_patch *skin_object_patches(skin_object *so, skin_patch_size *count)
{
	if (count)
		*count = so->p_patches_count;
	return so->p_patches;
}
static inline struct skin_sub_region *skin_object_sub_regions(skin_object *so, skin_sub_region_size *count)
{
	if (count)
		*count = so->p_sub_regions_count;
	return so->p_sub_regions;
}
static inline struct skin_region *skin_object_regions(skin_object *so, skin_region_size *count)
{
	if (count)
		*count = so->p_regions_count;
	return so->p_regions;
}
static inline skin_sub_region_id *skin_object_sub_region_indices(skin_object *so, skin_sub_region_index_size *count)
{
	if (count)
		*count = so->p_sub_region_indices_count;
	return so->p_sub_region_indices;
}
static inline skin_region_id *skin_object_region_indices(skin_object *so, skin_region_index_size *count)
{
	if (count)
		*count = so->p_region_indices_count;
	return so->p_region_indices;
}
static inline skin_sensor_type_size skin_object_sensor_types_count(skin_object *so)		{ return so->p_sensor_types_count; }
static inline skin_sensor_size skin_object_sensors_count(skin_object *so)			{ return so->p_sensors_count; }
static inline skin_module_size skin_object_modules_count(skin_object *so)			{ return so->p_modules_count; }
static inline skin_patch_size skin_object_patches_count(skin_object *so)			{ return so->p_patches_count; }
static inline skin_sub_region_size skin_object_sub_regions_count(skin_object *so)		{ return so->p_sub_regions_count; }
static inline skin_region_size skin_object_regions_count(skin_object *so)			{ return so->p_regions_count; }
static inline skin_sub_region_index_size skin_object_sub_region_indices_count(skin_object *so)	{ return so->p_sub_region_indices_count; }
static inline skin_region_index_size skin_object_region_indices_count(skin_object *so)		{ return so->p_region_indices_count; }
#ifndef SKIN_DS_ONLY
static inline const char *skin_object_last_error(skin_object *so)				{ return so->p_error_message; }
#endif

SKIN_DECL_END

#endif
