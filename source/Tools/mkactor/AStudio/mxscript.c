/****************************************************************************************/
/*  MXSCRIPT.C																			*/
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <io.h>						// _finddata_t

#include "RAM.H"
#include "mkutil.h"
#include "mxscript.h"
#include "filepath.h"

#define BIG (4096)
#define MAX_MX_LOGFILENAME (_MAX_PATH * 3)

typedef struct MXScript
{
	FILE *File;
	int Error;
	char ScriptFileName[BIG];
	char LogName[BIG];
} MXScript;

static void MXScript_AddLine(MXScript *M,const char *Line, MkUtil_Printf Printf)
{
	int written;
	assert( M );
	assert( Line );
	assert( M->File );
	if (M->Error == 0)
		{
			written = fprintf( M->File,"%s\n",Line);
			if (written <= 0)
				{
					Printf("Error:  trying to output line to 3DS MAX script.  Line was:\n");
					Printf(Line);
					M->Error++;
				}
		}
}

static void MXScript_AddLinS(MXScript *M,const char *Line,const char *Str,MkUtil_Printf Printf)
{
	char buff[MAX_MX_LOGFILENAME + 100];
	assert( M );
	assert( M->File );
	assert( Str );
	assert( Line );
	if (M->Error == 0)
		{
			int written = sprintf(buff,Line,Str);
			if (written <= 0)
				{
					Printf("Error: sprintf failed in trying to output to 3DS MAX script\n");
					M->Error++;
				}
			else
				{
					MXScript_AddLine(M,buff,Printf);
				}
		}
}

MXScript *MXScript_StartScript(const char *tempdir, MkUtil_Printf Printf)
{
	MXScript *M = NULL;

	M = GE_RAM_ALLOCATE_STRUCT( MXScript );
	if (M == NULL)
		{
			Printf("Error: unable to get memory for 3DS MAX script structure\n");
			goto StartScriptError;
		}
	
	M->ScriptFileName[0] = 0;
	M->LogName[0]        = 0;
	
	if (FilePath_AppendName	(tempdir,"max.ms",M->ScriptFileName)==GE_FALSE)
		{
			goto StartScriptError;
		}

	if (FilePath_AppendName	(tempdir,"max.log",M->LogName)==GE_FALSE)
		{
			goto StartScriptError;
		}

	
	M->File = fopen(M->ScriptFileName,"wt");
	if (M->File == NULL)
		{
			Printf("Error: unable to open script file (%s) for writing\n",M->ScriptFileName);
			goto StartScriptError;
		}
	
	M->Error = 0;
	
	MXScript_AddLinS(M,"deleteFile \"%s\"",M->LogName,Printf);
	MXScript_AddLinS(M,"logfile = createFile \"%s\"",M->LogName,Printf);
	MXScript_AddLine(M,"if logfile == undefined then quitMAX #noPrompt",Printf);
	if (M->Error > 0)
		{
			Printf("Error: unable to write header to 3DS MAX script file (%s)\n",M->ScriptFileName);
			goto StartScriptError;
		}

	return M;
	

StartScriptError:
	if (M->File != NULL)
		fclose(M->File);
	if (M != NULL)
		geRam_Free(M);
	return NULL;
}
	
const char *MXScript_GetScriptFileName(MXScript *Script)
{
	assert( Script != NULL );
	return Script->ScriptFileName;
}

geBoolean MXScript_EndScript(MXScript *Script,MkUtil_Printf Printf)
{
	geBoolean rval = GE_TRUE;
	assert( Script != NULL );
	assert( Script->File );
	assert( Script->ScriptFileName );
	assert( Script->LogName );

	MXScript_AddLinS(Script,"print \"Success\" to: logfile",Script->LogName,Printf);
	MXScript_AddLine(Script,"close logfile",Printf);
	MXScript_AddLine(Script,"quitMAX #noPrompt",Printf);
	if (Script->Error > 0)
		{
			Printf("Error: unable to write footer to script file (%s)\n",Script->ScriptFileName);
			rval = GE_FALSE;
		}
	
	fclose(Script->File);
	Script->File = NULL;
	return rval;
}

void MXScript_Destroy( MXScript *Script )
{
	assert( Script != NULL );

	if (Script->File)
		{
			fclose(Script->File);
			Script->File = NULL;
		}
	Script->Error = 999;
	geRam_Free(Script);
}
	

geBoolean MXScript_AddExport(MXScript *M,const char *LoadFilename, const char *ExportFilename, MkUtil_Printf Printf)
{
	assert( M != NULL );
	assert( M->File );
	assert(LoadFilename != NULL);
	assert(ExportFilename != NULL);

	MXScript_AddLinS(M,"test = loadMAXFile \"%s\" ",LoadFilename,Printf);
	MXScript_AddLine(M,"if test != true then",Printf);
	MXScript_AddLine(M,"	(",Printf);
	MXScript_AddLinS(M," 		print \"Error: failed to load %s in 3DSMax\" to: logfile",LoadFilename,Printf);
	MXScript_AddLine(M," 		flush logfile",Printf);
	MXScript_AddLine(M," 		close logfile",Printf);
	MXScript_AddLine(M," 		quitMAX #noPrompt",Printf);
	MXScript_AddLine(M," 	)",Printf);
	MXScript_AddLinS(M," test = exportFile \"%s\" #noprompt",ExportFilename,Printf);
	MXScript_AddLine(M," if test != true then",Printf);
	MXScript_AddLine(M," 	(",Printf);
	MXScript_AddLinS(M," 		print \"Error: failed to export %s from 3DSMax\" to: logfile",ExportFilename,Printf);
	MXScript_AddLine(M," 		flush logfile",Printf);
	MXScript_AddLine(M," 		close logfile",Printf);
	MXScript_AddLine(M," 		quitMAX #noprompt",Printf);
	MXScript_AddLine(M," 	)",Printf);
	
	if (M->Error > 0)
		{
			Printf("Error: unable to write export lines to 3DS MAX script for '%s' to script file\n",LoadFilename);
			return GE_FALSE;
		}
	return GE_TRUE;
}


geBoolean MXScript_ArePluginsInstalled( const char *MaxExeName, MkUtil_Printf Printf )
{
	long Handle;
	struct _finddata_t PluginData;
	char PluginName[BIG];
	char drive[BIG];
	char dir[BIG];
	char fname[BIG];
	char ext[BIG];
	assert( MaxExeName );
	
	_splitpath( MaxExeName, drive,dir,fname,ext);
	strcat(dir,"Plugins");
	_makepath( PluginName, drive,dir,"NFOEXP","DLE" );

	Handle = _findfirst( PluginName, &PluginData ); 
	if (Handle != -1)
		{
			_findclose(Handle);
		}
	else
		{
			Printf("Error: Did not locate 3DS MAX plugin: '%s'\n",PluginName);
			return GE_FALSE;
		}

	_makepath( PluginName, drive,dir,"KEYEXP","DLE" );
	Handle = _findfirst( PluginName, &PluginData );
	if (Handle != -1)
		{
			_findclose(Handle);
		}
	else
		{
			Printf("Error: Did not locate 3DS MAX plugin: '%s'\n",PluginName);
			return GE_FALSE;
		}
	return GE_TRUE;
}


geBoolean MXScript_RunScript( MXScript *M, const char *MaxExeName, MkUtil_Printf Printf )
{
	char Command[BIG];
	char FirstLine[BIG];
	FILE *F;
	assert( M != NULL );
	assert( M->File == NULL );		// have to MXScript_EndScript first
	assert( M->LogName );
		
	
	sprintf(Command,"start /wait %s -U MAXScript %s",MaxExeName,M->ScriptFileName);

	_flushall();
	errno = 0;
	Printf("Running 3DSMAX.  To interrupt close 3DSMAX\n");
	system(Command);
	if (errno != 0)
		{
			Printf("Error: unable to execute 3DSMax\n");
			return GE_FALSE;
		}
	if (MkUtil_Interrupt())
		{
			Printf("Interrupted.\n");
			return GE_FALSE;
		}

	F = fopen(M->LogName,"rt");
	if (F == NULL)
		{
			Printf("Error: unable to open temporary output file (%s)\n",M->LogName);
			return GE_FALSE;
		}
	if (fgets(FirstLine,BIG-1,F)== NULL)
		{
			Printf("Error: unable to access first line of temporary output file (%s)\n",M->LogName);
			fclose(F);
			return GE_FALSE;
		}
	fclose(F);
	if (strncmp(FirstLine,"\"Success",strlen("\"Success"))!=0)
		{
			Printf(FirstLine);
			return GE_FALSE;
		}
	Printf("3DSMAX data export succeeded\n");
	return GE_TRUE;
}
