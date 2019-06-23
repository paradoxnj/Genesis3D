
/****************************************************************************/
/*    FILE: bot.h                                                        */
/****************************************************************************/

#ifndef BOT_H
#define BOT_H

#include "_bot.h"
#include "botact.h"

#define BOT_DEBUG 1

// Bot Game Side
geBoolean Bot_Control(GenVSI *VSI, void *PlayerData, float Time);
GenVSI_CMove *Bot_AI_Control(void);

// Player Game Side
//geBoolean CheckVelocity(GenVSI *VSI, void *PlayerData, float Time);
geBoolean CheckPlayer(GenVSI *VSI, void *PlayerData);
void SetupPlayerXForm(GenVSI *VSI, void *PlayerData, float Time);

void Bot_MatchStart(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
void Bot_ActorStart(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);


// Function
typedef geBoolean (*CONTROLp)(GenVSI *VSI, void *PlayerData, float Time);

// Call functions based on a random range value
typedef struct 
    {
    short range;
    //CONTROLp action;
	void *action;
    //geBoolean (*action)(GenVSI *, void *, float);
    } DECISION, *DECISIONp;    

// Personality structure
struct PERSONALITYstruct
    {
    DECISIONp Battle;
    DECISIONp Offense;
    DECISIONp Surprised;
    DECISIONp Evasive;
    DECISIONp LostTarget;
    DECISIONp CloseRange;
	};
    
geBoolean Bot_ModeChange(GenVSI *VSI, void *PlayerData, int32 NewMode, geBoolean Think, float Time);
geBoolean Bot_ClearTrack(GenVSI *VSI, void *PlayerData);
geBoolean Bot_FinishTrack(GenVSI *VSI, void *PlayerData);

geBoolean Bot_InitGenericMove(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_FindNewMoveVec(GenVSI *VSI, void *PlayerData, const geVec3d *TargetPos, int32 dir, float DistToMove, geVec3d *);
geBoolean Bot_Physics(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_OverLedge(GenVSI *VSI, void *PlayerData, geVec3d*);
GPlayer	*Bot_PickTgtPlayer(GenVSI *VSI, void *PlayerData, geBoolean);
geBoolean Bot_LeaveTrack(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_Animate(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_GetContents(GenVSI *VSI, void *PlayerData, float Time);
int16 Bot_ComparePlayers(GenVSI *VSI, void *PlayerData1, void *PlayerData2);
int32 Bot_CompareWeapons(GenVSI *VSI, void *PlayerData, void *TgtPlayerData);
geBoolean Bot_SetupXForm(GenVSI *VSI, void *PlayerData, geVec3d *OrientVec);
geBoolean Bot_Shoot(GenVSI *VSI, void *PlayerData, geVec3d *ShootPosition, float Time);
geBoolean Bot_Keys(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_MoveToPoint(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitMoveFree(GenVSI *VSI, void *PlayerData, geVec3d *Pos);
geBoolean Bot_MoveFree(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitMoveToPoint(GenVSI *VSI, void *PlayerData, geVec3d *Pos);
geBoolean Bot_PastPoint(geVec3d *Pos, geVec3d *MoveVector, geVec3d *TgtPos);
geBoolean Bot_CheckPosition(GenVSI *VSI, void *PlayerData);
int32 Bot_GetRank(GenVSI *VSI, void *PlayerData);

geBoolean Bot_InitWaitForPlayer(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_WaitForPlayer(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_InitWaitForEntityVisible(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_WaitForEntityVisible(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_InitWaitForEntityDist(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_WaitForEntityDist(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_InitShootPoint(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ShootPoint(GenVSI *VSI, void *PlayerData, float Time);

geBoolean Bot_Reposition(GenVSI *VSI, void *PlayerData, geVec3d *, int32);
void Bot_WeaponSetFromArray(GenVSI *VSI, void *PlayerData, int32 *WeaponArr);
geBoolean Bot_SetWeapon(GenVSI *VSI, void *PlayerData, int16 WeaponNum);

geBoolean Bot_SetupShootXForm(GenVSI *VSI, void *PlayerData, geVec3d *TargetVec);
int32 Bot_GetRangeIndex(GenVSI *VSI, void *PlayerData);
int32 Bot_GetStrengthIndex(GenVSI *VSI, void *PlayerData);

geBoolean Bot_CanSeePointToPoint(geWorld *World, geVec3d *Pos1, geVec3d *Pos2);
geBoolean Bot_CanSeePlayerToPlayer(geWorld *World, geVec3d *Pos1, geVec3d *Pos2);
geBoolean Bot_CanSeePlayerToPoint(geWorld *World, geVec3d *Pos1, geVec3d *Pos2);
Track *Bot_FindTrackToGoal(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ValidateTrackPoints(GenVSI *VSI, void *PlayerData, Track* t);
geBoolean Bot_ValidateMultiTrackPoints(GenVSI *VSI, void *PlayerData, Track* t);

geBoolean Bot_FireWeapon(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitFindMultiTrack(GenVSI *VSI, void *PlayerData, geVec3d *DestPos, float Time);
Track *Bot_FindTrack(GenVSI *VSI, void *PlayerData, int32 TrackArr[]);

GPlayer *GetBotMatchSpawn(GenVSI *VSI);
BotActorStart *Bot_GetActorStart(GenVSI *VSI, void *PlayerData);
void BlockActor_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName);
geBoolean Bot_ActionGetHealth(GenVSI *VSI, void *PlayerData, float Range, float Time);
geBoolean Bot_ActionGetWeapon(GenVSI *VSI, void *PlayerData, float Range, float Time);
GPlayer *Bot_FindRandomItem(GenVSI *VSI, geVec3d *Pos, char *ClassList[]);
Track *Bot_FindRandomTrack(GenVSI *VSI, geVec3d *Pos, int32 *TrackTypeList);

// Bot_Init functions 
geBoolean Bot_InitRunCloser(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitMoveCloser(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitTrackToGoal(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitTrackAwayGoal(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitMaintain(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitMoveAway(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitRunAway(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitUnload(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitFindPlayer(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitWander(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitHide(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_InitFindMultiTrackAway(GenVSI *VSI, void *PlayerData, geVec3d *, float Time);

// ModeThink functions - called when a)finished with action b)encountered somthing unexpected
geBoolean Bot_ModeThink(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkAttack(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkRetreat(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkWander(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkFindPlayer(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkFindPlayerQuick(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkCrowdPlayer(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkGoalPoint(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkOnTrack(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkUnstick(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ModeThinkWanderGoal(GenVSI *VSI, void *PlayerData, float Time);
int32 Bot_RankWeapons(GenVSI *VSI, void *PlayerData);

#endif
