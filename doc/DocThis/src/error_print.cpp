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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "error_print.h"

string _currentFile;

void currentFileName(const char *file)
{
	_currentFile = file;
}

void currentFileName(const string &file)
{
	_currentFile = file;
}

void printMessage(int line, int column, const char *fmt, ...)
{
	printf("%s:%d:%d: ", _currentFile.c_str(), line, column);
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
}

void printMessage(int line, int column, string msg)
{
	printMessage(line, column, "%s", msg.c_str());
}

void printMessage(int line, const char *fmt, ...)
{
	printf("%s:%d: ", _currentFile.c_str(), line);
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
}

void printMessage(int line, string msg)
{
	printMessage(line, "%s", msg.c_str());
}

void printMessage(const char *fmt, ...)
{
	printf("%s: ", _currentFile.c_str());
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");
}

void printMessage(string msg)
{
	printMessage("%s", msg.c_str());
}

void sprintMessage(char *dest, int size, int line, int column, const char *fmt, ...)
{
	int n;
	snprintf(dest, size-1, "%s:%d:%d: %n", _currentFile.c_str(), line, column, &n);
	va_list args;
	va_start(args, fmt);
	vsnprintf(dest+n, size-n-1, fmt, args);
	va_end(args);
	int len = strlen(dest);
	dest[len] = '\n';
	dest[len+1] = '\0';
}

void sprintMessage(char *dest, int size, int line, int column, string msg)
{
	sprintMessage(dest, size, line, column, "%s", msg.c_str());
}

void sprintMessage(char *dest, int size, int line, const char *fmt, ...)
{
	int n;
	snprintf(dest, size-1, "%s:%d: %n", _currentFile.c_str(), line, &n);
	va_list args;
	va_start(args, fmt);
	vsnprintf(dest+n, size-n-1, fmt, args);
	va_end(args);
	int len = strlen(dest);
	dest[len] = '\n';
	dest[len+1] = '\0';
}

void sprintMessage(char *dest, int size, int line, string msg)
{
	sprintMessage(dest, size, line, "%s", msg.c_str());
}

void sprintMessage(char *dest, int size, const char *fmt, ...)
{
	int n;
	snprintf(dest, size-1, "%s: %n", _currentFile.c_str(), &n);
	va_list args;
	va_start(args, fmt);
	vsnprintf(dest+n, size-n-1, fmt, args);
	va_end(args);
	int len = strlen(dest);
	dest[len] = '\n';
	dest[len+1] = '\0';
}

void sprintMessage(char *dest, int size, string msg)
{
	sprintMessage(dest, size, "%s", msg.c_str());
}

void pprintMessage(shParser &parser, shLexer &lexer, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
        unsigned int length = vsnprintf('\0', 0, fmt, args);
	char temp[30];
	sprintf(temp, ":%d:%d: ", lexer.shLLine(), lexer.shLColumn());
	char *temp2 = new char[length+1];
	if (temp2 == '\0')
	{
		string error = lexer.shLFileName()+temp+"<INSUFFICIENT MEMORY> "+fmt+'\n';
		parser.shPAppendError(error);
		return;
	}
	vsnprintf(temp2, length+1, fmt, args);
	va_end(args);
	string error = lexer.shLFileName()+temp+temp2+'\n';
	delete[] temp2;
	parser.shPAppendError(error);
}

void pprintMessage(shParser &parser, shLexer &lexer, const string msg)
{
	char temp[30];
	sprintf(temp, ":%d:%d: ", lexer.shLLine(), lexer.shLColumn());
	string error = lexer.shLFileName()+temp+msg+'\n';
	parser.shPAppendError(error);
}
