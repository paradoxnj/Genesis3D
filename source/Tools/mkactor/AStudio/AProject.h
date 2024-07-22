/****************************************************************************************/
/*  APROJECT.H																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor studio/builder project file API.									*/
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
#ifndef AProject_H
#define AProject_H

#ifdef __cplusplus
	extern "C" {
#endif


#include "GENESIS.H"

typedef struct tag_AProject	AProject;

// Create and destroy
AProject *AProject_Create (const char *OutputName);
void AProject_Destroy (AProject **ppProject);

// file i/o
AProject *AProject_CreateFromFile (geVFile *FS);
AProject *AProject_CreateFromFilename (const char *Filename);

geBoolean AProject_WriteToFile (const AProject *pProject, geVFile *FS);
geBoolean AProject_WriteToFilename (const AProject *pProject, const char *Filename);

// Paths section
geBoolean AProject_GetForceRelativePaths (const AProject *pProject);
geBoolean AProject_SetForceRelativePaths (AProject *pProject, const geBoolean Flag);

const char *AProject_GetMaterialsPath (const AProject *pProject);
geBoolean AProject_SetMaterialsPath (AProject *pProject, const char *Path);

//Need a directory for 'temporary' or 'obj' files:
const char *AProject_GetObjPath (const AProject *pProject);
geBoolean AProject_SetObjPath (AProject *pProject, const char *Path);
		
// Output file section
typedef enum
{
	ApjOutput_Text = 0,
	ApjOutput_Binary = 1
} ApjOutputFormat;

const char *AProject_GetOutputFilename (const AProject *pProject);
geBoolean AProject_SetOutputFilename (AProject *pProject, const char *Filename);

ApjOutputFormat AProject_GetOutputFormat (const AProject *pProject);
geBoolean AProject_SetOutputFormat (AProject *pProject, const ApjOutputFormat Fmt);


// Body section
typedef enum
{
	ApjBody_Invalid = 0,
	ApjBody_Max = 1,
	ApjBody_Nfo = 2,
	ApjBody_Bdy = 3,
	ApjBody_Act = 4
} ApjBodyFormat;

ApjBodyFormat AProject_GetBodyFormatFromFilename (const char *Name);

const char *AProject_GetBodyFilename (const AProject *pProject);
geBoolean AProject_SetBodyFilename (AProject *pProject, const char *Filename);

ApjBodyFormat AProject_GetBodyFormat (const AProject *pProject);
geBoolean AProject_SetBodyFormat (AProject *pProject, ApjBodyFormat Fmt);


// Materials section
typedef enum
{
	ApjMaterial_Color = 0,
	ApjMaterial_Texture = 1
} ApjMaterialFormat;

int AProject_GetMaterialsCount (const AProject *pProject);

geBoolean AProject_AddMaterial
	(
	  AProject *pProject,
	  const char *MaterialName,
	  const ApjMaterialFormat Fmt,
	  const char *TextureFilename,
	  const float Red, const float Green, const float Blue, const float Alpha,
	  int *pIndex		// returned index
	);

geBoolean AProject_RemoveMaterial (AProject *pProject, const int Index);

int AProject_GetMaterialIndex (const AProject *pProject, const char *MaterialName);

ApjMaterialFormat AProject_GetMaterialFormat (const AProject *pProject, const int Index);
geBoolean AProject_SetMaterialFormat (AProject *pProject, const int Index, const ApjMaterialFormat Fmt);

const char *AProject_GetMaterialName (const AProject *pProject, const int Index);
geBoolean AProject_SetMaterialName (AProject *pProject, const int Index, const char *MaterialName);

const char *AProject_GetMaterialTextureFilename (const AProject *pProject, const int Index);
geBoolean AProject_SetMaterialTextureFilename (AProject *pProject, const int Index, const char *TextureFilename);

GE_RGBA AProject_GetMaterialTextureColor (const AProject *pProject, const int Index);
geBoolean AProject_SetMaterialTextureColor (AProject *pProject, const int Index, 
	const float Red, const float Green, const float Blue, const float Alpha);


// Motions section
typedef enum
{
	ApjMotion_Invalid = 0,
	ApjMotion_Max = 1,
	ApjMotion_Key = 2,
	ApjMotion_Mot = 3,
// Actor motions not yet supported
//	ApjMotion_Act = 4,
	// enter new types before this line
	ApjMotion_TypeCount
} ApjMotionFormat;

ApjMotionFormat AProject_GetMotionFormatFromFilename (const char *Filename);

int AProject_GetMotionsCount (const AProject *pProject);

geBoolean AProject_AddMotion
	(
	  AProject *pProject,
	  const char *MotionName,
	  const char *Filename,
	  const ApjMotionFormat Fmt,
	  const geBoolean OptFlag,
	  const int OptLevel,
	  const char *BoneName,
	  int *pIndex	// returned index
	);

geBoolean AProject_RemoveMotion (AProject *pProject, const int Index);

int AProject_GetMotionIndex (const AProject *pProject, const char *MotionName);

ApjMotionFormat AProject_GetMotionFormat (const AProject *pProject, const int Index);
geBoolean AProject_SetMotionFormat (AProject *pProject, const int Index, const ApjMotionFormat Fmt);

const char *AProject_GetMotionName (const AProject *pProject, const int Index);
geBoolean AProject_SetMotionName (AProject *pProject, const int Index, const char *MotionName);

const char *AProject_GetMotionFilename (const AProject *pProject, const int Index);
geBoolean AProject_SetMotionFilename (AProject *pProject, const int Index, const char *Filename);

geBoolean AProject_GetMotionOptimizationFlag (const AProject *pProject, const int Index);
geBoolean AProject_SetMotionOptimizationFlag (AProject *pProject, const int Index, const geBoolean Flag);

int AProject_GetMotionOptimizationLevel (const AProject *pProject, const int Index);
geBoolean AProject_SetMotionOptimizationLevel (AProject *pProject, const int Index, const int OptLevel);

const char *AProject_GetMotionBone (const AProject *pProject, const int Index);
geBoolean AProject_SetMotionBone (AProject *pProject, const int Index, const char *BoneName);

#ifdef __cplusplus
	}
#endif


#endif
