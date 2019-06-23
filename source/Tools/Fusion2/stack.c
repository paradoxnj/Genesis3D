/****************************************************************************************/
/*  Stack.c                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  simple stack                                                          */
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
#include "stack.h"
#include "ram.h"
#include <assert.h>

struct tag_Stack
{
	List *pList;
};

// Create a stack that is initially empty.
// On success, returns a pointer to the stack.
// Returns NULL on error.
Stack *Stack_Create (void)
{
	Stack *pStack;

	pStack = geRam_Allocate (sizeof (Stack));
	if (pStack != NULL)
	{
		pStack->pList = List_Create ();
	}
	return pStack;
}

// Destroy a stack.
void Stack_Destroy (Stack **ppStack, Stack_DestroyCallback DestroyCallback)
{
	assert (ppStack != NULL);
	assert (*ppStack != NULL);

	List_Destroy (&((*ppStack)->pList), DestroyCallback);
	geRam_Free (*ppStack);
}

// Push an item onto the stack.
// Returns GE_TRUE if the item was pushed successfully.
geBoolean Stack_Push (Stack *pStack, void *pData)
{
	ListIterator li;

	assert (pStack != NULL);

	li = List_Prepend (pStack->pList, pData);
	return (li != LIST_INVALID_NODE);
}


// Pop an item from the stack.
// It is an error to try to
// pop when there's nothing on the stack.
void *Stack_Pop (Stack *pStack)
{
	void *pData;
	ListIterator li;

	assert (pStack != NULL);
	assert (!Stack_IsEmpty (pStack));

	pData = List_GetFirst (pStack->pList, &li);
	return pData;
}

// returns GE_TRUE if the stack is empty (has no items)
geBoolean Stack_IsEmpty (const Stack *pStack)
{
	assert (pStack != NULL);

	return !(List_GetNumItems (pStack->pList) > 0);
}


