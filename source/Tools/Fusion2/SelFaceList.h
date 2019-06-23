/****************************************************************************************/
/*  SelFaceList.h                                                                       */
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
#ifndef SELFACELIST_H
#define SELFACELIST_H

#include "face.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct tag_SelFaceList SelFaceList;
typedef geBoolean (*SelFaceList_Callback)(Face *pFace, void *lParam);


SelFaceList *SelFaceList_Create (void);
void SelFaceList_Destroy (SelFaceList **ppList);

geBoolean SelFaceList_Add (SelFaceList *pList, Face *pFace);
geBoolean SelFaceList_Remove (SelFaceList *pList, Face *pFace);
void SelFaceList_RemoveAll (SelFaceList *pList);
int SelFaceList_GetSize (SelFaceList *pList);
Face *SelFaceList_GetFace (SelFaceList *pList, int FaceIndex);

void SelFaceList_Enum (SelFaceList *pList, SelFaceList_Callback Callback, void *lParam);


#ifdef __cplusplus
	}
#endif


#endif
