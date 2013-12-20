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

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <cmath>
#include <dirent.h>
#include <Ngin.h>

/*#define					SHMACROS_LOG_PREFIX	"View"
*/
#include <Ngin3d.h>
#include <shfont.h>
#include <skin.h>
#include <skin_motion.h>
#include <skin_user_motion.h>
#include <triangulate.h>
#include <scaler.h>
#include <filter.h>
#include <amplifier.h>
#include <vecmath.h>

using namespace std;

ofstream				fout("log.out");
bool					save_stat = false;
ofstream				statout;

bool					fullscreen		= false;
int					res_width		= 1024;
int					res_height		= 786;
#define					SH_WIDTH		res_width
#define					SH_HEIGHT		res_height
#define					FONT_SCALE		(res_width / 1000.0f)

/* skin */
static skin_object			skin;
static skin_reader			*sr;
static skin_service_manager		*sm;
static skin_acquisition_type		acq_method		= SKIN_ACQUISITION_ASAP;
static skin_rt_time			acq_period		= 50000000;

/* rendering data */
static vector<vector<triangle> >	modules_triangulated;
#define					TEMP_MESSAGE_TIME	3000
static string				temp_message;
static unsigned int			t_temp_message;

/* processing */
static bool				raw_results		= false;	// if raw results, all do_* will be ignored
static bool				do_scale		= true;
static bool				do_dampen		= true;
static bool				do_filter		= true;
static bool				do_amplify		= true;
static bool				raw_remove_baseline	= false;	// only if raw results
static int				filter_size		= 3;
static unsigned int			damp_size		= 6;
static unsigned int			t_last_damp		= 0;
#define					DAMP_PERIOD		100
//static amplifier			amp(-255, -255, 255);
//static amplifier			amp(-6, 3, 50.682993, -63.856570);
static amplifier			amp(0, 3, 50.182974, -63.226586);
struct layer_processed
{
	vector<uint8_t>	responses;
	uint64_t	last_read_count;
	scaler		sclr;
	filter		fltr;
};
static vector<layer_processed>		processed_layers;
static vector<skin_sensor_response>	baseline_response;

/* motion service */
static bool				motion_service_enabled	= false;
static bool				umotion_service_enabled	= false;
static skin_service			motion_service;
static skin_service			umotion_service;
struct motion
{
	int64_t			from[3],
				to[3];
	char			detected;
};

/* gui */
static bool				show_triangles		= false;
static bool				show_values		= false;
static bool				show_ids		= false;
static float				meter_scale		= 1.0f;
static float				sensor_radius_mult	= 1.5;
static float				value_font_size		= 0.01;
static float				sensor_color_good[]	= {0, 0.5f, 1};
static float				sensor_color_paused[]	= {1, 1, 0};
static float				sensor_color_bad[]	= {1, 0, 0};
//static unsigned char			sensor_color_bad[]	= {0, 0.5f, 1};
static map<skin_sensor_id, vector<skin_sensor_id> >		// maps a blacklisted id to a vector of its replacements
					blacklist;

/* other */
static char				home_path[PATH_MAX + 1]	= "";
static bool				_must_exit		= false;
static int				_error_code		= EXIT_SUCCESS;

void quit(int error_code)
{
	fout << "Quitting with error code " << error_code << endl;
	if (motion_service_enabled)
		skin_service_disconnect(&motion_service);
	if (umotion_service_enabled)
		skin_service_disconnect(&umotion_service);
	shNgin3dQuit();
	shNginQuit();
	SDL_Quit();
	_must_exit = true;
	_error_code = error_code;
}

bool initializing = true;

void initialize_SDL()
{
	if (SDL_Init(SDL_INIT_VIDEO) == -1)
	{
		fout << "Could not initialize SDL video..." << SDL_GetError() << endl;
		fout << "Exiting program" << endl;
		exit(0);
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
			fout << "Failed to detect screen resolution.  Using defaults" << endl;
		else
		{
			res_width = w;
			res_height = h;
			fout << "Screen resolution detected as " << res_width << "x" << res_height << endl;
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
		fout << "Could not open an OpenGL window..." << SDL_GetError() << endl;
		quit(EXIT_FAILURE);
		return;
	}
	initializing = true;
	SDL_ShowCursor(SDL_DISABLE);
}

void change_acq_method()
{
	skin_reader_stop(sr, SKIN_ALL_SENSOR_TYPES);
	switch (acq_method)
	{
		default:
		case SKIN_ACQUISITION_ASAP:
			acq_method = SKIN_ACQUISITION_PERIODIC;
			break;
		case SKIN_ACQUISITION_PERIODIC:
			acq_method = SKIN_ACQUISITION_SPORADIC;
			break;
		case SKIN_ACQUISITION_SPORADIC:
			acq_method = SKIN_ACQUISITION_ASAP;
			break;
	}
	skin_reader_start(sr, SKIN_ALL_SENSOR_TYPES, acq_method, acq_period);
}

void enable_motion_service(bool en)
{
	if (en)
	{
		if (skin_service_manager_connect_to_periodic_service(sm, SKIN_MOTION_SHMEM, SKIN_MOTION_RWLOCK, &motion_service) != SKIN_SUCCESS)
		{
			fout << "Could not connect to motion service" << endl;
			motion_service_enabled = false;
			temp_message = "Kernel motion service unavailable";
			t_temp_message = SDL_GetTicks();
		}
	}
	else
		skin_service_disconnect(&motion_service);
}

void enable_umotion_service(bool en)
{
	if (en)
	{
		if (skin_service_manager_connect_to_periodic_service(sm, SKIN_USER_MOTION_SHMEM, SKIN_USER_MOTION_RWLOCK, &umotion_service) != SKIN_SUCCESS)
		{
			fout << "Could not connect to motion service" << endl;
			umotion_service_enabled = false;
			temp_message = "User motion service unavailable";
			t_temp_message = SDL_GetTicks();
		}
	}
	else
		skin_service_disconnect(&umotion_service);
}

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
	for (skin_sensor_type_id j = 0; j < skin_object_sensor_types_count(&skin); ++j)
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

static int _calc_baseline(skin_sensor *s, void *d)
{
	baseline_response[s->id] = s->get_response();
	return SKIN_CALLBACK_CONTINUE;
}

void print_structures()
{
	fout << "Data structure:" << endl;
	fout << skin_object_sensor_types_count(&skin) << " sensor types:" << endl;
	skin_object_for_each_sensor_type(&skin, _print_sensor_type, NULL);
	fout << skin_object_sensors_count(&skin) << " sensors:" << endl;
	skin_object_for_each_sensor(&skin, _print_sensor, NULL);
	fout << skin_object_modules_count(&skin) << " modules:" << endl;
	skin_object_for_each_module(&skin, _print_module, NULL);
	fout << skin_object_patches_count(&skin) << " patches:" << endl;
	skin_object_for_each_patch(&skin, _print_patch, NULL);
	fout << skin_object_sub_regions_count(&skin) << " sub_regions:" << endl;
	skin_object_for_each_sub_region(&skin, _print_sub_region, NULL);
	fout << skin_object_regions_count(&skin) << " regions:" << endl;
	skin_object_for_each_region(&skin, _print_region, NULL);
	temp_message = "Data Structures Dumped to log.out";
	t_temp_message = SDL_GetTicks();
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
		case SDLK_0:
		case SDLK_1:
		case SDLK_2:
		case SDLK_3:
		case SDLK_4:
		case SDLK_5:
		case SDLK_6:
		case SDLK_7:
		case SDLK_8:
		case SDLK_9:
			if (kbe->type == SDL_KEYUP)
			{
				skin_sensor_type_id this_type = kbe->keysym.sym - SDLK_0;
				if (skin_reader_is_paused(sr, this_type))
					skin_reader_resume(sr, this_type);
				else
					skin_reader_pause(sr, this_type);
			}
			break;
		/* processing */
		case SDLK_r:
			if (kbe->type == SDL_KEYUP)
			{
				raw_results = !raw_results;
				if (raw_results)
				{
					value_font_size *= 0.7;
					skin_object_for_each_sensor(&skin, _calc_baseline, NULL);
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
					skin_object_for_each_sensor(&skin, _calc_baseline, NULL);
			}
			break;
		case SDLK_RETURN:
			if (kbe->type == SDL_KEYUP)
				for (vector<layer_processed>::iterator i = processed_layers.begin(), end = processed_layers.end(); i != end; ++i)
					i->sclr.reset();
			break;
		case SDLK_MINUS:
		case SDLK_UNDERSCORE:
			if (kbe->type == SDL_KEYUP)
				if (filter_size > 2)
				{
					--filter_size;
					for (vector<layer_processed>::iterator i = processed_layers.begin(), end = processed_layers.end(); i != end; ++i)
						i->fltr.change_size(filter_size);
				}
			break;
		case SDLK_PLUS:
		case SDLK_EQUALS:
			if (kbe->type == SDL_KEYUP)
			{
				++filter_size;
				for (vector<layer_processed>::iterator i = processed_layers.begin(), end = processed_layers.end(); i != end; ++i)
					i->fltr.change_size(filter_size);
			}
			break;
		/* motion service */
		case SDLK_m:
			if (kbe->type == SDL_KEYUP)
			{
				motion_service_enabled = !motion_service_enabled;
				umotion_service_enabled = false;
				enable_motion_service(motion_service_enabled);
				enable_umotion_service(umotion_service_enabled);
			}
			break;
		case SDLK_n:
			if (kbe->type == SDL_KEYUP)
			{
				umotion_service_enabled = !umotion_service_enabled;
				motion_service_enabled = false;
				enable_motion_service(motion_service_enabled);
				enable_umotion_service(umotion_service_enabled);
			}
			break;
		/* gui */
		case SDLK_t:				/* undocumented */
			if (kbe->type == SDL_KEYUP)
				show_triangles = !show_triangles;
			break;
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
			quit(EXIT_SUCCESS);
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
	while (SDL_PollEvent(&e) && !_must_exit)
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
				quit(EXIT_SUCCESS);
			default:
				break;
		}
	}
}

shNginTexture skybox[6];		// right, front, left, back, up, down

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
	float up[3] = {0.0f, 0.0f, 1.0f};	// if direction was not (0, 0, 1)
	if (ZERO(direction[0]) && ZERO(direction[1]))
	{
		up[1] = 1.0f;			// just change it.  Doesn't matter to what
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

static int _save_responses(skin_sensor_type *st, void *)
{
	uint64_t num_read = sr->tasks_statistics[st->id].number_of_reads;
	save_response_data d;
	if (processed_layers[st->id].last_read_count == num_read && !skin_reader_is_paused(sr, st->id))
		return SKIN_CALLBACK_CONTINUE;
	processed_layers[st->id].last_read_count = num_read;
	d.type_sensors_begin = st->sensors_begin;
	if (raw_results || !do_scale)
		skin_sensor_type_for_each_sensor(st, _save_raw_response, &d);
	else
	{
		d.temp.resize(skin_sensor_type_sensors_count(st));
		skin_sensor_type_for_each_sensor(st, _save_temp_response, &d);
		processed_layers[st->id].responses = processed_layers[st->id].sclr.scale(d.temp);
	}
	if (raw_results)
		return SKIN_CALLBACK_CONTINUE;
	if (do_filter)
	{
		processed_layers[st->id].fltr.new_responses(processed_layers[st->id].responses);
		processed_layers[st->id].responses = processed_layers[st->id].fltr.get_responses();
	}
	if (do_amplify)
		processed_layers[st->id].responses = amp.amplify(processed_layers[st->id].responses);
	return SKIN_CALLBACK_CONTINUE;
}

void update_responses()
{
	if (acq_method == SKIN_ACQUISITION_SPORADIC)
		skin_reader_request(sr, SKIN_ALL_SENSOR_TYPES, NULL);
	if (!raw_results && do_dampen)
		if (SDL_GetTicks() > t_last_damp + DAMP_PERIOD)
		{
			for (vector<layer_processed>::iterator i = processed_layers.begin(), end = processed_layers.end(); i != end; ++i)
				i->sclr.dampen(damp_size);
			t_last_damp += DAMP_PERIOD;
		}
	skin_object_for_each_sensor_type(&skin, _save_responses, NULL);
}

motion *copy_motion_service_res(unsigned int *count)
{
	static bool error_given = false;
	motion *copy = NULL;
	unsigned int motion_res_count = 0;
	if (motion_service_enabled)
		motion_res_count = SKIN_SERVICE_RESULT_COUNT(motion_service.memory);
	else
		motion_res_count = SKIN_SERVICE_RESULT_COUNT(umotion_service.memory);
	if (motion_res_count > 0)
	{
		copy = new motion[motion_res_count];
		if (copy == NULL)
			fout << "No memory!" << endl;
	}
	if (copy)
	{
		if (skin_rt_init() == SKIN_RT_FAIL)
		{
			if (!error_given)
			{
				fout << "Could not make main thread work with realtime functions" << endl;
				error_given = true;
			}
		}
		else
		{
			if (motion_service_enabled)
			{
				skin_service_lock(&motion_service, NULL);
				skin_motion *mem = (skin_motion *)motion_service.memory;
				for (unsigned int i = 0; i < motion_res_count; ++i)
				{
					copy[i].to[0] = mem[i].to[0];
					copy[i].to[1] = mem[i].to[1];
					copy[i].to[2] = mem[i].to[2];
					copy[i].from[0] = mem[i].from[0];
					copy[i].from[1] = mem[i].from[1];
					copy[i].from[2] = mem[i].from[2];
					copy[i].detected = mem[i].detected;
				}
				skin_service_unlock(&motion_service);
			}
			else
			{
				skin_service_lock(&umotion_service, NULL);
				skin_user_motion *mem = (skin_user_motion *)umotion_service.memory;
				for (unsigned int i = 0; i < motion_res_count; ++i)
				{
					copy[i].to[0] = mem[i].to[0];
					copy[i].to[1] = mem[i].to[1];
					copy[i].to[2] = mem[i].to[2];
					copy[i].from[0] = mem[i].from[0];
					copy[i].from[1] = mem[i].from[1];
					copy[i].from[2] = mem[i].from[2];
					copy[i].detected = mem[i].detected;
				}
				skin_service_unlock(&umotion_service);
			}
			skin_rt_stop();
			error_given = false;
		}
	}
	*count = motion_res_count;
	return copy;
}

void draw_triangles()
{
	for (unsigned int m = 0, size = modules_triangulated.size(); m < size; ++m)
	{
		glColor3ub(128, 255, 128);
		for (unsigned int t = 0, size2 = modules_triangulated[m].size(); t < size2; ++t)
		{
			glBegin(GL_TRIANGLES);
#define TRIANGLE_DISP 0.1
			glNormal3fv(modules_triangulated[m][t].normal);
			glVertex3f(modules_triangulated[m][t].p1[0] * meter_scale,
					modules_triangulated[m][t].p1[1] * meter_scale,
					modules_triangulated[m][t].p1[2] * meter_scale - TRIANGLE_DISP);
			glVertex3f(modules_triangulated[m][t].p2[0] * meter_scale,
					modules_triangulated[m][t].p2[1] * meter_scale,
					modules_triangulated[m][t].p2[2] * meter_scale - TRIANGLE_DISP);
			glVertex3f(modules_triangulated[m][t].p3[0] * meter_scale,
					modules_triangulated[m][t].p3[1] * meter_scale,
					modules_triangulated[m][t].p3[2] * meter_scale - TRIANGLE_DISP);
			glEnd();
		}
	}
}

struct draw_sensor_state
{
	bool is_bad;
	bool is_paused;
};

static int _draw_sensor(skin_sensor *s, void *d)
{
	uint8_t response = processed_layers[s->sensor_type_id].responses[s->id - skin_sensor_sensor_type(s)->sensors_begin];
	uint8_t color = ((255u - response) * 2 + 0) / 3;
	float *color_coef = sensor_color_good;
	draw_sensor_state *state = (draw_sensor_state *)d;
	if (state->is_bad)
		color_coef = sensor_color_bad;
	else if (state->is_paused)
		color_coef = sensor_color_paused;
	else
		color_coef = sensor_color_good;
	glColor3ub(color * color_coef[0], color * color_coef[1], color * color_coef[2]);
	glPushMatrix();
	glTranslatef(s->global_position[0] * meter_scale,
			s->global_position[1] * meter_scale,
			s->global_position[2] * meter_scale);
	float z[3] = {s->global_orientation[0], s->global_orientation[1], s->global_orientation[2]};
	float x[3] = {0.0f, 0.0f, 1.0f};	// if normal was (0, 1, 0)
	if (!ZERO(z[0]) || !ZERO(z[1]-1.0f) || !ZERO(z[2]))
	{
		// multiply (0, 1, 0) by normal which should give a vector perpendecular to normal
		x[0] = z[2];
		x[1] = 0.0f;
		x[2] = -z[0];
	}
	float y[3];
	CROSS(y, z, x);
	float height = response / 60.0f;//(255 - response)/60.0f;
	glBegin(GL_QUADS);
	float radius = s->radius*sensor_radius_mult*meter_scale;
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

static int _draw_module_sensors(skin_module *m, void *d)
{
	draw_sensor_state state;
	state.is_bad = !skin_module_sensor_type(m)->is_active;
	state.is_paused = skin_reader_is_paused(sr, m->sensor_type_id);
	skin_module_for_each_sensor(m, _draw_sensor, &state);
	return SKIN_CALLBACK_CONTINUE;
}

static int _draw_value(skin_sensor *s, void *d)
{
	glPushMatrix();
	glTranslatef(s->global_position[0] * meter_scale,
			s->global_position[1] * meter_scale,
			s->global_position[2] * meter_scale);
	float z[3] = {s->global_orientation[0], s->global_orientation[1], s->global_orientation[2]};
	skin_sensor_response response = processed_layers[s->sensor_type_id].responses[s->id - skin_sensor_sensor_type(s)->sensors_begin];
	float height = response / 60.0f;//(255 - response)/60.0f;

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
			{
				if (baseline_response[s->id] > response)
					baseline_response[s->id] = response;
				response -= baseline_response[s->id];
			}
		}
		snprintf(value, 10, "%u", (unsigned int)response);
	}
	else	// if (show_ids)
		snprintf(value, 10, "%u", (unsigned int)s->id);
	glTranslatef(z[0] * loc - shFontTextWidth(value) / 2, z[1] * loc + shFontTextHeight(value) / 2, z[2] * loc);
	shFontWrite(NULL, "%s", value);

	glPopMatrix();
	return SKIN_CALLBACK_CONTINUE;
}

void draw_hud()
{
	glDisable(GL_DEPTH_TEST);
	glPushMatrix();				// save modelview
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();				// save projection
	glLoadIdentity();
	glOrtho(0, res_width, 0, res_height, -10000.0f, 10000.0f);
	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	shFontColor(200, 150, 0);
	shFontColor(255, 200, 50);
	shFontSize(0.5f * FONT_SCALE);
	glTranslatef(20, SH_HEIGHT - 20, 0);
	if (raw_results)
		shFontWrite(NULL, "Raw values%s", raw_remove_baseline?" (baseline removed)":"");
	else
		shFontWrite(NULL, "Scale: %s\t\tFilter: %s (Size: %d)\tAmplification: %s\tHysteresis Compensation: %s",
				do_scale?"Yes":"No", do_filter?"Yes":"No", filter_size, do_amplify?"Yes":"No", do_dampen?"Yes":"No");

	glLoadIdentity();
	shFontColor(0, 200, 60);
	shFontColor(50, 255, 100);
	shFontSize(0.5f * FONT_SCALE);
	glTranslatef(20, SH_HEIGHT - 20 - shFontTextHeight("\n"), 0);
	char period[30] = "Unused";
	if (acq_method == SKIN_ACQUISITION_PERIODIC)
		snprintf(period, sizeof(period), "%8llu", acq_period);
	shFontWrite(NULL, "Acquisition Method: %s (Period: %s)\t\tMotion Tracking: %s", acq_method == SKIN_ACQUISITION_ASAP?"ASAP":
			acq_method == SKIN_ACQUISITION_PERIODIC?"Periodic":"Sporadic", period,
			motion_service_enabled?"Kernel Service": umotion_service_enabled?"User Service":"Disabled");

	glLoadIdentity();
	shFontColor(255, 255, 255);
	shFontSize(0.5f * FONT_SCALE);
	glTranslatef(20, SH_HEIGHT - 20 - shFontTextHeight("\n") * 2, 0);
	shFontWrite(NULL, "Legend:");
	glTranslatef(shFontTextWidth("Legend: "), 0, 0);
	shFontColor(255 * sensor_color_good[0], 255 * sensor_color_good[1], 255 * sensor_color_good[2]);
	shFontWrite(NULL, "Working");
	glTranslatef(shFontTextWidth("Working"), 0, 0);
	shFontColor(255, 255, 255);
	shFontWrite(NULL, ",");
	glTranslatef(shFontTextWidth(", "), 0, 0);
	shFontColor(255 * sensor_color_paused[0], 255 * sensor_color_paused[1], 255 * sensor_color_paused[2]);
	shFontWrite(NULL, "Paused");
	glTranslatef(shFontTextWidth("Paused"), 0, 0);
	shFontColor(255, 255, 255);
	shFontWrite(NULL, ",");
	glTranslatef(shFontTextWidth(", "), 0, 0);
	shFontColor(255 * sensor_color_bad[0], 255 * sensor_color_bad[1], 255 * sensor_color_bad[2]);
	shFontWrite(NULL, "Bad");
	glTranslatef(shFontTextWidth("Bad"), 0, 0);

	glLoadIdentity();
	shFontColor(255, 255, 255);
	shFontSize(0.5f * FONT_SCALE);
#if 0
	glTranslatef(20, 20 + shFontTextHeight("\n") * 16 /* number of lines */, 0);
	shFontWrite(NULL, "Keys:\n"
			  "q: Acq. Method\n"
			  "r: Raw Values\n"
			  "l: Scale\n"
			  "f: Filter\n"
			  "k: Amplify\n"
			  "h: Hysteresis Compensation\n"
			  "m: Motion Service\n"
			  "n: User Motion Track\n"
			  "p: Print Structure\n"
			  "0-%d: Pause/Resume Layer\n"
			  "+/-: Filter Size\n"
			  "Return: Reset Scaler\n"
			  "w/a/s/d/Ctrl/Space: Movement\n"
			  "Mouse: Look Around"
			, skin_object_sensor_types_count(&skin) > 10?9:(skin_object_sensor_types_count(&skin) - 1));
#endif
	glTranslatef(20, 20 + shFontTextHeight("\n") * 6 /* number of lines */, 0);
	shFontWrite(NULL, "Keys:\n"
			  "w/a/s/d/Ctrl/Space: Movement\tMouse: Look Around\t\tPage Up/Down: Sensor Size\n"
			  "q: Acquisition Method\t\t\tm: Kernel Motion Service\tn: User Motion Service\n"
			  "r: Raw Values\t\t\t\t0-%d: Pause/Resume Layer\tp: Print Structure\n"
			  "v: Show Values\t   i: Show Ids\tl: Scale\t    f: Filter\t\tk: Amplify\n"
			  "h: Hysteresis Compensation\t\t+/-: Filter Size\t\t\tReturn: Reset Scaler\n"
			, skin_object_sensor_types_count(&skin) > 10?9:(skin_object_sensor_types_count(&skin) - 1));

	if (SDL_GetTicks() < t_temp_message + TEMP_MESSAGE_TIME)
	{
		glLoadIdentity();
		shFontColor(255, 50, 50);
		shFontSize(0.6f * FONT_SCALE);
		glTranslatef((SH_WIDTH - shFontTextWidth(temp_message.c_str())) / 2, SH_HEIGHT / 2 - 32 * 0.6f, 0);
		shFontWrite(NULL, "%s", temp_message.c_str());
	}

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();				// restore projection
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();				// restore modelview
	glEnable(GL_DEPTH_TEST);
}

void render_screen()
{
	shNgin3dEffectuate(dt);
	shNginClear();
	draw_sky_box();
	shNginDisableTexture();
	update_responses();
	motion *motion_copy = NULL;
	unsigned int motion_res_count = 0;
	if (motion_service_enabled || umotion_service_enabled)
		motion_copy = copy_motion_service_res(&motion_res_count);
	if (show_triangles)
		draw_triangles();
	skin_object_for_each_module(&skin, _draw_module_sensors, NULL);
	if (show_values || show_ids)
		skin_object_for_each_sensor(&skin, _draw_value, NULL);
	if (motion_copy != NULL)
	{
		glColor3ub(255, 0, 0);
		for (unsigned int i = 0; i < motion_res_count; ++i)
		{
			if (!motion_copy[i].detected)
				continue;
			draw_motion_arrow(motion_copy[i].to, motion_copy[i].from);
		}
	}
	if (motion_copy)
		delete[] motion_copy;
	draw_hud();
	if (save_stat)
	{
		skin_sensor_type *sensor_types = skin_object_sensor_types(&skin, NULL);
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
	SDL_GL_SwapBuffers();
}

void load_data(string home)
{
	skybox[0].shNginTLoad((home + "room_east.bmp").c_str());
	skybox[1].shNginTLoad((home + "room_north.bmp").c_str());
	skybox[2].shNginTLoad((home + "room_west.bmp").c_str());
	skybox[3].shNginTLoad((home + "room_south.bmp").c_str());
	skybox[4].shNginTLoad((home + "room_up.bmp").c_str());
	skybox[5].shNginTLoad((home + "room_down.bmp").c_str());
	for (int i = 0; i < 6; ++i)
		if (!skybox[i].shNginTIsLoaded())
			fout << "Sky box " << i << " didn't load" << endl;
	if (shFontLoad((home + "font.shf").c_str()) == SH_FONT_FILE_NOT_FOUND)
		fout << "Could not load font" << endl;
	else
	{
		shFontSize(0.5f * FONT_SCALE);
		shFontColor(255, 0, 0);
		shFontShadowColor(0, 0, 0);
		shFontShadow(SH_FONT_FULL_SHADOW);
		fout << "shFont done (version: "SH_FONT_VERSION")" << endl;
	        fout << "Using shImage (version " << SH_IMAGE_VERSION << ")" << endl;
	}
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
	}
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

void initialize_Ngin3d(const char *settings, float middleX, float middleY)
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
	shNgin3dMove(-100, middleX, middleY, 1000);
}

static int _init_filters(skin_sensor_type *st, void *d)
{
	processed_layers[st->id].responses.resize(skin_sensor_type_sensors_count(st));
	processed_layers[st->id].last_read_count = 0;
	processed_layers[st->id].fltr.change_size(filter_size);
	processed_layers[st->id].sclr.set_range(512);
	return SKIN_CALLBACK_CONTINUE;
}

static int _triangulate_module(skin_module *m, void *d)
{
	char percent[] = "    ";
	skin_module_size *modules_count = (skin_module_size *)d;

	modules_triangulated.push_back(triangulate(m));
	sprintf(percent, "%3u", (unsigned int)m->id * 100 / *modules_count);
	cout << "\b\b\b\b" << percent << '%' << flush;

	return SKIN_CALLBACK_CONTINUE;
}

struct min_max_data
{
	float minX, maxX;
	float minY, maxY;
};

static int _find_min_max(skin_sensor *s, void *d)
{
	min_max_data *data = (min_max_data *)d;

	if (s->global_position[0] < data->minX)
		data->minX = s->global_position[0];
	if (s->global_position[0] > data->maxX)
		data->maxX = s->global_position[0];
	if (s->global_position[1] < data->minY)
		data->minY = s->global_position[1];
	if (s->global_position[1] > data->maxY)
		data->maxY = s->global_position[1];

	return SKIN_CALLBACK_CONTINUE;
}

int main(int argc, char **argv)
{
	int frame;
	fout << "Initializing..." << endl;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			cout << "Usage: " << argv[0] << " --help\n"
				"       " << argv[0] << " [--fullscreen] [--save-stat]" << endl << endl;
			return 0;
		}
		else if (strcmp(argv[i], "--fullscreen") == 0 || strcmp(argv[i], "-f") == 0)
			fullscreen = true;
		else if (strcmp(argv[i], "--save-stat") == 0 || strcmp(argv[i], "-s") == 0)
		{
			save_stat = true;
			statout.open("stat.out");
		}
	}
	bool use_defaults = false;
	char exelink[50];
	sprintf(exelink, "/proc/%d/exe", getpid());
	int bytes = readlink(exelink, home_path, sizeof(home_path) - 1);
	string home;
	if (bytes <= 0)
	{
		fout << "Failed to retrieve home directory.  Will use default settings" << endl;
		use_defaults = true;
	}
	else
	{
		for (--bytes; bytes >= 0; --bytes)
			if (home_path[bytes] == '/')
			{
				home_path[bytes] = '\0';
				break;
			}
		if (bytes < 0)		// could not find /, which means the file name didn't contain any directory!
			home = ".";	// assume .  (it shouldn't happen though!)
		else
			home = home_path;
		int length = home.length();
		if (home[length - 1] != '/')
			home += '/';
	}
	if (!use_defaults)
		read_triangulation_settings((home+"../settings/triangulation_settings").c_str());
	skin_object_init(&skin);
	if (skin_object_load(&skin) != SKIN_SUCCESS)
	{
		cout << "Could not initialize skin" << endl;
		return EXIT_FAILURE;
	}
	baseline_response.resize(skin_object_sensors_count(&skin));
	skin_sensor_type_size types_count = skin_object_sensor_types_count(&skin);
	processed_layers.resize(types_count);
	skin_object_for_each_sensor_type(&skin, _init_filters, NULL);
	skin_module_size modules_count = skin_object_modules_count(&skin);
	cout << "Constructing module shapes...  0%" << flush;
	skin_object_for_each_module(&skin, _triangulate_module, &modules_count);
	cout << "\b\b\b\b done" << endl;
	fout << "Constructing module shapes done" << endl;
	min_max_data min_max;
	min_max.minX = 10000000.0f;
	min_max.maxX = -10000000.0f;
	min_max.minY = 10000000.0f;
	min_max.maxY = -10000000.0f;
	skin_object_for_each_sensor(&skin, _find_min_max, &min_max);
	if (min_max.maxX == min_max.minX)
	{
		fout << "maxX is equal to minX" << endl;
		meter_scale = 1;
	}
	else if (min_max.maxX - min_max.minX > min_max.maxY - min_max.minY)
		meter_scale = 100.0f / (min_max.maxX - min_max.minX);
	else
		meter_scale = 100.0f / (min_max.maxY - min_max.minY);
	if (modules_count > 0)
	{
		skin_sensor_size module_sensors_count;
		skin_sensor *module_sensors = skin_module_sensors(&skin_object_modules(&skin, NULL)[0], &module_sensors_count);
		double min_distance = 1e12;
		float min_dist_diameter = 1;
		for (skin_sensor_id i = 0; i < module_sensors_count; ++i)
			for (skin_sensor_id j = i + 1; j < module_sensors_count; ++j)
			{
				double dist = DISTANCE(module_sensors[i].relative_position, module_sensors[j].relative_position);
				if (dist < min_distance)
				{
					min_distance = dist;
					min_dist_diameter = module_sensors[i].radius+module_sensors[j].radius;
				}
			}
		sensor_radius_mult = min_distance / min_dist_diameter * 1.35;	// visually, * 1.35 looked good
		value_font_size = min_distance * meter_scale * 0.008;
	}
	initialize_SDL();
	if (_must_exit)
	{
		skin_object_free(&skin);
		return _error_code;
	}
	fout << "SDL done" << endl;
	initialize_Ngin();
	fout << "Ngin done (version: "NGIN_VERSION")" << endl;
	initialize_Ngin3d((home + "../settings/Ngin3d_settings").c_str(),
			(min_max.maxX + min_max.minX) / 2 * meter_scale, (min_max.maxY + min_max.minY) / 2 * meter_scale);
	fout << "Ngin3d done (version: "NGIN3D_VERSION")" << endl;
	load_data(home + "../gui/");
	fout << "gui data loaded" << endl;
	sr = skin_object_reader(&skin);
	sm = skin_object_service_manager(&skin);
	skin_reader_start(sr, SKIN_ALL_SENSOR_TYPES, SKIN_ACQUISITION_ASAP, 0);
	fout << "skin done (version: "SKIN_VERSION")" << endl;
	frame = 0;
	t_last_damp = SDL_GetTicks();
	unsigned int last = SDL_GetTicks();
	unsigned int last_time = last;
	while (true)
	{
		unsigned int now = SDL_GetTicks();
		if (now >= last+1000)
		{
			fout << (FPS = frame * 1000 / (now - last)) << " fps" << endl;
			last += 1000;
			frame = 0;
		}
		dt = now - last_time;
		last_time = now;
		process_events();
		if (_must_exit)
			break;
		render_screen();
		++frame;
	}
	skin_object_free(&skin);
	return _error_code;
}
