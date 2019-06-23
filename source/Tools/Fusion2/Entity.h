/****************************************************************************************/
/*  Entity.h                                                                            */
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
#ifndef ENTITY_H_
#define ENTITY_H_

#include "render.h"    // very, very bad!!!!
#include "brush.h"
#include "model.h"
#include "EntityTable.h"
#include "parse3dt.h"

enum EntityStyles { ENTITY_S_ORIGIN, ENTITY_S_BRUSH };
			
enum
{
	ENTITY_SELECTED = 0x0001,
	ENTITY_ACTIVE   = 0x0002,
	ENTITY_HIDDEN	= 0x0004,
	ENTITY_LOCKED	= 0x0008
};

class CEntity;
/////////////////////////////////////////////////////////////////////////////
// make an entity array
typedef CArray<CEntity, CEntity&> CEntityArray;

typedef geBoolean (*EntityList_CB)( CEntity& Entity, void *lParam) ;
	
class CEntity
{
// Construction
public:
	void UpdateOriginFirst(const EntityTable *pEntityDefs);
	float RayDistance (CPoint point, ViewVars *v);
	float DistanceFrom (geVec3d const *pPoint);
	void Export (FILE *OutFile);
	void Move (geVec3d const *v);
	void DoneMove (double GridSize, const EntityTable *pEntityDefs);
	void Rotate (geXForm3d const *pXfmRotate, geVec3d const *pCenter, const EntityTable *pEntityDefs);
	void Scale (geFloat ScaleFactor, const EntityTable *pEntityDefs);

	void SetKeyValue (const char *Key, const char *Value);
	BOOL GetKeyValue (const char *Key, char *Value) const;
	BOOL GetKeyValue (const char *Key, CString &Value) const;
	int GetNumKeyValuePairs (void) const;
	BOOL GetKeyValuePair (int Index, CString &Key, CString &Value) const;

	void UpdateOrigin(const EntityTable *pEntityDefs);
    CEntity &operator=( CEntity &Entity );  // Right side is the argument.
	CString GetName (void) const;
	CString GetClassname (void) const;
	CEntity();
	~CEntity ();
	geBoolean IsCamera( void ) const;

	BOOL SetOrigin (geFloat x, geFloat y, geFloat z, const EntityTable *pEntityDefs);
	BOOL GetOriginString (CString &Origin, const EntityTable *pEntityDefs) const;

	BOOL GetAngles (geVec3d *pDir, const EntityTable *pEntityDefs) const;
	BOOL SetAngles (const geVec3d *pDir, const EntityTable *pEntityDefs);
	BOOL GetArc (geFloat *pArc, const EntityTable *pEntityDefs) const;
	BOOL SetArc (geFloat Arc, const EntityTable *pEntityDefs);
	BOOL GetRadius (geFloat *pRadius, const EntityTable *pEntityDefs) const;
	BOOL SetRadius (geFloat Radius, const EntityTable *pEntityDefs);

	BOOL GetSpecialField (EntityTable_GetFieldFunc Callback, CString &FieldValue, const EntityTable *pEntityDefs) const;
	BOOL SetSpecialField (EntityTable_GetFieldFunc Callback, CString const FieldValue, const EntityTable *pEntityDefs);

	// sets selected state of entity
	void Select();

	// clears selected flag of entity
	void DeSelect();

	// returns selected state of entity
	int IsSelected() const;

	// saves the entity out to a specified file stream
	geBoolean SaveToFile( FILE *file );

	// reads the entity in from a specified file stream
	geBoolean ReadFromFile (Parse3dt *Parser, int VersionMajor, int VersionMinor, const char **Expected, const EntityTable *pEntityDefs);

	// writes the entity out to the map file
	void WriteToMap (FILE *file, ModelList const *pModels, CEntityArray const *pEnts, const EntityTable *pEntityDefs) const;

	// whether this is a brush type
	// entity or an origin type entity
	EntityStyles EntityStyle;

	// the origin if it's an origin type
	geVec3d	mOrigin;

	void SetGroupId (int id);
	int GetGroupId (void) const ;
	void SetVisible (BOOL flag);
	BOOL IsVisible (void) const ;
	void SetLock (BOOL flag);
	BOOL IsLocked (void) const ;

	const geBitmap *GetBitmapPtr (const EntityTable *pEntityDefs) const;

private:
	// which group this entity belongs to.
	int mGroup;

	// some flags such as active or selected
	int mFlags;

	// array of key names
	CStringArray mKeyArray;
	// array of values
	CStringArray mValueArray;
	int GetKeyIndex (const char *Key) const;
	BOOL GetOriginFieldName (CString &FieldName, const EntityTable *pEntityDefs) const;
	int GetNumValidKeyValuePairs (ModelList const *pModels, CEntityArray const *pEnts, const EntityTable *pEntityDefs) const;
	BOOL IsValidKey (CString const &Key, ModelList const *pModels, CEntityArray const *pEnts, const EntityTable *pEntityDefs) const;
};

int EntityList_Enum
	(
		CEntityArray&       EntityArray,
		void *				lParam,
		EntityList_CB		CallBack
	) ;

CEntity * EntityList_FindByClassName
	(
	  CEntityArray  *	pEnts,
	  const char	*	pName
	) ;


#endif
