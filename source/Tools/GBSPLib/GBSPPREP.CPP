/****************************************************************************************/
/*  GBSPPrep.cpp                                                                        */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Saves non GFX data to GFX data (GFX data is data on disk)              */
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


#include "GBSPPrep.h"
#include "GBSPFile.h"
#include "Poly.h"		// For Planes, and NumPlanes
#include "BSP.h"
#include "Map.h"		// For texture
#include "Texture.h"
#include "Portals.h"
#include "Leaf.h"

#include "VFile.h"
#include "Ram.h"

geBoolean	SaveGFXModelData(geVFile *f);
geBoolean	SaveGFXNodes(geVFile *f);
geBoolean	SaveGFXPortals(geVFile *f);
geBoolean	SaveGFXBNodes(geVFile *f);
geBoolean	SaveGFXLeafs(geVFile *f);
geBoolean	SaveGFXClusters(geVFile *f);		// CHANGE: CLUSTER
geBoolean	SaveGFXAreasAndPortals(geVFile *f);
geBoolean	SaveGFXFaces(geVFile *f);
geBoolean	SaveGFXLeafSides(geVFile *f);
geBoolean	SaveGFXVerts(geVFile *f);
geBoolean	SaveGFXVertIndexList(geVFile *f);
geBoolean	SaveGFXPlanes(geVFile *f);
geBoolean	SaveGFXTextures(geVFile *f);
geBoolean	SaveGFXEntData(geVFile *f);
geBoolean	SaveGFXMotionData(geVFile *VFile);

char	VisFile[300];

int32 NumSolidLeafs;

geBoolean ConvertGBSPToFile(char *FileName)
{
	geVFile		*f;
	GBSP_Chunk	Chunk;

	strcpy(VisFile, FileName);

	if (!FixModelTJunctions())
	{
		GHook.Error("ConvertGBSPToFile:  FixModelTJunctions failed.\n");
		return GE_FALSE;
	}

	GFXVertIndexList = GE_RAM_ALLOCATE_ARRAY(int32,TotalIndexVerts);
	NumGFXVerts = NumWeldedVerts;
	GFXVerts = WeldedVerts;

	// Go through all the bsp models and setup data that the gfxmodels need
	if (!PrepAllGBSPModels())
	{
		GHook.Error("ConvertGBSPToFile:  Could not Prep Models.\n");
		return GE_FALSE;
	}

	f = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, FileName, NULL, GE_VFILE_OPEN_CREATE);

	if (!f)
	{
		GHook.Error("ConvertGBSPToFile:  geVFile_OpenNewSystem failed.\n");
		return GE_FALSE;
	}

	strcpy(GBSPHeader.TAG, "GBSP");
	GBSPHeader.TAG[4] = NULL;

	GBSPHeader.Version = GBSP_VERSION;

	// Record the time the bsp was saved...
	GetSystemTime(&GBSPHeader.BSPTime);

	Chunk.Type = GBSP_CHUNK_HEADER;
	Chunk.Size = sizeof(GBSP_Header);
	Chunk.Elements = 1;
	WriteChunk(&Chunk, (void*)&GBSPHeader, f);

	// HACK!  Just point these to the GBSP versions, but reset them below...
	NumGFXLeafSides = NumLeafSides;
	GFXLeafSides = (GFX_LeafSide*)LeafSides;
	
	//GHook.Printf("Saving GFX Model Data\n");
	if (!SaveGFXModelData(f))
	{
		GHook.Error("ConvertGBSPToFile:  SaveGFXModelData failed.\n");
		return GE_FALSE;
	}
	if (!SaveGFXNodes(f))
		return GE_FALSE;
	if (!SaveGFXLeafs(f))
	{
		GHook.Error("ConvertGBSPToFile:  SaveGFXLeafs failed.\n");
		return GE_FALSE;
	}
	if (!SaveGFXClusters(f))
		return GE_FALSE;
	if (!SaveGFXAreasAndPortals(f))
		return GE_FALSE;
	if (!SaveGFXLeafSides(f))
		return GE_FALSE;
	if (!SaveGFXFaces(f))
		return GE_FALSE;
	if (!SaveGFXPlanes(f))
		return GE_FALSE;
	if (!SaveGFXVerts(f))
		return GE_FALSE;
	if (!SaveGFXVertIndexList(f))
		return GE_FALSE;
	if (!SaveGFXTextures(f))
		return GE_FALSE;
	if (!SaveGFXEntData(f))
		return GE_FALSE;
	if (!SaveGFXMotionData(f))
		return GE_FALSE;
	
	Chunk.Type = GBSP_CHUNK_END;
	Chunk.Elements = 0;
	Chunk.Size = 0;
	WriteChunk(&Chunk, NULL, f);

	geVFile_Close(f);

	GHook.Printf(" --- Save GBSP File --- \n");
 	
	GHook.Printf("Num Models           : %5i, %6i\n", NumBSPModels, NumBSPModels*sizeof(GFX_Model));
	GHook.Printf("Num Nodes            : %5i, %6i\n", NumGFXNodes, NumGFXNodes*sizeof(GFX_Node));
	GHook.Printf("Num Solid Leafs      : %5i, %6i\n", NumSolidLeafs, NumSolidLeafs*sizeof(GFX_Leaf));
	GHook.Printf("Num Total Leafs      : %5i, %6i\n", NumGFXLeafs, NumGFXLeafs*sizeof(GFX_Leaf));
	GHook.Printf("Num Clusters         : %5i, %6i\n", NumGFXClusters, NumGFXClusters*sizeof(GFX_Cluster));
	GHook.Printf("Num Areas            : %5i, %6i\n", NumGFXAreas-1, (NumGFXAreas-1)*sizeof(GFX_Area));
	GHook.Printf("Num Area Portals     : %5i, %6i\n", NumGFXAreaPortals, NumGFXAreaPortals*sizeof(GFX_AreaPortal));
	GHook.Printf("Num Leafs Sides      : %5i, %6i\n", NumGFXLeafSides, NumGFXLeafSides*sizeof(GFX_LeafSide));
	GHook.Printf("Num Planes           : %5i, %6i\n", NumPlanes, NumPlanes*sizeof(GBSP_Plane));
	GHook.Printf("Num Faces            : %5i, %6i\n", NumGFXFaces, NumGFXFaces*sizeof(GFX_Face));
	GHook.Printf("Num Leaf Faces       : %5i, %6i\n", NumGFXLeafFaces, NumGFXLeafFaces*sizeof(int32));
	GHook.Printf("Num Vert Index       : %5i, %6i\n", NumGFXVertIndexList, NumGFXVertIndexList*sizeof(int32));
	GHook.Printf("Num Verts            : %5i, %6i\n", NumGFXVerts, NumGFXVerts*sizeof(geVec3d));
	GHook.Printf("Num FaceInfo         : %5i, %6i\n", NumGFXTexInfo, NumGFXTexInfo*sizeof(GFX_TexInfo));
	GHook.Printf("Num Textures         : %5i, %6i\n", NumGFXTextures, NumGFXTextures*sizeof(GFX_Texture));
	GHook.Printf("Motion Data Size     :        %6i\n", NumGFXMotionBytes);
	GHook.Printf("Tex Data Size        :        %6i\n", NumGFXTexData);

	geRam_Free(GFXVertIndexList);
	GFXVertIndexList = NULL;
	NumGFXVertIndexList = 0;
	
	//geRam_Free(GFXVerts);
	GFXVerts = NULL;

	NumGFXEntData = 0;

	// Reset these back, since we did not actually create them, we just
	// pointed them to the GBSP_LeafSide structure version (same structure size/type)
	GFXLeafSides = NULL;
	NumGFXLeafSides = 0;

	FreeGBSPFile();

	return GE_TRUE;
}

geBoolean SaveGFXModelData(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;
	GFX_Model	GModel;

	Chunk.Type = GBSP_CHUNK_MODELS;
	Chunk.Size = sizeof(GFX_Model);
	Chunk.Elements = NumBSPModels;

	WriteChunk(&Chunk, NULL, f);

	for (i=0; i< NumBSPModels; i++)
	{
		// FIXME:  Make a (BSPModel ---> GFXModel) function
		GModel.RootNode[0] = BSPModels[i].RootNodeID[0];
		GModel.Origin = BSPModels[i].Origin;
		GModel.Mins = BSPModels[i].Mins;
		GModel.Maxs = BSPModels[i].Maxs;
		GModel.RootNode[1] = BSPModels[i].RootNodeID[1];
		GModel.FirstFace = BSPModels[i].FirstFace;
		GModel.NumFaces = BSPModels[i].NumFaces;
		GModel.FirstLeaf = BSPModels[i].FirstLeaf;
		GModel.NumLeafs = BSPModels[i].NumLeafs;
		GModel.FirstCluster = BSPModels[i].FirstCluster;
		GModel.NumClusters = BSPModels[i].NumClusters;
		GModel.Areas[0] = BSPModels[i].Areas[0];
		GModel.Areas[1] = BSPModels[i].Areas[1];
		if (geVFile_Write(f, &GModel, sizeof(GFX_Model)) != GE_TRUE)
		{
			GHook.Error("SaveGFXModelData:  There was an error writing the model.\n");
			return GE_FALSE;
		}
	}	
	
	return GE_TRUE;	
}

geBoolean SaveGFXNodes_r(GBSP_Node *Node, geVFile *f)
{
	GFX_Node	GNode;

	if (Node->PlaneNum == PLANENUM_LEAF)
		return GE_TRUE;
	
	GNode.Children[0] = Node->ChildrenID[0];
	GNode.Children[1] = Node->ChildrenID[1];
	GNode.NumFaces = Node->NumFaces;
	GNode.FirstFace = Node->FirstFace;
	GNode.PlaneNum = Node->PlaneNum;
	GNode.Mins = Node->Mins;
	GNode.Maxs = Node->Maxs;

	if (geVFile_Write(f, &GNode, sizeof(GFX_Node)) != GE_TRUE)
	{
		GHook.Error("SaveGFXNodes_r:  There was an error writing the node.\n");
		return GE_FALSE;
	}

	if (!SaveGFXNodes_r(Node->Children[0], f))
		return GE_FALSE;

	if (!SaveGFXNodes_r(Node->Children[1], f))
		return GE_FALSE;

	return GE_TRUE;
}

geBoolean SaveGFXNodes(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_NODES;
	Chunk.Size = sizeof(GFX_Node);
	Chunk.Elements = NumGFXNodes;

	WriteChunk(&Chunk, NULL, f);
	
	for (i=0; i< NumBSPModels; i++)
	{
		if (!SaveGFXNodes_r(BSPModels[i].RootNode[0], f))
			return GE_FALSE;
	}

	return GE_TRUE;
}

geBoolean SaveGFXPortals_r(GBSP_Node *Node, geVFile *f)
{
	GBSP_Portal	*Portal;
	GFX_Portal	GPortal;
	int32		Side, i;
	geVec3d		Origin;
	GBSP_Poly	*Poly;

	if (Node->PlaneNum == PLANENUM_LEAF)
	{
		if (Node->Contents & BSP_CONTENTS_SOLID2)
			return GE_TRUE;	

		for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
		{
			Side = (Portal->Nodes[1] == Node);
			
			if (!(Portal->Nodes[0]->Contents & BSP_CONTENTS_SOLID2) &&
				!(Portal->Nodes[1]->Contents & BSP_CONTENTS_SOLID2))
			{
				Poly = Portal->Poly;
				geVec3d_Clear(&Origin);
				for (i=0; i< Poly->NumVerts; i++)
					geVec3d_Add(&Origin, &Poly->Verts[i], &Origin);
				for (i=0; i<3; i++)
					VectorToSUB(Origin, i) /= Poly->NumVerts;

				GPortal.Origin = Origin;
				GPortal.LeafTo = Portal->Nodes[!Side]->PortalLeafNum;

				if (geVFile_Write(f, &GPortal, sizeof(GFX_Portal)) != GE_TRUE)
				{
					GHook.Error("SaveGFXPortals_r:  There was an error writing the portal.\n");
					return GE_FALSE;
				}
			}
		}
		return GE_TRUE;
	}
	
	if (!SaveGFXPortals_r(Node->Children[0], f))
		return GE_FALSE;

	if (!SaveGFXPortals_r(Node->Children[1], f))
		return GE_FALSE;

	return GE_TRUE;
}

geBoolean SaveGFXPortals(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_PORTALS;
	Chunk.Size = sizeof(GFX_Portal);
	Chunk.Elements = NumGFXPortals;

	WriteChunk(&Chunk, NULL, f);
	
	for (i=0; i< NumBSPModels; i++)
	{
		if (!SaveGFXPortals_r(BSPModels[i].RootNode[0], f))
			return GE_FALSE;
	}

	return GE_TRUE;
}

geBoolean SaveGFXBNodes_r(GBSP_Node *Node, geVFile *f)
{
	GFX_BNode	GBNode;

	if (Node->PlaneNum == PLANENUM_LEAF)
		return GE_TRUE;
	
	GBNode.Children[0] = Node->ChildrenID[0];
	GBNode.Children[1] = Node->ChildrenID[1];
	GBNode.PlaneNum = Node->PlaneNum;
	//GBNode.PlaneSide = Node->PlaneSide;

	if (geVFile_Write(f, &GBNode, sizeof(GFX_BNode)) != GE_TRUE)
	{
		GHook.Error("SaveGFXBNodes_r:  There was an error writing the node.\n");
		return GE_FALSE;
	}

	if (!SaveGFXBNodes_r(Node->Children[0], f))
		return GE_FALSE;
	if (!SaveGFXBNodes_r(Node->Children[1], f))
		return GE_FALSE;

	return GE_TRUE;
}

geBoolean SaveGFXBNodes(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_BNODES;
	Chunk.Size = sizeof(GFX_BNode);
	Chunk.Elements = NumGFXBNodes;

	WriteChunk(&Chunk, NULL, f);
	
	for (i=0; i< NumBSPModels; i++)
		if (!SaveGFXBNodes_r(BSPModels[i].RootNode[1], f))
			return GE_FALSE;

	return GE_TRUE;
}

int32 TotalLeafSize;

//========================================================================================
//	SaveGFXLeafs_r
//========================================================================================
geBoolean SaveGFXLeafs_r(GBSP_Node *Node, geVFile *f)
{
	GFX_Leaf	GLeaf;
	int32		i;

	if (Node->PlaneNum == PLANENUM_LEAF)
	{
		GLeaf.Contents = Node->Contents;

		#if 0
		if (!GLeaf.Contents)
			GLeaf.Contents = BSP_CONTENTS_AIR;
		#endif

		GLeaf.Mins = Node->Mins;
		GLeaf.Maxs = Node->Maxs;

		GLeaf.FirstFace = NumGFXLeafFaces;
		GLeaf.FirstPortal = Node->FirstPortal;
		GLeaf.NumPortals = Node->NumPortals;

		GLeaf.Cluster = Node->Cluster;		// CHANGE: CLUSTER
		GLeaf.Area = Node->Area;

		GLeaf.FirstSide = Node->FirstSide;
		GLeaf.NumSides = Node->NumSides;

		GLeaf.NumFaces = 0;

		for (i=0; i< Node->NumLeafFaces; i++)
		{
			if (!Node->LeafFaces[i]->Visible)
				continue;
			
			// Don't output mark face if it was skipped in the face output stage
			// (or it will reference an invalid face...)
			if (Node->LeafFaces[i]->NumIndexVerts <= 0)
				continue;

			GFXLeafFaces[NumGFXLeafFaces] = Node->LeafFaces[i]->OutputNum;
			NumGFXLeafFaces++;

			GLeaf.NumFaces++;
		}

		TotalLeafSize += sizeof(GFX_Leaf);

		if (geVFile_Write(f, &GLeaf, sizeof(GFX_Leaf)) != GE_TRUE)
		{
			GHook.Error("SaveGFXLeafs_r:  There was an error writing the leaf.\n");
			return GE_FALSE;
		}
		return GE_TRUE;
	}

	if (!SaveGFXLeafs_r(Node->Children[0], f))
		return GE_FALSE;
	if (!SaveGFXLeafs_r(Node->Children[1], f))
		return GE_FALSE;

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXLeafs
//========================================================================================
geBoolean SaveGFXLeafs(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_LEAFS;
	Chunk.Size = sizeof(GFX_Leaf);
	Chunk.Elements = NumGFXLeafs;

	WriteChunk(&Chunk, NULL, f);

	TotalLeafSize = 0;

	// NumGFXLeafFaces was counted earlier in the PrepGfxNodes Stage
	GFXLeafFaces = GE_RAM_ALLOCATE_ARRAY(int32,NumGFXLeafFaces);

	NumGFXLeafFaces = 0;		// We must reset this...

	for (i=0; i< NumBSPModels; i++)
	{
		// Save all the leafs for this model
		if (!SaveGFXLeafs_r(BSPModels[i].RootNode[0], f))
		{
			GHook.Error("SaveGFXLeafs:  SaveGFXLeafs_r failed.\n");
			return GE_FALSE;
		}
	}

	if (TotalLeafSize != Chunk.Size * Chunk.Elements)
	{
		GHook.Error("SaveGFXLeafs:  Leaf sizes don't match.\n");
	//	return GE_FALSE;
	}

	// Save gfx leaf faces here...
	Chunk.Type = GBSP_CHUNK_LEAF_FACES;
	Chunk.Size = sizeof(int32);
	Chunk.Elements = NumGFXLeafFaces;

	WriteChunk(&Chunk, GFXLeafFaces, f);

	return GE_TRUE;
}

// CHANGE: CLUSTER
//========================================================================================
// SaveGFXClusters
//========================================================================================
geBoolean SaveGFXClusters(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;
	GFX_Cluster	GCluster;

	NumGFXClusters = NumLeafClusters;
	Chunk.Type = GBSP_CHUNK_CLUSTERS;
	Chunk.Size = sizeof(GFX_Cluster);
	Chunk.Elements = NumGFXClusters;

	WriteChunk(&Chunk, NULL, f);

	for (i=0; i< NumGFXClusters; i++)
	{
		GCluster.VisOfs = -1;

		if (geVFile_Write(f, &GCluster, sizeof(GFX_Cluster)) != GE_TRUE)
		{
			GHook.Error("SaveGFXClusters:  There was an error saving the cluster.\n");
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}
//========================================================================================
//	SaveGFXAreasAndPortals
//========================================================================================
geBoolean SaveGFXAreasAndPortals(geVFile *f)
{
	GBSP_Chunk	Chunk;

	//
	// Save the areas first
	//
	Chunk.Type = GBSP_CHUNK_AREAS;
	Chunk.Size = sizeof(GFX_Area);
	Chunk.Elements = NumGFXAreas;

	WriteChunk(&Chunk, GFXAreas, f);

	//
	//	Then, save the areaportals
	//
	Chunk.Type = GBSP_CHUNK_AREA_PORTALS;
	Chunk.Size = sizeof(GFX_AreaPortal);
	Chunk.Elements = NumGFXAreaPortals;

	WriteChunk(&Chunk, GFXAreaPortals, f);

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXFaces_r
//========================================================================================
geBoolean SaveGFXFaces_r(GBSP_Node *Node, geVFile *f)
{
	GBSP_Face	*Face;
	GFX_Face	GFace;

	if (Node->PlaneNum == PLANENUM_LEAF)
		return GE_TRUE;
	
	for (Face = Node->Faces; Face; Face = Face->Next)
	{
		if (!Face->Visible)
			continue;

		if (Face->Merged || Face->Split[0] || Face->Split[1])
			continue;

		if (Face->NumIndexVerts > 0)
		{
			GFace.FirstVert = Face->FirstIndexVert;
			GFace.NumVerts = Face->NumIndexVerts;
			GFace.PlaneNum = Face->PlaneNum;
			GFace.PlaneSide = Face->PlaneSide;
			GFace.TexInfo = Face->TexInfo;
			#pragma message ("Make LWidth/Height correct!")
			GFace.LWidth = 0;
			GFace.LHeight = 0;
			GFace.LightOfs = -1;		// No light info yet
			GFace.LTypes[0] = 255;		// Of course, no styles yet either
			GFace.LTypes[1] = 255;
			GFace.LTypes[2] = 255;
			GFace.LTypes[3] = 255;

			if (geVFile_Write(f, &GFace, sizeof(GFX_Face)) != GE_TRUE)
			{
				GHook.Error("SaveGFXFace_r:  There was an error writing the face.\n");
				return GE_FALSE;
			}
		}
	}

	if (!SaveGFXFaces_r(Node->Children[0], f))
		return GE_FALSE;

	if (!SaveGFXFaces_r(Node->Children[1], f))
		return GE_FALSE;

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXFaces
//========================================================================================
geBoolean SaveGFXFaces(geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_FACES;
	Chunk.Size = sizeof(GFX_Face);
	Chunk.Elements = NumGFXFaces;

	WriteChunk(&Chunk, NULL, f);
	
	for (i=0; i< NumBSPModels; i++)
		if (!SaveGFXFaces_r(BSPModels[i].RootNode[0], f))
			return GE_FALSE;

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXPlanes
//========================================================================================
geBoolean SaveGFXPlanes(geVFile *f)
{
	int32			i;
	GBSP_Chunk	Chunk;
	GFX_Plane	GPlane;

	NumGFXPlanes = NumPlanes;
	Chunk.Type = GBSP_CHUNK_PLANES;
	Chunk.Size = sizeof(GFX_Plane);
	Chunk.Elements = NumGFXPlanes;

	WriteChunk(&Chunk, NULL, f);

	for (i=0; i< NumGFXPlanes; i++)
	{
		GPlane.Normal = Planes[i].Normal;
		GPlane.Dist = Planes[i].Dist;
		GPlane.Type = Planes[i].Type;

		if (geVFile_Write(f, &GPlane, sizeof(GFX_Plane)) != GE_TRUE)
		{
			GHook.Error("SaveGFXPlanes:  There was an error saving the plane.\n");
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXLeafSides
//========================================================================================
geBoolean SaveGFXLeafSides(geVFile *f)
{
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_LEAF_SIDES;
	Chunk.Size = sizeof(GFX_LeafSide);
	Chunk.Elements = NumGFXLeafSides;

	if (!WriteChunk(&Chunk, GFXLeafSides, f))
	{
		GHook.Error("There was an error writing the verts.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXVerts
//========================================================================================
geBoolean SaveGFXVerts(geVFile *f)
{
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_VERTS;
	Chunk.Size = sizeof(geVec3d);
	Chunk.Elements = NumGFXVerts;

	if (!WriteChunk(&Chunk, GFXVerts, f))
	{
		GHook.Error("There was an error writing the verts.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXVertIndexList
//========================================================================================
geBoolean SaveGFXVertIndexList(geVFile *f)
{
	GBSP_Chunk	Chunk;

	Chunk.Type = GBSP_CHUNK_VERT_INDEX;
	Chunk.Size = sizeof(int32);
	Chunk.Elements = NumGFXVertIndexList;

	if (!WriteChunk(&Chunk, GFXVertIndexList, f))
	{
		GHook.Error("SaveGFXvertIndexList:  There was an error saving the VertIndexList.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//========================================================================================
//	PrepGFXNode
//========================================================================================
void PrepGFXNode(GBSP_Node *Node)
{
	GBSP_Face	*Face;
	int32		NumFaces;
	int32		i;
	geVec3d		*Verts;

	NumFaces = 0;
	Node->FirstFace = NumGFXFaces;

	for (Face = Node->Faces; Face; Face = Face->Next)
	{
		if (!Face->Visible)
			continue;

		if (Face->Merged || Face->Split[0] || Face->Split[1])
			continue;

		// Skip output of face, if IndexVerts not > 0
		// NOTE - The leaf faces output stage will also skip these same faces...
		if (Face->NumIndexVerts <= 0)
			continue;

		Face->FirstIndexVert = NumGFXVertIndexList;
		Face->OutputNum = NumGFXFaces;

		Verts = Face->Poly->Verts;
		
		for (i=0; i< Face->NumIndexVerts; i++)
		{
			GFXVertIndexList[NumGFXVertIndexList] = Face->IndexVerts[i];
			Verts++;
			NumGFXVertIndexList++;
		}
		NumGFXFaces ++;
		NumFaces++;
	}

	Node->NumFaces = NumFaces;
}

//========================================================================================
//	PrepGFXNodes_r
//	Store current nodes for model in GFXNodes/GFXLeafs so they can be saved to disk
//========================================================================================
int32 PrepGFXNodes_r(int32 Original, GBSP_Node *Node)
{
	int32			CurrentNode;

	// Prep the leaf and it's portals
	if (Node->PlaneNum == PLANENUM_LEAF)
	{
		if (Node->Contents & BSP_CONTENTS_SOLID2)
			NumSolidLeafs++;		// Remember how many solid leafs there are

		Node->NumPortals = 0;
		Node->FirstPortal = -1;

		// To be able to save out portal LeafTo's
		Node->PortalLeafNum = NumGFXLeafs;

		//  Count num gfx leaf faces here, so we know how big to make the array
		//	later, when they are saved out...
		NumGFXLeafFaces += Node->NumLeafFaces;

		#if 0
		{
			GBSP_Portal	*Portal;
			int32			Side;

			// Setup leaf portals
			Node->FirstPortal = NumGFXPortals;
			for (Portal = Node->Portals; Portal; Portal = Portal->Next[Side])
			{
				Side = (Portal->Nodes[1] == Node);

				if (!(Portal->Nodes[0]->Contents & BSP_CONTENTS_SOLID2) &&
					!(Portal->Nodes[1]->Contents & BSP_CONTENTS_SOLID2))
				{
					Node->NumPortals++;
					NumGFXPortals++;
				}
			}
		}
		#endif

		// Increase the number of leafs
		NumGFXLeafs++;

		return -(NumGFXLeafs);
	}
		
	CurrentNode = NumGFXNodes;

	PrepGFXNode(Node);

	NumGFXNodes++;

	Node->ChildrenID[0] = PrepGFXNodes_r(Node->ChildrenID[0], Node->Children[0]);
	Node->ChildrenID[1] = PrepGFXNodes_r(Node->ChildrenID[1], Node->Children[1]);

	return CurrentNode;
}

//typedef	unsigned char	RawPalette[256*3];
				
geBoolean SaveGFXTextures(geVFile *f)
{
	GBSP_Chunk	Chunk;
	int32		Pos1, Pos2;
	uint8		Data[256*256*4];
	int32		Size, Width, Height, Offset, i;
	char		*File;
	DRV_Palette	Palette;

	File = ValueForKey(&Entities[0], "TextureLib");
	if (!File || !File[0])
	{
		GHook.Error("SaveGFXTextures:  No TextureLib specified in map file.\n");
		return GE_FALSE;
	}

	if (!InitTextureLib(File))
		return GE_FALSE;

	Offset = 0;
	
	geVFile_Tell(f, &Pos1);

	Chunk.Type = GBSP_CHUNK_TEXDATA;
	Chunk.Size = 0;
	Chunk.Elements = 1;

	WriteChunk(&Chunk, NULL, f);

	//GHook.Printf("Num Textures:  %i\n", NumTextures);

	for (i=0; i< NumTextures; i++)
	{
		//GHook.Printf("Texture : %i, %s\n", i, Textures[i].Name);
		
		if (!GetTexture(Textures[i].Name, Data, &Size, &Width, &Height, f))
			return GE_FALSE;

		Textures[i].Width = Width;
		Textures[i].Height = Height;
		Textures[i].Offset = NumGFXTexData;
		Textures[i].PaletteIndex = i;

		NumGFXTexData += Size;
	}

	geVFile_Tell(f, &Pos2);

	geVFile_Seek(f, Pos1, GE_VFILE_SEEKSET);

	Chunk.Type = GBSP_CHUNK_TEXDATA;
	Chunk.Size = 1;
	Chunk.Elements = NumGFXTexData;
	WriteChunk(&Chunk, NULL, f);

	geVFile_Seek(f, Pos2, GE_VFILE_SEEKSET);

	// Now save textures
	NumGFXTextures = NumTextures;
	
	Chunk.Type = GBSP_CHUNK_TEXTURES;
	Chunk.Size = sizeof(GFX_Texture);
	Chunk.Elements = NumGFXTextures;

	WriteChunk(&Chunk, NULL, f);

	if (geVFile_Write(f, (GFX_Texture*)Textures, sizeof(GFX_Texture) * NumGFXTextures) != GE_TRUE)
	{
		GHook.Error("SaveGFXTexture:  There was an error saving the texture.\n");
		return GE_FALSE;
	}

	// Now save tex info
	NumGFXTexInfo = NumTexInfo;

	Chunk.Type = GBSP_CHUNK_TEXINFO;
	Chunk.Size = sizeof(GFX_TexInfo);
	Chunk.Elements = NumGFXTexInfo;

	WriteChunk(&Chunk, NULL, f);

	if (geVFile_Write(f, (GFX_TexInfo*)TexInfo, sizeof(GFX_TexInfo) * NumGFXTexInfo) != GE_TRUE)
	{
		GHook.Error("SaveGFXTexture:  There was an error saving the tex info.\n");
		return GE_FALSE;
	}

	// Save all of the palettes out, too
	Chunk.Type = GBSP_CHUNK_PALETTES;
	Chunk.Size = 256 * 3 * sizeof(unsigned char);
	Chunk.Elements = NumTextures;

	WriteChunk(&Chunk, NULL, f);

	for (i=0; i< NumTextures; i++)
	{
		//GHook.Printf("Texture : %i, %s\n", i, Textures[i].Name);
		
		if (!GetTexturePalette(Textures[i].Name, Palette))
			return GE_FALSE;

		if	(geVFile_Write(f, &Palette[0], sizeof(Palette)) != GE_TRUE)
		{
			GHook.Error("There was an error saving the palettes");
			return GE_FALSE;
		}
	}

	ShutdownTextureLib();

	return GE_TRUE;
}

//========================================================================================
//	SaveGFXEntData
//========================================================================================
geBoolean SaveGFXEntData(geVFile *f)
{
	geVFile_MemoryContext	Context;
	geVFile					*MemFile;
	GBSP_Chunk				Chunk;

	// Start an empty memory file
	Context.Data = NULL;
	Context.DataLength = 0;

	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_CREATE);

	if (!MemFile)
		return GE_FALSE;
	
	// Fill this file with the entitydata
	if (!SaveEntityData(MemFile))
	{
		geVFile_Close(MemFile);
		GHook.Error("SaveGFXEntData:  SaveEntityData failed.\n");
		return GE_FALSE;
	}

	// Update the context with this data
	if (!geVFile_UpdateContext(MemFile, &Context, sizeof(Context)))
	{
		geVFile_Close(MemFile);
		return GE_FALSE;
	}
	
	// Write the chunk out
	Chunk.Type = GBSP_CHUNK_ENTDATA;
	Chunk.Size = sizeof(uint8);
	Chunk.Elements = Context.DataLength;

	if (!WriteChunk(&Chunk, Context.Data, f))
	{
		GHook.Error("SaveEntData:  There was an error saving the EntData.\n");
		return GE_FALSE;
	}

	geVFile_Close(MemFile);

	return GE_TRUE;
}

//========================================================================================
//========================================================================================
void BeginGBSPModels(void)
{
	NumLeafClusters = 0;
	NumSolidLeafs = 0;
	NumLeafSides = 0;
	NumLeafBevels = 0;

	// Clear all gfx variables
	NumGFXPortals = 0;
	NumGFXNodes = 0;
	NumGFXBNodes = 0;
	NumGFXLeafs = 0;
	NumGFXFaces = 0;
	NumGFXVerts = 0;
	NumGFXVertIndexList = 0;
	NumGFXLeafFaces = 0;

}

void FinalizeNodeMinMax(GBSP_Node *RootNode);

//========================================================================================
//	PrepGBSPModel
//========================================================================================
geBoolean PrepGBSPModel(GBSP_Model *Model, geBoolean SaveVis)
{
	// Save vis file for vising model 0
	if (SaveVis)		
	{
		// Create vis portals
		if (!CreatePortals(Model->RootNode[0], Model, GE_TRUE))
		{
			GHook.Printf("PrepGBSPModel:  Could not create VIS portals.\n");
			return GE_FALSE;
		}
		
		// Mark clusters
		Model->FirstCluster = NumLeafClusters;
			
		if (!CreateLeafClusters(Model->RootNode[0]))
		{
			GHook.Error("Could not create leaf clusters.\n");
			return GE_FALSE;
		}
			
		Model->NumClusters = NumLeafClusters - Model->FirstCluster;

		if (!SavePortalFile(Model, VisFile))
			return GE_FALSE;

		// Free vis portals...
		if (!FreePortals(Model->RootNode[0]))
		{
			GHook.Printf("PrepGBSPModel:  Could not free portals.\n");
			return GE_FALSE;
		}
		
	}
	else	// Models > 0 are assumed simple and no vising performed, clusters are not needed either...
	{
		Model->FirstCluster = -1;
		Model->NumClusters = 0;
	}

	// Create the REAL structural portals
	if (!CreatePortals(Model->RootNode[0], Model, GE_FALSE))
	{
		GHook.Printf("PrepGBSPModel:  Could not create REAL portals.\n");
		return GE_FALSE;
	}
	
	// Create the leaf collision hulls for bevel bbox collisions
	if (!CreateLeafSides(Model->RootNode[0]))
	{
		GHook.Error("Could not create leaf sides.\n");
		return GE_FALSE;
	}

	// Create the area leafs
	if (Model == BSPModels)		// Only world needs areas set...
	if (!CreateAreas(Model->RootNode[0]))
	{
		GHook.Error("Could not create Areas.\n");
		return GE_FALSE;
	}

	//====
	Model->FirstFace = NumGFXFaces;
	Model->FirstLeaf = NumGFXLeafs;
		
	// Prep the nodes, so we can save them out later to disk
	Model->RootNodeID[0] = PrepGFXNodes_r(Model->RootNodeID[0], Model->RootNode[0]);

	Model->NumFaces = NumGFXFaces - Model->FirstFace;
	Model->NumLeafs = NumGFXLeafs - Model->FirstLeaf;
	
	return GE_TRUE;
}

geBoolean PrepAllGBSPModels(void)
{
	int32	i;

	// Restore verbose since BSP stage turns it off for entities
	Verbose = OriginalVerbose;

	for (i=0; i< NumBSPModels; i++)
	{
		// Turn it back off for entities if asked to do so...
		if (i > 0 && !EntityVerbose)
			Verbose = GE_FALSE;

		if (!PrepGBSPModel(&BSPModels[i], i == 0))
		{
			GHook.Error("PrepAllGBSPModels:  Could not prep model %i.\n", i);
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}
