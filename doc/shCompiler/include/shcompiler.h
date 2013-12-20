/*
 * Copyright (C) 2007-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of shCompiler.
 *
 * shCompiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * shCompiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with shCompiler.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SHCOMPILER_H_BY_CYCLOPS
#define SHCOMPILER_H_BY_CYCLOPS

#define SH_COMPILER_STRINGIFY_(x) #x
#define SH_COMPILER_STRINGIFY(x) SH_COMPILER_STRINGIFY_(x)

#define SH_COMPILER_VERSION_MAJOR 1
#define SH_COMPILER_VERSION_MINOR 3
#define SH_COMPILER_VERSION_REVISION 0
#define SH_COMPILER_VERSION_BUILD 325
#define SH_COMPILER_VERSION SH_COMPILER_STRINGIFY(SH_COMPILER_VERSION_MAJOR)"."\
		SH_COMPILER_STRINGIFY(SH_COMPILER_VERSION_MINOR)"."\
		SH_COMPILER_STRINGIFY(SH_COMPILER_VERSION_REVISION)"."\
		SH_COMPILER_STRINGIFY(SH_COMPILER_VERSION_BUILD)

#include <nfa2dfa.h>
#include <parser.h>
// The rest are included from parser.h

#endif
