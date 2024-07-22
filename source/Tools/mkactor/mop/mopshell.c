/****************************************************************************************/
/*  MOPSHELL.C	                                                                        */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Motion optimizer.														*/
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
#include <windows.h>

#pragma warning (disable:4201)
#include <mmsystem.h>	// timeGetTime
#pragma warning (disable:4201)

#include <assert.h>	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GENESIS.H"
#include "mkutil.h"
#include "motion.h"
#include "RAM.H"
#include "pop.h"
#include "log.h"
#include "mopshell.h"

#define MAX_OPTIMIZATION_LEVEL 9
#define MILLISECONDS_BETWEEN_NOTIFICATIONS (10000)

typedef struct MopShell_Options
{
	MK_Boolean TextOutput;
	MK_Boolean OutputSet;
	char DestinationMotionFile[_MAX_PATH];
	char SourceMotionFile[_MAX_PATH];
	char LogFile[_MAX_PATH];
	int OptimizationLevel;
} MopShell_Options;


// Use this to initialize the default settings
const static MopShell_Options DefaultOptions = 
{
	MK_TRUE,
	MK_FALSE,
	"",
	"",
	"",
	-1,
};
	
	

geMotion *MopShell_Optimize(geMotion *M, geFloat Tolerance,char *LogName,MkUtil_Printf Printf)
{
	int i; 
	int Count;
	unsigned int LastTime;
	geMotion *MO;
	char *EmptyName="";
	const char *Name;
	LogType *Log=NULL;
	
	assert( M != NULL );
	assert(LogName != NULL);
	
	if (LogName[0] != 0)
		{
			Log = Log_Open(LOG_TO_FILE,LogName,LOG_RESET);
			if (Log) Log_Timestamp(Log);
			else Printf("warning: unable to write log file '%s'\n",LogName);
		}
	
	MO = geMotion_Create(geMotion_HasNames(M));
	if (MO == NULL)
		{
			return NULL;
		}

	Name = geMotion_GetName(M);
	if (Name != NULL) 
		{
			geMotion_SetName(MO,Name);
			if (Log) Log_Output(Log,"Motion '%s'",Name);
		}
	else
		{
			Name = EmptyName;
			if (Log) Log_Output(Log,"Motion");
		}

	Count = geMotion_GetPathCount(M);
	LastTime = timeGetTime();
			
	for (i=0; i<Count; i++)
		{
			gePath *P;
			gePath *PO;
			int Index;
			const char *name;
			const char *emptyname="";
			
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					return NULL;
				}
		

			P = geMotion_GetPath(M,i);
			if (P == NULL)
				{
					geMotion_Destroy(&MO);
					return NULL;
				}

			name = geMotion_GetNameOfPath(M, i);
			if (name==NULL)
				name = emptyname;

			if (Log) Log_Output(Log,"Path %d,  '%s'",i,name);
				
			PO = Pop_PathOptimize(P,Tolerance);
			if (timeGetTime() > LastTime+MILLISECONDS_BETWEEN_NOTIFICATIONS)
			{
					LastTime = timeGetTime();
					Printf("\tCompression approximately %d%% complete\n", (int)(100.0f * (float)i/(float)Count));
			}
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					if (PO != NULL)
						gePath_Destroy(&PO);
					return NULL;
				}
		
			if (PO!=NULL)
				{
					int TKeyCount,TKeyCountO;
					int RKeyCount,RKeyCountO;
					
					TKeyCount  = gePath_GetKeyframeCount(P,GE_PATH_TRANSLATION_CHANNEL);
					TKeyCountO = gePath_GetKeyframeCount(PO,GE_PATH_TRANSLATION_CHANNEL);
					RKeyCount  = gePath_GetKeyframeCount(P,GE_PATH_ROTATION_CHANNEL);
					RKeyCountO = gePath_GetKeyframeCount(PO,GE_PATH_ROTATION_CHANNEL);
					if (Log) Log_Output(Log,"Path %4d: Translation: %4d keys to %4d keys (%4d keys saved)",
							i,
							TKeyCount, TKeyCountO, (TKeyCount-TKeyCountO) );
					if (Log) Log_Output(Log,"Path %4d:    Rotation: %4d keys to %4d keys (%4d keys saved)",
							i,
							RKeyCount, RKeyCountO, (RKeyCount-RKeyCountO) );
					if (Log) Log_Output(Log,"-");
					if (geMotion_AddPath(MO,PO,geMotion_GetNameOfPath(M,i),&Index)==GE_FALSE)
						{
							geMotion_Destroy(&MO);
							return NULL;
						}
					gePath_Destroy(&PO);

				}
			else
				{
					if (Log) Log_Output(Log,"Unable to optimize Path %4d: Name='%s'",i,name);
					PO = gePath_CreateCopy(P);
					if (PO==NULL)
						{
							geMotion_Destroy(&MO);
							return NULL;
						}
					if (geMotion_AddPath(MO,PO,geMotion_GetNameOfPath(M,i),&Index)==GE_FALSE)
						{
							geMotion_Destroy(&MO);
							return NULL;
						}
					gePath_Destroy(&PO);
				}
		}
	{
		geFloat T1,T2,T;
		const char *EventString;
		if (geMotion_GetTimeExtents(M,&T1,&T2)==GE_FALSE)
			{
				geMotion_Destroy(&MO);
				return NULL;
			}
		T1-=1.0f;
		T2+=1.0f;		// just make sure to avoid fp weirdnesses.

		geMotion_SetupEventIterator(M,T1,T2);
		while (geMotion_GetNextEvent(M,&T,&EventString)==GE_TRUE)
			{
				if (Log) Log_Output(Log,"Event...%f '%s'",T,EventString);
				if (geMotion_InsertEvent(MO,T,EventString)==GE_FALSE)
					{
						geMotion_Destroy(&MO);
						return NULL;
					}	
			}

	}
	if (Log) Log_Close(&Log);					
	geMotion_Destroy(&M);
	return MO;
}


ReturnCode MopShell_DoMake(MopShell_Options* options,MkUtil_Printf Printf)
{
	ReturnCode retValue = RETURN_SUCCESS;
	geVFile *sf;
	geVFile *df;
	geBoolean ok;
	geMotion* M = NULL;
	geFloat Tolerance;

	// Motion files must be specified
	if(options->SourceMotionFile[0] == 0)
		{
			Printf("ERROR: Must specify a source motion\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	if(options->DestinationMotionFile[0] == 0)
		{
			Printf("ERROR: Must specify a destination motion\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	if (options->OptimizationLevel<0)
		{
			options->OptimizationLevel = 4;
			Printf("Optimization level default = 4\n");
		}
	if (options->OptimizationLevel>MAX_OPTIMIZATION_LEVEL)
		{
			options->OptimizationLevel = 4;
			Printf("Optimization level default = 4\n");
		}

	sf = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->SourceMotionFile,NULL,GE_VFILE_OPEN_READONLY);
	if (sf==NULL)
		{
			Printf("ERROR: Could not open source motion file '%s'.\n", options->SourceMotionFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	M = geMotion_CreateFromFile(sf);
	geVFile_Close(sf);
	if (M==NULL)
		{
			Printf("ERROR: Failed to create motion from source motion file '%s'.\n", options->SourceMotionFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}

		switch (options->OptimizationLevel)
			{
				case (0):
					Tolerance = 0.00005f;
					break;
				case (1):
					Tolerance = 0.00008f;
					break;
				case (2):
					Tolerance = 0.0001f;
					break;
				case (3):
					Tolerance = 0.0005f;
					break;
				case (4):
					Tolerance = 0.001f;
					break;
				case (5):
					Tolerance = 0.005f;
					break;
				case (6):
					Tolerance = 0.01f;
					break;
				case (7):
					Tolerance = 0.05f;
					break;
				case (8):
					Tolerance = 0.1f;
					break;
				case (9):
					Tolerance = 0.5f;
					break;
				default:
					Printf("ERROR: Bad optimization level.\n");
					MkUtil_AdjustReturnCode(&retValue,RETURN_ERROR);
					return retValue;
			}

					

	M = MopShell_Optimize(M,Tolerance,options->LogFile,Printf);
	if (M==NULL)	
		{
			Printf("ERROR: unable to optimize motion '%s'.\n", options->SourceMotionFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}

	if (options->TextOutput)
		{
			df = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->DestinationMotionFile,NULL,GE_VFILE_OPEN_CREATE);
			//df = fopen(options->DestinationMotionFile,"w");
		}
	else
		{
			df = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->DestinationMotionFile,NULL,GE_VFILE_OPEN_CREATE);
			//df = fopen(options->DestinationMotionFile,"wb");
		}
	if (df==NULL)
		{
			Printf("ERROR: Could not open destination motion file '%s'.\n", options->DestinationMotionFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			unlink(options->DestinationMotionFile);
			return retValue;
		} 
	
	if (options->TextOutput)
		{
			ok = geMotion_WriteToFile(M,df);
		}
	else
		{
			ok = geMotion_WriteToBinaryFile(M,df);
		}
	if (ok == GE_TRUE)
		{
			ok = geVFile_Close(df);
		}
	geMotion_Destroy(&M);
	if (ok == GE_FALSE)
		{
			Printf("ERROR: Failed to write destination motion file '%s'.\n", options->DestinationMotionFile);
			unlink(options->DestinationMotionFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	Printf("Level %d optimization of '%s' into '%s' complete\n", options->OptimizationLevel,
			options->SourceMotionFile,options->DestinationMotionFile);

	return retValue;
}

void MopShell_OutputUsage(MkUtil_Printf Printf)
{
	//COLS: 0         1         2         3         4         5         6         7       | 8
	Printf("\n");
	Printf("Optimizes a motion file.  Default output format is text,\n");
	Printf("with optimization level 0.\n");
	Printf("\n");
	Printf("MOP [options] [/B][/T] [/On] /S<source motion file> \n");
	Printf("         /D<destination motion file> /L<log file>\n");
	Printf("\n");
	Printf("/S<motionfile>   Specifies source motion file (Required).\n");
	Printf("/D<motionfile>   Specifies destination motion file (Required).\n");
	Printf("/Ox              Specifies motion optimization level x.\n");
	Printf("/T               Specifies text destination motion file (default).\n");
	Printf("/B               Specifies binary destination motion file.\n");
	Printf("/L               Specifies optional log file for optimization stats.\n");
	Printf("\n");
	Printf("Destination motion file will be overwritten.\n");
	
}

MopShell_Options* MopShell_OptionsCreate()
{
	MopShell_Options* pOptions;

	pOptions = GE_RAM_ALLOCATE_STRUCT(MopShell_Options);
	if(pOptions != NULL)
		{
			*pOptions = DefaultOptions;
		}

	return pOptions;
}

void MopShell_OptionsDestroy(MopShell_Options** ppOptions)
{
	MopShell_Options* p;

	assert(ppOptions != NULL);
	assert(*ppOptions != NULL);

	p = *ppOptions;

	geRam_Free(*ppOptions);

	*ppOptions = NULL;
}

ReturnCode MopShell_ParseOptionString(MopShell_Options* options, 
						const char* string, MK_Boolean InScript,
						MkUtil_Printf Printf)
{
	ReturnCode retValue = RETURN_SUCCESS;
	int Index;

	assert(options != NULL);
	assert(string != NULL);

#define NO_FILENAME_WARNING Printf("WARNING: '%s' specified with no filename\n", string)

	if( (string[0] == '-') || (string[0] == '/') )
	{
		switch(string[1])
		{
		case 't':
		case 'T':
			if( options->OutputSet != MK_FALSE )
				{
					Printf("WARNING: Multiple output format specification '%s' \n", string);
					retValue = RETURN_WARNING;
				}
			options->TextOutput = MK_TRUE;
			options->OutputSet  = MK_TRUE;
			break;
		case 'b':
		case 'B':
			if( options->OutputSet != MK_FALSE )
				{
					Printf("WARNING: Multiple output format specification '%s' \n", string);
					retValue = RETURN_WARNING;
				}
			options->TextOutput = MK_TRUE;
			options->OutputSet  = MK_TRUE;
			break;

		case 's':
		case 'S':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( options->SourceMotionFile[0] != 0 )
				{
					Printf("WARNING: Multiple '%s' Specification\n", string);
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->SourceMotionFile, string + 2);
				}
			}
			break;
		
		case 'l':
		case 'L':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( options->LogFile[0] != 0 )
				{
					Printf("WARNING: Multiple '%s' Specification\n", string);
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->LogFile, string + 2);
				}
			}
			break;
		
		
		case 'd':
		case 'D':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( options->DestinationMotionFile[0] != 0 )
				{
					Printf("WARNING: Multiple '%s' Specification\n",string);
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->DestinationMotionFile, string + 2);
				}
			}
			break;

		case 'o':
		case 'O':
			if(string[2] == 0)
			{
				Printf("WARNING: %o specified with no level number\n", string);
				retValue = RETURN_WARNING;
			}
			else
			{
				int level = string[2]-'0';
				if (options->OptimizationLevel>0)
					Printf("WARNING: Multiple %o Specification.\n",string);
				if (level<0 || level>MAX_OPTIMIZATION_LEVEL)
					Printf("WARNING: Optimization level invalid.\n");
				
				options->OptimizationLevel = level;
			}
			break;

		default:
			retValue = RETURN_NOACTION;
		}
	}

	return retValue;

		// unneeded parameters
		InScript;
		Index;
}
