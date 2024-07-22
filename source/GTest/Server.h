/****************************************************************************************/
/*  Server.h                                                                            */
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
#ifndef SERVER_H
#define SERVER_H

#include <Windows.h>

#include "GENESIS.H"
#include "Errorlog.h"
#include "RAM.H"

#include "Game\Game.h"
#include "Game\GPlayer.h"
#include "Game\GenVSI.h"

#include "Client.h"
#include "GameMgr.h"
#include "NetMgr.h"
#include "Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

//===========================================================================
//	Struct defs
//===========================================================================
typedef struct Server_Server	Server_Server;

#define MAX_SERVER_CLASS_SPAWNS		256
#define MAX_CLASS_NAME_STRING		64

#define SERVER_MAX_PROC_INDEX		512

typedef struct Server_CSpawn
{
	char				Name[MAX_CLASS_NAME_STRING];
	GenVSI_SpawnFunc	*Func;
	GenVSI_DestroyFunc	*DFunc;

} Server_CSpawn;

typedef struct
{
	int32			NumSpawnTime;
	int32			NumViewFlags;
	int32			NumViewIndex;
	int32			NumMotionIndex;
	int32			NumFxFlags;
	int32			NumPos;
	int32			NumAngles;
	int32			NumFrameTime;
	int32			NumScale;
	int32			NumVelocity;
	int32			NumState;
	int32			NumControlIndex;
	int32			NumTriggerIndex;
	int32			NumMinsMaxs;

	int32			NumBytesToSend;
	int32			TotalBytesSent;
} Server_NetStat;

typedef geBoolean Server_Control(Server_Server *Server, GPlayer *Player, float Time);
typedef geBoolean Server_Trigger(Server_Server *Server, GPlayer *Player, GPlayer *Target);
typedef geBoolean Server_Blocked(Server_Server *Server, GPlayer *Player, GPlayer *Target);

typedef struct Server_Client
{
	geBoolean			Active;

	geBoolean			Spawned;						// GE_TRUE if this player has been spawned for the current world...

	geCSNetMgr_NetID	NetID;

	NetMgr_NetState		NetState;						// Current net state (Connected, etc)
	geBoolean			NetStateConfirmed[NETMGR_MAX_NET_STATES];

	GPlayer				*Player;						// Player for this client

	// Data sent by client for intended move
	float				OldMoveTime;

	float				MoveTime;
	float				ForwardSpeed;					// Forward/Back speed
	geVec3d				Angles;							// Orientation
	uint16				ButtonBits;						// Buttons currently pressed for this frame

	geVec3d				Pos;							// Where the client currently thinks he is...

	GenVSI_CMove		Move;

	// Some proprietary client info
	char				Name[64];					
	int32				Score;
	int32				Health;

	int32				NumPings;
	float				Pings[10];
	float				Ping;

	geBoolean			InventoryHas[MAX_PLAYER_ITEMS];	// If player has item
	uint16				Inventory[MAX_PLAYER_ITEMS];	// Amount of item

	int16				CurrentWeapon;					
	float				NextWeaponTime;

	geVec3d				GunOffset;

	// Bit set for each variable that needs to be sent over net for this client
	// Up to 16 are allowed now...
	uint16				SendFlags[NETMGR_MAX_PLAYERS];	
	float				NextUpdate;			// Next time this client needs an update
} Server_Client;

typedef struct Server_Server
{
	Client_Client	*Client;

	GenVSI			GenVSI;					// Genesis Vertual System Interface object (GenVSI)
	GameMgr			*GMgr;					// Game Manager object (GameMgr)
	NetMgr			*NMgr;					// NetMgr

	float			NextUpdate;

	Server_Client	Clients[NETMGR_MAX_CLIENTS];
	GPlayer			SvPlayers[NETMGR_MAX_PLAYERS];

	// Proc adresses
	void			*ProcIndex[SERVER_MAX_PROC_INDEX];

	int32			NumTotalPlayers;

	int32			NumClassSpawns;
	Server_CSpawn	ClassSpawns[MAX_SERVER_CLASS_SPAWNS];

	// NOTE - The host a	llways loads the world, the server just keeps track of it...
	char					WorldName[128];					// Current level name
	geBoolean				ChangeWorldRequest;				// == GE_TRUE if clients need update
	GenVSI_NewWorldCB		*NewWorldCB;
	GenVSI_ShutdownWorldCB	*ShutdownWorldCB1;				// Current world to be freed
	GenVSI_ShutdownWorldCB	*ShutdownWorldCB2;				// World to be freed on request

	int32			ViewPlayer;							 

	Server_NetStat	NetStats;

} Server_Server;

//===========================================================================
//	Function prototypes
//===========================================================================

Server_Server	*Server_Create(GameMgr *GMgr, NetMgr *NMgr, Client_Client *Client, const char *LevelHack);
void			Server_Destroy(Server_Server *Server);
geBoolean		Server_SetupClientWithCurrentWorld(void *S, Server_Client *Client);
geBoolean		Server_SetupAllClientsWithCurrentWorld(void *S);
geBoolean		Server_SendClientStartupData(void *S, Server_Client *Client);
geBoolean		Server_SendClientCurrentWorldData(void *S, Server_Client *Client);
geBoolean		Server_ClientConnect(void *S, const geCSNetMgr_NetClient *Client);
Server_Client	*Server_BotConnect(void *S, const char *BotName);
geBoolean		Server_ClientDisconnect(void *S, geCSNetMgr_NetID Id, const char *Name);
geBoolean		Server_ClientDisconnectByHandle(void *S, GenVSI_CHandle);
void			*Server_CreatePlayer2(void *S, const char *ClassName, int32 Mode);
void			*Server_CreatePlayer(void *S, const char *ClassName);
void			Server_DestroyPlayer(void *S, void *P);
geBoolean		Server_FreeWorldData(Server_Server *Server);
geBoolean		Server_NewWorldDefaults(Server_Server *Server);
geBoolean		Server_SpawnWorld(Server_Server *Server);
geBoolean		Server_StartupWorld(Server_Server *Server);
geBoolean		Server_Frame(Server_Server *Server, GameMgr *GMgr, float Time);

void			Server_ProcessClientMove(	Server_Server *Server, 
											Server_Client *Client, 
											float ForwardSpeed, 
											float Pitch,
											float Yaw,
											uint16 ButtonBits, 
											uint16 Weapon,
											float Time);

#ifdef __cplusplus
}
#endif

#endif