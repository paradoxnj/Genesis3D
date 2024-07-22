/****************************************************************************/
/*    FILE: Bot.c														*/
/****************************************************************************/
#include <Windows.h>
#include <assert.h>
#include <Math.h>

#include "GMain.h"

#include "_bot.h"
#include "track.h"
#include "Bot.h"
#include "botmatch.h"
#include "botact.h"

extern void GenVS_Error(const char *Msg, ...);
#define NEW_WANDER
//#define WANDER_TEST

geBoolean Bot_TrackAction(GenVSI *VSI, void *PlayerData, const TrackPt* ThisPoint, const TrackPt* NextPoint, float Time);

//=====================================================================================
//=====================================================================================
//
//	Forward function declarations
//
//=====================================================================================
//=====================================================================================

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

// ModeAction functions - called every iteration after the Action functions
geBoolean Bot_ModeActionRetreat(GenVSI *VSI, void *PlayerData, float Time);

void *ChooseAction(DECISION decision[]);
geBoolean Bot_SuicideTest(GenVSI *VSI, void *PlayerData, float Time);

// Game functions I need to call
geBoolean MovePlayerUpStep(GenVSI *VSI, void *PlayerData, GE_Collision *Collision);
void SwitchToNextBestWeapon(GenVSI *VSI, void *PlayerData);
geBoolean PlayerDead(GPlayer *Player);
int32 PlayerLiquid(GPlayer *Player);
void ValidateWeapon(GenVSI *VSI, void *PlayerData);
void FireBlaster(GenVSI *VSI, void *PlayerData, float Time);
void FireShredder(GenVSI *VSI, void *PlayerData, float Time);

//=====================================================================================
//=====================================================================================
//
//	Bot - Data and Defines
//
//=====================================================================================
//=====================================================================================

#define BOT_DIR_AWAY    -1
#define BOT_DIR_TOWARD   1
#define BOT_DIR_NONE     0

#define BOT_SEARCH_Y_RANGE 40.0f

#define BOT_STRENGTH_LOW(hd, wd) ((hd) < -30 && (wd) < -30)
#define BOT_STRENGTH_HIGH(hd, wd) ((hd) > 30 && (wd) > 30)

#define CONTENTS_WATER		GE_CONTENTS_USER1
#define CONTENTS_LAVA		GE_CONTENTS_USER2

#define BOT_GROUND_FRICTION		7.0f
#define BOT_LIQUID_FRICTION		2.5f
#define BOT_AIR_FRICTION		1.00f
#define BOT_GRAVITY				1600.0f
#define BOT_JUMP_THRUST			700//17000.0f
#define BOT_SIDE_SPEED			4000.0f
#define BOT_RUN_SPEED			3700.0f

#define CLOSE_RANGE_DIST 400.0f
#define MED_RANGE_DIST   900.0f
#define LONG_RANGE_DIST  1600.0f

#define CLOSE_RANGE      0 
#define MED_RANGE        1
#define LONG_RANGE       2

char *ItemHealthList[] = {"ItemHealth", "ItemArmor", NULL};
char *ItemWeaponList[] = {"ItemRocket", "ItemGrenade", "ItemShredder", NULL};
char *ItemAmmoList[] = {"ItemRocketAmmo", "ItemGrenadeAmmo", "ItemShredderAmmo", NULL};

int32 HealthTrackList[] =
    {
	//TRACK_TYPE_SCAN_HEALTH,
	-1
    };

int32 WeaponTrackList[] =
    {
	//TRACK_TYPE_SCAN_WEAPON_AMMO,
    //TRACK_TYPE_SCAN,
    -1
    };

int32 HideTrackList[] =
    {
	TRACK_TYPE_HIDE,
    -1
    };

static int32 WeaponSetClose[] = {3, 0, 1, 2};
static int32 WeaponSetMed[]   = {3, 1, 2, 0};
static int32 WeaponSetLong[]  = {3, 2, 1, 0};
static int32 *WeaponSetRange[] = {WeaponSetClose, WeaponSetMed, WeaponSetLong};

static GPlayer *BotDeathMatchStart = NULL;

typedef struct
{
	float		RunSpeed;		// Scale value for running speed
	CONTROLp    Action;			// Function that's called every thing
    TrackData   TrackInfo;		// Current track info - track/point/direction
    TrackData   TrackInfoPrev;	// Last point track info
    int32       Dir;			// TOWARD/AWAY from player - Should prob be MODE
	geVec3d		TgtPos;			// Current point you are moving towards
    GPlayer		*TgtPlayer;		// The player you are aware of at the moment
	geVec3d     MoveVec;		// Current Movement Vector
	float		TimeOut;		// Limit certain actions to a maximum time
	float		ModeTimeOut;	// Limit modes to a maximum time
	int32		ShootCount;		// Current number of shots
	float       TimeSeen;		// Time the player has been in view
	float       TimeNotSeen;    // Time the player has been out of view
	float       ModeTimeSeen;	// For Mode Routines only - Get reset on mode changes
	float       ModeTimeNotSeen;// For Mode Routines only - Get reset on mode changes
	float		ActiveTime;     // For Actor Bots only - limits movement when not in view
    int32       Mode;           // Current Mode of Bot
	int32		Bumps;			// Number of bumps in a row
	int32		LedgeBumps;		// Number of Ledge bumbs in a row
	geVec3d		GoalPos;	    // Current goal - could be the player or any other
							    // point.
	int8		BotType;        // Multi-player or Actor
	geBoolean	PastFirstTrackPoint; //Flag - sort of self documenting :Q
	int32		WeaponDiff;
	int32		HealthDiff;
    Stack       TrackStack;     // Stack of Tracks
	int32		HealthCheck;	// health check
	float		PickTimeout;
	float		TimeSinceTrack;
	GPlayer		*ClientPlayer;
	geBoolean	FaceTgtPlayerOnRetreat;
	int32		BotTgtPicked;

} Bot_Var;

geVec3d RayMins = {-1.0f,-1.0f,-1.0f}, RayMaxs = {1.0f,1.0f,1.0f};

enum {	
	MODE_NULL = -1,
	MODE_ATTACK = 0, 
	MODE_RETREAT, 
	MODE_WANDER, 
	MODE_FIND_PLAYER, 
	MODE_FIND_PLAYER_QUICK, 
    MODE_CROWD_PLAYER,
    MODE_GOAL_POINT,
	MODE_ON_TRACK,
    MODE_RETREAT_ON_TRACK,
    MODE_UNSTICK,
	MODE_WANDER_ON_TRACK,
	MODE_MAX,
	};

int32 ModeTable[MODE_MAX] =
    {
	MODE_ATTACK, 
	MODE_RETREAT, 
	MODE_WANDER, 
	MODE_FIND_PLAYER, 
	MODE_FIND_PLAYER_QUICK,
    MODE_CROWD_PLAYER,
    MODE_GOAL_POINT,
	MODE_ON_TRACK,
    MODE_RETREAT_ON_TRACK,
    MODE_UNSTICK,
	MODE_WANDER_ON_TRACK,
    };

// debug strings
char *ModeText[] = 
	{
	"MODE_ATTACK", 
	"MODE_RETREAT", 
	"MODE_WANDER", 
	"MODE_FIND_PLAYER", 
	"MODE_FIND_PLAYER_QUICK",
    "MODE_CROWD_PLAYER",
    "MODE_GOAL_POINT",
	"MODE_ON_TRACK",
    "MODE_RETREAT_ON_TRACK",
    "MODE_UNSTICK",
	"MODE_WANDER_ON_TRACK",
	};

CONTROLp Bot_ModeThinkFunc[MODE_MAX] = 
	{
	Bot_ModeThinkAttack,
	Bot_ModeThinkRetreat,
	Bot_ModeThinkWander,
	Bot_ModeThinkFindPlayer,
	Bot_ModeThinkFindPlayerQuick,
    Bot_ModeThinkCrowdPlayer,
    Bot_ModeThinkGoalPoint,
	Bot_ModeThinkOnTrack,
	Bot_ModeThinkOnTrack,//RetreatOnTrack,
	Bot_ModeThinkUnstick,
	Bot_ModeThinkOnTrack,//WanderGoal,
	};

CONTROLp Bot_ModeAction[MODE_MAX] = 
	{
	NULL,//Attack,
	Bot_ModeActionRetreat,//Retreat,
	NULL,//Wander,
	NULL,//FindPlayer,
	NULL,//FindPlayerQuick,
    NULL,//CrowdPlayer,
    NULL,//GoalPoint,
	NULL,//OnTrack,
	Bot_ModeActionRetreat,//RetreatOnTrack,
	NULL,//Unstick,
	NULL,//WanderGoal,
	};


int RankTable[3][3] = 
{//close    med     long
	{0,		2,		4},		//weak
	{5,     6,      7},		//med
	{6,		10,		9},	    //strong
};

#if 0
//formula for leading a player - would be too deadly I think
dist = Distance(sp->x, sp->y, hp->x, hp->y);
time_to_target = dist/wp->xvel;
lead_dist = time_to_target*hp->vel;
#endif

// Global Debug vars
geBoolean GodMode = GE_FALSE;
geBoolean PathLight = GE_FALSE;
geBoolean BotDebugPrint = GE_FALSE;
geBoolean MultiPathLight = GE_FALSE;

void Bot_SetLighting(GenVSI *VSI, void *PlayerData);
geBoolean Bot_TargetTest(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_ShootFoot(GenVSI *VSI, void *PlayerData, float Time);
geBoolean Bot_Suicide(GenVSI *VSI, void *PlayerData, float Time);
//=====================================================================================
//=====================================================================================
//
//	Bot_Control
//
//=====================================================================================
//=====================================================================================
geBoolean Bot_Control(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geWorld			*World;
	geBoolean		CanSee;

	World = GenVSI_GetWorld(VSI);
	assert(World);

	Player = (GPlayer*)PlayerData;
	assert(Player);

	DBot = static_cast<Bot_Var*>(Player->userData);

	assert(DBot);

	Player->Time += Time;
	DBot->ModeTimeOut += Time;
	DBot->TimeSinceTrack += Time;

	//Bot_SetLighting(VSI, Player);

	if (Bot_SuicideTest(VSI, Player, Time))
		return GE_TRUE;

	Bot_TargetTest(VSI, Player, Time);

    assert(DBot->TgtPlayer);

    CanSee = Bot_CanSeePlayerToPlayer(World, &Player->XForm.Translation, &DBot->TgtPlayer->XForm.Translation);
    if (CanSee)
		{
		DBot->TimeNotSeen = 0.0f;
		DBot->TimeSeen += Time;
		DBot->ModeTimeSeen += Time;
		}
	else
		{
		DBot->TimeSeen = 0.0f;
		DBot->TimeNotSeen += Time;
		DBot->ModeTimeNotSeen += Time;
		}

	// Actor Bot
	if (DBot->BotType == 1)
		{
	    if (CanSee)
			{
			// stay active for 20 seconds after bot goes from ALL players views
			DBot->ActiveTime = 20.0f;
			}
		else
			{
			if (DBot->ActiveTime <= 0.0f)
				return GE_TRUE;

			DBot->ActiveTime -= Time;
			}
		}
	
	Bot_Keys(VSI, Player, Time);

	// default goal to target player if not on a track or similar circumstance
	if (!Track_OnTrack(&DBot->TrackInfo) && !(DBot->Mode == MODE_GOAL_POINT))
		DBot->GoalPos = DBot->TgtPlayer->XForm.Translation; 

	// Purely for debug purposes so we can break at any point
    #if BOT_DEBUG
	if (GetAsyncKeyState('M') & 0x8000)
		Player = Player;
    #endif

    assert(DBot->Action);
    DBot->Action(VSI, PlayerData, Time);

	// mode specific action testing - called every time through the loop
	// for certain modes
	if (Bot_ModeAction[DBot->Mode])
		Bot_ModeAction[DBot->Mode](VSI, PlayerData, Time);

    return GE_TRUE;
    }


//=====================================================================================
//	Bot_TargetTest
//=====================================================================================
geBoolean Bot_TargetTest(GenVSI *VSI, void *PlayerData, float Time)
	{
	GPlayer			*Player,*NewTgtPlayer;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	// time to check for new target
	if (GenVSI_GetTime(VSI) >= DBot->PickTimeout || DBot->TgtPlayer == NULL)
		{
   		NewTgtPlayer = Bot_PickTgtPlayer(VSI, PlayerData, GE_FALSE);

		// setup timout again
		if (GenVSI_IsClientBot(VSI, NewTgtPlayer->ClientHandle))
			{
			DBot->BotTgtPicked++;
			DBot->PickTimeout = GenVSI_GetTime(VSI) + 3.0f;
			}
		else
			{
			DBot->BotTgtPicked = 0;
			DBot->PickTimeout = GenVSI_GetTime(VSI) + 10.0f;
			}

		if (++DBot->BotTgtPicked > 2)
			{
			// Bot was picked too many times - force human player
			DBot->BotTgtPicked = 0;
			NewTgtPlayer = Bot_PickTgtPlayer(VSI, PlayerData, GE_TRUE);
			DBot->PickTimeout = GenVSI_GetTime(VSI) + 12.0f;
			}

		// changed targets - reset vars
		if (NewTgtPlayer != DBot->TgtPlayer)	//changed targets
			{
			DBot->TgtPlayer = NewTgtPlayer;
			DBot->TimeNotSeen = 0.0f;
			DBot->TimeSeen = 0.0f;
			}
		}

	return GE_TRUE;
}

//=====================================================================================
//	Bot_SuicideTest
//=====================================================================================
geBoolean Bot_SuicideTest(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geBoolean DammagePlayer(GenVSI *VSI, void *PlayerData, void *TargetData, int32 Amount, float Power, float Time);
	geBoolean KillPlayer(GenVSI *VSI, void *PlayerData, void *TargetData, float Time);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	if (Track_OnTrack(&DBot->TrackInfo))
		DBot->TimeSinceTrack = 0.0f;
	else
	if (DBot->TimeSinceTrack >= 20.0f)
	{
		GPlayer *SaveOwner;

		Player->Health = 0;
		SaveOwner = Player->Owner;
		Player->Owner = Player;
		KillPlayer(VSI, Player, Player, Time);
		Player->Owner = SaveOwner;
/*
		DammagePlayer(VSI, PlayerData, TargetData, 300, 0, Time);
		// suicide after no tracks for a while
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
		if (!PlayerDead(Player)) Bot_Suicide(VSI, Player, Time);
*/
		return GE_TRUE;
	}

	return GE_FALSE;
}

//=====================================================================================
//	Bot_ModeThink - Run through the mode code
//=====================================================================================
geBoolean Bot_ModeThink(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	//if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Bot Think Mode %s", ModeText[DBot->Mode]);

	Bot_ModeThinkFunc[DBot->Mode](VSI, PlayerData, Time);

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeThinkLedge
//=====================================================================================
geBoolean Bot_ModeThinkLedge(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;
	int32 dir;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

    // Sometimes when making jumps the bot may land on a ledge
    // This attempts to see if we are in the middle of a track - not the first point
	// Also allows running off of ledges
    if (Track_OnTrack(&DBot->TrackInfo))
        {
		#if 1
        TrackPt *pt,*fpt;
        pt = Track_GetPoint(&DBot->TrackInfo);
        fpt = Track_GetFirstPoint(&DBot->TrackInfo);
        if (pt != fpt)
            return GE_FALSE;  // do notdetect a ledge
		#else
		if (DBot->PastFirstTrackPoint)
			return GE_FALSE;
		#endif

		Player->XForm.Translation = Player->LastGoodPos;
		geVec3d_Clear(&Player->Velocity);

		if (!Bot_InitTrackToGoal(VSI, Player, Time))
			{
			dir = RandomRange(3) - 1; //-1 to 1
			Bot_Reposition(VSI, Player, &DBot->GoalPos, dir); //Try the opposite direction
			}
        }
	else
		{
			Player->XForm.Translation = Player->LastGoodPos;
			geVec3d_Clear(&Player->Velocity);
		    dir = RandomRange(3) - 1; //-1 to 1
		    Bot_Reposition(VSI, Player, &DBot->GoalPos, dir); //Try the opposite direction
		}


    return GE_TRUE;
}

void Bot_SetMode(void *PlayerData, int32 NewMode)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	switch (NewMode)
		{
		case MODE_RETREAT:
		case MODE_RETREAT_ON_TRACK:
			DBot->HealthCheck = Player->Health;
			DBot->FaceTgtPlayerOnRetreat = GE_FALSE;
			break;
		}

	DBot->Mode = NewMode;
}


//=====================================================================================
//	Bot_ModeChange - This is the only place allowed to set modes
//=====================================================================================
geBoolean Bot_ModeChange(GenVSI *VSI, void *PlayerData, int32 NewMode, geBoolean Think, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;
	int32				*iptr;
	int32				Rank,RNdx,SNdx;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	// Reset Mode specific timers & counters
	DBot->ModeTimeSeen = 0.0f;
	DBot->ModeTimeNotSeen = 0.0f;
	DBot->ModeTimeOut = 0.0f;

	// get ranking info
	RNdx = Bot_GetRangeIndex(VSI, Player);
	SNdx = Bot_GetStrengthIndex(VSI, Player);
	Rank = RankTable[RNdx][SNdx];

    Bot_WeaponSetFromArray(VSI, Player, WeaponSetRange[RNdx]);

	// Set specific mode
	if (NewMode != MODE_NULL)
		{
		Bot_SetMode(Player, NewMode);
		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	// CAN NOT SEE
	if (DBot->TimeNotSeen > 10.0f)
		{
	#ifdef NEW_WANDER
		if (RandomRange(1000) < 500)
			Bot_SetMode(Player, MODE_FIND_PLAYER_QUICK);
		else
			Bot_SetMode(Player, MODE_WANDER);
	#else
		Bot_SetMode(Player, MODE_FIND_PLAYER_QUICK);
	#endif
		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	if (DBot->TimeNotSeen > 5.0f)
		{
		DECISION *dptr;

		static DECISION d[] =
			{
#ifdef WANDER_TEST
			{1000, &ModeTable[MODE_WANDER]},
#else
	#ifdef NEW_WANDER
			{200,  &ModeTable[MODE_FIND_PLAYER]},
			{500,  &ModeTable[MODE_FIND_PLAYER_QUICK]},
			{1000, &ModeTable[MODE_WANDER]},
	#else
			{600,  &ModeTable[MODE_FIND_PLAYER]},
			{1000,  &ModeTable[MODE_FIND_PLAYER_QUICK]},
	#endif
#endif
			};

		static DECISION d2[] =
			{
#ifdef WANDER_TEST
			{1000, &ModeTable[MODE_WANDER]},
#else
	#ifdef NEW_WANDER
			{200,  &ModeTable[MODE_FIND_PLAYER]},
			{900,  &ModeTable[MODE_FIND_PLAYER_QUICK]},
			{1000, &ModeTable[MODE_WANDER]},
	#else
			{400,  &ModeTable[MODE_FIND_PLAYER]},
			{1000,  &ModeTable[MODE_FIND_PLAYER_QUICK]},
	#endif
#endif
			};

		if (Rank >= 8)
			dptr = d2;
		else
			dptr = d;

		iptr = static_cast<int32*>(ChooseAction(dptr));
		Bot_SetMode(Player, *iptr);

		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	if (DBot->TimeNotSeen > 0.0f)
		{
		static DECISION d[] =
			{
			{600,  &ModeTable[MODE_FIND_PLAYER]},
			{1000,  &ModeTable[MODE_FIND_PLAYER_QUICK]},
			//{1000, &ModeTable[MODE_RETREAT]},
			};

		iptr = static_cast<int32*>(ChooseAction(d));
		Bot_SetMode(Player, *iptr);

		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	// CANSEE
	if (DBot->TimeSeen > 15.0f)
		{
		if (Rank <= 3)
			Bot_SetMode(Player, MODE_RETREAT);
		else
			Bot_SetMode(Player, MODE_ATTACK);

		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	if (DBot->TimeSeen > 5.0f)
		{
		if (Rank <= 2)
			Bot_SetMode(Player, MODE_RETREAT);
		else
			Bot_SetMode(Player, MODE_ATTACK);

		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	if (DBot->TimeSeen > 0.0f)
		{
		if (Rank <= 1)
			Bot_SetMode(Player, MODE_RETREAT);
		else
			Bot_SetMode(Player, MODE_ATTACK);

		if (Think)
			Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

return GE_FALSE;
}


//=====================================================================================
//	Bot_ModeThinkRetreat
//=====================================================================================
geBoolean Bot_ModeThinkRetreat(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;
	GPlayer *GoalPlayer;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->HealthCheck = Player->Health;

	if (Bot_ActionGetHealth(VSI, Player, 500.0f, Time))
		return GE_TRUE;
	if (Bot_ActionGetWeapon(VSI, Player, 500.0f, Time))
		return GE_TRUE;

	// first try and find some item to run towards
	if (GoalPlayer = Bot_FindRandomItem(VSI, &Player->XForm.Translation, ItemHealthList))
		{
		if (Bot_InitFindMultiTrack(VSI, Player, &GoalPlayer->XForm.Translation, Time))
			{
			Bot_ModeChange(VSI, Player, MODE_RETREAT, GE_FALSE, Time);
			return GE_TRUE;
			}
		}

	if (Bot_InitFindMultiTrackAway(VSI, Player, &DBot->TgtPlayer->XForm.Translation, Time))
		{
		Bot_ModeChange(VSI, Player, MODE_RETREAT_ON_TRACK, GE_FALSE, Time);
		return GE_TRUE;
		}

	// ModeTimeOut on this operation
	if (DBot->ModeTimeOut > 10.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
        return GE_TRUE;
		}
	// cant get away
	if (DBot->ModeTimeSeen > 5.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
        return GE_TRUE;
		}	

	if (DBot->ModeTimeNotSeen > 6.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
		return GE_TRUE;
		}	
	if (DBot->ModeTimeNotSeen > 3.0f)
		{
		if (Bot_InitHide(VSI, Player, Time))
			{
			// this sets the Action
			return GE_TRUE;
			}
		}	

	// this sets the Action
	Bot_InitRunAway(VSI, Player, Time);

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeActionRetreat
//=====================================================================================
geBoolean Bot_ModeActionRetreat(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	// if hit when retreating - stand and fight like a (wo)man
	if (DBot->HealthCheck < Player->Health - 20)
		{
		DBot->FaceTgtPlayerOnRetreat = GE_TRUE;
		//Bot_ModeChange(VSI, Player, MODE_ATTACK, GE_FALSE, Time);
		return GE_TRUE;
		}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_ModeThinkFindPlayerQuick
//=====================================================================================
geBoolean Bot_ModeThinkFindPlayerQuick(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	if (DBot->ModeTimeOut > 20.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
        return GE_TRUE;
		}

	if (Bot_InitFindMultiTrack(VSI, PlayerData, &DBot->TgtPlayer->XForm.Translation, Time))
		{
		Bot_ModeChange(VSI, Player, MODE_ON_TRACK, GE_FALSE, Time);
		return GE_TRUE;
		}
	else
		{
		GPlayer *GoalPlayer;

		if (GoalPlayer = Bot_FindRandomItem(VSI, &Player->XForm.Translation, ItemHealthList))
			{
			if (Bot_InitFindMultiTrack(VSI, Player, &GoalPlayer->XForm.Translation, Time))
				{
				Bot_ModeChange(VSI, Player, MODE_WANDER_ON_TRACK, GE_FALSE, Time);
				return GE_TRUE;
				}
			}
		}

	if (Bot_ActionGetHealth(VSI, Player, 400.0f, Time))
		return GE_TRUE;
	if (Bot_ActionGetWeapon(VSI, Player, 400.0f, Time))
		return GE_TRUE;

	if (Bot_InitHide(VSI, Player, Time))
		return GE_TRUE;

	// Found Player
	if (DBot->ModeTimeSeen > 1.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
		return GE_TRUE;
		}
	
	Bot_InitRunCloser(VSI, Player, Time);

	return GE_TRUE;
}


//=====================================================================================
//	Bot_ModeThinkFindPlayer
//=====================================================================================
geBoolean Bot_ModeThinkFindPlayer(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->WeaponDiff = Bot_CompareWeapons(VSI, Player, DBot->TgtPlayer);

	// outgunned
	if (DBot->WeaponDiff < -40)
		{
		Bot_ModeChange(VSI, Player, MODE_WANDER, GE_FALSE, Time);
	    return GE_TRUE;
		}

	if (DBot->ModeTimeOut > 7.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_FIND_PLAYER_QUICK, GE_TRUE, Time);
        return GE_TRUE;
		}

	if (Bot_ActionGetHealth(VSI, Player, 600.0f, Time))
		return GE_TRUE;
	if (Bot_ActionGetWeapon(VSI, Player, 600.0f, Time))
		return GE_TRUE;

	// Found Player
	if (DBot->ModeTimeSeen > 0.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
		return GE_TRUE;
		}

	if (DBot->ModeTimeNotSeen > 3.0f)
		{
		if (Bot_InitHide(VSI, Player, Time))
			return GE_TRUE;
		}

	Bot_InitRunCloser(VSI, Player, Time);

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeThinkWander
//=====================================================================================
geBoolean Bot_ModeThinkWander(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	GPlayer				*GoalPlayer;
	Bot_Var				*DBot;
	Track				*GoalTrack;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	if (GoalTrack = Bot_FindRandomTrack(VSI, &Player->XForm.Translation, HideTrackList))
		{
		if (Bot_InitFindMultiTrack(VSI, Player, GoalTrack->PointList[0].Pos, Time))
			{
			Bot_ModeChange(VSI, Player, MODE_ON_TRACK, GE_FALSE, Time);
			return GE_TRUE;
			}
		}

	// InitWanderGoal
	if (GoalPlayer = Bot_FindRandomItem(VSI, &Player->XForm.Translation, ItemHealthList))
		{
		if (Bot_InitFindMultiTrack(VSI, Player, &GoalPlayer->XForm.Translation, Time))
			{
			Bot_ModeChange(VSI, Player, MODE_WANDER_ON_TRACK, GE_FALSE, Time);
			return GE_TRUE;
			}
		}

	if (Bot_ActionGetHealth(VSI, Player, 1200.0f, Time))
		return GE_TRUE;
	if (Bot_ActionGetWeapon(VSI, Player, 1200.0f, Time))
		return GE_TRUE;

	Bot_ModeChange(VSI, Player, MODE_FIND_PLAYER, GE_FALSE, Time);

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeThinkWanderGoal
//=====================================================================================
geBoolean Bot_ModeThinkWanderGoal(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	if (!Track_OnTrack(&DBot->TrackInfo))
		{
			// Try and find an item
		if (Bot_ActionGetHealth(VSI, Player, 500.0f, Time))
			return GE_TRUE;
		if (Bot_ActionGetWeapon(VSI, Player, 500.0f, Time))
			return GE_TRUE;

		Bot_ModeChange(VSI, Player, MODE_FIND_PLAYER, GE_FALSE, Time);
		}
	else
	// IF HERE THEN HIT SOMETHING!!!!! - if didn't hit then check logic!
	if (Track_OnTrack(&DBot->TrackInfo))
		{
		// This MAY push a track on the stack
		Bot_InitRunCloser(VSI, Player, Time);
		DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
		return GE_TRUE;
		}

	Bot_InitRunCloser(VSI, Player, Time);

	return GE_TRUE;
}


//=====================================================================================
//	Bot_ModeThinkAttack
//=====================================================================================
geBoolean Bot_ModeThinkAttack(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;
	geVec3d				Pos,TgtPlayerPos;
	float				Dist;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

    Pos = Player->XForm.Translation;
    TgtPlayerPos = DBot->TgtPlayer->XForm.Translation;

	DBot->WeaponDiff = Bot_CompareWeapons(VSI, Player, DBot->TgtPlayer);

	// outgunned
	if (DBot->WeaponDiff < -40)
		{
		Bot_ModeChange(VSI, Player, MODE_RETREAT, GE_FALSE, Time);
	    return GE_TRUE;
		}

	if (DBot->ModeTimeNotSeen > 1.5f)
		{
		Bot_ModeChange(VSI, Player, MODE_FIND_PLAYER, GE_TRUE, Time);
        return GE_TRUE;
		}

    if (PlayerDead(DBot->TgtPlayer))
        {
	    if (Bot_InitFindMultiTrackAway(VSI, Player, &DBot->TgtPlayer->XForm.Translation, Time))
		    {
		    Bot_ModeChange(VSI, Player, MODE_RETREAT_ON_TRACK, GE_FALSE, Time);
		    return GE_TRUE;
            }
        else
            {
		    Bot_ModeChange(VSI, Player, MODE_RETREAT, GE_TRUE, Time);
		    return GE_TRUE;
            }
        }

    if (RandomRange(1000) < 300 &&
        Bot_GetRank(VSI, Player) >= 4)
		{
		Bot_ModeChange(VSI, Player, MODE_CROWD_PLAYER, GE_TRUE, Time);
        return GE_TRUE;
		}

	if (DBot->ModeTimeOut > 12.0f || DBot->ModeTimeSeen > 7.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
        return GE_TRUE;
		}

	if (Bot_ActionGetHealth(VSI, Player, 600.0f, Time))
		return GE_TRUE;
	if (Bot_ActionGetWeapon(VSI, Player, 600.0f, Time))
		return GE_TRUE;

	// keep a medium range distance
    Dist = geVec3d_DistanceBetween(&Pos, &TgtPlayerPos);
    if (Dist <= CLOSE_RANGE_DIST)
		{
		Bot_InitMoveAway(VSI, Player, Time);
		return GE_TRUE;
		}
	else
    if (Dist <= MED_RANGE_DIST)
		{
        if (RandomRange(1000) < 200)
		    Bot_InitRunCloser(VSI, Player, Time);
        else
            Bot_InitMaintain(VSI, Player, Time);
		return GE_TRUE;
		}
	else
		{
		Bot_InitRunCloser(VSI, Player, Time);
		return GE_TRUE;
		}

	// health assessment
    DBot->HealthDiff = Player->Health - DBot->TgtPlayer->Health;
	DBot->WeaponDiff = Bot_CompareWeapons(VSI, Player, DBot->TgtPlayer);

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeThinkCrowdPlayer
//=====================================================================================
geBoolean Bot_ModeThinkCrowdPlayer(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->WeaponDiff = Bot_CompareWeapons(VSI, Player, DBot->TgtPlayer);

	// outgunned
	if (DBot->WeaponDiff < -40)
		{
		Bot_ModeChange(VSI, Player, MODE_RETREAT, GE_FALSE, Time);
	    return GE_TRUE;
		}

    if (PlayerDead(DBot->TgtPlayer))
        {
	    if (Bot_InitFindMultiTrackAway(VSI, Player, &DBot->TgtPlayer->XForm.Translation, Time))
		    {
		    Bot_ModeChange(VSI, Player, MODE_RETREAT_ON_TRACK, GE_FALSE, Time);
		    return GE_TRUE;
            }
        else
            {
		    Bot_ModeChange(VSI, Player, MODE_RETREAT, GE_TRUE, Time);
		    return GE_TRUE;
            }
        }

	if (DBot->ModeTimeOut > 6.0f || 
		DBot->ModeTimeNotSeen > 6.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
        return GE_TRUE;
		}

	if (Bot_ActionGetHealth(VSI, Player, 400.0f, Time))
		return GE_TRUE;
	if (Bot_ActionGetWeapon(VSI, Player, 400.0f, Time))
		return GE_TRUE;

    if (Bot_GetRank(VSI, Player) <= 3)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
        return GE_TRUE;
		}

    // crowd the player at all times
	Bot_InitRunCloser(VSI, Player, Time);

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeThinkGoalPoint
//=====================================================================================
geBoolean Bot_ModeThinkGoalPoint(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	// either hit something or made it to the point

	Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);

	return GE_TRUE;
}


//=====================================================================================
//	Bot_ModeThinkOnTrack
//=====================================================================================
geBoolean Bot_ModeThinkOnTrack(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);


	// left the track?
	if (!Track_OnTrack(&DBot->TrackInfo))
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
		return GE_TRUE;
		}

	// IF HERE THEN HIT SOMETHING!!!!! - if didn't hit then check logic!
	if (Track_OnTrack(&DBot->TrackInfo))
		{
		// This MAY push a track on the stack
		Bot_InitRunCloser(VSI, Player, Time);
		DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
		return GE_TRUE;
		}


	return GE_TRUE;
}

//=====================================================================================
//	Bot_ModeThinkUnstick
//=====================================================================================
geBoolean Bot_ModeThinkUnstick(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer				*Player;
	Bot_Var				*DBot;
	int32				dir;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	if (DBot->ModeTimeOut > 3.0f)
		{
		Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
		return GE_TRUE;
		}

	dir = RandomRange(3) - 1;
	Bot_Reposition(VSI, Player, &DBot->TgtPlayer->XForm.Translation, dir);
	Bot_InitMoveFree(VSI, Player, &DBot->TgtPos);

	return GE_TRUE;
}


//=====================================================================================
//	Bot_Destroy
//=====================================================================================
void Bot_Destroy(GenVSI *VSI, void *PlayerData, void *ClassData)
{
	GPlayer			*Player,*Hit;
	Bot_Var			*DBot,*HBot;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	World = GenVSI_GetWorld(VSI);

	// run through players setting resetting any targets pointing to this player
	// being killed 

	Hit = NULL;
	while (1)
	{
		Hit = static_cast<GPlayer*>( GenVSI_GetNextPlayer(VSI, Hit, NULL) );

		if (!Hit)
			break;

		if (Hit == Player)
			break;

		if (Hit->ClientHandle == CLIENT_NULL_HANDLE) // Will not find ACTORS!
			continue;

		if (GenVSI_IsClientBot(VSI, Hit->ClientHandle))
			{
			HBot = static_cast<Bot_Var*>(Hit->userData);
			if (HBot)
				{
					//assert(HBot);

					if (HBot->TgtPlayer == Player)
						{
						HBot->TgtPlayer = NULL;
						}
				}
			}
	}

	Player = (GPlayer*)PlayerData;
    DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

    assert(DBot->TrackStack.Data);
    geRam_Free(DBot->TrackStack.Data);

	geRam_Free(Player->userData);
	Player->userData = NULL;

	//return GE_TRUE;
}


//=====================================================================================
//	ChooseAction
//=====================================================================================
void *ChooseAction(DECISION decision[])
    {
    int32 i, random_value;
    
    random_value = RandomRange(1000);

    for (i = 0; TRUE; i++)
        {
        if (random_value <= decision[i].range)
            {
            return (decision[i].action);
            }
        }
    }


//=====================================================================================
//	Bot_Reposition
//=====================================================================================
geBoolean Bot_Reposition(GenVSI *VSI, void *PlayerData, geVec3d *TgtPos, int32 Dir)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	geVec3d			Vec2Player;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	//geVec3d_Subtract(TgtPos, &Player->XForm.Translation, &Vec2Player);
	geVec3d_Subtract(TgtPos, &DBot->GoalPos, &Vec2Player);

	// TgtPos is where we want to move to
	if (!Bot_FindNewMoveVec(VSI, Player, TgtPos, Dir, 300.0f, &DBot->MoveVec))
	{
		// Couldn't find a dir to move
		if (!Bot_FindNewMoveVec(VSI, Player, TgtPos, -Dir, 250.0f, &DBot->MoveVec))
		{
			// Couldn't find a dir to move - again
			// Getting stuck

        return GE_FALSE;
		}	
	}

    return GE_TRUE;
}


//=====================================================================================
//	Bot_InitFindMultiTrack
//=====================================================================================
geBoolean Bot_InitFindMultiTrack(GenVSI *VSI, void *PlayerData, geVec3d *DestPos, float Time)
{
	Bot_Var			*DBot;
	GPlayer			*Player;
	TrackCB			CB;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	if (!StackIsEmpty(&DBot->TrackStack))
		return GE_FALSE;

	CB.Data = PlayerData;
	CB.CB = Bot_ValidateMultiTrackPoints;

	if (Track_FindMultiTrack (VSI, 
						&Player->XForm.Translation,
						DestPos,
                        BOT_DIR_TOWARD,
						&CB,
                        &DBot->TrackStack))
		{
		DBot->Dir = BOT_DIR_TOWARD;

		Track_NextMultiTrack(VSI, &Player->XForm.Translation, StackTop(&DBot->TrackStack), &DBot->TrackInfo);
		DBot->PastFirstTrackPoint = GE_FALSE;
		DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
		Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);

		return GE_TRUE;
		}

	return GE_FALSE;
}

//=====================================================================================
//	Bot_InitFindMultiTrackAway
//=====================================================================================
geBoolean Bot_InitFindMultiTrackAway(GenVSI *VSI, void *PlayerData, geVec3d *DestPos, float Time)
{
	Bot_Var			*DBot;
	GPlayer			*Player;
	TrackCB			CB;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	if (!StackIsEmpty(&DBot->TrackStack))
		return GE_FALSE;

	CB.Data = PlayerData;
	CB.CB = Bot_ValidateMultiTrackPoints;

	if (Track_FindMultiTrack(VSI, 
						&Player->XForm.Translation,
						DestPos,
                        BOT_DIR_AWAY,
						&CB,
						&DBot->TrackStack))
		{
		DBot->Dir = BOT_DIR_AWAY;

		Track_NextMultiTrack(VSI, &Player->XForm.Translation, StackTop(&DBot->TrackStack), &DBot->TrackInfo);
		DBot->PastFirstTrackPoint = GE_FALSE;
		DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
		Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
		return GE_TRUE;
		}

	return GE_FALSE;
}


//=====================================================================================
//	Bot_InitMoveToPoint
//=====================================================================================
geBoolean Bot_InitMoveToPoint(GenVSI *VSI, void *PlayerData, geVec3d *Pos)
{
	GPlayer	*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->TgtPos = *Pos;

	geVec3d_Subtract(&DBot->TgtPos, &Player->XForm.Translation, &DBot->MoveVec);
    DBot->MoveVec.Y = 0.0f;
	geVec3d_Normalize(&DBot->MoveVec);

	DBot->Action = Bot_MoveToPoint;

	return GE_TRUE;
}

//=====================================================================================
//	Bot_InitMoveFree
//=====================================================================================
geBoolean Bot_InitMoveFree(GenVSI *VSI, void *PlayerData, geVec3d *Pos)
{
	GPlayer	*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->TgtPos = *Pos;

	geVec3d_Subtract(&DBot->TgtPos, &Player->XForm.Translation, &DBot->MoveVec);
    DBot->MoveVec.Y = 0.0f;
	geVec3d_Normalize(&DBot->MoveVec);

	DBot->Action = Bot_MoveFree;

	return GE_TRUE;
}


//=====================================================================================
//	Bot_InitGenericMove
//=====================================================================================
geBoolean Bot_InitGenericMove(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    // if still on a track
	if (Track_OnTrack(&DBot->TrackInfo))
        {
        // continue on the track
		DBot->Action = Bot_MoveToPoint;
        return GE_TRUE;
        }
	
    Bot_Reposition(VSI, Player, &DBot->GoalPos, DBot->Dir);
    DBot->Action = Bot_MoveToPoint;

    return GE_TRUE;
}

//=====================================================================================
//	Bot_InitMoveCloser
//=====================================================================================
geBoolean Bot_InitMoveCloser(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_TOWARD;

	Bot_Reposition(VSI, Player, &DBot->GoalPos, DBot->Dir);
    DBot->Action = Bot_MoveToPoint;

    return GE_TRUE;
    }

//=====================================================================================
//	Bot_InitRunCloser
//=====================================================================================
geBoolean Bot_InitRunCloser(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_TOWARD;

    if (Bot_InitTrackToGoal(VSI, PlayerData, Time))
        return GE_TRUE;
        
	Bot_Reposition(VSI, Player, &DBot->GoalPos, DBot->Dir);
    DBot->Action = Bot_MoveToPoint;

    return GE_TRUE;
    }

//=====================================================================================
//	Bot_InitTrackToGoal
//=====================================================================================
geBoolean Bot_InitTrackToGoal(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_TOWARD;

    if (Bot_FindTrackToGoal(VSI, PlayerData, Time))
        {
        DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
        Bot_InitMoveToPoint(VSI, PlayerData, &DBot->GoalPos);
        return GE_TRUE;
        }
        
    return GE_FALSE;
    }

//=====================================================================================
//	Bot_InitTrackAwayGoal
//=====================================================================================
geBoolean Bot_InitTrackAwayGoal(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_AWAY;

    if (Bot_FindTrackToGoal(VSI, PlayerData, Time))
        {
        DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
        Bot_InitMoveToPoint(VSI, PlayerData, &DBot->GoalPos);
        return GE_TRUE;
        }
        
    return GE_FALSE;
    }


//=====================================================================================
//	Bot_InitHide
//=====================================================================================
geBoolean Bot_InitHide(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer *Player;
	Bot_Var *DBot;

	Player = static_cast<GPlayer*>(PlayerData);
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->Dir = BOT_DIR_NONE;

    if (Bot_FindTrack(VSI, Player, HideTrackList))
        {
        DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
        Bot_InitMoveToPoint(VSI, PlayerData, &DBot->GoalPos);
        Bot_InitMoveToPoint(VSI, Player, Track_GetPoint(&DBot->TrackInfo)->Pos);
        return GE_TRUE;
        }

    return GE_FALSE;
    }


//=====================================================================================
//	Bot_InitMoveAway
//=====================================================================================
geBoolean Bot_InitMoveAway(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_AWAY;

    Bot_Reposition(VSI, PlayerData, &DBot->GoalPos, DBot->Dir);
    DBot->Action = Bot_MoveToPoint;

    return GE_TRUE;
    }


//=====================================================================================
//	Bot_InitRunAway
//=====================================================================================
geBoolean Bot_InitRunAway(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_AWAY;

    if (Bot_InitTrackAwayGoal(VSI, PlayerData, Time))
        return GE_TRUE;
        
    Bot_Reposition(VSI, PlayerData, &DBot->GoalPos, DBot->Dir);
    DBot->Action = Bot_MoveToPoint;

    return GE_TRUE;
    }

//=====================================================================================
//	Bot_InitMaintain
//=====================================================================================
geBoolean Bot_InitMaintain(GenVSI *VSI, void *PlayerData, float Time)
    {
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

    DBot->Dir = BOT_DIR_NONE;

    Bot_Reposition(VSI, PlayerData, &DBot->GoalPos, DBot->Dir);
    DBot->Action = Bot_MoveToPoint;

    return GE_TRUE;
    }


//=====================================================================================
//	Bot_InitWaitForPlayer
//=====================================================================================
geBoolean Bot_InitWaitForPlayer(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Bot_SetupXForm(VSI, Player, &DBot->MoveVec);

	// Stop here
	geVec3d_Clear(&Player->Velocity);

	DBot->Action = Bot_WaitForPlayer;
	
	return GE_TRUE;
}

//=====================================================================================
//	Bot_WaitForPlayer
//=====================================================================================
geBoolean Bot_WaitForPlayer(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer	*Player;
	Bot_Var			*DBot;
	geWorld			*World;

	World = GenVSI_GetWorld(VSI);

	assert(World);
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Bot_GetContents(VSI, Player, Time);
	Bot_Animate(VSI, Player, Time);

	if (GenVSI_GetTime(VSI) >= DBot->TimeOut)
	{
        Bot_ClearTrack(VSI, Player);
		Bot_InitGenericMove(VSI, Player, Time);
		return GE_TRUE;
	}

	if (Bot_CanSeePlayerToPlayer(World, &Player->XForm.Translation, &DBot->TgtPlayer->XForm.Translation))
	{
		Bot_InitGenericMove(VSI, Player, Time);
		return GE_TRUE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	Bot_InitWaitForEntityVisible
//=====================================================================================
geBoolean Bot_InitWaitForEntityVisible(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Bot_SetupXForm(VSI, Player, &DBot->MoveVec);

	geVec3d_Clear(&Player->Velocity);	// Stop moving

	DBot->HealthCheck = Player->Health; // save health
	DBot->Action = Bot_WaitForEntityVisible;
	
	return GE_TRUE;
}

//=====================================================================================
//	Bot_WaitForEntityVisible
//=====================================================================================
geBoolean Bot_WaitForEntityVisible(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geWorld			*World;
    TrackPt         *Tp;
	geBoolean		CanSee;
	geVec3d			Vec2Player;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	// vec2player
	//geVec3d_Subtract(&Player->XForm.Translation, &DBot->TgtPlayer->XForm.Translation, &Vec2Player);
	geVec3d_Subtract(&DBot->TgtPlayer->XForm.Translation, &Player->XForm.Translation, &Vec2Player);
	Bot_SetupXForm(VSI, PlayerData, &Vec2Player);       //Face Player

	// shooting
	if (!PlayerDead(DBot->TgtPlayer) && DBot->TimeSeen > 0.0f)
	{
        Bot_Shoot(VSI, Player, &DBot->TgtPlayer->XForm.Translation, Time);
	}

	Bot_GetContents(VSI, Player, Time);
	Bot_Animate(VSI, Player, Time);

    Tp = Track_GetPoint(&DBot->TrackInfoPrev);
    assert(Tp->WatchPos);

	if (Track_GetTrack(&DBot->TrackInfoPrev)->Type == TRACK_TYPE_TRAVERSE_DOOR)
		CanSee = Bot_CanSeePointToPoint(World, &Player->XForm.Translation, Tp->WatchPos);
	else
		CanSee = Bot_CanSeePlayerToPoint(World, &Player->XForm.Translation, Tp->WatchPos);

	if (CanSee)
	{
		if (Track_OnTrack(&DBot->TrackInfo))
			DBot->Action = Bot_MoveToPoint;
		else
			Bot_InitMoveToPoint(VSI, Player, &DBot->TgtPos);
		return GE_TRUE;
	}

	// could not see point
	if (GenVSI_GetTime(VSI) >= DBot->TimeOut || DBot->HealthCheck < Player->Health)
		{
        Bot_ClearTrack(VSI, Player);//&DBot->TrackInfo);
		Bot_InitGenericMove(VSI, Player, Time);
		return GE_TRUE;
		}

	return GE_TRUE;
}

//=====================================================================================
//	Bot_InitWaitForEntityDist
//=====================================================================================
geBoolean Bot_InitWaitForEntityDist(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Bot_SetupXForm(VSI, Player, &DBot->MoveVec);

	// Stop
	geVec3d_Clear(&Player->Velocity);

	DBot->HealthCheck = Player->Health; // save health
	DBot->Action = Bot_WaitForEntityDist;
	
	return GE_TRUE;
}

//=====================================================================================
//	Bot_WaitForEntityDist
//=====================================================================================
geBoolean Bot_WaitForEntityDist(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geWorld			*World;
    TrackPt         *Tp;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);
	Bot_GetContents(VSI, Player, Time);
	Bot_Animate(VSI, Player, Time);

    Tp = Track_GetPoint(&DBot->TrackInfoPrev);
    assert(Tp->WatchPos);

	if (geVec3d_DistanceBetween(&Player->XForm.Translation, Tp->WatchPos))
		{
		if (Track_OnTrack(&DBot->TrackInfo))
			DBot->Action = Bot_MoveToPoint;
		else
			Bot_InitMoveToPoint(VSI, Player, &DBot->TgtPos);
		return GE_TRUE;
		}

	// timed out
	if (GenVSI_GetTime(VSI) >= DBot->TimeOut || DBot->HealthCheck < Player->Health)
	{
        Bot_ClearTrack(VSI, Player);
		Bot_InitGenericMove(VSI, Player, Time);
		return GE_TRUE;
	}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_WeaponJump
//=====================================================================================
geBoolean Bot_WeaponJump(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			Pos;
	float			MoveSpeed;
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	// Face Player
	//Bot_SetupXForm(VSI, PlayerData, &Vec2Player);
	// Face Running direction
	Bot_SetupXForm(VSI, PlayerData, &DBot->MoveVec);

	MoveSpeed = Time;

	if (Player->State == PSTATE_InAir)
		MoveSpeed *= 0.15f;
	
	geVec3d_AddScaled(&Player->Velocity, &DBot->MoveVec, DBot->RunSpeed*MoveSpeed, &Player->Velocity);

	// save off the position
    Pos = Player->XForm.Translation;

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	Bot_GetContents(VSI, Player, Time);

	Bot_Animate(VSI, Player, Time);

	//??bot
	Player->VPos = Player->XForm.Translation;

    if (Player->State == PSTATE_Normal)
        {
		DBot->RunSpeed = BOT_RUN_SPEED;
        Bot_InitGenericMove(VSI, Player, Time);
        return GE_TRUE;
        }

	return GE_TRUE;
}

//=====================================================================================
//	Bot_Jump
//=====================================================================================
geBoolean Bot_Jump(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			Pos;
	float			MoveSpeed;
	GPlayer			*Player;
	Bot_Var			*DBot;
	
	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Bot_SetupXForm(VSI, PlayerData, &DBot->MoveVec);

	MoveSpeed = Time;

	if (Player->State == PSTATE_InAir)
		MoveSpeed *= 0.15f;
	
	geVec3d_AddScaled(&Player->Velocity, &DBot->MoveVec, DBot->RunSpeed*MoveSpeed, &Player->Velocity);

	// save off the position
    Pos = Player->XForm.Translation;

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	Bot_GetContents(VSI, Player, Time);

	Bot_Animate(VSI, Player, Time);

	Player->VPos = Player->XForm.Translation;

    if (Player->State == PSTATE_Normal)
        {
		DBot->RunSpeed = BOT_RUN_SPEED;
		Bot_InitGenericMove(VSI, Player, Time);
        return GE_TRUE;
        }

	return GE_TRUE;
}


//=====================================================================================
//	Bot_InitShootPoint
//=====================================================================================
geBoolean Bot_InitShootPoint(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	//Bot_SetupXForm(VSI, Player, &DBot->MoveVec);

	// Stop
	geVec3d_Clear(&Player->Velocity);

	DBot->Action = Bot_ShootPoint;
	
	return GE_TRUE;
}


//=====================================================================================
//	Bot_ShootPoint
//=====================================================================================
geBoolean Bot_ShootPoint(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geWorld			*World;
    TrackPt         *Tp;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);
	Bot_GetContents(VSI, Player, Time);
	Bot_Animate(VSI, Player, Time);

	if (GenVSI_GetTime(VSI) >= Player->NextWeaponTime)
	{
		geVec3d ShootPos, Vec2Point;
		geXForm3d XFormSave;

		Tp = Track_GetPoint(&DBot->TrackInfoPrev);
		assert(Tp->WatchPos);

		ShootPos = *Tp->WatchPos;

		// adjust Y for Gun offset - shoot lower so it will be correct when 
		// the vector is moved up
		ShootPos.Y -= Player->GunOffset.Y;
		geVec3d_Subtract(&ShootPos, &Player->XForm.Translation, &Vec2Point);

		XFormSave = Player->XForm;
		Bot_SetupShootXForm(VSI, PlayerData, &Vec2Point);
		Bot_FireWeapon(VSI, PlayerData, Time);
		Player->XForm = XFormSave;

		DBot->ShootCount++;
		if (DBot->ShootCount >= Tp->ShootTimes)
			{
			DBot->ShootCount = 0;
			if (Track_OnTrack(&DBot->TrackInfo))
				DBot->Action = Bot_MoveToPoint;
			else
				Bot_InitMoveToPoint(VSI, Player, &DBot->TgtPos);
			return GE_TRUE;
			}

	}

	// time out
	if (GenVSI_GetTime(VSI) >= DBot->TimeOut)
	{
        Bot_ClearTrack(VSI, Player);
		Bot_InitGenericMove(VSI, Player, Time);
		return GE_TRUE;
	}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_SetupXForm
//=====================================================================================
geBoolean Bot_SetupXForm(GenVSI *VSI, void *PlayerData, geVec3d *OrientVec)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geVec3d			InVect;
	geVec3d			LVect, UpVect = {0.0f,1.0f,0.0f}, Pos;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Pos = Player->XForm.Translation;

	InVect = *OrientVec;

	InVect.Y = 0.0f; // Make it a 2D vector 

    if (geVec3d_Length(&InVect) == 0.0f)
        {
		// This is hardly ever going to happen - but need to test for it
		geXForm3d_SetIdentity(&Player->XForm);
        }
    else
        {
		geVec3d_Normalize(&InVect);
		geVec3d_CrossProduct(&UpVect, &InVect, &LVect);
		geVec3d_Normalize(&LVect);
		geVec3d_CrossProduct(&InVect, &LVect, &UpVect);
		geXForm3d_SetFromLeftUpIn(&Player->XForm, &LVect, &UpVect, &InVect);
		}

	// set the position back before moving
    Player->XForm.Translation = Pos;

	return GE_TRUE;
}


//=====================================================================================
//	Bot_SetupShootXForm
//=====================================================================================
geBoolean Bot_SetupShootXForm(GenVSI *VSI, void *PlayerData, geVec3d *TargetVec)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	geVec3d			InVect;
	geVec3d			LVect = {1.0f,0.0f,0.0f}, UpVect = {0.0f,1.0f,0.0f}, Pos;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Pos = Player->XForm.Translation;

	InVect = *TargetVec;

    if (geVec3d_Length(&InVect) == 0.0f)
        {
		// This is hardly ever going to happen - but need to test for it
		geXForm3d_SetIdentity(&Player->XForm);
        }
    else
		{
		geVec3d_Normalize(&InVect);
		if ((1.0f - fabs(geVec3d_DotProduct(&InVect, &UpVect))) < 0.01f)
			{
			// if co-linear
			geVec3d_CrossProduct(&LVect, &InVect, &UpVect);
			geVec3d_Normalize(&UpVect);
			geVec3d_CrossProduct(&UpVect, &InVect, &LVect);
			geXForm3d_SetFromLeftUpIn(&Player->XForm, &LVect, &UpVect, &InVect);
			}
		else
			{
			geVec3d_CrossProduct(&UpVect, &InVect, &LVect);
			geVec3d_Normalize(&LVect);
			geVec3d_CrossProduct(&InVect, &LVect, &UpVect);
			geXForm3d_SetFromLeftUpIn(&Player->XForm, &LVect, &UpVect, &InVect);
			}
		}

	// set the position back before moving
    Player->XForm.Translation = Pos;

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ComparePlayers
//=====================================================================================
int16 Bot_ComparePlayers(GenVSI *VSI, void *PlayerData1, void *PlayerData2)
{
	GPlayer			*Player1;
	GPlayer			*Player2;
	int32			diff;

	// this func should be extended and to make better decision

	Player1 = (GPlayer*)PlayerData1;
	Player2 = (GPlayer*)PlayerData2;

	diff = Player1->Health - Player2->Health;

	if (diff > 30)
	{
		return(1);
	}
	else
	if (diff < -30)
	{
		return(-1);
	}
	else
		return(1);
}


//=====================================================================================
//	Bot_FireWeapon
//=====================================================================================
geBoolean Bot_FireWeapon(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;

	assert(PlayerData);

	Player = (GPlayer*)PlayerData;

	if (GenVSI_GetTime(VSI) < Player->NextWeaponTime)
		return GE_FALSE;
	
	ValidateWeapon(VSI, PlayerData);

	switch (Player->CurrentWeapon)
	{
		case 0:
			FireBlaster(VSI, Player, Time);
			Player->NextWeaponTime += 0.4f;
			break;
		case 1:
			FireGrenade(VSI, Player, Time);
			Player->NextWeaponTime += 0.4f;
			break;
		case 2:
			FireRocket(VSI, Player, Time);
			Player->NextWeaponTime += 0.4f;
			break;
		case 3:
			FireShredder(VSI, Player, Time);
			Player->NextWeaponTime += 0.1f;
			break;
	}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_Shoot
//=====================================================================================
geBoolean Bot_Shoot(GenVSI *VSI, void *PlayerData, geVec3d *ShootPosition, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	static uint32	Delay=0;
	geVec3d Vec2Player;
	geXForm3d XFormSave;
	#define	TARGET_FEET_ADJUST_Y 130.0f
	#define	TARGET_FEET_ADJUST_Y2 100.0f

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	if (ShootPosition)
		{
		geVec3d ShootPos = *ShootPosition;

		switch (Player->CurrentWeapon)
			{
			case ITEM_BLASTER:
				if (ShootPos.Y <= Player->XForm.Translation.Y || 
				 fabs(ShootPos.Y - Player->XForm.Translation.Y) < TARGET_FEET_ADJUST_Y/2)
					{
					// aim dead on and at feet
					ShootPos.Y -= TARGET_FEET_ADJUST_Y;
					}
				break;

			case ITEM_ROCKETS:
				if (ShootPos.Y <= Player->XForm.Translation.Y || 
					fabs(ShootPos.Y - Player->XForm.Translation.Y) < TARGET_FEET_ADJUST_Y/2
					)
					{
					// aim dead on and at feet
					ShootPos.X += (float)(RandomRange(100) - 50);
					ShootPos.Y -= TARGET_FEET_ADJUST_Y2;
					ShootPos.Y += (float)(RandomRange(100) - 50);
					ShootPos.Z += (float)(RandomRange(100) - 50);
					}
				break;

			default:
				ShootPos.X += (float)(RandomRange(100) - 50);
				ShootPos.Y += (float)(RandomRange(100) - 50);
				ShootPos.Z += (float)(RandomRange(100) - 50);
				break;
			}

		geVec3d_Subtract(&ShootPos, &Player->XForm.Translation, &Vec2Player);

		// save off the current XForm
		XFormSave = Player->XForm;
		Bot_SetupShootXForm(VSI, PlayerData, &Vec2Player);
		Bot_FireWeapon(VSI, PlayerData, Time);
		// restore the current XForm
		Player->XForm = XFormSave;
		}
	else
		{
		Bot_FireWeapon(VSI, PlayerData, Time);
		}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_Animate
//=====================================================================================
geBoolean Bot_Animate(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	float			Speed;
	geBoolean		DoWalk = GE_FALSE;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Speed = geVec3d_Length(&Player->Velocity);

	if (Speed > 0.1f)
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

			//if (Move->ForwardSpeed<0)
			//	Speed *= -1.0f;
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

	return GE_TRUE;
}

//=====================================================================================
//	Bot_GetContents
//=====================================================================================
geBoolean Bot_GetContents(GenVSI *VSI, void *PlayerData, float Time)
{
	GE_Contents		Contents;
	geVec3d			CMins, CMaxs;
	GPlayer			*Player;
	Bot_Var			*DBot;
	uint32			ColFlags;
	geWorld			*World;
	CData			Data;
	geBoolean SelfCollisionCB(geWorld_Model *Model, geActor *Actor, void *Context);

	World = GenVSI_GetWorld(VSI);

	#define ALL_BUT_SELF	(0xffffffff & ~(1<<0))

	assert(World);

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	ColFlags = GE_COLLIDE_MODELS;

	if (VSI->Mode == MODE_Server)		// Only do this stuff when we are in server mode...
	{
		if (!PlayerDead(Player) && Player->Roll > 0.0f)
			Player->Roll -= Time;

		if (geWorld_GetContents(World, &Player->XForm.Translation, &Player->Mins, &Player->Maxs, GE_COLLIDE_MODELS, 0, NULL, NULL, &Contents))
		{
			// Check to see if player is in lava...
			if (Contents.Contents & CONTENTS_WATER)
			{
				Player->State = PSTATE_InWater;
			}

			if (Contents.Contents & CONTENTS_LAVA)
			{
				if (Player->Roll <= 0.0f)
				{
					DammagePlayer(VSI, NULL, Player, 20, 0.0f, Time);
					Player->Roll = 0.5f;
				}

				Player->State = PSTATE_InLava;
			}
		}
		else if (PlayerLiquid(Player))
				Player->State = PSTATE_Normal;

		// Get a box a little bigger than the player for doors, etc...
		CMins = Player->Mins;
		CMaxs = Player->Maxs;
	
		CMins.X -= 100;
		CMins.Y -= 10;
		CMins.Z -= 100;
		CMaxs.X += 100;
		CMaxs.Y += 10;
		CMaxs.Z += 100;

		if (geWorld_GetContents(World, &Player->XForm.Translation, &CMins, &CMaxs, GE_COLLIDE_MODELS, 0,  NULL, NULL, &Contents))
		{
			if (Contents.Model)
			{
				GPlayer	*TPlayer;

				TPlayer = (GPlayer*)geWorld_ModelGetUserData(Contents.Model);

				if (TPlayer && TPlayer->Trigger && TPlayer->ViewFlags & VIEW_TYPE_TOUCH)
				{
					TPlayer->Trigger(VSI, TPlayer, Player, NULL);
				}
			}
		}


		Data.VSI = VSI;
		Data.Player = Player;

		if (geWorld_GetContents(World, &Player->XForm.Translation, &CMins, &CMaxs, GE_COLLIDE_ACTORS, 0xffffffff, SelfCollisionCB, &Data, &Contents))
		{
			
			if (Contents.Actor)
			{
				GPlayer	*TPlayer;
				static	int32	HackV;

				TPlayer = (GPlayer*)GenVSI_ActorToPlayer(VSI, Contents.Actor);

				if (TPlayer && TPlayer->Trigger)
				{
					TPlayer->Trigger(VSI, TPlayer, Player, NULL);
				}
			}
		}

	}



	return GE_TRUE;
}

//=====================================================================================
//	Bot_CanSeePointToPoint
//=====================================================================================
geBoolean Bot_CanSeePointToPoint(geWorld *World, geVec3d *Pos1, geVec3d *Pos2)
{
	GE_Collision Collision;
    int32 Leaf1, Leaf2;

   	assert (World);

	geWorld_GetLeaf(World, Pos1, &Leaf1);
	geWorld_GetLeaf(World, Pos2, &Leaf2);
			
	if (geWorld_LeafMightSeeLeaf(World, Leaf1, Leaf2, 0))
		return !geWorld_Collision(World, &RayMins, &RayMaxs, Pos1, Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision);

	return(GE_FALSE);
}

//=====================================================================================
//	Bot_CanSeePlayerToPoint
//	Adjust FIRST point for player height
//=====================================================================================
geBoolean Bot_CanSeePlayerToPoint(geWorld *World, geVec3d *Pos1, geVec3d *Pos2)
{
	GE_Collision Collision;
	geVec3d PosAdj1,PosAdj2;
    int32 Leaf1, Leaf2;

   	assert (World);

	PosAdj1 = *Pos1;
	PosAdj2 = *Pos2;

	PosAdj1.Y += 30.0f;

	geWorld_GetLeaf(World, &PosAdj1, &Leaf1);
	geWorld_GetLeaf(World, &PosAdj2, &Leaf2);
			
	if (geWorld_LeafMightSeeLeaf(World, Leaf1, Leaf2, 0))
		return !geWorld_Collision(World, &RayMins, &RayMaxs, &PosAdj1, &PosAdj2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision);

	return(GE_FALSE);
}

//=====================================================================================
//	Bot_CanSeePlayerToPlayer
//	Adjust BOTH points for player height
//=====================================================================================
geBoolean Bot_CanSeePlayerToPlayer(geWorld *World, geVec3d *Pos1, geVec3d *Pos2)
{
	GE_Collision Collision;
	geVec3d PosAdj1,PosAdj2;
    int32 Leaf1, Leaf2;

   	assert (World);

	PosAdj1 = *Pos1;
	PosAdj2 = *Pos2;

	PosAdj1.Y += 140.0f;
	PosAdj2.Y += 140.0f;

	geWorld_GetLeaf(World, &PosAdj1, &Leaf1);
	geWorld_GetLeaf(World, &PosAdj2, &Leaf2);
			
	if (geWorld_LeafMightSeeLeaf(World, Leaf1, Leaf2, 0))
		return !geWorld_Collision(World, &RayMins, &RayMaxs, &PosAdj1, &PosAdj2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0, NULL, NULL, &Collision);

	return(GE_FALSE);
}


//=====================================================================================
//	Bot_PickTgtPlayer
//=====================================================================================
GPlayer *Bot_PickTgtPlayer(GenVSI *VSI, void *PlayerData, geBoolean ForcePick)
{
	GPlayer			*Hit;
	geVec3d			Pos, Pos2, Delta;
	float			Dist, BotNearDist = 999999.0f;
	float			PlayerNearDist = 999999.0f;
	GPlayer			*Player;
	Bot_Var			*DBot;
	geWorld			*World;
	GPlayer         *FoundPlayerNearDist, *FoundPlayerCanSee, *FoundPlayerCanSeeNearDist;
	GPlayer			*FoundBotNearDist, *FoundBotCanSee, *FoundBotCanSeeNearDist;
	GPlayer			*LastChance = NULL;
	GPlayer			*BestBot, *BestPlayer;
	GPlayer			*BestBotWeapons, *BestPlayerWeapons;
	int32			BestBotScore, BestPlayerScore;
	int32			WRank, BestBotWRank=0, BestPlayerWRank=0;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	if (VSI->Mode != MODE_Server)
		return NULL;
	
	Pos = Player->XForm.Translation;

	Hit = NULL;
    FoundPlayerNearDist = FoundPlayerCanSee = FoundPlayerCanSeeNearDist = NULL;
    FoundBotNearDist = FoundBotCanSee = FoundBotCanSeeNearDist = NULL;
	BestBot = BestPlayer = NULL;
	BestBotScore = BestPlayerScore = -1;

	World = GenVSI_GetWorld(VSI);

	while (1)
	{
		Hit = static_cast<GPlayer*>(GenVSI_GetNextPlayer(VSI, Hit, NULL));

		if (!Hit)
			break;

		if (Hit->ClientHandle == CLIENT_NULL_HANDLE) // Will not find ACTORS!
			continue;

		if (GenVSI_IsClientBot(VSI, Hit->ClientHandle))
			{

			WRank = Bot_RankWeapons(VSI, Hit);

			if (WRank > BestBotWRank)
				{
				BestBotWRank = WRank;
				BestBotWeapons = Hit;
				}

			if (Hit->Score > BestBotScore)
				{
				BestBotScore = Hit->Score;
				BestBot = Hit;
				}
			}
		else
			{
			DBot->ClientPlayer = Hit;
			WRank = Bot_RankWeapons(VSI, Hit);

			if (WRank > BestPlayerWRank)
				{
				BestPlayerWRank = WRank;
				BestPlayerWeapons = Hit;
				}

			if (Hit->Score > BestPlayerScore)
				{
				BestPlayerScore = Hit->Score;
				BestPlayer = Hit;
				}
			}


		// don't look at yourself
		if (Hit == Player)
			continue;

		// SOMEbody other than yourself - even if dead
		LastChance = Hit; // just in case everyone but you is dead

		// skip dead players/bots
		if (PlayerDead(Hit))
			continue;

		Pos2 = Hit->XForm.Translation;

		geVec3d_Subtract(&Pos, &Pos2, &Delta);
		Dist = geVec3d_Length(&Delta);

		if (GenVSI_IsClientBot(VSI, Hit->ClientHandle))
			{
			// force human player to be picked
			if (ForcePick)
				continue;

			if (Dist < BotNearDist)
				{
				BotNearDist = Dist;
				FoundBotNearDist = Hit;

				if (Bot_CanSeePlayerToPlayer(World, &Pos, &Pos2))
					FoundBotCanSeeNearDist = Hit;
				}
			else
				{
				if (Bot_CanSeePlayerToPlayer(World, &Pos, &Pos2))
					FoundBotCanSee = Hit;
				}
			}
		else
			{
			if (Dist < PlayerNearDist)
				{
				PlayerNearDist = Dist;
				FoundPlayerNearDist = Hit;

				if (Bot_CanSeePlayerToPlayer(World, &Pos, &Pos2))
					FoundPlayerCanSeeNearDist = Hit;
				}
			else
				{
				if (Bot_CanSeePlayerToPlayer(World, &Pos, &Pos2))
					FoundPlayerCanSee = Hit;
				}
			}

	}

	// if player gets ahead - let ALL bots go for player as first choice
	if (BestBot && BestPlayer) 
		if (BestPlayerScore > BestBotScore && (BestPlayerScore - BestBotScore) > 10)
			{
			return BestPlayer;
			}

	/*
	// if YOU aren't the bot best armed &&
		// if player and bot are visible AND best bot is heavily armed - then go for him
	if (BestBotWeapons != NULL && BestBotWeapons != Player)
		if (FoundPlayerCanSee && FoundBotCanSee && (FoundBotCanSee == BestBotWeapons))
			if (BestBotWRank > BestPlayerWRank && (BestBotWRank - BestPlayerWRank) > 30)
				{
				return BestBotWeapons;
				}
	*/

	if (FoundPlayerCanSeeNearDist)	return  FoundPlayerCanSeeNearDist;
	if (FoundBotCanSeeNearDist)		return  FoundBotCanSeeNearDist;
	if (FoundPlayerCanSee)			return  FoundPlayerCanSee;
	if (FoundBotCanSee)				return  FoundBotCanSee;
	if (FoundPlayerNearDist)		return  FoundPlayerNearDist;
	if (FoundBotNearDist)			return  FoundBotNearDist;

	return (LastChance);
}


//=====================================================================================
//	Bot_FindItem
//=====================================================================================
GPlayer *Bot_FindItem(GenVSI *VSI, geVec3d *Pos, char *ClassList[], float YDiff, float Range)
{
	GPlayer			*Hit;
	geVec3d			Pos2;
	float			Dist, NearDist = 999999.0f;
	geWorld			*World;
    int             i;
	GPlayer         *FoundPlayerCanSeeNearDist;

	if (VSI->Mode != MODE_Server)
		return NULL;
	
    FoundPlayerCanSeeNearDist = NULL;

	World = GenVSI_GetWorld(VSI);

	Hit = NULL;
	while (1)
	{
		Hit = static_cast<GPlayer*>(GenVSI_GetNextPlayer(VSI, Hit, NULL));

		if (!Hit)
			break;

        if (Hit->ViewIndex == 0xffff)
            continue;

        if (fabs(Pos->Y - Hit->Pos.Y) > YDiff)
            continue;

        for (i=0; ClassList[i]; i++)
        {
            if (strcmp(ClassList[i], Hit->ClassName) == 0)   
                break;
        }

        if (!ClassList[i])
            continue;

		Pos2 = Hit->XForm.Translation;

		Dist = geVec3d_DistanceBetween(Pos, &Pos2);

		if (Dist > Range)
			continue;

		if (Dist < NearDist)
		{
			if (Bot_CanSeePointToPoint(World, Pos, &Pos2))
			{
				FoundPlayerCanSeeNearDist = Hit;
			}	
		}
	}

	if (FoundPlayerCanSeeNearDist)
		return  FoundPlayerCanSeeNearDist;

	return NULL;
}


//=====================================================================================
//	Bot_FindRandomItem
//=====================================================================================
GPlayer *Bot_FindRandomItem(GenVSI *VSI, geVec3d *Pos, char *ClassList[])
{
	GPlayer			*Hit;
	geVec3d			Pos2;
	float			Dist, NearDist = 999999.0f;
	geWorld			*World;
    int             i,GoalCount;
	GPlayer         *FoundPlayerCanSeeNearDist;
	static GPlayer		*GoalList[300];

	if (VSI->Mode != MODE_Server)
		return NULL;
	
    FoundPlayerCanSeeNearDist = NULL;

	World = GenVSI_GetWorld(VSI);

	GoalCount = 0;
	Hit = NULL;
	while (1)
	{
		Hit = static_cast<GPlayer*>(GenVSI_GetNextPlayer(VSI, Hit, NULL));

		if (!Hit)
			break;

        if (Hit->ViewIndex == 0xffff)
            continue;

        for (i=0; ClassList[i]; i++)
        {
            if (strcmp(ClassList[i], Hit->ClassName) == 0)   
                break;
        }

        if (!ClassList[i])
            continue;

		Pos2 = Hit->XForm.Translation;
		Dist = geVec3d_DistanceBetween(Pos, &Pos2);

		if (Dist < 2000)
			continue;

		GoalList[GoalCount++] = Hit;
		if ( GoalCount >= sizeof(GoalList)/sizeof(GoalList[0]) )
			break;

	}

	if (!GoalCount)
		return NULL;

	return(GoalList[RandomRange(GoalCount)]);
}


//=====================================================================================
//	Bot_FindRandomTrack
//=====================================================================================
Track *Bot_FindRandomTrack(GenVSI *VSI, geVec3d *Pos, int32 *TrackTypeList)
{
	int             i,GoalCount;
	static Track	*GoalList[300];
	Track *t;

	GoalCount = 0;
	t = NULL;
	while (1)
	{
		t = Track_GetNextTrack(t);

		if (!t)
			break;

        for (i=0; TrackTypeList[i] >= 0; i++)
        {
			if (t->Type == TrackTypeList[i])
                break;
        }

        if (TrackTypeList[i] <= -1)
            continue;

		GoalList[GoalCount++] = t;
		if ( GoalCount >= sizeof(GoalList)/sizeof(GoalList[0]) )
			break;

	}

	if (!GoalCount)
		return NULL;

	return(GoalList[RandomRange(GoalCount)]);
}


//=====================================================================================
//	Bot_Dying
//=====================================================================================
geBoolean Bot_Dying(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	static float WaitTime = 0.0f;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	Player->MotionIndex = ACTOR_MOTION_PLAYER_DIE;
	AnimatePlayer(VSI, Player, Player->MotionIndex, Time, GE_FALSE);

	Player->Roll -= 2.0f*Time;

	if (Player->Roll < -(3.14159f/2.0f))
	{
		Player->Roll = -(3.14159f/2.0f);

		WaitTime += Time;

		if (WaitTime > 3.0f)
		{
			GPlayer	*DMStart;
			
			DMStart = (GPlayer*)GetBotMatchSpawn(VSI);

			Player->Roll = 0.0f;		// Make him stand right side up again
			Player->Control = Bot_Control;
			Player->ViewIndex = 0;
			Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
			Player->FrameTime = 0.0f;
			geVec3d_Clear(&Player->Velocity);
            geVec3d_Clear(&DBot->TgtPos);
            Bot_ClearTrack(VSI, Player);
            StackReset(&DBot->TrackStack);
            DBot->Action = Bot_InitGenericMove;
			DBot->PickTimeout = 0.0f;
			DBot->TimeSinceTrack = 0.0f;
			DBot->BotTgtPicked = 0;
            Bot_ModeChange(VSI, Player, MODE_WANDER, GE_TRUE, Time);
			DBot->RunSpeed = BOT_RUN_SPEED;
			Player->State = PSTATE_Normal;		// Back to normal now...

			Player->ViewFlags &= ~VIEW_TYPE_FORCE_XFORM;

			Player->XForm = DMStart->XForm;
			GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_PLAYER_SPAWN);

			return GE_TRUE;
		}
	}
	else
	{
	WaitTime = 0.0f;	
	}

	SetupPlayerXForm(VSI, Player, Time);

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	return GE_TRUE;
}


//=====================================================================================
//	Bot_ActorDying
//=====================================================================================
geBoolean Bot_ActorDying(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer			*Player;
	Bot_Var			*DBot;
	static float WaitTime = 0.0f;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	Player->MotionIndex = ACTOR_MOTION_PLAYER_DIE;
	AnimatePlayer(VSI, Player, Player->MotionIndex, Time, GE_FALSE);

	Player->Roll -= 2.0f*Time;

	if (Player->Roll < -(3.14159f/2.0f))
	{
		Player->Roll = -(3.14159f/2.0f);

		WaitTime += Time;

		//if (Move->ButtonBits & HOST_BUTTON_FIRE)
		if (WaitTime > 3.0f)
		{
			BotActorStart *Bs;
			
			Bs = Bot_GetActorStart(VSI, Player);

			assert(Bs);

			if (!Bs->Respawn)
				{
				GenVSI_DestroyPlayer(VSI, Player);
				return GE_TRUE;;
				}

			Player->Roll = 0.0f;		// Make him stand right side up again
			Player->Control = Bot_Control;
			Player->ViewIndex = 0;
			Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
			Player->FrameTime = 0.0f;
			geVec3d_Clear(&Player->Velocity);
            geVec3d_Clear(&DBot->TgtPos);
            Bot_ClearTrack(VSI, Player);
			DBot->PickTimeout = 0.0f;
			DBot->BotTgtPicked = 0;
			DBot->TimeSinceTrack = 0.0f;
            StackReset(&DBot->TrackStack);
            Bot_ModeChange(VSI, Player, MODE_WANDER, GE_TRUE, Time);
            DBot->Action = Bot_InitGenericMove;
			DBot->RunSpeed = BOT_RUN_SPEED;
			Player->State = PSTATE_Normal;		// Back to normal now...

			Player->ViewFlags &= ~VIEW_TYPE_FORCE_XFORM;

			geXForm3d_SetIdentity(&Player->XForm);
			Player->XForm.Translation = Bs->origin;
			GenVSI_SpawnFx(VSI, (uint8)FX_EXPLODE2, &Player->XForm.Translation, SOUND_INDEX_PLAYER_SPAWN);

			return GE_TRUE;
		}
	}
	else
	{
	WaitTime = 0.0f;	
	}

	Bot_SetupXForm(VSI, Player, &DBot->MoveVec);

	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

	return GE_TRUE;
}


geBoolean Bot_NudgePlayer(GenVSI *VSI, void *PlayerData, geVec3d *TgtPos, float Time)
	{
	GPlayer			*Player;
	Bot_Var			*DBot;
	GE_Collision	Collision;
	uint32			ColFlags;
	geVec3d			NewPos, Pos;
	geWorld			*World;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);
	World = GenVSI_GetWorld(VSI);
	assert(World);

	ColFlags = GE_COLLIDE_MODELS;

	NewPos = *TgtPos;
	Pos = Player->XForm.Translation;

	NewPos.Y += 40;
	// likely pushes you up in the air a bit
	if (!geWorld_Collision(World, &Player->Mins, &Player->Maxs, &Pos, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, ColFlags, 0xffffffff, NULL, NULL, &Collision))
		{
		Pos = NewPos;
		// back on the ground
		NewPos.Y -= 200.0f;
		assert (geWorld_Collision(World, &Player->Mins, &Player->Maxs, &Pos, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, ColFlags, 0xffffffff, NULL, NULL, &Collision));
		geVec3d_Copy(&Collision.Impact, &Player->XForm.Translation);
		return GE_TRUE;
		}
		
	return GE_FALSE;
	}

#define MAX_CLIP_PLANES		5
//=====================================================================================
//	Bot_CheckVelocity
//	Adjust players Velocity. based on what it's doing in the world
//=====================================================================================
geBoolean Bot_CheckVelocity(GenVSI *VSI, void *PlayerData, float Time)
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

	Player = (GPlayer*)PlayerData;

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	geVec3d_Copy(&Player->Velocity, &OriginalVelocity);
	geVec3d_Copy(&Player->Velocity, &PrimalVelocity);

	TimeLeft = Time;
	
	NumHits = 4;
	//NumHits = 1;
	NumPlanes = 0;

	Player->State = PSTATE_InAir;

	ColFlags = GE_COLLIDE_MODELS;

	for (HitCount=0 ; HitCount<NumHits ; HitCount++)
	{
		float	Fd, Bd;
		//Server_Player	*UPlayer;

		Pos = Player->XForm.Translation;

		geVec3d_AddScaled(&Pos, &Player->Velocity, TimeLeft, &NewPos);
		
		if (geWorld_GetContents(World, &Pos, &Mins, &Maxs, ColFlags, 1,  NULL, NULL, &Contents))
		{
			if (Contents.Contents & GE_CONTENTS_SOLID)
			{
				if (geWorld_GetContents(World, &NewPos, &Mins, &Maxs, ColFlags, 0xffffffff,  NULL, NULL, &Contents))
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
		
		if (!geWorld_Collision(World, &Mins, &Maxs, &Pos, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, ColFlags, 0xffffffff, NULL, NULL, &Collision))
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

        // This is the main change 
		if (Collision.Plane.Normal.Y < 0.2f)
		{
			//geVec3d_Copy(&Collision.Impact, &Player->XForm.Translation);
			geVec3d_Clear(&Player->Velocity);
			return GE_FALSE;
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
			ReflectVelocity(&OriginalVelocity, &Planes[i], &NewVelocity, 1.0f);

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
		{	// Go along the crease
			if (NumPlanes != 2)
			{
				geVec3d_Clear(&Player->Velocity);
				return GE_FALSE;
			}
			
			geVec3d_CrossProduct(&Planes[0], &Planes[1], &Dir);
			Dist = geVec3d_DotProduct(&Dir, &Player->Velocity);
			geVec3d_Scale(&Dir, Dist, &Player->Velocity);
		}

		//
		// If original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (geVec3d_DotProduct (&Player->Velocity, &PrimalVelocity) <= 0.0f)
		{
			geVec3d_Clear(&Player->Velocity);
			return GE_FALSE;
		}
		//

	}
	return GE_TRUE;									
}


//=====================================================================================
//	Bot_ScanCheckVelocity
//=====================================================================================
geBoolean Bot_ScanCheckVelocity(GenVSI *VSI, void *PlayerData, float Time)
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
	Bot_Var			*DBot;
	geWorld			*World;
	uint32			ColFlags;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	World = GenVSI_GetWorld(VSI);

	assert(World);

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	geVec3d_Copy(&Player->Velocity, &OriginalVelocity);
	geVec3d_Copy(&Player->Velocity, &PrimalVelocity);

	TimeLeft = Time;
	
	NumHits = 4;
	//NumHits = 1;
	NumPlanes = 0;

	Player->State = PSTATE_InAir;

	ColFlags = GE_COLLIDE_ALL;

	for (HitCount=0 ; HitCount<NumHits ; HitCount++)
	{
		float	Fd, Bd;

		Pos = Player->XForm.Translation;

		geVec3d_AddScaled(&Pos, &Player->Velocity, TimeLeft, &NewPos);
		
		if (geWorld_GetContents(World, &Pos, &Mins, &Maxs, ColFlags, 1,  NULL, NULL, &Contents))
		{
			if (Contents.Contents & GE_CONTENTS_SOLID)
			{
				if (geWorld_GetContents(World, &NewPos, &Mins, &Maxs, ColFlags, 0xffffffff,  NULL, NULL, &Contents))
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
		
		if (!geWorld_Collision(World, &Mins, &Maxs, &Pos, &NewPos, GE_CONTENTS_CANNOT_OCCUPY, ColFlags, 0xffffffff, NULL, NULL, &Collision))
			return GE_TRUE;		// Covered the entire distance...

		
		if (Collision.Actor)		// Just stop at actors for now...
		{
			//geVec3d_Copy(&Player->LastGoodPos, &Player->XForm.Translation);
			geVec3d_Copy(&Collision.Impact, &Player->XForm.Translation);
			geVec3d_Clear(&Player->Velocity);
			return GE_FALSE;
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

        // This is the main change - other than optimizations
		if (Collision.Plane.Normal.Y < 0.2f)
		{
			geVec3d_Copy(&Collision.Impact, &Player->XForm.Translation);
			geVec3d_Clear(&Player->Velocity);
			return GE_FALSE;
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
			ReflectVelocity(&OriginalVelocity, &Planes[i], &NewVelocity, 1.0f);

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
		{	// Go along the crease
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
		// If original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		//
		if (geVec3d_DotProduct (&Player->Velocity, &PrimalVelocity) <= 0.0f)
		{
			geVec3d_Clear(&Player->Velocity);
			return GE_TRUE;
		}
		//

	}
	return GE_TRUE;									
}


//=====================================================================================
//	Bot_Physics
//=====================================================================================
geBoolean Bot_Physics(GenVSI *VSI, void *PlayerData, float Time)
{
	float	Speed;
	GPlayer	*Player;
	Bot_Var			*DBot;
    geBoolean ClearMove = GE_TRUE;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	// Walk
	Player->LastGoodPos = Player->XForm.Translation;

	// Add gravity
	// Add gravity
	switch (Player->State)
	{
		case PSTATE_InLava:
		case PSTATE_InWater:
			Player->Velocity.Y -= BOT_GRAVITY*Time*0.05f;
			break;

		default:
			Player->Velocity.Y -= BOT_GRAVITY*Time;
			break;
	}

	ClearMove = Bot_CheckVelocity(VSI, Player, Time);

	SqueezeVector(&Player->Velocity, 0.2f);
	geVec3d_AddScaled(&Player->XForm.Translation, &Player->Velocity, Time, &Player->XForm.Translation);

	if (Bot_CheckPosition(VSI, Player))
	{
		// think
		geVec3d_Clear(&DBot->TgtPos);
	}
	
	Speed = geVec3d_Length(&Player->Velocity);

    // Note that this is changed from PlayerPhysics
    // Does the exact same thing - just simplified with algebra
	if (Speed > 0.001f)
	{
		float	Scale;

		if (Player->State == PSTATE_Normal || Player->State == PSTATE_DeadOnGround)
			Scale = 1.0f-(Time*BOT_GROUND_FRICTION);
		else if (Player->State == PSTATE_InAir || Player->State == PSTATE_Dead)
			Scale = 1.0f-(Time*BOT_AIR_FRICTION);
		else if (PlayerLiquid(Player))
			Scale = 1.0f-(Time*BOT_LIQUID_FRICTION);

		if (Scale < 0.0f)
			Scale = 0.0f;

		// Apply movement friction
		geVec3d_Scale(&Player->Velocity, Scale, &Player->Velocity);
	}

	return ClearMove;
}

//=====================================================================================
//	Bot_ScanPhysics
//=====================================================================================
geBoolean Bot_ScanPhysics(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer	*Player;
	Bot_Var			*DBot;
    geBoolean ClearMove = GE_TRUE;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	// Walk
	Player->LastGoodPos = Player->XForm.Translation;

	ClearMove = Bot_ScanCheckVelocity(VSI, Player, Time);

	SqueezeVector(&Player->Velocity, 0.2f);
	geVec3d_AddScaled(&Player->XForm.Translation, &Player->Velocity, Time, &Player->XForm.Translation);

	Bot_CheckPosition(VSI, Player);
	
	return ClearMove;
}


//=====================================================================================
//	Bot_CheckPosition
//=====================================================================================
geBoolean Bot_CheckPosition(GenVSI *VSI, void *PlayerData)
{
	geVec3d		Mins, Maxs, Pos;
	GE_Contents	Contents;
	GPlayer		*Player;
	Bot_Var			*DBot;
	uint32		ColFlags;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	Mins = Player->Mins;
	Maxs = Player->Maxs;

	Pos = Player->XForm.Translation;

	ColFlags = GE_COLLIDE_MODELS;

	if (geWorld_GetContents(GenVSI_GetWorld(VSI), &Pos, &Mins, &Maxs, ColFlags, 0,  NULL, NULL, &Contents))
	{
		if (Contents.Contents & GE_CONTENTS_SOLID)
		{
			Player->XForm.Translation = Player->LastGoodPos;
			return GE_TRUE;
		}
	}

	return GE_FALSE;
}


//=====================================================================================
//	Bot_MoveScan
//=====================================================================================
geBoolean Bot_MoveScan(GenVSI *VSI, void *PlayerData, geVec3d *vec_dir, float dist, geVec3d *stop_pos)
{
	GPlayer	*Player;
	Bot_Var			*DBot;
	geVec3d SaveVelocity, SaveGoodPos, Pos;
	geBoolean ret_val = GE_FALSE;
	int32 SaveState;
    float Time = 1.0f; // don't allow scaling by time

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

    // Save off values that will change
	Pos = Player->XForm.Translation;
    SaveVelocity = Player->Velocity;
    SaveGoodPos = Player->LastGoodPos;
    SaveState = Player->State;

    // New Velocity in this direction
    geVec3d_Scale(vec_dir, dist, &Player->Velocity);

    if (!Bot_ScanPhysics(VSI, Player, Time))
        {
        ret_val = GE_TRUE;
        }

    *stop_pos = Player->XForm.Translation;

    // Restore saved values
    Player->Velocity = SaveVelocity;
    Player->XForm.Translation = Pos;
    Player->LastGoodPos = SaveGoodPos;
    Player->State = (GPlayer_PState)SaveState;

	return ret_val;
}
		

//=====================================================================================
//	Bot_OverLedge
//=====================================================================================
geBoolean Bot_OverLedge(GenVSI *VSI, void *PlayerData, geVec3d *ScanPos)
{
	GE_Collision Collision;
	geWorld			*World;
	GPlayer	*Player;
	Bot_Var			*DBot;
	geVec3d Pos, Pos2;
	geBoolean ret_val;
	GE_Contents Contents;
	geVec3d Mins, Maxs;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	World = GenVSI_GetWorld(VSI);

	assert(World);

    if (Player->State == PSTATE_InAir)
        return GE_FALSE;

	Pos = *ScanPos;
	Pos2 = Pos;

	// Try and move down by this much
	Pos2.Y -= (100.0f - Player->Maxs.Y) + Player->Maxs.Y;

#if 0
	Mins = RayMins;
	Maxs = RayMaxs;
	Mins.Y = Player->Mins.Y;
	Maxs.Y = Player->Maxs.Y;
#else
	Mins = Player->Mins;
	Maxs = Player->Maxs;
	Mins.X /= 2.0f;
	Mins.Z /= 2.0f;
	Maxs.X /= 2.0f;
	Maxs.Z /= 2.0f;
#endif
	
    // just use a ray with no size - not the player box
	ret_val = geWorld_Collision(World, &Mins, &Maxs,//&RayMins, &RayMaxs, 
		&Pos, &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0xffffffff, NULL, NULL, &Collision);

	World = GenVSI_GetWorld(VSI);
	if (geWorld_GetContents(World, &Collision.Impact, &Mins, &Maxs, GE_COLLIDE_MODELS, 0,  NULL, NULL, &Contents))
	{
		if (Contents.Model)
		{
			GPlayer	*TPlayer;

			TPlayer = (GPlayer*)geWorld_ModelGetUserData(Contents.Model);

			if (TPlayer && TPlayer->Trigger && TPlayer->ViewFlags & VIEW_TYPE_TOUCH)
			{
				switch (TPlayer->Trigger(VSI, TPlayer, Player, NULL))
				{

					case 2:
						return GE_TRUE;
					default:
						break;
				}
			}
		}
	}


	if (!ret_val)
	{
		return GE_TRUE;
	}	

	World = GenVSI_GetWorld(VSI);
	if (geWorld_GetContents(World, &Player->XForm.Translation, &Player->Mins, &Player->Maxs, GE_COLLIDE_MODELS, 0,  NULL, NULL, &Contents))
	{
		if (Contents.Model)
		{
			GPlayer	*TPlayer;

			TPlayer = (GPlayer*)geWorld_ModelGetUserData(Contents.Model);

			if (TPlayer && TPlayer->Trigger && TPlayer->ViewFlags & VIEW_TYPE_TOUCH)
			{
				switch (TPlayer->Trigger(VSI, TPlayer, Player, NULL))
				{
					case 2:
						return GE_TRUE;
					default:
						break;
				}
			}
		}
	}

	return GE_FALSE;
}


//=====================================================================================
//	Bot_OverLedgeScan
//=====================================================================================
geBoolean Bot_OverLedgeScan(GenVSI *VSI, void *PlayerData, const geVec3d *StartPos, const geVec3d *EndPos, geVec3d *StopPos)
{
	GE_Collision Collision;
	geWorld			*World;
	Bot_Var			*DBot;
	geVec3d Pos, Pos2, Vec, LastGoodPos, Mins, Maxs;
	float len;
	int32 i;
	GPlayer *Player;
	geBoolean ret_val;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	World = GenVSI_GetWorld(VSI);

	assert(World);

    if (Player->State == PSTATE_InAir)
        return GE_FALSE;

	geVec3d_Subtract(EndPos, StartPos, &Vec);
	len = geVec3d_Length(&Vec);
	geVec3d_Normalize(&Vec);
	
	Mins = Player->Mins;
	Maxs = Player->Maxs;
	Mins.X /= 2.0f;
	Mins.Z /= 2.0f;
	Maxs.X /= 2.0f;
	Maxs.Z /= 2.0f;

	Pos = *StartPos;

	for (i = 4; i > 0; i--)
	{
		LastGoodPos = Pos;
		geVec3d_AddScaled(StartPos, &Vec, len/(i*2), &Pos);
	
		Pos2 = Pos;

		// Try and move down by this much
		Pos2.Y -= 100.0f;

		// just use a ray with no size - not the player box
		ret_val = geWorld_Collision(World, &Mins, &Maxs, 
			&Pos, &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS, 0xffffffff, NULL, NULL, &Collision);

		if (!ret_val)
			{
			*StopPos = LastGoodPos;
			return(GE_TRUE);
			}

	}

	return GE_FALSE;
}
    
//=====================================================================================
//	Bot_FindNewMoveVec
//=====================================================================================
geBoolean 
Bot_FindNewMoveVec(GenVSI *VSI, void *PlayerData, const geVec3d *TargetPos, int32 dir, float DistToMove, geVec3d *save_vec)
    {
	GPlayer	*Player;
	Bot_Var			*DBot;

    static float toward_angle_delta[4][10] = 
        { 
        { -28.0f, -68.0f, 28.0f, 68.0f, -45.0f, 45.0f, -90.0f, 90.0f, -999.0f}, 
        //{ 0.0f, -28.0f, -68.0f, 28.0f, 68.0f, -45.0f, 45.0f, -90.0f, 90.0f, -999}.0f,  // This is pretty good for circle strafing
        { -18.0f, -28.0f, 18.0f, 28.0f, -45.0f, 45.0f, -90.0f, 90.0f, -999.0f}, 
        { 28.0f, 68.0f, -28.0f, -68.0f, 45.0f, -45.0f, 90.0f, -90.0f, -999.0f}, 
        { 18.0f, 28.0f, -18.0f, -28.0f, 45.0f, -45.0f, 90.0f, -90.0f, -999.0f}
        };
        
    static float away_angle_delta[4][10] = 
        { 
        //{ 180.0f, 135.0f, -112.0f, 112.0f, -157.0f, 157.0f, 180.0f, 45.0f, -45.0f, -999}.0f, 
        { -135.0f, 135.0f, -112.0f, 112.0f, -157.0f, 157.0f, 180.0f, -999.0f}, 
        { 135.0f, -135.0f, 112.0f, -112.0f, -157.0f, 157.0f, 180.0f, -999.0f}, 
        { 157.0f, -157.0f, -135.0f, 135.0f, -112.0f, 112.0f, 180.0f, -999.0f},
        { 157.0f, -157.0f, 135.0f, -135.0f, 112.0f, -112.0f, 180.0f, -999.0f}
        };

    static float evade_angle_delta[4][8] = 
        { 
        { 90.0f,  -90.0f, -112.0f, 112.0f, -68.0f, 68.0f,  45.0f,  -999.0f}, 
        { -90.0f,  90.0f,  68.0f, -68.0f, -112.0f, 112.0f, 135.0f, -999.0f}, 
        { 68.0f, -68.0f,  -112.0f, 112.0f, -90.0f, 90.0f,  45.0f,  -999.0f},
        { 68.0f,  90.0f,  -68.0f, -90.0f, 112.0f, -112.0f, 135.0f, -999.0f}
        };
        
    float *adp;
        
    geBoolean ret;
	geBoolean found = GE_FALSE;
	
    float dist;
    // start out with mininum distance that will be accepted as a move
    float save_dist = 25.0f;

	geVec3d Vec2Tgt;
	geVec3d Pos, TgtPos;
	geVec3d vec_dir, stop_pos, save_stop_pos;
	float Dist2TgtPos,NewDist2TgtPos;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	Pos = Player->XForm.Translation;
	TgtPos = *TargetPos;
	Dist2TgtPos = geVec3d_DistanceBetween(&TgtPos, &Pos);

	save_dist = Dist2TgtPos;

	// Find vector to the player
    geVec3d_Subtract(&TgtPos, &Pos, &Vec2Tgt);
	Vec2Tgt.Y = 0.0f;
    if (geVec3d_Length(&Vec2Tgt) == 0)
        Vec2Tgt.X = 1.0f;

	// choose a random angle array
	if (dir == BOT_DIR_TOWARD)
		adp = &toward_angle_delta[RandomRange(4)][0];
	else
	if (dir == BOT_DIR_AWAY)
		adp = &away_angle_delta[RandomRange(4)][0];
	else
	if (dir == BOT_DIR_NONE)
		{
		adp = &evade_angle_delta[RandomRange(4)][0];
		save_dist = 20.0f;
		}

	for (; *adp != -999; adp++)
		{
		Vec2Tgt.Y = 0.0f;
		VectorRotateY(&Vec2Tgt, (*adp * (PI_2/360)), &vec_dir);
        vec_dir.Y = 0.0f;
		geVec3d_Normalize(&vec_dir);
    
		// check to see how far we can move
		ret = Bot_MoveScan(VSI, Player, &vec_dir, DistToMove, &stop_pos);

		NewDist2TgtPos = geVec3d_DistanceBetween(&TgtPos, &stop_pos);

		dist = geVec3d_DistanceBetween(&Pos, &stop_pos);

		// hit something really close
		if (dist < 25.0f)
			continue;

		if (Bot_OverLedgeScan(VSI, Player, &Pos, &stop_pos, &stop_pos))
			{
			dist = geVec3d_DistanceBetween(&Pos, &stop_pos);
			// hit something really close
			if (dist < 25.0f)
				continue;
			}

		if (dir == BOT_DIR_TOWARD)
			{
			if (NewDist2TgtPos < save_dist)
				{
				save_stop_pos = stop_pos;
				*save_vec = vec_dir;
				save_dist = dist;
				found = GE_TRUE;
				}	
			}
		else
		if (dir == BOT_DIR_AWAY)
			{
			if (NewDist2TgtPos > save_dist)
				{
				save_stop_pos = stop_pos;
				*save_vec = vec_dir;
				save_dist = dist;
				found = GE_TRUE;
				}	
			}
		else
		if (dir == BOT_DIR_NONE)
			{
			if (dist > save_dist)
				{
				save_stop_pos = stop_pos;
				*save_vec = vec_dir;
				save_dist = dist;
				found = GE_TRUE;
				}	
			}

		if (ret == GE_FALSE)
			{
			// cleanly moved in new direction without hitting something
			save_stop_pos = stop_pos;
			found = GE_TRUE;
			break;
			}
		}

	if (found)
	{
		geVec3d Vec2Point;
		
		// gimme the actual vector to the stop point
		geVec3d_Subtract(&save_stop_pos, &Pos, &Vec2Point);
		Vec2Point.Y = 0.0f;
		
		// scale it so that its a little shorter and find new stop point
		geVec3d_Scale(&Vec2Point, 0.8f, &Vec2Point);
		geVec3d_Add(&Pos, &Vec2Point, &save_stop_pos);
		
		DBot->TgtPos = save_stop_pos;
        
		return(GE_TRUE);
	}

    return(GE_FALSE);
    }


//=====================================================================================
//	Bot_PastPoint
//=====================================================================================
geBoolean Bot_PastPoint(geVec3d *Pos, geVec3d *MoveVector, geVec3d *TgtPos)
{
	geVec3d Vec2Point, MoveVec = *MoveVector;

	// Temp hack
	if (geVec3d_Length(TgtPos) == 0.0f)
	{
	return GE_TRUE;	
	}

	// see if the bot went past the point
	geVec3d_Subtract(TgtPos, Pos, &Vec2Point);

	// make sure they are 2d for this test
    Vec2Point.Y = 0.0f;
	MoveVec.Y = 0.0f;

	geVec3d_Normalize(&Vec2Point);	
    geVec3d_Normalize(&MoveVec);	

	return (geVec3d_DotProduct(&Vec2Point, &MoveVec) < 0.0f);
}	

//=====================================================================================
//	Bot_ShootFoot
//=====================================================================================
geBoolean Bot_ShootFoot(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer *Player = (GPlayer *)PlayerData;
	Bot_Var	*DBot;
    geVec3d ShootPos, ShootVec;
	int16 wpn;

	DBot = static_cast<Bot_Var*>(Player->userData);

	if (Player->Health < 100)
		return GE_FALSE;

    // get the shooing position
    geVec3d_Scale(&DBot->MoveVec, 50.0f, &ShootVec);
    geVec3d_Add(&Player->XForm.Translation, &ShootVec, &ShootPos);
    ShootPos.Y = Player->XForm.Translation.Y - 50.0f;

    // force a shot here
	wpn = Player->CurrentWeapon;
	Bot_SetWeapon(VSI, Player, 0);
    Player->NextWeaponTime = GenVSI_GetTime(VSI) - 0.01f;
    Bot_Shoot(VSI, Player, &ShootPos, Time);
	Player->CurrentWeapon = wpn;

    return GE_TRUE;
}

//=====================================================================================
//	Bot_Suicide
//=====================================================================================
geBoolean Bot_Suicide(GenVSI *VSI, void *PlayerData, float Time)
{
	GPlayer *Player = (GPlayer *)PlayerData;
	Bot_Var	*DBot;
    geVec3d ShootPos, ShootVec;
	int16 wpn;

	DBot = static_cast<Bot_Var*>(Player->userData);

    // get the shooing position
    geVec3d_Scale(&DBot->MoveVec, 50.0f, &ShootVec);
    geVec3d_Add(&Player->XForm.Translation, &ShootVec, &ShootPos);
    ShootPos.Y = Player->XForm.Translation.Y - 50.0f;

    // force a shot here
	wpn = Player->CurrentWeapon;
	Bot_SetWeapon(VSI, Player, 0);
    Player->NextWeaponTime = GenVSI_GetTime(VSI) - 0.01f;
    Bot_Shoot(VSI, Player, &ShootPos, Time);
	Player->CurrentWeapon = wpn;

    return GE_TRUE;
}


//=====================================================================================
//	Bot_MoveToPoint
//=====================================================================================
geBoolean Bot_MoveToPoint(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			Pos, TgtPlayerPos, Vec2Player;
	float			MoveSpeed;
	GPlayer			*Player;
	Bot_Var			*DBot;
	geBoolean		ResetVelocity = GE_FALSE, BotShoot;
    TrackPt         *ThisPoint = NULL, *NextPoint = NULL;
    //int32			DoAction;
	geWorld			*World;

	World = GenVSI_GetWorld(VSI);
	assert(World);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
    

	assert(DBot->TgtPlayer);

	// save off the position
    Pos = Player->XForm.Translation;
    TgtPlayerPos = DBot->TgtPlayer->XForm.Translation;

	geVec3d_Subtract(&TgtPlayerPos, &Pos, &Vec2Player);

	// decide whether to face player & shoot
	switch (DBot->Mode)
		{
		case MODE_UNSTICK:
			BotShoot = FALSE;
			Bot_SetupXForm(VSI, PlayerData, &Vec2Player);       //Face Player
			break;
		case MODE_RETREAT:
		case MODE_RETREAT_ON_TRACK:
			if (DBot->FaceTgtPlayerOnRetreat)
				{
				BotShoot = TRUE;
				Bot_SetupXForm(VSI, PlayerData, &Vec2Player);       //Face Player
				}
			else
				{
				BotShoot = FALSE;
				Bot_SetupXForm(VSI, PlayerData, &DBot->MoveVec);     //Face run direction
				}
			break;
		case MODE_WANDER:
		case MODE_WANDER_ON_TRACK:
		//case MODE_GOAL_POINT:
			BotShoot = FALSE;
			Bot_SetupXForm(VSI, PlayerData, &DBot->MoveVec);     //Face run direction
			break;
		default:
			BotShoot = TRUE;
			Bot_SetupXForm(VSI, PlayerData, &Vec2Player);       //Face Player
			break;

		}

	MoveSpeed = Time;

	if (Player->State == PSTATE_InAir)
		MoveSpeed *= 0.15f;

    #if 1
    //an attempt to counteract Velocity and zero in on the target position
	if (!Bot_PastPoint(&Player->XForm.Translation, &DBot->MoveVec, &DBot->TgtPos))
        {
        geVec3d save = DBot->MoveVec;

        // get a new MoveVec
	    geVec3d_Subtract(&DBot->TgtPos, &Player->XForm.Translation, &DBot->MoveVec);
        DBot->MoveVec.Y = 0.0f;
	    geVec3d_Normalize(&DBot->MoveVec);

        if (Bot_PastPoint(&Player->XForm.Translation, &DBot->MoveVec, &DBot->TgtPos))
            {
            DBot->MoveVec = save;
            }
        }
    #endif

	geVec3d_AddScaled(&Player->Velocity, &DBot->MoveVec, DBot->RunSpeed*MoveSpeed, &Player->Velocity);
	
    if (Track_OnTrack(&DBot->TrackInfo))
		{
		PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);

		if (DBot->TrackInfoPrev.TrackNdx == DBot->TrackInfo.TrackNdx &&
			Track_OnTrack(&DBot->TrackInfoPrev) && 
			Track_GetPoint(&DBot->TrackInfoPrev)->Action != POINT_TYPE_WAIT_POINT_VISIBLE)
			{
			if (geVec3d_DistanceBetween(&Player->XForm.Translation, &Player->LastGoodPos) <= 5.0f)
				{
				// get off of track
				Bot_ClearTrack(VSI, Player);
				// pick another mode
				Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
				//if (Bot_ModeThink(VSI, Player, Time))  //If couldn't move try a new action
				return GE_TRUE;
				}
			}
		else
			{
			if (geVec3d_DistanceBetween(&Player->XForm.Translation, &Player->LastGoodPos) <= 0.5f)
				{
				// get off of track
				Bot_ClearTrack(VSI, Player);
				// pick another mode
				Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
				//if (Bot_ModeThink(VSI, Player, Time))  //If couldn't move try a new action
				return GE_TRUE;
				}
			}
		}
    else
	// Run physics code...
	if (!Bot_Physics(VSI, Player, Time))
        {
		if (DBot->Bumps++ > 3)
			{
			DBot->Bumps = 0;
			Bot_ModeChange(VSI, Player, MODE_UNSTICK, GE_TRUE, Time);
            return GE_TRUE;
			}

        if (Bot_ModeThink(VSI, Player, Time))  //If couldn't move try a new action
            return GE_TRUE;
        }
	else
		{
		DBot->Bumps = 0;
		}

	if (Player->State == PSTATE_Normal)
        {
	    if (Bot_OverLedge(VSI, Player, &Player->XForm.Translation))
            {
			if (DBot->LedgeBumps++ > 16)
				{
				DBot->LedgeBumps = 0;
				Bot_ModeChange(VSI, Player, MODE_UNSTICK, GE_TRUE, Time);
				return GE_TRUE;
				}

			Bot_ModeThinkLedge(VSI, Player, Time);                                                                                                                                                                    
            }
		else
			DBot->LedgeBumps = 0;
	    }

	Bot_GetContents(VSI, Player, Time);

	Bot_Animate(VSI, Player, Time);

	// shooting
	if (Player->State != PSTATE_InAir && 
        !PlayerDead(DBot->TgtPlayer) &&
		BotShoot)
        //DBot->Dir != BOT_DIR_AWAY)
        {
        //if (Bot_CanSeePlayerToPlayer(World, &Player->XForm.Translation, &TgtPlayerPos))
        if (DBot->TimeSeen > 0.0f)
	        Bot_Shoot(VSI, Player, &TgtPlayerPos, Time);
        }

	//??bot
	Player->VPos = Player->XForm.Translation;

	// see about getting off of a path
	if (Bot_LeaveTrack(VSI, Player, Time))
    	{
		DBot->GoalPos = DBot->TgtPlayer->XForm.Translation;
		Bot_ModeThink(VSI, PlayerData, Time);
		return GE_TRUE;
	    }

    // test current pos and MoveVec
    if (Bot_PastPoint(&Player->XForm.Translation, &DBot->MoveVec, &DBot->TgtPos))    
	{
	    if (DBot->Mode == MODE_GOAL_POINT &&
            Bot_PastPoint(&Player->XForm.Translation, &DBot->MoveVec, &DBot->GoalPos))
            {
            // pick another mode
            Bot_ModeChange(VSI, Player, MODE_NULL, GE_TRUE, Time);
            return GE_TRUE;
            }

        if (!Track_OnTrack(&DBot->TrackInfo))
            {
			// decide what to do next
            if (Bot_ModeThink(VSI, PlayerData, Time))
                return GE_TRUE;

			// last resort - just reposition
            Bot_Reposition(VSI, Player, &DBot->GoalPos, DBot->Dir);
            return GE_TRUE;
            }

		// -=ON=- A TRACK PAST HERE
		DBot->PastFirstTrackPoint = GE_TRUE;

	    //if ((DBot->Mode == MODE_ON_TRACK || DBot->Mode == MODE_RETREAT_ON_TRACK) &&
	    if (Track_OnTrack(&DBot->TrackInfo) &&
			!Bot_PastPoint(&Player->XForm.Translation, &DBot->MoveVec, &DBot->GoalPos))
            {
            return GE_TRUE;
            }
    
        ThisPoint = Track_GetPoint(&DBot->TrackInfo);

		DBot->TrackInfoPrev = DBot->TrackInfo;
        if (NextPoint = Track_NextPoint(&DBot->TrackInfo))
            {
            // if there is a next point - face that direction
            DBot->GoalPos = DBot->TgtPos = *NextPoint->Pos;

		    //geVec3d_Subtract(&DBot->TgtPos, &Player->XForm.Translation, &DBot->MoveVec);
		    geVec3d_Subtract(NextPoint->Pos, ThisPoint->Pos, &DBot->MoveVec);
            DBot->MoveVec.Y = 0.0f;
		    geVec3d_Normalize(&DBot->MoveVec);
            }

        if (Bot_TrackAction(VSI, Player, ThisPoint, NextPoint, Time))
            return GE_TRUE;

		// Track was cleared?
		if (!Track_OnTrack(&DBot->TrackInfo))
			{
			Bot_ModeChange(VSI, PlayerData, MODE_NULL, GE_FALSE, Time);
			}

		if (!NextPoint)
        {
			// Finish Track will pop stack if necessary
            Bot_FinishTrack(VSI, Player);
			// ONLY THINK IF YOU ARE DONE WITH THE STACK!
			if (StackIsEmpty(&DBot->TrackStack))
				Bot_ModeThink(VSI, PlayerData, Time);
            return(GE_TRUE);
        }

	}

	return GE_TRUE;
}

//=====================================================================================
//	Bot_MoveFree
//=====================================================================================
geBoolean Bot_MoveFree(GenVSI *VSI, void *PlayerData, float Time)
{
	geVec3d			Pos, TgtPlayerPos, Vec2Player;
	float			MoveSpeed;
	GPlayer			*Player;
	Bot_Var			*DBot;
	geBoolean		ResetVelocity = GE_FALSE;
    TrackPt         *ThisPoint = NULL, *NextPoint = NULL;
	geWorld			*World;

	World = GenVSI_GetWorld(VSI);
	assert(World);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	// save off the position
    Pos = Player->XForm.Translation;
    TgtPlayerPos = DBot->TgtPlayer->XForm.Translation;

	geVec3d_Subtract(&TgtPlayerPos, &Pos, &Vec2Player);

    Bot_SetupXForm(VSI, PlayerData, &DBot->MoveVec);     //Face run direction

	MoveSpeed = Time;

	if (Player->State == PSTATE_InAir)
		MoveSpeed *= 0.15f;
	
	geVec3d_AddScaled(&Player->Velocity, &DBot->MoveVec, DBot->RunSpeed*MoveSpeed, &Player->Velocity);
	
	PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);
	if (geVec3d_DistanceBetween(&Player->XForm.Translation, &Player->LastGoodPos) <= 25.0f)
		{
        // reposition again
        Bot_ModeThink(VSI, Player, Time);
		return GE_TRUE;
		}

	Bot_GetContents(VSI, Player, Time);

	Bot_Animate(VSI, Player, Time);

	//??bot
	Player->VPos = Player->XForm.Translation;

    // test current pos and MoveVec
    if (Bot_PastPoint(&Player->XForm.Translation, &DBot->MoveVec, &DBot->TgtPos))    
		{
		// decide what to do next
        if (Bot_ModeThink(VSI, PlayerData, Time))
            return GE_TRUE;
		}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_ActionGetHealth
//=====================================================================================
geBoolean Bot_ActionGetHealth(GenVSI *VSI, void *PlayerData, float Range, float Time)
{
    GPlayer *HealthItem;
	GPlayer		*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

    if (HealthItem = Bot_FindItem(VSI, &Player->XForm.Translation, ItemHealthList, BOT_SEARCH_Y_RANGE, Range))
        {
		DBot->GoalPos = HealthItem->Pos;
        Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
        Bot_ModeChange(VSI, Player, MODE_GOAL_POINT, GE_FALSE, Time);
		Bot_ShootFoot(VSI, Player, Time);
        return GE_TRUE;
        }

#if 0
	DBot->Dir = BOT_DIR_NONE;

    if (Bot_FindTrack(VSI, Player, HealthTrackList))
        {
		DBot->GoalPos = Track_GetPoint(&DBot->TrackInfo)->Pos;
        Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
        return GE_TRUE;
        }
#endif

    return GE_FALSE;
}

//=====================================================================================
//	Bot_ActionGetWeapon
//=====================================================================================
geBoolean Bot_ActionGetWeapon(GenVSI *VSI, void *PlayerData, float Range, float Time)
{
    GPlayer *WeaponItem;
	GPlayer		*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

    if (WeaponItem = Bot_FindItem(VSI, &Player->XForm.Translation, ItemWeaponList, BOT_SEARCH_Y_RANGE, Range))
        {
		DBot->GoalPos = WeaponItem->Pos;
        Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
        Bot_ModeChange(VSI, Player, MODE_GOAL_POINT, GE_FALSE, Time);
		Bot_ShootFoot(VSI, Player, Time);
        return GE_TRUE;
        }

#if 0
	DBot->Dir = BOT_DIR_NONE;

    if (Bot_FindTrack(VSI, Player, WeaponTrackList))
        {
		DBot->GoalPos = Track_GetPoint(&DBot->TrackInfo)->Pos;
        Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
        return GE_TRUE;
        }
#endif

    return GE_FALSE;
}


//=====================================================================================
//	Bot_ChooseBestWeapon
//=====================================================================================
void Bot_ChooseBestWeapon(GenVSI *VSI, void *PlayerData)
{
	int16			i;
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	for (i=3; i>=0; i--)
	{
        if (i == 0)
            {
            Player->CurrentWeapon = 0;
            break;
            }

		if (Player->InventoryHas[i] && Player->Inventory[i] > 0)
            {
            Player->CurrentWeapon = i;
            break;
            }
	}

	assert(Player->CurrentWeapon >= 0 && Player->CurrentWeapon < 4);
}

//=====================================================================================
//	Bot_SetWeapon
//=====================================================================================
geBoolean Bot_SetWeapon(GenVSI *VSI, void *PlayerData, int16 WeaponNum)
{
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	if (WeaponNum == 0)
		{
		Player->CurrentWeapon = WeaponNum;
		return GE_TRUE;
		}

	if (Player->CurrentWeapon < 0 || Player->CurrentWeapon > 3)
		return GE_FALSE;

	if (Player->InventoryHas[WeaponNum] && Player->Inventory[WeaponNum] > 0)
	{
		Player->CurrentWeapon = WeaponNum;
		return GE_TRUE;
	}

	return GE_FALSE;
}


//=====================================================================================
//	Bot_WeaponSetFromArray
//=====================================================================================
void Bot_WeaponSetFromArray(GenVSI *VSI, void *PlayerData, int32 *WeaponArr)
{
	int32			i;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	for (i=0; i < 4; i++)
	{
        if (WeaponArr[i] == 0)
            {
            Player->CurrentWeapon = 0;
            break;
            }
        
		if (Player->InventoryHas[WeaponArr[i]] && Player->Inventory[WeaponArr[i]] > 0)
            {
            Player->CurrentWeapon = (int16)WeaponArr[i];
            break;
            }
	}

	assert(Player->CurrentWeapon >= 0 && Player->CurrentWeapon < 4);
}


//=====================================================================================
//=====================================================================================
//
//	Bot Related Setup for the World and Game
//
//=====================================================================================
//=====================================================================================



//=====================================================================================
//	Bot_Main
//=====================================================================================
geBoolean Bot_Main(GenVSI *VSI, const char *LevelName)
	{

	BotDeathMatchStart = NULL;

	GenVSI_SetClassSpawn(VSI, "BotMatchStart", Bot_MatchStart, NULL);
	GenVSI_SetClassSpawn(VSI, "BotActorStart", Bot_ActorStart, NULL);
	GenVSI_SetClassSpawn(VSI, "BlockActor", BlockActor_Spawn, NULL);

	return GE_TRUE;
	}


//=====================================================================================
//	BotMatch_Spawn
//=====================================================================================
void Bot_MatchStart(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	BotMatchStart	*Ps;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = NULL;
	Player->Trigger = NULL;
	Player->Blocked = NULL;
	Player->Time = 0.0f;

	if (ClassData == NULL)
		{
			GenVS_Error("Bot_MatchStart: entity missing class data ('%s')\n",EntityName);
		}

	Ps = (BotMatchStart*)ClassData;

	geXForm3d_SetIdentity(&Player->XForm);
	geXForm3d_SetTranslation(&Player->XForm, Ps->origin.X, Ps->origin.Y, Ps->origin.Z);

	Player->VPos = Player->XForm.Translation;

	//return GE_TRUE;
}


//=====================================================================================
//	GetBotMatchSpawn
//=====================================================================================
GPlayer *GetBotMatchSpawn(GenVSI *VSI)
{
	extern int32	GMode;

	if (GMode == 0)
	{
		BotDeathMatchStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, BotDeathMatchStart, "BotMatchStart");
			   
		if (!BotDeathMatchStart)
			BotDeathMatchStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, NULL, "BotMatchStart");
	}
	else
	{
		BotDeathMatchStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, BotDeathMatchStart, "BotDeathMatchStart");
			   
		if (!BotDeathMatchStart)
			BotDeathMatchStart = (GPlayer*)GenVSI_GetNextPlayer(VSI, NULL, "BotDeathMatchStart");
	}

	return BotDeathMatchStart;
}

//=====================================================================================
//	Bot_Create
//=====================================================================================
void Bot_Create(GenVSI *VSI, void *PlayerData)
{
	Bot_Var *DBot;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;
	assert(Player);

	Player->Control = Bot_Control;
	Player->DFunc = Bot_Destroy;

	if (!Player->userData)
	{
		DBot = GE_RAM_ALLOCATE_STRUCT(Bot_Var);
		assert(DBot);
		Player->userData = (void *)DBot;
	}
		
	memset(DBot, 0, sizeof(Bot_Var));
	DBot->RunSpeed = BOT_RUN_SPEED;
	DBot->Action = Bot_ModeThink;
    Bot_ClearTrack(VSI, Player);
	DBot->Dir = 1;
    StackInit(&DBot->TrackStack);
    //DBot->TOS = -1;
}

void Bot_SetLighting(GenVSI *VSI, void *PlayerData)
	{
	GPlayer			*Player;
	geVec3d			Normal = {0.0f, 1.0f, 0.0f};

	Player = (GPlayer*)PlayerData;

/*
GENESISAPI geBoolean GENESISCC geActor_SetLightingOptions(geActor *A,
									geBoolean UseFillLight,
									const geVec3d *FillLightNormal,
									geFloat FillLightRed,				// 0 .. 255
									geFloat FillLightGreen,				// 0 .. 255
									geFloat FillLightBlue,				// 0 .. 255
									geFloat AmbientLightRed,			// 0 .. 255
									geFloat AmbientLightGreen,			// 0 .. 255
									geFloat AmbientLightBlue,			// 0 .. 255
									geBoolean AmbientLightFromFloor,
									int MaximumDynamicLightsToUse,		// 0 for none
									const char *LightReferenceBoneName);
*/
	
	return;

	geActor_SetLightingOptions(Player->Actor, GE_TRUE, &Normal, 
		155.0f, 155.0f, 155.0f, 
		1.0f, 1.0f, 1.0f, 
		GE_TRUE, 3, NULL, GE_FALSE);
	}


//=====================================================================================
//	Game_SpawnBot
//=====================================================================================
geBoolean Game_SpawnBot(GenVSI *VSI, geWorld *World, void *PlayerData, void *ClassData)
{
	int32			i;
	GPlayer			*DMStart;
	GPlayer			*Player;
	geVec3d			Normal = {0.0f, 1.0f, 0.0f};

	Player = (GPlayer*)PlayerData;

	// Get a DM start
	DMStart = GetBotMatchSpawn(VSI);
	if (!DMStart)
		{
		//GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "No Bot spawn positions exist.");
		GenVSI_ClientDisconnect(VSI, Player->ClientHandle);
		return GE_TRUE;
		}
	
	Player->XForm = DMStart->XForm;

	Player->Time = 0.0f;

	Player->Scale = 2.7f;	
	
	Player->Mins.X = -30.0f;
	Player->Mins.Y = -10.0f;
	Player->Mins.Z = -30.0f;
	Player->Maxs.X =  30.0f;
	Player->Maxs.Y = 160.0f;
	Player->Maxs.Z =  30.0f;

	Player->GunOffset.X = 0.0f;
	Player->GunOffset.Y = 130.0f;
	Player->GunOffset.Z = 0.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_YAW_ONLY | VIEW_TYPE_COLLIDEMODEL;
	Player->ViewIndex = ACTOR_INDEX_PLAYER;
	Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
	Player->DammageFlags = DAMMAGE_TYPE_NORMAL | DAMMAGE_TYPE_RADIUS;	


	// Hook player up to client physics controller
	Player->ControlIndex = 0;
	Player->DFunc = Bot_Destroy;

	Bot_Create(VSI, Player);

	// Set the view player on this machine to the clients player
	// !Important! - don't set the player view to the bot
	//GenVSI_SetViewPlayer(VSI, ClientHandle, Player);

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
//	Bot_IsActorRespawn
//=====================================================================================
geBoolean Bot_IsActorRespawn(GenVSI *VSI, void *PlayerData)
{
	geEntity_EntitySet	*ClassSet;
	geEntity			*Entity;
	BotActorStart		*Bs;
	GPlayer				*Player;
	geWorld				*World;
	
	Player = (GPlayer*)PlayerData;

	World = GenVSI_GetWorld(VSI);
	assert(World);

	// Look for the class name in the world
	ClassSet = geWorld_GetEntitySet(World, "BotActorStart");
    if (!ClassSet)
        return GE_FALSE;

	Entity = NULL;

	while (1)
	{
		Entity = geEntity_EntitySetGetNextEntity(ClassSet, Entity);

		if (!Entity)
			break;

		Bs = (BotActorStart*)geEntity_GetUserData(Entity);

		if ((GPlayer*)Bs->Ptr == Player)
		{
			return(Bs->Respawn);
		}
	}

	return(GE_FALSE);

}

//=====================================================================================
//	Bot_GetActorStart
//=====================================================================================
BotActorStart *Bot_GetActorStart(GenVSI *VSI, void *PlayerData)
{
	geEntity_EntitySet	*ClassSet;
	geEntity			*Entity;
	BotActorStart		*Bs;
	GPlayer				*Player;
	geWorld				*World;
	
	Player = (GPlayer*)PlayerData;

	World = GenVSI_GetWorld(VSI);
	assert(World);

	// Look for the class name in the world
	ClassSet = geWorld_GetEntitySet(World, "BotActorStart");
    if (!ClassSet)
        return GE_FALSE;

	Entity = NULL;

	while (1)
	{
		Entity = geEntity_EntitySetGetNextEntity(ClassSet, Entity);

		if (!Entity)
			break;

		Bs = (BotActorStart*)geEntity_GetUserData(Entity);

		if ((GPlayer*)Bs->Ptr == Player)
		{
			return(Bs);
		}
	}

	return(NULL);

}


//=====================================================================================
//	Bot_ActorStart
//=====================================================================================
void Bot_ActorStart(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	int32			i;
	GPlayer			*Player;
	BotActorStart   *Start;
	Bot_Var			*DBot;

	if (ClassData == NULL)
		{
			GenVS_Error("Bot_ActorStart: entity missing class data ('%s')\n",EntityName);
		}

	Start = (BotActorStart*)ClassData;

	Player = (GPlayer*)GenVSI_SpawnPlayer(VSI, "Bot_Actor");

	Start->Ptr = (char*)(Player); //link to entity - sort of hacky

	//Player->XForm = Start->XForm;
	geXForm3d_SetIdentity(&Player->XForm);
	geXForm3d_SetTranslation(&Player->XForm, Start->origin.X, Start->origin.Y, Start->origin.Z);

	Player->Time = 0.0f;

	Player->Scale = 2.7f;	

	Player->Mins.X = -30.0f;
	Player->Mins.Y = -10.0f;
	Player->Mins.Z = -30.0f;
	Player->Maxs.X =  30.0f;
	Player->Maxs.Y = 160.0f;
	Player->Maxs.Z =  30.0f;

	Player->GunOffset.X = 0.0f;
	Player->GunOffset.Y = 130.0f;
	Player->GunOffset.Z = 0.0f;

	// Set the view info
	Player->ViewFlags = VIEW_TYPE_ACTOR | VIEW_TYPE_YAW_ONLY | VIEW_TYPE_COLLIDEMODEL;
	Player->ViewIndex = ACTOR_INDEX_PLAYER;
	Player->MotionIndex = ACTOR_MOTION_PLAYER_RUN;
	Player->DammageFlags = DAMMAGE_TYPE_NORMAL | DAMMAGE_TYPE_RADIUS;	

	// Hook player up to client physics controller
	Player->ControlIndex = 0;
	Player->DFunc = Bot_Destroy;

	Bot_Create(VSI, Player);
	//Player->Control = Bot_MoveToPoint;
	Player->Control = Bot_Control;
	DBot = static_cast<Bot_Var*>(Player->userData);

	DBot->BotType = 1;

	Player->Health = 100;
	Player->Score = 0;

	// FIXME:  Soon, Scores, Health, etc won't be so arbitrary.  Maybe somthing like
	// Quake2's Layouts will be used instead, making it easier to make games that don't
	// use this sort of scoring system...
	//GenVSI_SetClientScore(VSI, Player->ClientHandle, Player->Score);
	//GenVSI_SetClientHealth(VSI, Player->ClientHandle, Player->Health);

	for (i=0; i< MAX_PLAYER_ITEMS; i++)
	{
		Player->Inventory[i] = 0;
		Player->InventoryHas[i] = GE_FALSE;
	}

	Player->CurrentWeapon = 0;
	
	//GenVSI_SetClientInventory(VSI, Player->ClientHandle, ITEM_GRENADES, 0, GE_FALSE);
	//GenVSI_SetClientInventory(VSI, Player->ClientHandle, ITEM_ROCKETS, 0, GE_FALSE);
	//GenVSI_SetClientInventory(VSI, Player->ClientHandle, ITEM_SHREDDER, 0, GE_FALSE);

	Player->NextWeaponTime = 0.0f;

	//return GE_TRUE;
}


//=====================================================================================
//	BlockActor_Trigger
//=====================================================================================
geBoolean BlockActor_Trigger(GenVSI *VSI, void *PlayerData, GPlayer *TargetData, void *Data)
{
	GPlayer *Player = (GPlayer*)PlayerData;

	//if (strcmp(Player->ClassName,"Bot_Actor") == 0)
		return(2);

	return GE_FALSE;
}


//=====================================================================================
//	BlockActor_Spawn
//=====================================================================================
void BlockActor_Spawn(GenVSI *VSI, void *PlayerData, void *ClassData, char *EntityName)
{
	BlockActor		*Ba;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	Player->Control = NULL;
	Player->Trigger = BlockActor_Trigger;
	Player->Blocked = NULL;
	Player->Time = 0.0f;
	Player->ViewFlags = VIEW_TYPE_NONE | VIEW_TYPE_TOUCH;

	if (ClassData == NULL)
		{
			GenVS_Error("BlockActor_Spawn: entity missing class data ('%s')\n",EntityName);
		}

	Ba = (BlockActor*)ClassData;

	geXForm3d_SetIdentity(&Player->XForm);
	geXForm3d_SetTranslation(&Player->XForm, Ba->Origin.X, Ba->Origin.Y, Ba->Origin.Z);

	Player->VPos = Player->XForm.Translation;

	GenVSI_RegisterPlayerModel(VSI, Player, Ba->Model);

	//return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
//
//	Bot Health/Weapon/Dist comparisons
//
//=====================================================================================
//=====================================================================================

//=====================================================================================
//	Bot_RankWeapons
//=====================================================================================
int32 Bot_RankWeapons(GenVSI *VSI, void *PlayerData)
{
	GPlayer *Player;
	int32 Strength = 0;
	int32 amt;

	Player = static_cast<GPlayer*>(PlayerData);

	if (Player->InventoryHas[ITEM_SHREDDER] && Player->Inventory[ITEM_SHREDDER] > 100)
		{
		amt = Player->Inventory[ITEM_SHREDDER];
		amt = min(amt, 100);
		Strength += (int32)(amt * (60.0f/100.0f));
		}

	if (Player->InventoryHas[ITEM_GRENADES] && Player->Inventory[ITEM_GRENADES] > 3)
		{
		amt = Player->Inventory[ITEM_GRENADES];
		amt = min(amt, 5);
		Strength += (int32)(amt * (15.0f/5.0f));
		}

	if (Player->InventoryHas[ITEM_ROCKETS] && Player->Inventory[ITEM_ROCKETS] > 4)
		{
		amt = Player->Inventory[ITEM_ROCKETS];
		amt = min(amt, 10);
		Strength += (int32)(amt * (35.0f/10.0f));
		}

	return Strength;
}

//=====================================================================================
//	Bot_CompareWeapons
//=====================================================================================
int32 Bot_CompareWeapons(GenVSI *VSI, void *PlayerData, void *TgtPlayerData)
	{
	int32 d1, d2;

	d1 = Bot_RankWeapons(VSI, PlayerData);
	d2 = Bot_RankWeapons(VSI, TgtPlayerData);

	return d1 - d2;
	}

//=====================================================================================
//	Bot_GetRangeIndex
//=====================================================================================
int32 Bot_GetRangeIndex(GenVSI *VSI, void *PlayerData)
	{
	int32 index;
	GPlayer			*Player;
	Bot_Var			*DBot;
	float			Dist;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	Dist = geVec3d_DistanceBetween(&Player->XForm.Translation, &DBot->TgtPlayer->XForm.Translation);

	if (Dist <= CLOSE_RANGE_DIST)
		index = CLOSE_RANGE;
	else
	if (Dist <= MED_RANGE_DIST)
		index = MED_RANGE;
	else
		index = LONG_RANGE;

	return index;
	}

//=====================================================================================
//	Bot_GetStrengthIndex
//=====================================================================================
int32 Bot_GetStrengthIndex(GenVSI *VSI, void *PlayerData)
	{
	int32 index;
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	DBot = static_cast<Bot_Var*>(Player->userData);

	// health assessment
    DBot->HealthDiff = Player->Health - DBot->TgtPlayer->Health;
	// weapon assessment
	DBot->WeaponDiff = Bot_CompareWeapons(VSI, Player, DBot->TgtPlayer);

	if (BOT_STRENGTH_HIGH(DBot->HealthDiff, DBot->WeaponDiff))
		index = 2;
	else
	if (BOT_STRENGTH_LOW(DBot->HealthDiff, DBot->WeaponDiff))
		index = 0;
	else
		index = 1;

	return index;
	}

//=====================================================================================
//	Bot_GetRank
//=====================================================================================
int32 Bot_GetRank(GenVSI *VSI, void *PlayerData)
    {
    int32 RNdx,SNdx,Rank;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;

	RNdx = Bot_GetRangeIndex(VSI, Player);
	SNdx = Bot_GetStrengthIndex(VSI, Player);
	Rank = RankTable[RNdx][SNdx];

    return Rank;
    }

//=====================================================================================
//=====================================================================================
//
//	Bot Track Related
//
//=====================================================================================
//=====================================================================================

//=====================================================================================
//	Bot_FindTrackToGoal
//=====================================================================================
Track *Bot_FindTrackToGoal(GenVSI *VSI, void *PlayerData, float Time)
    {
    Track *track;
    int32 *type;
    float ydiff;
	TrackData TrackInfo;

	geVec3d			Pos, TgtPos;
	GPlayer			*Player;
	Bot_Var			*DBot;

    static int32 Away[] =
        {
        TRACK_TYPE_UP,
        TRACK_TYPE_DOWN,
		TRACK_TYPE_ELEVATOR_UP,
		TRACK_TYPE_ELEVATOR_DOWN,
        TRACK_TYPE_TRAVERSE,
        TRACK_TYPE_TRAVERSE_ONEWAY,
        TRACK_TYPE_EXIT,
        TRACK_TYPE_TRAVERSE_DOOR,
		-1
        };

    static int32 PlayerAbove[] =
        {
        TRACK_TYPE_UP,
		TRACK_TYPE_ELEVATOR_UP,
        TRACK_TYPE_TRAVERSE,
        TRACK_TYPE_TRAVERSE_ONEWAY,
		-1
        };

    static int32 PlayerBelow[] =
        {
        TRACK_TYPE_DOWN,
		TRACK_TYPE_ELEVATOR_DOWN,
        TRACK_TYPE_TRAVERSE,
        TRACK_TYPE_TRAVERSE_ONEWAY,
		-1
        };

    static int32 PlayerOnLevel[] =
        {
        TRACK_TYPE_TRAVERSE,
        TRACK_TYPE_TRAVERSE_ONEWAY,
        TRACK_TYPE_EXIT,
        TRACK_TYPE_TRAVERSE_DOOR,
		-1
        };

    geWorld *World;

    World = GenVSI_GetWorld(VSI);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Pos = Player->XForm.Translation;
	TgtPos = DBot->GoalPos;

    ydiff = TgtPos.Y - Pos.Y;

	if (DBot->Dir == BOT_DIR_TOWARD)
		{
		if (fabs(ydiff) <= BOT_SEARCH_Y_RANGE)
			type = PlayerOnLevel;
		else
			{
			if (ydiff < 0.0f)
				type = PlayerBelow;
			else
				type = PlayerAbove;
			}
		}
	else
	if (DBot->Dir == BOT_DIR_AWAY)
		{
		type = Away;
		}
	if (DBot->Dir == BOT_DIR_NONE)
		{
		type = Away;
		}

	Track_ClearTrack(&TrackInfo);
	while (1)
	{
		track = Track_FindTrack(VSI, &Pos, &TgtPos, DBot->Dir, type, &TrackInfo);

		if (!track)
			return NULL;

		if (Bot_ValidateTrackPoints(VSI, Player, track))
		{
			// can't add the same track!
			if (StackTop(&DBot->TrackStack) == TrackInfo.TrackNdx)
				continue;

			DBot->TrackInfo = TrackInfo;
			StackPush(&DBot->TrackStack, DBot->TrackInfo.TrackNdx);
			DBot->PastFirstTrackPoint = GE_FALSE;

			return (track);
		}
	}

    return NULL;
    }

//=====================================================================================
//	Bot_ValidateTrackPoints
//=====================================================================================
geBoolean Bot_ValidateTrackPoints(GenVSI *VSI, void *PlayerData, Track* t)
{
	GPlayer *Player;
	TrackPt *tp;

	Player = (GPlayer*)PlayerData;

    for (tp = t->PointList; tp < &t->PointList[t->PointCount]; tp++)
	{
		switch (tp->Action)
			{
			case POINT_TYPE_WAIT_POINT_VISIBLE:
				break;

			case POINT_TYPE_LOOK_FOR_ITEMS:
				{
				GPlayer *HealthItem;
				GPlayer *WeaponItem;

				if (!(HealthItem = Bot_FindItem(VSI, tp->Pos, ItemHealthList, BOT_SEARCH_Y_RANGE, 1000.0f)))
					return GE_FALSE;

				if (!(WeaponItem = Bot_FindItem(VSI, tp->Pos, ItemWeaponList, BOT_SEARCH_Y_RANGE, 1000.0f)))
					return GE_FALSE;

				break;
				}

			case POINT_TYPE_ROCKET_JUMP:

				if (!Player->InventoryHas[ITEM_ROCKETS] || Player->Inventory[ITEM_ROCKETS] <= 0)
					return GE_FALSE;

				if (Player->Health < 35)
					return GE_FALSE;

				break;


			#if 0
			case POINT_TYPE_SHOOT_BLASTER:
			case POINT_TYPE_SHOOT_GRENADE:
			case POINT_TYPE_SHOOT_ROCKET:
			case POINT_TYPE_SHOOT_SHREDDER:
                {
                geVec3d ShootPos;
                geVec3d Pos;
				int32 wpn_num
				int32 NumShots = ThisPoint->ShootTimes;

				wpn_num = ThisPoint->ActionType - POINT_TYPE_SHOOT_BLASTER;

				if (wpn_num == POINT_TYPE_SHOOT_BLASTER) // unlimited ammo
					break;

				if (Player->Inventory[wpn_num] >= NumShots && Player->InventoryHas[wpn_num])
					break;

				return GE_FALSE;
			#endif
			}
	}

	return GE_TRUE;
}

//=====================================================================================
//	Bot_ValidateMultiTrackPoints
//=====================================================================================
geBoolean Bot_ValidateMultiTrackPoints(GenVSI *VSI, void *PlayerData, Track* t)
{
	GPlayer *Player;
	TrackPt *tp;

	Player = (GPlayer*)PlayerData;

    for (tp = t->PointList; tp < &t->PointList[t->PointCount]; tp++)
	{
		switch (tp->Action)
			{
			case POINT_TYPE_WAIT_FOR_PLAYER: // no hiding on multi-tracks
				return GE_FALSE;

			case POINT_TYPE_LOOK_FOR_ITEMS:
				return GE_FALSE;

			case POINT_TYPE_ROCKET_JUMP:

				if (!Player->InventoryHas[ITEM_ROCKETS] || Player->Inventory[ITEM_ROCKETS] <= 0)
					return GE_FALSE;
	
				if (Player->Health < 35)
					return GE_FALSE;

				break;

			}
	}

	return GE_TRUE;
}


//=====================================================================================
//	Bot_FindTrack
//=====================================================================================
Track *Bot_FindTrack(GenVSI *VSI, void *PlayerData, int32 TrackArr[])
    {
    Track *track;
	TrackData TrackInfo;
    
	geVec3d			Pos, TgtPos;
	GPlayer			*Player;
	Bot_Var			*DBot;

    geWorld *World;

    World = GenVSI_GetWorld(VSI);

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);

	Pos = Player->XForm.Translation;
	TgtPos = DBot->GoalPos;

	Track_ClearTrack(&TrackInfo);
	while (1)
		{
	    track = Track_FindTrack(VSI, &Pos, &TgtPos, 0, TrackArr, &TrackInfo);

		if (!track)
			break;

		if (Bot_ValidateTrackPoints(VSI, Player, track))
			{
			// can't add the same track!
			if (StackTop(&DBot->TrackStack) == TrackInfo.TrackNdx)
    			continue;

			DBot->TrackInfo = TrackInfo;
			StackPush(&DBot->TrackStack, DBot->TrackInfo.TrackNdx);
			DBot->PastFirstTrackPoint = GE_FALSE;
			return (track);
			}
		}

    return (NULL);
    }

geBoolean Bot_TrackAction(GenVSI *VSI, void *PlayerData, const TrackPt* ThisPoint, const TrackPt* NextPoint, float Time)
    {
	GPlayer			*Player;
    Bot_Var         *DBot;
    int32           DoAction;

    Player = static_cast<GPlayer*>(PlayerData);
    DBot = static_cast<Bot_Var*>(Player->userData);

    DoAction = ThisPoint->Action;

    // make sure your going the right direction
    if (ThisPoint->ActionDir != 0 && ThisPoint->ActionDir != Track_GetDir(&DBot->TrackInfo))
        DoAction = -1;

    switch (DoAction)
        {
        case POINT_TYPE_JUMP:
		    {
		    float sy;
		    float Scale;

		    ////if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: Jump");

            Player->Velocity.Y += BOT_JUMP_THRUST;

		    if (ThisPoint->VelocityScale > 0.0f)
			    Scale = ThisPoint->VelocityScale;
		    else
			    Scale = 0.7f;

		    DBot->RunSpeed *= Scale;

            sy = Player->Velocity.Y;
		    Player->Velocity = DBot->MoveVec;
		    geVec3d_Scale(&Player->Velocity, 600*Scale, &Player->Velocity);
		    Player->Velocity.Y = sy;

            DBot->Action = Bot_Jump;


		    return(GE_TRUE);
            //break;
		    }

        case POINT_TYPE_LOOK_FOR_ITEMS:
            {
            GPlayer *HealthItem;
            GPlayer *WeaponItem;

		    // if multi-tracks
            if (DBot->TrackStack.TOS >= 1)
                break;

		    ////if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: LookForItems");

            if (HealthItem = Bot_FindItem(VSI, &Player->XForm.Translation, ItemHealthList, 40.0f, 500.0f))
                {
                // No longer on a track
                Bot_ClearTrack(VSI, Player);

			    DBot->GoalPos = HealthItem->Pos;
			    Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
			    Bot_ModeChange(VSI, Player, MODE_GOAL_POINT, GE_FALSE, Time);
			    Bot_ShootFoot(VSI, Player, Time);
                return GE_TRUE;
                }

            if (WeaponItem = Bot_FindItem(VSI, &Player->XForm.Translation, ItemWeaponList, 40.0f, 500.0f))
                {
                Bot_ClearTrack(VSI, Player);
			    DBot->GoalPos = WeaponItem->Pos;
			    Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
			    Bot_ModeChange(VSI, Player, MODE_GOAL_POINT, GE_FALSE, Time);
			    Bot_ShootFoot(VSI, Player, Time);
                return GE_TRUE;
                }

            break;
            }

        case POINT_TYPE_ROCKET_JUMP:

		    ////if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: RocketJump");

		    if (!Bot_SetWeapon(VSI, Player, ITEM_ROCKETS))
			    {
			    Bot_ClearTrack(VSI, Player);
			    break;
			    }

            if (Player->State == PSTATE_Normal && NextPoint)
                {
                float sy,Scale;
                geVec3d ShootPos, ShootVec;

				//Bot_NudgePlayer(VSI, Player, ThisPoint->Pos, Time);
                //Player->XForm.Translation.X = Pos.X;
                //Player->XForm.Translation.Z = Pos.Z;

                Player->Velocity.Y += BOT_JUMP_THRUST;

			    if (ThisPoint->VelocityScale > 0.0f)
				    Scale = ThisPoint->VelocityScale;
			    else
				    Scale = 0.3f; //default

			    DBot->RunSpeed *= Scale;

                // reset Velocity
				sy = Player->Velocity.Y;
				Player->Velocity = DBot->MoveVec;
				geVec3d_Scale(&Player->Velocity, 500.0f, &Player->Velocity);
				Player->Velocity.Y = sy;

                // facing the next point
                // get the shooing position in the opposite direction
                geVec3d_Scale(&DBot->MoveVec, 60.0f, &ShootVec);
                geVec3d_Add(&Player->XForm.Translation, &ShootVec, &ShootPos);
                ShootPos.Y = Player->XForm.Translation.Y - 30.0f;

                // force a shot here
                Player->NextWeaponTime = GenVSI_GetTime(VSI) - 0.01f;
                Bot_Shoot(VSI, Player, &ShootPos, Time);
                //Bot_ClearTrack(VSI, Player);
                //Bot_FinishTrack(VSI, Player);

                DBot->Action = Bot_WeaponJump;
                return GE_TRUE;
                }
		    else
			    {
			    Bot_ClearTrack(VSI, Player);
			    }

            break;

        case POINT_TYPE_BLASTER_JUMP:

		    ////if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: BlasterJump");

            if (Player->State == PSTATE_Normal && NextPoint)
                {
                float sy,Scale;
                geVec3d ShootPos, ShootVec;

                //Player->XForm.Translation.X = Pos.X;
                //Player->XForm.Translation.Z = Pos.Z;
				//Bot_NudgePlayer(VSI, Player, ThisPoint->Pos, Time);

                Player->Velocity.Y += BOT_JUMP_THRUST;

			    if (ThisPoint->VelocityScale > 0.0f)
				    Scale = ThisPoint->VelocityScale;
			    else
				    Scale = 0.6f;

			    DBot->RunSpeed *= Scale;

                // reset Velocity
				sy = Player->Velocity.Y;
				Player->Velocity = DBot->MoveVec;
				geVec3d_Scale(&Player->Velocity, 500.0f, &Player->Velocity);
				Player->Velocity.Y = sy;

                // facing the next point
                geVec3d_Scale(&DBot->MoveVec, 60.0f, &ShootVec);
                geVec3d_Add(&Player->XForm.Translation, &ShootVec, &ShootPos);
                ShootPos.Y = Player->XForm.Translation.Y - 30.0f;

                // force a shot here
                Player->NextWeaponTime = GenVSI_GetTime(VSI) - 0.01f;
			    Bot_SetWeapon(VSI, Player, ITEM_BLASTER);
                Bot_Shoot(VSI, Player, &ShootPos, Time);
                //Bot_ClearTrack(VSI, Player);
                //Bot_FinishTrack(VSI, Player);

                DBot->Action = Bot_WeaponJump;
                return GE_TRUE;
                }
		    else
			    {
			    Bot_ClearTrack(VSI, Player);
			    }

            break;

	    case POINT_TYPE_SHOOT_BLASTER:
	    case POINT_TYPE_SHOOT_GRENADE:
	    case POINT_TYPE_SHOOT_ROCKET:
	    case POINT_TYPE_SHOOT_SHREDDER:
            {
		    int32 wpn_num;
		    //int32 NumShots = ThisPoint->ShootTimes;

		    //if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: Shoot");

		    wpn_num = ThisPoint->Action - POINT_TYPE_SHOOT_BLASTER;
		    // track should be validated but test again for good measure

		    if (Player->Inventory[wpn_num] <= 0 || !Player->InventoryHas[wpn_num])
		    {
			    Bot_ClearTrack(VSI, Player);
			    return GE_TRUE;
		    }

		    assert(NextPoint);

		    Bot_InitShootPoint(VSI, Player, Time);
		    // still on track at this point
		    return GE_TRUE;
            }

        case POINT_TYPE_WAIT_FOR_PLAYER:
		    {
		    geWorld			*World;
		    World = GenVSI_GetWorld(VSI);

		    //if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: WaitForPlayer");

		    // if multi-tracks
            if (DBot->TrackStack.TOS >= 1)
                break;

		    if (!Bot_CanSeePlayerToPlayer(World, &Player->XForm.Translation, &DBot->TgtPlayer->XForm.Translation))
		    {
                geVec3d Pos = *ThisPoint->Pos;

                Player->XForm.Translation.X = Pos.X;
                Player->XForm.Translation.Z = Pos.Z;

			    // Set timeout
			    if (ThisPoint->Time > 0.0f)
				    DBot->TimeOut = GenVSI_GetTime(VSI) + ThisPoint->Time;
			    else
				    DBot->TimeOut = GenVSI_GetTime(VSI) + 20.0f;
				    
			    Bot_InitWaitForPlayer(VSI, Player, Time);
			    // still on track at this point
			    return GE_TRUE;
		    }

		    break;
		    }

        case POINT_TYPE_WAIT_POINT_VISIBLE:
		    {
            geVec3d Pos = *ThisPoint->Pos;

		    //if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: WaitPointVisible");

			// nudge to point
			geVec3d_Subtract(&Pos, &Player->XForm.Translation, &Player->Velocity);
			Player->Velocity.Y = 0.0f;
			PlayerPhysics(VSI, Player, PLAYER_GROUND_FRICTION, PLAYER_AIR_FRICTION, PLAYER_LIQUID_FRICTION, PLAYER_GRAVITY, 1.0f, GE_FALSE, Time);
            //Player->XForm.Translation.X = Pos.X;
            //Player->XForm.Translation.Z = Pos.Z;
		    // Set timeout
			
			if (ThisPoint->Time > 0.0f)
			    DBot->TimeOut = GenVSI_GetTime(VSI) + ThisPoint->Time;
		    else
			    DBot->TimeOut = GenVSI_GetTime(VSI) + 5.0f;

		    geVec3d_Clear(&Player->Velocity);

		    Bot_InitWaitForEntityVisible(VSI, Player, Time);
		    // still on track at this point
		    return GE_TRUE;
		    }

        case POINT_TYPE_WAIT_POINT_DIST:
		    {
		    //if (BotDebugPrint) GenVSI_ConsoleHeaderPrintf(VSI, DBot->TgtPlayer->ClientHandle, GE_TRUE, "Point: WaitPointDist");

		    // Set timeout
		    if (ThisPoint->Time > 0.0f)
			    DBot->TimeOut = GenVSI_GetTime(VSI) + ThisPoint->Time;
		    else
			    DBot->TimeOut = GenVSI_GetTime(VSI) + 5.0f;

		    Bot_InitWaitForEntityDist(VSI, Player, Time);
		    // still on track at this point
		    return GE_TRUE;
		    }
        }

    return GE_FALSE;
    }

//=====================================================================================
//	Bot_LeaveTrack
//=====================================================================================
geBoolean Bot_LeaveTrack(GenVSI *VSI, void *PlayerData, float Time)
{
	Bot_Var			*DBot;
	GPlayer			*Player;
	float		dist;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

    // run by when retreating
	if (DBot->Mode == MODE_RETREAT_ON_TRACK || DBot->Mode == MODE_RETREAT)
		return GE_FALSE;

    // don't leave the track when in the air
	if (Player->State == PSTATE_InAir)
		return GE_FALSE;

    dist = geVec3d_DistanceBetween(&Player->XForm.Translation, &DBot->TgtPlayer->XForm.Translation);

	if ((DBot->Mode == MODE_FIND_PLAYER || DBot->Mode == MODE_FIND_PLAYER_QUICK) &&
        dist < 500.0f && DBot->TimeSeen > 0.0f)
		{
        Bot_ClearTrack(VSI, Player);
		return GE_TRUE;
		}

	if (dist < 150.0f && DBot->TimeSeen > 0.0f)
		{
        Bot_ClearTrack(VSI, Player);
		return GE_TRUE;
		}

	return GE_FALSE;
}


//=====================================================================================
//	Bot_FinishTrack - finishing a track should allow movement to the next track
//=====================================================================================
geBoolean Bot_FinishTrack(GenVSI *VSI, void *PlayerData)
	{
	Bot_Var			*DBot;
	GPlayer			*Player;
	int32			NewTrack;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	Track_ClearTrack(&DBot->TrackInfo);

	StackPop(&DBot->TrackStack);
	if (!StackIsEmpty(&DBot->TrackStack))
		{
		NewTrack = StackTop(&DBot->TrackStack);
		Track_NextMultiTrack(VSI, &Player->XForm.Translation, NewTrack, &DBot->TrackInfo);
		DBot->PastFirstTrackPoint = GE_FALSE;
		// now on track
		// set goal to next point
		DBot->GoalPos = *Track_GetPoint(&DBot->TrackInfo)->Pos;
		Bot_InitMoveToPoint(VSI, Player, &DBot->GoalPos);
		}
    else 
        {
        DBot->GoalPos = DBot->TgtPlayer->XForm.Translation;
        }

	return GE_TRUE;
	}

//=====================================================================================
//	Bot_ClearTrack - clearing a track should clear ALL including stack
//=====================================================================================
geBoolean Bot_ClearTrack(GenVSI *VSI, void *PlayerData)
	{
	Bot_Var			*DBot;
	GPlayer			*Player;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	Track_ClearTrack(&DBot->TrackInfoPrev);
	Track_ClearTrack(&DBot->TrackInfo);
    StackReset(&DBot->TrackStack);

	return GE_TRUE;
	}



//=====================================================================================
//=====================================================================================
//
//	Misc Bot Junk
//
//=====================================================================================
//=====================================================================================

//=====================================================================================
//	Bot_Keys - debug mostly
//=====================================================================================
geBoolean Bot_Keys(GenVSI *VSI, void *PlayerData, float Time)
	{
	GPlayer			*Player;
	Bot_Var			*DBot;

	Player = (GPlayer*)PlayerData;
	assert(Player);
	DBot = static_cast<Bot_Var*>(Player->userData);
	assert(DBot);

	if (GetAsyncKeyState('O') & 0x8000)
		{
		static geBoolean Mode = GE_FALSE;
		if (Mode)
			GenVSI_SetViewPlayer(VSI, DBot->ClientPlayer->ClientHandle, Player);
		else
			GenVSI_SetViewPlayer(VSI, DBot->ClientPlayer->ClientHandle, DBot->TgtPlayer);
		Mode = !Mode;
		}

	if (GetAsyncKeyState('I') & 0x8000)
		{
		GodMode = !GodMode;
		GenVSI_ConsoleHeaderPrintf(VSI, DBot->ClientPlayer->ClientHandle, GE_TRUE, "God Mode %d", GodMode);
		}

	if (GetAsyncKeyState('P') & 0x8000)
		{
		PathLight = !PathLight;
		GenVSI_ConsoleHeaderPrintf(VSI, DBot->ClientPlayer->ClientHandle, GE_TRUE, "Path Light %d", PathLight);
		}

	if (GetAsyncKeyState('N') & 0x8000)
		{
		MultiPathLight = !MultiPathLight;
		GenVSI_ConsoleHeaderPrintf(VSI, DBot->ClientPlayer->ClientHandle, GE_TRUE, "Multi-Path Light %d", MultiPathLight);
		}

	if (GetAsyncKeyState('D') & 0x8000)
		{
		BotDebugPrint = !BotDebugPrint;
		GenVSI_ConsoleHeaderPrintf(VSI, DBot->ClientPlayer->ClientHandle, GE_TRUE, "Debug Print %d", BotDebugPrint);
		}

	return GE_TRUE;
	}

