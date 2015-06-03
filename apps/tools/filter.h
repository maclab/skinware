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

#ifndef FILTER_H_BY
#define FILTER_H_BY

#include <vector>
#include <skin.h>

enum filter_type
{
	FILTER_AVERAGE
};

class filter
{
private:
	filter_type type;
	std::vector<std::vector<uint8_t> > responses;
	unsigned int current;
	unsigned int size;	// size of `responses`
public:
	filter();
	filter(filter_type t);
	filter(unsigned int s);
	filter(filter_type t, unsigned int s);
	~filter();
	void change_type(filter_type t);
	void change_size(unsigned int s);
	void new_responses(const std::vector<uint8_t> &);	// Call new_responses once before getting in real-time functions
	uint8_t get_response(skin_sensor_id id) const;		// perform filter and return response
	void get_responses(std::vector<uint8_t> &result) const;	// perform filter and put responses in result
	// Convenience function - Not to be used in real-time context
	void new_responses(const uint8_t *, unsigned int count);
	void new_responses(struct skin *);			// store raw values from the skin
	std::vector<uint8_t> get_responses() const;		// perform filter and return responses for all sensors
};

#endif
