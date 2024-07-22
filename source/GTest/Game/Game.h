/****************************************************************************************/
/*  Game.h                                                                              */
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
#ifndef GAME_H
#define GAME_H

#include <Windows.h>

#include "GENESIS.H"
#include "RAM.H"

#include "GenVSI.h"
#include "GPlayer.h"
#include "GenVS.h"

#ifdef __cplusplus
extern "C" {
#endif


//===========================================
// This is also defined if Fx.h !!!!!  
#define FX_SMOKE_TRAIL			(1 << 0)
#define FX_PARTICLE_TRAIL		(1 << 1)
#define FX_SHREDDER				(1 << 2)

// Spawnable fx
#define FX_EXPLODE1				0
#define FX_EXPLODE2				1

//===========================================

//===========================================================================
//	Struct defs
//===========================================================================

//===========================================================================
//	Function prototypes
//===========================================================================
// Server_Main is called upon server-side initialization
// Client_Main is called for client only computers
geBoolean	Server_Main(GenVSI *VSI, const char *LevelName);
geBoolean	Client_Main(GenVSI *VSI);

// Client is a human controlled player, while a bot is a computer controlled player, that acts
// like a human controlled player...
geBoolean	Game_SpawnClient(GenVSI *VSI, geWorld *World, void *PlayerData, void *ClassData);
geBoolean	Game_SpawnBot(GenVSI *VSI, geWorld *World, void *PlayerData, void *ClassData);
void		Game_DestroyClient(GenVSI *VSI, void *PlayerData, void *ClassData);


#ifdef __cplusplus
}
#endif

#endif