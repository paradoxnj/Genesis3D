/****************************************************************************************/
/*  MKUTIL.C																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Actor make process utility functions.									*/
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
#include <assert.h>
#include <string.h>

#include "mkutil.h"
#include "RAM.H"


#ifdef FMTACTOR
#include "Fmtactor.h"
#define OPTIONS					FmtActor_Options
#define TOOL_CREATE_OPTIONS		FmtActor_OptionsCreate
#define TOOL_DESTROY_OPTIONS	FmtActor_OptionsDestroy
#define TOOL_PARSE_STRING		FmtActor_ParseOptionString
#define TOOL_OUTPUT_USAGE		FmtActor_OutputUsage
#define TOOL_DO_MAKE			FmtActor_DoMake
#endif

#ifdef MKACTOR
#include "Mkactor.h"
#define OPTIONS					MkActor_Options
#define TOOL_CREATE_OPTIONS		MkActor_OptionsCreate
#define TOOL_DESTROY_OPTIONS	MkActor_OptionsDestroy
#define TOOL_PARSE_STRING		MkActor_ParseOptionString
#define TOOL_OUTPUT_USAGE		MkActor_OutputUsage
#define TOOL_DO_MAKE			MkActor_DoMake
#endif

#ifdef MKBODY
#include "Mkbody.h"
#define OPTIONS					MkBody_Options
#define TOOL_CREATE_OPTIONS		MkBody_OptionsCreate
#define TOOL_DESTROY_OPTIONS	MkBody_OptionsDestroy
#define TOOL_PARSE_STRING		MkBody_ParseOptionString
#define TOOL_OUTPUT_USAGE		MkBody_OutputUsage
#define TOOL_DO_MAKE			MkBody_DoMake
#endif

#ifdef MKBVH
#include "MkBVH.h"
#define OPTIONS					MkBVH_Options
#define TOOL_CREATE_OPTIONS		MkBVH_OptionsCreate
#define TOOL_DESTROY_OPTIONS	MkBVH_OptionsDestroy
#define TOOL_PARSE_STRING		MkBVH_ParseOptionString
#define TOOL_OUTPUT_USAGE		MkBVH_OutputUsage
#define TOOL_DO_MAKE			MkBVH_DoMake
#endif

#ifdef MKMOTION
#include "Mkmotion.h"
#define OPTIONS					MkMotion_Options
#define TOOL_CREATE_OPTIONS		MkMotion_OptionsCreate
#define TOOL_DESTROY_OPTIONS	MkMotion_OptionsDestroy
#define TOOL_PARSE_STRING		MkMotion_ParseOptionString
#define TOOL_OUTPUT_USAGE		MkMotion_OutputUsage
#define TOOL_DO_MAKE			MkMotion_DoMake
#endif

#ifdef MOPSHELL
#include "MopShell.h"
#define OPTIONS					MopShell_Options
#define TOOL_CREATE_OPTIONS		MopShell_OptionsCreate
#define TOOL_DESTROY_OPTIONS	MopShell_OptionsDestroy
#define TOOL_PARSE_STRING		MopShell_ParseOptionString
#define TOOL_OUTPUT_USAGE		MopShell_OutputUsage
#define TOOL_DO_MAKE			MopShell_DoMake
#endif

#ifdef ACTBUILD
#include "ActBuild.h"
#define OPTIONS					ActBuild_Options
#define TOOL_CREATE_OPTIONS		ActBuild_OptionsCreate
#define TOOL_DESTROY_OPTIONS	ActBuild_OptionsDestroy
#define TOOL_PARSE_STRING		ActBuild_ParseOptionString
#define TOOL_OUTPUT_USAGE		ActBuild_OutputUsage
#define TOOL_DO_MAKE			ActBuild_DoMake
#endif


static ReturnCode MkUtil_ParseOptionString(OPTIONS* options, const char* string, MK_Boolean InScript);
static ReturnCode MkUtil_ParseCommandLineOptions(OPTIONS* options, int argc, char* argv[]);



static void MkUtil_PrintfCallback(const char *Fmt, ...)
{
	assert(Fmt);
 
	vprintf( Fmt, (char *)&Fmt + sizeof( Fmt ) );

}


// Kind of a hack here.  The ActBuild program includes MAKE.C, which also defines
// both of these functions.  So I'll conditionally compile it here...
#ifndef ACTBUILD

int MkUtil_Interrupt( void )
{
	return 0;
}


void MkUtil_AdjustReturnCode(ReturnCode* pToAdjust, ReturnCode AdjustBy)
{
	assert(pToAdjust != NULL);

	switch(AdjustBy)
	{
	case RETURN_SUCCESS:
		// do nothing
		break;

	case RETURN_WARNING:
		if(*pToAdjust == RETURN_SUCCESS)
			*pToAdjust = RETURN_WARNING;
		break;

	case RETURN_ERROR:
		if(*pToAdjust != RETURN_USAGE)
			*pToAdjust = RETURN_ERROR;
		break;

	case RETURN_USAGE:
		*pToAdjust = RETURN_USAGE;
		break;

	default:
		assert(0);
	}
}
#endif


static void MkUtil_OutputGeneralUsage(MkUtil_Printf PrintfCallback)
{
	//COLS:0         1         2         3         4         5         6         7       | 8
	#if 0
	PrintfCallback("\n");
	PrintfCallback("Options:\n");
	PrintfCallback("/F<scriptfile>  Specifies script file, command line arguments override script.\n");
	PrintfCallback("/?              Display this usage.\n");
	PrintfCallback("\n");
	PrintfCallback("Input may be specified either on the command line or in one script files.  A\n");
	PrintfCallback("script file is composed of options and arguments, each on an individual line.\n");
	PrintfCallback("'//' comment lines are permitted.  The first character of any line should be\n");
	PrintfCallback("a '-' or a '/'.  Maximum line length in a script is 256 characters.  Multiple\n");
	PrintfCallback("script files are supported, but nested scripts are not.\n");
	#endif
	PrintfCallback;
}


int main(int argc, char* argv[])
{
	OPTIONS* pOptions;
	ReturnCode retValue = RETURN_SUCCESS;
	ReturnCode newValue;

	pOptions = TOOL_CREATE_OPTIONS();
	if(pOptions == NULL)
	{
		return RETURN_ERROR;
	}

	// Parse command line
	retValue = MkUtil_ParseCommandLineOptions(pOptions, argc, argv);

	switch(retValue)
	{
	case RETURN_SUCCESS:
	case RETURN_WARNING:
		newValue = TOOL_DO_MAKE(pOptions,MkUtil_PrintfCallback);
		MkUtil_AdjustReturnCode(&retValue, newValue);
		break;

	case RETURN_ERROR:
		break;

	case RETURN_USAGE:
		TOOL_OUTPUT_USAGE(MkUtil_PrintfCallback);
		MkUtil_OutputGeneralUsage(MkUtil_PrintfCallback);	
		break;
	}

	TOOL_DESTROY_OPTIONS(&pOptions);

	geRam_ReportAllocations();

	return (int)retValue;
}

static ReturnCode MkUtil_ParseCommandLineOptions(OPTIONS* options, int argc, char* argv[])
{
	int i;
	ReturnCode retValue = RETURN_SUCCESS;
	ReturnCode newValue;

	assert(argc >= 1);
	assert(argv != NULL);
	assert(options != NULL);

	// start at 1, argv[0] is the executable
	for(i=1;i<argc;i++)
	{
		newValue = MkUtil_ParseOptionString(options, argv[i], MK_FALSE);
		MkUtil_AdjustReturnCode(&retValue, newValue);
		if( (retValue == RETURN_ERROR) || (retValue == RETURN_USAGE) )
			break;
	}

	return retValue;
}

static ReturnCode MkUtil_ParseOptionString(OPTIONS* options, const char* string, MK_Boolean InScript)
{
	ReturnCode retValue = RETURN_SUCCESS;

	assert(options != NULL);
	assert(string != NULL);

	retValue = TOOL_PARSE_STRING(options, string, InScript,MkUtil_PrintfCallback);

	if(retValue == RETURN_NOACTION)
	{
		if( (string[0] == '-') || (string[0] == '/') )
		{
			switch(string[1])
			{
			case '/':
				retValue = RETURN_SUCCESS; // comment line, ignore it
				break;

			#if 0
			case 'f':
			case 'F':
				if(InScript == MK_FALSE)
				{
					retValue = MkUtil_ParseScriptFile(options, string + 2);
				}
				else
				{
					printf("WARNING: Nested script files not supported\n");
					retValue = RETURN_WARNING;
				}
				break;
			#endif

			case '?':
				// don't continue with execution - should display help
				retValue = RETURN_USAGE;
				break;

			default:
				// unknown entry
				printf("WARNING: Option not supported: \"%s\"\n", string);
				retValue = RETURN_WARNING;
			}
		}
	}

	if(retValue == RETURN_NOACTION)
	{
		printf("ERROR: Option string not understood: \"%s\"\n", string);
		return RETURN_ERROR;
	}

	return retValue;
}
