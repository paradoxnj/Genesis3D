/****************************************************************************************/
/*  Prefs.c                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Program preferences.                                                  */
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
#include "prefs.h"
#include "RAM.H"
#include "util.h"
#include <assert.h>
#pragma warning(disable : 4201 4214 4115 4514)
#include <windows.h>
#pragma warning(default : 4201 4214 4115)
#include <stdlib.h>

static const char GRID_SECTION_NAME[]	= "Grid";
static const char BACKGROUND_KEY[]		= "Background";
static const char GRID_COLOR_KEY[]		= "Color";
static const char SNAPGRID_COLOR_KEY[]	= "SnapColor";

static const int DEFAULT_BACKGROUND_COLOR	= 0x00808080;
static const int DEFAULT_GRID_COLOR			= 0x00646464;
static const int DEFAULT_SNAPGRID_COLOR		= 0x00464646;


static const char PATHS_SECTION_NAME[]	= "Paths";
static const char TXLNAME_KEY[]			= "TxlName";
static const char TXLSEARCH_KEY[]		= "TxlSearchPath";
static const char PREVIEW_KEY[]			= "PreviewPath";
static const char HEADERS_KEY[]			= "Headers";
static const char OBJECTSDIR_KEY[]		= "ObjectsDir";
static const char PROJECTDIR_KEY[]		= "ProjectDir";

static const char DEFAULT_TXLNAME[]		= "gedit.txl";
static const char DEFAULT_TXLSEARCH[]	= ".\\";
static const char DEFAULT_PREVIEW[]		= "gpreview.exe";
static const char DEFAULT_HEADERS[]		= ".\\Headers";
static const char DEFAULT_OBJECTSDIR[]	= ".\\Objects";
static const char DEFAULT_PROJECTDIR[]	= ".\\Levels";


/*
#define FUSION_SECTION_NAME "GEdit"

#define FUSION_TXLPATH_ENTRY "DefaultTxLib"
#define FUSION_DEFAULT_TXL "GEdit.Txl"

#define FUSION_PREVIEWPATH_ENTRY "GPreviewPath"
#define FUSION_DEFAULT_PREVIEW "GPreview.exe"

#define FUSION_PROJECTDIR_ENTRY "ProjectDir"
#define FUSION_DEFAULT_PROJECTDIR ""

#define FUSION_EXTPATH_ENTRY "ExtensionsPath"
*/

struct tag_PathPrefs
{
	char *TxlName;
	char *TxlSearchPath;
	char *PreviewPath;
	char *HeadersList;
	char *ObjectsDir;
	char *ProjectDir;
};

struct tag_Prefs
{
	int BackgroundColor;
	int GridColor;
	int SnapGridColor;
	PathPrefs Paths;
};

static char *Prefs_GetString
	(
	  const char *Section,
	  const char *Key,
	  const char *Default,
	  const char *IniFilename
	)
{
	DWORD retval;
	char Buffer[1024];

	retval = GetPrivateProfileString (Section, Key, Default, Buffer, sizeof (Buffer), IniFilename);
	if (retval == 0)
	{
		return NULL;
	}

	return Util_Strdup (Buffer);
}

Prefs *Prefs_Create (void)
{
	Prefs *pPrefs;

	pPrefs = GE_RAM_ALLOCATE_STRUCT (Prefs);
	if (pPrefs != NULL)
	{
		pPrefs->BackgroundColor = DEFAULT_BACKGROUND_COLOR;
		pPrefs->GridColor = DEFAULT_GRID_COLOR;
		pPrefs->SnapGridColor = DEFAULT_SNAPGRID_COLOR;
		pPrefs->Paths.TxlName = Util_Strdup (DEFAULT_TXLNAME);
		pPrefs->Paths.TxlSearchPath = Util_Strdup (DEFAULT_TXLSEARCH);
		pPrefs->Paths.PreviewPath = Util_Strdup (DEFAULT_PREVIEW);
		pPrefs->Paths.HeadersList = Util_Strdup (DEFAULT_HEADERS);
		pPrefs->Paths.ObjectsDir = Util_Strdup (DEFAULT_OBJECTSDIR);
		pPrefs->Paths.ProjectDir = Util_Strdup (DEFAULT_PROJECTDIR);
	}
	return pPrefs;
}


void Prefs_Destroy (Prefs **ppPrefs)
{
	Prefs *pPrefs;
	PathPrefs *pPaths;

	assert (ppPrefs != NULL);

	pPrefs = *ppPrefs;
	assert (pPrefs != NULL);

	pPaths = &pPrefs->Paths;
	if (pPaths->TxlName != NULL)		geRam_Free (pPaths->TxlName);
	if (pPaths->TxlSearchPath != NULL)	geRam_Free (pPaths->TxlSearchPath);
	if (pPaths->PreviewPath != NULL)	geRam_Free (pPaths->PreviewPath);
	if (pPaths->HeadersList != NULL)	geRam_Free (pPaths->HeadersList);
	if (pPaths->ObjectsDir != NULL)		geRam_Free (pPaths->ObjectsDir);
	if (pPaths->ProjectDir != NULL)		geRam_Free (pPaths->ProjectDir);

	geRam_Free (*ppPrefs);
}
Prefs *Prefs_Read (const char *IniFilename)
{
	Prefs *pPrefs;

	pPrefs = GE_RAM_ALLOCATE_STRUCT (Prefs);
	if (pPrefs != NULL)
	{
		// read preferences from INI file
		pPrefs->BackgroundColor = GetPrivateProfileInt (GRID_SECTION_NAME, BACKGROUND_KEY, DEFAULT_BACKGROUND_COLOR, IniFilename);
		pPrefs->GridColor = GetPrivateProfileInt (GRID_SECTION_NAME, GRID_COLOR_KEY, DEFAULT_GRID_COLOR, IniFilename);
		pPrefs->SnapGridColor = GetPrivateProfileInt (GRID_SECTION_NAME, SNAPGRID_COLOR_KEY, DEFAULT_SNAPGRID_COLOR, IniFilename);

		pPrefs->Paths.TxlName = Prefs_GetString (PATHS_SECTION_NAME, TXLNAME_KEY, DEFAULT_TXLNAME, IniFilename);
		pPrefs->Paths.TxlSearchPath = Prefs_GetString (PATHS_SECTION_NAME, TXLSEARCH_KEY, DEFAULT_TXLSEARCH, IniFilename);
		pPrefs->Paths.PreviewPath = Prefs_GetString (PATHS_SECTION_NAME, PREVIEW_KEY, DEFAULT_PREVIEW, IniFilename);
		pPrefs->Paths.HeadersList = Prefs_GetString (PATHS_SECTION_NAME, HEADERS_KEY, DEFAULT_HEADERS, IniFilename);
		pPrefs->Paths.ObjectsDir = Prefs_GetString (PATHS_SECTION_NAME, OBJECTSDIR_KEY, DEFAULT_OBJECTSDIR, IniFilename);
		pPrefs->Paths.ProjectDir = Prefs_GetString (PATHS_SECTION_NAME, PROJECTDIR_KEY, DEFAULT_PROJECTDIR, IniFilename);
	}
	return pPrefs;
}

static geBoolean WritePrivateProfileInt
	(
	  const char *Section,
	  const char *Key,
	  int Val,
	  const char *IniFilename
	)
{
	char str[32];

	itoa (Val, str, 10);
	return WritePrivateProfileString (Section, Key, str, IniFilename);
}

geBoolean Prefs_Save (const Prefs *pPrefs, const char *IniFilename)
{
	WritePrivateProfileInt (GRID_SECTION_NAME, BACKGROUND_KEY, pPrefs->BackgroundColor, IniFilename);
	WritePrivateProfileInt (GRID_SECTION_NAME, GRID_COLOR_KEY, pPrefs->GridColor, IniFilename);
	WritePrivateProfileInt (GRID_SECTION_NAME, SNAPGRID_COLOR_KEY, pPrefs->SnapGridColor, IniFilename);

	WritePrivateProfileString (PATHS_SECTION_NAME, TXLNAME_KEY, pPrefs->Paths.TxlName, IniFilename);
	WritePrivateProfileString (PATHS_SECTION_NAME, TXLSEARCH_KEY, pPrefs->Paths.TxlSearchPath, IniFilename);
	WritePrivateProfileString (PATHS_SECTION_NAME, PREVIEW_KEY, pPrefs->Paths.PreviewPath, IniFilename);
	WritePrivateProfileString (PATHS_SECTION_NAME, HEADERS_KEY, pPrefs->Paths.HeadersList, IniFilename);
	WritePrivateProfileString (PATHS_SECTION_NAME, OBJECTSDIR_KEY, pPrefs->Paths.ObjectsDir, IniFilename);
	WritePrivateProfileString (PATHS_SECTION_NAME, PROJECTDIR_KEY, pPrefs->Paths.ProjectDir, IniFilename);	

	return GE_TRUE;
}



int Prefs_GetBackgroundColor (const Prefs *pPrefs)
{
	return pPrefs->BackgroundColor;
}

geBoolean Prefs_SetBackgroundColor (Prefs *pPrefs, int Color)
{
	pPrefs->BackgroundColor = Color;
	return GE_TRUE;
}

int Prefs_GetGridColor (const Prefs *pPrefs)
{
	return pPrefs->GridColor;
}

geBoolean Prefs_SetGridColor (Prefs *pPrefs, int Color)
{
	pPrefs->GridColor = Color;
	return GE_TRUE;
}

int Prefs_GetSnapGridColor (const Prefs *pPrefs)
{
	return pPrefs->SnapGridColor;
}

geBoolean Prefs_SetSnapGridColor (Prefs *pPrefs, int Color)
{
	pPrefs->SnapGridColor = Color;
	return GE_TRUE;
}

const char *Prefs_GetTxlName (const Prefs *pPrefs)
{
	return pPrefs->Paths.TxlName;
}

geBoolean Prefs_SetTxlName (Prefs *pPrefs, const char *NewName)
{
	return Util_SetString (&pPrefs->Paths.TxlName, NewName);
}

const char *Prefs_GetTxlSearchPath (const Prefs *pPrefs)
{
	return pPrefs->Paths.TxlSearchPath;
}

geBoolean Prefs_SetTxlSearchPath (Prefs *pPrefs, const char *NewPath)
{
	return Util_SetString (&pPrefs->Paths.TxlSearchPath, NewPath);
}

const char *Prefs_GetPreviewPath (const Prefs *pPrefs)
{
	return pPrefs->Paths.PreviewPath;
}

geBoolean Prefs_SetPreviewPath (Prefs *pPrefs, const char *NewPath)
{
	return Util_SetString (&pPrefs->Paths.PreviewPath, NewPath);
}

const char *Prefs_GetHeadersList (const Prefs *pPrefs)
{
	return pPrefs->Paths.HeadersList;
}

geBoolean Prefs_SetHeadersList (Prefs *pPrefs, const char *NewList)
{
	return Util_SetString (&pPrefs->Paths.HeadersList, NewList);
}

const char *Prefs_GetObjectsDir (const Prefs *pPrefs)
{
	return pPrefs->Paths.ObjectsDir;
}

geBoolean Prefs_SetObjectsDir (Prefs *pPrefs, const char *NewDir)
{
	return Util_SetString (&pPrefs->Paths.ObjectsDir, NewDir);
}

const char *Prefs_GetProjectDir (const Prefs *pPrefs)
{
	return pPrefs->Paths.ProjectDir;
}

geBoolean Prefs_SetProjectDir (Prefs *pPrefs, const char *NewDir)
{
	return Util_SetString (&pPrefs->Paths.ProjectDir, NewDir);
}

