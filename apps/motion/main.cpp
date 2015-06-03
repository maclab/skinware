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

#define URT_LOG_PREFIX "calibrator: "
#include <map>
#include <set>
#include <scaler.h>
#include <filter.h>
#include <amplifier.h>
#include <skin.hpp>
#include <skin_calibrator.h>
#include "skin_motion.h"

using namespace std;

URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("Example Motion Service:\n"
			"\t\t\t\tThe service takes a diff between sensor responses of consecutive frames and\n"
			"\t\t\t\tidentifies sensors whose responses have increased or decreased by at least `t`\n"
			"\t\t\t\tas taken from `threshold` option.  The motion is then identified as from the\n"
			"\t\t\t\taverage of positions where the responses have decreased to the average of\n"
			"\t\t\t\tpositions where the responses have been increased.\n\n");

static unsigned int frequency = 5;
static unsigned int threshold = 0;

static bool raw = false;	/* if raw results, all do_* will be ignored */
static bool do_scale = true;
static bool do_dampen = true;
static bool do_filter = true;
static bool do_amplify = true;
static unsigned int damp_size = 4;
static unsigned int filter_size = 3;

static char *name = NULL;
static char *calibrator = NULL;

URT_MODULE_PARAM_START()
URT_MODULE_PARAM(frequency, uint, "Set period of acquisition and motion detection (default: 5 (Hz))")
URT_MODULE_PARAM(threshold, uint, "Set threshold of motion detection (default: 500 (if raw), 30 (if dampening) or 60 (otherwise))")
URT_MODULE_PARAM(raw, bool, "Use raw responses (default: no)")
URT_MODULE_PARAM(do_scale, bool, "Scale responses (if not raw) (default: yes)")
URT_MODULE_PARAM(do_dampen, bool, "Dampen response ranges (if not raw and scaling) (default: yes)")
URT_MODULE_PARAM(do_filter, bool, "Filter responses (if not raw) (default: yes)")
URT_MODULE_PARAM(do_amplify, bool, "Amplify responses (if not raw) (default: yes)")
URT_MODULE_PARAM(damp_size, uint, "Dampen computed response range by this percent per period (if scaling) (default: 4 (%))")
URT_MODULE_PARAM(filter_size, uint, "Size of filter (if filtering) (default: 3 (samples))")
URT_MODULE_PARAM(name, charp, "Motion detection service name.  Default value is 'MD'")
URT_MODULE_PARAM(calibrator, charp, "Calibrator service name to attach to.  Default value is 'CAL'")
URT_MODULE_PARAM_END()

class data
{
public:
	Skin skin;

	/* processing */
	amplifier amp;
	scaler sclr;
	filter fltr;
	vector<uint8_t> responses;
	vector<SkinSensorResponse> temp_responses;

	/* map a blacklisted id to a vector of its replacements */
	map<SkinSensorUniqueId, set<SkinSensorUniqueId> > blacklist;

	/* motion service */
	vector<uint8_t> previous_responses;
	SkinWriter motion_service;
	bool prev_valid;

	/* calibrator */
	SkinReader calibrator_service;
	urt_task *calibrator_task;
	bool do_calibrate;
	bool done_calibrate;

	/* aux */
	SkinSensorId cur;

	data(): prev_valid(false), calibrator_task(NULL), do_calibrate(false), done_calibrate(false) {}
};

static int start(struct data *d);
static void body(struct data *d);
static void stop(struct data *d);

URT_GLUE(start, body, stop, struct data, interrupted, done)

struct sensor_extra_data
{
	skin_sensor_calibration_info calib_info;
	sensor_extra_data(): calib_info({0}) {}
};

static void sensor_init_hook(SkinSensor s)
{
	s.setUserData(new struct sensor_extra_data);
}

static void sensor_clean_hook(SkinSensor s)
{
	delete (struct sensor_extra_data *)s.getUserData();
}

static struct skin_sensor_calibration_info *get_calib_info(struct skin_sensor *s, void *user_data)
{
	struct sensor_extra_data *extra_data = (struct sensor_extra_data *)s->user_data;
	return &extra_data->calib_info;
}

static void calibrate(SkinReader &reader, void *mem, size_t size)
{
	skin_calibrate(reader.getSkin().getSkin(), mem, get_calib_info);
}

static SkinSensorResponse get_response(SkinSensor &s, struct data *d)
{
	/* if not in blacklist, return its response */
	auto i = d->blacklist.find(s.getUid());
	if (i == d->blacklist.end())
		return s.getResponse();

	set<SkinSensorUniqueId> &replacement = i->second;
	unsigned int sum = 0;
	unsigned int count = 0;

	d->skin.forEachSensor([&](SkinSensor b)
			{
				auto j = replacement.find(b.getUid());
				if (j == replacement.end())
					return SKIN_CALLBACK_CONTINUE;
				sum += b.getResponse();
				++count;
				return SKIN_CALLBACK_CONTINUE;
			});

	return count > 0?sum / count:s.getResponse();
}

static void save_responses(struct data *data)
{
	SkinSensorId cur = 0;
	if (raw || !do_scale)
		data->skin.forEachSensor([&](SkinSensor s)
				{
					data->responses[cur++] = get_response(s, data) >> 8;
					return SKIN_CALLBACK_CONTINUE;
				});
	else
	{
		data->skin.forEachSensor([&](SkinSensor s)
				{
					data->temp_responses[cur++] = get_response(s, data);
					return SKIN_CALLBACK_CONTINUE;
				});
		data->sclr.scale(data->temp_responses, data->responses);
	}
	if (raw)
		return;
	if (do_filter)
	{
		data->fltr.new_responses(data->responses);
		data->fltr.get_responses(data->responses);
	}
	if (do_amplify)
		data->amp.amplify(data->responses, data->responses);
}

static void update_responses(struct data *d)
{
	if (!raw && do_dampen)
		d->sclr.dampen(damp_size);
	save_responses(d);
}

struct from_to_data
{
	struct data *data;
	float from[3];
	float to[3];
	int from_count;
	int to_count;
};

static void _calculate_from_to(SkinSensor &s, struct from_to_data *ft)
{
	struct data *data = ft->data;
	struct sensor_extra_data *extra = (struct sensor_extra_data *)s.getUserData();

	uint8_t response = data->responses[data->cur];
	if ((s.getType() == SKIN_SENSOR_TYPE_CYSKIN_TAXEL
			|| s.getType() == SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL
			|| s.getType() == SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL)
		&& data->prev_valid && extra && extra->calib_info.calibrated)
	{
		uint8_t prev = data->previous_responses[data->cur];
		if (response < prev && (int)response < (int)prev - threshold)
		{
			++ft->from_count;
			for (int i = 0; i < 3; ++i)
				ft->from[i] += extra->calib_info.position_nm[i];
		}
		if (response > prev && (int)response > (int)prev + threshold)
		{
			++ft->to_count;
			for (int i = 0; i < 3; ++i)
				ft->to[i] += extra->calib_info.position_nm[i];
		}
	}

	data->previous_responses[data->cur] = response;
	++data->cur;
}

void recalculate_motion(skin_motion *m, struct data *d)
{
	struct from_to_data data = { d, {0, 0, 0}, {0, 0, 0}, 0, 0};

	d->cur = 0;
	d->skin.forEachSensor([&](SkinSensor s)
			{
				_calculate_from_to(s, &data);
				return SKIN_CALLBACK_CONTINUE;
			});

	if (d->prev_valid && data.to_count != 0 && data.from_count != 0)
	{
		for (int i = 0; i < 3; ++i)
		{
			data.from[i] *= 1000000000;
			data.to[i] *= 1000000000;
			data.from[i] /= data.from_count;
			data.to[i] /=  data.to_count;
			m->to[i] = data.to[i];
			m->from[i] = data.from[i];
		}
		m->detected = 1;
	}
	else
		m->detected = 0;

	d->prev_valid = true;
}

static int detect_motion(SkinWriter &writer, void *mem, size_t size, struct data *d)
{
	update_responses(d);
	recalculate_motion((skin_motion *)mem, d);
	return 0;
}

static void init_filters(struct data *d)
{
	SkinSensorSize s = d->skin.sensorCount();
	d->responses.resize(s);
	d->temp_responses.resize(s);
	d->previous_responses.resize(s);
	d->prev_valid = false;

	d->fltr.change_size(filter_size);
	d->sclr.set_range(512);
	d->sclr.scale(d->skin.getSkin());
	d->sclr.affect(d->skin.getSkin());
	d->fltr.new_responses(d->skin.getSkin());
	d->amp = amplifier(0, 3, 50.182974, -63.226586);
	d->amp.affect(d->skin.getSkin());
}

static void populate_blacklist(struct data *d)
{
	FILE *blin = fopen("blacklist", "r");
	if (blin == NULL)
		return;

	unsigned long long i;
	unsigned int n;

	while (fscanf(blin, "%llu %u", &i, &n) == 2)
	{
		unsigned long long r;
		set<SkinSensorUniqueId> replacement;

		for (unsigned int s = 0; s < n; ++s)
		{
			if (fscanf(blin, "%llu", &r) != 1)
				continue;
			if (r != i)
				replacement.insert(r);
		}

		if (replacement.size() > 0)
			d->blacklist[i] = replacement;
	}

	fclose(blin);
}

static void calibration_request(urt_task *task, void *user_data)
{
	data *d = (data *)user_data;

	while (!interrupted)
	{
		if (d->do_calibrate)
		{
			d->do_calibrate = false;
			d->calibrator_service.request(&interrupted);
			d->done_calibrate = true;
		}

		urt_sleep(1000000);
	}
}

static void loop_update_skin(struct data *d)
{
	bool warned = false;
	SkinWriterAttr attr(sizeof(struct skin_motion), 3, name?name:"MD");

	while (!interrupted)
	{
		urt_task_attr taskattr = { .period = 1000000000 / frequency };
		bool changed = d->skin.update(taskattr) == 0;
		d->skin.resume();

		/* if users have been updated, stop the service and try to restart it */
		if (changed)
		{
			if (d->motion_service.isValid())
			{
				d->skin.remove(d->motion_service);
				d->motion_service = SkinWriter();
				warned = false;
			}

			/* reset the processings */
			init_filters(d);

			/* update calibration */
			d->do_calibrate = true;
			while (!interrupted && !d->done_calibrate)
				urt_sleep(1000000);
			d->done_calibrate = false;
		}

		/* if motion_service is stopped try to start it */
		if (!d->motion_service.isValid())
		{
			d->motion_service = d->skin.add(attr, taskattr, SkinWriterCallbacks([=](SkinWriter &w, void *m, size_t s)
						{
							return detect_motion(w, m, s, d);
						}));

			if (d->motion_service.isValid())
				urt_out("note: service is up\n");
			else if (!warned)
				urt_out("note: service name '%s' is busy.  Waiting...\n", attr.getName());
			warned = true;

			if (d->motion_service.isValid())
				d->motion_service.resume();
		}

		urt_sleep(1000000000 / frequency);
	}
}

static void cleanup(struct data *d)
{
	urt_task_delete(d->calibrator_task);
	d->skin.free();
	urt_exit();
}

static int start(struct data *d)
{
	if (urt_init())
		return EXIT_FAILURE;

	/* sanitize the input */
	if (frequency < 1)
	{
		urt_err("Invalid frequency %u.  Defaulting to 1Hz\n", frequency);
		frequency = 1;
	}
	if (filter_size < 1)
	{
		urt_err("Invalid filter size %u.  Disabling filtering\n", filter_size);
		do_filter = false;;
		filter_size = 1;
	}
	if (damp_size < 1)
	{
		urt_err("Invalid damp size %u.  Disabling damping\n", damp_size);
		do_dampen = false;;
		damp_size = 1;
	}
	if (threshold == 0)
	{
		if (raw || !do_scale)
			threshold = 500;
		else if (do_dampen)
			threshold = 30;
		else
			threshold = 60;
	}

	if (d->skin.init())
		goto exit_no_skin;

	d->skin.setSensorInitHook(sensor_init_hook);
	d->skin.setSensorCleanHook(sensor_clean_hook);

	return 0;
exit_no_skin:
	urt_err("init failed\n");
	cleanup(d);
	return EXIT_FAILURE;
}

static void body(struct data *d)
{
	populate_blacklist(d);

	/* try to connect to calibrator */
	d->calibrator_service = d->skin.attach(SkinReaderAttr(calibrator?calibrator:"CAL"),
			(urt_task_attr){0}, SkinReaderCallbacks(calibrate));
	if (!d->calibrator_service.isValid())
		urt_err("error: calibrator service not running\n");
	else
	{
		/* create a soft real-time task to be able to request calibration */
		urt_task_attr tattr = {0};
		tattr.soft = true;

		d->calibrator_task = urt_task_new(calibration_request, d, &tattr);
		if (d->calibrator_task == NULL)
			urt_err("error: could not create real-time task for calibration requests\n");
		else
		{
			urt_task_start(d->calibrator_task);
			d->skin.resume();
			loop_update_skin(d);
		}
	}

	done = 1;
}

static void stop(struct data *d)
{
	cleanup(d);
}
