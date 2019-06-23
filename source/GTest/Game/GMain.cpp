/****************************************************************************************/
/*  GMain.c                                                                             */
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
#include <Math.h>

#include "GMain.h"

#include "Quatern.h"

#define INCHES_PER_METER (39.37007874016f)

void		GenVS_Error(const char *Msg, ...);  //oh, dear.

//#define FLY_MODE

// FIXME:  Take out Genesis.h dependency.  Replace with server API code that sits on top
//		   of Genesis calls...

geBoolean		Client_Control(GenVSI *VSI, void *PlayerData, float Time);
static geBoolean Door_Control(GenVSI *VSI, void *PlayerData, float Time);
static geBoolean Plat_Control(GenVSI *VSI, void *PlayerData, float Time);
static geBoolean Door_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data);
void			SetupPlayerXForm(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_Main(GenVSI *VSI, const char *LevelName);
static void Player_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData,char *EntityName);
static void Door_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData,char *EntityName);
static void Plat_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData,char *EntityName);
static geBoolean Plat_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data);


static void PhysicsObject_Spawn(GenVSI* VSI, void* PlayerData, void* Class,char *EntityName);
static void PhysicsObject_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData);
static geBoolean PhysicsObject_Trigger(GenVSI* VSI, void* PlayerData, GPlayer* TargetData, void* data);
static geBoolean PhysicsObject_Control(GenVSI* VSI, void* PlayerData, float Time);

static void PhysicsJoint_Spawn(GenVSI* VSI, void* PlayerData, void* Class,char *EntityName);
static void PhysicsJoint_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData);
static geBoolean PhysicsJoint_Trigger(GenVSI* VSI, void* PlayerData, GPlayer* TargetData, void* data);
static geBoolean PhysicsJoint_Control(GenVSI* VSI, void* PlayerData, float Time);

static void PhysicalSystem_Spawn(GenVSI* VSI, void* PlayerData, void* Class,char *EntityName);
static void PhysicalSystem_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData);
static geBoolean PhysicalSystem_Trigger(GenVSI* VSI, void* PlayerData, GPlayer* TargetData, void* data);
static geBoolean PhysicalSystem_Control(GenVSI* VSI, void* PlayerData, float Time);

static void ForceField_Spawn(GenVSI* VSI, void* PlayerData, void* Class,char *EntityName);
static geBoolean ForceField_Trigger(GenVSI* VSI, void* PlayerData, GPlayer* TargetData, void* data);
static geBoolean ForceField_Control(GenVSI* VSI, void* PlayerData, float Time);

static geBoolean IsKeyDown(int KeyCode);

GPlayer	*CurrentPlayerStart;


//=====================================================================================
//	Server_Main
//	Game code called, when server level starts up...
//=====================================================================================
geBoolean Server_Main(GenVSI *VSI, const char *LevelName)
{
	Bot_Main(VSI, LevelName);
    
	CurrentPlayerStart = NULL;

	// SetClassSpawn functions get called once, and are used for each SetWorld...
	// Set the calss spawns
	GenVSI_SetClassSpawn(VSI, "DeathMatchStart", Player_Spawn, NULL);
	
	// Doors/Plats/PhysicalObjects/PhysicsJoints/PhysicalSystems/etc.
	GenVSI_SetClassSpawn(VSI, "Door", Door_Spawn, NULL);
	GenVSI_SetClassSpawn(VSI, "MovingPlat", Plat_Spawn, NULL);
	
	GenVSI_SetClassSpawn(VSI, "PhysicsObject", PhysicsObject_Spawn, PhysicsObject_Destroy);
	GenVSI_SetClassSpawn(VSI, "PhysicsJoint", PhysicsJoint_Spawn, PhysicsJoint_Destroy);
	GenVSI_SetClassSpawn(VSI, "PhysicalSystem", PhysicalSystem_Spawn, PhysicalSystem_Destroy);
	GenVSI_SetClassSpawn(VSI, "ForceField", ForceField_Spawn, NULL);
	

	// Change level trigger
	GenVSI_SetClassSpawn(VSI, "ChangeLevel", ChangeLevel_Spawn, NULL);

	// Items
	GenVSI_SetClassSpawn(VSI, "ItemHealth", Item_HealthSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemArmor", Item_ArmorSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemRocket", Item_RocketSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemRocketAmmo", Item_RocketAmmoSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemGrenade", Item_GrenadeSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemGrenadeAmmo", Item_GrenadeAmmoSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemShredder", Item_ShredderSpawn, NULL);
	GenVSI_SetClassSpawn(VSI, "ItemShredderAmmo", Item_ShredderAmmoSpawn, NULL);

	GenVSI_SetClassSpawn(VSI, "AttackerTurret", Attacker_TurretSpawn, Attacker_TurretDestroy);

	// Set the level with these spawns...
	// FIXME:  Rename to: Server_ExecuteClassSpawns(Server, LevelName);???????
	GenVSI_SetWorld(VSI, SetupWorldCB, ShutdownWorldCB, LevelName);		// Use the level name supplied by the host

	// Proc indexes
	GenVSI_ProcIndex(VSI, 0, Client_Control);
	GenVSI_ProcIndex(VSI, 1, Door_Control);
	GenVSI_ProcIndex(VSI, 2, Door_Trigger);
	GenVSI_ProcIndex(VSI, 3, Plat_Control);
	GenVSI_ProcIndex(VSI, 4, Plat_Trigger);
	GenVSI_ProcIndex(VSI, 5, Blaster_Control);
	GenVSI_ProcIndex(VSI, 6, Rocket_Control);
	GenVSI_ProcIndex(VSI, 7, Grenade_Control);
	GenVSI_ProcIndex(VSI, 8, Shredder_Control);
	GenVSI_ProcIndex(VSI, 9, Item_ControlHealth);

	return GE_TRUE;
}

//=====================================================================================
//	Client_Main
//=====================================================================================
geBoolean Client_Main(GenVSI *VSI)
{
	GenVSI_ProcIndex(VSI, 0, Client_Control);
	GenVSI_ProcIndex(VSI, 1, Door_Control);
	GenVSI_ProcIndex(VSI, 2, Door_Trigger);
	GenVSI_ProcIndex(VSI, 3, Plat_Control);
	GenVSI_ProcIndex(VSI, 4, Plat_Trigger);
	GenVSI_ProcIndex(VSI, 5, Blaster_Control);
	GenVSI_ProcIndex(VSI, 6, Rocket_Control);
	GenVSI_ProcIndex(VSI, 7, Grenade_Control);
	GenVSI_ProcIndex(VSI, 8, Shredder_Control);
	GenVSI_ProcIndex(VSI, 9, Item_ControlHealth);

	return GE_TRUE;
}

// Returns GE_FALSE if colliding with self, or owner to self...
geBoolean SelfCollisionCB(geWorld_Model *Model, geActor *Actor, void *Context)
{
	if (Actor)
	{
		CData	*Data;

		Data = (CData*)Context;

		assert(Data);
		assert(Data->Player);

		if (GenVSI_ActorToPlayer(Data->VSI, Actor) == Data->Player)
			return GE_FALSE;

		// NOTE  - Owner CAN be NULL...
		if (GenVSI_ActorToPlayer(Data->VSI, Actor) == Data->Player->Owner)
			return GE_FALSE;
	}

	return GE_TRUE;
}

// Returns GE_FALSE if colliding with self...
geBoolean SelfCollisionCB2(geWorld_Model *Model, geActor *Actor, void *Context)
{
	if (Actor)
	{
		CData	*Data;

		Data = (CData*)Context;

		assert(Data);
		assert(Data->Player);

		if (GenVSI_ActorToPlayer(Data->VSI, Actor) == Data->Player)
			return GE_FALSE;
	}

	return GE_TRUE;
}

int32	GMode;
extern int32 MenuBotCount;

//=====================================================================================
//	GetDMSpawn
//=====================================================================================
void *GetDMSpawn(GenVSI *VSI)
{
	//if (GenVSI_GetMode(VSI) == 0)
	if (GMode == 0 && !MenuBotCount)
	{
		CurrentPlayerStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, CurrentPlayerStart, "PlayerStart");
			   
		if (!CurrentPlayerStart)
			CurrentPlayerStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, NULL, "PlayerStart");

		//if (!CurrentPlayerStart)
		//	GenVS_Error( "Game_SpawnClient:  No PlayerStart!\n");
	
		if (!CurrentPlayerStart)
			CurrentPlayerStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, NULL, "DeathMatchStart");

		if (!CurrentPlayerStart)
			GenVS_Error( "Game_SpawnClient:  No PlayerStart or DeathMatchStart!\n");
	}
	else
	{
		CurrentPlayerStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, CurrentPlayerStart, "DeathMatchStart");
			   
		if (!CurrentPlayerStart)
			CurrentPlayerStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, NULL, "DeathMatchStart");

		if (!CurrentPlayerStart)
			GenVS_Error( "Game_SpawnClient:  No DeathMatchStart!\n");
	}

	return CurrentPlayerStart;
}

//=====================================================================================
//	Game_SpawnClient
//=====================================================================================
geBoolean Game_SpawnClient(GenVSI *VSI, geWorld *World, void *PlayerData, void *ClassData)
{
	int32			i;
	GPlayer			*Player;

	//Player = (GPlayer*)GenVSI_GetPlayerData(VSI, PlayerHandle);
	Player = (GPlayer*)PlayerData;

	geXForm3d_SetIdentity(&Player->XForm);

	// Get a DM start
#ifndef FLY_MODE
	{
		GPlayer			*DMStart;

		DMStart = static_cast<GPlayer*>(GetDMSpawn(VSI));
		Player->XForm = DMStart->XForm;
	}

	//GenVSI_SetPlayerBox(VSI, PlayerHandle, &Mins, &Maxs);
	Player->Mins.X = -30.0f;
	Player->Mins.Y = -10.0f;
	Player->Mins.Z = -30.0f;
	Player->Maxs.X =  30.0f;
	Player->Maxs.Y = 160.0f;
	Player->Maxs.Z =  30.0f;
#else
	Player->Mins.X =-5.0f;
	Player->Mins.Y =-5.0f;
	Player->Mins.Z =-5.0f;
	Player->Maxs.X = 5.0f;
	Player->Maxs.Y = 5.0f;
	Player->Maxs.Z = 5.0f;
#endif

	Player->Time = 0.0f;
	Player->JustSpawned = GE_TRUE;

	//GenVSI_SetClientGunOffset(VSI, ClientHandle, 0.0f, 130.0f, 0.0f);
	Player->GunOffset.X = 0.0f;
	Player->GunOffset.Y = 130.0f;
	Player->GunOffset.Z = 0.0f;

	Player->Scale = 2.7f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_YAW_ONLY | VIEW_TYPE_COLLIDEMODEL;
	Player->ViewIndex = ACTOR_INDEX_PLAYER;
	Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
	Player->DammageFlags = DAMMAGE_TYPE_NORMAL | DAMMAGE_TYPE_RADIUS;
	
	// Hook player up to client physics controller
	Player->ControlIndex = 0;
	Player->Control = Client_Control;

	// Set the view player on this machine to the clients player
	GenVSI_SetViewPlayer(VSI, Player->ClientHandle, Player);

	// Players keep track of scoring, health, etc for now.
	// Server_SetClientScore/Health must be called to update a particular client
	// that is associated with a player...

	Player->Health = 100;
	Player->Score = 0;

	// FIXME:  Soon, Scores, Health, etc won't be so arbitrary.  Maybe somthing like
	// Quake2's Layouts will be used instead, making it easier to make games that don't
	// use this sort of scoring system...
	GenVSI_SetClientScore(VSI, Player->ClientHandle, Player->Score);
	GenVSI_SetClientHealth(VSI, Player->ClientHandle, Player->Health);

	for (i=0; i< MAX_PLAYER_ITEMS; i++)
	{
		Player->Inventory[i] = 0;
		Player->InventoryHas[i] = GE_FALSE;
	}

	Player->CurrentWeapon = 0;
	
	GenVSI_SetClientInventory(VSI, Player->ClientHandle, ITEM_GRENADES, 0, GE_FALSE);
	GenVSI_SetClientInventory(VSI, Player->ClientHandle, ITEM_ROCKETS, 0, GE_FALSE);
	GenVSI_SetClientInventory(VSI, Player->ClientHandle, ITEM_SHREDDER, 0, GE_FALSE);

	Player->NextWeaponTime = 0.0f;

	return GE_TRUE;
}

//=====================================================================================
//	Game_DestroyClient
//=====================================================================================
void Game_DestroyClient(GenVSI *VSI, void *PlayerData, void *ClassData)
{
	//GenVS_Error( "Client destroyed...");		// For debugging...
}

//=====================================================================================
//	Local static functions...
//=====================================================================================

//extern geEngine *Engine;

//=====================================================================================
//	PlayerLiquid
//
//	Checks to see if the player is in a liquid state, and returns the power of the state
//	Stronger numbers mean thicker liquid, etc...
//=====================================================================================
int32 PlayerLiquid(GPlayer *Player)
{
	if (Player->State == PSTATE_InLava)
		return 10;
	else if (Player->State == PSTATE_InWater)
		return 5;

	return 0;
}

//=====================================================================================
//	CheckPlayer
//=====================================================================================
static geBoolean CheckPlayer(GenVSI *VSI, void *PlayerData)
{
	geVec3d		Mins, Maxs, Pos;
	GE_Contents	Contents;
	GPlayer		*Player;
	uint32		ColFlags;

	Player = (GPlayer*)PlayerData;

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	Pos = Player->XForm.Translation;

	ColFlags = GE_COLLIDE_MODELS;

	if (geWorld_GetContents(GenVSI_GetWorld(VSI), &Pos, &Mins, &Maxs, ColFlags, 0, NULL, NULL, &Contents))
	{
		if (Contents.Contents & GE_CONTENTS_SOLID)
		{
			//geEngine_Printf(Engine, 10, 85, "Stuck");
			if (Player->JustSpawned)
				{
					GenVS_Error("Player_CheckPlayer: the player's starting position is bad - some of the player's bounding box is in solid space.");
				}
			Player->XForm.Translation = Player->LastGoodPos;
			Player->JustSpawned = 0;
			return GE_TRUE;
		}
	}

	Player->JustSpawned = 0;
	return GE_FALSE;
}

#define STEP_HEIGHT		42.0f

//=====================================================================================
//	MovePlayerUpStep
//=====================================================================================
geBoolean MovePlayerUpStep(GenVSI *VSI, void *PlayerData, GE_Collision *Collision)
{
	geVec3d			Mins, Maxs, Pos1, Pos2;
	GE_Collision	Collision2;
	GPlayer			*Player;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	Pos1 = Player->XForm.Translation;
	Pos2 = Player->XForm.Translation;

	Pos2.Y += STEP_HEIGHT;

	// Try to "pop" up on the step
	if (geWorld_Collision(World, &Mins, &Maxs, &Pos1, &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision2))
		return GE_FALSE;	// Can't move

	// Move forward (opposite the normal)
	Pos1 = Pos2;
	geVec3d_AddScaled(&Pos2, &Collision->Plane.Normal, -0.5f, &Pos2);
	
	if (geWorld_Collision(World, &Mins, &Maxs, &Pos1, &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision2))
		return GE_FALSE;	// Can't move

	// Put him back on the ground
	Pos1 = Pos2;
	Pos2.Y -= (STEP_HEIGHT+1.0f);

	// NOTE that this should allways return a collision
	if (geWorld_Collision(World, &Mins, &Maxs, &Pos1, &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision2))
	{
		Pos2 = Collision2.Impact;
		Player->State = PSTATE_Normal;
	}
	else
		return GE_FALSE;

	Player->XForm.Translation = Pos2;

	return GE_TRUE;
}

#define MAX_CLIP_PLANES		5
//=====================================================================================
//	CheckVelocity
//	Adjust players Velocity. based on what it's doing in the world
//=====================================================================================
geBoolean CheckVelocity(GenVSI *VSI, void *PlayerData, float BounceScale, geBoolean AllowBounce, float Time)
{
	int32			NumHits, NumPlanes;
	GE_Collision	Collision;
	int32			HitCount, i, j;
	geVec3d			Mins, Maxs;
	geVec3d			Planes[MAX_CLIP_PLANES];
	geVec3d			Pos, NewPos, OriginalVelocity, PrimalVelocity;
	geVec3d			NewVelocity, Dir;
	float			TimeLeft, Dist;
	GE_Contents		Contents;
	GPlayer			*Player;
	geWorld			*World;
	uint32			ColFlags;

#if 0
	geEntity_EntitySet *	Set;
	geEntity *				Entity;
	geVec3d boundingBoxCenter,
					boundingBoxScale,
					forceFieldCenterToBBCenter;
#endif

	// end VAR section
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// BEGIN

	Player = (GPlayer*)PlayerData;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	geVec3d_Copy(&Player->Velocity, &OriginalVelocity);
	geVec3d_Copy(&Player->Velocity, &PrimalVelocity);

	TimeLeft = Time;
	
	NumHits = 4;
	NumPlanes = 0;

	if (Player->State == PSTATE_Normal)	
		Player->State = PSTATE_InAir;	

	ColFlags = GE_COLLIDE_MODELS;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// check for penetration of Player's bounding box into the radius of a ForceField entity
	#if 0
	geVec3d_Copy(&Player->XForm.Translation, &boundingBoxCenter);

	boundingBoxScale.X = (float)fabs(0.5f * (Maxs.X - Mins.X));
	boundingBoxScale.Y = (float)fabs(0.5f * (Maxs.Y - Mins.Y));
	boundingBoxScale.Z = (float)fabs(0.5f * (Maxs.Z - Mins.Z));

	#pragma message ("Use: GenVSI_GetNextPlayer here...")
	Set = NULL;
	Set = geWorld_GetEntitySet(World, "ForceField");
	if (Set != NULL)
	{
		for	(Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
				 Entity != NULL;
				 Entity = geEntity_EntitySetGetNextEntity(Set, Entity))
		{
			ForceField* ff;
			float forceMultiplier;
			float distToFFCenter;
			float forceMagnitude;
			geVec3d forceDir;
			geVec3d forceToApply;
			geVec3d impulse;

			ff = geEntity_GetUserData(Entity);
			assert(ff != NULL);

			if (! ff->affectsPlayers) 
				continue;				
			
			geVec3d_Subtract(&boundingBoxCenter, &ff->Origin, &forceFieldCenterToBBCenter);
			geVec3d_Copy(&forceFieldCenterToBBCenter, &forceDir);
			#pragma message("does geVec3d_Normalize() return length?")
			geVec3d_Normalize(&forceDir);

			distToFFCenter = geVec3d_Length(&forceFieldCenterToBBCenter);
			if (distToFFCenter < 1.f)
				distToFFCenter = 1.f;

			if ((boundingBoxCenter.X + boundingBoxScale.X) < (ff->Origin.X - ff->radius)) continue;
			if ((boundingBoxCenter.X - boundingBoxScale.X) > (ff->Origin.X + ff->radius)) continue;
			if ((boundingBoxCenter.Y + boundingBoxScale.Y) < (ff->Origin.Y - ff->radius)) continue;
			if ((boundingBoxCenter.Y - boundingBoxScale.Y) > (ff->Origin.Y + ff->radius)) continue;
			if ((boundingBoxCenter.Z + boundingBoxScale.Z) < (ff->Origin.Z - ff->radius)) continue;
			if ((boundingBoxCenter.Z - boundingBoxScale.Z) > (ff->Origin.Z + ff->radius)) continue;

			////////////////////////////////////////////////////////////////////////////////////////////////////
			// player is inside radius of force field, figure out how much force
			// to apply based on force field's falloff function

			switch(ff->falloffType)
			{
				case FALLOFF_NONE:
					forceMultiplier = 1.f;
					break;
				case FALLOFF_ONE_OVER_D:					
					forceMultiplier = 1 / distToFFCenter;
					break;
				case FALLOFF_ONE_OVER_DSQUARED:
					forceMultiplier = 1 / (distToFFCenter * distToFFCenter);
					break;
				default:
					forceMultiplier = 1.f;
			}

			forceMagnitude = ff->strength * forceMultiplier * 1000.f;

			geVec3d_Scale(&forceDir, forceMagnitude, &forceToApply);

			geVec3d_Scale(&forceToApply, Time, &impulse);
			geVec3d_Add(&impulse, &Player->Velocity, &Player->Velocity);
		} // while
	} // if
	#endif
	////////////////////////////////////////////////////////////////////////////////////////////////////

	for (HitCount=0 ; HitCount<NumHits ; HitCount++)
	{
		float	Fd, Bd;
		//Server_Player	*UPlayer;

		Pos = Player->XForm.Translation;
		geVec3d_AddScaled(&Pos, &Player->Velocity, TimeLeft, &NewPos);

		// Make sure the pos did not start out in solid
		if (geWorld_GetContents(World, &Pos, &Mins, &Maxs, GE_COLLIDE_MODELS, 0, NULL, NULL, &Contents))
		{
			if (Contents.Contents & GE_CONTENTS_SOLID)
			{
				if (geWorld_GetContents(World, &NewPos, &Mins, &Maxs, GE_COLLIDE_MODELS, 0, NULL, NULL, &Contents))
				{
					if (Contents.Contents & GE_CONTENTS_SOLID)
					{
						geVec3d_Copy(&Player->LastGoodPos, &Player->XForm.Translation);
						geVec3d_Clear(&Player->Velocity);
						return GE_TRUE;
					}
				}
			}
		}
		
		if (!geWorld_Collision(World, &Mins, &Maxs, &Pos, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision))
			return GE_TRUE;		// Covered the entire distance...

		if (Collision.Plane.Normal.Y > 0.7f)
		{
			if (Player->State == PSTATE_InAir)
				Player->State = PSTATE_Normal;			// Put the player on the ground if in air
			else if (Player->State == PSTATE_Dead)
				Player->State = PSTATE_DeadOnGround;	// Put the player on the ground if in air

			if (Collision.Model)
			{
				GPlayer		*Target;
				Target = (GPlayer*)geWorld_ModelGetUserData(Collision.Model);
				if (Target && Target->Trigger && Target->ViewFlags & VIEW_TYPE_STANDON)
					Target->Trigger(VSI, Target, Player, NULL);
			}
		}

		if (Collision.Model)
		{
			GPlayer		*Target;
			#pragma message ("Use: GenVSI_ModelToPlayer...")
			Target = (GPlayer*)geWorld_ModelGetUserData(Collision.Model);
			if (Target && Target->Trigger && Target->ViewFlags & VIEW_TYPE_PHYSOB)
				Target->Trigger(VSI, Target, Player, (void*)&Collision);
		}
		
		if (Collision.Actor)		// Just stop at actors for now...
		{
			geVec3d_Copy(&Player->LastGoodPos, &Player->XForm.Translation);
			geVec3d_Clear(&Player->Velocity);
			return GE_TRUE;
		}

		// FIXME:  Save this ratio in the impact function...
		Fd = geVec3d_DotProduct(&Pos, &Collision.Plane.Normal) - Collision.Plane.Dist;
		Bd = geVec3d_DotProduct(&NewPos, &Collision.Plane.Normal) - Collision.Plane.Dist;

		Collision.Ratio = Fd / (Fd - Bd);
		
		if (Collision.Ratio > 0.00f)	// Actually covered some distance
		{	
			// Set the players pos to the impact point
			geVec3d_Copy(&Collision.Impact, &Player->XForm.Translation);
			// Restore Velocity
			geVec3d_Copy(&Player->Velocity, &OriginalVelocity);
			NumPlanes = 0;
		}
		
		if (!Collision.Plane.Normal.Y)
		{
			if (MovePlayerUpStep(VSI, Player, &Collision))
			{
				continue;
			}
		}

		//TimeLeft -= TimeLeft * Collision.Ratio;
		
		// Clipped to another plane
		if (NumPlanes >= MAX_CLIP_PLANES)
		{	
			GenVSI_ConsolePrintf(VSI, "MAX_CLIP_PLANES!!!\n");
			geVec3d_Clear(&Player->Velocity);
			return GE_TRUE;
		}

		// Add the plane hit, to the plane list
		geVec3d_Copy (&Collision.Plane.Normal, &Planes[NumPlanes]);
		NumPlanes++;

		//
		// Modify original_velocity so it parallels all of the clip planes
		//
		for (i=0 ; i<NumPlanes ; i++)
		{
			ReflectVelocity(&OriginalVelocity, &Planes[i], &NewVelocity, BounceScale);

			for (j=0 ; j<NumPlanes ; j++)
			{
				if (j != i)
				{
					if (geVec3d_DotProduct(&NewVelocity, &Planes[j]) < 0.0f)
						break;	// not ok
				}
			}
			if (j == NumPlanes)
				break;
		}
		
		if (i != NumPlanes)
		{	// Go along this plane
			geVec3d_Copy(&NewVelocity, &Player->Velocity);
		}
		else
		{	
			if (NumPlanes != 2)
			{
				//Console_Printf(Server->Host->Console, "Clip velocity, numplanes == %i\n",NumPlanes);
				geVec3d_Clear(&Player->Velocity);
				return GE_TRUE;
			}
			
			//Console_Printf(Server->Host->Console, "Cross product\n",NumPlanes);
			geVec3d_CrossProduct(&Planes[0], &Planes[1], &Dir);
			Dist = geVec3d_DotProduct(&Dir, &Player->Velocity);
			geVec3d_Scale(&Dir, Dist, &Player->Velocity);
		}

		//
		//	Don't allow new velocity to go against original velocity unless told otherwise
		//
		if (!AllowBounce && geVec3d_DotProduct (&Player->Velocity, &PrimalVelocity) <= 0.0f)
		{
			geVec3d_Clear(&Player->Velocity);
			return GE_TRUE;
		}
	}
	return GE_TRUE;									
}


//=====================================================================================
//	PlayerPhysics
//=====================================================================================
geBoolean PlayerPhysics(GenVSI *VSI, 
						void *PlayerData, 
						float GroundFriction, 
						float AirFriction, 
						float LiquidFriction, 
						float Gravity,
						float BounceScale, 
						geBoolean AllowBounce, 
						float Time)
{
	float	Speed;
	GPlayer	*Player;

	Player = (GPlayer*)PlayerData;

#ifndef FLY_MODE
	Player->LastGoodPos = Player->XForm.Translation;

	// Add gravity
	switch (Player->State)
	{
		case PSTATE_InLava:
		case PSTATE_InWater:
			Player->Velocity.Y -= PLAYER_GRAVITY*Time*0.05f;
			break;

		default:
			Player->Velocity.Y -= PLAYER_GRAVITY*Time;
			break;
	}

	CheckVelocity(VSI, Player, BounceScale, AllowBounce, Time);

	SqueezeVector(&Player->Velocity, 0.2f);
	geVec3d_AddScaled(&Player->XForm.Translation, &Player->Velocity, Time, &Player->XForm.Translation);

	CheckPlayer(VSI, Player);
#else		// Fly through walls
	Player->State = PSTATE_InAir;
	SqueezeVector(&Player->Velocity, 0.2f);
	geVec3d_AddScaled(&Player->XForm.Translation, &Player->Velocity, Time, &Player->XForm.Translation);
#endif
	
	Speed = geVec3d_Length(&Player->Velocity);

	// Apply friction
	if (Speed > 0.001)
	{
		float	NewSpeed;

		if (Player->State == PSTATE_Normal || Player->State == PSTATE_DeadOnGround)
			NewSpeed = Speed - Time*Speed*PLAYER_GROUND_FRICTION;
		else if (Player->State == PSTATE_InAir || Player->State == PSTATE_Dead)
			NewSpeed = Speed - Time*Speed*PLAYER_AIR_FRICTION;
		else if (PlayerLiquid(Player))
			NewSpeed = Speed - Time*Speed*PLAYER_LIQUID_FRICTION;

		if (NewSpeed < 0.0f)
			NewSpeed = 0.0f;

		NewSpeed /= Speed;

		// Apply movement friction
		geVec3d_Scale(&Player->Velocity, NewSpeed, &Player->Velocity);
	}
	
	UpdatePlayerContents(VSI, Player, Time);

	return GE_TRUE;
}

//=====================================================================================
//	UpdatePlayerContents
//=====================================================================================
geBoolean UpdatePlayerContents(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer		*Player;
	geWorld		*World;
	GE_Contents	Contents;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Player = (GPlayer*)PlayerData;

	if (geWorld_GetContents(World, &Player->XForm.Translation, &Player->Mins, &Player->Maxs, GE_COLLIDE_MODELS, 0, NULL, NULL, &Contents))
	{
		// Check to see if player is in lava...
		if (Contents.Contents & CONTENTS_WATER)
		{
			Player->State = PSTATE_InWater;
		}
		else if (Contents.Contents & CONTENTS_LAVA)
		{
			Player->State = PSTATE_InLava;
		}
		else if (PlayerLiquid(Player))
				Player->State = PSTATE_Normal;
	}
	else if (PlayerLiquid(Player))
			Player->State = PSTATE_Normal;

	return GE_TRUE;
}

geBoolean PlayerDead(GPlayer *Player)
{
	assert(Player);

	if (Player->State == PSTATE_Dead)
		return GE_TRUE;

	if (Player->State == PSTATE_DeadOnGround)
		return GE_TRUE;

	return GE_FALSE;
}

static int32 Hack2;

//=====================================================================================
//	Client_Control
//=====================================================================================
geBoolean Client_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			LVect, InVect;
	float			Speed;
	GE_Contents		Contents;
	geVec3d			CMins, CMaxs;
	float			MoveSpeed;
	geBoolean		DoWalk = GE_FALSE;
	geXForm3d		XForm;
	GPlayer			*Player;
	GenVSI_CMove	*Move;
	geWorld			*World;
	uint32			ColFlags;
	CData			Data;

	Player = (GPlayer*)PlayerData;
							
	assert(Player);

	assert(Player->ClientHandle != CLIENT_NULL_HANDLE);
	
	// FIXME:  Make this function take care of dead players also.  Use a switch statement or somthing.
	// We should not be changing control functions on the fly like that.  Could confuse the server/client...
	//assert(Player->State != PSTATE_Dead && Player->State != PSTATE_DeadOnGround);

	if (PlayerDead(Player))
		return GE_TRUE;
	
	if (IsKeyDown('L'))		
	{
		GenVSI_SetWorld(VSI, SetupWorldCB, ShutdownWorldCB, "Levels\\GenVs.Bsp");
		return GE_TRUE;
	}
	
		
	Move = GenVSI_GetClientMove(VSI, Player->ClientHandle);

	if (!Move)
		return GE_TRUE;

	assert(Move);

	Player->Time += Time;

	// Make sure the xform is valid
	SetupPlayerXForm(VSI, Player, Time);

	geXForm3d_SetYRotation(&XForm, Move->Angles.Y);

	geXForm3d_GetLeft(&XForm, &LVect);

	#ifdef FLY_MODE
		geXForm3d_GetIn(&Player->XForm, &InVect);		// Use the real in vector for liquid
	#else
		if (PlayerLiquid(Player))
			geXForm3d_GetIn(&Player->XForm, &InVect);		// Use the real in vector for liquid
		else
			geXForm3d_GetIn(&XForm, &InVect);
	#endif

	//if (VSI->Mode == MODE_Server)
	if (Move->ButtonBits & HOST_BUTTON_FIRE)
	{
		uint16		Amount;
		geBoolean	Has;

		Player->CurrentWeapon = Move->Weapon;

		assert(Player->CurrentWeapon >= 0 && Player->CurrentWeapon <= 3);

		assert(Player->ClientHandle != CLIENT_NULL_HANDLE);

		FireWeapon(VSI, PlayerData, Time);

		assert(Player->Inventory[Player->CurrentWeapon] >= 0 && Player->Inventory[Player->CurrentWeapon] < 65535);

		Amount = (uint16)Player->Inventory[Player->CurrentWeapon];
		Has = Player->InventoryHas[Player->CurrentWeapon];

		GenVSI_SetClientInventory(VSI, Player->ClientHandle, Player->CurrentWeapon, Amount, Has); // Force a weapon  update
	}

	MoveSpeed = Time;

	if (Player->State != PSTATE_Normal)
		MoveSpeed *= 0.15f;

	// Set his movement Velocity
	if (Move->ForwardSpeed)	
	{
		geVec3d_MA(&Player->Velocity, Move->ForwardSpeed*MoveSpeed, &InVect, &Player->Velocity);
	}

	if (Move->ButtonBits & HOST_BUTTON_LEFT)
		geVec3d_MA(&Player->Velocity, PLAYER_SIDE_SPEED*MoveSpeed, &LVect, &Player->Velocity);

	if (Move->ButtonBits & HOST_BUTTON_RIGHT)
		geVec3d_MA(&Player->Velocity, -PLAYER_SIDE_SPEED*MoveSpeed, &LVect, &Player->Velocity);

	if ((Move->ButtonBits & HOST_BUTTON_JUMP) && (PlayerLiquid(Player) || Player->State == PSTATE_Normal))
	{
		GenVSI_PlaySound(VSI, SOUND_INDEX_JUMP, &Player->XForm.Translation);

		if (Player->State == PSTATE_Normal)
			Player->Velocity.Y += PLAYER_JUMP_THRUST;
		else
			Player->Velocity.Y += PLAYER_JUMP_THRUST*0.2f;
	}

	// Run polayer physics code...
	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	World = GenVSI_GetWorld(VSI);

	assert(World);

	ColFlags = GE_COLLIDE_MODELS;

	if (VSI->Mode == MODE_Server)		// Only do this stuff when we are in server mode...
	{
		if (!PlayerDead(Player) && Player->Roll > 0.0f)
			Player->Roll -= Time;

	#ifndef FLY_MODE
		if (Player->State == PSTATE_InLava)
		{
			if (Player->Roll <= 0.0f)
			{
				DammagePlayer(VSI, NULL, Player, 20, 0.0f, Time);
				Player->Roll = 0.5f;
			}
		}
	#endif

		// Get a box a little bigger than the player for doors, etc...
		CMins = Player->Mins;
		CMaxs = Player->Maxs;
	
		CMins.X -= 100;
		CMins.Y -= 10;
		CMins.Z -= 100;
		CMaxs.X += 100;
		CMaxs.Y += 10;
		CMaxs.Z += 100;

		if (geWorld_GetContents(World, &Player->XForm.Translation, &CMins, &CMaxs, GE_COLLIDE_MODELS, 0, NULL, NULL, &Contents))
		{
			if (Contents.Model)
			{
				GPlayer	*TPlayer;

				TPlayer = (GPlayer*)geWorld_ModelGetUserData(Contents.Model);

				if (TPlayer)
				{
					Hack2 = 0;
				}
				if (TPlayer && TPlayer->Trigger && TPlayer->ViewFlags & VIEW_TYPE_TOUCH)
				{
					TPlayer->Trigger(VSI, TPlayer, Player, NULL);
				}
			}
		}
		
		Data.VSI = VSI;
		Data.Player = Player;

		if (geWorld_GetContents(World, &Player->XForm.Translation, &CMins, &CMaxs, GE_COLLIDE_ACTORS, 0xffffffff, SelfCollisionCB, &Data, &Contents))
		//if (geWorld_GetContents(World, &Player->XForm.Translation, &CMins, &CMaxs, GE_COLLIDE_ACTORS, 0xffffffff, NULL, NULL, &Contents))
		{
			if (Contents.Actor)
			{
				GPlayer	*TPlayer;
				static	int32	HackV;

				//GenVSI_ConsoleHeaderPrintf(VSI, Player->ClientHandle, GE_FALSE, "Hit actor:%i", HackV++);

				TPlayer = (GPlayer*)GenVSI_ActorToPlayer(VSI, Contents.Actor);

				if (TPlayer && TPlayer->Trigger)// && TPlayer->ViewFlags & VIEW_TYPE_TOUCH)
				{
					TPlayer->Trigger(VSI, TPlayer, Player, NULL);
				}
			}
		}
		
	}

	Speed = geVec3d_Length(&Player->Velocity);

	if ((Move->ButtonBits & HOST_BUTTON_LEFT)
			||	(Move->ButtonBits & HOST_BUTTON_RIGHT)
			||	Move->ForwardSpeed)
	{
		if (Player->MotionIndex != ACTOR_MOTION_PLAYER_RUN)
			Player->FrameTime = 0.0f;

		Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
		DoWalk = GE_TRUE;
	}

	if (Speed > 0.1f && DoWalk)
	{
		if (Player->MotionIndex == ACTOR_MOTION_PLAYER_RUN)
		{
			Speed*=0.004f;

			if (Move->ForwardSpeed<0)
				Speed *= -1.0f;
		}
		else
			Speed = 1.0f;
		
		if (AnimatePlayer(VSI, Player, Player->MotionIndex, Time*Speed, GE_TRUE))
		{
			if (Player->MotionIndex != ACTOR_MOTION_PLAYER_RUN)
				Player->FrameTime = 0.0f;

			Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
		}
	}
	else
	{
		if (Player->MotionIndex != ACTOR_MOTION_PLAYER_IDLE)
		{
			Player->MotionIndex = ACTOR_MOTION_PLAYER_IDLE;
			Player->FrameTime = 0.0f;
		}

		AnimatePlayer(VSI, Player, Player->MotionIndex, Time, GE_TRUE);
	}
	
	// Make sure the xform is valid
	SetupPlayerXForm(VSI, Player, Time);

	Player->VPos = Player->XForm.Translation;

	return GE_TRUE;
}

//=====================================================================================
//	Player_Spawn
//=====================================================================================
void Player_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	PlayerStart		*Ps;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = NULL;
	Player->Trigger = NULL;
	Player->Blocked = NULL;
	Player->Time = 0.0f;

	if (ClassData == NULL)
		{
			GenVS_Error("Player_Spawn: entity missing class data ('%s')\n",EntityName);
		}

	Ps = (PlayerStart*)ClassData;

	geXForm3d_SetIdentity(&Player->XForm);
	geXForm3d_SetTranslation(&Player->XForm, Ps->Origin.X, Ps->Origin.Y, Ps->Origin.Z);

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Door_Control
//=====================================================================================
static geBoolean Door_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	geWorld_Model	*Model;
	geMotion		*Motion;
	float			StartTime, EndTime, NewTime;
	geXForm3d		DestXForm;
	gePath			*Path;
	GPlayer			*Player;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;

	if (Player->State != PSTATE_Opened)
		return GE_TRUE;

	World = GenVSI_GetWorld(VSI);

	assert(World);
 
 	Model = Player->Model;

	assert(Model);

	Motion = geWorld_ModelGetMotion(Model);
	assert(Motion);

	NewTime = Player->FrameTime + Time;

	Path = geMotion_GetPath(Motion, 0);

	assert(Path);

	geMotion_GetTimeExtents(Motion, &StartTime , &EndTime);

	if (NewTime >= EndTime)		// Played through, done...
	{
		NewTime = StartTime;
		Player->State = PSTATE_Closed;

		Player->ViewFlags &= ~VIEW_TYPE_MODEL_OPEN;
	}

	// Get the xform for the current time
	gePath_Sample(Path, NewTime, &DestXForm);

	// If the player can move, then update time, and xform
	if (GenVSI_MovePlayerModel(VSI, Player, &DestXForm))
	{
		Player->XForm = DestXForm;
		//DData->AnimTime = NewTime;
		Player->FrameTime = NewTime;
	}

	//geWorld_GetModelRotationalCenter(World, Model, &Player->VPos);
	//geVec3d_Add(&Player->VPos, &Player->XForm.Translation, &Player->VPos);

	return GE_TRUE;
}

//=====================================================================================
//	Plat_Control
//=====================================================================================
static geBoolean Plat_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	geWorld_Model	*Model;
	geMotion		*Motion;
	float			StartTime, EndTime, NewTime;
	geXForm3d		DestXForm;
	gePath			*Path;
	GPlayer			*Player;
	geWorld			*World;
	MovingPlat		*Plat;

	Player = (GPlayer*)PlayerData;

	if (Player->State != PSTATE_Opened)
		return GE_TRUE;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Model = Player->Model;

	assert(Model);

	Motion = geWorld_ModelGetMotion(Model);
	assert(Motion);

	NewTime = Player->FrameTime + Time;

	Path = geMotion_GetPath(Motion, 0);

	assert(Path);

	geMotion_GetTimeExtents(Motion, &StartTime , &EndTime);

	if (NewTime >= EndTime)		// Played through, done...
	{
		NewTime = StartTime;
		Player->State = PSTATE_Closed;

		Player->ViewFlags &= ~VIEW_TYPE_MODEL_OPEN;
		
		Plat = (MovingPlat*)Player->ClassData;
		/*
		if (Plat->Loop)
		{
			Player->Trigger(VSI, Player, NULL);
		}
		*/
	}

	// Get the xform for the current time
	gePath_Sample(Path, NewTime, &DestXForm);

	// If the player can move, then update time, and xform
	if (GenVSI_MovePlayerModel(VSI, Player, &DestXForm))
	{
		Player->XForm = DestXForm;
		//DData->AnimTime = NewTime;
		Player->FrameTime = NewTime;
	}

	//geWorld_GetModelRotationalCenter(World, Model, &Player->VPos);
	//geVec3d_Add(&Player->VPos, &Player->XForm.Translation, &Player->VPos);

	return GE_TRUE;
}

//=====================================================================================
//	Door_Trigger
//=====================================================================================
static geBoolean Door_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer		*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = TargetData;

	if (Player->State == PSTATE_Opened)
		return GE_TRUE;

	Player->State = PSTATE_Opened;
	Player->FrameTime = 0.0f;

	assert(Player->Model);

	Player->ViewFlags |= VIEW_TYPE_MODEL_OPEN;

	GenVSI_PlaySound(VSI, 2, &Player->XForm.Translation);

	return GE_TRUE;
}

//=====================================================================================
//	Door_Blocked
//=====================================================================================
static geBoolean Door_Blocked(GenVSI *VSI, void *PlayerData, GPlayer *TargetData)
{
	return GE_TRUE;
}

//=====================================================================================
//	Door_Spawn
//=====================================================================================
static void Door_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	geWorld_Model	*Model;
	geMotion		*Motion;
	gePath			*Path;
	GPlayer			*Player;
	Door			*FuncDoor;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;

	Player->Control = Door_Control;
	Player->Trigger = Door_Trigger;
	Player->Blocked = Door_Blocked;

	Player->ControlIndex = 1;
	//Player->TriggerIndex = 2;

	Player->Time = 0.0f;

	// Setup the doors local user data (not passed accross network)
	// Grab the model
	FuncDoor = (Door*)ClassData;
	Player->VPos = FuncDoor->Origin;

	Model = FuncDoor->Model;

	if (!Model)
	{
		GenVS_Error( "Door_Spawn:  No model for entity:%s.\n",EntityName);
		return;
	}

	World = GenVSI_GetWorld(VSI);

	//geWorld_GetModelRotationalCenter(World, Model, &Player->VPos);

	// Set the default xform to time 0 of the animation
	Motion = geWorld_ModelGetMotion(Model);

	if (!Motion)
	{
		GenVS_Error( "Door_Spawn:  No motion for model ('%s').\n",EntityName);		
		return;
	}

	Path = geMotion_GetPath(Motion, 0);

	if (!Path)
	{
		GenVS_Error( "Door_Spawn:  No path for model motion ('%s').\n",EntityName);
		return;
	}

	// Put model at default position
	gePath_Sample(Path, 0.0f, &Player->XForm);

	Player->ViewFlags = VIEW_TYPE_MODEL | VIEW_TYPE_TOUCH;
	Player->ViewIndex = GenVSI_ModelToViewIndex(VSI, Model);		// Special index

	// Register the model with the player if all is ok
	// NOTE - If somthing went bad, then this should not be called!!!
	GenVSI_RegisterPlayerModel(VSI, Player, Model);
	
	//return GE_TRUE;
}
//=====================================================================================
//	Plat_Trigger
//=====================================================================================
static geBoolean Plat_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	geVec3d		Pos;
	GPlayer		*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (Player->State == PSTATE_Opened)
		return GE_TRUE;

	Player->State = PSTATE_Opened;
	Player->FrameTime = 0.0f;

	if (Target)
	{
		Pos = Target->XForm.Translation;
	}
	else
	{
		geWorld		*World;

		World = GenVSI_GetWorld(VSI);

		assert(World);

		geWorld_GetModelRotationalCenter(World, Player->Model, &Pos);
		geVec3d_Add(&Pos, &Player->XForm.Translation, &Pos);
	}
		

	GenVSI_PlaySound(VSI, 2, &Pos);

	return GE_TRUE;
}

//=====================================================================================
//	Plat_Spawn
//=====================================================================================
static void Plat_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	geWorld_Model	*Model;
	geMotion		*Motion;
	gePath			*Path;
	GPlayer			*Player;
	MovingPlat		*Plat;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;

	Player->Control = Plat_Control;
	Player->Trigger = Plat_Trigger;
	Player->Time = 0.0f;

	Player->ControlIndex = 3;
	//Player->TriggerIndex = 4;

	// Setup the plats local user data (not passed accross network)
	Plat = (MovingPlat*)ClassData;

	Player->VPos = Plat->Origin;

	// Grab the model
	Model = Plat->Model;

	if (!Model)
	{
		GenVS_Error( "Plat_Spawn:  No model for entity ('%s').\n",EntityName);
		return;
	}

	World = GenVSI_GetWorld(VSI);

	//geWorld_GetModelRotationalCenter(World, Model, &Player->VPos);

	// Set the default xform to time 0 of the animation
	Motion = geWorld_ModelGetMotion(Model);

	if (!Motion)
	{
		GenVS_Error( "Plat_Spawn:  No motion for model ('%s').\n",EntityName);
		return;
	}

	Path = geMotion_GetPath(Motion, 0);

	if (!Path)
	{
		GenVS_Error( "Plat_Spawn:  No path for model motion ('%s').\n",EntityName);
		return;
	}

	// Put model at default position
	gePath_Sample(Path, 0.0f, &Player->XForm);

	Player->ViewFlags = VIEW_TYPE_MODEL | VIEW_TYPE_STANDON;
	Player->ViewIndex = GenVSI_ModelToViewIndex(VSI, Model);		// Special index

	// Register the model with the player if all is ok
	// NOTE - If somthing went bad, then this should not be called!!!
	GenVSI_RegisterPlayerModel(VSI, Player, Model);
	/*
	if (Plat->Loop)
	{
		Player->Trigger(VSI, Player, NULL);
	}
	*/
	//return GE_TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

//=====================================================================================
//	PhysicsObject_Spawn
//=====================================================================================
static void PhysicsObject_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	geWorld_Model				*Model;
	GPlayer						*Player;
	PhysicsObject				*po;
	geWorld						*World;
	geVec3d						Mins;
	geVec3d						Maxs;
	
	Player = (GPlayer*)PlayerData;

	Player->Control = PhysicsObject_Control;
	Player->Trigger = PhysicsObject_Trigger;

	Player->Time = 0.0f;
	
	po = NULL;
	po = (PhysicsObject*)ClassData;

	if (po == NULL)
	{
		GenVS_Error( "PhysicsObject_Spawn:  NULL class data for physics entity '%s'. (at coordinates: x=%f  y=%f  z=%f)\n",
			EntityName,po->Origin.X,po->Origin.Y,po->Origin.Z);
		return;
	}

	Model = po->Model;

	Player->VPos = po->Origin;

	if (!Model)
	{
		GenVS_Error( "PhysicsObject_Spawn:  No model for physics entity '%s'. (at coordinates: x=%f  y=%f  z=%f)\n",
			EntityName,po->Origin.X,po->Origin.Y,po->Origin.Z);
		return;
	}

	World = GenVSI_GetWorld(VSI);

	////////////////////////////////////////////////////////////////////////////////////////////////////

	Player->ViewFlags = VIEW_TYPE_MODEL | VIEW_TYPE_PHYSOB;
	Player->ViewIndex = GenVSI_ModelToViewIndex(VSI, Model);



	#define PHYSOB_MAX_DAMPING		(1.f)
	#define PHYSOB_MIN_DAMPING		(0.f)

	if (po->linearDamping < PHYSOB_MIN_DAMPING)
		po->linearDamping = PHYSOB_MIN_DAMPING;

	else if (po->linearDamping > PHYSOB_MAX_DAMPING)
		po->linearDamping = PHYSOB_MAX_DAMPING;

	if (po->angularDamping < PHYSOB_MIN_DAMPING)
		po->angularDamping = PHYSOB_MIN_DAMPING;

	else if (po->angularDamping > PHYSOB_MAX_DAMPING)
		po->angularDamping = PHYSOB_MAX_DAMPING;


	
	geWorld_ModelGetBBox(World, po->Model, &Mins, &Maxs);

	#pragma message("Mins / maxs should be OK in the next build")
	
	po->stateInfo = gePhysicsObject_Create(&po->Origin,
																po->mass,
																po->isAffectedByGravity,
																po->respondsToForces,
																po->linearDamping,
																po->angularDamping,
																&Mins,
																&Maxs,
																0.01f);
																//po->physicsScale);

	if (po->stateInfo == NULL)
	{
		GenVS_Error( "PhysicsObject_Spawn:  gePhysicsObject_Create failed for physics entity '%s'\n",EntityName);
		return;
	}

	Player->userData = po->stateInfo;
	

	// Register the model with the player if all is ok
	// NOTE - If somthing went bad, then this should not be called!!!
	GenVSI_RegisterPlayerModel(VSI, Player, Model);
}

static void PhysicsObject_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData)
{
	GPlayer								*Player;
	PhysicsObject						*po;

	Player = (GPlayer*)PlayerData;
	po = (PhysicsObject*)ClassData;

	if (po)
		if (po->stateInfo)
			{
				gePhysicsObject_Destroy(&po->stateInfo);
			}
}

static geBoolean PhysicsObject_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *Target, void *data)
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// PlayerData actually points to the PhysicsObject.
	// Target is R. Havoc.
	// Makes sense, doesn't it?					:^D

	GPlayer		*Player;
	PhysicsObject* poPtr;
	gePhysicsObject* podPtr;
	GE_Collision* collisionInfo;
	float velMagN;
	geVec3d radiusVector;
	geVec3d force;
	geVec3d poCOM;
	int activeConfigIndex;
	float scale;

	Player = (GPlayer*)PlayerData;
	poPtr = (PhysicsObject*)Player->ClassData;
	podPtr = (gePhysicsObject*)Player->userData;
	collisionInfo = (GE_Collision*)data;


	scale = gePhysicsObject_GetPhysicsScale(podPtr);
	activeConfigIndex = gePhysicsObject_GetActiveConfig(podPtr);
	gePhysicsObject_GetLocationInEditorSpace(podPtr, &poCOM, activeConfigIndex);
	geVec3d_Subtract(&collisionInfo->Impact, &poCOM, &radiusVector);

	velMagN = geVec3d_DotProduct(&Target->Velocity, &collisionInfo->Plane.Normal);

	geVec3d_Scale(&collisionInfo->Plane.Normal, velMagN * 4.f, &force);
	geVec3d_Scale(&radiusVector, scale, &radiusVector);

	gePhysicsObject_ApplyGlobalFrameForce(podPtr, &force, &radiusVector, GE_TRUE, activeConfigIndex);
	
	return GE_TRUE;

}

geVec3d bboxVerts[8] = 
{
	{-1.0f, 1.0f, 1.0f},
	{-1.0f, -1.0f, 1.0f},
	{1.0f, -1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f},
	{-1.0f, 1.0f, -1.0f},
	{-1.0f, -1.0f, -1.0f},
	{1.0f, -1.0f, -1.0f},
	{1.0f, 1.0f, -1.0f}
};

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

static geBoolean PhysicsObject_Control(GenVSI* VSI, void* PlayerData, float Time)
{

	GPlayer* player;
	gePhysicsObject* pod;
	int activeConfigIndex;
	geXForm3d destXForm;
#if 0
	geXForm3d xform;
	geWorld* world;
	geVec3d bbmin, bbmax, bbdims;
	geVec3d tmpVec;
	int i;
	Matrix33 A;
	geVec3d boundingBoxCenter;
	geEntity_EntitySet*	Set;
	geEntity*	Entity;
	geVec3d boundingBoxScale;
#endif

	////////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////////

	player = (GPlayer*)PlayerData;	
	if (player == NULL)
	{	
		GenVS_Error( "PhysicsObject_Control:  invalid player.\n");
		return GE_FALSE;	
	}	

	pod = (gePhysicsObject*)player->userData;
	if (pod == NULL)
	{	
		GenVS_Error( "PhysicsObject_Control:  pod = NULL.\n");
		return GE_FALSE;	
	}
	activeConfigIndex = gePhysicsObject_GetActiveConfig(pod);

	#if 0
	world = GenVSI_GetWorld(VSI);

	geWorld_ModelGetBBox(world, player->Model, &bbmin, &bbmax);	

	gePhysicsObject_GetLocation(pod, &boundingBoxCenter);

	geVec3d_Scale(&bbmin, PHYSOB_SCALE, &bbmin);
	geVec3d_Scale(&bbmax, PHYSOB_SCALE, &bbmax);

	geVec3d_Subtract(&bbmax, &bbmin, &bbdims);
	geVec3d_Scale(&bbdims, 0.5f, &boundingBoxScale);

	gePhysicsObject_GetXForm(pod, &xform);
	Matrix33_ExtractFromXForm3d(&xform, &A);

	////////////////////////////////////////////////////////////////////////////////////////////////////
	// check for penetration of one or more model vertices into the radius of a ForceField entity

	Set = NULL;
	Set = geWorld_GetEntitySet(world, "ForceField");
	if (Set != NULL)
	{
		for	(Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
				 Entity != NULL;
				 Entity = geEntity_EntitySetGetNextEntity(Set, Entity))
		{
			ForceField* ff;
			geVec3d vertPos;
			float forceMultiplier;
			float distToFFCenter;
			float forceMagnitude;
			geVec3d forceDir;
			geVec3d forceToApply, ptOfApplication;
			geVec3d ffOriginInPhysicsSpace;

			ff = geEntity_GetUserData(Entity);
			assert(ff != NULL);

			if (! ff->affectsPhysicsObjects) 
				continue;

			for (i = 0; i < 8; i++)
			{
				tmpVec.X = boundingBoxScale.X * bboxVerts[i].X;
				tmpVec.Y = boundingBoxScale.Y * bboxVerts[i].Y;
				tmpVec.Z = boundingBoxScale.Z * bboxVerts[i].Z;

				Matrix33_MultiplyVec3d(&A, &tmpVec, &vertPos);
				geVec3d_Add(&boundingBoxCenter, &vertPos, &vertPos);

				geVec3d_Scale(&ff->Origin, PHYSOB_SCALE, &ffOriginInPhysicsSpace);

				geVec3d_Subtract(&vertPos, &ffOriginInPhysicsSpace, &forceDir);
				distToFFCenter = geVec3d_Length(&forceDir);

				if (distToFFCenter > ff->radius) continue;

				geVec3d_Normalize(&forceDir);
				
				////////////////////////////////////////////////////////////////////////////////////////////////////
				// figure out how much force to apply based on force field's falloff function

				switch(ff->falloffType)
				{
					case FALLOFF_NONE:
						forceMultiplier = 1.f;
						break;
					case FALLOFF_ONE_OVER_D:					
						forceMultiplier = 1 / distToFFCenter;
						break;
					case FALLOFF_ONE_OVER_DSQUARED:
						forceMultiplier = 1 / (distToFFCenter * distToFFCenter);
						break;
					default:
						forceMultiplier = 1.f;
				}

				forceMagnitude = ff->strength * forceMultiplier * 1000.f * PHYSOB_SCALE;

				geVec3d_Scale(&forceDir, forceMagnitude, &forceToApply);
				geVec3d_Subtract(&vertPos, &boundingBoxCenter, &ptOfApplication);
				gePhysicsObject_ApplyGlobalFrameForce(pod, &forceToApply, &ptOfApplication, GE_TRUE);

			}	// for i	
		} // while
	} // if
	#endif

	// update model's xform
	gePhysicsObject_GetXFormInEditorSpace(pod, &destXForm, activeConfigIndex);
	if (GenVSI_MovePlayerModel(VSI, player, &destXForm))
	{
		//gePhysicsObject_GetXFormInEditorSpace(pod, &xform, activeConfigIndex);
		geXForm3d_Copy(&destXForm, &player->XForm);
	}

	return GE_TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//=====================================================================================
//	PhysicsJoint_Spawn
//=====================================================================================
static void PhysicsJoint_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	GPlayer			*Player;
	PhysicsJoint	*ij;

	Player = (GPlayer*)PlayerData;
	
	ij = NULL;
	ij = (PhysicsJoint*)ClassData;

	if (ij == NULL)
	{
		GenVS_Error( "PhysicsJoint_Spawn:  NULL Class Data ('%s').\n",EntityName);
		return;
	}

	Player->ViewFlags = VIEW_TYPE_LOCAL;

	#define JOINT_MIN_ASSEMBLY_RATE		(0.01f)
	#define JOINT_MAX_ASSEMBLY_RATE		(1.f)

	if (ij->assemblyRate < JOINT_MIN_ASSEMBLY_RATE)
		ij->assemblyRate = JOINT_MIN_ASSEMBLY_RATE;

	else if (ij->assemblyRate > JOINT_MAX_ASSEMBLY_RATE)
		ij->assemblyRate = JOINT_MAX_ASSEMBLY_RATE;

	ij->jointData = NULL;
	ij->jointData = gePhysicsJoint_Create((gePhysicsJoint_Kind)ij->jointType,
																 &ij->Origin,
																 ij->assemblyRate,
																 ij->physicsObject1 ? ij->physicsObject1->stateInfo : NULL,
																 ij->physicsObject2 ? ij->physicsObject2->stateInfo : NULL,
																 0.01f);
																 //ij->physicsScale);
	if (ij->jointData == NULL)
	{
		GenVS_Error( "PhysicsJoint_Spawn:  Couldn't Create('%s').\n",EntityName);
		return;
	}

	//return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
static void PhysicsJoint_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData)
{
	GPlayer			*Player;
	PhysicsJoint	*ij;

	Player = (GPlayer*)PlayerData;
	
	ij = NULL;
	ij = (PhysicsJoint*)ClassData;
	
	assert(ij->jointData);

	gePhysicsJoint_Destroy(&ij->jointData);
}

//=====================================================================================
//=====================================================================================
static geBoolean PhysicsJoint_Trigger(GenVSI* VSI, void* PlayerData, void* TargetData, void* data)
{
	return GE_TRUE;
}

static geBoolean PhysicsJoint_Control(GenVSI* VSI, void* PlayerData, float Time)
{
	return GE_TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
//=====================================================================================
//	PhysicalSystem_Spawn
//=====================================================================================
static void PhysicalSystem_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	GPlayer									*Player;
	PhysicalSystem					*ips;
	PhysicsObject						*Object;
	PhysicsObject						*Object2;
	PhysicsJoint										*PhysicsJoint;

	Player = NULL;
	Player = (GPlayer*)PlayerData;
	if (Player == NULL)
	{
		GenVS_Error( "PhysicalSystem_Spawn:  NULL Player ('%s').\n",EntityName);
		return;
	}

	Player->Control = PhysicalSystem_Control;

	if (Player->Control == NULL)
	{
		GenVS_Error( "PhysicalSystem_Spawn:  NULL Player control ('%s').\n",EntityName);
		return;
	}

	Player->Blocked = NULL;
	Player->Trigger = NULL;

	Player->ViewFlags = VIEW_TYPE_LOCAL;
	Player->ViewIndex = VIEW_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	ips = NULL;
	ips = (PhysicalSystem*)ClassData;	

	if (ips == NULL)
	{
		GenVS_Error( "PhysicalSystem_Spawn:  NULL Class Data  ('%s').\n",EntityName);
		return;
	}

	ips->physsysData = NULL;

	ips->physsysData = gePhysicsSystem_Create();
	if (ips->physsysData == NULL)
	{
		GenVS_Error( "PhysicalSystem_Spawn:  Couldn't Create('%s').\n",EntityName);
		return;
	}
	Object = ips->physicsObjectListHeadPtr;
	Object2 = Object;
	while	(Object)
	{
		if	(gePhysicsSystem_AddObject(ips->physsysData, Object->stateInfo) == GE_FALSE)
		{
			GenVS_Error( "PhysicalSystem_Spawn:  Couldn't AddObject('%s').\n",EntityName);
			return;
		}
		Object = Object->Next;
		if (Object2)
			{
				Object2 = Object2->Next;
				if (Object2)
					{
						Object2=Object2->Next;
						if (Object2)
							{
								if (Object2==Object)
									{
										GenVS_Error("PhysicalSystem_Spawn:  Detected circular linked list in Next field('%s')\n",EntityName);
										return;
									}
							}
					}
			}
	}

	PhysicsJoint = ips->jointListHeadPtr;
	while	(PhysicsJoint)
	{
		if	(PhysicsJoint->jointData == NULL)
		{
			GenVS_Error( "PhysicalSystem_Spawn:  Null jointData ('%s').\n",EntityName);
			return;
		}

		if	(gePhysicsSystem_AddJoint(ips->physsysData, PhysicsJoint->jointData) == GE_FALSE)
		{
			GenVS_Error( "PhysicalSystem_Spawn:  Couldn't AddPhysicsJoint('%s').\n",EntityName);
			return;
		}
		PhysicsJoint = PhysicsJoint->Next;
	}

	//return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
static void PhysicalSystem_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData)
{
	GPlayer				*Player;
	PhysicalSystem		*ips;

	Player = (GPlayer*)PlayerData;

	ips = (PhysicalSystem*)ClassData;	

	assert(ips);

	assert(ips->physsysData);

	gePhysicsSystem_Destroy(&ips->physsysData);
}

//=====================================================================================
//=====================================================================================
static geBoolean PhysicalSystem_Trigger(GenVSI* VSI, void* PlayerData, void* TargetData, void* data)
{
	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
static geBoolean PhysicalSystem_Control(GenVSI* VSI, void* PlayerData, float Time)
{
	GPlayer* player;
	PhysicalSystem* psPtr;

	player = (GPlayer*)PlayerData;
	psPtr = (PhysicalSystem*)player->ClassData;

	if (!gePhysicsSystem_Iterate(psPtr->physsysData, Time))
	{
		GenVS_Error( "PhysicalSystem_Control: Iterate() failed.\n");
		return GE_FALSE;
	}	

	return GE_TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// force field-related stuff

static void ForceField_Spawn(GenVSI* VSI, void* PlayerData, void* Class, char *EntityName)
{
	GPlayer* player;
	ForceField* ff;

	player = (GPlayer*)PlayerData;
	if (player == NULL)
	{
		GenVS_Error( "ForceField_Spawn: player = NULL  ('%s').\n",EntityName);
		return;
	}

	ff = (ForceField*)player->ClassData;
	if (ff == NULL)
	{
		GenVS_Error( "ForceField_Spawn: ff = NULL  ('%s').\n",EntityName);
		return;
	}

	player->ViewFlags = VIEW_TYPE_LOCAL;

	player->Blocked = NULL;
	player->Control = ForceField_Control;
	player->Trigger = ForceField_Trigger;

	//return GE_TRUE;
}

static geBoolean ForceField_Trigger(GenVSI* VSI, void* PlayerData, GPlayer* TargetData, void* data)
{
	
	return GE_TRUE;
}

static geBoolean ForceField_Control(GenVSI* VSI, void* PlayerData, float Time)
{
	return GE_TRUE;
}

/*
//=====================================================================================
//	Spawn_AmbientSound
//=====================================================================================
static geBoolean Spawn_AmbientSound(Server_Server *Server, Server_Player *Player, void *ClassData)
{
	Player->SoundIndex = SOUND_INDEX_WATERFALL;

}
*/

//=====================================================================================
//	SqueezeVector
//=====================================================================================
void SqueezeVector(geVec3d *Vect, float Epsilon)
{
	if (Vect->X > -Epsilon && Vect->X < Epsilon)
		Vect->X = 0.0f;
	if (Vect->Y > -Epsilon && Vect->Y < Epsilon)
		Vect->Y = 0.0f;
	if (Vect->Z > -Epsilon && Vect->Z < Epsilon)
		Vect->Z = 0.0f;
}
		
//=====================================================================================
//	ClampVector
//=====================================================================================
void ClampVector(geVec3d *Vect, float Epsilon)
{
	if (Vect->X > Epsilon)
		Vect->X = Epsilon;
	if (Vect->Y > Epsilon)
		Vect->Y = Epsilon;
	if (Vect->Z > Epsilon)
		Vect->Z = Epsilon;

	if (Vect->X < -Epsilon)
		Vect->X = -Epsilon;
	if (Vect->Y < -Epsilon)
		Vect->Y = -Epsilon;
	if (Vect->Z < -Epsilon)
		Vect->Z = -Epsilon;
}

//=====================================================================================
//	ReflectVelocity
//=====================================================================================
void ReflectVelocity(geVec3d *In, geVec3d *Normal, geVec3d *Out, float Scale)
{
	float	Reflect;
	
	Reflect = geVec3d_DotProduct(In, Normal) * Scale;

	Out->X = In->X - Normal->X*Reflect;
	Out->Y = In->Y - Normal->Y*Reflect;
	Out->Z = In->Z - Normal->Z*Reflect;

	SqueezeVector(Out, 0.1f);
}

//=====================================================================================
//	IsKeyDown
//=====================================================================================
static geBoolean IsKeyDown(int KeyCode)
{
	if (GetAsyncKeyState(KeyCode) & 0x8000)
		return GE_TRUE;

	return GE_FALSE;
}

//=====================================================================================
//	XFormFromVector
//=====================================================================================
BOOL XFormFromVector(const geVec3d *Source, const geVec3d *Target, float Roll, geXForm3d *Out)
{
	geVec3d		p1, p2, Vect;
	geVec3d		Origin = {0.0f, 0.0f, 0.0f};

	geVec3d_Subtract(Source, Target, &Vect);

	if (geVec3d_Compare(&Vect, &Origin, 0.05f))
	{
		Vect.Y = -1.0f;
	}

	// First clear the xform
	geXForm3d_SetIdentity(Out);

	geVec3d_Normalize(&Vect);

	// Now set the IN vector
	Out->AZ = Vect.X;
	Out->BZ = Vect.Y;
	Out->CZ = Vect.Z;

	// Get a straight up vector
	p2.X = 0.0f;
	p2.Y = 1.0f;
	p2.Z = 0.0f;

	// Use it with the in vector to get the RIGHT vector
	geVec3d_CrossProduct(&p2, &Vect, &p1);
	geVec3d_Normalize(&p1);

	// Put the RIGHT vector in the matrix
	Out->AX = p1.X;
	Out->BX = p1.Y;
	Out->CX = p1.Z;

	// Now use the RIGHT vector with the IN vector to get the real UP vector
	geVec3d_CrossProduct(&Vect, &p1, &p2);
	geVec3d_Normalize(&p2);

	// Put the UP vector in the matrix
	Out->AY = p2.X;
	Out->BY = p2.Y;
	Out->CY = p2.Z;
	
	// Put the translation in...
	Out->Translation = *Source;

	return TRUE;
}

//=====================================================================================
//	SetupPlayerXForm
//=====================================================================================
void SetupPlayerXForm(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			Pos;
	GPlayer			*Player;
	GenVSI_CMove	*Move;

	Player = (GPlayer*)PlayerData;

	assert(Player->ClientHandle != CLIENT_NULL_HANDLE);

	Move = GenVSI_GetClientMove(VSI, Player->ClientHandle);

	assert(Move);
	
	Pos = Player->XForm.Translation;

	// Clear the matrix
	geXForm3d_SetIdentity(&Player->XForm);

	// Rotate then translate.
	geXForm3d_RotateZ(&Player->XForm, Move->Angles.Z+Player->Roll);

	geXForm3d_RotateX(&Player->XForm, Move->Angles.X);
	geXForm3d_RotateY(&Player->XForm, Move->Angles.Y);
	
	geXForm3d_Translate(&Player->XForm, Pos.X, Pos.Y, Pos.Z);

}

//=====================================================================================
//	AnimatePlayer
//=====================================================================================
geBoolean AnimatePlayer(GenVSI *VSI, void *PlayerData, uint16 MotionIndex, float Speed, geBoolean Loop)
{
	float		StartTime, EndTime, DeltaT;
	geBoolean	Looped;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	Looped = GE_FALSE;

	GenVSI_GetPlayerTimeExtents(VSI, Player, MotionIndex, &StartTime, &EndTime);

	if (Speed > 0)
	{
		Player->FrameTime += Speed;

		DeltaT = EndTime - StartTime;

		if (Player->FrameTime >= EndTime)
		{
			if (Loop)
				Player->FrameTime -= DeltaT;
			else
				Player->FrameTime = EndTime;
			Looped = GE_TRUE;
		}
	}
	else if (Speed < 0)
	{
		Player->FrameTime += Speed;

		DeltaT = EndTime - StartTime;

		if (Player->FrameTime <= StartTime)
		{
			if (Loop)
				Player->FrameTime += DeltaT;
			else
				Player->FrameTime = StartTime;
			Looped = GE_TRUE;
		}
	}

	return Looped;
}

//=====================================================================================
//	AnimatePlayer
//=====================================================================================
geBoolean AnimatePlayer2(GenVSI *VSI, void *PlayerData, int32 MotionSlot, float Speed, geBoolean Loop)
{
	float		StartTime, EndTime, DeltaT;
	geBoolean	Looped;
	GPlayer		*Player;
	uint16		MotionIndex;
	float		MotionTime;

	Player = (GPlayer*)PlayerData;

	assert(MotionSlot < Player->NumMotionData);

	Looped = GE_FALSE;

	MotionIndex = Player->MotionData[MotionSlot].MotionIndex;
	MotionTime = Player->MotionData[MotionSlot].MotionTime;

	GenVSI_GetPlayerTimeExtents(VSI, Player, MotionIndex, &StartTime, &EndTime);

	if (Speed > 0)
	{
		MotionTime += Speed;

		DeltaT = EndTime - StartTime;

		if (MotionTime >= EndTime)
		{
			if (Loop)
				MotionTime -= DeltaT;
			else
				MotionTime = EndTime;
			Looped = GE_TRUE;
		}
	}
	else if (Speed < 0)
	{
		MotionTime += Speed;

		DeltaT = EndTime - StartTime;

		if (MotionTime <= StartTime)
		{
			if (Loop)
				MotionTime += DeltaT;
			else
				MotionTime = StartTime;
			Looped = GE_TRUE;
		}
	}

	Player->MotionData[MotionSlot].MotionTime = MotionTime;

	return Looped;
}


//=====================================================================================
//	UpdateClientInventory
//=====================================================================================
void UpdateClientInventory(GenVSI *VSI, GPlayer *Player, int32 Slot)
{
	uint16		Amount;
	geBoolean	Has;

	assert(Player->Inventory[Player->CurrentWeapon] >= 0 && Player->Inventory[Player->CurrentWeapon] <= 65535);

	Amount = (uint16)Player->Inventory[Slot];
	Has = Player->InventoryHas[Slot];

	GenVSI_SetClientInventory(VSI, Player->ClientHandle, Slot, Amount, Has);
}