/****************************************************************************************/
/*  EntityTable.cpp                                                                     */
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
#include "stdafx.h"

#include "EntityTable.h"
#include <assert.h>

#include <stdio.h>
#include "cparser.h"
#include "RAM.H"
#include "util.h"
#include "Filepath.h"
#include "bitmap.h"
#include "array.h"
#include "vfile.h"

typedef struct
{
	Type *pType;
	geBitmap *pBitmap;
} TypeBmpEntry;

typedef struct
{
	Array *pItems;
	int nEntries;
} TypeBmpArray;

static TypeBmpArray *TypeBmpArray_Create (void)
{
	TypeBmpArray *pList;

	pList = GE_RAM_ALLOCATE_STRUCT (TypeBmpArray);
	if (pList != NULL)
	{
		pList->pItems = Array_Create (100, sizeof (TypeBmpEntry));
		if (pList->pItems != NULL)
		{
			pList->nEntries = 0;
		}
		else
		{
			geRam_Free (pList);
		}
	}
	return pList;
}

static TypeBmpEntry *TypeBmpArray_GetItem (TypeBmpArray *pList, int i)
{
	TypeBmpEntry *pEntry;

	pEntry = (TypeBmpEntry *)Array_ItemPtr (pList->pItems, i);
	return pEntry;
}

static void TypeBmpArray_Destroy (TypeBmpArray **ppList)
{
	int i;
	TypeBmpArray *pList;

	pList = *ppList;
	
	for (i = 0; i < pList->nEntries; ++i)
	{
		TypeBmpEntry *pEntry;

		pEntry = TypeBmpArray_GetItem (pList, i);
		geBitmap_Destroy (&pEntry->pBitmap);
	}
	Array_Destroy (&pList->pItems);
	geRam_Free (*ppList);
}


static TypeBmpEntry *TypeBmpArray_Add (TypeBmpArray *pList, Type *pType, geBitmap *pBitmap)
{
	TypeBmpEntry Entry;

	if (pList->nEntries == Array_GetSize (pList->pItems))
	{
		Array_Resize (pList->pItems, 2*pList->nEntries);
	}
	Entry.pType = pType;
	Entry.pBitmap = pBitmap;
	Array_PutAt (pList->pItems, pList->nEntries, &Entry, sizeof (TypeBmpEntry));
	++(pList->nEntries);

	return TypeBmpArray_GetItem (pList, pList->nEntries-1);
}

static TypeBmpEntry *TypeBmpArray_Find (TypeBmpArray *pList, Type *pType)
{
	int i;
	
	for (i = 0; i < pList->nEntries; ++i)
	{
		TypeBmpEntry *pEntry;

		pEntry = TypeBmpArray_GetItem (pList, i);
		if (pEntry->pType == pType)
		{
			return pEntry;
		}
	}
	return NULL;
}


struct tag_EntityTable
{
	CParser *EntityInfo;
	TypeBmpArray *BitmapCache;
};


const char EntityNameKey[] = "%name%";
static const char Unnamed[] = "Unnamed";
static const char ClassnameKey[] = "classname";
static const char TypenameKey[] = "%typename%";

/*
  The editor and tools depend on certain entities existing and
  having certain properties, and will fail horribly if those entities
  are missing or not defined properly.  So we put the definitions
  here in the code to eliminate any possibility of there being a problem.

  The names "BMP_LIGHT", etc. are the resource identifiers for these bitmaps!
*/
static const char FixedEntityDefinitions[] =
	"// Light\n"									// Light entity
	"#pragma GE_Type(\"BMP_LIGHT\")\n"
	"typedef struct	tag_light\n"
		"{\n"
		"#pragma GE_Published\n"
		"int		light;\n"
		"GE_RGBA	color;\n"
		"int		style;\n"
		"geVec3d	origin;\n"
		"#pragma GE_Origin(origin)\n"
		"#pragma GE_DefaultValue(light, \"150\")\n"
	"}	light;\n"
	"// SpotLight\n"									// SpotLight entity
	"#pragma GE_Type(\"BMP_SPOTLIGHT\")\n"
	"typedef struct	tag_spotlight\n"
		"{\n"
		"int		DummyRadius;\n"
		"#pragma GE_Published\n"
		"int		light;\n"
		"GE_RGBA	color;\n"
		"int		style;\n"
		"geVec3d	origin;\n"
		"geVec3d	angles;\n"
		"int		arc;\n"
		"#pragma GE_Origin(origin)\n"
		"#pragma GE_Radius(DummyRadius)\n"
		"#pragma GE_DefaultValue(DummyRadius, \"150\")\n"
		"#pragma GE_DefaultValue(light, \"150\")\n"
		"#pragma GE_Angles(angles)\n"
		"#pragma GE_DefaultValue(angles, \"0 0 0\")\n"
		"#pragma GE_Arc(arc)\n"
		"#pragma GE_DefaultValue(arc, \"45\")\n"
	"}	spotlight;\n"
	"// Model origin\n"								// model origin entity
	"#pragma GE_Type(\"BMP_MODELORG\")\n"
	"typedef struct tag_ModelOrigin\n"
	"{\n"
		"geVec3d origin;\n"
	"#pragma GE_Origin(origin)\n"
	"} ModelOrigin;\n"
	"// Camera\n"									// Camera entity
	"#pragma GE_Type(\"BMP_CAMERA\")\n"
	"typedef struct tag_Camera\n"
	"{\n"
		"geVec3d origin;\n"
		"geVec3d angles;\n"
		"#pragma GE_Origin(origin)\n"
		"#pragma GE_Angles(angles)\n"
		"#pragma GE_DefaultValue(angles, \"180 0 0\")\n"
	"} Camera;\n"
	"\0xFFFF";				// EOF required by parser

#pragma warning (disable:4100)
static void EntityTable_ParseErrorFunction
	(
	  char const *File,
	  int Line,
	  char const *Buff
	)
{
	MessageBox (NULL, Buff, "Fusion", MB_OK);
}
#pragma warning (default:4100)

EntityTable *EntityTable_Create (char const *DirName)
{
	EntityTable *pTable;
	BOOL NoErrors = GE_FALSE;

	// Create the structure
	pTable = GE_RAM_ALLOCATE_STRUCT (EntityTable);
	if (pTable != NULL)
	{
		pTable->EntityInfo = NULL;
		pTable->BitmapCache = TypeBmpArray_Create ();
		if (pTable->BitmapCache != NULL)
		{
			pTable->EntityInfo = CParser_Init (EntityTable_ParseErrorFunction);
			if (pTable->EntityInfo != NULL)
			{
				NoErrors = GE_TRUE;
			}
		}
	}

	if (NoErrors)
	{
		// and then initialize it

		// Parse the built-in definitions...
		CParser_ParseMemoryBlock (pTable->EntityInfo, FixedEntityDefinitions, strlen (FixedEntityDefinitions), "StandardDefs");

		// The passed DirName can specify multiple directories, separated by commas.
		// We need to parse all header files in all named directories.
		char *PathList = Util_Strdup (DirName);
		char *c;

		c = strtok (PathList, ";");
		while (c != NULL)
		{
			char EntityPathName[_MAX_PATH];
			// Now find all .H files in the directory and send them to CParser...
			HANDLE FindHandle;
			WIN32_FIND_DATA FindData;
			char FoundPath[_MAX_PATH];

			strcpy (EntityPathName, c);
			
			FilePath_SlashTerminate (EntityPathName, EntityPathName);

			// copy path for working dir...
			strcpy (FoundPath, EntityPathName);

			// append "*.H" to entity path name...
			strcat (EntityPathName, "*.H");

			FindHandle = FindFirstFile (EntityPathName, &FindData);
			if (FindHandle != INVALID_HANDLE_VALUE)
			{
				geBoolean FindResult;

				do
				{
					/*
					  The cFileName member of the WIN32_FIND_DATA structure contains
					  just the file name, not the full path.  So we have to prepend
					  the path before we do anything with the file...
					*/
					char FullFilePath[_MAX_PATH];

					strcpy (FullFilePath, FoundPath);
					strcat (FullFilePath, FindData.cFileName);

					CParser_ParseFile (pTable->EntityInfo, FullFilePath);
					FindResult = FindNextFile (FindHandle, &FindData);
				} while (FindResult != 0);

				FindClose (FindHandle);
			}
			c = strtok (NULL, ";");
		}
		geRam_Free (PathList);
	}
	if (NoErrors == GE_FALSE)
	{
		// some error.  Clean up and exit.
		EntityTable_Destroy (&pTable);
	}
	return pTable;
}

void EntityTable_Destroy (EntityTable **ppTable)
{
	EntityTable *pTable = *ppTable;

	if (pTable->EntityInfo != NULL)
	{
		CParser_Destroy (pTable->EntityInfo);
	}
	if (pTable->BitmapCache != NULL)
	{
		TypeBmpArray_Destroy (&pTable->BitmapCache);
	}
	geRam_Free (pTable);
	*ppTable = NULL;
}


static Type *EntityTable_GetEntityTypeFromName
    (
	  const EntityTable *pTable,
      char const *pName
    )
{
    StructIter si;
    Type *tp;

	assert (pTable != NULL);

    tp = CParser_GetFirstStruct (&si, pTable->EntityInfo);
    while (tp != NULL)
    {
        const char *typeName;

        typeName = CParser_GetTypeName (tp);
        if (strcmp (pName, typeName) == 0)
        {
            break;
        }
        tp = CParser_GetNextStruct (&si);
    }
    return tp;
}

geBoolean EntityTable_IsValidEntityType
	(
	  const EntityTable *pTable,
	  char const *pName
	)
{
	return (EntityTable_GetEntityTypeFromName (pTable, pName) != NULL);
}

typedef const char *(*CParser_GetSpecialFieldNameFcn)(Type *tp);

static geBoolean EntityTable_GetEntitySpecialFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &ResultFieldName,
	  CParser_GetSpecialFieldNameFcn Callback
	)
{
	char const *szFieldName;
	Type *pEntityType;

	pEntityType = EntityTable_GetEntityTypeFromName (pTable, pEntityName);
	if (pEntityType != NULL)
	{
		szFieldName = Callback (pEntityType);
		if (szFieldName != NULL)
		{
			ResultFieldName = szFieldName;
			return TRUE;
		}
	}
	return FALSE;
}


geBoolean EntityTable_GetEntityOriginFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &OriginFieldName
	)
{
	return EntityTable_GetEntitySpecialFieldName (pTable, pEntityName, OriginFieldName, CParser_GetOriginFieldName);
}


geBoolean EntityTable_GetEntityRadiusFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &RadiusFieldName
	)
{
	return EntityTable_GetEntitySpecialFieldName (pTable, pEntityName, RadiusFieldName, CParser_GetRadiusFieldName);
}

geBoolean EntityTable_GetEntityAnglesFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &AnglesFieldName
	)
{
	return EntityTable_GetEntitySpecialFieldName (pTable, pEntityName, AnglesFieldName, CParser_GetAnglesFieldName);
}

geBoolean EntityTable_GetEntityArcFieldName
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  CString &ArcFieldName
	)
{
	return EntityTable_GetEntitySpecialFieldName (pTable, pEntityName, ArcFieldName, CParser_GetArcFieldName);
}

char const *EntityTable_GetEntityFieldDoc
	(
	  const EntityTable *pTable,
	  char const *pEntityName,
	  char const *pFieldName
	)
{
	Type *pEntityType;

	pEntityType = EntityTable_GetEntityTypeFromName (pTable, pEntityName);
	if (pEntityType != NULL)
	{
		return CParser_GetTypeFieldDocumentation (pTable->EntityInfo, pEntityType, pFieldName);
	}
	return NULL;
}

static geBoolean EntityTable_GetPropertyType
    (
      Type *tp,
      char const *pKey,
	  TopType *pType
    )
{
    FieldIter	fi;
	int				res;
	int				published;
	TopType			tt;
	const char *	typeName;
	const char *	fieldName;
	const char *	defaultValue;

	res = CParser_GetFirstField(&fi, tp, &tt, &typeName, &fieldName, &published, &defaultValue);
	while (res)
	{
        if (strcmp (pKey, fieldName) == 0)
        {
			*pType = tt;
			return TRUE;
        }
		res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName, &published, &defaultValue);
	}
	return FALSE;
}

geBoolean EntityTable_GetEntityPropertyType
    (
	  const EntityTable *pTable,
	  char const *pEntityName,
	  char const *pPropertyName,
	  TopType *pType
	)
{
    Type *pEntityType;

	pEntityType = EntityTable_GetEntityTypeFromName (pTable, pEntityName);

	if (pEntityType == NULL)
	{
		return FALSE;
	}

	// and return the type for the particular property...
	return EntityTable_GetPropertyType (pEntityType, pPropertyName, pType);
}

CString EntityTable_GetEntityPropertyTypeName
    (
	  const EntityTable *pTable,
	  char const *pEntityName,
	  char const *pPropertyName
	)
{
	Type *pEntityType;

	pEntityType = EntityTable_GetEntityTypeFromName (pTable, pEntityName);
	if (pEntityType == NULL)
	{
	    assert (0);  // can't happen???
		return "Invalid";
	}
	
    FieldIter	fi;
	int				res;
	int				published;
	TopType			tt;
	const char *	typeName;
	const char *	fieldName;
	const char *	defaultValue;

	res = CParser_GetFirstField(&fi, pEntityType, &tt, &typeName, &fieldName, &published, &defaultValue);
	while (res)
	{
        if (strcmp (pPropertyName, fieldName) == 0)
        {
            return typeName;
        }
		res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName, &published, &defaultValue);
	}

    assert (0);    // doesn't even exist...
    return "Invalid";  // really need an invalid return here....
}

static int __cdecl PropCompareFcn
    (
      const void *elem1,
      const void *elem2
    )
{
    EntityProperty const *p1;
    EntityProperty const *p2;

	p1 = static_cast<EntityProperty const *>(elem1);
	p2 = static_cast<EntityProperty const *>(elem2);

	return stricmp (p1->pKey, p2->pKey);
}

static void EntityTable_FillProperty
    (
	  EntityProperty *p,
	  char const *Keyname,
	  TopType tt,
	  int Keynum,
	  char const *Value,
	  int published
	)
{
	static char * DefaultTypeValues[] =
	{
	    "0",			// T_INT
		"0.0",			// T_FLOAT
		"255 255 255",  // T_COLOR
		"0 0 0",		// T_POINT,
		"<null>",		// T_STRING
		"<null>",		// T_MODEL
		"<null>",		// T_PTR
		"<null>"		// T_STRUCT
	};

    p->pKey = Util_Strdup (Keyname);
	if ((tt < T_INT) || (tt > T_STRUCT))
	{
	    tt = T_STRING;
	}
    p->type = tt;
    p->KeyNum = Keynum;
	if (Value == NULL)
	{
		// if the value is NULL, then set it to the default
		// for this property type
		p->pValue = Util_Strdup (DefaultTypeValues[tt]);
	}
	else
	{
		p->pValue = Util_Strdup (Value);
	}
	p->published = published;
}

EntityPropertiesList *EntityTable_GetEntityPropertiesFromName
    (
	  const EntityTable *pTable,
	  CString const Classname,
	  EntityTablePropType WhichProps
	)
{
    EntityPropertiesList *pProps;
    int propNo;
    Type *pEntityType;

    // Now obtain this class type's type entry
    pEntityType = EntityTable_GetEntityTypeFromName (pTable, Classname);
    if (pEntityType == NULL)
    {
		// if we get here, we've been passed an entity type that wasn't
		// parsed.  It's an error, and the caller will have to handle
		// it somehow.
        return NULL;
    }

    // The stuff exists, so let's create the properties array.
	// Since there's no API to count the number of entries in the type array,
	// I'll loop through and count them...
    FieldIter	fi;
	int				res;
	int				published;
	TopType			tt;
	const char *	typeName;
	const char *	fieldName;
	const char *	defaultValue;

	int nEntries = 1;  // FIRST entry is for the class name....
    // should GetFirstField take a const for tp??
	res = CParser_GetFirstField(&fi, pEntityType, &tt, &typeName, &fieldName, &published, &defaultValue);
	while (res)
	{
		++nEntries;
		res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName, &published, &defaultValue);
	}

    pProps = (EntityPropertiesList *)geRam_Allocate
        (sizeof (EntityPropertiesList) + nEntries * sizeof (EntityProperty));

    if (pProps == NULL)
    {
        return NULL;
    }

    pProps->NumProps = 0;
    // initialize the structure to all NULLs
    for (propNo = 0; propNo < nEntries; propNo++)
    {
        EntityProperty *p;

        p = &(pProps->Props[propNo]);

        p->KeyNum = -1;
        p->pKey = NULL;
        p->pValue = NULL;
    }


    // fill structure with the properties...
    propNo = 0;

//	EntityProperty *p = &(pProps->Props[propNo]);
	EntityTable_FillProperty (&(pProps->Props[0]), ClassnameKey, tt, 0, Classname, 0  /*not published*/);

	propNo = 1;
	res = CParser_GetFirstField (&fi, pEntityType, &tt, &typeName, &fieldName, &published, &defaultValue);
	while (res)
	{
		if ((WhichProps == ET_ALL) || (published != 0))
		{
			EntityTable_FillProperty
				(
				  &(pProps->Props[propNo]),
				  fieldName,
				  tt,
				  0,
				  defaultValue,
				  published
				);

            propNo++;
        }        
		res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName, &published, &defaultValue);
    }
    pProps->NumProps = propNo;
    // Sort the list...
    qsort (pProps->Props, propNo, sizeof (EntityProperty), PropCompareFcn);

    // structure is built...return it.
    return pProps;
}

void EntityTable_ReleaseEntityProperties
    (
      EntityPropertiesList const *pcProps
    )
{
    int propNo;
	EntityPropertiesList *pProps = const_cast<EntityPropertiesList *>(pcProps);

    for (propNo = 0; propNo < pProps->NumProps; propNo++)
    {
        EntityProperty *p;

        p = &(pProps->Props[propNo]);
        if (p->pKey != NULL)
        {
            geRam_Free (p->pKey);
        }
        if (p->pValue != NULL)
        {
            geRam_Free (p->pValue);
        }
        p->KeyNum = -1;
    }

    geRam_Free (pProps);
}

EntityTypeList *EntityTable_GetAvailableEntityTypes
	(
	  const EntityTable *pTable
	)
{
    StructIter si;
    Type *tp;
	int TypeCount;
	EntityTypeList *pList;


	TypeCount = EntityTable_GetTypeCount (pTable);


	// allocate memory for the structure
	pList = (EntityTypeList *)geRam_Allocate (sizeof (EntityTypeList) + (TypeCount*sizeof (char *)));
	if (pList == NULL)
	{
	    return NULL;
	}
	pList->nTypes = 0;

	// now get the type names and put them in the list
	tp = CParser_GetFirstStruct (&si, pTable->EntityInfo);
	while (tp != NULL)
	{	
        const char *typeName;

        typeName = CParser_GetTypeName (tp);
		pList->TypeNames[pList->nTypes] = Util_Strdup (typeName);
		++(pList->nTypes);
        tp = CParser_GetNextStruct (&si);
	}


    return pList;

}

void EntityTable_ReleaseEntityTypes
	(
	  EntityTypeList *pList
	)
{
	assert (pList != NULL);

	// deallocate all of the strings...
	while (pList->nTypes)
	{
		char *c;

		c = pList->TypeNames[pList->nTypes-1];
		if (c != NULL)
		{
		    geRam_Free (c);
		}
		--(pList->nTypes);
	}

	// and free the structure
	geRam_Free (pList);
}

int EntityTable_WriteTypesToMap
	(
	  const EntityTable *pTable,
	  FILE *f
	)
{
	assert (pTable->EntityInfo != NULL);
	assert (f != NULL);

	return CParser_WriteTypesToMap (pTable->EntityInfo, f);
}

// GetTypeCount added 02/20/98 by Jim
int EntityTable_GetTypeCount
	(
	  const EntityTable *pTable
	)
{
    StructIter si;
    Type *tp;
	int TypeCount;

	TypeCount = 0;
    tp = CParser_GetFirstStruct (&si, pTable->EntityInfo);
    while (tp != NULL)
    {
		++TypeCount;
        tp = CParser_GetNextStruct (&si);
    }

	return TypeCount;
}

// This function assumes that the passed bitmap is a Windows 16-bit 555.
static TypeBmpEntry *CreateEntryFromLoadedBitmap (TypeBmpArray *pBitmapCache, Type *pType, HBITMAP bmpHandle)
{
	BITMAPINFO *bmi;
	BITMAPINFOHEADER *bmih;
	void *bits;
	geBitmap *pBitmap;
	HDC dcMem;
	int retval;

	dcMem = CreateCompatibleDC (NULL);

	bmi = (BITMAPINFO *)geRam_Allocate (sizeof (BITMAPINFO) + (255 * sizeof (RGBQUAD)));
	bmih = &bmi->bmiHeader;

	bmih->biSize = sizeof (BITMAPINFOHEADER);
	bmih->biBitCount = 0;

	// get bitmap information
	retval = ::GetDIBits (dcMem, bmpHandle, 0, 0, NULL, bmi, DIB_RGB_COLORS);

	// and then get the bits...
	bmih->biBitCount = 32;
	bmih->biCompression = BI_BITFIELDS;
	bits = geRam_Allocate((bmih->biWidth * bmih->biHeight) * 4);
	retval = ::GetDIBits (dcMem, bmpHandle, 0, bmih->biHeight, bits, bmi, DIB_RGB_COLORS);

	// now create our internal-format bitmap
	pBitmap = geBitmap_Create (bmih->biWidth, bmih->biHeight, 1, GE_PIXELFORMAT_32BIT_XRGB);

	// copy the bits into it
	geBitmap *OutputBitmap;
	geBitmap_LockForWrite (pBitmap, &OutputBitmap, 0, 0);

	void *NewBits = geBitmap_GetBits (OutputBitmap);

	const int CopySize = bmih->biSizeImage;

	memcpy (NewBits, bits, CopySize);

	geBitmap_UnLock (OutputBitmap);
	
	// get rid of old bits pointer and bitmap header and Windows bitmap
	geRam_Free (bits);
	geRam_Free (bmi);
	DeleteObject (bmpHandle);
	DeleteDC (dcMem);

	//convert to 555
	geBitmap_SetFormatMin(pBitmap, GE_PIXELFORMAT_16BIT_555_RGB);

	TypeBmpEntry *pEntry;

	pEntry = TypeBmpArray_Add (pBitmapCache, pType, pBitmap);

	return pEntry;
}

static TypeBmpEntry *LoadResourceBitmap (TypeBmpArray *pBitmapCache, Type *pType, const char *BmpResourceName)
{
	HBITMAP bmpHandle;

	bmpHandle = ::LoadBitmap (AfxGetInstanceHandle (), BmpResourceName);

	if (bmpHandle == NULL)
	{
		return NULL;
	}

	return CreateEntryFromLoadedBitmap (pBitmapCache, pType, bmpHandle);
}

static TypeBmpEntry *LoadDefaultEntityBitmap (TypeBmpArray *pBitmapCache)
{
	HBITMAP bmpHandle;
	
	bmpHandle = ::LoadBitmap (AfxGetInstanceHandle (), "BMP_DEFAULT");
	assert (bmpHandle != NULL);  // this would be bad!
	
	return CreateEntryFromLoadedBitmap (pBitmapCache, NULL, bmpHandle);
}

static TypeBmpEntry *LoadBitmapFromFile (TypeBmpArray *pBitmapCache, Type *pType, const char *BmpFilename)
{
	geBitmap *pBitmap = NULL;
	geVFile *vfs;

	vfs = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_DOS, BmpFilename, NULL, GE_VFILE_OPEN_READONLY);
	if (vfs != NULL)
	{
		pBitmap = geBitmap_CreateFromFile (vfs);
		geBitmap_SetFormat (pBitmap, GE_PIXELFORMAT_16BIT_555_RGB, GE_FALSE, 0, NULL);
		geVFile_Close (vfs);
	}
	if (pBitmap == NULL)
	{
		return NULL;
	}

	TypeBmpEntry *pEntry;

	pEntry = TypeBmpArray_Add (pBitmapCache, pType, pBitmap);
	return pEntry;
}

static TypeBmpEntry *EntityTable_GetBitmap
	(
	  const EntityTable *pTable, 
	  const char *EntityClassname
	)
{
	Type *pType;
	TypeBmpEntry *pEntry;

	pType = EntityTable_GetEntityTypeFromName (pTable, EntityClassname);

	if (pType == NULL)
	{
		return NULL;
	}

	// have we loaded this bitmap yet?
	pEntry = TypeBmpArray_Find (pTable->BitmapCache, pType);
	if (pEntry == NULL)
	{
		const char *BmpFilename;

		BmpFilename = CParser_GetIconName (pType);
		if (BmpFilename != NULL)
		{
			if ((strcmp (EntityClassname, "light") == 0) ||
				(strcmp (EntityClassname, "spotlight") == 0) ||
				(strcmp (EntityClassname, "ModelOrigin") == 0) ||
				(strcmp (EntityClassname, "Camera") == 0))
			{
				pEntry = LoadResourceBitmap (pTable->BitmapCache, pType, BmpFilename);
			}
			else
			{
				pEntry = LoadBitmapFromFile (pTable->BitmapCache, pType, BmpFilename);
			}
		}
	}
	return pEntry;
}


const geBitmap *EntityTable_GetBitmapPtr
	(
	  const EntityTable *pTable, 
	  const char *EntityClassname
	)
{
	TypeBmpEntry *pEntry = EntityTable_GetBitmap (pTable, EntityClassname);

	if (pEntry == NULL)
	{
		pEntry = LoadDefaultEntityBitmap (pTable->BitmapCache);
		assert (pEntry != NULL);
	}

	return pEntry->pBitmap;
}

ContentsTable *EntityTable_GetContentsList
	(
	  const EntityTable *pTable
	)
{
	int nEntries;
	ContentsTable *pContentsTable;
	int StructSize;

	assert (pTable->EntityInfo != NULL);

	nEntries = CParser_GetContentsCount (pTable->EntityInfo);
	// I allocate one more than necessary...oh well
	StructSize = sizeof (ContentsTable) + (nEntries * sizeof (ContentsTableEntry));
	pContentsTable = (ContentsTable *)geRam_Allocate (StructSize);
	if (pContentsTable == NULL)
	{
		return NULL;
	}
	pContentsTable->nEntries = nEntries;
	for (int i = 0; i < nEntries; ++i)
	{
		ContentsTableEntry *pEntry = &pContentsTable->Entries[i];
		CParser_GetContentsNameAndValue (pTable->EntityInfo, i, &pEntry->Name, &pEntry->Value);
	}
	return pContentsTable;
}


void EntityTable_FreeContentsList
	(
	  ContentsTable **ppContents
	)
{
	geRam_Free (*ppContents);
}
