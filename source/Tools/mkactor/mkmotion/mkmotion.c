/****************************************************************************************/
/*  MKMOTION.C	                                                                        */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "body.h"
#include "motion.h"
#include "RAM.H"
#include "strblock.h"

#include "mkutil.h"
#include "tdbody.h"
#include "maxmath.h"
#include "mkmotion.h"

#define MK_PI 3.141592654f
#define LINE_LENGTH 5000

typedef struct MkMotion_Options
{
	char BodyFile[_MAX_PATH];
	char KeyFile[_MAX_PATH];
	char MotionFile[_MAX_PATH];
	MK_Boolean KeepStationary;
	MK_Boolean Originate;
	MK_Boolean Capitalize;
	float TimeOffset;
	geStrBlock* pMotionRoots;
	char* pMotionName;
	char EventBoneSeparator;
	geVec3d RootEulerAngles;
	geVec3d RootTranslation;
	geVec3d EulerAngles;		// rotation at read time
} MkMotion_Options;

const MkMotion_Options DefaultOptions =
{
	"",
	"",
	"",
	MK_FALSE,
	MK_FALSE,
	MK_FALSE,
	0.0f,
	NULL,
	NULL,
	'\0',						// default to no bone names on events
	{ 0.0f, 0.0f, 0.0f},
	{ 0.0f, 0.0f, 0.0f},
	{ 0.0f, 0.0f, 0.0f},
};

typedef struct
{
	geStrBlock* pEvents;
	char name[LINE_LENGTH];
	geXForm3d* pMatrixKeys;	// keys with respect to parent
	geXForm3d* pWSKeys;		// keys in world space
} BoneKeyInfo;

void DestroyBoneKeyArray(BoneKeyInfo** ppInfo, int nNumBones)
{
	int i;
	BoneKeyInfo* pInfo;

	assert(ppInfo != NULL);
	pInfo = *ppInfo;
	assert(pInfo != NULL);
	assert(nNumBones > 0);

	for(i=0;i<nNumBones;i++)
	{
		if(pInfo[i].pEvents != NULL)
			geStrBlock_Destroy(&pInfo[i].pEvents);
		if(pInfo[i].pMatrixKeys != NULL)
			geRam_Free(pInfo[i].pMatrixKeys);
		if(pInfo[i].pWSKeys != NULL)
			geRam_Free(pInfo[i].pWSKeys);
	}

	geRam_Free(*ppInfo);
}

BoneKeyInfo* CreateBoneKeyArray(int nNumBones, int nNumKeys)
{
	BoneKeyInfo* pInfo;
	int i;

	assert(nNumBones > 0);
	assert(nNumKeys > 0);

	pInfo = GE_RAM_ALLOCATE_ARRAY(BoneKeyInfo, nNumBones);
	if(pInfo == NULL)
		return(NULL);

	// explicitly null the pointers
	for(i=0;i<nNumBones;i++)
	{
		pInfo[i].pEvents = NULL;
		pInfo[i].pMatrixKeys = NULL;
		pInfo[i].pWSKeys = NULL;
	}

	for(i=0;i<nNumBones;i++)
	{
		pInfo[i].pEvents = geStrBlock_Create();
		if(pInfo[i].pEvents == NULL)
			goto CreateBoneKeyArray_Failure;

		pInfo[i].pMatrixKeys = GE_RAM_ALLOCATE_ARRAY(geXForm3d, nNumKeys);
		if(pInfo[i].pMatrixKeys == NULL)
			goto CreateBoneKeyArray_Failure;

		pInfo[i].pWSKeys = GE_RAM_ALLOCATE_ARRAY(geXForm3d, nNumKeys);
		if(pInfo[i].pWSKeys == NULL)
			goto CreateBoneKeyArray_Failure;
	}

	return(pInfo);

CreateBoneKeyArray_Failure:

	DestroyBoneKeyArray(&pInfo, nNumBones);
	return(NULL);
}

void StripNewLine(char* pString)
{
	assert(pString != NULL);

	while(*pString != 0)
	{
		if(*pString == '\n')
			*pString = 0;
		pString++;
	}
}

// Are two xforms the same?
geBoolean geXForm3d_Compare(const geXForm3d* pM1, const geXForm3d* pM2)
{
	geVec3d v1, v2;

#define XFORM_COMPARE_TOLERANCE 0.001f

	geXForm3d_GetLeft(pM1, &v1);
	geXForm3d_GetLeft(pM2, &v2);

	if(geVec3d_Compare(&v1, &v2, XFORM_COMPARE_TOLERANCE) == GE_FALSE)
		return(GE_FALSE);

	geXForm3d_GetUp(pM1, &v1);
	geXForm3d_GetUp(pM2, &v2);

	if(geVec3d_Compare(&v1, &v2, XFORM_COMPARE_TOLERANCE) == GE_FALSE)
		return(GE_FALSE);

	geXForm3d_GetIn(pM1, &v1);
	geXForm3d_GetIn(pM2, &v2);

	if(geVec3d_Compare(&v1, &v2, XFORM_COMPARE_TOLERANCE) == GE_FALSE)
		return(GE_FALSE);

	if(geVec3d_Compare(&pM1->Translation, &pM2->Translation, XFORM_COMPARE_TOLERANCE) == GE_FALSE)
		return(GE_FALSE);

	return(GE_TRUE);
}

// TKArray cannot handle two keys with the same time, so keep incrementing the
// time by a little bit until an empty hole is found or reach the LimitDistance
geBoolean KEYMotion_InsertEventNoDuplicateTime(geMotion* pMotion, geFloat tKey, const char* String, geFloat LimitDistance)
{
	geFloat TimeOffset;
	geFloat KeyTime;
	geFloat InsertKeyTime;
	const char* pEventString;

#define GE_TKA_TIME_TOLERANCE (0.00001f)
#define TIME_STEP_DISTANCE (GE_TKA_TIME_TOLERANCE * 10.0f)

	TimeOffset = 0.0f;
	while(TimeOffset < LimitDistance)
	{
		InsertKeyTime = tKey + TimeOffset;
		geMotion_SetupEventIterator(pMotion, InsertKeyTime, InsertKeyTime + TIME_STEP_DISTANCE);
		if(geMotion_GetNextEvent(pMotion, &KeyTime, &pEventString) == GE_FALSE)
		{
#pragma todo("geMotion_InsertEvent should take a const const*")
			return(geMotion_InsertEvent(pMotion, InsertKeyTime, (char*)String));
		}

		TimeOffset += TIME_STEP_DISTANCE;
	}

	return(GE_FALSE);
}

ReturnCode MkMotion_DoMake(MkMotion_Options* options,MkUtil_Printf Printf)
{
	int i;
	char* pdot;
	geBody* pBody;
	geMotion* pMotion;
	gePath* pPath;
	geVFile *VF=NULL;
	FILE* fp=NULL;
	ReturnCode retValue = RETURN_SUCCESS;
	int nVersion = 0;
	int j, k, Index, ParentIndex;
	int NumBones=0;
	int NumFrames, FramesPerSecond;
	float SecondsPerFrame;
	char line[LINE_LENGTH];
	char name[LINE_LENGTH];
	geXForm3d InvAttach, KeyMatrix, TmpMatrix;
	geQuaternion Q;
	TopDownBody* pTDBody = NULL;
	int MotionRootIndex = GE_BODY_NO_PARENT_BONE;
	TDBodyHeritage BoneIsDescendent = TDBODY_IS_DESCENDENT; // default to all in the family
	geXForm3d RootRotation;
	geXForm3d euler;
	BoneKeyInfo* pKeyInfo = NULL;
	const BoneKeyInfo* pParentInfo;

	assert(options != NULL);

	if(options->BodyFile[0] == 0)
	{
		Printf("ERROR: No body file specified\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		return retValue;
	}

	if(options->KeyFile[0] == 0)
	{
		Printf("ERROR: No key file specified\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		return retValue;
	}

	// who knows how many times this will get used in here
	geXForm3d_SetEulerAngles(&euler, &options->EulerAngles);

	// Here is the official fclose to use
#define FCLOSE(f) { fclose(f); f = NULL; }

	// Read the body file
	
	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->BodyFile,NULL,GE_VFILE_OPEN_READONLY);
		
	if(VF == NULL)
	{
		Printf("ERROR: Could not open '%s' body file\n", options->BodyFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		return retValue;
	}
	else
	{
		pBody = geBody_CreateFromFile(VF);
		if(pBody == NULL)
		{
			Printf("ERROR: Could not create body from file '%s'\n", options->BodyFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			geVFile_Close(VF);
			VF = NULL;
			return retValue;
		}
		geVFile_Close(VF);
		VF = NULL;
	}

	// Create the Top Down Hierarchy if needed
	if(geStrBlock_GetCount(options->pMotionRoots) > 0)
	{
		pTDBody = TopDownBody_CreateFromBody(pBody);
		if(pTDBody == NULL)
		{
			Printf("ERROR: Could not create top down hierarchy from body.\n");
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			geBody_Destroy(&pBody);
			return retValue;
		}
	}

	// Options permit a rotation applied to the root.  Convert the Euler angles
	// to a transform for easier use later.
	geXForm3d_SetEulerAngles(&RootRotation, &options->RootEulerAngles);

	pMotion = geMotion_Create(GE_TRUE);
	if(pMotion == NULL)
	{
		Printf("ERROR: Could not create motion\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto DoMake_Cleanup;
	}

	if(options->pMotionName != NULL)
		geMotion_SetName(pMotion, options->pMotionName);

	// Read key data
	fp = fopen(options->KeyFile, "rt");
	if(fp == NULL)
	{
		Printf("ERROR: Could not open '%s' key file\n", options->KeyFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		geMotion_Destroy(&pMotion);
		goto DoMake_Cleanup;
	}

#define FGETS_LINE_OR_QUIT(s)												\
	if(fgets(s, LINE_LENGTH, fp) == NULL)										\
	{																			\
		Printf("ERROR: Could not read from '%s' key file\n", options->KeyFile);	\
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);								\
		goto DoMake_Cleanup;													\
	}

	// Read and check version
	FGETS_LINE_OR_QUIT(line);
#define FILETYPE_LENGTH 7
	if(strncmp(line, "KEYEXP ", FILETYPE_LENGTH) != 0)
	{
		Printf("ERROR: '%s' is not a KEY file\n", options->KeyFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto DoMake_Cleanup;
	}

	// check version numbers
	StripNewLine(line);
	if(strcmp(line + FILETYPE_LENGTH, "1.0") == 0)
	{
		nVersion = 0x0100;
	}
	else if(strcmp(line + FILETYPE_LENGTH, "2.0") == 0)
	{
		nVersion = 0x0200;
	}
	else if(strcmp(line + FILETYPE_LENGTH, "2.1") == 0)
	{
		nVersion = 0x0210; // added notetracks
	}
	else
	{
		Printf("ERROR: '%s' KEY file version \"%s\" is not supported\n", options->KeyFile, line + FILETYPE_LENGTH);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto DoMake_Cleanup;
	}
	assert(nVersion != 0);

	// Read number of bones
	FGETS_LINE_OR_QUIT(line);
	sscanf(line, "Number of Bones = %d", &NumBones);

	// Read "Key Data"
	FGETS_LINE_OR_QUIT(line);
	if(strcmp(line, "Key Data\n") != 0)
	{
		Printf("ERROR: '%s' key file does not contain \"Key Data\"\n", options->KeyFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto DoMake_Cleanup;
	}

	// Read number of frames
	FGETS_LINE_OR_QUIT(line);
	sscanf(line, "%d %d %d", &j, &NumFrames, &FramesPerSecond);
	NumFrames -= j;
	NumFrames++;

	SecondsPerFrame = 1.0f / (float)FramesPerSecond;

	// Allocate key data
	pKeyInfo = CreateBoneKeyArray(NumBones, NumFrames);
	if(pKeyInfo == NULL)
	{
		Printf("ERROR: Could not create key data\n");
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		goto DoMake_Cleanup;
	}

	// Read in key data
	for(k=0;k<NumBones;k++)
	{

		if (MkUtil_Interrupt())
			{
				Printf("Interrupted\n");
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
		

		// Read bone name
		FGETS_LINE_OR_QUIT(line);
		strcpy(pKeyInfo[k].name, line + strlen("Node: "));
		
		StripNewLine(pKeyInfo[k].name);

		if(options->Capitalize != MK_FALSE)
			strupr(pKeyInfo[k].name);

		// Notes appear here
		if(nVersion >= 0x0210)
		{
			int NumNotes;

			// Read number of notes
			FGETS_LINE_OR_QUIT(line);
			sscanf(line, "Number of Notes = %d", &NumNotes);

			for(i=0;i<NumNotes;i++)
			{
				FGETS_LINE_OR_QUIT(line);

				// Strip the \n from the line
				StripNewLine(line);

				if(geStrBlock_Append(&pKeyInfo[k].pEvents, line) == GE_FALSE)
				{
					Printf("ERROR: Could not create key data\n");
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto DoMake_Cleanup;
				}
			}
		}

		// Read the key data
		for(j=0;j<NumFrames;j++)
		{
			// Read raw key matrix
			FGETS_LINE_OR_QUIT(line);
			sscanf(line, "%f, %f, %f", &pKeyInfo[k].pMatrixKeys[j].AX, &pKeyInfo[k].pMatrixKeys[j].AY, &pKeyInfo[k].pMatrixKeys[j].AZ);
			FGETS_LINE_OR_QUIT(line);
			sscanf(line, "%f, %f, %f", &pKeyInfo[k].pMatrixKeys[j].BX, &pKeyInfo[k].pMatrixKeys[j].BY, &pKeyInfo[k].pMatrixKeys[j].BZ);
			FGETS_LINE_OR_QUIT(line);
			sscanf(line, "%f, %f, %f", &pKeyInfo[k].pMatrixKeys[j].CX, &pKeyInfo[k].pMatrixKeys[j].CY, &pKeyInfo[k].pMatrixKeys[j].CZ);
			FGETS_LINE_OR_QUIT(line);
			sscanf(line, "%f, %f, %f", &pKeyInfo[k].pMatrixKeys[j].Translation.X, &pKeyInfo[k].pMatrixKeys[j].Translation.Y, &pKeyInfo[k].pMatrixKeys[j].Translation.Z);
		}
	}

	FCLOSE(fp);

	// Now, go thru all the keys and generate the world space keys
	// We make the (safe) assumption that the parent bone occurs earlier in the
	// list and its WS keys are already assigned.
	for(k=0;k<NumBones;k++)
	{
		if(geBody_GetBoneByName(pBody, pKeyInfo[k].name, &Index, &InvAttach, &ParentIndex) == GE_FALSE)
		{
			int m;
			Printf("ERROR: Could not find bone '%s' in body '%s' for key file '%s'\n", pKeyInfo[k].name, options->BodyFile, options->KeyFile);
			Printf("Body has these bones:\n");
			for (m=0; m<NumBones; m++)
				{
					const char *Name;
					geBody_GetBone(pBody,m,&Name,&InvAttach,&ParentIndex);
					Printf("\t#%d\t\t'%s'\n",name);
				}
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			goto DoMake_Cleanup;
		}

		if(ParentIndex != GE_BODY_NO_PARENT_BONE)
		{
			geBody_GetBone(pBody, ParentIndex, &pdot, &InvAttach, &Index);
			pParentInfo = NULL;
			for(j=0;j<k;j++)
			{
				if(strcmp(pdot, pKeyInfo[j].name) == 0)
				{
					pParentInfo = pKeyInfo + j;
					break;
				}
			}
			if(j == k)
			{
				// didn't find it?!
				Printf("ERROR: Could not find bone '%s' in key array\n", pdot);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
			assert(pParentInfo != NULL);

			for(j=0;j<NumFrames;j++)
			{
#ifdef KEYS_ARE_WORLD_SPACE
				pKeyInfo[k].pWSKeys[j] = pKeyInfo[k].pMatrixKeys[j];
#else
				MaxMath_Multiply(	pKeyInfo[k].pMatrixKeys + j, 
									pParentInfo->pWSKeys + j, 
									pKeyInfo[k].pWSKeys + j );
#endif
			}
		}
		else
		{
			for(j=0;j<NumFrames;j++)
			{
				pKeyInfo[k].pWSKeys[j] = pKeyInfo[k].pMatrixKeys[j];
			}
		}

	}

	// Rotate the world space matrix according to the options and recalculate the MatrixKeys.
	for(k=0;k<NumBones;k++)
	{
		if (MkUtil_Interrupt())
			{
				Printf("Interrupted\n");
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
		
		if(geBody_GetBoneByName(pBody, pKeyInfo[k].name, &Index, &InvAttach, &ParentIndex) == GE_FALSE)
		{
			Printf("ERROR: Could not find bone '%s' in body '%s' for key file '%s'\n", name, options->BodyFile, options->KeyFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			goto DoMake_Cleanup;
		}

		// apply rotation to world space
		for(j=0;j<NumFrames;j++)
		{
			// this first one should be the correct one
			geXForm3d_Multiply(&euler, pKeyInfo[k].pWSKeys + j, pKeyInfo[k].pWSKeys + j);
		}

		if(ParentIndex != GE_BODY_NO_PARENT_BONE)
		{
			geBody_GetBone(pBody, ParentIndex, &pdot, &InvAttach, &Index);
			pParentInfo = NULL;
			for(j=0;j<k;j++)
			{
				if(strcmp(pdot, pKeyInfo[j].name) == 0)
				{
					pParentInfo = pKeyInfo + j;
					break;
				}
			}
			if(j == k)
			{
				// didn't find it?!
				Printf("ERROR: Could not find bone '%s' in key array\n", pdot);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
			assert(pParentInfo != NULL);

			for(j=0;j<NumFrames;j++)
			{
				// recalc pMatrixKeys
				MaxMath_InverseMultiply(	pKeyInfo[k].pWSKeys + j, 
											pParentInfo->pWSKeys + j,
											pKeyInfo[k].pMatrixKeys + j );
			}
		}
		else
		{
			for(j=0;j<NumFrames;j++)
			{
				pKeyInfo[k].pMatrixKeys[j] = pKeyInfo[k].pWSKeys[j];
			}
		}

	}

	// Finally, let's generate the paths and add them to the motion
	for(k=0;k<NumBones;k++)
	{
		geVec3d FirstFrameOffset;
		if (MkUtil_Interrupt())
			{
				Printf("Interrupted\n");
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
		
		// Read bone name and get the inverse of its attachment
		strcpy(name, pKeyInfo[k].name);

		if(geBody_GetBoneByName(pBody, name, &Index, &InvAttach, &ParentIndex) == GE_FALSE)
		{
			Printf("ERROR: Could not find bone '%s' in body '%s' for key file '%s'\n", name, options->BodyFile, options->KeyFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			goto DoMake_Cleanup;
		}

		// If we have specified a root for the motion, make sure this bone
		// is a descendent.
		j = geStrBlock_GetCount(options->pMotionRoots);
		if(j > 0)
		{
			assert(pTDBody != NULL);

			while(j > 0)
			{
				j--;

				if(geBody_GetBoneByName(pBody, geStrBlock_GetString(options->pMotionRoots, j), &MotionRootIndex, &TmpMatrix, &ParentIndex) == GE_FALSE)
				{
					Printf("ERROR: Could not find bone '%s' in body '%s' for motion root\n", geStrBlock_GetString(options->pMotionRoots, j), options->BodyFile);
					
					{
						int m;
						Printf("Body has these bones:\n");
						for (m=0; m<NumBones; m++)
							{
								const char *Name;
								geBody_GetBone(pBody,m,&Name,&InvAttach,&ParentIndex);
								Printf("\t#%d\t\t'%s'\n",m,Name);
							}
					}

					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto DoMake_Cleanup;
				}
				BoneIsDescendent = TopDownBody_IsDescendentOf(pTDBody, MotionRootIndex, Index);
				if(BoneIsDescendent == TDBODY_IS_DESCENDENT)
					break; // found it!
			}
		}
		else
		{
			// All in the family.
			BoneIsDescendent = TDBODY_IS_DESCENDENT;
		}

		{
			int NumNotes, note, noteframe;
			unsigned int linelen;
			geFloat notetime;
			const char* notestring;

			NumNotes = geStrBlock_GetCount(pKeyInfo[k].pEvents);

			for(note=0;note<NumNotes;note++)
			{
				// note is valid, so string should be
				strcpy(line, geStrBlock_GetString(pKeyInfo[k].pEvents, note));
				noteframe = atoi(line);
				notestring = strchr(line, ':');
				// the color must be there with a space following it
				if( (notestring == NULL) || (notestring[1] != ' ') )
				{
					Printf("ERROR: Bad note key \"%s\" for bone '%s' for key file '%s'\n", line, name, options->KeyFile);
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto DoMake_Cleanup;
				}
				notestring += 2; // this jump past the space to, at the very least, a terminator

				if(options->EventBoneSeparator != '\0')
				{
					// base string length decisions on "line", not "notestring"
					linelen = strlen(line);
					linelen++; // add one for separator character

					if( (LINE_LENGTH - linelen) < (strlen(name)))
					{
						// some string too long
						Printf("ERROR: Note \"%s\" is too long for bone '%s' for key file '%s'\n", notestring, name, options->KeyFile);
						MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
						goto DoMake_Cleanup;
					}
					line[linelen - 1] = options->EventBoneSeparator;
					line[linelen] = '\0';
					strcat(line, name);
				}

				notetime = (float)noteframe * SecondsPerFrame + options->TimeOffset;
				// insert the note, but not past this frame
				if(KEYMotion_InsertEventNoDuplicateTime(pMotion, notetime, notestring, SecondsPerFrame) == GE_FALSE)
				{
					Printf("ERROR: Could not add note \"%s\" for bone '%s' for key file '%s'\n", notestring, name, options->KeyFile);
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto DoMake_Cleanup;
				}
			}
		}

		pPath = gePath_Create(GE_PATH_INTERPOLATE_HERMITE, GE_PATH_INTERPOLATE_SLERP, GE_FALSE);
		if (pPath == NULL)
			{
				Printf("ERROR: Unable to create a path for bone '%s' for key file '%s'\n",name, options->KeyFile);
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
		if(BoneIsDescendent != TDBODY_IS_NOT_DESCENDENT)
		{
			
			if(geMotion_AddPath(pMotion, pPath, name,  &Index) == GE_FALSE)
			{
				if (geMotion_GetPathNamed(pMotion,name) != NULL)
					{
						Printf("ERROR: More than one bone named '%s' in key file '%s'\n", name, options->KeyFile);
					}
				else
					{
						Printf("ERROR: Could not add path for bone '%s' for key file '%s'\n", name, options->KeyFile);
					}
				MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
				goto DoMake_Cleanup;
			}
			//pPath = geMotion_GetPath(pMotion, Index);
		}

		// Read the key data, divide out the attachment (multiply by inverse),
		// and insert the keyframe
		for(j=0;j<NumFrames;j++)
		{
			KeyMatrix = pKeyInfo[k].pMatrixKeys[j];
			if (MkUtil_Interrupt())
				{
					Printf("Interrupted\n");
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					goto DoMake_Cleanup;
				}
		
			// rotation for root only from options
			if(ParentIndex == GE_BODY_NO_PARENT_BONE)
			{
				geXForm3d_Multiply(&RootRotation, &KeyMatrix, &KeyMatrix);
			}
			
			// permit global move of motion
			if(ParentIndex == GE_BODY_NO_PARENT_BONE)
				geXForm3d_Translate(&KeyMatrix, options->RootTranslation.X, options->RootTranslation.Y, options->RootTranslation.Z);

			// flip the rotation direction
			TmpMatrix.Translation = KeyMatrix.Translation;
			geQuaternion_FromMatrix(&KeyMatrix, &Q);
			Q.W = -Q.W;
			geQuaternion_ToMatrix(&Q, &KeyMatrix);
			KeyMatrix.Translation = TmpMatrix.Translation;

			geXForm3d_GetTranspose(&InvAttach, &TmpMatrix);
			geXForm3d_Multiply(&TmpMatrix, &KeyMatrix, &TmpMatrix);
			Q;
			FirstFrameOffset;

			geXForm3d_Orthonormalize(&TmpMatrix);
			gePath_InsertKeyframe(	pPath,
									GE_PATH_ROTATION_CHANNEL | GE_PATH_TRANSLATION_CHANNEL,
									(float)j / (float)FramesPerSecond + options->TimeOffset,
									&TmpMatrix);
		}

		// First, destroy path
		//if(BoneIsDescendent == MK_FALSE)
		{
			gePath_Destroy(&pPath);
		}

		// Then, check for errors
		if(j != NumFrames)
		{
			// There must have been an error
			goto DoMake_Cleanup;
		}
	}
	if(k != NumBones)
	{
		// There must have been an error
		goto DoMake_Cleanup;
	}

	if (options->MotionFile[0] == 0)
		{
			// build motion filename: keyfile.key -> keyfile.mot
			strcpy(options->MotionFile,options->KeyFile);
			pdot = strrchr(options->MotionFile, '.');
			if(pdot == NULL)
			{
				pdot = options->MotionFile + strlen(options->MotionFile);
			}
			strcpy(pdot, ".mot");
		}

	// Write the motion
	VF = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,options->MotionFile,NULL,GE_VFILE_OPEN_CREATE);
	//fp = fopen(options->MotionFile, "wt");
	if(VF == NULL)
	{
		Printf("ERROR: Could not create '%s' motion file\n", options->MotionFile);
		MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
		unlink(options->MotionFile);
		goto DoMake_Cleanup;
	}
	else
	{
		if(geMotion_WriteToFile(pMotion, VF) == GE_FALSE)
		{
			geVFile_Close(VF);  VF = NULL;
			Printf("ERROR: Motion file '%s' was not written correctly\n", options->MotionFile);
			MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
			unlink(options->MotionFile);
		}
		else
		{
			if (geVFile_Close(VF)==GE_FALSE) 
				{
					Printf("ERROR: Motion file '%s' was not written correctly\n", options->MotionFile);
					MkUtil_AdjustReturnCode(&retValue, RETURN_ERROR);
					unlink(options->MotionFile);
				}
			VF = NULL;
			Printf("SUCCESS: Motion file '%s' written successfully\n", options->MotionFile);
		}
	}

DoMake_Cleanup:

	if (MkUtil_Interrupt())
		{
			Printf("Interrupted... Unable to make motion '%s'\n",options->MotionFile);
		}
	else
		if (retValue==RETURN_ERROR)
			{
				Printf("Error.  Unable to make motion '%s'\n",options->MotionFile);
			}
	if(pKeyInfo != NULL)
		DestroyBoneKeyArray(&pKeyInfo, NumBones);

	if(fp != NULL)
		FCLOSE(fp);

	if(VF != NULL)
		geVFile_Close(VF);

	if(pTDBody != NULL)
		TopDownBody_Destroy(&pTDBody);

	if(pMotion != NULL)
		geMotion_Destroy(&pMotion);

	if(pBody != NULL)
		geBody_Destroy(&pBody);

	return retValue;
}

void MkMotion_OutputUsage(MkUtil_Printf Printf)
{
	//COLS: 0         1         2         3         4         5         6         7       | 8
	Printf("\n");
	Printf("Builds a motion file from a key info file from 3DSMax.\n");
	Printf("\n");
	Printf("MKMOTION [options] /B<bodyfile> /K<keyfile> [/C] [/E[<sepchar>]] [/N<name>] [/O]\n");
	Printf("         [/R<rootbone>] [/S] [/T<time>] [/M<motionfile>]\n");
	Printf("\n");
	Printf("/B<bodyfile>  Specifies body file.  A body is required.\n");
	Printf("/C            Capitalize all motion key names.\n");
	Printf("/E[<sepchar>] Attach bone name to event strings, optionally specifying the\n");
	Printf("              separation character (default is ';').\n");
	Printf("/K<keyfile>   Specifies key file for motion.  A key file is required.\n");
	Printf("/N<name>      Specifies the name for the motion.\n");
	Printf("/Px,y,z       Specifies a positional offset for the root bone.\n");
	Printf("/O            Forces translation keys to be relative to first frame.\n");
	Printf("/R<rootbone>  Specifies a root bone for the motion.  This option can appear\n");
	Printf("              more than once to specify multiple roots.\n");
	Printf("/S            Removes all translation keys.\n");
	Printf("/T<time>      Specifies a time offset to apply to motions.\n");
	Printf("/M<motionfile>  Specifies output motion file name.  if not specified\n");
	Printf("                output will be in <keyfile prefix>.mot\n");
	Printf("\n");
	Printf("Any existing motion file will be renamed to motionfile.bak\n");
}

MkMotion_Options* MkMotion_OptionsCreate()
{
	MkMotion_Options* pOptions;

	pOptions = GE_RAM_ALLOCATE_STRUCT(MkMotion_Options);
	if(pOptions != NULL)
	{
		*pOptions = DefaultOptions;

		pOptions->pMotionRoots = geStrBlock_Create();
		if(pOptions->pMotionRoots == NULL)
		{
			geRam_Free(pOptions);
		}
	}

	return pOptions;
}

void MkMotion_OptionsDestroy(MkMotion_Options** ppOptions)
{
	MkMotion_Options* p;

	assert(ppOptions != NULL);
	assert(*ppOptions != NULL);

	p = *ppOptions;

	geStrBlock_Destroy(&p->pMotionRoots);

	if(p->pMotionName != NULL)
		geRam_Free(p->pMotionName);

	geRam_Free(*ppOptions);

	*ppOptions = NULL;
}

ReturnCode MkMotion_ParseOptionString(MkMotion_Options* options, 
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
		case 'b':
		case 'B':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->BodyFile[0] != 0) )
				{
					Printf("WARNING: Body filename in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->BodyFile, string + 2);
				}
			}
			break;

		case 'c':
		case 'C':
			options->Capitalize = MK_TRUE;
			break;

		case 'e':
		case 'E':
			if(string[2] == 0)
				options->EventBoneSeparator = ';';
			else
				options->EventBoneSeparator = string[2];
			break;

		case 'k':
		case 'K':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->KeyFile[0] != 0) )
				{
					Printf("WARNING: Key filename in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->KeyFile, string + 2);
				}
			}
			break;
		case 'm':
		case 'M':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if( (InScript != MK_FALSE) && (options->MotionFile[0] != 0) )
				{
					Printf("WARNING: Motion filename in script ignored\n");
					retValue = RETURN_WARNING;
				}
				else
				{
					strcpy(options->MotionFile, string + 2);
				}
			}
			break;

		case 'n':
		case 'N':
			if(strlen(string + 2) > 0)
			{
				options->pMotionName = (char*)geRam_Allocate(strlen(string + 2) + 1);
				if(options->pMotionName == NULL)
				{
					Printf("ERROR: Could not allocate motion name\n");
					retValue = RETURN_ERROR;
				}
				else
				{
					strcpy(options->pMotionName, string + 2);
				}
			}
			break;

		case 'o':
		case 'O':
			options->Originate = MK_TRUE;
			break;

		case 'p':
		case 'P':
			{
				char* ptext1;
				char* ptext2;

				options->RootTranslation.X = (float)strtod(string + 2, &ptext1);
				if( (ptext1 == string + 2) || (*ptext1 != ',') )
					goto PositionError;

				ptext1++; // skip ','
				options->RootTranslation.Y = (float)strtod(ptext1, &ptext2);
				if( (ptext2 == ptext1) || (*ptext2 != ',') )
					goto PositionError;

				ptext2++; // skip ','
				options->RootTranslation.Z = (float)strtod(ptext2, &ptext1);
				if(ptext1 == ptext2)
					goto PositionError;

				break;

PositionError:
				retValue = RETURN_ERROR;
				Printf("ERROR: Could not convert position offset\n");
			}
			break;

		case 'r':
		case 'R':
			if(string[2] == 0)
			{
				NO_FILENAME_WARNING;
				retValue = RETURN_WARNING;
			}
			else
			{
				if(geStrBlock_FindString(options->pMotionRoots, string + 2, &Index) == MK_FALSE)
				{
					if(geStrBlock_Append(&options->pMotionRoots, string + 2) == MK_FALSE)
					{
						Printf("ERROR: Could not add \"%s\" root to string block\n", string + 2);
						retValue = RETURN_ERROR;
					}
				}
				else
				{
					Printf("WARNING: Duplicate root \"%s\" ignored\n", string + 2);
					retValue = RETURN_WARNING;
				}
			}
			break;

		case 's':
		case 'S':
			options->KeepStationary = MK_TRUE;
			break;

		case 't':
		case 'T':
			options->TimeOffset = (float)atof(string + 2);
			break;

		default:
			retValue = RETURN_NOACTION;
		}
	}

	return retValue;
}
