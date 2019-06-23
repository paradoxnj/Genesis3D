/****************************************************************************************/
/*  BSP2.cpp                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to actually build the tree                                        */
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
#include <Assert.h>
#include <Math.h>

#include "Mathlib.h"
#include "Poly.h"
#include "BSP.h"
#include "GBSPFile.h"
#include "Map.h"
#include "Portals.h"
#include "Texture.h"
#include "Fill.h"
#include "Brush2.h"

#include "Vec3d.h"
#include "Ram.h"

#define USE_VOLUMES

geVec3d	TreeMins;
geVec3d	TreeMaxs;

float	MicroVolume = 0.1f;
int32	NumVisNodes, NumNonVisNodes;

extern int32	NumMerged;
extern int32	NumSubdivided;

int32	NumMakeFaces = 0;

#define PLANESIDE_EPSILON	0.001f

//=======================================================================================
//	VisibleContents
//=======================================================================================
uint32 VisibleContents(uint32 Contents)
{
	int32	j;
	uint32	MajorContents;

	if (!Contents)
		return 0;

	// Only check visible contents
	Contents &= BSP_VISIBLE_CONTENTS;
	
	// Return the strongest one, return the first lsb
	for (j=0; j<32; j++)
	{
		MajorContents = (Contents & (1<<j));
		if (MajorContents)
			return MajorContents;
	}

	return 0;
}	

//=======================================================================================
//	FindPortalSide
//=======================================================================================
void FindPortalSide (GBSP_Portal *p, int32 PSide)
{
	uint32		VisContents, MajorContents;
	GBSP_Brush	*Brush;
	int32		i,j;
	int32		PlaneNum;
	GBSP_Side	*BestSide;
	geFloat		Dot, BestDot;
	GBSP_Plane	*p1, *p2;

	// First, check to see if the contents are intersecting sheets (special case)
	if ((p->Nodes[0]->Contents & BSP_CONTENTS_SHEET) && (p->Nodes[1]->Contents & BSP_CONTENTS_SHEET))
	{
		// The contents are intersecting sheets, so or them together
		VisContents = p->Nodes[0]->Contents | p->Nodes[1]->Contents;
	}
	else
	{
		// Make sure the contents on both sides are not the same
		VisContents = p->Nodes[0]->Contents ^ p->Nodes[1]->Contents;
	}

	// There must be a visible contents on at least one side of the portal...
	MajorContents = VisibleContents(VisContents);

	if (!MajorContents)
		return;

	PlaneNum = p->OnNode->PlaneNum;
	BestSide = NULL;
	BestDot = 0.0f;

	for (j=0 ; j<2 ; j++)
	{
		GBSP_Node	*Node;

		Node = p->Nodes[j];
		p1 = &Planes[p->OnNode->PlaneNum];

		for (Brush=Node->BrushList ; Brush ; Brush=Brush->Next)
		{
			MAP_Brush	*MapBrush;
			GBSP_Side	*pSide;

			MapBrush = Brush->Original;

			// Only use the brush that contains a major contents (solid)
			if (!(MapBrush->Contents & MajorContents))
				continue;

			pSide = MapBrush->OriginalSides;

			for (i=0 ; i<MapBrush->NumSides ; i++, pSide++)
			{
				if (pSide->Flags & SIDE_NODE)
					continue;		// Side not visible (result of a csg'd topbrush)

				// First, Try an exact match
				if (pSide->PlaneNum == PlaneNum)
				{	
					BestSide = pSide;
					goto GotSide;
				}
				// In the mean time, try for the closest match
				p2 = &Planes[pSide->PlaneNum];
				Dot = geVec3d_DotProduct(&p1->Normal, &p2->Normal);
				if (Dot > BestDot)
				{
					BestDot = Dot;
					BestSide = pSide;
				}
			}

		}

	}

	GotSide:

	if (!BestSide)
		GHook.Printf("WARNING: Could not map portal to original brush...\n");

	p->SideFound = GE_TRUE;
	p->Side = BestSide;
}

//=======================================================================================
//	MarkVisibleSides_r
//=======================================================================================
void MarkVisibleSides_r (GBSP_Node *Node)
{
	GBSP_Portal	*p;
	int32		s;

	// Recurse to leafs 
	if (Node->PlaneNum != PLANENUM_LEAF)
	{
		MarkVisibleSides_r (Node->Children[0]);
		MarkVisibleSides_r (Node->Children[1]);
		return;
	}

	// Empty (air) leafs don't have faces
	if (!Node->Contents)
		return;

	for (p=Node->Portals ; p ; p = p->Next[s])
	{
		s = (p->Nodes[1] == Node);

		if (!p->OnNode)
			continue;		// Outside node (assert for it here!!!)

		if (!p->SideFound)
			FindPortalSide (p, s);

		if (p->Side)
			p->Side->Flags |= SIDE_VISIBLE;

		#if 1
		if (p->Side)
		{
			if (!(p->Nodes[!s]->Contents & BSP_CONTENTS_SOLID2) && 
				(p->Nodes[s]->Contents & BSP_CONTENTS_SHEET) && 
				!(p->Side->Flags & SIDE_SHEET))
			{ 
				p->Side->Flags &= ~SIDE_VISIBLE;
				p->Side = NULL;
				p->SideFound = GE_TRUE;		// Don't look for this side again!!!
			}
		}
		#endif
	}

}

//=======================================================================================
//	MarkVisibleSides
//	Looks at all the portals, and marks leaf boundrys that seperate different contents
//  into faces.
//=======================================================================================
void MarkVisibleSides(GBSP_Node *Node, MAP_Brush *Brushes)
{
	int32		j;
	MAP_Brush	*Brush;
	int32		NumSides;

	if (Verbose)
		GHook.Printf("--- Map Portals to Brushes ---\n");

	// Clear all the visible flags
	for (Brush = Brushes; Brush; Brush = Brush->Next)
	{
		NumSides = Brush->NumSides;

		for (j=0 ; j<NumSides ; j++)
			Brush->OriginalSides[j].Flags &= ~SIDE_VISIBLE;
	}
	
	// Set visible flags on the sides that are used by portals
	MarkVisibleSides_r (Node);
}

//=======================================================================================
//	BoxOnPlaneSide
//=======================================================================================
int32 BoxOnPlaneSide(geVec3d *Mins, geVec3d *Maxs, GBSP_Plane *Plane)
{
	int32	Side;
	int32	i;
	geVec3d	Corners[2];
	geFloat	Dist1, Dist2;
	
	if (Plane->Type < 3)
	{
		Side = 0;
		if (VectorToSUB(*Maxs, Plane->Type) > Plane->Dist+PLANESIDE_EPSILON)
			Side |= PSIDE_FRONT;
		if (VectorToSUB(*Mins, Plane->Type) < Plane->Dist-PLANESIDE_EPSILON)
			Side |= PSIDE_BACK;
		return Side;
	}
	
	for (i=0 ; i<3 ; i++)
	{
		if (VectorToSUB(Plane->Normal, i) < 0)
		{
			VectorToSUB(Corners[0], i) = VectorToSUB(*Mins, i);
			VectorToSUB(Corners[1], i) = VectorToSUB(*Maxs, i);
		}
		else
		{
			VectorToSUB(Corners[1], i) = VectorToSUB(*Mins, i);
			VectorToSUB(Corners[0], i) = VectorToSUB(*Maxs, i);
		}
	}

	Dist1 = geVec3d_DotProduct(&Plane->Normal, &Corners[0]) - Plane->Dist;
	Dist2 = geVec3d_DotProduct(&Plane->Normal, &Corners[1]) - Plane->Dist;
	Side = 0;
	if (Dist1 >= PLANESIDE_EPSILON)
		Side = PSIDE_FRONT;
	if (Dist2 < PLANESIDE_EPSILON)
		Side |= PSIDE_BACK;

	return Side;
}

//=======================================================================================
//	MakeBSPBrushes
//	Converts map brushes into useable bsp brushes
//=======================================================================================
GBSP_Brush *MakeBSPBrushes(MAP_Brush *MapBrushes)
{
	MAP_Brush		*MapBrush;
	GBSP_Brush		*NewBrush, *NewBrushes;
	int32			j, NumSides;
	int32			Vis;

	NewBrushes = NULL;

	for (MapBrush = MapBrushes; MapBrush; MapBrush = MapBrush->Next)
	{
		NumSides = MapBrush->NumSides;

		if (!NumSides)
			continue;

		Vis = 0;
		for (j=0; j< NumSides; j++)
		{
			if (MapBrush->OriginalSides[j].Poly)
				Vis++;
		}

		if (!Vis)
			continue;

		NewBrush = AllocBrush(NumSides);
	
		NewBrush->Original = MapBrush;
		NewBrush->NumSides = NumSides;

		memcpy (NewBrush->Sides, MapBrush->OriginalSides, NumSides*sizeof(GBSP_Side));

		for (j=0 ; j<NumSides ; j++)
		{
			if (MapBrush->OriginalSides[j].Flags & SIDE_HINT)
				NewBrush->Sides[j].Flags |= SIDE_VISIBLE;

			if (NewBrush->Sides[j].Poly)
			{
				CopyPoly(NewBrush->Sides[j].Poly, &NewBrush->Sides[j].Poly);
			}
		}
		
		NewBrush->Mins = MapBrush->Mins;
		NewBrush->Maxs = MapBrush->Maxs;

		BoundBrush (NewBrush);

		if (!CheckBrush(NewBrush))
		{
			GHook.Error("MakeBSPBrushes:  Bad brush.\n");
			continue;
		}

		NewBrush->Next = NewBrushes;
		NewBrushes = NewBrush;
	}

	return NewBrushes;
}

//=======================================================================================
//	TestBrushToPlane
//=======================================================================================
int32 TestBrushToPlane(	GBSP_Brush *Brush, int32 PlaneNum, int32 PSide, 
						int32 *NumSplits, geBoolean *HintSplit, int32 *EpsilonBrush)
{
	int32		i, j, Num;
	GBSP_Plane	*Plane;
	int32		s;
	GBSP_Poly	*p;
	geFloat		d, FrontD, BackD;
	int32		Front, Back;
	GBSP_Side	*pSide;

	*NumSplits = 0;
	*HintSplit = GE_FALSE;

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		Num = Brush->Sides[i].PlaneNum;
		
		if (Num == PlaneNum && !Brush->Sides[i].PlaneSide)
			return PSIDE_BACK|PSIDE_FACING;

		if (Num == PlaneNum && Brush->Sides[i].PlaneSide)
			return PSIDE_FRONT|PSIDE_FACING;
	}
	
	// See if it's totally on one side or the other
	Plane = &Planes[PlaneNum];
	s = BoxOnPlaneSide (&Brush->Mins, &Brush->Maxs, Plane);

	if (s != PSIDE_BOTH)
		return s;
	
	// The brush is split, count the number of splits 
	FrontD = BackD = 0.0f;

	for (pSide = Brush->Sides, i=0 ; i<Brush->NumSides ; i++, pSide++)
	{
		geVec3d	*pVert;
		uint8	PSide;

		if (pSide->Flags & SIDE_NODE)
			continue;		

		if (!(pSide->Flags & SIDE_VISIBLE))
			continue;		

		p = pSide->Poly;

		if (!p)
			continue;

		PSide = pSide->PlaneSide;

		Front = Back = 0;

		for (pVert = p->Verts, j=0 ; j<p->NumVerts; j++, pVert++)
		{
		#if 1
			d = Plane_PointDistanceFast(Plane, pVert);
		#else
			d = geVec3d_DotProduct(pVert, &Plane->Normal) - Plane->Dist;
		#endif

			if (d > FrontD)
				FrontD = d;
			else if (d < BackD)
				BackD = d;

			if (d > 0.1) 
				Front = 1;
			else if (d < -0.1) 
				Back = 1;
		}

		if (Front && Back)
		{
			(*NumSplits)++;
			if (pSide->Flags & SIDE_HINT)
				*HintSplit = GE_TRUE;
		}
	}

	// Check to see if this split would produce a tiny brush (would result in tiny leafs, bad for vising)
	if ( (FrontD > 0.0 && FrontD < 1.0) || (BackD < 0.0 && BackD > -1.0) )
		(*EpsilonBrush)++;

	return s;
}

//=======================================================================================
//	CheckPlaneAgainstParents
//	Makes sure no plane gets used twice in the tree, from the children up.  
//	This would screw up the portals
//=======================================================================================
geBoolean CheckPlaneAgainstParents (int32 PNum, GBSP_Node *Node)
{
	GBSP_Node	*p;

	for (p=Node->Parent ; p ; p=p->Parent)
	{
		if (p->PlaneNum == PNum)
		{
			GHook.Error ("Tried parent");
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//=======================================================================================
//	CheckPlaneAgainstVolume
//	Makes sure that a potential splitter does not make tiny volumes from the parent volume
//=======================================================================================
geBoolean CheckPlaneAgainstVolume (int32 PNum, GBSP_Node *Node)
{
	GBSP_Brush	*Front, *Back;
	geBoolean	Good;

	SplitBrush(Node->Volume, PNum, 0, SIDE_NODE, GE_FALSE, &Front, &Back);

	Good = (Front && Back);

	if (Front)
		FreeBrush(Front);
	if (Back)
		FreeBrush(Back);

	return Good;
}

//=======================================================================================
//	SelectSplitSide
//=======================================================================================
GBSP_Side *SelectSplitSide(GBSP_Brush *Brushes, GBSP_Node *Node)
{
	int32		Value, BestValue;
	GBSP_Brush	*Brush, *Test;
	GBSP_Side	*Side, *BestSide;
	int32		i, j, Pass, NumPasses;
	int32		PNum, PSide;
	int32		s;
	int32		Front, Back, Both, Facing, Splits;
	int32		BSplits;
	int32		BestSplits;
	int32		EpsilonBrush;
	geBoolean	HintSplit;

	if (CancelRequest)
		return NULL;

	BestSide = NULL;
	BestValue = -999999;
	BestSplits = 0;

	NumPasses = 4;
	for (Pass = 0 ; Pass < NumPasses ; Pass++)
	{
		for (Brush = Brushes ; Brush ; Brush=Brush->Next)
		{
			if ( (Pass & 1) && !(Brush->Original->Contents & BSP_CONTENTS_DETAIL2) )
				continue;
			if ( !(Pass & 1) && (Brush->Original->Contents & BSP_CONTENTS_DETAIL2) )
				continue;
			
			for (i=0 ; i<Brush->NumSides ; i++)
			{
				Side = &Brush->Sides[i];

				if (!Side->Poly)
					continue;	
				if (Side->Flags & (SIDE_TESTED|SIDE_NODE))
					continue;	
 				if (!(Side->Flags&SIDE_VISIBLE) && Pass<2)
					continue;	

				PNum = Side->PlaneNum;
				PSide = Side->PlaneSide;
				
				assert(CheckPlaneAgainstParents (PNum, Node) == GE_TRUE);
				
			#ifdef USE_VOLUMES
				if (!CheckPlaneAgainstVolume (PNum, Node))
					continue;	
			#endif
				
				Front = 0;
				Back = 0;
				Both = 0;
				Facing = 0;
				Splits = 0;
				EpsilonBrush = 0;

				for (Test = Brushes ; Test ; Test=Test->Next)
				{
					s = TestBrushToPlane(Test, PNum, PSide, &BSplits, &HintSplit, &EpsilonBrush);

					Splits += BSplits;

					if (BSplits && (s&PSIDE_FACING) )
						GHook.Error("PSIDE_FACING with splits\n");

					Test->TestSide = s;

					if (s & PSIDE_FACING)
					{
						Facing++;
						for (j=0 ; j<Test->NumSides ; j++)
						{
							if (Test->Sides[j].PlaneNum == PNum)
								Test->Sides[j].Flags |= SIDE_TESTED;
						}
					}
					if (s & PSIDE_FRONT)
						Front++;
					if (s & PSIDE_BACK)
						Back++;
					if (s == PSIDE_BOTH)
						Both++;
				}

				Value = 5*Facing - 5*Splits - abs(Front-Back);
				
				if (Planes[PNum].Type < 3)
					Value+=5;				
				
				Value -= EpsilonBrush*1000;	

				if (HintSplit && !(Side->Flags & SIDE_HINT) )
					Value = -999999;

				if (Value > BestValue)
				{
					BestValue = Value;
					BestSide = Side;
					BestSplits = Splits;
					for (Test = Brushes ; Test ; Test=Test->Next)
						Test->Side = Test->TestSide;
				}
			}

		#if 0
			if (BestSide)		// Just take the first side...
				break;
		#endif

		}

		if (BestSide)
		{
			if (Pass > 1)
				NumNonVisNodes++;
			
			if (Pass > 0)
			{
				Node->Detail = GE_TRUE;			// Not needed for vis
				if (BestSide->Flags & SIDE_HINT)
					GHook.Printf("*** Hint as Detail!!! ***\n");
			}
			
			break;
		}
	}

	for (Brush = Brushes ; Brush ; Brush=Brush->Next)
	{
		for (i=0 ; i<Brush->NumSides ; i++)
			Brush->Sides[i].Flags &= ~SIDE_TESTED;
	}

	return BestSide;
}

//=======================================================================================
//	SplitBrushList
//=======================================================================================
void SplitBrushList (GBSP_Brush *Brushes, GBSP_Node *Node, GBSP_Brush **Front, GBSP_Brush **Back)
{
	GBSP_Brush	*Brush, *NewBrush, *NewBrush2, *Next;
	GBSP_Side	*Side;
	int32		Sides;
	int32		i;

	*Front = *Back = NULL;

	for (Brush = Brushes ; Brush ; Brush = Next)
	{
		Next = Brush->Next;

		Sides = Brush->Side;

		if (Sides == PSIDE_BOTH)
		{	
			SplitBrush(Brush, Node->PlaneNum, 0, SIDE_NODE, GE_FALSE, &NewBrush, &NewBrush2);
			if (NewBrush)
			{
				NewBrush->Next = *Front;
				*Front = NewBrush;
			}
			if (NewBrush2)
			{
				NewBrush2->Next = *Back;
				*Back = NewBrush2;
			}
			continue;
		}

		NewBrush = CopyBrush(Brush);

		if (Sides & PSIDE_FACING)
		{
			for (i=0 ; i<NewBrush->NumSides ; i++)
			{
				Side = NewBrush->Sides + i;
				if (Side->PlaneNum == Node->PlaneNum)
					Side->Flags |= SIDE_NODE;
			}
		}


		if (Sides & PSIDE_FRONT)
		{
			NewBrush->Next = *Front;
			*Front = NewBrush;
			continue;
		}
		if (Sides & PSIDE_BACK)
		{
			NewBrush->Next = *Back;
			*Back = NewBrush;
			continue;
		}
	}
}

//=======================================================================================
//	LeafNode
//	Converts a node into a leaf, and create the contents
//=======================================================================================
void LeafNode (GBSP_Node *Node, GBSP_Brush *Brushes)
{
	GBSP_Brush	*Brush;
	int32		i;

	Node->PlaneNum = PLANENUM_LEAF;
	Node->Contents = 0;

	// Get the contents of this leaf, by examining all the brushes that made this leaf
	for (Brush = Brushes; Brush; Brush = Brush->Next)
	{
		if (Brush->Original->Contents & BSP_CONTENTS_SOLID2)
		{
			for (i=0 ; i < Brush->NumSides ;i++)
			{
				if (!(Brush->Sides[i].Flags & SIDE_NODE))
					break;
			}
		
			// If all the planes in this leaf where caused by splits, then
			// we can force this leaf to be solid...
			if (i == Brush->NumSides)
			{
				//Node->Contents &= 0xffff0000;
				Node->Contents |= BSP_CONTENTS_SOLID2;
				//break;
			}
			
		}
		
		Node->Contents |= Brush->Original->Contents;
	}

	// Once brushes get down to the leafs, we don't need to keep the polys on them anymore...
	// We can free them now...
	for (Brush = Brushes ; Brush ; Brush=Brush->Next)
	{
		// Don't need to keep polygons anymore...
		for (i=0; i< Brush->NumSides; i++)
		{
			if (Brush->Sides[i].Poly)
			{
				FreePoly(Brush->Sides[i].Poly);
				Brush->Sides[i].Poly = NULL;
			}
		}
	}
	
	Node->BrushList = Brushes;
}

//=======================================================================================
//	BuildTree_r
//=======================================================================================
GBSP_Node *BuildTree_r (GBSP_Node *Node, GBSP_Brush *Brushes)
{
	GBSP_Node	*NewNode;
	GBSP_Side	*BestSide;
	int32		i;
	GBSP_Brush	*Children[2];

	NumVisNodes++;

	// find the best plane to use as a splitter
	BestSide = SelectSplitSide (Brushes, Node);
	
	if (!BestSide)
	{
		// leaf node
		Node->Side = NULL;
		Node->PlaneNum = PLANENUM_LEAF;
		LeafNode (Node, Brushes);

	#ifdef USE_VOLUMES
		FreeBrush(Node->Volume);
	#endif
		return Node;
	}

	// This is a splitplane node
	Node->Side = BestSide;
	Node->PlaneNum = BestSide->PlaneNum;

	SplitBrushList (Brushes, Node, &Children[0], &Children[1]);
	FreeBrushList(Brushes);
	
	// Allocate children before recursing
	for (i=0 ; i<2 ; i++)
	{
		NewNode = AllocNode ();
		NewNode->Parent = Node;
		Node->Children[i] = NewNode;
	}
	
#ifdef USE_VOLUMES
	// Distribute this nodes volume to its children
	SplitBrush(Node->Volume, Node->PlaneNum, 0, SIDE_NODE, GE_FALSE, &Node->Children[0]->Volume,
		&Node->Children[1]->Volume);

	if (!Node->Children[0]->Volume || !Node->Children[1]->Volume)
		GHook.Printf("*WARNING* BuildTree_r:  Volume was not split on both sides...\n");
	
	FreeBrush(Node->Volume);
#endif	

	// Recursively process children
	for (i=0 ; i<2 ; i++)
		Node->Children[i] = BuildTree_r (Node->Children[i], Children[i]);

	return Node;
}


//=======================================================================================
//	BuildBSP
//=======================================================================================
GBSP_Node *BuildBSP(GBSP_Brush *BrushList)
{
	GBSP_Node	*Node;
	GBSP_Brush	*b;
	int32		NumVisFaces, NumNonVisFaces;
	int32		NumVisBrushes;
	int32		i;
	geFloat		Volume;
	geVec3d		Mins, Maxs;

	if (Verbose)
		GHook.Printf("--- Build BSP Tree ---\n");

	ClearBounds(&Mins, &Maxs);

	NumVisFaces = 0;
	NumNonVisFaces = 0;
	NumVisBrushes = 0;

	for (b=BrushList ; b ; b=b->Next)
	{
		NumVisBrushes++;

		Volume = BrushVolume (b);

		if (Volume < MicroVolume)
		{
			GHook.Printf("**WARNING** BuildBSP: Brush with NULL volume\n");
		}
		
		for (i=0 ; i<b->NumSides ; i++)
		{
			if (!b->Sides[i].Poly)
				continue;
			if (b->Sides[i].Flags & SIDE_NODE)
				continue;
			if (b->Sides[i].Flags & SIDE_VISIBLE)
				NumVisFaces++;
			else
				NumNonVisFaces++;
		}

		AddPointToBounds (&b->Mins, &Mins, &Maxs);
		AddPointToBounds (&b->Maxs, &Mins, &Maxs);
		
	}
	
	if (Verbose)
	{
		GHook.Printf("Total Brushes          : %5i\n", NumVisBrushes);
		GHook.Printf("Total Faces            : %5i\n", NumVisFaces);
		GHook.Printf("Faces Removed          : %5i\n", NumNonVisFaces);
	}

	NumVisNodes = 0;
	NumNonVisNodes = 0;
	
	Node = AllocNode();

#ifdef USE_VOLUMES
	Node->Volume = BrushFromBounds (&Mins, &Maxs);

	if (BrushVolume(Node->Volume) < 1.0f)
		GHook.Printf("**WARNING** BuildBSP: BAD world volume.\n");
#endif

	Node = BuildTree_r (Node, BrushList);

	// Top node is always valid, this way portals can use top node to get box of entire bsp...
	Node->Mins = Mins;
	Node->Maxs = Maxs;
	
	TreeMins = Mins;
	TreeMaxs = Maxs;

	if (Verbose)
	{
		GHook.Printf("Total Nodes            : %5i\n", NumVisNodes/2 - NumNonVisNodes);
		GHook.Printf("Nodes Removed          : %5i\n", NumNonVisNodes);
		GHook.Printf("Total Leafs            : %5i\n", (NumVisNodes+1)/2);
	}

	return Node;
}

//=======================================================================================
//	FaceFromPortal
//=======================================================================================
GBSP_Face *FaceFromPortal (GBSP_Portal *p, int32 PSide)
{
	GBSP_Face	*f;
	GBSP_Side	*Side;

	Side = p->Side;
	
	if (!Side)
		return NULL;	// Portal does not bridge different visible contents

	if ( (p->Nodes[PSide]->Contents & BSP_CONTENTS_WINDOW2)
		&& VisibleContents(p->Nodes[!PSide]->Contents^p->Nodes[PSide]->Contents) == BSP_CONTENTS_WINDOW2)
		return NULL;

	f = AllocFace (0);

	if (Side->TexInfo >= NumTexInfo || Side->TexInfo < 0)
		GHook.Printf("*WARNING* FaceFromPortal:  Bad texinfo.\n");

	f->TexInfo = Side->TexInfo;
	f->PlaneNum = Side->PlaneNum;
	f->PlaneSide = PSide;
	f->Portal = p;
	f->Visible = GE_TRUE;

	if (PSide)
		ReversePoly(p->Poly, &f->Poly);
	else
		CopyPoly(p->Poly, &f->Poly);

	return f;
}

void SubdivideNodeFaces (GBSP_Node *Node);

//=======================================================================================
//	CountLeafFaces_r
//=======================================================================================
void CountLeafFaces_r(GBSP_Node *Node, GBSP_Face *Face)
{
	
	while (Face->Merged)
		Face = Face->Merged;

	if (Face->Split[0])
	{
		CountLeafFaces_r(Node, Face->Split[0]);
		CountLeafFaces_r(Node, Face->Split[1]);
		return;
	}

	Node->NumLeafFaces++;
}

//=======================================================================================
//	GetLeafFaces_r
//=======================================================================================
void GetLeafFaces_r(GBSP_Node *Node, GBSP_Face *Face)
{
	
	while (Face->Merged)
		Face = Face->Merged;

	if (Face->Split[0])
	{
		GetLeafFaces_r(Node, Face->Split[0]);
		GetLeafFaces_r(Node, Face->Split[1]);
		return;
	}

	Node->LeafFaces[Node->NumLeafFaces++] = Face;
}

//=======================================================================================
//	MakeLeafFaces_r
//=======================================================================================
void MakeLeafFaces_r(GBSP_Node *Node)
{
	GBSP_Portal	*p;
	int32		s;

	// Recurse down to leafs
	if (Node->PlaneNum != PLANENUM_LEAF)
	{
		MakeLeafFaces_r (Node->Children[0]);
		MakeLeafFaces_r (Node->Children[1]);
		return;
	}

	// Solid leafs never have visible faces
	if (Node->Contents & BSP_CONTENTS_SOLID2)
		return;

	// Reset counter
	Node->NumLeafFaces = 0;

	// See which portals are valid
	for (p=Node->Portals ; p ; p = p->Next[s])
	{
		s = (p->Nodes[1] == Node);

		if (!p->Face[s])
			continue;

		CountLeafFaces_r(Node, p->Face[s]);
	}

	Node->LeafFaces = (GBSP_Face**)geRam_Allocate(sizeof(GBSP_Face*)*(Node->NumLeafFaces+1));
	
	// Reset counter
	Node->NumLeafFaces = 0;
	
	// See which portals are valid
	for (p=Node->Portals ; p ; p = p->Next[s])
	{
		s = (p->Nodes[1] == Node);

		if (!p->Face[s])
			continue;

		GetLeafFaces_r(Node, p->Face[s]);
	}
}

//=======================================================================================
//	MakeLeafFaces
//=======================================================================================
void MakeLeafFaces(GBSP_Node *Root)
{
	MakeLeafFaces_r(Root);
}

geBoolean MergeFaceList2(GBSP_Face *Faces);

//=======================================================================================
//	MakeFaces_r
//=======================================================================================
void MakeFaces_r (GBSP_Node *Node)
{
	GBSP_Portal	*p;
	int32		s;

	// Recurse down to leafs
	if (Node->PlaneNum != PLANENUM_LEAF)
	{
		MakeFaces_r (Node->Children[0]);
		MakeFaces_r (Node->Children[1]);
		
		// Marge list
		MergeFaceList2(Node->Faces);
		// Subdivide them for lightmaps
		SubdivideNodeFaces(Node);
		return;
	}

	// Solid leafs never have visible faces
	if (Node->Contents & BSP_CONTENTS_SOLID2)
		return;

	// See which portals are valid
	for (p=Node->Portals ; p ; p = p->Next[s])
	{
		s = (p->Nodes[1] == Node);

		p->Face[s] = FaceFromPortal (p, s);
		if (p->Face[s])
		{
			// Record the contents on each side of the face
			p->Face[s]->Contents[0] = Node->Contents;			// Front side contents is this leaf
			p->Face[s]->Contents[1] = p->Nodes[!s]->Contents;	// Back side contents is the leaf on the other side of this portal

			// Add the face to the list of faces on the node that originaly created the portal
			p->Face[s]->Next = p->OnNode->Faces;
			p->OnNode->Faces = p->Face[s];

			NumMakeFaces++;
		}
	}
}


//=======================================================================================
//	MakeFaces
//=======================================================================================
void MakeFaces (GBSP_Node *Node)
{
	if (Verbose)
		GHook.Printf("--- Finalize Faces ---\n");
	
	NumMerged = 0;
	NumSubdivided = 0;
	NumMakeFaces = 0;

	MakeFaces_r (Node);

	if (Verbose)
	{
		GHook.Printf("TotalFaces             : %5i\n", NumMakeFaces);
		GHook.Printf("Merged Faces           : %5i\n", NumMerged);
		GHook.Printf("Subdivided Faces       : %5i\n", NumSubdivided);
		GHook.Printf("FinalFaces             : %5i\n", (NumMakeFaces-NumMerged)+NumSubdivided);
	}
}

int32	MergedNodes;

//=======================================================================================
//=======================================================================================
void MergeNodes_r (GBSP_Node *Node)
{
	GBSP_Brush		*b, *Next;

	if (Node->PlaneNum == PLANENUM_LEAF)
		return;

	MergeNodes_r (Node->Children[0]);
	MergeNodes_r (Node->Children[1]);

	if (Node->Children[0]->PlaneNum == PLANENUM_LEAF && Node->Children[1]->PlaneNum == PLANENUM_LEAF)
	//if ((Node->Children[0]->Contents == Node->Children[1]->Contents) ||
	if ((Node->Children[0]->Contents & BSP_CONTENTS_SOLID2) && (Node->Children[1]->Contents & BSP_CONTENTS_SOLID2))
	if ((Node->Children[0]->Contents & 0xffff0000) == (Node->Children[1]->Contents & 0xffff0000))
	{
		if (Node->Faces)
			GHook.Error ("Node->Faces seperating BSP_CONTENTS_SOLID!");

		if (Node->Children[0]->Faces || Node->Children[1]->Faces)
			GHook.Error ("!Node->faces with children");

		// FIXME: free stuff
		Node->PlaneNum = PLANENUM_LEAF;
		//Node->Contents = BSP_CONTENTS_SOLID2;
		Node->Contents = Node->Children[0]->Contents;
		Node->Contents |= Node->Children[1]->Contents;

		Node->Detail = GE_FALSE;

		if (Node->BrushList)
			GHook.Error ("MergeNodes: node->brushlist");

		// combine brush lists
		Node->BrushList = Node->Children[1]->BrushList;

		for (b=Node->Children[0]->BrushList ; b ; b=Next)
		{
			Next = b->Next;
			b->Next = Node->BrushList;
			Node->BrushList = b;
		}

		MergedNodes++;
	}
}


//=======================================================================================
//=======================================================================================
void MergeNodes(GBSP_Node *Node)
{
	if (Verbose)
		GHook.Printf("--- Merge Nodes ---\n");

	MergedNodes = 0;
	
	MergeNodes_r (Node);

	if (Verbose)
		GHook.Printf("Num Merged             : %5i\n", MergedNodes);
}

//=======================================================================================
//	FreeBSP_r
//=======================================================================================
void FreeBSP_r(GBSP_Node *Node)
{
	if (!Node)
		return;

	if (Node->PlaneNum == PLANENUM_LEAF)
	{
		FreeNode(Node);
		return;
	}

	FreeBSP_r(Node->Children[0]);
	FreeBSP_r(Node->Children[1]);

	FreeNode(Node);
}

//=======================================================================================
//=======================================================================================
geBoolean ProcessWorldModel(GBSP_Model *Model, MAP_Brush *MapBrushes)
{
	GBSP_Brush		*Brushes;
	GBSP_Node		*Node;

	// Make the bsp brush list
	Brushes = MakeBSPBrushes(MapBrushes);

	if (!Brushes)
	{
		GHook.Error("ProcessWorldModel:  Could not make real brushes.\n");
		return GE_FALSE;
	}

	// Csg the brushes together so none of them are overlapping
	// (this is legal, but makes alot more brushes that land in leafs in the long run...)
	Brushes = CSGBrushes(Brushes);

#if 0
	OutputBrushes("Test.3dt", Brushes);
#endif

	// Build the bsp
	Node = BuildBSP(Brushes);

	Model->Mins = TreeMins;
	Model->Maxs = TreeMaxs;
	
	if (!CreatePortals(Node, Model, GE_FALSE))	
	{
		GHook.Error("Could not create the portals.\n");
		return GE_FALSE;
	}

	// Remove "unseen" leafs so MarkVisibleSides won't reach unseeen areas
	if (RemoveHiddenLeafs(Node, &BSPModels[0].OutsideNode) == -1)
	{
		GHook.Printf("Failed to remove hidden leafs.\n");
		return GE_FALSE;
	}
						    
	// Mark visible sided on the portals
	MarkVisibleSides (Node, MapBrushes);

	// Free vis portals...
	if (!FreePortals(Node))
	{
		GHook.Printf("BuildBSP:  Could not free portals.\n");
		return GE_FALSE;
	}

	// Free the tree
	FreeBSP_r(Node);

	// Make the bsp brush list
	Brushes = MakeBSPBrushes(MapBrushes);

	if (!Brushes)
	{
		GHook.Error("ProcessWorldModel:  Could not make real brushes.\n");
		return GE_FALSE;
	}

	// Csg the brushes
	Brushes = CSGBrushes(Brushes);

	// Build the bsp
	Node = BuildBSP(Brushes);

	Model->Mins = TreeMins;
	Model->Maxs = TreeMaxs;

	if (!CreatePortals(Node, Model, GE_FALSE))	
	{
		GHook.Error("Could not create the portals.\n");
		return GE_FALSE;
	}

	// Remove hidden leafs one last time
	if (RemoveHiddenLeafs(Node, &BSPModels[0].OutsideNode) == -1)
	{
		GHook.Printf("Failed to remove hidden leafs.\n");
		return GE_FALSE;
	}

	// Mark visible sides one last time
	MarkVisibleSides (Node, MapBrushes);

	// Finally make the faces on the visible portals
	MakeFaces(Node);

	// Make the leaf LeafFaces (record what faces touch what leafs...)
	MakeLeafFaces(Node);

	// Free portals...
	if (!FreePortals(Node))
	{
		GHook.Printf("BuildBSP:  Could not free portals.\n");
		return GE_FALSE;
	}

	// Prune the tree
	MergeNodes (Node);

	//FreeBrushList(Brushes);

	// Assign the root node to the model
	Model->RootNode[0] = Node;

	//ShowBrushHeap();

	return GE_TRUE;
}

//=======================================================================================
//=======================================================================================
geBoolean ProcessSubModel(GBSP_Model *Model, MAP_Brush *MapBrushes)
{
	GBSP_Brush		*Brushes;
	GBSP_Node		*Node;

	// Make the bsp brush list
	Brushes = MakeBSPBrushes(MapBrushes);
	Brushes = CSGBrushes(Brushes);

	// Build the bsp
	Node = BuildBSP(Brushes);

	Model->Mins = TreeMins;
	Model->Maxs = TreeMaxs;

	if (!CreatePortals(Node, Model, GE_FALSE))	
	{
		GHook.Error("Could not create the portals.\n");
		return NULL;
	}

	MarkVisibleSides (Node, MapBrushes);
						    
	MakeFaces(Node);

	if (!FreePortals(Node))
	{
		GHook.Printf("BuildBSP:  Could not free portals.\n");
		return GE_FALSE;
	}

	MergeNodes (Node);

	// Assign the root node to the model
	Model->RootNode[0] = Node;

	return GE_TRUE;
}

char SkyNames[6][32] = {
	"SkyTop",
	"SkyBottom",
	"SkyLeft",
	"SkyRight",
	"SkyFront",
	"SkyBack"
};

//========================================================================================
//	GetSkyBoxInfo
//========================================================================================
geBoolean GetSkyBoxInfo(void)
{

	char	*Name;
	int32	i;

	for (i=0; i<6; i++)
		GFXSkyData.Textures[i] = -1;
	
	for (i=0; i<6; i++)
	{
		Name = ValueForKey(&Entities[0], SkyNames[i]);

		if (!Name || !Name[0])
			continue;

		GFXSkyData.Textures[i] = FindTextureIndex(Name, TEXTURE_SKYBOX);

		//GHook.Printf("Sky %i, %s, %s\n", i, SkyNames[i], Name);
	}
	
	if (!GetVectorForKey2(&Entities[0], "SkyAxis", &GFXSkyData.Axis))
	{
		GFXSkyData.Axis.X = 0.0f;
		GFXSkyData.Axis.Y = 0.0f;
		GFXSkyData.Axis.Z = 0.0f;
	}

	GFXSkyData.Dpm = FloatForKey(&Entities[0], "SkyRotation");

	//if (!GFXSkyData.Dpm)
	//	GFXSkyData.Dpm = 10.0f;

	GFXSkyData.DrawScale = FloatForKey(&Entities[0], "SkyDrawScale");

	if (!GFXSkyData.DrawScale)
		GFXSkyData.DrawScale = 1.0f;		// Default to 1.0f

	return GE_TRUE;
}

//=======================================================================================
//	ProcessEntities
//=======================================================================================
geBoolean ProcessEntities(void)
{
	int32		i;
	geBoolean	OldVerbose;

	OldVerbose = Verbose;

	for (i=0; i< NumEntities; i++)
	{
		if (CancelRequest)
		{
			GHook.Printf("Cancel requested...\n");
			return GE_FALSE;
		}
		
		if (!Entities[i].Brushes2)		// No model if no brushes
			continue;
		
		BSPModels[Entities[i].ModelNum].Origin = Entities[i].Origin;

		if (i == 0)
		{
			if (!ProcessWorldModel(&BSPModels[0], Entities[i].Brushes2))
				return GE_FALSE;
			
			if (!GetSkyBoxInfo())
			{
				GHook.Error("Could not get SkyBox names from world...\n");
				return GE_FALSE;
			}
		}
		else
		{
			if (!EntityVerbose)
				Verbose = GE_FALSE;

			if (!ProcessSubModel(&BSPModels[NumBSPModels], Entities[i].Brushes2))
				return GE_FALSE;
		}
		
		NumBSPModels++;
	}

	Verbose = OldVerbose;

	return GE_TRUE;
}

//=======================================================================================
//	SaveGFXMotionData
//=======================================================================================
geBoolean SaveGFXMotionData(geVFile *VFile)
{
	int32		i, NumMotions;
	GBSP_Chunk	Chunk;
	long		StartPos;
	long		EndPos;

	geVFile_Tell(VFile, &StartPos);
	Chunk.Type = GBSP_CHUNK_MOTIONS;
	Chunk.Size = 0;
	Chunk.Elements = 1;

	WriteChunk(&Chunk, NULL, VFile);

	geVFile_Printf(VFile, "Genesis_Motion_File v1.0\r\n");

	// Count the motions
	NumMotions = 0;
	for (i=0; i< NumEntities; i++)
	{
		if (!Entities[i].Brushes2)	
			continue;
		
		if (!Entities[i].Motion)			// No motion data for model
			continue;
		
		NumMotions++;
	}

	geVFile_Printf(VFile, "NumMotions %i\r\n", NumMotions);	// Save out number of motions

	// For all entities that have motion, save their motion out in a motion file...
	for (i=0; i< NumEntities; i++)
	{
		if (!Entities[i].Brushes2)			// No model if no brushes
			continue;

		if (!Entities[i].Motion)			// No motion data for model
			continue;

		geVFile_Printf(VFile, "ModelNum %i\r\n", Entities[i].ModelNum);
			
		// Save out the motion
		if (!geMotion_WriteToFile(Entities[i].Motion, VFile))
		{
			GHook.Error("Error saving motion data.\n");
			return GE_FALSE;
		}
		geMotion_Destroy(&Entities[i].Motion);
		Entities[i].Motion = NULL;
	}

	geVFile_Tell(VFile, &EndPos);

	geVFile_Seek(VFile, StartPos, GE_VFILE_SEEKSET);

	Chunk.Type = GBSP_CHUNK_MOTIONS;
	Chunk.Size = 1;
	Chunk.Elements = EndPos - StartPos - sizeof(Chunk);
	WriteChunk(&Chunk, NULL, VFile);
	geVFile_Seek(VFile, EndPos, GE_VFILE_SEEKSET);
	NumGFXMotionBytes = Chunk.Elements;

	return GE_TRUE;
}

