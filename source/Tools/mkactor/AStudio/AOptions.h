/****************************************************************************************/
/*  AOPTIONS.H																			*/
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef AOPTIONS_H
#define AOPTIONS_H

#include "BASETYPE.H"

#ifdef __cplusplus
	extern "C" {
#endif


typedef struct tag_AOptions AOptions;

AOptions *AOptions_Create (void);

AOptions *AOptions_CreateFromFile (const char *IniFilename);
void AOptions_Destroy (AOptions **pOptions);
geBoolean AOptions_WriteToFile (const AOptions *Options, const char *IniFilename);

const char *AOptions_GetViewerPath (const AOptions *Options);
geBoolean AOptions_SetViewerPath (AOptions *Options, const char *ViewerPath);

const char *AOptions_Get3DSMaxPath (const AOptions *Options);
geBoolean AOptions_Set3DSMaxPath (AOptions *Options, const char *MaxPath);

geBoolean AOptions_GetMotionOptimizationFlag (const AOptions *Options);
geBoolean AOptions_SetMotionOptimizationFlag (AOptions *Options, geBoolean Flag);

int AOptions_GetMotionOptimizationLevel (const AOptions *Options);
geBoolean AOptions_SetMotionOptimizationLevel (AOptions *Options, int OptLevel);

#ifdef __cplusplus
	}
#endif


#endif
