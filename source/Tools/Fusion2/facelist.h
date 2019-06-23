/****************************************************************************************/
/*  facelist.h                                                                          */
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
#ifndef FACELIST_H
#define FACELIST_H

#include "face.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct tag_FaceList FaceList;

FaceList	*FaceList_Create(int NumFaces);
void		FaceList_Destroy(FaceList **ppList);

// Grows the face list by doubling its size.
geBoolean	FaceList_Grow (FaceList **ppList);

FaceList	*FaceList_Clone(const FaceList *pList);
void		FaceList_RemoveFace(FaceList *pList, int WhichFace);

void		FaceList_SetDirty(FaceList *pList);		//rethink this
geBoolean	FaceList_SetNextSelectedFace(FaceList *fl);
geBoolean	FaceList_SetPrevSelectedFace(FaceList *fl);
void		FaceList_SetTranslucency(const FaceList *fl, geFloat trans);
void		FaceList_SetTransparent (const FaceList *fl, geBoolean trans);

void		FaceList_GetCenter(const FaceList *pList, geVec3d *pCenter);
void		FaceList_GetBounds(const FaceList *pList, Box3d *pBounds);
int			FaceList_GetNumFaces(const FaceList *pList);
int			FaceList_GetFaceLimit (const FaceList *pList);
Face		*FaceList_GetFace(const FaceList *pList, int WhichFace);
Face		*FaceList_GetSelectedFace(const FaceList *fl);

void		FaceList_AddFace(FaceList *pList, Face *pFace);
void		FaceList_Rotate(FaceList *pList, const geXForm3d *pXfm, const geVec3d *pCenter);
void		FaceList_Move(FaceList *pList, const geVec3d *trans);
void		FaceList_Transform(FaceList *pList, const geXForm3d *pXfm);
void		FaceList_Scale(FaceList *pList, const geVec3d *ScaleVec);
void		FaceList_Shear(FaceList *pList, const geVec3d *ScaleVec, const geVec3d *ShearAxis);
void		FaceList_ClipFaceToList(const FaceList *fl, Face **f);
void		FaceList_CopyFaceInfo(const FaceList *src, FaceList *dst);

geBoolean	FaceList_Write(const FaceList *pList, FILE *f);
FaceList	*FaceList_CreateFromFile(Parse3dt *Parser, int VersionMajor, int VersionMinor, const char **Expected);
geBoolean	FaceList_WriteToMap(const FaceList *pList, FILE *f);
geBoolean	FaceList_WriteToQuakeMap(const FaceList *pList, FILE *f);

#ifdef __cplusplus
	}
#endif

#endif
