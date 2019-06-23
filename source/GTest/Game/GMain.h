/****************************************************************************************/
/*  GMain.h                                                                             */
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
#ifndef GMAIN_H
#define GMAIN_H

#include <Windows.h>

#include "Game.h"

#ifdef __cplusplus
extern "C" {
#endif

// Actor indexes
#define ACTOR_INDEX_PLAYER			0
#define ACTOR_INDEX_TURRET			1

#define ACTOR_INDEX_HEALTH			2
#define ACTOR_INDEX_ROCKET_AMMO		3
#define ACTOR_INDEX_ROCKET			4
#define ACTOR_INDEX_SHREDDER		5
#define ACTOR_INDEX_SHREDDER_AMMO	6
#define ACTOR_INDEX_GRENADE_AMMO	7
#define ACTOR_INDEX_GRENADE			8
#define ACTOR_INDEX_ARMOR			9

// Motion indexes
#define ACTOR_MOTION_PLAYER_WALK	0
#define ACTOR_MOTION_PLAYER_RUN		1
#define ACTOR_MOTION_PLAYER_HIT		2
#define ACTOR_MOTION_PLAYER_DIE		3
#define ACTOR_MOTION_PLAYER_IDLE	4
#define ACTOR_MOTION_T1				5
#define ACTOR_MOTION_T2				6

// Sound Indexes
#define SOUND_INDEX_HURT1			5
#define SOUND_INDEX_HURT2			6
#define SOUND_INDEX_HURT3			7
#define SOUND_INDEX_HURT4			8
#define SOUND_INDEX_DIE				9
#define SOUND_INDEX_PICKUP_HEALTH	10
#define SOUND_INDEX_PICKUP_WEAPON1	11
#define SOUND_INDEX_PICKUP_WEAPON2	12
#define SOUND_INDEX_JUMP			13
#define SOUND_INDEX_BLASTER			14
#define SOUND_INDEX_BLASTER_BANG	15
#define SOUND_INDEX_SHREDDER		16
#define SOUND_INDEX_ITEM_SPAWN		17
#define SOUND_INDEX_PLAYER_SPAWN	18
#define SOUND_INDEX_GRENADE			19

// Texture indexes
#define TEXTURE_INDEX_WEAPON2		0

// Inventory types for players...
#define ITEM_BLASTER			0
#define ITEM_GRENADES			1
#define ITEM_ROCKETS			2
#define ITEM_SHREDDER			3
// Reserve 16 slots for weapons

// Armor is hacked for now...  Client assumes armor is in inventory slot 16...
#define ITEM_ARMOR				16

//
// Items info
//

// Health
#define HEALTH_RESPAWN			45.0f

// Rocket
#define ROCKET_RESPAWN			10.0f
#define ROCKET_AMMO_RESPAWN		30.0f

#define ROCKET_AMOUNT			5
#define ROCKET_MAX_AMOUNT		100
#define ROCKET_AMMO_AMOUNT		5

// Grenade
#define GRENADE_RESPAWN			10.0f
#define GRENADE_AMMO_RESPAWN	30.0f

#define GRENADE_AMOUNT			5
#define GRENADE_MAX_AMOUNT		100
#define GRENADE_AMMO_AMOUNT		5

// Shredder
#define SHREDDER_RESPAWN		10.0f
#define SHREDDER_AMMO_RESPAWN	30.0f

#define SHREDDER_AMOUNT			25
#define SHREDDER_MAX_AMOUNT		200
#define SHREDDER_AMMO_AMOUNT	50

// The algo used for armor is as followed:
// Dammage -= ((Armor/MAX_ARMOR)*MAX_ARMOR_PROETECTION)*Dammage
#define	ARMOR_RESPAWN			25.0f
#define ARMOR_AMOUNT			100
#define MAX_ARMOR				300
#define MAX_ARMOR_PROTECTION	0.7f			// Between 0-1

#define ROTATION_FRICTION		0.90f
#define PLAYER_GROUND_FRICTION	7.0f
#define PLAYER_LIQUID_FRICTION	2.5f
#define PLAYER_AIR_FRICTION		1.00f
#define PLAYER_GRAVITY			1600.0f
#define PLAYER_JUMP_THRUST		700//17000.0f
#define PLAYER_SIDE_SPEED		4000.0f

#define CONTENTS_WATER		GE_CONTENTS_USER1
#define CONTENTS_LAVA		GE_CONTENTS_USER2

//===========================================================================
//	Struct defs
//===========================================================================

typedef struct
{
	GenVSI		*VSI;
	GPlayer		*Player;
} CData;


extern GPlayer	*CurrentPlayerStart;

//===========================================================================
//	Function prototypes
//===========================================================================

// GMain.c
BOOL		XFormFromVector(const geVec3d *Source, const geVec3d *Target, float Roll, geXForm3d *Out);
void		SqueezeVector(geVec3d *Vect, float Epsilon);
void		ClampVector(geVec3d *Vect, float Epsilon);
void		ReflectVelocity(geVec3d *In, geVec3d *Normal, geVec3d *Out, float Scale);

geBoolean	AnimatePlayer(GenVSI *VSI, void *PlayerData, uint16 MotionIndex, float Speed, geBoolean Loop);
geBoolean	AnimatePlayer2(GenVSI *VSI, void *PlayerData, int32 MotionSlot, float Speed, geBoolean Loop);

void		UpdateClientInventory(GenVSI *VSI, GPlayer *Player, int32 Slot);

geBoolean	CheckVelocity(GenVSI *VSI, void *PlayerData, float BounceScale, geBoolean AllowBounce, float Time);

// Weapons.c
geBoolean	DammagePlayer(GenVSI *VSI, void *PlayerData, void *TargetData, int32 Amount, float Power, float Time);

void		FireGrenade(GenVSI *VSI, void *PlayerData, float Time);
void		FireRocket(GenVSI *VSI, void *PlayerData, float Time);
geBoolean	FireWeapon(GenVSI *VSI, void *PlayerData, float Time);

geBoolean	Blaster_Control(GenVSI *VSI, void *PlayerData, float Time);
geBoolean	Grenade_Control(GenVSI *VSI, void *PlayerData, float Time);
geBoolean	Rocket_Control(GenVSI *VSI, void *PlayerData, float Time);
geBoolean	Shredder_Control(GenVSI *VSI, void *PlayerData, float Time);

// Items.c
geBoolean	Item_ControlHealth(GenVSI *VSI, void *PlayerData, float Time);

void		Item_HealthSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_ArmorSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_RocketSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_RocketAmmoSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_GrenadeSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_GrenadeAmmoSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_ShredderSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void		Item_ShredderAmmoSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);

// Level.c
geBoolean	SetupWorldCB(GenVSI *VSI, const char *WorldName, geVFile *MainFS);
geBoolean	ShutdownWorldCB(GenVSI *VSI);

void		ChangeLevel_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);

// Attacker.c
void Attacker_TurretSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);	
void Attacker_TurretDestroy(GenVSI *VSI, void *PlayerData, void *ClassData);	
void Attacker_Fire(GenVSI *VSI, GPlayer *Player, GPlayer *Target, float Time);

geBoolean PlayerPhysics(GenVSI *VSI, 
						void *PlayerData, 
						float GroundFriction, 
						float AirFriction, 
						float LiquidFriction, 
						float Gravity,
						float BounceScale, 
						geBoolean AllowBounce,
						float Time);

geBoolean	UpdatePlayerContents(GenVSI *VSI, void *PlayerData, float Time);

geBoolean SelfCollisionCB(geWorld_Model *Model, geActor *Actor, void *Context);
geBoolean SelfCollisionCB2(geWorld_Model *Model, geActor *Actor, void *Context);

void			*GetDMSpawn(GenVSI *VSI);

#ifdef __cplusplus
}
#endif

#endif