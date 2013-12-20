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

#include "filter.h"

using namespace std;

filter::filter()
{
	type = FILTER_AVERAGE;
	size = 1;
}

filter::filter(filter_type t)
{
	type = t;
	size = 1;
}

filter::filter(unsigned int s)
{
	type = FILTER_AVERAGE;
	size = s;
}

filter::filter(filter_type t, unsigned int s)
{
	type = t;
	size = s;
}

filter::~filter()
{
}

void filter::change_type(filter_type t)
{
	type = t;
}

void filter::change_size(unsigned int s)
{
	responses.clear();
	size = s;
}

void filter::new_responses(const uint8_t *r, unsigned int count)
{
	++current;
	if (current >= size)
		current = 0;
	vector<uint8_t> v(r, r + count);
	if (responses.size() < size)
		while (responses.size() < size)
			responses.push_back(v);
	else
		responses[current] = v;
}

void filter::new_responses(const vector<uint8_t> &v)
{
	++current;
	if (current >= size)
		current = 0;
	if (responses.size() < size)
		while (responses.size() < size)
			responses.push_back(v);
	else
	{
		unsigned int sz = v.size();
		if (responses[current].size() < sz)
			sz = responses[current].size();
		for (unsigned int i = 0; i < sz; ++i)
			responses[current][i] = v[i];
	}
}

struct store_responses_data
{
	vector<uint8_t> v;
	unsigned int type_sensors_begin;
};

static int _store_responses(skin_sensor *s, void *d)
{
	store_responses_data *data = (store_responses_data *)d;

	data->v[s->id - data->type_sensors_begin] = (uint32_t)skin_sensor_get_response(s) * 255 / SKIN_SENSOR_RESPONSE_MAX;
	return SKIN_CALLBACK_CONTINUE;
}


void filter::new_responses(skin_object &o)
{
	store_responses_data d;
	d.v.resize(skin_object_sensors_count(&o));
	d.type_sensors_begin = 0;

	skin_object_for_each_sensor(&o, _store_responses, &d);
	if (responses.size() < size)
		while (responses.size() < size)
			responses.push_back(d.v);
	else
		responses[current] = d.v;
}

void filter::new_responses(skin_sensor_type &t)
{
	store_responses_data d;
	d.v.resize(skin_sensor_type_sensors_count(&t));
	d.type_sensors_begin = t.sensors_begin;

	skin_sensor_type_for_each_sensor(&t, _store_responses, &d);
	if (responses.size() < size)
		while (responses.size() < size)
			responses.push_back(d.v);
	else
		responses[current] = d.v;
}

uint8_t filter::get_response(skin_sensor_id id) const
{
	unsigned int result = 0;
	unsigned int count = 0;
	for (unsigned int i = 0; i < size; ++i)
	{
		unsigned int c = current + i + 1;
		if (c >= size)
			c -= size;
		if (c >= responses.size())
			continue;
		switch (type)
		{
			default:
			case FILTER_AVERAGE:
				result += responses[c][id];
				++count;
				break;
		}
	}
	switch (type)
	{
		default:
		case FILTER_AVERAGE:
			if (count == 0)
				result = 0;
			else
				result /= count;
			break;
	}
	return result;
}

void filter::get_responses(vector<uint8_t> &result) const
{
	if (size == 0)
		return;
	unsigned int sz = result.size();
	if (responses[0].size() < sz)
		sz = responses[0].size();
	for (unsigned int i = 0, s = responses[0].size(); i < s; ++i)
		result[i] = get_response(i);
}

vector<uint8_t> filter::get_responses() const
{
	if (size == 0)
		return vector<uint8_t>();
	vector<uint8_t> res(responses[0].size());
	get_responses(res);
	return res;
}
