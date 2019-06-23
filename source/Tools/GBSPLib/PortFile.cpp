/****************************************************************************************/
/*  PortFile.cpp                                                                        */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Saves BSP portals to disk.                                             */
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

#include "Bsp.h"
#include "Portals.h"
#include "Leaf.h"
#include "GBspFile.h"
#include "Poly.h"
#include "Utils.h"

int32	NumPortals;
int32	NumPortalLeafs;

//====================================================================================
//	ContentsPassable
//====================================================================================
geBoolean ContentsPassable(uint32 Contents)
{
	//return (Contents & BSP_CONTENTS_PASSABLE);
	return !(Contents & BSP_CONTENTS_SOLID2);
}

//====================================================================================
//	ContentsSolid
//====================================================================================
geBoolean ContentsSolid(uint32 Contents)
{
	/*
	Contents &= (BSP_CONTENTS_PASSABLE | BSP_CONTENTS_VISIBLE | BSP_CONTENTS_TRANSLUCENT);
	// If it's only visible, then it "bites" other brushes/contents
	if (Contents == BSP_CONTENTS_VISIBLE)
		return GE_TRUE;
	return GE_FALSE;
	*/
	return (Contents & BSP_CONTENTS_SOLID2);
}
	
//====================================================================================
//	MajorVisibleContents
//====================================================================================
uint32 MajorVisibleContents(uint32 Contents)
{
	/*
	Contents &= (BSP_CONTENTS_PASSABLE | BSP_CONTENTS_VISIBLE | BSP_CONTENTS_TRANSLUCENT);

	if (Contents == BSP_CONTENTS_VISIBLE)
		return Contents;

	if (Contents == (BSP_CONTENTS_PASSABLE | BSP_CONTENTS_VISIBLE)
		return Contents;

	if (Contents == (BSP_CONTENTS_TRANSLUCENT | BSP_CONTENTS_VISIBLE))
		return Contents;

	if (Contents)
		return Contents;
	return 0;
	*/
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

//====================================================================================
//	ClusterContents
//====================================================================================
uint32 ClusterContents(GBSP_Node *Node)
{
	uint32	c1, c2, Contents;

	// Stop at leafs, and start returning contents
	if (Node->PlaneNum == PLANENUM_LEAF)
		return Node->Contents;

	c1 = ClusterContents(Node->Children[0]);
	c2 = ClusterContents(Node->Children[1]);

	Contents = (c1 | c2);		// Or together children, and return

	if (!(c1 & BSP_CONTENTS_SOLID2) || !(c2 & BSP_CONTENTS_SOLID2))
		Contents &= ~BSP_CONTENTS_SOLID2;

	return (Contents);
}

//====================================================================================
//	CanSeeThroughPortal
//====================================================================================
geBoolean CanSeeThroughPortal(GBSP_Portal *Portal)
{
	uint32	c1, c2;

	// Can't see into or from solid
	if ((Portal->Nodes[0]->Contents & BSP_CONTENTS_SOLID2) || 
			(Portal->Nodes[1]->Contents & BSP_CONTENTS_SOLID2))
		return GE_FALSE;
	//return GE_TRUE;

	if (!Portal->OnNode)
		return GE_FALSE;	

	// 'Or' together all cluster contents under portals nodes
	c1 = ClusterContents(Portal->Nodes[0]);
	c2 = ClusterContents(Portal->Nodes[1]);

	// Can only see through portal if contents on both sides are translucent...
	//if ((c1 & BSP_CONTENTS_TRANSLUCENT) && (c2 & BSP_CONTENTS_TRANSLUCENT))
	//	return GE_TRUE;

	if (!MajorVisibleContents (c1^c2))
		return GE_TRUE;

	// Cancel solid if it's detail, or translucent on the leafs/clusters
	if (c1 & (BSP_CONTENTS_TRANSLUCENT2|BSP_CONTENTS_DETAIL2))
		c1 = 0;
	if (c2 & (BSP_CONTENTS_TRANSLUCENT2|BSP_CONTENTS_DETAIL2))
		c2 = 0;

	if ( (c1|c2) & BSP_CONTENTS_SOLID2 )
		return GE_FALSE;		// If it's solid on either side, return GE_FALSE

	if (! (c1 ^ c2))
		return GE_TRUE;		// If it's the same on both sides then we can definitly see through it...

	if (!MajorVisibleContents(c1^c2))
		return GE_TRUE;

	return GE_FALSE;
}

//====================================================================================
//	PrepPortalFile_r
//====================================================================================
geBoolean PrepPortalFile_r(GBSP_Node *Node)
{
	GBSP_Node	*Nodes[2];
	GBSP_Portal	*Portal;
	int32		Side;

	// Stop at leafs, and detail nodes (stop at beginning of clusters)
	if (Node->PlaneNum == PLANENUM_LEAF || Node->Detail)
	{
		if (ContentsSolid(Node->Contents))
			return GE_TRUE;

		// Give this portal it's leaf number...
		Node->PortalLeafNum = NumPortalLeafs;
		NumPortalLeafs++;

		if (!Node->Portals)
		{
			GHook.Printf("*WARNING* PrepPortalFile_r:  Leaf without any portals.\n");
			return GE_TRUE;
		}

		// Save out all the portals that belong to this leaf...
		for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
		{
			Nodes[0] = Portal->Nodes[0];
			Nodes[1] = Portal->Nodes[1];
	
			Side = (Nodes[1] == Node);

			if (!Portal->Poly)
			{
				GHook.Printf("*WARNING*  SavePortalFile_r:  Portal with NULL poly.\n");
				continue;
			}

			if (!CanSeeThroughPortal(Portal))
				continue;
			
			if (Nodes[0]->Cluster == Nodes[1]->Cluster)	
			{
				GHook.Error("PrepPortalFile_r:  Portal seperating the same cluster.\n");
				return GE_FALSE;
			}
			
			// This portal is good...
			NumPortals++;
		}

		return GE_TRUE;
	}

	Node->PortalLeafNum = -1;

	if (Node->Portals)
		GHook.Printf("*WARNING* PrepPortalFile_r:  Node with portal.\n");

	if (!PrepPortalFile_r(Node->Children[0]))
		return GE_FALSE;

	if (!PrepPortalFile_r(Node->Children[1]))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//	SavePortalFile_r
//====================================================================================
geBoolean SavePortalFile_r(GBSP_Node *Node, FILE *f)
{
	GBSP_Node	*Nodes[2];
	GBSP_Portal	*Portal;
	int32		i, Side, Side2, Cluster;
	GBSP_Plane	Plane;
	GBSP_Poly	*Poly;

	if (Node->PlaneNum == PLANENUM_LEAF || Node->Detail)
	{
		// Can't see from solid
		if (ContentsSolid(Node->Contents))
			return GE_TRUE;
		
		if (!Node->Portals)
			return GE_TRUE;

		for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
		{
			Nodes[0] = Portal->Nodes[0];
			Nodes[1] = Portal->Nodes[1];

			Side = (Nodes[1] == Node);

			if (!Portal->Poly)
				continue;

			if (!CanSeeThroughPortal(Portal))
				continue;

			if (Nodes[0]->Cluster == Nodes[1]->Cluster)	
			{
				GHook.Error("PrepPortalFile_r:  Portal seperating the same cluster.\n");
				return GE_FALSE;
			}

			Poly = Portal->Poly;

			if (Poly->NumVerts < 3)
			{
				GHook.Error("SavePortalFile_r:  Portal poly verts < 3.\n");
				return GE_FALSE;
			}

			if (fwrite(&Poly->NumVerts, sizeof(int32), 1, f) != 1)
			{
				GHook.Error(" SavePortalFile_r:  Error writing portal NumVerts.\n");
				return GE_FALSE;
			}

			if (!Side)	// If on front side, reverse so it points to the other leaf
			{
				for (i= Poly->NumVerts-1; i>=0; i--)
				{
					if (fwrite(&Poly->Verts[i], sizeof(geVec3d), 1, f) != 1)
					{
						GHook.Error(" SavePortalFile_r:  Error writing portal verts.\n");
						return GE_FALSE;
					}
				}
			}
			// It's allready pointing to the other leaf
			else if (fwrite(Poly->Verts, sizeof(geVec3d), Poly->NumVerts, f) != (uint32)Poly->NumVerts)
			{
				GHook.Error(" SavePortalFile_r:  Error writing portal verts.\n");
				return GE_FALSE;
			}

			Side2 = Side;

			PlaneFromVerts(Poly->Verts, &Plane);

			if (geVec3d_DotProduct(&Planes[Portal->PlaneNum].Normal, &Plane.Normal) < 0.99f)
				Side2 = !Side2;

			// CHANGE: CLUSTER
			if (Nodes[Side2]->Cluster < 0 || Nodes[Side2]->Cluster > NumLeafClusters)
			{
				GHook.Error("SavePortalFile_r:  Bad Leaf Cluster Number.\n");
				return GE_FALSE;
			}

			Cluster = Nodes[Side2]->Cluster;
			fwrite(&Cluster, sizeof(int32), 1, f);
				
			if (Nodes[!Side2]->Cluster < 0 || Nodes[!Side2]->Cluster > NumLeafClusters)
			{
				GHook.Error("SavePortalFile_r:  Bad Leaf Cluster Number.\n");
				return GE_FALSE;
			}

			Cluster = Nodes[!Side2]->Cluster;
			fwrite(&Cluster, sizeof(int32), 1, f);
		}

		return GE_TRUE;
	}

	if (Node->Portals)
		GHook.Printf("*WARNING* SavePortalFile_r:  Node with portal.\n");

	if (!SavePortalFile_r(Node->Children[0], f))
		return GE_FALSE;

	if (!SavePortalFile_r(Node->Children[1], f))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//	SavePortalFile
//====================================================================================
geBoolean SavePortalFile(GBSP_Model *Model, char *FileName)
{
	char	PortalFile[200];
	char	TAG[14];
	FILE	*f;

	GHook.Printf(" --- Save Portal File --- \n");
	  
	strcpy(PortalFile, FileName);

	StripExtension(PortalFile);
	DefaultExtension(PortalFile, ".GPF");

	f = fopen(PortalFile, "wb");

	if (!f)
	{
		GHook.Error("SavePortalFile:  Error opening %s for writing.\n", PortalFile);
		return GE_FALSE;
	}

	NumPortals = 0;			// Number of portals
	NumPortalLeafs = 0;		// Current leaf number

	if (!PrepPortalFile_r(Model->RootNode[0]))
	{
		fclose(f);
		GHook.Error("SavePortalFile:  Could not PrepPortalFile.\n");
		return GE_FALSE;
	}

	if (NumPortalLeafs != Model->NumClusters)
	{
		GHook.Error("SavePortalFile:  Invalid number of clusters!!!\n");
		return GE_FALSE;
	}

	strcpy(TAG, "GBSP_PRTFILE");
	fwrite(TAG, sizeof(char), 12, f);

	fwrite(&NumPortals, sizeof(int32), 1, f);
	fwrite(&Model->NumClusters, sizeof(int32), 1, f);
	
	if (!SavePortalFile_r(Model->RootNode[0], f))
	{
		fclose(f);
		return GE_FALSE;
	}

	fclose(f);

	if (Verbose)
	{
		GHook.Printf("Num Portals          : %5i\n", NumPortals);
		GHook.Printf("Num Portal Leafs     : %5i\n", NumPortalLeafs);
	}

	return GE_TRUE;
}
