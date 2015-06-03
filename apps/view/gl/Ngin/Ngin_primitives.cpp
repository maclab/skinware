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

#include <math.h>
#include "Ngin_opengl.h"
#include "Ngin_primitives.h"

#define				PI					3.14159265358979323f
#define				PI_BY_2					6.28318530717958647f
extern bool			g_Ngin_textureEnabled;

void shNginSphere(double radius, int slices, int stacks, bool mode)
{
//	if (radius < stacks)
//		stacks = (int)radius;
//	if (2*PI*radius < slices)
//		slices = (int)(2*PI*radius);
	double angleAroundZ = 2*PI/(double)slices;
	double angleAlongZ = PI/(double)stacks;
	double currentZ, previousZ = radius;
	double currentRadius;
	double previousX, previousY, currentX, currentY;
	double previousUpperX, previousUpperY, currentUpperX, currentUpperY;
	double previousRadius;
	currentZ = radius*cos(angleAlongZ);
	currentRadius = radius*sin(angleAlongZ);
	previousX = currentRadius;
	previousY = 0;
	int i;
	if (g_Ngin_textureEnabled)
	{
		// First draw the upper and lower triangles
		for (i = 1; i < slices; ++i)
		{
			currentX = currentRadius*cos(i*angleAroundZ);
			currentY = currentRadius*sin(i*angleAroundZ);
			if (mode == NGIN_SOLID)
				glBegin(GL_TRIANGLES);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(0, 0, 1);
				glTexCoord2f(i/(double)slices, 1);
				glVertex3f(0, 0, radius);
				glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
				glTexCoord2f((i-1)/(double)slices, 1-1/(double)stacks);
				glVertex3f(previousX, previousY, currentZ);
				glNormal3f(currentX/radius, currentY/radius, currentZ/radius);
				glTexCoord2f(i/(double)slices, 1-1/(double)stacks);
				glVertex3f(currentX, currentY, currentZ);
			if (mode == NGIN_WIREFRAME)
			{
				glEnd();
				glBegin(GL_LINE_LOOP);
			}
				glNormal3f(0, 0, -1);
				glTexCoord2f(i/(double)slices, 0);
				glVertex3f(0, 0, -radius);
				glNormal3f(currentX/radius, currentY/radius, -currentZ/radius);
				glTexCoord2f(i/(double)slices, 1/(double)stacks);
				glVertex3f(currentX, currentY, -currentZ);
				glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
				glTexCoord2f((i-1)/(double)slices, 1/(double)stacks);
				glVertex3f(previousX, previousY, -currentZ);
			glEnd();
			previousX = currentX;
			previousY = currentY;
		}
		if (mode == NGIN_SOLID)
			glBegin(GL_TRIANGLES);
		else
			glBegin(GL_LINE_LOOP);
			glNormal3f(0, 0, 1);
			glTexCoord2f(1, 1);
			glVertex3f(0, 0, radius);
			glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
			glTexCoord2f((slices-1)/(double)slices, 1-1/(double)stacks);
			glVertex3f(previousX, previousY, currentZ);
			glNormal3f(currentRadius/radius, 0, currentZ/radius);
			glTexCoord2f(1, 1-1/(double)stacks);
			glVertex3f(currentRadius, 0, currentZ);
		if (mode == NGIN_WIREFRAME)
		{
			glEnd();
			glBegin(GL_LINE_LOOP);
		}
			glNormal3f(0, 0, -1);
			glTexCoord2f(1, 0);
			glVertex3f(0, 0, -radius);
			glNormal3f(currentRadius/radius, 0, -currentZ/radius);
			glTexCoord2f((slices-1)/(double)slices, 1/(double)stacks);
			glVertex3f(currentRadius, 0, -currentZ);
			glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
			glTexCoord2f(1, 1/(double)stacks);
			glVertex3f(previousX, previousY, -currentZ);
		glEnd();
		// Then draw the middle quadangles
		for (i = 2; i < (stacks+1)/2; ++i)
		{
			previousZ = currentZ;
			currentZ = radius*cos(angleAlongZ*i);
			previousRadius = currentRadius;
			currentRadius = radius*sin(angleAlongZ*i);
			previousX = currentRadius;
			previousY = 0;
			previousUpperX = previousRadius;
			previousUpperY = 0;
			for (int j = 1; j < slices; ++j)
			{
				currentX = currentRadius*cos(j*angleAroundZ);
				currentY = currentRadius*sin(j*angleAroundZ);
				currentUpperX = previousRadius*cos(j*angleAroundZ);
				currentUpperY = previousRadius*sin(j*angleAroundZ);
				if (mode == NGIN_SOLID)
					glBegin(GL_QUADS);
				else
					glBegin(GL_LINE_LOOP);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, previousZ/radius);
					glTexCoord2f((j-1)/(double)slices,1-(i-1)/(double)stacks);
					glVertex3f(previousUpperX, previousUpperY, previousZ);
					glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
					glTexCoord2f((j-1)/(double)slices,1-i/(double)stacks);
					glVertex3f(previousX, previousY, currentZ);
					glNormal3f(currentX/radius, currentY/radius, currentZ/radius);
					glTexCoord2f(j/(double)slices,1-i/(double)stacks);
					glVertex3f(currentX, currentY, currentZ);
					glNormal3f(currentUpperX/radius, currentUpperY/radius, previousZ/radius);
					glTexCoord2f(j/(double)slices,1-(i-1)/(double)stacks);
					glVertex3f(currentUpperX, currentUpperY, previousZ);
				if (mode == NGIN_WIREFRAME)
				{
					glEnd();
					glBegin(GL_LINE_LOOP);
				}
					glNormal3f(currentUpperX/radius, currentUpperY/radius, -previousZ/radius);
					glTexCoord2f(j/(double)slices,(i-1)/(double)stacks);
					glVertex3f(currentUpperX, currentUpperY, -previousZ);
					glNormal3f(currentX/radius, currentY/radius, -currentZ/radius);
					glTexCoord2f(j/(double)slices,i/(double)stacks);
					glVertex3f(currentX, currentY, -currentZ);
					glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
					glTexCoord2f((j-1)/(double)slices,i/(double)stacks);
					glVertex3f(previousX, previousY, -currentZ);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, -previousZ/radius);
					glTexCoord2f((j-1)/(double)slices,(i-1)/(double)stacks);
					glVertex3f(previousUpperX, previousUpperY, -previousZ);
				glEnd();
				previousX = currentX;
				previousY = currentY;
				previousUpperX = currentUpperX;
				previousUpperY = currentUpperY;
			}
			if (mode == NGIN_SOLID)
				glBegin(GL_QUADS);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, previousZ/radius);
				glTexCoord2f((slices-1)/(double)slices,1-(i-1)/(double)stacks);
				glVertex3f(previousUpperX, previousUpperY, previousZ);
				glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
				glTexCoord2f((slices-1)/(double)slices,1-i/(double)stacks);
				glVertex3f(previousX, previousY, currentZ);
				glNormal3f(currentRadius/radius, 0, currentZ/radius);
				glTexCoord2f(1,1-i/(double)stacks);
				glVertex3f(currentRadius, 0, currentZ);
				glNormal3f(previousRadius/radius, 0, previousZ/radius);
				glTexCoord2f(1,1-(i-1)/(double)stacks);
				glVertex3f(previousRadius, 0, previousZ);
			if (mode == NGIN_WIREFRAME)
			{
				glEnd();
				glBegin(GL_LINE_LOOP);
			}
				glNormal3f(previousRadius/radius, 0, -previousZ/radius);
				glTexCoord2f(1,(i-1)/(double)stacks);
				glVertex3f(previousRadius, 0, -previousZ);
				glNormal3f(currentRadius/radius, 0, -currentZ/radius);
				glTexCoord2f(1,i/(double)stacks);
				glVertex3f(currentRadius, 0, -currentZ);
				glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
				glTexCoord2f((slices-1)/(double)slices,i/(double)stacks);
				glVertex3f(previousX, previousY, -currentZ);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, -previousZ/radius);
				glTexCoord2f((slices-1)/(double)slices,(i-1)/(double)stacks);
				glVertex3f(previousUpperX, previousUpperY, -previousZ);
			glEnd();
		}
		if (stacks%2 == 1)
		{
			previousX = currentRadius;
			previousY = 0;
			for (i = 1; i < slices; ++i)
			{
				currentX = currentRadius*cos(i*angleAroundZ);
				currentY = currentRadius*sin(i*angleAroundZ);
				if (mode == NGIN_SOLID)
					glBegin(GL_QUADS);
				else
					glBegin(GL_LINE_LOOP);
					glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
					glTexCoord2f((i-1)/(double)slices, (stacks+1)/(2*(double)stacks));
					glVertex3f(previousX, previousY, currentZ);
					glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
					glTexCoord2f((i-1)/(double)slices, (stacks-1)/(2*(double)stacks));
					glVertex3f(previousX, previousY, -currentZ);
					glNormal3f(currentX/radius, currentY/radius, -currentZ/radius);
					glTexCoord2f(i/(double)slices, (stacks-1)/(2*(double)stacks));
					glVertex3f(currentX, currentY, -currentZ);
					glNormal3f(currentX/radius, currentY/radius, currentZ/radius);
					glTexCoord2f(i/(double)slices, (stacks+1)/(2*(double)stacks));
					glVertex3f(currentX, currentY, currentZ);
				glEnd();
				previousX = currentX;
				previousY = currentY;
			}
			if (mode == NGIN_SOLID)
				glBegin(GL_QUADS);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
				glTexCoord2f((slices-1)/(double)slices, (stacks+1)/(2*(double)stacks));
				glVertex3f(previousX, previousY, currentZ);
				glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
				glTexCoord2f((slices-1)/(double)slices, (stacks-1)/(2*(double)stacks));
				glVertex3f(previousX, previousY, -currentZ);
				glNormal3f(currentRadius/radius, 0, -currentZ/radius);
				glTexCoord2f(1, (stacks-1)/(2*(double)stacks));
				glVertex3f(currentRadius, 0, -currentZ);
				glNormal3f(currentRadius/radius, 0, currentZ/radius);
				glTexCoord2f(1, (stacks+1)/(2*(double)stacks));
				glVertex3f(currentRadius, 0, currentZ);
			glEnd();
		}
		else
		{
			previousUpperX = currentRadius;
			previousUpperY = 0;
			previousX = radius;
			previousY = 0;
			for (i = 1; i < slices; ++i)
			{
				currentUpperX = currentRadius*cos(i*angleAroundZ);
				currentUpperY = currentRadius*sin(i*angleAroundZ);
				currentX = radius*cos(i*angleAroundZ);
				currentY = radius*sin(i*angleAroundZ);
				if (mode == NGIN_SOLID)
					glBegin(GL_QUADS);
				else
					glBegin(GL_LINE_LOOP);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, currentZ/radius);
					glTexCoord2f((i-1)/(double)slices, 0.5+1/(double)stacks);
					glVertex3f(previousUpperX, previousUpperY, currentZ);
					glNormal3f(previousX/radius, previousY/radius, 0);
					glTexCoord2f((i-1)/(double)slices, 0.5);
					glVertex3f(previousX, previousY, 0);
					glNormal3f(currentX/radius, currentY/radius, 0);
					glTexCoord2f(i/(double)slices, 0.5);
					glVertex3f(currentX, currentY, 0);
					glNormal3f(currentUpperX/radius, currentUpperY/radius, currentZ/radius);
					glTexCoord2f(i/(double)slices, 0.5+1/(double)stacks);
					glVertex3f(currentUpperX, currentUpperY, currentZ);
				if (mode == NGIN_WIREFRAME)
				{
					glEnd();
					glBegin(GL_LINE_LOOP);
				}
					glNormal3f(currentUpperX/radius, currentUpperY/radius, -currentZ/radius);
					glTexCoord2f(i/(double)slices, 0.5-1/(double)stacks);
					glVertex3f(currentUpperX, currentUpperY, -currentZ);
					glNormal3f(currentX/radius, currentY/radius, 0);
					glTexCoord2f(i/(double)slices, 0.5);
					glVertex3f(currentX, currentY, 0);
					glNormal3f(previousX/radius, previousY/radius, 0);
					glTexCoord2f((i-1)/(double)slices, 0.5);
					glVertex3f(previousX, previousY, 0);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, -currentZ/radius);
					glTexCoord2f((i-1)/(double)slices, 0.5-1/(double)stacks);
					glVertex3f(previousUpperX, previousUpperY, -currentZ);
				glEnd();
				previousX = currentX;
				previousY = currentY;
				previousUpperX = currentUpperX;
				previousUpperY = currentUpperY;
			}
			if (mode == NGIN_SOLID)
				glBegin(GL_QUADS);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, currentZ/radius);
				glTexCoord2f((slices-1)/(double)slices, 0.5+1/(double)stacks);
				glVertex3f(previousUpperX, previousUpperY, currentZ);
				glNormal3f(previousX/radius, previousY/radius, 0);
				glTexCoord2f((slices-1)/(double)slices, 0.5);
				glVertex3f(previousX, previousY, 0);
				glNormal3f(1, 0, 0);
				glTexCoord2f(1, 0.5);
				glVertex3f(radius, 0, 0);
				glNormal3f(currentRadius/radius, 0, currentZ/radius);
				glTexCoord2f(1, 0.5+1/(double)stacks);
				glVertex3f(currentRadius, 0, currentZ);
			if (mode == NGIN_WIREFRAME)
			{
				glEnd();
				glBegin(GL_LINE_LOOP);
			}
				glNormal3f(currentRadius/radius, 0, -currentZ/radius);
				glTexCoord2f(1, 0.5-1/(double)stacks);
				glVertex3f(currentRadius, 0, -currentZ);
				glNormal3f(1, 0, 0);
				glTexCoord2f(1, 0.5);
				glVertex3f(radius, 0, 0);
				glNormal3f(previousX/radius, previousY/radius, 0);
				glTexCoord2f((slices-1)/(double)slices, 0.5);
				glVertex3f(previousX, previousY, 0);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, -currentZ/radius);
				glTexCoord2f((slices-1)/(double)slices, 0.5-1/(double)stacks);
				glVertex3f(previousUpperX, previousUpperY, -currentZ);
			glEnd();
		}
	}
	else	// g_Ngin_textureEnabled is not enabled here
	{
		// First draw the upper and lower triangles
		for (i = 1; i < slices; ++i)
		{
			currentX = currentRadius*cos(i*angleAroundZ);
			currentY = currentRadius*sin(i*angleAroundZ);
			if (mode == NGIN_SOLID)
				glBegin(GL_TRIANGLES);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(0, 0, 1);
				glVertex3f(0, 0, radius);
				glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
				glVertex3f(previousX, previousY, currentZ);
				glNormal3f(currentX/radius, currentY/radius, currentZ/radius);
				glVertex3f(currentX, currentY, currentZ);
			if (mode == NGIN_WIREFRAME)
			{
				glEnd();
				glBegin(GL_LINE_LOOP);
			}
				glNormal3f(0, 0, -1);
				glVertex3f(0, 0, -radius);
				glNormal3f(currentX/radius, currentY/radius, -currentZ/radius);
				glVertex3f(currentX, currentY, -currentZ);
				glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
				glVertex3f(previousX, previousY, -currentZ);
			glEnd();
			previousX = currentX;
			previousY = currentY;
		}
		if (mode == NGIN_SOLID)
			glBegin(GL_TRIANGLES);
		else
			glBegin(GL_LINE_LOOP);
			glNormal3f(0, 0, 1);
			glVertex3f(0, 0, radius);
			glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
			glVertex3f(previousX, previousY, currentZ);
			glNormal3f(currentRadius/radius, 0, currentZ/radius);
			glVertex3f(currentRadius, 0, currentZ);
		if (mode == NGIN_WIREFRAME)
		{
			glEnd();
			glBegin(GL_LINE_LOOP);
		}
			glNormal3f(0, 0, -1);
			glVertex3f(0, 0, -radius);
			glNormal3f(currentRadius/radius, 0, -currentZ/radius);
			glVertex3f(currentRadius, 0, -currentZ);
			glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
			glVertex3f(previousX, previousY, -currentZ);
		glEnd();
		// Then draw the middle quadangles
		for (i = 2; i < (stacks+1)/2; ++i)
		{
			previousZ = currentZ;
			currentZ = radius*cos(angleAlongZ*i);
			previousRadius = currentRadius;
			currentRadius = radius*sin(angleAlongZ*i);
			previousX = currentRadius;
			previousY = 0;
			previousUpperX = previousRadius;
			previousUpperY = 0;
			for (int j = 1; j < slices; ++j)
			{
				currentX = currentRadius*cos(j*angleAroundZ);
				currentY = currentRadius*sin(j*angleAroundZ);
				currentUpperX = previousRadius*cos(j*angleAroundZ);
				currentUpperY = previousRadius*sin(j*angleAroundZ);
				if (mode == NGIN_SOLID)
					glBegin(GL_QUADS);
				else
					glBegin(GL_LINE_LOOP);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, previousZ/radius);
					glVertex3f(previousUpperX, previousUpperY, previousZ);
					glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
					glVertex3f(previousX, previousY, currentZ);
					glNormal3f(currentX/radius, currentY/radius, currentZ/radius);
					glVertex3f(currentX, currentY, currentZ);
					glNormal3f(currentUpperX/radius, currentUpperY/radius, previousZ/radius);
					glVertex3f(currentUpperX, currentUpperY, previousZ);
				if (mode == NGIN_WIREFRAME)
				{
					glEnd();
					glBegin(GL_LINE_LOOP);
				}
					glNormal3f(currentUpperX/radius, currentUpperY/radius, -previousZ/radius);
					glVertex3f(currentUpperX, currentUpperY, -previousZ);
					glNormal3f(currentX/radius, currentY/radius, -currentZ/radius);
					glVertex3f(currentX, currentY, -currentZ);
					glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
					glVertex3f(previousX, previousY, -currentZ);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, -previousZ/radius);
					glVertex3f(previousUpperX, previousUpperY, -previousZ);
				glEnd();
				previousX = currentX;
				previousY = currentY;
				previousUpperX = currentUpperX;
				previousUpperY = currentUpperY;
			}
			if (mode == NGIN_SOLID)
				glBegin(GL_QUADS);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, previousZ/radius);
				glVertex3f(previousUpperX, previousUpperY, previousZ);
				glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
				glVertex3f(previousX, previousY, currentZ);
				glNormal3f(currentRadius/radius, 0, currentZ/radius);
				glVertex3f(currentRadius, 0, currentZ);
				glNormal3f(previousRadius/radius, 0, previousZ/radius);
				glVertex3f(previousRadius, 0, previousZ);
			if (mode == NGIN_WIREFRAME)
			{
				glEnd();
				glBegin(GL_LINE_LOOP);
			}
				glNormal3f(previousRadius/radius, 0, -previousZ/radius);
				glVertex3f(previousRadius, 0, -previousZ);
				glNormal3f(currentRadius/radius, 0, -currentZ/radius);
				glVertex3f(currentRadius, 0, -currentZ);
				glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
				glVertex3f(previousX, previousY, -currentZ);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, -previousZ/radius);
				glVertex3f(previousUpperX, previousUpperY, -previousZ);
			glEnd();
		}
		if (stacks%2 == 1)
		{
			previousX = currentRadius;
			previousY = 0;
			for (i = 1; i < slices; ++i)
			{
				currentX = currentRadius*cos(i*angleAroundZ);
				currentY = currentRadius*sin(i*angleAroundZ);
				if (mode == NGIN_SOLID)
					glBegin(GL_QUADS);
				else
					glBegin(GL_LINE_LOOP);
					glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
					glVertex3f(previousX, previousY, currentZ);
					glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
					glVertex3f(previousX, previousY, -currentZ);
					glNormal3f(currentX/radius, currentY/radius, -currentZ/radius);
					glVertex3f(currentX, currentY, -currentZ);
					glNormal3f(currentX/radius, currentY/radius, currentZ/radius);
					glVertex3f(currentX, currentY, currentZ);
				glEnd();
				previousX = currentX;
				previousY = currentY;
			}
			if (mode == NGIN_SOLID)
				glBegin(GL_QUADS);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(previousX/radius, previousY/radius, currentZ/radius);
				glVertex3f(previousX, previousY, currentZ);
				glNormal3f(previousX/radius, previousY/radius, -currentZ/radius);
				glVertex3f(previousX, previousY, -currentZ);
				glNormal3f(currentRadius/radius, 0, -currentZ/radius);
				glVertex3f(currentRadius, 0, -currentZ);
				glNormal3f(currentRadius/radius, 0, currentZ/radius);
				glVertex3f(currentRadius, 0, currentZ);
			glEnd();
		}
		else
		{
			previousUpperX = currentRadius;
			previousUpperY = 0;
			previousX = radius;
			previousY = 0;
			for (i = 1; i < slices; ++i)
			{
				currentUpperX = currentRadius*cos(i*angleAroundZ);
				currentUpperY = currentRadius*sin(i*angleAroundZ);
				currentX = radius*cos(i*angleAroundZ);
				currentY = radius*sin(i*angleAroundZ);
				if (mode == NGIN_SOLID)
					glBegin(GL_QUADS);
				else
					glBegin(GL_LINE_LOOP);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, currentZ/radius);
					glVertex3f(previousUpperX, previousUpperY, currentZ);
					glNormal3f(previousX/radius, previousY/radius, 0);
					glVertex3f(previousX, previousY, 0);
					glNormal3f(currentX/radius, currentY/radius, 0);
					glVertex3f(currentX, currentY, 0);
					glNormal3f(currentUpperX/radius, currentUpperY/radius, currentZ/radius);
					glVertex3f(currentUpperX, currentUpperY, currentZ);
				if (mode == NGIN_WIREFRAME)
				{
					glEnd();
					glBegin(GL_LINE_LOOP);
				}
					glNormal3f(currentUpperX/radius, currentUpperY/radius, -currentZ/radius);
					glVertex3f(currentUpperX, currentUpperY, -currentZ);
					glNormal3f(currentX/radius, currentY/radius, 0);
					glVertex3f(currentX, currentY, 0);
					glNormal3f(previousX/radius, previousY/radius, 0);
					glVertex3f(previousX, previousY, 0);
					glNormal3f(previousUpperX/radius, previousUpperY/radius, -currentZ/radius);
					glVertex3f(previousUpperX, previousUpperY, -currentZ);
				glEnd();
				previousX = currentX;
				previousY = currentY;
				previousUpperX = currentUpperX;
				previousUpperY = currentUpperY;
			}
			if (mode == NGIN_SOLID)
				glBegin(GL_QUADS);
			else
				glBegin(GL_LINE_LOOP);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, currentZ/radius);
				glVertex3f(previousUpperX, previousUpperY, currentZ);
				glNormal3f(previousX/radius, previousY/radius, 0);
				glVertex3f(previousX, previousY, 0);
				glNormal3f(1, 0, 0);
				glVertex3f(radius, 0, 0);
				glNormal3f(currentRadius/radius, 0, currentZ/radius);
				glVertex3f(currentRadius, 0, currentZ);
			if (mode == NGIN_WIREFRAME)
			{
				glEnd();
				glBegin(GL_LINE_LOOP);
			}
				glNormal3f(currentRadius/radius, 0, -currentZ/radius);
				glVertex3f(currentRadius, 0, -currentZ);
				glNormal3f(1, 0, 0);
				glVertex3f(radius, 0, 0);
				glNormal3f(previousX/radius, previousY/radius, 0);
				glVertex3f(previousX, previousY, 0);
				glNormal3f(previousUpperX/radius, previousUpperY/radius, -currentZ/radius);
				glVertex3f(previousUpperX, previousUpperY, -currentZ);
			glEnd();
		}
	}
}
