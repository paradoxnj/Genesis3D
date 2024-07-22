/****************************************************************************************/
/*  Group.cpp                                                                           */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Brush/entity group ui handling code.                                  */
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
#include "group.h"
#include "resource.h"
#include <assert.h>
#include "RAM.H"
#include "util.h"

struct tag_Group
{
	BOOL		Visible;
	BOOL		Locked;
	int			GroupId;
	char	*	GroupName;
	GE_RGBA		Color ;
	tag_Group *Next;
};


typedef struct tagCompare
{
	Group * pGroup ;
} SCompare, *PCompare ;

static int compare( const void * arg1, const void * arg2 )
{
	PCompare pLeft ;
	PCompare pRight ;

	pLeft = (PCompare)arg1 ;
	pRight = (PCompare)arg2 ;

	if( pLeft->pGroup->GroupId == 0 ) // "Default" group always 1st
		return -1 ;

	if( pRight->pGroup->GroupId == 0 )	// "Default" group always 1st
		return 1 ;

	return strcmp( pLeft->pGroup->GroupName, pRight->pGroup->GroupName ) ;

}/* ::compare */


Group *Group_Create
	(
	  int id,
	  char const *pName
	)
{
	Group *pGroup;
	
	assert (pName != NULL);

	pGroup = (Group *)geRam_Allocate (sizeof (Group));
	if (pGroup != NULL)
	{
		pGroup->Visible = TRUE;
		pGroup->Locked = FALSE ;
		pGroup->GroupId = id;
		pGroup->GroupName = Util_Strdup (pName);
		pGroup->Color.r = 255.0 ;
		pGroup->Color.g = 255.0 ;
		pGroup->Color.b = 255.0 ;
		pGroup->Color.a = 255.0 ;
		pGroup->Next = NULL;
	}
	return pGroup;
}

void Group_Destroy
	(
	  Group **ppGroup
	)
// Sets all brushes and entities GroupId fields to 0,
// and deallocates the list structure
{
	Group *pGroup;

	assert (ppGroup != NULL);
	assert (*ppGroup != NULL);

	pGroup = *ppGroup;

	if (pGroup->GroupName != NULL)
	{
		geRam_Free (pGroup->GroupName);
	}

	geRam_Free (*ppGroup);
	*ppGroup = NULL;
}


struct tag_GroupListType
{
	Group *First;
};

GroupListType *Group_CreateList
	(
	  void
	)
{
	GroupListType *pList;

	pList = (GroupListType *)geRam_Allocate (sizeof (GroupListType));
	if (pList != NULL)
	{
		pList->First = NULL;
	}
	return pList;
}

void Group_DestroyList
	(
	  GroupListType **ppList
	)
{
	GroupListType *pList;
	Group *p, *q;

	assert (ppList != NULL);
	assert (*ppList != NULL);

	pList = *ppList;
	// destroy all of the nodes
	p = pList->First;
	while (p != NULL)
	{
		q = p;
		p = p->Next;
		Group_Destroy (&q);
	}
	geRam_Free (pList);
	*ppList = NULL;
}

int Group_AddToList
	(
	  GroupListType *pList
	)
{
	Group *pGroup;
	Group *p, *g;
	int GroupId;
	char GroupName[100];

	assert (pList != NULL);

	// find first available group number
	g = pList->First;
	p = g;
	GroupId = 0;
	while ((g != NULL) && (g->GroupId == GroupId))
	{
		p = g;
		g = g->Next;
		++GroupId;
	}

	// create a new group with this id
	sprintf (GroupName, "Group%d", GroupId);
	pGroup = Group_Create (GroupId, GroupName);
	if (pGroup != NULL)
	{
		// link to parent
		if (p == NULL)
		{
			// it's the first node...
			pList->First = pGroup;
		}
		else
		{
			assert (p->Next == g);
			p->Next = pGroup;
			pGroup->Next = g;
		}
		return GroupId;
	}
	return 0;
}


void Group_RemoveFromList
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	Group *g, *p;

	assert (pList != NULL);
	g = pList->First;
	p = g;
	// search the list looking for this group ID.
	// keep track of the parent so we can delete...
	while (g != NULL)
	{
		if (g->GroupId == GroupId)
		{
			if (g == pList->First)
			{
				// deleting first node
				pList->First = g->Next;
			}
			else
			{
				// link next to parent
				p->Next = g->Next;
			}
			// go through brush list and set group number
			// to 0 for those brushes that are in this group
			{
				BrushIterator bi;
				Brush *b;

				b = BrushList_GetFirst (BrushList, &bi);
				while (b != NULL)
				{
					if (Brush_GetGroupId (b) == GroupId)
					{
						Brush_SetGroupId (b, 0);
					}
					b = BrushList_GetNext (&bi);
				}
			}
			{
				// remove entities from group
				for (int i = 0; i < Entities->GetSize (); i++)
				{
					if ((*Entities)[i].GetGroupId () == GroupId)
					{
						(*Entities)[i].SetGroupId (0);
					}
				}
			}
			Group_Destroy (&g);
			return;
		}
		p = g;
		g = g->Next;
	}
}

geBoolean GroupList_IsValidId
	(
	  const GroupListType *pList,
	  int GroupId
	)
{
	Group *g;

	assert (pList != NULL);

	g = pList->First;
	while ((g != NULL) && (g->GroupId != GroupId))
	{
		g = g->Next;
	}
	if( g != NULL )
		return GE_TRUE ;
	else
		return GE_FALSE ;
}/* GroupList_IsValidId */


static Group *Group_FindById
	(
	  GroupListType const *pList,
	  int GroupId
	)
{
	Group *g;

	assert (pList != NULL);

	g = pList->First;
	while ((g != NULL) && (g->GroupId != GroupId))
	{
		g = g->Next;
	}
	return g;
}


const char * Group_GetNameFromId
	(
	  GroupListType *pList,
	  int GroupId
	)
{
	Group *g;

	assert (pList != NULL);

	g = Group_FindById (pList, GroupId);
	if (g != NULL)
	{
		return g->GroupName;
	}
	return NULL;
}

void Group_SetGroupName (Group *g, const char *pName)
{
	assert (g != NULL);
	assert (pName != NULL);

	if (g->GroupName != NULL)
	{
		geRam_Free (g->GroupName);
	}
	g->GroupName = Util_Strdup (pName);
}

void Group_SetName
	(
	  GroupListType *pList,
	  int GroupId,
	  const char *pName
	)
{
	assert (pList != NULL);
	assert (pName != NULL);

	Group *g;

	g = Group_FindById (pList, GroupId);
	assert (g != NULL);
	Group_SetGroupName (g, pName);
}

int Group_GetIdFromName
	(
	  GroupListType *pList,
	  char const *pName
	)
{
	Group *g;

	assert (pList != NULL);
	assert (pName != NULL);

	g = pList->First;
	while (g != NULL)
	{
		if (stricmp (g->GroupName, pName) == 0)
		{
			return g->GroupId;
		}
		g = g->Next;
	}
	return NO_MORE_GROUPS;
}

#pragma warning (disable:4100)
void Group_AddBrush
	(
	  GroupListType *pList,
	  int GroupId,
	  Brush *pBrush
	)
{
	assert (pList != NULL);
	assert (pBrush != NULL);
	// just making sure it's a good group...
	assert (Group_FindById (pList, GroupId) != NULL);

	Brush_SetGroupId (pBrush, GroupId);
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
void Group_RemoveBrush
	(
	  GroupListType *pList,
	  int GroupId,
	  Brush *pBrush
	)
{
	assert (pList != NULL);
	assert (pBrush != NULL);
	// just making sure it's a good group
	assert (Group_FindById (pList, GroupId) != NULL);
		
	Brush_SetGroupId (pBrush, 0);
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
void Group_AddEntity
	(
	  GroupListType *pList,
	  int GroupId,
	  CEntity *pEntity
	)
{
	assert (pList != NULL);
	assert (pEntity != NULL);
	// just making sure it's a good group
	assert (Group_FindById (pList, GroupId) != NULL);

	pEntity->SetGroupId (GroupId);
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
void Group_RemoveEntity
	(
	  GroupListType *pList,
	  int GroupId,
	  CEntity *pEntity
	)
{
	assert (pList != NULL);
	assert (pEntity != NULL);
	// just making sure it's a good group
	assert (Group_FindById (pList, GroupId) != NULL);
#pragma todo( "See if this entity is really in this group?" )

	pEntity->SetGroupId (0);
}
#pragma warning (default:4100)

BOOL Group_IsVisible
	(
	  GroupListType *pList,
	  int GroupId
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	// just a nice little check here...
	assert (pGroup != NULL);

	return pGroup->Visible;
}

static void Group_SetVisible
	(
	  Group *pGroup,
	  BOOL Visible,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	assert (pGroup != NULL);
	assert (BrushList != NULL);
	assert (Entities != NULL);

	pGroup->Visible = Visible;

// HACK!  HACK!  Don't set local attributes to invisible, use only group attributes
// I'm letting this set to visible though--in case there are invisible items we need
// to get access to...
	if( Visible == GE_FALSE )
		return ;
// END HACK

	// iterate all brushes and entities, setting visible flag
	{
		BrushIterator bi;
		Brush *b;

		b = BrushList_GetFirst (BrushList, &bi);
		while (b != NULL)
		{
			if (Brush_GetGroupId (b) == pGroup->GroupId)
			{
				Brush_SetVisible (b, Visible);
			}
			b = BrushList_GetNext (&bi);
		}
	}
	{
		for (int i = 0; i < Entities->GetSize (); i++)
		{
			if ((*Entities)[i].GetGroupId () == pGroup->GroupId)
			{
				(*Entities)[i].SetVisible (Visible);
			}
		}
	}
}

void Group_Hide
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	assert (pGroup != NULL);

	Group_SetVisible (pGroup, FALSE, BrushList, Entities);
}

void Group_Show
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	assert (pGroup != NULL);
	
	Group_SetVisible (pGroup, TRUE, BrushList, Entities);
}

BOOL Group_IsLocked
	(
	  GroupListType *pList,
	  int GroupId
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	// just a nice little check here...
	assert (pGroup != NULL);

	return pGroup->Locked;
}

static void Group_SetLock
	(
	  Group *pGroup,
	  BOOL Locked,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	assert (pGroup != NULL);
	assert (BrushList != NULL);
	assert (Entities != NULL);

	pGroup->Locked = Locked;

	// iterate all brushes and entities, setting Locked flag
	{
		BrushIterator bi;
		Brush *b;

		b = BrushList_GetFirst (BrushList, &bi);
		while (b != NULL)
		{
			if (Brush_GetGroupId (b) == pGroup->GroupId)
			{
				Brush_SetLocked (b, Locked);
			}
			b = BrushList_GetNext (&bi);
		}
	}
	{
		for (int i = 0; i < Entities->GetSize (); i++)
		{
			if ((*Entities)[i].GetGroupId () == pGroup->GroupId)
			{
				(*Entities)[i].SetLock (Locked);
			}
		}
	}
}

void Group_Lock
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	assert (pGroup != NULL);

	Group_SetLock (pGroup, TRUE, BrushList, Entities);
}


void Group_Unlock
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	assert (pGroup != NULL);

	Group_SetLock (pGroup, FALSE, BrushList, Entities);
}


// returns group ID.  NO_MORE_GROUPS if done.
// 0 is the immutable default id!!
int Group_GetFirstId
	(
	  GroupListType const *pList,
	  GroupIterator *gi
	)
{
	assert (pList != NULL);
	assert (gi != NULL);

	*gi = pList->First;

	if (*gi != NULL)
	{
		return (*gi)->GroupId;
	}
	else
	{
		return NO_MORE_GROUPS ;
	}
}


#pragma warning (disable:4100)
int Group_GetNextId
	(
	  GroupListType const *pList,
	  GroupIterator *gi
	)
{
	assert (pList != NULL);
	assert (gi != NULL);
	assert (*gi != NULL);

	*gi = (*gi)->Next;

	if (*gi != NULL)
	{
		return (*gi)->GroupId;
	}
	else
	{
		return NO_MORE_GROUPS ;
	}
}
#pragma warning (default:4100)


geBoolean Group_WriteList
	(
	  GroupListType const *pList,
	  FILE *f
	)
{
	Group const *g;

	assert (pList != NULL);
	assert (f != NULL);

	g = pList->First;
	while (g != NULL)
	{
		char Name[300];

		Util_QuoteString (g->GroupName, Name);
		if (fprintf (f, "Group %s\n", Name) < 0) return GE_FALSE;
		if (fprintf (f, "\tGroupId %d\n", g->GroupId) < 0) return GE_FALSE;
		if (fprintf (f, "\tVisible %d\n", (g->Visible ? 1 : 0)) < 0) return GE_FALSE;
		if (fprintf (f, "\tLocked %d\n", (g->Locked ? 1 : 0)) < 0) return GE_FALSE;
		if (fprintf (f, "Color %f %f %f\n", g->Color.r, g->Color.g, g->Color.b ) < 0) return GE_FALSE;

		g = g->Next;
	}
	return GE_TRUE;
}

geBoolean Group_ReadList
	(
	  GroupListType *pList,
	  int nGroups,		// so we know how many to read
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	)
{
	int Count;
	char GroupName[MAX_PATH];
	int GroupId;

	assert (pList != NULL);

	// list end pointer for fast append
	for (Count = 0; Count < nGroups; Count++)
	{
		Group *g;

		if ((VersionMajor == 1) && (VersionMinor < 19))
		{
			if (!Parse3dt_GetIdentifier (Parser, (*Expected = "Group"), GroupName)) return GE_FALSE;
		}
		else
		{
			if (!Parse3dt_GetLiteral (Parser, (*Expected = "Group"), GroupName)) return GE_FALSE;
		}

		if (!Parse3dt_GetInt (Parser, (*Expected = "GroupId"), &GroupId)) return GE_FALSE;

		// create the group structure
		g = Group_Create (GroupId, GroupName);
		if (g == NULL)
		{
			return 0;
		}

		// read the rest of the group data...
		int TempInt;

		if (!Parse3dt_GetInt (Parser, (*Expected = "Visible"), &TempInt)) return GE_FALSE;
		g->Visible = !(!TempInt);
		if (!Parse3dt_GetInt (Parser, (*Expected = "Locked"), &TempInt)) return GE_FALSE;
		g->Locked = !(!TempInt);
		
		if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 9)))
		{
			if (!Parse3dt_GetFloat (Parser, (*Expected = "Color"), &g->Color.r)) return GE_FALSE;
			if (!Parse3dt_GetFloat (Parser, NULL, &g->Color.g)) return GE_FALSE;
			if (!Parse3dt_GetFloat (Parser, NULL, &g->Color.b)) return GE_FALSE;
		}
		else
		{
			g->Color.r = g->Color.g = g->Color.b = 255.0f ;
		}

		g->Color.a = 255.0f ;

		// If this group id already exists, then get rid of it...
		if (Group_FindById (pList, g->GroupId) != NULL)
		{
			Group_Destroy (&g);
		}
		else
		{
			GroupList_Add (pList, g);
		}
	}

	return GE_TRUE;
}

int Group_GetCount
	(
	  GroupListType const *pList
	)
{
	Group const *g;
	int Count;

	assert (pList != NULL);

	Count = 0;
	g = pList->First;
	while (g != NULL)
	{
		++Count;
		g = g->Next;
	}
	return Count;
}

const GE_RGBA * Group_GetColor
	(
	  const GroupListType *pList,
	  int GroupId
	)
{
	Group *pGroup;

	assert (pList != NULL);

	pGroup = Group_FindById (pList, GroupId);
	// just a nice little check here...
	assert (pGroup != NULL);

	return &pGroup->Color ;
}

void Group_SetColor
	(
	  GroupListType *pList,
	  int GroupId,
	  const GE_RGBA * pColor
	)
{
	assert (pList != NULL);
	assert (pColor != NULL);

	Group *g;

	g = Group_FindById (pList, GroupId);
	assert (g != NULL);
	g->Color = *pColor ;
}

void GroupList_FillCombobox( const GroupListType *pList, CComboBox * pCombo, int GroupId )
{
	Group *	g;
	PCompare			pSortArray ;
	int					i ;
	int					Count ;
	int					Index ;

	pCombo->ResetContent() ;
	Count = Group_GetCount( pList ) ;
	assert( Count ) ;

	pSortArray = (PCompare)geRam_Allocate ( sizeof(SCompare) * Count ) ;
	if( pSortArray == NULL )
		return ;

	g = pList->First;
	i = 0 ;
	while (g != NULL)
	{
		pSortArray[i].pGroup = g ;
		i++ ;
		g = g->Next;
	}

	qsort( pSortArray, Count, sizeof( SCompare ), compare ) ;

	for( i=0; i < Count; i++ )
	{
		Index = pCombo->AddString( pSortArray[i].pGroup->GroupName );
		pCombo->SetItemData( Index, pSortArray[i].pGroup->GroupId );
		if( pSortArray[i].pGroup->GroupId == GroupId )
		{
			pCombo->SetCurSel( Index );
		}
	}// Fill the list
	geRam_Free ( pSortArray ) ;

}/* GroupList_FillCombobox */


Group *GroupList_GetFromId (GroupListType *pList, int GroupId)
{
	return Group_FindById (pList, GroupId);
}

Group *Group_Clone (const Group *OldGroup)
{
	Group *pGroup;

	assert (OldGroup != NULL);

	pGroup = Group_Create (OldGroup->GroupId, OldGroup->GroupName);
	if (pGroup != NULL)
	{
		pGroup->Visible = OldGroup->Visible;
		pGroup->Locked = OldGroup->Locked;
		pGroup->Color = OldGroup->Color;
	}
	return pGroup;
}


void GroupList_Add (GroupListType *Groups, Group *pGroup)
{
	Group *p, *q;

	// find the insertion spot
	p = Groups->First;
	q = NULL;
	while (p != NULL)
	{
		if (p->GroupId > pGroup->GroupId)
		{
			break;
		}
		q = p;
		p = p->Next;
	}

	if (q == NULL)
	{
		Groups->First = pGroup;
	}
	else
	{
		pGroup->Next = q->Next;
		q->Next = pGroup;
	}
}

const char *Group_GetName (const Group *pGroup)
{
	assert (pGroup != NULL);

	return pGroup->GroupName;
}

typedef struct
{
	int OldId;
	int NewId;
} Group_MapEntry;

static geBoolean Group_RenumBrush (Brush *pBrush, void *lParam)
{
	Group_MapEntry *Mappings;
	int i, GroupId;

	Mappings = (Group_MapEntry *)lParam;
	GroupId = Brush_GetGroupId (pBrush);

	for (i = 0; Mappings[i].OldId != 0; ++i)
	{
		if (Mappings[i].OldId == GroupId)
		{
			Brush_SetGroupId (pBrush, Mappings[i].NewId);
			break;
		}
	}
	return GE_TRUE;
}

static geBoolean Group_RenumEntity (CEntity &Ent, void *lParam)
{
	Group_MapEntry *Mappings;
	int i, GroupId;

	Mappings = (Group_MapEntry *)lParam;

	GroupId = Ent.GetGroupId ();
	
	for (i = 0; Mappings[i].OldId != 0; ++i)
	{
		if (Mappings[i].OldId == GroupId)
		{
			Ent.SetGroupId ( Mappings[i].NewId);
			break;
		}
	}
	return GE_TRUE;
}

void GroupList_Collapse (GroupListType *Groups, int StartingId, BrushList *Brushes, CEntityArray *Entities)
{
	int nEntries;		
		
	assert (Groups != NULL);
	assert (StartingId > 0);
	assert (Brushes != NULL);
	assert (Entities != NULL);

	nEntries = Group_GetCount (Groups);
	if (nEntries > 1)
	{
		Group_MapEntry *Mappings;

		Mappings = (Group_MapEntry *)geRam_Allocate (nEntries * sizeof (Group_MapEntry));
		if (Mappings != NULL)
		{
			int i;
			Group *pGroup;
			
			pGroup = Groups->First;
			assert (pGroup->GroupId == 0);

			pGroup = pGroup->Next;
			i = 0;
			while (pGroup != NULL)
			{
				assert (pGroup->GroupId != 0);
				Mappings[i].OldId = pGroup->GroupId;
				Mappings[i].NewId = i+StartingId;
				pGroup->GroupId = Mappings[i].NewId;
				++i;
				pGroup = pGroup->Next;
			}
			Mappings[i].OldId = 0;	// stop sentinel
			Mappings[i].NewId = 0;

			BrushList_Enum (Brushes, Mappings, Group_RenumBrush);
			EntityList_Enum (*Entities, Mappings, Group_RenumEntity);

			geRam_Free (Mappings);
		}
		
	}
}
