/****************************************************************************************/
/*  Atacker.c                                                                           */
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
extern void GenVS_Error(const char *Msg, ...);
typedef struct 
{
	geVec3d		LookPos;
	uint32		MotionOnOff;		// 32 bits for 32 motions, 1=on/0=off...

} Turret_Data;

typedef struct
{
	int32		NumMotions;
	uint8		MotionIndex[3];
} Motion_Table;

Motion_Table	MotionTable[2] = {
	{1, ACTOR_MOTION_T1, MOTION_INDEX_NONE, MOTION_INDEX_NONE},
	{1, ACTOR_MOTION_T2, MOTION_INDEX_NONE, MOTION_INDEX_NONE},
};

void ChangeTurretState(GPlayer *Player);
void Vec3d_RotateY(geVec3d *Source, geVec3d *Origin, float RadAmount);

//=====================================================================================
//=====================================================================================
geBoolean Attacker_ControlTurret(GenVSI *VSI, void *PlayerData, float Time)
{
	#define MY_PI	(3.15159f)
	#define HALF_PI	(MY_PI/2.0f)
	#define MY_PI2	(MY_PI*2)

	GPlayer			*Player, *Target;
	Turret_Data		*TData;
	geVec3d			Vect;
	float			RAngle, Length, DeltaY;
	int32			i;

	Player = (GPlayer*)PlayerData;

	Target = static_cast<GPlayer*>(GenVSI_GetNextPlayerInRadius(VSI, Player, NULL, "ClientPlayer", 400.0f));

	TData = (Turret_Data*)Player->userData;

	if (Target)	// Make the turret look at the target...
	{
		geVec3d		TPos;

		TPos = Target->XForm.Translation;
		TPos.Y += 140.0f;

		// Make the turrets look pos head towards the player, if there is one
		geVec3d_Subtract(&TPos, &TData->LookPos, &Vect);
		Length = geVec3d_Normalize(&Vect);
		geVec3d_AddScaled(&TData->LookPos, &Vect, Length*2.0f*Time, &TData->LookPos);

		if (Player->Time >= 1.5f)
		{
			Attacker_Fire(VSI, Player, Target, Time);
			Player->Time = 0.0f;
		}

		Player->Time += Time;
	}
	else		// Go into search mode mode
	{
		switch(Player->State)
		{
			case PSTATE_LookLeft:
			{
				Vec3d_RotateY(&TData->LookPos, &Player->XForm.Translation, 0.02f);
				break;
			}
			case PSTATE_LookRight:
			{
				Vec3d_RotateY(&TData->LookPos, &Player->XForm.Translation, -0.02f);
				break;
			}
			case PSTATE_LookUp:
			{
				if (TData->LookPos.Y < Player->XForm.Translation.Y+60.0f)
					TData->LookPos.Y += 200.0f*Time;
				else
					Player->State = PSTATE_LookLeft;
				break;
			}
			case PSTATE_LookDown:
			{
				if (TData->LookPos.Y > Player->XForm.Translation.Y-80.0f)
					TData->LookPos.Y -= 200.0f*Time;
				else
					Player->State = PSTATE_LookLeft;
				break;
			}
			case PSTATE_Normal:
			{
				break;
			}
		}
		
		ChangeTurretState(Player);
	}

	// Make the Turret look at the target pos
	
	// First, get the vector to the turret
	geVec3d_Subtract(&Player->XForm.Translation, &TData->LookPos, &Vect);
	DeltaY = Vect.Y;
	Length = geVec3d_Normalize(&Vect);

	// Then set the Root...
	RAngle = (float)atan2(Vect.X, Vect.Z);
	geXForm3d_SetZRotation(&Player->XFormData[0].XForm, RAngle-MY_PI);

	// Now set the Axle...
	RAngle = (float)acos(-DeltaY / Length);

	geXForm3d_SetXRotation(&Player->XFormData[1].XForm, RAngle-HALF_PI);

	// control main animation
	AnimatePlayer(VSI, PlayerData, Player->MotionIndex, Time, GE_TRUE);

	for (i=0; i<2; i++)
	{
		uint32	Bit;

		Bit = (1<<i);

		if (!(Bit&TData->MotionOnOff))
		{
			if ((rand() % 100) > 98)
				TData->MotionOnOff |= Bit;

			continue;
		}

		if (AnimatePlayer2(VSI, PlayerData, i, Time, GE_TRUE))
		{
			if ((rand() % 100) > 40)
				TData->MotionOnOff &= ~Bit;
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
void Attacker_TurretSpawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	AttackerTurret	*Turret;
	GPlayer			*Player;
	Turret_Data		*TData;

	Player = (GPlayer*)PlayerData;

	Player->Time = 0.0f;
	Player->Control = Attacker_ControlTurret;

	if (ClassData == NULL)
		{
			GenVS_Error("Attacker_TurrentSpawn: entity missing class data ('%s')\n",EntityName);
		}

	Turret = (AttackerTurret*)ClassData;

	geXForm3d_SetIdentity(&Player->XForm);
	geXForm3d_SetTranslation(&Player->XForm, Turret->Origin.X, Turret->Origin.Y, Turret->Origin.Z);

	Player->VPos = Player->XForm.Translation;
	
	geVec3d_Set(&Player->Mins, -50.0f, -50.0f, -50.0f);
	geVec3d_Set(&Player->Maxs,  50.0f,  50.0f,  50.0f);

	Player->Scale = 0.4f;
	Player->State = PSTATE_LookLeft;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_HACK;
	Player->ViewIndex = ACTOR_INDEX_TURRET;
	Player->MotionIndex = ACTOR_MOTION_T1;

	// Setup hacked extra bone stuff
	Player->NumXFormData = 2;

	Player->XFormData[0].XForm = Player->XForm;
	Player->XFormData[0].BoneIndex = 0;

	Player->XFormData[1].XForm = Player->XForm;
	Player->XFormData[1].BoneIndex = 1;

	// Setup motions
	Player->NumMotionData = 2;

	Player->MotionData[0].MotionTime = 0.0f;
	Player->MotionData[0].MotionIndex = ACTOR_MOTION_T1;

	Player->MotionData[1].MotionTime = 0.0f;
	Player->MotionData[1].MotionIndex = ACTOR_MOTION_T2;

	TData = GE_RAM_ALLOCATE_STRUCT(Turret_Data);

	if (!TData)
		return;

	memset(TData, 0, sizeof(Turret_Data));

	TData->LookPos = Player->XForm.Translation;
	TData->LookPos.Z += 40.0f;

	Player->userData = TData;

	//return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
void Attacker_TurretDestroy(GenVSI *VSI, void *PlayerData, void *ClassData)
{
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	if (Player->userData)
		geRam_Free(Player->userData);

}

//=====================================================================================
//	Attacker_Fire
//=====================================================================================
void Attacker_Fire(GenVSI *VSI, GPlayer *Player, GPlayer *Target, float Time)
{
	GPlayer			*Weapon;

	Weapon = static_cast<GPlayer*>(GenVSI_SpawnPlayer(VSI, "Attacker_Fire"));

	if (!Weapon)
	{
		return;
	}

	// Setup the callbacks for this weapon
	Weapon->Control = Blaster_Control;
	Weapon->Blocked = NULL;
	Weapon->Trigger = NULL;
	Weapon->Owner = Player;
	Weapon->Time = 0.0f;
	Weapon->PingTime = Player->PingTime;

	Weapon->ControlIndex = 5;

	Weapon->ViewFlags = (VIEW_TYPE_SPRITE | VIEW_TYPE_LIGHT);
	Weapon->ViewIndex = TEXTURE_INDEX_WEAPON2;
	Weapon->Scale = 1.0f;
	Weapon->FxFlags = (uint16)FX_PARTICLE_TRAIL;		// Make some particles follow Weapon_Blaster

	XFormFromVector(&Player->XForm.Translation, &Target->XForm.Translation, 0.0f, &Weapon->XForm);

	// Play the "take off" sound
	GenVSI_PlaySound(VSI, SOUND_INDEX_BLASTER, &Player->XForm.Translation);

	Weapon->VPos = Player->XForm.Translation;
}

void ChangeTurretState(GPlayer *Player)
{
	int32	Seed;

	Seed = rand() % 100;

	if (Seed > 97)
		Player->State = PSTATE_LookRight;
	else if (Seed > 96)
		Player->State = PSTATE_LookUp;
	else if (Seed > 94)
		Player->State = PSTATE_LookDown;
	else if (Seed > 90)
		Player->State = PSTATE_Normal;
}

void Vec3d_RotateY(geVec3d *Source, geVec3d *Origin, float RadAmount)
{
	geXForm3d	XForm;

	geVec3d_Subtract(Source, Origin, Source);

	geXForm3d_SetYRotation(&XForm, RadAmount);
	geXForm3d_Transform(&XForm, Source, Source);
	
	geVec3d_Add(Source, Origin, Source);
}