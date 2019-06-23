/****************************************************************************************/
/*  Map.cpp                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Module for loading a .map file.                                        */
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
#include <Windows.h>

#include "Map.h"
#include "GBSPFile.h"
#include "Texture.h"
#include "BSP.h"
#include "Stdio.h"
#include "Math.h"
#include "Brush2.h"
#include "Poly.h"
#include "Mathlib.h"

#include "Motion.h"				// From Genesis (in include path)
#include "Errorlog.h"
#include "Ram.h"

int32		NumEntities;
MAP_Entity	Entities[MAX_MAP_ENTITIES];

//=====================================================================================
//	Entity parsing stuff
//=====================================================================================
char *ValueForKey (MAP_Entity *Ent, char *Key)
{
	MAP_Epair	*Ep;
	
	for (Ep=Ent->Epairs ; Ep ; Ep=Ep->Next)
		if (!stricmp (Ep->Key, Key) )
			return Ep->Value;
	return "";
}

//=====================================================================================
//=====================================================================================
void SetKeyValue (MAP_Entity *Ent, char *Key, char *Value)
{
	MAP_Epair	*Ep;
	
	for (Ep=Ent->Epairs ; Ep ; Ep=Ep->Next)
	{
		if (!stricmp (Ep->Key, Key) )
		{
			geRam_Free(Ep->Value);
			Ep->Value = NewString(Value);
			return;
		}
	}

	Ep = GE_RAM_ALLOCATE_STRUCT(MAP_Epair);
	Ep->Next = Ent->Epairs;
	Ent->Epairs = Ep;
	Ep->Key = NewString(Key);
	Ep->Value = NewString(Value);
}

//=====================================================================================
//=====================================================================================
float FloatForKey (MAP_Entity *Ent, char *Key)
{
	char	*k;
	
	k = ValueForKey (Ent, Key);
	return (float)atof(k);
}

//=====================================================================================
//=====================================================================================
void GetVectorForKey (MAP_Entity *Ent, char *Key, geVec3d *Vec)
{
	char	*k;
	double	v1, v2, v3;

	k = ValueForKey (Ent, Key);
	v1 = v2 = v3 = 0;

	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);
	
	Vec->X = (geFloat)v1;
	Vec->Y = (geFloat)v2;
	Vec->Z = (geFloat)v3;
}

//=====================================================================================
//=====================================================================================
geBoolean GetVectorForKey2 (MAP_Entity *Ent, char *Key, geVec3d *Vec)
{
	char	*k;
	double	v1, v2, v3;

	Vec->X = (geFloat)0;
	Vec->Y = (geFloat)0;
	Vec->Z = (geFloat)0;

	k = ValueForKey (Ent, Key);
	if (!k || !k[0])
		return GE_FALSE;

	v1 = v2 = v3 = 0;

	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);

	Vec->X = (geFloat)v1;
	Vec->Y = (geFloat)v2;
	Vec->Z = (geFloat)v3;

	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
void GetColorForKey (MAP_Entity *Ent, char *Key, geVec3d *Vec)
{
	char	*k;
	double	v1, v2, v3;

	k = ValueForKey (Ent, Key);
	v1 = v2 = v3 = 0;

	sscanf (k, "%lf %lf %lf", &v1, &v2, &v3);

	Vec->X = (geFloat)v1;
	Vec->Y = (geFloat)v2;
	Vec->Z = (geFloat)v3;
}

//============================================================================
//============================================================================
char *NewString(char *Str)
{
	char *NewStr = GE_RAM_ALLOCATE_ARRAY(char, strlen(Str)+1);

	if (!NewStr)
	{
		GHook.Error("New string: Not enough memory.\n");
		return NULL;
	}

	strcpy(NewStr, Str);

	return NewStr;
}

//============================================================================
//	SaveEntityData
//	Reads and writes to/from global Entities structure (love them globals :)
//============================================================================

//============================================================================
//	SaveEntityData
//============================================================================
geBoolean SaveEntityData(geVFile *VFile)
{
	int32		i;

	// Write the number of entities
	if (!geVFile_Write(VFile, &NumEntities, sizeof(int32)))
		return GE_FALSE;

	// Write out each entity
	for (i=0 ; i< NumEntities; i++)
	{
		MAP_Epair	*Ep;
		int32		NumEpairs;
		
		// Count the epairs
		for (NumEpairs = 0, Ep = Entities[i].Epairs ; Ep ; Ep = Ep->Next, NumEpairs++);

		// Write out how many epsirs
		if (!geVFile_Write(VFile, &NumEpairs, sizeof(int32)))
			return GE_FALSE;

		for (Ep = Entities[i].Epairs ; Ep ; Ep = Ep->Next)
		{
			int32		Size;

			// Write out the Key
			Size = strlen(Ep->Key)+1;

			if (!geVFile_Write(VFile, &Size, sizeof(int32)))
				return GE_FALSE;

			if (!geVFile_Write(VFile, Ep->Key, sizeof(uint8)*Size))
				return GE_FALSE;

			// Write out the Value
			Size = strlen(Ep->Value)+1;

			if (!geVFile_Write(VFile, &Size, sizeof(int32)))
				return GE_FALSE;

			if (!geVFile_Write(VFile, Ep->Value, sizeof(uint8)*Size))
				return GE_FALSE;
		
			//GHook.Printf("  Writing: Key: %s, Value: %s\n", Ep->Key, Ep->Value);
		}
	}

	return GE_TRUE;
}

//============================================================================
//	LoadEntityData
//============================================================================
geBoolean LoadEntityData(geVFile *VFile)
{
	int32		i;

	// Get the number of entities
	if (!geVFile_Read(VFile, &NumEntities, sizeof(int32)))
		return GE_FALSE;

	//GHook.Printf("Reading %i Entities...\n", NumEntities);
	
	// Load all entities
	for (i=0; i< NumEntities; i++)
	{
		MAP_Entity	*Entity;
		MAP_Epair	*Epair, *Current;
		int32		Size, e;
		int32		NumEpairs;

		//GHook.Printf("    Reading Entity...\n");

		Entity = &Entities[i];

		memset(Entity, 0, sizeof(MAP_Entity));

		if (!geVFile_Read(VFile, &NumEpairs, sizeof(int32)))
			return GE_FALSE;

		Current = NULL;

		//GHook.Printf("        Reading %i Epairs...\n", NumEpairs);

		for (e=0; e< NumEpairs; e++)
		{
			Epair = GE_RAM_ALLOCATE_STRUCT(MAP_Epair);
		
			if (!Epair)
				return GE_FALSE;

			memset (Epair, 0, sizeof(MAP_Epair));

			// Read the size of the key
			if (!geVFile_Read(VFile, &Size, sizeof(int32)))
				return GE_FALSE;

			Epair->Key = GE_RAM_ALLOCATE_ARRAY(char, Size);
			if (!Epair->Key)
				return GE_FALSE;
		
			// Read the Key
			if (!geVFile_Read(VFile, Epair->Key, Size))
				return GE_FALSE;

			// Read the size of the value
			if (!geVFile_Read(VFile, &Size, sizeof(int32)))
				return GE_FALSE;

			Epair->Value = GE_RAM_ALLOCATE_ARRAY(char, Size);
			if (!Epair->Value)
				return GE_FALSE;

			// Read the Value
			if (!geVFile_Read(VFile, Epair->Value, Size))
				return GE_FALSE;
			
			// Add to end of list
			Epair->Next = NULL;

			if (!Current)
			{
				Entity->Epairs = Epair;
				Current = Epair;
			}
			else
			{
				Current->Next = Epair;
				Current = Epair;
			}

			//GHook.Printf("  Reading: Key: %s, Value: %s\n", Epair->Key, Epair->Value);

			if (!stricmp(Epair->Key, "ClassName"))
				strcpy (Entity->ClassName, Epair->Value);
			else if (!stricmp(Epair->Key, "Target"))
				strcpy (Entity->Target, Epair->Value);
			else if (!stricmp(Epair->Key, "TargetName"))
				strcpy (Entity->TargetName, Epair->Value);
			else if (!stricmp(Epair->Key, "Origin"))
			{
				double	Val[3];
			
				Entity->Flags |= ENTITY_HAS_ORIGIN;
				
				if (sscanf(Epair->Value, "%lf %lf %lf", &Val[0], &Val[1], &Val[2]) == 3)
				{
					int32		j;
			
					for (j=0; j< 3; j++)
							VectorToSUB(Entity->Origin, j) = (geFloat)Val[j];
				}
			}
			else if (!strnicmp(Epair->Key, "Light", 5) || !strnicmp(Epair->Key, "_Light", 5))
			{
				Entity->Light = (int32)atof(Epair->Value);
			}
			else if (!stricmp(Epair->Key, "LType") || !stricmp(Epair->Key, "Style"))
			{
				Entity->LType = (int32)atof(Epair->Value);
				
				//GHook.Printf ("LType: %i\n", Entity->LType);

				if (Entity->LType > 254)
				{
					GHook.Error ("Bad light LType %i (must be 0-254)\n", Entity->LType);
					return GE_FALSE;
				}
			}
			else if (!strcmp(Epair->Key, "Angle"))
			{
				Entity->Angle = (float)atof(Epair->Value);
			}
			
			if (Entity->Light < 0 || Entity->Light > 2500)
				GHook.Printf("*WARNING*  Bad light value: %i\n", Entity->Light);
		}
	}
	//GHook.Printf ("Num Entities Read    : %5i\n", NumEntities);

	return GE_TRUE;
}

//========================================================================================
//	ConvertGFXEntDataToEntities
//========================================================================================
geBoolean ConvertGFXEntDataToEntities(void)
{
	geVFile_MemoryContext	Context;
	geVFile					*MemFile;

	//GHook.Printf("ConvertGFXEntDataToEntities: %i\n", NumGFXEntData);

	Context.Data = GFXEntData;
	Context.DataLength = NumGFXEntData;

	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_READONLY);

	if (!MemFile)
		return GE_FALSE;
	
	if (!LoadEntityData(MemFile))
	{
		geVFile_Close(MemFile);
		return GE_FALSE;
	}

	geVFile_Close(MemFile);

	return GE_TRUE;
}

//========================================================================================
//	ConvertEntitiesToGFXEntData
//========================================================================================
geBoolean ConvertEntitiesToGFXEntData(void)
{
	geVFile_MemoryContext	Context;
	geVFile					*MemFile;

	Context.Data = NULL;
	Context.DataLength = 0;

	MemFile = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_MEMORY, NULL, &Context, GE_VFILE_OPEN_CREATE);

	if (!MemFile)
		return GE_FALSE;
	
	if (!SaveEntityData(MemFile))
	{
		geVFile_Close(MemFile);
		return GE_FALSE;
	}

	if (!geVFile_UpdateContext(MemFile, &Context, sizeof(Context)))
	{
		geVFile_Close(MemFile);
		return GE_FALSE;
	}

	GFXEntData = GE_RAM_ALLOCATE_ARRAY(uint8, Context.DataLength);

	memcpy(GFXEntData, Context.Data, Context.DataLength);

	NumGFXEntData = Context.DataLength;

	geVFile_Close(MemFile);

	return GE_TRUE;
}

//============================================================================
//	FreeAllEntities
//============================================================================
void FreeAllEntities(void)
{
	MAP_Entity	*Entity;
	MAP_Epair	*Epair, *Next;
	int32		i;

	for (i=0; i< NumEntities; i++)
	{
		Entity = &Entities[i];

		if (Entity->Brushes2)
			FreeMapBrushList(Entity->Brushes2);

		Entity->Brushes2 = NULL;

		for (Epair = Entity->Epairs; Epair; Epair = Next)
		{
			Next = Epair->Next;
			geRam_Free(Epair->Key);
			geRam_Free(Epair->Value);
			geRam_Free(Epair);
		}

		Entity->Epairs = NULL;
	}

	NumEntities = 0;
}

geBoolean LoadEntity(geVFile *VFile);

int32	NumSolidBrushes;
int32	NumCutBrushes;
int32	NumHollowCutBrushes;
int32	NumDetailBrushes;
int32	NumTotalBrushes;

//================================================================================
//	CopyBrush2ToEntity
//================================================================================
geBoolean CopyBrush2ToEntity(MAP_Brush *Brush, MAP_Entity *Entity, int32 Contents)
{
	MAP_Brush	*NewBrush;
	int32		i;

	NewBrush = CopyMapBrush(Brush);

	NewBrush->OrderID = -1;

	for (i=0; i< NewBrush->NumSides; i++)
	{
		NewBrush->OriginalSides[i].Flags &= ~SIDE_VISIBLE;
	}

	// Assign the new contents
	NewBrush->Contents = Contents;

	// Append it to the beginning of the list...
	NewBrush->Next = Entity->Brushes2;
	Entity->Brushes2 = NewBrush;

	return GE_TRUE;
}

//================================================================================
//	LoadBrushFile
//================================================================================
geBoolean LoadBrushFile(char *FileName)
{
	MAP_BrushFileHeader	Header;
	int32				i;
	char				TempName[256];
	geVFile				*VFile;

	memset(Entities, 0, sizeof(Entities));

	GetCurrentDirectory(sizeof(TempName), TempName);
	
	VFile = geVFile_OpenNewSystem(NULL,
								   GE_VFILE_TYPE_DOS,
								   FileName,
								   NULL,
								   GE_VFILE_OPEN_READONLY);

	if (!VFile)
	{
		GHook.Error("LoadBrushFile:  Error opening Brush File: %s.\n", FileName);
		return GE_FALSE;
	}
	
	GHook.Printf(" --- Load Brush File ---\n");
	
	if (!geVFile_Read(VFile, &Header, sizeof(MAP_BrushFileHeader)))
	{
		geVFile_Close(VFile);
		GHook.Error("LoadBrushFile:  There was an error loading the header.\n");
		return GE_FALSE;
	}

	NumSolidBrushes = 0;
	NumCutBrushes = 0;
	NumHollowCutBrushes = 0;
	NumDetailBrushes = 0;
	NumTotalBrushes = 0;
	
	for (i=0; i< Header.NumEntities; i++)
	{
		if (!LoadEntity(VFile))
		{
			geVFile_Close(VFile);
			GHook.Error("LoadBrushFile:  Could not load entity.\n");
			return GE_FALSE;
		}
	}

	geVFile_Close(VFile);
	
	GHook.Printf("Num Solid Brushes      : %5i\n", NumSolidBrushes);
	GHook.Printf("Num Cut Brushes        : %5i\n", NumCutBrushes);
	GHook.Printf("Num Hollow Cut Brushes : %5i\n", NumHollowCutBrushes);
	GHook.Printf("Num Detail Brushes     : %5i\n", NumDetailBrushes);
	GHook.Printf("Num Total Brushes      : %5i\n", NumTotalBrushes);

	return GE_TRUE;
}

#define ENTITY_MODEL_MOTION			(1<<0)

//================================================================================
//	LoadMotion
//================================================================================
geBoolean LoadMotion(MAP_Entity *Entity, geVFile *VFile)
{
	Entity->Motion = geMotion_CreateFromFile(VFile);

	if (!Entity->Motion)
	{
		FILE		*f;

		f = fopen("Gedit.Log", "wb");

		if (f)
		{
			int32		i, NumErrors;
		
			fprintf(f, "     --Error Report--\n");

			NumErrors = geErrorLog_Count();

			for (i=0; i<NumErrors; i++)
			{
				geErrorLog_ErrorClassType	Error;
				const char					*String;

				if (geErrorLog_Report(NumErrors-i-1, &Error, &String))
				{
					fprintf(f, "Error#:%3i, Code#:%3i, Info:%s\n", NumErrors-i-1, Error, String);
				}
			}

			fclose(f);
		
		}
		else
			GHook.Error("Could not open gedit.log...\n");
		
		GHook.Error("LoadMotion:  Could not load motion.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//================================================================================
//	LoadEntity
//================================================================================
geBoolean LoadEntity(geVFile *VFile)
{
	int32		NumBrushes, NumFields, KeySize, ValueSize;
	MAP_Epair	*Epair, *Current;
	MAP_Entity	*Entity;
	int32		i;
	uint32		EntityFlags;
	//char		*Value;

	if (NumEntities >= MAX_MAP_ENTITIES)
	{
		GHook.Error("LoadEntity:  Max Entities.\n");
		return GE_FALSE;
	}

	Entity = &Entities[NumEntities++];

	memset(Entity, 0, sizeof(MAP_Entity));
	
	if (!geVFile_Read(VFile, &NumBrushes, sizeof(int32)))
	{
		GHook.Error("LoadEntity:  There was an error reading data.\n");
		return GE_FALSE;
	}

	for (i=0; i< NumBrushes; i++)
	{
		MAP_Brush *MBrush2 = LoadMapBrush(VFile);
		if (!MBrush2)
		{
			GHook.Error("LoadEntity:  Could not load entities brush.\n");
			return GE_FALSE;
		}
		MBrush2->OrderID = i;

		// Put at the beginning of entities brush list...
		MBrush2->Next = Entity->Brushes2;
		Entity->Brushes2 = MBrush2;
	}

	if (NumEntities > 1)	// For now, only models can contribute area brushes to the world
	{
		MAP_Brush	*Brush;

		for (Brush = Entity->Brushes2; Brush; Brush = Brush->Next)
		{
			if (!(Brush->Contents & BSP_CONTENTS_AREA2))
				continue;		// Only copy area brushes

			CopyBrush2ToEntity(Brush, &Entities[0], BSP_CONTENTS_AREA2 | BSP_CONTENTS_DETAIL2);
		}
	}
	
	
	if (!geVFile_Read(VFile, &EntityFlags, sizeof(uint32)))
	{
		GHook.Error("LoadEntity:  There was an error reading entity flags.\n");
		return GE_FALSE;
	}
	
	// Check for motion data for model...
	if (EntityFlags == ENTITY_MODEL_MOTION)
	{
		if (!LoadMotion(Entity, VFile))
		{
			GHook.Error("LoadEntity:  Failed to load motion data for model.\n");
			return GE_FALSE;
		}
	}
	

	if (!geVFile_Read(VFile, &NumFields, sizeof(int32)))
	{
		GHook.Error("LoadEntity:  There was an error reading num fields.\n");
		return GE_FALSE;
	}

	Current = NULL;

	for (i=0; i< NumFields; i++)
	{
		Epair = GE_RAM_ALLOCATE_STRUCT(MAP_Epair);

		if (!geVFile_Read(VFile, &KeySize, sizeof(int32)))
		{
			geRam_Free(Epair);
			GHook.Error("LoadEntity:  There was an error reading key size.\n");
			return GE_FALSE;
		}
		
		Epair->Key = GE_RAM_ALLOCATE_ARRAY(char,KeySize+1);

		if (!geVFile_Read(VFile, Epair->Key, sizeof(char)*KeySize))
		{
			geRam_Free(Epair->Key);
			GHook.Error("LoadEntity:  There was an error reading key data.\n");
			return GE_FALSE;
		}
		Epair->Key[KeySize] = 0;

		if (!geVFile_Read(VFile, &ValueSize, sizeof(int32)))
		{
			geRam_Free(Epair->Key);
			GHook.Error("LoadEntity:  There was an error reading value size.\n");
			return GE_FALSE;
		}
		
		Epair->Value = GE_RAM_ALLOCATE_ARRAY(char,ValueSize+1);

		if (!geVFile_Read(VFile, Epair->Value, sizeof(char)*ValueSize))
		{
			geRam_Free(Epair->Value);
			geRam_Free(Epair->Key);
			GHook.Error("LoadEntity:  There was an error reading value data.\n");
			return GE_FALSE;
		}

		Epair->Value[ValueSize] = 0;

		// Un-Comment for debugging
		//GHook.Printf("\"%s\" \"%s\"\n", Epair->Key, Epair->Value);

		Epair->Next = NULL;

		if (!Current)
		{
			Entity->Epairs = Epair;
			Current = Epair;
		}
		else
		{
			Current->Next = Epair;
			Current = Epair;
		}
	}

	if (GetVectorForKey2(Entity, "origin", &Entity->Origin))
		Entity->Flags |= ENTITY_HAS_ORIGIN;

	for (Epair = Entity->Epairs; Epair; Epair = Epair->Next)
	{
		//if (Epair->Value[0] == '%')
		//	Entity->Flags &= ~ENTITY_HAS_ORIGIN;		// Remove the origin flag

		// Remove origin flag on typedefs and model entities.
		if (!stricmp(Epair->Value, "%typedef%"))
			Entity->Flags &= ~ENTITY_HAS_ORIGIN;
		else if ((stricmp (Epair->Key, "classname") == 0) &&
				 (stricmp (Epair->Value, "%Model%") == 0))
		{
			Entity->Flags &= ~ENTITY_HAS_ORIGIN;
		}
	}

	return GE_TRUE;
}

