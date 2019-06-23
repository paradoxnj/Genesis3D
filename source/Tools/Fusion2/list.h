/****************************************************************************************/
/*  list.h                                                                              */
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
#ifndef LIST_H
#define LIST_H

#include "basetype.h"

#ifdef __cplusplus
	extern "C" {
#endif

#define LIST_INVALID_NODE NULL

typedef struct tag_List List;
typedef struct tag_ListIterator *ListIterator;

typedef void (*List_DestroyCallback)(void *pData);
typedef void (*List_ForEachCallback)(void *pData, void *lParam);
// SearchCallback returns GE_TRUE if the passed item (pData) meets
// the search criteria.  GE_FALSE otherwise.
typedef geBoolean (*List_SearchCallback)(void *pData, void *lParam);

// Create an empty list and return a pointer to it.
// returns NULL if list couldn't be created
List *List_Create (void);

// Destroy a list object.
// Deallocates all memory allocated to the list, and sets *ppList to NULL.
// If DestroyFcn is not NULL, the function is called for each item in the list.
void List_Destroy (List **ppList, List_DestroyCallback DestroyFcn);

// Return pointer to data stored in list node
void *List_GetData (ListIterator pli);

// Append an item to the list (add to the end)
ListIterator List_Append (List *pList, void *pData);

// Prepend (add to the front) an item to the list
ListIterator List_Prepend (List *pList, void *pData);

// Insert an item after an existing item.
// *pli references the node after which the data should be inserted,
// and must have been returned by one of the Get functions,
// or by List_Search.
ListIterator List_InsertAfter (List *pList, ListIterator pli, void *pData);

// Insert an item before an existing item.
// *pli references the node before which the data should be inserted,
// and must have been returned by one of the Get functions,
ListIterator List_InsertBefore (List *pList, ListIterator pli, void *pData);

// Remove the item referenced by *pli.
// If DestroyFcn is not NULL, it will be called with the address of the
// item's data.
geBoolean List_Remove (List *pList, ListIterator pli, List_DestroyCallback DestroyFcn);

// Get a pointer to the data for the first item in the list.
// ListIterator is initialized by this function
void *List_GetFirst (List *pList, ListIterator *pli);

// Retrieve pointer to next item's data.
// ListIterator must have been previously initialized.
void *List_GetNext (List *pList, ListIterator *pli);

// Get a pointer to the data for the last item in the list.
// ListIterator is initialized by this function.
void *List_GetLast (List *pList, ListIterator *pli);

// Retrieve pointer to the previous item's data.
// ListIterator must have been previously initialized.
void *List_GetPrev (List *pList, ListIterator *pli);

// Return number of items in the list
int List_GetNumItems (const List *pList);

// Call the CallbackFcn for each item in the list.
// CallbackFcn will be called with each list item's data, and lParam.
// lParam is a pointer to a user-defined data block.
int List_ForEach (List *pList, List_ForEachCallback CallbackFcn, void *lParam);

// Search for an item in the list.
// For each item in the list, the SearchFcn is called with the item's data,
// and lParam.
// lParam is a pointer to a user-defined data block.
// Returns GE_TRUE if the search is successful.  If GE_TRUE is returned,
// a pointer to the found item's data is returned in *ppData, and
// *pli is initialized to reference the found item.
// Returns GE_FALSE if the search is not successful.
geBoolean List_Search (List *pList, List_SearchCallback SearchFcn, void *lParam, void **ppData, ListIterator *pli);

#ifdef __cplusplus
	}
#endif



#endif
