/****************************************************************************************/
/*  BSP.cpp                                                                             */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <Stdio.h>
#include <Math.h>

#include "GBSPPrep.h"
#include "BSP.h"
#include "Map.h"
#include "Poly.h"
#include "Portals.h"
#include "Brush2.h"
#include "Vis.h"
#include "Leaf.h"
#include "Fill.h"
#include "Utils.h"
#include "Light.h"
#include "GBSPFile.h"

#include "Ram.h"

// Globals
GBSP_Model	BSPModels[MAX_BSP_MODELS];
int32		NumBSPModels;

geBoolean	Verbose = GE_TRUE;
geBoolean	OriginalVerbose;
geBoolean	EntityVerbose = GE_FALSE;

//
// BSP2.cpp defs
//
geBoolean	ProcessEntities(void);				
void		FreeBSP_r(GBSP_Node *Node);	

//=======================================================================================
//	InsertModelNumbers
//=======================================================================================
geBoolean InsertModelNumbers(void)
{
	int32	i, NumModels;
	char	ModelNum[128];

	NumModels = 0;

	for (i=0; i< NumEntities; i++)
	{
		if (CancelRequest)
			return GE_FALSE;
		
		if (!Entities[i].Brushes2)		// No model if no brushes
			continue;
		
		Entities[i].ModelNum = NumModels;

		if (i != 0)
		{
			sprintf(ModelNum, "%i", NumModels);
			SetKeyValue(&Entities[i], "Model", ModelNum);
		}
		NumModels++;
	}
	return GE_TRUE;
}

//========================================================================================
//	CreateBSP
//========================================================================================
geBoolean CreateBSP(char *FileName, BspParms *Parms)
{
	OriginalVerbose = Verbose = Parms->Verbose;
	EntityVerbose = Parms->EntityVerbose;
		
	gCountVerts = GE_TRUE;

	NumLeafSides = 0;
	NumLeafClusters = 0;
	NumLeafBevels = 0;
	NumPlanes = 0;

	if (!LoadBrushFile(FileName))
	{
		FreeAllEntities();
		return GE_FALSE;
	}
	
	InsertModelNumbers();

	BeginGBSPModels();
	
	if (!ProcessEntities())
	{
		FreeAllGBSPData();
		FreeAllEntities();
		return GE_FALSE;
	}

	return GE_TRUE;
}

//========================================================================================
//	UpdateEntities
//	Updates the entities only...
//========================================================================================
geBoolean UpdateEntities(char *MapName, char *BSPName)
{
	GHook.Printf("--- Update Entities --- \n");

	if (!LoadBrushFile(MapName))
		goto ExitWithError;

	InsertModelNumbers();

	// Load the old .BSP
	if (!LoadGBSPFile(BSPName))
	{
		GHook.Error("UpdateEntities:  Could not load .bsp file.\n");
		goto ExitWithError;
	}

	// Destroy any old GFXEntData in the .BSP file
	if (GFXEntData)
	{
		geRam_Free(GFXEntData);
		GFXEntData = NULL;
	}
	NumGFXEntData = 0;

	if (!ConvertEntitiesToGFXEntData())
	{
		GHook.Error("UpdateEntities:  ConvertEntitiesToGFXEntData failed.\n");
		return GE_FALSE;
	}

	// Save it!!!
	if (!SaveGBSPFile(BSPName))
	{
		GHook.Error("UpdateEntities:  SaveGBSPFile failed.\n");
		goto ExitWithError;
	}

	FreeAllEntities();					// Free any entities that might be left behind
	FreeGBSPFile();						// Free any GFX BSP data left over
	return GE_TRUE;
	
	ExitWithError:
		FreeAllEntities();				// Free any entities that might be left behind
		FreeGBSPFile();					// Free any GFX BSP data left over
		return GE_FALSE;
}

//========================================================================================
//	AllocNode
//========================================================================================
GBSP_Node *AllocNode(void)
{
	GBSP_Node	*Node;

	Node = GE_RAM_ALLOCATE_STRUCT(GBSP_Node);

	if (Node == NULL)
	{
		GHook.Error("AllocNode:  Out of memory!\n");
		return NULL;
	}

	memset(Node, 0, sizeof(GBSP_Node));

	return Node;
}

//========================================================================================
//	FreeNode
//========================================================================================
void FreeNode(GBSP_Node *Node)
{
	GBSP_Face	*Face, *Next;
	GBSP_Brush	*Brush, *NextB;

	if (Node->LeafFaces)
		geRam_Free(Node->LeafFaces);

	for (Face = Node->Faces; Face; Face = Next)
	{
		Next = Face->Next;
		FreeFace(Face);
	}

	for (Brush = Node->BrushList; Brush; Brush = NextB)
	{
		NextB = Brush->Next;

		FreeBrush(Brush);
	}

	geRam_Free(Node);
}

//========================================================================================
//	FreeAllGBSPData
//========================================================================================
geBoolean FreeAllGBSPData(void)
{
	int32	i;

	for (i=0; i< NumBSPModels; i++)
	{
		if (!FreePortals(BSPModels[i].RootNode[0]))
		{
			GHook.Error("FreeAllGBSPData:  Could not free portals.\n");
			return GE_FALSE;
		}
		FreeBSP_r(BSPModels[i].RootNode[0]);
		
		memset(&BSPModels[i], 0, sizeof(GBSP_Model));
	}

	return GE_TRUE;
}

//========================================================================================
//	CleanupGBSP
//========================================================================================
void CleanupGBSP(void)
{
	FreeAllGBSPData();
	FreeAllEntities();
}

//
//	Planes
//

GBSP_Plane	Planes[MAX_BSP_PLANES];
int32		NumPlanes = 0;

//====================================================================================
//	PlaneFromVerts
//	Expects at least 3 verts
//====================================================================================
void PlaneFromVerts(geVec3d *Verts, GBSP_Plane *Plane)
{
	geVec3d		Vect1, Vect2;
	
	geVec3d_Subtract(&Verts[0], &Verts[1], &Vect1);
	geVec3d_Subtract(&Verts[2], &Verts[1], &Vect2);
	
	geVec3d_CrossProduct(&Vect1, &Vect2, &Plane->Normal);
	geVec3d_Normalize(&Plane->Normal);

	Plane->Dist = geVec3d_DotProduct(&Verts[0], &Plane->Normal);

	Plane->Type = geVec3d_PlaneType(&Plane->Normal);
}

//====================================================================================
//	SidePlane
//====================================================================================
void SidePlane(GBSP_Plane *Plane, int32 *Side)
{
	int32		Type;

	*Side = 0;					// Default to same direction

	Plane->Type = geVec3d_PlaneType(&Plane->Normal);

	Type = Plane->Type % PLANE_ANYX;

	// Point the major axis in the positive direction
	if (VectorToSUB(Plane->Normal, Type) < 0)
	{
		geVec3d_Inverse(&Plane->Normal);
		Plane->Dist = -Plane->Dist;
		*Side = 1;									// Flip if direction is opposite match
	}
}

#define NORMAL_EPSILON		(geFloat)0.00001

//====================================================================================
//	PlaneEqual
//====================================================================================
geBoolean PlaneEqual(GBSP_Plane *Plane1, GBSP_Plane *Plane2)
{
	if (fabs(Plane1->Normal.X - Plane2->Normal.X) < NORMAL_EPSILON && 
		fabs(Plane1->Normal.Y - Plane2->Normal.Y) < NORMAL_EPSILON && 
		fabs(Plane1->Normal.Z - Plane2->Normal.Z) < NORMAL_EPSILON && 
		fabs(Plane1->Dist - Plane2->Dist) < DIST_EPSILON )
			return GE_TRUE;

	return GE_FALSE;
}

//====================================================================================
//	SnapVector
//====================================================================================
void SnapVector(geVec3d *Normal)
{
	int		i;

	for (i=0 ; i<3 ; i++)
	{
		if ( fabs(VectorToSUB(*Normal,i) - (geFloat)1) < ANGLE_EPSILON )
		{
			geVec3d_Clear(Normal);
			VectorToSUB(*Normal,i) = (geFloat)1;
			break;
		}

		if ( fabs(VectorToSUB(*Normal,i) - (geFloat)-1) < ANGLE_EPSILON )
		{
			geVec3d_Clear(Normal);
			VectorToSUB(*Normal,i) = (geFloat)-1;
			break;
		}
	}
}

//====================================================================================
//	RoundInt
//====================================================================================
geFloat RoundInt(geFloat In)
{
	return (geFloat)floor(In + (geFloat)0.5);
}

//====================================================================================
//	SnapPlane
//====================================================================================
void SnapPlane(geVec3d *Normal, geFloat *Dist)
{
	SnapVector (Normal);

	if (fabs(*Dist-RoundInt(*Dist)) < DIST_EPSILON)
		*Dist = RoundInt(*Dist);
}


//=======================================================================================
//	PlaneInverse
//=======================================================================================
void PlaneInverse(GBSP_Plane *Plane)
{
	geVec3d_Inverse(&Plane->Normal);
	Plane->Dist = -Plane->Dist;

}

//====================================================================================
//	FindPlane
//====================================================================================
int32 FindPlane(GBSP_Plane *Plane, int32 *Side)
{
	GBSP_Plane	Plane1;
	GBSP_Plane	*Plane2;
	//geFloat		Dot;
	int32		i;

	SnapPlane(&Plane->Normal, &Plane->Dist);

	Plane1 = *Plane;

	SidePlane(&Plane1, Side);		// Find axis, and flip if necessary, to make major axis positive

	Plane2 = Planes;	

	for (i=0; i< NumPlanes; i++)	// Try to return a plane allready in the list
	{
		if (PlaneEqual(&Plane1, Plane2))
			return i;

		Plane2++;
	}

	if (NumPlanes >= MAX_BSP_PLANES)
	{
		GHook.Error("Max BSP Planes.\n");
		return -1;
	}
	
	Planes[NumPlanes++] = Plane1;

	return i;
}

//=======================================================================================
//=======================================================================================
float _fastcall Plane_PointDistanceFast(GBSP_Plane *Plane, geVec3d *Point)
{
   float	Dist,Dist2;

   Dist2 = Plane->Dist;

   switch (Plane->Type)
   {
	   
	   case PLANE_X:
           Dist = (Point->X - Dist2);
           break;
	   case PLANE_Y:
           Dist = (Point->Y - Dist2);
           break;
	   case PLANE_Z:
           Dist = (Point->Z - Dist2);
           break;
	      
       default:
           Dist = geVec3d_DotProduct(Point, &Plane->Normal) - Dist2;
           break;
    }

    return Dist;
}
