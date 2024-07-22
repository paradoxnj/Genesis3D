/****************************************************************************************/
/*  Host.c                                                                              */
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
#include <Windows.h>
#include <assert.h>

#include "Host.h"

#include "Server.h"
#include "Client.h"

#include "GMenu.h"

#include "Game\Game.h"

#include "cd.h"

extern int32			CWidth;
extern int32			CHeight;

extern void	GenVS_Error(const char *Msg, ...);

Server_Server *GServer;

extern int32	GMode;

//===========================================================================
//	Host_Create
//===========================================================================
Host_Host *Host_Create(geEngine *Engine, Host_Init *InitData, GameMgr *GMgr, VidMode VidMode)
{
	Host_Host				*NewHost;
	geCSNetMgr_NetClient	GEClient;
	Console_Console			*Console;
	int32					Width, Height;
	
	assert(Engine != NULL);

	// Create the  host object
	NewHost = GE_RAM_ALLOCATE_STRUCT(Host_Host);

	assert(NewHost != NULL);
	
	if (!NewHost)
		return NULL;

	// Clear it
	memset(NewHost, 0, sizeof(Host_Host));

	// Grab the console
	Console = GameMgr_GetConsole(GMgr);
	assert(Console);

	//Console_SetActive(Console, GE_TRUE, GE_TRUE);
	VidMode_GetResolution(GameMgr_GetVidMode(GMgr), (int*)&Width, (int*)&Height);

	// Save the mode we are in...
	NewHost->VidMode = VidMode;
	NewHost->Mode = InitData->Mode;
	NewHost->GMgr = GMgr;

	// The host will need a copy of the VALID engine object
	NewHost->Engine = Engine;

	//
	//	Create a net mgr... (if needed)
	//
	if (InitData->Mode == HOST_MODE_SINGLE_PLAYER)
		NewHost->NMgr = NetMgr_Create(GE_TRUE);
	else
		NewHost->NMgr = NetMgr_Create(GE_FALSE);
	
	// Create a client if NOT running dedicated...
	if (InitData->Mode != HOST_MODE_SERVER_DEDICATED)
	{
		Client_Mode		ClientMode;

		if (InitData->Mode == HOST_MODE_CLIENT || InitData->DemoMode == HOST_DEMO_PLAY)
			ClientMode = ClientMode_Proxy;
		else
			ClientMode = ClientMode_Dumb;

		// Create the client for server/client mode
		// NOTE - DemoMode assumes that demomodes in host.h map directly to demo modes in client.h!!!
		NewHost->Client = Client_Create(Engine, ClientMode, GMgr, NewHost->NMgr, VidMode, InitData->DemoMode, InitData->DemoFile, InitData->Mode != HOST_MODE_SINGLE_PLAYER);

		if (!NewHost->Client)
			goto ExitWithError;
	
	}

	// Copy name over
	strcpy(GEClient.Name, InitData->ClientName);

	if (InitData->Mode == HOST_MODE_SERVER_CLIENT)		// Create a game as server with local client
	{
		GMode = 1;

		// Start up session
		if (!NetMgr_StartSession(NewHost->NMgr, "Genesis Virtual Studio", InitData->ClientName))
			goto ExitWithError;
	
		// Create the server
		NewHost->Server = Server_Create(GMgr, NewHost->NMgr, NewHost->Client, InitData->LevelHack);
		assert(NewHost->Server != NULL);

		if (!NewHost->Server)
			goto ExitWithError;

		// Steal the ID we were assigned by netplay
		GEClient.Id = NetMgr_GetOurID(NewHost->NMgr);
	}
	else if (InitData->Mode == HOST_MODE_CLIENT)		// Join as client
	{
		GMode = 2;

		//GameMgr_ConsolePrintf(GMgr, GE_TRUE,"Connecting to server...\n");
		if (!GameMgr_ClearBackground(GMgr, (Width/2)-(24*8/2), Height/2-10, "Connecting to server..."))
			GenVS_Error("GameMgr_SetWorld:  GameMgr_ClearBackground failed.\n");

		if (!NetMgr_JoinSession(NewHost->NMgr, InitData->IPAddress, InitData->ClientName))
			GenVS_Error("Host_Create:  NetMgr_JoinSession failed...");

		NewHost->Server = NULL;

		// Steal the Id we were assigned by netplay
		GEClient.Id = NetMgr_GetOurID(NewHost->NMgr);
		
		if (!GameMgr_ClearBackground(GMgr, (Width/2)-(13*8/2), Height/2-10, "Connected..."))
			GenVS_Error("GameMgr_SetWorld:  GameMgr_ClearBackground failed.\n");
	}
	else if (InitData->Mode == HOST_MODE_SINGLE_PLAYER)
	{
		GMode = 0;

		NewHost->Mode = HOST_MODE_SINGLE_PLAYER;
		// Record the single player ID
		GEClient.Id = NETMGR_SINGLE_PLAYER_NETID;

		// If we are going to play a demo, then we do not need a server...
		if (InitData->DemoMode != HOST_DEMO_PLAY)
		{
			// Create the server
			NewHost->Server = Server_Create(GMgr, NewHost->NMgr, NewHost->Client, InitData->LevelHack);

			assert(NewHost->Server != NULL);

			if (!NewHost->Server)
				goto ExitWithError;
		
			// Manually add client...
			// FIXME:  Make this a message!!!
			if (!Server_ClientConnect(NewHost->Server, &GEClient))
				GenVS_Error("Host_Create:  Could not add client in single player mode...\n");
		}
	}
	
	//GameMgr_ConsolePrintf(GMgr, GE_TRUE,"Connected...\n");
	//Console_SetActive(Console, GE_TRUE, GE_FALSE);

	// HACK!!! Hack for global console test
	GServer = NewHost->Server;

	NewHost->hWnd = 0;

	//NewHost->Time = 0.0f;
	
	Console_Printf(GameMgr_GetConsole(NewHost->GMgr), "--- Genesis Host initialized v%i.%i ---\n", NETMGR_VERSION_MAJOR, NETMGR_VERSION_MINOR);

	return NewHost;	

	ExitWithError:
	{
		if (NewHost)
		{
			Host_DestroyAllObjects(NewHost);
			geRam_Free(NewHost);
		}
	}
	return NULL;
}

//===========================================================================
//	Host_Destroy
//===========================================================================
void Host_Destroy(Host_Host *Host)
{
	assert(Host != NULL);

	Host_DestroyAllObjects(Host);

	geRam_Free(Host);
}

//===========================================================================
//	Host_ClientRefreshStatusBar
//===========================================================================
void Host_ClientRefreshStatusBar(int32 NumPages)
{
	Client_RefreshStatusBar(NumPages);
}

//===========================================================================
//	Host_DestroyAllObjects
//===========================================================================
void Host_DestroyAllObjects(Host_Host *Host)
{
	if (Host->Server)
	{
		Server_Destroy(Host->Server);
		Host->Server = NULL;
	}

	if (Host->Client)
	{
		Client_Destroy(Host->Client);
		Host->Client = NULL;
	}

	if (Host->NMgr)
	{
		NetMgr_Destroy(Host->NMgr);
		Host->NMgr = NULL;
	}

	if (Host->GMgr)
	{
		// Make we clean up any old world that might by laying around
		GameMgr_FreeWorld(Host->GMgr);		
	}
}

//===========================================================================
//	Host_Frame
//===========================================================================
geBoolean Host_Frame(Host_Host *Host, float Time)
{
	
	if (Time < 0.001f)
		Time = 0.001f;
	
	if (Time > 0.1f)
		Time = 0.1f;
	
	//Host->Time += Time;

	Host->Engine = GameMgr_GetEngine(Host->GMgr);

	//
	// Do a server frame (if not in demo play mode)
	//
	if (Host->Server)
	{
		if (!Server_Frame(Host->Server, Host->GMgr, Time))
			return GE_FALSE;
	}

	//
	//	Do a client frame
	//
	NetMgr_ResetClientBuffer(Host->NMgr);

	if (Host->Client)	// Client will be NULL for dedicated servers
	{
		if (!Client_Frame(Host->Client, Time))
			return GE_FALSE;
	}

	NetMgr_ResetServerBuffer(Host->NMgr);

	return GE_TRUE;
}

//===========================================================================
//	Host_RenderFrame
//===========================================================================
geBoolean Host_RenderFrame(Host_Host *Host, float Time)
{
	if (Host->Client)
	{
		if (!Client_RenderFrame(Host->Client, Time))
		{
			geErrorLog_AddString(-1, "Host_RenderFrame:  Client_RenderFrame failed...", NULL);
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}
