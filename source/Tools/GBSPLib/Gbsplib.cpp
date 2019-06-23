/****************************************************************************************/
/*  GBSPLib.cpp                                                                         */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>

#include "GBSPPrep.h"
#include "BSP.h"
#include "Vis.h"
#include "Light.h"
#include "Map.h"

#define HANDLE_EXCEPTIONS

GBSP_Hook GHook;

GBSP_RETVAL GBSP_CreateBSP(char *FileName);
GBSP_RETVAL GBSP_SaveGBSPFile(char *FileName);
void GBSP_FreeBSP(void);
GBSP_RETVAL GBSP_VisGBSPFile(char *FileName);
GBSP_RETVAL GBSP_LightGBSPFile(char *FileName, LightParms *Parms);
geBoolean GBSP_Cancel(void);
geBoolean GBSP_UpdateEntities(char *MapName, char *BSPName);

geBoolean	CancelRequest;

//========================================================================================
//	GBSP_CreateBSP
//========================================================================================
GBSP_RETVAL GBSP_CreateBSP(char *FileName, BspParms *Parms)
{
	CancelRequest = GE_FALSE;

	GHook.Printf("** BSP Compile Version: %i \n", GBSP_VERSION);
	//GHook.Printf("** Build Date/Time: "__DATE__","__TIME__"\n");

#ifdef HANDLE_EXCEPTIONS
	__try
	{
		if (!CreateBSP(FileName, Parms))
		{
			if (CancelRequest)
				return GBSP_CANCEL;
			else
				return GBSP_ERROR;
		}
	
		return GBSP_OK;
	}
	__except(1)
	{
		// Clean up all errors, free any possible left-over data
		GHook.Printf("GBSPLib: Fatal error in BSP!  Doing Clean-up work.\n");

		CleanupGBSP();
		return GBSP_ERROR;
	}
#else
	if (!CreateBSP(FileName, Parms))
	{
		if (CancelRequest)
			return GBSP_CANCEL;
		else
			return GBSP_ERROR;
	}
#endif
	
	return GBSP_OK;
}

//========================================================================================
//	GBSP_UpdateEntities
//========================================================================================
geBoolean GBSP_UpdateEntities(char *MapName, char *BSPName)
{
	if (!UpdateEntities(MapName, BSPName))
	{
		if (CancelRequest)
			return GBSP_CANCEL;
		else
			return GBSP_ERROR;
	}

	return GBSP_OK;
}

//========================================================================================
// GBSP_SaveGBSPFile
//========================================================================================
GBSP_RETVAL GBSP_SaveGBSPFile(char *FileName)
{
	CancelRequest = GE_FALSE;

#ifdef HANDLE_EXCEPTIONS
	__try
	{
		if (!ConvertGBSPToFile(FileName))
		{
			if (CancelRequest)
				return GBSP_CANCEL;
			else
				return GBSP_ERROR;
		}
	}
	__except(1)
	{
		// Clean up all errors, free any possible left-over data
		GHook.Printf("GBSPLib: Fatal error Saving BSP!  Doing Clean-up work.\n");

		CleanupGBSP();
		return GBSP_ERROR;
	}
#else
	if (!ConvertGBSPToFile(FileName))
	{
		if (CancelRequest)
			return GBSP_CANCEL;
		else
			return GBSP_ERROR;
	}
#endif

	return GBSP_OK;
}

#ifdef SHOW_DEBUG_STATS
	#include "Poly.h"
	#include "Brush2.h"
#endif
//========================================================================================
//	GBSP_FreeBSP
//========================================================================================
void GBSP_FreeBSP(void)
{
	FreeAllGBSPData();
	FreeAllEntities();

#ifdef SHOW_DEBUG_STATS
	GHook.Printf("------------------------\n");
	GHook.Printf("Num Total Verts   : %5i\n", gTotalVerts);
	GHook.Printf("Num Peek Verts    : %5i\n", gPeekVerts);
	GHook.Printf("Num Total Brushes : %5i\n", gTotalBrushes);
	GHook.Printf("Num Peek Brushes  : %5i\n", gPeekBrushes);
	GHook.Printf("------------------------\n");
#endif
}

//========================================================================================
//	GBSP_VisGBSPFile
//========================================================================================
GBSP_RETVAL GBSP_VisGBSPFile(char *FileName, VisParms *Parms)
{
	CancelRequest = GE_FALSE;

#ifdef HANDLE_EXCEPTIONS
	__try
	{
		if (!VisGBSPFile(FileName, Parms))
		{
			if (CancelRequest)
				return GBSP_CANCEL;
			else
				return GBSP_ERROR;
		}
	}
	__except(1)
	{
		// Clean up all errors, free any possible left-over data
		GHook.Printf("GBSPLib: Fatal error in Vis!  Doing Clean-up work.\n");

		CleanupVis();
		return GBSP_ERROR;
	}
#else
	if (!VisGBSPFile(FileName, Parms))
	{
		if (CancelRequest)
			return GBSP_CANCEL;
		else
			return GBSP_ERROR;
	}
#endif

	return GBSP_OK;
}

//========================================================================================
//	GBSP_LightGBSPFile
//========================================================================================
GBSP_RETVAL GBSP_LightGBSPFile(char *FileName, LightParms *Parms)
{
	CancelRequest = GE_FALSE;

#ifdef HANDLE_EXCEPTIONS
	__try
	{
		if (!LightGBSPFile(FileName, Parms))
		{
			if (CancelRequest)
				return GBSP_CANCEL;
			else
				return GBSP_ERROR;
		}
	}
	__except(1)
	{
		// Clean up all errors, free any possible left-over data
		GHook.Printf("GBSPLib: Fatal error in Radiosity!  Doing Clean-up work.\n");

		CleanupLight();
		return GBSP_ERROR;
	}
#else
	if (!LightGBSPFile(FileName, Parms))
	{
		if (CancelRequest)
			return GBSP_CANCEL;
		else
			return GBSP_ERROR;
	}
#endif
	
	return GBSP_OK;
}

geBoolean GBSP_Cancel(void)
{
	CancelRequest = GE_TRUE;

	GHook.Printf("Cancel requested...\n");

	return GE_TRUE;
}

GBSP_FuncHook GBSP_FHook = {
	//5,
	GBSP_VERSION_MAJOR,
	GBSP_VERSION_MINOR,
	GBSP_CreateBSP,
	GBSP_SaveGBSPFile,
	GBSP_FreeBSP,
	GBSP_VisGBSPFile,
	GBSP_LightGBSPFile,
	GBSP_Cancel,
	GBSP_UpdateEntities
};

//========================================================================================
//	GBSP_Init
//========================================================================================
DllExport GBSP_FuncHook *GBSP_Init(GBSP_Hook *Hook)
{
	GHook = *Hook;

	return &GBSP_FHook;
}

