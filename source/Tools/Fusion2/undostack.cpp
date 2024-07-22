/****************************************************************************************/
/*  undostack.cpp                                                                       */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Editor undo stack                                                     */
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
#include "stdafx.h"

#include "undostack.h"
#include "stack.h"
#include "array.h"
#include "RAM.H"
#include <assert.h>


typedef union
{
	UndoMoveEntry Move;
} UndoEntryUnion;

struct tag_UndoStackEntry
{
	UndoEntryType	EntryType;
	UndoEntryUnion	EntryData;
};


struct tag_UndoStack
{
	Stack *pStack;
};


static UndoStackEntry *UndoStackEntry_Create 
	(
	  UndoEntryType EntryType
	)
{
	UndoStackEntry *pEntry;
	int Size;

	Size = sizeof (UndoStackEntry) - sizeof (UndoEntryUnion);
	switch (EntryType)
	{
		case UNDO_MOVE :
			Size += sizeof (UndoMoveEntry);
			break;

		case UNDO_ROTATE :
		case UNDO_SCALE :
		case UNDO_SHEAR :
		case UNDO_DELETE :
		case UNDO_ADD :
		default :
			assert (0);
			break;
	}

	pEntry = (UndoStackEntry *)geRam_Allocate (Size);
	if (pEntry != NULL)
	{
		pEntry->EntryType = EntryType;
	}
	return pEntry;
}

void UndoStackEntry_Destroy (UndoStackEntry **ppEntry)
{
	UndoStackEntry *pEntry;

	assert (ppEntry != NULL);
	assert (*ppEntry != NULL);

	pEntry = *ppEntry;
	switch (pEntry->EntryType)
	{
		case UNDO_MOVE :
		{
			UndoMoveEntry *pMove;

			pMove = &(pEntry->EntryData.Move);
			if (pMove->BrushArray != NULL)
			{
				Array_Destroy (&(pMove->BrushArray));
			}
			if (pMove->EntityArray != NULL)
			{
				Array_Destroy (&(pMove->EntityArray));
			}
			break;
		}

		case UNDO_ROTATE :
		case UNDO_SCALE :
		case UNDO_SHEAR :
		case UNDO_DELETE :
		case UNDO_ADD :
		default :
			assert (0);  // bad, bad
			break;
	}
	geRam_Free (*ppEntry);
}


static geBoolean UndoStackEntry_SetupBrushArray
	(
	  Array **ppArray,
	  int nBrushes,
	  Brush **pBrushes
	)
{
	Array *pArray;
	int i;

	
	if ((nBrushes == 0) || (pBrushes == NULL))
	{
		pArray = NULL;
	}
	else
	{
		pArray = Array_Create (nBrushes, sizeof (Brush *));
		if (pArray == NULL)
		{
			return GE_FALSE;
		}
	}
	if (pArray != NULL)
	{
		for (i = 0; i < nBrushes; ++i)
		{
			Array_PutAt (pArray, i, &pBrushes[i], sizeof (Brush *));
		}
	}

	*ppArray = pArray;

	return GE_TRUE;
}

static geBoolean UndoStackEntry_SetupEntityArray
	(
	  Array **ppArray,
	  CEntityArray *Entities
	)
{
	Array *pArray;
	int i;
	int nEntities;

	nEntities = 0;
	if (Entities != NULL)
	{
		// count the selected entities
		for (i = 0; i < Entities->GetSize (); ++i)
		{
			if ((*Entities)[i].IsSelected ())
			{
				++nEntities;
			}
		}
	}

	if ((Entities == NULL) || (nEntities == 0))
	{
		pArray = NULL;
	}
	else
	{
		// create the array
		pArray = Array_Create (nEntities, sizeof (CEntity *));
		if (pArray == NULL)
		{
			return GE_FALSE;
		}
	}

	if (pArray != NULL)
	{
		int EntNo = 0;

		// Add pointers to the selected entities to the array
		for (i = 0; i < Entities->GetSize (); ++i)
		{
			CEntity const *pEnt;

			pEnt = &(*Entities)[i];
			if (pEnt->IsSelected ())
			{
				Array_PutAt (pArray, EntNo, &pEnt, sizeof (CEntity *));
				++EntNo;
			}
		}
	}

	*ppArray = pArray;

	return GE_TRUE;
}



UndoStack *UndoStack_Create (void)
{
	UndoStack *pUndo;

	pUndo = (UndoStack *)geRam_Allocate (sizeof (UndoStack));
	if (pUndo != NULL)
	{
		pUndo->pStack = Stack_Create ();
		if (pUndo->pStack == NULL)
		{
			UndoStack_Destroy (&pUndo);
		}
	}
	return pUndo;
}

static void UndoStack_DestroyEntry (void *pItem)
{
	UndoStackEntry *pEntry = (UndoStackEntry *)pItem;

	UndoStackEntry_Destroy (&pEntry);
}

void UndoStack_Destroy (UndoStack **ppUndo)
{
	UndoStack *pUndo;

	assert (ppUndo != NULL);
	assert (*ppUndo != NULL);

	pUndo = *ppUndo;
	if (pUndo->pStack != NULL)
	{
		Stack_Destroy (&pUndo->pStack, UndoStack_DestroyEntry);
	}
	geRam_Free (*ppUndo);
}

geBoolean UndoStack_IsEmpty (const UndoStack *pUndo)
{
	return Stack_IsEmpty (pUndo->pStack);
}

UndoStackEntry *UndoStack_Pop (UndoStack *pUndo)
{
	UndoStackEntry *pEntry;

	assert (!UndoStack_IsEmpty (pUndo));

	pEntry = (UndoStackEntry *)Stack_Pop (pUndo->pStack);
	return pEntry;
}


geBoolean UndoStack_Move 
	(
	  UndoStack *pUndo, 
	  const geVec3d *MoveVec, 
	  int nBrushes, 
	  Brush **pBrushes, 
	  CEntityArray *Entities
	)
{
	UndoStackEntry *pEntry;
	UndoMoveEntry *pMove;

	pEntry = UndoStackEntry_Create (UNDO_MOVE);
	if (pEntry == NULL)
	{
		return GE_FALSE;
	}

	pMove = &(pEntry->EntryData.Move);
	geVec3d_Copy (MoveVec, &(pMove->MoveDelta));

	if ((!UndoStackEntry_SetupBrushArray (&(pMove->BrushArray), nBrushes, pBrushes)) ||
	    (!UndoStackEntry_SetupEntityArray (&(pMove->EntityArray), Entities)))
	{
		UndoStackEntry_Destroy (&pEntry);
		return GE_FALSE;
	}

	return Stack_Push (pUndo->pStack, pEntry);
}

UndoEntryType UndoStackEntry_GetType (const UndoStackEntry *pEntry)
{
	return pEntry->EntryType;
}

UndoMoveEntry *UndoStackEntry_GetMoveInfo (UndoStackEntry *pEntry)
{
	assert (pEntry->EntryType == UNDO_MOVE);

	return &(pEntry->EntryData.Move);
}

