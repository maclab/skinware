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

#ifndef SCALER_H
#define SCALER_H

#include <vector>
#include <skin.h>

class scaler
{
public:
	scaler();
	scaler(struct skin *);				// If scaler is to be used in real-time threads, use this as constructor
							// or call scale(skin) once to preallocate memory
	scaler(unsigned int r);
	scaler(struct skin *, unsigned int r);
	~scaler();
	void affect(struct skin *);
	void set_range(unsigned int r);
	void scale(const skin_sensor_response *, unsigned int count, std::vector<uint8_t> &result);
	void scale(const std::vector<skin_sensor_response> &, std::vector<uint8_t> &result);
	void scale(struct skin *, std::vector<uint8_t> &result);
	void dampen(unsigned int amount);
	void reset();
	// Convenience functions - Not to be used in real-time context
	std::vector<uint8_t> scale(const skin_sensor_response *, unsigned int count);
	std::vector<uint8_t> scale(const std::vector<skin_sensor_response> &);
	std::vector<uint8_t> scale(struct skin *);		// store responses from the skin

	/* internal */
	unsigned int min_range;
	std::vector<skin_sensor_response> mins, maxes;
	std::vector<bool> do_scale;			/* which responses need scaling */
	void initialize(unsigned int count);
};

#endif
