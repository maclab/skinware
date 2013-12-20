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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shLoad3ds.h"

#define				SHMACROS_LOG_PREFIX			"Ngin"

#include "shmacros.h"

#define WHILE_OVER_CHUNK(level) while (length[level] > 6)
#define READ_CHUNK_HEADER(level) fread(&chunkID, 2, 1, fin);\
				fread(&length[level], 4, 1, fin);\
				length[level-1] -= length[level]
#define SKIP_CHUNK(level) fseek(fin, length[level]-6, SEEK_CUR)

sh3DSMaterial::sh3DSMaterial()
{
}

sh3DSMaterial::sh3DSMaterial(FILE *file, int length)
{
	sh3DSMLoad(file, length);
}

void sh3DSMaterial::sh3DSMLoad(FILE *fin, int len)
{
	unsigned short int chunkID;
	int length[3];
	length[0] = len;
	WHILE_OVER_CHUNK(0)
	{
		READ_CHUNK_HEADER(1);
		switch (chunkID)
		{
			case 0xa000:
				for (int i = 0; i < length[1]-6; ++i)
					fread(&name[i], 1, 1, fin);
				texFile[0] = '\0';
				break;
			case 0xa200:
				WHILE_OVER_CHUNK(1)
				{
					READ_CHUNK_HEADER(2);
					switch (chunkID)
					{
						case 0xa300:
							for (int i = 0; i < length[2]-6; ++i)
								fread(&texFile[i], 1, 1, fin);
							break;
						default:
							SKIP_CHUNK(2);
							break;
					}
				}
				break;
			default:
				SKIP_CHUNK(1);
				break;
		}
	}
}

sh3DSObject::sh3DSObject()
{
	vertices = '\0';
	normals = '\0';
	texCoords = '\0';
	faces = '\0';
	verticesCount = 0;
	texCoordsCount = 0;
	facesCount = 0;
	material = -1;
}

sh3DSObject::sh3DSObject(FILE *file, int length, vector<sh3DSMaterial> &mats)
{
	vertices = '\0';
	normals = '\0';
	texCoords = '\0';
	faces = '\0';
	verticesCount = 0;
	texCoordsCount = 0;
	facesCount = 0;
	material = -1;
	sh3DSOLoad(file, length, mats);
}

sh3DSObject::sh3DSObject(const sh3DSObject &RHS)
{
	*this = RHS;
}

sh3DSObject::~sh3DSObject()
{
	sh3DSOFree();
}

void sh3DSObject::sh3DSOFree()
{
	if (vertices)
		delete[] vertices;
	if (texCoords)
		delete[] texCoords;
	if (faces)
		delete[] faces;
	vertices = '\0';
	texCoords = '\0';
	faces = '\0';
	verticesCount = 0;
	texCoordsCount = 0;
	facesCount = 0;
	material = -1;
}

void sh3DSObject::sh3DSOLoad(FILE *fin, int len, vector<sh3DSMaterial> &mats)
{
	sh3DSOFree();
	unsigned short int chunkID;
	int length[4];
	length[0] = len;
	int l = 0;
	do
	{
		fread(name+l, 1, 1, fin);
		--length[0];
	} while (name[l++]);
	WHILE_OVER_CHUNK(0)
	{
		READ_CHUNK_HEADER(1);
		switch (chunkID)
		{
			case 0x4100:
				WHILE_OVER_CHUNK(1)
				{
					READ_CHUNK_HEADER(2);
					switch (chunkID)
					{
						case 0x4110:
							fread(&verticesCount, 2, 1, fin);
							vertices = new float[verticesCount*3];
							for (int i = 0; i < verticesCount; ++i)
								fread(vertices+i*3, 4, 3, fin);
							break;
						case 0x4120:
							unsigned short int faceinfo;
							fread(&facesCount, 2, 1, fin);
							length[2] -= 2;
							faces = new unsigned short int[facesCount*3];
							for (int i = 0; i < facesCount; ++i)
							{
								fread(faces+i*3, 2, 3, fin);
								fread(&faceinfo, 2, 1, fin);
								length[2] -= 8;
							}
							WHILE_OVER_CHUNK(2)
							{
								READ_CHUNK_HEADER(3);
								switch (chunkID)
								{
									case 0x4130:
										char name[100];
										int l;
										l = 0;
										do
										{
											fread(name+l, 1, 1, fin);
										} while (name[l++]);
										unsigned short int facecount;
										fread(&facecount, 2, 1, fin);
										unsigned short int face;		// actually, must read which faces are affected by this material
										for (int i = 0; i < facecount; ++i)
											fread(&face, 2, 1, fin);
										for (unsigned int i = 0; i < mats.size(); ++i)
											if (strcmp(name, mats[i].name) == 0)
											{
												material = i;
												break;
											}
										break;
									default:
										SKIP_CHUNK(3);
										break;
								}
							}
							break;
						case 0x4140:
							fread(&texCoordsCount, 2, 1, fin);
							texCoords = new float[texCoordsCount*2];
							for (int i = 0; i < texCoordsCount; ++i)
								fread(texCoords+i*2, 4, 2, fin);
							break;
						default:
							SKIP_CHUNK(2);
							break;
					}
				}
				break;
			default:
				SKIP_CHUNK(1);
				break;
		}
	}
}

void sh3DSObject::sh3DSOCalculateNormals()
{
	if (normals)
		delete[] normals;
	normals = new float[verticesCount*3];
	memset(normals, 0, verticesCount*3*sizeof(float));
	float vec1[3], vec2[3];
	for (int i = 0; i < facesCount; ++i)
	{
		vec1[0] = vertices[faces[i*3+1]*3]-vertices[faces[i*3]*3];
		vec1[1] = vertices[faces[i*3+1]*3+1]-vertices[faces[i*3]*3+1];
		vec1[2] = vertices[faces[i*3+1]*3+2]-vertices[faces[i*3]*3+2];
		vec2[0] = vertices[faces[i*3+2]*3]-vertices[faces[i*3+1]*3];
		vec2[1] = vertices[faces[i*3+2]*3+1]-vertices[faces[i*3+1]*3+1];
		vec2[2] = vertices[faces[i*3+2]*3+2]-vertices[faces[i*3+1]*3+2];
		float norm[3] = {CROSS(vec1, vec2)};
		normals[faces[i*3]*3] += norm[0]; normals[faces[i*3]*3+1] += norm[1]; normals[faces[i*3]*3+2] += norm[2];
		normals[faces[i*3+1]*3] += norm[0]; normals[faces[i*3+1]*3+1] += norm[1]; normals[faces[i*3+1]*3+2] += norm[2];
		normals[faces[i*3+2]*3] += norm[0]; normals[faces[i*3+2]*3+1] += norm[1]; normals[faces[i*3+2]*3+2] += norm[2];
	}
	for (int i = 0; i < verticesCount; ++i)
	{
		float norm = NORM(normals+i*3);
		if (norm == 0)
			norm = 1.0f;
		normals[i*3] /= norm;
		normals[i*3+1] /= norm;
		normals[i*3+2] /= norm;
	}
}

sh3DSObject &sh3DSObject::operator =(const sh3DSObject &RHS)
{
	verticesCount = RHS.verticesCount;
	texCoordsCount = RHS.texCoordsCount;
	facesCount = RHS.facesCount;
	vertices = new float[verticesCount*3];
	memcpy(vertices, RHS.vertices, verticesCount*3*sizeof(float));
	texCoords = new float[texCoordsCount*2];
	memcpy(texCoords, RHS.texCoords, texCoordsCount*2*sizeof(float));
	faces = new unsigned short int[facesCount*3];
	memcpy(faces, RHS.faces, facesCount*3*sizeof(unsigned short int));
	material = RHS.material;
	return *this;
}

bool sh3DSObject::sh3DSOIsLoaded() const
{
	return (vertices && faces);
}

bool sh3DSObject::sh3DSOHasTextureMapping() const
{
	return texCoords != '\0';
}

sh3DSHierarchy::sh3DSHierarchy()
{
	objects.clear();
	parents = '\0';
}

sh3DSHierarchy::sh3DSHierarchy(const char *file)
{
	objects.clear();
	parents = '\0';
	sh3DSHLoad(file);
}

sh3DSHierarchy::sh3DSHierarchy(const sh3DSHierarchy &RHS)
{
	*this = RHS;
}

sh3DSHierarchy::~sh3DSHierarchy()
{
	sh3DSHFree();
}

void sh3DSHierarchy::sh3DSHFree()
{
	if (parents)
		delete[] parents;
	objects.clear();
	parents = '\0';
}

void sh3DSHierarchy::sh3DSHLoad(const char *file)
{
	sh3DSHFree();
	int length[4];
	unsigned short int chunkID;
	sh3DSObject obj;
	sh3DSMaterial mat;
	FILE *fin = fopen(file, "rb");
	fread(&chunkID, 2, 1, fin);
	fread(&length[0], 4, 1, fin);
	if (chunkID != 0x4d4d)
		return;
	WHILE_OVER_CHUNK(0)
	{
		READ_CHUNK_HEADER(1);
		switch (chunkID)
		{
			case 0x3d3d:
				WHILE_OVER_CHUNK(1)
				{
					READ_CHUNK_HEADER(2);
					switch (chunkID)
					{
						case 0xafff:
							mat.sh3DSMLoad(fin, length[2]);
							materials.push_back(mat);
							break;
						case 0x4000:
							obj.sh3DSOLoad(fin, length[2], materials);
							objects.push_back(obj);
							break;
						default:
							SKIP_CHUNK(2);
							break;
					}
				}
				break;
			case 0xb000:
				parents = new unsigned short int[objects.size()];
				memset(parents, -1, objects.size()*sizeof(unsigned short int));
				WHILE_OVER_CHUNK(1)
				{
					READ_CHUNK_HEADER(2);
					switch (chunkID)
					{
						case 0xb002:
							int objid;
							objid = -1;
							WHILE_OVER_CHUNK(2)
							{
								READ_CHUNK_HEADER(3);
								switch (chunkID)
								{
									case 0xb010:
										char name[100];
										int l;
										l = 0;
										do
										{
											fread(name+l, 1, 1, fin);
										} while (name[l++]);
										for (unsigned int i = 0; i < objects.size(); ++i)
											if (strcmp(name, objects[i].name) == 0)
											{
												objid = i;
												break;
											}
										unsigned short int temp;
										fread(&temp, 2, 1, fin);
										fread(&temp, 2, 1, fin);
										fread(&temp, 2, 1, fin);
										if (objid != -1)
											parents[objid] = temp;
										break;
									default:
										SKIP_CHUNK(3);
										break;
								}
							}
							break;
						default:
							SKIP_CHUNK(2);
							break;
					}
				}
				break;
			default:
				SKIP_CHUNK(1);
				break;
		}
	}
}

sh3DSHierarchy &sh3DSHierarchy::operator =(const sh3DSHierarchy &RHS)
{
	objects = RHS.objects;
	parents = new unsigned short int[objects.size()];
	memcpy(parents, RHS.parents, objects.size()*sizeof(unsigned short int));
	return *this;
}

bool sh3DSHierarchy::sh3DSHIsLoaded() const
{
	return (objects.size() > 0 && parents);
}
