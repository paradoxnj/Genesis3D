/****************************************************************************************/
/*  prefs.h                                                                             */
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
#ifndef PREFS_H
#define PREFS_H

#include "basetype.h"

#ifdef __cplusplus
	extern "C" {
#endif


typedef struct tag_PathPrefs PathPrefs;
typedef struct tag_Prefs Prefs;

Prefs *Prefs_Create (void);
Prefs *Prefs_Read (const char *IniFilename);
void Prefs_Destroy (Prefs **ppPrefs);

geBoolean Prefs_Save (const Prefs *pPrefs, const char *IniFilename);

int Prefs_GetBackgroundColor (const Prefs *pPrefs);
geBoolean Prefs_SetBackgroundColor (Prefs *pPrefs, int Color);

int Prefs_GetGridColor (const Prefs *pPrefs);
geBoolean Prefs_SetGridColor (Prefs *pPrefs, int Color);

int Prefs_GetSnapGridColor (const Prefs *pPrefs);
geBoolean Prefs_SetSnapGridColor (Prefs *pPrefs, int Color);

const char *Prefs_GetTxlName (const Prefs *pPrefs);
geBoolean Prefs_SetTxlName (Prefs *pPrefs, const char *NewName);

const char *Prefs_GetTxlSearchPath (const Prefs *pPrefs);
geBoolean Prefs_SetTxlSearchPath (Prefs *pPrefs, const char *NewPath);

const char *Prefs_GetPreviewPath (const Prefs *pPrefs);
geBoolean Prefs_SetPreviewPath (Prefs *pPrefs, const char *NewPath);

const char *Prefs_GetHeadersList (const Prefs *pPrefs);
geBoolean Prefs_SetHeadersList (Prefs *pPrefs, const char *NewList);

const char *Prefs_GetObjectsDir (const Prefs *pPrefs);
geBoolean Prefs_SetObjectsDir (Prefs *pPrefs, const char *NewDir);

const char *Prefs_GetProjectDir (const Prefs *pPrefs);
geBoolean Prefs_SetProjectDir (Prefs *pPrefs, const char *NewDir);

#ifdef __cplusplus
	}
#endif


#endif
