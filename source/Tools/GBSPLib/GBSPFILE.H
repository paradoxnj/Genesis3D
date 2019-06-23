/****************************************************************************************/
/*  GBSPFile.h                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Loads a BSP                                                            */
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
#ifndef GBSPFILE_H
#define GBSPFILE_H

#include <Windows.h>
#include <Stdio.h>

#include "DCommon.h"
#include "MathLib.h"
#include "vfile.h"
#include "motion.h"
#include "Vec3d.h"

#define GBSP_VERSION				15

#define GBSP_CHUNK_HEADER			0

#define GBSP_CHUNK_MODELS			1
#define GBSP_CHUNK_NODES			2
#define GBSP_CHUNK_BNODES			3
#define GBSP_CHUNK_LEAFS			4
#define GBSP_CHUNK_CLUSTERS			5	
#define GBSP_CHUNK_AREAS			6	
#define GBSP_CHUNK_AREA_PORTALS		7	
#define GBSP_CHUNK_LEAF_SIDES		8
#define GBSP_CHUNK_PORTALS			9
#define GBSP_CHUNK_PLANES			10
#define GBSP_CHUNK_FACES			11
#define GBSP_CHUNK_LEAF_FACES		12
#define GBSP_CHUNK_VERT_INDEX		13
#define GBSP_CHUNK_VERTS			14
#define GBSP_CHUNK_RGB_VERTS		15
#define GBSP_CHUNK_ENTDATA			16

#define GBSP_CHUNK_TEXINFO			17
#define GBSP_CHUNK_TEXTURES			18 
#define GBSP_CHUNK_TEXDATA			19

#define GBSP_CHUNK_LIGHTDATA		20
#define GBSP_CHUNK_VISDATA			21

#define GBSP_CHUNK_SKYDATA			22

#define GBSP_CHUNK_PALETTES			23

#define GBSP_CHUNK_MOTIONS			24

#define GBSP_CHUNK_END				0xffff

#define MAX_GBSP_ENTDATA			200000

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//	IF THESE FLAGS CHANGE, THEY MUST CHANGE IN GBSPFILE.H in Genesis AND GBSPLIB!!!!!
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
#define BSP_CONTENTS_SOLID2			(1<<0)		// Solid (Visible)
#define BSP_CONTENTS_WINDOW2		(1<<1)		// Window (Visible)
#define BSP_CONTENTS_EMPTY2			(1<<2)		// Empty but Visible (water, lava, etc...)

#define BSP_CONTENTS_TRANSLUCENT2	(1<<3)		// Vis will see through it
#define BSP_CONTENTS_WAVY2			(1<<4)		// Wavy (Visible)
#define BSP_CONTENTS_DETAIL2		(1<<5)		// Won't be included in vis oclusion

#define BSP_CONTENTS_CLIP2			(1<<6)		// Structural but not visible
#define BSP_CONTENTS_HINT2			(1<<7)		// Primary splitter (Non-Visible)
#define BSP_CONTENTS_AREA2			(1<<8)		// Area seperator leaf (Non-Visible)

#define BSP_CONTENTS_FLOCKING		(1<<9)		// flocking flag.  Not really a contents type
#define BSP_CONTENTS_SHEET			(1<<10)
#define RESERVED3					(1<<11)
#define RESERVED4					(1<<12)
#define RESERVED5					(1<<13)
#define RESERVED6					(1<<14)
#define RESERVED7					(1<<15)

// 16-31 reserved for user contents
#define BSP_CONTENTS_USER1			(1<<16)
#define BSP_CONTENTS_USER2			(1<<17)
#define BSP_CONTENTS_USER3			(1<<18)
#define BSP_CONTENTS_USER4			(1<<19)
#define BSP_CONTENTS_USER5			(1<<20)
#define BSP_CONTENTS_USER6			(1<<21)
#define BSP_CONTENTS_USER7			(1<<22)
#define BSP_CONTENTS_USER8			(1<<23)
#define BSP_CONTENTS_USER9			(1<<24)
#define BSP_CONTENTS_USER10			(1<<25)
#define BSP_CONTENTS_USER11			(1<<26)
#define BSP_CONTENTS_USER12			(1<<27)
#define BSP_CONTENTS_USER13			(1<<28)
#define BSP_CONTENTS_USER14			(1<<29)
#define BSP_CONTENTS_USER15			(1<<30)
#define BSP_CONTENTS_USER16			(1<<31)
// 16-31 reserved for user contents


// These contents are all solid types
#define BSP_CONTENTS_SOLID_CLIP		(BSP_CONTENTS_SOLID2 | BSP_CONTENTS_WINDOW2 | BSP_CONTENTS_CLIP2)

// These contents are all visible types
#define BSP_VISIBLE_CONTENTS		(	BSP_CONTENTS_SOLID2 | \
										BSP_CONTENTS_EMPTY2 | \
										BSP_CONTENTS_WINDOW2 | \
										BSP_CONTENTS_SHEET | \
										BSP_CONTENTS_WAVY2)

// These contents define where faces are NOT allowed to merge across
#define BSP_MERGE_SEP_CONTENTS		(	BSP_CONTENTS_WAVY2 | \
										BSP_CONTENTS_HINT2 | \
										BSP_CONTENTS_AREA2)
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=

typedef struct
{
	int32			Type;						// Type of chunk
	int32			Size;						// Size of each element
	int32			Elements;					// Number of elements
} GBSP_Chunk;

typedef struct
{
	int32			Type;
	int32			Size;
	int32			Elements;
	void			*Data;
} GBSP_ChunkData;

typedef struct
{
	char			TAG[5];						// 'G','B','S','P','0'
	int32			Version;
	SYSTEMTIME		BSPTime;					// Time at which the BSP data was created...
} GBSP_Header;

typedef struct
{
	geVec3d			Axis;						// Axis of rotation
	float			Dpm;						// Degres per minute
	int32			Textures[6];				// Texture indexes for all six sides...
	float			DrawScale;
} GFX_SkyData;

typedef struct
{
	geVec3d			Normal;
	geFloat			Dist;
	int32			Type;						// Defined in MathLib.h (PLANE_X, PLANE_Y, etc...)
} GFX_Plane;

typedef struct
{
	int32			Children[2];				// Children, indexed into GFXNodes, < 0 = Leaf
	int32			NumFaces;					// Num faces
	int32			FirstFace;					// First face
	int32			PlaneNum;					// 
	geVec3d			Mins;						// For BBox frustum culling
	geVec3d			Maxs;
} GFX_Node;

typedef struct
{
	int32			Children[2];				// Children, indexed into GFXBNodes, < 0 = Contents
	int32			PlaneNum;					// 
} GFX_BNode;

typedef struct
{
	int32			PlaneNum;
	int32			PlaneSide;
} GFX_LeafSide;

typedef struct
{
	int32	ModelNum;
	int32	Area;
} GFX_AreaPortal;

typedef struct
{
	int32	NumAreaPortals;
	int32	FirstAreaPortal;
} GFX_Area;

typedef struct
{
	int32			Contents;					// Contents of leaf
	geVec3d			Mins;						// For BBox vis testing
	geVec3d			Maxs;
	int32			FirstFace;					// First face in GFXLeafFaces 
	int32			NumFaces;

	int32			FirstPortal;				// Number of portals
	int32			NumPortals;					// First portal

	int32			Cluster;					// CHANGE: CLUSTER
	int32			Area;

	int32			FirstSide;					// Planes (plus bevels) pointing out of leaf
	int32			NumSides;					// (For axial bounding box collisions)
} GFX_Leaf;

typedef struct
{
	int32			VisOfs;
} GFX_Cluster;

typedef struct
{
	int32			FirstVert;					// First vertex indexed in GFXVertices
	int32			NumVerts;					// Number of vertices in face
	int32			PlaneNum;					// PlaneNum 
	int32			PlaneSide;					// 0 = Same direction of plane normal
	int32			TexInfo;
	int32			LightOfs;					// Offset info GFXLightData, -1 = No light
	int32			LWidth;
	int32			LHeight;
	uint8			LTypes[4];
} GFX_Face;

typedef struct
{
	int32			RootNode[2];				// Top level Node in GFXNodes/GFXBNodes
	geVec3d			Mins;
	geVec3d			Maxs;
	geVec3d			Origin;						// Center of model
	int32			FirstFace;					// First face in GFXFaces
	int32			NumFaces;					// Number of faces
	int32			FirstLeaf;					// First leaf in GFXLeafs;
	int32			NumLeafs;					// Number of leafs (not including solid leaf)
	int32			FirstCluster;
	int32			NumClusters;
	int32			Areas[2];					// Area on each side of the model
	geMotion *		Motion;
} GFX_Model;

typedef struct
{
	uint8			RGB[256*3];
} GFX_Palette;

typedef struct
{
	char			Name[32];
	uint32			Flags;
	int32			Width;
	int32			Height;
	int32			Offset;
	int32			PaletteIndex;
} GFX_Texture;

typedef struct
{
	geVec3d			Vecs[2];
	float			Shift[2];
	float			DrawScale[2];
	int32			Flags;
	float			FaceLight;
	float			ReflectiveScale;
	float			Alpha;
	float			MipMapBias;
	int32			Texture;
} GFX_TexInfo;

typedef struct
{
	geVec3d			Origin;						// Center of portal
	int32			LeafTo;						// Leaf looking into
} GFX_Portal;

extern GBSP_Header		GBSPHeader;					// Header
extern GFX_SkyData		GFXSkyData;
extern GFX_Model		*GFXModels;					// Model data
extern GFX_Node			*GFXNodes;					// Nodes
extern GFX_BNode		*GFXBNodes;					// Bevel Clip Nodes
extern GFX_Leaf			*GFXLeafs;					// Leafs
extern GFX_Cluster		*GFXClusters;				// CHANGE: CLUSTER
extern GFX_Area			*GFXAreas;			
extern GFX_AreaPortal	*GFXAreaPortals;
extern GFX_Plane		*GFXPlanes;					// Planes
extern GFX_Face			*GFXFaces;					// Faces
extern int32			*GFXLeafFaces;
extern GFX_LeafSide		*GFXLeafSides;
extern geVec3d			*GFXVerts;					// Verts
extern int32			*GFXVertIndexList;			// Index list
extern geVec3d			*GFXRGBVerts;

extern uint8			*GFXEntData;
extern GFX_Texture		*GFXTextures;				// Textures
extern GFX_TexInfo		*GFXTexInfo;				// TexInfo
extern uint8			*GFXTexData;				// TexData
extern DRV_Palette		*GFXPalettes;				// Palettes

extern uint8			*GFXMotionData;				// Model motion keyframe data

extern uint8			*GFXLightData;				// Lightmap data
extern uint8			*GFXVisData;				// Vis data
extern GFX_Portal		*GFXPortals;				// Portal data

extern int32		NumGFXModels;
extern int32		NumGFXNodes;
extern int32		NumGFXBNodes;
extern int32		NumGFXLeafs;
extern int32		NumGFXClusters;				// CHANGE: CLUSTER
extern int32		NumGFXAreas; 
extern int32		NumGFXAreaPortals;
extern int32		NumGFXPlanes;
extern int32		NumGFXFaces;
extern int32		NumGFXLeafFaces;
extern int32		NumGFXLeafSides;
extern int32		NumGFXVerts;
extern int32		NumGFXVertIndexList;		// For RGB verts, and regular verts
extern int32		NumGFXRGBVerts;

extern int32		NumGFXEntData;
extern int32		NumGFXTextures;
extern int32		NumGFXTexInfo;
extern int32		NumGFXTexData;
extern int32		NumGFXPalettes;

extern int32		NumGFXLightData;
extern int32			NumGFXVisData;
extern int32		NumGFXPortals;

extern int32		NumGFXMotionBytes;

geBoolean LoadGBSPFile(char *FileName);
geBoolean SaveGBSPFile(char *FileName);
geBoolean FreeGBSPFile(void);
geBoolean WriteChunk(GBSP_Chunk *Chunk, void *Data, geVFile *f);

geBoolean WriteChunks(GBSP_ChunkData *Data, int32 NumChunkData, geVFile *f);


#endif