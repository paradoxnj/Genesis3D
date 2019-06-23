/****************************************************************************************/
/*  Weapons.c                                                                           */
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

#include "GMain.h"
#include "Quatern.h"

geBoolean		Client_Control(GenVSI *VSI, void *PlayerData, float Time);
//void			*GetDMSpawn(GenVSI *VSI);
void			SetupPlayerXForm(GenVSI *VSI, void *PlayerData, float Time);

#define BLASTER_RADIUS			150.0f
#define BLASTER_POWER			250.0f
#define BLASTER_POWER_RADIUS	200.0f

#define GRENADE_RADIUS			350.0f
#define	GRENADE_POWER			400.0f	
#define	GRENADE_POWER_RADIUS	400.0f	

#define ROCKET_RADIUS			320.0f
#define	ROCKET_POWER			500.0f
#define	ROCKET_POWER_RADIUS		450.0f

//=====================================================================================
//	SwitchToNextBestWeapon
//=====================================================================================
void SwitchToNextBestWeapon(GenVSI *VSI, void *PlayerData)
{
	int32			i;
	int32			BestWeapon;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	//assert(Player->ClientHandle != CLIENT_NULL_HANDLE);

	BestWeapon = -1;
	
	for (i=0; i<5; i++)
	{
		Player->CurrentWeapon++;
		Player->CurrentWeapon %= 4;
		
		if ((!Player->InventoryHas[Player->CurrentWeapon] || Player->Inventory[Player->CurrentWeapon] <= 0) && Player->CurrentWeapon != 0)
			continue;

		if (Player->CurrentWeapon > BestWeapon)
			BestWeapon = Player->CurrentWeapon;
	}

	assert(BestWeapon >= 0 && BestWeapon < 5);

	if (BestWeapon == -1)		// Should'nt happen, but... Assert will get it if it does...
		BestWeapon = 0;
	
	if (Player->ClientHandle != CLIENT_NULL_HANDLE)
		GenVSI_SetClientWeapon(VSI, Player->ClientHandle, (uint16)BestWeapon);
}

//=====================================================================================
//	ValidateWeapon
//=====================================================================================
void ValidateWeapon(GenVSI *VSI, void *PlayerData)
{
	int32			i;
	GPlayer			*Player;

	assert(PlayerData);
	
	Player = (GPlayer*)PlayerData;

	//assert(Player->ClientHandle != CLIENT_NULL_HANDLE);

	for (i=0; i<5; i++)
	{
		if (!Player->InventoryHas[Player->CurrentWeapon] && Player->CurrentWeapon != 0)
		{
			Player->CurrentWeapon++;
			Player->CurrentWeapon %= 4;
			continue;
		}
		
		assert(Player->CurrentWeapon >= 0 && Player->CurrentWeapon <= 3);

		if (Player->ClientHandle != CLIENT_NULL_HANDLE)
			GenVSI_SetClientWeapon(VSI, Player->ClientHandle, Player->CurrentWeapon);
		return;
	}
	
	// Should'nt get here!!!!!  There is allways the default weapon...
	assert(!"No best weapon!!!");

	if (Player->ClientHandle != CLIENT_NULL_HANDLE)
		GenVSI_SetClientWeapon(VSI, Player->ClientHandle, 0);
}

//=====================================================================================
//	Player_Dying
//=====================================================================================
static geBoolean Player_Dying(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	GenVSI_CMove	*Move;

	Player = (GPlayer*)PlayerData;

	assert(Player->ClientHandle != CLIENT_NULL_HANDLE);

	Player->MotionIndex = ACTOR_MOTION_PLAYER_DIE;
	AnimatePlayer(VSI, Player, Player->MotionIndex, Time, GE_FALSE);

	Move = GenVSI_GetClientMove(VSI, Player->ClientHandle);

	Player->Roll -= 2.0f*Time;

	if (Player->Roll < -(3.14159f/2.0f))
	{
		Player->Roll = -(3.14159f/2.0f);

		if (Move->ButtonBits & HOST_BUTTON_FIRE)
		{
			GPlayer	*DMStart;
			
			DMStart = (GPlayer*)GetDMSpawn(VSI);

			Player->Roll = 0.0f;		// Make him stand right side up again
			Player->Control = Client_Control;
			//Player->Dead = GE_FALSE;
			Player->State = PSTATE_Normal;		// Back to normal now...
			Player->ViewIndex = 0;
			Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
			Player->FrameTime = 0.0f;

			Player->ViewFlags &= ~VIEW_TYPE_FORCE_XFORM;

			Player->XForm = DMStart->XForm;
			GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_PLAYER_SPAWN);

			return GE_TRUE;
		}
	}

	SetupPlayerXForm(VSI, Player, Time);

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	return GE_TRUE;
}

#define NUM_DIE_MSGS		4
static int32 CurrentDieMsg = 0;

static char DieMsgs[NUM_DIE_MSGS][128] = 
{
	" can't handle life anymore",
	" is a loser",
	" puts a gun to his head",
	" had a nervous breakdown",
};

#define NUM_KILL_MSGS		5
static int32 CurrentKillMsg = 0;

static char KillMsgs[NUM_KILL_MSGS<<1][128] = 
{
	" hit ", " with an ugly stick",
	" lays the smack down on ", "...",
	" cracked ", "'s head open",
	" hit ", " with an ugly forest",
	" put a load in ", "'s head",
};

static geBoolean Bot_Dying(GenVSI *VSI, void *PlayerData, float elapsedTime)
{
	return GE_TRUE; 
}

static geBoolean Bot_ActorDying(GenVSI *VSI, void *PlayerData, float elapsedTime)
{
	return GE_TRUE;
}

//=====================================================================================
//	KillPlayer
//=====================================================================================
geBoolean KillPlayer(GenVSI *VSI, void *PlayerData, void *TargetData, float Time)
{
	GPlayer				*Owner;
	GPlayer				*Player;
	GPlayer				*Target;
    
	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	Target->Health = 100;

	if (Target->DammageFlags == 0)
		return GE_TRUE;

	// Actors don't have a client handle
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
		GenVSI_SetClientHealth(VSI, Target->ClientHandle, Target->Health);

	if (Player && Player->Owner->ClientHandle != CLIENT_NULL_HANDLE)
	{
		Owner = Player->Owner;		
	
		if (!Owner)
		{
			GenVSI_ConsolePrintf(VSI, "KillPlayer:  Stray weapon!\n");
			return GE_TRUE;
		}

		if (Owner == Target)	// We just killed ourself!!
		{
			GenVSI_ConsoleHeaderPrintf(VSI, Owner->ClientHandle, GE_TRUE, "%s%s", Owner->ClassName, DieMsgs[CurrentDieMsg]);
			Owner->Score--;

			CurrentDieMsg++;
			CurrentDieMsg%=NUM_DIE_MSGS;
		}
		else
		{
			GenVSI_ConsoleHeaderPrintf(VSI, Owner->ClientHandle, GE_TRUE, "%s%s%s%s", Owner->ClassName, KillMsgs[CurrentKillMsg<<1], Target->ClassName, KillMsgs[(CurrentKillMsg<<1)+1]);
			Owner->Score++;

			CurrentKillMsg++;
			CurrentKillMsg%=NUM_KILL_MSGS;
		}

		GenVSI_SetClientScore(VSI, Owner->ClientHandle, Owner->Score);
	}

	//Target->Dead = GE_TRUE;
	Target->State = PSTATE_Dead;
	//??bot Hack - talk to john
	if (strcmp(Target->ClassName, "Bot_Actor") == 0)
		Target->Control = Bot_ActorDying;
	else
	if (GenVSI_IsClientBot(VSI, Target->ClientHandle))
		Target->Control = Bot_Dying;
	else
		Target->Control = Player_Dying;
	Target->Velocity.Y += 700.0f;
	Target->MotionIndex = ACTOR_MOTION_PLAYER_DIE;
	Target->FrameTime = 0.0f;

	// Play dying sound...
	GenVSI_PlaySound(VSI, SOUND_INDEX_DIE, &Target->XForm.Translation);

	Target->ViewFlags |= VIEW_TYPE_FORCE_XFORM;

	// Update the targets inventory (won't have nothin anymore...)
	{
		int32		i;

		// TODO:  Create a new player, and copy over inventory for a backpack...
		for (i=1; i<MAX_PLAYER_ITEMS; i++)
		{
			Target->Inventory[i] = 0;
			Target->InventoryHas[i] = GE_FALSE;
			// Actors don't have a client handle
			if (Target->ClientHandle != CLIENT_NULL_HANDLE)
				UpdateClientInventory(VSI, Target, i);
		}

		Target->CurrentWeapon = 0;
		// Actors don't have a client handle
		if (Target->ClientHandle != CLIENT_NULL_HANDLE)
			GenVSI_SetClientWeapon(VSI, Target->ClientHandle, Target->CurrentWeapon);
	}

	return GE_TRUE;
}

//=====================================================================================
//	DammagePlayer
//=====================================================================================
geBoolean DammagePlayer(GenVSI *VSI, void *PlayerData, void *TargetData, int32 Amount, float Power, float Time)
{
	geVec3d		Vect, Pos;
	int32		DammageSound;
	GPlayer		*Player, *Target;
	float		Amountf, ArmorRatio;
	extern geBoolean GodMode;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (VSI->Mode != MODE_Server)
		return GE_TRUE;

	//if (Target->Dead)
	//	return GE_TRUE;
	if (Target->State == PSTATE_Dead)				// Ignore dead players...
		return GE_TRUE;
	if (Target->State == PSTATE_DeadOnGround)		// Ignore dead players...
		return GE_TRUE;

	if (Target->DammageFlags == 0)
		return GE_TRUE;
	//if (Target->ClientHandle == CLIENT_NULL_HANDLE)			// For now, only hurt client players for deathmatch...
	//	return GE_TRUE;

	Amountf = (float)Amount;

	// Dammage -= ((Armor/MAX_ARMOR)*0.5)*Dammaga
	ArmorRatio = (float)Target->Inventory[ITEM_ARMOR] / (float)MAX_ARMOR;
	Amountf -= (ArmorRatio * MAX_ARMOR_PROTECTION) * Amountf;

	if (Amountf < 0.0f)
		Amountf = 0.0f;

	Target->Inventory[ITEM_ARMOR] -= Amount;

	if (Target->Inventory[ITEM_ARMOR] < 0)
		Target->Inventory[ITEM_ARMOR] = 0;

	// Actors don't have a client handle
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)		
		UpdateClientInventory(VSI, Target, ITEM_ARMOR);

	// debug godmode
	if (!GodMode || GenVSI_IsClientBot(VSI, Target->ClientHandle))
		Target->Health -= (int32)Amountf;
	else
		return GE_FALSE;

	if (Player)
	{
		if (Player->Owner == Target)	// Don't hurt ourselves as much
			Amountf *= 0.5f;

		// Knock the target opposite of the player hitting the target
		Pos = Target->XForm.Translation;
		Pos.Y += 100.0f;
		geVec3d_Subtract(&Pos, &Player->XForm.Translation, &Vect);
		geVec3d_Normalize(&Vect);
	
		geVec3d_AddScaled(&Target->Velocity, &Vect, Power, &Target->Velocity);
	}

	if (Target->Health <= 0)
	{
		if (!KillPlayer(VSI, Player, Target, Time))
			return GE_FALSE;
	}

	// Actors don't have a client handle
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)		
		GenVSI_SetClientHealth(VSI, Target->ClientHandle, Target->Health);

	Target->MotionIndex = ACTOR_MOTION_PLAYER_HIT;
	Target->FrameTime = 0.0f;

	if	((rand() % 100) > 90)
		DammageSound = (rand()%2) + SOUND_INDEX_HURT3;
	else
		DammageSound = (rand()%2) + SOUND_INDEX_HURT1;

	GenVSI_PlaySound(VSI, (uint16)DammageSound, &Target->XForm.Translation);

	return GE_TRUE;
}

//=====================================================================================
//	RadiusDammage
//=====================================================================================
static geBoolean RadiusDammage(GenVSI *VSI, void *PlayerData, int32 Amount, float Power, float Radius, float Time)
{
	GPlayer			*Hit;
	geVec3d			Pos, Pos2;
	GPlayer			*Player;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;

	if (VSI->Mode != MODE_Server)
		return GE_TRUE;
	
	Pos = Player->XForm.Translation;

	// Sweep through all the players in radius and do radius dammage...
	Hit = NULL;

	World = GenVSI_GetWorld(VSI);

	while (1)
	{
		GE_Collision Collision;

		Hit = static_cast<GPlayer*>(GenVSI_GetNextPlayerInRadius(VSI, Player, Hit, NULL, Radius));

		if (!Hit)
			break;

		//Console_Printf(Server->Host->Console, "Found player in radius: %s\n", Hit->ClassName);
		//if (Hit->ClientHandle == CLIENT_NULL_HANDLE)
		//	continue;
		if (!(Hit->DammageFlags & DAMMAGE_TYPE_RADIUS))
			continue;

		Pos2 = Hit->XForm.Translation;

		if (World)
			if (geWorld_Collision(World, NULL, NULL, &Pos, &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision))
				continue;

		if (Hit == Player->Owner)		// To allow rocket jumps
		{
			Power *= 2.65f;
		}

		if (!DammagePlayer(VSI, Player, Hit, Amount, Power, Time))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
geBoolean Grenade_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			NewPos;
	geVec3d			Mins;
	geVec3d			Maxs;
	GE_Contents		Contents;
	GPlayer			*Player;
	geWorld			*World;
	GE_Collision	Col;
	CData			Data;
	GE_CollisionCB	*ThisSelfCollisionCB = nullptr;


	World = GenVSI_GetWorld(VSI);
	
	Player = (GPlayer*)PlayerData;

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	if (Player->Time > 1.0f)		// Give the user 1 sec till it starts to collide with himself...
		ThisSelfCollisionCB = SelfCollisionCB2;
	else
		ThisSelfCollisionCB = SelfCollisionCB;

	Player->Time += Time;
	
	Data.VSI = VSI;
	Data.Player = Player;

	geVec3d_AddScaled(&Player->XForm.Translation, &Player->Velocity, Time, &NewPos);

	// Make bounce sound
	if (geWorld_Collision(World, &Mins, &Maxs, &Player->XForm.Translation, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, ThisSelfCollisionCB, &Data, &Col))
	{
		geVec3d		Vect;

		geVec3d_Subtract(&Col.Impact, &NewPos, &Vect);

		if (geVec3d_Length(&Vect) > 5.0f)
			GenVSI_PlaySound(VSI, 3, &Player->XForm.Translation);
	}

	if (geWorld_Collision(World, &Mins, &Maxs, &Player->XForm.Translation, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_ACTORS, 0xffffffff, ThisSelfCollisionCB, &Data, &Col))
	{
		if (Col.Actor)
		{
			GPlayer		*PlayerHit;

			PlayerHit = (GPlayer*)GenVSI_ActorToPlayer(VSI, Col.Actor);

			if (PlayerHit)
			{
				// Do radius dammage
				DammagePlayer(VSI, Player, PlayerHit, 25, GRENADE_POWER, Time);
				RadiusDammage(VSI, Player, 20, GRENADE_POWER_RADIUS, GRENADE_RADIUS, Time);

				Player->XForm.Translation.Y += 40.0f;
				GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE1, &Player->XForm.Translation, 1);

			}

			// Remove the player...
			GenVSI_DestroyPlayer(VSI, PlayerData);
			return GE_TRUE;
		}
	}
	
	if (geWorld_GetContents(World, &Player->XForm.Translation, &Player->Mins, &Player->Maxs, GE_COLLIDE_ACTORS, 0xffffffff, ThisSelfCollisionCB, &Data, &Contents))
	{
		if (Contents.Actor)
		{
			GPlayer		*PlayerHit;

			PlayerHit = (GPlayer*)GenVSI_ActorToPlayer(VSI, Contents.Actor);

			if (PlayerHit)
			{
				// Do radius dammage
				DammagePlayer(VSI, Player, PlayerHit, 25, GRENADE_POWER, Time);
				RadiusDammage(VSI, Player, 20, GRENADE_POWER_RADIUS, GRENADE_RADIUS, Time);
				
				Player->XForm.Translation.Y += 40.0f;
				GenVSI_SpawnFx(VSI,  (uint8)FX_EXPLODE1, &Player->XForm.Translation, 1);
			
				// Remove the player...
				GenVSI_DestroyPlayer(VSI, PlayerData);
				return GE_TRUE;
			}
		}
	}
	
	// Run the physics on the Grenade
	PlayerPhysics(VSI, Player, 0.75f, 0.65f, 2.1f, 1000.0f, 1.8f, GE_TRUE, Time);

	Player->VPos = Player->XForm.Translation;
	
	if (Player->Time >= 3.5f)		// Grenade last for 3 secs
	{
		// Do radius dammage
		RadiusDammage(VSI, Player, 35, GRENADE_POWER_RADIUS, GRENADE_RADIUS, Time);

		Player->XForm.Translation.Y += 40.0f;
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE1, &Player->XForm.Translation, 1);
		// Remove the player...
		GenVSI_DestroyPlayer(VSI, Player);

	}

	return GE_TRUE;
}

//=====================================================================================
//	FireGrenade
//=====================================================================================
void FireGrenade(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Weapon;
	geVec3d			In;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;
	
	if (!Player->Inventory[ITEM_GRENADES] || !Player->InventoryHas[ITEM_GRENADES])
	{
		SwitchToNextBestWeapon(VSI, Player);
		return;
	}
	
	Weapon = (GPlayer*)GenVSI_SpawnPlayer(VSI, "Weapon_Grenade");

	if (!Weapon)
	{
		GenVSI_ConsolePrintf(VSI, "FireGrenade:  Failed to spawn player.\n");
		return;
	}

	Player->Inventory[ITEM_GRENADES]--;
	
	if (Player->Inventory[ITEM_GRENADES] < 0)
	{
		SwitchToNextBestWeapon(VSI, Player);
		Player->Inventory[ITEM_GRENADES] = 0;
	}
	
	//UpdateClientInventory(VSI, Player, ITEM_GRENADES);

	// Setup the callbacks for this weapon
	Weapon->Control = Grenade_Control;
	Weapon->Blocked = NULL;
	Weapon->Trigger = NULL;
	Weapon->Time = 0.0f;
	Weapon->Owner = Player;
	Weapon->PingTime = Player->PingTime;

	Weapon->ControlIndex = 7;

	Weapon->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_LIGHT | VIEW_TYPE_COLLIDEMODEL;
	Weapon->ViewIndex = ACTOR_INDEX_GRENADE_AMMO;
	Weapon->FxFlags = (uint16)FX_SMOKE_TRAIL;
	Weapon->Scale = 1.0f;
	Weapon->MotionIndex = MOTION_INDEX_NONE;
	
	geVec3d_Set(&Weapon->Mins, -10.0f, -10.0f, -10.0f);
	geVec3d_Set(&Weapon->Maxs,  10.0f,  10.0f,  10.0f);

	Weapon->XForm = Player->XForm;

	geVec3d_Add(&Weapon->XForm.Translation, &Player->GunOffset, &Weapon->XForm.Translation);

	Weapon->XForm.Translation.Y -= 30.0f;

	geXForm3d_GetIn(&Weapon->XForm, &In);

	In.Y += 0.2f;
	geVec3d_Scale(&In, 1100.0f, &Weapon->Velocity);

	GenVSI_PlaySound(VSI, SOUND_INDEX_GRENADE, &Player->XForm.Translation);

	Player->NextWeaponTime = GenVSI_GetTime(VSI) + 0.5f;
	//Player->NextThinkTime = Server->Host->Time+0.5f;		// 0.5 sec between firing...

	Player->VPos = Player->XForm.Translation;
}

//=====================================================================================
//	Rocket_Control
//=====================================================================================
geBoolean Rocket_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			In, Front, Back;
	geXForm3d		DestXForm;
	GE_Collision	Collide;
	float			Speed;
	GPlayer			*Player;
	geWorld			*World;
	CData			Data;

	Player = (GPlayer*)PlayerData;

	Player->Time += Time;
	
	Speed = Player->Time*3600.0f;

	if (Speed > 4000.0f)
		Speed = 4000.0f;

	geXForm3d_Copy(&Player->XForm, &DestXForm);

	geXForm3d_GetIn(&Player->XForm, &In);
	geVec3d_Scale(&In, -Speed*Time, &In);		// - speed since xform was rotated 180!!!
	geXForm3d_Translate(&DestXForm, In.X, In.Y, In.Z);

	Front = Player->XForm.Translation;
	Back = DestXForm.Translation;

	geXForm3d_Copy(&DestXForm, &Player->XForm);

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Data.VSI = VSI;
	Data.Player = Player;//->Owner;

	if (geWorld_Collision(	World, NULL, NULL, &Front, &Back, 
							GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS|GE_COLLIDE_ACTORS, 
							0xffffffff, SelfCollisionCB, &Data, &Collide))
	{
		Player->XForm.Translation = Collide.Impact;
		geVec3d_AddScaled(&Player->XForm.Translation, &Collide.Plane.Normal, 40.0f, &Player->XForm.Translation);

		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE1, &Player->XForm.Translation, 1);

		if (Collide.Model)
		{
			GPlayer		*Target;

			Target = (GPlayer*)geWorld_ModelGetUserData(Collide.Model);
			if (Target && Target->Trigger && (Target->ViewFlags & VIEW_TYPE_PHYSOB))
			{
				geXForm3d_GetIn(&Player->XForm, &In);
				Player->Velocity = In;
				geVec3d_Scale(&In, -1000.f, &Player->Velocity);
				Target->Trigger(VSI, Target, Player, (void*)&Collide);
			}
		}

		if (Collide.Actor)
		{
			GPlayer		*PlayerHit;

			PlayerHit = (GPlayer*)GenVSI_ActorToPlayer(VSI, Collide.Actor);

			// Only radius dammage will hurt the player that shot the rocket...
			if (PlayerHit && PlayerHit != Player->Owner)	
				DammagePlayer(VSI, Player, PlayerHit, 35, ROCKET_POWER, Time);
			
			// Do radius dammage
			RadiusDammage(VSI, Player, 30, ROCKET_POWER_RADIUS, ROCKET_RADIUS, Time);

			GenVSI_DestroyPlayer(VSI, PlayerData);
			return GE_TRUE;
		}

		// Do radius dammage
		RadiusDammage(VSI, Player, 40, ROCKET_POWER_RADIUS, ROCKET_RADIUS, Time);
		GenVSI_DestroyPlayer(VSI, PlayerData);

		return GE_TRUE;
	}
	
	Player->VPos = Player->XForm.Translation;

	if (Player->Time >= 20.0f)				// Kill it after 20 seconds
		GenVSI_DestroyPlayer(VSI, PlayerData);

	return GE_TRUE;
}


//=====================================================================================
//	FireRocket
//=====================================================================================
void FireRocket(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Weapon;
	geVec3d			Up, In, Rt, Front, Back, Impact;
	geQuaternion	Quat;
	geXForm3d		RotXForm;
	GE_Collision	Collision;
	geWorld			*World;
	GPlayer			*Player;
	CData			Data;

	Player = (GPlayer*)PlayerData;
	
	if (Player->Inventory[ITEM_ROCKETS] <= 0 || !Player->InventoryHas[ITEM_ROCKETS])
	{
		SwitchToNextBestWeapon(VSI, Player);
		return;
	}
	
	Weapon = static_cast<GPlayer*>(GenVSI_SpawnPlayer(VSI, "Weapon_Rocket"));

	if (!Weapon)
	{
		GenVSI_ConsolePrintf(VSI, "FireRocket:  Failed to add player.\n");
		return;
	}

	Player->Inventory[ITEM_ROCKETS]--;
	
	if (Player->Inventory[ITEM_ROCKETS] < 0)
	{
		SwitchToNextBestWeapon(VSI, Player);
		Player->Inventory[ITEM_ROCKETS] = 0;
	}
	
	//UpdateClientInventory(VSI, Player, ITEM_ROCKETS);

	// Setup the callbacks for this weapon
	Weapon->Control = Rocket_Control;
	Weapon->Blocked = NULL;
	Weapon->Trigger = NULL;
	Weapon->Owner = Player;
	Weapon->Time = 0.0f;
	Weapon->PingTime = Player->PingTime;

	geVec3d_Set(&Weapon->Mins, -10.0f, -10.0f, -10.0f);
	geVec3d_Set(&Weapon->Maxs,  10.0f,  10.0f,  10.0f);

	Weapon->ControlIndex = 6;

	Weapon->ViewFlags = (VIEW_TYPE_ACTOR | VIEW_TYPE_LIGHT);
	Weapon->ViewIndex = ACTOR_INDEX_ROCKET_AMMO;
	Weapon->FxFlags = (uint16)FX_SMOKE_TRAIL;		// Make some smoke follow rocket
	Weapon->Scale = 1.0f;
	Weapon->MotionIndex = MOTION_INDEX_NONE;

	Weapon->XForm = Player->XForm;
	
	geVec3d_Add(&Weapon->XForm.Translation, &Player->GunOffset, &Weapon->XForm.Translation);

	geXForm3d_GetIn(&Weapon->XForm, &In);
	geXForm3d_GetUp(&Weapon->XForm, &Up);
	geXForm3d_GetLeft(&Weapon->XForm, &Rt);

	Front = Weapon->XForm.Translation;

	geVec3d_AddScaled(&Front, &In, 20000.0f, &Back);

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Data.VSI = VSI;
	Data.Player = Player;

	// Find out what direction the rocket needs to go to hit the target in the center of the screen
	if (geWorld_Collision(World, NULL, NULL, &Front, &Back, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0xffffffff, SelfCollisionCB, &Data, &Collision))

		Impact = Collision.Impact;
	else
		Impact = Back;

	// Position the rocket under the lower left arm
	geVec3d_AddScaled(&Front, &In, 70.0f, &Front);
	geVec3d_AddScaled(&Front, &Up, -30.0f, &Front);
	geVec3d_AddScaled(&Front, &Rt, -28.0f, &Front);

	XFormFromVector(&Front, &Impact, 0.0f, &Weapon->XForm);
	
	// HACK!!! Rocket is backwards, so rotate about up vector 180
	// Rotate the thing 180
	Up.X = 0.0f;
	Up.Y = 1.0f;
	Up.Z = 0.0f;
	geQuaternion_SetFromAxisAngle(&Quat, &Up, 3.14159f);
	geQuaternion_ToMatrix(&Quat, &RotXForm);
	geXForm3d_Multiply(&Weapon->XForm, &RotXForm, &Weapon->XForm);

	// Play the "take off" sound
	GenVSI_PlaySound(VSI, 0, &Player->XForm.Translation);

	Player->NextWeaponTime = GenVSI_GetTime(VSI) + 0.8f;

	Player->VPos = Player->XForm.Translation;
}

//=====================================================================================
//	Blaster_Control
//=====================================================================================
geBoolean Blaster_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			In, Front, Back;
	geXForm3d		DestXForm;
	GE_Collision	Collide;
	float			Speed;
	GPlayer			*Player;
	geWorld			*World;
	CData			Data;

	Player = (GPlayer*)PlayerData;

	Player->Time += Time;
	
	Speed = Player->Time*3600.0f;

	if (Speed > 4000.0f)
		Speed = 4000.0f;

	geXForm3d_Copy(&Player->XForm, &DestXForm);

	geXForm3d_GetIn(&Player->XForm, &In);
	geVec3d_Scale(&In, Speed*Time, &In);	
	geXForm3d_Translate(&DestXForm, In.X, In.Y, In.Z);

	geXForm3d_GetIn(&Player->XForm, &In);
	geVec3d_Scale(&In, 500.f, &Player->Velocity);

	Front = Player->XForm.Translation;
	Back = DestXForm.Translation;

	geXForm3d_Copy(&DestXForm, &Player->XForm);

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Data.VSI = VSI;
	Data.Player = Player;

	if (geWorld_Collision(World, NULL, NULL, &Front, &Back, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS|GE_COLLIDE_ACTORS, 0xffffffff, SelfCollisionCB, &Data, &Collide))
	{
		Player->XForm.Translation = Collide.Impact;
		geVec3d_AddScaled(&Player->XForm.Translation, &Collide.Plane.Normal, 40.0f, &Player->XForm.Translation);

		if (Collide.Model)
		{
			GPlayer		*Target;

			Target = (GPlayer*)geWorld_ModelGetUserData(Collide.Model);
			if (Target && Target->Trigger && Target->ViewFlags & VIEW_TYPE_PHYSOB)
			{				
				Target->Trigger(VSI, Target, Player, (void*)&Collide);
			}
		}

		//if (VSI->Mode == MODE_Server)
		{
			if (Collide.Actor)
			{
				GPlayer		*PlayerHit;
				PlayerHit = static_cast<GPlayer*>(GenVSI_ActorToPlayer(VSI, Collide.Actor));

				// Only radius dammage will hurt the player that shot the rocket...
				if (PlayerHit && PlayerHit != Player->Owner)	
					DammagePlayer(VSI, Player, PlayerHit, 15, BLASTER_POWER,Time);
			}
			else
				RadiusDammage(VSI, Player, 10, BLASTER_POWER_RADIUS, BLASTER_RADIUS, Time);
		}

		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_BLASTER_BANG);
		GenVSI_DestroyPlayer(VSI, PlayerData);
	}
	
	//if (Player->Time >= 25.0f)				// Kill it after 20 seconds
	//	GenVSI_DestroyPlayer(VSI, PlayerData);

	Player->VPos = Player->XForm.Translation;

	return GE_TRUE;
}

//=====================================================================================
//	FireBlaster
//=====================================================================================
void FireBlaster(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Weapon;
	geVec3d			Up, In, Rt, Front, Back, Impact;
	GE_Collision	Collision;
	geWorld			*World;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;
	
	Weapon = static_cast<GPlayer*>(GenVSI_SpawnPlayer(VSI, "Weapon_Blaster"));

	if (!Weapon)
	{
		GenVSI_ConsolePrintf(VSI, "FireBlaster:  Failed to add player.\n");
		return;
	}

	// Setup the callbacks for this weapon
	Weapon->Control = Blaster_Control;
	Weapon->Blocked = NULL;
	Weapon->Trigger = NULL;
	Weapon->Owner = Player;
	Weapon->Time = 0.0f;
	Weapon->PingTime = Player->PingTime;

	geVec3d_Set(&Weapon->Mins, -10.0f, -10.0f, -10.0f);
	geVec3d_Set(&Weapon->Maxs,  10.0f,  10.0f,  10.0f);

	Weapon->ControlIndex = 5;

	Weapon->ViewFlags = (VIEW_TYPE_SPRITE | VIEW_TYPE_LIGHT);
	Weapon->ViewIndex = TEXTURE_INDEX_WEAPON2;
	Weapon->Scale = 2.5f;
	Weapon->FxFlags = (uint16)FX_PARTICLE_TRAIL;		// Make some particles follow Weapon_Blaster

	Weapon->XForm = Player->XForm;
	
	geVec3d_Add(&Weapon->XForm.Translation, &Player->GunOffset, &Weapon->XForm.Translation);

	geXForm3d_GetIn(&Weapon->XForm, &In);
	geXForm3d_GetUp(&Weapon->XForm, &Up);
	geXForm3d_GetLeft(&Weapon->XForm, &Rt);

	Front = Weapon->XForm.Translation;

	geVec3d_AddScaled(&Front, &In, 10000.0f, &Back);

	World = GenVSI_GetWorld(VSI);
	assert(World);

	// Find out what direction the rocket needs to go to hit the target in the center of the screen
	if (geWorld_Collision(World, NULL, NULL, &Front, &Back, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_ALL, 0, NULL, NULL, &Collision))
		Impact = Collision.Impact;
	else
		Impact = Back;

	// Position the Weapon_Blaster under the lower left arm
	geVec3d_AddScaled(&Front, &In, 70.0f, &Front);
	geVec3d_AddScaled(&Front, &Up, -10.0f, &Front);
	geVec3d_AddScaled(&Front, &Rt, -10.0f, &Front);

	XFormFromVector(&Front, &Impact, 0.0f, &Weapon->XForm);

	// HACK!!! Rocket is backwards, so rotate about up vector 180
	// Rotate the thing 180

	// Play the "take off" sound
	GenVSI_PlaySound(VSI, SOUND_INDEX_BLASTER, &Player->XForm.Translation);

	Player->NextWeaponTime = GenVSI_GetTime(VSI) + 0.6f;

	Player->VPos = Player->XForm.Translation;
}

//=====================================================================================
//	Shredder_Control
//=====================================================================================
geBoolean Shredder_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	Player->Time += Time;

	if (Player->Time > 0.25f)
	{
		Player->Owner->Weapon = NULL;
		GenVSI_DestroyPlayer(VSI, PlayerData);
		return GE_TRUE;
	}

	Player->XForm = Player->Owner->XForm;

	geVec3d_Add(&Player->XForm.Translation, &Player->Owner->GunOffset, &Player->XForm.Translation);

	Player->VPos = Player->XForm.Translation;

	return GE_TRUE;
}

//=====================================================================================
//	FireShredder
//=====================================================================================
void FireShredder(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Weapon;
	GE_Collision	Collision;
	geVec3d			Front, Back, In;
	GPlayer			*Player;
	geWorld			*World;
	CData			Data;

	Player = (GPlayer*)PlayerData;

	if (Player->Inventory[ITEM_SHREDDER] <= 0 || !Player->InventoryHas[ITEM_SHREDDER])
	{
		SwitchToNextBestWeapon(VSI, Player);
		return;
	}

	if (!Player->Weapon)		// Check to see if allready firing
	{
		Weapon = static_cast<GPlayer*>(GenVSI_SpawnPlayer(VSI, "Weapon_Shredder"));

		if (!Weapon)
		{
			GenVSI_ConsolePrintf(VSI, "FireShredder:  Failed to add player.\n");
			return;
		}
	}
	else
		return;

	Player->Inventory[ITEM_SHREDDER]-= 5;

	if (Player->Inventory[ITEM_SHREDDER] < 0)
	{
		SwitchToNextBestWeapon(VSI, Player);
		Player->Inventory[ITEM_SHREDDER] = 0;
	}

	// Setup the callbacks for this weapon
	Weapon->Control = Shredder_Control;
	//Weapon->ControlIndex = 8;
	Weapon->Blocked = NULL;
	Weapon->Trigger = NULL;
	Weapon->Owner = Player;
	Player->Weapon = Weapon;
	Weapon->Time = 0.0f;

	Weapon->ViewFlags = VIEW_TYPE_NONE;
	Weapon->ViewIndex = VIEW_INDEX_NONE;
	Weapon->FxFlags = (uint16)FX_SHREDDER;	

	geVec3d_Set(&Weapon->Mins, -1.0f, -1.0f, -1.0f);
	geVec3d_Set(&Weapon->Maxs,  1.0f,  1.0f,  1.0f);

	Weapon->XForm = Player->XForm;

	geVec3d_Add(&Weapon->XForm.Translation, &Player->GunOffset, &Weapon->XForm.Translation);

	// Play the sound
	GenVSI_PlaySound(VSI, SOUND_INDEX_SHREDDER, &Player->XForm.Translation);

	Front = Weapon->XForm.Translation;
	geXForm3d_GetIn(&Weapon->XForm, &In);
	geVec3d_AddScaled(&Front, &In, 10000.0f, &Back);

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Data.VSI = VSI;
	Data.Player = Player;

	if (geWorld_Collision(World, NULL, NULL, &Front, &Back, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_ACTORS, 0xffffffff, SelfCollisionCB, &Data, &Collision))
	{
		if (Collision.Actor)
		{
			GPlayer		*PlayerHit;

			PlayerHit = static_cast<GPlayer*>(GenVSI_ActorToPlayer(VSI, Collision.Actor));

			if (PlayerHit)
				DammagePlayer(VSI, Weapon, PlayerHit, 25, 90.0f, Time);
		}

		if (Collision.Model)
		{
			GPlayer		*Target;

			Target = (GPlayer*)geWorld_ModelGetUserData(Collision.Model);
			if (Target && Target->Trigger && (Target->ViewFlags & VIEW_TYPE_PHYSOB))
			{
				geXForm3d_GetIn(&Player->XForm, &In);
				Player->Velocity = In;
				geVec3d_Scale(&In, 1000.f, &Player->Velocity);
				Target->Trigger(VSI, Target, Player, (void*)&Collision);
			}
		}
	}

	Player->VPos = Player->XForm.Translation;

	Player->NextWeaponTime = GenVSI_GetTime(VSI) + 0.0f;
}

//=====================================================================================
//	FireWeapon
//=====================================================================================
geBoolean FireWeapon(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;

	assert(PlayerData);

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextWeaponTime)
		return GE_TRUE;
	
	ValidateWeapon(VSI, PlayerData);

	switch (Player->CurrentWeapon)
	{
		case 0:
			FireBlaster(VSI, Player, Time);
			break;
		case 1:
			FireGrenade(VSI, Player, Time);
			break;
		case 2:
			FireRocket(VSI, Player, Time);
			break;
		case 3:
			FireShredder(VSI, Player, Time);
			break;
	}

	return GE_TRUE;
}
