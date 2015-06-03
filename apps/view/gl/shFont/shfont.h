/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shFont.
 *
 * shFont is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shFont is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shFont.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SH_FONT_H_BY_CYCLOPS
#define SH_FONT_H_BY_CYCLOPS

#define SH_FONT_STRINGIFY_(x) #x
#define SH_FONT_STRINGIFY(x) SH_FONT_STRINGIFY_(x)

#define SH_FONT_VERSION_MAJOR 0
#define SH_FONT_VERSION_MINOR 2
#define SH_FONT_VERSION_REVISION 1
#define SH_FONT_VERSION_BUILD 171
#define SH_FONT_VERSION SH_FONT_STRINGIFY(SH_FONT_VERSION_MAJOR)"."\
		SH_FONT_STRINGIFY(SH_FONT_VERSION_MINOR)"."\
		SH_FONT_STRINGIFY(SH_FONT_VERSION_REVISION)"."\
		SH_FONT_STRINGIFY(SH_FONT_VERSION_BUILD)

#include <stdio.h>
#include "shfont_base.h"

#endif
