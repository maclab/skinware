/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#ifndef EXAMPLE_PYSERVICE_H
#define EXAMPLE_PYSERVICE_H

struct example_pyservice_data
{
	/* data to be used as array */
	int x;
	int y;
};

#define EXAMPLE_PYSERVICE_MEMNAME "ESVMEM"
#define EXAMPLE_PYSERVICE_REQUEST "ESVREQ"
#define EXAMPLE_PYSERVICE_RESPONSE "ESVRES"

#endif
