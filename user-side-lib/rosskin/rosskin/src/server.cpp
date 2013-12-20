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

#include <ros/ros.h>
#include <rosskin/Init.h>
#include <rosskin/Command.h>
#include <rosskin/Read.h>
#include <rosskin/Data.h>
#include <skin.h>
#include <list>
#ifndef NDEBUG
# include <cstdio>
#endif

struct reader
{
	/* only if periodic */
	ros::Publisher pub;
	ros::Timer timer;
	bool publisher_created;
	/* only if sporadic */
	ros::ServiceServer srv;
	uint32_t id;
	/* common */
	uint64_t period;
	skin_object skin;
	bool readers_started;
	unsigned int ref_count;		/* how many users */
	unsigned int paused_count;	/* how many of them paused */

	reader(): publisher_created(false), period(0), readers_started(false), ref_count(0), paused_count(0) {}
	~reader() {}
	void publish(const ros::TimerEvent& event);
	bool serve(rosskin::Read::Request &req, rosskin::Read::Response &res);
private:
	/* should not be copied! */
	reader(const reader &);
	void fill_data(rosskin::Data &d);
};

class server
{
private:
	std::list<reader *> readers;
	uint32_t last_id_given;
#ifndef NDEBUG
	void print_readers(const char *prefix, const std::list<reader *>::iterator &cur);
#endif
public:
	ros::NodeHandle *node_handle;
	ros::ServiceServer init_srv;
	ros::ServiceServer cmd_srv;

	server(): last_id_given(1) {}
	~server();

	std::list<reader *>::iterator get_reader(uint64_t period, uint32_t id);
	void del_reader(std::list<reader *>::iterator &it);

	bool init_responder(rosskin::Init::Request &req, rosskin::Init::Response &res);
	bool command_handler(rosskin::Command::Request &req, rosskin::Command::Response &res);
};

#ifndef NDEBUG
void server::print_readers(const char *prefix, const std::list<reader *>::iterator &cur)
{
	if (prefix)
		std::printf("%s\n", prefix);
	std::printf("+-------------+----------------+--------+--------+--------+---------+---------+\n");
	std::printf("|   pointer   |     period     |   id   |   ref  | paused | started | has pub |\n");
	std::printf("+-------------+----------------+--------+--------+--------+---------+---------+\n");
	for (std::list<reader *>::iterator i = readers.begin(), end = readers.end(); i != end; ++i)
		std::printf("| %c%10p | %14llu | %6u | %6u | %6u |   %3s   |   %3s   |\n",
				i == cur?'*':' ', *i, (*i)->period, (*i)->id, (*i)->ref_count, (*i)->paused_count,
				(*i)->readers_started?"yes":"no", (*i)->publisher_created?"yes":"no");
	std::printf("+-------------+----------------+--------+--------+--------+---------+---------+\n");
}
#else
# define print_readers(...) ((void)0)
#endif

server::~server()
{
	for (std::list<reader *>::iterator i = readers.begin(), end = readers.end(); i != end; ++i)
		delete *i;
	readers.clear();
}

std::list<reader *>::iterator server::get_reader(uint64_t period, uint32_t id)
{
	/* sharing sporadic readers is not so trivial */
	if (period == 0 && id == 0)
		return readers.end();

	for (std::list<reader *>::iterator i = readers.begin(), end = readers.end(); i != end; ++i)
		if ((period != 0 && (*i)->period == period)
			|| (period == 0 && (*i)->id == id))
			return i;
	return readers.end();
}

void server::del_reader(std::list<reader *>::iterator &it)
{
	if ((*it)->ref_count > 0)
		--(*it)->ref_count;
	else
		ROS_WARN("del_reader: assertion (ref_count > 0) failed");
	if ((*it)->paused_count > 0)
		--(*it)->paused_count;
	else
		ROS_WARN("del_reader: assertion (paused_count > 0) failed");
	if ((*it)->ref_count == 0)
	{
		delete *it;
		readers.erase(it);
	}
}

bool server::init_responder(rosskin::Init::Request &req, rosskin::Init::Response &res)
{
	reader *r = NULL;
	std::list<reader *>::iterator iter;

	iter = get_reader(req.period, 0);
	if (iter != readers.end())
		r = *iter;
	else
	{
		r = new reader;
		if (r == NULL)
			return false;

		r->period = req.period;
		if (r->skin.load())
		{
			delete r;
			return false;
		}

		r->id = last_id_given++;
	}
	++r->ref_count;
	++r->paused_count;

	try
	{
		skin_object *o = &r->skin;

		res.id = r->id;		/* only meaningful for sporadic users */
		res.sensor_types_count = o->p_sensor_types_count;
		res.sensors_count = o->p_sensors_count;
		res.modules_count = o->p_modules_count;
		res.patches_count = o->p_patches_count;
		res.sub_regions_count = o->p_sub_regions_count;
		res.regions_count = o->p_regions_count;
		res.sub_region_indices_count = o->p_sub_region_indices_count;
		res.region_indices_count = o->p_region_indices_count;
		res.sensor_neighbors_count = o->p_sensor_neighbors_count;

		/* packed helper arrays */
		res.sub_region_sensors_begins.reserve(o->p_sub_region_sensors_begins_count);
		for (uint32_t i = 0, size = o->p_sub_region_sensors_begins_count; i < size; ++i)
			res.sub_region_sensors_begins.push_back(o->p_sub_region_sensors_begins[i]);

		res.sub_region_sensors_ends.reserve(o->p_sub_region_sensors_ends_count);
		for (uint32_t i = 0, size = o->p_sub_region_sensors_ends_count; i < size; ++i)
			res.sub_region_sensors_ends.push_back(o->p_sub_region_sensors_ends[i]);

		res.region_sub_region_indices.reserve(o->p_sub_region_indices_count);
		for (skin_sub_region_index_id i = 0, size = o->p_sub_region_indices_count; i < size; ++i)
			res.region_sub_region_indices.push_back(o->p_sub_region_indices[i]);

		res.sub_region_region_indices.reserve(o->p_region_indices_count);
		for (skin_region_index_id i = 0, size = o->p_region_indices_count; i < size; ++i)
			res.sub_region_region_indices.push_back(o->p_region_indices[i]);

		res.sensor_neighbors.reserve(o->p_sensor_neighbors_count);
		for (uint32_t i = 0, size = o->p_sensor_neighbors_count; i < size; ++i)
			res.sensor_neighbors.push_back(o->p_sensor_neighbors[i]);

		/* sensor types */
		res.sensor_type_names.reserve(o->p_sensor_types_count);
		res.sensor_type_sensors_begins.reserve(o->p_sensor_types_count);
		res.sensor_type_sensors_ends.reserve(o->p_sensor_types_count);
		res.sensor_type_modules_begins.reserve(o->p_sensor_types_count);
		res.sensor_type_modules_ends.reserve(o->p_sensor_types_count);
		res.sensor_type_patches_begins.reserve(o->p_sensor_types_count);
		res.sensor_type_patches_ends.reserve(o->p_sensor_types_count);
		for (skin_sensor_type_id i = 0, size = o->p_sensor_types_count; i < size; ++i)
		{
			skin_sensor_type *st = &o->p_sensor_types[i];
			res.sensor_type_names.push_back(st->name);
			res.sensor_type_sensors_begins.push_back(st->sensors_begin);
			res.sensor_type_sensors_ends.push_back(st->sensors_end);
			res.sensor_type_modules_begins.push_back(st->modules_begin);
			res.sensor_type_modules_ends.push_back(st->modules_end);
			res.sensor_type_patches_begins.push_back(st->patches_begin);
			res.sensor_type_patches_ends.push_back(st->patches_end);
		}

		/* patches */
		res.patch_modules_begins.reserve(o->p_patches_count);
		res.patch_modules_ends.reserve(o->p_patches_count);
		for (skin_patch_id i = 0, size = o->p_patches_count; i < size; ++i)
		{
			skin_patch *p = &o->p_patches[i];
			res.patch_modules_begins.push_back(p->modules_begin);
			res.patch_modules_ends.push_back(p->modules_end);
		}

		/* modules */
		res.module_sensors_begins.reserve(o->p_modules_count);
		res.module_sensors_ends.reserve(o->p_modules_count);
		for (skin_module_id i = 0, size = o->p_modules_count; i < size; ++i)
		{
			skin_module *m = &o->p_modules[i];
			res.module_sensors_begins.push_back(m->sensors_begin);
			res.module_sensors_ends.push_back(m->sensors_end);
		}

		/* regions */
		res.region_sub_region_indices_begins.reserve(o->p_regions_count);
		res.region_sub_region_indices_ends.reserve(o->p_regions_count);
		for (skin_region_id i = 0, size = o->p_regions_count; i < size; ++i)
		{
			skin_region *r = &o->p_regions[i];
			res.region_sub_region_indices_begins.push_back(r->sub_region_indices_begin);
			res.region_sub_region_indices_ends.push_back(r->sub_region_indices_end);
		}

		/* sub-regions */
		res.sub_region_region_indices_begins.reserve(o->p_sub_regions_count);
		res.sub_region_region_indices_ends.reserve(o->p_sub_regions_count);
		for (skin_sub_region_id i = 0, size = o->p_sub_regions_count; i < size; ++i)
		{
			skin_sub_region *sr = &o->p_sub_regions[i];
			res.sub_region_region_indices_begins.push_back(sr->region_indices_begin);
			res.sub_region_region_indices_ends.push_back(sr->region_indices_end);
		}

		/* sensors */
		res.sensor_relative_positions.reserve(o->p_sensors_count * 3);
		res.sensor_relative_orientations.reserve(o->p_sensors_count * 3);
		res.sensor_flattened_positions.reserve(o->p_sensors_count * 2);
		res.sensor_neighbors_begins.reserve(o->p_sensors_count);
		res.sensor_neighbors_counts.reserve(o->p_sensors_count);
		res.sensor_radii.reserve(o->p_sensors_count);
		res.sensor_robot_links.reserve(o->p_sensors_count);
		for (skin_sensor_id i = 0, size = o->p_sensors_count; i < size; ++i)
		{
			skin_sensor *s = &o->p_sensors[i];
			res.sensor_relative_positions.push_back(s->relative_position[0]);
			res.sensor_relative_positions.push_back(s->relative_position[1]);
			res.sensor_relative_positions.push_back(s->relative_position[2]);
			res.sensor_relative_orientations.push_back(s->relative_orientation[0]);
			res.sensor_relative_orientations.push_back(s->relative_orientation[1]);
			res.sensor_relative_orientations.push_back(s->relative_orientation[2]);
			res.sensor_flattened_positions.push_back(s->flattened_position[0]);
			res.sensor_flattened_positions.push_back(s->flattened_position[1]);
			res.sensor_radii.push_back(s->radius);
			res.sensor_neighbors_begins.push_back(s->neighbors - o->p_sensor_neighbors);
			res.sensor_neighbors_counts.push_back(s->neighbors_count);
			res.sensor_robot_links.push_back(s->robot_link);
		}
	}
	catch (...)
	{
		if (iter != readers.end())
			del_reader(iter);
		else
			delete r;
		return false;
	}

	if (iter == readers.end())
		readers.push_front(r);

	ROS_INFO("rosskin server: new user with period %llu and id %u%s",
			r->period, r->period == 0?r->id:0, r->period == 0?"":"(unused)");
	print_readers("", iter);
	return true;
}

bool server::command_handler(rosskin::Command::Request &req, rosskin::Command::Response &res)
{
	int ret = SKIN_SUCCESS;
	std::list<reader *>::iterator iter = get_reader(req.period, req.id);

	print_readers("Received a command", iter);
	if (iter == readers.end())
		return false;

	reader *r = *iter;

	switch (req.cmd)
	{
	case rosskin::Command::Request::ROSSKIN_START:
		if (!r->readers_started)
		{
			ret = r->skin.reader()->start(SKIN_ALL_SENSOR_TYPES,
				(r->period == 0)?SKIN_ACQUISITION_SPORADIC:SKIN_ACQUISITION_PERIODIC, r->period);
			if (ret != SKIN_SUCCESS && ret != SKIN_PARTIAL)
			{
				ROS_WARN("rosskin server: could not start readers");
				return false;
			}
		}
		r->readers_started = true;
		if (r->period == 0)
		{
			char tmp[50];
			sprintf(tmp, "data_s%u", r->id);
			r->srv = node_handle->advertiseService(tmp, &reader::serve, r);
		}
		else if (!r->publisher_created)
		{
			char tmp[50];
			sprintf(tmp, "data_%llu", r->period);
			r->pub = node_handle->advertise<rosskin::Data>(tmp, 1);
			r->timer = node_handle->createTimer(ros::Duration(r->period / 1000000000, r->period % 1000000000),
					&reader::publish, r);
			r->publisher_created = true;
		}
		ROS_INFO("rosskin server: start acquisition for user with period %llu and id %u%s%s",
			r->period, r->period == 0?r->id:0, r->period == 0?"":"(unused)",
			ret == SKIN_PARTIAL?" (Partial)":"");
		if (r->paused_count > 0)
			--r->paused_count;
		else
			ROS_WARN("Given start command: assertion (paused_count > 0) failed");
		break;
	case rosskin::Command::Request::ROSSKIN_STOP:
		/*
		 * note: before stop, send a pause command, so that book-keeping on paused_count
		 * would be correct.
		 */
		ROS_INFO("rosskin server: stop acquisition for user with period %llu and id %u%s",
			r->period, r->period == 0?r->id:0, r->period == 0?"":"(unused)");
		del_reader(iter);
		break;
	case rosskin::Command::Request::ROSSKIN_PAUSE:
		/*
		 * note: user module should not send pause command when already paused
		 * and likewise with the resume command
		 */
		ROS_INFO("rosskin server: pause acquisition for user with period %llu and id %u%s",
			r->period, r->period == 0?r->id:0, r->period == 0?"":"(unused)");
		if (r->paused_count >= r->ref_count)
			ROS_WARN("Given pause command: assertion (paused_count < ref_count) failed");
		else
			++r->paused_count;
		if (r->paused_count == r->ref_count)
			r->skin.reader()->pause();
		break;
	case rosskin::Command::Request::ROSSKIN_RESUME:
		ROS_INFO("rosskin server: resume acquisition for user with period %llu and id %u%s",
			r->period, r->period == 0?r->id:0, r->period == 0?"":"(unused)");
		if (r->paused_count <= 0)
			ROS_WARN("Given resume command: assertion (paused_count > 0) failed");
		else
			--r->paused_count;
		r->skin.reader()->resume();
		break;
	default:
		return false;
	}

	return true;
}

void reader::publish(const ros::TimerEvent& event)
{
	rosskin::Data d;

	fill_data(d);
	pub.publish(d);
}

bool reader::serve(rosskin::Read::Request &req, rosskin::Read::Response &res)
{
	skin.reader()->request();

	fill_data(res.d);
	return true;
}

void reader::fill_data(rosskin::Data &d)
{
	d.sensor_type_is_actives.reserve(skin.p_sensor_types_count);
	for (skin_sensor_type_id i = 0, size = skin.p_sensor_types_count; i < size; ++i)
		d.sensor_type_is_actives.push_back(skin.p_sensor_types[i].is_active);

	d.sensor_responses.reserve(skin.p_sensors_count);
	for (skin_sensor_id i = 0, size = skin.p_sensors_count; i < size; ++i)
		d.sensor_responses.push_back(skin.p_sensors[i].get_response());
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "rosskin_server");
	ros::NodeHandle n;
	server s;

	s.init_srv = n.advertiseService("init", &server::init_responder, &s);
	s.cmd_srv = n.advertiseService("command", &server::command_handler, &s);
	s.node_handle = &n;

	ROS_INFO("rosskin server: up and running");

	ros::spin();

	return 0;
}
