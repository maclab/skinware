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

#ifndef SHLOAD3DS_H_BY_CYCLOPS
#define SHLOAD3DS_H_BY_CYCLOPS

#include <stdio.h>
#include <vector>

using namespace std;

class sh3DSMaterial
{
public:
	char name[31];
	char texFile[13];			// 8.3 system
	sh3DSMaterial();
	sh3DSMaterial(FILE *file, int length);
	void sh3DSMLoad(FILE *file, int length);// file should be already opened. Length of material chunk must be given (that includes the first 6 bytes of each chunk)
						// assumes 0xafff is already read
};

class sh3DSObject
{
public:
	float *vertices;			// an array of size verticesCount*3 (x, y, z of each vertex)
	float *normals;				// an array of size verticesCount*3 (x, y, z of normals to each vertex). Note, this property is not stored in the 3DS file (TODO: check this)
	float *texCoords;			// an array of size texCoordsCount*2 (x, y of the texture map)
	unsigned short int verticesCount;
	unsigned short int texCoordsCount;
	unsigned short int *faces;		// an array of size facesCount*3 (indices to vertices array indicating vertices of each triangular face)
	unsigned short int facesCount;
	int material;				// NOTE: support for only ONE texture for each object. TODO: support for all kind of material
						// index to sh3DSHierarchy::materials
	char name[31];
	sh3DSObject();
	sh3DSObject(FILE *file, int length, vector<sh3DSMaterial> &mats);	// file should be already opened. Length of object chunk must be given (that includes the first 6 bytes of each chunk)
						// assumes 0x4000 is already read
						// A list of materials is passed to the function to determine which material it has
	sh3DSObject(const sh3DSObject &RHS);
	~sh3DSObject();
	void sh3DSOFree();
	void sh3DSOLoad(FILE *file, int length, vector<sh3DSMaterial> &mats);
	void sh3DSOCalculateNormals();		// Takes average of normals of faces each vertex is a vertex of. Guarantees that the normal is scaled to size 1.
	sh3DSObject &operator =(const sh3DSObject &RHS);
	bool sh3DSOIsLoaded() const;
	bool sh3DSOHasTextureMapping() const;
};

class sh3DSHierarchy
{
public:
	vector<sh3DSObject> objects;
	vector<sh3DSMaterial> materials;
	unsigned short int *parents;
	sh3DSHierarchy();
	sh3DSHierarchy(const char *file);
	sh3DSHierarchy(const sh3DSHierarchy &RHS);
	~sh3DSHierarchy();
	void sh3DSHFree();
	void sh3DSHLoad(const char *file);
	sh3DSHierarchy &operator =(const sh3DSHierarchy &RHS);
	bool sh3DSHIsLoaded() const;
};

#endif
