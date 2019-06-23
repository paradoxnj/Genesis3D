/****************************************************************************************/
/*  GameMgr.c                                                                           */
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
#include <direct.h>

#include "Ram.h"
#include "Errorlog.h"

#include "GameMgr.h"

#include "Procedurals\gebmutil.h"
#include "procedurals/ProcEng.h"


extern void	GenVS_Error(const char *Msg, ...);

extern geVFile		*MainFS;
extern geBoolean	g_FarClipPlaneEnable;

//====================================================================================
//	Misc defs
//====================================================================================
static HWND CreateMainWindow(HINSTANCE hInstance, const char *AppName, int32 Width, int32 Height);

//====================================================================================
//	Structure defs
//====================================================================================
typedef struct
{
	geWorld				*World;								// World pointer for this world data set
	char				WorldName[GAMEMGR_MAX_WORLD_NAME];

	int32				NumModels;
	geWorld_Model		*Models[GAMEMGR_MAX_MODELS];

} GameMgr_WorldInfo;

// The GameMgr object
typedef struct GameMgr
{
	GameMgr				*SelfCheck1;						// Head self valid check

	float				Time;

	// Objects the the game mgr maintains...
	geEngine			*Engine;							// Engine object
	geSound_System		*SoundSys;							// Soundsystem object
	Console_Console		*Console;

	HWND				hWnd;

	GameMgr_FrameState	FrameState;
	Fx_System			*FxSystem;
	geBitmap			*ShadowMap;


	GameMgr_WorldInfo	WorldInfo[GAMEMGR_MAX_WORLDS];

	// Mode specific stuff
	VidMode				VidMode;							// Current VidMode
	geCamera			*Camera;

	GameMgr_ActorIndex		ActorIndex[GAMEMGR_MAX_ACTOR_INDEX];
	GameMgr_MotionIndexDef	MotionIndexDefs[GAMEMGR_MAX_MOTION_INDEX];
	GameMgr_BoneIndex		BoneIndex[GAMEMGR_MAX_BONE_INDEX];
	GameMgr_TextureIndex	TextureIndex[GAMEMGR_MAX_TEXTURE_INDEX];
	GameMgr_SoundIndex		SoundIndex[GAMEMGR_MAX_SOUND_INDEX];

	ProcEng				*ProcEng;

	GameMgr				*SelfCheck2;						// Tail self valid check
} GameMgr;

//====================================================================================
//	GameMgr_FreeAllObjects
//====================================================================================
void GameMgr_FreeAllObjects(GameMgr *GMgr)
{
	assert(GMgr);

	if (GMgr->Camera)
		geCamera_Destroy(&GMgr->Camera);

	GameMgr_FreeWorld(GMgr);		// Make sure no old world is laying around
	
	if (GMgr->FxSystem)
		Fx_SystemDestroy(GMgr->FxSystem);

	if (GMgr->Console)
		Console_Destroy(GMgr->Console);

	if (GMgr->SoundSys)
		geSound_DestroySoundSystem(GMgr->SoundSys);

	if (GMgr->Engine)
		geEngine_Free(GMgr->Engine);

	GMgr->FxSystem	= NULL;
	GMgr->Engine	= NULL;
	GMgr->Console	= NULL;
	GMgr->SoundSys	= NULL;
	GMgr->Camera	= NULL;

	geRam_Free(GMgr);				// Free the manager itself...
}

//====================================================================================
//	GameMgr_Create
//====================================================================================
GameMgr *GameMgr_Create(HINSTANCE hInstance, int32 Width, int32 Height, const char *AppName)
{
	GameMgr	*GMgr;
	char	PathBuf[_MAX_PATH];

	assert(AppName);

	// Allocate the GameMgr structure
	GMgr = GE_RAM_ALLOCATE_STRUCT(GameMgr);

	if (!GMgr)
	{
		geErrorLog_AddString(-1, "GameMgr_Create:  Out of memory for GameMgr.", NULL);
		return NULL;
	}

	// Zero out memory
	memset(GMgr, 0, sizeof(GameMgr));

	// Save self check flags
	GMgr->SelfCheck1 = GMgr;
	GMgr->SelfCheck2 = GMgr;

	// Create the window
	GMgr->hWnd = CreateMainWindow(hInstance, AppName, Width, Height);
	
	// Create an engine object
	if (_getcwd(PathBuf,_MAX_PATH)==NULL)
		{
			geErrorLog_AddString(-1, "GameMgr_Create:  Could not get current working directory.", NULL);
			goto ExitWithError;
		}
	GMgr->Engine = geEngine_Create(GMgr->hWnd, AppName, PathBuf);
	
	if (!GMgr->Engine)
	{
		geErrorLog_AddString(-1, "GameMgr_Create:  Could not create the geEngine object.", NULL);
		goto ExitWithError;
	}

	#ifdef _DEBUG
	geXForm3d_SetMaximalAssertionMode(GE_FALSE);
	#endif
	
	//
	// Create the sound system
	//
	GMgr->SoundSys = geSound_CreateSoundSystem(GMgr->hWnd);

	if (!GMgr->SoundSys)
	{
		geErrorLog_AddString(-1, "GameMgr_Create:  Could not create the geSound_System object. (continuing)", NULL);
		//goto ExitWithError;
	}

	GMgr->ProcEng = NULL;

	return GMgr;

	ExitWithError:
	{
		if (GMgr)
			GameMgr_FreeAllObjects(GMgr);

		return NULL;
	}
}

//====================================================================================
//	GameMgr_Destroy
//====================================================================================
void GameMgr_Destroy(GameMgr *GMgr)
{
	assert(GMgr);
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	GameMgr_FreeAllObjects(GMgr);
}

/*
static	int32	WaterPosX = 2, WaterPosY = 64;
static	int32	NPage;
static	float	TimeToSplashWater = 0.0f;

static	uint8	OriginalBits[256*256];
static	int16	WaterData[2][256*256];

//static	uint8	Blend

void CalcRippleData(int16 *Src, int16 *Dest, int16 Density, int32 W, int32 H)
{
	int32	i,j;
	int16	Val;

	for(i=0; i< H; i++)
	{
		for(j=0; j< W; j++)
		{
			if (i > 0)							// Get top
				Val = *(Dest - W);
			else
				Val = *(Dest + W * (H-1));

			if (i < H-1)						// Get bottom
				Val += *(Dest + W);
			else
				Val += *(Dest - W * (H-1));

			if (j > 0)							// Get left
				Val += *(Dest - 1);
			else
				Val += *(Dest + (W-1));

			if (j < W-1)						// Get right
				Val += *(Dest + 1);
			else
				Val += *(Dest - (W-1));

			Val >>= 1;
			Val -= *Src;
			Val -= (Val >> Density);

			*Src = Val;

			Src++;
			Dest++;

		}
	}
}

void UpdateWaterTable(int32 Width, int32 Height, float Time)
{
	int16		*Page1, *Page2, *Page3;

	Page1 = WaterData[NPage];
	Page2 = WaterData[!NPage];

	Page3 = Page2;

	TimeToSplashWater += Time;

	if (TimeToSplashWater > 0.5f)
	{
		int32	px, py, cx, cy, c;
		
		TimeToSplashWater = 0.0f;
		
		for (c=0; c< 2; c++)
		{
			px=(1+(rand()%(Width-1-3)));
			py=(1+(rand()%(Height-1-3)));

			for(cy=py; cy< (py+3); cy++)
				for(cx=px; cx< (px+3); cx++)
					WaterData[!NPage][cy * Width + cx]=128;
		}
	}

	CalcRippleData(Page1, Page2, 4, Width, Height);

	NPage = !NPage;
}
*/

//====================================================================================
//	GameMgr_Frame
//====================================================================================
geBoolean GameMgr_Frame(GameMgr *GMgr, float Time)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	if (Time < 0.001f)
		Time = 0.001f;
	
	if (Time > 0.1f)
		Time = 0.1f;
	
	GMgr->Time += Time;

	//
	//	Do an fx system frame
	//
	if (GMgr->FxSystem)
	{
		if (!Fx_SystemFrame(GMgr->FxSystem, Time))
			GenVS_Error("GameMgr_Frame:  Fx_SystemFrame failed.\n");
	}

	if (GMgr->ProcEng)
	{
		if (!ProcEng_Animate(GMgr->ProcEng, Time))
			GenVS_Error("GameMgr_Frame:  ProcEng_Animate failed.\n");
	}

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_IsValid
//====================================================================================
geBoolean GameMgr_IsValid(GameMgr *GMgr)
{
	if (!GMgr)
	{
		geErrorLog_AddString(-1, "GameMgr_IsValid:  The GameMgr object passed was NULL.", NULL);
		return GE_FALSE;
	}

	if (GMgr->SelfCheck1 != GMgr)			// Check head of struct
	{
		geErrorLog_AddString(-1, "GameMgr_IsValid:  SelfCheck1 is invalid.", NULL);
		return GE_FALSE;
	}

	if (GMgr->SelfCheck2 != GMgr)			// Check tail of struct
	{
		geErrorLog_AddString(-1, "GameMgr_IsValid:  SelfCheck2 is invalid.", NULL);
		return GE_FALSE;
	}

	if (!GMgr->Engine)
	{
		geErrorLog_AddString(-1, "GameMgr_IsValid:  Engine is NULL.", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_PrepareToChangeMode
//====================================================================================
void GameMgr_PrepareToChangeMode(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	if (GMgr->Camera)
		geCamera_Destroy(&GMgr->Camera);
	GMgr->Camera = NULL;

	if (GMgr->Console)
		Console_Destroy(GMgr->Console);
	GMgr->Console = NULL;
}

//====================================================================================
//	GameMgr_SetDriverAndMode
//====================================================================================
geBoolean GameMgr_SetDriverAndMode(GameMgr *GMgr, geDriver *Driver, geDriver_Mode *DriverMode, int Width, int Height)
{
	geRect		CameraRect = {0, 0, 0, 0};

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	if (VidMode_SetResolution(&(GMgr->VidMode),Width,Height)==GE_FALSE)
		GenVS_Error("GameMgr_SetDriverAndMode:  VidMode_SetResolution failed.\n");

	// Make the camera be a good default value...
	CameraRect.Left = 0;
	CameraRect.Right = Width-1;
	CameraRect.Top = 0;
	CameraRect.Bottom = Height-1;

	//
	// Then create a camera for client, etc to use for rendering, etc...
	//
	GMgr->Camera = geCamera_Create(2.0f, &CameraRect);

	if (!GMgr->Camera)
		GenVS_Error("GameMgr_SetDriverAndMode:  geCamera_Create failed.\n");

	// Set the camera ZScale
//	geCamera_SetZScale(GMgr->Camera, 0.5f);
	geCamera_SetZScale(GMgr->Camera, 1.0f);

	// Set up far clip plane (if enabled)
	if (g_FarClipPlaneEnable)
		geCamera_SetFarClipPlane(GMgr->Camera, GE_TRUE, 1500.0f);

	//
	// Create the console with the new vid mode...
	//
	GMgr->Console = Console_Create(GMgr->Engine, GMgr->VidMode);

	if (!GMgr->Console)
		GenVS_Error("GameMgr_SetDriverAndMode:  Console_Create failed.\n");

	Console_Printf(GMgr->Console, "Console Created...\n");

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_BeginFrame
//====================================================================================
geBoolean GameMgr_BeginFrame(GameMgr *GMgr, geWorld *World, geBoolean ClearScreen)
{
	geEngine	*Engine;
	geCamera	*Camera;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	if (GMgr->FrameState != FrameState_None)
		GenVS_Error("GameMgr_BeginFrame:  GMgr->FrameState != FrameState_None.\n");

	Engine = GameMgr_GetEngine(GMgr);
	assert(Engine);

	Camera = GameMgr_GetCamera(GMgr);
	assert(Camera);

	if (!geEngine_BeginFrame(Engine, Camera, ClearScreen))
	{
		GMgr->FrameState = FrameState_None;
		GenVS_Error("GameMgr_BeginFrame:  geEngine_BeginFrame failed.\n");
	}

	GMgr->FrameState = FrameState_Begin;

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_EndFrame
//====================================================================================
geBoolean GameMgr_EndFrame(GameMgr *GMgr)
{
	geEngine	*Engine;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	if (GMgr->FrameState != FrameState_Begin)
		GenVS_Error("GameMgr_EndFrame:  GMgr->FrameState != FrameState_Begin.\n");

	Engine = GameMgr_GetEngine(GMgr);
	assert(Engine);

	if (!geEngine_EndFrame(Engine))
	{
		GMgr->FrameState = FrameState_None;
		GenVS_Error("GameMgr_EndFrame:  geEngine_EndFrame failed.\n");
	}
	
	GMgr->FrameState = FrameState_None;

	return GE_TRUE;
}

//=====================================================================================
//	GameMgr_ConsolePrintf
//=====================================================================================
void GameMgr_ConsolePrintf(GameMgr *GMgr, geBoolean DrawNow, const char *Str, ...)
{
	geCamera			*Camera;
	geEngine			*Engine;
	GameMgr_FrameState	OldFrameState;
	geWorld				*World;

	Engine = GameMgr_GetEngine(GMgr);
	assert(Engine);

	Camera = GameMgr_GetCamera(GMgr);
	assert(Camera);

	World = GMgr->WorldInfo[0].World;
	
	OldFrameState = GMgr->FrameState;

	if (DrawNow && GMgr->FrameState != FrameState_Begin)
	{
		if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
			GenVS_Error("GameMgr_ConsolePrintf:  GameMgr_BeginFrame 1 failed.\n");
	}

	if (!Console_Printf(GMgr->Console, Str))
		GenVS_Error("GameMgr_ConsolePrintf:  Console_Printf failed.\n");

	if (DrawNow && GMgr->FrameState != FrameState_None)
	{
		Console_Frame(GMgr->Console, 0.0f);

		if (!GameMgr_EndFrame(GMgr))
			GenVS_Error("GameMgr_ConsolePrintf:  GameMgr_EndFrame failed.\n");
		
		if (OldFrameState != GMgr->FrameState)		// Restore old state...
		{
			if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
				GenVS_Error("GameMgr_ConsolePrintf:  GameMgr_BeginFrame 3 failed.\n");
		}
	}
}

//=====================================================================================
//	GameMgr_ClearBackground
//=====================================================================================
geBoolean GameMgr_ClearBackground(GameMgr *GMgr, int32 x, int32 y, const char *Str)
{
	geCamera			*Camera;
	geEngine			*Engine;
	GameMgr_FrameState	OldFrameState;
	geWorld				*World;

	Engine = GameMgr_GetEngine(GMgr);
	assert(Engine);

	Camera = GameMgr_GetCamera(GMgr);
	assert(Camera);

	World = GMgr->WorldInfo[0].World;

	OldFrameState = GMgr->FrameState;

	if (GMgr->FrameState != FrameState_None)		// Make sure we are not in a nested begin/frame loop
	{
		if (!GameMgr_EndFrame(GMgr))
			GenVS_Error("GameMgr_ClearBackground:  GameMgr_EndFrame 1 failed.\n");
	}
	
	if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
		GenVS_Error("GameMgr_ClearBackground:  GameMgr_BeginFrame 1 failed.\n");

	if (!GameMgr_EndFrame(GMgr))
		GenVS_Error("GameMgr_ClearBackground:  GameMgr_EndFrame failed.\n");

	if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
		GenVS_Error("GameMgr_ClearBackground:  GameMgr_BeginFrame 2 failed.\n");

	if (Str)
		geEngine_Printf(Engine, x, y, (char*)Str);

	if (!GameMgr_EndFrame(GMgr))
		GenVS_Error("GameMgr_ClearBackground:  GameMgr_EndFrame 2 failed.\n");

	if (OldFrameState != FrameState_None)		// Restore old state...
	{
		if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
			GenVS_Error("GameMgr_ClearBackground:  GameMgr_BeginFrame 3 failed.\n");
	}

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_FreeWorld
//	Clears GameMgr of all world dependent objects.
//====================================================================================
geBoolean GameMgr_FreeWorld(GameMgr *GMgr)
{
	GameMgr_WorldInfo	*WInfo;
	int32				i;
	geBoolean			Ret;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	Ret = GE_TRUE;
	
	WInfo = &GMgr->WorldInfo[0];

	// There is nothing to fre in these arrays, so just zap them
	memset(GMgr->MotionIndexDefs, 0, sizeof(GMgr->MotionIndexDefs));
	memset(GMgr->BoneIndex, 0, sizeof(GMgr->BoneIndex));

	// Free all the actors
	for (i=0; i<GAMEMGR_MAX_ACTOR_INDEX; i++)
	{
		GameMgr_ActorIndex		*AIndex;

		AIndex = &GMgr->ActorIndex[i];
		
		if (AIndex->ActorDef)
		{
			assert(WInfo->World);
			assert(AIndex->Active == GE_TRUE);
			assert(AIndex->ActorHack);
			
			if (!geWorld_RemoveActor(WInfo->World, AIndex->ActorHack))
			{
				geErrorLog_AddString(-1, "GameMgr_FreeWorld:  geWorld_RemoveActor failed.", NULL);
				Ret = GE_FALSE;
			}
			
			geActor_Destroy(&AIndex->ActorHack);

			geActor_DefDestroy(&AIndex->ActorDef);
		}
		
		memset(AIndex, 0, sizeof(*AIndex));
	}

	// Free all the textures
	for (i=0; i<GAMEMGR_MAX_TEXTURE_INDEX; i++)
	{
		GameMgr_TextureIndex *TIndex;

		TIndex = &GMgr->TextureIndex[i];

		if (TIndex->TextureDef)
		{
			assert(WInfo->World);				// If there are bitmaps, there better be a world!!!
			assert(TIndex->Active == GE_TRUE);

			if (!geWorld_RemoveBitmap(WInfo->World, TIndex->TextureDef))
				Ret = GE_FALSE;

			geBitmap_Destroy(&TIndex->TextureDef);
		}

		memset(TIndex, 0, sizeof(*TIndex));
	}

	for (i=0; i<GAMEMGR_MAX_SOUND_INDEX; i++)
	{
		GameMgr_SoundIndex *SIndex;

		SIndex = &GMgr->SoundIndex[i];

		if (SIndex->SoundDef)
		{
			assert(GMgr->SoundSys);				// If there are sounds, there better be a sound system!!!
			assert(SIndex->Active == GE_TRUE);
			geSound_FreeSoundDef(GMgr->SoundSys, SIndex->SoundDef);
		}
		memset(SIndex, 0, sizeof(*SIndex));
	}

	if (GMgr->FxSystem)		// YES, Fx_System depends on a world, so it must be freed now, until a new world is loaded...
	{
		Fx_SystemDestroy(GMgr->FxSystem);
		GMgr->FxSystem = NULL;
	}

	if (GMgr->ShadowMap)
	{
		assert(WInfo->World);

		geWorld_RemoveBitmap(WInfo->World, GMgr->ShadowMap);
		geBitmap_Destroy( &(GMgr->ShadowMap) );
		GMgr->ShadowMap = NULL;
	}

	// Free the world last, so that all data contained in the world would have been freed above...
	// Free any previously existing world
	if (WInfo->World)
	{
		if (!Electric_Shutdown())
		{
			geErrorLog_AddString(-1, "GameMgr_FreeWorld:  Electric_Shutdown failed...", NULL);
			Ret = GE_FALSE;
		}

		if (!Corona_Shutdown())
		{
			geErrorLog_AddString(-1, "GameMgr_FreeWorld:  Corona_Shutdown failed...", NULL);
			Ret = GE_FALSE;
		}

		if (!geEngine_RemoveWorld(GMgr->Engine, WInfo->World))
		{
			geErrorLog_AddString(-1, "GameMgr_FreeWorld:  geEngine_RemoveWorld failed...", NULL);
			Ret = GE_FALSE;
		}

		geWorld_Free(WInfo->World);
		
		if ( GMgr->ProcEng )
			ProcEng_Destroy(&(GMgr->ProcEng));
		GMgr->ProcEng = NULL;
	}

	WInfo->World = NULL;

	GMgr->Time = 0.0f;		// FIXME:  Take out?  Put in GameMgr_SetWorld?

	return Ret;
}

#define PATH_SEPERATOR '/'

//=====================================================================================
//	DefaultExtension
//=====================================================================================
void DefaultExtension (char *Path, const char *Ext)
{
	char    *Src;
	
	Src = Path + strlen(Path) - 1;

	while (*Src != PATH_SEPERATOR && Src != Path)
	{
		if (*Src == '.')
			return;                 // it has an extension
		Src--;
	}

	strcat (Path, Ext);
}

//====================================================================================
//	GameMgr_SetWorld
//	Sets the current world in the GameMgr.
//====================================================================================
geBoolean GameMgr_SetWorld(GameMgr *GMgr, const char *WorldName)
{
	GameMgr_WorldInfo	*WInfo;
	geWorld_Model		*Model;
	geVFile * 			WorldFile;
	char				TFile[GAMEMGR_MAX_WORLD_NAME+4];
	int					Width,Height;
	int					MessageWidth = 250;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(strlen(WorldName) < GAMEMGR_MAX_WORLD_NAME);
	
	strcpy(TFile, WorldName);

	DefaultExtension(TFile, ".BSP");

	WInfo = &GMgr->WorldInfo[0];

	if (GameMgr_GetFrameState(GMgr) != FrameState_None)
		GenVS_Error("GameMgr_SetWorld:  GameMgr_GetFrameState(GMgr) != FrameState_None.\n");

	VidMode_GetResolution(GMgr->VidMode,&Width,&Height);

	if (!GameMgr_ClearBackground(GMgr, (Width/2)-(MessageWidth/2), Height/2-10, "Loading level, please wait..."))
		GenVS_Error("GameMgr_SetWorld:  GameMgr_ClearBackground failed.\n");
	//GameMgr_ConsolePrintf(GMgr, GE_TRUE, "Loading level, please wait...\n");

	// Make sure we free the old world!!!
	GameMgr_FreeWorld(GMgr);		// Clear all that belongs to world stuff

	WorldFile = geVFile_Open(MainFS, TFile, GE_VFILE_OPEN_READONLY);

	if	(!WorldFile)
		GenVS_Error("GameMgr_SetWorld:  geVFile_Open failed: %s.\n", TFile);

	WInfo->World = geWorld_Create(WorldFile);
	geVFile_Close(WorldFile);

	if (!WInfo->World)
		GenVS_Error("GameMgr_SetWorld:  geWorld_Create failed: %s.\n", TFile);

	if (!geEngine_AddWorld(GMgr->Engine, WInfo->World))
		GenVS_Error("GameMgr_SetWorld:  geEngine_AddWorld failed: %s.\n", TFile);

	{
	char ProcCfgName[1024];
	geVFile * ProcCfgFile;
		strcpy(ProcCfgName,TFile);
		assert( ProcCfgName[ strlen(TFile) - 4 ] == '.' );
		ProcCfgName[ strlen(TFile) - 4 ] = 0;
		strcat(ProcCfgName,".prc");
		ProcCfgFile = geVFile_Open(MainFS,ProcCfgName, GE_VFILE_OPEN_READONLY);
		
		if	(!ProcCfgFile)
		{
		}
		else
		{
			GMgr->ProcEng = ProcEng_Create(ProcCfgFile, WInfo->World);
			geVFile_Close(ProcCfgFile);
		}
	}

	GMgr->FxSystem = Fx_SystemCreate(WInfo->World, NULL);

	if (!GMgr->FxSystem)
		GenVS_Error("GameMgr_SetWorld:  Fx_SystemCreate failed.\n");

	// Create the gamemgr shadow map
	if (!(GMgr->ShadowMap = geBitmapUtil_CreateFromFileAndAlphaNames(MainFS, "Bmp\\Fx\\Smoke_05.bmp", "Bmp\\Fx\\A_Smk05.bmp")) )
		GenVS_Error("GameMgr_SetWorld:  geBitmapUtil_CreateFromFileAndAlphaNames failed.\n");

	if (!geWorld_AddBitmap( WInfo->World, GMgr->ShadowMap) )
		GenVS_Error("GameMgr_SetWorld:  geWorld_AddBitmap failed.\n");

	// Setup the models with the world
	WInfo->NumModels = 0;
	Model = NULL;

	while(1)
	{
		assert(WInfo->NumModels <= GAMEMGR_MAX_MODELS);

		Model = geWorld_GetNextModel(WInfo->World, Model);

		if (!Model)
			break;

		WInfo->Models[WInfo->NumModels] = Model;
		WInfo->NumModels++;
	}

	// Set some light type defaults
	geWorld_SetLTypeTable(WInfo->World, 0, "z");
	geWorld_SetLTypeTable(WInfo->World, 1, "mmnmmommommnonmmonqnmmo");
	geWorld_SetLTypeTable(WInfo->World, 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	geWorld_SetLTypeTable(WInfo->World, 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	geWorld_SetLTypeTable(WInfo->World, 4, "mamamamamama");
	geWorld_SetLTypeTable(WInfo->World, 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	geWorld_SetLTypeTable(WInfo->World, 6, "nmonqnmomnmomomno");
	geWorld_SetLTypeTable(WInfo->World, 7, "mmmaaaabcdefgmmmmaaaammmaamm");
	geWorld_SetLTypeTable(WInfo->World, 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	geWorld_SetLTypeTable(WInfo->World, 9, "aaaaaaaazzzzzzzz");
	geWorld_SetLTypeTable(WInfo->World, 10,"mmamammmmammamamaaamammma");
	geWorld_SetLTypeTable(WInfo->World, 11,"abcdefghijklmnopqrrqponmlkjihgfedcba");

	assert(GMgr->Engine);
	
	// Create the coronas
	if (!Corona_Init(GameMgr_GetEngine(GMgr), GameMgr_GetWorld(GMgr), MainFS))
		GenVS_Error("GameMgr_SetWorld:  Corona_Init failed.\n");

	// Create the electric interface
	if (!Electric_Init(GameMgr_GetEngine(GMgr), GameMgr_GetWorld(GMgr), MainFS, GameMgr_GetSoundSystem(GMgr)))
		GenVS_Error("GameMgr_SetWorld:  Electric_Init failed.\n");

	// Create the dynlight interface
	if (!DynLight_Init(GameMgr_GetEngine(GMgr), GameMgr_GetWorld(GMgr), MainFS))
		GenVS_Error("GameMgr_SetWorld:  DynLight_Init failed.\n");

	// Create the ModelCtl interface
	if (!ModelCtl_Init())
		GenVS_Error("GameMgr_SetWorld:  ModelCtl_Init failed.\n");

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_SetActorIndex
//====================================================================================
geBoolean GameMgr_SetActorIndex(GameMgr *GMgr, int32 Index, const char *FileName)
{
	GameMgr_WorldInfo	*WInfo;
	GameMgr_ActorIndex	*ActorIndex;
	geVFile *				File;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(FileName);
	assert(strlen(FileName) < GAMEMGR_MAX_ACTOR_NAME);
	assert(Index < GAMEMGR_MAX_ACTOR_INDEX);

	ActorIndex = &GMgr->ActorIndex[Index];

	assert(ActorIndex->ActorDef == NULL);
	assert(ActorIndex->ActorHack == NULL);
	assert(ActorIndex->Active == GE_FALSE);
	assert(GameMgr_GetFrameState(GMgr) == FrameState_None);

	if (GameMgr_GetFrameState(GMgr) != FrameState_None)
		GenVS_Error("GameMgr_SetActorIndex:  GameMgr_GetFrameState(GMgr) != FrameState_None.\n");

	WInfo = &GMgr->WorldInfo[0];

	assert(WInfo->World);

	if (!WInfo->World)
		GenVS_Error("GameMgr_SetActorIndex:  NULL World.\n");

	// It is safe to load the mesh index at this point...
	File = geVFile_Open(MainFS, FileName, GE_VFILE_OPEN_READONLY);

	if	(!File)
		GenVS_Error("GameMgr_SetActorIndex:  geVFile_Open failed: %s.\n", FileName);

	ActorIndex->ActorDef = geActor_DefCreateFromFile(File);
	geVFile_Close(File);

	if (!ActorIndex->ActorDef)
		GenVS_Error("GameMgr_SetActorIndex:  geActor_DefCreateFromFile failed: %s.\n", FileName);

	// Make our "fake" player, do we know for sure that the textures will remain in memory...
	ActorIndex->ActorHack = geActor_Create(ActorIndex->ActorDef);
	geWorld_AddActor(WInfo->World, ActorIndex->ActorHack, 0, 0);
	
	if (!ActorIndex->ActorHack)
		GenVS_Error("GameMgr_SetActorIndex:  geWorld_AddActor failed: %s.\n", FileName);

	strcpy(ActorIndex->FileName, FileName);
	ActorIndex->Active = GE_TRUE;

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_GetActorIndex
//====================================================================================
GameMgr_ActorIndex *GameMgr_GetActorIndex(GameMgr *GMgr, int32 Index)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(Index >= 0 && Index < GAMEMGR_MAX_ACTOR_INDEX);

	return &GMgr->ActorIndex[Index];
}

//====================================================================================
//	GameMgr_MotionIndexIsValid
//====================================================================================
geBoolean GameMgr_MotionIndexIsValid(GameMgr *GMgr, GameMgr_MotionIndex Index)
{
	if (Index == GAMEMGR_MOTION_INDEX_NONE)
		return GE_FALSE;

	if (Index < 0 || Index >= GAMEMGR_MAX_MOTION_INDEX)
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	GameMgr_SetMotionIndexDef
//=====================================================================================
geBoolean GameMgr_SetMotionIndexDef(GameMgr *GMgr, GameMgr_MotionIndex Index, const char *MotionName)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(MotionName);
	assert(strlen(MotionName) < GAMEMGR_MAX_MOTION_NAME);
	assert(GameMgr_MotionIndexIsValid(GMgr, Index));
	assert(GMgr->MotionIndexDefs[Index].Active == GE_FALSE);

	strcpy(GMgr->MotionIndexDefs[Index].MotionName, MotionName);
	GMgr->MotionIndexDefs[Index].Active = GE_TRUE;

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_GetMotionIndexDef
//====================================================================================
GameMgr_MotionIndexDef *GameMgr_GetMotionIndexDef(GameMgr *GMgr, GameMgr_MotionIndex Index)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(GameMgr_MotionIndexIsValid(GMgr, Index));

	return &GMgr->MotionIndexDefs[Index];
}

//=====================================================================================
//	GameMgr_SetBoneIndex
//=====================================================================================
geBoolean GameMgr_SetBoneIndex(GameMgr *GMgr, int32 BoneIndex, const char *BoneName)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(BoneName);
	assert(BoneIndex < GAMEMGR_MAX_BONE_INDEX);
	assert(strlen(BoneName) < GAMEMGR_MAX_BONE_NAME);
	assert(GMgr->BoneIndex[BoneIndex].Active == GE_FALSE);

	strcpy(GMgr->BoneIndex[BoneIndex].BoneName, BoneName);
	GMgr->BoneIndex[BoneIndex].Active = GE_TRUE;

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_GetBoneIndex
//====================================================================================
GameMgr_BoneIndex *GameMgr_GetBoneIndex(GameMgr *GMgr, int32 Index)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(Index >= 0 && Index < GAMEMGR_MAX_BONE_INDEX);

	return &GMgr->BoneIndex[Index];
}

//====================================================================================
//	GameMgr_SetTextureIndex
//====================================================================================
geBoolean GameMgr_SetTextureIndex(GameMgr *GMgr, int32 Index, const char *FileName, const char *AFileName)
{
	GameMgr_WorldInfo		*WInfo;
	const char				*pFileName, *pAFileName;
	GameMgr_TextureIndex	*TextureIndex;
	geBitmap				*ABitmap;

	ABitmap = NULL;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(Index >= 0 && Index < GAMEMGR_MAX_TEXTURE_INDEX);
	assert(FileName);
	assert(FileName[0]);
	assert(strlen(FileName) < GAMEMGR_MAX_FILENAME);

	TextureIndex = &GMgr->TextureIndex[Index];

	assert(TextureIndex->TextureDef == NULL);
	assert(TextureIndex->Active == GE_FALSE);
	assert(GameMgr_GetFrameState(GMgr) == FrameState_None);

	if (GameMgr_GetFrameState(GMgr) != FrameState_None)
		GenVS_Error("GameMgr_SetTextureIndex:  GameMgr_GetFrameState(GMgr) != FrameState_None.\n");

	WInfo = &GMgr->WorldInfo[0];

	assert(WInfo->World);

	if (!WInfo->World)
		GenVS_Error("GameMgr_SetTextureIndex:  NULL World.\n");

	// It is now safe to load the texture...
	// It better be NULL first!!! ot somthing went wrong...
	pFileName = FileName;
	
	if (AFileName && AFileName[0])
	{
		assert(strlen(AFileName) < GAMEMGR_MAX_FILENAME);
		pAFileName = AFileName;
	}
	else
		pAFileName = NULL;

	// Create the bitmaps
	TextureIndex->TextureDef = geBitmap_CreateFromFileName(MainFS, pFileName);

	if (!TextureIndex->TextureDef)
	{
		geErrorLog_AddString(-1, "GameMgr_SetTextureIndex:  geBitmap_CreateFromFileName failed:", pFileName);
		goto ExitWithError;
	}

	// Load the alpha bitmap
	if (pAFileName)
	{
		ABitmap = geBitmap_CreateFromFileName(MainFS, pAFileName);

		if (!geBitmap_SetAlpha(TextureIndex->TextureDef, ABitmap))
		{
			geErrorLog_AddString(-1, "GameMgr_SetTextureIndex:  geBitmap_SetAlpha failed.", NULL);
			goto ExitWithError;
		}
		
		// Don't need this anymore
		geBitmap_Destroy(&ABitmap);
	}

	if (!geWorld_AddBitmap(WInfo->World, TextureIndex->TextureDef))
	{
		geErrorLog_AddString(-1, "GameMgr_SetTextureIndex:  geWorld_AddBimap failed.", NULL);
		goto ExitWithError;
	}

	strcpy(TextureIndex->FileName, FileName);
	strcpy(TextureIndex->AFileName, AFileName);
	TextureIndex->Active = GE_TRUE;

	return GE_TRUE;

	ExitWithError:
	{
		if (TextureIndex->TextureDef)
		{
			geBitmap_Destroy(&TextureIndex->TextureDef);
			TextureIndex->TextureDef = NULL;
		}

		if (ABitmap)
		{
			geBitmap_Destroy(&ABitmap);
			ABitmap = NULL;
		}

		GenVS_Error("GameMgr_SetTextureIndex:  Failed.\n");
		return GE_FALSE;		// Shut up compiler
	}
}

//====================================================================================
//	GameMgr_GetTextureIndex
//====================================================================================
GameMgr_TextureIndex *GameMgr_GetTextureIndex(GameMgr *GMgr, int32 Index)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(Index >= 0 && Index < GAMEMGR_MAX_TEXTURE_INDEX);

	return &GMgr->TextureIndex[Index];
}

//=====================================================================================
//	GameMgr_SetSoundIndex
//=====================================================================================
geBoolean GameMgr_SetSoundIndex(GameMgr *GMgr, int32 SoundIndex, const char *FileName)
{
	GameMgr_SoundIndex	*pSoundIndex;
	geVFile *				File;

	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	
	if (!GMgr->SoundSys)
		return GE_TRUE;
	
	assert(FileName);
	assert(SoundIndex < GAMEMGR_MAX_SOUND_INDEX);
	assert(strlen(FileName) < GAMEMGR_MAX_FILENAME);
	
	pSoundIndex = &GMgr->SoundIndex[SoundIndex];

	assert(pSoundIndex->Active == GE_FALSE);
	assert(pSoundIndex->SoundDef == NULL);

	File = geVFile_Open(MainFS, FileName, GE_VFILE_OPEN_READONLY);

	if (!File)
		GenVS_Error("GameMgr_SetSoundIndex:  geVFile_Open failed: %s.\n", FileName);

	pSoundIndex->SoundDef = geSound_LoadSoundDef(GMgr->SoundSys, File);
	geVFile_Close(File);

	if (!pSoundIndex->SoundDef)
		GenVS_Error("geSound_LoadSoundDef:  geVFile_Open failed: %s.\n", FileName);

	assert(pSoundIndex->SoundDef);

	strcpy(pSoundIndex->FileName, FileName);
	pSoundIndex->Active = GE_TRUE;

	return GE_TRUE;
}

//====================================================================================
//	GameMgr_GetSoundIndex
//====================================================================================
GameMgr_SoundIndex *GameMgr_GetSoundIndex(GameMgr *GMgr, int32 Index)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);
	assert(Index >= 0 && Index < GAMEMGR_MAX_SOUND_INDEX);

	return &GMgr->SoundIndex[Index];
}

//====================================================================================
//	GameMgr_GetEngine
//====================================================================================
geEngine *GameMgr_GetEngine(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->Engine;
}

//====================================================================================
//	GameMgr_GetSoundSystem
//====================================================================================
geSound_System *GameMgr_GetSoundSystem(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->SoundSys;
}

//====================================================================================
//	GameMgr_GetConsole
//====================================================================================
Console_Console *GameMgr_GetConsole(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->Console;
}

//====================================================================================
//	GameMgr_GetWorld
//====================================================================================
geWorld *GameMgr_GetWorld(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->WorldInfo[0].World;
}

//====================================================================================
//	GameMgr_GetCamera
//====================================================================================
geCamera *GameMgr_GetCamera(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->Camera;
}

//====================================================================================
//	GameMgr_GetFrameState
//====================================================================================
GameMgr_FrameState GameMgr_GetFrameState(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->FrameState;
}

//====================================================================================
//	GameMgr_GetNumModels
//====================================================================================
int32 GameMgr_GetNumModels(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->WorldInfo[0].NumModels;
}

//====================================================================================
//	GameMgr_GetModel
//====================================================================================
geWorld_Model *GameMgr_GetModel(GameMgr *GMgr, int32 Index)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	assert(Index >= 0 && Index < GameMgr_GetNumModels(GMgr));

	return GMgr->WorldInfo[0].Models[Index];
}

//====================================================================================
//	GameMgr_GetFxSystem
//====================================================================================
Fx_System *GameMgr_GetFxSystem(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->FxSystem;
}

//====================================================================================
//	GameMgr_GetShadowMap
//====================================================================================
geBitmap *GameMgr_GetShadowMap(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->ShadowMap;
}

//====================================================================================
//	GameMgr_GetTime
//====================================================================================
float GameMgr_GetTime(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->Time;
}

//====================================================================================
//	GameMgr_GethWnd
//====================================================================================
HWND GameMgr_GethWnd(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->hWnd;
}

//====================================================================================
//	GameMgr_GetVidMode
//====================================================================================
VidMode GameMgr_GetVidMode(GameMgr *GMgr)
{
	assert(GameMgr_IsValid(GMgr) == GE_TRUE);

	return GMgr->VidMode;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);


void GameMgr_ResetMainWindow(HWND hWnd, int32 Width, int32 Height)
{
	RECT ScreenRect;

	GetWindowRect(GetDesktopWindow(),&ScreenRect);

	SetWindowLong(hWnd, 
                 GWL_STYLE, 
                 GetWindowLong(hWnd, GWL_STYLE) & ~WS_POPUP);

	SetWindowLong(hWnd, 
                 GWL_STYLE, 
                 GetWindowLong(hWnd, GWL_STYLE) | (WS_OVERLAPPED  | 
                                                   WS_CAPTION     | 
                                                   WS_SYSMENU     | 
                                                   WS_MINIMIZEBOX));

	SetWindowLong(hWnd, 
                  GWL_STYLE, 
                  GetWindowLong(hWnd, GWL_STYLE) | WS_THICKFRAME |
                                                     WS_MAXIMIZEBOX);

	//SetWindowLong(hWnd, 
    //              GWL_EXSTYLE, 
    //              GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_TOPMOST);

	{
		RECT ClientRect;
		int Style = GetWindowLong (hWnd, GWL_STYLE);
		
		ClientRect.left = 0;
		ClientRect.top = 0;
		ClientRect.right = Width - 1;
		ClientRect.bottom = Height - 1;
		AdjustWindowRect (&ClientRect, Style, FALSE);	// FALSE == No menu	
		
		{
			int WindowWidth = ClientRect.right - ClientRect.left + 1;
			int WindowHeight = ClientRect.bottom - ClientRect.top + 1;
			SetWindowPos 
			(
				hWnd, HWND_TOP,
				(ScreenRect.right+ScreenRect.left)/2 - WindowWidth/2,
				(ScreenRect.bottom+ScreenRect.top)/2 - WindowHeight/2, 
				WindowWidth, WindowHeight,
				SWP_NOCOPYBITS | SWP_NOZORDER
			);
		}
	}

	ShowWindow(hWnd, SW_SHOWNORMAL);
	UpdateWindow(hWnd);
	SetFocus(hWnd);
}

//=====================================================================================
//	CreateMainWindow
//=====================================================================================
static HWND CreateMainWindow(HINSTANCE hInstance, const char *AppName, int32 Width, int32 Height)
{
	WNDCLASS		wc;
	HWND			hWnd;
	RECT		    ScreenRect;
	
	//
	// Set up and register application window class
	//
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	// change this if we ever make an icon.
	wc.hIcon         = NULL; //LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = (const char*)NULL;
	wc.lpszClassName = AppName;

	RegisterClass(&wc);

	//
	// Create application's main window
	//
	GetWindowRect(GetDesktopWindow(),&ScreenRect);
	hWnd = CreateWindowEx(
		0,
		AppName,
		AppName,
		0,
		(ScreenRect.right+ScreenRect.left)/2 - Width/2,
		(ScreenRect.bottom+ScreenRect.top)/2 - Height/2, 
		Width,
		Height,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hWnd)
	{
		MessageBox(0, "Could not create window.", "** ERROR **", MB_OK);
		_exit(1);
	}	

	
	GameMgr_ResetMainWindow(hWnd, Width, Height);
	SetFocus(hWnd);

	return hWnd;

}

