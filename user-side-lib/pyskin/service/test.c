/*
 * Copyright (C) 2013  Shahbaz Youssefi <ShabbyX@gmail.com>
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
#include <skin.h>
#include "example_pyservice.h"

int main(void)
{
	skin_object skin;
	skin_service_manager *sm;
	skin_service service;
	unsigned int i;

	skin_object_init(&skin);
	if (skin_object_load(&skin) != SKIN_SUCCESS)
	{
		printf("Failed to load skin\n");
		goto exit_no_load;
	}
	sm = skin_object_service_manager(&skin);

	if (skin_service_manager_connect_to_sporadic_service(sm, EXAMPLE_PYSERVICE_MEMNAME,
			EXAMPLE_PYSERVICE_REQUEST, EXAMPLE_PYSERVICE_RESPONSE,
			&service) != SKIN_SUCCESS)
	{
		printf("Failed to connect to service\n");
		goto exit_no_connect;
	}

	if (skin_service_request(&service, NULL) != SKIN_SUCCESS)
		goto exit_no_lock;
	for (i = 0; i < SKIN_SERVICE_RESULT_COUNT(service.memory); ++i)
		printf("%u: %d %d\n", i,
			((struct example_pyservice_data *)service.memory)[i].x,
			((struct example_pyservice_data *)service.memory)[i].y);

exit_no_lock:
	skin_service_disconnect(&service);
exit_no_connect:
exit_no_load:
	skin_object_free(&skin);
	return 0;
}
