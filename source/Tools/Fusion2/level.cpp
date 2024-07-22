/****************************************************************************************/
/*  level.cpp                                                                           */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird                                                */
/*  Description:  Editor level structure                                                */
/*                                                                                      */
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
#include "stdafx.h"
#include "level.h"
#include "Parse3dt.h"
#include "EntTypeName.h"
#include <assert.h>
#include "RAM.H"
#include "units.h"
#include "util.h"
#include "Filepath.h"

#define NUM_VIEWS (4)
struct tag_Level
{
    BrushList *Brushes;
	CEntityArray *Entities;
    char *WadPath;
	char *HeadersDir;
	EntTypeNameList	*EntTypeNames;
	GroupListType *Groups;
	SizeInfo	*WadSizeInfos;
	CWadFile	*WadFile;
	EntityTable	*pEntityDefs;

	ModelInfo_Type	ModelInfo;

	SkyFaceTexture SkyFaces[6];
	geVec3d SkyRotationAxis;
	geFloat SkyRotationSpeed;
	geFloat	SkyTextureScale;
	
	// level edit settings
	CompileParamsType CompileParams;
	int GroupVisSetting;
	EntityViewList *pEntityView;

	GridInfo GridSettings;
	geBoolean BspRebuildFlag;
	ViewStateInfo ViewInfo[NUM_VIEWS];

	BrushTemplate_Arch ArchTemplate;
	BrushTemplate_Box	BoxTemplate;
	BrushTemplate_Cone	ConeTemplate;
	BrushTemplate_Cylinder CylinderTemplate;
	BrushTemplate_Spheroid	SpheroidTemplate;
	BrushTemplate_Staircase StaircaseTemplate;

	geVec3d TemplatePos;

	float DrawScale;		// default draw scale
	float LightmapScale;	// default lightmap scale
};


EntityViewList *Level_GetEntityVisibilityInfo (Level *pLevel)
{
	return pLevel->pEntityView;
}

void Level_SetGroupVisibility (Level *pLevel, int Setting)
{
	assert ((Setting == Group_ShowAll) ||
			(Setting == Group_ShowVisible) ||
			(Setting == Group_ShowCurrent));
	assert (pLevel != NULL);

	pLevel->GroupVisSetting = Setting;
}

int Level_GetGroupVisibility (const Level *pLevel)
{
	assert (pLevel != NULL);

	return pLevel->GroupVisSetting;
}

static void Level_AssignEntityName (Level *pLevel, CEntity *pEnt)
{
	CString EntityClassname;
	CString NewName;
	int Num;

	EntityClassname = pEnt->GetClassname ();
	Num = EntTypeNameList_UpdateCount (pLevel->EntTypeNames, EntityClassname);
	NewName.Format ("%s%d", EntityClassname, Num);
	pEnt->SetKeyValue ("%name%", NewName);
}

static geBoolean Level_LoadEntities
	(
	  Level *pLevel,
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	)
{
	int EntityCount, i;
	
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "Class"))) return GE_FALSE;
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "CEntList"))) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "EntCount"), &EntityCount)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "CurEnt"), &i)) return GE_FALSE;	

	for (i=0; i < EntityCount; i++)
	{
		CEntity ent;
		CString Value;

		if (!ent.ReadFromFile (Parser, VersionMajor, VersionMinor, Expected, pLevel->pEntityDefs)) return GE_FALSE;
		ent.DeSelect ();  // don't want it selected on load...
		if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "End"))) return GE_FALSE;
		if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "CEntity"))) return GE_FALSE;

		if( ent.IsCamera() == GE_FALSE )	// Exclude cameras
		{
			if (ent.GetKeyValue ("%name%", Value))
			{
				EntTypeNameList_UpdateCount (pLevel->EntTypeNames, Value);
			}
			else
			{
				Level_AssignEntityName (pLevel, &ent);
			}
			pLevel->Entities->Add (ent);
		}
	}
	return GE_TRUE;
}

static geBoolean Level_SaveEntities (CEntityArray *Entities, FILE *f)
{
	int i;
	int NumEntities;

	assert (Entities != NULL);
	assert (f != NULL);

	NumEntities = Entities->GetSize ();
	if (fprintf(f, "Class CEntList\nEntCount %d\n", NumEntities) < 0) return GE_FALSE;
	if (fprintf(f, "CurEnt 0\n") < 0) return GE_FALSE;

	for(i=0;i < NumEntities;i++)
	{
		if (!(*Entities)[i].SaveToFile (f)) return GE_FALSE;
		if (fprintf(f, "End CEntity\n") < 0) return GE_FALSE;
	}
	return GE_TRUE;
}


static void Level_UnloadEntityDefs (Level *pLevel)
{
	if (pLevel->pEntityDefs != NULL)
	{
		EntityTable_Destroy (&pLevel->pEntityDefs);
	}
	if (pLevel->HeadersDir != NULL)
	{
		geRam_Free (pLevel->HeadersDir);
	}
}

geBoolean Level_LoadEntityDefs (Level *pLevel, const char *HeadersDir)
{
	Level_UnloadEntityDefs (pLevel);
	pLevel->HeadersDir = Util_Strdup (HeadersDir);
	pLevel->pEntityDefs = EntityTable_Create (pLevel->HeadersDir);
	if (pLevel->pEntityDefs == NULL)
	{
		return GE_FALSE;
	}
	return GE_TRUE;
}

const EntityTable *Level_GetEntityDefs (const Level *pLevel)
{
	return pLevel->pEntityDefs;
}

Level *Level_Create (const char *pWadName, const char *HeadersDir)
{
	Level *pLevel;

	pLevel = GE_RAM_ALLOCATE_STRUCT (Level);
	if (pLevel != NULL)
	{
		pLevel->Brushes = BrushList_Create ();
		if (pLevel->Brushes == NULL) goto CreateError;

		pLevel->Entities = new (CEntityArray);
		if (pLevel->Entities == NULL) goto CreateError;

		pLevel->Entities->SetSize (0, 20);

		pLevel->EntTypeNames = EntTypeNameList_Create ();
		if (pLevel->EntTypeNames == NULL) goto CreateError;

		pLevel->Groups = Group_CreateList ();
		if (pLevel->Groups == NULL) goto CreateError;

		{
			// add the default group
			Group *pGroup = Group_Create (0, "Default");
			if (pGroup != NULL)
			{
				GroupList_Add (pLevel->Groups, pGroup);
			}
		}


		pLevel->ModelInfo.CurrentModel = 0;
		pLevel->ModelInfo.Models = ModelList_Create ();
		if (pLevel->ModelInfo.Models == NULL) goto CreateError;

		pLevel->HeadersDir = NULL;
		pLevel->pEntityDefs = NULL;
		if (Level_LoadEntityDefs (pLevel, HeadersDir) == GE_FALSE)
		{
			goto CreateError;
		}

		pLevel->WadPath = Util_Strdup (pWadName);
		pLevel->WadFile = NULL;
		pLevel->WadSizeInfos = NULL;

		// initialize sky
		geVec3d_Set (&pLevel->SkyRotationAxis, 1.0f, 0.0f, 0.0f);
		pLevel->SkyRotationSpeed = 10.0f;
		pLevel->SkyTextureScale = 1.0f;
		for (int i = 0; i < 6; ++i)
		{
			pLevel->SkyFaces[i].TextureName = NULL;
			pLevel->SkyFaces[i].Apply = GE_FALSE;
		}

		// Set default compile dialog params
		pLevel->CompileParams.EntitiesOnly = GE_FALSE;
		pLevel->CompileParams.VisDetailBrushes = GE_FALSE;
		pLevel->CompileParams.DoVis = GE_TRUE;
		pLevel->CompileParams.DoLight = GE_TRUE;
		pLevel->CompileParams.RunBsp = GE_TRUE;
		pLevel->CompileParams.RunPreview = GE_TRUE;
		pLevel->CompileParams.UseMinLight = GE_TRUE;
		pLevel->CompileParams.SuppressHidden = GE_FALSE;
		pLevel->CompileParams.Filename[0] = '\0';

		pLevel->CompileParams.Light.Verbose = GE_FALSE;
		pLevel->CompileParams.Light.ExtraSamples = GE_FALSE;
		pLevel->CompileParams.Light.LightScale = 1.0f;
		pLevel->CompileParams.Light.Radiosity = GE_FALSE;
		pLevel->CompileParams.Light.NumBounce = 10;
		pLevel->CompileParams.Light.PatchSize = 128.0f;
		pLevel->CompileParams.Light.FastPatch = GE_FALSE;
		pLevel->CompileParams.Light.ReflectiveScale = 1.0f;
		geVec3d_Set (&pLevel->CompileParams.Light.MinLight, 128.0f, 128.0f, 128.0f);

		pLevel->CompileParams.Bsp.Verbose = GE_FALSE;
		pLevel->CompileParams.Bsp.EntityVerbose = GE_FALSE;

		pLevel->CompileParams.Vis.Verbose = GE_FALSE;
		pLevel->CompileParams.Vis.FullVis = GE_FALSE;
		pLevel->CompileParams.Vis.SortPortals = GE_TRUE;

		pLevel->GroupVisSetting = Group_ShowVisible;

		pLevel->pEntityView = EntityViewList_Create (pLevel->pEntityDefs);

		// grid settings
		//default to texel grid and snap
		//with rotational snap of 15
		{
			GridInfo *pGridInfo;

			pGridInfo = &pLevel->GridSettings;
			pGridInfo->UseGrid = GE_TRUE;
			pGridInfo->GridType = GridTexel;
			pGridInfo->SnapType = GridTexel;
			pGridInfo->MetricSnapSize = GridSize_Decimeter;
			pGridInfo->TexelSnapSize = 8;
			pGridInfo->RotationSnap = 15;
		}

		pLevel->BspRebuildFlag = GE_TRUE;
		for (int iView = 0; iView < NUM_VIEWS; ++iView)
		{
			ViewStateInfo *pInfo;

			pInfo = &pLevel->ViewInfo[iView];
			pInfo->IsValid = GE_FALSE;
			pInfo->ZoomFactor = 1.0f;
			geVec3d_Clear (&pInfo->PitchRollYaw);
			geVec3d_Set (&pInfo->CameraPos, 0.0f, 0.0f, 0.0f);
		}

		BrushTemplate_ArchDefaults (&pLevel->ArchTemplate);
		BrushTemplate_BoxDefaults (&pLevel->BoxTemplate);
		BrushTemplate_ConeDefaults (&pLevel->ConeTemplate);
		BrushTemplate_CylinderDefaults (&pLevel->CylinderTemplate);
		BrushTemplate_SpheroidDefaults (&pLevel->SpheroidTemplate);
		BrushTemplate_StaircaseDefaults (&pLevel->StaircaseTemplate);

		geVec3d_Clear (&pLevel->TemplatePos);

		pLevel->DrawScale = 1.0f;
		pLevel->LightmapScale = 2.0f;
	}

	return pLevel;

CreateError :
	Level_Destroy (&pLevel);
	return pLevel;
}

geBoolean Level_LoadWad (Level *pLevel)
{
	// get rid of the old wad...
	Level_UnloadWad (pLevel);

	pLevel->WadFile = new CWadFile;
	if (pLevel->WadFile == NULL)
	{
		return GE_FALSE;
	}

	if (pLevel->WadFile->Setup (pLevel->WadPath))
	{
		pLevel->WadSizeInfos = (SizeInfo *)geRam_Allocate(sizeof(SizeInfo)*pLevel->WadFile->mBitmapCount);

		if (pLevel->WadSizeInfos != NULL)
		{
			int i;

			for (i = 0; i < pLevel->WadFile->mBitmapCount;i++)
			{
				SizeInfo *pInfo;
				WadFileEntry *Entry;

				pInfo = &(pLevel->WadSizeInfos[i]);
				Entry = &(pLevel->WadFile->mBitmaps[i]);

				pInfo->TexWidth		=Entry->Width;
				pInfo->TexHeight	=Entry->Height;
				pInfo->TexData		=(uint8 *)Entry->BitsPtr;
			}
		}
	}

	return (pLevel->WadSizeInfos != NULL);
}

void Level_UnloadWad (Level *pLevel)
{
	if (pLevel->WadSizeInfos != NULL)
	{
		geRam_Free (pLevel->WadSizeInfos);
		pLevel->WadSizeInfos = NULL;
	}
	if (pLevel->WadFile != NULL)
	{
		delete pLevel->WadFile;
		pLevel->WadFile = NULL;
	}
}

void Level_Destroy (Level **ppLevel)
{
	Level *pLevel;

	assert (ppLevel != NULL);
	assert (*ppLevel != NULL);

	pLevel = *ppLevel;

	if (pLevel->Brushes != NULL)
	{
		BrushList_Destroy (&pLevel->Brushes);
	}
	if (pLevel->Entities != NULL)
	{
		delete pLevel->Entities;
	}
	if (pLevel->WadPath != NULL)
	{
		geRam_Free (pLevel->WadPath);
	}
	if (pLevel->EntTypeNames != NULL)
	{
		EntTypeNameList_Destroy (&pLevel->EntTypeNames);
	}
	if (pLevel->Groups != NULL)
	{
		Group_DestroyList (&pLevel->Groups);
	}
	Level_UnloadEntityDefs (pLevel);
	Level_UnloadWad (pLevel);
	if (pLevel->ModelInfo.Models != NULL)
	{
		ModelList_Destroy (&pLevel->ModelInfo.Models);
	}
	// destroy sky...
	{
		int i;

		for (i = 0; i < 6; ++i)
		{
			SkyFaceTexture *sft;

			sft = &(pLevel->SkyFaces[i]);
			if (sft->TextureName != NULL)
			{
				geRam_Free (sft->TextureName);
			}
		}
	}

	if (pLevel->pEntityView != NULL)
	{
		EntityViewList_Destroy (&pLevel->pEntityView);
	}

	geRam_Free (pLevel);
	*ppLevel = NULL;
}

static uint16 Level_GetDibIdFromWad (const CWadFile *WadFile, const char *Name)
{
	uint16	i;

	for(i=0;i < WadFile->mBitmapCount;i++)
	{
		if(strcmpi(WadFile->mBitmaps[i].Name, Name) == 0)
		{
			return i;
		}
	}

	return 0xffff;
}

uint16 Level_GetDibId (const Level *pLevel, const char *Name)
{
	return Level_GetDibIdFromWad (pLevel->WadFile, Name);
}


WadFileEntry *Level_GetWadBitmap (Level *pLevel, const char *Name)
{
	uint16 i;

	i = Level_GetDibIdFromWad (pLevel->WadFile, Name);
	if (i != 0xffff)
	{
		return &(pLevel->WadFile->mBitmaps[i]);
	}
	else
	{
		return NULL;
	}
}

CompileParamsType *Level_GetCompileParams (Level *pLevel)
{
	return &(pLevel->CompileParams);
}


geBoolean Level_FaceFixupCallback (Brush *pBrush, void *lParam)
{
	int i;
	Level *pLevel;

	pLevel = (Level *)lParam;
	// set up the dib ids in the brush faces...
	for (i = 0;i < Brush_GetNumFaces(pBrush); i++)
	{
		Face	*f;

		f	=Brush_GetFace(pBrush, i);
		Face_SetTextureDibId(f, Level_GetDibId(pLevel, Face_GetTextureName(f)));
	}
	return GE_TRUE;
}

static void Level_BrushListToTexels(Level *pLevel)
{
	int				i;
	Brush			*cb;
	BrushIterator	bi;
	Face			*f;
	geFloat			uscale, vscale;

	cb = BrushList_GetFirst (pLevel->Brushes, &bi);
	while (cb != NULL)
	{
		for(i=0;i < Brush_GetNumFaces(cb);i++)
		{
			f	=Brush_GetFace(cb, i);
			Face_GetTextureScale(f, &uscale, &vscale);
			Face_SetTextureScale(f, CENTIMETERS_TO_ENGINE(uscale), CENTIMETERS_TO_ENGINE(vscale));
		}
		cb = BrushList_GetNext (&bi);
	}
}

#pragma warning (disable:4100)
static geBoolean Level_LoadSky 
	(
	  Level *pLevel,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	int i;
	char BigString[SCANNER_MAXDATA];
	int Apply;
		
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "Sky"))) return GE_FALSE;

	for (i = 0; i < 6; ++i)
	{
		*Expected = "Integer";
		if (!Parse3dt_GetInt (Parser, NULL, &Apply)) return GE_FALSE;
		pLevel->SkyFaces[i].Apply = (Apply) ? GE_TRUE : GE_FALSE;

		*Expected = "String";
		if ((VersionMajor == 1) && (VersionMinor < 14))
		{
			// first character of the string is x
			if (!Parse3dt_GetIdentifier (Parser, NULL, BigString)) return GE_FALSE;
			pLevel->SkyFaces[i].TextureName = Util_Strdup (&BigString[1]);
		}
		else
		{
			if (!Parse3dt_GetLiteral (Parser, NULL, BigString)) return GE_FALSE;
			pLevel->SkyFaces[i].TextureName = Util_Strdup (BigString);
		}
	}
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 26)))
	{
		if (!Parse3dt_GetVec3d (Parser, "Axis", &pLevel->SkyRotationAxis)) return GE_FALSE;
		if (!Parse3dt_GetFloat (Parser, "Speed", &pLevel->SkyRotationSpeed)) return GE_FALSE;
	}
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 28)))
	{
		if (!Parse3dt_GetFloat (Parser, "Scale", &pLevel->SkyTextureScale)) return GE_FALSE;
	}
	return GE_TRUE;
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
static geBoolean Level_LoadCompileInfo
	(
	  Level *pLevel,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	CompileParamsType *pParms;
	LightParms *pLight;
	VisParms *pVis;
	BspParms *pBsp;

	pParms = &(pLevel->CompileParams);
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "CompileInfo"))) return GE_FALSE;
	if (!Parse3dt_GetLiteral (Parser, (*Expected = "Filename"), pParms->Filename)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Vis"), &pParms->DoVis)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Light"), &pParms->DoLight)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Bsp"), &pParms->RunBsp)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Preview"), &pParms->RunPreview)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "MinLight"), &pParms->UseMinLight)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "SuppressHidden"), &pParms->SuppressHidden)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "EntitiesOnly"), &pParms->EntitiesOnly)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "VisDetail"), &pParms->VisDetailBrushes)) return GE_FALSE;

	// light params
	pLight = &(pParms->Light);
	if (!Parse3dt_GetInt (Parser, (*Expected = "Verbose"), &pLight->Verbose)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "ExtraSamples"), &pLight->ExtraSamples)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "LightScale"), &pLight->LightScale)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Radiosity"), &pLight->Radiosity)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "NumBounce"), &pLight->NumBounce)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "PatchSize"), &pLight->PatchSize)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "FastPatch"), &pLight->FastPatch)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "ReflectScale"), &pLight->ReflectiveScale)) return GE_FALSE;
	if (!Parse3dt_GetVec3d (Parser, (*Expected = "MinLight"), &pLight->MinLight)) return GE_FALSE;

	// vis params
	pVis = &(pParms->Vis);
	if (!Parse3dt_GetInt (Parser, (*Expected = "VisVerbose"), &pVis->Verbose)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "FullVis"), &pVis->FullVis)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "SortPortals"), &pVis->SortPortals)) return GE_FALSE;

	// Bsp Params
	pBsp = &(pParms->Bsp);
	if (!Parse3dt_GetInt (Parser, (*Expected = "BspVerbose"), &pBsp->Verbose)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "EntityVerbose"), &pBsp->EntityVerbose)) return GE_FALSE;

	return GE_TRUE;
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
static geBoolean Level_LoadGridInfo
	(
	  Level *pLevel,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	GridInfo *pGridInfo = &pLevel->GridSettings;

	if (!Parse3dt_GetInt (Parser, (*Expected = "Grid"), &pGridInfo->UseGrid)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Type"), &pGridInfo->GridType)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Snap"), &pGridInfo->SnapType)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Metric"), &pGridInfo->MetricSnapSize)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Texel"), &pGridInfo->TexelSnapSize)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Rotation"), &pGridInfo->RotationSnap)) return GE_FALSE;

	// force texel grid and snap
	pGridInfo->GridType = GridTexel;
	pGridInfo->SnapType = GridTexel;

    return GE_TRUE;
}
#pragma warning (default:4100)

static char *ViewNames[NUM_VIEWS] = {"TexturedView", "TopView", "FrontView", "SideView"};

#pragma warning (disable:4100)
static geBoolean Level_LoadOneView
	(
	  ViewStateInfo *pViewInfo,
	  const char *ViewName,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_GetInt (Parser, (*Expected = ViewName), &pViewInfo->IsValid)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Zoom"), &pViewInfo->ZoomFactor)) return GE_FALSE;
	if (!Parse3dt_GetVec3d (Parser, (*Expected = "PitchRollYaw"), &pViewInfo->PitchRollYaw)) return GE_FALSE;
	if (!Parse3dt_GetVec3d (Parser, (*Expected = "CamPos"), &pViewInfo->CameraPos)) return GE_FALSE;
	return GE_TRUE;
}
#pragma warning (default:4100)

static geBoolean Level_LoadViewInfo
	(
	  Level *pLevel,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	int iView;

	for (iView = 0; iView < NUM_VIEWS; ++iView)
	{
		if (!Level_LoadOneView (&pLevel->ViewInfo[iView], ViewNames[iView], Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;
	}
	return GE_TRUE;
}

static geBoolean Level_LoadBrushTemplates
	(
	  Level *pLevel,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!BrushTemplate_LoadArch (&pLevel->ArchTemplate, Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;
	if (!BrushTemplate_LoadBox (&pLevel->BoxTemplate, Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;
	if (!BrushTemplate_LoadCone (&pLevel->ConeTemplate, Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;
	if (!BrushTemplate_LoadCylinder (&pLevel->CylinderTemplate, Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;
	if (!BrushTemplate_LoadSpheroid (&pLevel->SpheroidTemplate, Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;
	if (!BrushTemplate_LoadStaircase (&pLevel->StaircaseTemplate, Parser, VersionMajor, VersionMinor, Expected)) return GE_FALSE;

	return GE_TRUE;
}


Level *Level_CreateFromFile (const char *FileName, const char **ErrMsg, const char *DefaultHeadersDir)
{
	int NumModels;
	int VersionMajor, VersionMinor;
	int NumGroups = 0;
	int NumBrushes = 0;
	int NumEntities;
	Parse3dt *Parser;
	const char *Expected = "!*ERROR*!";
	int k;
	Level *pLevel = NULL;
	char WadPath[MAX_PATH];
	char HeadersDir[MAX_PATH];

	assert (FileName != NULL);

	Parser = Parse3dt_Create (FileName);
	if (Parser == NULL)
	{
		*ErrMsg = "Can't open file";
		return NULL;
	}

	Expected = "3dtVersion";
	if (!Parse3dt_GetVersion (Parser, &VersionMajor, &VersionMinor)) goto DoneLoad;

	if(VersionMajor > LEVEL_VERSION_MAJOR)
	{
		*ErrMsg = "Version mismatch.";
		return NULL ;
	}

	if	(VersionMajor <= 1 && VersionMinor < 16)
	{
		char	PalPath[_MAX_PATH];

		if (!Parse3dt_GetPath (Parser, (Expected = "PalLoc"), PalPath)) goto DoneLoad;
	}

	// texture library path
	if ((VersionMajor <= 1) && (VersionMinor < 18))
	{
		if (!Parse3dt_GetPath (Parser, (Expected = "WadLoc"), WadPath)) goto DoneLoad;
	}
	else
	{
		if (!Parse3dt_GetLiteral (Parser, (Expected = "TextureLib"), WadPath)) goto DoneLoad;
	}

	// headers directory
	if ((VersionMajor <= 1) && (VersionMinor < 31))
	{
		strcpy (HeadersDir, DefaultHeadersDir);
	}
	else
	{
		if (!Parse3dt_GetLiteral (Parser, (Expected = "HeadersDir"), HeadersDir)) goto DoneLoad;
	}

	pLevel = Level_Create (WadPath, HeadersDir);
	if (pLevel == NULL)
	{
		*ErrMsg = "Error creating level.";
		return NULL;
	}


	if ((VersionMajor == 1) && (VersionMinor < 15))
	{
		if (!Parse3dt_GetInt  (Parser, (Expected = "NumBrushes"), &NumBrushes)) goto DoneLoad;
	}
	if (!Parse3dt_GetInt  (Parser, (Expected = "NumEntities"), &NumEntities)) goto DoneLoad;
	if (!Parse3dt_GetInt  (Parser, (Expected = "NumModels"), &NumModels)) goto DoneLoad;

	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 3)))
	{
		if (!Parse3dt_GetInt (Parser, (Expected = "NumGroups"), &NumGroups)) goto DoneLoad;
	}


	if ((VersionMajor == 1) && (VersionMinor < 15))
	{
		for (k = 0; k < NumBrushes; k++)
		{
			Brush *pBrush;

			pBrush	=Brush_CreateFromFile(Parser, VersionMajor, VersionMinor, &Expected);
			if (pBrush == NULL) goto DoneLoad;
			BrushList_Append (pLevel->Brushes, pBrush);
		}
	}
	else
	{
		if (pLevel->Brushes != NULL)
		{
			BrushList_Destroy (&pLevel->Brushes);
		}
		pLevel->Brushes = BrushList_CreateFromFile (Parser, VersionMajor, VersionMinor, &Expected);
		if (pLevel->Brushes == NULL)
			goto DoneLoad;
	}

	if((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor < 6)))
	{
		Level_BrushListToTexels (pLevel);
	}

	if (!Level_LoadEntities (pLevel, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;

	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 2)))
	{
		if (!ModelList_Read (pLevel->ModelInfo.Models, NumModels, Parser, VersionMajor, VersionMinor, &Expected))
		{
			goto DoneLoad;
		}
	}

	// collapse model list so numbers are consecutive
	Level_CollapseModels (pLevel, 1);

	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 3)))
	{
		if (!Group_ReadList (pLevel->Groups, NumGroups, Parser, VersionMajor, VersionMinor, &Expected ))
		{
			goto DoneLoad;
		}
	}
	// collapse the group list so numbers are consecutive
	Level_CollapseGroups (pLevel, 1);

	// load sky information...
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 12)))
	{
		if (!Level_LoadSky (pLevel, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;
	}

	// load compile information and other editing settings
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 20)))
	{
		if (!Level_LoadCompileInfo (pLevel, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;
		if (!Parse3dt_GetInt (Parser, (Expected = "ShowGroups"), &pLevel->GroupVisSetting)) goto DoneLoad;
		if (!EntityViewList_LoadSettings (pLevel->pEntityView, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;
		if (!Level_LoadGridInfo (pLevel, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;
		if (!Parse3dt_GetInt (Parser, (Expected = "BspRebuild"), &pLevel->BspRebuildFlag)) goto DoneLoad;
		if (!Level_LoadViewInfo (pLevel, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;
		if (!Level_LoadBrushTemplates (pLevel, Parser, VersionMajor, VersionMinor, &Expected)) goto DoneLoad;
		if (!Parse3dt_GetVec3d (Parser, (Expected = "TemplatePos"), &pLevel->TemplatePos)) goto DoneLoad;
	}

	// load level settings
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 22)))
	{
		if (!Parse3dt_GetFloat (Parser, (Expected = "DrawScale"), &pLevel->DrawScale)) goto DoneLoad;
		if (!Parse3dt_GetFloat (Parser, (Expected = "LightmapScale"), &pLevel->LightmapScale)) goto DoneLoad;
	}

	goto AllDone;

DoneLoad:
	*ErrMsg = Parse3dt_Error (Parser, "Expected %s", Expected);

//DoneLoad1:
	if (pLevel != NULL)
	{
		Level_Destroy (&pLevel);
	}

AllDone:
	if (Parser != NULL)
	{
		Parse3dt_Destroy (&Parser);
	}

	//fixup hollows
	if(pLevel != NULL)
	{
		BrushList_MakeHollowsMulti(pLevel->Brushes);
	}

	return pLevel;
}



static geBoolean Level_WriteSky
	(
	  SkyFaceTexture const SkyFaces[],
	  geVec3d const *Axis,
	  const float Speed,
	  const float Scale,
	  FILE *ArFile
	)
{
	int i;

	if (fprintf (ArFile, "%s\n", "Sky") < 0) return GE_FALSE;
	for (i = 0; i < 6; ++i)
	{
		char QuotedValue[SCANNER_MAXDATA];
		char const *StringToQuote;

		StringToQuote = "";
		if (SkyFaces[i].TextureName != NULL)
		{
			StringToQuote = SkyFaces[i].TextureName;
		}
		Util_QuoteString (StringToQuote, QuotedValue);
		if (fprintf (ArFile, "%d %s\n", SkyFaces[i].Apply, QuotedValue) < 0) return GE_FALSE;
	}

	// Write rotation axis and speed
	if (fprintf (ArFile, "Axis %f %f %f\n", Axis->X, Axis->Y, Axis->Z) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Speed %f\n", Speed) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Scale %f\n", Scale) < 0) return GE_FALSE;

	return GE_TRUE;
}

static geBoolean Level_WriteCompileInfo 
	(
	  const CompileParamsType *pParms,
	  FILE *ArFile
	)
{
	char QuotedString[MAX_PATH];
	const LightParms *pLight;
	const VisParms *pVis;
	const BspParms *pBsp;

	if (fprintf (ArFile, "%s\n", "CompileInfo") < 0) return GE_FALSE;
	Util_QuoteString (pParms->Filename, QuotedString);
	if (fprintf (ArFile, "Filename %s\n", QuotedString) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Vis %d\n", pParms->DoVis) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Light %d\n", pParms->DoLight) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Bsp %d\n", pParms->RunBsp) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Preview %d\n", pParms->RunPreview) < 0) return GE_FALSE;
	if (fprintf (ArFile, "MinLight %d\n", pParms->UseMinLight) < 0) return GE_FALSE;
	if (fprintf (ArFile, "SuppressHidden %d\n", pParms->SuppressHidden) < 0) return GE_FALSE;
	if (fprintf (ArFile, "EntitiesOnly %d\n", pParms->EntitiesOnly) < 0) return GE_FALSE;
	if (fprintf (ArFile, "VisDetail %d\n", pParms->VisDetailBrushes) < 0) return GE_FALSE;

	// light params
	pLight = &(pParms->Light);
	if (fprintf (ArFile, "Verbose %d\n", pLight->Verbose) < 0) return GE_FALSE;
	if (fprintf (ArFile, "ExtraSamples %d\n", pLight->ExtraSamples) < 0) return GE_FALSE;
	if (fprintf (ArFile, "LightScale %f\n", pLight->LightScale) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Radiosity %d\n", pLight->Radiosity) < 0) return GE_FALSE;
	if (fprintf (ArFile, "NumBounce %d\n", pLight->NumBounce) < 0) return GE_FALSE;
	if (fprintf (ArFile, "PatchSize %f\n", pLight->PatchSize) < 0) return GE_FALSE;
	if (fprintf (ArFile, "FastPatch %d\n", pLight->FastPatch) < 0) return GE_FALSE;
	if (fprintf (ArFile, "ReflectScale %f\n", pLight->ReflectiveScale) < 0) return GE_FALSE;
	if (fprintf (ArFile, "MinLight %f %f %f\n", pLight->MinLight.X, pLight->MinLight.Y, pLight->MinLight.Z) < 0) return GE_FALSE;

	// vis params
	pVis = &(pParms->Vis);
	if (fprintf (ArFile, "VisVerbose %d\n", pVis->Verbose) < 0) return GE_FALSE;
	if (fprintf (ArFile, "FullVis %d\n", pVis->FullVis) < 0) return GE_FALSE;
	if (fprintf (ArFile, "SortPortals %d\n", pVis->SortPortals) < 0) return GE_FALSE;

	// Bsp Params
	pBsp = &(pParms->Bsp);
	if (fprintf (ArFile, "BspVerbose %d\n", pBsp->Verbose) < 0) return GE_FALSE;
	if (fprintf (ArFile, "EntityVerbose %d\n", pBsp->EntityVerbose) < 0) return GE_FALSE;

	return GE_TRUE;
}

static geBoolean Level_WriteGridInfo 
	(
	  const GridInfo *pGridInfo,
	  FILE *ArFile
	)
{
	if (fprintf (ArFile, "Grid %d\n", pGridInfo->UseGrid) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Type %d\n", pGridInfo->GridType) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Snap %d\n", pGridInfo->SnapType) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Metric %d\n", pGridInfo->MetricSnapSize) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Texel %d\n", pGridInfo->TexelSnapSize) < 0) return GE_FALSE;
	if (fprintf (ArFile, "Rotation %d\n", pGridInfo->RotationSnap) < 0) return GE_FALSE;

	return GE_TRUE;
}

static geBoolean Level_WriteOneView
	(
	  const ViewStateInfo *pViewInfo,
	  const char *ViewName,
	  FILE *f
	)
{
	if (fprintf (f, "%s %d\n", ViewName, pViewInfo->IsValid) < 0) return GE_FALSE;
	if (fprintf (f, "Zoom %f\n", pViewInfo->ZoomFactor) < 0) return GE_FALSE;
	if (fprintf (f, "PitchRollYaw %f %f %f\n", pViewInfo->PitchRollYaw.X, pViewInfo->PitchRollYaw.Y, pViewInfo->PitchRollYaw.Z) < 0) return GE_FALSE;
	if (fprintf (f, "CamPos %f %f %f\n", pViewInfo->CameraPos.X, pViewInfo->CameraPos.Y, pViewInfo->CameraPos.Z) < 0) return GE_FALSE;
	return GE_TRUE;
}

static geBoolean Level_WriteViewInfo
	(
	  const ViewStateInfo pViewInfo[],
	  FILE *ArFile
	)
{
	int iView;

	for (iView = 0; iView < NUM_VIEWS; iView++)
	{
		if (!Level_WriteOneView (&pViewInfo[iView], ViewNames[iView], ArFile)) return GE_FALSE;
	}
	return GE_TRUE;
}

static geBoolean Level_WriteBrushTemplates
	(
	  const Level *pLevel,
	  FILE *f
	)
{
	if (BrushTemplate_WriteArch (&pLevel->ArchTemplate, f) == GE_FALSE) return GE_FALSE;
	if (BrushTemplate_WriteBox (&pLevel->BoxTemplate, f) == GE_FALSE) return GE_FALSE;
	if (BrushTemplate_WriteCone (&pLevel->ConeTemplate, f) == GE_FALSE) return GE_FALSE;
	if (BrushTemplate_WriteCylinder (&pLevel->CylinderTemplate, f) == GE_FALSE) return GE_FALSE;
	if (BrushTemplate_WriteSpheroid (&pLevel->SpheroidTemplate, f) == GE_FALSE) return GE_FALSE;
	if (BrushTemplate_WriteStaircase (&pLevel->StaircaseTemplate, f) == GE_FALSE) return GE_FALSE;

	return GE_TRUE;
}

geBoolean Level_WriteToFile (Level *pLevel, const char *Filename)
{
	FILE	*ArFile;
	char QuotedString[MAX_PATH];
	geBoolean WriteRslt;

	assert (pLevel != NULL);
	assert (Filename != NULL);

	// error checking required!
	ArFile = fopen(Filename, "wt");

	if (ArFile == NULL)
	{
		return GE_FALSE;
	}

	WriteRslt = GE_FALSE;
	if (fprintf(ArFile, "3dtVersion %d.%d\n", LEVEL_VERSION_MAJOR, LEVEL_VERSION_MINOR) < 0) goto WriteDone;

	Util_QuoteString (pLevel->WadPath, QuotedString);
	if (fprintf(ArFile, "TextureLib %s\n", QuotedString) < 0) goto WriteDone;

	Util_QuoteString (pLevel->HeadersDir, QuotedString);
	if (fprintf (ArFile, "HeadersDir %s\n", QuotedString) < 0) goto WriteDone;

	if (fprintf(ArFile, "NumEntities %d\n", pLevel->Entities->GetSize ()) < 0) goto WriteDone;
	if (fprintf(ArFile, "NumModels %d\n", ModelList_GetCount (pLevel->ModelInfo.Models)) < 0) goto WriteDone;
	if (fprintf(ArFile, "NumGroups %d\n", Group_GetCount (pLevel->Groups)) < 0) goto WriteDone;
	if (BrushList_Write (pLevel->Brushes, ArFile) == GE_FALSE) goto WriteDone;
	if (Level_SaveEntities (pLevel->Entities, ArFile) == GE_FALSE) goto WriteDone;
	if (ModelList_Write (pLevel->ModelInfo.Models, ArFile) == GE_FALSE) goto WriteDone;
	if (Group_WriteList (pLevel->Groups, ArFile) == GE_FALSE) goto WriteDone;
	if (Level_WriteSky (pLevel->SkyFaces, &pLevel->SkyRotationAxis, pLevel->SkyRotationSpeed, pLevel->SkyTextureScale, ArFile) == GE_FALSE) goto WriteDone;
	if (Level_WriteCompileInfo (&pLevel->CompileParams, ArFile) == GE_FALSE) goto WriteDone;
	if (fprintf (ArFile, "ShowGroups %d\n", pLevel->GroupVisSetting) < 0) goto WriteDone;
	if (EntityViewList_WriteToFile (pLevel->pEntityView, ArFile) == GE_FALSE) goto WriteDone;
	if (Level_WriteGridInfo (&pLevel->GridSettings, ArFile) == GE_FALSE) goto WriteDone;
	if (fprintf (ArFile, "BspRebuild %d\n", pLevel->BspRebuildFlag) < 0) goto WriteDone;
	if (Level_WriteViewInfo (pLevel->ViewInfo, ArFile) == GE_FALSE) goto WriteDone;
	if (Level_WriteBrushTemplates (pLevel, ArFile) == GE_FALSE) goto WriteDone;
	if (fprintf (ArFile, "TemplatePos %f %f %f\n", pLevel->TemplatePos.X, pLevel->TemplatePos.Y, pLevel->TemplatePos.Z) < 0) goto WriteDone;

	// level options
	if (fprintf (ArFile, "DrawScale %f\n", pLevel->DrawScale) < 0) goto WriteDone;
	if (fprintf (ArFile, "LightmapScale %f\n", pLevel->LightmapScale) < 0) goto WriteDone;


	WriteRslt = GE_TRUE;

WriteDone:
	if (fclose(ArFile) != 0) return GE_FALSE;

	return GE_TRUE ;
}

CEntityArray *Level_GetEntities (Level *pLevel)
{
	return pLevel->Entities;
}

BrushList *Level_GetBrushes (Level *pLevel)
{
	return pLevel->Brushes;
}

SkyFaceTexture *Level_GetSkyInfo (Level *pLevel, geVec3d *Axis, geFloat *Speed, geFloat *Scale)
{
	*Axis = pLevel->SkyRotationAxis;
	*Speed = pLevel->SkyRotationSpeed;
	*Scale = pLevel->SkyTextureScale;
	return pLevel->SkyFaces;
}


void Level_SetSkyRotationAxis (Level *pLevel, const geVec3d *Axis)
{
	pLevel->SkyRotationAxis = *Axis;
}

void Level_SetSkyRotationSpeed (Level *pLevel, const geFloat Speed)
{
	pLevel->SkyRotationSpeed = Speed;
}

void Level_SetSkyTextureScale (Level *pLevel, const geFloat Scale)
{
	pLevel->SkyTextureScale = Scale;
}

ModelInfo_Type *Level_GetModelInfo (Level *pLevel)
{
	assert (pLevel != NULL);

	return &(pLevel->ModelInfo);
}

Model *Level_GetModel (Level *pLevel, int ModelId)
{
	return ModelList_FindById (pLevel->ModelInfo.Models, ModelId);
}

void Level_AddModel (Level *pLevel, Model *pModel)
{
	ModelList_AddModel (pLevel->ModelInfo.Models, pModel);
}


GroupListType *Level_GetGroups (Level *pLevel)
{
	assert (pLevel != NULL);

	return pLevel->Groups;
}

Group *Level_GetGroup (Level *pLevel, int GroupId)
{
	assert (pLevel != NULL);
	
	return GroupList_GetFromId (pLevel->Groups, GroupId);
}

void Level_AddGroup (Level *pLevel, Group *pGroup)
{
	GroupList_Add (pLevel->Groups, pGroup);
}


const char *Level_GetWadPath (const Level *pLevel)
{
	assert (pLevel != NULL);

	return (pLevel->WadPath);
}

void Level_SetWadPath (Level *pLevel, const char *NewWad)
{
	if (pLevel->WadPath != NULL)
	{
		geRam_Free (pLevel->WadPath);
	}
	pLevel->WadPath = Util_Strdup (NewWad);
}


SizeInfo *Level_GetWadSizeInfos (Level *pLevel)
{
	assert (pLevel != NULL);

	return pLevel->WadSizeInfos;
}

CWadFile *Level_GetWadFile (Level *pLevel)
{
	return pLevel->WadFile;
}



int Level_AddEntity (Level *pLevel, CEntity &Entity)
{
	assert (pLevel != NULL);

	Level_AssignEntityName (pLevel, &Entity);
	return pLevel->Entities->Add (Entity);
}

void Level_AppendBrush (Level *pLevel, Brush *pBrush)
{
	assert (pLevel != NULL);
	assert (pBrush != NULL);

	BrushList_Append (pLevel->Brushes, pBrush);
}

void Level_RemoveBrush (Level *pLevel, Brush *pBrush)
{
	assert (pLevel != NULL);
	assert (pBrush != NULL);

	BrushList_Remove (pLevel->Brushes, pBrush);
}


int Level_EnumEntities (Level *pLevel, void *lParam, EntityList_CB Callback)
{
	assert (pLevel != NULL);

	return EntityList_Enum (*(pLevel->Entities), lParam, Callback);
}

int Level_EnumBrushes (Level *pLevel, void *lParam, BrushList_CB Callback)
{
	return BrushList_Enum (pLevel->Brushes, lParam, Callback);
}

int Level_EnumAllBrushes (Level *pLevel, void *lParam, BrushList_CB Callback)
{
	return BrushList_EnumAll (pLevel->Brushes, lParam, Callback);
}


int Level_EnumLeafBrushes (Level *pLevel, void *lParam, BrushList_CB Callback)
{
	return BrushList_EnumLeafBrushes (pLevel->Brushes, lParam, Callback);
}

void Level_TranslateAll (Level *pLevel, const geVec3d *VecXlate)
{
	int i;
	CEntityArray *Entities;

	assert (pLevel != NULL);

	// move all brushes
	BrushList_Move (pLevel->Brushes, VecXlate);
	// and entities
	Entities = pLevel->Entities;
	for (i = 0; i < Entities->GetSize (); ++i)
	{
		(*Entities)[i].Move (VecXlate);
	}

	// update the models' origins
	{
		Model *pModel;
		ModelIterator mi;

		pModel = ModelList_GetFirst (pLevel->ModelInfo.Models, &mi);
		while (pModel != NULL)
		{
			Model_UpdateOrigin (pModel, BRUSH_MOVE, VecXlate);
			pModel = ModelList_GetNext (pLevel->ModelInfo.Models, &mi);
		}
	}
}

void Level_CollapseGroups (Level *pLevel, int StartingGroup)
{
	assert (pLevel != NULL);

	GroupList_Collapse (pLevel->Groups, StartingGroup, pLevel->Brushes, pLevel->Entities);
}

void Level_CollapseModels (Level *pLevel, int StartingModel)
{
	assert (pLevel != NULL);

	ModelList_Collapse (pLevel->ModelInfo.Models, StartingModel, pLevel->Brushes);
}

float Level_GetGridSnapSize (const Level *pLevel)
{
	const GridInfo *pGridInfo = &pLevel->GridSettings;

	switch (pGridInfo->SnapType)
	{
		case GridMetric :
			return CENTIMETERS_TO_ENGINE (pGridInfo->MetricSnapSize);
			break;
		default :
			assert (0);
		case GridTexel :
			return (float)pGridInfo->TexelSnapSize;
			break;
	}
}

int Level_GetRotationSnap (const Level *pLevel)
{
	return pLevel->GridSettings.RotationSnap;
}

int Level_GetGridType (const Level *pLevel)
{
	return pLevel->GridSettings.GridType;
}

geBoolean Level_UseGrid (const Level *pLevel)
{
	return pLevel->GridSettings.UseGrid;
}

GridInfo *Level_GetGridInfo (Level *pLevel)
{
	return &pLevel->GridSettings;
}

geBoolean Level_RebuildBspAlways (const Level *pLevel)
{
	return pLevel->BspRebuildFlag;
}

void Level_SetBspRebuild (Level *pLevel, geBoolean RebuildFlag)
{
	pLevel->BspRebuildFlag = RebuildFlag;
}

ViewStateInfo *Level_GetViewStateInfo (Level *pLevel, int iView)
{
	assert (iView >= 0);
	assert (iView < 4);

	return &pLevel->ViewInfo[iView];
}

BrushTemplate_Arch *Level_GetArchTemplate (Level *pLevel)
{
	return &pLevel->ArchTemplate;
}

BrushTemplate_Box  *Level_GetBoxTemplate (Level *pLevel)
{
	return &pLevel->BoxTemplate;
}

BrushTemplate_Cone *Level_GetConeTemplate (Level *pLevel)
{
	return &pLevel->ConeTemplate;
}

BrushTemplate_Cylinder *Level_GetCylinderTemplate (Level *pLevel)
{
	return &pLevel->CylinderTemplate;
}

BrushTemplate_Spheroid *Level_GetSpheroidTemplate (Level *pLevel)
{
	return &pLevel->SpheroidTemplate;
}

BrushTemplate_Staircase *Level_GetStaircaseTemplate (Level *pLevel)
{
	return &pLevel->StaircaseTemplate;
}

geVec3d *Level_GetTemplatePos (Level *pLevel)
{
	return &pLevel->TemplatePos;
}

float Level_GetDrawScale (const Level *pLevel)
{
	return pLevel->DrawScale;
}

float Level_GetLightmapScale (const Level *pLevel)
{
	return pLevel->LightmapScale;
}

void Level_SetDrawScale (Level *pLevel, float Scale)
{
	pLevel->DrawScale = Scale;
}

void Level_SetLightmapScale (Level *pLevel, float Scale)
{
	pLevel->LightmapScale = Scale;
}

const char *Level_GetHeadersDirectory (const Level *pLevel)
{
	return pLevel->HeadersDir;
}
