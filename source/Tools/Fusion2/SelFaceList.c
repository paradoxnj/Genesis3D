/****************************************************************************************/
/*  SelFaceList.c                                                                       */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Selected face list stuff                                              */
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
#include "SelFaceList.h"
#include "array.h"
#include "ram.h"
#include <assert.h>


struct tag_SelFaceList
{
	Array *pItems;
	int FirstFree;
};


SelFaceList *SelFaceList_Create (void)
{
	SelFaceList *pList;
	
	pList = GE_RAM_ALLOCATE_STRUCT (SelFaceList);
	if (pList != NULL)
	{
		pList->pItems = Array_Create (10, sizeof (Face *));
		if (pList->pItems != NULL)
		{
			pList->FirstFree = 0;
		}
		else
		{
			SelFaceList_Destroy (&pList);
		}
	}
	return pList;
}

void SelFaceList_Destroy (SelFaceList **ppList)
{
	SelFaceList *pList;

	assert (ppList != NULL);
	assert (*ppList != NULL);
	pList = *ppList;

	if (pList->pItems != NULL)
	{
		Array_Destroy (&pList->pItems);
	}
	geRam_Free (*ppList);
}


geBoolean SelFaceList_Add (SelFaceList *pList, Face *pFace)
{
	int i, Size;

	// go through list to see if this face is already in the list
	for (i = 0; i < pList->FirstFree; ++i)
	{
		Face *pRet;

		pRet= SelFaceList_GetFace (pList, i);
		if (pRet == pFace)
		{
			// face already in list
			return GE_FALSE;
		}
	}

	Size = Array_GetSize (pList->pItems);
	assert (pList->FirstFree <= Size);

	// Face isn't already in list.  Put it at the end...
	if (pList->FirstFree == Size)
	{
		int NewSize;
		// Need to allocate more space
		NewSize = Array_Resize (pList->pItems, 2*Size);
		if (NewSize == Size)
		{
			// couldn't resize.  Guess I can't add the face
			return GE_FALSE;
		}
	}
	Array_PutAt (pList->pItems, pList->FirstFree, &pFace, sizeof (pFace));
	++(pList->FirstFree);

	return GE_TRUE;
}

geBoolean SelFaceList_Remove (SelFaceList *pList, Face *pFace)
{
	int i;

	// find the item in the list
	for (i = 0; i < pList->FirstFree; ++i)
	{
		Face *pRet;

		pRet = SelFaceList_GetFace (pList, i);
		if (pRet == pFace)
		{
			Array_DeleteAt (pList->pItems, i);
			--(pList->FirstFree);
			return GE_TRUE;
		}
	}
	return GE_FALSE;	// not found
}

void SelFaceList_RemoveAll (SelFaceList *pList)
{
	pList->FirstFree = 0;
}

int SelFaceList_GetSize (SelFaceList *pList)
{
	return pList->FirstFree;
}

Face *SelFaceList_GetFace (SelFaceList *pList, int FaceIndex)
{
	Face **ppFace;

	ppFace = (Face **)Array_ItemPtr (pList->pItems, FaceIndex);

	return *ppFace;
}

void SelFaceList_Enum (SelFaceList *pList, SelFaceList_Callback Callback, void *lParam)
{
	int i;

	for (i = 0; i < pList->FirstFree; ++i)
	{
		Face *pFace;

		pFace = SelFaceList_GetFace (pList, i);
		Callback (pFace, lParam);
	}
}
