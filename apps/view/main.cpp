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

#define URT_LOG_PREFIX "visualizer: "
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <map>
#include <set>
#include <string>
#include <dirent.h>
#include <math.h>
#include <Ngin.h>
#include <Ngin3d.h>
#include <shfont.h>
#include <skin.h>
#include <skin_motion.h>
#include <skin_calibrator.h>
#include <scaler.h>
#include <filter.h>
#include <amplifier.h>
#include "vecmath.h"

using namespace std;

URT_MODULE_LICENSE("GPL");
URT_MODULE_AUTHOR("Shahbaz Youssefi");
URT_MODULE_DESCRIPTION("Skin visualizer\n");

static bool fullscreen = false;
#ifdef TODO_IMPL_SAVE_STAT
static bool save_stat = false;
#endif
static char *calibrator = NULL;
static char *motion_detector = NULL;
static bool auto_update = false;
static char *home = NULL;
static bool show_nontaxel = false;
static bool show_hud = true;

URT_MODULE_PARAM_START()
URT_MODULE_PARAM(fullscreen, bool, "Full screen (default: no)")
URT_MODULE_PARAM(auto_update, bool, "Automatically adapt to changes in skin (default: no)")
#ifdef TODO_IMPL_SAVE_STAT
URT_MODULE_PARAM(save_stat, bool, "Save reader statistics (default: no)")
#endif
URT_MODULE_PARAM(calibrator, charp, "Calibrator service name to attach to.  Default value is 'CAL'")
URT_MODULE_PARAM(motion_detector, charp, "Motion detection service name to attach to.  Default value is 'MD'")
URT_MODULE_PARAM(home, charp, "Home path, where settings and gui items are placed.  Default value is '$HOME/.skin'")
URT_MODULE_PARAM(show_nontaxel, bool, "Whether sensor types other than taxels should be shown (default: no)")
URT_MODULE_PARAM(show_hud, bool, "Whether heads up display text must be shown (default: yes)")
URT_MODULE_PARAM_END()

struct data
{
	int unused;
};

/* skin */
static struct skin *skin = NULL;
static struct skin_reader *calibrator_service = NULL;
static struct skin_reader *motion_service = NULL;
static bool motion_service_enabled = false;
static urt_task *requester_task = NULL;
static enum possible_requests {
	REQUEST_NONE,
	REQUEST_CALIBRATE,
	REQUEST_SKIN,
} to_request = REQUEST_NONE;

/* acquisition */
static enum {
	ACQ_PERIODIC,
	ACQ_SPORADIC,
	ACQ_SOFT,
} acq_method = ACQ_PERIODIC;
static urt_time acq_period = 50000000;

/* processing */
static scaler sclr;
static filter fltr;
static amplifier amp(0, 3, 50.182974, -63.226586);
static vector<uint8_t> responses;
static vector<skin_sensor_response> temp_responses;
static vector<skin_sensor_response> baseline_response;

/* map a blacklisted id to a vector of its replacements */
static map<skin_sensor_unique_id, set<skin_sensor_unique_id> > blacklist;

#ifdef TODO_IMPL_SAVE_STAT
static FILE *statout;
#endif

static int res_width = 1024;
static int res_height = 786;
#define SH_WIDTH res_width
#define SH_HEIGHT res_height
#define FONT_SCALE (res_width / 1000.0f)

/* rendering data */
#define TEMP_MESSAGE_TIME 3000
static string temp_message;
static unsigned int t_temp_message;

/* processing */
static bool raw_results = false; /* if raw results, all do_* will be ignored */
static bool do_scale = true;
static bool do_dampen = true;
static bool do_filter = false;
static bool do_amplify = true;
static bool raw_remove_baseline = false; /* only if raw results */
static int filter_size = 3;
static unsigned int damp_size = 6;
static unsigned int t_last_damp = 0;
#define DAMP_PERIOD 100

/* gui */
static bool show_values = false;
static bool show_ids = false;
static float meter_scale = 1.0f;
static float sensor_radius_mult = 1.5;
static float value_font_size = 0.01;
static float sensor_color_unknown[] = {0, 1, 0.85f};
static float sensor_color_taxel[] = {0, 0.5f, 1};
static float sensor_color_temperature[] = {1, 0.5f, 0};
static float sensor_color_paused[] = {1, 1, 0};
static float sensor_color_bad[] = {1, 0, 0};

static shNginTexture logo_unige, logo_cyskin;

/* other */
static char exec_path[PATH_MAX + 1] = "";

static int start(struct data *d);
static void body(struct data *d);
static void stop(struct data *d);

URT_GLUE(start, body, stop, struct data, interrupted, done)

void cleanup()
{
	urt_task_delete(requester_task);
	skin_free(skin);
	shNgin3dQuit();
	shNginQuit();
	SDL_Quit();
	urt_exit();
}

bool initializing = true;

void initialize_SDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1)
	{
		urt_err("Could not initialize SDL video: %s\n", SDL_GetError());
		interrupted = 1;
		return;
	}
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	fflush(stdout);
	FILE *resin = popen("xdpyinfo | grep 'dimensions:'", "r");
	if (resin)
	{
		int w, h;
		if (fscanf(resin, "%*s %dx%d", &w, &h) != 2)
			urt_err("Failed to detect screen resolution.  Using defaults\n");
		else
		{
			res_width = w;
			res_height = h;
			urt_err("Screen resolution detected as %dx%d\n", res_width, res_height);
		}
		pclose(resin);
	}
	if (!fullscreen)
	{
		res_height -= 100;
		res_width -= 75;
	}
	if (SDL_SetVideoMode(res_width, res_height, 0, SDL_OPENGL | (fullscreen?SDL_FULLSCREEN:0)) == NULL)
	{
		urt_err("Could not open an OpenGL window: %s\n", SDL_GetError());
		interrupted = 1;
		return;
	}
	initializing = true;
	SDL_ShowCursor(SDL_DISABLE);
}

struct sensor_extra_data
{
	struct skin_sensor_calibration_info calib_info;
};

static void sensor_init_hook(struct skin_sensor *s, void *user_data)
{
	s->user_data = urt_mem_new(sizeof(struct sensor_extra_data));
	*(struct sensor_extra_data *)s->user_data = (struct sensor_extra_data){0};
}

static void sensor_clean_hook(struct skin_sensor *s, void *user_data)
{
	urt_mem_delete(s->user_data);
}

static struct skin_sensor_calibration_info *get_calib_info(struct skin_sensor *s, void *user_data)
{
	struct sensor_extra_data *extra_data = (struct sensor_extra_data *)s->user_data;
	return &extra_data->calib_info;
}

static void calibrate(struct skin_reader *reader, void *mem, size_t size, void *user_data)
{
	skin_calibrate(skin, mem, get_calib_info);
}

bool update_skin();

void change_acq_method()
{
	switch (acq_method)
	{
		default:
		case ACQ_SOFT:
			acq_method = ACQ_PERIODIC;
			break;
		case ACQ_PERIODIC:
			acq_method = ACQ_SPORADIC;
			break;
		case ACQ_SPORADIC:
			acq_method = ACQ_SOFT;
			break;
	}

	if (!auto_update)
		update_skin();
}

static struct skin_motion last_motion = {0};

void update_motion_service(struct skin_reader *reader, void *mem, size_t size, void *user_data)
{
	last_motion = *(struct skin_motion *)mem;
}

void connect_motion_service(bool en)
{
	static bool warning_given = false;
	if (en)
	{
		if (motion_service)
			return;

		struct skin_reader_attr motion_attr = {0};
		motion_attr.name = motion_detector?motion_detector:"MD";
		urt_task_attr tattr = {0};
		tattr.soft = true;
		struct skin_reader_callbacks callbacks = {0};
		callbacks.read = update_motion_service;

		motion_service = skin_service_attach(skin, &motion_attr, &tattr, &callbacks);
		if (motion_service == NULL && !warning_given)
		{
			urt_err("Could not connect to motion service\n");
			temp_message = "Motion service unavailable";
			t_temp_message = SDL_GetTicks();
			warning_given = true;
		}
	}
	else
	{
		skin_service_detach(motion_service);
		motion_service = NULL;
		last_motion.detected = 0;

		warning_given = false;
	}
}

#if 0
static int _list_region_id(skin_region *r, void *d)
{
	bool *first = (bool *)d;

	fout << (*first?"":", ") << r->id;
	*first = false;

	return SKIN_CALLBACK_CONTINUE;
}

static int _list_sub_region_id(skin_sub_region *sr, void *d)
{
	bool *first = (bool *)d;

	fout << (*first?"":", ") << sr->id;
	*first = false;

	return SKIN_CALLBACK_CONTINUE;
}

static int _print_sensor_type(skin_sensor_type *st, void *d)
{
	fout << "\tId: " << st->id
		<< ", name: " << (st->name?st->name:"<none>")
		<< ", sensors: [" << st->sensors_begin
		<< ", " << st->sensors_end << ")"
		<< ", modules: [" << st->modules_begin
		<< ", " << st->modules_end << ")"
		<< ", patches: [" << st->patches_begin
		<< ", " << st->patches_end << ")" << endl;
	return SKIN_CALLBACK_CONTINUE;
}

static int _print_sensor(skin_sensor *s, void *d)
{
	fout << "\tId: " << s->id
		<< ", sub_region: " << s->sub_region_id
		<< ", module: " << s->module_id
		<< ", type: " << s->sensor_type_id
		<< ", position: (" << s->relative_position[0]
		<< ", " << s->relative_position[1]
		<< ", " << s->relative_position[2] << ")"
		<< ", on robot link: " << s->robot_link
		<< ", neighbors: {";
	for (skin_sensor_id j = 0; j < s->neighbors_count; ++j)
		fout << (j == 0?"":", ") << s->neighbors[j];
	fout << "}" << endl;
	return SKIN_CALLBACK_CONTINUE;
}

static int _print_module(skin_module *m, void *d)
{
	fout << "\tId: " << m->id
		<< ", patch: " << m->patch_id
		<< ", type: " << m->sensor_type_id
		<< ", sensors: [" << m->sensors_begin
		<< ", " << m->sensors_end << ")" << endl;
	return SKIN_CALLBACK_CONTINUE;
}

static int _print_patch(skin_patch *p, void *d)
{
	fout << "\tId: " << p->id
		<< ", type: " << p->sensor_type_id
		<< ", modules: [" << p->modules_begin
		<< ", " << p->modules_end << ")" << endl;
	return SKIN_CALLBACK_CONTINUE;
}

static int _print_sub_region(skin_sub_region *sr, void *d)
{
	fout << "\tId: " << sr->id
		<< ", sensors:";
	for (skin_sensor_type_id j = 0; j < skin_sensor_types_count(skin); ++j)
		fout << " [" << sr->sensors_begins[j]
			<< ", " << sr->sensors_ends[j] << ")";
	fout << ", regions: {";
	bool first = true;
	skin_sub_region_for_each_region(sr, _list_region_id, &first);
	fout << "}" << endl;
	return SKIN_CALLBACK_CONTINUE;
}

static int _print_region(skin_region *r, void *d)
{
	fout << "\tId: " << r->id
		<< ", sub_regions: {";
	bool first = true;
	skin_region_for_each_sub_region(r, _list_sub_region_id, &first);
	fout << "}" << endl;
	return SKIN_CALLBACK_CONTINUE;
}

void print_structures()
{
	fout << "Data structure:" << endl;
	fout << skin_sensor_types_count(skin) << " sensor types:" << endl;
	skin_for_each_sensor_type(skin, _print_sensor_type, NULL);
	fout << skin_sensors_count(skin) << " sensors:" << endl;
	skin_for_each_sensor(skin, _print_sensor, NULL);
	fout << skin_modules_count(skin) << " modules:" << endl;
	skin_for_each_module(skin, _print_module, NULL);
	fout << skin_patches_count(skin) << " patches:" << endl;
	skin_for_each_patch(skin, _print_patch, NULL);
	fout << skin_sub_regions_count(skin) << " sub_regions:" << endl;
	skin_for_each_sub_region(skin, _print_sub_region, NULL);
	fout << skin_regions_count(skin) << " regions:" << endl;
	skin_for_each_region(skin, _print_region, NULL);
	temp_message = "Data Structures Dumped to log.out";
	t_temp_message = SDL_GetTicks();
}
#else
void print_structures()
{
	urt_err("print_structures(): TODO!\n");
	temp_message = "Functionality not yet implemented";
	t_temp_message = SDL_GetTicks();
}
#endif

static int _calc_baseline(skin_sensor *s, void *d)
{
	skin_sensor_id *cur = (skin_sensor_id *)d;
	baseline_response[*cur] = skin_sensor_get_response(s);
	++*cur;
	return SKIN_CALLBACK_CONTINUE;
}

void keyboard(SDL_KeyboardEvent *kbe)
{
	shNgin3dReceivedInput(NGIN3D_KEY, (shNgin3dKey)kbe->keysym.sym, (kbe->type == SDL_KEYDOWN)?NGIN3D_PRESSED:NGIN3D_RELEASED);
	switch (kbe->keysym.sym)
	{
		/* skin */
		case SDLK_q:
			if (kbe->type == SDL_KEYUP)
				change_acq_method();
			break;
		/* processing */
		case SDLK_r:
			if (kbe->type == SDL_KEYUP)
			{
				raw_results = !raw_results;
				if (raw_results)
				{
					value_font_size *= 0.7;
					skin_sensor_id i = 0;
					skin_for_each_sensor(skin, _calc_baseline, &i);
				}
				else
					value_font_size /= 0.7;
			}
			break;
		case SDLK_h:
			if (kbe->type == SDL_KEYUP)
				do_dampen = !do_dampen;
			break;
		case SDLK_f:
			if (kbe->type == SDL_KEYUP)
				do_filter = !do_filter;
			break;
		case SDLK_l:
			if (kbe->type == SDL_KEYUP)
				do_scale = !do_scale;
			break;
		case SDLK_k:
			if (kbe->type == SDL_KEYUP)
				do_amplify = !do_amplify;
			break;
		case SDLK_b:
			if (kbe->type == SDL_KEYUP)
			{
				raw_remove_baseline = !raw_remove_baseline;
				if (raw_remove_baseline)
				{
					skin_sensor_id i = 0;
					skin_for_each_sensor(skin, _calc_baseline, &i);
				}
			}
			break;
		case SDLK_RETURN:
			if (kbe->type == SDL_KEYUP)
			{
				sclr.reset();
				sclr.affect(skin);
			}
			break;
		case SDLK_MINUS:
		case SDLK_UNDERSCORE:
			if (kbe->type == SDL_KEYUP)
				if (filter_size > 2)
				{
					--filter_size;
					fltr.change_size(filter_size);
				}
			break;
		case SDLK_PLUS:
		case SDLK_EQUALS:
			if (kbe->type == SDL_KEYUP)
			{
				++filter_size;
				fltr.change_size(filter_size);
			}
			break;
		/* motion service */
		case SDLK_m:
			if (kbe->type == SDL_KEYUP)
			{
				motion_service_enabled = !motion_service_enabled;
				connect_motion_service(motion_service_enabled);
			}
			break;
		/* gui */
		case SDLK_v:
			if (kbe->type == SDL_KEYUP)
			{
				show_values = !show_values;
				if (show_values)
					show_ids = false;
			}
			break;
		case SDLK_i:
			if (kbe->type == SDL_KEYUP)
			{
				show_ids = !show_ids;
				if (show_ids)
					show_values = false;
			}
			break;
		case SDLK_c:
			if (kbe->type == SDL_KEYUP)
				show_hud = !show_hud;
			break;
		case SDLK_PAGEUP:
			if (kbe->type == SDL_KEYUP)
				sensor_radius_mult *= 1.1;
			break;
		case SDLK_PAGEDOWN:
			if (kbe->type == SDL_KEYUP)
				if (sensor_radius_mult > 0.01)
					sensor_radius_mult /= 1.1;
			break;
		case SDLK_KP_MINUS:				/* undocumented */
			if (kbe->type == SDL_KEYUP)
				if (value_font_size > 0.001)
					value_font_size *= 0.8;
			break;
		case SDLK_KP_PLUS:				/* undocumented */
			if (kbe->type == SDL_KEYUP)
				value_font_size /= 0.8;
			break;
		/* other */
		case SDLK_p:
			if (kbe->type == SDL_KEYUP)
				print_structures();
			break;
		case SDLK_ESCAPE:
			interrupted = 1;
			break;
		default:
			break;
	}
}

int warpmotionx = 0, warpmotiony = 0;

void mouse_motion(SDL_MouseMotionEvent *mme)
{
	if (initializing)
	{
		if (mme->x == res_width / 2 && mme->y == res_height / 2)
			initializing = false;
		else
			SDL_WarpMouse(res_width / 2, res_height / 2);
	}
	else if (mme->xrel != -warpmotionx || -mme->yrel != -warpmotiony)
	{
		shNgin3dReceivedInput(NGIN3D_MOUSE_MOVE, mme->xrel, -mme->yrel);
		warpmotionx += mme->xrel;
		warpmotiony += -mme->yrel;
		SDL_WarpMouse(res_width / 2, res_height / 2);
	}
	else
		warpmotionx = warpmotiony = 0;
}

void process_events()
{
	SDL_Event e;
	while (SDL_PollEvent(&e) && !interrupted)
	{
		switch (e.type)
		{
			case SDL_KEYUP:
			case SDL_KEYDOWN:
				keyboard(&e.key);
				break;
			case SDL_MOUSEMOTION:
				mouse_motion(&e.motion);
				break;
			case SDL_QUIT:
				interrupted = 1;
			default:
				break;
		}
	}
}

static shNginTexture skybox[6];		/* right, front, left, back, up, down */

void draw_sky_box()
{
#define SKYBOX_DIMENSION 10000
#define SKYBOX_NOTCH_REMOVE 10
	shNginEnableTexture();
	glColor3ub(255, 255, 255);
	skybox[0].shNginTBind();
	glBegin(GL_QUADS);
		skybox[0].shNginTTextureCoord(0, 0); glVertex3f(SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION, -SKYBOX_DIMENSION);
		skybox[0].shNginTTextureCoord(1, 0); glVertex3f(SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION, SKYBOX_DIMENSION);
		skybox[0].shNginTTextureCoord(1, 1); glVertex3f(SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION, SKYBOX_DIMENSION);
		skybox[0].shNginTTextureCoord(0, 1); glVertex3f(SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION, -SKYBOX_DIMENSION);
	glEnd();
	skybox[1].shNginTBind();
	glBegin(GL_QUADS);
		skybox[1].shNginTTextureCoord(0, 0); glVertex3f(-SKYBOX_DIMENSION, -SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE);
		skybox[1].shNginTTextureCoord(1, 0); glVertex3f(SKYBOX_DIMENSION, -SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE);
		skybox[1].shNginTTextureCoord(1, 1); glVertex3f(SKYBOX_DIMENSION, SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE);
		skybox[1].shNginTTextureCoord(0, 1); glVertex3f(-SKYBOX_DIMENSION, SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE);
	glEnd();
	skybox[2].shNginTBind();
	glBegin(GL_QUADS);
		skybox[2].shNginTTextureCoord(0, 0); glVertex3f(-SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION, SKYBOX_DIMENSION);
		skybox[2].shNginTTextureCoord(1, 0); glVertex3f(-SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION, -SKYBOX_DIMENSION);
		skybox[2].shNginTTextureCoord(1, 1); glVertex3f(-SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION, -SKYBOX_DIMENSION);
		skybox[2].shNginTTextureCoord(0, 1); glVertex3f(-SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION, SKYBOX_DIMENSION);
	glEnd();
	skybox[3].shNginTBind();
	glBegin(GL_QUADS);
		skybox[3].shNginTTextureCoord(0, 0); glVertex3f(SKYBOX_DIMENSION, -SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE);
		skybox[3].shNginTTextureCoord(1, 0); glVertex3f(-SKYBOX_DIMENSION, -SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE);
		skybox[3].shNginTTextureCoord(1, 1); glVertex3f(-SKYBOX_DIMENSION, SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE);
		skybox[3].shNginTTextureCoord(0, 1); glVertex3f(SKYBOX_DIMENSION, SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE);
	glEnd();
	skybox[4].shNginTBind();
	glBegin(GL_QUADS);
		skybox[4].shNginTTextureCoord(0, 0); glVertex3f(SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION);
		skybox[4].shNginTTextureCoord(1, 0); glVertex3f(-SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION);
		skybox[4].shNginTTextureCoord(1, 1); glVertex3f(-SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION);
		skybox[4].shNginTTextureCoord(0, 1); glVertex3f(SKYBOX_DIMENSION, SKYBOX_DIMENSION - SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION);
	glEnd();
	skybox[5].shNginTBind();
	glBegin(GL_QUADS);
		skybox[5].shNginTTextureCoord(0, 0); glVertex3f(-SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION);
		skybox[5].shNginTTextureCoord(1, 0); glVertex3f(SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, SKYBOX_DIMENSION);
		skybox[5].shNginTTextureCoord(1, 1); glVertex3f(SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION);
		skybox[5].shNginTTextureCoord(0, 1); glVertex3f(-SKYBOX_DIMENSION, -SKYBOX_DIMENSION + SKYBOX_NOTCH_REMOVE, -SKYBOX_DIMENSION);
	glEnd();
}

int FPS = 0;
unsigned int dt;

#define PI 3.14159265358979323

void draw_cylinder(float from[3], float to[3], float left[3], float up[3], float tickness)
{
	glBegin(GL_QUADS);
	for (float angle = 0; angle < PI; angle += 0.5)
	{
		float sa = sin(angle)*tickness, ca = cos(angle)*tickness;
		float sb = sin(angle+0.5)*tickness, cb = cos(angle+0.5)*tickness;
#define ARROW_DISP 4
		glVertex3f(from[0]*meter_scale+left[0]*cb+up[0]*sb, from[1]*meter_scale+left[1]*cb+up[1]*sb, from[2]*meter_scale+left[2]*cb+up[2]*sb + ARROW_DISP);
		glVertex3f(from[0]*meter_scale+left[0]*ca+up[0]*sa, from[1]*meter_scale+left[1]*ca+up[1]*sa, from[2]*meter_scale+left[2]*ca+up[2]*sa + ARROW_DISP);
		glVertex3f(to[0]*meter_scale+left[0]*ca+up[0]*sa, to[1]*meter_scale+left[1]*ca+up[1]*sa, to[2]*meter_scale+left[2]*ca+up[2]*sa + ARROW_DISP);
		glVertex3f(to[0]*meter_scale+left[0]*cb+up[0]*sb, to[1]*meter_scale+left[1]*cb+up[1]*sb, to[2]*meter_scale+left[2]*cb+up[2]*sb + ARROW_DISP);

		glVertex3f(from[0]*meter_scale-(left[0]*ca+up[0]*sa), from[1]*meter_scale-(left[1]*ca+up[1]*sa), from[2]*meter_scale-(left[2]*ca+up[2]*sa) + ARROW_DISP);
		glVertex3f(from[0]*meter_scale-(left[0]*cb+up[0]*sb), from[1]*meter_scale-(left[1]*cb+up[1]*sb), from[2]*meter_scale-(left[2]*cb+up[2]*sb) + ARROW_DISP);
		glVertex3f(to[0]*meter_scale-(left[0]*cb+up[0]*sb), to[1]*meter_scale-(left[1]*cb+up[1]*sb), to[2]*meter_scale-(left[2]*cb+up[2]*sb) + ARROW_DISP);
		glVertex3f(to[0]*meter_scale-(left[0]*ca+up[0]*sa), to[1]*meter_scale-(left[1]*ca+up[1]*sa), to[2]*meter_scale-(left[2]*ca+up[2]*sa) + ARROW_DISP);
	}
	glEnd();
}

void draw_motion_arrow(int64_t motion_to[], int64_t motion_from[])
{
	float to[3] = {motion_to[0] / 1000000000.0f, motion_to[1] / 1000000000.0f, motion_to[2] / 1000000000.0f};
	float from[3] = {motion_from[0] / 1000000000.0f, motion_from[1] / 1000000000.0f, motion_from[2] / 1000000000.0f};
	float direction[3];
	ASSIGN(direction, to);
	SUB(direction, direction, from);
	float dirnorm = sqrt(direction[0] * direction[0] + direction[1] * direction[1] + direction[2] * direction[2]);
	if (dirnorm < 1e-10)
		dirnorm = 1;
	DIV(direction, direction, dirnorm);
	float up[3] = {0.0f, 0.0f, 1.0f};	/* if direction was not (0, 0, 1) */
	if (ZERO(direction[0]) && ZERO(direction[1]))
	{
		up[1] = 1.0f;			/* just change it.  Doesn't matter to what */
		up[2] = 0.0f;
	}
	float left[3];
	CROSS(left, up, direction);
	CROSS(up, direction, left);
#define SQRT_3_HALF 0.866
	float dirplus30[3];
	float dirminus30[3];
	float temp[3];
	ASSIGN(temp, left);
	MUL(temp, temp, 0.5f);
	ASSIGN(dirplus30, direction);
	MUL(dirplus30, dirplus30, SQRT_3_HALF);
	ASSIGN(dirminus30, dirplus30);
	SUB(dirplus30, dirplus30, temp);
	ADD(dirminus30, dirminus30, temp);
	float leftplus30[3];
	float leftminus30[3];
	CROSS(leftplus30, up, dirplus30);
	CROSS(leftminus30, up, dirminus30);
#define HEAD_LENGTH 6
	MUL(dirplus30, dirplus30, HEAD_LENGTH / meter_scale);
	MUL(dirminus30, dirminus30, HEAD_LENGTH / meter_scale);
	float fromplus30[3];
	float fromminus30[3];
	ASSIGN(fromplus30, to);
	ASSIGN(fromminus30, to);
	SUB(fromplus30, fromplus30, dirplus30);
	SUB(fromminus30, fromminus30, dirminus30);
#define ARROW_THICKNESS 1
	draw_cylinder(from, to, left, up, ARROW_THICKNESS);
	draw_cylinder(fromplus30, to, leftplus30, up, ARROW_THICKNESS);
	draw_cylinder(fromminus30, to, leftminus30, up, ARROW_THICKNESS);
}

struct blacklist_average_data
{
	set<skin_sensor_unique_id> &replacement;
	unsigned int sum;
	unsigned int count;
};

int _get_sensor_blacklist_average(struct skin_sensor *s, void *d)
{
	struct blacklist_average_data *data = (struct blacklist_average_data *)d;

	if (data->replacement.find(s->uid) == data->replacement.end())
		return SKIN_CALLBACK_CONTINUE;

	data->sum += skin_sensor_get_response(s);
	++data->count;

	return SKIN_CALLBACK_CONTINUE;
}

skin_sensor_response get_response(struct skin_sensor *s)
{
	/* if not in blacklist, return its response */
	if (blacklist.find(s->uid) == blacklist.end())
		return skin_sensor_get_response(s);

	struct blacklist_average_data bd = { blacklist[s->uid], 0, 0 };
	skin_for_each_sensor(skin, _get_sensor_blacklist_average, &bd);

	return bd.count > 0?bd.sum / bd.count:skin_sensor_get_response(s);
}

static int _save_raw_response(struct skin_sensor *s, void *d)
{
	skin_sensor_id *cur = (skin_sensor_id *)d;

	responses[*cur] = get_response(s) >> 8;
	++*cur;

	return SKIN_CALLBACK_CONTINUE;
}

static int _save_temp_response(struct skin_sensor *s, void *d)
{
	skin_sensor_id *cur = (skin_sensor_id *)d;

	temp_responses[*cur] = get_response(s);
	++*cur;

	return SKIN_CALLBACK_CONTINUE;
}

static void save_responses()
{
	skin_sensor_id i = 0;

	/* TODO: MAKE SURE THE DATA IS NEW! IF NOT, SKIP THIS READ! TODO: add the same mechanism to motion service */

	if (raw_results || !do_scale)
		skin_for_each_sensor(skin, _save_raw_response, &i);
	else
	{
		skin_for_each_sensor(skin, _save_temp_response, &i);
		sclr.scale(temp_responses, responses);
	}
	if (raw_results)
		return;
	if (do_filter)
	{
		fltr.new_responses(responses);
		fltr.get_responses(responses);
	}
	if (do_amplify)
		amp.amplify(responses, responses);
}

void requester(urt_task *task, void *d)
{
	while (!interrupted)
	{
		switch (to_request)
		{
		default:
			break;
		case REQUEST_CALIBRATE:
			skin_reader_request(calibrator_service, &interrupted);
			to_request = REQUEST_NONE;
			break;
		case REQUEST_SKIN:
			skin_request(skin, &interrupted);
			to_request = REQUEST_NONE;
			break;
		}

		urt_sleep(1000000);
	}
}

void do_request(possible_requests req)
{
	to_request = req;
	while (!interrupted && to_request != REQUEST_NONE)
		urt_sleep(1000000);
}

void update_responses()
{
	if (acq_method == ACQ_SPORADIC)
		do_request(REQUEST_SKIN);
	if (!raw_results && do_dampen)
	{
		if (SDL_GetTicks() > t_last_damp + DAMP_PERIOD)
		{
			sclr.dampen(damp_size);
			t_last_damp += DAMP_PERIOD;
		}
	}
	save_responses();
}

struct draw_sensor_state
{
	skin_sensor_id cur;
	bool is_bad;
	bool is_paused;
};

static int _draw_sensor(skin_sensor *s, void *d)
{
	skin_sensor_id *cur = (skin_sensor_id *)d;
	uint8_t response = responses[*cur];
	float *color_coef = sensor_color_unknown;
	bool is_bad = false, is_paused = false;
	struct sensor_extra_data *extra = (struct sensor_extra_data *)s->user_data;
	float radius_mult = sensor_radius_mult;

	if (extra == NULL || !extra->calib_info.calibrated)
		return SKIN_CALLBACK_CONTINUE;

	/* if told not to show nontaxels, and the sensor is not taxel, don't show it */
	if (!show_nontaxel && s->type != SKIN_SENSOR_TYPE_CYSKIN_TAXEL
			&& s->type != SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL
			&& s->type != SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL)
	{
		++*cur;
		return SKIN_CALLBACK_CONTINUE;
	}

	/* if removing baseline, correct the height */
	if (raw_results && raw_remove_baseline)
	{
		uint8_t b = baseline_response[*cur] >> 8;
		response = b < response?response - b:0;
	}

	uint8_t color = ((255u - response) * 2 + 0) / 3;

	++*cur;

	is_bad = !skin_user_is_active(s->user);
	is_paused = skin_user_is_paused(s->user);

	if (is_bad)
		color_coef = sensor_color_bad;
	else if (is_paused)
		color_coef = sensor_color_paused;
	else if (s->type == SKIN_SENSOR_TYPE_CYSKIN_TAXEL
			|| s->type == SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL
			|| s->type == SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL)
		color_coef = sensor_color_taxel;
	else if (s->type == SKIN_SENSOR_TYPE_CYSKIN_TEMPERATURE)
	{
		color_coef = sensor_color_temperature;
		radius_mult = 1;
	}
	else
		color_coef = sensor_color_unknown;
	glColor3ub(color * color_coef[0], color * color_coef[1], color * color_coef[2]);
	glPushMatrix();
	glTranslatef(extra->calib_info.position_nm[0] / 1000000000.0f * meter_scale,
			extra->calib_info.position_nm[1] / 1000000000.0f * meter_scale,
			extra->calib_info.position_nm[2] / 1000000000.0f * meter_scale);
	float z[3] = {extra->calib_info.orientation_nm[0] / 1000000000.0f,
		extra->calib_info.orientation_nm[1] / 1000000000.0f,
		extra->calib_info.orientation_nm[2] / 1000000000.0f};
	float x[3] = {0.0f, 0.0f, 1.0f};	/* if normal was (0, 1, 0) */
	if (!ZERO(z[0]) || !ZERO(z[1]-1.0f) || !ZERO(z[2]))
	{
		/* multiply (0, 1, 0) by normal which should give a vector perpendecular to normal */
		x[0] = z[2];
		x[1] = 0.0f;
		x[2] = -z[0];
	}
	float y[3];
	CROSS(y, z, x);
	float height = response / 60.0f;
	glBegin(GL_QUADS);
	float radius = extra->calib_info.radius_nm / 1000000000.0f*radius_mult*meter_scale;
	for (float angle = 0; angle < PI; angle += 0.5)
	{
		float sa = sin(angle)*radius, ca = cos(angle)*radius;
		float sb = sin(angle+0.5)*radius, cb = cos(angle+0.5)*radius;
		glVertex3f(x[0]*ca+y[0]*sa, x[1]*ca+y[1]*sa, x[2]*ca+y[2]*sa);
		glVertex3f(x[0]*cb+y[0]*sb, x[1]*cb+y[1]*sb, x[2]*cb+y[2]*sb);
		glVertex3f(x[0]*cb+y[0]*sb+z[0]*height, x[1]*cb+y[1]*sb+z[1]*height, x[2]*cb+y[2]*sb+z[2]*height);
		glVertex3f(x[0]*ca+y[0]*sa+z[0]*height, x[1]*ca+y[1]*sa+z[1]*height, x[2]*ca+y[2]*sa+z[2]*height);

		glVertex3f(-(x[0]*ca+y[0]*sa), -(x[1]*ca+y[1]*sa), -(x[2]*ca+y[2]*sa));
		glVertex3f(-(x[0]*cb+y[0]*sb), -(x[1]*cb+y[1]*sb), -(x[2]*cb+y[2]*sb));
		glVertex3f(-(x[0]*cb+y[0]*sb)+z[0]*height, -(x[1]*cb+y[1]*sb)+z[1]*height, -(x[2]*cb+y[2]*sb)+z[2]*height);
		glVertex3f(-(x[0]*ca+y[0]*sa)+z[0]*height, -(x[1]*ca+y[1]*sa)+z[1]*height, -(x[2]*ca+y[2]*sa)+z[2]*height);
	}
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(z[0]*height, z[1]*height, z[2]*height);
	for (float angle = 0; angle < PI; angle += 0.5)
	{
		float sa = sin(angle)*radius, ca = cos(angle)*radius;
		glVertex3f(x[0]*ca+y[0]*sa+z[0]*height, x[1]*ca+y[1]*sa+z[1]*height, x[2]*ca+y[2]*sa+z[2]*height);
	}
		glVertex3f(-(x[0]*radius)+z[0]*height, -(x[1]*radius)+z[1]*height, -(x[2]*radius)+z[2]*height);
	glEnd();
	glBegin(GL_TRIANGLE_FAN);
		glVertex3f(z[0]*height, z[1]*height, z[2]*height);
	for (float angle = 0; angle < PI; angle += 0.5)
	{
		float sa = sin(angle)*radius, ca = cos(angle)*radius;
		glVertex3f(-(x[0]*ca+y[0]*sa)+z[0]*height, -(x[1]*ca+y[1]*sa)+z[1]*height, -(x[2]*ca+y[2]*sa)+z[2]*height);
	}
		glVertex3f(x[0]*radius+z[0]*height, x[1]*radius+z[1]*height, x[2]*radius+z[2]*height);
	glEnd();

	glPopMatrix();
	return SKIN_CALLBACK_CONTINUE;
}

static int _draw_value(skin_sensor *s, void *d)
{
	skin_sensor_id *cur = (skin_sensor_id *)d;
	struct sensor_extra_data *extra = (struct sensor_extra_data *)s->user_data;

	if (extra == NULL || !extra->calib_info.calibrated)
		return SKIN_CALLBACK_CONTINUE;

	/* if told not to show nontaxels, and the sensor is not taxel, don't show its value */
	if (!show_nontaxel && s->type != SKIN_SENSOR_TYPE_CYSKIN_TAXEL
			&& s->type != SKIN_SENSOR_TYPE_MACLAB_ROBOSKIN_TAXEL
			&& s->type != SKIN_SENSOR_TYPE_ROBOSKIN_TAXEL)
	{
		++*cur;
		return SKIN_CALLBACK_CONTINUE;
	}

	glPushMatrix();
	glTranslatef(extra->calib_info.position_nm[0] / 1000000000.0f * meter_scale,
			extra->calib_info.position_nm[1] / 1000000000.0f * meter_scale,
			extra->calib_info.position_nm[2] / 1000000000.0f * meter_scale);
	float z[3] = {extra->calib_info.orientation_nm[0] / 1000000000.0f,
		extra->calib_info.orientation_nm[1] / 1000000000.0f,
		extra->calib_info.orientation_nm[2] / 1000000000.0f};
	int response = responses[*cur];
	/* if removing baseline, correct the height */
	if (raw_results && raw_remove_baseline)
	{
		uint8_t b = baseline_response[*cur] >> 8;
		response = b < response?response - b:0;
	}
	float height = response / 60.0f;

	shFontColor(0, 255, 0);
	shFontSize(value_font_size * ((show_values && raw_results)?1.0f:1.5f) * FONT_SCALE);
	char value[10] = "";
	float loc = height + 1;
	if (show_values)
	{
		if (raw_results)
		{
			response = get_response(s);
			if (raw_remove_baseline)
				response -= baseline_response[*cur];
		}
		snprintf(value, 10, "%d", response);
	}
	else
		snprintf(value, 10, "%u", *cur);
	glTranslatef(z[0] * loc - shFontTextWidth(value) / 2, z[1] * loc + shFontTextHeight(value) / 2, z[2] * loc);
	shFontWrite(NULL, "%s", value);

	++*cur;

	glPopMatrix();
	return SKIN_CALLBACK_CONTINUE;
}

void draw_hud()
{
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, res_width, 0, res_height, -10000.0f, 10000.0f);
	glMatrixMode(GL_MODELVIEW);

	if (show_hud)
	{
		glLoadIdentity();
		shFontColor(50, 255, 200);
		shFontSize(0.25f * FONT_SCALE);
		glTranslatef(20, SH_HEIGHT - 10, 0);
		shFontWrite(NULL, "%d", FPS);

		glLoadIdentity();
		shFontColor(255, 200, 50);
		shFontSize(0.5f * FONT_SCALE);
		glTranslatef(20, SH_HEIGHT - 20, 0);
		if (raw_results)
			shFontWrite(NULL, "Raw values%s", raw_remove_baseline?" (baseline removed)":"");
		else
			shFontWrite(NULL, "Scale: %s\t\tFilter: %s (Size: %d)\tAmplification: %s\tHysteresis Compensation: %s",
					do_scale?"Yes":"No", do_filter?"Yes":"No", filter_size, do_amplify?"Yes":"No", do_dampen?"Yes":"No");

		glLoadIdentity();
		shFontColor(50, 255, 100);
		shFontSize(0.5f * FONT_SCALE);
		glTranslatef(20, SH_HEIGHT - 20 - shFontTextHeight("\n"), 0);
		char period[30] = "Unused";
		if (acq_method == ACQ_PERIODIC)
			snprintf(period, sizeof(period), "%8llu", acq_period);
		shFontWrite(NULL, "Acquisition Method: %s (Period: %s)\t\t\t%sMotion Tracking: %s",
				acq_method == ACQ_SOFT?"Soft":acq_method == ACQ_PERIODIC?"Periodic":"Sporadic", period,
				acq_method == ACQ_SOFT?"\t":"",
				motion_service?"Enabled":motion_service_enabled?"Waiting":"Disabled");

		if (!auto_update)
		{
			glLoadIdentity();
			shFontColor(255, 255, 255);
			shFontSize(0.5f * FONT_SCALE);
			glTranslatef(20, SH_HEIGHT - 20 - shFontTextHeight("\n") * 2, 0);
			shFontWrite(NULL, "Legend:");
			glTranslatef(shFontTextWidth("Legend: "), 0, 0);
			shFontColor(255 * sensor_color_unknown[0], 255 * sensor_color_unknown[1], 255 * sensor_color_unknown[2]);
			shFontWrite(NULL, "Working");
			glTranslatef(shFontTextWidth("Working"), 0, 0);
#if 0
			shFontColor(255, 255, 255);
			shFontWrite(NULL, ",");
			glTranslatef(shFontTextWidth(", "), 0, 0);
			shFontColor(255 * sensor_color_paused[0], 255 * sensor_color_paused[1], 255 * sensor_color_paused[2]);
			shFontWrite(NULL, "Paused");
			glTranslatef(shFontTextWidth("Paused"), 0, 0);
#endif
			shFontColor(255, 255, 255);
			shFontWrite(NULL, ",");
			glTranslatef(shFontTextWidth(", "), 0, 0);
			shFontColor(255 * sensor_color_bad[0], 255 * sensor_color_bad[1], 255 * sensor_color_bad[2]);
			shFontWrite(NULL, "Bad");
			glTranslatef(shFontTextWidth("Bad"), 0, 0);
		}

		glLoadIdentity();
		shFontColor(255, 255, 255);
		shFontSize(0.5f * FONT_SCALE);
		glTranslatef(20, 20 + shFontTextHeight("\n") * 7 /* number of lines */, 0);
		shFontWrite(NULL, "Keys:\n"
				  "w/a/s/d/Ctrl/Space: Movement\tMouse: Look Around\tPage Up/Down: Sensor Size\n"
				  "p: Print Structure\t\t\tc: Clean HUD\t\tKP +/-: Font Size\n"
				  "q: Acquisition Method\t\t\tm: Motion Service\tv: Show Values\n"
				  "r: Raw Values\t\t\t\tb: Remove Baseline\ti: Show Ids\n"
				  "l: Scale\t\t\t\t\tf: Filter\t\t\tk: Amplify\n"
				  "h: Hysteresis Compensation\t\t+/-: Filter Size\t\tReturn: Reset Scaler\n");

	}

	/* logos */
	shNginEnableTexture();
	glColor3ub(255, 255, 255);
	glLoadIdentity();
	if (show_hud)
	{
		int xoffset = (logo_cyskin.origSizeX * 2 / 3 - logo_unige.origSizeX / 2) / 2;
		int yoffset = logo_cyskin.origSizeY * 2 / 3 + 20;
		logo_unige.shNginTBind();
		glBegin(GL_QUADS);
			logo_unige.shNginTTextureCoord(0, 0); glVertex3f(SH_WIDTH - 20 - xoffset - logo_unige.origSizeX / 2, 20 + yoffset, 0);
			logo_unige.shNginTTextureCoord(1, 0); glVertex3f(SH_WIDTH - 20 - xoffset, 20 + yoffset, 0);
			logo_unige.shNginTTextureCoord(1, 1); glVertex3f(SH_WIDTH - 20 - xoffset, 20 + yoffset + logo_unige.origSizeY / 2, 0);
			logo_unige.shNginTTextureCoord(0, 1); glVertex3f(SH_WIDTH - 20 - xoffset - logo_unige.origSizeX / 2, 20 + yoffset + logo_unige.origSizeY / 2, 0);
		glEnd();

		logo_cyskin.shNginTBind();
		glBegin(GL_QUADS);
			logo_cyskin.shNginTTextureCoord(0, 0); glVertex3f(SH_WIDTH - 20 - logo_cyskin.origSizeX * 2 / 3, 20, 0);
			logo_cyskin.shNginTTextureCoord(1, 0); glVertex3f(SH_WIDTH - 20, 20, 0);
			logo_cyskin.shNginTTextureCoord(1, 1); glVertex3f(SH_WIDTH - 20, 20 + logo_cyskin.origSizeY * 2 / 3, 0);
			logo_cyskin.shNginTTextureCoord(0, 1); glVertex3f(SH_WIDTH - 20 - logo_cyskin.origSizeX * 2 / 3, 20 + logo_cyskin.origSizeY * 2 / 3, 0);
		glEnd();
	}
	else
	{
		logo_unige.shNginTBind();
		glBegin(GL_QUADS);
			logo_unige.shNginTTextureCoord(0, 0); glVertex3f(SH_WIDTH - 20 - logo_unige.origSizeX * 2 / 3, SH_HEIGHT - 20 - logo_unige.origSizeY * 2 / 3, 0);
			logo_unige.shNginTTextureCoord(1, 0); glVertex3f(SH_WIDTH - 20, SH_HEIGHT - 20 - logo_unige.origSizeY * 2 / 3, 0);
			logo_unige.shNginTTextureCoord(1, 1); glVertex3f(SH_WIDTH - 20, SH_HEIGHT - 20, 0);
			logo_unige.shNginTTextureCoord(0, 1); glVertex3f(SH_WIDTH - 20 - logo_unige.origSizeX * 2 / 3, SH_HEIGHT - 20, 0);
		glEnd();

		logo_cyskin.shNginTBind();
		glBegin(GL_QUADS);
			logo_cyskin.shNginTTextureCoord(0, 0); glVertex3f(20, 20, 0);
			logo_cyskin.shNginTTextureCoord(1, 0); glVertex3f(20 + logo_cyskin.origSizeX, 20, 0);
			logo_cyskin.shNginTTextureCoord(1, 1); glVertex3f(20 + logo_cyskin.origSizeX, 20 + logo_cyskin.origSizeY, 0);
			logo_cyskin.shNginTTextureCoord(0, 1); glVertex3f(20, 20 + logo_cyskin.origSizeY, 0);
		glEnd();
	}

	if (SDL_GetTicks() < t_temp_message + TEMP_MESSAGE_TIME)
	{
		glLoadIdentity();
		shFontColor(255, 50, 50);
		shFontSize(0.6f * FONT_SCALE);
		glTranslatef((SH_WIDTH - shFontTextWidth(temp_message.c_str())) / 2, SH_HEIGHT / 2 - 32 * 0.6f, 0);
		shFontWrite(NULL, "%s", temp_message.c_str());
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_DEPTH_TEST);
}

void render_screen()
{
	shNgin3dEffectuate(dt);
	shNginClear();
	draw_sky_box();
	shNginDisableTexture();
	update_responses();
	skin_sensor_id id = 0;
	skin_for_each_sensor(skin, _draw_sensor, &id);
	id = 0;
	if (show_values || show_ids)
		skin_for_each_sensor(skin, _draw_value, &id);
	if (last_motion.detected)
	{
		glColor3ub(255, 0, 0);
		draw_motion_arrow(last_motion.to, last_motion.from);
	}
	draw_hud();
#ifdef TODO_IMPL_SAVE_STAT
	if (save_stat)
	{
		skin_sensor_type *sensor_types = skin_sensor_types(skin, NULL);
		for (unsigned int i = 0; i < sr->tasks_count; ++i)
		{
			skin_task_statistics *st = &sr->tasks_statistics[i];
			statout << (i?" ":"") << st->number_of_reads << " "
				<< skin_sensor_type_sensors_count(&sensor_types[i]) << " "
				<< st->worst_read_time << " "
				<< st->best_read_time << " "
				<< st->accumulated_read_time;
		}
		statout << endl;
	}
#endif
	SDL_GL_SwapBuffers();
}

void populate_blacklist()
{
	FILE *blin = fopen("blacklist", "r");
	if (blin == NULL)
		return;

	unsigned long long i;
	unsigned int n;

	while (fscanf(blin, "%llu %u", &i, &n) == 2)
	{
		unsigned long long r;
		set<skin_sensor_unique_id> replacement;

		for (unsigned int s = 0; s < n; ++s)
		{
			if (fscanf(blin, "%llu", &r) != 1)
				continue;
			if (r != i)
				replacement.insert(r);
		}

		if (replacement.size() > 0)
			blacklist[i] = replacement;
	}

	fclose(blin);
}

void load_data(string home_path)
{
	skybox[0].shNginTLoad((home_path + "/room_east.bmp").c_str());
	skybox[1].shNginTLoad((home_path + "/room_north.bmp").c_str());
	skybox[2].shNginTLoad((home_path + "/room_west.bmp").c_str());
	skybox[3].shNginTLoad((home_path + "/room_south.bmp").c_str());
	skybox[4].shNginTLoad((home_path + "/room_up.bmp").c_str());
	skybox[5].shNginTLoad((home_path + "/room_down.bmp").c_str());
	for (int i = 0; i < 6; ++i)
		if (!skybox[i].shNginTIsLoaded())
			urt_err("Sky box %d didn't load\n", i);

	logo_unige.shNginTLoad((home_path + "/logo_unige.bmp").c_str());
	if (!logo_unige.shNginTIsLoaded())
		urt_err("Unige logo didn't load\n");
	logo_cyskin.shNginTLoad((home_path + "/logo_cyskin.bmp").c_str());
	if (!logo_cyskin.shNginTIsLoaded())
		urt_err("Cyskin logo didn't load\n");

	logo_unige.shNginTSetTransparent(255, 255, 255);
	logo_cyskin.shNginTSetTransparent(255, 255, 255);

	if (shFontLoad((home_path + "/font.shf").c_str()) == SH_FONT_FILE_NOT_FOUND)
		urt_err("Could not load font\n");
	else
	{
		shFontSize(0.5f * FONT_SCALE);
		shFontColor(255, 0, 0);
		shFontShadowColor(0, 0, 0);
		shFontShadow(SH_FONT_FULL_SHADOW);
	}
	populate_blacklist();
}

void initialize_Ngin()
{
	shNginInitialize(res_width, res_height, 90);
	shNginViewableArea(SH_WIDTH, SH_HEIGHT, 90);
	shNginSetNearZ(1);
	shNginSetFarZ(5000);
	shNginDisableTexture();
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
}

void initialize_Ngin3d(const char *settings)
{
	shNgin3dInitialize();
	shNgin3dCameraPositionFunction(shNginSetCameraPositionv);
	shNgin3dCameraTargetFunction(shNginSetCameraTargetv);
	shNgin3dCameraUpFunction(shNginSetCameraUpv);
	shNgin3dUpright();
	shNgin3dUpsideDownNotPossible();
	shNgin3dReadSettings(settings);
	shNgin3dRegisterAction(NGIN3D_KEY, NGIN3D_MOVE_FORWARD, NGIN3D_w);
	shNgin3dRegisterAction(NGIN3D_KEY, NGIN3D_MOVE_BACKWARD, NGIN3D_s);
	shNgin3dRegisterAction(NGIN3D_KEY, NGIN3D_MOVE_LEFT, NGIN3D_a);
	shNgin3dRegisterAction(NGIN3D_KEY, NGIN3D_MOVE_RIGHT, NGIN3D_d);
	shNgin3dRegisterAction(NGIN3D_KEY, NGIN3D_FLY_UP, NGIN3D_SPACE);
	shNgin3dRegisterAction(NGIN3D_KEY, NGIN3D_FLY_DOWN, NGIN3D_LCTRL);
	shNgin3dMove(-100, 0, 0, 1000);
}

static void init_filters()
{
	skin_sensor_size s = skin_sensor_count(skin);
	responses.resize(s);
	temp_responses.resize(s);
	baseline_response.resize(s);

	fltr.change_size(filter_size);
	sclr.set_range(16384);
	sclr.scale(skin);
	sclr.affect(skin);
	fltr.new_responses(skin);
	amp = amplifier(0, 3, 50.182974, -63.226586);
	amp.affect(skin);
}

struct min_max_data
{
	float minX, maxX;
	float minY, maxY;
	bool valid;
};

static int _find_min_max(skin_sensor *s, void *d)
{
	min_max_data *data = (min_max_data *)d;

	struct sensor_extra_data *extra = (struct sensor_extra_data *)s->user_data;

	if (extra == NULL || !extra->calib_info.calibrated)
		return SKIN_CALLBACK_CONTINUE;

	if (extra->calib_info.position_nm[0] / 1000000000.0f < data->minX)
		data->minX = extra->calib_info.position_nm[0] / 1000000000.0f;
	if (extra->calib_info.position_nm[0] / 1000000000.0f > data->maxX)
		data->maxX = extra->calib_info.position_nm[0] / 1000000000.0f;
	if (extra->calib_info.position_nm[1] / 1000000000.0f < data->minY)
		data->minY = extra->calib_info.position_nm[1] / 1000000000.0f;
	if (extra->calib_info.position_nm[1] / 1000000000.0f > data->maxY)
		data->maxY = extra->calib_info.position_nm[1] / 1000000000.0f;
	data->valid = true;

	return SKIN_CALLBACK_CONTINUE;
}

struct module_min_sensor_distance_data
{
	double min_distance;
	float diameter;
	bool valid;
	struct skin_module *module;
	struct skin_sensor *sensor;
};

static int _sensor_calc_min_distance_other(struct skin_sensor *other, void *d)
{
	struct module_min_sensor_distance_data *data = (struct module_min_sensor_distance_data *)d;
	struct sensor_extra_data *extra_one = (struct sensor_extra_data *)data->sensor->user_data;
	struct sensor_extra_data *extra_other = (struct sensor_extra_data *)other->user_data;

	if (extra_one == NULL || extra_other == NULL || !extra_one->calib_info.calibrated || !extra_other->calib_info.calibrated
			|| data->sensor->type == SKIN_SENSOR_TYPE_CYSKIN_TEMPERATURE || other->type ==  SKIN_SENSOR_TYPE_CYSKIN_TEMPERATURE)
		return SKIN_CALLBACK_CONTINUE;

	/* only check with previous sensors.  The further sensors will come back and check with this sensor again */
	if (data->sensor == other)
		return SKIN_CALLBACK_STOP;

	float p1[3];
	float p2[3];

	DIV(p1, extra_one->calib_info.position_nm, 1000000000.0f);
	DIV(p2, extra_other->calib_info.position_nm, 1000000000.0f);

	double dist = DISTANCE(p1, p2);
	if (dist < data->min_distance)
	{
		data->min_distance = dist;
		data->diameter = (extra_one->calib_info.radius_nm + extra_other->calib_info.radius_nm) / 1000000000.0f;
		data->valid = true;
	}
	return SKIN_CALLBACK_CONTINUE;
}

static int _sensor_calc_min_distance(struct skin_sensor *sensor, void *d)
{
	struct module_min_sensor_distance_data *data = (struct module_min_sensor_distance_data *)d;
	data->sensor = sensor;
	skin_module_for_each_sensor(data->module, _sensor_calc_min_distance_other, d);
	return SKIN_CALLBACK_CONTINUE;
}

static int _module_calc_min_sensor_distance(struct skin_module *module, void *d)
{
	struct module_min_sensor_distance_data data = {0};
	data.min_distance = 1e12;
	data.diameter = 1;
	data.module = module;
	skin_module_for_each_sensor(module, _sensor_calc_min_distance, &data);

	if (data.valid)
	{
		sensor_radius_mult = data.min_distance / data.diameter * 1.35;	/* visually, * 1.35 looked good */
		value_font_size = data.min_distance * meter_scale * 0.008;
	}

	/* calculate just for the first module that succeeds */
	return data.valid?SKIN_CALLBACK_STOP:SKIN_CALLBACK_CONTINUE;
}

void center_skin()
{
	min_max_data min_max;
	min_max.minX = 10000000.0f;
	min_max.maxX = -10000000.0f;
	min_max.minY = 10000000.0f;
	min_max.maxY = -10000000.0f;
	skin_for_each_sensor(skin, _find_min_max, &min_max);
	if (!min_max.valid || min_max.maxX == min_max.minX)
		meter_scale = 1;
	else if (min_max.maxX - min_max.minX > min_max.maxY - min_max.minY)
		meter_scale = 100.0f / (min_max.maxX - min_max.minX);
	else
		meter_scale = 100.0f / (min_max.maxY - min_max.minY);

	skin_for_each_module(skin, _module_calc_min_sensor_distance);

	if (min_max.valid)
	{
		float middleX = (min_max.maxX + min_max.minX) / 2 * meter_scale;
		float middleY = (min_max.maxY + min_max.minY) / 2 * meter_scale;
		shNgin3dMove(0, middleX, middleY, 1000);
	}
}

static string get_home()
{
	/* if specified from command line, use that */
	if (home)
		return home;

	/* otherwise try ~/.skin */
	char *h = getenv("HOME");
	if (h)
		return (string(h) + "/.skin");

	/* if for some reason $HOME didn't exist, try to get executable's location */
	char exec_link[50];
	sprintf(exec_link, "/proc/%d/exe", (int)getpid());
	int bytes = readlink(exec_link, exec_path, sizeof(exec_path) - 1);
	if (bytes > 0)
	{
		for (--bytes; bytes >= 0; --bytes)
			if (exec_path[bytes] == '/')
			{
				exec_path[bytes] = '\0';
				break;
			}
		/*
		 * if bytes < 0, then could not find /, which means the file name didn't contain
		 * any directory!  It shouldn't happen though
		 */
		if (bytes >= 0)
			return exec_path;
	}

	/* as a last resort, return current directory */
	return ".";
}

static int _count_uncalibrated(struct skin_sensor *s, void *d)
{
	skin_sensor_size *count = (skin_sensor_size *)d;
	struct sensor_extra_data *extra = (struct sensor_extra_data *)s->user_data;
	if (extra == NULL || !extra->calib_info.calibrated)
		++*count;
	return SKIN_CALLBACK_CONTINUE;
}

bool update_skin()
{
	urt_task_attr task_attr = {0};

	switch (acq_method)
	{
		default:
		case ACQ_PERIODIC:
			task_attr.period = acq_period;
			task_attr.soft = false;
			break;
		case ACQ_SPORADIC:
			task_attr.period = 0;
			task_attr.soft = false;
			break;
		case ACQ_SOFT:
			task_attr.soft = true;
			break;
	}

	bool changed = skin_update(skin, &task_attr) == 0;
	skin_resume(skin);

	/* if changed, detach from services and try to attach to them later */
	if (changed && motion_service)
		connect_motion_service(false);

	if (changed)
	{
		init_filters();
		do_request(REQUEST_CALIBRATE);

		/* if there are uncalibrated sensors, warn! */
		skin_sensor_size uncalibrated = 0;
		skin_for_each_sensor(skin, _count_uncalibrated, &uncalibrated);
		if (uncalibrated)
		{
			char message[100];
			sprintf(message, "There are %u uncalibrated sensors", uncalibrated);
			urt_err("%s\n", message);
			temp_message = message;
			t_temp_message = SDL_GetTicks();
		}
	}

	return changed;
}

void main_loop()
{
	int frame = 0;
	unsigned int last = SDL_GetTicks();
	unsigned int last_time = last;
	bool first = true;

	t_last_damp = SDL_GetTicks();

	while (!interrupted)
	{
		/*
		 * - try to update the skin, if auto_update
		 * - if not and first time, update the skin anyway
		 * - if update successful, recalibrate skin
		 * - if update successful, init_filters again
		 * - if first time and update suceeded, center_skin()
		 *
		 * - if motion service should be enabled, but is not attached to, attach to it
		 * - if motion service is not active anymore, detach from it
		 *
		 * - process events
		 * - render screen
		 */
		bool changed = false;
		if (auto_update || first)
			changed = update_skin();

		/*
		 * center skin only if before the first rendering the skin has anything at all!  Otherwise,
		 * making the screen jump on every update becomes irritating.
		 */
		if (changed && first)
			center_skin();

		if (motion_service_enabled && motion_service == NULL)
		{
			connect_motion_service(true);
			skin_resume(skin);
		}

		if (motion_service && !skin_reader_is_active(motion_service))
			connect_motion_service(false);

		unsigned int now = SDL_GetTicks();
		if (now >= last+1000)
		{
			FPS = frame * 1000 / (now - last);
			last += 1000;
			frame = 0;
		}
		dt = now - last_time;
		last_time = now;
		process_events();
		render_screen();
		++frame;

		urt_sleep(10000000);

		first = false;
	}
}

static int start(struct data *d)
{
	if (urt_init())
		return EXIT_FAILURE;

	string home_path = get_home();
	initialize_SDL();
	initialize_Ngin();
	initialize_Ngin3d((home_path + "/settings/Ngin3d_settings").c_str());
	load_data(home_path + "/gui/");

	skin = skin_init();
	if (skin == NULL)
		goto exit_no_skin;

#ifdef TODO_IMPL_SAVE_STAT
	statout = fopen("stat.out", "w");
	if (statout == NULL)
		goto exit_no_statout;
#endif

	skin_set_sensor_init_hook(skin, sensor_init_hook);
	skin_set_sensor_clean_hook(skin, sensor_clean_hook);

	return 0;
#ifdef TODO_IMPL_SAVE_STAT
exit_no_statout:
	urt_err("could not open 'stat.out' for writing\n");
#endif
exit_no_skin:
	urt_err("init failed\n");
	cleanup();
	return EXIT_FAILURE;
}

static void body(struct data *d)
{
	/* try to connect to calibrator */
	struct skin_reader_attr calib_attr = {0};
	calib_attr.name = calibrator?calibrator:"CAL";
	urt_task_attr tattr = {0};
	struct skin_reader_callbacks callbacks = {0};
	callbacks.read = calibrate;

	calibrator_service = skin_service_attach(skin, &calib_attr, &tattr, &callbacks);
	if (calibrator_service == NULL)
		urt_err("error: calibrator service not running\n");
	else
	{
		/* create a soft real-time task to be able to send requests */
		urt_task_attr tattr = {0};
		tattr.soft = true;

		requester_task = urt_task_new(requester, NULL, &tattr);
		if (requester_task == NULL)
			urt_err("error: could not create real-time task for requests\n");
		else
		{
			urt_task_start(requester_task);
			skin_resume(skin);
			main_loop();
		}
	}

	done = 1;
}

static void stop(struct data *d)
{
	cleanup();
}
