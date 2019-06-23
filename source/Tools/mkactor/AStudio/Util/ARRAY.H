/****************************************************************************************/
/*  ARRAY.H																				*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Lightweight dynamic array.												*/
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
#ifndef ARRAY_H
#define ARRAY_H

#include "basetype.h"
#include <string.h>

#ifdef __cplusplus
	extern "C" {
#endif


typedef struct tag_Array Array;
struct tag_Array
{
	int ItemsAllocated;		// number of items currently allocated
	int ItemSize;
	void *Items;			// array of data
};


// Initialize an allocated array object
geBoolean Array_Init (Array *pArray, int InitialSize, int ItemSize);

// Free memory allocated by an array's items
void Array_Uninit (Array *pArray);

// Create an array object with given initial size (possibly 0).
// The array contains items of size ItemSize (must be > 0)
Array *Array_Create (int InitialSize, int ItemSize);

// Destroy an array object
void Array_Destroy (Array **ppArray);

// Size the array to contain NewSize items.
// returns new size in items.
int Array_Resize (Array *pArray, int NewSize);

#ifdef _DEBUG
	// Returns the array's current size.
	int Array_GetSize (const Array *pArray);

	// Return size of items held by the array
	int Array_GetItemSize (const Array *pArray);

	// Returns a pointer to the item's data.
	void *Array_ItemPtr (Array *pArray, int Index);

	// Copies DataSize bytes from pData to array[Index]
	void Array_PutAt (Array *pArray, int Index, void *pData, int DataSize);

	// Inserts an item at the given position, moving all other items
	// down by 1.  The last item in the array is lost...
	void Array_InsertAt (Array *pArray, int Index, void *pData, int DataSize);

	// Deletes the item at and moves all following items in the array up
	// to fill in the empty spot.
	void Array_DeleteAt (Array *pArray, int Index);

#else
	#define Array_GetSize(a) ((a)->ItemsAllocated)
	#define Array_GetItemSize(a) ((a)->ItemSize)
	#define Array_ItemPtr(a,i) ((void *)(((long)((a)->Items))+((i)*(a)->ItemSize)))
	#define Array_PutAt(a,i,d,s) (memcpy (Array_ItemPtr((a),(i)),(d),(s)))
	#define Array_InsertAt(a,i,d,s) \
		(memmove(Array_ItemPtr((a),(i)+1),Array_ItemPtr((a),(i)),((a)->ItemsAllocated-(i)-1)*(a)->ItemSize), \
		 memcpy(Array_ItemPtr((a),(i)),(d),(s)))
	#define Array_DeleteAt(a,i) \
		(memcpy (Array_ItemPtr((a),(i)),Array_ItemPtr((a),(i+1)),((a)->ItemsAllocated-(i)-1)*(a)->ItemSize))
#endif

#ifdef __cplusplus
	}
#endif

#endif
