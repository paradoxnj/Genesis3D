/****************************************************************************************/
/*  MXSCRIPT.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: 3DS MAX script maintenance and execution for actor make process.		*/
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
#ifndef MXSCRIPT_H
#define MXSCRIPT_H

#include "mkutil.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MXScript MXScript;

geBoolean MXScript_ArePluginsInstalled( const char *MaxExeName, MkUtil_Printf Printf );

MXScript *MXScript_StartScript(const char *TempDir, MkUtil_Printf Printf);

geBoolean MXScript_AddExport(MXScript *Script,const char *LoadFilename, 
								const char *ExportFilename,MkUtil_Printf Printf);

geBoolean MXScript_EndScript(MXScript *Script,MkUtil_Printf Printf);

void MXScript_Destroy(MXScript *Script);
geBoolean MXScript_RunScript(MXScript *Script, const char *MaxExeName, MkUtil_Printf Printf);


#ifdef __cplusplus
}
#endif


#endif



