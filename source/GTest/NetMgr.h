/****************************************************************************************/
/*  NetMgr.h                                                                            */
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
#ifndef NETMGR_H
#define NETMGR_H

#include <Windows.h>

#include "GENESIS.H"

#include "Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif


//===========================================================================
//	Struct defs
//===========================================================================
typedef	struct NetMgr						NetMgr;

#define NETMGR_VERSION_MAJOR				1	
#define NETMGR_VERSION_MINOR				0

// Upper bounds for both client/server	(they must share the same number of players)
#define NETMGR_MAX_CLIENTS					8

#define NETMGR_MAX_PLAYERS					512

#define NETMGR_MAX_IP_ADDRESS				128

#define NETMGR_SINGLE_PLAYER_NETID			0xff		// For single player games
#define NETMGR_SPECIAL_BOT_NETID			-2			// For bots

#define NETMGR_LOCAL_MSG_BUFFER_SIZE		20000

//===========================================================================
//	NetState
//===========================================================================
typedef	int32								NetMgr_NetState;

#define	NetState_Disconnected 				0	// The client is currently not connected to a server
#define	NetState_Connecting					1	// The client is not yet fully connected...
#define	NetState_ConnectedIdle				2	// The client is connected, but doing nothing
#define	NetState_WorldChange				3	// The client is in the middle of a world change
#define	NetState_WorldActive				4	// A world is running, and receiving data

#define NETMGR_MAX_NET_STATES				5

//===========================================================================
//	Message types
//===========================================================================
// Client to Server msg's
#define	NETMGR_MSG_CLIENT_MOVE				1
#define NETMGR_MSG_CLIENT_CONFIRM			2

// Server to Client msg's
#define NETMGR_MSG_VERSION					3
#define NETMGR_MSG_TIME						4
#define NETMGR_MSG_PLAYER_DATA				5
#define NETMGR_MSG_NEW_WORLD_PLAYER_DATA	6
#define NETMGR_MSG_VIEW_PLAYER				7
#define NETMGR_MSG_MESH_INDEX				8
#define NETMGR_MSG_ACTOR_INDEX				9
#define NETMGR_MSG_MOTION_INDEX				10
#define NETMGR_MSG_BONE_INDEX				11
#define NETMGR_MSG_TEXTURE_INDEX			12
#define NETMGR_MSG_SOUND_INDEX				13
#define NETMGR_MSG_SET_WORLD				14
#define NETMGR_MSG_CD_TRACK					15
#define NETMGR_MSG_PLAY_SOUND_INDEX			16
#define NETMGR_MSG_EFFECT					17
#define NETMGR_MSG_CLIENT_INDEX				18
#define NETMGR_MSG_CLIENT_ACTIVE			19
#define NETMGR_MSG_CLIENT_NAME				20
#define NETMGR_MSG_CLIENT_SCORE				21
#define NETMGR_MSG_CLIENT_HEALTH			22
#define NETMGR_MSG_CLIENT_INVENTORY			23
#define NETMGR_MSG_CLIENT_WEAPON			24
#define NETMGR_MSG_SPAWN_FX					25
#define NETMGR_MSG_HEADER_PRINTF			26
#define NETMGR_MSG_CLIENT_PLAYER_INDEX		27
#define NETMGR_MSG_NET_STATE_CHANGE			28

#define NETMGR_MSG_SHUTDOWN					128

// Flags for what needs to be sent/read for player updates
#define NETMGR_SEND_SPAWN_TIME				(1<<0)			// Must Send/Read Sapwn Time
#define NETMGR_SEND_VIEW_FLAGS				(1<<1)			// Must Send/Read view flags
#define NETMGR_SEND_VIEW_INDEX				(1<<2)			// Must Send/Read view index
#define NETMGR_SEND_MOTION_INDEX			(1<<3)			// Must Send/Read motion index
#define NETMGR_SEND_FX_FLAGS				(1<<4)			// Must Send/Read fx flags
#define NETMGR_SEND_POS						(1<<5)			// Must Send/Read Pos
#define NETMGR_SEND_ANGLES					(1<<6)			// Must Send/Read Angles
#define NETMGR_SEND_FRAME_TIME				(1<<7)			// Must Send/Read FrameTime
#define NETMGR_SEND_SCALE					(1<<8)			// Must Send/Read scale
#define NETMGR_SEND_VELOCITY				(1<<9)			// Must Send/Read Velocity in player
#define NETMGR_SEND_STATE					(1<<10)			// Must Send/Read State variable
#define NETMGR_SEND_CONTROL_INDEX			(1<<11)			// Means of connecting functions accross net
#define NETMGR_SEND_TRIGGER_INDEX			(1<<12)			// Means of connecting functions accross net
#define NETMGR_SEND_MINS_MAXS				(1<<13)			// Must Send/Receive mins/maxs
//===========================================================================
//	Function prototypes
//===========================================================================
NetMgr				*NetMgr_Create(geBoolean UseLocalBuffers);
void				NetMgr_Destroy(NetMgr *NMgr);
void				NetMgr_FreeAllObjects(NetMgr *NMgr);

geBoolean			NetMgr_StartSession(NetMgr *NMgr, const char *SessionName, const char *PlayerName);
geBoolean			NetMgr_JoinSession(NetMgr *NMgr, const char *IPAddress, const char *PlayerName);
geCSNetMgr_NetID	NetMgr_GetOurID(NetMgr *NMgr);

geBoolean			NetMgr_IsValid(NetMgr *NMgr);

geBoolean			NetMgr_SendServerMessage(NetMgr *NMgr, Buffer_Data *Buffer, geBoolean G);
geBoolean			NetMgr_SendClientMessage(NetMgr *NMgr, geCSNetMgr_NetID NetID, Buffer_Data *Buffer, geBoolean G);
geBoolean			NetMgr_ReceiveServerMessage(NetMgr *NMgr, geCSNetMgr_NetMsgType *Type, Buffer_Data *Buffer);
geBoolean			NetMgr_ReceiveClientMessage(NetMgr *NMgr, geCSNetMgr_NetMsgType *MsgType, geCSNetMgr_NetID *ClientID, Buffer_Data *Buffer);

void				NetMgr_ResetClientBuffer(NetMgr *NMgr);
void				NetMgr_ResetServerBuffer(NetMgr *NMgr);

#ifdef __cplusplus
}
#endif

#endif