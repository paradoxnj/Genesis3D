/****************************************************************************************/
/*  EntTypeName.c                                                                       */
/*                                                                                      */
/*  Author:       Jim Mischel, Jeff Lomax                                               */
/*  Description:  Entity code                                                           */
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
#include "EntTypeName.h"
#include "list.h"
#include "util.h"
#include <string.h>
#include <assert.h>
#include "ram.h"

typedef struct
{
	char	*Name;
	int		Count;
} EntTypeName;

static EntTypeName *EntTypeName_Create (const char *Name)
{
	EntTypeName *et;

	assert (Name != NULL);

	et = GE_RAM_ALLOCATE_STRUCT (EntTypeName);
	if (et != NULL)
	{
		et->Name = Util_Strdup (Name);
		et->Count = 0;
	}
	return et;
}

static void EntTypeName_Destroy (EntTypeName **ppItem)
{
	assert (ppItem != NULL);
	assert (*ppItem != NULL);

	if ((*ppItem)->Name != NULL)
	{
		geRam_Free ((*ppItem)->Name);
	}
	geRam_Free (*ppItem);
	*ppItem = NULL;
}

EntTypeNameList *EntTypeNameList_Create (void)
{
	return List_Create ();
}

static void EntTypeName_DestroyCallback (void *p)
{
	EntTypeName_Destroy ((EntTypeName **)&p);
}

void EntTypeNameList_Destroy (EntTypeNameList **ppList)
{
	List_Destroy (ppList, EntTypeName_DestroyCallback);
}

static geBoolean FindName (void *pData, void *lParam)
{
	EntTypeName *et = (EntTypeName *)pData;
	char *s2 = (char *)lParam;

	return (strcmp (et->Name, s2) == 0);
}


int EntTypeNameList_UpdateCount (EntTypeNameList *pList, const char *pName)
{
	EntTypeName *et;
	char *pWorkName;
	long Num;
	long iEnd;
	ListIterator pli;

	assert (pList != NULL);
	assert (pName != NULL);

	pWorkName = Util_Strdup (pName);
	if (Util_GetEndStringValue (pWorkName, &Num, &iEnd))
	{
		pWorkName[iEnd] = '\0';
	}
	else
	{
		Num = 0;
	}

	if (!List_Search (pList, FindName, (void *)pWorkName, (void **)&et, &pli))
	{
		et = EntTypeName_Create (pWorkName);
		List_Append (pList, et);
	}
	if (Num > et->Count)
	{
		et->Count = Num;
	}
	else
	{
		++(et->Count);
		Num = et->Count;
	}
	geRam_Free (pWorkName);
	return Num;
}
