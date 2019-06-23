/****************************************************************************************/
/*  MKACTOR.H                                                                           */
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: Actor construction from body and motions.								*/
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
#ifndef MKACTOR_H
#define MKACTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mkutil.h"			//ReturnCode

typedef struct MkActor_Options MkActor_Options;

MkActor_Options*	MkActor_OptionsCreate    (void);
void				MkActor_OptionsDestroy   (MkActor_Options** ppOptions);
ReturnCode			MkActor_ParseOptionString(MkActor_Options* options, const char* string, 
					MK_Boolean InScript,MkUtil_Printf PrintfCallback);
void				MkActor_OutputUsage	     (MkUtil_Printf PrintfCallback);
ReturnCode			MkActor_DoMake			 (MkActor_Options* options,MkUtil_Printf PrintfCallback);


#ifdef __cplusplus
}
#endif

#endif

