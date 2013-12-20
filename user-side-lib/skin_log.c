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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include "skin_log.h"
#include "skin_object.h"

static FILE *_log_file = NULL;
static bool _no_file_line_next = false;

int skin_log_init(void)
{
	unsigned int i;
	char tmp[20];

	_log_file = fopen("skin.log", "w");

	for (i = 0; i < 100 && _log_file == NULL; ++i)
	{
		sprintf(tmp, "skin-%02u.log", i);
		_log_file = fopen(tmp, "w");
	}

	return _log_file == NULL?-1:0;
}

void skin_log_c(const char *file, unsigned int line, const char *fmt, ...)
{
	va_list args;

	if (_log_file != NULL)
	{
		if (file != NULL && !_no_file_line_next)
			fprintf(_log_file, "%s:%u: ", file, line);
		va_start(args, fmt);
		vfprintf(_log_file, fmt, args);
		va_end(args);
		fprintf(_log_file, "\n");
	}
	_no_file_line_next = false;
}

void skin_log_no_newline_c(const char *file, unsigned int line, const char *fmt, ...)
{
	va_list args;

	if (_log_file != NULL)
	{
		if (file != NULL && !_no_file_line_next)
			fprintf(_log_file, "%s:%u: ", file, line);
		va_start(args, fmt);
		vfprintf(_log_file, fmt, args);
		va_end(args);
		fflush(_log_file);
	}
	_no_file_line_next = true;
}

void skin_log_set_error_c(skin_object *so, const char *file, unsigned int line, const char *fmt, ...)
{
	va_list args;
#ifndef SKIN_DS_ONLY
	size_t len;
#endif

	if (_log_file != NULL)
	{
		if (file != NULL && !_no_file_line_next)
			fprintf(_log_file, "%s:%u: ", file, line);
		va_start(args, fmt);
		vfprintf(_log_file, fmt, args);
		va_end(args);
		fprintf(_log_file, "\n");
	}
	_no_file_line_next = false;

#ifndef SKIN_DS_ONLY
	if (so == NULL)
	{
		fprintf(_log_file, "Internal error: skin_log_set_error given NULL parameter\n");
		return;
	}

	va_start(args, fmt);
	len = vsnprintf(NULL, 0, fmt, args);
	va_end(args);

	if (len == 0)
		return;

	/* account for ending '\0' and check if realloc is needed */
	++len;
	if (len > so->p_error_message_mem_size)
	{
		char *enlarged;
		size_t new_size = len;

		/* assign a sane minimum size to avoid many reallocs */
		if (new_size < 256)
			new_size = 256;
		enlarged = realloc(so->p_error_message, new_size * sizeof(*enlarged));
		if (enlarged == NULL)
			return;

		so->p_error_message = enlarged;
		so->p_error_message_mem_size = new_size;
	}

	va_start(args, fmt);
	len = vsnprintf(so->p_error_message, len + 1, fmt, args);
	va_end(args);
#endif
}

void skin_log_flush(void)
{
	if (_log_file)
		fflush(_log_file);
}

void skin_log_free(void)
{
	if (_log_file != NULL)
		fclose(_log_file);
	_log_file = NULL;
}
