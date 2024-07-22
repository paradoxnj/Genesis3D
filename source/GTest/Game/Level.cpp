/****************************************************************************************/
/*  Level.c                                                                             */
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
#include <stdio.h>

#include "GMain.h"

#include "PathPt.h"
extern void GenVS_Error(const char *Msg, ...);
//=====================================================================================
//	SetupWorldCB
//	Callback function called everytime a new world is set.  This functions is passed
//	through the setworld function...
//=====================================================================================
geBoolean SetupWorldCB(GenVSI *VSI, const char *WorldName, geVFile *MainFS)
{
	// Setup the pathpoints for bots...
	if (!PathPt_Startup(GenVSI_GetWorld(VSI), MainFS))
		return GE_FALSE;

	// Don't forget to reset this between worlds...
	CurrentPlayerStart = NULL;

	// Actor indexes
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_PLAYER, "PlayerClient", "Actors\\Dema.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_TURRET, "PlayerClient", "Actors\\Turret.Act");

	GenVSI_ActorIndex(VSI, ACTOR_INDEX_HEALTH, "foo", "Actors\\MedKit.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_ROCKET_AMMO, "foo", "Actors\\rocket.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_ROCKET, "foo", "Actors\\rlaunch.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_SHREDDER, "foo", "Actors\\Shredder.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_SHREDDER_AMMO, "foo", "Actors\\SAmmo.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_GRENADE_AMMO, "foo", "Actors\\Grenade.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_GRENADE, "foo", "Actors\\GLaunch.Act");
	GenVSI_ActorIndex(VSI, ACTOR_INDEX_ARMOR, "foo", "Actors\\Armor.Act");

	// Motion indexes
	GenVSI_MotionIndex(VSI, ACTOR_MOTION_PLAYER_WALK, "Walk");
	GenVSI_MotionIndex(VSI, ACTOR_MOTION_PLAYER_RUN, "Run");
	GenVSI_MotionIndex(VSI, ACTOR_MOTION_PLAYER_HIT, "Hit");
	GenVSI_MotionIndex(VSI, ACTOR_MOTION_PLAYER_DIE, "Die");
	GenVSI_MotionIndex(VSI, ACTOR_MOTION_PLAYER_IDLE, "Idle");

	//GenVSI_MotionIndex(VSI, ACTOR_MOTION_T1, "BigArmLeft");
	//GenVSI_MotionIndex(VSI, ACTOR_MOTION_T2, "BigArmEndLeft");

	GenVSI_TextureIndex(VSI, TEXTURE_INDEX_WEAPON2, "Bmp\\Weapon\\Spot_Red.Bmp", "Bmp\\Weapon\\A_Spot.Bmp");

	GenVSI_SoundIndex(VSI, 0, "Wav\\MLaunch.wav");
	GenVSI_SoundIndex(VSI, 1, "Wav\\MImpact.wav");
	GenVSI_SoundIndex(VSI, 2, "Wav\\DoorOpen.wav");
	GenVSI_SoundIndex(VSI, 3, "Wav\\Bounce.wav");
	GenVSI_SoundIndex(VSI, 4, "Wav\\KeyPress.wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_HURT1, "Wav\\Hurt1.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_HURT2, "Wav\\Hurt2.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_HURT3, "Wav\\Hurt3.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_HURT4, "Wav\\Hurt4.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_DIE, "Wav\\Die.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_PICKUP_HEALTH, "Wav\\PickupHealth.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_PICKUP_WEAPON1, "Wav\\LockLoad.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_PICKUP_WEAPON2, "Wav\\WeaponPickup.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_JUMP, "Wav\\Jump.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_BLASTER, "Wav\\Blaster.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_BLASTER_BANG, "Wav\\BlasterBang.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_SHREDDER, "Wav\\Shredder.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_ITEM_SPAWN, "Wav\\itemspwn.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_PLAYER_SPAWN, "Wav\\plyrspwn.Wav");
	GenVSI_SoundIndex(VSI, SOUND_INDEX_GRENADE, "Wav\\grenade.Wav");

	// Register bone indexes
	GenVSI_BoneIndex(VSI, 0, "ROOT");
	GenVSI_BoneIndex(VSI, 1, "AXLE");

	return GE_TRUE;
}

//=====================================================================================
//	ShutdownWorldCB
//=====================================================================================
geBoolean ShutdownWorldCB(GenVSI *VSI)
{
	// Shutdown all the path points for this world...
	PathPt_Shutdown();

	return GE_TRUE;
}

//=====================================================================================
//	ChangeLevel_Trigger
//=====================================================================================
static geBoolean ChangeLevel_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void *Context)
{
	ChangeLevel		*Cl;
	GPlayer			*Player;
	char			Name[256];

	Player = (GPlayer*)PlayerData;
	Cl = (ChangeLevel*)Player->ClassData;

	assert(strlen(Cl->LevelName)+8 < 256);
	
	sprintf(Name, "Levels\\%s", Cl->LevelName);

	// Set the world using the name supplied in the entity that is attached to this player...
	GenVSI_SetWorld(VSI, SetupWorldCB, ShutdownWorldCB, Name);

	return GE_TRUE;		
}

//=====================================================================================
//	ChangeLevel_Spawn
//=====================================================================================
void ChangeLevel_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	ChangeLevel		*Cl;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = NULL;
	Player->Trigger = ChangeLevel_Trigger;
	Player->Blocked = NULL;
	Player->Time = 0.0f;
	Player->ViewFlags = VIEW_TYPE_NONE | VIEW_TYPE_LOCAL | VIEW_TYPE_STANDON;

	if (ClassData == NULL)
		{
			GenVS_Error("ChangeLevel_Spawn: entity missing class data ('%s')\n",EntityName);
		}
	Cl = (ChangeLevel*)ClassData;

	if (Cl->LevelName == NULL)
		{
			GenVS_Error("ChangeLevel_Spawn: entity missing LevelName ('%s')\n",EntityName);
		}
	if (Cl->Model == NULL)
		{
			GenVS_Error("ChangeLevel_Spawn: entity missing model ('%s')\n",EntityName);
		}


	geXForm3d_SetIdentity(&Player->XForm);
	geXForm3d_SetTranslation(&Player->XForm, Cl->Origin.X, Cl->Origin.Y, Cl->Origin.Z);

	Player->VPos = Player->XForm.Translation;


	GenVSI_RegisterPlayerModel(VSI, Player, Cl->Model);


	//return GE_TRUE;
}
