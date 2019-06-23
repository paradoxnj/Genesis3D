/****************************************************************************************/
/*  MKMOTION.H	                                                                        */
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: Motion construction from MAX export.									*/
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
#ifndef MKMOTION_H
#define MKMOTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Mkutil.h"			//ReturnCode

typedef struct MkMotion_Options MkMotion_Options;

MkMotion_Options*	MkMotion_OptionsCreate    (void);
void				MkMotion_OptionsDestroy   (MkMotion_Options** ppOptions);
ReturnCode			MkMotion_ParseOptionString(MkMotion_Options* options, const char* string, 
												MK_Boolean InScript,MkUtil_Printf PrintfCallback);
void				MkMotion_OutputUsage	  (MkUtil_Printf PrintfCallback);
ReturnCode			MkMotion_DoMake		      (MkMotion_Options* options,MkUtil_Printf PrintfCallback);


#ifdef __cplusplus
}
#endif

#endif

