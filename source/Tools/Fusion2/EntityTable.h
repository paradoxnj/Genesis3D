/****************************************************************************************/
/*  EntityTable.h                                                                         */
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
#ifndef ENTITYTABLE_H
#define ENTITYTABLE_H

#include "basetype.h"
#include "iden.h"
#include "type.h"
#include "bitmap.h"

extern const char EntityNameKey[];

typedef struct tag_EntityTable EntityTable;

typedef struct
{
    int KeyNum;
    char *pKey;
    char *pValue;
    TopType type;
	int published;
} EntityProperty;

typedef struct
{
    int NumProps;
    EntityProperty Props[1];
} EntityPropertiesList;

enum EntityTablePropType
{
	ET_PUBLISHED,
	ET_ALL
};

struct EntityTypeList
{
	int nTypes;
	char *TypeNames[1];
};

EntityTable *EntityTable_Create (char const *DirName);
void EntityTable_Destroy (EntityTable **ppTable);

geBoolean EntityTable_GetEntityPropertyType
    (
	  const EntityTable *pTable,
	  char const *pEntityName,
	  char const *pPropertyName,
	  TopType *pType
	);

typedef geBoolean (*EntityTable_GetFieldFunc)(EntityTable const *pTable, char const *pEntityName, CString &FieldName);

geBoolean EntityTable_GetEntityOriginFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &OriginFieldName
	);

geBoolean EntityTable_GetEntityRadiusFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &RadiusFieldName
	);

geBoolean EntityTable_GetEntityAnglesFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &AnglesFieldName
	);

geBoolean EntityTable_GetEntityArcFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &ArcFieldName
	);

char const *EntityTable_GetEntityFieldDoc
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  char const *pFieldName
	);

CString EntityTable_GetEntityPropertyTypeName
    (
	  const EntityTable *pTable,
	  char const *pEntityName,
	  char const *pPropertyName
	);

EntityPropertiesList *EntityTable_GetEntityPropertiesFromName
    (
	  const EntityTable *pTable,
	  CString const Classname,
	  EntityTablePropType WhichProps
	);

geBoolean EntityTable_IsValidEntityType
	(
	  const EntityTable *pTable,
	  char const *pName
	);

void EntityTable_ReleaseEntityProperties
    (
      EntityPropertiesList const *pProps
    );

EntityTypeList *EntityTable_GetAvailableEntityTypes
	(
	  const EntityTable *pTable
	);

void EntityTable_ReleaseEntityTypes
	(
	  EntityTypeList *pList
	);

int EntityTable_WriteTypesToMap
	(
	  const EntityTable *pTable,
	  FILE *f
	);

int EntityTable_GetTypeCount
	(
	  const EntityTable *pTable
	);

const geBitmap *EntityTable_GetBitmapPtr
	(
	  const EntityTable *pTable,
	  const char *EntityClassname
	);


struct ContentsTableEntry
{
	const char *Name;
	unsigned long Value;
};

struct ContentsTable
{
	int nEntries;
	ContentsTableEntry Entries[1];
};


ContentsTable *EntityTable_GetContentsList
	(
	  const EntityTable *pTable
	);

void EntityTable_FreeContentsList
	(
	  ContentsTable **ppContents
	);

#endif
