/****************************************************************************************/
/*  AOPTIONS.C																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor studio/builder INI file options API.								*/
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
#include "AOptions.h"
#include "RAM.H"
#include <assert.h>

// Include windows for profile reading stuff...
#include <windows.h>

struct tag_AOptions
{
	char ViewerPath[MAX_PATH];
	char MaxPath[MAX_PATH];
	geBoolean OptFlag;
	int OptLevel;
};

static const char SectionName[]	= "AStudio";
static const char ViewerKey[]	= "ViewerPath";
static const char MaxKey[]		= "MaxPath";
static const char OptFlagKey[]	= "OptFlag";
static const char OptLevelKey[]	= "OptLevel";

AOptions *AOptions_Create (void)
{
	AOptions *Options = GE_RAM_ALLOCATE_STRUCT (AOptions);
	if (Options != NULL)
	{
		strcpy (Options->ViewerPath, "c:\\Program Files\\Eclipse\\Genesis3D\\ActView.exe");
		strcpy (Options->MaxPath, "c:\\3dsmax2\\3dsmax.exe");
		Options->OptFlag = GE_TRUE;
		Options->OptLevel = 4;
	}
	return Options;
}


AOptions *AOptions_CreateFromFile (const char *IniFilename)
{
	AOptions *Options = AOptions_Create ();
	if (Options != NULL)
	{
		int Flag;
		// Use default names for these two.
		// Probably at some time should try to find them at install?
		GetPrivateProfileString (SectionName, ViewerKey, Options->ViewerPath, Options->ViewerPath, MAX_PATH, IniFilename);
		GetPrivateProfileString (SectionName, MaxKey, Options->MaxPath, Options->MaxPath, MAX_PATH, IniFilename);
		Flag = GetPrivateProfileInt (SectionName, OptFlagKey, Options->OptFlag, IniFilename);
		Options->OptFlag = Flag ? GE_TRUE : GE_FALSE;
		Options->OptLevel = GetPrivateProfileInt (SectionName, OptLevelKey, Options->OptLevel, IniFilename);
	}

	return Options;
}

void AOptions_Destroy (AOptions **pOptions)
{
	geRam_Free (*pOptions);
}

geBoolean AOptions_WriteToFile (const AOptions *Options, const char *IniFilename)
{
	char OptLevelString[2] = "0";

	WritePrivateProfileString (SectionName, ViewerKey, Options->ViewerPath, IniFilename);
	WritePrivateProfileString (SectionName, MaxKey, Options->MaxPath, IniFilename);
	
	WritePrivateProfileString (SectionName, OptFlagKey, Options->OptFlag ? "1" : "0", IniFilename);

	if ((Options->OptLevel >= 0) && (Options->OptLevel <= 9))
	{
		*OptLevelString = (char)(Options->OptLevel + '0');
	}
	WritePrivateProfileString (SectionName, OptLevelKey, OptLevelString, IniFilename);

	return GE_TRUE;
}


const char *AOptions_GetViewerPath (const AOptions *Options)
{
	return Options->ViewerPath;
}

geBoolean AOptions_SetViewerPath (AOptions *Options, const char *ViewerPath)
{
	strcpy (Options->ViewerPath, ViewerPath);
	return GE_TRUE;
}

const char *AOptions_Get3DSMaxPath (const AOptions *Options)
{
	return Options->MaxPath;
}

geBoolean AOptions_Set3DSMaxPath (AOptions *Options, const char *MaxPath)
{
	strcpy (Options->MaxPath, MaxPath);
	return GE_TRUE;
}


geBoolean AOptions_GetMotionOptimizationFlag (const AOptions *Options)
{
	return Options->OptFlag;
}

geBoolean AOptions_SetMotionOptimizationFlag (AOptions *Options, geBoolean Flag)
{
	Options->OptFlag = Flag;
	return GE_TRUE;
}

int AOptions_GetMotionOptimizationLevel (const AOptions *Options)
{
	return Options->OptLevel;
}

geBoolean AOptions_SetMotionOptimizationLevel (AOptions *Options, int OptLevel)
{
	assert (OptLevel >= 0);
	assert (OptLevel <= 9);

	Options->OptLevel = OptLevel;
	return GE_TRUE;
}
