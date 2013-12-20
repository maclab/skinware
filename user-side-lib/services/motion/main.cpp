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

#include <iostream>
#include <fstream>
#include <cmath>
#include <map>
#include <signal.h>
#include <skin.h>
#include <scaler.h>
#include <filter.h>
#include <amplifier.h>
#include <vecmath.h>
#include "skin_user_motion.h"

using namespace std;

ofstream				fout("log.out");

/* skin */
static skin_object			skin;
static skin_reader			*sr;
static skin_service_manager		*sm;

/* processing */
static bool				raw_results		= false;	// if raw results, all do_* will be ignored
static bool				do_scale		= true;
static bool				do_dampen		= true;
static bool				do_filter		= true;
static bool				do_amplify		= true;
static int				filter_size		= 3;
static unsigned int			damp_size		= 4;
static amplifier			amp(-255, -255, 255);
struct layer_processed
{
	vector<uint8_t>	responses;
	uint64_t	last_read_count;
	scaler		sclr;
	filter		fltr;
};
static vector<layer_processed>		processed_layers;
static map<skin_sensor_id, vector<skin_sensor_id> >		// maps a blacklisted id to a vector of its replacements
					blacklist;

/* motion service */
static vector<uint8_t>			previous_responses;
static unsigned int			service_frequency	= 5;
static unsigned int			motion_threshold	= 0;
static unsigned int			service_id		= SKIN_INVALID_ID;

/* other */
static volatile sig_atomic_t		must_exit		= 0;
static int				error_code		= EXIT_SUCCESS;

void sighandler(int signum)
{
	must_exit = 1;
	error_code = EXIT_SUCCESS;
}

void fatal_error(const char *msg, int error)
{
	must_exit = 1;
	cout << msg << ".  Error: " << error << endl;
	error_code = error;
}

skin_sensor_response get_response(skin_sensor *s)
{
	if (blacklist.find(s->id) == blacklist.end())
		return skin_sensor_get_response(s);
	vector<skin_sensor_id> replacement = blacklist[s->id];
	skin_sensor *sensors = skin_object_sensors(&skin, NULL);
	unsigned int avg = 0;
	for (unsigned int i = 0, size = replacement.size(); i < size; ++i)
		avg += skin_sensor_get_response(&sensors[replacement[i]]);
	avg /= replacement.size();
	s->response = avg;
	return avg;
}

struct save_response_data
{
	vector<skin_sensor_response> temp;
	skin_sensor_id type_sensors_begin;
};

static int _save_raw_response(skin_sensor *s, void *d)
{
	save_response_data *data = (save_response_data *)d;

	processed_layers[s->sensor_type_id].responses[s->id - data->type_sensors_begin] = get_response(s) >> 8;
	return SKIN_CALLBACK_CONTINUE;
}

static int _save_temp_response(skin_sensor *s, void *d)
{
	save_response_data *data = (save_response_data *)d;

	data->temp[s->id - data->type_sensors_begin] = get_response(s);
	return SKIN_CALLBACK_CONTINUE;
}

static int _save_responses(skin_sensor_type *st, void *d)
{
	uint64_t num_read = sr->tasks_statistics[st->id].number_of_reads;
	if (processed_layers[st->id].last_read_count == num_read)
		return SKIN_CALLBACK_CONTINUE;
	processed_layers[st->id].last_read_count = num_read;
	save_response_data data;
	data.type_sensors_begin = st->sensors_begin;
	if (raw_results || !do_scale)
		skin_sensor_type_for_each_sensor(st, _save_raw_response, &data);
	else
	{
		data.temp.resize(skin_sensor_type_sensors_count(st));
		skin_sensor_type_for_each_sensor(st, _save_temp_response, &data);
		processed_layers[st->id].responses = processed_layers[st->id].sclr.scale(data.temp);
	}
	if (raw_results)
		return SKIN_CALLBACK_CONTINUE;
	if (do_filter)
	{
		processed_layers[st->id].fltr.new_responses(processed_layers[st->id].responses);
		processed_layers[st->id].fltr.get_responses(processed_layers[st->id].responses);
	}
	if (do_amplify)
		amp.amplify(processed_layers[st->id].responses, processed_layers[st->id].responses);
	return SKIN_CALLBACK_CONTINUE;
}

void update_responses()
{
	if (!raw_results && do_dampen)
		for (vector<layer_processed>::iterator i = processed_layers.begin(), end = processed_layers.end(); i != end; ++i)
			i->sclr.dampen(damp_size);
	skin_object_for_each_sensor_type(&skin, _save_responses, NULL);
}

struct from_to_data
{
	float from[3];
	float to[3];
	int from_count;
	int to_count;
};

static int _calculate_from_to(skin_sensor *s, void *d)
{
	from_to_data *data = (from_to_data *)d;

	uint8_t response = processed_layers[s->sensor_type_id].responses[s->id - skin_sensor_sensor_type(s)->sensors_begin];
	uint8_t prev = previous_responses[s->id];
	if (response < prev && (int)response < (int)prev - (int)motion_threshold)
	{
		++data->from_count;
		ADD(data->from, data->from, s->global_position);
	}
	if (response > prev && (int)response > (int)prev + (int)motion_threshold)
	{
		++data->to_count;
		ADD(data->to, data->to, s->global_position);
	}
	previous_responses[s->id] = response;

	return SKIN_CALLBACK_CONTINUE;
}

void recalculate_motion(skin_user_motion *m)
{
	from_to_data data = { {0, 0, 0}, {0, 0, 0}, 0, 0};
	skin_object_for_each_sensor(&skin, _calculate_from_to, &data);
	if (data.to_count != 0 && data.from_count != 0)
	{
		MUL(data.from, data.from, 1000000000);
		MUL(data.to, data.to, 1000000000);
		DIV(data.from, data.from, data.from_count);
		DIV(data.to, data.to, data.to_count);
		m[0].detected = 1;
		ASSIGN(m[0].to, data.to);
		ASSIGN(m[0].from, data.from);
	}
	else
		m[0].detected = 0;
}

void detect_motion(void *mem, void *data)
{
	update_responses();
	recalculate_motion((skin_user_motion *)mem);
}

static int _init_filters(skin_sensor_type *st, void *d)
{
	processed_layers[st->id].responses.resize(skin_sensor_type_sensors_count(st));
	processed_layers[st->id].last_read_count = 0;
	processed_layers[st->id].fltr.change_size(filter_size);
	processed_layers[st->id].sclr.set_range(512);
	processed_layers[st->id].sclr.scale(*st);
	processed_layers[st->id].fltr.new_responses(*st);

	return SKIN_CALLBACK_CONTINUE;
}

int main(int argc, char **argv)
{
	fout << "Initializing..." << endl;
	for (int i = 1; i < argc; ++i)
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			cout << "Usage: " << argv[0] << " [options]\n\n"
				"Options:\n"
				"--help or -h\n"
				"\tShow this help message\n"
				"--period p\n"
				"\tSet period of acquisition and motion detection (default 5 (Hz))\n"
				"--threshold t\n"
				"\tSet threshold of motion detection (default below)\n"
				"\t\t500: if using raw responses or not scaling,\n"
				"\t\t40: if dampening response ranges,\n"
				"\t\t60: otherwise\n"
				"--raw\n"
				"\tUse raw responses (default: no)\n"
				"--no-scale\n"
				"\tDon't scale responses (default: yes)\n"
				"--no-filter\n"
				"\tDon't filter responses (default: yes)\n"
				"--no-amplify\n"
				"\tDon't amplify responses (default: yes)\n"
				"--no-dampen\n"
				"\tDon't dampen response ranges (default: yes)\n"
				"--filter-size f\n"
				"\tUse this window for an averaging filter (default: 3)\n"
				"--damp-size d\n"
				"\tDampen computed response range by this percent per period (default: 6 (%))\n"
				"\tOnly meaningful when scaling\n\n"
				"This program will spawn a simple motion detection service.  It also scales,\n"
				"filters and amplifies the values to account for different range of values due\n"
				"different skin technology and problems such as hysteresis.\n\n"
				"The service takes a diff between sensor responses of consecutive frames and\n"
				"identifies sensors whose responses have increased or decreased by at least `t`\n"
				"as taken from `--threshold` option.  The motion is then identified as from the\n"
				"average of positions where the responses have decreased to the average of\n"
				"positions where the responses have been increased.\n\n"
				"To terminate the service, use CTRL+C or send SIGTERM to this process." << endl << endl;
			return 0;
		}
		else if (strcmp(argv[i], "--period") == 0)
		{
			++i;
			if (i < argc)
			{
				sscanf(argv[i], "%u", &service_frequency);
				if (service_frequency < 1)
				{
					cout << "Invalid frequency " << service_frequency << ".  Defaulting to 1Hz" << endl;
					service_frequency = 1;
				}
			}
			else
				cout << "Missing value after --period" << endl;
		}
		else if (strcmp(argv[i], "--threshold") == 0)
		{
			++i;
			if (i < argc)
				sscanf(argv[i], "%u", &motion_threshold);
			else
				cout << "Missing value after --threshold" << endl;
		}
		else if (strcmp(argv[i], "--filter-size") == 0)
		{
			++i;
			if (i < argc)
			{
				sscanf(argv[i], "%u", &filter_size);
				if (filter_size < 1)
				{
					cout << "Invalid filter size " << filter_size << ".  Disabling filtering" << endl;
					do_filter = false;;
					filter_size = 1;
				}
			}
			else
				cout << "Missing value after --filter-size" << endl;
		}
		else if (strcmp(argv[i], "--damp-size") == 0)
		{
			++i;
			if (i < argc)
			{
				sscanf(argv[i], "%u", &damp_size);
				if (damp_size < 1)
				{
					cout << "Invalid damp size " << damp_size << ".  Disabling damping" << endl;
					do_dampen = false;;
					damp_size = 1;
				}
			}
			else
				cout << "Missing value after --damp-size" << endl;
		}
		else if (strcmp(argv[i], "--raw") == 0)
			raw_results = true;
		else if (strcmp(argv[i], "--no-scale") == 0)
			do_scale = false;
		else if (strcmp(argv[i], "--no-filter") == 0)
			do_filter = false;
		else if (strcmp(argv[i], "--no-amplify") == 0)
			do_amplify = false;
		else if (strcmp(argv[i], "--no-dampen") == 0)
			do_dampen = false;
		else
			cout << "Invalid option " << argv[i] << endl;
	if (motion_threshold == 0)
	{
		if (raw_results || !do_scale)
			motion_threshold = 500;
		else if (do_dampen)
			motion_threshold = 30;
		else
			motion_threshold = 60;
	}
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	ifstream blin("blacklist");
	if (blin.is_open())
	{
		skin_sensor_id i;
		unsigned int n;
		skin_sensor_size sensors_count = skin_object_sensors_count(&skin);
		vector<skin_sensor_id> replacement;
		while (blin >> i >> n)
		{
			skin_sensor_id r;
			replacement.resize(n);
			for (unsigned int s = 0; s < n; ++s)
			{
				blin >> r;
				if (r < sensors_count)
					replacement[s] = r;
			}
			if (replacement.size() > 0)
				blacklist[i] = replacement;
		}
		blin.close();
	}
	skin_object_init(&skin);
	if (skin_object_load(&skin) != SKIN_SUCCESS)
	{
		cout << "Could not initialize skin" << endl;
		skin_object_free(&skin);
		return EXIT_FAILURE;
	}
	previous_responses.resize(skin_object_sensors_count(&skin));
	sr = skin_object_reader(&skin);
	sm = skin_object_service_manager(&skin);
	skin_reader_start(sr, SKIN_ALL_SENSOR_TYPES, SKIN_ACQUISITION_PERIODIC, 1000000000 / service_frequency);
	skin_sensor_type_size types_count = skin_object_sensor_types_count(&skin);
	processed_layers.resize(types_count);
	skin_object_for_each_sensor_type(&skin, _init_filters, NULL);
	fout << "skin done (version: "SKIN_VERSION")" << endl;
	int ret = skin_service_manager_initialize_periodic_service(sm, SKIN_USER_MOTION_SHMEM, sizeof(skin_user_motion), 1,
			SKIN_USER_MOTION_RWLOCK, 1000000000 / service_frequency);
	if (ret < 0)
		fatal_error("Could not initialize service", ret);
	else
	{
		service_id = ret;
		if ((ret = skin_service_manager_start_service(sm, service_id, detect_motion, NULL)) != SKIN_SUCCESS)
			fatal_error("Could not start service", ret);
	}
	while (!must_exit)
		usleep(100000);
	skin_service_manager_stop_service(sm, service_id);
	skin_object_free(&skin);
	return error_code;
}
