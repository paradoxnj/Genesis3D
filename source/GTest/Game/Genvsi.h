/****************************************************************************************/
/*  GenVSI.h                                                                            */
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
#ifndef GENVSI_H
#define GENVSI_H

#include "GENESIS.H"	// FIXME:  Should get rid of this, and make wrapper functions

//===========================================================================
//===========================================================================
typedef int32					GenVSI_CHandle;		// Client Handle
typedef int32					GenVSI_PHandle;		// Player handle
#define CLIENT_NULL_HANDLE		-1

//===========================================================================
//===========================================================================
typedef enum
{
	MODE_Server,									// Genuine Server mode
	MODE_SmartClient,								// Smart "simulation" Client mode
	MODE_DumbClient,								// Dumb client terminal (pure interpolation)
	MOVE_ServerClient								// Should do NOTHING, on the same machine as server...
} GenVSI_Mode;

typedef struct
{
	float				MoveTime;
	float				ForwardSpeed;				// Forward/Back speed
	geVec3d				Angles;						// Orientation
	uint16				ButtonBits;					// Buttons currently pressed for this frame

	uint16				Weapon;

	geVec3d				Pos;						// Where the client currently thinks he is...

	float				PingTime;					
} GenVSI_CMove;

//===========================================================================
//	Button flags
//===========================================================================
//#pragma todo ("Change the nameing convention here.")

#define HOST_BUTTON_JUMP				(1<<0)
#define HOST_BUTTON_LEFT				(1<<1)
#define HOST_BUTTON_RIGHT				(1<<2)
#define HOST_BUTTON_FIRE				(1<<3)

//===========================================================================
// View Index flags
//===========================================================================

// Pre-defined no view index
#define VIEW_INDEX_NONE					0xffff

// ViewFlags for player
#define VIEW_TYPE_NONE					(1<<0)
//#define VIEW_TYPE_MESH					(1<<1)
#define VIEW_TYPE_ACTOR					(1<<2)
#define VIEW_TYPE_MODEL					(1<<3)
#define VIEW_TYPE_MODEL_OPEN			(1<<4)
#define VIEW_TYPE_LIGHT					(1<<5)
#define VIEW_TYPE_SPRITE				(1<<6)
#define VIEW_TYPE_YAW_ONLY				(1<<7)			// Used for viewplayers/clients, etc..
#define VIEW_TYPE_LOCAL					(1<<8)			// On local server only currently
#define VIEW_TYPE_FORCE_XFORM			(1<<10)			// Do not use clients angles, bbut xform instead (good for when dead so roll will be in)
#define VIEW_TYPE_TOUCH					(1<<11)
#define VIEW_TYPE_STANDON				(1<<12)
#define VIEW_TYPE_COLLIDEMODEL			(1<<13)			// Collide models against this item
#define	VIEW_TYPE_PHYSOB				(1<<14)
#define	VIEW_TYPE_HACK					(1<<15)

typedef struct GenVSI		GenVSI;

// Callback defines
typedef geBoolean GenVSI_NewWorldCB(GenVSI *VSI, const char *WorldName, geVFile *MainFS);
typedef geBoolean GenVSI_ShutdownWorldCB(GenVSI *VSI);

typedef void GenVSI_SpawnFunc(GenVSI *VSI, void *Player, void *ClassData, char *EntityName);
typedef void GenVSI_DestroyFunc(GenVSI *VSI, void *Player, void *ClassData);

typedef void GenVSI_SetCSpawnCB(void *UData, const char *ClassName, GenVSI_SpawnFunc *Func, GenVSI_DestroyFunc *DFunc); 
typedef void GenVSI_SetWorldCB(void *UData, GenVSI_NewWorldCB *NewWorldCB, GenVSI_ShutdownWorldCB *ShutdownWorldCB, const char *WorldName);
typedef void GenVSI_ActorIndexCB(void *UData, uint16 Index, const char *GFXPath, const char *FileName);
typedef void GenVSI_MotionIndexCB(void *UData, uint16 Index, const char *MotionName);
typedef void GenVSI_BoneIndexCB(void *UData, uint16 Index, const char *BoneName);
typedef void GenVSI_TextureIndexCB(void *UData, uint16 Index, const char *TextureName, const char *ATextureName);
typedef void GenVSI_SoundIndexCB(void *UData, uint16 Index, const char *FileName);

typedef void GenVSI_PlaySoundCB(void *UData, uint16 Index, const geVec3d *Pos);
typedef void GenVSI_SpawnFxCB(void *UData, uint8 FxNum, const geVec3d *Pos, uint8 SoundIndex);

typedef void *GenVSI_GetPClientDataCB(void *UData, GenVSI_CHandle ClientHandle);
typedef void GenVSI_SetClientValue(void *UData, GenVSI_CHandle ClientHandle, int32 Value);
typedef GenVSI_CMove *GenVSI_GetClientMoveCB(void *UData, GenVSI_CHandle ClientHandle);
typedef geBoolean GenVSI_IsClientBotCB(void *UData, GenVSI_CHandle ClientHandle);
typedef geBoolean GenVSI_ClientDisconnectCB(void *UData, GenVSI_CHandle ClientHandle);

typedef uint16 GenVSI_ModelToVIndexCB(void *UData, geWorld_Model *Model);
typedef void GenVSI_RegisterPModelCB(void *UData, void *PlayerData, geWorld_Model *Model);
typedef void *GenVSI_GetNextPlayerCB(void *UData, void *Start, const char *ClassName);
typedef void *GenVSI_GetNextPlayerRCB(void *UData, void *PlayerData, void *Start, const char *ClassName, float Radius);
typedef void GenVSI_SetViewPlayerCB(void *UData, GenVSI_CHandle ClientHandle, void *PlayerData);
typedef geBoolean GenVSI_MovePModelCB(void *UData, void *PlayerData, const geXForm3d *DestXForm);
typedef void GenVSI_GetPlayerTExtCB(void *UData, void *PlayerData, uint16 MotionIndex, float *Start, float *End);
typedef void GenVSI_SetCInvCB(void *UData, GenVSI_CHandle ClientHandle, int32 Slot, uint16 Amount, geBoolean HasItem);
typedef void *GenVSI_ActorToPlayerCB(void *UData, geActor *Actor);

typedef geWorld *GenVSI_GetWorldCB(void *UData);
typedef void GenVSI_ConsolePrintfCB(void *Udata, const char *Str, ...);
typedef void GenVSI_ConsoleHeaderPrintfCB(void *Udata, int32 ClientHandle, geBoolean AllClients, const char *Str, ...);
typedef float GenVSI_GetTimeCB(void *UData);

//#pragma todo ("all this player stuff will soon be handles, not void...")
typedef void *GenVSI_SpawnPlayerCB(void *UData, const char *ClassName);
typedef void GenVSI_DestroyPlayerCB(void *UData, void *PlayerData);


typedef void GenVSI_ProcIndexCB(void *UData, uint16 Index, void *Proc);

typedef struct GenVSI
{
	GenVSI_Mode				Mode;

	geWorld					*World;				// Current world (should be valid on Clients and Servers)

	// Callbacks set by owner of GenVSI object
	// NULL callbacks should just be ignored at run-time
	GenVSI_SetCSpawnCB		*SetClassSpawn;
	GenVSI_SetWorldCB		*SetWorld;
	GenVSI_ActorIndexCB		*ActorIndex;
	GenVSI_MotionIndexCB	*MotionIndex;
	GenVSI_BoneIndexCB		*BoneIndex;
	GenVSI_TextureIndexCB	*TextureIndex;
	GenVSI_SoundIndexCB		*SoundIndex;

	// Candy functions
	GenVSI_PlaySoundCB		*PlaySound;
	GenVSI_SpawnFxCB		*SpawnFx;

	// Built in for now, BUT SHOULD CHANGE!!!!!
	GenVSI_GetPClientDataCB	*GetPlayerClientData;
	GenVSI_SetClientValue	*SetClientScore;
	GenVSI_SetClientValue	*SetClientHealth;
	GenVSI_SetClientValue	*SetClientWeapon;
	GenVSI_SetCInvCB		*SetClientInventory;
	GenVSI_GetClientMoveCB	*GetClientMove;
	GenVSI_IsClientBotCB    *IsClientBot;
	GenVSI_ClientDisconnectCB    *ClientDisconnect;

	// Support functions
	GenVSI_ModelToVIndexCB	*ModelToViewIndex;
	GenVSI_RegisterPModelCB *RegisterPlayerModel;
	GenVSI_GetNextPlayerCB	*GetNextPlayer;
	GenVSI_GetNextPlayerRCB	*GetNextPlayerInRadius;
	GenVSI_SetViewPlayerCB	*SetViewPlayer;
	GenVSI_MovePModelCB		*MovePlayerModel;
	GenVSI_GetPlayerTExtCB  *GetPlayerTimeExtents;
	GenVSI_ActorToPlayerCB	*ActorToPlayer;
	GenVSI_GetWorldCB		*GetWorld;
	GenVSI_ConsolePrintfCB	*ConsolePrintf;
	GenVSI_ConsoleHeaderPrintfCB	*ConsoleHeaderPrintf;
	GenVSI_GetTimeCB		*GetTime;

	GenVSI_SpawnPlayerCB	*SpawnPlayer;
	GenVSI_DestroyPlayerCB	*DestroyPlayer;

	GenVSI_ProcIndexCB		*ProcIndex;
	
	void					*UData;

} GenVSI;


// Loading/Prep functions (Authoritive Server side only)
void GenVSI_SetClassSpawn(GenVSI *VSI, const char *ClassName, GenVSI_SpawnFunc *Func, GenVSI_DestroyFunc *DFunc); 
void GenVSI_SetWorld(GenVSI *VSI, GenVSI_NewWorldCB *NewWorldCB, GenVSI_ShutdownWorldCB *ShutdownWorldCB, const char *WorldName);
void GenVSI_ActorIndex(GenVSI *VSI, uint16 Index, const char *GFXPath, const char *FileName);
void GenVSI_MotionIndex(GenVSI *VSI, uint16 Index, const char *MotionName);
void GenVSI_BoneIndex(GenVSI *VSI, uint16 Index, const char *BoneName);
void GenVSI_TextureIndex(GenVSI *VSI, uint16 Index, const char *TextureName, const char *ATextureName);
void GenVSI_SoundIndex(GenVSI *VSI, uint16 Index, const char *FileName);

// Candy functions (Client and server should be able to call these)
void GenVSI_PlaySound(GenVSI *VSI, uint16 Index, const geVec3d *Pos);
void GenVSI_SpawnFx(GenVSI *VSI, uint8 FxNum, const geVec3d *Pos, uint8 SoundIndex);

// Collision
//void GenVSI_RayCollision(GenVSI *VSI, geVec3d *Front, geVec3d *Back,...)	// FIXME:  Implement...

// Stray (don't know what to do with yet) functions
// These are currently built in, and will be soon be converted to the new net-variable
// interface system.  I.e:  NetUseVariable("ClientScore"), etc...
void *GenVSI_GetPlayerClientData(GenVSI *VSI, GenVSI_CHandle ClientHandle);
void GenVSI_SetClientScore(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Score);
void GenVSI_SetClientHealth(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Health);
void GenVSI_SetClientWeapon(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Weapon);
void GenVSI_SetClientInventory(GenVSI *VSI, GenVSI_CHandle ClientHandle, int32 Slot, uint16 Amount, geBoolean HasItem);
GenVSI_CMove *GenVSI_GetClientMove(GenVSI *VSI, GenVSI_CHandle ClientHandle);
geBoolean GenVSI_IsClientBot(GenVSI *VSI, GenVSI_CHandle ClientHandle);
geBoolean GenVSI_ClientDisconnect(GenVSI *VSI, GenVSI_CHandle ClientHandle);

// Support functions
void *GenVSI_GetNextPlayer(GenVSI *VSI, void *Start, const char *ClassName);
void *GenVSI_GetNextPlayerInRadius(GenVSI *VSI, void *PlayerData, void *Start, const char *ClassName, float Radius);
void GenVSI_SetViewPlayer(GenVSI *VSI, GenVSI_CHandle ClientHandle, void *PlayerData);
uint16 GenVSI_ModelToViewIndex(GenVSI *VSI, geWorld_Model *Model);
void GenVSI_RegisterPlayerModel(GenVSI *VSI, void *PlayerData, geWorld_Model *Model);
geBoolean GenVSI_MovePlayerModel(GenVSI *VSI, void *PlayerData, const geXForm3d *DestXForm);
void GenVSI_GetPlayerTimeExtents(GenVSI *VSI, void *PlayerData, uint16 MotionIndex, float *Start, float *End);
void *GenVSI_ActorToPlayer(GenVSI *VSI, geActor *Actor);

geWorld *GenVSI_GetWorld(GenVSI *VSI);
void *GenVSI_GetPlayerData(GenVSI *VSI, GenVSI_PHandle PlayerHandle);
void GenVSI_ConsolePrintf(GenVSI *VSI, const char *Str, ...);
void GenVSI_ConsoleHeaderPrintf(GenVSI *VSI, int32 ClientHandle, geBoolean AllClients, const char *Str, ...);
float GenVSI_GetTime(GenVSI *VSI);

void *GenVSI_SpawnPlayer(GenVSI *VSI, const char *ClassName);
void GenVSI_DestroyPlayer(GenVSI *VSI, void *PlayerData);

void GenVSI_ProcIndex(GenVSI *VSI, uint16 Index, void *Proc);

#endif