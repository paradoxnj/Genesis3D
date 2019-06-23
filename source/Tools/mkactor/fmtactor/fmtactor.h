/****************************************************************************************/
/*  FMTACTOR.H                                                                          */
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
#ifndef FORMAT_H
#define FORMAT_H

#include "Mkutil.h"			//ReturnCode, PrintfCallback

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FmtActor_Options FmtActor_Options;

FmtActor_Options*	FmtActor_OptionsCreate    (void);
void				FmtActor_OptionsDestroy   (FmtActor_Options** ppOptions);
ReturnCode			FmtActor_ParseOptionString(FmtActor_Options* options, const char* string, 
												MK_Boolean InScript,MkUtil_Printf PrintfCallback);
void				FmtActor_OutputUsage	  (MkUtil_Printf PrintfCallback);
ReturnCode			FmtActor_DoMake			  (FmtActor_Options* options,MkUtil_Printf PrintfCallback);


#ifdef __cplusplus
}
#endif

#endif

