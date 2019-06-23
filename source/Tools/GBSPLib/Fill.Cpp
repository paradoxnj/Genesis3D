/****************************************************************************************/
/*  Fill.cpp                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Does the flood filling of the leafs, and removes untouchable leafs     */
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

#include "Fill.h"
#include "Map.h"
#include "Portals.h"
#include "Leaf.h"
#include "Poly.h"
#include "GBSPFile.h"
#include "Bsp.h"

int32		CurrentFill;
geBoolean	HitEntity = GE_FALSE;
int32		EntityHit = 0;
GBSP_Node	*HitNode;

int32		NumRemovedLeafs;

//=====================================================================================
//	PlaceEntities
//=====================================================================================
geBoolean PlaceEntities(GBSP_Node *RootNode)
{				
	int32		i;
	GBSP_Node	*Node;
	geBoolean	Empty;

	Empty = GE_FALSE;

	for (i=1; i< NumEntities; i++)
	{
		if (!(Entities[i].Flags & ENTITY_HAS_ORIGIN))		// No "Origin" in entity
			continue;

		Node = FindLeaf(RootNode, &Entities[i].Origin);
		if (!Node)
			return GE_FALSE;

		if (!(Node->Contents & BSP_CONTENTS_SOLID2))
		{
			Node->Entity = i;
			Empty = GE_TRUE;
		}
	}
	
	if (!Empty)
	{
		GHook.Error("PlaceEntities:  No valid entities for operation %i.\n", NumEntities);
		return GE_FALSE;
	}
	
	return GE_TRUE;
}

//=====================================================================================
//	FillUnTouchedLeafs_r
//=====================================================================================
void FillUnTouchedLeafs_r(GBSP_Node *Node)
{
	if (Node->PlaneNum != PLANENUM_LEAF)
	{
		FillUnTouchedLeafs_r(Node->Children[0]);
		FillUnTouchedLeafs_r(Node->Children[1]);
		return;
	}

	if ((Node->Contents & BSP_CONTENTS_SOLID2))
		return;		//allready solid or removed...

	if (Node->CurrentFill != CurrentFill)
	{
		// Fill er in with solid so it does not show up...(Preserve user contents)
		Node->Contents &= (0xffff0000);
		Node->Contents |= BSP_CONTENTS_SOLID2;
		NumRemovedLeafs++;
	}
}

//=====================================================================================
//	FillLeafs2_r
//=====================================================================================
geBoolean FillLeafs2_r(GBSP_Node *Node)
{
	GBSP_Portal		*Portal;
	int32			Side;
	
	if (Node->Contents & BSP_CONTENTS_SOLID2)
		return GE_TRUE;

	if (Node->CurrentFill == CurrentFill)
		return GE_TRUE;

	Node->CurrentFill = CurrentFill;

	for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
	{
		if (Portal->Nodes[0] == Node)
			Side = 0;
		else if (Portal->Nodes[1] == Node)
			Side = 1;
		else
		{
			GHook.Error("RemoveOutside2_r:  Portal does not look at either node.\n");
			return GE_FALSE;
		}

		// Go though the portal to the node on the other side (!side)
		if (!FillLeafs2_r(Portal->Nodes[!Side]))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	CanPassPortal
//	See if a portal can be passed through or not
//=====================================================================================
geBoolean CanPassPortal(GBSP_Portal *Portal)
{
	if (Portal->Nodes[0]->PlaneNum != PLANENUM_LEAF)
		GHook.Printf("*WARNING* CanPassPortal:  Portal does not seperate leafs.\n");

	if (Portal->Nodes[1]->PlaneNum != PLANENUM_LEAF)
		GHook.Printf("*WARNING* CanPassPortal:  Portal does not seperate leafs.\n");

	if (Portal->Nodes[0]->Contents & BSP_CONTENTS_SOLID2)
		return GE_FALSE;

	if (Portal->Nodes[1]->Contents & BSP_CONTENTS_SOLID2)
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	FillLeafs_r
//=====================================================================================
geBoolean FillLeafs_r(GBSP_Node *Node, geBoolean Fill, int32 Dist)
{
	GBSP_Portal		*Portal;
	int32			Side;
	
	//if (HitEntity)
	//	return GE_TRUE;
	
	if (Node->Contents & BSP_CONTENTS_SOLID2)
		return GE_TRUE;

	if (Node->CurrentFill == CurrentFill)
		return GE_TRUE;

	Node->CurrentFill = CurrentFill;

	Node->Occupied = Dist;

	if (Fill)
	{
		// Preserve user contents
		Node->Contents &= 0xffff0000;
		Node->Contents |= BSP_CONTENTS_SOLID2;
	}
	else 
	{
		if (Node->Entity)
		{
			HitEntity = GE_TRUE;
			EntityHit = Node->Entity;
			HitNode = Node;
			return GE_TRUE;
		}
	}

	for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
	{
		if (Portal->Nodes[0] == Node)
			Side = 0;
		else if (Portal->Nodes[1] == Node)
			Side = 1;
		else
		{
			GHook.Error("FillLeafs_r:  Portal does not look at either node.\n");
			return GE_FALSE;
		}
		
		//if (!CanPassPortal(Portal))
		//	continue;

		if (!FillLeafs_r(Portal->Nodes[!Side], Fill, Dist+1))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	FillFromEntities
//=====================================================================================
geBoolean FillFromEntities(GBSP_Node *RootNode)
{
	int32		i;
	GBSP_Node	*Node;
	geBoolean	Empty;

	Empty = GE_FALSE;
	
	for (i=1; i< NumEntities; i++)	// Don't use the world as an entity (skip 0)!!
	{
		if (!(Entities[i].Flags & ENTITY_HAS_ORIGIN))		// No "Origin" in entity
			continue;

		Node = FindLeaf(RootNode, &Entities[i].Origin);

		if (Node->Contents & BSP_CONTENTS_SOLID2)
			continue;
		
		// There is at least one entity in empty space...
		Empty = GE_TRUE;

		if (!FillLeafs2_r(Node))
			return GE_FALSE;
	}

	if (!Empty)
	{
		GHook.Error("FillFromEntities:  No valid entities for operation.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	LeafCenter
//=====================================================================================
void LeafCenter(GBSP_Node *Node, geVec3d *PortalMid)
{
	int32		NumPortals, Side;
	geVec3d		Mid;
	GBSP_Portal	*Portal;

	NumPortals = 0;
	geVec3d_Clear(PortalMid);

	for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
	{
		Side = Portal->Nodes[1] == Node;

		PolyCenter(Portal->Poly, &Mid);
		geVec3d_Add(PortalMid, &Mid, PortalMid);
		NumPortals++;
	}

	geVec3d_Scale(PortalMid, 1.0f/(geFloat)NumPortals, PortalMid);
}

//=====================================================================================
//	WriteLeakFile
//=====================================================================================
geBoolean WriteLeakFile (char *FileName, GBSP_Node *Start, GBSP_Node *Outside)
{
	geVec3d		Mid;
	FILE		*PntFile;
	char		FileName2[1024];
	int32		Count;
	GBSP_Node	*Node;
	int			Pos1, Pos2;
	char		TAG[5];
	geBoolean	Stop;

	GHook.Printf("--- WriteLeakFile ---\n");

	//
	// Write the points to the file
	//
	sprintf (FileName2, "%s.Pnt", FileName);
	PntFile = fopen (FileName2, "wb");

	if (!PntFile)
		GHook.Error ("WriteLeakFile:  Couldn't open %s\n", FileName);

	Count = 0;

	strcpy(TAG, "LEAK");
	fwrite(TAG, sizeof(char), 4, PntFile);

	Pos1 = ftell(PntFile);

	fwrite(&Count, sizeof(int32), 1, PntFile);
	
	Node = Start;

	fwrite(&Entities[EntityHit].Origin, sizeof(geVec3d), 1, PntFile);
	Count++;

	Stop = GE_FALSE;

	while (Node && Node->Occupied > 1)
	{
		int32		Next;
		GBSP_Portal	*p, *NextPortal;
		GBSP_Node	*NextNode;
		int32		s;

		// find the best portal exit
		Next = Node->Occupied;
		NextNode = NULL;
		NextPortal = NULL;

		GHook.Printf("Occupied   : %5i\n", Node->Occupied);

		for (p=Node->Portals ; p ; p = p->Next[!s])
		{
			s = (p->Nodes[0] == Node);

			if (p->Nodes[s] == Outside)
			{
				Stop = GE_TRUE;
				break;
			}

			if (p->Nodes[s]->Occupied && p->Nodes[s]->Occupied < Next)
			{
				NextPortal = p;
				NextNode = p->Nodes[s];
				Next = NextNode->Occupied;
			}
		}

		Node = NextNode;
		
		if (Stop)
			break;

		if (NextPortal && Node && Node != Outside)
		{
			// Write out the center of the portal
			PolyCenter(NextPortal->Poly, &Mid);
			fwrite(&Mid, sizeof(geVec3d), 1, PntFile);
			Count++;
			// Then writer out the center of the leaf it goes too
			LeafCenter(Node, &Mid);
			fwrite(&Mid, sizeof(geVec3d), 1, PntFile);
			Count++;
		}
	}

	GHook.Printf("Num Points    : %5i\n", Count);

	// HACK!!!!
	Pos2 = ftell(PntFile);

	GHook.Printf("Pos1 = %5i\n", Pos1);
	GHook.Printf("Pos2 = %5i\n", Pos2);

	fseek(PntFile, Pos1, SEEK_SET);
	//Count-=2;
	fwrite(&Count, sizeof(int32), 1, PntFile);

	fseek(PntFile, Pos2, SEEK_SET);

	fclose (PntFile);

	return GE_TRUE;
}

//=====================================================================================
//	RemoveHiddenLeafs
//=====================================================================================
int32 RemoveHiddenLeafs(GBSP_Node *RootNode, GBSP_Node *ONode)
{
	int32	Side;

	GHook.Printf(" --- Remove Hidden Leafs --- \n");

	OutsideNode = ONode;
	
	Side = OutsideNode->Portals->Nodes[0] == OutsideNode;

	NumRemovedLeafs = 0;

	if (!PlaceEntities(RootNode))
		return -1;

	HitEntity = GE_FALSE;
	HitNode = NULL;

	CurrentFill = 1;
	if (!FillLeafs_r(OutsideNode->Portals->Nodes[Side], GE_FALSE, 1))
		return -1;

	if (HitEntity)
	{
		GHook.Printf("*****************************************\n");
		GHook.Printf("*           *** LEAK ***                *\n");
		GHook.Printf("* Level is NOT sealed.                  *\n");
		GHook.Printf("* Optimal removal will not be performed.*\n");
		GHook.Printf("*****************************************\n");

		WriteLeakFile("Test", HitNode, ONode);
		return -1;
	}

	CurrentFill = 2;
	
	if (!FillFromEntities(RootNode))
		return -1;
	
	FillUnTouchedLeafs_r(RootNode);

	if (Verbose)
		GHook.Printf("Removed Leafs          : %5i\n", NumRemovedLeafs);

	return NumRemovedLeafs;
}
