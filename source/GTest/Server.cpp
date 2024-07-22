/****************************************************************************************/
/*  Server.c                                                                            */
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
#include <Time.h>

#include "Server.h"

LARGE_INTEGER			g_Freq, g_OldTick, g_CurTick;

#define	NUM_AVG			10
static float			AvgTime[NUM_AVG];
static int32			CurAvg;

static void SubLarge(LARGE_INTEGER *start, LARGE_INTEGER *end, LARGE_INTEGER *delta)
{
	_asm {
		mov ebx,dword ptr [start]
		mov esi,dword ptr [end]

		mov eax,dword ptr [esi+0]
		sub eax,dword ptr [ebx+0]

		mov edx,dword ptr [esi+4]
		sbb edx,dword ptr [ebx+4]

		mov ebx,dword ptr [delta]
		mov dword ptr [ebx+0],eax
		mov dword ptr [ebx+4],edx
	}
}

#define BEGIN_TIMER()		QueryPerformanceCounter(&g_OldTick)
				
#define END_TIMER(g)											\
				{												\
					LARGE_INTEGER	DeltaTick;					\
					float			ElapsedTime, Total;			\
					int32			i;							\
																\
					QueryPerformanceCounter(&g_CurTick);		\
					SubLarge(&g_OldTick, &g_CurTick, &DeltaTick);	\
																\
					if (DeltaTick.LowPart > 0)					\
						ElapsedTime =  1.0f / (((float)g_Freq.LowPart / (float)DeltaTick.LowPart));		\
					else										\
						ElapsedTime = 0.001f;					\
																\
					AvgTime[CurAvg] = ElapsedTime;				\
					CurAvg++;									\
					CurAvg %= NUM_AVG;							\
																\
					for (Total = 0.0f, i=0; i< NUM_AVG; i++)	\
						Total += AvgTime[i];					\
																\
					Total *= (1.0f/ NUM_AVG);					\
																\
					geEngine_Printf(GameMgr_GetEngine(g), 1, 70, "Timer ms: %2.3f/%2.3f", ElapsedTime, Total);	\
				}
extern		geBoolean	ShowStats;
static		int ServerBotCount = 0;

void		GenVS_Error(const char *Msg, ...);
static		geBoolean Server_ManageBots(Server_Server *Server);

#ifdef _DEBUG
	uint32 SERVER_GPLAYER_TO_INDEX(Server_Server *Server, GPlayer *Player)
	{
		uint32		Index;

		assert(Server);
		assert(Player);

		assert(Player >= Server->SvPlayers);
		
		Index = (Player - Server->SvPlayers);

		assert(Index < NETMGR_MAX_PLAYERS);

		return Index;
	}
#else
	#define SERVER_GPLAYER_TO_INDEX(s, p) ((p) - (s)->SvPlayers)
#endif

//=====================================================================================
//	Local static functions
//=====================================================================================
static geBoolean SendAllClientsMessage(Server_Server *Server, Buffer_Data *Buffer, geBoolean G);

static geBoolean ReadClientMessages(Server_Server *Server, float Time);
static void FillBufferWithPlayerData(Server_Server *Server, Buffer_Data *Buffer, GPlayer *Player, uint16 SendFlags);
static geBoolean SendPlayersToClients(Server_Server *Server);
static void ControlPlayer(Server_Server *Server, GPlayer *Player, float Time);
static geBoolean ControlPlayers(Server_Server *Server, float Time);
static geBoolean Server_IsClientBot(void *S, GenVSI_CHandle ClientHandle);

static void ForceServerPlayerOnLocalClient(Server_Server *Server, GPlayer *Player);

static void Server_SetupGenVSI(void *S);
static geBoolean Server_ManageBots(Server_Server *Server);

//=====================================================================================
//	Server_SetPlayerDefaults
//=====================================================================================
static void Server_SetPlayerDefaults(GPlayer *Player)
{
	memset(Player, 0, sizeof(GPlayer));

	Player->OldViewFlags	= Player->ViewFlags		= VIEW_TYPE_NONE;
	Player->OldViewIndex	= Player->ViewIndex		= VIEW_INDEX_NONE;
	Player->OldControlIndex = Player->ControlIndex	= CONTROL_INDEX_NONE;
	Player->OldTriggerIndex = Player->TriggerIndex	= TRIGGER_INDEX_NONE;
	Player->OldScale		= Player->Scale			= 1.0f;
	Player->OldMotionIndex	= Player->MotionIndex	= MOTION_INDEX_NONE;

	Player->PingTime = -1.0f;		// PingTime is the players ping time if player belongs a client.
									// If the player does not belong to a client, PingTime is the PingTime of the player who created this player...
}

//=====================================================================================
//	Server_Create
//=====================================================================================
Server_Server *Server_Create(GameMgr *GMgr, NetMgr *NMgr, Client_Client *Client, const char *LevelHack)
{
	int32			i;
	Server_Server	*NewServer;

	NewServer = GE_RAM_ALLOCATE_STRUCT(Server_Server);

	assert(NewServer != NULL);
	
	if (!NewServer)
		return NULL;

	memset(NewServer, 0, sizeof(Server_Server));

	// These objects CANNOT change throughout this servers life!!!
	NewServer->GMgr = GMgr;
	NewServer->NMgr = NMgr;
	NewServer->Client = Client;

	Server_SetupGenVSI(NewServer);

	if (!Server_NewWorldDefaults(NewServer))
	{
		geRam_Free(NewServer);
		return NULL;
	}

	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
		Server_SetPlayerDefaults(&NewServer->SvPlayers[i]);

	// Reset the numbers of spawn callbacks
	NewServer->NumClassSpawns = 0;
	ServerBotCount = 0;

	// Call the game main code, to initialize everything...
	Server_Main(&NewServer->GenVSI, LevelHack);
	Console_Printf(GameMgr_GetConsole(NewServer->GMgr), "Server_Create:  Game_Main initialized\n");
	Console_Printf(GameMgr_GetConsole(NewServer->GMgr), "Server_Create:  Num Main Spawned Players: %i\n", NewServer->NumTotalPlayers);

	Console_Printf(GameMgr_GetConsole(NewServer->GMgr), "Server_Create:  Server created...\n");
	
	return NewServer;
}

//=====================================================================================
//	Server_Destroy
//=====================================================================================
void Server_Destroy(Server_Server *Server)
{
	Buffer_Data		Buffer;
	char			Data[128];
	geBoolean		Ret;

	assert(Server);
	
	Ret = Server_FreeWorldData(Server);

	assert(Ret == GE_TRUE);

	// Make sure everybody knows we are quiting as the server
	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_SHUTDOWN);

	SendAllClientsMessage(Server, &Buffer, GE_TRUE);

	geRam_Free(Server);
}

//=====================================================================================
//	FillBufferWithClientInfo
//=====================================================================================
static geBoolean FillBufferWithClientInfo(Server_Server *Server, Buffer_Data *Buffer)
{
	int32			i;
	Server_Client	*Client;

	Client = Server->Clients;

	for (i=0; i< NETMGR_MAX_CLIENTS; i++, Client++)
	{
		if (!Client->Active)
			continue;

		Buffer_FillByte(Buffer, NETMGR_MSG_CLIENT_ACTIVE);
		Buffer_FillByte(Buffer, (char)i);
		Buffer_FillByte(Buffer, 1);

		Buffer_FillByte(Buffer, NETMGR_MSG_CLIENT_NAME);
		Buffer_FillByte(Buffer, (char)i);
		Buffer_FillString(Buffer, (uint8*)Client->Name);
	
		Buffer_FillByte(Buffer, NETMGR_MSG_CLIENT_SCORE);
		Buffer_FillByte(Buffer, (char)i);
		Buffer_FillSLong(Buffer, (int32)Client->Score);
	
		Buffer_FillByte(Buffer, NETMGR_MSG_CLIENT_HEALTH);
		Buffer_FillByte(Buffer, (char)i);
		Buffer_FillSLong(Buffer, (int32)Client->Health);
	}

	return GE_TRUE;
}

//=====================================================================================
//	SendClientPlayerData
//	Forces a player update toa client
//=====================================================================================
static geBoolean SendClientPlayerData(Server_Server *Server, Server_Client *Client)
{
	int32			i;
	Buffer_Data		Buffer;
	char			Data[20000];
	GPlayer			*Player;

	Buffer_Set(&Buffer, Data, 20000);

	Buffer_FillByte(&Buffer, NETMGR_MSG_TIME);
	Buffer_FillFloat(&Buffer, GameMgr_GetTime(Server->GMgr));
	Buffer_FillFloat(&Buffer, Client->Ping);

	Player = Server->SvPlayers;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++, Player++)
	{
		if (!Player->Active)
			continue;

		Buffer_FillByte(&Buffer, NETMGR_MSG_NEW_WORLD_PLAYER_DATA);

		FillBufferWithPlayerData(Server, &Buffer, Player, 0xffff);	// Send all on a force update
	}

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("SendClientPlayerData:  NetMgr_SendClientMessage failed.\n");

	return GE_TRUE;
}

//=====================================================================================
//	Server_ChangeClientState
//=====================================================================================
geBoolean Server_ChangeClientState(Server_Server *Server, Server_Client *Client, NetMgr_NetState NetState)
{
	Buffer_Data		Buffer;
	char			Data[128];

	assert(Server);
	assert(Client);

	Buffer_Set(&Buffer, Data, 128);

	Buffer_FillByte(&Buffer, NETMGR_MSG_NET_STATE_CHANGE);
	Buffer_FillSLong(&Buffer, NetState);

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_ChangeClientState:  NetMgr_SendClientMessage failed.\n");

	Client->NetState = NetState;
	Client->NetStateConfirmed[NetState] = GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	Server_SetupClientWithCurrentWorld
//	Send a client all the info needed to change into a new world
//=====================================================================================
geBoolean Server_SetupClientWithCurrentWorld(void *S, Server_Client *Client)
{
	geWorld		*World;
	Server_Server *Server = static_cast<Server_Server*>(S);

	World = GameMgr_GetWorld(Server->GMgr);

	assert(World != NULL);		// If we got here, there should be a world!!!

	// Change the state of the client to NetState_WorldChange...
	if (!Server_ChangeClientState(Server, Client, NetState_WorldChange))
		GenVS_Error("Server_SetupClientWithCurrentWorld:  Server_ChangeClientState failed.\n");

	if (!Server_SendClientCurrentWorldData(Server, Client))
		GenVS_Error("Server_SetupClientWithCurrentWorld:  Server_SendClientCurrentWorldData failed.\n");

	if (!SendClientPlayerData(Server, Client))
		GenVS_Error("Server_SetupClientWithCurrentWorld:  SendClientPlayerData failed.\n");

	if (!Server_ChangeClientState(Server, Client, NetState_WorldActive))
		GenVS_Error("Server_SetupClientWithCurrentWorld:  Server_ChangeClientState failed.\n");

	return GE_TRUE;
}

//=====================================================================================
//	Server_SetupAllClientsWithCurrentWorld
//	Update everything that needs to be updated when the world changes...
//	This function sends to all clients currently connected...
//=====================================================================================
geBoolean Server_SetupAllClientsWithCurrentWorld(void *S)
{
	int32			i; 
	Server_Server *Server = static_cast<Server_Server*>(S);

	// Send all the clients what index slot they are in...
	for (i=0; i< NETMGR_MAX_CLIENTS; i++)
	{
		Server_Client	*Client;

		Client = &Server->Clients[i];

		if (!Client->Active)
			continue;

		if (!Server_SetupClientWithCurrentWorld(Server, Client))
		{
			Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//	Server_SendClientStartupData
//=====================================================================================
geBoolean Server_SendClientStartupData(void *S, Server_Client *Client)
{

	uint8			Data[20000];
	Buffer_Data		Buffer;
	int32			ClientIndex;
	geWorld			*World;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Client->Player);
	
	ClientIndex = Client - Server->Clients;
	assert(ClientIndex >=0 && ClientIndex < NETMGR_MAX_CLIENTS);

	// First, Tell this client what index slot he is...
	Buffer_Set(&Buffer, (char*)Data, 20000);

	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_INDEX);
	Buffer_FillByte(&Buffer, (uint8)ClientIndex);
	
	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientStartupData:  NetMgr_SendClientMessage failed.\n");

	Buffer_Set(&Buffer, (char*)Data, 20000);

	if (!FillBufferWithClientInfo(Server, &Buffer))
		GenVS_Error("Server_SendClientStartupData:  FillBufferWithClientInfo failed.\n");

	// Send everyone (even client) the info about the clients, score, health, active, etc...
	if (!SendAllClientsMessage(Server, &Buffer, GE_TRUE))		// Everyone needs to know about this one...
		GenVS_Error("Server_SendClientStartupData:  SendAllClientsMessage failed.\n");

	World = GameMgr_GetWorld(Server->GMgr);

	if (World)		// If there is a world, setup this client with it...
	{
		if (!Server_SetupClientWithCurrentWorld(Server, Client))
			GenVS_Error("Server_SendClientStartupData:  Server_SetupClientWithCurrentWorld failed.\n");
	}

	return GE_TRUE;
}

//=====================================================================================
//=====================================================================================
static geBoolean SendAllClientsMessage(Server_Server *Server, Buffer_Data *Buffer, geBoolean G)
{
	int32		i;

	// Send the message
	for (i=0; i< NETMGR_MAX_CLIENTS; i++)
	{
		Server_Client	*Client;

		Client = &Server->Clients[i];

		if (!Client->Active)
			continue;

		if (Server_IsClientBot(Server,i))
 			continue;

		if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, Buffer, G))
		{
			Server_ClientDisconnect(Server, Client->NetID, Client->Name);
			//return GE_TRUE;
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//	Server_SendClientCurrentWorldData
//	Sends the client all the info about the current world.
//	Sends the world name, all the current meshes, actors, textures, sounds, etc...
//=====================================================================================
geBoolean Server_SendClientCurrentWorldData(void *S, Server_Client *Client)
{
	int32		i;
	Buffer_Data	Buffer;
	char		Data[20000];
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(GameMgr_GetWorld(Server->GMgr));
	assert(Client->Active == GE_TRUE);

	// Fill message with world data
	Buffer_Set(&Buffer, Data, 20000);
	Buffer_FillByte(&Buffer, NETMGR_MSG_SET_WORLD);
	Buffer_FillString(&Buffer, (uint8*)Server->WorldName);

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 1.\n");

	// fill message with actor index data
	Buffer_Set(&Buffer, Data, 20000);
	for (i=0; i< GAMEMGR_MAX_ACTOR_INDEX; i++)
	{
		GameMgr_ActorIndex	*ActorIndex;

		ActorIndex = GameMgr_GetActorIndex(Server->GMgr, i);

		if (!ActorIndex->Active)
			continue;

		Buffer_FillByte(&Buffer, NETMGR_MSG_ACTOR_INDEX);
		Buffer_FillSLong(&Buffer, i);
		Buffer_FillString(&Buffer, (uint8*)ActorIndex->FileName);
	}

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 2.\n");

	// fill message with motion index data
	Buffer_Set(&Buffer, Data, 20000);
	for (i=0; i< GAMEMGR_MAX_MOTION_INDEX; i++)
	{
		GameMgr_MotionIndexDef		*MotionIndex;

		MotionIndex = GameMgr_GetMotionIndexDef(Server->GMgr, i);

		// Only send motion indexes that are active, or need to be shutdown
		if (!MotionIndex->Active)
			continue;

		Buffer_FillByte(&Buffer, NETMGR_MSG_MOTION_INDEX);
		Buffer_FillSLong(&Buffer, i);
		Buffer_FillString(&Buffer, (uint8*)MotionIndex->MotionName);
	}

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 3.\n");

	// fill message with bone index data
	Buffer_Set(&Buffer, Data, 20000);
	for (i=0; i< GAMEMGR_MAX_BONE_INDEX; i++)
	{
		GameMgr_BoneIndex		*BoneIndex;

		BoneIndex = GameMgr_GetBoneIndex(Server->GMgr, i);

		// Only send motion indexes that are active, or need to be shutdown
		if (!BoneIndex->Active)
			continue;

		Buffer_FillByte(&Buffer, NETMGR_MSG_BONE_INDEX);
		Buffer_FillSLong(&Buffer, i);
		Buffer_FillString(&Buffer, (uint8*)BoneIndex->BoneName);
	}

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 4.\n");

	// Fill message with texture index data
	Buffer_Set(&Buffer, Data, 20000);
	for (i=0; i< GAMEMGR_MAX_TEXTURE_INDEX; i++)
	{
		GameMgr_TextureIndex		*TextureIndex;

		TextureIndex = GameMgr_GetTextureIndex(Server->GMgr, i);

		// Only send motion indexes that are active, or need to be shutdown
		if (!TextureIndex->Active)
			continue;

		Buffer_FillByte(&Buffer, NETMGR_MSG_TEXTURE_INDEX);
		Buffer_FillSLong(&Buffer, i);
		Buffer_FillString(&Buffer, (uint8*)TextureIndex->FileName);
		Buffer_FillString(&Buffer, (uint8*)TextureIndex->AFileName);
	}

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 5.\n");

	// Fill message with sound index data
	Buffer_Set(&Buffer, Data, 20000);
	for (i=0; i< GAMEMGR_MAX_SOUND_INDEX; i++)
	{
		GameMgr_SoundIndex		*SoundIndex;

		SoundIndex = GameMgr_GetSoundIndex(Server->GMgr, i);

		// Only send motion indexes that are active, or need to be shutdown
		if (!SoundIndex->Active)
			continue;

		Buffer_FillByte(&Buffer, NETMGR_MSG_SOUND_INDEX);
		Buffer_FillSLong(&Buffer, i);
		Buffer_FillString(&Buffer, (uint8*)SoundIndex->FileName);
	}

	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 6.\n");

	// Tell the client what player he is
	{
		int32			PlayerIndex;

		assert(Client->Player);

		PlayerIndex = (int32)SERVER_GPLAYER_TO_INDEX(Server, Client->Player);

		Buffer_Set(&Buffer, Data, 10000);
		Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_PLAYER_INDEX);
		Buffer_FillSLong(&Buffer, PlayerIndex);

		if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
			GenVS_Error("Server_SendClientCurrentWorldData:  NetMgr_SendClientMessage failed 7.\n");
	}

	return GE_TRUE;
}

//=====================================================================================
//	Server_ValidateClient
//=====================================================================================
void Server_ValidateClient(Server_Server *Server, Server_Client *Client)
{
	GPlayer			*Player;
	geWorld			*World;

	assert(Server);
	assert(Client);
	assert(Client->Active);

	assert(Client->Player == NULL);
	//if (Client->Player)		// Already has a player...
	//	return;

	// Create a player for the client
	//Player = Server_CreatePlayer(Server, "ClientPlayer");
	Player = static_cast<GPlayer*>( Server_CreatePlayer(Server, Client->Name));

	if (!Player)
		GenVS_Error("Server_ClientConnect:  No more players available for client.");

	// Connect them
	Client->Player = Player;
	Player->ClientHandle = (GenVSI_CHandle)(Client - Server->Clients);

	World = GameMgr_GetWorld(Server->GMgr);

	// Make sure the client is spawned if there is a world to spawn it in...
	if (!Client->Spawned && World)
	{
		int32		PlayerIndex;
		Buffer_Data	Buffer;
		char		Data[1024];
        
        if (Server_IsClientBot(Server,Player->ClientHandle))
		{
			if (!Game_SpawnBot(&Server->GenVSI, World, Player, Player->ClassData))
				GenVS_Error("Server_ValidateClient:  Game_SpawnBot failed...");
		}
		else
		{
			if (!Game_SpawnClient(&Server->GenVSI, World, Player, Player->ClassData))
				GenVS_Error("Server_ValidateClient:  Game_SpawnClient failed...");
		}

		if (!Client->Active)		// Client got removed...
			return;

		Client->Spawned = GE_TRUE;
		
		//??bot HACK ask john if its ok
		if (!Player->DFunc)
			Player->DFunc = Game_DestroyClient;

		PlayerIndex = (int32)SERVER_GPLAYER_TO_INDEX(Server, Client->Player);

		Buffer_Set(&Buffer, Data, 1024);

		Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_PLAYER_INDEX);
		Buffer_FillSLong(&Buffer, PlayerIndex);

		if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		{
			Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		}

		ForceServerPlayerOnLocalClient(Server, Client->Player);
	}
}

//=====================================================================================
//	Server_ClientConnect
//=====================================================================================
geBoolean Server_ClientConnect(void *S, const geCSNetMgr_NetClient *Client)
{
	int32			i;
	Buffer_Data		Buffer;
	char			Data[128];
	Server_Client	*SClient;
	Server_Server *Server = static_cast<Server_Server*>(S);

	SClient = Server->Clients;

	for (i=0; i< NETMGR_MAX_CLIENTS; i++, SClient++)
	{
		if (!SClient->Active)			// Look for a non active client slot
			break;
	}
	
	if (i >= NETMGR_MAX_CLIENTS)
		return FALSE;
	
	memset(SClient, 0, sizeof(Server_Client));
	
	strcpy(SClient->Name, Client->Name);
	SClient->NetID = Client->Id;

	SClient->Active = GE_TRUE;
	
	// Send version to client FIRST thing
	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_VERSION);
	Buffer_FillLong(&Buffer, NETMGR_VERSION_MAJOR);
	Buffer_FillLong(&Buffer, NETMGR_VERSION_MINOR);

	if (!NetMgr_SendClientMessage(Server->NMgr, SClient->NetID, &Buffer, GE_TRUE))
	{
		Server_ClientDisconnect(Server, SClient->NetID, SClient->Name);
		return GE_TRUE;
	}

	// Let the client know that it is currently connecting
	Server_ChangeClientState(Server, SClient, NetState_Connecting);

	// Make the client legit, create it's player, etc...
	Server_ValidateClient(Server, SClient);

	if (!SClient->Active)		// Client was dicconected...
		return GE_TRUE;

	// Send this client the startupdata for everything at it's current state
	if (!Server_SendClientStartupData(Server, SClient))
	{
		Server_ClientDisconnect(Server, SClient->NetID, SClient->Name);
		
		GenVS_Error("Server_ClientConnect:  Server_SendClientStartupData failed.\n");
		return GE_FALSE;
	}

	Console_Printf(GameMgr_GetConsole(Server->GMgr), "   Client connected: %s, %i...\n", Client->Name, Client->Id);

	return GE_TRUE;
}


//=====================================================================================
//	Server_BotConnect
//=====================================================================================
Server_Client *Server_BotConnect(void *S, const char *BotName)
{
	int32			i;
	Server_Client	*SClient;
	Server_Server *Server = static_cast<Server_Server*>(S);

	SClient = Server->Clients;

	for (i=0; i< NETMGR_MAX_CLIENTS; i++, SClient++)
	{
		if (!SClient->Active)			// Look for a non active client slot
			break;
	}
	
	if (i >= NETMGR_MAX_CLIENTS)
		return NULL;
	
	memset(SClient, 0, sizeof(Server_Client));
	
	strcpy(SClient->Name, BotName);
	SClient->NetID = NETMGR_SPECIAL_BOT_NETID;

	SClient->Active = GE_TRUE;
	
	// Make the client legit, create it's player, etc...
	Server_ValidateClient(Server, SClient);

	if (!SClient->Active)		// Client was dicconected...
		return SClient;

	// Send this client the startupdata for everything at it's current state
	if (!Server_SendClientStartupData(Server, SClient))
	{
		Server_ClientDisconnect(Server, SClient->NetID, SClient->Name);
		return NULL;
	}

	Console_Printf(GameMgr_GetConsole(Server->GMgr), "(Server) Client connected: %s, %i...\n", "Bot", NETMGR_SPECIAL_BOT_NETID);

	return SClient;
}

	
//=====================================================================================
//	Server_ClientDisconnect
//=====================================================================================
geBoolean Server_ClientDisconnect(void *S, geCSNetMgr_NetID Id, const char *Name)
{
	int32			i;
	Server_Client	*SClient;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(Name);

	for (i=0; i< NETMGR_MAX_CLIENTS; i++)
	{
		SClient = &Server->Clients[i];

		if (!SClient->Active)	
			continue;

		if (SClient->NetID == Id)
		{
			Buffer_Data		Buffer;
			char			Data[512];

			if (SClient->Player)
				Server_DestroyPlayer(Server, SClient->Player);
			
			SClient->Player = NULL;
			SClient->Active = GE_FALSE;

			// Tell all clients (execpt this one of course), that this client is not active
			Buffer_Set(&Buffer, Data, 512);

			Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_ACTIVE);
			Buffer_FillByte(&Buffer, (char)i);
			Buffer_FillByte(&Buffer, 0);		// The client is not active anymore!!!

			SendAllClientsMessage(Server, &Buffer, GE_TRUE);

			Console_Printf(GameMgr_GetConsole(Server->GMgr), "[SERVER] Client disconnected: %s, %i...\n", Name, Id);
			return GE_TRUE;
		}
	}
	
	Console_Printf(GameMgr_GetConsole(Server->GMgr), "[SERVER] Client not found for disconnect: %s, %i...\n", Name, Id);

	return GE_FALSE;
}

//=====================================================================================
//	Server_ClientDisconnectByHandle
//=====================================================================================
geBoolean Server_ClientDisconnectByHandle(void *S, GenVSI_CHandle ClientHandle)
{
	Server_Client	*SClient;
	Buffer_Data		Buffer;
	char			Data[512];
	Server_Server	*Server = static_cast<Server_Server*>(S);

	assert(Server);

	SClient = &Server->Clients[ClientHandle];

	assert(SClient);

	if (SClient->Player)
		Server_DestroyPlayer(Server, SClient->Player);
	
	SClient->Player = NULL;
	SClient->Active = GE_FALSE;

	// Tell all clients (execpt this one of course), that this client is not active
	Buffer_Set(&Buffer, Data, 512);

	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_ACTIVE);
	Buffer_FillByte(&Buffer, (char)ClientHandle);
	Buffer_FillByte(&Buffer, 0);		// The client is not active anymore!!!

	SendAllClientsMessage(Server, &Buffer, GE_TRUE);
	
	return GE_TRUE;
}


//=====================================================================================
//	Server_CreatePlayer
//=====================================================================================
void *Server_CreatePlayer(void *S, const char *ClassName)
{
	int32		i, Start, End;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(strlen(ClassName) < MAX_CLASS_NAME_STRING);

	Start = 0;
	End = NETMGR_MAX_PLAYERS;

	for (i=Start; i< End; i++)
	{
		if (!Server->SvPlayers[i].Active)
			break;
	}

	if (i >= End)
	{
		GenVS_Error("Failed to add player!!!\n");

		Console_Printf(GameMgr_GetConsole(Server->GMgr), "Server_CreatePlayer2:  Max players exceeded...\n");
		return NULL;
	}

	assert(Server->NumTotalPlayers >= 0);

	Server->NumTotalPlayers++;

	assert(Server->NumTotalPlayers <= End);
	
	memset(&Server->SvPlayers[i], 0, sizeof(GPlayer));

	Server->SvPlayers[i].Active = GE_TRUE;
	// Default to no view index, and local only player
	Server->SvPlayers[i].OldViewFlags = VIEW_TYPE_NONE | VIEW_TYPE_LOCAL;
	Server->SvPlayers[i].ViewFlags = VIEW_TYPE_NONE | VIEW_TYPE_LOCAL;
	Server->SvPlayers[i].OldViewIndex = 0xffff;
	Server->SvPlayers[i].ViewIndex = 0xffff;
	Server->SvPlayers[i].ControlIndex = 0xffff;
	Server->SvPlayers[i].TriggerIndex = 0xffff;
	strcpy(Server->SvPlayers[i].ClassName, ClassName);
	Server->SvPlayers[i].ClientHandle = CLIENT_NULL_HANDLE;

	return &Server->SvPlayers[i];
}

//=====================================================================================
//	Callback_CallDestroy
//=====================================================================================
void Callback_CallDestroy(Server_Server *Server, GPlayer *Player)
{
	if (Player->DFunc)
		Player->DFunc(&Server->GenVSI, Player, Player->ClassData);

	Player->DFunc = NULL;
}

//=====================================================================================
//	Server_DestroyPlayer
//=====================================================================================
void Server_DestroyPlayer(void *S, void *P)
{
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer *Player = static_cast<GPlayer*>(P);

	assert(Server);
	assert(Player);

	assert(Player->Active);

	assert(Server->NumTotalPlayers <= NETMGR_MAX_PLAYERS);

	Callback_CallDestroy(Server, Player);

	Server->NumTotalPlayers--;

	assert(Server->NumTotalPlayers >= 0);

	memset(Player, 0, sizeof(GPlayer));
}

//=====================================================================================
//	Server_FreeWorldData
//	Frees any data that the server may have in the currently loaded world
//=====================================================================================
geBoolean Server_FreeWorldData(Server_Server *Server)
{
	GPlayer			*Player;
	int32			i;

	assert(Server);
	
	// Let the Game stuff free all the data they might have allocated, etc...
	if (Server->ShutdownWorldCB1)
	{
		if (!Server->ShutdownWorldCB1(&Server->GenVSI))
			GenVS_Error("Server_FreeWorldData:  ShutdownWorldCB1 failed.\n");
	}

	// Free all the players data that is in the world
	for (Player = Server->SvPlayers, i=0; i< NETMGR_MAX_PLAYERS; i++, Player++)
	{
		if (Player->DFunc)
			Player->DFunc(&Server->GenVSI, Player, Player->ClassData);

		Player->DFunc = NULL;
	}

	return GE_TRUE;
}

//=====================================================================================
//	Server_NewWorldDefaults
//=====================================================================================
geBoolean Server_NewWorldDefaults(Server_Server *Server)
{
	int32			i;
	GPlayer			*Player;
	Server_Client	*Client;

	assert(Server);

	// Free all the players that are left over (lazy garbage collection)
	for (Player = Server->SvPlayers, i=0; i< NETMGR_MAX_PLAYERS; i++, Player++)
	{
		assert(Player->DFunc == NULL);
		Server_SetPlayerDefaults(Player);
	}

	// Reset the number of players, since we just freed them...
	Server->NumTotalPlayers = 0;;

	// Make sure the clients don't have any players attached...
	for (Client = Server->Clients, i=0; i< NETMGR_MAX_CLIENTS; i++, Client++)
	{
		// FIXME:  Reset more stuff?
		Client->Player = NULL;		// Make sure the clients make new players
		Client->NextUpdate = 0.0f;	// Reset all the timer stuff
		Client->OldMoveTime = 0.0f;
		Client->MoveTime = 0.0f;
		Client->Spawned = GE_FALSE;	// Make sure clients are respawned in the new world
	}

	return GE_TRUE;
}

//=====================================================================================
//	Server_SpawnWorld
//	Calls the spawn functions of all registered entities
//	Calls the spawn function of the clients currently connected...
//=====================================================================================
geBoolean Server_SpawnWorld(Server_Server *Server)
{
	int32				i;
	geWorld				*World;
	Server_CSpawn		*CSpawn;
	GPlayer				*Player;
	void				*ClassData;
	geEntity_EntitySet	*ClassSet;
	geEntity			*Entity;
						
	assert(Server);

	World = GameMgr_GetWorld(Server->GMgr);
	assert(World);

	if (!Server->NumClassSpawns)
	{
		GenVS_Error("Server_SpawnWorld: No Class Spawns in Game code.\n");
	}

	// Setup all the class callbacks...
	for (i=0; i < Server->NumClassSpawns; i++)
	{
		CSpawn = &Server->ClassSpawns[i];

		// Look for the class name in the world
		ClassSet = geWorld_GetEntitySet(World, CSpawn->Name);

		// If not found, just continue...
		if (!ClassSet)
			continue;

		Entity = NULL;

		while (1)
		{
			#define MAX_NAME 200
			char	EntityName[MAX_NAME];

			Entity = geEntity_EntitySetGetNextEntity(ClassSet, Entity);
			if (!Entity)
				break;

			geEntity_GetName(Entity, EntityName, MAX_NAME-1);
			EntityName[MAX_NAME-1]=0;

			ClassData = geEntity_GetUserData(Entity);

			// Create the player for the game
			Player = static_cast<GPlayer*>(Server_CreatePlayer(Server, CSpawn->Name));

			Player->Mode = MODE_Authoritive;
			Player->ClassData = ClassData;
			Player->DFunc = CSpawn->DFunc;

			assert(CSpawn->Func);

			// Call the game spawn for the player
			CSpawn->Func(&Server->GenVSI, Player, ClassData, EntityName);
			// this better not fail!
		}
			
	}

	// Make sure all clients get spawned with this new world
	for (i=0; i< NETMGR_MAX_CLIENTS; i++)
	{
		Server_Client	*Client;
		//int32			ClientIndex;

		Client = &Server->Clients[i];

		if (!Client->Active)
			continue;

		// Validate the client
		Server_ValidateClient(Server, Client);
	}
	
	//geRam_ReportAllocations();

	//DebugBreak();
	
	return GE_TRUE;
}

extern geVFile *MainFS;

//=====================================================================================
//	Server_SetupWorld
//=====================================================================================
geBoolean Server_SetupWorld(Server_Server *Server)
{
	geWorld			*World;
	GameMgr			*GMgr;

	assert(Server);
	assert(Server->NewWorldCB);

	GMgr = Server->GMgr;
	World = GameMgr_GetWorld(GMgr);

	assert(GMgr);
	assert(World);
	assert(MainFS);
	
	// Call the Games NewWorld callback function to set up this world with meshes, actors, sounds, etc...
	if (!Server->NewWorldCB(&Server->GenVSI, Server->WorldName, MainFS))
		GenVS_Error("Server_SetupWorld:  NewWorldCB failed.\n");

	// Setup the new world, and spawn all connected clients...
	if (!Server_SpawnWorld(Server))
		GenVS_Error("Server_SetWorld:  Failed to spawn world.\n");

	// Send any currently connected clients all the info about this new world...
	if (!Server_SetupAllClientsWithCurrentWorld(Server))
		GenVS_Error("Server_SetWorld:  Failed send all clients world data.\n");

	//Console_Printf(GameMgr_GetConsole(Server->GMgr), "[SERVER] SetWorld: %s\n", Name);
	return GE_TRUE;
}

//=====================================================================================
//	Server_Frame
//=====================================================================================
geBoolean Server_Frame(Server_Server *Server, GameMgr *GMgr, float Time)
{
	assert(Server != NULL);
	assert(GMgr);

	Server->GMgr = GMgr;

	// Make sure our local client always has the correct time!!!
	Server->Client->NetTime = GameMgr_GetTime(Server->GMgr);

	if (Server->ChangeWorldRequest)
	{
		Server->ChangeWorldRequest = GE_FALSE;

		if (!Server_FreeWorldData(Server))
			GenVS_Error("Server_Frame:  Server_FreeWorldData failed.\n");

		// Reset the server for a new world
		if (!Server_NewWorldDefaults(Server))
			GenVS_Error("Server_Frame:  Server_NewWorldDefaults failed.\n");

		// Reset the local client for a new world
		if (Server->Client)
			Client_NewWorldDefaults(Server->Client);

		if (!GameMgr_SetWorld(GMgr, Server->WorldName))
			GenVS_Error("Server_Frame:  GameMgr_SetWorld failed.\n");
		
		if (!Server_SetupWorld(Server))
			GenVS_Error("Server_Frame:  Server_SetupWorld failed.\n");
	
		// Shift the Shutdown Callbacks down one in the stack
		Server->ShutdownWorldCB1 = Server->ShutdownWorldCB2;
		Server->ShutdownWorldCB2 = NULL;

		//return GE_TRUE;
	}

	if (!GameMgr_GetWorld(GMgr))	
		return GE_TRUE;

	//memset(&Server->NetStats, 0, sizeof(Server_NetStat));

	if (!ReadClientMessages(Server, Time))	// Read inputs from clients to control their players
		GenVS_Error("Server_Frame:  ReadClientMessages failed.\n");
	
	if (!ControlPlayers(Server, Time))				// Apply physics and stuff to the players
		GenVS_Error("Server_Frame:  ControlPlayers failed.\n");
    
	Server_ManageBots(Server);				// Add and remove bots
	
	//BEGIN_TIMER();	
	if (!SendPlayersToClients(Server))		// Send the updated players to the clients
		GenVS_Error("Server_Frame:  SendPlayersToClients failed.\n");
	//END_TIMER(Server->GMgr);
	
	// Print client pings
	{
		Server_Client	*Client;
		int32			i, t;

		Client = Server->Clients;

		t = 0;

		for (i = 0; i< NETMGR_MAX_CLIENTS; i++, Client++)
		{
			int32		k;

			if (!Client->Active)
				continue;

			t++;

			Client->Ping = 0.0f;

			for (k=0; k< 10; k++)
				Client->Ping += Client->Pings[k];

			Client->Ping *= (1.0f/10.0f);

			if (ShowStats)
				geEngine_Printf(GameMgr_GetEngine(Server->GMgr), 2, 140+t*15, "Client: %s, Ping: %2.2f", Client->Name, Client->Ping*1000.0f);
		}
	}

#if 0		// FIXME:  put in Server_RenderFrame!!!
{
	geEngine		*Engine;

	Engine = GameMgr_GetEngine(Server->GMgr);

	geEngine_Printf(Engine, 2, 15*2 , "SpawnTime   : %3i", Server->NetStats.NumSpawnTime);
	geEngine_Printf(Engine, 2, 15*3 , "ViewFlags   : %3i", Server->NetStats.NumViewFlags);
	geEngine_Printf(Engine, 2, 15*4 , "ViewIndex   : %3i", Server->NetStats.NumViewIndex);
	geEngine_Printf(Engine, 2, 15*5 , "MotionIndex : %3i", Server->NetStats.NumMotionIndex);
	geEngine_Printf(Engine, 2, 15*6 , "FxFlags     : %3i", Server->NetStats.NumFxFlags);
	geEngine_Printf(Engine, 2, 15*7 , "Pos         : %3i", Server->NetStats.NumPos);
	geEngine_Printf(Engine, 2, 15*8 , "Angles      : %3i", Server->NetStats.NumAngles);
	geEngine_Printf(Engine, 2, 15*9 , "FrameTime   : %3i", Server->NetStats.NumFrameTime);
	geEngine_Printf(Engine, 2, 15*10, "Scale       : %3i", Server->NetStats.NumScale);
	geEngine_Printf(Engine, 2, 15*11, "Velocity    : %3i", Server->NetStats.NumVelocity);
	geEngine_Printf(Engine, 2, 15*12, "State       : %3i", Server->NetStats.NumState);
	geEngine_Printf(Engine, 2, 15*13, "ControlIndex: %3i", Server->NetStats.NumControlIndex);
	geEngine_Printf(Engine, 2, 15*14, "TriggerIndex: %3i", Server->NetStats.NumTriggerIndex);
	geEngine_Printf(Engine, 2, 15*15, "MinsMaxs    : %3i", Server->NetStats.NumMinsMaxs);
	geEngine_Printf(Engine, 2, 15*16, "BytesToSend : %3i", Server->NetStats.NumBytesToSend);
}
#endif
	
	return GE_TRUE;
}

//=====================================================================================
//	FindClient
//=====================================================================================
static Server_Client *FindClient(Server_Server *Server, geCSNetMgr_NetID ClientID)
{
	int32			i;
	Server_Client	*Client;

	Client = Server->Clients;

	for (i=0; i< NETMGR_MAX_CLIENTS; i++, Client++)
	{
		if (!Client->Active)
			continue;

		if (Client->NetID == ClientID)
			return Client;
	}

	return NULL;
}

//=====================================================================================
//	ParseClientMove
//=====================================================================================
static void ParseClientMove(Server_Server *Server, Buffer_Data *Buffer, Server_Client *Client, float Time)
{
	float		DeltaTime, MoveTime, NetTime;
	geVec3d		Origin = {0.0f, 0.0f, 0.0f};

	Buffer_GetFloat(Buffer, &MoveTime);
	Buffer_GetFloat(Buffer, &NetTime);					// For getting pings for throttling messages to this client
	Buffer_GetFloat(Buffer, &Client->ForwardSpeed);
	Buffer_GetFloat(Buffer, &Client->Angles.X);
	Buffer_GetFloat(Buffer, &Client->Angles.Y);
	Buffer_GetShort(Buffer, &Client->ButtonBits);

	if (NetTime >= 0)	// -1 represents that the client did not get an update that frame...
	{
		float	Ping;

		Ping = GameMgr_GetTime(Server->GMgr) - NetTime;

		if (Ping >= 0)
		{
			Client->Pings[Client->NumPings%10] = Ping;
			Client->NumPings++;
		}
	}
	
	if (Client->ButtonBits & HOST_BUTTON_FIRE)
		Buffer_GetShort(Buffer, (uint16*)&Client->CurrentWeapon);		// Read Current Weapon if firing

#ifdef CALC_ERROR
	Buffer_GetAngle(Buffer, &Client->Pos);
#endif

	if (!GameMgr_GetWorld(Server->GMgr))
		return;

	Client->OldMoveTime = Client->MoveTime;
	Client->MoveTime = MoveTime;

	if (MoveTime <= Client->OldMoveTime)
		return;

	// Since the client is going to be controlled in the main loop, pull off the amount
	// of time it is going to move it from this time.  
	DeltaTime = Client->MoveTime - Client->OldMoveTime;

	if (DeltaTime <= 0.0f)		// Ignore older messages (this will lose button bits every now and then...)
		return;
	
	if (DeltaTime > 0.1f)
		DeltaTime = 0.1f;
	else if (DeltaTime < 0.001f)
		DeltaTime = 0.001f;

#ifdef CALC_ERROR	
	if (!geVec3d_Compare(&Client->Pos, &Origin, 0.1f))
	{
		geVec3d		Vect;

		geVec3d_Subtract(&Client->Player->XForm.Translation, &Client->Pos, &Vect);

		if (geVec3d_Length(&Vect) > 1.0f)
		{
			Client->Player->OldPos.X = 0.0f;
			geEngine_Printf(Server->Host->Engine, 10, 100, "Client prediction error.");
		}
	}
#endif

	if (Client->Player)		// Must control the client now, with these move intentions...
	{	
		Client->Player->SpawnTime = Client->MoveTime;					// Set this now, so weapons spawned by this move can
		Client->Player->PingTime = Client->Ping;
																		// Be time stamped...
		//Client->Player->Control(&Server->GenVSI, Client->Player, DeltaTime);
		ControlPlayer(Server, Client->Player, DeltaTime);
		
		ForceServerPlayerOnLocalClient(Server, Client->Player);
	}
	else
		GenVS_Error("ParseClientMove:  No player for client!!!");
}
				
//=====================================================================================
//	ParseClientMessage
//=====================================================================================
static geBoolean ParseClientMessage(Server_Server *Server, geCSNetMgr_NetID ClientID, Buffer_Data *Buffer, float Time)
{
	uint8			Type;
	Server_Client	*Client;

	Client = FindClient(Server, ClientID);

	if (!Client)
	{
		//GenVS_Error("Server_ParseClientMessage:  FindClient failed.\n");
		Console_Printf(GameMgr_GetConsole(Server->GMgr), "ParseClientMessage:  FindClient failed.\n");
		return GE_TRUE;		// Ignore??
	}

	if (!Client->Player)		
		return GE_TRUE;			// The client spawn function has not beed called yet in the game code...

	Buffer->Pos = 0;

	// Keep reading till end of buffer
	while (Buffer->Pos < Buffer->Size)
	{
		//geVec3d	Angles;

		Buffer_GetByte(Buffer, &Type);

		switch(Type)
		{
			case NETMGR_MSG_CLIENT_MOVE:
			{
				ParseClientMove(Server, Buffer, Client, Time);
				break;
			}	

			case NETMGR_MSG_CLIENT_CONFIRM:
			{
				NetMgr_NetState		NetState;

				Buffer_GetSLong(Buffer, &NetState);

				//Console_Printf(GameMgr_GetConsole(Server->GMgr), "Client %s, confirmed state: %i\n", Client->Name, NetState);

				// This NetState has been confirmed
				Client->NetStateConfirmed[NetState] = GE_TRUE;

				break;
			}	

			default:
			{
				GenVS_Error("Server_ParseClientMessage:  Invalid msg type.\n");
			}
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//	ReadClientMessages
//=====================================================================================
static geBoolean ReadClientMessages(Server_Server *Server, float Time)
{
	Buffer_Data				Buffer;
	geCSNetMgr_NetID		ClientId;
	geCSNetMgr_NetMsgType	MsgType;
	geCSNetMgr_NetClient	*pGEClient;

	while (1)		// Keep reading messages till none left...
	{
		if (!NetMgr_ReceiveClientMessage(Server->NMgr, &MsgType, &ClientId, &Buffer))
			GenVS_Error("ReadClientMessages:  NetMgr_ReceiveClientMessage failed.\n");

		if (MsgType == NET_MSG_NONE)
			break;

		assert(Buffer.Size > 0);

		switch(MsgType)
		{
			case NET_MSG_CREATE_CLIENT:
			{
				pGEClient = (geCSNetMgr_NetClient*)Buffer.Data;

				if (!Server_ClientConnect(Server, pGEClient))
					GenVS_Error("Could not add client...\n");

				break;
			}
				
			case NET_MSG_DESTROY_CLIENT:
			{

				pGEClient = (geCSNetMgr_NetClient*)Buffer.Data;

				Server_ClientDisconnect(Server, pGEClient->Id, pGEClient->Name);
				break;
			}

			case NET_MSG_USER:
			{
				if (!ParseClientMessage(Server, ClientId, &Buffer, Time))
					GenVS_Error("ReadClientMessages:  ParseClientMessage failed.\n");
				break;
			}

			case NET_MSG_HOST:
			{
				break;
			}

			case NET_MSG_SESSIONLOST:
			{
				GenVS_Error("ReadClientMessages:  Session was lost.\n");
			}

			default:
				//GenVS_Error("ReadClientMessages:  Unknown msg: %i.\n", MsgType);
				break;
		}
	}

	return GE_TRUE;
}

#define ANGLE_EPSILON		0.0001f
#define POS_EPSILON			0.0001f
#define VELOCITY_EPSILON	0.0001f
#define MIN_MAX_EPSILON		0.001f

//=====================================================================================
//	GetPlayerSendFlags
//=====================================================================================
static uint16 GetPlayerSendFlags(Server_Server *Server, GPlayer *Player)
{
	uint16 SendFlags;

	assert(Server);
	assert(Player);

	// Make sure Player->Pos and Angles are valid (These are temporary data that are used to send data between server and client)
	geXForm3d_GetEulerAngles(&Player->XForm, &Player->Angles);
	Player->Pos = Player->XForm.Translation;

	SendFlags = 0;		// Start out with nothing...

	if (Player->SpawnTime != Player->OldSpawnTime)
	{
		SendFlags |= NETMGR_SEND_SPAWN_TIME;
		Player->OldSpawnTime = Player->SpawnTime;
		Server->NetStats.NumSpawnTime++;
	}

	if (Player->ViewFlags != Player->OldViewFlags)
	{
		SendFlags |= NETMGR_SEND_VIEW_FLAGS;
		Player->OldViewFlags = Player->ViewFlags;
		Server->NetStats.NumViewFlags++;
	}

	if (Player->ViewIndex != Player->OldViewIndex)
	{
		SendFlags |= NETMGR_SEND_VIEW_INDEX;
		Player->OldViewIndex = Player->ViewIndex;
		Server->NetStats.NumViewIndex++;
	}

	if (Player->MotionIndex != Player->OldMotionIndex)
	{
		SendFlags |= NETMGR_SEND_MOTION_INDEX;
		Player->OldMotionIndex = Player->MotionIndex;
		Server->NetStats.NumMotionIndex++;
	}

	if (Player->FxFlags != Player->OldFxFlags)
	{
		SendFlags |= NETMGR_SEND_FX_FLAGS;
		Player->OldFxFlags = Player->FxFlags;
		Server->NetStats.NumFxFlags++;
	}

	if (!geVec3d_Compare(&Player->OldPos, &Player->Pos, POS_EPSILON))
	{
		SendFlags |= NETMGR_SEND_POS;
		Player->OldPos = Player->Pos;
		Server->NetStats.NumPos++;
	}

	if (!geVec3d_Compare(&Player->OldAngles, &Player->Angles, ANGLE_EPSILON))
	{
		SendFlags |= NETMGR_SEND_ANGLES;
		Player->OldAngles = Player->Angles;
		Server->NetStats.NumAngles++;
	}

	if (Player->OldFrameTime != Player->FrameTime)
	{
		SendFlags |= NETMGR_SEND_FRAME_TIME;
		Player->OldFrameTime = Player->FrameTime;
		Server->NetStats.NumFrameTime++;
	}

	if (Player->Scale != Player->OldScale)
	{
		SendFlags |= NETMGR_SEND_SCALE;
		Player->OldScale = Player->Scale;
		Server->NetStats.NumScale++;
	}

	if (!geVec3d_Compare(&Player->OldVelocity, &Player->Velocity, VELOCITY_EPSILON))
	{
		SendFlags |= NETMGR_SEND_VELOCITY;
		Player->OldVelocity = Player->Velocity;
		Server->NetStats.NumVelocity++;
	}

	if (Player->OldState != Player->State)
	{
		SendFlags |= NETMGR_SEND_STATE;
		Player->OldState = Player->State;
		Server->NetStats.NumState++;
	}

	if (Player->OldControlIndex != Player->ControlIndex)
	{
		Player->OldControlIndex = Player->ControlIndex;
		SendFlags |= NETMGR_SEND_CONTROL_INDEX;
		Server->NetStats.NumControlIndex++;
	}

	if (Player->OldTriggerIndex != Player->TriggerIndex)
	{
		SendFlags |= NETMGR_SEND_TRIGGER_INDEX;
		Player->OldTriggerIndex = Player->TriggerIndex;
		Server->NetStats.NumTriggerIndex++;
	}

	if (!geVec3d_Compare(&Player->OldMins, &Player->Mins, MIN_MAX_EPSILON) || !geVec3d_Compare(&Player->OldMaxs, &Player->Maxs, MIN_MAX_EPSILON))
	{
		SendFlags |= NETMGR_SEND_MINS_MAXS;
		Player->OldMins = Player->Mins;
		Player->OldMaxs = Player->Maxs;
		Server->NetStats.NumMinsMaxs++;
	}

	return SendFlags;
}

//=====================================================================================
//	FillBufferWithPlayerData
//=====================================================================================
static void FillBufferWithPlayerData(Server_Server *Server, Buffer_Data *Buffer, GPlayer *Player, uint16 SendFlags)
{
	uint16		Index;

	assert(Server);
	assert(Buffer);
	assert(Player);

	Index = (uint16)SERVER_GPLAYER_TO_INDEX(Server, Player);

	assert(Index < (65535>>1));

	//SendFlags = 0xffff;		// For debugging

	if (SendFlags)
		Index |= (1<<15);
	
	Buffer_FillShort(Buffer, Index);

	if (!SendFlags)
		return;

	// Write out what we are sending
	Buffer_FillShort(Buffer, SendFlags);

	// Make sure Player->Pos and Angles are valid (These are temporary data that are used to send data between server and client)
	geXForm3d_GetEulerAngles(&Player->XForm, &Player->Angles);
	Player->Pos = Player->XForm.Translation;

	// Write out the real data
	if (SendFlags & NETMGR_SEND_SPAWN_TIME)
		Buffer_FillFloat(Buffer, Player->SpawnTime);

	if (SendFlags & NETMGR_SEND_VIEW_FLAGS)
		Buffer_FillShort(Buffer, Player->ViewFlags);

	if (SendFlags & NETMGR_SEND_VIEW_INDEX)
		Buffer_FillShort(Buffer, Player->ViewIndex);

	if (SendFlags & NETMGR_SEND_MOTION_INDEX)
		Buffer_FillByte(Buffer, Player->MotionIndex);

	if (SendFlags & NETMGR_SEND_FX_FLAGS)
		Buffer_FillShort(Buffer, Player->FxFlags);

	if (SendFlags & NETMGR_SEND_POS)
		//Buffer_FillPos2(Buffer, Player->Pos);
		Buffer_FillPos(Buffer, Player->Pos);

	if (SendFlags & NETMGR_SEND_ANGLES)
		Buffer_FillAngle(Buffer, Player->Angles);
	
	if (SendFlags & NETMGR_SEND_FRAME_TIME)
		//Buffer_FillFloat(Buffer, Player->FrameTime);
		Buffer_FillFloat2(Buffer, Player->FrameTime, 60.0f);

	if (SendFlags & NETMGR_SEND_SCALE)
		//Buffer_FillFloat(Buffer, Player->Scale);
		Buffer_FillFloat2(Buffer, Player->Scale, 100.0f);

	if (SendFlags & NETMGR_SEND_VELOCITY)
		Buffer_FillAngle(Buffer, Player->Velocity);

	if (SendFlags & NETMGR_SEND_STATE)
	{
		assert(Player->State >= 0 && Player->State <= 255);

		Buffer_FillByte(Buffer, (uint8)Player->State);
	}

	if (SendFlags & NETMGR_SEND_CONTROL_INDEX)
		Buffer_FillShort(Buffer, Player->ControlIndex);

	if (SendFlags & NETMGR_SEND_TRIGGER_INDEX)
		Buffer_FillShort(Buffer, Player->TriggerIndex);

	if (SendFlags & NETMGR_SEND_MINS_MAXS)
	{
		Buffer_FillPos(Buffer, Player->Mins);
		Buffer_FillPos(Buffer, Player->Maxs);
	}

#if 0
	// TOTAL hack for now... (this is a way to get extra xforms to control extra bones...
	if (Player->ViewFlags & VIEW_TYPE_HACK)
	{
		GPlayer_XFormData	*pXFormData;
		int32				i;

		assert(Player->NumXFormData <= 255 && Player->NumXFormData < GPLAYER_MAX_XFORM_DATA);

		Buffer_FillByte(Buffer, (uint8)Player->NumXFormData);

		pXFormData = Player->XFormData;
		
		for (i=0; i<Player->NumXFormData; i++, pXFormData++)
		{
			geVec3d		EulerAngles;

			geXForm3d_GetEulerAngles(&pXFormData->XForm, &EulerAngles);
			Buffer_FillAngle(Buffer, EulerAngles);
			Buffer_FillPos(Buffer, pXFormData->XForm.Translation);
			Buffer_FillByte(Buffer, pXFormData->BoneIndex);
		}
		
	}
	
	// TOTAL hack for now... (this is a way to get extra motions in a player for actors
	if (Player->ViewFlags & VIEW_TYPE_HACK)
	{
		GPlayer_MotionData	*pMotionData;
		int32				i;

		assert(Player->NumMotionData <= 255 && Player->NumMotionData < GPLAYER_MAX_MOTION_DATA);

		Buffer_FillByte(Buffer, (uint8)Player->NumMotionData);

		pMotionData = Player->MotionData;

		for (i=0; i<Player->NumMotionData; i++, pMotionData++)
		{
			Buffer_FillFloat(Buffer, pMotionData->MotionTime);
			//Buffer_FillFloat2(Buffer, pMotionData->MotionTime, 60.0f);
			Buffer_FillByte(Buffer, pMotionData->MotionIndex);
		}
	}
	
#endif

}

//=====================================================================================
//	SetupClientPlayerSendFlags
//	Merges what needs to be change for this frame into each player that belongs to the clients
//	This way they can be sent independent for each client, with out conflicting with one another...
//	When the clients send the players out, they simply clear their sendflags for that player...
//=====================================================================================
static void SetupClientPlayerSendFlags(Server_Server *Server)
{
	GPlayer		*Player;
	int32		i;

	Player = Server->SvPlayers;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++, Player++)
	{
		uint16			SendFlags;
		int32			k;
		Server_Client	*Client;

		if (!Player->Active)
			continue;
		
		if (Player->ViewFlags & VIEW_TYPE_LOCAL)
			continue;	// Only on local server, don't send data accros net

		if (!Player->Control)
			continue;

		SendFlags = GetPlayerSendFlags(Server, Player);

		Client = Server->Clients;

		// Or what needs to be changed about this player into all the clients...
		for (k=0; k< NETMGR_MAX_CLIENTS; k++, Client++)
		{
			if (!Client->Active)
				continue;

			Client->SendFlags[i] |= SendFlags;
		}
	}
}

#if 1
static int32	NumData;
#endif

//=====================================================================================
//	SendPlayersToClients
//=====================================================================================
static geBoolean SendPlayersToClients(Server_Server *Server)
{

#if 1		// Optimized for internet gameplay (can use on LAN just fine too)
	int32			i;
	uint8			Data[20000];
	Buffer_Data		Buffer;
	Server_Client	*Client;
	geWorld			*World;

	World = GameMgr_GetWorld(Server->GMgr);

	if (!World)
		return GE_TRUE;

	SetupClientPlayerSendFlags(Server);

	Client = Server->Clients;

	// Tell the host to route this message to the clients
	for (i=0; i< NETMGR_MAX_CLIENTS; i++, Client++)
	{
		GPlayer			*Player;
		int32			k;
		geVec3d			Pos1[5];
		geVec3d			Pos2;
		geVec3d			In;
		int32			Leaf1[5], Leaf2;
		float			NextUpdate, Ping, GTime;

		if (!Client->Active)
			continue;

		// Don't send the players to the client until he has confirmed that the world was loaded
		if (!(Client->NetState == NetState_WorldActive && Client->NetStateConfirmed[NetState_WorldActive]))
			continue;

		// See if this client needs an update
		GTime = GameMgr_GetTime(Server->GMgr);

		if (GTime < Client->NextUpdate)		// Send time only
			continue;

		// Setup the buffer
		Buffer_Set(&Buffer, (char*)Data, 20000);
	
		// All player updates have the Servers current time for pings, and client side interpolation...
		Buffer_FillByte(&Buffer, NETMGR_MSG_TIME);
		Buffer_FillFloat(&Buffer, GTime);
		Buffer_FillFloat(&Buffer, Client->Ping);

		Player = Server->SvPlayers;

		// Get client position info
		Pos1[0] = Client->Player->VPos;
		Pos1[1] = Client->Player->VPos;

		Pos1[1].Y += 120.0f;

		geXForm3d_GetIn(&Client->Player->XForm, &In);
		geVec3d_AddScaled(&Pos1[1], &In, 450.0f, &Pos1[1]);

		geWorld_GetLeaf(World, &Pos1[0], &Leaf1[0]);
		geWorld_GetLeaf(World, &Pos1[1], &Leaf1[1]);

		for (k=0; k< NETMGR_MAX_PLAYERS; k++, Player++)
		{
			GE_Collision	Collision;

			if (!Player->Active)
				continue;

			if (Player->ViewFlags & VIEW_TYPE_LOCAL)
				continue;	// Only on local server, don't send data accros net

			if (!Player->Control)
				continue;

			Pos2 = Player->VPos;

			geWorld_GetLeaf(World, &Pos2, &Leaf2);
			
		#if 1	
			//if (Client != &Server->Clients[0])
			//if (i != 0)
			if (Player != Client->Player)	// Make sure we allways send the client player
			{
				if (!geWorld_LeafMightSeeLeaf(World, Leaf2, Leaf1[0], 0))
				if (!geWorld_LeafMightSeeLeaf(World, Leaf2, Leaf1[1], 0))
				{
					// For client 0 (the local client) keep the player alive.
					// For all other clients, we can destroy them while not on the screen...
					if (i == 0)
					{
						Buffer_FillByte(&Buffer, NETMGR_MSG_PLAYER_DATA);
						FillBufferWithPlayerData(Server, &Buffer, Player, 0);
					}
					else
						Client->SendFlags[k] = 0xffff;		// Force an update w hen it comes back into view...

					continue;
				}
						  			
			
				// Fill the buffer, with the needed data of this player on this client...
				if (Player->ClientHandle == -1)
				if (!Player->Model)
				if (geWorld_Collision(World, NULL, NULL, &Pos1[0], &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS | GE_COLLIDE_NO_SUB_MODELS, 0, NULL, NULL, &Collision))
				if (geWorld_Collision(World, NULL, NULL, &Pos1[1], &Pos2, GE_CONTENTS_CANNOT_OCCUPY, GE_COLLIDE_MODELS | GE_COLLIDE_NO_SUB_MODELS, 0, NULL, NULL, &Collision))
				{
					// If can't see, then don't send
					if (i==0)
					{
						Buffer_FillByte(&Buffer, NETMGR_MSG_PLAYER_DATA);
						FillBufferWithPlayerData(Server, &Buffer, Player, 0);
					}
					else
						Client->SendFlags[k] = 0xffff;		// Force an update w hen it comes back into view...

					continue;
				}
			}
		#endif
			
			//geEngine_Printf(Host->Engine, 2, 20, "Bytes Received: %i", TotalMsgBytes);

			// Send'er over...
			Buffer_FillByte(&Buffer, NETMGR_MSG_PLAYER_DATA);
			FillBufferWithPlayerData(Server, &Buffer, Player, Client->SendFlags[k]);
			Client->SendFlags[k] = 0;		// All sent out now for this player/client...
		}

		Ping = Client->Ping * 1000.0f;

		// Based on Ping, see when this client should get another update...
		if (Ping < 100.0f)						
			NextUpdate = Client->Ping*0.1f;				// LAN, T3
		else if (Ping < 300.0f)
			NextUpdate = Client->Ping*0.15f;				// Pretty good modem play
	
		else if (Ping < 700.0f)
			NextUpdate = Client->Ping*0.2f;			// Ok modem play
		else if (Ping < 1000.0f)
			NextUpdate = Client->Ping*0.25f;				// poor modem play
		else if (Ping < 2500.0f)
			NextUpdate = Client->Ping*0.3f;			// This person better log off!!!
		else 
			NextUpdate = 1.0f;		// Wait 1 1/2 secs for things to clear out...
		
		// Store the next update time...
		Client->NextUpdate = GTime + NextUpdate;

		Server->NetStats.NumBytesToSend += Buffer.Pos;

	#if 0
		if (i != 0)
		{
			geEngine_Printf(GameMgr_GetEngine(Server->GMgr), 3, 65, "Sending client data: %i", NumData);
			NumData += Buffer.Pos;
		}
	#endif

		// Rip out the message to this client
		if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, FALSE))
			Server_ClientDisconnect(Server, Client->NetID, Client->Name);
	}
#else
	int32			i;
	uint8			Data[20000];
	Buffer_Data		Buffer;
	GPlayer			*Player;
	uint16			SendFlags;

	Buffer_Set(&Buffer, Data, 20000);
	
	Buffer_FillByte(&Buffer, NETMGR_MSG_TIME);
	Buffer_FillFloat(&Buffer, Host->Time);
	Buffer_FillFloat(&Buffer, 0.0f);

	// Fill the buffer with all the players at once
	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		Player = &Server->SvPlayers[i];

		// Don't bother sending non active players, or players without entities...
		if (!Player->Active)
			continue;

		if (Player->ViewFlags & VIEW_TYPE_LOCAL)
			continue;	// Only on local server, don't send data accros net

		if (!Player->Control)
			continue;

		assert(i >=0 && i <= 65535);

		// We must send this data
		Buffer_FillByte(&Buffer, NETMGR_MSG_PLAYER_DATA);

		SendFlags = GetPlayerSendFlags(Server, Player);

		// Fill the buffer, with the needed data...
		FillBufferWithPlayerData(Server, &Buffer, Player, SendFlags);
	}
	
	// Tell the host to route this message to the clients
	for (i=0; i< NETMGR_MAX_CLIENTS; i++)
	{
		if (!Server->Clients[i].Active)
			continue;

		if (!NetMgr_SendClientMessage(Server->NMgr, Server->Clients[i].NetID, &Buffer, FALSE))
			GenVS_Error("SendPlayersToClients:  NetMgr_SendClientMessage failed.\n");

		Server->NetStats.NumBytesToSend += Buffer.Pos;
	}

#endif
	return GE_TRUE;
}

//====================================================================================
//====================================================================================
static	void	DrawFace(geWorld *World, const geVec3d **Verts)
{
	GE_LVertex	LVerts[4];
	int			i;

	for	(i = 0; i < 4; i++)
	{
		LVerts[i].r = 40.0f;
		LVerts[i].g = 40.0f;
		LVerts[i].b = 80.0f + 20.0f * (geFloat)i;
		LVerts[i].a = 128.0f;
		LVerts[i].X = Verts[i]->X;
		LVerts[i].Y = Verts[i]->Y;
		LVerts[i].Z = Verts[i]->Z;
	}

	geWorld_AddPolyOnce(World, &LVerts[0], 4, NULL, GE_GOURAUD_POLY, GE_RENDER_DO_NOT_OCCLUDE_OTHERS, 1.0f);
}

void	DrawBoundBox(geWorld *World, const geVec3d *Pos, const geVec3d *Min, const geVec3d *Max)
{
	geFloat	dx;
	geFloat	dy;
	geFloat	dz;
static	geVec3d		Verts[8];
static	geVec3d *	Faces[6][4] =
{
	{ &Verts[0], &Verts[1], &Verts[2], &Verts[3] },	//Top
	{ &Verts[4], &Verts[5], &Verts[6], &Verts[7] },	//Bottom
	{ &Verts[3], &Verts[2], &Verts[6], &Verts[7] }, //Side
	{ &Verts[1], &Verts[0], &Verts[4], &Verts[5] }, //Side
	{ &Verts[0], &Verts[3], &Verts[7], &Verts[4] }, //Front
	{ &Verts[2], &Verts[1], &Verts[5], &Verts[6] }, //Back
};
	int			i;

	for	(i = 0; i < 8; i++)
		geVec3d_Add(Pos, Min, &Verts[i]);

	dx = Max->X - Min->X;
	dy = Max->Y - Min->Y;
	dz = Max->Z - Min->Z;

	Verts[0].Y += dy;
	Verts[3].Y += dy;
	Verts[3].X += dx;
	Verts[7].X += dx;

	Verts[1].Y += dy;
	Verts[1].Z += dz;
	Verts[5].Z += dz;
	Verts[6].Z += dz;
	Verts[6].X += dx;

	Verts[2].X += dx;
	Verts[2].Y += dy;
	Verts[2].Z += dz;

	for	(i = 0; i < 6; i++)
		DrawFace(World, const_cast<const geVec3d**>(&Faces[i][0]));
}

//=====================================================================================
//	ForceServerPlayerOnClient
//	Sends the server player directly to the local client...
//=====================================================================================
static void ForceServerPlayerOnLocalClient(Server_Server *Server, GPlayer *Player)
{
	GPlayer			*ClientPlayer;
	int32			PlayerIndex;
	Buffer_Data		Buffer;
	uint8			Data[1024];

	assert(Server);
	assert(Server->Client);
	assert(Player);

	if (!GameMgr_GetWorld(Server->GMgr))		// Don't do nothing with genesis until world is loaded
		return;

	if (!Player->Active)
		return;

	PlayerIndex = (int32)SERVER_GPLAYER_TO_INDEX(Server, Player);

	assert(PlayerIndex >= 0 && PlayerIndex < NETMGR_MAX_PLAYERS);

	// Get a pointer to the client version of this player on the server
	//ClientPlayer = Client_GetPlayerForIndex(Server->Host->Client, PlayerIndex);
	ClientPlayer = &Server->Client->Players[PlayerIndex];

	Buffer_Set(&Buffer, (char*)Data, 1024);
	
	// Fill the buffer with the player data
	FillBufferWithPlayerData(Server, &Buffer, Player, 0xffff);

	// FIXME:  Call Buffer_Reset
	Buffer.Pos = 0;

	Client_ParsePlayerData(Server->Client, &Buffer, GE_FALSE);

	// Override some important ones...
	ClientPlayer->Pos = Player->Pos;
	ClientPlayer->Angles = Player->Angles;

	// Make the client update this player NOW
	Client_UpdateSinglePlayer(Server->Client, ClientPlayer, 1.0f, GameMgr_GetTime(Server->GMgr), GE_FALSE);
	
}


//=====================================================================================
//	ControlPlayer
//=====================================================================================
static void ControlPlayer(Server_Server *Server, GPlayer *Player, float Time)
{
	assert (Player->Control);

	if (!Player->Control)
		return;

	// Force an update now, so the player will be in the correct location for collision, etc...
	//ForceServerPlayerOnLocalClient(Server, Player);

	if (!Player->Control(&Server->GenVSI, Player, Time))
		GenVS_Error("[SERVER] ControlPlayer:  Failed.");

	geXForm3d_GetEulerAngles(&Player->XForm, &Player->Angles);
	Player->Pos = Player->XForm.Translation;

	// Force an update now, so the player will be in the correct location for collision, etc...
	//ForceServerPlayerOnLocalClient(Server, Player);
}

//=====================================================================================
//	ControlPlayers
//=====================================================================================
static geBoolean ControlPlayers(Server_Server *Server, float Time)
{
	GPlayer     		*Player;
	int32				i;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		Player = &Server->SvPlayers[i];

		if (!Player->Active)
			continue;
		
		if (!Player->Control)		// If no control set, then don't worry about it...
			continue;
		
		if (Player->ClientHandle != CLIENT_NULL_HANDLE && !Server_IsClientBot(Server, Player->ClientHandle))
		{
			
			geXForm3d_GetEulerAngles(&Player->XForm, &Player->Angles);
			Player->Pos = Player->XForm.Translation;

			// Force an update now, so the player will be in the correct location for collision, etc...
			ForceServerPlayerOnLocalClient(Server, Player);
			
			continue;
		}
		
		// If the players ping time is greater than 0, than he got spawned from a client.
		// Play out all the clients ping first, so we will be in the correct location for the 
		// client on his machine.  Everyone else will suffer though...
		if (Player->PingTime > 0)		// Guess where it will be on the poor client who created this player
		{
			float		Val;

			Val = Player->PingTime * (1.0f/20.0f);		// Play through in 20 ms intervals
			//Val = Player->PingTime / Val;
			
			// Drain all the ping time (that belonged to the client that spawned this player) out of the player
			while (Player->PingTime > 0.0f && Player->Active)
			{
				ControlPlayer(Server, Player, Val);

				Player->PingTime -= Val;
			}

			// Reset ping time so it won't get used unless initialized again by a move intention...
			Player->PingTime = -1.0f;
		}
		else
			ControlPlayer(Server, Player, Time);

		// Force an update now, so the player will be in the correct location for collision, etc...
		ForceServerPlayerOnLocalClient(Server, Player);
	}

	return GE_TRUE;
}

//=====================================================================================
//	Server_CountBots
//=====================================================================================
static int32 Server_CountBots(Server_Server *Server)
{
	GPlayer     		*Player;
	int32				i, Count;

	for (i=Count=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		Player = &Server->SvPlayers[i];

		if (!Player->Active)
			continue;
		
		if (!Player->Control)
			continue;
		
		if (Player->ClientHandle != CLIENT_NULL_HANDLE && Server_IsClientBot(Server, Player->ClientHandle))
		{
        Count++;
		}
	}

    return Count;
}

//=====================================================================================
//	Server_GetBotByName
//=====================================================================================
static GenVSI_CHandle Server_GetBotByName(Server_Server *Server, char *Name)
{
	GPlayer     		*Player;
	int32				i, Count;
	Server_Client		*SClient;

	for (i=Count=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		Player = &Server->SvPlayers[i];

		if (!Player->Active)
			continue;
		
		if (!Player->Control)
			continue;
		
		if (Player->ClientHandle != CLIENT_NULL_HANDLE && Server_IsClientBot(Server, Player->ClientHandle))
			{
			SClient = &Server->Clients[Player->ClientHandle];

			if (strcmp(SClient->Name, Name) == 0)
				return (Player->ClientHandle);
			}
	}

    return CLIENT_NULL_HANDLE;
}


//=====================================================================================
//	Server_ManageBots
//=====================================================================================
static geBoolean Server_ManageBots(Server_Server *Server)
{
    int cnt;
    int i;
	extern int32 MenuBotCount;
	Server_Client *SClient;
	int32 Index;

	static int32 RandomSet = -1;

	static char *BotNames[4][4] = {
		{"Masque",		"Velvet",	"M&M",				"Mellow"},
		{"Red Death",	"Royal",	"Yellow Streak",	"Green Machine"},
		{"Anger",		"Envy",		"Melancholy",		"Chicken"},
		{"Scarlet",		"Violet",	"Azure",			"Chartreuse"},
		};

	static GE_RGBA BotLight[4][4] =	{
							{	
							{255.0f, 050.0f, 050.0f, 0.0f}, // Red
							{255.0f, 050.0f, 255.0f, 0.0f}, // Purple
							{000.0f, 255.0f, 000.0f, 0.0f}, // Green
							{255.0f, 229.0f, 094.0f, 0.0f}, // Yellow
							},
							{
							{255.0f, 050.0f, 050.0f, 0.0f}, // Red
							{000.0f, 050.0f, 255.0f, 0.0f}, // Blue
							{255.0f, 229.0f, 094.0f, 0.0f}, // Yellow
							{000.0f, 255.0f, 000.0f, 0.0f}, // Green
							},
							{
							{255.0f, 050.0f, 050.0f, 0.0f}, // Red
							{000.0f, 255.0f, 000.0f, 0.0f}, // Green
							{000.0f, 050.0f, 255.0f, 0.0f}, // Blue
							{255.0f, 229.0f, 094.0f, 0.0f}, // Yellow
							},
							{	
							{255.0f, 050.0f, 050.0f, 0.0f}, // Red
							{255.0f, 050.0f, 255.0f, 0.0f}, // Purple
							{000.0f, 050.0f, 255.0f, 0.0f}, // Blue
							{176.0f, 232.0f, 082.0f, 0.0f}, // Chartruese
							},
							};


	if (RandomSet == -1)
		{
		srand((unsigned)time(NULL));
		RandomSet = rand() % 4; // probably should be done in startup code but this
								// will keep it more seperated
		}

	if (ServerBotCount == MenuBotCount)
		return GE_TRUE;

    cnt = Server_CountBots(Server);

    if (cnt < MenuBotCount)
        {
        for (i = cnt; i < MenuBotCount; i++)
            {
		    if (!(SClient = Server_BotConnect(Server, BotNames[RandomSet][i])))
				{
			    GenVS_Error("Server_ManageBots:  Could not add BOT client in single player mode...\n");
				}
			else
				{
				if (!SClient->Active)
					continue;

				Index = SClient->Player - Server->SvPlayers;

				if (Server->Client->Players[Index].Actor)
					{
					geVec3d			Normal = {0.0f, 1.0f, 0.0f};

					geActor_SetLightingOptions(	Server->Client->Players[Index].Actor, 
												GE_TRUE, &Normal, 
												BotLight[RandomSet][i].r, 
												BotLight[RandomSet][i].g, 
												BotLight[RandomSet][i].b, 
												1.0f, 1.0f, 1.0f, 
												GE_TRUE, 3, NULL, GE_FALSE);
					}
				}

			ServerBotCount++;
            }
        }
    else
    if (cnt > MenuBotCount)
        {
		GenVSI_CHandle ClientHandle;
        for (i = cnt-1; i >= MenuBotCount; i--)
            {
			if ((ClientHandle = Server_GetBotByName(Server, BotNames[RandomSet][i])) != CLIENT_NULL_HANDLE)
				Server_ClientDisconnectByHandle(Server, ClientHandle);
			ServerBotCount--;
            }
        }

	return GE_TRUE;
}



//=====================================================================================
//	*** Callbacks into GenVSI ***
//=====================================================================================

//=====================================================================================
//	Server_SetClassSpawn
//=====================================================================================
static void Server_SetClassSpawn(void *UData, const char *ClassName, GenVSI_SpawnFunc *Func, GenVSI_DestroyFunc *DFunc)
{
	int32			i;
	Server_Server	*Server;
	
	assert(ClassName);
	assert(strlen(ClassName) < MAX_CLASS_NAME_STRING);
	assert(Func);

	Server = (Server_Server*)UData;

	for (i=0; i< Server->NumClassSpawns; i++)
	{
		if (!strcmp(Server->ClassSpawns[i].Name, ClassName))
			return;
	}

	if (i >= MAX_SERVER_CLASS_SPAWNS)
	{
		GenVS_Error("Server_SetClassSpawn:  No more slots left.\n");
	}

	strcpy(Server->ClassSpawns[i].Name, ClassName);
	Server->ClassSpawns[i].Func = Func;
	Server->ClassSpawns[i].DFunc = DFunc;
	
	Server->NumClassSpawns++;
}

//=====================================================================================
//	Server_SetWorldRequest
//=====================================================================================
static void Server_SetWorldRequest(void *S, GenVSI_NewWorldCB *NewWorldCB, GenVSI_ShutdownWorldCB *ShutdownWorldCB, const char *Name)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(NewWorldCB);
	assert(Name);
	
	strcpy(Server->WorldName, Name);
	Server->ChangeWorldRequest = GE_TRUE;

	Server->NewWorldCB = NewWorldCB;

	// When the server gets the request, it will call ShutdownWorldCB1, then set ShutdownWorldCB1 to 
	// ShutdownWorldCB2, then NULL out ShutdownWorldCB2...
	Server->ShutdownWorldCB2 = ShutdownWorldCB;
}

//=====================================================================================
//	Server_ActorIndex
//=====================================================================================
static void Server_ActorIndex(void *S, uint16 Index, const char *GFXPath, const char *ActorName)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(GFXPath);
	assert(ActorName);
	assert(strlen(GFXPath) < GAMEMGR_MAX_GFX_PATH);
	assert(strlen(ActorName) < GAMEMGR_MAX_ACTOR_NAME);
	assert(Index < GAMEMGR_MAX_ACTOR_INDEX);

	if (!GameMgr_SetActorIndex(Server->GMgr, Index, ActorName))
	{
		// FIXME:  Either use a better error handling convention, or bubble it out to the caller, and make them handle it.
		// This is ok, but I was trying to make it as easy as possible for them, and didn't want to make them handle errors.
		//GameMgr_Error(Server->GMgr, "...");
		GenVS_Error("Server_ActorIndex:  GameMgr_SetActorIndex failed...");
	}
}

//=====================================================================================
//	Server_MotionIndex
//=====================================================================================
static void Server_MotionIndex(void *S, uint16 MotionIndex, const char *MotionName)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	if (!GameMgr_SetMotionIndexDef(Server->GMgr, MotionIndex, MotionName))
		GenVS_Error("Server_MotionIndex:  Could not set motion: %s.\n", MotionName);
}

//=====================================================================================
//	Server_BoneIndex
//=====================================================================================
static void Server_BoneIndex(void *S, uint16 BoneIndex, const char *BoneName)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	if (!GameMgr_SetBoneIndex(Server->GMgr, BoneIndex, BoneName))
		GenVS_Error("Server_BoneIndex:  Could not set Bone Index: %s.\n", BoneName);
}

//=====================================================================================
//	Server_TextureIndex
//=====================================================================================
static void Server_TextureIndex(void *S, uint16 Index, const char *FileName, const char *AFileName)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(FileName);
	assert(strlen(FileName) < GAMEMGR_MAX_FILENAME);

	if (!GameMgr_SetTextureIndex(Server->GMgr, Index, FileName, AFileName))
	{
		GenVS_Error("Server_TextureIndex:  GameMgr_SetTextureIndex failed...");
	}
}

//=====================================================================================
//	Server_SoundIndex
//=====================================================================================
static void Server_SoundIndex(void *S, uint16 Index, const char *FileName)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(FileName);

	if (!GameMgr_SetSoundIndex(Server->GMgr, Index, FileName))
	{
		GenVS_Error("Server_SoundIndex:  GameMgr_SetSoundIndex failed...");
	}
}

//=====================================================================================
//	Server_PlaySound
//=====================================================================================
static void Server_PlaySound(void *S, uint16 SoundIndex, const geVec3d *Pos)
{
	int32			i;
	uint8			Data[1000];
	Buffer_Data		Buffer;
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	Buffer_Set(&Buffer, (char*)Data, 1000);
	
	// Fill the buffer with the message
	Buffer_FillByte(&Buffer, NETMGR_MSG_PLAY_SOUND_INDEX);
	Buffer_FillShort(&Buffer, (uint16)SoundIndex);
	Buffer_FillPos(&Buffer, *Pos);

	Client = Server->Clients;
	
	// Tell the host to route this message to the clients
	for (i=0; i< NETMGR_MAX_CLIENTS; i++, Client++)
	{
		if (!Client->Active)
			continue;

		if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, FALSE))
		{
			Server_ClientDisconnect(Server, Client->NetID, Client->Name);
			//GenVS_Error("Server_PlaySound:  Could not sent message to client.\n");
		}
	}
}

//=====================================================================================
//	Server_SpawnFx
//=====================================================================================
static void Server_SpawnFx(void *S, uint8 Type, const geVec3d *Pos, uint8 Sound)
{
	Buffer_Data		Buffer;
	char			Data[128];
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_SPAWN_FX);
	Buffer_FillPos(&Buffer, *Pos);
	Buffer_FillByte(&Buffer, Type);
	Buffer_FillByte(&Buffer, Sound);
	
	if (!SendAllClientsMessage(Server, &Buffer, GE_FALSE))
		GenVS_Error("Server_SpawnFx:  Could not send mesages to clients.\n");
}

//=====================================================================================
//	Server_SetClientScore
//=====================================================================================
static void Server_SetClientScore(void *S, GenVSI_CHandle ClientHandle, int32 Score)
{
	Buffer_Data		Buffer;
	char			Data[128];
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	assert(ClientHandle >= 0 && ClientHandle <= 255);

	Client = &Server->Clients[ClientHandle];
	
	if (!Client->Active)
	{
		Console_Printf(GameMgr_GetConsole(Server->GMgr), "Server_SetClientScore:  Client not active!\n");
		return;
	}
	
	Client->Score = Score;
	
	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_SCORE);
	Buffer_FillByte(&Buffer, (uint8)ClientHandle);
	Buffer_FillSLong(&Buffer, Client->Score);
	
	if (!SendAllClientsMessage(Server, &Buffer, GE_TRUE))
		GenVS_Error("Server_SetClientScore:  Could not send message.\n");
}

//=====================================================================================
//	Server_IsClientBot
//=====================================================================================
static geBoolean Server_IsClientBot(void *S, GenVSI_CHandle ClientHandle)
{
	Server_Client	*Client;
	Server_Server	*Server = static_cast<Server_Server*>(S);

	assert(Server);

	if (ClientHandle < 0 || ClientHandle > NETMGR_MAX_CLIENTS)
		return GE_FALSE;

	Client = &Server->Clients[ClientHandle];
	
	assert(Client->Active);

	if (Client->NetID == NETMGR_SPECIAL_BOT_NETID)
		return GE_TRUE;

	return GE_FALSE;
}

//=====================================================================================
//	Server_SetClientHealth
//=====================================================================================
static void Server_SetClientHealth(void *S, GenVSI_CHandle ClientHandle, int32 Health)
{
	Buffer_Data		Buffer;
	char			Data[128];
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	assert(ClientHandle >= 0 && ClientHandle <= 255);
	
	Client = &Server->Clients[ClientHandle];

	if (!Client->Active)
	{
		Console_Printf(GameMgr_GetConsole(Server->GMgr), "Server_SetClientScore:  Client not active!\n");
		return;
	}

	Client->Health = Health;
	
	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_HEALTH);
	Buffer_FillByte(&Buffer, (uint8)ClientHandle);
	Buffer_FillSLong(&Buffer, Client->Health);
	
	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
	{
		Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		//GenVS_Error("Server_SetClientHealth:  Could not send message.\n");
	}
}

//=====================================================================================
//	Server_SetClientWeapon
//=====================================================================================
static void Server_SetClientWeapon(void *S, GenVSI_CHandle ClientHandle, int32 Weapon)
{
	Buffer_Data		Buffer;
	char			Data[512];
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	assert(ClientHandle >= 0 && ClientHandle <= 255);
	assert(Weapon >= 0 && Weapon <= 255);

	Client = &Server->Clients[ClientHandle];

	if (!Client->Active)
		GenVS_Error("Server_SetClientWeapon:  Client not active!\n");

	Client->CurrentWeapon = (uint16)Weapon;
	
	Buffer_Set(&Buffer, Data, 512);
	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_WEAPON);
	Buffer_FillByte(&Buffer, (uint8)ClientHandle);
	Buffer_FillShort(&Buffer, (uint16)Client->CurrentWeapon);
	
	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_FALSE))
	{
		Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		//GenVS_Error("Server_SetClientWeapon:  Could not send message.\n");
	}
}

//=====================================================================================
//	Server_SetClientInventory
//=====================================================================================
static void Server_SetClientInventory(void *S, GenVSI_CHandle ClientHandle, int32 Slot, uint16 Amount, geBoolean HasItem)
{
	Buffer_Data		Buffer;
	char			Data[128];
	uint16			SendVal;
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(ClientHandle >= 0 && ClientHandle < NETMGR_MAX_CLIENTS);

	Client = &Server->Clients[ClientHandle];

	if (!Client->Active)
		GenVS_Error("Server_SetClientInventory:  Client not active.");

	assert(Slot < MAX_PLAYER_ITEMS && Slot < 256);

	Client->Inventory[Slot] = Amount;
	Client->InventoryHas[Slot] = HasItem;

	SendVal = Client->Inventory[Slot];

	if (Client->InventoryHas[Slot])
		SendVal |= (1<<15);

	assert(ClientHandle <= 255);

	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_INVENTORY);
	Buffer_FillByte(&Buffer, (uint8)ClientHandle);
	Buffer_FillByte(&Buffer, (uint8)Slot);
	Buffer_FillShort(&Buffer, SendVal);
	
	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
	{
		Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		//GenVS_Error("Server_SetClientInventory:  Could not send message.\n");
	}
}

//=====================================================================================
//	Server_GetClientMove
//=====================================================================================
static GenVSI_CMove *Server_GetClientMove(void *S, GenVSI_CHandle ClientHandle)
{
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);

	Client = &Server->Clients[ClientHandle];

	Client->Move.MoveTime = Client->MoveTime;
	Client->Move.ForwardSpeed = Client->ForwardSpeed;
	Client->Move.Angles = Client->Angles;
	Client->Move.ButtonBits = Client->ButtonBits;
	Client->Move.Pos = Client->Pos;
	Client->Move.Weapon = Client->CurrentWeapon;

	return &Client->Move;
}

//=====================================================================================
//	Server_ModelToViewIndex
//=====================================================================================
static uint16 Server_ModelToViewIndex(void *S, geWorld_Model *Model)
{
	int32		i, NumModels;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(GameMgr_GetWorld(Server->GMgr));

	NumModels = GameMgr_GetNumModels(Server->GMgr);

	for (i=0; i< NumModels; i++)
	{
		if (Model == GameMgr_GetModel(Server->GMgr, i))
			break;
	}

	if (i >= NumModels)
	{
		GenVS_Error("Server_ModelToViewIndex:  Model not in world...");
	}

	return (uint16)i;
}

//=====================================================================================
//	Server_RegisterPlayerModel
//=====================================================================================
static void Server_RegisterPlayerModel(void *S, void *P, geWorld_Model *Model)
{
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer *Player = static_cast<GPlayer*>(P);

	assert(Server);
	assert(Player);
	assert(Model);

	//	Attach the player to the model, and the model to the player...
	geWorld_ModelSetUserData(Model, (void*)Player);
	Player->Model = Model;	
}

//=====================================================================================
//	Server_GetNextPlayer
//=====================================================================================
static void *Server_GetNextPlayer(void *S, void *St, const char *ClassName)
{
	GPlayer	*RealStart, *End;
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer *Start = static_cast<GPlayer*>(St);

	RealStart = &Server->SvPlayers[0];
	End = &Server->SvPlayers[NETMGR_MAX_PLAYERS-1];

	if (Start == End)
		return NULL;

	if (!Start)
		Start = Server->SvPlayers;
	else
		Start++;

	assert(Start >= RealStart && Start <= End);

	if (Start < RealStart || Start > End)
	{
		Console_Printf(GameMgr_GetConsole(Server->GMgr), "[SERVER] Server_GetNextPlayer:  Bad start.\n");
		return NULL;		// Bad!!!
	}

	for ( ; Start < End; Start++)
	{
		if (!Start->Active)
			continue;

		if (!ClassName)		// Just return the next one, if no classname asked for
			return Start;

		if (!strcmp(Start->ClassName, ClassName))	// Found it!!
			return Start;
	}

	return NULL;
}

//=====================================================================================
//	Server_GetNextPlayerInRadius
//=====================================================================================
static void *Server_GetNextPlayerInRadius(void *S, void *P, void *St, const char *ClassName, float Radius)
{
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer *Player = static_cast<GPlayer*>(P);
	GPlayer *Start = static_cast<GPlayer*>(St);

	while (1)
	{
		geVec3d		Vect;

		Start = static_cast<GPlayer*>(Server_GetNextPlayer(Server, Start, ClassName));

		if (!Start)
			return NULL;

		// If start is within radius, then return it...
		geVec3d_Subtract(&Start->XForm.Translation, &Player->XForm.Translation, &Vect);

		if (geVec3d_Length(&Vect) <= Radius)
			return Start;		// Got one...
	}

	return NULL;
}

//=====================================================================================
//	Server_SetViewPlayer
//=====================================================================================
static void Server_SetViewPlayer(void *S, GenVSI_CHandle ClientHandle, void *P)
{
	int32			i;
	uint8			Data[1000];
	Buffer_Data		Buffer;
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer *Player = static_cast<GPlayer*>(P);

	assert(Server);
	assert(ClientHandle >= 0 && ClientHandle <= NETMGR_MAX_CLIENTS);
	assert(Player);

	Client = &Server->Clients[ClientHandle];

	assert(Client->Active);

	Buffer.Data = Data;
	Buffer.Size = 1000;
	Buffer.Pos = 0;

	// Get player index number
	i = (int32)SERVER_GPLAYER_TO_INDEX(Server, Player);
	
	Buffer_FillByte(&Buffer, NETMGR_MSG_VIEW_PLAYER);
	Buffer_FillShort(&Buffer, (uint16)i);
	
	Console_Printf(GameMgr_GetConsole(Server->GMgr), "View Player sent: %i\n", i);
	
	if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
	{
		Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		//GenVS_Error("Server_SetViewPlayer:  Could not send message.\n");
	}
}

//=====================================================================================
//	Server_MovePlayerModel
//=====================================================================================
static geBoolean Server_MovePlayerModel(void *S, void *P, const geXForm3d *DestXForm)
{
	geBoolean	CanMove;
	int32		i;
	geWorld		*World;
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer		*Player = static_cast<GPlayer*>(P);

	assert(Server);

	World = GameMgr_GetWorld(Server->GMgr);
	assert(World);

	CanMove = GE_TRUE;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		geVec3d		Mins, Maxs, Pos, Out;
		GPlayer		*Target;

		Target = &Server->SvPlayers[i];

		if (!Target->Active)			// Not an active player...
			continue;

		if (Target->ViewFlags & VIEW_TYPE_MODEL)	// Don't check other models, don't care...
			continue;

		// Don't test against this thing unless it's got the COLLIDEMODEL flag set.
		if (!(Target->ViewFlags & VIEW_TYPE_COLLIDEMODEL))
			continue;

		Mins = Target->Mins;
		Maxs = Target->Maxs;

		Pos = Target->XForm.Translation;

		// Test the models move intentions agianst all the players in the level
		if (!geWorld_TestModelMove(World, Player->Model, DestXForm, &Mins, &Maxs, &Pos, &Out))
		{
			CanMove = GE_FALSE;		// Can't move, would've pushed player into world

			// The model was blocked by player target
			if (Player->Blocked)	// Call the blocked callback, and tell Player that Target is in the way..
			{
				Player->Blocked(&Server->GenVSI, Player, Target);
			}

		}
		else
		{
			Target->XForm.Translation = Out;
		}
	}

	if (CanMove)
	{
		if (World)
			geWorld_SetModelXForm(World, Player->Model, DestXForm);
		else
			Console_Printf(GameMgr_GetConsole(Server->GMgr), "Server_MovePlayerModel:  No world!\n");
	}

	return CanMove;
}

//=====================================================================================
//	Server_GetPlayerTimeExtents
//=====================================================================================
static void Server_GetPlayerTimeExtents(void *S, void *P, uint16 MotionIndex, float *Start, float *End)
{
	geMotion				*Motion;
	int32					Index;
	GameMgr_MotionIndexDef	*pMotionIndex;
	Server_Server *Server = static_cast<Server_Server*>(S);
	GPlayer *Player = static_cast<GPlayer*>(P);

	assert(Server);
	assert(Player);
	assert(Start && End);
	assert(GameMgr_GetWorld(Server->GMgr));
	
	Index = SERVER_GPLAYER_TO_INDEX(Server, Player);

	if (!Server->Client->Players[Index].ActorDef)
	{
		*Start = 0.0f;
		*End = 0.0f;
		return;
	}

	pMotionIndex = GameMgr_GetMotionIndexDef(Server->GMgr, MotionIndex);
	
	Motion = geActor_GetMotionByName(Server->Client->Players[Index].ActorDef, pMotionIndex->MotionName);

	if (!Motion)
		GenVS_Error("Server_GetPlayerTimeExtents:  Motion not found in actor: %s", pMotionIndex->MotionName);

	if (!geMotion_GetTimeExtents(Motion, Start, End))
		GenVS_Error("Server_GetPlayerTimeExtents:  geMotion_GetTimeExtents failed...\n");
}

#define CLIENT_TO_SERVER_PLAYER(s, p) (s->SvPlayers + (int32)(p - s->Client->Players))

//=====================================================================================
//	Server_ActorToPlayer
//=====================================================================================
static void *Server_ActorToPlayer(void *S, geActor *Actor)
{

	GPlayer			*CPlayer;
	GPlayer			*SPlayer;
	Server_Server *Server = static_cast<Server_Server*>(S);

	// Convert the client player to a server player (they are the same index number so it's easy...)
	CPlayer = (GPlayer*)geActor_GetUserData(Actor);

	if (!CPlayer)
		return NULL;

	SPlayer = CLIENT_TO_SERVER_PLAYER(Server, CPlayer);

	return static_cast<void*>(SPlayer);
}

//=====================================================================================
//	Server_GetWorld
//=====================================================================================
static geWorld *Server_GetWorld(void *S)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	return GameMgr_GetWorld(Server->GMgr);
}

//=====================================================================================
//	Server_ConsolePrintf
//=====================================================================================
static void Server_ConsolePrintf(void *S, const char *Str, ...)
{
	va_list		ArgPtr;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(Str);

	va_start (ArgPtr, Str);
    Console_Printf(GameMgr_GetConsole(Server->GMgr), Str, ArgPtr);
	va_end (ArgPtr);
}

//=====================================================================================
//	Server_ConsoleHeaderPrintf
//=====================================================================================
static void Server_ConsoleHeaderPrintf(void *S, int32 ClientHandle, geBoolean AllClients, const char *Str, ...)
{
	Buffer_Data		Buffer;
	char			Data[1024];
	Server_Client	*Client;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(ClientHandle != CLIENT_NULL_HANDLE);

	assert(Server);
	assert(Str);

	// Set the buffer up
	Buffer_Set(&Buffer, Data, 1024);

	// Fill the buffer with the message type/body...
	Buffer_FillByte(&Buffer, NETMGR_MSG_HEADER_PRINTF);
	Buffer_FillString(&Buffer, (uint8*)Str);

	Client = &Server->Clients[ClientHandle];

	assert(Client->Active);

	if (AllClients)
	{
		SendAllClientsMessage(Server, &Buffer, GE_TRUE);
	}
	else if (Client->Active)
	{
		if (!NetMgr_SendClientMessage(Server->NMgr, Client->NetID, &Buffer, GE_TRUE))
		{
			//GenVS_Error("Could not send local client message!!!");

			Server_ClientDisconnect(Server, Client->NetID, Client->Name);
		}
	}
		
}

//=====================================================================================
//	Server_GetTime
//=====================================================================================
static float Server_GetTime(void *Server)
{
	assert(Server);

	return GameMgr_GetTime(static_cast<Server_Server*>(Server)->GMgr);
}


//=====================================================================================
//	Server_GetPlayerClientData
//=====================================================================================
static void *Server_GetPlayerClientData(void *S, GenVSI_CHandle ClientHandle)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(ClientHandle >=0 && ClientHandle <= 255);

	return &Server->Clients[ClientHandle];
}

//=====================================================================================
//	Server_Error
//=====================================================================================
static void Server_Error(void *S, const char *ErrorStr, ...)
{
	va_list		ArgPtr;
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(ErrorStr);

	va_start (ArgPtr, ErrorStr);
    GenVS_Error(ErrorStr, ArgPtr);
	va_end (ArgPtr);
}

//=====================================================================================
//	Server_ProcIndex
//=====================================================================================
static void Server_ProcIndex(void *S, uint16 Index, void *Proc)
{
	Server_Server *Server = static_cast<Server_Server*>(S);

	assert(Server);
	assert(Index >= 0 && Index < 65535);
	assert(Index >= 0 && Index < SERVER_MAX_PROC_INDEX);
	
	Server->ProcIndex[Index] = Proc;
}

//=====================================================================================
//	Server_SetupGenVSI
//	Sets up the Genesis Virtual System Interface to the Game code through the server
//=====================================================================================
static void Server_SetupGenVSI(void *S)
{
	GenVSI	*VSI;
	Server_Server *Server = static_cast<Server_Server*>(S);

	VSI = &Server->GenVSI;

	// Clear out everything...
	memset(VSI, 0, sizeof(GenVSI));

	// Then fill in data that is valid for a true server
	VSI->Mode = MODE_Server;
	
	// Put the server object as user data in the GenVSI object
	VSI->UData = Server;

	// Setup Callbacks

	// Load/Prep
	VSI->SetClassSpawn = Server_SetClassSpawn;
	VSI->SetWorld = Server_SetWorldRequest;
	VSI->ActorIndex = Server_ActorIndex;
	VSI->MotionIndex = Server_MotionIndex;
	VSI->BoneIndex = Server_BoneIndex;
	VSI->TextureIndex = Server_TextureIndex;
	VSI->SoundIndex = Server_SoundIndex;

	// Candy functions
	VSI->PlaySound = Server_PlaySound;
	VSI->SpawnFx = Server_SpawnFx;

	// Temp, hardcoded variable functions (for now)
	VSI->GetPlayerClientData = Server_GetPlayerClientData;
	VSI->SetClientScore = Server_SetClientScore;
	VSI->SetClientHealth = Server_SetClientHealth;
	VSI->SetClientWeapon = Server_SetClientWeapon;
	VSI->SetClientInventory = Server_SetClientInventory;
	VSI->GetClientMove = Server_GetClientMove;
#pragma todo ("Bot - Server.c")
	VSI->IsClientBot = Server_IsClientBot;
	VSI->ClientDisconnect = Server_ClientDisconnectByHandle;

	// Support functions
	VSI->ModelToViewIndex = Server_ModelToViewIndex;
	VSI->RegisterPlayerModel = Server_RegisterPlayerModel;
	VSI->GetNextPlayer = Server_GetNextPlayer;
	VSI->GetNextPlayerInRadius = Server_GetNextPlayerInRadius;
	VSI->SetViewPlayer = Server_SetViewPlayer;
	VSI->MovePlayerModel = Server_MovePlayerModel;
	VSI->GetPlayerTimeExtents = Server_GetPlayerTimeExtents;
	VSI->ActorToPlayer = Server_ActorToPlayer;
	VSI->GetWorld = Server_GetWorld;
	VSI->ConsolePrintf = Server_ConsolePrintf;
	VSI->ConsoleHeaderPrintf = Server_ConsoleHeaderPrintf;
	VSI->GetTime = Server_GetTime;

	VSI->SpawnPlayer = Server_CreatePlayer;
	VSI->DestroyPlayer = Server_DestroyPlayer;

	VSI->ProcIndex = Server_ProcIndex;
}
