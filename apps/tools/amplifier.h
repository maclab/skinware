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

#ifndef AMPLIFIER_H
#define AMPLIFIER_H

#include <vector>
#include <skin.h>

enum amplifier_type
{
	AMPLIFIER_LINEAR,		// apply a linear function (ax+b) and cap the result
	AMPLIFIER_QUADRATIC,		// apply a quadratic function ((1/a)(x+b)^2+c)
	AMPLIFIER_ROOT,			// apply a root function: c*root(x+a,b)+d
	AMPLIFIER_CUSTOM		// apply a custom callback
};

typedef uint8_t (*amplifier_callback)(uint8_t);

class amplifier
{
private:
	amplifier_type type;
	double a, b, c, d;
	amplifier_callback cb;
public:
	amplifier();
	amplifier(double, double);
	amplifier(double, double, double);
	amplifier(double, double, double, double);
	amplifier(amplifier_callback);
	~amplifier();
	void make_linear(double, double);
	void make_quadatic(double, double, double);
	void make_root(double, double, double, double);
	void make_custom(amplifier_callback);
	void affect(struct skin *s);
	uint8_t amplify(unsigned int s, uint8_t);
	void amplify(const std::vector<uint8_t> &, std::vector<uint8_t> &result);
	// Convenience function - Not to be used in real-time context
	std::vector<uint8_t> amplify(const std::vector<uint8_t> &);

	/* internal */
	std::vector<bool> do_amplify;
};

#endif
