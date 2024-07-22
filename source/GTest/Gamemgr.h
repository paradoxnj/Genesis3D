/****************************************************************************************/
/*  GameMgr.h                                                                           */
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
#ifndef GAMEMGR_H
#define GAMEMGR_H

#include <Windows.h>

#include "GENESIS.H"
#include "Console.h"

// Effects
#include "Fx\\Fx.h"
#include "Fx\\Corona.h"
#include "Fx\\Electric.h"
#include "Fx\\DynLIGHT.H"
#include "Fx\\ModelCtl.h"

#include "VidMode.h"



#ifdef __cplusplus
extern "C" {
#endif

// Upper bounds
#define GAMEMGR_MAX_WORLDS			1

#define GAMEMGR_MAX_FILENAME		128

#define	GAMEMGR_MAX_MESH_INDEX		128
#define	GAMEMGR_MAX_ACTOR_INDEX		128
#define	GAMEMGR_MAX_MOTION_INDEX	128
#define	GAMEMGR_MAX_BONE_INDEX		128
#define GAMEMGR_MAX_TEXTURE_INDEX	128
#define	GAMEMGR_MAX_SOUND_INDEX		128

#define GAMEMGR_MAX_MODELS			256

#define GAMEMGR_MAX_WORLD_NAME		256

#define	GAMEMGR_MAX_GFX_PATH		64
#define	GAMEMGR_MAX_MESH_NAME		64
#define	GAMEMGR_MAX_ACTOR_NAME		64
#define	GAMEMGR_MAX_MOTION_NAME		32
#define	GAMEMGR_MAX_BONE_NAME		32

//====================================================================================
// The GameMgr typedef
//====================================================================================
typedef struct	GameMgr		GameMgr;

//====================================================================================
//	Misc defs
//====================================================================================
typedef int32	GameMgr_MotionIndex;

typedef geBoolean GameMgr_SetWorldCB(GameMgr *GMgr, const char *WorldName, void *Context);
void GameMgr_ResetMainWindow(HWND hWnd, int32 Width, int32 Height);

typedef enum
{
	FrameState_None = 0,
	FrameState_Begin
} GameMgr_FrameState;

//
//	Default presets
//
#define GAMEMGR_MOTION_INDEX_NONE	255

//====================================================================================
//	Structure defs
//====================================================================================

// Index data
typedef struct
{
	geBoolean		Active;
	
	char			FileName[GAMEMGR_MAX_ACTOR_NAME];

	geActor_Def		*ActorDef;
	geActor			*ActorHack;	// Actor is loaded for def, so textures will remain loaded...
} GameMgr_ActorIndex;

typedef struct
{
	geBoolean			Active;
	
	char				FileName[GAMEMGR_MAX_FILENAME];
	char				AFileName[GAMEMGR_MAX_FILENAME];

	geBitmap			*TextureDef;

} GameMgr_TextureIndex;

typedef struct
{
	geBoolean		Active;
	char			MotionName[GAMEMGR_MAX_MOTION_NAME];
} GameMgr_MotionIndexDef;

typedef struct
{
	geBoolean		Active;
	char			BoneName[GAMEMGR_MAX_BONE_NAME];
} GameMgr_BoneIndex;

typedef struct
{
	geBoolean		Active;
	char			FileName[GAMEMGR_MAX_FILENAME];
	geSound_Def		*SoundDef;
} GameMgr_SoundIndex;

//====================================================================================
// Function prototypes
//====================================================================================
// Create/Destroy management
GameMgr				*GameMgr_Create(HINSTANCE hInstance, int32 Width, int32 Height, const char *AppName);
void				GameMgr_Destroy(GameMgr *GMgr);

geBoolean			GameMgr_Frame(GameMgr *GMgr, float Time);

// Sanity checking
geBoolean			GameMgr_IsValid(GameMgr *GMgr);

// Driver management
void				GameMgr_PrepareToChangeMode(GameMgr *GMgr);
geBoolean			GameMgr_SetDriverAndMode(GameMgr *GMgr, geDriver *Driver, geDriver_Mode *DriverMode, int Width, int Height);

// Frame management
geBoolean			GameMgr_BeginFrame(GameMgr *GMgr, geWorld *World, geBoolean ClearScreen);
geBoolean			GameMgr_EndFrame(GameMgr *GMgr);

void				GameMgr_ConsolePrintf(GameMgr *GMgr, geBoolean DrawNow, const char *Str, ...);
geBoolean			GameMgr_ClearBackground(GameMgr *GMgr, int32 x, int32 y, const char *Str);

geBoolean				GameMgr_FreeWorld(GameMgr *GMgr);
geBoolean				GameMgr_SetWorld(GameMgr *GMgr, const char *WorldName);
geBoolean				GameMgr_SetActorIndex(GameMgr *GMgr, int32 Index, const char *FileName);
GameMgr_ActorIndex		*GameMgr_GetActorIndex(GameMgr *GMgr, int32 Index);
geBoolean				GameMgr_MotionIndexIsValid(GameMgr *GMgr, GameMgr_MotionIndex Index);
geBoolean				GameMgr_SetMotionIndexDef(GameMgr *GMgr, GameMgr_MotionIndex MotionIndex, const char *MotionName);
GameMgr_MotionIndexDef	*GameMgr_GetMotionIndexDef(GameMgr *GMgr, GameMgr_MotionIndex Index);
geBoolean				GameMgr_SetBoneIndex(GameMgr *GMgr, int32 BoneIndex, const char *BoneName);
GameMgr_BoneIndex		*GameMgr_GetBoneIndex(GameMgr *GMgr, int32 Index);
geBoolean				GameMgr_SetTextureIndex(GameMgr *GMgr, int32 Index, const char *FileName, const char *AFileName);
GameMgr_TextureIndex	*GameMgr_GetTextureIndex(GameMgr *GMgr, int32 Index);
geBoolean				GameMgr_SetSoundIndex(GameMgr *GMgr, int32 SoundIndex, const char *FileName);
GameMgr_SoundIndex		*GameMgr_GetSoundIndex(GameMgr *GMgr, int32 Index);

// Accessor functions
geEngine			*GameMgr_GetEngine(GameMgr *GMgr);
geSound_System		*GameMgr_GetSoundSystem(GameMgr *GMgr);
Console_Console		*GameMgr_GetConsole(GameMgr *GMgr);
geWorld				*GameMgr_GetWorld(GameMgr *GMgr);
geCamera			*GameMgr_GetCamera(GameMgr *GMgr);
GameMgr_FrameState	GameMgr_GetFrameState(GameMgr *GMgr);
int32				GameMgr_GetNumModels(GameMgr *GMgr);
geWorld_Model		*GameMgr_GetModel(GameMgr *GMgr, int32 Index);
Fx_System			*GameMgr_GetFxSystem(GameMgr *GMgr);
geBitmap			*GameMgr_GetShadowMap(GameMgr *GMgr);
float				GameMgr_GetTime(GameMgr *GMgr);
HWND				GameMgr_GethWnd(GameMgr *GMgr);
VidMode				GameMgr_GetVidMode(GameMgr *GMgr);

#ifdef __cplusplus
}
#endif

#endif