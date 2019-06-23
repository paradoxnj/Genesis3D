/****************************************************************************************/
/*  Light.h                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Lights a BSP                                                           */
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
#ifndef LIGHT_H
#define LIGHT_H

#include <Windows.h>
#include <Stdio.h>

#include "GBSPfile.h"
#include "GBSPLib.h"				// Lightparms
#include "BSP.h"

#define MAX_PATCHES		65000		// Any higher and 16 bit overflow will result

#define MAX_LTYPES		4
#define MAX_LTYPE_INDEX	12

#define LGRID_SIZE		16			// Must be multiple of 2
//#define	MAX_LMAP_SIZE	128			// In lightmap pexels (This will allow the face to be 128*16 in floating units at lmap scale of 1.0f)
//#define	MAX_LMAP_SIZE	64			// In lightmap pexels (This will allow the face to be 64*16 in floating units at lmap scale of 1.0f)
#define	MAX_LMAP_SIZE	18			// In lightmap pexels

extern float		LightScale;
extern float		EntityScale;
extern float		MaxLight;
extern int32		NumSamples;

extern geBoolean	LVerbose;
extern geBoolean	DoRadiosity;
extern float		PatchSize;
extern int32		NumBounce;
extern geBoolean	FastPatch;
extern float		ReflectiveScale;

extern geVec3d		MinLight;

int32 FindGFXLeaf(int32 Node, geVec3d *Vert);
geBoolean RayCollision(geVec3d *Front, geVec3d *Back, geVec3d *I);

typedef struct
{
	geVec3d		*RGBLData[MAX_LTYPE_INDEX];
	int32		NumLTypes;
	geBoolean	RGB;
	float		Mins[2];
	float		Maxs[2];
	int32		LMaxs[2];
	int32		LMins[2];
	int32		LSize[2];
} LInfo;

typedef struct
{
	int32		Face;
	GFX_Plane	Plane;
	geVec3d		T2WVecs[2];
	geVec3d		TexOrg;
	geVec3d		*Points;
	int32		NumPoints;

	geVec3d		Center;
	geFloat		Radius;
} FInfo;

extern LInfo	*Lightmaps;
extern FInfo	*FaceInfo;

geBoolean LightGBSPFile(char *FileName, LightParms *Parms);

void CleanupLight(void);

//====================================================================================
//	Radiosity stuff
//====================================================================================
struct _RAD_Receiver;

typedef struct _RAD_Patch
{
	_RAD_Patch		*Next;				// Next patch in list

	GBSP_Poly		*Poly;				// Poly for patch	(Not used thoughout entire life)
	geVec3d			Origin;				// Origin
	int32			Leaf;				// Leaf patch is looking into
	float			Area;				// Area of patch
	GBSP_Plane		Plane;				// Plane
	uint16			NumReceivers;
	_RAD_Receiver	*Receivers;			// What patches this patch emits to
	int32			NumSamples;			// Number of samples lightmaps has contributed

	geVec3d			RadStart;			// Power of patch from original lightmap
	geVec3d			RadSend;			// How much to send each bounce
	geVec3d			RadReceive;			// How much received from current bounce
	geVec3d			RadFinal;			// How much received from all bounces (what to add back to the lightmap)

	geVec3d			Reflectivity;

	geVec3d			Mins;				// Mins/ Max of patch
	geVec3d			Maxs;
} RAD_Patch, *pRAD_Patch;

typedef struct _RAD_Receiver
{
	//_RAD_Receiver	*Next;				// Next Receiver for patch
	//RAD_Patch		*Patch;				// Patch this Receiver emits ti
	//float			Amount;				// How much to the receiving patch gets
	uint16			Patch;
	uint16			Amount;
} RAD_Receiver;

extern pRAD_Patch	*FacePatches;
extern pRAD_Patch	*PatchList;
extern float		*RecAmount;

extern int32		NumPatches;
extern int32		NumReceivers;

geBoolean BuildPatches(void);
void FreePatches(void);
geBoolean CalcReceivers(char *FileName);
void FreeReceivers(void);
geBoolean BouncePatches(void);
geBoolean AbsorbPatches(void);

typedef struct Tri_Edge
{
	int32				p0, p1;
	geVec3d				normal;
	float				dist;
	struct Tri			*tri;
} Tri_Edge;

typedef struct Tri
{
	Tri_Edge			*Edges[3];
} Tri;

#define	MAX_TRI_POINTS		1024
#define	MAX_TRI_EDGES		(MAX_TRI_POINTS*6)
#define	MAX_TRI_TRIS		(MAX_TRI_POINTS*2)

typedef struct
{
	int			NumPoints;
	int			NumEdges;
	int			NumTris;
	GBSP_Plane	*Plane;
	Tri_Edge	*EdgeMatrix[MAX_TRI_POINTS][MAX_TRI_POINTS];
	RAD_Patch	*Points[MAX_TRI_POINTS];
	Tri_Edge	Edges[MAX_TRI_EDGES];
	Tri			TriList[MAX_TRI_TRIS];
} Tri_Patch;

Tri_Patch	*Tri_PatchCreate(GBSP_Plane *Plane);
void		Tri_PatchDestroy(Tri_Patch *tr);
Tri_Edge	*FindEdge (Tri_Patch *TriPatch, int p0, int p1);
Tri			*AllocTriangle(Tri_Patch *TriPatch);
geBoolean	Tri_Edge_r(Tri_Patch *TriPatch, Tri_Edge *e);
geBoolean	TriangulatePoints(Tri_Patch *TriPatch);
geBoolean	AddPointToTriangulation (RAD_Patch *patch, Tri_Patch *TriPatch);
void		LerpTriangle (Tri_Patch *TriPatch, Tri *t, geVec3d *Point, geVec3d *Color);
geBoolean	Tri_PointInside(Tri *Tri, geVec3d *Point);
geBoolean	SampleTriangulation(geVec3d *Point, Tri_Patch *TriPatch, geVec3d *Color);
geBoolean	FindClosestTriPoint(geVec3d *Point, Tri_Patch *Tri, geVec3d *Color);


#endif

