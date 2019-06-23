/****************************************************************************************/
/*  Vis.cpp                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Vises a BSP                                                            */
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

#include "Utils.h"
#include "Vis.h"
#include "GBSPFile.h"
#include "Poly.h"
#include "Bsp.h"

#include "Ram.h"

int32		NumVisPortals;					// Total portals
int32		NumVisPortalBytes;				// Total portals / 8
int32		NumVisPortalLongs;				// Total portalbytes / sizeof(uint32)
VIS_Portal	*VisPortals;					// NumVisPortals
pVIS_Portal	*VisSortedPortals;				// Pointers to portals sorted by MightSee
uint8		*PortalSeen;					// Temp vis array
uint8		*PortalBits;

int32		NumVisLeafs;					// Total VisLeafs
int32		NumVisLeafBytes;				// NumVisLeaf / 8
int32		NumVisLeafLongs;				// NumVisBytes / sizeof(uint32)
uint8		*LeafVisBits;					// Should be NumVisLeafs * (NumVisLeafs / 8)
VIS_Leaf	*VisLeafs;						// NumVisLeafs

int32		TotalVisibleLeafs;

geBoolean	VisVerbose = GE_FALSE;
geBoolean	NoSort = GE_FALSE;
geBoolean	FullVis = GE_TRUE;

void FreeFileVisData(void);
geBoolean StartWritingVis(geVFile *f);
geBoolean FinishWritingVis(geVFile *f);
void FreeAllVisData(void);
void SortPortals(void);
geBoolean CalcPortalInfo(VIS_Portal *Portal);

//=======================================================================================
//	VisGBSPFile
//=======================================================================================
geBoolean VisGBSPFile(char *FileName, VisParms *Parms)
{
	char	PFile[200];
	geVFile	*f;

	f = NULL;

	GHook.Printf(" --- Vis GBSP File --- \n");

	NoSort = !Parms->SortPortals;
	VisVerbose = Parms->Verbose;
	FullVis = Parms->FullVis;
	
	// Fill in the global bsp data
	if (!LoadGBSPFile(FileName))
	{
		GHook.Error("PvsGBSPFile:  Could not load GBSP file: %s\n", FileName);
		goto ExitWithError;
	}

	// Clean out any old vis data
	FreeFileVisData();		

	// Open the bsp file for writing
	f = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, FileName, NULL, GE_VFILE_OPEN_CREATE);
	
	if (!f)		// Oops
	{
		GHook.Error("VisGBSPFile:  Could not open GBSP file for writing: %s.\n", FileName);
		goto ExitWithError;
	}

	// Prepare the portal file name
	strcpy(PFile, FileName);
	StripExtension(PFile);
	DefaultExtension(PFile, ".GPF");
	
	// Load the portal file
	if (!LoadPortalFile(PFile))
		goto ExitWithError;

	GHook.Printf("NumPortals           : %5i\n", NumVisPortals);

	// Write out everything but vis info
	if (!StartWritingVis(f))
		goto ExitWithError;

	// Vis'em
	if (!VisAllLeafs())
		goto ExitWithError;

	// Record the vis data
	NumGFXVisData = NumVisLeafs*NumVisLeafBytes;
	GFXVisData = LeafVisBits;

	// Save the leafs, clusters, vis data, etc
	if (!FinishWritingVis(f))
		goto ExitWithError;

	// Free all the vis stuff
	FreeAllVisData();

	// Free any remaining leftover bsp data
	FreeGBSPFile();		

	geVFile_Close(f);

	return GE_TRUE;

	// ==== ERROR ====
	ExitWithError:
	{
		GHook.Error("PvsGBSPFile:  Could not vis the file: %s\n", FileName);

		if (f)
			geVFile_Close(f);

		FreeAllVisData();
		FreeGBSPFile();

		return GE_FALSE;
	}
}

//=======================================================================================
//	FreeFileVisData
//=======================================================================================
void FreeFileVisData(void)
{
	if (GFXVisData)
		geRam_Free(GFXVisData);
	GFXVisData = NULL;
	NumGFXVisData = 0;
}

int32 LeafSee;
//=======================================================================================
//	VisAllLeafs
//=======================================================================================
geBoolean VisAllLeafs(void)
{
	int32	i;

	// Create PortalSeen array.  This is used by Vis flooding routines
	// This is deleted below...
	PortalSeen = GE_RAM_ALLOCATE_ARRAY(uint8,NumVisPortals);

	if (!PortalSeen)
		goto ExitWithError;

	// Flood all the leafs with the fast method first...
	for (i=0; i< NumVisLeafs; i++)
		FloodLeafPortalsFast(i);

	// Check for cancel request
	if (CancelRequest)
	{
		GHook.Printf("Cancel requested...\n");
		goto ExitWithError;
	}

	// Sort the portals with MightSee
	SortPortals();

	if (FullVis)
		if (!FloodPortalsSlow())
			return GE_FALSE;

	// Don't need this anymore...
	geRam_Free(PortalSeen);
	PortalSeen = NULL;

	LeafVisBits = GE_RAM_ALLOCATE_ARRAY(uint8,NumVisLeafs*NumVisLeafBytes);

	if (!LeafVisBits)
	{
		GHook.Error("VisAllLeafs:  Out of memory for LeafVisBits.\n");
		goto ExitWithError;
	}

	memset(LeafVisBits, 0, NumVisLeafs*NumVisLeafBytes);
	TotalVisibleLeafs = 0;

	PortalBits = GE_RAM_ALLOCATE_ARRAY(uint8,NumVisPortalBytes);

	if (!PortalBits)
		goto ExitWithError;

	for (i=0; i< NumVisLeafs; i++)
	{
		LeafSee = 0;
		
		if (!CollectLeafVisBits(i))
			goto ExitWithError;

		TotalVisibleLeafs += LeafSee;
	}
	geRam_Free(PortalBits);

	GHook.Printf("Total visible areas           : %5i\n", TotalVisibleLeafs);
	GHook.Printf("Average visible from each area: %5i\n", TotalVisibleLeafs / NumVisLeafs);

	return GE_TRUE;

	// ==== ERROR ====
	ExitWithError:
	{
		// Free all the global vis data
		FreeAllVisData();

		return GE_FALSE;
	}
}

//=======================================================================================
//	CollectLeafVisBits
//=======================================================================================
geBoolean CollectLeafVisBits(int32 LeafNum)
{
	VIS_Portal	*Portal, *SPortal;
	VIS_Leaf	*Leaf;
	uint8		*LeafBits, *VisBits;
	int32		k, Bit, SLeaf;
	
	Leaf = &VisLeafs[LeafNum];

	LeafBits = &LeafVisBits[LeafNum * NumVisLeafBytes];

	memset(PortalBits, 0, NumVisPortalBytes);

	// 'OR' all portals that this portal can see into one list
	for (Portal = Leaf->Portals; Portal; Portal = Portal->Next)
	{
		if (Portal->FinalVisBits)				// Try to use final vis info first
			VisBits = Portal->FinalVisBits;
		else if (Portal->VisBits)
			VisBits = Portal->VisBits;
		else
		{
			GHook.Error("No VisInfo for portal.\n");
			return GE_FALSE;
		}

		for (k=0; k< NumVisPortalBytes; k++)
			PortalBits[k] |= VisBits[k];

		if (Portal->VisBits)
			geRam_Free(Portal->VisBits);
		if (Portal->FinalVisBits)
			geRam_Free(Portal->FinalVisBits);				// Don't need this anymore

		Portal->VisBits = NULL;
		Portal->FinalVisBits = NULL;
	}

	// Take this list, and or all leafs that each visible portal looks in to
	for (k=0; k< NumVisPortals; k++)
	{
		if (PortalBits[k>>3] & (1<<(k&7)) )
		{
			SPortal = VisPortals+k;
			SLeaf = SPortal->Leaf;
			LeafBits[SLeaf>>3] |= 1<<(SLeaf&7);
		}
	}
			
	Bit = 1<<(LeafNum&7);

	// He should not have seen himself (yet...)
	if ((LeafBits[LeafNum>>3] & Bit))
		GHook.Printf("*WARNING* CollectLeafVisBits:  Leaf:%i can see himself!\n", LeafNum);

	LeafBits[LeafNum>>3] |= Bit;			// Make sure he can see himself!!!

	for (k=0; k< NumVisLeafs; k++)
	{
		Bit = 1<<(k&7);
		if ((LeafBits[k>>3] & Bit) )
			LeafSee++;
	}

	if (LeafSee == 0)
	{
		GHook.Error("CollectLeafVisBits:  Leaf can't see nothing.\n");
		return GE_FALSE;
	}

	GFXClusters[LeafNum].VisOfs = (int32)(LeafBits - LeafVisBits);

	return GE_TRUE;
}

//=======================================================================================
//	LoadPortalFile
//=======================================================================================
geBoolean LoadPortalFile(char *FileName)
{
	int32		LeafFrom, LeafTo;
	VIS_Portal	*pPortal;
	VIS_Leaf	*pLeaf;
	GBSP_Poly	*pPoly;
	int32		i, NumVerts;
	char		TAG[13];
	geVFile		*f;

	pPoly = NULL;

	// open the file
	f = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, FileName, NULL, GE_VFILE_OPEN_READONLY);

	if (!f)		// opps
	{
		GHook.Error("LoadPortalFile:  Could not open %s for reading.\n", FileName);
		goto ExitWithError;
	}
	
	// 
	//	Check the TAG
	//
	if (geVFile_Read(f, TAG, sizeof(char) * 12) != GE_TRUE)
	{
		GHook.Error("LoadPortalFile:  Error reading portal file TAG.\n");
		goto ExitWithError;
	}

	if (strncmp(TAG, "GBSP_PRTFILE", 12))
	{
		GHook.Error("LoadPortalFile:  %s is not a GBSP Portal file.\n", FileName);
		goto ExitWithError;
	}

	//
	//	Get the number of portals
	//
	if (geVFile_Read(f, &NumVisPortals, sizeof(int32)) != GE_TRUE)
	{
		GHook.Error("LoadPortalFile:  Error reading NumVisPortals.\n");
		goto ExitWithError;
	}

	if (NumVisPortals >= MAX_TEMP_PORTALS)
	{
		GHook.Error("LoadPortalFile:  Max portals for temp buffers.\n");
		goto ExitWithError;
	}
	
	VisPortals = GE_RAM_ALLOCATE_ARRAY(VIS_Portal,NumVisPortals);
	
	if (!VisPortals)
	{
		GHook.Error("LoadPortalFile:  Out of memory for VisPortals.\n");
		goto ExitWithError;
	}
	
	memset(VisPortals, 0, sizeof(VIS_Portal)*NumVisPortals);

	VisSortedPortals = GE_RAM_ALLOCATE_ARRAY(pVIS_Portal,NumVisPortals);
	
	if (!VisSortedPortals)
	{
		GHook.Error("LoadPortalFile:  Out of memory for VisSortedPortals.\n");
		goto ExitWithError;
	}

	//
	//	Get the number of leafs
	//
	if (geVFile_Read(f, &NumVisLeafs, sizeof(int32)) != GE_TRUE)
	{
		GHook.Error("LoadPortalFile:  Error reading NumVisLeafs.\n");
		goto ExitWithError;
	}

	if (NumVisLeafs > NumGFXLeafs)
		goto ExitWithError;
	
	VisLeafs = GE_RAM_ALLOCATE_ARRAY(VIS_Leaf,NumVisLeafs);
	if (!VisLeafs)
	{
		GHook.Error("LoadPortalFile:  Out of memory for VisLeafs.\n");
		goto ExitWithError;
	}

	memset(VisLeafs, 0, sizeof(VIS_Leaf)*NumVisLeafs);
	
	//
	//	Load in the portals
	//
	for (i=0; i< NumVisPortals; i++)
	{
		if (geVFile_Read(f, &NumVerts, sizeof(int32)) != GE_TRUE)
		{
			GHook.Error("LoadPortalFile:  Error reading NumVerts.\n");
			goto ExitWithError;
		}

		pPoly = AllocPoly(NumVerts);

		if (!pPoly)
			goto ExitWithError;

		if (geVFile_Read(f, pPoly->Verts, sizeof(geVec3d) * NumVerts) != GE_TRUE)
		{
			GHook.Error("LoadPortalFile:  Error reading portal vertices.\n");
			goto ExitWithError;
		}
		
		if (geVFile_Read(f, &LeafFrom, sizeof(int32)) != GE_TRUE)
		{
			GHook.Error("LoadPortalFile:  Error reading portal LeafFrom.\n");
			goto ExitWithError;
		}
		
		if (geVFile_Read(f, &LeafTo, sizeof(int32)) != GE_TRUE)
		{
			GHook.Error("LoadPortalFile:  Error reading portal LeafTo.\n");
			goto ExitWithError;
		}

		if (LeafFrom >= NumVisLeafs || LeafFrom < 0)
		{
			GHook.Error("LoadPortalFile:  Invalid LeafFrom: %i.\n", LeafFrom);
			goto ExitWithError;
		}

		if (LeafTo >= NumVisLeafs || LeafTo < 0)
		{
			GHook.Error("LoadPortalFile:  Invalid LeafTo: %i.\n", LeafTo);
			goto ExitWithError;
		}

		pLeaf = &VisLeafs[LeafFrom];
		pPortal = &VisPortals[i];

		pPortal->Poly = pPoly;
		pPortal->Leaf = LeafTo;
		PlaneFromVerts(pPoly->Verts, &pPortal->Plane);
		
		pPortal->Next = pLeaf->Portals;
		pLeaf->Portals = pPortal;

		CalcPortalInfo(pPortal);
	}
	
	NumVisLeafBytes = ((NumVisLeafs+63)&~63) >> 3;
	NumVisPortalBytes = ((NumVisPortals+63)&~63) >> 3;

	NumVisPortalLongs = NumVisPortalBytes/sizeof(uint32);
	NumVisLeafLongs = NumVisLeafBytes/sizeof(uint32);

	geVFile_Close(f);

	return GE_TRUE;

	// ==== ERROR ===
	ExitWithError:
	{
		if (f)
			geVFile_Close(f);

		if (VisPortals)
			geRam_Free(VisPortals);
		if (VisSortedPortals)
			geRam_Free(VisSortedPortals);
		if (VisLeafs)
			geRam_Free(VisLeafs);

		if (pPoly)
			FreePoly(pPoly);

		VisPortals = NULL;
		VisSortedPortals = NULL;
		VisLeafs = NULL;
		pPoly = NULL;

		return GE_FALSE;
	}
}

//================================================================================
//	StartWritingVis
//================================================================================
geBoolean StartWritingVis(geVFile *f)
{
	// Write out everything but the vis data

	GBSP_ChunkData	CurrentChunkData[] = {
		{ GBSP_CHUNK_HEADER			, sizeof(GBSP_Header)	,1             , &GBSPHeader},
		{ GBSP_CHUNK_MODELS			, sizeof(GFX_Model)		,NumGFXModels  , GFXModels },
		{ GBSP_CHUNK_NODES			, sizeof(GFX_Node)		,NumGFXNodes   , GFXNodes  },
		{ GBSP_CHUNK_PORTALS		, sizeof(GFX_Portal)	,NumGFXPortals , GFXPortals},
		{ GBSP_CHUNK_BNODES			, sizeof(GFX_BNode)		,NumGFXBNodes  , GFXBNodes },
		{ GBSP_CHUNK_PLANES			, sizeof(GFX_Plane)		,NumGFXPlanes  , GFXPlanes },
		{ GBSP_CHUNK_FACES			, sizeof(GFX_Face)		,NumGFXFaces   , GFXFaces  },
		{ GBSP_CHUNK_AREAS			, sizeof(GFX_Area)		,NumGFXAreas   , GFXAreas  },
		{ GBSP_CHUNK_AREA_PORTALS	, sizeof(GFX_AreaPortal),NumGFXAreaPortals	, GFXAreaPortals  },
		{ GBSP_CHUNK_LEAF_FACES		, sizeof(int32)          ,NumGFXLeafFaces, GFXLeafFaces  },
		{ GBSP_CHUNK_LEAF_SIDES		, sizeof(GFX_LeafSide)	,NumGFXLeafSides, GFXLeafSides  },
		{ GBSP_CHUNK_VERTS			, sizeof(geVec3d)		,NumGFXVerts   , GFXVerts  },
		{ GBSP_CHUNK_VERT_INDEX		, sizeof(int32)			,NumGFXVertIndexList , GFXVertIndexList},
		{ GBSP_CHUNK_RGB_VERTS		, sizeof(geVec3d)		,NumGFXRGBVerts, GFXRGBVerts  },
		{ GBSP_CHUNK_ENTDATA		, sizeof(uint8)			,NumGFXEntData , GFXEntData},
		{ GBSP_CHUNK_TEXTURES		, sizeof(GFX_Texture)	,NumGFXTextures, GFXTextures},
		{ GBSP_CHUNK_TEXINFO		, sizeof(GFX_TexInfo)	,NumGFXTexInfo , GFXTexInfo},
		{ GBSP_CHUNK_TEXDATA		, sizeof(uint8)			,NumGFXTexData , GFXTexData},
		{ GBSP_CHUNK_LIGHTDATA		, sizeof(uint8)			,NumGFXLightData , GFXLightData},
		{ GBSP_CHUNK_SKYDATA		, sizeof(GFX_SkyData)	,1				, &GFXSkyData},
		{ GBSP_CHUNK_PALETTES		, sizeof(DRV_Palette)	,NumGFXPalettes, GFXPalettes},
		{ GBSP_CHUNK_MOTIONS		, sizeof(uint8)			,NumGFXMotionBytes, GFXMotionData},
	};

	if (!WriteChunks(CurrentChunkData, sizeof(CurrentChunkData) / sizeof(CurrentChunkData[0]), f))
	{
		GHook.Error("leaf StartWritingVis:  Could not write ChunkData.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//================================================================================
//	FinishWritingVis
//================================================================================
geBoolean FinishWritingVis(geVFile *f)
{
	GBSP_ChunkData	ChunkDataEnd[] = {
 		{ GBSP_CHUNK_LEAFS      , sizeof(GFX_Leaf)		,NumGFXLeafs   , GFXLeafs  },
		{ GBSP_CHUNK_CLUSTERS   , sizeof(GFX_Cluster)	,NumGFXClusters, GFXClusters},
		{ GBSP_CHUNK_VISDATA	, sizeof(uint8)			, NumGFXVisData, GFXVisData},
		{ GBSP_CHUNK_END		, 0						,0             , NULL },
	};

	if (!WriteChunks(ChunkDataEnd, 4, f))
	{
		GHook.Error("FinishWritingVis:  Could not write ChunkData.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//================================================================================
//	FreeAllVisData
//================================================================================
void FreeAllVisData(void)
{

	int32		i;

	if (LeafVisBits)
		geRam_Free(LeafVisBits);
	LeafVisBits = NULL;

	GFXVisData = NULL;
	NumGFXVisData = 0;

	if (VisPortals)
	{
		for (i=0; i< NumVisPortals; i++)
		{
			FreePoly(VisPortals[i].Poly);

			if (VisPortals[i].FinalVisBits)
				geRam_Free(VisPortals[i].FinalVisBits);

			if (VisPortals[i].VisBits)
				geRam_Free(VisPortals[i].VisBits);
		}

		geRam_Free(VisPortals);
	}

	if (VisSortedPortals)
		geRam_Free(VisSortedPortals);
	if (PortalSeen)
		geRam_Free(PortalSeen);
	if (VisLeafs)
		geRam_Free(VisLeafs);

	VisPortals = NULL;
	VisSortedPortals = NULL;
	PortalSeen = NULL;
	VisLeafs = NULL;

	FreeGBSPFile();		// Free rest of GBSP GFX data
}

//================================================================================
//	CleanupVis
//================================================================================
void CleanupVis(void)
{
	FreeAllVisData();
}

//================================================================================
//	PComp
//================================================================================
int PComp(const void *a, const void *b)
{
	if ( (*(VIS_Portal**)a)->MightSee == (*(VIS_Portal **)b)->MightSee)
		return 0;
	if ( (*(VIS_Portal**)a)->MightSee < (*(VIS_Portal**)b)->MightSee)
		return -1;
	return 1;
}

//================================================================================
//	SortPortals
//================================================================================
void SortPortals(void)
{
	int		i;
	
	for (i=0 ; i<NumVisPortals ; i++)
		VisSortedPortals[i] = &VisPortals[i];

	if (NoSort)
		return;

	qsort(VisSortedPortals, NumVisPortals, sizeof(VisSortedPortals[0]), PComp);
}

//================================================================================
//	CalcPortalInfo
//================================================================================
geBoolean CalcPortalInfo(VIS_Portal *Portal)
{
	float		BestDist, Dist;
	GBSP_Poly	*Poly;
	geVec3d		Vect;
	int32			i;

	Poly = Portal->Poly;

	PolyCenter(Poly, &Portal->Center);	

	BestDist = 0.0f;

	for (i=0; i< Poly->NumVerts; i++)
	{
		geVec3d_Subtract(&Poly->Verts[i], &Portal->Center, &Vect);

		Dist = geVec3d_Length(&Vect);

		if (Dist > BestDist)
			BestDist = Dist;
	}

	Portal->Radius = BestDist;

	return GE_TRUE;
}