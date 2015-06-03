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

#ifndef NAMES_H
#define NAMES_H

/* utilities for generating names */

/* check whether two prefixes are equal */
int skin_internal_name_cmp_prefix(const char *prefix1, const char *prefix2);
/* copy prefix over another */
void skin_internal_name_cpy_prefix(char *dest, const char *src);
/*
 * fill name by prefix.  If NULL, fill by default_prefix.  At maximum URT_NAME_LEN - 3 chars
 * are written plus a '\0'.  A pointer to the suffix part is returned, which can be used in
 * set_suffix to complete the name.
 */
char *skin_internal_name_set_prefix(char *name, const char *prefix, const char *default_prefix);
/*
 * fill the rest of the name (returned from name_set_prefix) with a suffix.
 * suffix cannot be NULL.
 */
void skin_internal_name_set_suffix(char *name, const char *suffix);
/* build a name out of prefix and suffix */
void skin_internal_name_set(char *name, const char *prefix, const char *suffix);
/* build a name out of prefix and suffix and a one digit index */
void skin_internal_name_set_indexed(char *name, const char *prefix, const char *suffix, unsigned int index);

#endif
