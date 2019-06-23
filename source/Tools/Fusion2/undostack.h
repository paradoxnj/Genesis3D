/****************************************************************************************/
/*  undostack.h                                                                         */
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
#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include "entity.h"
#include "brush.h"
#include "array.h"

typedef struct tag_UndoStack UndoStack;
typedef struct tag_UndoStackEntry UndoStackEntry;

typedef enum
{
	UNDO_MOVE,
	UNDO_ROTATE,
	UNDO_SCALE,
	UNDO_SHEAR,
	UNDO_DELETE,
	UNDO_ADD
} UndoEntryType;


typedef struct
{
	Array			*BrushArray;
	Array			*EntityArray;
	geVec3d			MoveDelta;
} UndoMoveEntry;


UndoStack *UndoStack_Create (void);
void UndoStack_Destroy (UndoStack **ppUndo);

geBoolean UndoStack_IsEmpty (const UndoStack *pUndo);

geBoolean UndoStack_Move (UndoStack *pUndo, const geVec3d *MoveVec, int nBrushes, Brush **pBrushes, CEntityArray *Entities);

UndoStackEntry *UndoStack_Pop (UndoStack *pUndo);


void UndoStackEntry_Destroy (UndoStackEntry **ppEntry);

UndoEntryType UndoStackEntry_GetType (const UndoStackEntry *pEntry);

UndoMoveEntry *UndoStackEntry_GetMoveInfo (UndoStackEntry *pEntry);

#endif
