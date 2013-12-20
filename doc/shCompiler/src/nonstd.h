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

#ifndef NONSTD_H_BY_CYCLOPS
#define NONSTD_H_BY_CYCLOPS

#include <string>
#include <cstdlib>
#include <dirent.h>
#ifdef _WIN32
  #include <direct.h>
  #include <limits.h>
  #define getCurrentDir _getcwd
  #define makeDir(dir) _mkdir(FIX_PATH(dir).c_str())
#else
  #include <unistd.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #define getCurrentDir getcwd
  #define makeDir(dir) mkdir(FIX_PATH(dir).c_str(), 0777)		// permission would be the maximum given to running process
#endif

using namespace std;

#ifdef _WIN32

static string FIX_PATH(const string &s)
{
	string res;
	for (const char *p = s.c_str(); *p; ++p)
		if (*p == '/')
			res += '\\';
		else
			res += *p;
	return res;
}

static string FIX_PATH(const char *s)
{
	string res;
	for (const char *p = s; *p; ++p)
		if (*p == '/')
			res += '\\';
		else
			res += *p;
	return res;
}

#else /* _WIN32 */

#define FIX_PATH(x) string(x)

#endif /* _WIN32 */

#endif
