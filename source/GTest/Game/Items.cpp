/****************************************************************************************/
/*  Items.c                                                                             */
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
#include <assert.h>
#include <Math.h>

#include "GMain.h"
extern void GenVS_Error(const char *Msg, ...);
//=====================================================================================
//	Item_TriggerHealth
//=====================================================================================
static geBoolean Item_TriggerHealth(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer			*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;
	
	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->Health >= 100)
		return GE_TRUE;				// Does'nt need health, leave it for someone else

	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + HEALTH_RESPAWN;

	// Increase the health of the player that touches this health
	Target->Health += 25;
		
	if (Target->Health > 100)
		Target->Health = 100;

	GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_HEALTH, &Player->XForm.Translation);

	// Send the info to the client machine that is attached to this player
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		GenVSI_SetClientHealth(VSI, Target->ClientHandle, Target->Health);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get 25 health");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerArmor
//=====================================================================================
static geBoolean Item_TriggerArmor(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer			*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;
	
	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->Inventory[ITEM_ARMOR] >= MAX_ARMOR)
		return GE_TRUE;

	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + ARMOR_RESPAWN;

	GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_HEALTH, &Player->XForm.Translation);

	// Update the amount of armor the player has...
	Target->InventoryHas[ITEM_ARMOR] = GE_TRUE;
	
	Target->Inventory[ITEM_ARMOR] += ARMOR_AMOUNT;
	
	if (Target->Inventory[ITEM_ARMOR] > MAX_ARMOR)
		Target->Inventory[ITEM_ARMOR] = MAX_ARMOR;

	// Send the info to the client machine that is attached to this player
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_ARMOR);

		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get %i armor", ARMOR_AMOUNT);
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerRocket
//=====================================================================================
static geBoolean Item_TriggerRocket(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer	*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->InventoryHas[ITEM_ROCKETS])
		return GE_TRUE;

	// Make the rockets dissapear
	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + ROCKET_RESPAWN;

	if (!Target->InventoryHas[ITEM_ROCKETS] || Target->Inventory[ITEM_ROCKETS] < 5)
		GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON2, &Player->XForm.Translation);
	else
		GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON1, &Player->XForm.Translation);

	Target->InventoryHas[ITEM_ROCKETS] = GE_TRUE;
	Target->CurrentWeapon = ITEM_ROCKETS;

	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
		GenVSI_SetClientWeapon(VSI, Target->ClientHandle, ITEM_ROCKETS);
		
	Target->Inventory[ITEM_ROCKETS] += ROCKET_AMOUNT;	

	// Send the info to the client machine that is attached to this player
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_ROCKETS);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get the Rocket Launcher");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerRocketAmmo
//=====================================================================================
static geBoolean Item_TriggerRocketAmmo(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer	*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->Inventory[ITEM_ROCKETS] >= ROCKET_MAX_AMOUNT)
		return GE_TRUE;

	// Make the rockets dissapear
	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + ROCKET_AMMO_RESPAWN;		// Make them come back at a later time

	// Increase the items of the player the touches this ammo
	Target->Inventory[ITEM_ROCKETS] += ROCKET_AMMO_AMOUNT;
	
	if (Target->Inventory[ITEM_ROCKETS] > ROCKET_MAX_AMOUNT)
		Target->Inventory[ITEM_ROCKETS] = ROCKET_MAX_AMOUNT;
		
	GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON1, &Player->XForm.Translation);

	// Send the info to the client machine that is attached to this player
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_ROCKETS);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get some Rocket Ammo");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerGrenade
//=====================================================================================
static geBoolean Item_TriggerGrenade(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer	*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Target->InventoryHas[ITEM_GRENADES])
		return GE_TRUE;

	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	// Make the ammo dissapear
	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + GRENADE_RESPAWN;		// Make them come back at a later time

	if (!Target->InventoryHas[ITEM_GRENADES] || Target->Inventory[ITEM_GRENADES] < 5)
		GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON2, &Player->XForm.Translation);
	else
		GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON1, &Player->XForm.Translation);

	Target->InventoryHas[ITEM_GRENADES] = GE_TRUE;
	Target->CurrentWeapon = ITEM_GRENADES;

	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
		GenVSI_SetClientWeapon(VSI, Target->ClientHandle, ITEM_GRENADES);
		
	Target->Inventory[ITEM_GRENADES] += GRENADE_AMOUNT;		
	
	// Send the info to the client machine that is attached to this player
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_GRENADES);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get the Grenade Launcher");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlShredder
//=====================================================================================
static geBoolean Item_ControlShredder(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d	Pos;
	GPlayer	*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Player->ViewIndex == 0xffff)
	{
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);
		Player->ViewIndex = ACTOR_INDEX_SHREDDER;
	}
	
	Pos = Player->XForm.Translation;

	geXForm3d_RotateY(&Player->XForm, Time*2.0f);

	Player->XForm.Translation = Pos;

	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlShredderAmmo
//=====================================================================================
static geBoolean Item_ControlShredderAmmo(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer	*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Player->ViewIndex == 0xffff)		// Time to respawn
	{
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);
		Player->ViewIndex = ACTOR_INDEX_SHREDDER_AMMO;
	}
	
	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerShredder
//=====================================================================================
static geBoolean Item_TriggerShredder(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer			*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->InventoryHas[ITEM_SHREDDER])
		return GE_TRUE;

	// Make the ammo dissapear
	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + SHREDDER_RESPAWN;		// Make them come back at a later time

	GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON2, &Player->XForm.Translation);

	Target->InventoryHas[ITEM_SHREDDER] = GE_TRUE;
	Target->CurrentWeapon = ITEM_SHREDDER;

	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
		GenVSI_SetClientWeapon(VSI, Target->ClientHandle, ITEM_SHREDDER);
		
	Target->Inventory[ITEM_SHREDDER] += SHREDDER_AMOUNT;
	
	// Actors don't have a client handle
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_SHREDDER);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get the Shredder");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerShredderAmmo
//=====================================================================================
static geBoolean Item_TriggerShredderAmmo(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer	*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->Inventory[ITEM_SHREDDER] >= SHREDDER_MAX_AMOUNT)
		return GE_TRUE;

	// Make the ammo dissapear
	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + SHREDDER_AMMO_RESPAWN;		// Make them come back at a later time

	// Increase the items of the player the touches this ammo
	Target->Inventory[ITEM_SHREDDER] += SHREDDER_AMMO_AMOUNT;
	
	if (Target->Inventory[ITEM_SHREDDER] > SHREDDER_MAX_AMOUNT)
		Target->Inventory[ITEM_SHREDDER] = SHREDDER_MAX_AMOUNT;
		
	GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON1, &Player->XForm.Translation);

	// Actors don't have a client handle
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_SHREDDER);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get some Shredder Ammo");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_TriggerGrenadeAmmo
//=====================================================================================
static geBoolean Item_TriggerGrenadeAmmo(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void* data)
{
	GPlayer	*Player, *Target;

	Player = (GPlayer*)PlayerData;
	Target = (GPlayer*)TargetData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Target->ClientHandle == CLIENT_NULL_HANDLE)
		return GE_TRUE;

	if (Target->Inventory[ITEM_GRENADES] >= GRENADE_MAX_AMOUNT)
		return GE_TRUE;

	// Make the ammo dissapear
	Player->ViewIndex = 0xffff;
	Player->NextThinkTime = GenVSI_GetTime(VSI) + GRENADE_AMMO_RESPAWN;		// Make them come back at a later time

	// Increase the items of the player the touches this ammo
	Target->Inventory[ITEM_GRENADES] += GRENADE_AMMO_AMOUNT;
	
	if (Target->Inventory[ITEM_GRENADES] > GRENADE_MAX_AMOUNT)
		Target->Inventory[ITEM_GRENADES] = GRENADE_MAX_AMOUNT;
		
	GenVSI_PlaySound(VSI, SOUND_INDEX_PICKUP_WEAPON1, &Player->XForm.Translation);

	// Actors don't have a client handle
	if (Target->ClientHandle != CLIENT_NULL_HANDLE)
	{
		UpdateClientInventory(VSI, Target, ITEM_GRENADES);
		GenVSI_ConsoleHeaderPrintf(VSI, Target->ClientHandle, GE_FALSE, "You get some Grenade Ammo");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlHealth
//=====================================================================================
geBoolean Item_ControlHealth(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d	Pos;
	GPlayer	*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;		// Not time to come back yet

	if (Player->ViewIndex == 0xffff)	// Comming back into view 
	{
		Player->Roll = 0.0f;
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);
		Player->ViewIndex = ACTOR_INDEX_HEALTH;
	}
	Pos = Player->XForm.Translation;

	Player->XForm.Translation.X = 0.0f;
	Player->XForm.Translation.Y = 0.0f;
	Player->XForm.Translation.Z = 0.0f;

	geXForm3d_RotateY(&Player->XForm, Time*2.0f);

	Player->XForm.Translation = Pos;

	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlArmor
//=====================================================================================
geBoolean Item_ControlArmor(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d	Pos;
	GPlayer	*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;		// Not time to come back yet

	if (Player->ViewIndex == 0xffff)	// Comming back into view 
	{
		Player->Roll = 0.0f;
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);
		Player->ViewIndex = ACTOR_INDEX_ARMOR;
	}
	Pos = Player->XForm.Translation;

	Player->XForm.Translation.X = 0.0f;
	Player->XForm.Translation.Y = 0.0f;
	Player->XForm.Translation.Z = 0.0f;

	geXForm3d_RotateY(&Player->XForm, Time*2.0f);

	Player->XForm.Translation = Pos;

	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlRocket
//=====================================================================================
static geBoolean Item_ControlRocket(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d	Pos;
	GPlayer	*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Player->ViewIndex == 0xffff)
	{
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);
		Player->ViewIndex = ACTOR_INDEX_ROCKET;
	}
	
	Player->Roll += Time*2.0f;

	if (Player->Roll > 3.14159f*2.0f)
		Player->Roll = 0.0f;

	Pos = Player->XForm.Translation;

	geXForm3d_SetYRotation(&Player->XForm, Player->Roll);

	Player->XForm.Translation = Pos;

	return GE_TRUE;
}


//=====================================================================================
//	Item_ControlRocketAmmo
//=====================================================================================
static geBoolean Item_ControlRocketAmmo(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Player->ViewIndex == 0xffff)		// Time to respawn
	{
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);
		Player->ViewIndex = ACTOR_INDEX_ROCKET_AMMO;
	}
	
	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlGrenade
//=====================================================================================
static geBoolean Item_ControlGrenade(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Player->ViewIndex == 0xffff)
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);

	// Make sure the viewidex is set when NextThinkTime is reached...
	Player->ViewIndex = ACTOR_INDEX_GRENADE;
	
	return GE_TRUE;
}

//=====================================================================================
//	Item_ControlGrenadeAmmo
//=====================================================================================
static geBoolean Item_ControlGrenadeAmmo(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextThinkTime)
		return GE_TRUE;

	if (Player->ViewIndex == 0xffff)
		GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_ITEM_SPAWN);

	// Make sure the viewidex is set when NextThinkTime is reached...
	Player->ViewIndex = ACTOR_INDEX_GRENADE_AMMO;
	
	return GE_TRUE;
}

//=====================================================================================
//	Item_HealthSpawn
//=====================================================================================
void Item_HealthSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemHealth	*Health;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;
	
	Player->Control = Item_ControlHealth;
	//Player->ControlIndex = 9;
	Player->Trigger = Item_TriggerHealth;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_HEALTH;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	// Clean up the matrix
	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error( "HealthSpawn: entity missing class data ('%s')\n",EntityName);
		}

	Health = (ItemHealth*)ClassData;

	// Set the initial pos
	Player->XForm.Translation = Health->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_ArmorSpawn
//=====================================================================================
void Item_ArmorSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemArmor	*Armor;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;
	
	Player->Control = Item_ControlArmor;
	//Player->ControlIndex = 9;
	Player->Trigger = Item_TriggerArmor;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_ARMOR;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	// Clean up the matrix
	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("ArmorSpawn: entity missing class data ('%s')\n",EntityName);
		}

	Armor = (ItemArmor*)ClassData;

	// Set the initial pos
	Player->XForm.Translation = Armor->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_RocketSpawn
//=====================================================================================
void Item_RocketSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	GPlayer		*Player;
	ItemRocket	*Rocket;

	Player = (GPlayer*)PlayerData;

	Player->Control = Item_ControlRocket;
	Player->Trigger = Item_TriggerRocket;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex =ACTOR_INDEX_ROCKET;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("RocketSpawn: entity missing class data ('%s')\n",EntityName);
		}

	Rocket = (ItemRocket*)ClassData;

	Player->XForm.Translation = Rocket->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_RocketAmmoSpawn
//=====================================================================================
void Item_RocketAmmoSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemRocketAmmo	*RocketAmmo;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = Item_ControlRocketAmmo;
	Player->Trigger = Item_TriggerRocketAmmo;

	Player->Time = 0.0f;
	
	geVec3d_Set(&Player->Mins, -20.0f, -20.0f, -20.0f);
	geVec3d_Set(&Player->Maxs,  20.0f,  20.0f,  20.0f);

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_ROCKET_AMMO;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("RocketAmmoSpawn: entity missing class data ('%s')\n",EntityName);
		}

	RocketAmmo = (ItemRocketAmmo*)ClassData;

	Player->XForm.Translation = RocketAmmo->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_GrenadeSpawn
//=====================================================================================
void Item_GrenadeSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemGrenade	*Grenade;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = Item_ControlGrenade;
	Player->Trigger = Item_TriggerGrenade;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_GRENADE;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("GrenadeSpawn: entity missing class data ('%s')\n",EntityName);
		}

	Grenade = (ItemGrenade*)ClassData;

	Player->XForm.Translation = Grenade->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_GrenadeAmmoSpawn
//=====================================================================================
void Item_GrenadeAmmoSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemGrenadeAmmo	*GrenadeAmmo;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = Item_ControlGrenadeAmmo;
	Player->Trigger = Item_TriggerGrenadeAmmo;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_GRENADE_AMMO;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("GrenadeAmmo: entity missing class data ('%s')\n",EntityName);
		}

	GrenadeAmmo = (ItemGrenadeAmmo*)ClassData;

	Player->XForm.Translation = GrenadeAmmo->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_ShredderSpawn
//=====================================================================================
void Item_ShredderSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemShredder	*Shredder;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = Item_ControlShredder;
	Player->Trigger = Item_TriggerShredder;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_SHREDDER;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("Shreadder: entity missing class data ('%s')\n",EntityName);
		}

	Shredder = (ItemShredder*)ClassData;

	Player->XForm.Translation = Shredder->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

//=====================================================================================
//	Item_ShredderAmmoSpawn
//=====================================================================================
void Item_ShredderAmmoSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ItemShredderAmmo	*ShredderAmmo;
	GPlayer		*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = Item_ControlShredderAmmo;
	Player->Trigger = Item_TriggerShredderAmmo;

	Player->Time = 0.0f;
	
	Player->Mins.X =-20.0f;
	Player->Mins.Y =-20.0f;
	Player->Mins.Z =-20.0f;
	Player->Maxs.X = 20.0f;
	Player->Maxs.Y = 20.0f;
	Player->Maxs.Z = 20.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_TOUCH;
	Player->ViewIndex = ACTOR_INDEX_SHREDDER_AMMO;
	Player->FrameTime = 0.0f;
	Player->Scale	  = 1.0f;
	Player->MotionIndex = MOTION_INDEX_NONE;

	geXForm3d_SetIdentity(&Player->XForm);

	if (ClassData == NULL)
		{
			GenVS_Error("ShredderAmmoSpawn: entity missing class data ('%s')\n",EntityName);
		}

	ShredderAmmo = (ItemShredderAmmo*)ClassData;

	Player->XForm.Translation = ShredderAmmo->Origin;	

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}

