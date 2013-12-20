/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of Ngin3d.
 *
 * Ngin3d is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ngin3d is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ngin3d.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NGIN3D_INTERNAL_H_BY_CYCLOPS
#define NGIN3D_INTERNAL_H_BY_CYCLOPS

/*
 * This file is internal to Ngin3d. It is used to pass around definitions shared between files, but are unnecessary for the user to know.
 * There is no need to include this file in the release of the library.
 */

struct shNgin3dPlayerState
{
	int crouched;
	int in_air;
	void reset()
	{
		crouched = false;
		in_air = false;
	}
};

extern shNgin3dPlayerState		g_Ngin3d_state;

#define					STATE_CROUCHED			g_Ngin3d_state.crouched
#define					STATE_IN_AIR			g_Ngin3d_state.in_air

#endif
