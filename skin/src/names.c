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

#include <urt.h>
#include "config.h"
#include "names.h"

int skin_internal_name_cmp_prefix(const char *prefix1, const char *prefix2)
{
	return strncmp(prefix1, prefix2, URT_NAME_LEN - 3);
}

void skin_internal_name_cpy_prefix(char *dest, const char *src)
{
	strncpy(dest, src, URT_NAME_LEN - 3);
	dest[URT_NAME_LEN - 3] = '\0';
}

char *skin_internal_name_set_prefix(char *name, const char *prefix, const char *default_prefix)
{
	if (prefix == NULL)
		prefix = default_prefix;

	strncpy(name, prefix, URT_NAME_LEN - 3);
	name[URT_NAME_LEN - 3] = '\0';

	return name + strlen(name);
}

void skin_internal_name_set_suffix(char *name, const char *suffix)
{
	strncpy(name, suffix, 3);
}

void skin_internal_name_set(char *name, const char *prefix, const char *suffix)
{
	char *rest = skin_internal_name_set_prefix(name, prefix, NULL);
	skin_internal_name_set_suffix(rest, suffix);
}

void skin_internal_name_set_indexed(char *name, const char *prefix, const char *suffix, unsigned int index)
{
	char digit;
	size_t len;

	skin_internal_name_set(name, prefix, suffix);
	if (index < 10)
		digit = '0' + index;
	else if (index < 36)
		digit = 'A' + index - 10;
	else
		digit = '_';

	/* append digit to end of name, or replace its last character if name is already full */
	len = strlen(name);
	if (len >= URT_NAME_LEN)
		len = URT_NAME_LEN - 1;
	name[len] = digit;
	name[len + 1] = '\0';
}
