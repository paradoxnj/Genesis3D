/****************************************************************************************/
/*  FMTACTOR.C                                                                          */
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


#include "genesis.h"
#include "actor.h"
#include "ram.h"
#include "fmtactor.h"

#define MAX_OPTIMIZATION_LEVEL 1

typedef struct FmtActor_Options
{
	MK_Boolean TextOutput;
	char DestinationActorFile[_MAX_PATH];
	char SourceActorFile[_MAX_PATH];
	char SummaryFile[_MAX_PATH];
	int OptimizationLevel;
} FmtActor_Options;

// Use this to initialize the default settings
const static FmtActor_Options DefaultOptions = 
{
	MK_FALSE,
	"",
	"",
	"",
	-1,
};


static void FmtActor_Summary(geActor_Def *A, char *SummaryFile, MkUtil_Printf Printf)
{
	FILE *f;
	geBody *B;
	int i;
	int j;
	
	assert( A != NULL );
	assert( SummaryFile != NULL );

	f=fopen(SummaryFile,"w");
	if (f==NULL)
		{
			Printf("WARNING: unable to open summary file '%s'\n",SummaryFile);
			return;
		}

	B = geActor_GetBody(A);
	if (B==NULL)
		{
			fprintf(f,"Actor has no body\n");
		}
	else
		{
			fprintf(f,"Body:\n");
			fprintf(f,"\t%d Bones\n",geBody_GetBoneCount(B));
			for (j=0; j<geBody_GetBoneCount(B); j++)
				{
					geXForm3d A;
					int parent;
					const char *Name;
					geBody_GetBone(B,j,&Name,&A,&parent);
					fprintf(f,"\t\tBone %d Name='%s'\n",j,Name);
				}
			fprintf(f,"\t%d Materials\n",geBody_GetMaterialCount(B));
			for (j=0; j<geBody_GetMaterialCount(B); j++)
				{
					const char *n;
					const char *a;
					geFloat r,g,b;
					geBody_GetMaterial(B,j,&n,&a,&r,&g,&b);
					fprintf(f,"\t\tMaterial %d Name='%s'  Alpha='%s'  rgb=(%f %f %f)\n",
						j,n,a,r,g,b);
				}
			fprintf(f,"\t%d Levels of Detail\n",GE_BODY_NUMBER_OF_LOD);
			for (j=0; j<GE_BODY_NUMBER_OF_LOD; j++)
				{
					int v,n,faces;
					geBody_GetGeometryStats(B,j,&v,&faces,&n);
					fprintf(f,"\t\tLOD%d  %d Vertices   %d Normals   %d Faces\n",j,v,n,faces);
				}
		}
	fprintf(f,"%d Motions\n",geActor_GetMotionCount(A));
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
					fprintf(f,"\tMotion %d Name='%s' (%d Joints)  (%s)\n",i,
						name,geMotion_GetPathCount(M),
						(Match==1)?"Matches Bones":"Doesn't Match Bones" );
				else
					fprintf(f,"\tMotion %d (no name) (%d Joints)  (%s)\n",i,
						geMotion_GetPathCount(M),
						(Match==1)?"Matches Bones":"Doesn't Match Bones");
			}
		}
	fclose(f);
}



			
	

ReturnCode FmtActor_DoMake(FmtActor_Options* options, MkUtil_Printf Printf)
{
	ReturnCode retValue = RETURN_SUCCESS;
	geVFile *sf;
	geVFile *df;
	geBoolean ok;
	geActor_Def* A = NULL;

	// Actor files must be specified
	if(options->SourceActorFile[0] == 0)
		{
			Printf("ERROR: Must specify a source actor file\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	if(options->DestinationActorFile[0] == 0)
		{
			Printf("ERROR: Must specify a destination actor file\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	if(options->OptimizationLevel<0)
		{
			options->OptimizationLevel = 0;
		}

	sf = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->SourceActorFile,NULL,GE_VFILE_OPEN_READONLY);
	if (sf==NULL)
		{
			Printf("ERROR: Could not open source actor file (%s).\n", options->SourceActorFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			return retValue;
		}
	
	if (options->TextOutput)
		{
			df = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->DestinationActorFile,NULL,GE_VFILE_OPEN_CREATE);
			//		df = fopen(options->DestinationActorFile,"w");
		}
	else
		{
			df = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->DestinationActorFile,NULL,GE_VFILE_OPEN_CREATE);
			//      df = fopen(options->DestinationActorFile,"wb");
		}
	if (df==NULL)
		{
			Printf("ERROR: Could not open destination actor file (%s).\n", options->SourceActorFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			geVFile_Close(sf);
			unlink(options->DestinationActorFile);
			return retValue;
		} 
	
	
	A = geActor_DefCreateFromFile(sf);
	geVFile_Close(sf);
	if (A==NULL)
		{
			Printf("ERROR: Failed to create actor from source actor file (%s).\n", options->SourceActorFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			geVFile_Close(df);
			unlink(options->DestinationActorFile);
			return retValue;
		}

	switch (options->OptimizationLevel)
		{
			case (0): break;
			default:
				break;
		}

	if (options->TextOutput)
		{
			ok = geActor_DefWriteToFile(A,df);
		}
	else
		{
			ok = geActor_DefWriteToBinaryFile(A,df);
		}
	if (ok == GE_TRUE)
		{
			ok = geVFile_Close(df);
		}

	if (options->SummaryFile[0]!=0)
		{
			FmtActor_Summary(A,options->SummaryFile,Printf);
		}

	geActor_DefDestroy(&A);
	if (ok == GE_FALSE)
		{
			Printf("ERROR: Failed to write destination actor file (%s).\n", options->DestinationActorFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			unlink(options->DestinationActorFile);
			return retValue;
		}


	return retValue;
}

void FmtActor_OutputUsage(MkUtil_Printf Printf)
{
	//COLS: 0         1         2         3         4         5         6         7       | 8
	Printf("\n");
	Printf("Reformats an actor library.  Default output format is binary,\n");
	Printf("with no motion optimzation.\n");
	Printf("\n");
//	Printf("FMTACTOR [options] [/Ox] [/T] S/<source actor file> \n");
	Printf("FMTACTOR [options] [/T] /S<source actor file> \n");
	Printf("         /D<destination actor file> /I<actor summary file>\n");
	Printf("\n");
	Printf("/S<actorfile>   Specifies source actor file (Required).\n");
	Printf("/D<actorfile>   Specifies destination actor file (Required).\n");
//	Printf("/Ox             Specifies motion optimization level x.\n");
	Printf("/T              Specifies text destination actor file.\n");
	Printf("/I              Specifies optional actor summary file.\n");
	Printf("\n");
	Printf("Destination actor file will be overwritten.\n");
}

FmtActor_Options* FmtActor_OptionsCreate()
{
	FmtActor_Options* pOptions;

	pOptions = GE_RAM_ALLOCATE_STRUCT(FmtActor_Options);
	if(pOptions != NULL)
		{
			*pOptions = DefaultOptions;
		}

	return pOptions;
}

void FmtActor_OptionsDestroy(FmtActor_Options** ppOptions)
{
	FmtActor_Options* p;

	assert(ppOptions != NULL);
	assert(*ppOptions != NULL);

	p = *ppOptions;

	geRam_Free(*ppOptions);

	*ppOptions = NULL;
}

ReturnCode FmtActor_ParseOptionString(FmtActor_Options* options, const char* string, MK_Boolean InScript, MkUtil_Printf Printf)
{
	ReturnCode retValue = RETURN_SUCCESS;
	int Index;

	assert(options != NULL);
	assert(string != NULL);

#define NO_FILENAME_WARNING Printf("WARNING: %s specified with no filename\n", string)

	if( (string[0] == '-') || (string[0] == '/') )
	{
		switch(string[1])
		{
		case 't':
		case 'T':
			options->TextOutput = MK_TRUE;
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
				if( options->SourceActorFile[0] != 0 )
				{
					Printf("WARNING: Multiple %s Specification ignored\n", string);
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->SourceActorFile, string + 2);
				}
			}
			break;

		case 'i':
		case 'I':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( options->SummaryFile[0] != 0 )
				{
					Printf("WARNING: Multiple %s Specification ignored\n", string);
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->SummaryFile, string + 2);
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
				if( options->DestinationActorFile[0] != 0 )
				{
					Printf("WARNING: Multiple %s Specification ignored\n",string);
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->DestinationActorFile, string + 2);
				}
			}
			break;

		#if 0
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
					Printf("WARNING: Multiple %o Specification ignored.\n",string);
				if (level<0 || level>MAX_OPTIMIZATION_LEVEL)
					Printf("WARNING: Optimization level invalid.\n");
				
				options->OptimizationLevel = level;
			}
			break;
		#endif

		default:
			retValue = RETURN_NOACTION;
		}
	}

	return retValue;

		// unneeded parameters
		InScript;
		Index;
}



