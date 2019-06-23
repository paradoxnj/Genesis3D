/****************************************************************************************/
/*  GbspLib.h                                                                           */
/*                                                                                      */
/*  Author:       John Pollard                                                          */
/*  Description:  Genesis world editor header file                                      */
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
#ifndef GBSPLIB_H
#define GBSPLIB_H

#include <Windows.h>
#include "vec3d.h"
#include "basetype.h"

//#include "MyDef.h"
//#include "MathLib.h"

//#include "Motion.h"

#define GBSP_VERSION_MAJOR	6
#define GBSP_VERSION_MINOR	0
/*
//====================================================================================
//	Global defines
//====================================================================================
#define PLANENUM_LEAF		-1
#define TEXINFO_NODE		-1


#define MAX_BSP_PLANES		32000

#define	DIST_EPSILON	(VecType)0.01
#define	ANGLE_EPSILON	(VecType)0.00001

//====================================================================================
//	BSP sructures
//====================================================================================

struct _GBSP_Portal;

typedef struct GBSP_Brush2	GBSP_Brush2;
typedef struct GBSP_Side	GBSP_Side;

typedef struct
{
	S32			NumVerts;
	Vec3d		*Verts;
} GBSP_Poly;


typedef struct _GBSP_Face
{
	_GBSP_Face	*Next;
	_GBSP_Face	*Original;
	GBSP_Poly	*Poly;
	S32			Contents[2];
	S32			TexInfo;
	S32			PlaneNum;
	S32			PlaneSide;

	S32			Entity;					// Originating entity

	U8			Detail;					// 1 if was on detail brush
	U8			Hint;
	U8			Visible;			

	// For GFX file saving
	S32			OutputNum;	

	S32			*IndexVerts;
	S32			FirstIndexVert;
	S32			NumIndexVerts;

	_GBSP_Portal *Portal;
	_GBSP_Face	*Split[2];
	_GBSP_Face	*Merged;
} GBSP_Face;

typedef struct
{
	Vec3d		Normal;
	VecType		Dist;
	S32			Type;
} GBSP_Plane;

typedef struct
{
	S32		Contents;					// Contents of leaf
} GBSP_Leaf;

typedef struct _GBSP_Node
{
	// Info for this node as a node or leaf
	S32				PlaneNum;			// -1 if a leaf
	S32				PlaneSide;			// CHANGE1!!!
	S32				Contents;			// Contents node/leaf
	GBSP_Face		*Faces;				// Faces on this node
	_GBSP_Node		*Children[2];		// Front and back child
	_GBSP_Node		*Parent;			// Parent of this node
	Vec3d			Mins;				// Current BBox of node
	Vec3d			Maxs;

	// Info for this node as a leaf
	_GBSP_Portal	*Portals;			// Portals on this leaf
	S32				NumMarkFaces;		// Number of faces touching this leaf
	GBSP_Face		**MarkFaces;		// Pointer to Faces touching this leaf
	S32				CurrentFill;		// For the outside filling stage
	S32				Entity;				// 1 if entity touching leaf
	S32				Occupied;			// FIXME:  Can use Entity!!!
	S32				PortalLeafNum;		// For portal saving

	BOOL			Detail;
	S32				Cluster;
	S32				Area;				// Area number, -1 if area portal, 0 if solid

	GBSP_Brush2		*Volume;
	GBSP_Side		*Side;
	GBSP_Brush2		*BrushList;

	// For GFX file saving
	S32				ChildrenID[2];
	S32				FirstFace;
	S32				NumFaces;
	S32				FirstPortal;
	S32				NumPortals;

	S32				FirstSide;			// For bevel bbox clipping
	S32				NumSides;

} GBSP_Node;

typedef struct
{
	S32		PlaneNum;
	S32		PlaneSide;
} GBSP_LeafSide;

typedef struct _GBSP_Node2
{
	_GBSP_Node2		*Children[2];
	S32				PlaneNum;			// -1 == Leaf
	
	// For leafs
	S32				Contents;
} GBSP_Node2;

// Side flags...
#define SIDE_HINT	(1<<0)

typedef struct GBSP_Side
{
	GBSP_Poly	*Poly;

	S32			PlaneNum;
	U8			PlaneSide;

	S32			TexInfo;

	U8			Flags;

	U8			Visible;
	U8			Tested;
	//U8			Bevel;

} GBSP_Side;

typedef struct _GBSP_Portal
{
	GBSP_Poly		*Poly;				// Convex poly that holds the shape of the portal
	GBSP_Node		*Nodes[2];			// Node on each side of the portal
	_GBSP_Portal	*Next[2];			// Next portal for each node
	S32				PlaneNum;

	GBSP_Node		*OnNode;
	GBSP_Face		*Face[2];
	GBSP_Side		*Side;
	U8				SideFound;
} GBSP_Portal;

typedef struct MAP_Brush2	MAP_Brush2;

typedef struct GBSP_Brush2
{
	GBSP_Brush2	*Next;
	Vec3d		Mins, Maxs;
	S32			Side, TestSide;		// Side of node during construction
	MAP_Brush2	*Original;
	S32			NumSides;
	GBSP_Side	Sides[6];			// Variably sized

} GBSP_Brush2;

typedef struct
{
	GBSP_Node	*RootNode[2];		// 0 = DrawHull, 1 = Bevel ClipHull

	Vec3d		Origin;

	GBSP_Node	OutsideNode;		// So each model can have it's own outside node

	S32			TempAreas[5];		// Areas that model touches
	S32			NumTempAreas;
	// For GFX File saving
	S32			RootNodeID[2];
	S32			FirstFace;
	S32			NumFaces;
	S32			FirstLeaf;
	S32			NumLeafs;
	S32			FirstCluster;
	S32			NumClusters;
	S32			FirstArea;
	S32			NumAreas;
	S32			NumSolidLeafs;		// So we can skip over solid leafs for vis stuff

} GBSP_Model;
*/
//====================================================================================
//	Main driver interfaces
//====================================================================================
#define DllImport	extern "C" __declspec( dllimport )
#define DllExport	extern "C" __declspec( dllexport )

typedef void ERROR_CB(char *String, ...);
typedef void PRINTF_CB(char *String, ...);

typedef struct
{
	BOOL	Verbose;
	BOOL	EntityVerbose;

} BspParms;

typedef struct
{
	BOOL	Verbose;
	BOOL	ExtraSamples;
	float	LightScale;
	BOOL	Radiosity;
	int		NumBounce;
	float	PatchSize;
	BOOL	FastPatch;
	float	ReflectiveScale;

	geVec3d	MinLight;			// R,G,B (XYZ) min color for each faces lightmap

} LightParms;

typedef struct
{
	BOOL	Verbose;
	BOOL	FullVis;
	BOOL	SortPortals;

} VisParms;

extern BOOL		CancelRequest;		// Global cancel request...

typedef enum
{
	GBSP_ERROR,
	GBSP_OK,
	GBSP_CANCEL
} GBSP_RETVAL;

typedef GBSP_RETVAL GBSP_CREATE_BSP(char *MapText, BspParms *Parms);
typedef GBSP_RETVAL GBSP_S_FILE(char *FileName);
typedef void GBSP_FREE_BSP(void);
typedef GBSP_RETVAL GBSP_VIS_FILE(char *FileName, VisParms *Parms);
typedef GBSP_RETVAL GBSP_LIGHT_FILE(char *FileName, LightParms *Parms);
typedef BOOL GBSP_CANCEL_COMPILE(void);
typedef BOOL GBSP_UPDATE_ENTITIES(char *MapName, char *BSPName);

typedef struct
{
	ERROR_CB		*Error;			// This must be set by caller
	PRINTF_CB		*Printf;		// This too

} GBSP_Hook;

typedef struct
{
	int						VersionMajor;			// Major changes, like function parms...
	int						VersionMinor;			// Internal changes, that do not effect caller
	GBSP_CREATE_BSP			*GBSP_CreateBSP;
	GBSP_S_FILE				*GBSP_SaveGBSPFile;
	GBSP_FREE_BSP			*GBSP_FreeBSP;
	GBSP_VIS_FILE			*GBSP_VisGBSPFile;
	GBSP_LIGHT_FILE			*GBSP_LightGBSPFile;
	GBSP_CANCEL_COMPILE		*GBSP_Cancel;
	GBSP_UPDATE_ENTITIES	*GBSP_UpdateEntities;
} GBSP_FuncHook;

typedef GBSP_FuncHook *GBSP_INIT(GBSP_Hook *Hook);

#endif


