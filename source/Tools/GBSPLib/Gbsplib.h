/****************************************************************************************/
/*  GBSPLib.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: API to GBSPLIB                                                         */
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
#ifndef GBSPLIB_H
#define GBSPLIB_H

#include <Windows.h>

#include "MathLib.h"

#include "Vec3d.h"

#define GBSP_VERSION_MAJOR	6
#define GBSP_VERSION_MINOR	0

//#define SHOW_DEBUG_STATS

//====================================================================================
//	Main driver interfaces
//====================================================================================
#define DllImport	extern "C" __declspec( dllimport )
#define DllExport	extern "C" __declspec( dllexport )

typedef void ERROR_CB(char *String, ...);
typedef void PRINTF_CB(char *String, ...);

typedef struct
{
	geBoolean	Verbose;
	geBoolean	EntityVerbose;

} BspParms;

typedef struct
{
	geBoolean	Verbose;
	geBoolean	ExtraSamples;
	float		LightScale;
	geBoolean	Radiosity;
	int32		NumBounce;
	float		PatchSize;
	geBoolean	FastPatch;
	float		ReflectiveScale;

	geVec3d	MinLight;			// R,G,B (XYZ) min color for each faces lightmap

} LightParms;

typedef struct
{
	geBoolean	Verbose;
	geBoolean	FullVis;
	geBoolean	SortPortals;

} VisParms;

typedef enum
{
	GBSP_ERROR,
	GBSP_OK,
	GBSP_CANCEL
} GBSP_RETVAL;

extern geBoolean	CancelRequest;		// Global cancel request...

typedef GBSP_RETVAL GBSP_CREATE_BSP(char *MapText, BspParms *Parms);
typedef GBSP_RETVAL GBSP_S_FILE(char *FileName);
typedef void GBSP_FREE_BSP(void);
typedef GBSP_RETVAL GBSP_VIS_FILE(char *FileName, VisParms *Parms);
typedef GBSP_RETVAL GBSP_LIGHT_FILE(char *FileName, LightParms *Parms);
typedef geBoolean GBSP_CANCEL_COMPILE(void);
typedef geBoolean GBSP_UPDATE_ENTITIES(char *MapName, char *BSPName);

typedef struct
{
	ERROR_CB		*Error;			// This must be set by caller
	PRINTF_CB		*Printf;		// This too

} GBSP_Hook;

typedef struct
{
	int32					VersionMajor;			// Major changes, like function parms...
	int32					VersionMinor;			// Internal changes, that do not effect caller
	GBSP_CREATE_BSP			*GBSP_CreateBSP;
	GBSP_S_FILE				*GBSP_SaveGBSPFile;
	GBSP_FREE_BSP			*GBSP_FreeBSP;
	GBSP_VIS_FILE			*GBSP_VisGBSPFile;
	GBSP_LIGHT_FILE			*GBSP_LightGBSPFile;
	GBSP_CANCEL_COMPILE		*GBSP_Cancel;
	GBSP_UPDATE_ENTITIES	*GBSP_UpdateEntities;
} GBSP_FuncHook;

extern GBSP_Hook GHook;

typedef GBSP_FuncHook *GBSP_INIT(GBSP_Hook *Hook);

#endif


