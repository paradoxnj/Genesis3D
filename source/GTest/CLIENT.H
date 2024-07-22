/****************************************************************************************/
/*  Client.h                                                                            */
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
#ifndef CLIENT_H
#define CLIENT_H

//#include <Windows.h>
#include <stdio.h>

#include "VidMode.h"

#include "GENESIS.H"
#include "Errorlog.h"
#include "Quatern.h"
#include "RAM.H"

#include "Game\Game.h"
#include "Game\GPlayer.h"
#include "Game\GenVSI.h"

#include "GameMgr.h"
#include "NetMgr.h"

#ifdef __cplusplus
extern "C" {
#endif

//================================================================================================
//================================================================================================
typedef struct	Client_Client		Client_Client;

#define CLIENT_MAX_PROC_INDEX			512
#define CLIENT_MAX_TEMP_PLAYERS			10

#define CLIENT_DEMO_NONE				0
#define CLIENT_DEMO_PLAY				1
#define CLIENT_DEMO_RECORD				2

typedef enum
{
	ClientMode_Invalid = 0,
	ClientMode_Dumb,						// Client does nothing but what server says to do...(a local client with a server)
	ClientMode_Proxy,						// Client predicts while waiting for server updates (a client without a server)
} Client_Mode;

//================================================================================================
//================================================================================================
#define CLIENT_MAX_MOVE_STACK	256

typedef struct Client_Move
{
	float			Time;
	float			Delta;
	float			ForwardSpeed;
	float			Pitch;
	float			Yaw;
	geVec3d			Pos;
	uint16			ButtonBits;
	uint16			CurrentWeapon;

	struct Client_Move	*Next;
} Client_Move;

// Info about the clients connected to the server
typedef struct Client_ClientInfo
{
	geBoolean	Active;			// == GE_TRUE if this client is active on the server
	char		Name[64];
	int32		Score;
	int32		Health;

	int32		OldScore;
	int32		OldHealth;

} Client_ClientInfo;

typedef enum
{
	STATE_Disable,
	STATE_Normal,
	STATE_Selected,
	STATE_Warning,
} Client_StatusState;

// 6 menu elements, with 3 states (normal/selected/inactive)
typedef struct Client_StatusElement
{
	geBitmap			*Bitmaps[4];
	int32				XPos;
	int32				YPos;
	int32				Print1XPos;
	int32				Print1YPos;
	int32				Low;						// Display warning, when amount gets lower than this number
	Client_StatusState	State;

} Client_StatusElement;

typedef struct Client_StatusBar
{
	Client_StatusElement	Elements[6];

} Client_StatusBar;

#define CLIENT_MAX_DEMOS			10
#define CLIENT_MAX_DEMO_NAME_SIZE	64

typedef struct
{
	int32		Mode;

	int32		NumDemos;	
	char		DemoNames[CLIENT_MAX_DEMOS][CLIENT_MAX_DEMO_NAME_SIZE];
	int32		CurrentDemo;

	FILE		*File;							// Current demo file opened (only one at a time)

	geBoolean	OriginalSpeed;					// Demo should run at original speed
} Client_Demo;

typedef struct Client_Client
{
	Client_Client		*Self1;
	HWND				hWnd;

	VidMode				VidMode;
	Client_Mode			Mode;
	geEngine			*Engine;
	NetMgr				*NMgr;
	GameMgr				*GMgr;

	Client_StatusBar	StatusBar;

	NetMgr_NetState		NetState;
	geBoolean			MultiPlayer;			// == GE_TEUE if the client is in a multi player game

	geCSNetMgr_NetClient	geClient;			// Info about us as a client (NetId, Name, etc...)

	int32				ViewPlayer;
	geXForm3d			ViewXForm;

	float				Time;
	float				Ping;						// Current ping info from server
	float				NetTime;
	geBoolean			NetTimeGood;				// Last net time was recent...
	float				OldNetTime;
	float				TempTime;
	float				ServerPingBack;				// Time stamp from server, -1 if no time from server this frame

	GenVSI_CMove		Move;						// Current move intentions

	float				ForwardSpeed;				// Forward/Back speed
	float				UpwardSpeed;				// Forward/Back speed
	geVec3d				Angles;						// Orientation
	uint16				ButtonBits;					// Current pressed buttons...

	Client_Move			*LastMove;
	Client_Move			*MoveStack;

	GPlayer				Players[NETMGR_MAX_PLAYERS];
	GPlayer				TempPlayers[CLIENT_MAX_TEMP_PLAYERS];		// For pre-prediction
	Fx_Player			TempFxPlayers[CLIENT_MAX_TEMP_PLAYERS];

	// Fx Players NOTE - That Players index into FxPlayers using there index nbumber (Player - Client->Players)
	Fx_Player			FxPlayers[NETMGR_MAX_PLAYERS];

	// Proc adresses
	void				*ProcIndex[CLIENT_MAX_PROC_INDEX];

	// Scores, Health, etc about all clients connected from server
	Client_ClientInfo	ClientInfo[NETMGR_MAX_CLIENTS];
	int32				ClientIndex;	// Index in list above where our info is stored...

	uint16				Inventory[MAX_PLAYER_ITEMS];
	uint16				CurrentWeapon;
	uint16				OldWeapon;

	uint16				OldInventory[MAX_PLAYER_ITEMS];

	//Server_Client		ServerClient;
	int32				ClientPlayer;

	Client_Demo			Demo;					// Demo related info

	float				NextSend;

	GenVSI				GenVSI;

	Client_Client		*Self2;
} Client_Client;


//================================================================================================
//================================================================================================
Client_Client		*Client_Create(	geEngine	*Engine, 
									Client_Mode Mode, 
									GameMgr		*GMgr, 
									NetMgr		*NMgr, 
									VidMode		VidMode,
									int32		DemoMode,
									const char	*DemoName,
									geBoolean	MultiPlayer);

void				Client_Destroy(Client_Client *Client);
geBoolean			Client_IsValid(const Client_Client *Client);
geBoolean			Client_CreateStatusBar(Client_Client *Client, VidMode VidMode);
void				Client_DestroyStatusBar(Client_Client *Client);
void				Client_DestroyALLPlayers(Client_Client *Client);
void				Client_FreeALLResources(Client_Client *Client);
void				Client_FreeResourcesForNewWorld(Client_Client *Client);
void				Client_NewWorldDefaults(Client_Client *Client);
geBoolean			Client_Frame(Client_Client *Client, float Time);
geBoolean			Client_RenderFrame(Client_Client *Client, float Time);
					
Client_Move			*Client_PeekMove(Client_Client *Client);
void				Client_RemoveFirstMove(Client_Client *Client);

geBoolean			Client_SendMove(Client_Client *Client, float Time);

geBoolean			Client_UpdateSinglePlayer(Client_Client *Client, GPlayer *Player, float Ratio, float Time, geBoolean TempPlayer);
GPlayer				*Client_ParsePlayerData(Client_Client *Client, Buffer_Data *Buffer, GPlayer *FakePlayer);

geBoolean			Client_ChangeNetState(Client_Client *Client, NetMgr_NetState NewNetState);

geBoolean			Client_SetDemo(Client_Client *Client, int32 DemoNum);
geBoolean			Client_ReadServerMessages(Client_Client *Client, Buffer_Data *Buffer);
geBoolean			Client_SetupDemos(Client_Client *Client, int32 Mode, const char *DemoFile);
void				Client_RefreshStatusBar(int32 NumPages);

void				Client_DestroyTempPlayer(Client_Client *Client, GPlayer *Player);
GPlayer				*Client_GetPlayer(Client_Client *Client, int32 Index);

#ifdef __cplusplus
}
#endif

#endif