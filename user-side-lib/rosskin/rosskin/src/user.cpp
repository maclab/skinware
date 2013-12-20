/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rosskin/Init.h>
#include <rosskin/Command.h>
#include <rosskin/Read.h>
#include <rosskin/Data.h>
#include <rosskin.h>
#include <iostream>

static void _fill_data(const rosskin::Data &d, rosskin::Connector &con)
{
	skin_sensor_type_size st_count = con.skin.p_sensor_types_count;
	skin_sensor_size s_count = con.skin.p_sensors_count;

	if (d.sensor_type_is_actives.size() != st_count)
	{
		ROS_WARN("rosskin user: mismatch in sensor types count between init and data");
		if (d.sensor_type_is_actives.size() < st_count)
			st_count = d.sensor_type_is_actives.size();
	}
	if (d.sensor_responses.size() != s_count)
	{
		ROS_WARN("rosskin user: mismatch in sensors count between init and data");
		if (d.sensor_responses.size() < s_count)
			s_count = d.sensor_responses.size();
	}

	for (skin_sensor_type_id i = 0; i < st_count; ++i)
		con.skin.p_sensor_types[i].is_active = d.sensor_type_is_actives[i];
	for (skin_sensor_id i = 0; i < s_count; ++i)
		con.skin.p_sensors[i].response = d.sensor_responses[i];
}

class just_for_ros_callback
{
public:
	rosskin::Connector *con;
	void receive(const rosskin::Data::ConstPtr &d);
};

void just_for_ros_callback::receive(const rosskin::Data::ConstPtr &d)
{
	_fill_data(*d, *con);
	if (con->callback)
		con->callback(*con, con->user_data);
}

int rosskin::Connector::load(ros::NodeHandle &n, uint64_t period, rosskin::Connector::callback_func cb, void *data)
{
	ros::ServiceClient srv = n.serviceClient<rosskin::Init>("init");
	rosskin::Init s;

	/* unload the skin and unsubscribe in case of double load */
	unload();

	s.request.period = period;
	if (!srv.call(s))
	{
		ROS_WARN("rosskin user: could not contact rosskin server");
		return -1;
	}

	this->id = s.response.id;
	this->period = period;
	this->started = true;

	skin.p_sensor_types_count = s.response.sensor_types_count;
	skin.p_sensors_count = s.response.sensors_count;
	skin.p_modules_count = s.response.modules_count;
	skin.p_patches_count = s.response.patches_count;
	skin.p_regions_count = s.response.regions_count;
	skin.p_sub_regions_count = s.response.sub_regions_count;
	skin.p_sub_region_indices_count = s.response.sub_region_indices_count;
	skin.p_region_indices_count = s.response.region_indices_count;
	skin.p_sensor_neighbors_count = s.response.sensor_neighbors_count;
	skin.p_sub_region_sensors_begins_count = skin.p_sensor_types_count * (uint32_t)skin.p_sub_regions_count;
	skin.p_sub_region_sensors_ends_count = skin.p_sensor_types_count * (uint32_t)skin.p_sub_regions_count;

	skin.p_sensor_types = (skin_sensor_type *)malloc(skin.p_sensor_types_count * sizeof(*skin.p_sensor_types));
	skin.p_sensors = (skin_sensor *)malloc(skin.p_sensors_count * sizeof(*skin.p_sensors));
	skin.p_modules = (skin_module *)malloc(skin.p_modules_count * sizeof(*skin.p_modules));
	skin.p_patches = (skin_patch *)malloc(skin.p_patches_count * sizeof(*skin.p_patches));
	skin.p_sub_regions = (skin_sub_region *)malloc(skin.p_sub_regions_count * sizeof(*skin.p_sub_regions));
	skin.p_regions = (skin_region *)malloc(skin.p_regions_count * sizeof(*skin.p_regions));
	skin.p_sub_region_indices = (skin_sub_region_id *)malloc(skin.p_sub_region_indices_count * sizeof(*skin.p_sub_region_indices));
	skin.p_region_indices = (skin_region_id *)malloc(skin.p_region_indices_count * sizeof(*skin.p_region_indices));
	skin.p_sub_region_sensors_begins = (skin_sensor_id *)malloc(skin.p_sub_region_sensors_begins_count * sizeof(*skin.p_sub_region_sensors_begins));
	skin.p_sub_region_sensors_ends = (skin_sensor_id *)malloc(skin.p_sub_region_sensors_ends_count * sizeof(*skin.p_sub_region_sensors_ends));
	if (skin.p_sensor_neighbors_count > 0)
		skin.p_sensor_neighbors = (skin_sensor_id *)malloc(skin.p_sensor_neighbors_count * sizeof(*skin.p_sensor_neighbors));

	if ((skin.p_sensor_neighbors_count > 0 && skin.p_sensor_neighbors == NULL) || skin.p_sensor_types == NULL
		|| skin.p_sensors == NULL || skin.p_modules == NULL || skin.p_patches == NULL || skin.p_sub_regions == NULL
		|| skin.p_regions == NULL || skin.p_sub_region_indices == NULL || skin.p_region_indices == NULL
		|| skin.p_sub_region_sensors_begins == NULL || skin.p_sub_region_sensors_ends == NULL)
	{
		ROS_ERROR("rosskin user: out of memory");
		unload();
		return -1;
	}

	/* packed helper arrays */
	for (uint32_t i = 0, size = s.response.sub_region_sensors_begins.size(); i < size; ++i)
		skin.p_sub_region_sensors_begins[i] = s.response.sub_region_sensors_begins[i];
	for (uint32_t i = 0, size = s.response.sub_region_sensors_ends.size(); i < size; ++i)
		skin.p_sub_region_sensors_ends[i] = s.response.sub_region_sensors_ends[i];
	for (uint64_t i = 0, size = s.response.region_sub_region_indices.size(); i < size; ++i)
		skin.p_sub_region_indices[i] = s.response.region_sub_region_indices[i];
	for (uint64_t i = 0, size = s.response.sub_region_region_indices.size(); i < size; ++i)
		skin.p_region_indices[i] = s.response.sub_region_region_indices[i];
	for (uint32_t i = 0, size = s.response.sensor_neighbors.size(); i < size; ++i)
		skin.p_sensor_neighbors[i] = s.response.sensor_neighbors[i];

	/* sensor types */
	for (skin_sensor_type_id i = 0, size = skin.p_sensor_types_count; i < size; ++i)
	{
		skin_sensor_type *st = &skin.p_sensor_types[i];
		st->name = strdup(s.response.sensor_type_names[i].c_str());
		st->sensors_begin = s.response.sensor_type_sensors_begins[i];
		st->sensors_end = s.response.sensor_type_sensors_ends[i];
		st->modules_begin = s.response.sensor_type_modules_begins[i];
		st->modules_end = s.response.sensor_type_modules_ends[i];
		st->patches_begin = s.response.sensor_type_patches_begins[i];
		st->patches_end = s.response.sensor_type_patches_ends[i];
	}

	/* patches */
	for (skin_patch_id i = 0, size = skin.p_patches_count; i < size; ++i)
	{
		skin_patch *p = &skin.p_patches[i];
		p->modules_begin = s.response.patch_modules_begins[i];
		p->modules_end = s.response.patch_modules_ends[i];
	}

	/* modules */
	for (skin_module_id i = 0, size = skin.p_modules_count; i < size; ++i)
	{
		skin_module *m = &skin.p_modules[i];
		m->sensors_begin = s.response.module_sensors_begins[i];
		m->sensors_end = s.response.module_sensors_ends[i];
	}

	/* regions */
	for (skin_region_id i = 0, size = skin.p_regions_count; i < size; ++i)
	{
		skin_region *r = &skin.p_regions[i];
		r->sub_region_indices_begin = s.response.region_sub_region_indices_begins[i];
		r->sub_region_indices_end = s.response.region_sub_region_indices_ends[i];
	}

	/* sub_regions */
	for (skin_sub_region_id i = 0, size = skin.p_sub_regions_count; i < size; ++i)
	{
		skin_sub_region *sr = &skin.p_sub_regions[i];
		sr->region_indices_begin = s.response.sub_region_region_indices_begins[i];
		sr->region_indices_end = s.response.sub_region_region_indices_ends[i];
	}

	/* sensors */
	for (skin_sensor_id i = 0, size = skin.p_sensors_count; i < size; ++i)
	{
		skin_sensor *sn = &skin.p_sensors[i];
		sn->relative_position[0] = s.response.sensor_relative_positions[3 * i];
		sn->relative_position[1] = s.response.sensor_relative_positions[3 * i + 1];
		sn->relative_position[2] = s.response.sensor_relative_positions[3 * i + 2];
		sn->relative_orientation[0] = s.response.sensor_relative_orientations[3 * i];
		sn->relative_orientation[1] = s.response.sensor_relative_orientations[3 * i + 1];
		sn->relative_orientation[2] = s.response.sensor_relative_orientations[3 * i + 2];
		sn->flattened_position[0] = s.response.sensor_flattened_positions[2 * i];
		sn->flattened_position[1] = s.response.sensor_flattened_positions[2 * i + 1];
		sn->radius = s.response.sensor_radii[i];
		sn->neighbors = skin.p_sensor_neighbors + s.response.sensor_neighbors_begins[i];
		sn->neighbors_count = s.response.sensor_neighbors_counts[i];
		sn->robot_link = s.response.sensor_robot_links[i];
	}

	/* fix up the internal links between the structures */
	for (skin_sensor_type_id i = 0, size = skin.p_sensor_types_count; i < size; ++i)
	{
		skin_sensor_type *st = &skin.p_sensor_types[i];
		st->id = i;
		st->p_object = &skin;
		st->is_active = false;
		for (skin_sensor_id j = st->sensors_begin; j < st->sensors_end; ++j)
			skin.p_sensors[j].sensor_type_id = i;
		for (skin_module_id j = st->modules_begin; j < st->modules_end; ++j)
			skin.p_modules[j].sensor_type_id = i;
		for (skin_patch_id j = st->patches_begin; j < st->patches_end; ++j)
			skin.p_patches[j].sensor_type_id = i;
	}
	for (skin_patch_id i = 0, size = skin.p_patches_count; i < size; ++i)
	{
		skin_patch *p = &skin.p_patches[i];
		p->id = i;
		p->p_object = &skin;
		for (skin_module_id j = p->modules_begin; j < p->modules_end; ++j)
			skin.p_modules[j].patch_id = i;
	}
	for (skin_module_id i = 0, size = skin.p_modules_count; i < size; ++i)
	{
		skin_module *m = &skin.p_modules[i];
		m->id = i;
		m->p_object = &skin;
		for (skin_sensor_id j = m->sensors_begin; j < m->sensors_end; ++j)
			skin.p_sensors[j].module_id = i;
	}
	for (skin_region_id i = 0, size = skin.p_regions_count; i < size; ++i)
	{
		skin_region *r = &skin.p_regions[i];
		r->id = i;
		r->p_object = &skin;
	}
	for (skin_sub_region_id i = 0, size = skin.p_sub_regions_count; i < size; ++i)
	{
		skin_sub_region *sr = &skin.p_sub_regions[i];
		sr->id = i;
		sr->p_object = &skin;
		sr->sensors_begins = skin.p_sub_region_sensors_begins + i * (uint32_t)skin.p_sensor_types_count;
		sr->sensors_ends = skin.p_sub_region_sensors_ends + i * (uint32_t)skin.p_sensor_types_count;
		for (skin_sensor_type_id j = 0, size2 = skin.p_sensor_types_count; j < size2; ++j)
		{
			for (skin_sensor_id k = sr->sensors_begins[j]; k < sr->sensors_ends[j]; ++k)
				skin.p_sensors[k].sub_region_id = i;
		}
	}
	for (skin_sensor_id i = 0, size = skin.p_sensors_count; i < size; ++i)
	{
		skin_sensor *s = &skin.p_sensors[i];
		s->id = i;
		s->p_object = &skin;
		s->global_position[0] = s->relative_position[0];
		s->global_position[1] = s->relative_position[1];
		s->global_position[2] = s->relative_position[2];
		s->global_orientation[0] = s->relative_orientation[0];
		s->global_orientation[1] = s->relative_orientation[1];
		s->global_orientation[2] = s->relative_orientation[2];
	}

	cmd_srv = n.serviceClient<rosskin::Command>("command");

	/* start the acquisition */
	if (period == 0)
	{
		char tmp[50];
		sprintf(tmp, "data_s%u", id);
		req_srv = n.serviceClient<rosskin::Read>(tmp);
	}
	else
	{
		char tmp[50];
		sprintf(tmp, "data_%llu", period);
		internal = new just_for_ros_callback;
		if (internal == NULL)
		{
			ROS_ERROR("rosskin user: out of memory");
			unload();
			return -1;
		}
		((just_for_ros_callback *)internal)->con = this;
		callback = cb;
		user_data = data;
		sub = n.subscribe(tmp, 1, &just_for_ros_callback::receive, (just_for_ros_callback *)internal);
	}

	rosskin::Command c;

	c.request.period = period;
	c.request.id = id;
	c.request.cmd = rosskin::Command::Request::ROSSKIN_START;

	if (!cmd_srv.call(c))
	{
		ROS_WARN("rosskin user: could not setup acquisition");
		unload();
		return -1;
	}
	paused = false;

	ROS_INFO("rosskin user: up and running");

	return 0;
}

void rosskin::Connector::unload()
{
	if (started && ros::isShuttingDown())
		std::cout << "rosskin user: I cannot guarantee a clean exit if ROS is shutting down." << std::endl
			  << "              Make sure you handle the signal yourself and call unload() before ros::shutdown()" << std::endl;
	pause();
	if (started)
	{
		rosskin::Command c;
		c.request.period = period;
		c.request.id = id;
		c.request.cmd = rosskin::Command::Request::ROSSKIN_STOP;
		if (!cmd_srv.call(c))
			ROS_WARN("rosskin user: could not unregister for a clean exit");
	}
	skin.unload();
	cmd_srv = ros::ServiceClient();
	sub = ros::Subscriber();
	req_srv = ros::ServiceClient();
	if (internal)
		delete (just_for_ros_callback *)internal;
	period = 0;
	id = 0;
	callback = NULL;
	user_data = NULL;
	internal = NULL;
	started = false;
	paused = false;
}

int rosskin::Connector::request()
{
	rosskin::Read r;

	if (!req_srv.call(r))
	{
		ROS_WARN("rosskin user: could not request acquisition");
		return -1;
	}

	_fill_data(r.response.d, *this);

	return 0;
}

void rosskin::Connector::pause()
{
	if (paused || !started)
		return;

	rosskin::Command c;

	c.request.period = period;
	c.request.id = id;
	c.request.cmd = rosskin::Command::Request::ROSSKIN_PAUSE;

	if (!cmd_srv.call(c))
		ROS_WARN("rosskin user: could not pause acquisition");
	else
		paused = true;
}

void rosskin::Connector::resume()
{
	if (!paused || !started)
		return;

	rosskin::Command c;

	c.request.period = period;
	c.request.id = id;
	c.request.cmd = rosskin::Command::Request::ROSSKIN_RESUME;

	if (!cmd_srv.call(c))
		ROS_WARN("rosskin user: could not resume acquisition");
	else
		paused = false;
}
