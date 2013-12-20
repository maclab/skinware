/*
 * Copyright (C) 2011-2012 Shahbaz Youssefi <ShabbyX@gmail.com>
 *
 * This file is part of Ngin.
 *
 * Ngin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Ngin is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ngin.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NGIN_PRIMITIVES_H_BY_CYCLOPS
#define NGIN_PRIMITIVES_H_BY_CYCLOPS

#define				NGIN_WIREFRAME				false
#define				NGIN_SOLID				true

// If texture was enabled, primitives bind the texture

// Binds texture around the sphere with z axis as the main axis. Y direction of texture from 0 to 1 with z going from -radius to radius.
// X direction of texture from 0 to 1 from (radius, 0) back to itself in counter clockwise direction.
void shNginSphere(double radius, int slices, int stacks, bool mode = NGIN_SOLID);

#endif
