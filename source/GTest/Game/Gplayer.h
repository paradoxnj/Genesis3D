/****************************************************************************************/
/*  GPlayer.h                                                                           */
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
#ifndef GPLAYER_H
#define GPLAYER_H

#include "Genesis.h"

#include "GenVSI.h"

#define MAX_CLASS_NAME_SIZE			32
#define MAX_PLAYER_ITEMS			32

#define CONTROL_INDEX_NONE			0xffff
#define TRIGGER_INDEX_NONE			0xffff
#define VIEW_INDEX_NONE				0xffff
#define MOTION_INDEX_NONE			0xff

#define DAMMAGE_TYPE_NONE			(0<<0)
#define DAMMAGE_TYPE_NORMAL			(1<<0)
#define DAMMAGE_TYPE_RADIUS			(1<<1)
//===========================================================================
//===========================================================================
typedef enum
{
	MODE_Authoritive,
	MODE_Proxy,
	MODE_Dumb
} GPlayer_Mode;

// NOTE - You cannot go over 255 enums, cause network code converts this to a byte when sending...
typedef enum
{
	// For clients, etc...
	PSTATE_Normal = 0,
	PSTATE_InAir,
	PSTATE_InLava,
	PSTATE_InWater,
	PSTATE_Running,
	PSTATE_Dead,
	PSTATE_DeadOnGround,

	// For Doors, etc...
	PSTATE_Closed,
	PSTATE_Opened,

	// Turret
	PSTATE_LookLeft,
	PSTATE_LookRight,
	PSTATE_LookUp,
	PSTATE_LookDown,

} GPlayer_PState;

#define NULL_BONE_INDEX				255
#define GPLAYER_MAX_XFORM_DATA		5

typedef struct
{
	geXForm3d	XForm;
	uint8		BoneIndex;					// 255 = NULL bone index (root)
} GPlayer_XFormData;

#define GPLAYER_MAX_MOTION_DATA		5

typedef struct
{
	float		MotionTime;
	uint8		MotionIndex;
} GPlayer_MotionData;

//===========================================================================
//===========================================================================
typedef struct GPlayer			GPlayer;

typedef geBoolean	GPlayer_Control(GenVSI *VSI, void *PlayerData, float TimeElapsed);
typedef geBoolean	GPlayer_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *Target, void* data);
typedef geBoolean	GPlayer_Blocked(GenVSI *VSI, void *PlayerData, GPlayer *Target);

//===========================================================================
//===========================================================================
typedef struct GPlayer
{
	geBoolean			Active;
	geBoolean			JustSpawned;				// only true until first update.

	GPlayer_Mode		Mode;						// Role on this machine
	GPlayer_Mode		RemoteMode;					// If on the server, the roll of the same player on the clients
													// If on the clients, the role of this same player on the server

	char				ClassName[MAX_CLASS_NAME_SIZE];

	float				Time;
	float				UpdateTime;					// Clients use this to mark what players have been updated...
	
	// Callback functions
	GPlayer_Control		*Control;
	GPlayer_Trigger		*Trigger;
	GPlayer_Trigger		*StandOn;
	GPlayer_Blocked		*Blocked;

	struct GPlayer		*Owner;						// Player that spawned this player

	float				PingTime;

	int32				Score;
	int32				Health;

	// Special old flags used on client side only to see what changed on his end...
	uint16				OldViewIndex2;				// *Index of what mesh/model to use in client/server mesh list
	uint16				OldViewFlags2;				// *View flags
	float				OldScale2;					// *

//=============================================
	// ** DO NOT USE (Internal use only) **

	float				OldSpawnTime;				// When client spawned this player
	uint16				OldViewFlags;				// *View flags
	uint16				OldViewIndex;				// *Index of what mesh/model to use in client/server mesh list
	uint8				OldMotionIndex;				// *
	uint16				OldFxFlags;					//
	uint16				OldDammageFlags;			//
	geVec3d				OldPos;
	geVec3d				OldAngles;					// *Euler angles for passing across network
	float				OldFrameTime;				// *Frame number
	float				OldScale;					// *
	geVec3d				OldVelocity;				// Movement Velocity
	GPlayer_PState		OldState;
	uint16				OldControlIndex;
	uint16				OldTriggerIndex;

	geVec3d				OldMins;					// Mins of player
	geVec3d				OldMaxs;					// Maxs of player

	geVec3d				Pos;
	geVec3d				Angles;	
//=============================================
	// ** Data sent across network **
	float				SpawnTime;					// When client spawned this player
	uint16				ViewFlags;					// View flags
	uint16				ViewIndex;					// Index of what mesh/model to use in client/server mesh list
	uint8				MotionIndex;				//
	uint16				FxFlags;					//
	uint16				DammageFlags;				//

	geXForm3d			XForm;						

	float				FrameTime;					// Frame number
	float				Scale;		
	geVec3d				Velocity;					// Movement Velocity
	GPlayer_PState		State;
	uint16				ControlIndex;
	uint16				TriggerIndex;

	geVec3d				Mins;						// Mins of player
	geVec3d				Maxs;						// Maxs of player
//=============================================

	// Data used for local AI/etc only 
	geVec3d				LastGoodPos;

	float				NextThinkTime;
	float				Roll;

	uint32				Mask;							// For GenVSI_GetNextPlayer.. functions

	// Inventory
	uint8				InventoryHas[MAX_PLAYER_ITEMS];	// If player has item
	int32				Inventory[MAX_PLAYER_ITEMS];	// Amount of item

	// Weapon data 
	int16				CurrentWeapon;					// Offset into inventory slot
	float				NextWeaponTime;

	// Gun offset (Note - This variable is hacked and NOT sent accross network.
	// Both the client and the server just set it to the same number (FIXTHIS!!!!)
	geVec3d				GunOffset;

	// Server side ONLY.  Used for determining the vis pos of the player (for vis culling while sending data across network)
	geVec3d				VPos;

	struct GPlayer		*Weapon;					// Current Weapon, Hack till we find a better way to do auto firing weapons

	GenVSI_CHandle		ClientHandle;				// == -1 if does not belong to a client

	void				*ClassData;
	void				*userData;

	//==

	// For client rendering, etc...
	geMesh				*Mesh;
	geActor				*Actor;
	geActor_Def			*ActorDef;
	geLight				*Light;
	geWorld_Model		*Model;
	gePoly				*Poly;
	
	GenVSI_DestroyFunc	*DFunc;

	// Hack for now, till we get a better way to add stuff to the net traffic dynamically...
	int32				NumXFormData;
	GPlayer_XFormData	XFormData[GPLAYER_MAX_XFORM_DATA];

	int32				NumMotionData;
	GPlayer_MotionData	MotionData[GPLAYER_MAX_MOTION_DATA];

} GPlayer;

#endif