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

#include <cmath>
#include "amplifier.h"

using namespace std;

static uint8_t _amplify_default(uint8_t x) { return x; };

amplifier::amplifier()
{
	make_custom(NULL);
}

amplifier::amplifier(double aa, double bb)
{
	make_linear(aa, bb);
}

amplifier::amplifier(double aa, double bb, double cc)
{
	make_quadatic(aa, bb, cc);
}

amplifier::amplifier(double aa, double bb, double cc, double dd)
{
	make_root(aa, bb, cc, dd);
}

amplifier::amplifier(amplifier_callback cbcb)
{
	make_custom(cbcb);
}

amplifier::~amplifier()
{
}

void amplifier::make_linear(double aa, double bb)
{
	type = AMPLIFIER_LINEAR;
	a = aa;
	b = bb;
}

void amplifier::make_quadatic(double aa, double bb, double cc)
{
	type = AMPLIFIER_QUADRATIC;
	a = aa;
	b = bb;
	c = cc;
}

void amplifier::make_root(double aa, double bb, double cc, double dd)
{
	type = AMPLIFIER_ROOT;
	a = aa;
	b = bb;
	c = cc;
	d = dd;
}

void amplifier::make_custom(amplifier_callback cbcb)
{
	type = AMPLIFIER_CUSTOM;
	if (cbcb)
		cb = cbcb;
	else
		cb = _amplify_default;
}

uint8_t amplifier::amplify(uint8_t response)
{
	double res;
	uint8_t amplified;
	switch (type)
	{
		default:
		case AMPLIFIER_LINEAR:
			res = a * response + b;
			break;
		case AMPLIFIER_QUADRATIC:
			res = (response + b) * (response + b) / a + c;
			break;
		case AMPLIFIER_ROOT:
			if (response + a < 0)
				res = -pow(-(response + a), 1.0 / b);
			else
				res = pow((response + a), 1.0 / b);
			res = c * res + d;
			break;
		case AMPLIFIER_CUSTOM:
			res = (*cb)(response);
			break;
	}
	res = round(res);
	if (res < 0)
		amplified = 0;
	else if (res > 255)
		amplified = 255;
	else
		amplified = (uint8_t)res;
	return amplified;
}

void amplifier::amplify(const vector<uint8_t> &responses, vector<uint8_t> &result)
{
	unsigned int size = responses.size();
	if (result.size() < size)
		size = result.size();
	for (unsigned int i = 0; i < size; ++i)
		result[i] = amplify(responses[i]);
}

vector<uint8_t> amplifier::amplify(const vector<uint8_t> &responses)
{
	vector<uint8_t> res(responses.size());
	amplify(responses, res);
	return res;
}
