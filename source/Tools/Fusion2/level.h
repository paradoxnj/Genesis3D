/****************************************************************************************/
/*  level.h                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Genesis world editor header file                                      */
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
#ifndef LEVEL_H
#define LEVEL_H

#include "entity.h"
#include "brush.h"
#include "model.h"
#include "group.h"
#include "wadfile.h"
#include "compiler.h"
#include "EntView.h"
#include "BrushTemplate.h"
#include "EntityTable.h"

#define LEVEL_VERSION_MAJOR	1
//#define LEVEL_VERSION_MINOR 16		// version 1.16 06/01/98 - eli - Removed palette dependencies (no more PalPath)
//#define LEVEL_VERSION_MINOR 17		// version 1.17 06/10/98 - jim - added LightXScale and LightYScale to face
//#define LEVEL_VERSION_MINOR 18		// version 1.18 06/17/98 - jim - read paths as literal strings
//#define LEVEL_VERSION_MINOR 19		// version 1.19 06/26/98 - jim - Quote group and model names
//#define LEVEL_VERSION_MINOR 20		// version 1.20 07/06/98 - jim - Store build and edit information in level file
//#define LEVEL_VERSION_MINOR 21		// VERSION 1.21 07/14/98 - jim - removed entity RenderOrigin field
//#define LEVEL_VERSION_MINOR 22		// version 1.22 07/27/98 - jim - Added level options (draw scale and lightmap scale)
//#define LEVEL_VERSION_MINOR 23		// version 1.23 08/20/98 - jim - Added hull thickness to cylinder template
//#define LEVEL_VERSION_MINOR 24		// version 1.24 08/26/98 - jim - Quoted texture name in brush and texinfo I/O
//#define LEVEL_VERSION_MINOR 25		// Version 1.25 10/06/98 - jim - Add support for Gouraud and flat shading faces
//#define LEVEL_VERSION_MINOR 26		// Version 1.26 10/14/98 - jim - Add sky box rotation axis and speed
//#define LEVEL_VERSION_MINOR 27		// Version 1.27 10/27/98 - Ken - New face flags and values
//#define LEVEL_VERSION_MINOR 28		// Version 1.28 10/27/98 - Ken - Sky texture scale
//#define LEVEL_VERSION_MINOR 29		// Version 1.29 12/09/98 - Jim - Added sheet brushes
//#define LEVEL_VERSION_MINOR 30		// Version 1.30 12/22/98 - Jim - Added face transparent flag
//#define LEVEL_VERSION_MINOR 31			// Version 1.31 01/05/99 - jim - Added headers directory
#define LEVEL_VERSION_MINOR 32			// Version 1.32 11/04/99 - Brian - Face Info save out Base Vec for Tex Lock

struct SkyFaceTexture
{
	char *TextureName;
	geBoolean Apply;
};

enum
{
	SkyFace_Left	= 0,
	SkyFace_Right	= 1,
	SkyFace_Top		= 2,
	SkyFace_Bottom	= 3,
	SkyFace_Front	= 4,
	SkyFace_Back	= 5
};

enum
{
	Group_ShowAll,
	Group_ShowVisible,
	Group_ShowCurrent
};

enum {GridMetric = 0, GridTexel = 1};

enum
{
	GridSize_Centimeter	=1,
	GridSize_Decimeter	=10,
	GridSize_Meter		=100
};

typedef struct
{
	geBoolean UseGrid;
	int GridType;

	int SnapType;
	int MetricSnapSize;
	int TexelSnapSize;
	int RotationSnap;
} GridInfo;

typedef struct
{
	geBoolean IsValid;
	float ZoomFactor;
	geVec3d PitchRollYaw;
	geVec3d CameraPos;
} ViewStateInfo;

typedef struct tag_Level Level;


Level *Level_Create (const char *DefaultWad, const char *HeadersDir);
Level *Level_CreateFromFile (const char *FileName, const char **ErrMsg, const char *DefaultHeadersDir);
void Level_Destroy (Level **ppLevel);

geBoolean Level_WriteToFile (Level *pLevel, const char *Filename);


CEntityArray *Level_GetEntities (Level *pLevel);
BrushList *Level_GetBrushes (Level *pLevel);
SkyFaceTexture *Level_GetSkyInfo (Level *pLevel, geVec3d *Axis, geFloat *Speed, geFloat *Scale);

void Level_SetSkyRotationAxis (Level *pLevel, const geVec3d *Axis);
void Level_SetSkyRotationSpeed (Level *pLevel, const geFloat Speed);
void Level_SetSkyTextureScale (Level *pLevel, const geFloat Scale);

ModelInfo_Type *Level_GetModelInfo (Level *pLevel);
Model *Level_GetModel (Level *pLevel, int ModelId);
void Level_AddModel (Level *pLevel, Model *pModel);

GroupListType *Level_GetGroups (Level *pLevel);
Group *Level_GetGroup (Level *pLevel, int GroupId);
void Level_AddGroup (Level *pLevel, Group *pGroup);

const char *Level_GetWadPath (const Level *pLevel);
SizeInfo *Level_GetWadSizeInfos (Level *pLevel);
CWadFile *Level_GetWadFile (Level *pLevel);

uint16 Level_GetDibId (const Level *pLevel, const char *Name);
WadFileEntry *Level_GetWadBitmap (Level *pLevel, const char *Name);

CompileParamsType *Level_GetCompileParams (Level *pLevel);

void Level_SetGroupVisibility (Level *pLevel, int Setting);
int Level_GetGroupVisibility (const Level *pLevel);
EntityViewList *Level_GetEntityVisibilityInfo (Level *pLevel);

void Level_SetWadPath (Level *pLevel, const char *NewWad);
geBoolean Level_LoadWad (Level *pLevel);
void Level_UnloadWad (Level *pLevel);

int Level_AddEntity (Level *pLevel, CEntity &Entity);
void Level_AppendBrush (Level *pLevel, Brush *pBrush);
void Level_RemoveBrush (Level *pLevel, Brush *pBrush);

int Level_EnumEntities (Level *pLevel, void *lParam, EntityList_CB Callback);
int Level_EnumBrushes (Level *pLevel, void *lParam, BrushList_CB Callback);
int Level_EnumAllBrushes (Level *pLevel, void *lParam, BrushList_CB Callback);
int Level_EnumLeafBrushes (Level *pLevel, void *lParam, BrushList_CB Callback);

geBoolean Level_FaceFixupCallback (Brush *pBrush, void *lParam);

void Level_TranslateAll (Level *pLevel, const geVec3d *VecXlate);
void Level_CollapseGroups (Level *pLevel, int StartingGroup);
void Level_CollapseModels (Level *pLevel, int StartingModel);

float Level_GetGridSnapSize (const Level *pLevel);
int Level_GetRotationSnap (const Level *pLevel);
int Level_GetGridType (const Level *pLevel);
geBoolean Level_UseGrid (const Level *pLevel);
GridInfo *Level_GetGridInfo (Level *pLevel);

geBoolean Level_RebuildBspAlways (const Level *pLevel);
void Level_SetBspRebuild (Level *pLevel, geBoolean RebuildFlag);

ViewStateInfo *Level_GetViewStateInfo (Level *pLevel, int iView);

BrushTemplate_Arch *Level_GetArchTemplate (Level *pLevel);
BrushTemplate_Box  *Level_GetBoxTemplate (Level *pLevel);
BrushTemplate_Cone *Level_GetConeTemplate (Level *pLevel);
BrushTemplate_Cylinder *Level_GetCylinderTemplate (Level *pLevel);
BrushTemplate_Spheroid *Level_GetSpheroidTemplate (Level *pLevel);
BrushTemplate_Staircase *Level_GetStaircaseTemplate (Level *pLevel);

geVec3d *Level_GetTemplatePos (Level *pLevel);

float Level_GetDrawScale (const Level *pLevel);
float Level_GetLightmapScale (const Level *pLevel);
void Level_SetDrawScale (Level *pLevel, float Scale);
void Level_SetLightmapScale (Level *pLevel, float Scale);

geBoolean Level_LoadEntityDefs (Level *pLevel, const char *HeadersDir);
const char *Level_GetHeadersDirectory (const Level *pLevel);
const EntityTable *Level_GetEntityDefs (const Level *pLevel);

#endif
