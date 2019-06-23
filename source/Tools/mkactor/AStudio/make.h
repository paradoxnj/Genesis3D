/****************************************************************************************/
/*  MAKE.H																				*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Actor make process main module.										*/
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
#ifndef MAKE_H
#define MAKE_H

#include "basetype.h"
#include "mkutil.h"
#include "AProject.h"
#include "AOptions.h"

#ifdef __cplusplus
extern "C" {
#endif


geBoolean Make_Body(AProject *Prj, AOptions *Options, MkUtil_Printf Printf);

geBoolean Make_Motion(AProject *Prj, AOptions *Options, 
					int Count, int *MotionIndexArray, MkUtil_Printf Printf);

geBoolean Make_Actor(AProject *Prj, AOptions *Options, MkUtil_Printf Printf);

geBoolean Make_Clean(AProject *Prj, AOptions *Options, MkUtil_Printf Printf);

void Make_SetInterruptFlag (geBoolean State);

geBoolean Make_ActorSummary(AProject *Prj, MkUtil_Printf Printf);

#ifdef __cplusplus
}
#endif

#endif

