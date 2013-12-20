/*
 * Copyright (C) 2011-2012  Shahbaz Youssefi <ShabbyX@gmail.com>
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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "../init.h"

int main(int argc, char **argv)
{
	char element[1001] = {0};		// If any string was bigger than 1000, truncate it
	FILE *fin, *fout;
	if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
	{
		printf("Usage: %s data_file\n\n", argv[0]);
		return EXIT_FAILURE;
	}
	fin  = fopen(argv[1], "r");
	if (!fin)
	{
		printf("Could not open data file \"%s\"\n", argv[1]);
		return EXIT_FAILURE;
	}
	while (!(fout = fopen("/proc/"PROCFS_ENTRY, "w")))
		usleep(100000);						// wait until the module creates the procfs file
	while (fscanf(fin, "%1000s", element) == 1)
	{
		fprintf(fout, "%s ", element);
		ungetc('x', fin);
		fscanf(fin, "%*s");	// truncate any name bigger than 1000 characters
	}
	fclose(fin);
	fclose(fout);
	return EXIT_SUCCESS;
}
