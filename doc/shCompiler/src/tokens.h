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

#ifndef TOKENS_H_BY_CYCLOPS
#define TOKENS_H_BY_CYCLOPS

// These functions should not be visible by user. They are not static because they are implemented and used in different files, therefore
// I have prefixed them with something strange so they wouldn't accidentally be used by a user
void shcompiler_TF_init(bool rebuild, istream &letters, const char *cachePath);
void shcompiler_TF_cleanup();
list<shDFA> shcompiler_TF_getDFAs();
int shcompiler_TF_error();

#endif
