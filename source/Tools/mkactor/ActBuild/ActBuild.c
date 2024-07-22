/****************************************************************************************/
/*  ACTBUILD.C																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor builder main module.												*/
/*				 Reads project file and starts actor make process.						*/
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
#include "ActBuild.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GENESIS.H"
#include "make.h"
#include "mkutil.h"
#include "RAM.H"

#define MAX_PATH 260

struct ActBuild_Options
{
	char PathTo3DSMax[MAX_PATH];
	char ProjectFile[MAX_PATH];
	MK_Boolean MaxPathSet;
};

// Use this to initialize the default settings
const static ActBuild_Options DefaultOptions =
{
	"3dsmax.exe",
	"",
	MK_FALSE
};

ActBuild_Options *ActBuild_OptionsCreate    (void)
{
	ActBuild_Options *pOptions;

	pOptions = GE_RAM_ALLOCATE_STRUCT (ActBuild_Options);
	if (pOptions != NULL)
	{
		*pOptions = DefaultOptions;
	}
	return pOptions;
}

void ActBuild_OptionsDestroy (ActBuild_Options** ppOptions)
{
	ActBuild_Options *p;

	assert (ppOptions != NULL);
	p = *ppOptions;
	assert (p != NULL);

	geRam_Free (*ppOptions);
	*ppOptions = NULL;
}

#pragma warning (disable:4100)		// unused parameter
ReturnCode ActBuild_ParseOptionString(ActBuild_Options* options, const char* string, 
												MK_Boolean InScript,MkUtil_Printf Printf)
{
	ReturnCode retValue = RETURN_SUCCESS;

	assert(options != NULL);
	assert(string != NULL);

#define NO_FILENAME_WARNING Printf("WARNING: '%s' specified with no filename\n", string)

	if ((string[0] == '-') || (string[0] == '/'))
	{
		switch (string[1])
		{
			case 'm' :	// 3DSMax path
			case 'M' :
				if (string[2] == 0)
				{
					NO_FILENAME_WARNING;
					retValue = RETURN_WARNING;
				}
				else
				{
					if (options->MaxPathSet  != MK_FALSE)
					{
						Printf ("WARNING:  Multiple '%s' specifications\n", string);
						retValue = RETURN_WARNING;
					}
					else
					{
						strcpy (options->PathTo3DSMax, string + 2);
						options->MaxPathSet = MK_TRUE;
					}
				}
				break;

			case 'f' :	// project File
			case 'F' :
				if (string[2] == 0)
				{
					NO_FILENAME_WARNING;
					retValue = RETURN_WARNING;
				}
				else
				{
					if (options->ProjectFile[0] != 0)
					{
						Printf ("WARNING: Multiple input file specification '%s'\n", string);
						retValue = RETURN_WARNING;
					}
					else
					{
						strcpy (options->ProjectFile, string + 2);
					}
				}
				break;
			default :
				retValue = RETURN_NOACTION;
		}
	}
	return retValue;
}
#pragma warning (disable:4100)

void ActBuild_OutputUsage (MkUtil_Printf Printf)
{
	//COLS: 0         1         2         3         4         5         6         7       | 8
	Printf("\n");
	Printf("Batch builds an actor from an actor project file (.apj)\n");
	Printf("\n");
	Printf("ACTBUILD [options] /F<projectfile> \n");
	Printf("         /M<maxpath>\n");
	Printf("\n");
	Printf("/F<projectfile>  Specifies project file to build (Required)\n");
	Printf("/M<maxpath>      Specifies full path to 3D Studio MAX executable.\n");
	Printf("\n");
}

ReturnCode ActBuild_DoMake (ActBuild_Options* options,MkUtil_Printf Printf)
{
	ReturnCode retValue = RETURN_SUCCESS;
	AOptions *LocalOptions = NULL;
	AProject *Project = NULL;

	if (options->ProjectFile[0] == 0)
	{
		Printf ("ERROR: Must specify a project file.\n");
		MkUtil_AdjustReturnCode (&retValue, RETURN_ERROR);
		goto AllDone;
	}

	// create options structure and fill it in
	LocalOptions = AOptions_Create ();
	if (LocalOptions == NULL)
	{
		Printf ("ERROR: Out of memory creating options structure.\n");
		MkUtil_AdjustReturnCode (&retValue, RETURN_ERROR);
		goto AllDone;
	}

	// fill in local options structure.
	// Don't care about viewer path...only MAX path
	AOptions_Set3DSMaxPath (LocalOptions, options->PathTo3DSMax);

	// Load the project file
	Project = AProject_CreateFromFilename (options->ProjectFile);
	if (Project == NULL)
	{
		// failed.  Bummer.
		Printf ("ERROR:  Could not load project file '%s'.\n", options->ProjectFile);
		MkUtil_AdjustReturnCode (&retValue, RETURN_ERROR);
		goto AllDone;
	}

	// Guess it's time to go making...
	if (Make_Actor (Project, LocalOptions, Printf) == GE_TRUE)
	{
		Printf ("SUCCESS: Actor build process completed successfully.\n");
	}
	else
	{
		Printf ("ERROR: Actor build process failed.");
		MkUtil_AdjustReturnCode (&retValue, RETURN_ERROR);
		goto AllDone;
	}

AllDone :
	if (LocalOptions != NULL)
	{
		AOptions_Destroy (&LocalOptions);
	}
	if (Project != NULL)
	{
		AProject_Destroy (&Project);
	}
	return retValue;
}
