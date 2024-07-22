/****************************************************************************************/
/*  EntView.cpp                                                                         */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Entity visibility list code                                           */
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
#include "stdafx.h"  // for CString.  This is really ugly!
#include "EntView.h"
#include <assert.h>
#include "RAM.H"
#include "util.h"

EntityViewList *EntityViewList_Create
	(
	  const EntityTable *pEntityDefs
	)
{
	EntityTypeList *pEntList;
	EntityViewList *pViewList;

	pViewList = NULL;
	pEntList = EntityTable_GetAvailableEntityTypes (pEntityDefs);
	if ((pEntList != NULL) && (pEntList->nTypes > 0))
	{
		pViewList = (EntityViewList *)geRam_Allocate
		  (sizeof (EntityViewList) + 
		   (pEntList->nTypes * sizeof (EntityViewEntry))
		  );
		if (pViewList != NULL)
		{
			pViewList->nEntries = pEntList->nTypes;
			for (int i = 0; i < pEntList->nTypes; i++)
			{
				pViewList->pEntries[i].pName = Util_Strdup(pEntList->TypeNames[i]);
				pViewList->pEntries[i].IsVisible = GE_TRUE;
			}
		}
		EntityTable_ReleaseEntityTypes (pEntList);
	}
	return pViewList;
}

void EntityViewList_Destroy
	(
	  EntityViewList **ppList
	)
{
	EntityViewList *pList;

	assert (ppList != NULL);

	pList = *ppList;
	assert (pList != NULL);

	for (int i = 0; i < pList->nEntries; i++)
	{
		EntityViewEntry *pEntry;

		pEntry = &(pList->pEntries[i]);
		if (pEntry->pName != NULL)
		{
			geRam_Free (pEntry->pName);
			pEntry->pName = NULL;
		}
	}
	geRam_Free (pList);
	*ppList = NULL;
}

EntityViewList *EntityViewList_Copy
	(
	  EntityViewList const *pSrc
	)
{
	EntityViewList *pDest;

	assert (pSrc != NULL);

	pDest = (EntityViewList *)geRam_Allocate
		  (sizeof (EntityViewList) + 
		   (pSrc->nEntries * sizeof (EntityViewEntry))
		  );
	if (pDest != NULL)
	{
		pDest->nEntries = pSrc->nEntries;
		for (int i = 0; i < pSrc->nEntries; ++i)
		{
			pDest->pEntries[i].pName = Util_Strdup (pSrc->pEntries[i].pName);
			pDest->pEntries[i].IsVisible = pSrc->pEntries[i].IsVisible;
		}
	}
	return pDest;
}

static void EntityViewList_SetVisibility (EntityViewList *pList, const char *EntityName, int Visible)
{
	int i;

	// find EntityName in the list
	// If found, set visibility state
	for (i = 0; i < pList->nEntries; ++i)
	{
		EntityViewEntry *pEntry;
		pEntry = &(pList->pEntries[i]);
		if (strcmp (EntityName, pEntry->pName) == 0)
		{
			pEntry->IsVisible = Visible;
			return;
		}
	}
}

#pragma warning (disable:4100)
geBoolean EntityViewList_LoadSettings
	(
	  EntityViewList *pList,
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	)
{
	int nEntries;
	int i;

	// go through the items in the .3DT file and set the flags as appropriate in
	// the master list.
	if (!Parse3dt_GetInt (Parser, (*Expected = "EntityVis"), &nEntries)) return GE_FALSE;
	for (i = 0; i < nEntries; ++i)
	{
		char EntityName[1000];
		int Visible;

		*Expected = "Entity Visibility Setting";
		if (!Parse3dt_GetLiteral (Parser, NULL, EntityName)) return GE_FALSE;
		if (!Parse3dt_GetInt (Parser, NULL, &Visible)) return GE_FALSE;
		EntityViewList_SetVisibility (pList, EntityName, Visible);
	}
	return GE_TRUE;
}
#pragma warning (default:4100)

geBoolean EntityViewList_WriteToFile
	(
	  EntityViewList *pList,
	  FILE *f
	)
{
	int i;

	if (fprintf (f, "EntityVis %d\n", pList->nEntries) < 0) return GE_FALSE;
	for (i = 0; i < pList->nEntries; ++i)
	{
		char QuotedString[1000];

		Util_QuoteString (pList->pEntries[i].pName, QuotedString);
		if (fprintf (f, "%s %d\n", QuotedString, pList->pEntries[i].IsVisible) < 0) return GE_FALSE;
	}
	return GE_TRUE;
}
