#ifndef	TRACK_H
#define	TRACK_H

#include	"GENESIS.H"
#include    "bit.h"
#include	"genvsi.h"

// For ease of development these are set up as static sized arrays 
// Can be changed later if needed 

#define MAX_TRACKS 100
#define MAX_TRACK_POINTS 30
#define MAX_TRACK_VIS 20

typedef struct Track Track;

typedef struct 
    {
	geVec3d *Pos;
    geVec3d *WatchPos;
    int32 Action;
    int32 ShootTimes;
    int32 ActionDir;
	int32 Flags;
	float		Time;
	float		Dist;
	float		VelocityScale;
    }TrackPt;

typedef struct Track
    {
    TrackPt			PointList[MAX_TRACK_POINTS];
    int32			PointCount;
	int32   		Type;
    int32			Flags;
	Track*          Vis[MAX_TRACK_VIS+1];
	int32			VisFlag[MAX_TRACK_VIS+1];
	}Track;
    

typedef struct
    {
    int32           TrackNdx;
    int32			PointNdx;
	int32   		TrackDir;
    }TrackData;

typedef struct TrackCB
	{
	geBoolean (*CB)(GenVSI *VSI, void *Data, Track *Track);
	void *Data;
	}TrackCB;

extern int32 TrackCount;
extern Track TrackList[];

geBoolean Track_OnTrack(TrackData *td);
void Track_ClearTrack(TrackData *td);
Track *Track_GetTrack(TrackData *td);
int32 Track_GetDir(TrackData *td);
geBoolean Track_PastStartPoint(TrackData *td);
TrackPt *Track_NextPoint(TrackData *td);
TrackPt *Track_PrevPoint(TrackData *td);
TrackPt *Track_GetPoint(TrackData *td);
float Track_Length(Track* t);
TrackPt* Track_GetEndPoint(TrackData *td);
TrackPt* Track_GetFirstPoint(TrackData *td);
geBoolean Track_IsOneWay(Track *t);
Track* Track_FindTrack(GenVSI *VSI, geVec3d *StartPos, geVec3d *TgtPos, int32 player_dir, int32 *track_type, TrackData *td);
Track* Track_FindFarTrack(GenVSI *VSI, geVec3d *StartPos);
geBoolean Track_FindMultiTrack(GenVSI *VSI, geVec3d *StartPos, geVec3d *EndPos, int32 dir, TrackCB*, Stack *nodestack);
geBoolean Track_NextMultiTrack(GenVSI *VSI, geVec3d *StartPos, int32 TrackNdx, TrackData *td);
void Track_LinkTracks(geWorld *World);
Track* Track_GetNextTrack(Track *t);

//=====================================================================================
// TRACK TYPES
//=====================================================================================

#define TRACK_TYPE_UP				1	// Gets bot higher
#define TRACK_TYPE_DOWN				2	// Gets bot lower
#define TRACK_TYPE_ELEVATOR_UP		3	// 
#define TRACK_TYPE_ELEVATOR_DOWN	4	// Shortcut - just fall down it
#define TRACK_TYPE_MOVING_PLAT		5	// 
#define TRACK_TYPE_SCAN				6	// Run to this point and look for something else
#define TRACK_TYPE_HIDE				7	// Move to this point and wait for player
#define TRACK_TYPE_EXIT				8	// There is an exit here
#define TRACK_TYPE_SCAN_HEALTH		9	// Bot can see health from here
#define TRACK_TYPE_TRAVERSE			10	// Traverse a obsticles
#define TRACK_TYPE_SCAN_LOOP		11	// Run around in this loop to search for player
#define TRACK_TYPE_SCAN_WEAPON_AMMO	12	// Bot can see weapons or ammo from here
#define TRACK_TYPE_TRAVERSE_DOOR	13	// Bot can see weapons or ammo from here
#define TRACK_TYPE_TRAVERSE_ONEWAY  14	// Bot can see weapons or ammo from here

#define TRACK_FLAG_LOOP             BIT(0)

//=====================================================================================
//POINT TYPES
//=====================================================================================

#define POINT_TYPE_JUMP						1
#define POINT_TYPE_WAIT_POINT_DIST	        2
#define POINT_TYPE_ROCKET_JUMP				3
#define POINT_TYPE_WAIT_FOR_PLAYER			4		//
#define POINT_TYPE_WAIT_POINT_VISIBLE	    5		//doors
#define POINT_TYPE_LOOK_FOR_ITEMS			6
#define POINT_TYPE_BLASTER_JUMP				7

#define POINT_TYPE_SHOOT_BLASTER    		20
#define POINT_TYPE_SHOOT_GRENADE    		21
#define POINT_TYPE_SHOOT_ROCKET	    		22
#define POINT_TYPE_SHOOT_SHREDDER    		23

#endif
