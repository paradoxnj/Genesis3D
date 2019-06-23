/****************************************************************************************/
/*  GBSPFile.cpp                                                                        */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <Stdio.h>

#include "DCommon.h"
#include "GBSPFile.h"
#include "VFile.h"
#include "Ram.h"

//========================================================================================
//	Globals
//========================================================================================
GBSP_Header	GBSPHeader;						// Header
GFX_SkyData	GFXSkyData;

GFX_Model		*GFXModels;						// Model data
GFX_Node		*GFXNodes;						// Nodes
GFX_BNode		*GFXBNodes;						// Bevel Clip nodes
GFX_Leaf		*GFXLeafs;						// Leafs
GFX_Cluster		*GFXClusters;					// CHANGE: CLUSTER
GFX_Area		*GFXAreas;
GFX_AreaPortal	*GFXAreaPortals;
GFX_Plane		*GFXPlanes;						// Planes
GFX_Face		*GFXFaces;						// Faces
int32			*GFXLeafFaces;
GFX_LeafSide	*GFXLeafSides;
geVec3d			*GFXVerts;						// Verts
int32			*GFXVertIndexList;				// Index list
geVec3d			*GFXRGBVerts;

uint8			*GFXEntData;
GFX_Texture		*GFXTextures;					// Textures
GFX_TexInfo		*GFXTexInfo;					// TexInfo
uint8			*GFXTexData;					// TexData

uint8			*GFXLightData;					// Lightmap data
uint8			*GFXVisData;					// Vis data
GFX_Portal		*GFXPortals;					// Portal data
DRV_Palette		*GFXPalettes;					// Texture palettes
uint8			*GFXMotionData;					// Model motion keyframe data

int32		NumGFXModels;
int32		NumGFXNodes;
int32		NumGFXBNodes;
int32		NumGFXLeafs;
int32		NumGFXClusters;		// CHANGE: CLUSTER
int32		NumGFXAreas;	
int32		NumGFXAreaPortals;
int32		NumGFXPlanes;
int32		NumGFXFaces;
int32		NumGFXLeafFaces;
int32		NumGFXLeafSides;
int32		NumGFXVerts;
int32		NumGFXVertIndexList;
int32		NumGFXRGBVerts;

int32		NumGFXEntData;
int32		NumGFXTextures;
int32		NumGFXTexInfo;
int32		NumGFXTexData;

int32		NumGFXLightData;
int32		NumGFXVisData;
int32		NumGFXPortals;
int32		NumGFXPalettes;
int32		NumGFXMotionBytes;

//#define	DEBUGCHUNKS
#ifdef	DEBUGCHUNKS
static	 char *ChunkNames[] =
{
"GBSP_CHUNK_HEADER",
"GBSP_CHUNK_MODELS",
"GBSP_CHUNK_NODES",
"GBSP_CHUNK_BNODES",
"GBSP_CHUNK_LEAFS",
"GBSP_CHUNK_CLUSTERS",
"GBSP_CHUNK_AREAS",
"GBSP_CHUNK_LEAF_SIDES",
"GBSP_CHUNK_PORTALS",
"GBSP_CHUNK_PLANES",
"GBSP_CHUNK_FACES",
"GBSP_CHUNK_LEAF_FACES",
"GBSP_CHUNK_VERT_INDEX",
"GBSP_CHUNK_VERTS",
"GBSP_CHUNK_RGB_VERTS",
"GBSP_CHUNK_ENTDATA",
"GBSP_CHUNK_TEXINFO",
"GBSP_CHUNK_TEXTURES",
"GBSP_CHUNK_TEXDATA",
"GBSP_CHUNK_LIGHTDATA",
"GBSP_CHUNK_VISDATA",
"GBSP_CHUNK_SKYDATA",
"GBSP_CHUNK_PALETTES",
"GBSP_CHUNK_MOTIONS",
};
#endif

//========================================================================================
//	WriteChunk
//========================================================================================
geBoolean WriteChunk(GBSP_Chunk *Chunk, void *Data, geVFile *f)
{
	if (geVFile_Write(f, Chunk, sizeof(GBSP_Chunk)) != GE_TRUE)
	{
		//Hook.Error("WriteChunk:  There was an error writing the chunk (Duh...).\n");
		return GE_FALSE;
	}

#ifdef	DEBUGCHUNKS
	if	(Chunk->Type != GBSP_CHUNK_END)
	{
		long	Pos;

		geVFile_Tell(f, &Pos);
		GHook.Printf(" WriteChunkData: @%08x '%s', %d elements of %d size\n", Pos,
		ChunkNames[Chunk->Type], Chunk->Elements, Chunk->Size);
	}
	else
	{
		long	Pos;

		geVFile_Tell(f, &Pos);
		GHook.Printf(" WriteChunk: @%08x 'GBSP_CHUNK_END', %d elements of %d size\n", Pos, Chunk->Elements, Chunk->Size);
	}
#endif
	if (Chunk->Size * Chunk->Elements > 0)
	{
		if (!Data)
			return GE_TRUE;

		if (geVFile_Write(f, Data, Chunk->Size * Chunk->Elements) != GE_TRUE)
		{
			//Hook.Error("WriteChunk:  There was an error writing the chunk data.\n");
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//========================================================================================
// ReadChunkData
//========================================================================================
geBoolean ReadChunkData(GBSP_Chunk *Chunk, void *Data, geVFile *f)
{
	if (geVFile_Read(f, Data, Chunk->Size * Chunk->Elements) != GE_TRUE)
		return GE_FALSE;

	return GE_TRUE;
}

//========================================================================================
//	ReadChunk
//========================================================================================
geBoolean ReadChunk(GBSP_Chunk *Chunk, geVFile *f)
{
	if (geVFile_Read(f, Chunk, sizeof(GBSP_Chunk)) != GE_TRUE)
	{
		return GE_FALSE;
	}

#ifdef	DEBUGCHUNKS
	if	(Chunk->Type != GBSP_CHUNK_END)
	{
		long	Pos;

		geVFile_Tell(f, &Pos);
		GHook.Printf(" ReadChunk: @%08x '%s', %d elements of %d size\n", Pos,
		ChunkNames[Chunk->Type], Chunk->Elements, Chunk->Size);
	}
	else
	{
		long	Pos;

		geVFile_Tell(f, &Pos);
		GHook.Printf(" ReadChunk: @%08x 'GBSP_CHUNK_END', %d elements of %d size\n", Pos, Chunk->Elements, Chunk->Size);
	}
#endif

	switch(Chunk->Type)
	{
		case GBSP_CHUNK_HEADER:
		{
			if (!ReadChunkData(Chunk, (void*)&GBSPHeader, f))
				return GE_FALSE;
			if (strcmp(GBSPHeader.TAG, "GBSP"))
				return GE_FALSE;
			if (GBSPHeader.Version != GBSP_VERSION)
				return GE_FALSE;

			break;
		}
		case GBSP_CHUNK_MODELS:
		{
			NumGFXModels = Chunk->Elements;
			GFXModels = GE_RAM_ALLOCATE_ARRAY(GFX_Model, NumGFXModels);
			if (!ReadChunkData(Chunk, GFXModels, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_NODES:
		{
			NumGFXNodes = Chunk->Elements;
			GFXNodes = GE_RAM_ALLOCATE_ARRAY(GFX_Node,NumGFXNodes);
			if (!ReadChunkData(Chunk, GFXNodes, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_BNODES:
		{
			NumGFXBNodes = Chunk->Elements;
			GFXBNodes = GE_RAM_ALLOCATE_ARRAY(GFX_BNode,NumGFXBNodes);
			if (!ReadChunkData(Chunk, GFXBNodes, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LEAFS:
		{
			NumGFXLeafs = Chunk->Elements;
			GFXLeafs = GE_RAM_ALLOCATE_ARRAY(GFX_Leaf,NumGFXLeafs);
			if (!ReadChunkData(Chunk, GFXLeafs, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_CLUSTERS:
		{
			NumGFXClusters = Chunk->Elements;

			GFXClusters = GE_RAM_ALLOCATE_ARRAY(GFX_Cluster,NumGFXClusters);
			if (!ReadChunkData(Chunk, GFXClusters, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_AREAS:
		{
			NumGFXAreas = Chunk->Elements;
			GFXAreas = GE_RAM_ALLOCATE_ARRAY(GFX_Area,NumGFXAreas);
			if (!ReadChunkData(Chunk, GFXAreas, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_AREA_PORTALS:
		{
			NumGFXAreaPortals = Chunk->Elements;
			GFXAreaPortals = GE_RAM_ALLOCATE_ARRAY(GFX_AreaPortal,NumGFXAreaPortals);
			if (!ReadChunkData(Chunk, GFXAreaPortals, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_PORTALS:
		{
			NumGFXPortals = Chunk->Elements;
			GFXPortals = GE_RAM_ALLOCATE_ARRAY(GFX_Portal,NumGFXPortals);
			if (!ReadChunkData(Chunk, GFXPortals, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_PLANES:
		{
			NumGFXPlanes = Chunk->Elements;
			GFXPlanes = GE_RAM_ALLOCATE_ARRAY(GFX_Plane,NumGFXPlanes);
			if (!ReadChunkData(Chunk, GFXPlanes, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_FACES:
		{
			NumGFXFaces = Chunk->Elements;
			GFXFaces = GE_RAM_ALLOCATE_ARRAY(GFX_Face,NumGFXFaces);
			if (!ReadChunkData(Chunk, GFXFaces, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LEAF_FACES:
		{
			NumGFXLeafFaces = Chunk->Elements;
			GFXLeafFaces = GE_RAM_ALLOCATE_ARRAY(int32,NumGFXLeafFaces);
			if (!ReadChunkData(Chunk, GFXLeafFaces, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LEAF_SIDES:
		{
			NumGFXLeafSides = Chunk->Elements;

			GFXLeafSides = GE_RAM_ALLOCATE_ARRAY(GFX_LeafSide,NumGFXLeafSides);
			if (!ReadChunkData(Chunk, GFXLeafSides, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_VERTS:
		{
			NumGFXVerts = Chunk->Elements;
			GFXVerts = GE_RAM_ALLOCATE_ARRAY(geVec3d,NumGFXVerts);
			if (!ReadChunkData(Chunk, GFXVerts, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_VERT_INDEX:
		{
			NumGFXVertIndexList = Chunk->Elements;
			GFXVertIndexList = GE_RAM_ALLOCATE_ARRAY(int32,NumGFXVertIndexList);
			if (!ReadChunkData(Chunk, GFXVertIndexList, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_RGB_VERTS:
		{
			NumGFXRGBVerts = Chunk->Elements;
			GFXRGBVerts = GE_RAM_ALLOCATE_ARRAY(geVec3d,NumGFXRGBVerts);
			if (!ReadChunkData(Chunk, GFXRGBVerts, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_TEXINFO:
		{
			NumGFXTexInfo = Chunk->Elements;
			GFXTexInfo = GE_RAM_ALLOCATE_ARRAY(GFX_TexInfo,NumGFXTexInfo);
			if (!ReadChunkData(Chunk, GFXTexInfo, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_TEXTURES:
		{
			NumGFXTextures = Chunk->Elements;
			GFXTextures = GE_RAM_ALLOCATE_ARRAY(GFX_Texture,NumGFXTextures);
			if (!ReadChunkData(Chunk, GFXTextures, f))
				return GE_FALSE;
			break;
		}

		case GBSP_CHUNK_TEXDATA:
		{
//	GHook.Printf(" Reading TEXDATA: %d bytes of %d size \n", Chunk->Elements, Chunk->Size);
			NumGFXTexData = Chunk->Elements;
			GFXTexData = GE_RAM_ALLOCATE_ARRAY(uint8,NumGFXTexData);
			if (!ReadChunkData(Chunk, GFXTexData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_ENTDATA:
		{
			NumGFXEntData = Chunk->Elements;
			GFXEntData = GE_RAM_ALLOCATE_ARRAY(uint8,NumGFXEntData);
			if (!ReadChunkData(Chunk, GFXEntData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_LIGHTDATA:
		{
			NumGFXLightData = Chunk->Elements;
			GFXLightData = GE_RAM_ALLOCATE_ARRAY(uint8,NumGFXLightData);
			if (!ReadChunkData(Chunk, GFXLightData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_VISDATA:
		{
			NumGFXVisData = Chunk->Elements;
			GFXVisData = GE_RAM_ALLOCATE_ARRAY(uint8,NumGFXVisData);
			if (!ReadChunkData(Chunk, GFXVisData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_SKYDATA:
		{
			if (!ReadChunkData(Chunk, &GFXSkyData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_PALETTES:
		{
			NumGFXPalettes = Chunk->Elements;
			GFXPalettes = GE_RAM_ALLOCATE_ARRAY(DRV_Palette,NumGFXPalettes);
			if (!GFXPalettes)
				return GE_FALSE;
			if (!ReadChunkData(Chunk, GFXPalettes, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_MOTIONS:
		{
			//	GHook.Printf(" Reading motions: %d Elements of %d size\n", Chunk->Elements, Chunk->Size);
			NumGFXMotionBytes = Chunk->Elements;
			GFXMotionData = GE_RAM_ALLOCATE_ARRAY(uint8,NumGFXMotionBytes);
			if (!ReadChunkData(Chunk, GFXMotionData, f))
				return GE_FALSE;
			break;
		}
		case GBSP_CHUNK_END:
		{
			break;
		}
		default:
			return GE_FALSE;
	}

	return GE_TRUE;
}

//========================================================================================
//	LoadGBSPFile
//========================================================================================
geBoolean LoadGBSPFile(char *FileName)
{
	geVFile		*f;
	GBSP_Chunk	Chunk;

	f = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, FileName, NULL, GE_VFILE_OPEN_READONLY);

	if (!f)
		return GE_FALSE;

	while (1)
	{
		if (!ReadChunk(&Chunk, f))
			return GE_FALSE;
		
		if (Chunk.Type == GBSP_CHUNK_END)
			break;
	}

	return GE_TRUE;
}

//========================================================================================
//	FreeGBSPFile
//========================================================================================
geBoolean FreeGBSPFile(void)
{
	if (GFXModels)
		geRam_Free(GFXModels);
	if (GFXNodes)
		geRam_Free(GFXNodes);
	if (GFXBNodes)
		geRam_Free(GFXBNodes);
	if (GFXLeafs)
		geRam_Free(GFXLeafs);
	if (GFXClusters)			// CHANGE: CLUSTER
		geRam_Free(GFXClusters);
	if (GFXAreas)
		geRam_Free(GFXAreas);
	if (GFXPortals)
		geRam_Free(GFXPortals);
	if (GFXPlanes)
		geRam_Free(GFXPlanes);
	if (GFXFaces)
		geRam_Free(GFXFaces);
	if (GFXLeafFaces)
		geRam_Free(GFXLeafFaces);
	if (GFXLeafSides)
		geRam_Free(GFXLeafSides);
	if (GFXVerts)
		geRam_Free(GFXVerts);
	if (GFXVertIndexList)
		geRam_Free(GFXVertIndexList);
	if (GFXRGBVerts)
		geRam_Free(GFXRGBVerts);
	if (GFXTextures)
		geRam_Free(GFXTextures);
	if (GFXTexInfo)
		geRam_Free(GFXTexInfo);
	if (GFXTexData)
		geRam_Free(GFXTexData);
	if (GFXEntData)
		geRam_Free(GFXEntData);
	if (GFXLightData)
		geRam_Free(GFXLightData);
	if (GFXVisData)
		geRam_Free(GFXVisData);
	if (GFXPalettes)
		geRam_Free(GFXPalettes);
	if (GFXMotionData)
		geRam_Free(GFXMotionData);

	GFXModels = NULL;
	GFXNodes = NULL;
	GFXBNodes = NULL;
	GFXLeafs = NULL;
	GFXClusters = NULL;		// CHANGE: CLUSTER
	GFXAreas = NULL;
	GFXPlanes = NULL;
	GFXFaces = NULL;
	GFXLeafFaces = NULL;
	GFXLeafSides = NULL;
	GFXVerts = NULL;
	GFXVertIndexList = NULL;
	GFXRGBVerts = NULL;
	GFXEntData = NULL;
	
	GFXTextures = NULL;
	GFXTexInfo = NULL;
	GFXTexData = NULL;
	GFXPalettes = NULL;
	GFXMotionData = NULL;

	GFXLightData = NULL;
	GFXVisData = NULL;
	GFXPortals = NULL;

	NumGFXModels = 0;
	NumGFXNodes = 0;
	NumGFXBNodes = 0;
	NumGFXLeafs = 0;
	NumGFXClusters = 0;		// CHANGE: CLUSTER
	NumGFXAreas = 0;
	NumGFXPlanes = 0;
	NumGFXFaces = 0;
	NumGFXLeafFaces = 0;
	NumGFXLeafSides = 0;
	NumGFXVerts = 0;
	NumGFXVertIndexList = 0;
	NumGFXRGBVerts = 0;

	NumGFXEntData = 0;
	NumGFXTexInfo = 0;
	NumGFXTextures = 0;
	NumGFXTexData = 0;
	NumGFXPalettes = 0;

	NumGFXMotionBytes = 0;

	NumGFXLightData = 0;
	NumGFXVisData = 0;
	NumGFXPortals = 0;

	return GE_TRUE;
}

//================================================================================
//	WriteChunks
//================================================================================
geBoolean WriteChunks(GBSP_ChunkData *Data, int32 NumChunkData, geVFile *f)
{
	int32		i;
	GBSP_Chunk	Chunk;

	for (i=0; i< NumChunkData; i++)
	{
		Chunk.Type = Data[i].Type;
		Chunk.Size = Data[i].Size;
		Chunk.Elements = Data[i].Elements;
		if (!WriteChunk(&Chunk, Data[i].Data, f))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//================================================================================
//	SaveGBSPFile
//================================================================================
geBoolean SaveGBSPFile(char *FileName)
{
	GBSP_ChunkData	CurrentChunkData[] = {
		{ GBSP_CHUNK_HEADER			, sizeof(GBSP_Header)	,1					, &GBSPHeader},
		{ GBSP_CHUNK_MODELS			, sizeof(GFX_Model)		,NumGFXModels		, GFXModels },
		{ GBSP_CHUNK_NODES			, sizeof(GFX_Node)		,NumGFXNodes		, GFXNodes  },
		{ GBSP_CHUNK_PORTALS		, sizeof(GFX_Portal)	,NumGFXPortals		, GFXPortals},
		{ GBSP_CHUNK_BNODES			, sizeof(GFX_BNode)		,NumGFXBNodes		, GFXBNodes },
		{ GBSP_CHUNK_PLANES			, sizeof(GFX_Plane)		,NumGFXPlanes		, GFXPlanes },
		{ GBSP_CHUNK_FACES			, sizeof(GFX_Face)		,NumGFXFaces		, GFXFaces  },
		{ GBSP_CHUNK_AREAS			, sizeof(GFX_Area)		,NumGFXAreas		, GFXAreas  },
		{ GBSP_CHUNK_AREA_PORTALS	, sizeof(GFX_AreaPortal),NumGFXAreaPortals	, GFXAreaPortals  },
		{ GBSP_CHUNK_LEAF_FACES		, sizeof(int32)			,NumGFXLeafFaces	, GFXLeafFaces  },
		{ GBSP_CHUNK_LEAF_SIDES		, sizeof(GFX_LeafSide)	,NumGFXLeafSides	, GFXLeafSides  },
		{ GBSP_CHUNK_VERTS			, sizeof(geVec3d)		,NumGFXVerts		, GFXVerts  },
		{ GBSP_CHUNK_VERT_INDEX		, sizeof(int32)			,NumGFXVertIndexList , GFXVertIndexList},
		{ GBSP_CHUNK_RGB_VERTS		, sizeof(geVec3d)		,NumGFXRGBVerts		, GFXRGBVerts  },
		{ GBSP_CHUNK_ENTDATA		, sizeof(uint8)			,NumGFXEntData		, GFXEntData},
		{ GBSP_CHUNK_TEXTURES		, sizeof(GFX_Texture)	,NumGFXTextures		, GFXTextures},
		{ GBSP_CHUNK_TEXINFO		, sizeof(GFX_TexInfo)	,NumGFXTexInfo		, GFXTexInfo},
		{ GBSP_CHUNK_TEXDATA		, sizeof(uint8)			,NumGFXTexData		, GFXTexData},
		{ GBSP_CHUNK_LIGHTDATA		, sizeof(uint8)			,NumGFXLightData	, GFXLightData},
 		{ GBSP_CHUNK_LEAFS			, sizeof(GFX_Leaf)		,NumGFXLeafs		, GFXLeafs  },
		{ GBSP_CHUNK_CLUSTERS		, sizeof(GFX_Cluster)	,NumGFXClusters		, GFXClusters},
		{ GBSP_CHUNK_VISDATA		, sizeof(uint8)			,NumGFXVisData		, GFXVisData},
		{ GBSP_CHUNK_SKYDATA		, sizeof(GFX_SkyData)	,1					, &GFXSkyData},
		{ GBSP_CHUNK_PALETTES		, sizeof(DRV_Palette)	,NumGFXPalettes		, GFXPalettes},
		{ GBSP_CHUNK_MOTIONS		, sizeof(uint8)			,NumGFXMotionBytes	, GFXMotionData},
		{ GBSP_CHUNK_END			, 0						,0					,NULL },
	};

	geVFile	*f;

	f = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, FileName, NULL, GE_VFILE_OPEN_CREATE);

	if (!f)
		return GE_FALSE;

	if (!WriteChunks(CurrentChunkData, sizeof(CurrentChunkData) / sizeof(CurrentChunkData[0]), f))
	{
		geVFile_Close(f);
		return GE_FALSE;
	}

	geVFile_Close(f);

	return GE_TRUE;
}

