/****************************************************************************************/
/*  Host.h                                                                              */
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
#ifndef __HOST_H__
#define __HOST_H__

#include <Windows.h>
#include <stdio.h>

#include "Genesis.h"
#include "Errorlog.h"
#include "Ram.h"

#include "GameMgr.h"
#include "NetMgr.h"
#include "Buffer.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct Server_Server	Server_Server;
typedef struct Client_Client	Client_Client;

//===========================================================================
//	Struct defs
//===========================================================================
#define HOST_DEMO_NONE				0
#define HOST_DEMO_PLAY				1
#define HOST_DEMO_RECORD			2

#define HOST_MODE_SINGLE_PLAYER		1		// Single player
#define HOST_MODE_SERVER_CLIENT		2		// Server with local client
#define HOST_MODE_SERVER_DEDICATED	3		// Server dedicated (no client)
#define HOST_MODE_CLIENT			4		// Client only...

#define HOST_MAX_DEMOS			10
#define MAX_DEMO_NAME_SIZE		64

typedef struct
{
	HWND				hWnd;

	int32				Mode;					// Server/Client, Server/Dedicated, Client, Single...

	geEngine			*Engine;
	VidMode				VidMode;
	GameMgr				*GMgr;
	NetMgr				*NMgr;

	geCSNetMgr			*CSNetMgr;
	UINT				CdID;

	Server_Server		*Server;
	Client_Client		*Client;

	geBoolean			Debug;

	geBoolean			WorldGammaHack;
} Host_Host;

typedef struct
{
	HWND				hWnd;
	char				ClientName[128];
	char				LevelHack[128];
	char				UserLevel[128]; // Frank
	char				IPAddress[NETMGR_MAX_IP_ADDRESS];
	int32				Mode;
	int32				DemoMode;
	char				DemoFile[64];
} Host_Init;

//===========================================================================
//	Function prototypes
//===========================================================================
Host_Host	*Host_Create(geEngine *Engine, Host_Init *InitData, GameMgr *GMgr, VidMode VidMode);
void		Host_Destroy(Host_Host *Host);
void		Host_DestroyAllObjects(Host_Host *Host);

geBoolean	Host_Frame(Host_Host *Host, float Time);
geBoolean	Host_RenderFrame(Host_Host *Host, float Time);
void		Host_ClientRefreshStatusBar(int32 NumPages);

#ifdef __cplusplus
}
#endif

#endif