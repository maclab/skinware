/*
 * Copyright (C) 2011-2012  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#include <fstream>
#include <iostream>
#include <string>
#include <cmath>
#include <set>
#include <map>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <ctime>

using namespace std;

int sensor_layers = 1;
int sensors = 1;
int sensors_in_module = 1;
int modules_in_patch_mean = 2;
int modules_in_patch_var = 1;
int sensors_o_x = 1;
int sensors_o_y = 1;
int sensors_d_x = 1;
int sensors_d_y = 1;
int sensors_in_row = 1;
int sensors_layer_offset_mean = 0;
int sensors_layer_offset_var = 0;
int frequency_mean = 60;
int frequency_var = 40;

#define CHECK_AND_GET(x) if (s == #x) x = v

int read_settings()
{
	ifstream fin("settings");
	if (!fin.is_open())
	{
		cout << "Could not open file \"settings\"" << endl;
		return -1;
	}
	string s;
	char c;
	int v;
	while (fin >> s >> c >> v)
	{
		CHECK_AND_GET(sensor_layers);
		CHECK_AND_GET(sensors);
		CHECK_AND_GET(sensors_in_module);
		CHECK_AND_GET(modules_in_patch_mean);
		CHECK_AND_GET(modules_in_patch_var);
		CHECK_AND_GET(sensors_o_x);
		CHECK_AND_GET(sensors_o_y);
		CHECK_AND_GET(sensors_d_x);
		CHECK_AND_GET(sensors_d_y);
		CHECK_AND_GET(sensors_in_row);
		CHECK_AND_GET(sensors_layer_offset_mean);
		CHECK_AND_GET(sensors_layer_offset_var);
		CHECK_AND_GET(frequency_mean);
		CHECK_AND_GET(frequency_var);
	}
	return 0;
}

void generate_initializer_data()
{
	ofstream fout("data");
	if (!fout.is_open())
	{
		cout << "Could not open \"data\" file" << endl;
		return;
	}
	fout << sensor_layers << endl;
	vector<int> sensors_in_layers(sensor_layers);
	vector<vector<int> > modules_in_patches;
	vector<pair<int, pair<int, int> > > patches;
	for (int i = 0; i < sensor_layers; ++i)
	{
		fout << "Sensor_type" << (i<10?"00":i<100?"0":"") << i << " " << frequency_mean + rand() % (2 * frequency_var + 1) - frequency_var << " ";
		if (i == sensor_layers-1)
			sensors_in_layers[i] = sensors - ((sensor_layers - 1) * (sensors / sensor_layers));
		else
			sensors_in_layers[i] = sensors / sensor_layers;
		fout << sensors_in_layers[i] << " " << (int)(ceil(sensors_in_layers[i] / (double)sensors_in_module) + 0.1) << " ";
		modules_in_patches.push_back(vector<int>());
		for (int j = sensors_in_module; j <= sensors_in_layers[i]; j += sensors_in_module)
			modules_in_patches[i].push_back(sensors_in_module);
		if (sensors_in_layers[i] % sensors_in_module)
			modules_in_patches[i].push_back(sensors_in_layers[i] % sensors_in_module);
		int patches_in_layer = 0;
		for (unsigned int current = 0; current < modules_in_patches[i].size(); )
		{
			unsigned int next = modules_in_patch_mean + rand() % (modules_in_patch_var * 2 + 1) - modules_in_patch_var;
			if (current + next > modules_in_patches[i].size())
				next = modules_in_patches[i].size() - current;
			patches.push_back(pair<int, pair<int, int> >(i, pair<int, int>(current, current + next)));
			current += next;
			++patches_in_layer;
		}
		fout << patches_in_layer << endl;
	}
	random_shuffle(patches.begin(), patches.end());
	for (unsigned int i = 0; i < patches.size(); ++i)
	{
		fout << patches[i].second.second - patches[i].second.first << " " << patches[i].first;
		for (int j = patches[i].second.first; j < patches[i].second.second; ++j)
			fout << " " << modules_in_patches[patches[i].first][j];
		fout << endl;
	}
}

void generate_sensors()
{
	ofstream fout("skin_sensors");
	if (!fout.is_open())
	{
		cout << "Could not open \"skin_sensors\" file" << endl;
		return;
	}
	int sid = 0;
	fout << sensors << " " << sensors << endl;
	for (int i = 0; i < sensor_layers; ++i)
	{
		int sensors_in_layer;
		if (i == sensor_layers-1)
			sensors_in_layer = sensors - ((sensor_layers - 1) * (sensors / sensor_layers));
		else
			sensors_in_layer = sensors / sensor_layers;
		float offset_x = sensors_layer_offset_mean + rand() % (2 * sensors_layer_offset_var + 1) - sensors_layer_offset_var;
		float offset_y = sensors_layer_offset_mean + rand() % (2 * sensors_layer_offset_var + 1) - sensors_layer_offset_var;
		offset_x /= 1000000.0f;
		offset_y /= 1000000.0f;
		for (int j = 0; j < sensors_in_layer; ++j, ++sid)
			fout << (sensors_o_x + (j % sensors_in_row) * sensors_d_x) * 0.001f + offset_x << " "
			     << (sensors_o_y + j / sensors_in_row * sensors_d_y) * 0.001f + offset_y << " "
			     << (float)i / sensor_layers / 50 << " 0 0 1 0 0 " << sensors_d_x * 0.001f << " 0 1 " << sid << endl;
	}
}

int main()
{
	srand(time('\0'));
	if (read_settings())
		return EXIT_FAILURE;
	generate_initializer_data();
	generate_sensors();
	return EXIT_SUCCESS;
}
