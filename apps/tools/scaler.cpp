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

#include <algorithm>
#include "scaler.h"

using namespace std;

scaler::scaler()
{
	min_range = 500;
}

scaler::scaler(struct skin *skin)
{
	initialize(skin_sensor_count(skin));
	min_range = 500;
}

scaler::scaler(unsigned int r)
{
	set_range(r);
}

scaler::scaler(struct skin *skin, unsigned int r)
{
	initialize(skin_sensor_count(skin));
	set_range(r);
}

scaler::~scaler()
{
}

void scaler::initialize(unsigned int count)
{
	mins.resize(count);
	fill(mins.begin(), mins.end(), SKIN_SENSOR_RESPONSE_MAX);
	maxes.resize(count);
	fill(maxes.begin(), maxes.end(), 0);
	do_scale.resize(count);
	fill(do_scale.begin(), do_scale.end(), true);
}

struct set_affect_data
{
	skin_sensor_id sensor;
	scaler *s;
};

static int _set_scaler_affect(struct skin_sensor *s, void *user_data)
{
	struct set_affect_data *data = (struct set_affect_data *)user_data;

	if (data->s->do_scale.size() <= data->sensor)
		return SKIN_CALLBACK_STOP;
	data->s->do_scale[data->sensor++] = s->type == SKIN_SENSOR_TYPE_CYSKIN_TAXEL
			|| s->type == SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL
			|| s->type == SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL;

	return SKIN_CALLBACK_CONTINUE;
}

void scaler::affect(struct skin *s)
{
	struct set_affect_data d = { 0, this };
	skin_for_each_sensor(s, _set_scaler_affect, &d);
}

void scaler::set_range(unsigned int r)
{
	if (r == 0)
		r = 1;
	min_range = r;
}

void scaler::scale(const skin_sensor_response *r, unsigned int count, vector<uint8_t> &result)
{
	if (result.size() < count)
		count = result.size();
	for (unsigned int i = 0; i < count; ++i)
		result[i] = r[i];
}

vector<uint8_t> scaler::scale(const skin_sensor_response *r, unsigned int count)
{
	vector<skin_sensor_response> v(r, r + count);
	return scale(v);
}

static uint8_t _fix_response(scaler *s, unsigned int i, unsigned int response)
{
	if (response < /*800*/1)		/* the first time could be 0 while the skinware actually starts reading */
		return 0;

	if (s->do_scale[i])
	{
		if (response < s->mins[i])
			s->mins[i] = response;
		if (response > s->maxes[i])
			s->maxes[i] = response;
	}
	else
	{
		s->mins[i] = 0;
		s->maxes[i] = SKIN_SENSOR_RESPONSE_MAX;
	}
	unsigned int rmin = s->mins[i];
	unsigned int rmax = s->maxes[i];
	response -= rmin;
	unsigned int range = rmax - rmin;
	if (range < s->min_range)
		range = s->min_range;
	response = response * 255 / range;
	return (uint8_t)response;
}

struct fix_responses_data
{
	scaler *sclr;
	vector<uint8_t> &result;
	unsigned int size;
	skin_sensor_id type_sensors_begin;
};

static int _fix_responses(skin_sensor *s, void *d)
{
	fix_responses_data *data = (fix_responses_data *)d;
	unsigned int response;
	unsigned int i;

	response = skin_sensor_get_response(s);
	i = s->id - data->type_sensors_begin;
	if (i < data->size)
		data->result[i] = _fix_response(data->sclr, i, response);

	return SKIN_CALLBACK_CONTINUE;
}

void scaler::scale(const vector<skin_sensor_response> &v, vector<uint8_t> &result)
{
	unsigned int size = v.size();
	if (result.size() < size)
		size = result.size();
	if (size > mins.size())
		return;		// bad initialization.  Use scale(object) once before getting to real-time context
	for (unsigned int i = 0; i < size; ++i)
	{
		unsigned int response = v[i];
		result[i] = _fix_response(this, i, response);
	}
}

vector<uint8_t> scaler::scale(const vector<skin_sensor_response> &v)
{
	vector<uint8_t> res(v.size(), 0);
	if (v.size() > mins.size())
		initialize(v.size());
	scale(v, res);
	return res;
}

void scaler::scale(struct skin *skin, vector<uint8_t> &result)
{
	unsigned int size = skin_sensor_count(skin);
	if (result.size() < size)
		size = result.size();
	fix_responses_data data = { this, result, size, 0 };
	skin_for_each_sensor(skin, _fix_responses, &data);
}

struct store_responses_data
{
	vector<skin_sensor_response> v;
	unsigned int type_sensors_begin;
};

static int _store_responses(skin_sensor *s, void *d)
{
	store_responses_data *data = (store_responses_data *)d;

	data->v[s->id - data->type_sensors_begin] = skin_sensor_get_response(s);
	return SKIN_CALLBACK_CONTINUE;
}

vector<uint8_t> scaler::scale(struct skin *skin)
{
	store_responses_data d;
	d.v.resize(skin_sensor_count(skin));
	d.type_sensors_begin = 0;

	skin_for_each_sensor(skin, _store_responses, &d);
	return scale(d.v);
}

void scaler::dampen(unsigned int amount)
{
#if 0
	if (amount > SKIN_SENSOR_RESPONSE_MAX)
		amount = SKIN_SENSOR_RESPONSE_MAX;
#else
	if (amount > 100)
		amount = 100;
#endif
	for (unsigned int i = 0, size = mins.size(); i < size; ++i)
	{
		if (!do_scale[i])
			continue;
#if 0
		// linearly change the mins and maxes
		if (SKIN_SENSOR_RESPONSE_MAX - amount > mins[i])
			mins[i] += amount;
		else
			mins[i] = SKIN_SENSOR_RESPONSE_MAX;
		if (maxes[i] > amount)
			maxes[i] -= amount;
		else
			maxes[i] = 0;
#else
		// change the mins and maxes by a percentage of their range
		if (mins[i] < maxes[i])
		{
			unsigned int range = maxes[i] - mins[i];
			unsigned int to_cut = range * amount / 100;
			if (to_cut < amount)
			{
				to_cut = amount;
				if (to_cut > range)
					to_cut = range;
			}
			if (to_cut == 0)
				to_cut = 1;
			mins[i] += to_cut;
			maxes[i] -= to_cut;
		}
	}
#endif
}

void scaler::reset()
{
	initialize(mins.size());
}
