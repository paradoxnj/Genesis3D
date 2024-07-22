#ifndef	BOTACTOR_H
#define	BOTACTOR_H

#include	"GENESIS.H"

#pragma warning( disable : 4068 )

//#pragma GE_Type("BotActor.bmp")
//typedef struct BotActorStart BotActorStart;

#pragma GE_Type("BotActor.bmp")
typedef struct BotActorStart {
#pragma GE_Published

geVec3d		origin;
geBoolean   Respawn;
geBoolean   IgnoreTurrets;
int			SkillLevel;

#pragma GE_Private

char		*Ptr;

#pragma GE_DefaultValue(SkillLevel, "1")
#pragma GE_Documentation(SkillLevel, "From 1 to 4 - Higher is more difficult.")

#pragma GE_DefaultValue(Respawn, "False")
#pragma GE_Documentation(Respawn, "Respawn actor after death.")

#pragma GE_DefaultValue(IgnoreTurrets, "True")
#pragma GE_Documentation(IgnoreTurrets, "Actor will not shoot at turrets if True.")

#pragma GE_Origin(origin)
} BotActorStart;


#pragma GE_Type("BlockActor.bmp")
typedef struct BlockActor BlockActor;

#pragma GE_Type("BlockActor.bmp")
typedef struct BlockActor {
#pragma GE_Published

geVec3d		Origin;
geWorld_Model *Model;

#pragma GE_Documentation(Model, "Blocking Model - Actors will not move into this model.")

#pragma GE_Origin(Origin)
} BlockActor;

#pragma warning( default : 4068 )

#endif
