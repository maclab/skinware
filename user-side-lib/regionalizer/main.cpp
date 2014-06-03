/*
 * Copyright (C) 2011-2013  Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * The research leading to these results has received funding from
 * the European Commission's Seventh Framework Programme (FP7) under
 * Grant Agreement n.  231500 (ROBOSKIN).
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
#include <vector>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cmath>
#include <dirent.h>
#include <Ngin.h>
#include <Ngin3d.h>
#include <shfont.h>
#include <skin.h>
#include <triangulate.h>
#include <vecmath.h>

using namespace std;

static char			home_path[PATH_MAX + 1]	= "";

struct module_mesh
{
	vector<triangle> triangles;
	float center[3];		// center, radius is used to clip modules that cannot
	float radius;			// possibly coincide with the selection beam
	vector<skin_region_id> regions; // regions it belongs to.  Filled during execution
};

struct color
{
	unsigned int numerator1, numerator2;
	unsigned int rgb;	// 0, 1, or 2 (showing missing color)
};

vector<set<skin_module_id> >	regions;
vector<color>			region_colors;
set<skin_module_id>		cur_region;
vector<skin_module_id>		insertion_order;
vector<skin_module_id>		candidate_modules;
unsigned int			selected_module		= 0;
vector<module_mesh>		modules_triangulated;

uint8_t				selection_threshold	= 127;

FILE *fout = NULL;

//#define SH_FULLSCREEN
#ifdef SH_FULLSCREEN
int SH_RES_WIDTH =		1024;
int SH_RES_HEIGHT =		786;
#else
#define SH_RES_WIDTH		1024
#define SH_RES_HEIGHT		768
#endif
#define SH_WIDTH		SH_RES_WIDTH
#define SH_HEIGHT		SH_RES_HEIGHT

static skin_object		skin;
static bool			_must_exit		= false;
static int			_error_code		= EXIT_FAILURE;

#define log(...)				\
do {						\
	if (fout)				\
		fprintf(fout, __VA_ARGS__);	\
	fprintf(stderr, __VA_ARGS__);		\
} while (0)

void quit(int error_code)
{
	log("Quitting with error code %d\n", error_code);
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
		log("Could not initialize SDL video... %s\n", SDL_GetError());
		log("Exiting program\n");
		exit(0);
	}
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
#ifdef SH_FULLSCREEN
	fflush(stdout);
	char temp[20];
	FILE *resin = popen("xdpyinfo | grep 'dimensions:'", "r");
	if (resin)
	{
		fscanf(resin, "%s %dx%d", temp, &SH_RES_WIDTH, &SH_RES_HEIGHT);
		log("Screen resolution detected as %dx%d\n", SH_RES_WIDTH, SH_RES_HEIGHT);
		pclose(resin);
	}
	if (SDL_SetVideoMode(SH_RES_WIDTH, SH_RES_HEIGHT, 0, SDL_OPENGL|SDL_FULLSCREEN) == NULL)
#else
	if (SDL_SetVideoMode(SH_RES_WIDTH, SH_RES_HEIGHT, 0, SDL_OPENGL) == NULL)
#endif
	{
		log("Could not open an OpenGL window... %s\n", SDL_GetError());
		quit(EXIT_SUCCESS);
		return;
	}
	initializing = true;
	SDL_ShowCursor(SDL_DISABLE);
}

char				message[1000];
bool				wireframe		= false;
bool				showsensors		= true;
bool				enable_mouse		= false;

bool				quitting		= false;
bool				cancel_regionalization	= false;

unsigned int gcd(unsigned a, unsigned int b)
{
	// Assumes a is bigger than b
	while (b != 0)
	{
		unsigned int temp = b;
		b = a % b;
		a = temp;
	}
	return a;
}

color get_next_color()
{
	color c;
	if (region_colors.size() == 0)
	{
		c.numerator1 = 1;
		c.numerator2 = 1;
		c.rgb = 0;
	}
	else
	{
		c = region_colors.back();
		if (c.rgb < 2)
			++c.rgb;
		else
		{
			unsigned int denom = c.numerator1+c.numerator2;
			unsigned int num = c.numerator1;
			bool found = false;
			for (++num; num < denom; ++num)
				if (gcd(denom, num) == 1)
				{
					found = true;
					break;
				}
			if (!found)
				for (++denom; denom < 10000; ++denom)
				{
					for (num = 1; num < denom; ++num)
						if (gcd(denom, num) == 1)
						{
							found = true;
							break;
						}
					if (found)
						break;
				}
			if (denom == 10000)
				c.numerator1 = 12345;
			else
				c.numerator1 = num;
			c.numerator2 = denom - c.numerator1;
			c.rgb = 0;
		}
	}
	return c;
}

bool				mouse_clicked		= false;
bool				can_right_click		= false;

void keyboard(SDL_KeyboardEvent *kbe)
{
	shNgin3dReceivedInput(NGIN3D_KEY, (shNgin3dKey)kbe->keysym.sym, (kbe->type == SDL_KEYDOWN)?NGIN3D_PRESSED:NGIN3D_RELEASED);
	switch (kbe->keysym.sym)
	{
		case SDLK_1:
			if (kbe->type == SDL_KEYUP)
				wireframe = !wireframe;
			break;
		case SDLK_2:
			if (kbe->type == SDL_KEYUP)
				showsensors = !showsensors;
			break;
		case SDLK_3:
			if (quitting)
				break;
			if (kbe->type == SDL_KEYUP)
			{
				enable_mouse = !enable_mouse;
				if (!enable_mouse)
				{
					for (unsigned int i = 0, size = candidate_modules.size(); i < size; ++i)
						modules_triangulated[candidate_modules[i]].regions.pop_back();
					candidate_modules.clear();
					selected_module = 0;
					can_right_click = false;
				}
			}
			break;
		case SDLK_r:
			if (quitting)
				break;
			if (kbe->type == SDL_KEYUP)
			{
				for (set<skin_module_id>::const_iterator i = cur_region.begin(), end = cur_region.end(); i != end; ++i)
					modules_triangulated[*i].regions.pop_back();
				cur_region.clear();
				insertion_order.clear();
				for (unsigned int i = 0, size = candidate_modules.size(); i < size; ++i)
					modules_triangulated[candidate_modules[i]].regions.pop_back();
				candidate_modules.clear();
				selected_module = 0;
				can_right_click = false;
			}
			break;
		case SDLK_RETURN:
			if (quitting)
				break;
			if (candidate_modules.size() > 0)	// still deciding which module
				break;
			if (kbe->type == SDL_KEYUP)
				if (cur_region.size() > 0)
				{
					region_colors.push_back(get_next_color());
					regions.push_back(cur_region);
					cur_region.clear();
					insertion_order.clear();
				}
			break;
		case SDLK_BACKSPACE:
			if (quitting)
				break;
			if (candidate_modules.size() > 0)	// still deciding which module
				break;
			if (kbe->type == SDL_KEYUP)
				if (insertion_order.size() > 0)
				{
					modules_triangulated[insertion_order.back()].regions.pop_back();
					cur_region.erase(insertion_order.back());
					insertion_order.pop_back();
				}
			break;
		case SDLK_DELETE:
			if (quitting)
				break;
			if (kbe->type == SDL_KEYUP)
			{
				for (set<skin_module_id>::const_iterator i = cur_region.begin(), end = cur_region.end(); i != end; ++i)
					modules_triangulated[*i].regions.pop_back();
				cur_region.clear();
				insertion_order.clear();
				for (unsigned int i = 0, size = candidate_modules.size(); i < size; ++i)
					modules_triangulated[candidate_modules[i]].regions.pop_back();
				candidate_modules.clear();
				selected_module = 0;
				can_right_click = false;
				if (regions.size() > 0)
				{
					for (set<skin_module_id>::const_iterator i = regions.back().begin(), end = regions.back().end(); i != end; ++i)
						modules_triangulated[*i].regions.pop_back();
					regions.pop_back();
				}
			}
			break;
		case SDLK_PAGEUP:
			if (quitting)
				break;
			if (kbe->type == SDL_KEYUP)
			{
				unsigned int candidate_count = candidate_modules.size();
				if (candidate_count > 0)
				{
					selected_module += candidate_count - 1;
					if (selected_module >= candidate_count)
						selected_module -= candidate_count;
				}
			}
			break;
		case SDLK_PAGEDOWN:
			if (quitting)
				break;
			if (kbe->type == SDL_KEYUP)
			{
				unsigned int candidate_count = candidate_modules.size();
				if (candidate_count > 0)
				{
					++selected_module;
					if (selected_module >= candidate_count)
						selected_module -= candidate_count;
				}
			}
			break;
		case SDLK_y:
			if (quitting)
			{
				cancel_regionalization = false;
				quit(EXIT_SUCCESS);
			}
			break;
		case SDLK_n:
			if (quitting)
			{
				cancel_regionalization = true;
				quit(EXIT_FAILURE);
			}
			break;
		case SDLK_ESCAPE:
			quitting = true;
			sprintf(message, "Proceed with regionalization? [Y]es/[N]o/[C]ancel");
			break;
		default:
			if (quitting)
			{
				quitting = false;
				message[0] = '\0';
			}
			break;
	}
}

void mouse(SDL_MouseButtonEvent *mbe)
{
	if (mbe->state == SDL_PRESSED)			// shouldn't happen.
		return;
	if (!enable_mouse)
		return;
	if (quitting)
		return;
	if (mbe->button == SDL_BUTTON_LEFT)
	{
		if ((uint64_t)regions.size() >= (uint64_t)SKIN_REGION_MAX - 1)
		{
			shFontColor(255, 0, 0);
			sprintf(message, "Too many regions!");
			return;
		}
		mouse_clicked = true;
		can_right_click = true;
	}
	else if (can_right_click && mbe->button == SDL_BUTTON_RIGHT)
	{
		can_right_click = false;
		if (candidate_modules.size() == 0)
			return;
		cur_region.insert(candidate_modules[selected_module]);
		for (unsigned int i = 0, size = candidate_modules.size(); i < size; ++i)
			if (i != selected_module)
				modules_triangulated[candidate_modules[i]].regions.pop_back();
			else
				insertion_order.push_back(candidate_modules[i]);
		candidate_modules.clear();
		selected_module = 0;
	}
	else if (mbe->button == SDL_BUTTON_WHEELUP)
	{
		unsigned int candidate_count = candidate_modules.size();
		if (candidate_count > 0)
		{
			selected_module += candidate_count - 1;
			if (selected_module >= candidate_count)
				selected_module -= candidate_count;
		}
	}
	else if (mbe->button == SDL_BUTTON_WHEELDOWN)
	{
		unsigned int candidate_count = candidate_modules.size();
		if (candidate_count > 0)
		{
			++selected_module;
			if (selected_module >= candidate_count)
				selected_module -= candidate_count;
		}
	}
}

int warpmotionx = 0, warpmotiony = 0;

void motion(SDL_MouseMotionEvent *mme)
{
	if (initializing)
	{
		if (mme->x == SH_RES_WIDTH / 2 && mme->y == SH_RES_HEIGHT / 2)
			initializing = false;
		else
			SDL_WarpMouse(SH_RES_WIDTH / 2, SH_RES_HEIGHT / 2);
	}
	else if (mme->xrel != -warpmotionx || -mme->yrel != -warpmotiony)
	{
		shNgin3dReceivedInput(NGIN3D_MOUSE_MOVE, mme->xrel, -mme->yrel);
		warpmotionx += mme->xrel;
		warpmotiony += -mme->yrel;
		SDL_WarpMouse(SH_RES_WIDTH / 2, SH_RES_HEIGHT / 2);
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
			case SDL_MOUSEBUTTONUP:
				mouse(&e.button);
				break;
			case SDL_MOUSEMOTION:
				motion(&e.motion);
				break;
			case SDL_QUIT:
				quit(EXIT_SUCCESS);
			default:
				break;
		}
	}
}

bool rescale_responses = false;
unsigned int response_range = 1000;
skin_sensor_response *sensor_mins;

uint8_t fix_response(skin_sensor *s)
{
	skin_sensor_response r = skin_sensor_get_response(s);
	if (!rescale_responses)
		return r >> 8;
	if (r > 7000)	// the first time could be 0, while the skinware actually starts reading
	{
		if (r < sensor_mins[s->id])
			sensor_mins[s->id] = r;
		r -= sensor_mins[s->id];
		if (r > response_range)
			r = response_range;
		r = (uint32_t)r * 255 / response_range;
	}
	return (uint8_t)r;
}

static int _check_sensor_and_add_module(skin_sensor *s, void *d)
{
	uint8_t response = fix_response(s);

	if (response <= selection_threshold)
		return SKIN_CALLBACK_CONTINUE;

	if (cur_region.find(s->module_id) != cur_region.end())
		return SKIN_CALLBACK_STOP;
	cur_region.insert(s->module_id);
	modules_triangulated[s->module_id].regions.push_back(regions.size());
	insertion_order.push_back(s->module_id);

	return SKIN_CALLBACK_CONTINUE;
}

static int _check_and_add_active_modules(skin_module *m, void *d)
{
	if (cur_region.find(m->id) == cur_region.end())		// no point in selecting the ones that are already confirmed
		skin_module_for_each_sensor(m, _check_sensor_and_add_module, NULL);

	return SKIN_CALLBACK_CONTINUE;
}

void add_active_modules()
{
	if (enable_mouse)
		return;
	skin_object_for_each_module(&skin, _check_and_add_active_modules, NULL);
}

float meter_scale = 1.0f;

struct module_with_z
{
	skin_module_id module;
	float z;
	bool operator <(const module_with_z &rhs) const
	{
		return z < rhs.z;
	}
};

void get_candidates()
{
	float pos[3], dir[3], up[3];
	shNgin3dGetCamera(pos, dir, up);
	dir[0] -= pos[0];
	dir[1] -= pos[1];
	dir[2] -= pos[2];
	for (unsigned int i = 0, size = candidate_modules.size(); i < size; ++i)
		modules_triangulated[candidate_modules[i]].regions.pop_back();
	candidate_modules.clear();
	vector<module_with_z> temp_candidates;
	for (unsigned int m = 0, size = modules_triangulated.size(); m < size; ++m)
	{
		if (cur_region.find(m) != cur_region.end())		// no point in selecting the ones that are already confirmed
			continue;
		float center[3], pc[3];
		MUL(center, modules_triangulated[m].center, meter_scale);
		SUB(pc, center, pos);
		if (DOT(pc, dir) <= 0 && NORM(pc) > modules_triangulated[m].radius*meter_scale)
			continue;	// Looking away from the sphere and not inside it
		float temp[3];
		CROSS(temp, pc, dir);
		if (NORM(temp) > modules_triangulated[m].radius*meter_scale)
			continue;	// Will not cross the sphere around the module
		for (unsigned int t = 0, size2 = modules_triangulated[m].triangles.size(); t < size2; ++t)
		{
			float a[3], b[3], c[3], norm[3], ca[3], ab[3]/*, bc[3]*/, ap[3];
			ASSIGN(a, modules_triangulated[m].triangles[t].p1);
			ASSIGN(b, modules_triangulated[m].triangles[t].p2);
			ASSIGN(c, modules_triangulated[m].triangles[t].p3);
			ASSIGN(norm, modules_triangulated[m].triangles[t].normal);
			MUL(a, a, meter_scale);
			MUL(b, b, meter_scale);
			MUL(c, c, meter_scale);
			SUB(ca, a, c);
			SUB(ab, b, a);
			//SUB(bc, c, b);
			SUB(ap, pos, a);
			if (DOT(norm, ap) <= 0)			// that is, position is behind the triangle, so triangle is not visible (can't select from behind of triangle)
				continue;
			if (DOT(norm, dir) >= 0)		// looking away from the triangle
				continue;
			// The line starting from pos in direction dir is:
			//   pos+kdir
			// The plane of the current triangle is:
			//   a+u(b-a)+v(c-a)
			// Intersectin results:
			//   u(b-a)+v(c-a)-kdir=pos-a
			// After solving, if k > 0, then it's on the right direction (Which should be anyway because we checked DOT(norm, dir) before as well as DOT(norm, ap)
			// If u, v > 0 and u+v<=1 then the intersection is inside the triangle.
			float denom_mat[3][3] = {{ab[0], -ca[0], -dir[0]},
				{ab[1], -ca[1], -dir[1]},
				{ab[2], -ca[2], -dir[2]}};
			float denom = DET3(denom_mat);
			if (ZERO(denom))			// shouldn't happen
				continue;
			float u_mat[3][3] = {{ap[0], -ca[0], -dir[0]},
				{ap[1], -ca[1], -dir[1]},
				{ap[2], -ca[2], -dir[2]}};
			float v_mat[3][3] = {{ab[0], ap[0], -dir[0]},
				{ab[1], ap[1], -dir[1]},
				{ab[2], ap[2], -dir[2]}};
			float k_mat[3][3] = {{ab[0], -ca[0], ap[0]},
				{ab[1], -ca[1], ap[1]},
				{ab[2], -ca[2], ap[2]}};
			float u = DET3(u_mat) / denom, v = DET3(v_mat) / denom, k = DET3(k_mat)/denom;
			if (k < 0 || u < 0 || v < 0 || u + v > 1)
				continue;
			module_with_z module;
			module.module = m;
			module.z = k;
			temp_candidates.push_back(module);
			modules_triangulated[m].regions.push_back(regions.size());
			break;
		}
	}
	sort(temp_candidates.begin(), temp_candidates.end());
	for (unsigned int i = 0, size = temp_candidates.size(); i < size; ++i)
		candidate_modules.push_back(temp_candidates[i].module);
	selected_module = 0;
	// If there was only one possible choice, just accept it (so here's the code that is taken from handling of right-click, although with fewer error-checking)
	if (candidate_modules.size() == 1)
	{
		can_right_click = false;
		cur_region.insert(candidate_modules[0]);
		insertion_order.push_back(candidate_modules[0]);
		candidate_modules.clear();
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

static int _render_sensor(skin_sensor *s, void *d)
{
	uint8_t response = fix_response(s);

	glColor3ub(response, response, response);
	glPushMatrix();
	glTranslatef(s->global_position[0] * meter_scale,
			s->global_position[1] * meter_scale,
			s->global_position[2] * meter_scale);
	shNginSphere(1, 5, 3);
	glPopMatrix();

	return SKIN_CALLBACK_CONTINUE;
}

void render_screen()
{
	shNgin3dEffectuate(dt);
	shNginClear();
	draw_sky_box();
	shNginDisableTexture();
	if (mouse_clicked)
	{
		get_candidates();
		mouse_clicked = false;
	}
	glEnable(GL_CULL_FACE);
	for (unsigned int m = 0, size = modules_triangulated.size(); m < size; ++m)
	{
		if (modules_triangulated[m].regions.size() == 0)
			glColor3ub(192, 192, 192);
		else if (modules_triangulated[m].regions.back() == regions.size())
		{
			if (candidate_modules.size() != 0 && m == candidate_modules[selected_module])
				glColor3ub(0, 255, 0);
			else
			{
				bool inCandidates = false;
				for (unsigned int i = 0, size2 = candidate_modules.size(); i < size2; ++i)
					if (m == candidate_modules[i])
					{
						inCandidates = true;
						break;
					}
				if (inCandidates)
					glColor3ub(0, 0, 255);
				else
					glColor3ub(255, 0, 0);
			}
		}
		else
		{
			color c = region_colors[modules_triangulated[m].regions.back()];
			unsigned char denom = c.numerator1 + c.numerator2;
			unsigned char c1 = c.numerator1 * 200 / denom + 25;
			unsigned char c2 = c.numerator2 * 200 / denom + 25;
			if (c.rgb == 0)
				glColor3ub(0, c1, c2);
			else if (c.rgb == 1)
				glColor3ub(c1, 0, c2);
			else
				glColor3ub(c1, c2, 0);
		}
		for (unsigned int t = 0, size2 = modules_triangulated[m].triangles.size(); t < size2; ++t)
		{
			if (wireframe)
				glBegin(GL_LINE_LOOP);
			else
				glBegin(GL_TRIANGLES);
			glNormal3fv(modules_triangulated[m].triangles[t].normal);
			glVertex3f(modules_triangulated[m].triangles[t].p1[0] * meter_scale,
					modules_triangulated[m].triangles[t].p1[1] * meter_scale,
					modules_triangulated[m].triangles[t].p1[2] * meter_scale);
			glVertex3f(modules_triangulated[m].triangles[t].p2[0] * meter_scale,
					modules_triangulated[m].triangles[t].p2[1] * meter_scale,
					modules_triangulated[m].triangles[t].p2[2] * meter_scale);
			glVertex3f(modules_triangulated[m].triangles[t].p3[0] * meter_scale,
					modules_triangulated[m].triangles[t].p3[1] * meter_scale,
					modules_triangulated[m].triangles[t].p3[2] * meter_scale);
			glEnd();
		}
	}
	if (showsensors)
		skin_object_for_each_sensor(&skin, _render_sensor, NULL);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-SH_RES_WIDTH / 2, SH_RES_WIDTH / 2, -SH_RES_HEIGHT / 2, SH_RES_HEIGHT / 2, -100000.0f, 100000.0f);
	glMatrixMode(GL_MODELVIEW);
	glDisable(GL_DEPTH_TEST);
	if (enable_mouse)
	{
		glColor3ub(255, 255, 255);
		glBegin(GL_LINES);
			glVertex3f(-15, 0, 0);
			glVertex3f(-4, 0, 0);
			glVertex3f(15, 0, 0);
			glVertex3f(4, 0, 0);
			glVertex3f(0, -15, 0);
			glVertex3f(0, -4, 0);
			glVertex3f(0, 15, 0);
			glVertex3f(0, 4, 0);
		glEnd();
		glBegin(GL_POINTS);
			glVertex3f(0, 0, 0);
		glEnd();
	}
	glTranslatef(-SH_WIDTH / 2 + 20, SH_HEIGHT / 2 - 15, 0);
	shFontColor(255, 255, 0);
	shFontWrite(NULL, "backspace\n"
			"delete\n"
			"r\n"
			"return\n"
			"esc\n"
			"%s",
			enable_mouse?"left-click\nright-click\nmouse-scroll\npage up/down":"touch");
	shFontColor(100, 200, 255);
	shFontWrite(NULL, "\t\t\tRemove last module from current region\n"
			"\t\t\tRemove last region\n"
			"\t\t\tRestart creation of current region\n"
			"\t\t\tConfirm and add current region\n"
			"\t\t\tExit [and regionalize]\n"
			"%s",
			enable_mouse?"\t\t\tShow possible modules\n\t\t\tSelect highlighted module\n\t\t\tChange selection\n\t\t\tChange selection":"\t\t\tSelect module");
	glLoadIdentity();
	glTranslatef(-SH_WIDTH / 2 + 20, -SH_HEIGHT / 2 + 15 + shFontTextHeight("\n\n\n"), 0);
	shFontColor(255, 255, 0);
	shFontWrite(NULL, "1\n"
			"2\n"
			"3");
	shFontColor(100, 200, 255);
	shFontWrite(NULL, "\t%s\n"
			"\t%s\n"
			"\t%s",
			wireframe?"Solid":"Wireframe",
			showsensors?"Hide sensors":"Show sensors",
			enable_mouse?"Disable mouse selection":"Enable mouse selection");
	shFontColor(255, 255, 0);
	glLoadIdentity();
	glTranslatef(SH_WIDTH / 2 - 100, SH_HEIGHT / 2 - 15, 0);
	shFontWrite(NULL, "%d fps", FPS);
	glLoadIdentity();
	shFontSize(0.75f);
	glTranslatef(-shFontTextWidth(message) / 2, 0, 0);
	shFontColor(255, 0, 0);
	shFontWrite(NULL, "%s", message);
	shFontSize(0.5f);
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glEnable(GL_DEPTH_TEST);
	SDL_GL_SwapBuffers();
}

void load_data(const char *home)
{
	string h = home;
	skybox[0].shNginTLoad((h+"../gui/room_east.bmp").c_str());
	skybox[1].shNginTLoad((h+"../gui/room_north.bmp").c_str());
	skybox[2].shNginTLoad((h+"../gui/room_west.bmp").c_str());
	skybox[3].shNginTLoad((h+"../gui/room_south.bmp").c_str());
	skybox[4].shNginTLoad((h+"../gui/room_up.bmp").c_str());
	skybox[5].shNginTLoad((h+"../gui/room_down.bmp").c_str());
	for (int i = 0; i < 6; ++i)
		if (!skybox[i].shNginTIsLoaded())
			log("Sky box %d didn't load\n", i);
}

void initialize_Ngin()
{
	shNginInitialize(SH_RES_WIDTH, SH_RES_HEIGHT, 90);
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
	shNgin3dMove(-150, middleX, middleY, 1000);
}

#define GETMINMAX(x) \
	do { typeof(x) _x = EXPANDED(x);\
		if (maxes[0] < _x[0]) maxes[0] = _x[0];\
		if (maxes[1] < _x[1]) maxes[1] = _x[1];\
		if (maxes[2] < _x[2]) maxes[2] = _x[2];\
		if (mins[0] > _x[0]) mins[0] = _x[0];\
		if (mins[1] > _x[1]) mins[1] = _x[1];\
		if (mins[2] > _x[2]) mins[2] = _x[2];\
	} while (0)

module_mesh compute_boundary(vector<triangle> t)
{
	module_mesh mesh;
	mesh.triangles = t;
	float maxes[3] = {-LARGENUM, -LARGENUM, -LARGENUM},
	      mins[3]  = { LARGENUM,  LARGENUM,  LARGENUM};
	for (unsigned int i = 0, size = t.size(); i < size; ++i)
	{
		GETMINMAX(t[i].p1);
		GETMINMAX(t[i].p2);
		GETMINMAX(t[i].p3);
	}
	mesh.center[0] = (maxes[0] + mins[0]) / 2;
	mesh.center[1] = (maxes[1] + mins[1]) / 2;
	mesh.center[2] = (maxes[2] + mins[2]) / 2;
	mesh.radius = 0;
	for (unsigned int i = 0, size = t.size(); i < size; ++i)
	{
		float d = DISTANCE(mesh.center, t[i].p1);
		if (mesh.radius < d)
			mesh.radius = d;
		d = DISTANCE(mesh.center, t[i].p2);
		if (mesh.radius < d)
			mesh.radius = d;
		d = DISTANCE(mesh.center, t[i].p3);
		if (mesh.radius < d)
			mesh.radius = d;
	}
	return mesh;
}

#define CHECK_AND_GET(x) if (s == #x) x = v

int read_settings(const char *file)
{
	ifstream fin(file);
	if (!fin.is_open())
	{
		log("Could not open file '%s'\n", file);
		return -1;
	}
	string s;
	string c;
	float v;
	while (fin >> s >> c >> v)
	{
		CHECK_AND_GET(selection_threshold);
	}
	return 0;
}

static int _triangulate_module(skin_module *m, void *d)
{
	char percent[] = "   ";
	skin_module_size *modules_count = (skin_module_size *)d;

	modules_triangulated.push_back(compute_boundary(triangulate(m)));
	modules_triangulated.back().regions.clear();
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

void regionalize()
{
	int frame;
	bool use_defaults = false;
	fprintf(fout, "Initializing...\n");
	srand(time(NULL));
	char exelink[50];
	sprintf(exelink, "/proc/%d/exe", getpid());
	int bytes = readlink(exelink, home_path, sizeof(home_path) - 1);
	string home;
	if(bytes <= 0)
	{
		cout << "Failed to retrieve home directory.  Will use default settings" << endl;
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
	{
		read_triangulation_settings((home+"../settings/triangulation_settings").c_str());
		read_settings((home+"../settings/regionalizer_settings").c_str());
	}
	skin_module_size modules_count = skin_object_modules_count(&skin);
	cout << "Constructing module shapes...  0%" << flush;
	skin_object_for_each_module(&skin, _triangulate_module, &modules_count);
	cout << "\b\b\b\b done" << endl;
	min_max_data min_max;
	min_max.minX = 10000000.0f;
	min_max.maxX = -10000000.0f;
	min_max.minY = 10000000.0f;
	min_max.maxY = -10000000.0f;
	skin_object_for_each_sensor(&skin, _find_min_max, &min_max);
	if (min_max.maxX == min_max.minX)
	{
		fprintf(fout, "maxX is equal to minX\n");
		meter_scale = 1;
	}
	else if (min_max.maxX - min_max.minX > min_max.maxY - min_max.minY)
		meter_scale = 100.0f / (min_max.maxX - min_max.minX);
	else
		meter_scale = 100.0f / (min_max.maxY - min_max.minY);
	fprintf(fout, "Constructing module shapes done\n");
	initialize_SDL();
	if (_must_exit)
		return;
	fprintf(fout, "SDL done\n");
	initialize_Ngin();
	glPointSize(2);
	fprintf(fout, "Ngin done (version: "NGIN_VERSION")\n");
	initialize_Ngin3d((home+"../settings/Ngin3d_settings").c_str(),
			(min_max.maxX + min_max.minX) / 2 * meter_scale, (min_max.maxY + min_max.minY) / 2 * meter_scale);
	fprintf(fout, "Ngin3d done (version: "NGIN3D_VERSION")\n");
	if (shFontLoad((home+"../gui/font.shf").c_str()) == SH_FONT_FILE_NOT_FOUND)
		fprintf(fout, "Could not load font\n");
	else
	{
		shFontSize(0.5f);
		shFontColor(255, 0, 0);
		shFontShadowColor(0, 0, 0);
		shFontShadow(SH_FONT_FULL_SHADOW);
		fprintf(fout, "shFont done (version: "SH_FONT_VERSION")\n");
	        fprintf(fout, "Using shImage (version "SH_IMAGE_VERSION")\n");
	}
	load_data(home.c_str());
	fprintf(fout, "gui data loaded\n");
	if (rescale_responses)
	{
		skin_sensor_size sensors_count = skin_object_sensors_count(&skin);
		sensor_mins = new skin_sensor_response[sensors_count];
		if (sensor_mins == NULL)
		{
			fprintf(fout, "Could not allocate memory for sensor minimums.  Resclaing disabled\n");
			cout << "Out of memory.  Rescaling disabled" << endl;
			rescale_responses = false;
		}
		else
			for (skin_sensor_id i = 0; i < sensors_count; ++i)
				sensor_mins[i] = SKIN_SENSOR_RESPONSE_MAX;
	}
	frame = 0;
	unsigned int last = SDL_GetTicks();
	unsigned int last_time = last;
	while (true)
	{
		unsigned int now = SDL_GetTicks();
		if (now >= last+1000)
		{
			fprintf(fout, "%d fps\n", (FPS = frame * 1000 / (now - last)));
			last += 1000;
			frame = 0;
		}
		dt = now - last_time;
		last_time = now;
		process_events();
		if (_must_exit)
			break;
		add_active_modules();
		render_screen();
		++frame;
	}
}

void auto_regionalize()
{
	skin_module_size mcount = skin_object_modules_count(&skin);
	for (skin_module_id i = 0; i < mcount; ++i)
		cur_region.insert(i);
	regions.push_back(cur_region);
}

int main(int argc, char *argv[])
{
	const char *cachefile = "unnamed.cache";
	bool rebuild = false;
	bool ignore_options = false;
	bool fake_regionalize = false;

	fout = fopen("log.out", "w");

	for (int i = 1; i < argc; ++i)
		if (ignore_options)
			cachefile = argv[i];
		else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
		{
			cout << "Usage: " << argv[0] << " [options] [cachefile]\n\n"
				"Options:\n"
				"--help or -h\n"
				"\tShow this help message\n"
				"--reregionalize or -r\n"
				"\tForce regionalization even if cache exists\n"
				"--fake or -f\n"
				"\tGenerate fake regionalization and put all in one region\n"
				"--range X\n"
				"\tScale values from their minimum to the given range X\n"
				"--\n"
				"\tThreat the rest of the arguments as though they are not options.\n\n"
				"If cachefile is not given, the name would default to unnamed.cache\n"
				"If --reregionalize is not given, the program tries to regionalize by using data\n"
				"from cache file.  If no cache file exists or cache file produces error, it will\n"
				"proceed with regionalization.  If multiple cache file names are given, the last\n"
				"one is taken.\n\n"
				"If faking regionalization (for test purposes), the regionalization data would\n"
				"not be cached.\n\n"
				"Depending on the covering material over the sensors, the dynamic range of\n"
				"values may change.  If this option is given, the minimum value of sensors are\n"
				"stored and the sensor values are scaled from that starting point and up to X\n"
				"above it to [0, MAX_RESPONSE]\n\n"
				"If you want to name your file one that clashes with the regionalizer options,\n"
				"use -- to stop threating the rest of the arguments as options.  For example:\n\n"
				"\t" << argv[0] << " --reregionalize -- --help\n\n"
				"will use --help as the name of the cache file" << endl << endl;
			return 0;
		}
		else if (strcmp(argv[i], "--reregionalize") == 0 || strcmp(argv[i], "-r") == 0)
			rebuild = true;
		else if (strcmp(argv[i], "--range") == 0)
		{
			++i;
			rescale_responses = true;
			if (i < argc)
				sscanf(argv[i], "%u", &response_range);
			else
				cout << "Missing value after --range.  Defaulting to 1000" << endl;
		}
		else if (strcmp(argv[i], "--fake") == 0 || strcmp(argv[i], "-f") == 0)
			fake_regionalize = true;
		else if (strcmp(argv[i], "--") == 0)
			ignore_options = true;
		else
			cachefile = argv[i];
	skin_object_init(&skin);
	if (!rebuild && !fake_regionalize)
	{
		int ret;
		if ((ret = skin_object_regionalization_reload(&skin, cachefile)) != SKIN_SUCCESS)
		{
			if (ret == SKIN_NO_FILE)
				log("Regionalization cache does not exist.  Proceeding with regionalization...\n");
			else
				log("Skin regionalization cache reload error: %s\n", skin_object_last_error(&skin));
			rebuild = true;
		}
		else
			_error_code = EXIT_SUCCESS;
	}
	if (rebuild || fake_regionalize)
	{
		if (skin_object_regionalization_begin(&skin) != SKIN_SUCCESS)
		{
			log("Skin regionalization initialization error: %s\n", skin_object_last_error(&skin));
			goto exit_fail;
		}
		if (!fake_regionalize)
			regionalize();
		else
			auto_regionalize();
		if (!cancel_regionalization)
		{
			skin_regionalization_data *rdata = new skin_regionalization_data[regions.size()];
			if (rdata == NULL)
			{
				log("Skin regionalization error: Out of memory\n");
				goto exit_fail;
			}
			for (skin_region_id i = 0; i < regions.size(); ++i)
			{
				rdata[i].modules = NULL;
				rdata[i].size = 0;
				if (regions[i].size() == 0)
					continue;
				rdata[i].modules = new skin_module_id[regions[i].size()];
				if (rdata[i].modules == NULL)
				{
					log("Skin regionalization error: Out of memory\n");
					goto exit_fail;
				}
				for (set<skin_module_id>::iterator j = regions[i].begin(), end = regions[i].end(); j != end; ++j)
					rdata[i].modules[rdata[i].size++] = *j;
			}
			if (skin_object_regionalization_end(&skin, rdata, regions.size(), fake_regionalize?NULL:cachefile) != SKIN_SUCCESS)
			{
				log("Skin regionalization error: %s\n", skin_object_last_error(&skin));
				goto exit_fail;
			}
		}
		else
			goto exit_fail;
	}
exit_fail:
	skin_object_free(&skin);
	if (fout)
		fclose(fout);
	return _error_code;
}
