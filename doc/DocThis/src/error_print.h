/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of DocThis!.
 *
 * DocThis! is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DocThis! is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DocThis!.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ERROR_PRINT_H_BY_CYCLOPS
#define ERROR_PRINT_H_BY_CYCLOPS

#include <string>
#include <shcompiler.h>

#ifdef __GNUC__
#define PRINTF_STYLE(format_index, args_index) __attribute__ ((format (printf, format_index, args_index)))
#else
#define PRINTF_STYLE(format_index, args_index)
#endif

using namespace std;

void currentFileName(const char *file);
void currentFileName(const string &file);
// call the printMessage function line per line of error
void printMessage(int line, int column, const char *fmt, ...) PRINTF_STYLE(3, 4);
void printMessage(int line, int column, const string msg);
void printMessage(int line, const char *fmt, ...) PRINTF_STYLE(2, 3);
void printMessage(int line, const string msg);
void printMessage(const char *fmt, ...) PRINTF_STYLE(1, 2);
void printMessage(const string msg);
void sprintMessage(char *dest, int size, int line, int column, const char *fmt, ...) PRINTF_STYLE(5, 6);
void sprintMessage(char *dest, int size, int line, int column, const string msg);
void sprintMessage(char *dest, int size, int line, const char *fmt, ...) PRINTF_STYLE(4, 5);
void sprintMessage(char *dest, int size, int line, const string msg);
void sprintMessage(char *dest, int size, const char *fmt, ...) PRINTF_STYLE(3, 4);
void sprintMessage(char *dest, int size, const string msg);
void pprintMessage(shParser &parser, shLexer &lexer, const char *fmt, ...) PRINTF_STYLE(3, 4);
void pprintMessage(shParser &parser, shLexer &lexer, const string msg);

#endif
