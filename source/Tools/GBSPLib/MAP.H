/****************************************************************************************/
/*  Map.h                                                                               */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Module for loading a .map file.                                        */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef MAP_H
#define MAP_H

#include <Windows.h>

#include "BSP.h"
#include "Motion.h"

#define MAX_MAP_ENTITIES			4096
#define MAX_MAP_TEXINFO				8192
#define MAX_MAP_TEXTURES			1024

#pragma pack(1)

typedef struct
{
	int32		Version;
	char		TAG[4];
	int32		NumEntities;
} MAP_BrushFileHeader;

typedef struct
{
	int32		Flags;
	int32		NumFaces;
} MAP_BrushHeader;

typedef struct
{
	int32		Flags;
	float		MipMapBias;
	float		Alpha;					// Alpha for brush
	float		FaceLight;				// Light intensity for face light
	float		ReflectiveScale;
	char		TexName[32];
	float		uVecX, uVecY, uVecZ;
	float		DrawScaleX, OffsetX;
	float		vVecX, vVecY, vVecZ;
	float		DrawScaleY, OffsetY;

} MAP_FaceHeader;

#pragma pack()

typedef struct _MAP_Epair
{
	_MAP_Epair	*Next;
	char		*Key;
	char		*Value;

} MAP_Epair;

#define ENTITY_HAS_ORIGIN		(1<<0)

typedef struct
{
	MAP_Brush		*Brushes2;

	geMotion		*Motion;			// Temp motion data for entity if it contains a model

	MAP_Epair		*Epairs;

	int32			ModelNum;			// If brushes != NULL, entity will have a model num

	// For light stage
	char			ClassName[64];
	geVec3d			Origin;				// Well, this is used everywhere...
	float			Angle;
	int32			Light;
	int32			LType;
	char			Target[32];
	char			TargetName[32];

	uint32			Flags;
} MAP_Entity;

extern	int32		NumEntities;
extern	MAP_Entity	Entities[MAX_MAP_ENTITIES];

//=====================================================================================
//	Entity parsing functions
//=====================================================================================
char		*ValueForKey(MAP_Entity *Ent, char *Key);
void		SetKeyValue(MAP_Entity *Ent, char *Key, char *Value);
float		FloatForKey(MAP_Entity *Ent, char *Key);
void		GetVectorForKey(MAP_Entity *Ent, char *Key, geVec3d *Vec);
geBoolean	GetVectorForKey2(MAP_Entity *Ent, char *Key, geVec3d *Vec);
void		GetColorForKey(MAP_Entity *Ent, char *Key, geVec3d *Vec);
char		*NewString(char *Str);

//=====================================================================================
//	Misc functions
//=====================================================================================
geBoolean	SaveEntityData(geVFile *VFile);
geBoolean	LoadEntityData(geVFile *VFile);
geBoolean	ConvertGFXEntDataToEntities(void);
geBoolean	ConvertEntitiesToGFXEntData(void);
void		FreeAllEntities(void);
geBoolean	LoadBrushFile(char *FileName);

#endif
