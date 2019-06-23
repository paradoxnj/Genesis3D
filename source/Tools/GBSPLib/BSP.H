/****************************************************************************************/
/*  BSP.h                                                                               */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Module distributes code to all the other modules                       */
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
#ifndef BSP_H
#define BSP_H

#include <Windows.h>

#include "GBSPLib.h"

//====================================================================================
//	Global defines
//====================================================================================
#define MAX_BSP_MODELS			2048

#define	PSIDE_FRONT				1
#define	PSIDE_BACK				2
#define	PSIDE_BOTH				(PSIDE_FRONT|PSIDE_BACK)
#define	PSIDE_FACING			4

#define PLANENUM_LEAF			-1

#define MAX_BSP_PLANES			32000

#define	DIST_EPSILON			(geFloat)0.01
#define	ANGLE_EPSILON			(geFloat)0.00001

//====================================================================================
//	Global sructures
//====================================================================================

struct _GBSP_Portal;

typedef struct GBSP_Brush	GBSP_Brush;
typedef struct GBSP_Side	GBSP_Side;

typedef struct
{
	int32		NumVerts;
	geVec3d		*Verts;
} GBSP_Poly;


typedef struct _GBSP_Face
{
	_GBSP_Face	*Next;
	_GBSP_Face	*Original;
	GBSP_Poly	*Poly;
	int32		Contents[2];
	int32		TexInfo;
	int32		PlaneNum;
	int32		PlaneSide;

	int32		Entity;					// Originating entity

	uint8		Visible;			

	// For GFX file saving
	int32		OutputNum;	

	int32		*IndexVerts;
	int32		FirstIndexVert;
	int32		NumIndexVerts;

	_GBSP_Portal *Portal;
	_GBSP_Face	*Split[2];
	_GBSP_Face	*Merged;
} GBSP_Face;

typedef struct
{
	geVec3d		Normal;
	geFloat		Dist;
	int32		Type;
} GBSP_Plane;

typedef struct
{
	int32	Contents;					// Contents of leaf
} GBSP_Leaf;

typedef struct _GBSP_Node
{
	// Info for this node as a node or leaf
	int32			PlaneNum;			// -1 if a leaf
	int32			PlaneSide;			// CHANGE1!!!
	int32			Contents;			// Contents node/leaf
	GBSP_Face		*Faces;				// Faces on this node
	_GBSP_Node		*Children[2];		// Front and back child
	_GBSP_Node		*Parent;			// Parent of this node
	geVec3d			Mins;				// Current BBox of node
	geVec3d			Maxs;

	// Info for this node as a leaf
	_GBSP_Portal	*Portals;			// Portals on this leaf
	int32			NumLeafFaces;		// Number of faces touching this leaf
	GBSP_Face		**LeafFaces;		// Pointer to Faces touching this leaf
	int32			CurrentFill;		// For the outside filling stage
	int32			Entity;				// 1 if entity touching leaf
	int32			Occupied;			// FIXME:  Can use Entity!!!
	int32			PortalLeafNum;		// For portal saving

	geBoolean		Detail;
	int32			Cluster;
	int32			Area;				// Area number, 0 == invalid area

	GBSP_Brush		*Volume;
	GBSP_Side		*Side;
	GBSP_Brush		*BrushList;

	// For GFX file saving
	int32			ChildrenID[2];
	int32			FirstFace;
	int32			NumFaces;
	int32			FirstPortal;
	int32			NumPortals;

	int32			FirstSide;			// For bevel bbox clipping
	int32			NumSides;

} GBSP_Node;

typedef struct
{
	int32	PlaneNum;
	int32	PlaneSide;
} GBSP_LeafSide;

typedef struct _GBSP_Node2
{
	_GBSP_Node2		*Children[2];
	int32			PlaneNum;			// -1 == Leaf
	
	// For leafs
	int32			Contents;
} GBSP_Node2;

// Side flags...
#define SIDE_HINT		(1<<0)		// Side is a hint side
#define SIDE_SHEET		(1<<1)		// Side is a sheet (only visible face in a sheet contents)
#define SIDE_VISIBLE	(1<<2)		// 
#define SIDE_TESTED		(1<<3)		// 
#define SIDE_NODE		(1<<4)		// 

typedef struct GBSP_Side
{
	GBSP_Poly	*Poly;

	int32		PlaneNum;
	uint8		PlaneSide;

	int32		TexInfo;

	uint8		Flags;
} GBSP_Side;

typedef struct _GBSP_Portal
{
	GBSP_Poly		*Poly;				// Convex poly that holds the shape of the portal
	GBSP_Node		*Nodes[2];			// Node on each side of the portal
	_GBSP_Portal	*Next[2];			// Next portal for each node
	int32			PlaneNum;

	GBSP_Node		*OnNode;
	GBSP_Face		*Face[2];
	GBSP_Side		*Side;
	uint8			SideFound;
} GBSP_Portal;

typedef struct MAP_Brush	MAP_Brush;

typedef struct
{
	GBSP_Node	*RootNode[2];		// 0 = DrawHull, 1 = Bevel ClipHull

	geVec3d		Origin;

	GBSP_Node	OutsideNode;		// So each model can have it's own outside node

	geVec3d		Mins;
	geVec3d		Maxs;

	// For GFX File saving
	int32		RootNodeID[2];
	int32		FirstFace;
	int32		NumFaces;
	int32		FirstLeaf;
	int32		NumLeafs;
	int32		FirstCluster;
	int32		NumClusters;
	int32		NumSolidLeafs;		// So we can skip over solid leafs for vis stuff

	// Temorary area portal stuff
	geBoolean	IsAreaPortal;
	int32		Areas[2];			// Used when this model is an area sperator (AreaPortal)

} GBSP_Model;

//====================================================================================
//	Globals
//====================================================================================
extern GBSP_Model	BSPModels[MAX_BSP_MODELS];
extern int32		NumBSPModels;

extern geBoolean	Verbose;
extern geBoolean	OriginalVerbose;
extern geBoolean	EntityVerbose;

#define MAX_WELDED_VERTS			64000

// Defined in TJunct.cpp
extern int32		NumWeldedVerts;
extern int32		TotalIndexVerts;
extern geVec3d		WeldedVerts[MAX_WELDED_VERTS];

geBoolean			FixModelTJunctions(void);

//====================================================================================
//	Global functions
//====================================================================================
geBoolean	CreateBSP(char *FileName, BspParms *Parms);	
geBoolean	UpdateEntities(char *MapName, char *BSPName);

GBSP_Node	*AllocNode(void);
void		FreeNode(GBSP_Node *Node);

geBoolean	FreeAllGBSPData(void);
void		CleanupGBSP(void);

//
//	Planes
//

extern GBSP_Plane	Planes[MAX_BSP_PLANES];
extern int32		NumPlanes;

void		PlaneFromVerts(geVec3d *Verts, GBSP_Plane *Plane);
void		SidePlane(GBSP_Plane *Plane, int32 *Side);
geBoolean	PlaneEqual(GBSP_Plane *Plane1, GBSP_Plane *Plane2);
geBoolean	PlaneCompare(GBSP_Plane *Plane1, GBSP_Plane *Plane2);
void		SnapVector(geVec3d *Normal);
geFloat		RoundInt(geFloat In);
void		SnapPlane(geVec3d *Normal, geFloat *Dist);
void		PlaneInverse(GBSP_Plane *Plane);
int32		FindPlane(GBSP_Plane *Plane, int32 *Side);
void		PlaneInverse(GBSP_Plane *Plane);
float		_fastcall Plane_PointDistanceFast(GBSP_Plane *Plane, geVec3d *Point);

#endif
