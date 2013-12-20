/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <skin.h>
#include "skin_test_service.h"

FILE *fout;

/* skin */
static skin_object			skin;
static skin_reader			*sr;
static skin_service_manager		*sm;

/* other */
static bool				must_exit		= false;
static int				error_code		= EXIT_SUCCESS;

void sighandler(int signum)
{
	must_exit = true;
	error_code = EXIT_SUCCESS;
}

void fatal_error(const char *msg, int error)
{
	must_exit = true;
	printf("%s.  Error: %d\n", msg, error);
	error_code = error;
}

#define SIZE 10

void fill_responses(void *mem, void *data)
{
	skin_test_service *array = mem;
	bool *first_time = data;
	int i;

	if (*first_time)
		memset(array, 0, sizeof(*array) * SIZE);
	else
		for (i = 0; i < SIZE; ++i)
			array[i].x += i;
	*first_time = false;
}

int main(int argc, char **argv)
{
	struct sigaction sa;
	unsigned int service_id;
	bool first_time = true;
	int ret;

	fout = fopen("log.out", "w");
	if (fout == NULL)
	{
		printf("Could not open log file\n");
		return EXIT_FAILURE;
	}

	fprintf(fout, "Initializing...\n");
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sighandler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);

	skin_object_init(&skin);
	if (skin_object_load(&skin) != SKIN_SUCCESS)
	{
		fprintf(fout, "Could not initialize skin\n");
		skin_object_free(&skin);
		return EXIT_FAILURE;
	}

	sr = skin_object_reader(&skin);
	sm = skin_object_service_manager(&skin);
	skin_reader_start(sr, SKIN_ALL_SENSOR_TYPES, SKIN_ACQUISITION_SPORADIC, 0);
	fprintf(fout, "skin done (version: %s)\n", SKIN_VERSION);
	ret = skin_service_manager_initialize_sporadic_service(sm, SKIN_TEST_SERVICE_SHMEM, sizeof(skin_test_service), SIZE,
			SKIN_TEST_SERVICE_REQUEST, SKIN_TEST_SERVICE_RESPONSE);
	if (ret < 0)
		fatal_error("Could not initialize service", ret);
	else
	{
		service_id = ret;
		if ((ret = skin_service_manager_start_service(sm, service_id, fill_responses, &first_time)) != SKIN_SUCCESS)
			fatal_error("Could not start service", ret);
	}

	while (!must_exit)
		usleep(100000);

	skin_service_manager_stop_service(sm, service_id);
	skin_object_free(&skin);
	return error_code;
}
