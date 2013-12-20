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

#ifndef NGIN_LIGHT_H_BY_CYCLOPS
#define NGIN_LIGHT_H_BY_CYCLOPS

class shNginLight
{
private:
	bool dirty;
public:
	bool enabled;
	int id;
	float position[4];
	float ambientColor[4];
	float diffuseColor[4];
	float specularColor[4];
	float shininess;		// is actually a material property!
	float direction[3];
	float cutoffAngle;
	float spotExponent;
	float constantAttenuation;
	float linearAttenuation;
	float quadraticAttenuation;
	shNginLight(int lightId);	// starting from 0, will correspond to GL_LIGHTx. Get maximum number of lights possible with shNginGetMaximumNumberOfLightsPossible()
					// It is possible to have multiple light objects on the same opengl light source, but of course only one can be enabled in each scene.
					// The library would reapply the light properties if light with the same id as a previously enabled one is enabled, therefore, enabling
					// and disabling lights with the same id frequently is NOT at all recommended. In case absolutely necessary, it may be a good idea to do
					// so manually to prevent extra unnecessary calls. To reapply the light properties, 12 function calls are made to openGL. However,
					// enabling and disabling the same light object only costs a call to glEnable() and therefore is optimal.
	shNginLight(const shNginLight &RHS);
	~shNginLight();
	void shNginLSetPosition(float pos[4]);
	void shNginLSetAmbient(float light[4]);
	void shNginLSetDiffuse(float light[4]);
	void shNginLSetSpecular(float light[4]);
	void shNginLSetShininess(float s);
	void shNginLSetDirection(float dir[3]);
	void shNginLSetCutoffAngle(float angle);
	void shNginLSetSpotExponent(float spotExp);
	void shNginLSetConstantAttenuation(float att);
	void shNginLSetLinearAttenuation(float att);
	void shNginLSetQuadraticAttenuation(float att);
	void shNginLEnable();
	void shNginLDisable();
	void shNginLUpdateLight();	// After calls to set light properties, either shNginLEnable must be called or shNginLUpdateLight to ensure actual setting of these
					// parameters in openGL. Due to the frequency of solely updating the position of the light, shNginLUpdatePosition is dedicated to do so.
	void shNginLUpdatePosition();	// A call to this function is necessary in case of moving light, or light fixed relative to the world coordinates (as opposed to the
					// eye coordinates) as openGL light position also undergoes transformations. Make sure to call this function after proper transformations
					// are made.
	shNginLight &operator =(const shNginLight &RHS);
};

int shNginGetMaximumNumberOfLightsPossible();

#endif
