/****************************************************************************************/
/*  Leaf.cpp                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates leaf collision hulls, areas, etc...                            */
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

#include "Ram.h"
#include "Portals.h"
#include "GBSPFile.h"
#include "BSP.h"
#include "Poly.h"
#include "Leaf.h"
#include "Brush2.h"

#include "Vec3d.h"

int32		 NumLeafClusters;

//======================================================================================
//	FillLeafClusters_r
//======================================================================================
geBoolean FillLeafClusters_r(GBSP_Node *Node, int32 Cluster)
{
	if (Node->PlaneNum == PLANENUM_LEAF)
	{
		if ((Node->Contents & BSP_CONTENTS_SOLID2))
			Node->Cluster = -1;
		else
			Node->Cluster = Cluster;

		return GE_TRUE;
	}
	
	Node->Cluster = Cluster;

	FillLeafClusters_r(Node->Children[0], Cluster);
	FillLeafClusters_r(Node->Children[1], Cluster);
	
	return GE_TRUE;	
}

//======================================================================================
//	FindLeafClusters_r
//======================================================================================
geBoolean CreateLeafClusters_r(GBSP_Node *Node)
{
	if (Node->PlaneNum != PLANENUM_LEAF && !Node->Detail)
	{
		CreateLeafClusters_r(Node->Children[0]);
		CreateLeafClusters_r(Node->Children[1]);
		return GE_TRUE;
	}
	
	// Either a leaf or detail node
	if (Node->Contents & BSP_CONTENTS_SOLID2)
	{
		Node->Cluster = -1;
		return GE_TRUE;
	}
	
	if (!FillLeafClusters_r(Node, NumLeafClusters))
		return GE_FALSE;

	NumLeafClusters++;

	return GE_TRUE;
}

//======================================================================================
//	CreateLeafClusters
//======================================================================================
geBoolean CreateLeafClusters(GBSP_Node *RootNode)
{
	GHook.Printf(" --- CreateLeafClusters --- \n");

	if (!CreateLeafClusters_r(RootNode))
	{
		GHook.Error("CreateLeafClusters:  Failed to find leaf clusters.\n");
		return GE_FALSE;
	}

	if (Verbose)
		GHook.Printf("Num Clusters       : %5i\n", NumLeafClusters);

	return GE_TRUE;
}

//======================================================================================
//	Leaf Sides (Used for expandable collision HULL)
//======================================================================================

int32 NumLeafSides;
int32 NumLeafBevels;

GBSP_LeafSide	LeafSides[MAX_LEAF_SIDES];

#define MAX_TEMP_LEAF_SIDES	100

int32 CNumLeafSides;
int32 LPlaneNumbers[MAX_TEMP_LEAF_SIDES];
int32	LPlaneSides[MAX_TEMP_LEAF_SIDES];

//====================================================================================
//	FinishLeafSides
//====================================================================================
geBoolean FinishLeafSides(GBSP_Node *Node)
{
	geVec3d		Mins, Maxs;
	GBSP_Plane	Plane;
	int32		Axis, i, Dir;

	if (!GetLeafBBoxFromPortals(Node, &Mins, &Maxs))
	{
		GHook.Error("FinishLeafSides:  Could not get leaf portal BBox.\n");
		return GE_FALSE;
	}
	
	if (CNumLeafSides < 4)
		GHook.Printf("*WARNING*  FinishLeafSides:  Incomplete leaf volume.\n");

	// Add any bevel planes to the sides so we can expand them for axial box collisions
	else for (Axis=0 ; Axis <3 ; Axis++)
	{
		for (Dir=-1 ; Dir <= 1 ; Dir+=2)
		{
			// See if the plane is allready in the sides
			for (i=0; i< CNumLeafSides; i++)
			{
				Plane = Planes[LPlaneNumbers[i]];
				if (LPlaneSides[i])
				{
					geVec3d_Inverse(&Plane.Normal);
					Plane.Dist = -Plane.Dist;
				}

				if (VectorToSUB(Plane.Normal, Axis) == (geFloat)Dir)
					break;
			}

			if (i >= CNumLeafSides)
			{	
				// Add a new axial aligned side

				geVec3d_Clear(&Plane.Normal);

				VectorToSUB(Plane.Normal, Axis) = (geFloat)Dir;
				
				// get the mins/maxs from the gbsp brush
				if (Dir == 1)
					Plane.Dist = VectorToSUB(Maxs, Axis);
				else
					Plane.Dist = -VectorToSUB(Mins, Axis);
				
				LPlaneNumbers[i] = FindPlane(&Plane, &LPlaneSides[i]);

				if (LPlaneNumbers[i] == -1)
				{
					GHook.Error("FinishLeafSides:  Could not create the plane.\n");
					return GE_FALSE;
				}

				CNumLeafSides++;
				
				NumLeafBevels++;
			}
		}
	}		
	
	Node->FirstSide = NumLeafSides;
	Node->NumSides = CNumLeafSides;
	
	for (i=0; i< CNumLeafSides; i++)
	{
		if (NumLeafSides >= MAX_LEAF_SIDES)
		{
			GHook.Error("FinishLeafSides:  Max Leaf Sides.\n");
			return GE_FALSE;
		}
		
		LeafSides[NumLeafSides].PlaneNum = LPlaneNumbers[i];
		LeafSides[NumLeafSides].PlaneSide = LPlaneSides[i];

		NumLeafSides++;
	}

	return GE_TRUE;
}
		
//====================================================================================
//	CreateLeafSides_r
//====================================================================================
geBoolean CreateLeafSides_r(GBSP_Node *Node)
{
	GBSP_Portal	*Portal, *Next;
	int32		Side, i;

	Node->FirstSide = -1;
	Node->NumSides = 0;

	if (Node->PlaneNum == PLANENUM_LEAF)	// At a leaf, convert portals to leaf sides...
	{
		if (!(Node->Contents & BSP_CONTENTS_SOLID_CLIP))		// Don't convert empty leafs
			return GE_TRUE;				

		if (!Node->Portals)
		{
			GHook.Printf("*WARNING* CreateLeafSides:  Contents leaf with no portals!\n");
			return GE_TRUE;
		}

		// Reset number of sides for this solid leaf (should we save out other contents?)
		//	(this is just for a collision hull for now...)
		CNumLeafSides = 0;

		for (Portal = Node->Portals; Portal; Portal = Next)
		{
			Side = (Portal->Nodes[0] == Node);
			Next = Portal->Next[!Side];

			for (i=0; i< CNumLeafSides; i++)
			{
				if (LPlaneNumbers[i] == Portal->PlaneNum && LPlaneSides[i] == Side)
					break;
			}

			// Make sure we don't duplicate planes (this happens with portals)
			if (i >= MAX_TEMP_LEAF_SIDES)
			{
				GHook.Error("CreateLeafSides_r:  Max portal leaf sides.\n");
				return GE_FALSE;
			}

			if (i >= CNumLeafSides)
			{
				LPlaneNumbers[i] = Portal->PlaneNum;
				LPlaneSides[i] = Side;
				CNumLeafSides++;
			}
			
		}
		
		if (!FinishLeafSides(Node))
		{
			return GE_FALSE;
		}

		return GE_TRUE;
	}

	if (!CreateLeafSides_r(Node->Children[0]))
		return GE_FALSE;

	if (!CreateLeafSides_r(Node->Children[1]))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//	CreateLeafSides
//	Creates leaf sides with bevel edges for axial bounding box clipping hull
//====================================================================================
geBoolean CreateLeafSides(GBSP_Node *RootNode)
{
	if (Verbose)
		GHook.Printf(" --- Create Leaf Sides --- \n");
	
	if (!CreateLeafSides_r(RootNode))
		return GE_FALSE;

	if (Verbose)
	{
		GHook.Printf("Num Leaf Sides       : %5i\n", NumLeafSides);
		GHook.Printf("Num Leaf Bevels      : %5i\n", NumLeafBevels);
	}

	return GE_TRUE;
}

//=====================================================================================
//			 *** AREA LEAFS ***
//=====================================================================================

#define	MAX_AREAS			256
#define	MAX_AREA_PORTALS	1024

//=====================================================================================
//	ModelForLeafNode
//=====================================================================================
GBSP_Model *ModelForLeafNode(GBSP_Node *Node)
{
	GBSP_Brush		*Brush;

	if (Node->PlaneNum != PLANENUM_LEAF)
	{
		GHook.Error("ModelForLeafNode:  Node not a leaf!\n");
		return NULL;
	}

	for (Brush = Node->BrushList; Brush; Brush = Brush->Next)
	{
		if (Brush->Original->EntityNum)
			break;
	}

	if (!Brush)
		return NULL;

	return &BSPModels[Entities[Brush->Original->EntityNum].ModelNum];
}

//=====================================================================================
//	FillAreas_r
//=====================================================================================
geBoolean FillAreas_r(GBSP_Node *Node, int32 Area)
{
	GBSP_Portal		*Portal;
	int32			Side;

	if ((Node->Contents & BSP_CONTENTS_SOLID2))
		return GE_TRUE;			// Stop at solid leafs

	if ((Node->Contents & BSP_CONTENTS_AREA2))
	{
		GBSP_Model		*Model;

		Model = ModelForLeafNode(Node);
		
		if (!Model)
		{
			GHook.Error("FillAreas_r:  No model for leaf.\n");
			return GE_FALSE;
		}

		if (Model->Areas[0] == Area || Model->Areas[1] == Area)
			return GE_TRUE;	// Already flooded into this portal from this area

		if (!Model->Areas[0])
			Model->Areas[0] = Area;
		else if (!Model->Areas[1])
			Model->Areas[1] = Area;
		else
			GHook.Printf("*WARNING* FillAreas_r:  Area Portal touched more than 2 areas.\n");

		return GE_TRUE;		
	}

	if (Node->Area != 0)		// Already set
		return GE_TRUE;

	// Mark it
	Node->Area = Area;

	// Flood through all of this leafs portals
	for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
	{
		Side = (Portal->Nodes[1] == Node);
		
		if (!FillAreas_r(Portal->Nodes[!Side], Area))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	CreateAreas_r
//=====================================================================================
geBoolean CreateAreas_r(GBSP_Node *Node)
{
	if (Node->PlaneNum == PLANENUM_LEAF)
	{
		if (Node->Contents & BSP_CONTENTS_SOLID2)	// Stop at solid
			return GE_TRUE;

		if (Node->Contents & BSP_CONTENTS_AREA2)	// Don't start at area portals
			return GE_TRUE;

		if (Node->Area != 0)						// Already set
			return GE_TRUE;

		if (NumGFXAreas >= MAX_AREAS)
		{
			GHook.Error("CreateAreas_r:  Max Areas.\n");
			return GE_FALSE;
		}

		// Once we find a normal leaf, flood out marking the current area
		// stopping at other areas leafs, and solid leafs (unpassable leafs)
		if (!FillAreas_r(Node, NumGFXAreas++))
			return GE_FALSE;

		return GE_TRUE;
	}

	if (!CreateAreas_r(Node->Children[0]))
		return GE_FALSE;
	if (!CreateAreas_r(Node->Children[1]))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	FinishAreaPortals_r
//=====================================================================================
geBoolean FinishAreaPortals_r(GBSP_Node *Node)
{
	GBSP_Model		*Model;

	if (Node->PlaneNum != PLANENUM_LEAF)
	{
		if (!FinishAreaPortals_r(Node->Children[0]))
			return GE_FALSE;
		if (!FinishAreaPortals_r(Node->Children[1]))
			return GE_FALSE;
	}

	if (!(Node->Contents & BSP_CONTENTS_AREA2))
		return GE_TRUE;		// Only interested in area portals

	if (Node->Area)
		return GE_TRUE;		// Already set...

	Model = ModelForLeafNode(Node);

	if (!Model)
	{
		GHook.Error("FinishAreaPortals_r:  No model for leaf.\n");
		return GE_FALSE;
	}

	Node->Area = Model->Areas[0];		// Set to first area that flooded into portal
	Model->IsAreaPortal = GE_TRUE;
	
	return GE_TRUE;
}

//=====================================================================================
//	FinishAreas
//=====================================================================================
geBoolean FinishAreas(void)
{
	int32		i;
	GFX_Area	*a;
	GBSP_Model	*pModel;
	int32		m;

	// First, go through and print out all errors pertaining to model areas
	for (pModel = BSPModels+1, m=1; m < NumBSPModels; m++, pModel++)	// Skip world model
	{
		if (!pModel->IsAreaPortal)
			continue;		// Not an area portal model

		if (!pModel->Areas[0])
			GHook.Printf("*WARNING* FinishAreas:  AreaPortal did not touch any areas!\n");
		else if (!pModel->Areas[1])
			GHook.Printf("*WARNING* FinishAreas:  AreaPortal only touched one area.\n");
	}

	NumGFXAreaPortals = 0;		// Clear the Portals var

	// Area 0 is the invalid area, set it here, and skip it in the loop below
	GFXAreas[0].FirstAreaPortal = 0;
	GFXAreas[0].NumAreaPortals = 0;

	for (a = GFXAreas+1, i = 1; i < NumGFXAreas; i++, a++)		// Skip invalid area (area 0)
	{
		a->FirstAreaPortal = NumGFXAreaPortals;

		// Go through all the area portals (models), and create all portals that belong to this area
		for (pModel = BSPModels+1, m=1; m < NumBSPModels; m++, pModel++)	// Skip world model
		{
			int32			a0, a1;
			GFX_AreaPortal	*p;

			a0 = pModel->Areas[0];
			a1 = pModel->Areas[1];

			if (!a0 || !a1)
				continue;				// Model didn't seperate 2 areas

			if (a0 == a1)				// Portal seperates same area
				continue;

			if (a0 != i && a1 != i)
				continue;				// Portal is not a part of this area

			if (NumGFXAreaPortals >= MAX_AREA_PORTALS)
			{
				GHook.Error("FinishAreas:  Max area portals.\n");
				return GE_FALSE;		// Oops
			}
			
			p = &GFXAreaPortals[NumGFXAreaPortals];

			if (a0 == i)		// Grab the area on the opposite side of the portal
				p->Area = a1;
			else if (a1 == i)
				p->Area = a0;

			p->ModelNum = m;	// Set the portals model number
			NumGFXAreaPortals++;
		}
		a->NumAreaPortals = NumGFXAreaPortals - a->FirstAreaPortal;
	}

	return GE_TRUE;
}

//=====================================================================================
//	CreateAreas
//=====================================================================================
geBoolean CreateAreas(GBSP_Node *RootNode)
{
	GBSP_Model	*pModel;
	int32		m;

	GHook.Printf(" --- Create Area Leafs --- \n");

	// Clear all model area info
	for (pModel = BSPModels+1, m=1; m < NumBSPModels; m++, pModel++)	// Skip world model
	{
		pModel->Areas[0] = pModel->Areas[1] = 0;
		pModel->IsAreaPortal = GE_FALSE;
	}

	if (GFXAreas)
		geRam_Free(GFXAreas);
	if (GFXAreaPortals)
		geRam_Free(GFXAreaPortals);

	GFXAreas = GE_RAM_ALLOCATE_ARRAY(GFX_Area, MAX_AREAS);

	if (!GFXAreas)
		return GE_FALSE;

	GFXAreaPortals = GE_RAM_ALLOCATE_ARRAY(GFX_AreaPortal, MAX_AREA_PORTALS);

	if (!GFXAreaPortals)
		return GE_FALSE;
	
	NumGFXAreas = 1;			// 0 is invalid 
	NumGFXAreaPortals = 0;

	if (!CreateAreas_r(RootNode))
	{
		GHook.Error("Could not create model areas.\n");
		return GE_FALSE;
	}

	if (!FinishAreaPortals_r(RootNode))
	{
		GHook.Error("CreateAreas: FinishAreaPortals_r failed.\n");
		return GE_FALSE;
	}

	if (!FinishAreas())
	{
		GHook.Error("Could not finalize model areas.\n");
		return GE_FALSE;
	}

	//GHook.Printf("Num Areas            : %5i\n", NumGFXAreas-1);

	return GE_TRUE;
}

//=====================================================================================
//	FindLeaf
//=====================================================================================
GBSP_Node *FindLeaf(GBSP_Node *Node, geVec3d *Origin)
{
	GBSP_Plane	*Plane;
	geFloat		Dist;

	while(Node && Node->PlaneNum != PLANENUM_LEAF)
	{
		Plane = &Planes[Node->PlaneNum];
		Dist = geVec3d_DotProduct(Origin, &Plane->Normal) - Plane->Dist;
		if (Dist > 0)
			Node = Node->Children[0];
		else
			Node = Node->Children[1];
	}

	if (!Node)
	{
		GHook.Error("FindLeaf:  NULL Node/Leaf.\n");
		return NULL;
	}

	return Node;
}
