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

#include <string.h>
#include "Ngin_opengl.h"
#include "Ngin_light.h"

shNginLight			**g_Ngin_lightsDirty			= '\0';

shNginLight::shNginLight(int lightId)
{
	enabled = false;
	id = lightId;
	position[0] = position[1] = position[2] = 0; position[3] = 1.0f;
	ambientColor[0] = ambientColor[1] = ambientColor[2] = 0; ambientColor[3] = 1.0f;
	diffuseColor[0] = diffuseColor[1] = diffuseColor[2] = 0; diffuseColor[3] = 1.0f;
	specularColor[0] = specularColor[1] = specularColor[2] = 0; specularColor[3] = 1.0f;
	shininess = 0;
	direction[0] = direction[1] = 0; direction[2] = -1.0f;
	cutoffAngle = 180.0f;
	spotExponent = 0;
	constantAttenuation = 1.0f;
	linearAttenuation = 0;
	quadraticAttenuation = 0;
}

shNginLight::shNginLight(const shNginLight &RHS)
{
	*this = RHS;
}

shNginLight::~shNginLight()
{
	shNginLDisable();
}

void shNginLight::shNginLSetPosition(float pos[4])
{
	position[0] = pos[0];
	position[1] = pos[1];
	position[2] = pos[2];
	position[3] = pos[3];
	dirty = true;
}

void shNginLight::shNginLSetAmbient(float light[4])
{
	ambientColor[0] = light[0];
	ambientColor[1] = light[1];
	ambientColor[2] = light[2];
	ambientColor[3] = light[3];
	dirty = true;
}

void shNginLight::shNginLSetDiffuse(float light[4])
{
	diffuseColor[0] = light[0];
	diffuseColor[1] = light[1];
	diffuseColor[2] = light[2];
	diffuseColor[3] = light[3];
	dirty = true;
}

void shNginLight::shNginLSetSpecular(float light[4])
{
	specularColor[0] = light[0];
	specularColor[1] = light[1];
	specularColor[2] = light[2];
	specularColor[3] = light[3];
	dirty = true;
}

void shNginLight::shNginLSetShininess(float s)
{
	shininess = s;
	dirty = true;
}

void shNginLight::shNginLSetDirection(float dir[3])
{
	direction[0] = dir[0];
	direction[1] = dir[1];
	direction[2] = dir[2];
	dirty = true;
}

void shNginLight::shNginLSetCutoffAngle(float angle)
{
	cutoffAngle = angle;
	dirty = true;
}

void shNginLight::shNginLSetSpotExponent(float spotExp)
{
	spotExponent = spotExp;
	dirty = true;
}

void shNginLight::shNginLSetConstantAttenuation(float att)
{
	constantAttenuation = att;
	dirty = true;
}

void shNginLight::shNginLSetLinearAttenuation(float att)
{
	linearAttenuation = att;
	dirty = true;
}

void shNginLight::shNginLSetQuadraticAttenuation(float att)
{
	quadraticAttenuation = att;
	dirty = true;
}

void shNginLight::shNginLEnable()
{
	glEnable(GL_LIGHT0+id);
	if (dirty || g_Ngin_lightsDirty[id] != this)				// It means either the light is not initialized, or another shNginLight object works on the same id
		shNginLUpdateLight();
	enabled = true;
}

void shNginLight::shNginLDisable()
{
	glDisable(GL_LIGHT0+id);
	enabled = false;
}

void shNginLight::shNginLUpdateLight()
{
	glLightfv(GL_LIGHT0+id, GL_POSITION, position);
	glLightfv(GL_LIGHT0+id, GL_AMBIENT, ambientColor);
	glLightfv(GL_LIGHT0+id, GL_DIFFUSE, diffuseColor);
	glLightfv(GL_LIGHT0+id, GL_SPECULAR, specularColor);
	float matspec[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	glMaterialfv(GL_FRONT, GL_SPECULAR, matspec);			// default is 0, 0, 0, 1. Play with the numbers see which is good
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
	glLightf(GL_LIGHT0+id, GL_SPOT_CUTOFF, cutoffAngle);
	glLightf(GL_LIGHT0+id, GL_SPOT_EXPONENT, spotExponent);
	glLightfv(GL_LIGHT0+id, GL_SPOT_DIRECTION, direction);
	glLightf(GL_LIGHT0+id, GL_CONSTANT_ATTENUATION, constantAttenuation);
	glLightf(GL_LIGHT0+id, GL_LINEAR_ATTENUATION, linearAttenuation);
	glLightf(GL_LIGHT0+id, GL_QUADRATIC_ATTENUATION, quadraticAttenuation);
	dirty = false;
	g_Ngin_lightsDirty[id] = this;
}

void shNginLight::shNginLUpdatePosition()
{
	glLightfv(GL_LIGHT0+id, GL_POSITION, position);
}

int shNginGetMaximumNumberOfLightsPossible()
{
	int res = 8;		// OpenGL's guaranteed minimum
	glGetIntegerv(GL_MAX_LIGHTS, &res);
	return res;
}

shNginLight &shNginLight::operator =(const shNginLight &RHS)
{
	dirty = RHS.dirty;
	enabled = false;
	id = RHS.id;
	position[0] = RHS.position[0];
	position[1] = RHS.position[1];
	position[2] = RHS.position[2];
	position[3] = RHS.position[3];
	ambientColor[0] = RHS.ambientColor[0];
	ambientColor[1] = RHS.ambientColor[1];
	ambientColor[2] = RHS.ambientColor[2];
	ambientColor[3] = RHS.ambientColor[3];
	diffuseColor[0] = RHS.diffuseColor[0];
	diffuseColor[1] = RHS.diffuseColor[1];
	diffuseColor[2] = RHS.diffuseColor[2];
	diffuseColor[3] = RHS.diffuseColor[3];
	specularColor[0] = RHS.specularColor[0];
	specularColor[1] = RHS.specularColor[1];
	specularColor[2] = RHS.specularColor[2];
	specularColor[3] = RHS.specularColor[3];
	shininess = RHS.shininess;
	direction[0] = RHS.direction[0];
	direction[1] = RHS.direction[1];
	direction[2] = RHS.direction[2];
	cutoffAngle = RHS.cutoffAngle;
	spotExponent = RHS.spotExponent;
	constantAttenuation = RHS.constantAttenuation;
	linearAttenuation = RHS.linearAttenuation;
	quadraticAttenuation = RHS.quadraticAttenuation;
	return *this;
}
