/****************************************************************************************/
/*  MAKE.C																				*/
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
/* notes:
	would like to out-of-date things whose build options have changed 
	would like to out-of-date the actor if any options have changed
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <time.h>
#include <string.h>
#include <direct.h>		// _mkdir
#include "GENESIS.H"
#include "actor.h"
#include "body.h"
#include "motion.h"
#include "make.h"
#include "AProject.h"
#include "MXScript.h"
#include "RAM.H"
#include "mkbody.h"
#include "mkmotion.h"
#include "mopshell.h"
#include "mkactor.h"
#include "filepath.h"

#pragma todo ("Need a force-build flag for each target")


#define BIG (2048)

static geBoolean Make_GlobalInterruptFlag = GE_FALSE;

void Make_SetInterruptFlag (geBoolean State)
{
	Make_GlobalInterruptFlag = State;
}

int MkUtil_Interrupt(void)
{
	return Make_GlobalInterruptFlag;
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


geBoolean Make_IsTargetOutOfDate( const char *TargetFileName, 
								  const char *SourceFileName, 
								  geBoolean  *OutOfDate,
								  MkUtil_Printf Printf)
{
	long Handle;
	struct _finddata_t SourceData,TargetData;

	assert( Printf         != NULL );
	assert( OutOfDate      != NULL );
	assert( SourceFileName != NULL );
	assert( TargetFileName != NULL );



	Handle = _findfirst( SourceFileName, &SourceData );
	if (Handle == -1)
		{
			Printf("Error: Source file '%s' not found\n",SourceFileName);
			return GE_FALSE;		
		}
		
	_findclose(Handle);
	
	Handle = _findfirst( TargetFileName, &TargetData );
	if (Handle == -1)
		{
			*OutOfDate = GE_TRUE; // is out of date if target isn't there.
			Printf("'%s' out of date.  (Doesn't exist)\n",TargetFileName,SourceFileName);
			return GE_TRUE;		
		}
	_findclose(Handle);

	if (SourceData.time_write >= TargetData.time_write)
		{
			*OutOfDate = GE_TRUE;
			Printf("'%s' out of date.  (Older than '%s')\n",TargetFileName,SourceFileName);
			return GE_TRUE;
		}

	*OutOfDate = GE_FALSE;
	return GE_TRUE;
}

void Make_TargetFileName( char *TargetFileName, const char *SourceFileName, 
					const char *ObjDir, const char *NewExt )
{
	char drive[BIG];
	char dir[BIG];
	char fname[BIG];
	char ext[BIG];
	assert( SourceFileName );
	assert( ObjDir );
	assert( NewExt );
	assert( TargetFileName );
	
	_splitpath( SourceFileName, drive,dir,fname,ext);
	_makepath( TargetFileName, "",ObjDir,fname,NewExt );
}


geBoolean Make_CopyBodyFile( const char *TargetFileName, 
						     const char *SourceFileName,
							 MkUtil_Printf Printf)
{
	geVFile *VF;
	geBody* pBody = NULL;
	geBoolean Worked;

	assert( Printf         != NULL );
	assert( SourceFileName != NULL );
	assert( TargetFileName != NULL );
	Printf("\tCopying Body file '%s' into '%s'\n",SourceFileName,TargetFileName);

	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,SourceFileName,NULL,GE_VFILE_OPEN_READONLY);
	if(VF == NULL)
	{
		Printf("ERROR: Could not open source body file '%s'\n", SourceFileName);
		return GE_FALSE;
	}
	
	pBody = geBody_CreateFromFile(VF);
	geVFile_Close(VF);
	if(pBody == NULL)
		{
			Printf("ERROR: Failed to load source body from file '%s'\n", SourceFileName);
			geVFile_Close(VF);
			return GE_FALSE;
		}

	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,TargetFileName,NULL,GE_VFILE_OPEN_CREATE);
	if(VF == NULL)
	{
		Printf("ERROR: Could not open target body file '%s'\n", TargetFileName);
		geBody_Destroy(&pBody);
		return GE_FALSE;
	}

	Worked = geBody_WriteToFile(pBody,VF);
	if (geVFile_Close(VF)==GE_FALSE)
		Worked = GE_FALSE;
	geBody_Destroy(&pBody);

	if (Worked == GE_FALSE)
		{
			Printf("Error:  Failed to write target body to file '%s'",TargetFileName);
			return GE_FALSE;
		}
	return GE_TRUE;
}

geBoolean Make_CopyMotionFile( const char *TargetFileName, 
						     const char *SourceFileName,
							 MkUtil_Printf Printf)
{
	geVFile *VF;
	geMotion* pMotion = NULL;
	geBoolean Worked;

	assert( Printf         != NULL );
	assert( SourceFileName != NULL );
	assert( TargetFileName != NULL );

	Printf("\tCopying Motion file '%s' into '%s'\n",SourceFileName,TargetFileName);

	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,SourceFileName,NULL,GE_VFILE_OPEN_READONLY);
	if(VF == NULL)
	{
		Printf("ERROR: Could not open source motion file '%s'\n", SourceFileName);
		return GE_FALSE;
	}
	
	pMotion = geMotion_CreateFromFile(VF);
	geVFile_Close(VF);
	if(pMotion == NULL)
		{
			Printf("ERROR: Failed to load source motion file '%s'\n", SourceFileName);
			geVFile_Close(VF);
			return GE_FALSE;
		}

	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,TargetFileName,NULL,GE_VFILE_OPEN_CREATE);
	if(VF == NULL)
	{
		Printf("ERROR: Could not open target motion file '%s'\n", TargetFileName);
		geMotion_Destroy(&pMotion);
		return GE_FALSE;
	}

	Worked = geMotion_WriteToFile(pMotion,VF);
	if (geVFile_Close(VF)==GE_FALSE)
		Worked = GE_FALSE;
	geMotion_Destroy(&pMotion);

	if (Worked == GE_FALSE)
		{
			Printf("Error:  Failed to write target motion file '%s'",TargetFileName);
			return GE_FALSE;
		}
	return GE_TRUE;
}


geBoolean Make_Body_NFO_OutOfDate( AProject *Prj, 
			geBoolean *OutOfDate,
			MkUtil_Printf Printf)
{
	ApjBodyFormat BodyFmt;
	const char *SourceName;
	const char *ObjDir;
	char TargetName[BIG];
	
	assert( Prj       != NULL );
	assert( OutOfDate != NULL );
	assert( Printf    != NULL );
	
	*OutOfDate = GE_FALSE;

	BodyFmt = AProject_GetBodyFormat(Prj);
	if (BodyFmt != ApjBody_Max)
		return GE_TRUE;
	
	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error:  Unable to get temporary path\n");
			return GE_FALSE;
		}
	
	SourceName = AProject_GetBodyFilename (Prj);
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file name for body\n");
			return GE_FALSE;
		} 
	Make_TargetFileName( TargetName, SourceName, ObjDir,"NFO" );
	if (Make_IsTargetOutOfDate(TargetName, SourceName, OutOfDate,Printf ) == GE_FALSE)
		return GE_FALSE;
	
	return GE_TRUE;
}


geBoolean Make_Body_BDY_OutOfDate(AProject *Prj, 
							 geBoolean *OutOfDate,
							 MkUtil_Printf Printf)
{
	ApjBodyFormat Fmt;
	char TargetName[BIG];
	const char *SourceName;
	const char *ObjDir;

	assert( Prj       != NULL );
	assert( OutOfDate != NULL );
	assert( Printf    != NULL );
	
	
	Fmt = AProject_GetBodyFormat(Prj);
	switch (Fmt)
		{
			case (ApjBody_Max):	
				if (Make_Body_NFO_OutOfDate(Prj, OutOfDate, Printf)==GE_FALSE)
					return GE_FALSE;
				if (*OutOfDate != GE_FALSE)
					return GE_TRUE;
				break;
			case (ApjBody_Nfo):
			case (ApjBody_Bdy):
			case (ApjBody_Act):
				break;
			case (ApjBody_Invalid):
			default:
				{
					Printf("Error: unrecognized format specifier for body (%d) \n",Fmt);
					return GE_FALSE;
				}
		}

	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error: Unable to get temporary path\n");
			return GE_FALSE;
		}

	SourceName = AProject_GetBodyFilename (Prj);
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file name for body\n");
			return GE_FALSE;
		} 
	Make_TargetFileName( TargetName, SourceName, ObjDir, "BDY");

	if (Make_IsTargetOutOfDate( TargetName, SourceName, OutOfDate, Printf) == GE_FALSE)
		{	// already posted
			return GE_FALSE;
		}
	return GE_TRUE;
}

geBoolean Make_SourceOutOfDateFromBDY( AProject *Prj, 
			const char *FileName,
			geBoolean *OutOfDate,
			MkUtil_Printf Printf)
{
	char TargetName[BIG];
	const char *SourceName;
	const char *ObjDir;

	assert( Prj       != NULL );
	assert( OutOfDate != NULL );
	assert( Printf    != NULL );
	assert( FileName  != NULL );

	// if body needs to be made, then the source is out-of-date	
	if (Make_Body_BDY_OutOfDate( Prj, OutOfDate, Printf) == GE_FALSE)
		return GE_FALSE;
	if (*OutOfDate == GE_TRUE)
		return GE_TRUE;
	// body doesn't need to be made.  But maybe the source is older than the bdy
	
	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error: Unable to get temporary path\n");
			return GE_FALSE;
		}

	SourceName = AProject_GetBodyFilename (Prj);
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file name for body\n");
			return GE_FALSE;
		} 
	Make_TargetFileName( TargetName, SourceName, ObjDir, "BDY");

	if (Make_IsTargetOutOfDate( FileName, TargetName, OutOfDate, Printf) == GE_FALSE)
		{	// already posted
			return GE_FALSE;
		}

	// now lets see if source is out of date 

	return GE_TRUE;
}


geBoolean Make_Motion_KEY_OutOfDate(AProject *Prj, int MotionIndex,
				geBoolean *OutOfDate, MkUtil_Printf Printf)
{
	const char *ObjDir;
	ApjMotionFormat MotionFmt;
	const char *SourceName;
	char TargetName[BIG];

	assert( Prj       != NULL );
	assert( OutOfDate != NULL );
	assert( Printf    != NULL );
	assert( MotionIndex >= 0  );
	assert( MotionIndex < AProject_GetMotionsCount( Prj ) );
	
	*OutOfDate = GE_FALSE;
	MotionFmt = AProject_GetMotionFormat( Prj, MotionIndex );

	if ( MotionFmt != ApjMotion_Max )
		return GE_TRUE;

	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error:  Unable to get temporary path to check dependencies for motions\n");
			return GE_FALSE;
		}

	SourceName = AProject_GetMotionFilename( Prj, MotionIndex );
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file name for motion %d\n", MotionIndex);
			return GE_FALSE;
		}
	Make_TargetFileName( TargetName, SourceName, ObjDir,"KEY" );
	if (Make_IsTargetOutOfDate(TargetName, SourceName, OutOfDate, Printf ) == GE_FALSE )
		return GE_FALSE;

	return GE_TRUE;
}


geBoolean Make_Motion_MOT_OutOfDate(AProject *Prj, int MotionIndex, 
				geBoolean *OutOfDate, MkUtil_Printf Printf)
{
	const char *ObjDir;
	ApjMotionFormat MotionFmt;
	const char *SourceName;
	char TargetName[BIG];

	assert( Printf != NULL );
	assert( Prj != NULL );
	assert( OutOfDate != NULL );

	
	*OutOfDate = GE_FALSE;

	SourceName = AProject_GetMotionFilename( Prj, MotionIndex );
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file name for motion %d\n", MotionIndex);
			return GE_FALSE;
		}

	MotionFmt = AProject_GetMotionFormat( Prj, MotionIndex );
	switch (MotionFmt)
		{
			case (ApjMotion_Max):
				if (*OutOfDate == GE_TRUE)
					return GE_TRUE;
				if (Make_Motion_KEY_OutOfDate( Prj, MotionIndex, OutOfDate, Printf)== GE_FALSE)
					return GE_FALSE;
				if (*OutOfDate == GE_TRUE)
					return GE_TRUE;
				break;
			case (ApjMotion_Key):
				if (*OutOfDate == GE_TRUE)
					return GE_TRUE;
				break;
// motions from actors not yet supported
//			case (ApjMotion_Act):
			case (ApjMotion_Mot):
				// body needed for mkmotion
				if (Make_SourceOutOfDateFromBDY( Prj, SourceName, OutOfDate, Printf) == GE_FALSE)
					return GE_FALSE;
				break;
			case (ApjMotion_Invalid):
			default:
				{
					Printf("Error: unrecognized format specifier for motion #%d (%d) \n",MotionIndex,MotionFmt);
					return GE_FALSE;
				}
		}

	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error:  Unable to get temporary path to check dependencies for motions\n");
			return GE_FALSE;
		}

	
	Make_TargetFileName( TargetName, SourceName, ObjDir, "MOT" );
	if (Make_IsTargetOutOfDate(TargetName, SourceName, OutOfDate, Printf ) == GE_FALSE )
		return GE_FALSE;
	return GE_TRUE;
}



geBoolean Make_AnyMotion_KEY_OutOfDate(AProject *Prj, 
				int MotionIndexCount, int *MotionIndexArray, 
				geBoolean *OutOfDate,
				MkUtil_Printf Printf)
{
	int i;

	assert( Printf           != NULL );
	assert( Prj              != NULL );
	assert( OutOfDate        != NULL );

	*OutOfDate = GE_FALSE;
	for (i=0; i<MotionIndexCount; i++)
		{
			assert( MotionIndexArray != NULL );
			if (Make_Motion_KEY_OutOfDate( Prj, MotionIndexArray[i], OutOfDate, Printf )==GE_FALSE)
				return GE_FALSE;
			if (*OutOfDate != GE_FALSE)
				return GE_TRUE;
		}
	return GE_TRUE;
}


geBoolean Make_AnyMotion_MOT_OutOfDate(AProject *Prj, 
				int MotionIndexCount, int *MotionIndexArray, 
				geBoolean *OutOfDate,
				MkUtil_Printf Printf)
{
	int i;

	assert( Printf           != NULL );
	assert( Prj              != NULL );
	assert( OutOfDate        != NULL );

	*OutOfDate = GE_FALSE;
	for (i=0; i<MotionIndexCount; i++)
		{
			assert( MotionIndexArray != NULL );
			if (Make_Motion_MOT_OutOfDate( Prj, MotionIndexArray[i], OutOfDate, Printf )==GE_FALSE)
				return GE_FALSE;
			if (*OutOfDate != GE_FALSE)
				return GE_TRUE;
		}
	return GE_TRUE;
}



geBoolean Make_MaxScript( AProject *Prj, 
			AOptions *Options, 
			int DoBody,
			int MotionIndexCount, int *MotionIndexArray, 
			MkUtil_Printf Printf )
{
#define MAXIMUM_MOTIONS_PER_EXPORT 25
	MXScript *Script = NULL;
	char TargetName[BIG];

	const char *SourceName;
	const char *ObjDir;
	const char *Max;
			
	int i;
	//int MotionCount;
	int Times;

	geBoolean OutOfDate = GE_FALSE;

	assert( Prj != NULL );
	assert( MotionIndexCount >= 0 );
	assert( (MotionIndexCount==0) || ((MotionIndexCount > 0) && (MotionIndexArray != NULL)) );
	assert( Options != NULL );
	assert( Printf != NULL );

	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error:  Unable to get temporary path to build MAXScript\n");
			goto Make_MaxScriptError;
		}

	// first, see if we even need to do any exporting:
	if (DoBody!=GE_FALSE)
		{
			if (Make_Body_NFO_OutOfDate( Prj, &OutOfDate, Printf) == GE_FALSE)
				goto Make_MaxScriptError;
		}

	if (OutOfDate == GE_FALSE)
		{
			if (Make_AnyMotion_KEY_OutOfDate(Prj, MotionIndexCount, 
							MotionIndexArray, &OutOfDate, Printf) == GE_FALSE)
					goto Make_MaxScriptError;
		}

	if ( OutOfDate == GE_FALSE)
		{
			return GE_TRUE;
		}

	Max = AOptions_Get3DSMaxPath( Options );
	if (Max == NULL)
		{
			Printf("Error: Cant Get 3DS MAX exe file name\n");
			goto Make_MaxScriptError;
		}
	
	if (MXScript_ArePluginsInstalled(Max,Printf)==GE_FALSE)
		{
			Printf("Error: Plugin missing. Max export halted.\n");
			goto Make_MaxScriptError;
		}

	Printf("\tBuilding 3DS MAX MAXScript\n");
	
	// build the script
	Script = MXScript_StartScript(ObjDir,Printf);
	if (Script == NULL)
		{
			Printf("Error: Can't start script file\n");
			goto Make_MaxScriptError;
		}

	if (DoBody!=GE_FALSE)
		{
			if (Make_Body_NFO_OutOfDate( Prj, &OutOfDate, Printf) == GE_FALSE)
				goto Make_MaxScriptError;
			if (OutOfDate!=GE_FALSE)
				{
					SourceName = AProject_GetBodyFilename (Prj);
					if (SourceName == NULL)
						{
							Printf("Error: Can't get source file name for body\n");
							goto Make_MaxScriptError;
						} 

					Make_TargetFileName( TargetName, SourceName, ObjDir, "NFO" );
					if (MXScript_AddExport(Script,SourceName,TargetName,Printf)==GE_FALSE)
						{
							Printf("Error: Can't add body export to script file\n");
							goto Make_MaxScriptError;
						}
				}
		}

	i=0;
	Times=1;

	while (i<MotionIndexCount || (DoBody!=GE_FALSE))
		{
			int j;
			if (Script==NULL)
				{
					Printf("\tBuilding 3DS MAX MAXScript (%d)\n",Times);
					
					// build the script
					Script = MXScript_StartScript(ObjDir,Printf);
					if (Script == NULL)
						{
							Printf("Error: Can't start script file\n");
							goto Make_MaxScriptError;
						}
				}
					
			//MotionCount = AProject_GetMotionsCount( Prj );
			for (j=0; j<MAXIMUM_MOTIONS_PER_EXPORT && i<MotionIndexCount; i++,j++)
				{
					if (Make_Motion_KEY_OutOfDate(Prj, MotionIndexArray[i],&OutOfDate,Printf) == GE_FALSE)
						goto Make_MaxScriptError;

					if (OutOfDate != GE_FALSE)
						{
							SourceName = AProject_GetMotionFilename( Prj, MotionIndexArray[i] );
							if (SourceName == NULL)
								{
									Printf("Error: Can't get source file name for motion %d\n", MotionIndexArray[i]);
									goto Make_MaxScriptError;
								}
							Make_TargetFileName( TargetName, SourceName, ObjDir,"KEY" );
							if (MXScript_AddExport(Script,SourceName,TargetName,Printf)==GE_FALSE)
								{
									Printf("Error: Can't add '%s' export to script file\n",SourceName);
									goto Make_MaxScriptError;
								}
						}
				}

			if (MXScript_EndScript(Script,Printf)==GE_FALSE)
				{
					Printf("Error: Failed to close script file\n");
					goto Make_MaxScriptError;
				}
			
			// run the script
			Printf("\tRunning 3DS MAX MAXScript\n");

			if ( MXScript_RunScript( Script, Max, Printf ) == GE_FALSE)
				{
					Printf("Error: 3DS MAX export script failed\n");
					goto Make_MaxScriptError;
				}

			MXScript_Destroy( Script );
			Script = NULL;
			Times++;
			DoBody = GE_FALSE;
		}

	return GE_TRUE;



Make_MaxScriptError:
	if ( Script != NULL )
		MXScript_Destroy( Script );
	return GE_FALSE;

}


geBoolean Make_Body_BDY(AProject *Prj, AOptions *BuildOptions, MkUtil_Printf Printf)
{
	ApjBodyFormat Fmt;
	ReturnCode RVal;
		
	char OptionString[BIG];
	char TargetName[BIG];
	char NFOName[BIG];
	const char *SourceName;
	const char *ObjDir;
	geBoolean OutOfDate;
	
	assert( Prj      != NULL );
	assert( BuildOptions  != NULL );
	assert( Printf   != NULL );

	if (Make_Body_BDY_OutOfDate( Prj, &OutOfDate, Printf) == GE_FALSE)
		return GE_FALSE;

	if (OutOfDate == GE_FALSE)
		return GE_TRUE;
	
	Printf("\tMaking Body\n");
	
	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error: (Make_Body) Unable to get temporary path\n");
			return GE_FALSE;
		}
	
	SourceName = AProject_GetBodyFilename (Prj);
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file name for body\n");
			return GE_FALSE;
		} 

	Make_TargetFileName( TargetName, SourceName, ObjDir, "BDY");
	strcpy(NFOName,SourceName);

	Fmt = AProject_GetBodyFormat(Prj);
	switch (Fmt)
		{
			case (ApjBody_Max):	
				if (Make_MaxScript( Prj, BuildOptions, GE_TRUE, 
									0, NULL, Printf ) == GE_FALSE)
					{
						Printf("Error: unable to complete 3DS MAX script and export step for Make_Body\n");
						return GE_FALSE;
					}
				Make_TargetFileName( NFOName, SourceName, ObjDir, "NFO");
				// fall through
			case (ApjBody_Nfo):
				{
					MkBody_Options *Options;
					const char *TexturePathName;
				
					Printf("\tMaking Body '%s' from NFO '%s'\n", TargetName,NFOName);

					Options = MkBody_OptionsCreate();
					if (Options == NULL)
						{
							Printf("Error: unable to allocate option block for mkBody\n");
							return GE_FALSE;
						}
					sprintf(OptionString,"-R");
					RVal = MkBody_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkBody:\n");
							Printf(OptionString);
							MkBody_OptionsDestroy(&Options);
							return GE_FALSE;
						}

					sprintf(OptionString,"-C");
					RVal = MkBody_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkBody:\n");
							Printf(OptionString);
							MkBody_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					sprintf(OptionString,"-B%s",TargetName);
					RVal = MkBody_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkBody:\n");
							Printf(OptionString);
							MkBody_OptionsDestroy(&Options);
							return GE_FALSE;
						}

					sprintf(OptionString,"-N%s",NFOName);
					RVal = MkBody_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkBody:\n");
							Printf(OptionString);
							MkBody_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					
					TexturePathName = AProject_GetMaterialsPath(Prj);
					if (TexturePathName == NULL)
						{
							Printf("Error: Can't get texture path for body\n");
							MkBody_OptionsDestroy(&Options);
							return GE_FALSE;
						} 
					if (TexturePathName[0] == 0)
						{
							Printf("Warning: no material path specified\n");
						}
					else
						{
							sprintf(OptionString,"-T%s",TexturePathName);
							RVal = MkBody_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
							if (RVal != RETURN_SUCCESS)
								{
									Printf("Error: unable to set options into MkBody:\n");
									Printf(OptionString);
									MkBody_OptionsDestroy(&Options);
									return GE_FALSE;
								}
						}
					
					{
						int i;
						int MatCnt = AProject_GetMaterialsCount(Prj);
						assert( MatCnt >= 0 );
						for (i=0; i<MatCnt; i++)
							{
								ApjMaterialFormat MatFmt; 
								GE_RGBA Color;
								const char *MatName;
								const char *MatFileName;
								MatFmt  = AProject_GetMaterialFormat(Prj,i);
								MatName = AProject_GetMaterialName(Prj,i);
								MatFileName = AProject_GetMaterialTextureFilename (Prj, i);

								if (MatName == NULL)
									{
										Printf("Error: unable to get extra material name %d\n",i);
										Printf(OptionString);
										MkBody_OptionsDestroy(&Options);
										return GE_FALSE;
									}
								
								if (strlen(MatName) == 0)
									{
										Printf("Error: empty extra material name %d\n",i);
										Printf(OptionString);
										MkBody_OptionsDestroy(&Options);
										return GE_FALSE;
									}
								switch (MatFmt)
									{
										case (ApjMaterial_Color):
											Color = AProject_GetMaterialTextureColor (Prj,i);
											sprintf(OptionString,"-M(RGB) %s: %f %f %f",MatName,Color.r,Color.g,Color.b);
											break;
										case (ApjMaterial_Texture):
											if (MatFileName == NULL)
												{
													Printf("Error: unable to get extra material filename %d\n",i);
													Printf(OptionString);
													MkBody_OptionsDestroy(&Options);
													return GE_FALSE;
												}
											if (strlen(MatFileName) == 0)
												{
													Printf("Error: empty extra material filename %d\n",i);
													Printf(OptionString);
													MkBody_OptionsDestroy(&Options);
													return GE_FALSE;
												}
											sprintf(OptionString,"-M(MAP) %s: %s",MatName,MatFileName);
											break;
										default:
											assert(0);
									}
								RVal = MkBody_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
									if (RVal != RETURN_SUCCESS)
										{
											Printf("Error: unable to set extra material %d options into MkBody:\n",i);
											Printf(OptionString);
											MkBody_OptionsDestroy(&Options);
											return GE_FALSE;
										}
							}
					}


					RVal = MkBody_DoMake(Options,Printf);
					MkBody_OptionsDestroy(&Options);

					if (RVal == RETURN_ERROR )
						{
							Printf("Error: Failed to build '%s' file from '%s' file\n",
										TargetName,NFOName);
							return GE_FALSE;
						}
					if (RVal == RETURN_WARNING)
						{
							Printf("Warnings issued during building of '%s' from '%s'\n",
										TargetName,NFOName);
						}
					Printf("\tBody file '%s' successfully built\n",TargetName);
					return GE_TRUE;
				}
			case (ApjBody_Bdy):
				{
					if (Make_CopyBodyFile(TargetName, SourceName,Printf )==GE_FALSE)
						{
							Printf("Error: failed to copy body file '%s' into work file '%s'\n", SourceName,TargetName);
							return GE_FALSE;
						}
					Printf("\tBody file '%s' successfully prepared\n",TargetName);
					return GE_TRUE;
				}
			case (ApjBody_Act):
				{
					Printf("Error: getting body from existing actor file not yet implemented\n");
					return GE_FALSE;
				}
			case (ApjBody_Invalid):
			default:
				{
					Printf("Error: unrecognized format specifier for body\n");
					return GE_FALSE;
				}
		}
}


geBoolean Make_Optimize_Motion( char *FromFile, char *ToFile, char *LogFile, int Level, MkUtil_Printf Printf )
{
	assert( Printf         != NULL );
	assert( FromFile       != NULL );
	assert( ToFile         != NULL );
	assert( LogFile        != NULL );
	assert( (Level >=0) && (Level<=9));

	Printf("\tCompressing Motion '%s'    (Details in log file '%s')\n",ToFile,LogFile);

	{
		char OptionString[BIG];
		MopShell_Options *Options;
		ReturnCode RVal;
	
		Options = MopShell_OptionsCreate();
		if (Options == NULL)
			{
				Printf("Error: unable to allocate option block for motion optimizer\n");
				return GE_FALSE;
			}
		sprintf(OptionString,"-O%d",Level);
		RVal = MopShell_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
		if (RVal != RETURN_SUCCESS)
			{
				Printf("Error: unable to set options into motion optimizer:\n");
				Printf(OptionString);
				MopShell_OptionsDestroy(&Options);
				return GE_FALSE;
			}
		
		sprintf(OptionString,"-S%s",FromFile);
		RVal = MopShell_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
		if (RVal != RETURN_SUCCESS)
			{
				Printf("Error: unable to set options into motion optimizer:\n");
				Printf(OptionString);
				MopShell_OptionsDestroy(&Options);
				return GE_FALSE;
			}
		
		sprintf(OptionString,"-D%s",ToFile);
		RVal = MopShell_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
		if (RVal != RETURN_SUCCESS)
			{
				Printf("Error: unable to set options into motion optimizer:\n");
				Printf(OptionString);
				MopShell_OptionsDestroy(&Options);
				return GE_FALSE;
			}
		sprintf(OptionString,"-L%s",LogFile);
		RVal = MopShell_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
		if (RVal != RETURN_SUCCESS)
			{
				Printf("Error: unable to set options into motion optimizer:\n");
				Printf(OptionString);
				MopShell_OptionsDestroy(&Options);
				return GE_FALSE;
			}
		

		RVal = MopShell_DoMake(Options,Printf);
		MopShell_OptionsDestroy(&Options);

		if (RVal == RETURN_ERROR )
			{
				Printf("Error: motion optimize failed\n");
				return GE_FALSE;
			}
		if (RVal == RETURN_WARNING)
			{
				Printf("Warnings issued during motion optimize\n");
			}
	}
	
	return GE_TRUE;	
}


geBoolean Make_Motion_MOT( AProject *Prj, AOptions *AOptions, 
							int MotionIndex, MkUtil_Printf Printf)
// makes MOT from KEY, MOT, or ACT
{
	char OptionString[BIG];
	char TargetName[BIG];
	char FinalTargetName[BIG];
	char BodyName[BIG];
	char KEYName[BIG];

	const char *MotionName;
	const char *SourceName;
	const char *ObjDir;
	const char *BoneName;
	const char *Empty = "";
	ApjMotionFormat Fmt;
	ReturnCode RVal;
	geBoolean OutOfDate;
	
	assert( Prj      != NULL );
	assert( Printf   != NULL );
	assert( AOptions != NULL );
	
	MotionName = AProject_GetMotionName (Prj,MotionIndex);
	if (MotionName == NULL) MotionName =Empty;

	Printf("\tMaking Motion #%d '%s'\n",MotionIndex, MotionName);
	
	if (Make_Motion_MOT_OutOfDate( Prj, MotionIndex, &OutOfDate, Printf) == GE_FALSE)
		return GE_FALSE;
	if (OutOfDate == GE_FALSE)
		{
			Printf("\tMotion #%d '%s' is up to date \n",MotionIndex, MotionName);
			return GE_TRUE;
		}

	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error: (Make_Motion) Unable to get temporary path\n");
			return GE_FALSE;
		}

	SourceName = AProject_GetBodyFilename (Prj);
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source name for body\n");
			return GE_FALSE;
		} 

	Make_TargetFileName( BodyName, SourceName, ObjDir, "BDY");


	SourceName = AProject_GetMotionFilename (Prj,MotionIndex);
	if (SourceName == NULL)
		{
			Printf("Error: Can't get source file for motion #%d '%s'\n",MotionIndex,MotionName);
			return GE_FALSE;
		} 
	
	Make_TargetFileName( TargetName, SourceName, ObjDir, "MO1");
	Make_TargetFileName( FinalTargetName, SourceName, ObjDir, "MOT");
	strcpy(KEYName,SourceName);

	Fmt = AProject_GetMotionFormat(Prj,MotionIndex);
	switch (Fmt)
		{
			case (ApjMotion_Max):
				{
					int MotionIndexArray[1];
					MotionIndexArray[0] = MotionIndex;
					if (Make_MaxScript( Prj, AOptions, GE_FALSE, 1, MotionIndexArray, Printf ) == GE_FALSE)
						{
							Printf("Error: unable to complete 3DS MAX script and export step for Make_Motion\n");
							return GE_FALSE;
						}
					Make_TargetFileName( KEYName, SourceName, ObjDir, "KEY");
				}
				// fall through
			case (ApjMotion_Key):
				{
					MkMotion_Options *Options;
	
					if (Make_Body_BDY( Prj, AOptions, Printf)==GE_FALSE)
						return GE_FALSE;	
					
					MotionName = AProject_GetMotionName (Prj,MotionIndex);
					if (MotionName == NULL) MotionName =Empty;
					
					BoneName = AProject_GetMotionBone(Prj,MotionIndex);
					if (BoneName == NULL) BoneName =Empty;
					
					Printf("\tMaking Motion %d (Name = '%s'): '%s' from KEY '%s'\n", MotionIndex, MotionName, TargetName,KEYName);

					Options = MkMotion_OptionsCreate();
					if (Options == NULL)
						{
							Printf("Error: unable to allocate option block for mkMotion\n");
							return GE_FALSE;
						}
					sprintf(OptionString,"-C");
					RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkMotion:\n");
							Printf(OptionString);
							MkMotion_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					
					sprintf(OptionString,"-E");
					RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkMotion:\n");
							Printf(OptionString);
							MkMotion_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					
					sprintf(OptionString,"-M%s",TargetName);
					RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkMotion:\n");
							Printf(OptionString);
							MkMotion_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					
					sprintf(OptionString,"-K%s",KEYName);
					RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkMotion:\n");
							Printf(OptionString);
							MkMotion_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					sprintf(OptionString,"-B%s",BodyName);
					RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
					if (RVal != RETURN_SUCCESS)
						{
							Printf("Error: unable to set options into MkMotion:\n");
							Printf(OptionString);
							MkMotion_OptionsDestroy(&Options);
							return GE_FALSE;
						}
					if (MotionName[0] != 0)
						{
							sprintf(OptionString,"-N%s",MotionName);
							RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
							if (RVal != RETURN_SUCCESS)
								{
									Printf("Error: unable to set options into MkMotion:\n");
									Printf(OptionString);
									MkMotion_OptionsDestroy(&Options);
									return GE_FALSE;
								}
						}
									
					{
						char BName[BIG];
						char *BPtr;
						char *BStart;
						strcpy(BName,BoneName);
						BPtr = BName;
						do
							{
								BStart = BPtr;
								BPtr = strchr(BName,';');
								if (BStart != NULL)
									{
										if (BPtr != NULL)
											{
												*BPtr=0;
												BPtr++;
											}
										if (BStart[0] != 0)
										{
											sprintf(OptionString,"-R%s", BStart);
											RVal = MkMotion_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
											if (RVal != RETURN_SUCCESS)
												{
													Printf("Error: unable to set options into MkMotion:\n");
													Printf(OptionString);
													MkMotion_OptionsDestroy(&Options);
													return GE_FALSE;
												}
										}
									}
							}
						while (BPtr != NULL);
					}	
					
					RVal = MkMotion_DoMake(Options,Printf);
					MkMotion_OptionsDestroy(&Options);
	
					if (RVal == RETURN_ERROR )
						{
							Printf("Error: Failed to build '%s' file from '%s' file\n",
										TargetName,KEYName);
							return GE_FALSE;
						}
					if (RVal == RETURN_WARNING)
						{
							Printf("Warnings issued during building of '%s' from '%s'\n",
										TargetName,KEYName);
						}
				}
				break;
			case (ApjMotion_Mot):
				if (Make_CopyMotionFile(TargetName, SourceName,Printf )==GE_FALSE)
					{
						Printf("Error: failed to copy motion fle '%s' into work file '%s'\n", SourceName,TargetName);
						return GE_FALSE;
					}
				break;
// Motions from actors not yet implemented
/*
			case (ApjMotion_Act):
				{
					Printf("Error: getting motion from existing actor file not yet implemented\n");
					return GE_FALSE;
				}
*/
			case (ApjMotion_Invalid):
			default:
				{
					Printf("Error: unrecognized format specifier for motion '%s'\n",SourceName);
					return GE_FALSE;
				}				
		}


	if (AProject_GetMotionOptimizationFlag ( Prj, MotionIndex ) == GE_FALSE)
		{
			if (Make_CopyMotionFile( FinalTargetName, TargetName, Printf)==GE_FALSE)
				{
					Printf("Error: unable to copy temporary motion file '%s' to final '%s'\n",TargetName, FinalTargetName);
					return GE_FALSE;
				}
		}
	else
		{
			char LogName[BIG];
			
			Make_TargetFileName( LogName, SourceName, ObjDir, "LOG");
	
			if (Make_Optimize_Motion( TargetName, FinalTargetName, LogName, 
						AProject_GetMotionOptimizationLevel ( Prj, MotionIndex ), Printf)==GE_FALSE)
				{
					Printf("Error: unable to optimize temporary motion file '%s' to final '%s'\n",TargetName, FinalTargetName);
					return GE_FALSE;
				}
		}
				
	return GE_TRUE;	
}



geBoolean Make_Body(AProject *Prj, AOptions *BuildOptions, MkUtil_Printf Printf)
{
	geBoolean OutOfDate;
	
	assert( Prj      != NULL );
	assert( BuildOptions  != NULL );
	assert( Printf   != NULL );

	if (Make_Body_BDY_OutOfDate( Prj, &OutOfDate, Printf) == GE_FALSE)
		return GE_FALSE;
	
	if (OutOfDate == GE_FALSE)
		{
			Printf("\tBody is up to date\n");
			return GE_TRUE;
		}

	if (Make_Body_BDY( Prj, BuildOptions, Printf )==GE_FALSE)
		{
			Printf("Error: Failed to make BDY file\n");
			return GE_FALSE;
		}
	
	return GE_TRUE;
}


geBoolean Make_Actor_ACT_OutOfDate( AProject *Prj, geBoolean *OutOfDate, MkUtil_Printf Printf)
{
	int MotionCount;
	int *MotionIndexArray;
	int i;

	assert( Prj       != NULL );
	assert( OutOfDate != NULL );
	assert( Printf    != NULL );

	*OutOfDate = GE_FALSE;

	if (Make_Body_BDY_OutOfDate( Prj, OutOfDate, Printf )==GE_FALSE)
		return GE_FALSE;
	if (*OutOfDate != GE_FALSE)
		return GE_TRUE;

	MotionCount = AProject_GetMotionsCount( Prj );
	MotionIndexArray = GE_RAM_ALLOCATE_ARRAY( int, MotionCount);
	if (MotionIndexArray == NULL)
		{
			Printf("Error: unable to get memory for Make Actor step\n");
			return GE_FALSE;
		}
	
	for (i=0; i<MotionCount; i++)
		MotionIndexArray[i] = i;
	
	if (Make_AnyMotion_MOT_OutOfDate( Prj, MotionCount, MotionIndexArray, OutOfDate, Printf )==GE_FALSE)
		{
			geRam_Free(MotionIndexArray);
			return GE_FALSE;
		}

	geRam_Free(MotionIndexArray);

	return GE_TRUE;
}


geBoolean Make_Motion(AProject *Prj, AOptions *Options, 
					  int MotionCount, int *MotionIndexArray, 
					  MkUtil_Printf Printf)
{
	int i;
	geBoolean OutOfDate;

	assert( Prj       != NULL );
	assert( Options   != NULL );
	assert( Printf    != NULL );
	assert( MotionIndexArray != NULL );

	Printf("\tMaking Motions\n");			
	if (Make_MaxScript( Prj, Options, GE_TRUE, 
					MotionCount, MotionIndexArray, Printf ) == GE_FALSE)
		{
			Printf("Error: unable to complete 3DS MAX script and export step for Make_Actor\n");
			geRam_Free(MotionIndexArray);
			return GE_FALSE;
		}

	if (Make_AnyMotion_MOT_OutOfDate( Prj, MotionCount, MotionIndexArray, &OutOfDate, Printf )==GE_FALSE)
		return GE_FALSE;
		
	if (OutOfDate == GE_FALSE)
		{
			Printf("\tMotions are up to date\n");
			return GE_TRUE;
		}
	for (i=0; i<MotionCount; i++)
		{
			if (Make_Motion_MOT( Prj, Options, i, Printf)==GE_FALSE)
				return GE_FALSE;
		}
	Printf("\tFinished making Motions successfully\n");			
	return GE_TRUE;
}


geBoolean Make_Actor(AProject *Prj, AOptions *Options, MkUtil_Printf Printf)
{
	int MotionCount;
	int *MotionIndexArray;
	int i;
	geBoolean OutOfDate=GE_FALSE;
	const char *ObjDir;
	
	assert( Prj      != NULL );
	assert( Options  != NULL );
	assert( Printf   != NULL );

	{
		const char *TargetName;
		long Handle;
		struct _finddata_t TargetData;
		TargetName  = AProject_GetOutputFilename (Prj);
		if (TargetName == NULL)
			{
				Printf("Error: Can't get output filename\n");
				return GE_FALSE;
			} 
		#pragma todo ("would like to test against date of apj file")
	
		Handle = _findfirst( TargetName, &TargetData );
		if (Handle == -1)
			{
				Printf("Actor doesn't exist:\n",TargetName);
				OutOfDate = GE_TRUE;
			}
		else
			{
				_findclose(Handle);
			}
	}
	

		
	
	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error: Unable to get temporary path\n");
			return GE_FALSE;
		}

	if (_mkdir(ObjDir)==-1)
		{
			FILE *F;
			char TempFileName[BIG];
			sprintf(TempFileName,"%s\\tempdir.tst",ObjDir);
			F=fopen(TempFileName,"a");
			if (F==NULL)
				{
					Printf("Error: Unable to create temporary path '%s'\n",ObjDir);
					return GE_FALSE;
				}
			fclose(F);
		}

	if (OutOfDate==GE_FALSE)
		if (Make_Actor_ACT_OutOfDate( Prj, &OutOfDate, Printf )==GE_FALSE)
			return GE_FALSE;

	if (OutOfDate == GE_FALSE)
		{
			Printf("\tActor is up to date.\n");
			return GE_TRUE;
		}

	// any script stuff first
	MotionCount = AProject_GetMotionsCount( Prj );
	MotionIndexArray = GE_RAM_ALLOCATE_ARRAY( int, MotionCount);
	if (MotionIndexArray == NULL)
		{
			Printf("Error: unable to get memory for Make Actor step\n");
			return GE_FALSE;
		}
	
	for (i=0; i<MotionCount; i++)
		{
			MotionIndexArray[i] = i;
		}
	if (Make_MaxScript( Prj, Options, GE_TRUE, 
						MotionCount, MotionIndexArray, Printf ) == GE_FALSE)
		{
			Printf("Error: unable to complete 3DS MAX script and export step for Make_Actor\n");
			geRam_Free(MotionIndexArray);
			return GE_FALSE;
		}
	
	// body
	if (Make_Body(Prj,Options,Printf)==GE_FALSE)
		{
			Printf("Error: Failed to build body.  Unable to make Actor.\n");
			geRam_Free(MotionIndexArray);
			return GE_FALSE;
		}

	// motions
	if (Make_Motion(Prj, Options, MotionCount, MotionIndexArray, Printf)==GE_FALSE)
		{
			Printf("Error: Failed to build motions.  Unable to make Actor.\n");
			geRam_Free(MotionIndexArray);
			return GE_FALSE;
		}

	geRam_Free(MotionIndexArray);

	Printf("\tCombining components into final Actor...\n");
	{
		char OptionString[BIG];
		char BodyName[BIG];
		char MotionName[BIG];
		const char *SourceName;
		const char *TargetName;
	
		MkActor_Options *Options;
		ReturnCode RVal;


		SourceName = AProject_GetBodyFilename (Prj);
		if (SourceName == NULL)
			{
				Printf("Error: Can't get source name for body\n");
				return GE_FALSE;
			} 
		Make_TargetFileName( BodyName, SourceName, ObjDir, "BDY");

		TargetName  = AProject_GetOutputFilename (Prj);
		if (TargetName == NULL)
			{
				Printf("Error: Can't get output filename\n");
				return GE_FALSE;
			} 
		
		Options = MkActor_OptionsCreate();
		if (Options == NULL)
			{
				Printf("Error: unable to allocate option block for MkActor\n");
				return GE_FALSE;
			}
		sprintf(OptionString,"-A%s",TargetName);
		RVal = MkActor_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
		if (RVal != RETURN_SUCCESS)
			{
				Printf("Error: unable to set options into MkActor:\n");
				Printf(OptionString);
				MkActor_OptionsDestroy(&Options);
				return GE_FALSE;
			}
		
		sprintf(OptionString,"-B%s",BodyName);
		RVal = MkActor_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
		if (RVal != RETURN_SUCCESS)
			{
				Printf("Error: unable to set options into MkActor:\n");
				Printf(OptionString);
				MkActor_OptionsDestroy(&Options);
				return GE_FALSE;
			}
		
		for (i=0; i<MotionCount; i++)
			{

				SourceName = AProject_GetMotionFilename (Prj,i);
				if (SourceName == NULL)
					{
						Printf("Error: Can't get source file for motion #%d '%s'\n",i,MotionName);
						return GE_FALSE;
					} 
			
				Make_TargetFileName( MotionName, SourceName, ObjDir, "MOT");

				sprintf(OptionString,"-M%s",MotionName);
				RVal = MkActor_ParseOptionString(Options,OptionString,MK_FALSE,Printf);
				if (RVal != RETURN_SUCCESS)
					{
						Printf("Error: unable to set motion option into MkActor:\n");
						Printf(OptionString);
						MkActor_OptionsDestroy(&Options);
						return GE_FALSE;
					}
			}
						
		unlink(TargetName);		// don't want backup files

		RVal = MkActor_DoMake(Options,Printf);
		MkActor_OptionsDestroy(&Options);

		if (RVal == RETURN_ERROR )
			{
				Printf("Error: Final actor compilation failed\n");
				return GE_FALSE;
			}
		if (RVal == RETURN_WARNING)
			{
				Printf("Warnings issued during final actor compilation\n");
			}
	}

	return GE_TRUE;
	
}




geBoolean Make_Clean(AProject *Prj, AOptions *Options, MkUtil_Printf Printf)
{
	const char *TargetName;
	const char *ObjDir;
	char DeleteName[BIG];
	long Handle;
	struct _finddata_t TargetData;
	
Options;		// remove unused parameter warning
	assert( Prj      != NULL );
	assert( Options  != NULL );
	assert( Printf   != NULL );

	TargetName  = AProject_GetOutputFilename (Prj);
	if (TargetName == NULL)
		{
			Printf("Error: Can't get output filename\n");
			return GE_FALSE;
		} 
	
	Handle = _findfirst( TargetName, &TargetData );
	if (Handle != -1)
		{
			_findclose(Handle);
			if (unlink(TargetName) != 0)
				{
					Printf("Error: Can't delete '%s'\n",TargetName);
					return GE_FALSE;
				}
			Printf("Target Actor file deleted\n");
		}
	else
		{
			Printf("Target Actor file not present\n");
		}

	ObjDir = AProject_GetObjPath( Prj );
	if (ObjDir == NULL)
		{
			Printf("Error: Unable to get temporary path\n");
			return GE_FALSE;
		}
	Make_TargetFileName( DeleteName, "*", ObjDir, "*");
	
	
	Handle = _findfirst( DeleteName, &TargetData );
	if (Handle == -1)
		{
		}
	else
		{
			do 
				{
					if (TargetData.name[0]!='.')
						if (FilePath_AppendName(ObjDir,TargetData.name,DeleteName)!=GE_FALSE)
							{
								unlink(DeleteName);
							}
				}
			while (_findnext( Handle, &TargetData) == 0);
			_findclose(Handle);
		}
			

	
	Printf("Clean complete.  Temporary files deleted\n");

	return GE_TRUE;
}



geBoolean Make_ActorSummary(AProject *Prj, MkUtil_Printf Printf)
{
	const char *TargetName;
	geBody *B;
	geActor_Def *A;
	int i;
	int j;
	geVFile *VF;
	long Handle;
	struct _finddata_t TargetData;
	
	assert( Prj    != NULL );
	assert( Printf != NULL );

	TargetName  = AProject_GetOutputFilename (Prj);
	if (TargetName == NULL)
		{
			Printf("Error: Can't get target Actor filename\n");
			return GE_FALSE;
		} 

	Handle = _findfirst( TargetName, &TargetData );
	if (Handle == -1)
		{
			Printf("Error: Target Actor '%s' is not built\n",TargetName);
			return GE_FALSE;
		}
	_findclose(Handle);
	
	
	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,TargetName,NULL,GE_VFILE_OPEN_READONLY);
	if (VF==NULL)
		{
			Printf("Error: Could not open target Actor file (%s).\n", TargetName);
			return GE_FALSE;
		}
	
	A = geActor_DefCreateFromFile(VF);
	geVFile_Close(VF);
	if (A==NULL)
		{
			Printf("Error: Failed to load actor from target actor file '%s'.\n", TargetName);
			return GE_FALSE;
		}

	B = geActor_GetBody(A);
	if (B==NULL)
		{
			Printf("Actor has no body\n");
		}
	else
		{
			Printf("Body:\n");
			Printf("\t%d Bones\n",geBody_GetBoneCount(B));
			for (j=0; j<geBody_GetBoneCount(B); j++)
				{
					geXForm3d A;
					int parent;
					const char *Name;
					geBody_GetBone(B,j,&Name,&A,&parent);
					Printf("\t\tBone %d Name='%s'\n",j,Name);
				}
			Printf("\t%d Materials\n",geBody_GetMaterialCount(B));
			for (j=0; j<geBody_GetMaterialCount(B); j++)
				{
					const char *n;
					geBitmap_Info BmpInfo;
					geBitmap *Bmp;
					geFloat r,g,b;
					int ir,ig,ib;
					geBody_GetMaterial(B,j,&n,&Bmp,&r,&g,&b);
					ir=(int)r;
					ig=(int)g;
					ib=(int)b;
					Printf("\t\tMaterial %d Name='%s'  rgb=(%d %d %d)\n",
								j,n,ir,ig,ib);
							
					if (Bmp!=NULL)
						{
							geBitmap_Info SecondaryInfo;

							geBitmap_GetInfo(Bmp,&BmpInfo,&SecondaryInfo);
							Printf("\t\t         Bitmap Info:  Width=%d   Height=%d   Format ID=%d  \n",
								BmpInfo.Width,BmpInfo.Height,(int)BmpInfo.Format);
							Printf("\t\t                       Minimum Mip=%d   Maximum Mip=%d\n",
								BmpInfo.MinimumMip,BmpInfo.MaximumMip);
							if (BmpInfo.HasColorKey)
								{
									Printf("\t\t                       Color Key=%d\n",
										BmpInfo.ColorKey);
								}
							if (BmpInfo.Palette!=NULL)
								{
									Printf("\t\t                       (Palettized)\n");
								}
						}
					
				}
			Printf("\t%d Levels of Detail\n",GE_BODY_NUMBER_OF_LOD);
			for (j=0; j<GE_BODY_NUMBER_OF_LOD; j++)
				{
					int v,n,faces;
					geBody_GetGeometryStats(B,j,&v,&faces,&n);
					Printf("\t\tLOD%d  %d Vertices   %d Normals   %d Faces\n",j,v,n,faces);
				}
		}
	Printf("%d Motions\n",geActor_GetMotionCount(A));
	for (i=0; i<geActor_GetMotionCount(A); i++)
		{
			geMotion *M;
			const char *name;
			M=geActor_GetMotionByIndex(A,i);
			name= geMotion_GetName(M);

			{
				int Match=0;

				if (B!=NULL)
					{
						if (geBody_GetBoneNameChecksum(B) == geMotion_GetNameChecksum(M))
							Match = 1;
						else
							Match = 0;
					}
				if (name != NULL)
					Printf("\tMotion %d Name='%s' (%d Joints)  (%s)\n",i,
						name,geMotion_GetPathCount(M),
						(Match==1)?"Matches Bones":"Doesn't Match Bones" );
				else
					Printf("\tMotion %d (no name) (%d Joints)  (%s)\n",i,
						geMotion_GetPathCount(M),
						(Match==1)?"Matches Bones":"Doesn't Match Bones");
			}
		}
	return GE_TRUE;
}
