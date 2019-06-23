/****************************************************************************************/
/*  Compiler.h                                                                          */
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
#ifndef COMPILER_H
#define COMPILER_H

#include "gbsplib.h"
#include <stdlib.h>

#define WM_USER_COMPILE_MSG (WM_USER + 1)
#define WM_USER_COMPILE_ERR (WM_USER + 2)
#define WM_USER_COMPILE_DONE (WM_USER + 3)

typedef struct tag_CompileParamsType
{
	geBoolean DoVis;
	geBoolean DoLight;
	geBoolean RunBsp;
	geBoolean RunPreview;
	geBoolean UseMinLight;
	geBoolean SuppressHidden;
	geBoolean EntitiesOnly;
	geBoolean VisDetailBrushes;
	LightParms Light;
	VisParms Vis;
	BspParms Bsp;
	char Filename[_MAX_PATH];
} CompileParamsType;

typedef enum
{
	COMPILER_ERROR_NONE,
	COMPILER_ERROR_USERCANCEL,
	COMPILER_ERROR_THREAD,			// couldn't spawn thread
	// Errors returned by DLL loader
	COMPILER_ERROR_NODLL,			// couldn't find the DLL
	COMPILER_ERROR_MISSINGFUNC,		// the DLL is missing a required function
	// Errors returned by GBSP functions
	COMPILER_ERROR_BSPFAIL,			// unable to compile the BSP
	COMPILER_ERROR_BSPSAVE,			// unable to save the compiled BSP
	// Errors returned by preview launcher
	COMPILER_ERROR_NOBSP,
	COMPILER_ERROR_BSPCOPY
} CompilerErrorEnum;


#define COMPILER_PROCESSID 0x12345678

CompilerErrorEnum Compiler_Compile
	(
	  CompileParamsType const *pParams,
	  ERROR_CB ErrorCallbackFcn,
	  PRINTF_CB PrintfCallbackFcn
	);

CompilerErrorEnum Compiler_StartThreadedCompile
	(
	  CompileParamsType const *pParams,
	  HWND hwndNotify
	);


CompilerErrorEnum Compiler_RunPreview
	(
	  char const *PreviewFilename,
	  char const *MotionFilename,
	  char const *GPreviewPath
	);

geBoolean Compiler_CompileInProgress
	(
	  void
	);

geBoolean Compiler_CancelCompile 
	(
	  void
	);

#endif
