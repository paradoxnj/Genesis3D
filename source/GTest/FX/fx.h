/****************************************************************************************/
/*  Fx.h                                                                                */
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
#ifndef FXFX_H
#define FXFX_H

#include <Windows.h>

#include "GENESIS.H"

#include "Console.h"

//=====================================================================================
//=====================================================================================
// Flagged fx for players
#define FX_SMOKE_TRAIL			(1 << 0)
#define FX_PARTICLE_TRAIL		(1 << 1)
#define FX_SHREDDER				(1 << 2)

// Spawnable fx
#define FX_EXPLODE1				0
#define FX_EXPLODE2				1

// Upper bounds
#define NUM_SMOKE_TEXTURES		10
#define NUM_PARTICLE_TEXTURES	8
#define NUM_EXPLODE_TEXTURES	6

#define FX_MAX_TEMP_PLAYERS		512

//=====================================================================================
//=====================================================================================
typedef struct	Fx_System			Fx_System;
typedef struct	Fx_Player			Fx_Player;
typedef struct	Fx_TempPlayer		Fx_TempPlayer;

typedef geBoolean Fx_TempPlayerControl(Fx_System *Fx, Fx_TempPlayer *Player, float Time);


typedef struct Fx_Player
{
	float				Time;

	uint16				FxFlags;

	geXForm3d			XForm;
} Fx_Player;

typedef struct Fx_TempPlayer
{
	geBoolean				Active;
	Fx_TempPlayerControl	*Control;

	float					Time;
	geVec3d					Pos;
} Fx_TempPlayer;

typedef struct Fx_System
{
	geWorld				*World;	

	geBitmap			*SmokeBitmaps[NUM_SMOKE_TEXTURES];
	geBitmap			*ParticleBitmaps[NUM_PARTICLE_TEXTURES];
	geBitmap			*ExplodeBitmaps[NUM_EXPLODE_TEXTURES];

	Fx_TempPlayer		TempPlayers[FX_MAX_TEMP_PLAYERS];
	Fx_TempPlayer		*CurrentTempPlayer;

	Console_Console		*Console;
} Fx_System;


//=====================================================================================
//=====================================================================================
Fx_System	*Fx_SystemCreate(geWorld *World, Console_Console *Console);
void		Fx_SystemDestroy(Fx_System *Fx);
geBoolean	Fx_SystemFrame(Fx_System *Fx, float Time);
geBoolean	Fx_SpawnFx(Fx_System *Fx, geVec3d *Pos, uint8 Type);
Fx_TempPlayer *Fx_SystemAddTempPlayer(Fx_System *Fx);
void		Fx_SystemRemoveTempPlayer(Fx_System *Fx, Fx_TempPlayer *Player);
geBoolean	Fx_PlayerSetFxFlags(Fx_System *Fx, Fx_Player *Player, uint16 FxFlags);
geBoolean	Fx_PlayerFrame(Fx_System *Fx, Fx_Player *Player, const geXForm3d *XForm, float Time);

#endif