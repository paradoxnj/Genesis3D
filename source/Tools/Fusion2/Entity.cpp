/****************************************************************************************/
/*  Entity.cpp                                                                          */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, Bruce Cooner                      */
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
#include "Entity.h"
#include <stdio.h>
#include "typeio.h"
#include <assert.h>
#include "units.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define CLASSNAME "classname"


void CEntity::Move(geVec3d const *v)
{
	assert (v != NULL);

	if(EntityStyle!=ENTITY_S_ORIGIN)
		return;

	geVec3d_Add (&mOrigin, v, &mOrigin);
}

void CEntity::Rotate
	(
	  geXForm3d const *pXfmRotate, 
	  geVec3d const *pCenter,
	  const EntityTable *pEntityDefs
	)
// Rotate the entity about the given point
{
	geVec3d NewPos;

	assert (pXfmRotate != NULL);
	assert (pCenter != NULL);

	geVec3d_Subtract (&mOrigin, pCenter, &NewPos);
	geXForm3d_Rotate (pXfmRotate, &NewPos, &NewPos);
	geVec3d_Add (&NewPos, pCenter, &NewPos);

	SetOrigin (NewPos.X, NewPos.Y, NewPos.Z, pEntityDefs);
}

void CEntity::Scale(geFloat ScaleFactor, const EntityTable *pEntityDefs)
{
	char	temp[255];
	geVec3d NewPos;

	assert(ScaleFactor > 0.0f);

	geVec3d_Scale(&mOrigin, ScaleFactor, &NewPos);

	SetOrigin(NewPos.X, NewPos.Y, NewPos.Z, pEntityDefs);

	if(GetKeyValue("light", temp))
	{
		int	lval	=atoi(temp);
		itoa((int)(((float)lval)*ScaleFactor), temp, 10);
		SetKeyValue("light", temp);
	}
}


CEntity::CEntity()
{
	// start of with new pairs but room for 10.
	mKeyArray.SetSize(0, 10);
	mValueArray.SetSize(0, 10);

	// default is an origin entity
	EntityStyle = ENTITY_S_ORIGIN;

	// start off active
	mFlags = 0;

	// start off with no group
	mGroup = 0;
}

CEntity::~CEntity ()
{
	mKeyArray.RemoveAll ();
	mValueArray.RemoveAll ();
}

geBoolean CEntity::IsCamera( void ) const
{
	return GetClassname().Compare( "Camera" ) == 0 ;
}


// sets selected state of entity
void CEntity::Select()
{
	mFlags |= ENTITY_SELECTED;
}

// clears selected flag of entity
void CEntity::DeSelect()
{ 
	mFlags &= ~ENTITY_SELECTED;
}

// returns selected state of entity
int CEntity::IsSelected() const
{
	return mFlags & ENTITY_SELECTED;
}

CString CEntity::GetClassname
	(
	  void
	) const
{
	CString MyClassname = "";

	this->GetKeyValue (CLASSNAME, MyClassname);
	return MyClassname;
}

CString CEntity::GetName 
	(
	  void
	) const
{
	CString MyName = "";

	this->GetKeyValue ("%name%", MyName);
	return MyName;
}

CEntity& CEntity::operator=( CEntity& Entity )
{
	EntityStyle = Entity.EntityStyle;
	mFlags = Entity.mFlags;
	mGroup = Entity.mGroup;
	mOrigin = Entity.mOrigin;

	mKeyArray.RemoveAll();
	mValueArray.RemoveAll();

	// copy the key/value entries
	CString Key, Value;
	for( int Current = 0; Current < Entity.GetNumKeyValuePairs (); Current++ )
	{
		if (Entity.GetKeyValuePair (Current, Key, Value))
		{
			this->SetKeyValue (Key, Value);
		}
	}

	return *this;
}

// Key/value string manipulation

int CEntity::GetKeyIndex (const char *Key) const
{
	int CurrentString;
	int NumberOfStrings = mKeyArray.GetSize();

	// go through the array of entities
	for( CurrentString = 0; CurrentString < NumberOfStrings; CurrentString++ ) {
		if( !mKeyArray[CurrentString].CompareNoCase(Key) )
			return CurrentString;
	}
    return -1;
}

void CEntity::SetKeyValue (const char *Key, const char *Value)
{
	int KeyNo;

	ASSERT (Key != NULL);
	ASSERT (*Key != '\0');  // no empty key strings
	ASSERT (Value != NULL);

	KeyNo = this->GetKeyIndex (Key);
	if (KeyNo == -1)
	{
		// doesn't exist, add it...
		// the arrays must be the same size...
		ASSERT (this->mKeyArray.GetSize () == this->mValueArray.GetSize ());
		this->mKeyArray.Add (Key);
		this->mValueArray.Add (Value);
	}
	else
	{
		this->mValueArray[KeyNo] = Value;
	}
}

BOOL CEntity::GetKeyValue (const char *Key, char *Value) const
{
	CString ValueString;

	if (this->GetKeyValue (Key, ValueString))
	{
		strcpy (Value, ValueString);
		return TRUE;
	}
	return FALSE;
}

BOOL CEntity::GetKeyValue (const char *Key, CString &Value) const
{
	int KeyNo;

	KeyNo = this->GetKeyIndex (Key);
	if (KeyNo == -1)
	{
		return FALSE;
	}
	Value = this->mValueArray[KeyNo];
	return TRUE;
}

int CEntity::GetNumKeyValuePairs 
	(
	  void
	) const
{
	return this->mKeyArray.GetSize ();
}

BOOL CEntity::GetKeyValuePair 
	(
	  int Index,
	  CString &Key, 
	  CString &Value
	) const
{
	if (Index < this->GetNumKeyValuePairs ())
	{
		Key = mKeyArray[Index];
		Value = mValueArray[Index];
		return TRUE;
	}
	return FALSE;
}

// update our origin
void CEntity::UpdateOrigin (const EntityTable *pEntityDefs)
{
	// find our origin
	CString OriginStr;
	
	// if we empty leave
	if (!GetOriginString (OriginStr, pEntityDefs))
	{
		EntityStyle = ENTITY_S_BRUSH;
		return;
	}
	else {
		EntityStyle = ENTITY_S_ORIGIN;
	}

	// get our x y z
	int x, y, z;
	sscanf(OriginStr, "%d %d %d", &x, &y, &z);

	// assign them 
	geVec3d_Set (&mOrigin, (geFloat)x, (geFloat)y, (geFloat)z);
}

BOOL CEntity::GetOriginFieldName 
	(
	  CString &FieldName,
	  const EntityTable *pEntityDefs
	) const
{
	CString Classname;

	if (!this->GetKeyValue (CLASSNAME, Classname))
	{
		return FALSE;
	}

	return EntityTable_GetEntityOriginFieldName (pEntityDefs, Classname, FieldName);
}

BOOL CEntity::SetOrigin 
	(
	  geFloat x, 
	  geFloat y, 
	  geFloat z,
	  const EntityTable *pEntityDefs
	)
{
	CString OriginFieldName;

	geVec3d_Set (&mOrigin, x, y, z);

	if (GetOriginFieldName (OriginFieldName, pEntityDefs))
	{
		CString NewOriginString;

		NewOriginString.Format ("%d %d %d", (int)x, (int)y, (int)z);
		
		// update the origin string
		SetKeyValue (OriginFieldName, NewOriginString);
	}

	return TRUE;
}

BOOL CEntity::GetOriginString
	(
	  CString &OriginStr, 
	  const EntityTable *pEntityDefs
	) const
{
	CString OriginFieldName; 
	
	if (!GetOriginFieldName (OriginFieldName, pEntityDefs) || OriginFieldName.IsEmpty ())
	{
		return FALSE;
	}

	return this->GetKeyValue (OriginFieldName, OriginStr);
}

BOOL CEntity::GetSpecialField
	(
	  EntityTable_GetFieldFunc Callback,
	  CString &FieldValue,
	  const EntityTable *pEntityDefs
	) const
{
	CString Classname;

	if (GetKeyValue (CLASSNAME, Classname))
	{
		CString FieldName;

		if (Callback (pEntityDefs, Classname, FieldName))
		{
			return GetKeyValue (FieldName, FieldValue);
		}
	}
	return FALSE;
}

BOOL CEntity::SetSpecialField
	(
	  EntityTable_GetFieldFunc Callback,
	  CString const FieldValue,
	  const EntityTable *pEntityDefs
	)
{
	CString Classname;

	if (GetKeyValue (CLASSNAME, Classname))
	{
		CString FieldName;

		if (Callback (pEntityDefs, Classname, FieldName))
		{
			SetKeyValue (FieldName, FieldValue);
			return TRUE;
		}
	}
	return FALSE;
}

// Angles are stored here in degrees, but reported in radians
BOOL CEntity::GetAngles (geVec3d *pDir, const EntityTable *pEntityDefs) const
{
	CString FieldValue;

	if (GetSpecialField (EntityTable_GetEntityAnglesFieldName, FieldValue, pEntityDefs))
	{
		geVec3d Degrees;

		sscanf (FieldValue, "%f %f %f", &Degrees.X, &Degrees.Y, &Degrees.Z);
		geVec3d_Set (pDir, Units_DegreesToRadians (Degrees.X),Units_DegreesToRadians (Degrees.Y),Units_DegreesToRadians (Degrees.Z));
		return TRUE;
	}

	return FALSE;
}

BOOL CEntity::SetAngles (const geVec3d *pDir, const EntityTable *pEntityDefs)
{
	CString FieldValue;
	geVec3d Degrees;

	geVec3d_Set (&Degrees, Units_RadiansToDegrees (pDir->X), Units_RadiansToDegrees (pDir->Y), Units_RadiansToDegrees (pDir->Z)) ;

	FieldValue.Format ("%f %f %f", Degrees.X, Degrees.Y, Degrees.Z);
	return SetSpecialField (EntityTable_GetEntityAnglesFieldName, FieldValue, pEntityDefs);
}

/*
  The arc value is stored as an integer degrees, and returned as a float radians.
*/
BOOL CEntity::GetArc (geFloat *pArc, const EntityTable *pEntityDefs) const
{
	CString FieldValue;

	if (GetSpecialField (EntityTable_GetEntityArcFieldName, FieldValue, pEntityDefs))
	{
		int ArcDegrees;

		sscanf (FieldValue, "%d", &ArcDegrees);
		*pArc = Units_DegreesToRadians (ArcDegrees);
		return TRUE;
	}
	return FALSE;
}

// SetArc takes a floating point radians value but stores the arc as an integer
// number of degrees.
BOOL CEntity::SetArc (geFloat Arc, const EntityTable *pEntityDefs)
{
	CString FieldValue;
	int ArcDegrees;

	ArcDegrees = Units_Round (Units_RadiansToDegrees (Arc));
	if( ArcDegrees > 359 )
		ArcDegrees = 359 ;
	if( ArcDegrees < 0 )
		ArcDegrees = 0 ;
	FieldValue.Format ("%d", ArcDegrees);
	return SetSpecialField (EntityTable_GetEntityArcFieldName, FieldValue, pEntityDefs);
}

BOOL CEntity::GetRadius (geFloat *pRadius, const EntityTable *pEntityDefs) const
{
	CString FieldValue;

	if (GetSpecialField (EntityTable_GetEntityRadiusFieldName, FieldValue, pEntityDefs))
	{
		sscanf (FieldValue, "%f", pRadius);
		return TRUE;
	}
	return FALSE;
}

BOOL CEntity::SetRadius (geFloat Radius, const EntityTable *pEntityDefs)
{
	CString FieldValue;

	FieldValue.Format ("%f", Radius);
	return SetSpecialField (EntityTable_GetEntityRadiusFieldName, FieldValue, pEntityDefs);
}

void CEntity::Export(FILE *OutFile)
{
	int		i;

	fprintf( OutFile, "{\n");

	for(i = 0; i < this->GetNumKeyValuePairs (); i++)
	{
		CString KeyS, ValueS;

		if (this->GetKeyValuePair (i, KeyS, ValueS))
		{
			fprintf (OutFile, "\"%s\" \"%s\"\n", (LPCSTR)KeyS, (LPCSTR)ValueS);
		}
	}

}

float CEntity::DistanceFrom (geVec3d const *pPoint)
{
	return geVec3d_DistanceBetween (pPoint, &mOrigin);
}

float CEntity::RayDistance(CPoint point, ViewVars *v)
{
	if(EntityStyle!=ENTITY_S_ORIGIN)
		return 0;

	//first check the entity bounds to see if the click is inside
	geVec3d box[8], wPoint;
	CPoint min, max, pnt;
	min.x=min.y=9999;
	max.x=max.y=-9999;

	float xPlus8  = (float)(mOrigin.X + 8.0);
	float xMinus8 = (float)(mOrigin.X - 8.0);
	float yPlus8  = (float)(mOrigin.Y + 8.0);
	float yMinus8 = (float)(mOrigin.Y - 8.0);
	float zPlus8  = (float)(mOrigin.Z + 8.0);
	float zMinus8 = (float)(mOrigin.Z - 8.0);

	geVec3d_Set (&box[0], xPlus8,  yPlus8,  zPlus8);
	geVec3d_Set (&box[1], xMinus8, yPlus8,  zPlus8);
	geVec3d_Set (&box[2], xMinus8, yMinus8, zPlus8);
	geVec3d_Set (&box[3], xPlus8,  yMinus8, zPlus8);
	geVec3d_Set (&box[4], xPlus8,  yPlus8,  zMinus8);
	geVec3d_Set (&box[5], xMinus8, yPlus8,  zMinus8);
	geVec3d_Set (&box[6], xMinus8, yMinus8, zMinus8);
	geVec3d_Set (&box[7], xPlus8,  yMinus8, zMinus8);

//	BOOL anyIn=false;

	for(int i=0;i<8;i++)
	{
		wPoint	=Render_XFormVert(v, &box[i]);
		pnt.x	= (long)(wPoint.X);
		pnt.y	= (long)(wPoint.Y);
		//anyIn=(anyIn || PtInRect(Camera.mClip, pnt));
		if(pnt.x < min.x) min.x=pnt.x;
		if(pnt.x > max.x) max.x=pnt.x;
		if(pnt.y < min.y) min.y=pnt.y;
		if(pnt.y > max.y) max.y=pnt.y;
	}

	if((point.x > min.x && point.x < max.x)
		&&(point.y > min.y && point.y < max.y))
	{
		Render_GetCameraPos(v, &wPoint);
		geVec3d_Subtract(&mOrigin, &wPoint, &wPoint);

		return	geVec3d_Length(&wPoint);
	}
	else
	{
		return	0;
	}
}

// check whether or not this entity is within this
// bounding box
//==================================================================
// When we are done moving the entity we want to snap it to the
// grid etc.
//==================================================================
#define RoundDouble(x)  (floor( (x) + 0.5 ))

static float SnapToGrid
	(
	  float x,
	  double GridSize
	)
{
	return (float)(RoundDouble ((x/GridSize) ) * GridSize) ;
}

void CEntity::DoneMove(double GridSize, const EntityTable *pEntityDefs)
{
	float x, y, z;

	// Snap to the grid
	x = SnapToGrid (mOrigin.X, GridSize);
	y = SnapToGrid (mOrigin.Y, GridSize);
	z = SnapToGrid (mOrigin.Z, GridSize);

	SetOrigin (x, y, z, pEntityDefs);
}

// update our origin
void CEntity::UpdateOriginFirst(const EntityTable *pEntityDefs)
{
	// find our origin
	CString OriginStr;

	// if we empty leave
	if (!GetOriginString (OriginStr, pEntityDefs)) 
	{
		EntityStyle = ENTITY_S_BRUSH;
		return;
	}
	else 
	{
		EntityStyle = ENTITY_S_ORIGIN;
	}

	// get our x y z
	int x, y, z;
	sscanf(OriginStr, "%d %d %d", &x, &y, &z);

	SetOrigin ((float)x, (float)y, (float)z, pEntityDefs);
}


#define CENTITYTYPE "CEntity"
#define CENTSTYLE "eStyle"
#define CENTORIGIN "eOrigin"
#define CENTRENDERORIGIN "eRenderOrigin"
#define CENTPAIRCOUNT "ePairCount"
#define CENTFLAGS "eFlags"
#define CENTGROUP "eGroup"
#define CENTKEY "K"
#define CENTVALUE "V"
#define CENDENTTYPE "End CEntity"

// -------------------------------------------------------------------------------
// saves the entity out to a specified file stream
geBoolean CEntity::SaveToFile( FILE *file )
{
	ASSERT( file );

	int	Count;	// number of keys

	if (fprintf(file, "CEntity\neStyle %d\n", EntityStyle ) < 0) return GE_FALSE;
	if (fprintf(file, "eOrigin %d %d %d 2\n",
		Units_Round( mOrigin.X ), Units_Round( mOrigin.Y ), Units_Round( mOrigin.Z ) ) < 0) return GE_FALSE;

	if (fprintf(file, "eFlags %d\n",  mFlags) < 0) return GE_FALSE;
	if (fprintf(file, "eGroup %d\n",  mGroup) < 0) return GE_FALSE;

	Count	= GetNumKeyValuePairs ();
	if (fprintf(file, "ePairCount %d\n", Count) < 0) return GE_FALSE;

	for (int j = 0; j < Count; j++)
	{
		CString Key, Value;

		if (GetKeyValuePair (j, Key, Value))
		{
			char QuotedValue[SCANNER_MAXDATA];

			Util_QuoteString (Value, QuotedValue);
			if (fprintf (file, "Key %s Value %s\n", (LPCSTR)Key, QuotedValue) < 0) return GE_FALSE;
		}
	}
	return GE_TRUE;
}


static char *StripNewline
	(
	  char *s
	)
// terminates the string at the first cr or lf character
{
	char *c;

	ASSERT (s != NULL);

	c = s;
	while (*c != '\0')
	{
		switch (*c)
		{
			case '\r' :
			case '\n' :
				*c = '\0';
				break;
			default :
				++c;
				break;
		}
	}
	return s;
}

// -----------------------------------------------------------------------------------
// reads the entity in from a specified file stream
geBoolean CEntity::ReadFromFile
	(
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected,
	  const EntityTable *pEntityDefs
	)
{
	int Trash;
	int	Count;

	assert (Parser != NULL);

	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "CEntity"))) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "eStyle"), &Trash)) return GE_FALSE;
	EntityStyle = (enum EntityStyles)Trash;

	if (!Parse3dt_GetVec3d (Parser, (*Expected = "eOrigin"), &mOrigin)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, NULL, &Trash)) return GE_FALSE;

	if ((VersionMajor == 1) && (VersionMinor < 21))
	{
		geVec3d TrashVec;
		if (!Parse3dt_GetVec3d (Parser, (*Expected = "eRenderOrigin"), &TrashVec)) return GE_FALSE;
		if (!Parse3dt_GetInt (Parser, NULL, &Trash)) return GE_FALSE;
	}

	// need to flip Z on file versions prior to 1.4
	if ((VersionMajor==1) && (VersionMinor < 4))
	{
		mOrigin.Z		=-mOrigin.Z;
	}

	if (!Parse3dt_GetInt (Parser, (*Expected = "eFlags"), &mFlags)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "eGroup"), &mGroup)) return GE_FALSE;

	if( (VersionMajor==1) && (VersionMinor <= 6 ) )
	{
		if( mGroup == -1 )	// "No group" was -1 and now is 0
			mGroup = 0 ;
	}

	if (!Parse3dt_GetInt (Parser, (*Expected = "ePairCount"), &Count)) return GE_FALSE;

	if ((VersionMajor == 1) && (VersionMinor < 14))
	{
		// old key/value stuff
		FILE *file;

		file = Scanner_GetFile (Parser->Scanner);

		for( int j = 0; j < Count; j++)
		{
			char Key[255];
			char Value[255];

			fscanf( file, "K " );
			fgets( Key, 255, file );
			StripNewline (Key);

			fscanf( file, "V " );
			fgets( Value, 255, file );
			StripNewline (Value);

			this->SetKeyValue (Key, Value);
		}
	}
	else
	{
		int j;
		char Key[SCANNER_MAXDATA];
		char Value[SCANNER_MAXDATA];

		for (j = 0; j < Count; ++j)
		{
			if (!Parse3dt_GetIdentifier (Parser, (*Expected = "Key"), Key)) return GE_FALSE;
			if (!Parse3dt_GetLiteral (Parser, (*Expected = "Value"), Value)) return GE_FALSE;
			SetKeyValue (Key, Value);
		}
	}
	/*
	  If the Z was flipped above (i.e. this is a version 1.3 file or earlier,
	  then the Z in the origin key string (if it exists) will not agree
	  with mOrigin.Z.
	  So here we make sure they agree.  This has the effect of making the
	  origin key string superfluous on save/restore, but wtf.
	*/

	//versions < 1.10 need to convert to texels from centimeters
	if ((VersionMajor==1) && (VersionMinor < 10))
	{
		geVec3d_Scale (&mOrigin, Units_CentimetersToEngine (1.0f), &mOrigin);
	}
	SetOrigin (mOrigin.X, mOrigin.Y, mOrigin.Z, pEntityDefs);

	return GE_TRUE;
}

void CEntity::SetGroupId 
	(
	  int id
	)
{
	this->mGroup = id;
}

int CEntity::GetGroupId 
	(
	  void
	) const 
{
	return this->mGroup;
}

void CEntity::SetVisible 
	(
	  BOOL flag
	)
{
	if (flag)
	{
		this->mFlags &= ~ENTITY_HIDDEN;
	}
	else
	{
		this->mFlags |= ENTITY_HIDDEN;
	}	
}

BOOL CEntity::IsVisible 
	(
	  void
	) const
{
	return !(this->mFlags & ENTITY_HIDDEN);
}

void CEntity::SetLock
	(
	  BOOL flag
	)
{
	if (flag)
	{
		this->mFlags |= ENTITY_LOCKED;
	}
	else
	{
		this->mFlags &= ~ENTITY_LOCKED;
	}
}

BOOL CEntity::IsLocked 
	(
	  void
	) const
{
	return (this->mFlags & ENTITY_LOCKED);
}

static CEntity const *EntityList_FindByName
	(
	  CEntityArray  *pEnts,
	  char const *pName
	)
{
	for (int i = 0; i < pEnts->GetSize (); ++i)
	{
		CEntity const *pEnt;
		CString Name;

//		pEnt = &((*mEntityArray)[CurrentEnt]);
		pEnt = &((*pEnts)[i]);
		pEnt->GetKeyValue ("%name%", Name);
		if (stricmp (pName, Name) == 0)
		{
			return pEnt;
		}
	}
	return NULL;
}


CEntity * EntityList_FindByClassName
	(
	  CEntityArray  *	pEnts,
	  const char	*	pName
	)
{
	CEntity * pEnt;

	for (int i = 0; i < pEnts->GetSize (); ++i)
	{
		pEnt = &((*pEnts)[i]);
		if( pEnt->GetClassname().Compare( pName ) == 0 )
		{
			return pEnt;
		}
	}
	return NULL;
}/* EntityList_FindByClassName */


BOOL CEntity::IsValidKey
	(
	  CString const &Key,
	  ModelList const *pModels, 
	  CEntityArray const *pEnts,
	  const EntityTable *pEntityDefs
	) const
// returns TRUE if the key is one of the specials, or if it has type information
{
	BOOL Success;
	TopType EntityType;
	CString EntityClassname = GetClassname ();

	if ((Key == CLASSNAME) || (Key == "%name%"))
	{
		Success = TRUE;
	}
	else
	{
		Success = EntityTable_GetEntityPropertyType (pEntityDefs, EntityClassname, Key, &EntityType);
		if (Success)
		{
			CString Value;

			GetKeyValue (Key, Value);
			switch (EntityType)
			{
				case T_STRUCT :
				{
					// look for an entity with this name...
					CEntity const *Target;

					// yeah, I know the cast is ugly.
					// FindByName didn't like taking the address of something in a
					// const array.  I don't know what the deal is...
					Target = EntityList_FindByName (const_cast<CEntityArray *>(pEnts), Value);
					Success = (Target != NULL);
					break;
				}

				case T_MODEL :
				{
					// look for model of this name in models list
					Model *pModel;

					pModel = ModelList_FindByName (pModels, Value);
					Success = (pModel != NULL);
					break;
				}

				default :
					break;
			}
		}
	}
	return Success;
}

int CEntity::GetNumValidKeyValuePairs
	(
	  ModelList const *pModels, 
	  CEntityArray const *pEnts,
	  const EntityTable *pEntityDefs
	) const
{
	int j;
	int NumKeys;
	int NumValidKeys;

	NumValidKeys = 0;
	NumKeys = GetNumKeyValuePairs ();
	for (j = 0; j < NumKeys; j++)
	{
		CString Key, Value;

		if (GetKeyValuePair (j, Key, Value))
		{
			BOOL Success;

			// only count those that have type information entries
			Success = IsValidKey (Key, pModels, pEnts, pEntityDefs);

			if (Success)
			{
				++NumValidKeys;
			}
		}		
	}
	return NumValidKeys;
}

// writes the entity out to the map file
void CEntity::WriteToMap 
	(
	  FILE *exfile, 
	  ModelList const *pModels, 
	  CEntityArray const *pEnts,
	  const EntityTable *pEntityDefs
	) const
{
	int NumValidKeys;
	int NumKeys;
#ifdef _DEBUG
	int NumKeysWritten;
#endif
	CString EntityClassname = GetClassname ();

	TypeIO_WriteInt (exfile, 0);	// numbrushes
	TypeIO_WriteInt (exfile, 0);	// no motion data today

	// output entity key/value pairs
	// We output only those keys that have type information entries.
	NumValidKeys = GetNumValidKeyValuePairs (pModels, pEnts, pEntityDefs);
	TypeIO_WriteInt (exfile, NumValidKeys);	//number of keys
	
	NumKeys = GetNumKeyValuePairs ();

#ifdef _DEBUG
	NumKeysWritten = 0;
#endif

	for(int j = 0; j < NumKeys; j++)
	{
		CString Key, Value;

		if (GetKeyValuePair (j, Key, Value))
		{
			// only output those that have type information entries
			BOOL Success;

			Success = IsValidKey (Key, pModels, pEnts, pEntityDefs);
		
			if (Success)
			{						
				char ValueString [100];  // string to store value
				TopType KeyType;

				// get the type.
				// Since classname and %name% don't have type info,
				// we need to check success flag ...
				Success = EntityTable_GetEntityPropertyType (pEntityDefs, GetClassname(), Key, &KeyType);

				// Boolean keys are converted to integer
				if (Success && (KeyType == T_BOOLEAN))
				{
					if (Value == "True")
					{
						strcpy (ValueString, "1");
					}
					else
					{
						strcpy (ValueString, "0");
					}
				}
				else
				{
					strcpy (ValueString, Value);
				}

				// output name and value
				TypeIO_WriteString (exfile, Key);
				TypeIO_WriteString (exfile, ValueString);
#ifdef _DEBUG
				++NumKeysWritten;
#endif
			}
		}
	}
#ifdef _DEBUG
	assert (NumKeysWritten == NumValidKeys);
#endif
}


int EntityList_Enum
	(
		CEntityArray&       EntityArray,
		void *				lParam,
		EntityList_CB		CallBack
	)
{
	geBoolean bResult = GE_TRUE ;	// TRUE means entire list was processed
	int	i ;

	for( i=0; i< EntityArray.GetSize(); i++ )
	{
		if( (bResult = CallBack( EntityArray[i], lParam )) == GE_FALSE )
			break ;		
	}

	return bResult ;
}

const geBitmap *CEntity::GetBitmapPtr (const EntityTable *pEntityDefs) const
{

	return EntityTable_GetBitmapPtr (pEntityDefs, GetClassname ());
}
