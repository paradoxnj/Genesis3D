/****************************************************************************************/
/*  Fx.c                                                                                */
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

#include "Genesis.h"
#include "Errorlog.h"
#include "Ram.h"

#include "Fx.h"

#include "..\\Procedurals\gebmutil.h"

static geBoolean ControlParticleAnim(Fx_System *Fx, Fx_TempPlayer *Player, float Time);

#define POLY_FLAGS	(GE_RENDER_DEPTH_SORT_BF)
//#define POLY_FLAGS	(GE_RENDER_DO_NOT_OCCLUDE_OTHERS)

extern	geFloat	EffectScale;

static char SmokeBitmapNames[NUM_SMOKE_TEXTURES][32] = {
	"Bmp\\Fx\\Smoke_01.Bmp",
	"Bmp\\Fx\\Smoke_02.Bmp",
	"Bmp\\Fx\\Smoke_03.Bmp",
	"Bmp\\Fx\\Smoke_04.Bmp",
	"Bmp\\Fx\\Smoke_05.Bmp",
	"Bmp\\Fx\\Smoke_06.Bmp",
	"Bmp\\Fx\\Smoke_07.Bmp",
	"Bmp\\Fx\\Smoke_08.Bmp",
	"Bmp\\Fx\\Smoke_09.Bmp",
	"Bmp\\Fx\\Smoke_10.Bmp"
};

static char SmokeBitmapANames[NUM_SMOKE_TEXTURES][32] = {
	"Bmp\\Fx\\A_Smk01.Bmp",
	"Bmp\\Fx\\A_Smk02.Bmp",
	"Bmp\\Fx\\A_Smk03.Bmp",
	"Bmp\\Fx\\A_Smk04.Bmp",
	"Bmp\\Fx\\A_Smk05.Bmp",
	"Bmp\\Fx\\A_Smk06.Bmp",
	"Bmp\\Fx\\A_Smk07.Bmp",
	"Bmp\\Fx\\A_Smk08.Bmp",
	"Bmp\\Fx\\A_Smk09.Bmp",
	"Bmp\\Fx\\A_Smk10.Bmp",
};

static char ExplodeBitmapNames[NUM_EXPLODE_TEXTURES][32] = {
	"Bmp\\Explode\\1EXP01.Bmp",
	"Bmp\\Explode\\1EXP02.Bmp",
	"Bmp\\Explode\\1EXP03.Bmp",
	"Bmp\\Explode\\1EXP04.Bmp",
	"Bmp\\Explode\\1EXP05.Bmp",
	"Bmp\\Explode\\1EXP06.Bmp"
};

static char ExplodeBitmapANames[NUM_EXPLODE_TEXTURES][32] = {
	"Bmp\\Explode\\A_1EXP01.Bmp",
	"Bmp\\Explode\\A_1EXP02.Bmp",
	"Bmp\\Explode\\A_1EXP03.Bmp",
	"Bmp\\Explode\\A_1EXP04.Bmp",
	"Bmp\\Explode\\A_1EXP05.Bmp",
	"Bmp\\Explode\\A_1EXP06.Bmp"
};

static char ParticleBitmapNames[NUM_PARTICLE_TEXTURES][32] = {
	"Bmp\\Fx\\Parti1.Bmp",
	"Bmp\\Fx\\Parti2.Bmp",
	"Bmp\\Fx\\Parti3.Bmp",
	"Bmp\\Fx\\Parti4.Bmp",
	"Bmp\\Fx\\Parti5.Bmp",
	"Bmp\\Fx\\Parti6.Bmp",
	"Bmp\\Fx\\Parti7.Bmp",
	"Bmp\\Fx\\Parti8.Bmp",
};

static char ParticleBitmapANames[NUM_PARTICLE_TEXTURES][32] = {
	"Bmp\\Fx\\Parti1.Bmp",
	"Bmp\\Fx\\Parti2.Bmp",
	"Bmp\\Fx\\Parti3.Bmp",
	"Bmp\\Fx\\Parti4.Bmp",
	"Bmp\\Fx\\Parti5.Bmp",
	"Bmp\\Fx\\Parti6.Bmp",
	"Bmp\\Fx\\Parti7.Bmp",
	"Bmp\\Fx\\Parti8.Bmp",
};

//=====================================================================================
// Private local static functions
//=====================================================================================
static geBoolean StartupSmokeTrail(Fx_System *Fx, Fx_Player *Player);
static geBoolean ShutdownSmokeTrail(Fx_System *Fx, Fx_Player *Player);
static geBoolean LoadFxTextures(Fx_System *Fx);
static geBoolean FreeFxTextures(Fx_System *Fx);

static geBoolean ControlExplode1Anim(Fx_System *Fx, Fx_TempPlayer *Player, float Time);
static geBoolean ControlExplode2Anim(Fx_System *Fx, Fx_TempPlayer *Player, float Time);

static geBoolean ControlParticleTrail(Fx_System *Fx, Fx_Player *Player, float Time);
static geBoolean ControlSmokeTrail(Fx_System *Fx, Fx_Player *Player, float Time);
static geBoolean ControlShredderFx(Fx_System *Fx, Fx_Player *Player, float Time);

//=====================================================================================
//	Fx_SystemCreate
//=====================================================================================
Fx_System *Fx_SystemCreate(geWorld *World, Console_Console *Console)
{
	Fx_System	*Fx;

	assert(World);
	//assert(Console);

	Fx = GE_RAM_ALLOCATE_STRUCT(Fx_System);

	if (!Fx)
		return NULL;

	memset(Fx, 0, sizeof(Fx_System));

	Fx->World = World;
	Fx->Console = Console;

	if (!LoadFxTextures(Fx))
	{
		Fx_SystemDestroy(Fx);
		return NULL;
	}

	return Fx;
}

//=====================================================================================
//	Fx_SystemDestroy
//=====================================================================================
void Fx_SystemDestroy(Fx_System *Fx)
{
	geBoolean		Ret;

	assert(Fx);

	Ret = FreeFxTextures(Fx);

	assert(Ret == GE_TRUE);

	geRam_Free(Fx);
}

//=====================================================================================
//	Fx_SystemFrame
//=====================================================================================
geBoolean Fx_SystemFrame(Fx_System *Fx, float Time)
{
	Fx_TempPlayer	*Player;
	int32			i;

	// Control the temp players
	Player = Fx->TempPlayers;
	
	for (i=0; i< FX_MAX_TEMP_PLAYERS; i++, Player++)
	{
		if (!Player->Active)
			continue;

		if (!Player->Control)
			continue;

		// Call the players control routine...
		if (!Player->Control(Fx, Player, Time))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	Fx_SpawnFx
//=====================================================================================
geBoolean Fx_SpawnFx(Fx_System *Fx, geVec3d *Pos, uint8 Type)
{
	Fx_TempPlayer	*TempPlayer;

	TempPlayer = Fx_SystemAddTempPlayer(Fx);

	if (!TempPlayer)
		return GE_TRUE;		// Oh well, they just won't see any smoke...

	TempPlayer->Pos = *Pos;
	TempPlayer->Time = 0.0f;

	switch(Type)
	{
		case FX_EXPLODE1:
			TempPlayer->Control = ControlExplode1Anim;
			break;

		case FX_EXPLODE2:
			TempPlayer->Control = ControlExplode2Anim;
			break;

		default:
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	Fx_SystemAddTempPlayer
//=====================================================================================
Fx_TempPlayer *Fx_SystemAddTempPlayer(Fx_System *Fx)
{
	Fx_TempPlayer	*Player, *End;
	int32			i;


	Player = Fx->CurrentTempPlayer;

	if (!Player)
		Player = Fx->TempPlayers;

	End = &Fx->TempPlayers[FX_MAX_TEMP_PLAYERS-1];
	
	for (i=0; i< FX_MAX_TEMP_PLAYERS; i++)
	{
		if (!Player->Active)		// Look for a non active player
		{
			Fx->CurrentTempPlayer = Player;

			memset(Player, 0, sizeof(Fx_TempPlayer));
			Player->Active = GE_TRUE;
			return Player;
		}

		Player++;

		if (Player >= End)		// Wrap player at end of structure to beginning
			Player = Fx->TempPlayers;
	}

return NULL;
}

//=====================================================================================
//	Fx_SystemRemoveTempPlayer
//=====================================================================================
void Fx_SystemRemoveTempPlayer(Fx_System *Fx, Fx_TempPlayer *Player)
{
	Player->Active = GE_FALSE;

	Fx->CurrentTempPlayer = Player;		// Easy steal for any one who wants to create a temp player
}

//=====================================================================================
//	Fx_PlayerSetFxFlags
//=====================================================================================
geBoolean Fx_PlayerSetFxFlags(Fx_System *Fx, Fx_Player *Player, uint16 FxFlags)
{
	if (FxFlags == Player->FxFlags)
		return GE_TRUE;		// Nothing to change...

	//Console_Printf(Fx->Console, "Flags changed.\n");

	if ((Player->FxFlags & FX_SMOKE_TRAIL) && !(FxFlags & FX_SMOKE_TRAIL))
		ShutdownSmokeTrail(Fx, Player);
	else if (!(Player->FxFlags & FX_SMOKE_TRAIL) && (FxFlags & FX_SMOKE_TRAIL))
		StartupSmokeTrail(Fx, Player);

	Player->FxFlags = FxFlags;				// The flags are now current

	return GE_TRUE;
}

//=====================================================================================
//	Fx_PlayerFrame
//=====================================================================================
geBoolean Fx_PlayerFrame(Fx_System *Fx, Fx_Player *Player, const geXForm3d *XForm, float Time)
{
	Player->XForm = *XForm;

	if (Player->FxFlags & FX_SMOKE_TRAIL)
		ControlSmokeTrail(Fx, Player, Time);

	if (Player->FxFlags & FX_PARTICLE_TRAIL)
		ControlParticleTrail(Fx, Player, Time);

	if (Player->FxFlags & FX_SHREDDER)
		ControlShredderFx(Fx, Player, Time);

	return GE_TRUE;
}

//=====================================================================================
//	StartupSmokeTrail
//=====================================================================================
static geBoolean StartupSmokeTrail(Fx_System *Fx, Fx_Player *Player)
{
	//Console_Printf(Fx->Console, "Smoke trail started.\n");

	return GE_TRUE;
}

//=====================================================================================
//	ShutdownSmokeTrail
//=====================================================================================
static geBoolean ShutdownSmokeTrail(Fx_System *Fx, Fx_Player *Player)
{
	//Console_Printf(Fx->Console, "Smoke trail stopped.\n");

	return GE_TRUE;
}

//=====================================================================================
//	ControlExplode1Anim
//=====================================================================================
static geBoolean ControlExplode1Anim(Fx_System *Fx, Fx_TempPlayer *Player, float Time)
{
	GE_LVertex		Vert;
	int32			Frame;

	Player->Time += Time;

	Vert.X = Player->Pos.X;
	Vert.Y = Player->Pos.Y;
	Vert.Z = Player->Pos.Z;
	Vert.r = Vert.g = Vert.b = Vert.a = 255.0f;
	Vert.u = Vert.v = 0.0f;

	Frame = (int32)(Player->Time*20.0f);

	if (Frame >= NUM_EXPLODE_TEXTURES)
	{
		// Make sure we draw the last frame...
		Frame = NUM_EXPLODE_TEXTURES-1;
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->ExplodeBitmaps[Frame], GE_TEXTURED_POINT, GE_RENDER_DEPTH_SORT_BF, 10.0f * EffectScale);
		Fx_SystemRemoveTempPlayer(Fx, Player);		// Smoke is done...
	}
	else
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->ExplodeBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 10.0f * EffectScale);

	return GE_TRUE;
}

//=====================================================================================
//	ControlExplode2Anim
//=====================================================================================
static geBoolean ControlExplode2Anim(Fx_System *Fx, Fx_TempPlayer *Player, float Time)
{
	GE_LVertex		Vert;
	int32			Frame;

	Player->Time += Time;

	Vert.X = Player->Pos.X;
	Vert.Y = Player->Pos.Y;
	Vert.Z = Player->Pos.Z;
	Vert.r = Vert.g = 255.0f;
	Vert.b = 100.0f;
	Vert.a = 255.0f;
	Vert.u = Vert.v = 0.0f;

	Frame = (int32)(Player->Time*20.0f);

	if (Frame >= NUM_PARTICLE_TEXTURES)
	{
		// Make sure we draw the last frame...
		Frame = NUM_PARTICLE_TEXTURES-1;
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->ParticleBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 8.0f * EffectScale);
		Fx_SystemRemoveTempPlayer(Fx, Player);		// Smoke is done...
	}
	else
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->ParticleBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 8.0f * EffectScale);

	return GE_TRUE;
}

//=====================================================================================
//	LoadFxTextures
//=====================================================================================
extern geVFile *MainFS;
static geBoolean LoadFxTextures(Fx_System *Fx)
{
	int32		i;

	assert(Fx);
	assert(Fx->World);

	for (i=0; i< NUM_SMOKE_TEXTURES; i++)
	{
		Fx->SmokeBitmaps[i] = geBitmapUtil_CreateFromFileAndAlphaNames(MainFS, SmokeBitmapNames[i], SmokeBitmapANames[i]);

		if (!Fx->SmokeBitmaps[i])
		{
			char	Str[1024];
			sprintf(Str, "%s, %s", SmokeBitmapNames[i], SmokeBitmapANames[i]);
			geErrorLog_AddString(-1, "Fx_LoadFxTextures:  geBitmapUtil_CreateFromFileAndAlphaNames failed:", Str);
			goto ExitWithError;
		}

		if (!geWorld_AddBitmap(Fx->World, Fx->SmokeBitmaps[i]))
		{
			geErrorLog_AddString(-1, "Fx_LoadFxTextures: geWorld_AddBItmap failed.", NULL);
			goto ExitWithError;
		}
	}

	for (i=0; i< NUM_PARTICLE_TEXTURES; i++)
	{
		Fx->ParticleBitmaps[i] = geBitmapUtil_CreateFromFileAndAlphaNames(MainFS, ParticleBitmapNames[i], ParticleBitmapANames[i]);

		if (!Fx->ParticleBitmaps[i])
		{
			char	Str[1024];
			sprintf(Str, "%s, %s", ParticleBitmapNames[i], ParticleBitmapANames[i]);
			geErrorLog_AddString(-1, "Fx_LoadFxTextures:  geBitmapUtil_CreateFromFileAndAlphaNames failed:", Str);
			goto ExitWithError;
		}

		if (!geWorld_AddBitmap(Fx->World, Fx->ParticleBitmaps[i]))
		{
			geErrorLog_AddString(-1, "Fx_LoadFxTextures: geWorld_AddBItmap failed.", NULL);
			goto ExitWithError;
		}
	}

	for (i=0; i< NUM_EXPLODE_TEXTURES; i++)
	{
		Fx->ExplodeBitmaps[i] = geBitmapUtil_CreateFromFileAndAlphaNames(MainFS, ExplodeBitmapNames[i], ExplodeBitmapANames[i]);

		if (!Fx->ExplodeBitmaps[i])
		{
			char	Str[1024];
			sprintf(Str, "%s, %s", ExplodeBitmapNames[i], ExplodeBitmapANames[i]);
			geErrorLog_AddString(-1, "Fx_LoadFxTextures:  geBitmapUtil_CreateFromFileAndAlphaNames failed:", Str);
			goto ExitWithError;
		}

		if (!geWorld_AddBitmap(Fx->World, Fx->ExplodeBitmaps[i]))
		{
			geErrorLog_AddString(-1, "Fx_LoadFxTextures: geWorld_AddBItmap failed.", NULL);
			goto ExitWithError;
		}
	}

	return GE_TRUE;

	ExitWithError:
	{
		FreeFxTextures(Fx);

		return GE_FALSE;
	}
}

//=====================================================================================
//	LoadFxTextures
//=====================================================================================
static geBoolean FreeFxTextures(Fx_System *Fx)
{
	int32		i;

	assert(Fx);
	assert(Fx->World);
		
	for (i=0; i< NUM_SMOKE_TEXTURES; i++)
	{
		if (Fx->SmokeBitmaps[i])
		{
			geWorld_RemoveBitmap(Fx->World, Fx->SmokeBitmaps[i]);
			geBitmap_Destroy(&Fx->SmokeBitmaps[i]);
			Fx->SmokeBitmaps[i] = NULL;
		}
	}

	for (i=0; i< NUM_PARTICLE_TEXTURES; i++)
	{
		if (Fx->ParticleBitmaps[i])
		{
			geWorld_RemoveBitmap(Fx->World, Fx->ParticleBitmaps[i]);
			geBitmap_Destroy(&Fx->ParticleBitmaps[i]);
			Fx->ParticleBitmaps[i] = NULL;
		}
	}

	for (i=0; i< NUM_EXPLODE_TEXTURES; i++)
	{
		if (Fx->ExplodeBitmaps[i])
		{
			geWorld_RemoveBitmap(Fx->World, Fx->ExplodeBitmaps[i]);
			geBitmap_Destroy(&Fx->ExplodeBitmaps[i]);
			Fx->ExplodeBitmaps[i] = NULL;
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//	FxPlayer controls
//=====================================================================================

//=====================================================================================
//	ControlParticleAnim
//=====================================================================================
static geBoolean ControlParticleAnim(Fx_System *Fx, Fx_TempPlayer *Player, float Time)
{
	GE_LVertex		Vert;
	int32			Frame;

	Player->Time += Time;

	Vert.X = Player->Pos.X;
	Vert.Y = Player->Pos.Y;
	Vert.Z = Player->Pos.Z;
	Vert.r = Vert.g = Vert.b = Vert.a = 255.0f;
	Vert.u = Vert.v = 0.0f;

	Frame = (int32)(Player->Time*20.0f);

	if (Frame >= NUM_PARTICLE_TEXTURES)
	{
		// Make sure we draw the last frame...
		Frame = NUM_PARTICLE_TEXTURES-1;
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->ParticleBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 2.3f * EffectScale);
		Fx_SystemRemoveTempPlayer(Fx, Player);		// Smoke is done...
	}
	else
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->ParticleBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 2.3f * EffectScale);

	return GE_TRUE;
}

//=====================================================================================
//	ControlParticleTrail
//=====================================================================================
static geBoolean ControlParticleTrail(Fx_System *Fx, Fx_Player *Player, float Time)
{
	Fx_TempPlayer	*TempPlayer;

	Player->Time += Time;

	if (Player->Time >= 0.04f)
	{
		TempPlayer = Fx_SystemAddTempPlayer(Fx);

		if (!TempPlayer)
			return GE_TRUE;		// Oh well, they just won't see any smoke...

		TempPlayer->Control = ControlParticleAnim;
		TempPlayer->Pos = Player->XForm.Translation;
		TempPlayer->Time = 0.0f;

		Player->Time = 0.0f;
	}

	return GE_TRUE;
}

//=====================================================================================
//	ControlSmokeAnim
//=====================================================================================
static geBoolean ControlSmokeAnim(Fx_System *Fx, Fx_TempPlayer *Player, float Time)
{
	GE_LVertex		Vert;
	int32			Frame;

	Player->Time += Time + ((float)(rand()%1000) * (1.0f/1000.0f)* 0.1f);

	Vert.X = Player->Pos.X;
	Vert.Y = Player->Pos.Y;
	Vert.Z = Player->Pos.Z;
	Vert.r = Vert.g = Vert.b = Vert.a = 255.0f;
	Vert.u = Vert.v = 0.0f;

	Frame = (int32)(Player->Time*15.0f);

	if (Frame >= NUM_SMOKE_TEXTURES)
	{
		// Make sure we draw the last frame...
		Frame = NUM_SMOKE_TEXTURES-1;
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->SmokeBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 2.6f * EffectScale);	// 2,6
		Fx_SystemRemoveTempPlayer(Fx, Player);		// Smoke is done...
	}
	else
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->SmokeBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 2.6f * EffectScale);

	return GE_TRUE;
}

//=====================================================================================
//	ControlSmokeTrail
//=====================================================================================
static geBoolean ControlSmokeTrail(Fx_System *Fx, Fx_Player *Player, float Time)
{
	Fx_TempPlayer	*TempPlayer;

	Player->Time += Time;

	if (Player->Time >= 0.04f)
	{
		TempPlayer = Fx_SystemAddTempPlayer(Fx);

		if (!TempPlayer)
			return GE_TRUE;		// Oh well, they just won't see any smoke...

		TempPlayer->Control = ControlSmokeAnim;
		TempPlayer->Pos = Player->XForm.Translation;
		TempPlayer->Time = 0.0f;

		Player->Time = 0.0f;
	}

	return GE_TRUE;
}

//=====================================================================================
//	ControlSmokeAnim
//=====================================================================================
static geBoolean ControlShredderAnim(Fx_System *Fx, Fx_TempPlayer *Player, float Time)
{
	GE_LVertex		Vert;
	int32			Frame;

	Player->Time += Time;

	Vert.X = Player->Pos.X;
	Vert.Y = Player->Pos.Y;
	Vert.Z = Player->Pos.Z;
	Vert.r = Vert.g = Vert.b = Vert.a = 255.0f;
	Vert.u = Vert.v = 0.0f;

	Frame = (int32)(Player->Time*20.0f);

	if (Frame >= NUM_SMOKE_TEXTURES)
	{
		// Make sure we draw the last frame...
		Frame = NUM_SMOKE_TEXTURES-1;
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->SmokeBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 0.6f * EffectScale);	// 2,6
		Fx_SystemRemoveTempPlayer(Fx, Player);		// Smoke is done...
	}
	else
		geWorld_AddPolyOnce(Fx->World, &Vert, 1, Fx->SmokeBitmaps[Frame], GE_TEXTURED_POINT, POLY_FLAGS, 0.6f * EffectScale);

	return GE_TRUE;
}

//=====================================================================================
//	ControlShredderFx
//=====================================================================================
static geBoolean ControlShredderFx(Fx_System *Fx, Fx_Player *Player, float Time)
{
	assert(Fx);
	assert(Player);

	Player->Time += Time;

	if (Player->Time >= 0.03f)
	{
		Fx_TempPlayer	*TempPlayer;
		GE_Collision	Collision;
		geVec3d			Front, Back, In;
		geVec3d			Mins = {-1.0f, -1.0f, -1.0f};
		geVec3d			Maxs = { 1.0f,  1.0f,  1.0f};

		TempPlayer = Fx_SystemAddTempPlayer(Fx);

		if (!TempPlayer)
			return GE_TRUE;		// Oh well...

		TempPlayer->Control = ControlShredderAnim;
		TempPlayer->Time = 0.0f;

		Front = Player->XForm.Translation;

		geXForm3d_GetIn(&Player->XForm, &In);
		geVec3d_AddScaled(&Front, &In, 10000.0f, &Back);
		
		if (geWorld_Collision(Fx->World, NULL, NULL, &Front, &Back, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_ACTORS | GE_COLLIDE_MODELS, 0xffffffff, NULL, NULL, &Collision))
		{
			// Move it back a little from the wall
			geVec3d_AddScaled(&Collision.Impact, &In, -50.0f, &Collision.Impact);
			
			// Randomize the impact point
			Collision.Impact.X += (float)(rand()%15);
			Collision.Impact.Y += (float)(rand()%15);
			Collision.Impact.Z += (float)(rand()%15);

			TempPlayer->Pos = Collision.Impact;
		}
		else
			TempPlayer->Pos = Player->XForm.Translation;

		Player->Time = 0.0f;
	}
	return GE_TRUE;
}

