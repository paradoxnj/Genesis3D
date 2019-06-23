/****************************************************************************************/
/*  GenVSI.c                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description:                                                                        */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <Assert.h>
#include <stdio.h>

#include "GenVSI.h"

#include "Genesis.h"


//=====================================================================================
// Loading/Prep functions (Authoritive Server side ONLY)
//=====================================================================================

//=====================================================================================
//	GenVSI_SetClassSpawn
//=====================================================================================
void GenVSI_SetClassSpawn(GenVSI *VSI, const char *ClassName, GenVSI_SpawnFunc *Func, GenVSI_DestroyFunc *DFunc)
{
	if (!VSI->SetClassSpawn)
		return;

	VSI->SetClassSpawn(VSI->UData, ClassName, Func, DFunc);
}

//=====================================================================================
//	GenVSI_SetWorld
//=====================================================================================
void GenVSI_SetWorld(GenVSI *VSI, GenVSI_NewWorldCB *NewWorldCB, GenVSI_ShutdownWorldCB *ShutdownWorldCB, const char *WorldName)
{
	if (!VSI->SetWorld)
		return;

	VSI->SetWorld(VSI->UData, NewWorldCB, ShutdownWorldCB, WorldName);
}

//=====================================================================================
//	GenVSI_ActorIndex
//=====================================================================================
void GenVSI_ActorIndex(GenVSI *VSI, uint16 Index, const char *GFXPath, const char *FileName)
{
	if (!VSI->ActorIndex)
		return;

	VSI->ActorIndex(VSI->UData, Index, GFXPath, FileName);
}

//=====================================================================================
//	GenVSI_MotionIndex
//=====================================================================================
void GenVSI_MotionIndex(GenVSI *VSI, uint16 Index, const char *MotionName)
{
	if (!VSI->MotionIndex)
		return;

	VSI->MotionIndex(VSI->UData, Index, MotionName);
}

//=====================================================================================
//	GenVSI_BoneIndex
//=====================================================================================
void GenVSI_BoneIndex(GenVSI *VSI, uint16 Index, const char *BoneName)
{
	if (!VSI->BoneIndex)
		return;

	VSI->BoneIndex(VSI->UData, Index, BoneName);
}

//=====================================================================================
//	GenVSI_TextureIndex
//=====================================================================================
void GenVSI_TextureIndex(GenVSI *VSI, uint16 Index, const char *TextureName, const char *ATextureName)
{
	if (!VSI->TextureIndex)
		return;

	VSI->TextureIndex(VSI->UData, Index, TextureName, ATextureName);
}

//=====================================================================================
//	GenVSI_SoundIndex
//=====================================================================================
void GenVSI_SoundIndex(GenVSI *VSI, uint16 Index, const char *FileName)
{
	if (!VSI->SoundIndex)
		return;

	VSI->SoundIndex(VSI->UData, Index, FileName);
}

//=====================================================================================
// Candy functions (Client and server should be able to call these)
//=====================================================================================

//=====================================================================================
//	GenVSI_PlaySound
//=====================================================================================
void GenVSI_PlaySound(GenVSI *VSI, uint16 Index, const geVec3d *Pos)
{
	if (!VSI->PlaySound)
		return;

	VSI->PlaySound(VSI->UData, Index, Pos);
}

//=====================================================================================
//	GenVSI_SpawnFx
//=====================================================================================
void GenVSI_SpawnFx(GenVSI *VSI, uint8 FxNum, const geVec3d *Pos, uint8 SoundIndex)
{
	if (!VSI->SpawnFx)
		return;

	VSI->SpawnFx(VSI->UData, FxNum, Pos, SoundIndex);
}

//=====================================================================================
// Collision
//=====================================================================================
//void GenVSI_RayCollision(GenVSI *VSI, geVec3d *Front, geVec3d *Back,...)	// FIXME:  Implement...

//=====================================================================================
// Stray (don't know what to do with yet) functions
// These are currently built in, and will be soon be converted to the new net-variable
// interface system.  I.e:  NetUseVariable("ClientScore"), etc...
//=====================================================================================

//=====================================================================================
//	GenVSI_GetPlayerClientData
//=====================================================================================
void *GenVSI_GetPlayerClientData(GenVSI *VSI, GenVSI_CHandle ClientHandle)
{
	if (!VSI->GetPlayerClientData)
		return NULL;

	return VSI->GetPlayerClientData(VSI->UData, ClientHandle);
}

//=====================================================================================
//	GenVSI_SetClientScore
//=====================================================================================
void GenVSI_SetClientScore(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Score)
{
	if (!VSI->SetClientScore)
		return;

	VSI->SetClientScore(VSI->UData, ClientHandle, Score);
}

//=====================================================================================
//	GenVSI_SetClientHealth
//=====================================================================================
void GenVSI_SetClientHealth(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Health)
{
	if (!VSI->SetClientHealth)
		return;

	VSI->SetClientHealth(VSI->UData, ClientHandle, Health);
}

//=====================================================================================
//	GenVSI_SetClientWeapon
//=====================================================================================
void GenVSI_SetClientWeapon(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Weapon)
{
	if (!VSI->SetClientWeapon)
		return;

	VSI->SetClientWeapon(VSI->UData, ClientHandle, Weapon);
}

//=====================================================================================
//	GenVSI_SetClientInventory
//=====================================================================================
void GenVSI_SetClientInventory(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Slot, uint16 Amount, geBoolean HasItem)
{
	if (!VSI->SetClientInventory)
		return;

	VSI->SetClientInventory(VSI->UData, ClientHandle, Slot, Amount, HasItem);
}

//=====================================================================================
//	GenVSI_IsClientBot
//=====================================================================================
#pragma message ("Bot - Genvsi.c")
geBoolean GenVSI_IsClientBot(GenVSI *VSI, GenVSI_CHandle ClientHandle)
{
	if (!VSI->IsClientBot)
		return GE_FALSE;

	return VSI->IsClientBot(VSI->UData, ClientHandle);
}

//=====================================================================================
//	GenVSI_ClientDisconnect
//=====================================================================================
geBoolean GenVSI_ClientDisconnect(GenVSI *VSI, GenVSI_CHandle ClientHandle)
{
	if (!VSI->ClientDisconnect)
		return GE_FALSE;

	return VSI->ClientDisconnect(VSI->UData, ClientHandle);
}

//=====================================================================================
//	GenVSI_GetClientMove
//=====================================================================================
GenVSI_CMove *GenVSI_GetClientMove(GenVSI *VSI, GenVSI_CHandle ClientHandle)
{
	if (!VSI->GetClientMove)
		return NULL;

	return VSI->GetClientMove(VSI->UData, ClientHandle);
}

// Support functions
//=====================================================================================
//	GenVSI_ModelToViewIndex
//=====================================================================================
uint16 GenVSI_ModelToViewIndex(GenVSI *VSI, geWorld_Model *Model)
{
	if (!VSI->ModelToViewIndex)
	{
		assert(!"Should'nt be here!!!");
		return 0xffff;
	}

	return VSI->ModelToViewIndex(VSI->UData, Model);
}

//=====================================================================================
//	GenVSI_RegisterPlayerModel
//=====================================================================================
void GenVSI_RegisterPlayerModel(GenVSI *VSI, void *PlayerData, geWorld_Model *Model)
{
	if (!VSI->RegisterPlayerModel)
		return;

	VSI->RegisterPlayerModel(VSI->UData, PlayerData, Model);
}

//=====================================================================================
//	GenVSI_GetNextPlayer
//=====================================================================================
void *GenVSI_GetNextPlayer(GenVSI *VSI, void *Start, const char *ClassName)
{
	if (!VSI->GetNextPlayer)
		return NULL;

	return VSI->GetNextPlayer(VSI->UData, Start, ClassName);
}

//=====================================================================================
//	GenVSI_GetNextPlayerInRadius
//=====================================================================================
void *GenVSI_GetNextPlayerInRadius(GenVSI *VSI, void *PlayerData, void *Start, const char *ClassName, float Radius)
{
	if (!VSI->GetNextPlayerInRadius)
		return NULL;

	return VSI->GetNextPlayerInRadius(VSI->UData, PlayerData, Start, ClassName, Radius);
}

//=====================================================================================
//	GenVSI_SetViewPlayer
//=====================================================================================
void GenVSI_SetViewPlayer(GenVSI *VSI, GenVSI_CHandle ClientHandle, void *PlayerData)
{
	if (!VSI->SetViewPlayer)
		return;

	VSI->SetViewPlayer(VSI->UData, ClientHandle, PlayerData);
}

//=====================================================================================
//	GenVSI_MovePlayerModel
//=====================================================================================
geBoolean GenVSI_MovePlayerModel(GenVSI *VSI, void *PlayerData, const geXForm3d *DestXForm)
{
	if (!VSI->MovePlayerModel)
		return GE_FALSE;

	return VSI->MovePlayerModel(VSI->UData, PlayerData, DestXForm);
}

//=====================================================================================
//	GenVSI_GetPlayerTimeExtents
//=====================================================================================
void GenVSI_GetPlayerTimeExtents(GenVSI *VSI, void *PlayerData, uint16 MotionIndex, float *Start, float *End)
{
	if (!VSI->GetPlayerTimeExtents)
		return;

	VSI->GetPlayerTimeExtents(VSI->UData, PlayerData, MotionIndex, Start, End);
}

//=====================================================================================
//	GenVSI_ActorToPlayer
//=====================================================================================
void *GenVSI_ActorToPlayer(GenVSI *VSI, geActor *Actor)
{
	if (!VSI->ActorToPlayer)
		return NULL;

	return VSI->ActorToPlayer(VSI->UData, Actor);
}

//=====================================================================================
//	GenVSI_GetWorld
//=====================================================================================
geWorld *GenVSI_GetWorld(GenVSI *VSI)
{
	if (!VSI->GetWorld)
		return NULL;

	return VSI->GetWorld(VSI->UData);
}

//=====================================================================================
//	GenVSI_ConsolePrintf
//=====================================================================================
void GenVSI_ConsolePrintf(GenVSI *VSI, const char *Str, ...)
{
	va_list		ArgPtr;

	if (!VSI->ConsolePrintf)
		return;

	assert(VSI);
	assert(Str);

	va_start (ArgPtr, Str);
    VSI->ConsolePrintf(VSI->UData, Str, ArgPtr);
	va_end (ArgPtr);
}

//=====================================================================================
//	GenVSI_ConsoleHeaderPrintf
//=====================================================================================
void GenVSI_ConsoleHeaderPrintf(GenVSI *VSI, int32 ClientHandle, geBoolean AllClients, const char *Str, ...)
{
	va_list		ArgPtr;
    char		TempStr[256];

	if (!VSI->ConsoleHeaderPrintf)
		return;

	assert(VSI);
	assert(Str);

	va_start (ArgPtr, Str);
    vsprintf (TempStr, Str, ArgPtr);
	va_end (ArgPtr);

    VSI->ConsoleHeaderPrintf(VSI->UData, ClientHandle, AllClients, TempStr);
}

//=====================================================================================
//	GenVSI_GetTime
//=====================================================================================
float GenVSI_GetTime(GenVSI *VSI)
{
	if (!VSI->GetTime)
		return 0.0f;

	return VSI->GetTime(VSI->UData);
}

//=====================================================================================
//	GenVSI_SpawnPlayer
//=====================================================================================
void *GenVSI_SpawnPlayer(GenVSI *VSI, const char *ClassName)
{
	if (!VSI->SpawnPlayer)
		return NULL;

	return VSI->SpawnPlayer(VSI->UData, ClassName);
}

//=====================================================================================
//	GenVSI_DestroyPlayer
//=====================================================================================
void GenVSI_DestroyPlayer(GenVSI *VSI, void *PlayerData)
{
	if (!VSI->DestroyPlayer)
	{
		assert(!"Bad! No destroy!!!");
		return;
	}
	
	VSI->DestroyPlayer(VSI->UData, PlayerData);
}


//=====================================================================================
//	GenVSI_ProcIndex
//=====================================================================================
void GenVSI_ProcIndex(GenVSI *VSI, uint16 Index, void *Proc)
{
	if (!VSI->ProcIndex)
		return;

	VSI->ProcIndex(VSI->UData, Index, Proc);
}
