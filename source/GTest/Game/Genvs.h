// USERTYPES.H -- Genesis Editor standard entity types
//
// Copyright  1999, WildTangent, Inc.
#ifndef GENVS_H
#define GENVS_H

#include "Quatern.h"
#include "GenVSI.h"

#include "PhysicsObject.h"
#include "PhysicsJoint.h"
#include "PhysicsSystem.h"

#pragma warning( disable : 4068 )
#ifdef __cplusplus
extern "C" {
#endif

#pragma GE_BrushContents
typedef enum
{
	Water = 0x00010000,
	Lava = 0x00020000,
	/*
	type3 = 0x00040000,
	type4 = 0x00080000,
	type5 = 0x00100000,
	type6 = 0x00200000,
	type7 = 0x00400000,
	type8 = 0x00800000,
	type9 = 0x01000000,
	type10 = 0x02000000,
	type11 = 0x04000000,
	type12 = 0x08000000,
	type13 = 0x10000000,
	type14 = 0x20000000,
	type15 = 0x40000000,
	type16 = 0x80000000
	*/
} UserContentsEnum;

// Playerstart
#pragma GE_Type("Player.ico")
typedef struct	PlayerStart
{
#pragma GE_Published
    geVec3d     Origin;
#pragma GE_Origin(Origin)
}	PlayerStart;

// DeathMatchstart
#pragma GE_Type("Player.ico")
typedef struct  DeathMatchStart
{
#pragma GE_Published
    geVec3d     Origin;
#pragma GE_Origin(Origin)
}   DeathMatchStart;

// MissleTurret
//#pragma GE_Type("Player.ico")
typedef struct	AttackerTurret
{
//#pragma GE_Published
    geVec3d     Origin;
//#pragma GE_Origin(Origin)
}	AttackerTurret;

// PhysicsObject
typedef struct PhysicsObject PhysicsObject;
#pragma GE_Type("Model.ico")
typedef struct	PhysicsObject
{
#pragma GE_Published
  geWorld_Model		*Model;
  geVec3d					Origin;
	float						mass;
	int							isAffectedByGravity;
	int							respondsToForces;
	float						linearDamping;
	float						angularDamping;
	PhysicsObject		*Next;
	float						physicsScale;
#pragma GE_DefaultValue(mass, "10.0")
#pragma GE_DefaultValue(isAffectedByGravity, "1")
#pragma GE_DefaultValue(respondsToForces, "1")
#pragma GE_DefaultValue(linearDamping, "0.0005")
#pragma GE_DefaultValue(angularDamping, "0.0005")
#pragma GE_DefaultValue(physicsScale, "0.01")

#pragma GE_Private
	gePhysicsObject *			stateInfo;
#pragma GE_Origin(Origin)
}	PhysicsObject;

typedef struct PhysicsJoint PhysicsJoint;


// Joint
#pragma GE_Type("Item.ico")
typedef struct  PhysicsJoint
{
#pragma GE_Published
    geVec3d					Origin;
		PhysicsObject *	physicsObject1;
		PhysicsObject *	physicsObject2;
		PhysicsJoint			* Next;
		float						assemblyRate;
		int jointType;
		float physicsScale;
#pragma GE_DefaultValue(assemblyRate, "0.03")
#pragma GE_DefaultValue(physicsScale, "0.01")

#pragma GE_Private
		gePhysicsJoint *			jointData;
#pragma GE_Origin(Origin)
}   PhysicsJoint;

#pragma GE_Type("Item.ico")

// PhysicalSystem
typedef struct PhysicalSystem
{
#pragma GE_Published
	geVec3d						Origin;
	PhysicsObject		*	physicsObjectListHeadPtr;
	PhysicsJoint				*	jointListHeadPtr;
#pragma GE_Private
//		AnyStruct			*	physsysData;
		gePhysicsSystem	*				physsysData;
#pragma GE_Origin(Origin)
} PhysicalSystem;

// ForceField

enum
{
	FALLOFF_NONE = 0,
	FALLOFF_ONE_OVER_D,
	FALLOFF_ONE_OVER_DSQUARED
};

#pragma GE_Type("Item.ico")

typedef struct ForceField
{
#pragma GE_Published
	geVec3d						Origin;
	float							radius;
	float							strength;
	int								falloffType;
	int								affectsPlayers;
	int								affectsPhysicsObjects;

#pragma GE_DefaultValue(radius, "50.0")
#pragma GE_DefaultValue(falloffType, "1")
#pragma GE_DefaultValue(affectsPlayers, "1")
#pragma GE_DefaultValue(affectsPhysicsObjects, "1")

#pragma GE_Origin(Origin)
} ForceField;

// Door
#pragma GE_Type("Model.ico")
typedef struct  Door
{
#pragma GE_Published
    geWorld_Model   *Model;
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   Door;

// MovingPlat
#pragma GE_Type("Model.ico")
typedef struct  MovingPlat
{
#pragma GE_Published
    geWorld_Model	*Model;
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   MovingPlat;

// ChangeLevel
#pragma GE_Type("Player.ico")
typedef struct ChangeLevel
{
#pragma GE_Published
    geWorld_Model	*Model;
	char			*LevelName;
    geVec3d			Origin;
#pragma GE_Origin(Origin)
} ChangeLevel;

// ItemHealth
#pragma GE_Type("Item.ico")
typedef struct  ItemHealth
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemHealth;

// ItemArmor
#pragma GE_Type("Item.ico")
typedef struct ItemArmor
{
#pragma GE_Published
    geVec3d     Origin;
#pragma GE_Origin(Origin)
} ItemArmor;

// ItemRocket
#pragma GE_Type("Item.ico")
typedef struct  ItemRocket
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemRocket;

// ItemRocketAmmo
#pragma GE_Type("Item.ico")
typedef struct  ItemRocketAmmo
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemRocketAmmo;

// ItemGrenade
#pragma GE_Type("Item.ico")
typedef struct  ItemGrenade
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemGrenade;

// ItemGrenadeAmmo
#pragma GE_Type("Item.ico")
typedef struct  ItemGrenadeAmmo
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemGrenadeAmmo;

// ItemShredder
#pragma GE_Type("Item.ico")
typedef struct  ItemShredder
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemShredder;

// ItemShredderAmmo
#pragma GE_Type("Item.ico")
typedef struct  ItemShredderAmmo
{
#pragma GE_Published
    geVec3d			Origin;
#pragma GE_Origin(Origin)
}   ItemShredderAmmo;

// FogLight
#pragma GE_Type("FogLight.ico")
typedef struct FogLight
{

#pragma GE_Published
	geVec3d		Origin;
	GE_RGBA		Color;
	float		Brightness;
	float		Radius;
#pragma GE_Origin(Origin)
} FogLight;

#ifdef __cplusplus
}
#endif

#pragma warning( default : 4068 )

#endif
