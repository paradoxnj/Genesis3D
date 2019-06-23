/****************************************************************************************/
/*  SelBrushList.h                                                                      */
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
#ifndef SELBRUSHLIST_H
#define SELBRUSHLIST_H

#include "brush.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct tag_SelBrushList SelBrushList;
typedef geBoolean (*SelBrushList_Callback)(Brush *pBrush, void *lParam);

SelBrushList *SelBrushList_Create (void);
void SelBrushList_Destroy (SelBrushList **ppList);

geBoolean SelBrushList_Add (SelBrushList *pList, Brush *pBrush);
geBoolean SelBrushList_Find (SelBrushList *pList, const Brush *pBrush);
geBoolean SelBrushList_Remove (SelBrushList *pList, Brush *pBrush);
void SelBrushList_RemoveAll (SelBrushList *pList);
int SelBrushList_GetSize (SelBrushList *pList);
Brush *SelBrushList_GetBrush (SelBrushList *pList, int BrushIndex);

void SelBrushList_Enum (SelBrushList *pList, SelBrushList_Callback Callback, void *lParam);


#ifdef __cplusplus
	}
#endif


#endif
