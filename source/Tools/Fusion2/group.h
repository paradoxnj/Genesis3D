/****************************************************************************************/
/*  group.h                                                                             */
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
#ifndef GROUP_H
#define GROUP_H

#include <BASETYPE.H>
#include <getypes.h>
#include "brush.h"
#include "entity.h"
#include "Parse3dt.h"

typedef struct tag_GroupListType GroupListType;
typedef struct tag_Group Group;

#define NO_MORE_GROUPS	(-1)

Group *Group_Create
	(
	  int id,
	  char const *pName
	);

geBoolean GroupList_IsValidId
	(
	  const GroupListType *pList,
	  int GroupId
	);

GroupListType *Group_CreateList
	(
	  void
	);

void Group_DestroyList
	(
	  GroupListType **ppList
	);

int Group_AddToList
	(
	  GroupListType *pList
	);

void Group_RemoveFromList
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	);

const char * Group_GetNameFromId
	(
	  GroupListType *pList,
	  int GroupId
	);

void Group_SetName
	(
	  GroupListType *pList,
	  int GroupId,
	  const char *pName
	);

int Group_GetIdFromName
	(
	  GroupListType *pList,
	  char const *pName
	);

void Group_AddBrush
	(
	  GroupListType *pList,
	  int GroupId,
	  Brush *pBrush
	);

void Group_RemoveBrush
	(
	  GroupListType *pList,
	  int GroupId,
	  Brush *pBrush
	);

void Group_AddEntity
	(
	  GroupListType *pList,
	  int GroupId,
	  CEntity *pEntity
	);

void Group_RemoveEntity
	(
	  GroupListType *pList,
	  int GroupId,
	  CEntity *pEntity
	);

BOOL Group_IsVisible
	(
	  GroupListType *pList,
	  int GroupId
	);

void Group_Hide
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	);

void Group_Show
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	);

BOOL Group_IsLocked
	(
	  GroupListType *pList,
	  int GroupId
	);

void Group_Lock
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	);

void Group_Unlock
	(
	  GroupListType *pList,
	  int GroupId,
	  BrushList *BrushList,
	  CEntityArray *Entities
	);

// Group iterators
typedef struct tag_Group * GroupIterator;

// returns group ID.  0 if no more groups.
// 0 is not a valid group id!!
int Group_GetFirstId
	(
	  GroupListType const *pList,
	  GroupIterator *gi
	);

int Group_GetNextId
	(
	  GroupListType const *pList,
	  GroupIterator *gi
	);

geBoolean Group_WriteList
	(
	  GroupListType const *pList,
	  FILE *f
	);

geBoolean Group_ReadList
	(
	  GroupListType *pList,
	  int nGroups,		// so we know how many to read
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	);

int Group_GetCount
	(
	  GroupListType const *pList
	);

const GE_RGBA * Group_GetColor
	(
	  const GroupListType *pList,
	  int GroupId
	);

void Group_SetColor
	(
	  GroupListType *pList,
	  int GroupId,
	  const GE_RGBA * pColor
	);

void GroupList_FillCombobox
	( 
		const GroupListType *pList, 
		CComboBox * pCombo, 
		int GroupId 
	);
	
Group *GroupList_GetFromId (GroupListType *pList, int GroupId);
Group *Group_Clone (const Group *OldGroup);

void GroupList_Add (GroupListType *Groups, Group *pGroup);
const char *Group_GetName (const Group *pGroup);

void GroupList_Collapse (GroupListType *Groups, int StartingId, BrushList *Brushes, CEntityArray *Entities);
void Group_SetGroupName (Group *pGroup, const char *Name);

#endif	// __GROUP_H
