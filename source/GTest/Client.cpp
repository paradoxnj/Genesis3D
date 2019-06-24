/****************************************************************************************/
/*  Client.c                                                                            */
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
#include <Assert.h>
#include <Math.h>

#include "Client.h"

#include "Buffer.h"
#include "Gmenu.h"
#include "cd.h"

LARGE_INTEGER			gc_Freq, gc_OldTick, gc_CurTick;

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
					geEngine_Printf(GameMgr_GetEngine(g), 1, 50, "Timer ms: %2.3f/%2.3f", ElapsedTime, Total);	\
				}
extern	geVFile *MainFS;
extern	geFloat	EffectScale;

static	int32 NumUpdates = 0;		// For status bar updating...

#ifdef _DEBUG
	Fx_Player *PLAYER_TO_FXPLAYER(Client_Client *Client, GPlayer *Player)
	{
		uint32 Index;

		assert(Client_IsValid(Client) == GE_TRUE);
		assert(Player);

		assert(Player >= Client->Players);

		Index = Player - Client->Players;

		assert(Index < NETMGR_MAX_PLAYERS);

		return &Client->FxPlayers[Index];
	}

	Fx_Player *TEMP_PLAYER_TO_FXPLAYER(Client_Client *Client, GPlayer *Player)
	{
		uint32 Index;

		assert(Client_IsValid(Client) == GE_TRUE);
		assert(Player);

		assert(Player >= Client->TempPlayers);

		Index = Player - Client->TempPlayers;

		assert(Index < CLIENT_MAX_TEMP_PLAYERS);

		return &Client->FxPlayers[Index];
	}

	uint32 CLIENT_GPLAYER_TO_INDEX(Client_Client *Client, GPlayer *Player)
	{
		uint32		Index;

		assert(Client_IsValid(Client) == GE_TRUE);
		assert(Player);

		assert(Player >= Client->Players);
		
		Index = (Player - Client->Players);

		assert(Index < NETMGR_MAX_PLAYERS);

		return Index;
	}

	uint32 CLIENT_TEMP_GPLAYER_TO_INDEX(Client_Client *Client, GPlayer *Player)
	{
		uint32		Index;

		assert(Client_IsValid(Client) == GE_TRUE);
		assert(Player);

		assert(Player >= Client->TempPlayers);
		
		Index = (Player - Client->TempPlayers);

		assert(Index < CLIENT_MAX_TEMP_PLAYERS);

		return Index;
	}
#else
	#define PLAYER_TO_FXPLAYER(c, p) (&(c)->FxPlayers[(p) - (c)->Players])
	#define TEMP_PLAYER_TO_FXPLAYER(c, p) (&(c)->TempFxPlayers[(p) - (c)->TempPlayers])
	#define CLIENT_GPLAYER_TO_INDEX(c, p) ((p) - (c)->Players)
	#define CLIENT_TEMP_GPLAYER_TO_INDEX(c, p) ((p) - (c)->TempPlayers)
#endif

#define REFRESH_TIMES (4)

//#define SMALL_CLIENT_CUTOFF_HEIGHT (200)
#define	PREDICT_CLIENT

void		GenVS_Error(const char *Msg, ...);

//=====================================================================================
//	Local Static functions
//=====================================================================================
static BOOL IsKeyDown(int KeyCode, HWND hWnd);
BOOL NewKeyDown(int KeyCode, HWND hWnd);
geBoolean ReadServerMessages(Client_Client *Client, GameMgr *GMgr, float Time);
static void UpdatePlayers(Client_Client *Client, float Time);
static geBoolean RenderWorld(Client_Client *Client, GameMgr *GMgr, float Time);
static void SetupCamera(geCamera *Camera, GE_Rect *Rect, geXForm3d *XForm);
static void ParsePlayerDataLocally(Client_Client *Client, Buffer_Data *Buffer, geBoolean Fake);
static void Client_SetupGenVSI(Client_Client *Client);
static geBoolean Client_MovePlayerModel(Client_Client *Client, GPlayer *Player, const geXForm3d *DestXForm);
static geBoolean CheckClientPlayerChanges(Client_Client *Client, GPlayer *Player, geBoolean TempPlayer);
void Client_DestroyPlayer(Client_Client *Client, GPlayer *Player);

#define M_PI	(3.14159f)
#define PI_2	(M_PI*2.0f)
#define M_PI2	(PI_2)

uint8		SData[5000];
Buffer_Data	SendBuffer;

//=====================================================================================
//	Client_Create
//=====================================================================================
Client_Client *Client_Create(	geEngine	*Engine, 
								Client_Mode Mode, 
								GameMgr		*GMgr, 
								NetMgr		*NMgr, 
								VidMode		VidMode,
								int32		DemoMode,
								const char	*DemoName,
								geBoolean	MultiPlayer)
{
	geRect		Rect = {0, 0, 0, 0};

	Client_Client	*NewClient;

	NewClient = GE_RAM_ALLOCATE_STRUCT(Client_Client);

	assert(NewClient != NULL);
	
	if (!NewClient)
		return NULL;

	// We can't clear the client array in SetDefaults.  Because (more)
	// Host_SetWorld calls Client_NewWorldDefaults, and that would clear some things like Camera, etc (more)
	// that need to hang around.
	memset(NewClient, 0, sizeof(Client_Client));

	// Set the sanity check signatures
	NewClient->Self1 = NewClient;
	NewClient->Self2 = NewClient;

	NewClient->MultiPlayer = MultiPlayer;

	// The client is current disconnected
	NewClient->NetState = NetState_Disconnected;

	// Save thes objects
	NewClient->GMgr = GMgr;
	NewClient->NMgr = NMgr;

	// Save the mode we will operate in...
	NewClient->Mode = Mode;

	// And the vidmode
	NewClient->VidMode = VidMode;

	// Set some defaults that won't effect anystate of the client
	Client_NewWorldDefaults(NewClient);
	
	// Set initial view xform and player
	geXForm3d_SetIdentity(&NewClient->ViewXForm);	
	NewClient->ViewPlayer = -1;
	NewClient->ClientPlayer = -1;

	NewClient->Engine = Engine;

	NewClient->Time = 0.0f;	

	NewClient->CurrentWeapon = 0;

	Buffer_Set(&SendBuffer, (char*)SData, 5000);

	// Setup the status bar...
	if (!Client_CreateStatusBar(NewClient, VidMode))
	{
		geErrorLog_AddString(-1, "Client_Create:  Client_CreateStatusBar failed.", NULL);
		goto ExitWithError;
	}

	// Setup the GenVSI interface
	Client_SetupGenVSI(NewClient);

	// Setup the demo system
	if (!Client_SetupDemos(NewClient, DemoMode, DemoName))
		NewClient->Demo.Mode = CLIENT_DEMO_NONE;

	// Call the client main code in the game stuff
	Client_Main(&NewClient->GenVSI);
	Console_Printf(GameMgr_GetConsole(GMgr), "Client_Create:  Client_Main initialized...\n");

	QueryPerformanceFrequency(&gc_Freq);

	return NewClient;

	ExitWithError:
	{
		if (NewClient)
		{
			Client_FreeALLResources(NewClient);
			Client_Destroy(NewClient);
		}

		return NULL;
	}
}

//=====================================================================================
//	Client_Destroy
//=====================================================================================
void Client_Destroy(Client_Client *Client)
{
	assert(Client_IsValid(Client) == GE_TRUE);

	Client_FreeALLResources(Client);

	if (Client->Demo.File != NULL)
		fclose (Client->Demo.File);

	geRam_Free(Client);
}


//=====================================================================================
//	Client_IsValid
//=====================================================================================
geBoolean Client_IsValid(const Client_Client *Client)
{
	if (!Client)
		return GE_FALSE;

	if (Client->Self1 != Client)
		return GE_FALSE;

	if (Client->Self2 != Client)
		return GE_FALSE;

	return GE_TRUE;
}

#define CONSOLE_HEIGHT_320 (30)
#define CONSOLE_HEIGHT_640 (60)

//=====================================================================================
//	Client_CreateStatusBar
//=====================================================================================
geBoolean Client_CreateStatusBar(Client_Client *Client, VidMode VidMode)
{
	int32	i, k;
	int     Width,Height;
	int     LeftElementWidth,ElementHeight,RightElementWidth,PrintOffsX,PrintOffsY,Left;
	char	BmpPath[MAX_PATH];

	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Client->Engine);

	VidMode_GetResolution(VidMode,&Width,&Height);
	
	if (Width < SMALL_CONSOLE_CUTOFF_WIDTH)
		{
			strcpy(BmpPath,"320x240");
			LeftElementWidth    = 56;
			RightElementWidth   = 52;
			ElementHeight       = CONSOLE_HEIGHT_320;
			PrintOffsX          = 18;
			PrintOffsY          = 10;
			Left				= (Width-320)/2;
		}
	else
		{
			strcpy(BmpPath,"640x480");
			LeftElementWidth    = 112;
			RightElementWidth   = 104;
			ElementHeight       = CONSOLE_HEIGHT_640;
			PrintOffsX          = 36;
			PrintOffsY          = 22;
			Left				= (Width-640)/2;
		}

	for (i=0; i<6; i++)
		{
			if (i<2)
				{
					Client->StatusBar.Elements[i].XPos = Left + LeftElementWidth*i;
				}
			else
				{
					Client->StatusBar.Elements[i].XPos = Left + LeftElementWidth*2 + RightElementWidth*(i-2);
				}

			Client->StatusBar.Elements[i].YPos       = Height - 1 - ElementHeight;
			Client->StatusBar.Elements[i].Print1XPos = Client->StatusBar.Elements[i].XPos + PrintOffsX;
			Client->StatusBar.Elements[i].Print1YPos = Height - 1 - PrintOffsY;
		}


	for (i=0; i<6; i++)
	{
		for (k=0; k<3; k++)
		{
			char		Name[64];

			sprintf(Name, "Bmp\\SBar\\%s\\SBar%i-%i.Bmp", BmpPath, i, k);

			assert(Client->StatusBar.Elements[i].Bitmaps[k] == NULL);
			
			Client->StatusBar.Elements[i].Bitmaps[k] = geBitmap_CreateFromFileName(MainFS, Name);

			if (!Client->StatusBar.Elements[i].Bitmaps[k])
			{
				geErrorLog_AddString(-1, "Client_CreateStatusBar:  Could not load status bar element:", Name);
				goto ExitWithError;
			}

			if (!geEngine_AddBitmap(Client->Engine, Client->StatusBar.Elements[i].Bitmaps[k]))
			{
				geErrorLog_AddString(-1, "Client_CreateStatusBar:  geEngine_AddBitmap failed.", NULL);
				goto ExitWithError;
			}

		}
	}

	return GE_TRUE;

	ExitWithError:
	{
		Client_FreeALLResources(Client);
		return GE_FALSE;
	}
}

//=====================================================================================
//	Client_DestroyStatusBar
//=====================================================================================
void Client_DestroyStatusBar(Client_Client *Client)
{
	int32		i, k;

	assert(Client_IsValid(Client) == GE_TRUE);

	// Delete the status bar bitmaps bitmaps
	for (i=0; i<6; i++)
	{
		for (k=0; k<3; k++)
		{
			if (!Client->StatusBar.Elements[i].Bitmaps[k])
				continue;

			if (!geEngine_RemoveBitmap(Client->Engine, Client->StatusBar.Elements[i].Bitmaps[k]))
			{
				geErrorLog_AddString(-1, "Client_DestroyConsole:  geEngine_RemoveBitmap failed.", NULL);
				assert(0);
			}

			geBitmap_Destroy(&Client->StatusBar.Elements[i].Bitmaps[k]);
			Client->StatusBar.Elements[i].Bitmaps[k] = NULL;
		}
	}
}

//=====================================================================================
//	SetPlayerDefaults
//=====================================================================================
static void SetPlayerDefaults(GPlayer *Player)
{
	//memset(Player, 0, sizeof(GPlayer));

	Player->OldScale =
	Player->Scale	 = 1.0f;

	Player->OldViewFlags = VIEW_TYPE_NONE;
	Player->OldViewFlags2 = VIEW_TYPE_NONE;
	Player->ViewFlags = VIEW_TYPE_NONE;

	Player->OldViewIndex = 0xffff;
	Player->OldViewIndex2 = 0xffff;
	Player->ViewIndex = 0xffff;

	Player->OldControlIndex = 0xffff;
	Player->OldTriggerIndex = 0xffff;
	Player->ControlIndex = 0xffff;
	Player->TriggerIndex = 0xffff;

	Player->UpdateTime = -1.0f;

	Player->Mesh = NULL;
	Player->Actor = NULL;
	Player->Light = NULL;
	Player->Poly = NULL;

	Player->OldMotionIndex  = 
	Player->MotionIndex		= GAMEMGR_MOTION_INDEX_NONE;

	Player->ActorDef = NULL;
}

//=====================================================================================
//	Client_DestroyALLPlayers
//=====================================================================================
void Client_DestroyALLPlayers(Client_Client *Client)
{
	int32		i;
	GPlayer		*Player = nullptr;

	assert(Client_IsValid(Client) == GE_TRUE);

	// Destroy all normal players
	Player = Client->Players;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++, Player++)
	{
		if (!Player->Active)
			continue;

		Client_DestroyPlayer(Client, Player);
	}

	// Now destroy all temp players (if any)
	Player = Client->TempPlayers;

	for (i=0; i< CLIENT_MAX_TEMP_PLAYERS; i++, Player++)
	{
		if (!Player->Active)
			continue;

		Client_DestroyTempPlayer(Client, Player);
	}
}

//=====================================================================================
//	Client_FreeALLResources
//=====================================================================================
void Client_FreeALLResources(Client_Client *Client)
{
	geBoolean	Ret = GE_FALSE;

	assert(Client_IsValid(Client) == GE_TRUE);

	Ret = GE_TRUE;

	// Destroy the status bar
	Client_DestroyStatusBar(Client);

	// Delete any active players that might by laying around
	Client_DestroyALLPlayers(Client);

	// Delete the move list
	while (Client_PeekMove(Client))
		Client_RemoveFirstMove(Client);

	assert(Ret == GE_TRUE);
}

//=====================================================================================
//	Client_FreeResourcesForNewWorld
//=====================================================================================
void Client_FreeResourcesForNewWorld(Client_Client *Client)
{
	// Delete any active players that might by laying around
	Client_DestroyALLPlayers(Client);

	// Delete the move list
	while (Client_PeekMove(Client))
		Client_RemoveFirstMove(Client);
}

static int32 LastPlayer = -1;

//=====================================================================================
//	Client_NewWorldDefaults
//=====================================================================================
void Client_NewWorldDefaults(Client_Client *Client)
{
	int32		i;

	assert(Client_IsValid(Client) == GE_TRUE);

	// Set the default on all players
	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
		SetPlayerDefaults(&Client->Players[i]);

	Client->Time = 0.0f;
	Client->NetTime = 0.0f;
	Client->OldNetTime = 0.0f;
	//Client->ClientPlayer = -1;
	LastPlayer = -1;

	// Make sure the status bar updates between world cahnges...
	NumUpdates = REFRESH_TIMES;		
}

//=====================================================================================
//	PrintClientScores
//=====================================================================================
static geBoolean PrintClientScores(Client_Client *Client)
{
	int32				i, k, j, Total;
	int32				Sorted[NETMGR_MAX_CLIENTS];
	int32				SortedScores[NETMGR_MAX_CLIENTS];
	Client_ClientInfo	*ClientInfo = nullptr;
	char				*Name = nullptr;
	int32				Score;
	int					Width,Height;

	assert(Client_IsValid(Client) == GE_TRUE);

	// Sort the clients by score
	Total = 0;
	ClientInfo = Client->ClientInfo;

	for (i=0 ; i<NETMGR_MAX_CLIENTS; i++, ClientInfo++)
	{
		if (!ClientInfo->Active)
			continue;

		Score = ClientInfo->Score;

		for (j=0 ; j<Total ; j++)
		{
			if (Score > SortedScores[j])
				break;
		}

		for (k=Total ; k>j ; k--)
		{
			Sorted[k] = Sorted[k-1];
			SortedScores[k] = SortedScores[k-1];
		}
		Sorted[j] = i;
		SortedScores[j] = Score;
		Total++;
	}

	VidMode_GetResolution(Client->VidMode,&Width,&Height);

	for (i=0; i< Total; i++)
	{
		int Line;
		ClientInfo = &Client->ClientInfo[Sorted[i]];

		Name = ClientInfo->Name;
		Score = ClientInfo->Score;

		if (Width < SMALL_CONSOLE_CUTOFF_WIDTH)
			Line = i+((Height-30)/6)-9;
		else
			Line = i+((Height-60)/15)-9;

		if (!Console_XYPrintf(GameMgr_GetConsole(Client->GMgr), 0, Line, 0, "%12s: %4i", Name, Score))
					return GE_FALSE;
	}
	
	// Go ahead and print ping time here
	if (Width < SMALL_CONSOLE_CUTOFF_WIDTH)
	{
		if (Client->MultiPlayer)
		{
			if (!Console_XYPrintf(GameMgr_GetConsole(Client->GMgr), (Width/6)-13, ((Height-30)/6)-2, 0, "Ping: %2.2f", Client->Ping*1000.0f))
				return GE_FALSE;
		}
	}
	else
	{
		if (Client->MultiPlayer)
		{
			if (!Console_XYPrintf(GameMgr_GetConsole(Client->GMgr), (Width/9)-13, ((Height-60)/16)-1, 0, "Ping: %2.2f", Client->Ping*1000.0f))
				return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//=====================================================================================
//	PrintCrossHair
//=====================================================================================
static void PrintCrossHair(Client_Client *Client)
{
	int					Width,Height;

	VidMode_GetResolution(Client->VidMode,&Width,&Height);

	if (Width < SMALL_CONSOLE_CUTOFF_WIDTH)
	{
		if (!Console_XYPrintf(GameMgr_GetConsole(Client->GMgr), ((Width/6)/2), (((Height-30)/6)/2), 0, "*"))
			GenVS_Error("PrintCrossHair:  Console_XYPrintf failed.\n");
	}
	else
	{
		if (!Console_XYPrintf(GameMgr_GetConsole(Client->GMgr), ((Width/9)/2), (((Height-60)/15)/2)-1, 0, "+"))
			GenVS_Error("PrintCrossHair:  Console_XYPrintf failed.\n");
	}
}

//=====================================================================================
//	DrawStatusBar
//=====================================================================================
static geBoolean DrawStatusBar(Client_Client *Client)
{
	Client_ClientInfo	*ClientInfo;
	uint32				Ammo;
	geBoolean			HasItem;
	int32				i;
	int32				Val[6];

	assert(Client_IsValid(Client) == GE_TRUE);

	ClientInfo = &Client->ClientInfo[Client->ClientIndex];

	Val[0] = ClientInfo->Health;
	Val[1] = Client->Inventory[16] & ((1<<15)-1);

	// Totally %$%^% hacked!!!
	for (i=0; i< 2; i++)
	{
		geBitmap				*Bitmap;
		Client_StatusElement	*Element;

		Element = &Client->StatusBar.Elements[i];

		//if (Val[i] < Element->Low)
		//	Texture = Element->Textures[STATE_Warning];
		//else
			Bitmap = Element->Bitmaps[STATE_Normal];

		geEngine_DrawBitmap(Client->Engine, Bitmap, NULL, Element->XPos, Element->YPos);
	}

	for (i=0; i< 4; i++)
	{
		geBitmap				*Bitmap;
		Client_StatusState		State;
		Client_StatusElement	*Element;

		Element = &Client->StatusBar.Elements[i+2];

		Ammo = Client->Inventory[i];

		HasItem = (Ammo & (1<<15));

		Ammo &= 0xff;

		if (i == 0)		// Always have blaster...
		{
			HasItem = GE_TRUE;
			Ammo = 99;
		}

		if (!HasItem)
		{
			State = STATE_Disable;
		}
		else if (i == Client->CurrentWeapon)
		{
			//if ((int32)Ammo < Element->Low)
			//	State = STATE_Warning;
			//else
				State = STATE_Selected;
		}
		else
		{
			//if ((int32)Ammo < Element->Low)
			//	State = STATE_Warning;
			//else
				State = STATE_Normal;
		}
						  		
		Element->State = State;

		Bitmap = Element->Bitmaps[State];
		
		geEngine_DrawBitmap(Client->Engine, Bitmap, NULL, Element->XPos, Element->YPos);

		Val[i+2] = Ammo;
	}

	for (i=0; i<6; i++)
	{
		char	Temp[32];
		int32	XPos, YPos;

		if (i==2)
			strcpy(Temp, "Inf");
		else
			sprintf(Temp, "%i", Val[i]);

		XPos = Client->StatusBar.Elements[i].Print1XPos;
		YPos = Client->StatusBar.Elements[i].Print1YPos;

		Console_XYPrintf(GameMgr_GetConsole(Client->GMgr), XPos, YPos, 1, "%s", Temp);
	}

	return GE_TRUE;

}

//=====================================================================================
//	Client_RefreshStatusBar
//=====================================================================================
void Client_RefreshStatusBar(int32 NumPages)
{
	NumPages;
	NumUpdates = REFRESH_TIMES;
}

extern geBoolean g_FarClipPlaneEnable;		// This is REALLY bad

//=====================================================================================
//=====================================================================================
static void UpdateStatusBar(Client_Client *Client)
{
	Client_ClientInfo	*ClientInfo;
	int32				i;

	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Client->ClientIndex >= 0 && Client->ClientIndex < NETMGR_MAX_CLIENTS);

	ClientInfo = &Client->ClientInfo[Client->ClientIndex];

	for (i=0; i<4; i++)
	{
		if (Client->Inventory[i] != Client->OldInventory[i])
		{
			Client->OldInventory[i] = Client->Inventory[i];
			NumUpdates = REFRESH_TIMES;		
			break;
		}
	}

	for (i=16; i<17; i++)
	{
		if (Client->Inventory[i] != Client->OldInventory[i])
		{
			NumUpdates = REFRESH_TIMES;		
			break;
		}
	}

	for (i=0; i<4; i++)
	{
		if (Client->Inventory[i] != Client->OldInventory[i])
		{
			Client->OldInventory[i] = Client->Inventory[i];
			NumUpdates = REFRESH_TIMES;		
			break;
		}
	}

	if (Client->CurrentWeapon != Client->OldWeapon)
	{
		Client->OldWeapon = Client->CurrentWeapon;
		NumUpdates = REFRESH_TIMES;
	}

	if (ClientInfo->OldHealth != ClientInfo->Health)
	{
		ClientInfo->OldHealth = ClientInfo->Health;
		NumUpdates = REFRESH_TIMES;
	}

	if (g_FarClipPlaneEnable)		// Always update if clipping is on (because the screen is cleared every frame)
		NumUpdates = REFRESH_TIMES;

	if (NumUpdates <= 0)
		return;		// Nothing to do...

	if (!DrawStatusBar(Client))
		GenVS_Error("UpdateStatusBar: Could not draw the status bar.");

	NumUpdates--;
}

//=====================================================================================
//	ControlTempPlayers
//=====================================================================================
static void ControlTempPlayers(Client_Client *Client, float Time)
{
	int32		i;
	GPlayer		*Player;

	assert(Client_IsValid(Client) == GE_TRUE);

	Player = Client->TempPlayers;

	for (i=0; i<CLIENT_MAX_TEMP_PLAYERS; i++, Player++)
	{
		if (!Player->Active)
			continue;

		if (!Player->Control)
			continue;

		Player->Control(&Client->GenVSI, Player, Time);

		if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), TEMP_PLAYER_TO_FXPLAYER(Client, Player), Player->FxFlags))
			GenVS_Error("Client_ControlTempPlayers:  Could not set fx flags.\n");

		Player->Pos = Player->XForm.Translation;
		geXForm3d_GetEulerAngles(&Player->XForm, &Player->Angles);
		
		Client_UpdateSinglePlayer(Client, Player, 1.0f,  Time, GE_TRUE);
	}
}

//=====================================================================================
//	Client_Frame
//	Sends movement for this machine
//	Renders the world
//=====================================================================================
geBoolean Client_Frame(Client_Client *Client, float Time)
{
	assert(Client_IsValid(Client) == GE_TRUE);

	Client->Time += Time;						// Update client time

	// Read and process msg's from the server
	if (!ReadServerMessages(Client, Client->GMgr, Time))		
		GenVS_Error("Client_Frame:  ReadServerMessages failed.\n");

	if (Client->Demo.Mode != CLIENT_DEMO_PLAY)
	{
		// If the world is loaded and we have the go from server, start sending move cmd's
		if (Client->NetState == NetState_WorldActive)
		{
			if (!Client_SendMove(Client, Time))			// Send movement commands
				GenVS_Error("Client_Frame:  Client_SendMove failed.\n");
		}

		ControlTempPlayers(Client, Time);
	}

	return GE_TRUE;
}

//=====================================================================================
//	Client_RenderFrame
//=====================================================================================
geBoolean Client_RenderFrame(Client_Client *Client, float Time)
{
	assert(Client_IsValid(Client) == GE_TRUE);

	if (Client->NetState != NetState_WorldActive)		// If no world currently set, then return...
		return GE_TRUE;

	if (!RenderWorld(Client, Client->GMgr, Time))
		GenVS_Error("Client_RenderFrame:  RenderWorld failed.\n");

	//if (Client->MultiPlayer)
	{
		if (!PrintClientScores(Client))
			GenVS_Error("Client_RenderFrame:  PrintClientScores failed.\n");
	}
	
	PrintCrossHair(Client);

	UpdateStatusBar(Client);

	return GE_TRUE;
}

extern	float	GlobalMouseSpeedX;
extern	float	GlobalMouseSpeedY;

extern int32		KeyLut[256];
extern geBoolean	MouseInvert;


//=====================================================================================
//	Client_MoveClientLocally
//=====================================================================================
static void Client_MoveClientLocally(Client_Client *Client, Client_Move *Move)
{
	GPlayer *Player;

	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Client->ClientPlayer != -1);

	if (!GameMgr_GetWorld(Client->GMgr))
		return;

	// Fill the client with this move, so when the game code calls the GneVSI_GetClientMove, we will be able to
	// fullfill the request...
	Client->Move.MoveTime = Move->Time;
	Client->Move.ForwardSpeed = Move->ForwardSpeed;
	Client->Move.Angles.X = Move->Pitch;
	Client->Move.Angles.Y = Move->Yaw;
	Client->Move.Angles.Z = 0.0f;
	Client->Move.ButtonBits = Move->ButtonBits;
	Client->Move.Weapon = Move->CurrentWeapon;
	Client->Move.PingTime = Move->Delta;

	Player = &Client->Players[Client->ClientPlayer];

	if (Player->Control)
		Player->Control(&Client->GenVSI, Player, Move->Delta);

	Player->Pos = Player->XForm.Translation;
}

int32		NumMoves = 0;

//=====================================================================================
//	Client_AddMove
//=====================================================================================
void Client_AddMove(Client_Client *Client, Client_Move *Move)
{
	Client_Move		*NewMove, *Last;
	
	NewMove = GE_RAM_ALLOCATE_STRUCT(Client_Move);	
	
	if (!NewMove)
		GenVS_Error("Client_AddMove:  Out of memory.\n");

	*NewMove = *Move;

	NewMove->Next = NULL;
	
	Last = Client->LastMove;

	if (!Last)
		Client->MoveStack = NewMove;	
	else
		Last->Next = NewMove;
	
	Client->LastMove = NewMove;

	NumMoves++;

	if (NumMoves >= 1000)
		GenVS_Error("NumMoves > 1000.\n");
}

//=====================================================================================
//	Client_PeekMove
//=====================================================================================
Client_Move *Client_PeekMove(Client_Client *Client)
{
	assert(Client_IsValid(Client) == GE_TRUE);

	return Client->MoveStack;
}

//=====================================================================================
//	Client_RemoveFirstMove
//=====================================================================================
void Client_RemoveFirstMove(Client_Client *Client)
{
	Client_Move		*Next;

	assert(Client_IsValid(Client) == GE_TRUE);

	NumMoves--;

	if (NumMoves < 0)
		GenVS_Error("NumMove < 0.\n");

	if (!Client->MoveStack)
		GenVS_Error("Client_RemoveFirstMove:  No more moves left...");

	Next = Client->MoveStack->Next;

	geRam_Free(Client->MoveStack);
	
	if (Next)
		Client->MoveStack = Next;
	else
	{
		Client->MoveStack = NULL;
		Client->LastMove = NULL;
	}
}

//=====================================================================================
//	Client_ValidateWeapon
//=====================================================================================
void Client_ValidateWeapon(Client_Client *Client)
{
	int32			i;

	assert(Client_IsValid(Client) == GE_TRUE);

	for (i=0; i<5; i++)
	{
		geBoolean	HasItem;

		HasItem = (Client->Inventory[Client->CurrentWeapon] & (1<<15));

		if (!HasItem && Client->CurrentWeapon != 0)
		{
			Client->CurrentWeapon++;
			Client->CurrentWeapon %= 4;
			continue;
		}
		
		assert(Client->CurrentWeapon >= 0 && Client->CurrentWeapon <= 3);

		return;
	}
	
	// Should'nt get here!!!!!  There is allways the default weapon...
	assert(!"No best weapon!!!");
}

//=====================================================================================
//	Client_SendMove
//	Sends clients intentions to the server (through the supplied host)
//=====================================================================================
geBoolean Client_SendMove(Client_Client *Client, float Time)
{
	Buffer_Data		Buffer;
	uint8			Data[512];
	uint16			ButtonBits;
	HWND			hWnd;

	assert(Client_IsValid(Client) == GE_TRUE);

	if (GMenu_IsAMenuActive() == GE_TRUE)
		return GE_TRUE;

	hWnd = GameMgr_GethWnd(Client->GMgr);
	
	//if (Client->ClientPlayer == -1)
	//	return GE_TRUE;

	Buffer_Set(&Buffer, (char*)Data, 512);

	ButtonBits = 0;

	// Change forward/back motion
	if (IsKeyDown(KeyLut[VK_UP], hWnd))
		Client->ForwardSpeed = 4000.0f;
	else if (IsKeyDown(KeyLut[VK_DOWN], hWnd))
		Client->ForwardSpeed = -4000.0f;
	else
		Client->ForwardSpeed = 0.0f;

	if (MouseInvert)
		Client->Angles.X -= GlobalMouseSpeedY;
	else
		Client->Angles.X += GlobalMouseSpeedY;

	Client->Angles.Y += GlobalMouseSpeedX;

	if (Client->Angles.X > 1.3f)
		Client->Angles.X = 1.3f;
	else if (Client->Angles.X < -1.3f)
		Client->Angles.X = -1.3f;

	GlobalMouseSpeedX = 0.0f;
	GlobalMouseSpeedY = 0.0f;

	if (IsKeyDown(KeyLut[VK_LEFT], hWnd))
		ButtonBits |= HOST_BUTTON_LEFT;

	if (IsKeyDown(KeyLut[VK_RIGHT], hWnd))
		ButtonBits |= HOST_BUTTON_RIGHT;

	// Do misc move actions
	if (NewKeyDown(KeyLut[VK_RBUTTON], hWnd))
		ButtonBits |= HOST_BUTTON_JUMP;

	if (IsKeyDown(KeyLut[VK_LBUTTON], hWnd))
	{
		ButtonBits |= HOST_BUTTON_FIRE;
	}
	
	if (IsKeyDown('1', hWnd))
	{
		Client->CurrentWeapon = 0;
	}
	else if (IsKeyDown('2', hWnd))
	{
		Client->CurrentWeapon = 1;
	}
	else if (IsKeyDown('3', hWnd))
	{
		Client->CurrentWeapon = 2;
	}
	else if (IsKeyDown('4', hWnd))
	{
		Client->CurrentWeapon = 3;
	}
	else if (NewKeyDown(KeyLut[VK_CONTROL], hWnd))
	{
		Client->CurrentWeapon ++;
		Client->CurrentWeapon %= 4;
		
		Client_ValidateWeapon(Client);
	}
	
	Client->ButtonBits = ButtonBits;

	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_MOVE);			// Let server know we're moving
	Buffer_FillFloat(&Buffer, Client->Time);				// Current Time
	Buffer_FillFloat(&Buffer, Client->ServerPingBack);		// Current Time
	Buffer_FillFloat(&Buffer, Client->ForwardSpeed);		// Current Speed
	Buffer_FillFloat(&Buffer, Client->Angles.X);			// Pitch
	Buffer_FillFloat(&Buffer, Client->Angles.Y);			// Yaw
	Buffer_FillShort(&Buffer, ButtonBits);					// Current Buttons

	if (ButtonBits & HOST_BUTTON_FIRE)
	{
		assert(Client->CurrentWeapon >= 0 && Client->CurrentWeapon <= 65535);
		Buffer_FillShort(&Buffer, (uint16)Client->CurrentWeapon);		// Send Current Weapon if firing
	}

#ifdef CALC_ERROR
	if (Client->ClientPlayer >= 0)
	{
		GPlayer		*Player;

		Player = &Client->Players[Client->ClientPlayer];
		Buffer_FillAngle(&Buffer, Player->Pos);	
	}
	else
	{
		geVec3d		FakePos = {0.0f, 0.0f, 0.0f};

		Buffer_FillAngle(&Buffer, FakePos);	
	}
#endif

//BEGIN_TIMER();
#if 1
	if (!NetMgr_SendServerMessage(Client->NMgr, &Buffer, GE_FALSE))
		GenVS_Error("Client_SendMove:  NetMgr_SendServerMessage failed.\n");
#else
	if (Client->Mode == ClientMode_Dumb)
	{
		// Send the move intention to the server for processing
		if (!NetMgr_SendServerMessage(Client->NMgr, &Buffer, GE_FALSE))
			GenVS_Error("Client_SendMove:  NetMgr_SendServerMessage failed.\n");
	}
	else if (Client->Time >= Client->NextSend)
	{
		Buffer_FillBuffer(&SendBuffer, &Buffer);

		// Send the move intention to the server for processing
		if (!NetMgr_SendServerMessage(Client->NMgr, &SendBuffer, GE_FALSE))
			GenVS_Error("Client_SendMove:  NetMgr_SendServerMessage failed.\n");

		Buffer_Set(&SendBuffer, SData, 5000);

		//Client->NextSend = Client->Time + (float)(rand()%100) * (1/100.0f) * 0.5f + 0.25f;
		//Client->NextSend = Client->Time + 0.5f;
		Client->NextSend = Client->Time;
	}
#endif
//END_TIMER(Client->GMgr);

	// Send the message directly to our proxy player so we can begin moving NOW
	if (Client->Mode == ClientMode_Proxy && Client->ClientPlayer != -1)
	{
		Client_Move		Move;
		GPlayer		*Player;

		Player = &Client->Players[Client->ClientPlayer];

		Move.Time = Client->Time;
		Move.Delta = Time;
		Move.ForwardSpeed = Client->ForwardSpeed;
		Move.Pitch = Client->Angles.X;
		Move.Yaw = Client->Angles.Y;
		Move.Pos = Player->Pos;								// So we know our position at this time...
		Move.ButtonBits = ButtonBits;
		Move.CurrentWeapon = Client->CurrentWeapon;

	#ifdef PREDICT_CLIENT

		Client_MoveClientLocally(Client, &Move);
			
		//geEngine_Printf(Host->Engine, 2, 35, "Inventory: %i, %i", Player->InventoryHas[Client->CurrentWeapon], Player->Inventory[Client->CurrentWeapon]);

		// Remember this move, so when we get an update that is older then where we
		// are actually at, we can replay all the moves after the update, and "catch"
		// back up...
		Client_AddMove(Client, &Move);
	#endif
	}

	return GE_TRUE;
}

//=====================================================================================
//	IsKeyDown
//=====================================================================================
static geBoolean IsKeyDown(int KeyCode, HWND hWnd)
{
	//if (GetFocus() == hWnd)
		if (GetAsyncKeyState(KeyCode) & 0x8000)
			return GE_TRUE;

	return GE_FALSE;
}

//=====================================================================================
//	NewKeyDown
//=====================================================================================
geBoolean NewKeyDown(int KeyCode, HWND hWnd)
{
	//if (GetFocus() == hWnd)
	if (GetAsyncKeyState(KeyCode) & 1)
			return GE_TRUE;

	return GE_FALSE;
}

//===========================================================================
//	SetClientProxyPlayer
//===========================================================================
static void SetClientProxyPlayer(Client_Client *Client, int32 Player)
{
	GPlayer			*PPlayer;

	assert(Client_IsValid(Client) == GE_TRUE);

	if (Client->Mode != ClientMode_Proxy)
		return;

	Client->ClientPlayer = Player;

	// If the player already exist, we must make sure we can't see it
	if (Client->Players[Player].Actor)
	{
		geWorld		*World;

		World = GameMgr_GetWorld(Client->GMgr);
							
		assert(World);

		geWorld_SetActorFlags(World, Client->Players[Player].Actor, GE_ACTOR_RENDER_MIRRORS|GE_ACTOR_COLLIDE);
	}
				
	PPlayer = &Client->Players[Player];

	PPlayer->Active = GE_TRUE;		// Set this?  Should be set automatically when updated by server

	PPlayer->Owner = NULL;
	
	PPlayer->ClientHandle = Client->ClientIndex;
	
	// Hard coded for now!  Should be passed accross network via GenVSI_SetWeaponOffset
	PPlayer->GunOffset.X = 0.0f;
	PPlayer->GunOffset.Y = 130.0f;
	PPlayer->GunOffset.Z = 0.0f;
}

geBoolean MHack = GE_TRUE;

int32		TotalMsgBytes;
geBoolean NetPlayGetNumMessages(int32 *NumMsgSend, int32 *NumBytesSend, int32 *NumMsgRec, int32 *NumBytesRec);

//===========================================================================
//	ReadServerMessages
//	Read ALL mesages from server
//===========================================================================
static geBoolean ReadServerMessages(Client_Client *Client, GameMgr *GMgr, float Time)
{
	uint8			Type;
	Buffer_Data		Buffer;

	assert(Client_IsValid(Client) == GE_TRUE);

	MHack = GE_FALSE;

	Client->ServerPingBack = -1.0f;		// Reset ping back variable

	// Keep on reading till there is no more messages...
	// We have to process every message just in case they are critical ones
	while (Client_ReadServerMessages(Client, &Buffer))
	{
		assert(Buffer.Size < NETMGR_LOCAL_MSG_BUFFER_SIZE);
		assert(Buffer.Size >= 0);

		TotalMsgBytes += Buffer.Size;

		// Keep reading till end of buffer
		while (Buffer.Pos < Buffer.Size)
		{
			// Get the type of message...
			Buffer_GetByte(&Buffer, &Type);

			//geEngine_Printf(GameMgr_GetEngine(Client->GMgr), 2, 380, "Msg Type: %i", Type);

			switch(Type)
			{
				case NETMGR_MSG_VERSION:
				{
					uint32		VersionMajor, VersionMinor;

					Buffer_GetLong(&Buffer, &VersionMajor);
					Buffer_GetLong(&Buffer, &VersionMinor);

					if (VersionMajor != NETMGR_VERSION_MAJOR || VersionMinor != NETMGR_VERSION_MINOR)
						GenVS_Error("Wrong Server Version:  %i.%i", VersionMajor, VersionMinor);

					break;
				}

				case NETMGR_MSG_TIME:
				{
					Client->TempTime = 0.0f;
					Client->OldNetTime = Client->NetTime;
					Buffer_GetFloat(&Buffer, &Client->NetTime);

					if (Client->NetTime >= Client->OldNetTime)
						Client->NetTimeGood = GE_TRUE;
					else
						Client->NetTimeGood = GE_FALSE;
					
					Client->ServerPingBack = Client->NetTime;		// So server can calc ping times

					Buffer_GetFloat(&Buffer, &Client->Ping);

					break;
				}

				case NETMGR_MSG_PLAYER_DATA:
				{
					if (Client->NetState != NetState_WorldActive)
						GenVS_Error("ReadServerMessages:  Client->NetState != NetState_WorldActive\n");

					// If we are in the middle of a world change, or the net time is bad, do a fake update
					if (!Client->NetTimeGood || Client->NetState != NetState_WorldActive)
						ParsePlayerDataLocally(Client, &Buffer, GE_TRUE);
					else
						ParsePlayerDataLocally(Client, &Buffer, GE_FALSE);

					break;
				}

				case NETMGR_MSG_NEW_WORLD_PLAYER_DATA:
				{
					if (Client->NetState != NetState_WorldChange)
						GenVS_Error("ReadServerMessages:  Client->NetState != NetState_WorldChange\n");

					ParsePlayerDataLocally(Client, &Buffer, GE_FALSE);
					break;
				}

				case NETMGR_MSG_VIEW_PLAYER:
				{
					uint16		Player;
					GPlayer		*pPlayer;
					int32		OldViewPlayer;

					Buffer_GetShort(&Buffer, &Player);

					assert(Player != 0);	// For debugging, it could be possible, but not likely...
											// Usually the view player will the client...

					OldViewPlayer = Client->ViewPlayer;

					Client->ViewPlayer = (int32)Player;
					
					if (OldViewPlayer != -1)
					{
						pPlayer = &Client->Players[OldViewPlayer];
						
						if (pPlayer->Actor)
						{
							pPlayer->OldViewIndex2 = 0xffff;
							CheckClientPlayerChanges(Client, pPlayer, GE_FALSE);
						}
					}

					// Force it to rewcreate the actor that represents this player (kind of a hack)
					pPlayer = &Client->Players[Player];
					
					if (pPlayer->Actor)
					{
						pPlayer->OldViewIndex2 = 0xffff;
						CheckClientPlayerChanges(Client, pPlayer, GE_FALSE);
					}

					//Console_Printf(GameMgr_GetConsole(Client->GMgr), "View Player assigned by server: %i\n", Player);
					break;
				}

				case NETMGR_MSG_SET_WORLD:
				{
					char		Name[256];

					assert( Client->NetState == NetState_WorldChange);		// The world should be in the middle of a change to get this message

					Buffer_GetString(&Buffer, (uint8*)Name);

					assert(Name[0]);

					if (!Name[0])
						GenVS_Error("ReadServerMessages:  No World Name!\n");

					if (Client->Mode == ClientMode_Dumb)
						break;	// World allready loaded if server on same machine 

					Client_FreeResourcesForNewWorld(Client);
					Client_NewWorldDefaults(Client);

					if (!GameMgr_SetWorld(GMgr, Name))
						GenVS_Error("ReadServerMessages:  GameMgr_SetWorld failed.\n");
					break;
				}

				case NETMGR_MSG_ACTOR_INDEX:
				{
					int32		ActorIndex;
					char		ActorName[GAMEMGR_MAX_ACTOR_NAME];
					
					Buffer_GetSLong(&Buffer, &ActorIndex);
					Buffer_GetString(&Buffer, (uint8*)ActorName);

					if (Client->Mode == ClientMode_Dumb)
						break;			// Server already issued the load, just ignore
					
					if (!GameMgr_SetActorIndex(GMgr, ActorIndex, ActorName))
						GenVS_Error("ReadServerMessages:  GameMgr_SetActorIndex failed.\n");
	
					break;
				}

				case NETMGR_MSG_MOTION_INDEX:
				{
					GameMgr_MotionIndex		MotionIndex;
					char					MotionName[GAMEMGR_MAX_MOTION_NAME];
					
					Buffer_GetSLong(&Buffer, &MotionIndex);
					Buffer_GetString(&Buffer, (uint8*)MotionName);

					if (Client->Mode == ClientMode_Dumb)
						break;			// Server already issued the load, just ignore
					
					if (!GameMgr_SetMotionIndexDef(Client->GMgr, MotionIndex, MotionName))
						GenVS_Error("ReadServerMessages:  GameMgr_SetMotionIndexDef failed.\n");

					break;
				}

				case NETMGR_MSG_BONE_INDEX:
				{
					int32					BoneIndex;
					char					BoneName[GAMEMGR_MAX_BONE_NAME];
					
					Buffer_GetSLong(&Buffer, &BoneIndex);
					Buffer_GetString(&Buffer, (uint8*)BoneName);

					if (Client->Mode == ClientMode_Dumb)
						break;			// Server already issued the load, just ignore
					
					if (!GameMgr_SetBoneIndex(Client->GMgr, BoneIndex, BoneName))
						GenVS_Error("ReadServerMessages:  GameMgr_SetBoneIndex failed.\n");

					break;
				}

				case NETMGR_MSG_TEXTURE_INDEX:
				{
					int32		Index;
					char		FileName[256];
					char		AFileName[256];

					assert(Client->NetState == NetState_WorldChange);

					Buffer_GetSLong(&Buffer, &Index);
					Buffer_GetString(&Buffer, (uint8*)FileName);
					Buffer_GetString(&Buffer, (uint8*)AFileName);
					
					if (Client->Mode == ClientMode_Dumb)
						break;			// Server already issued the load, just ignore

					if (!GameMgr_SetTextureIndex(Client->GMgr, Index, FileName, AFileName))
						GenVS_Error("ReadServerMessages:  GameMgr_SetTextureIndex failed.\n");

					break;
				}

				case NETMGR_MSG_SOUND_INDEX:
				{
					int32		Index;
					char		FileName[GAMEMGR_MAX_FILENAME];

					Buffer_GetSLong(&Buffer, &Index);
					Buffer_GetString(&Buffer, (uint8*)FileName);
					
					if (Client->Mode == ClientMode_Dumb)
						break;			// Server already issued the load, just ignore

					if (!GameMgr_SetSoundIndex(Client->GMgr, Index, FileName))
						GenVS_Error("ReadServerMessages:  GameMgr_SetSoundIndex failed.\n");

					break;
				}

				case NETMGR_MSG_CD_TRACK:
				{
					uint8	Track, Smin, Ssec, Emin, Esec;

					Buffer_GetByte(&Buffer, &Track);
					Buffer_GetByte(&Buffer, &Smin);
					Buffer_GetByte(&Buffer, &Ssec);
					Buffer_GetByte(&Buffer, &Emin);
					Buffer_GetByte(&Buffer, &Esec);

					Track = 1;
					Smin = 0;
					Ssec = 0;
					Emin = 2;
					Esec = 42;

					//PlayCDTrack(GameMgr_GetCdID(Client->GMgr), 0, Track, Smin, Ssec, Emin, Esec);
					break;
				}
				
				case NETMGR_MSG_PLAY_SOUND_INDEX:
				{
					uint16				SoundIndex;
					geVec3d				Pos;
					geWorld				*World;
					GameMgr_SoundIndex	*pSoundIndex;
					geSound_System		*SoundSys;

					Buffer_GetShort(&Buffer, &SoundIndex);
					Buffer_GetPos(&Buffer, &Pos);
					
					World = GameMgr_GetWorld(Client->GMgr);

					if (!World)
						break;

					SoundSys = GameMgr_GetSoundSystem(Client->GMgr);
					//assert(SoundSys);

					if (SoundSys)
					{
						pSoundIndex = GameMgr_GetSoundIndex(Client->GMgr, SoundIndex);

						if (pSoundIndex->SoundDef)
						{
							float	Vol, Pan, Freq;

							geSound3D_GetConfig(World, &Client->ViewXForm, &Pos, 500.0f, 2.0f, &Vol, &Pan, &Freq);
							geSound_PlaySoundDef(SoundSys, pSoundIndex->SoundDef, Vol, Pan, Freq, GE_FALSE);
						}
					}
					
					break;
				}

				case NETMGR_MSG_CLIENT_INDEX:
				{
					uint8		ClientIndex;
					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");
					
					Client->ClientIndex = (int32)ClientIndex;

					break;
				}

				case NETMGR_MSG_CLIENT_ACTIVE:
				{
					uint8		ClientIndex, Active;
					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");
					
					Buffer_GetByte(&Buffer, &Active);

					Client->ClientInfo[ClientIndex].Active = (geBoolean)Active;
					break;
				}

				case NETMGR_MSG_CLIENT_NAME:
				{
					uint8		ClientIndex;
					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");
					
					Buffer_GetString(&Buffer, (uint8*)Client->ClientInfo[ClientIndex].Name);
					break;
				}

				case NETMGR_MSG_CLIENT_SCORE:
				{
					uint8		ClientIndex;
					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");
					
					Buffer_GetSLong(&Buffer, &Client->ClientInfo[ClientIndex].Score);
					break;
				}

				case NETMGR_MSG_CLIENT_HEALTH:
				{
					uint8		ClientIndex;
					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");
					
					Buffer_GetSLong(&Buffer, &Client->ClientInfo[ClientIndex].Health);
					break;
				}

				case NETMGR_MSG_CLIENT_INVENTORY:
				{
					uint8		ClientIndex, Slot;
					uint16		Val;
					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");
					
					Buffer_GetByte(&Buffer, &Slot);
					Buffer_GetShort(&Buffer, &Val);
					Client->Inventory[Slot] = Val;
					
					if (Client->ClientPlayer != -1)
					{
						GPlayer		*Player;
						int32		Ammo;
						geBoolean	HasItem;

						Player = &Client->Players[Client->ClientPlayer];

						Ammo = Client->Inventory[Slot];

						HasItem = (Val & (1<<15))>>15;
						Val &= 0xff;

						Player->Inventory[Slot] = Val;
						Player->InventoryHas[Slot] = HasItem;
					}
				
					break;
				}
				
				case NETMGR_MSG_CLIENT_WEAPON:
				{
					uint8		ClientIndex;

					Buffer_GetByte(&Buffer, &ClientIndex);
					
					if (ClientIndex >= NETMGR_MAX_CLIENTS)
						GenVS_Error("ReadServerMessages:  Bad client index number.\n");

					Buffer_GetShort(&Buffer, &Client->CurrentWeapon);
					break;
				}

				case NETMGR_MSG_SPAWN_FX:
				{
					uint8			Type, Sound;
					geVec3d			Pos;
					geWorld			*World;
					geSound_System	*SoundSys;
					GameMgr_SoundIndex	*pSoundIndex;

					Buffer_GetPos(&Buffer, &Pos);
					Buffer_GetByte(&Buffer, &Type);
					Buffer_GetByte(&Buffer, &Sound);

					World = GameMgr_GetWorld(Client->GMgr);
					if (!World)
						break;

					if (!Fx_SpawnFx(GameMgr_GetFxSystem(Client->GMgr), &Pos, Type))
						GenVS_Error("[CLIENT] ReadServerMessages:  Could not spawn fx:  %i.\n", Type);

					if (Sound == 255)
						break;

					SoundSys = GameMgr_GetSoundSystem(Client->GMgr);

					if (SoundSys)
					{
						pSoundIndex = GameMgr_GetSoundIndex(Client->GMgr, Sound);

						if (pSoundIndex->SoundDef)
						{
							float	Vol, Pan, Freq;

							assert(pSoundIndex->Active == GE_TRUE);

							geSound3D_GetConfig(World, &Client->ViewXForm, &Pos, 500.0f, 2.0f, &Vol, &Pan, &Freq);
								geSound_PlaySoundDef(SoundSys, pSoundIndex->SoundDef, Vol, Pan, Freq, GE_FALSE);
						}
					}

					break;
				}


				case NETMGR_MSG_HEADER_PRINTF:
				{
					char	Str[256];

					Buffer_GetString(&Buffer, (uint8*)Str);

					Console_HeaderPrintf(GameMgr_GetConsole(Client->GMgr), Str);

					break;
				}

				case NETMGR_MSG_CLIENT_PLAYER_INDEX:
				{
					int32		PlayerIndex;

					Buffer_GetSLong(&Buffer, &PlayerIndex);

					SetClientProxyPlayer(Client, PlayerIndex);
					break;
				}

				case NETMGR_MSG_NET_STATE_CHANGE:
				{
					NetMgr_NetState	NetState;

					Buffer_GetSLong(&Buffer, &NetState);

					if (!Client_ChangeNetState(Client, NetState))
						GenVS_Error("ReadServerMessages:  Client_ChangeNetState failed.\n");

					break;
				}

				case NETMGR_MSG_SHUTDOWN:
				{
					GenVS_Error("Lost connection with server!!!");
				}

				default:
				{
					GenVS_Error("Unknown message from server: %i\n", Type);
				}
			}
		}

		// Draw every frame if full speed is set
		if (Client->Demo.Mode == CLIENT_DEMO_PLAY && !Client->Demo.OriginalSpeed)
			break;
	}

	// Update the players.  NOTE - This is where this client's view xform gets built.
	UpdatePlayers(Client, Time);
	
#if 0
	{
		/*
		int32	NumMsgSend, NumBytesSend, NumMsgRec, NumBytesRec;

		if (!NetPlayGetNumMessages(&NumMsgSend, &NumBytesSend, &NumMsgRec, &NumBytesRec))
			GenVS_Error("ReadServerMessages:  NetPlayGetNumMessages failed.\n");
		geEngine_Printf(GameMgr_GetEngine(Client->GMgr), 2, 380, "Msg Queue: %i, %i, %i, %i", NumMsgSend, NumBytesSend, NumMsgRec, NumBytesRec);
		*/
		geEngine_Printf(GameMgr_GetEngine(Client->GMgr), 2, 400, "Bytes Received: %i", TotalMsgBytes);
	}
#endif

	return GE_TRUE;
}

//===========================================================================
//	UpdateProxyPlayer
//	On each update from the server, make sure the PROXY players on the
//	PROXY server are up to snuff with the BOSS server...
//===========================================================================
static void UpdateProxyPlayer(Client_Client *Client, GPlayer *Player)
{
	int32		PlayerIndex;
	geXForm3d	XForm;
	GPlayer		*Player2;
	int32		i;

	assert(Client_IsValid(Client) == GE_TRUE);

	PlayerIndex = (int32)(Player - Client->Players);

	// Set this player where the server says the player is...
	Client_UpdateSinglePlayer(Client, Player, 1.0f, 1.0f, GE_FALSE);

	// If this player, is our client player, replay all the moves that have happened since this update
	if (PlayerIndex == Client->ClientPlayer)
	{
		Client_Move *Move, *CMove;
		int32		NumMoves2;

		MHack = GE_TRUE;
		
		geXForm3d_SetEulerAngles(&XForm, &Player->Angles);
		XForm.Translation = Player->Pos;
		Player->LastGoodPos = Player->Pos;

		Player->XForm = XForm;	

		//if (LastPlayer != PlayerIndex && LastPlayer != -1)
		//	GenVS_Error("LastPlayer != PlayerIndex");
		
		LastPlayer = PlayerIndex;

		Move = Client_PeekMove(Client);

		// Take all the clients moves after this message, and re-run them.
		// We must do this since the server might think we are somewhere in the past, and we
		// need to predict (from the servers position) where we are now...
		for (Move = Client_PeekMove(Client); Move; Move = Client_PeekMove(Client))
		{
			if (Move->Time > Player->SpawnTime)		// Look for moves after this update
				break;

			Client_RemoveFirstMove(Client);
		}

		NumMoves2 = 0;

		// Re-run all the moves from this time on
		for (CMove = Move; CMove; CMove = CMove->Next)
		{
			NumMoves2++;

			Client_MoveClientLocally(Client, CMove);
		}
		
		// Destroy all temp players that are before this update
		Player2 = Client->TempPlayers;

		for (i=0; i< CLIENT_MAX_TEMP_PLAYERS; i++, Player2++)
		{
			if (!Player2->Active)
				continue;

			if (Player->SpawnTime < Player2->SpawnTime)	
				continue;

			Client_DestroyTempPlayer(Client, Player2);
		}

	}
}

#define CLIENT_POLY_FLAGS	(GE_RENDER_DEPTH_SORT_BF)
//#define CLIENT_POLY_FLAGS	(GE_RENDER_DO_NOT_OCCLUDE_OTHERS)

//===========================================================================
//	CheckClientPlayerChanges
//	Simply sees what has changed in a client player.  Stuff like different mesh/actors,
//	lights, etc. 
//===========================================================================
static geBoolean CheckClientPlayerChanges(Client_Client *Client, GPlayer *Player, geBoolean TempPlayer)
{
	int32		ViewFlags;
	int32		ViewIndex;
	int32		PlayerIndex;
	geVec3d		PlayerPos;
	geWorld		*World;

	assert(Client_IsValid(Client) == GE_TRUE);

	World = GameMgr_GetWorld(Client->GMgr);
	assert(World);		// This better be true to be here!!!

	if (TempPlayer)	
		PlayerIndex = CLIENT_TEMP_GPLAYER_TO_INDEX(Client, Player);
	else
		PlayerIndex = CLIENT_GPLAYER_TO_INDEX(Client, Player);

	ViewFlags = Player->ViewFlags;
	ViewIndex = Player->ViewIndex;

	PlayerPos = Player->Pos;

	if (ViewFlags & VIEW_TYPE_ACTOR)
	{
		if (ViewIndex == 0xffff)							// Remove old actors if they are attached...
		{
			if (Player->Actor)
			{
				geWorld_RemoveActor(World, Player->Actor);
				geActor_Destroy(&Player->Actor);
				Player->Actor = NULL;
			}
		}							
		else if (Player->OldViewIndex2 != ViewIndex)		// if the Index changed, then make sure it gets applied
		{
			geActor_Def			*ActorDef;
			GameMgr_ActorIndex	*ActorIndex;
			geVec3d				Normal = {0.0f, 1.0f, 0.0f};
			geVec3d				Mins;
			geVec3d				Maxs;
			geExtBox			ExtBox;
			geVec3d				NullVec = {0.0f, 0.0f, 0.0f};

			ActorIndex = GameMgr_GetActorIndex(Client->GMgr, ViewIndex);

			assert(ActorIndex->Active == GE_TRUE);

			ActorDef = ActorIndex->ActorDef;

			assert(ActorDef);

			if (Player->Actor)								// Remove any old actors...
			{
				geWorld_RemoveActor(World, Player->Actor);
				geActor_Destroy(&Player->Actor);
				Player->Actor = NULL;
			}
			
			Mins = Player->Mins;		 
			Maxs = Player->Maxs;		 

			assert(geVec3d_Compare(&Mins, &NullVec, 0.01f) == GE_FALSE);
			assert(geVec3d_Compare(&Maxs, &NullVec, 0.01f) == GE_FALSE);
			
			Player->Actor = geActor_Create(ActorDef);

			if	(!Player->Actor)
				GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Could not create actor.  ActorDef Name: %s.\n", ActorIndex->FileName);

			if (Client->ViewPlayer == PlayerIndex && !TempPlayer)			// Only render viewplayer in mirrors
			{
				if	(geWorld_AddActor(World, Player->Actor, GE_ACTOR_RENDER_MIRRORS|GE_ACTOR_COLLIDE, 0xffffffff) == GE_FALSE)
					GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Could not add actor to world.  ActorDef Name: %s.\n", ActorIndex->FileName);
 			}
			else
			{
				if	(geWorld_AddActor(World, Player->Actor, GE_ACTOR_RENDER_NORMAL|GE_ACTOR_COLLIDE, 0xffffffff) == GE_FALSE)
					GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Could not add actor to world.  ActorDef Name: %s.\n", ActorIndex->FileName);
			}

			geActor_SetScale(Player->Actor, Player->Scale, Player->Scale, Player->Scale);

			ExtBox.Min = Mins;
			ExtBox.Max = Maxs;

			if (!geActor_SetRenderHintExtBox(Player->Actor, &ExtBox, NULL))
				GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Set actor HintBox failed.\n");
			
			if (!geActor_SetExtBox(Player->Actor, &ExtBox, NULL))
				GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Set actor AABox failed.\n");

			if (Client->ViewPlayer == PlayerIndex && !TempPlayer)
			{
				if (!geActor_SetShadow(Player->Actor, GE_TRUE, 100.0f, GameMgr_GetShadowMap(Client->GMgr), NULL))
					GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Could not set actor shadow.  ActorDef Name: %s.\n", ActorIndex->FileName);
			}

			if (!geActor_SetLightingOptions(Player->Actor, GE_TRUE, &Normal, 155.0f, 155.0f, 155.0f, 1.0f, 1.0f, 1.0f, GE_TRUE, 3, NULL, GE_FALSE))
				GenVS_Error("[CLIENT] CheckClientPlayerChanges:  Could not set actor lighting options.  ActorDef Name: %s.\n", ActorIndex->FileName);

			geActor_SetUserData(Player->Actor, Player);
			
			Player->ActorDef = ActorDef;
			
		
		}
	}
	else if (ViewFlags & VIEW_TYPE_MODEL)
	{
		if (ViewIndex == 0xffff)
			Player->Model = NULL;
		else
			Player->Model = GameMgr_GetModel(Client->GMgr, ViewIndex);

		if (Player->Model)
		{
			if (Player->ViewFlags & VIEW_TYPE_MODEL_OPEN)
				geWorld_OpenModel(World, Player->Model, GE_TRUE);
			else
				geWorld_OpenModel(World, Player->Model, GE_FALSE);

			if (Client->Mode == ClientMode_Proxy)
			{
				geWorld_ModelSetUserData(Player->Model, Player);
			}
		}
	}
	else if (ViewFlags & VIEW_TYPE_SPRITE)
	{
		if (ViewIndex == 0xffff)
		{
			if (Player->Poly)
			{
				geWorld_RemovePoly(World, Player->Poly);
				Player->Poly = NULL;
			}
		}
		// if the Index changed, or the Scale changed, then make sure it gets applied
		else if (Player->OldViewIndex2 != ViewIndex || Player->OldScale2 != Player->Scale)		
		{
			geBitmap				*Bitmap;
			GE_LVertex				Vert;
			GameMgr_TextureIndex	*TextureIndex;

			TextureIndex = GameMgr_GetTextureIndex(Client->GMgr, ViewIndex);

			Bitmap = TextureIndex->TextureDef;

			assert(Bitmap);

			if (Bitmap)
			{
				geVec3d	Pos = Player->Pos;

				if (Player->Poly)		// Remove any old polys
					geWorld_RemovePoly(World, Player->Poly);
		
				Vert.X = Pos.X;
				Vert.Y = Pos.Y;
				Vert.Z = Pos.Z;
				Vert.u = 0.0f;
				Vert.v = 0.0f;
				Vert.r = Vert.g = Vert.b = Vert.a = 255.0f;

				Player->Poly = 
					geWorld_AddPoly(World, &Vert, 1, Bitmap, GE_TEXTURED_POINT, 
							CLIENT_POLY_FLAGS, Player->Scale * EffectScale);

				assert(Player->Poly);
			}
		}
	}
	
	// Lights can be mixed with the others...
	if (ViewFlags & VIEW_TYPE_LIGHT)
	{
		if (!Player->Light)
		{
			// If the light has not been created yet, create it now...
			Player->Light = geWorld_AddLight(World);

			// NOTE - That the engine might return NULL.  We are not worried about this, because
			// it juts means there won't be a light for this player...
		}
	}
	else if (Player->Light)		// Remove the light if there is one...
	{
		geWorld_RemoveLight(World, Player->Light);
		Player->Light = NULL;
	}

	// Make these flags recent...
	Player->OldViewFlags2 = Player->ViewFlags;
	Player->OldViewIndex2 = Player->ViewIndex;
	Player->OldScale2 = Player->Scale;

	return GE_TRUE;
}

//=====================================================================================
//	BuildXForm
//=====================================================================================
void BuildXForm(geXForm3d *XForm, const geVec3d *Angles)
{
	geVec3d		Pos;

	Pos = XForm->Translation;

	// Clear the matrix
	geXForm3d_SetIdentity(XForm);

	// Rotate then translate.
	geXForm3d_RotateZ(XForm, Angles->Z);

	geXForm3d_RotateX(XForm, Angles->X);
	geXForm3d_RotateY(XForm, Angles->Y);
	
	geXForm3d_Translate(XForm, Pos.X, Pos.Y, Pos.Z);

}

//=====================================================================================
//	BuildClientViewXForm
//	Builds the client view xform
//	Short-curcuits the process by using the local angles that were sent in the last
//	move intention, unless told otherwise by the viewplayer, or demomode...
//=====================================================================================
static void BuildClientViewXForm(Client_Client *Client)
{
	geVec3d			Pos;
	GPlayer			*Player;

	assert(Client_IsValid(Client) == GE_TRUE);

	assert(Client->ViewPlayer >= 0 && Client->ViewPlayer < NETMGR_MAX_PLAYERS);
	
	Player = &Client->Players[Client->ViewPlayer];

	Client->ViewXForm = Player->XForm;

	if (Client->Demo.Mode != CLIENT_DEMO_PLAY && !(Player->ViewFlags & VIEW_TYPE_FORCE_XFORM))
	{
		Pos = Client->ViewXForm.Translation;

		// Clear the matrix
		geXForm3d_SetIdentity(&Client->ViewXForm);

		// Rotate then translate.
		geXForm3d_RotateZ(&Client->ViewXForm, Client->Angles.Z);

		geXForm3d_RotateX(&Client->ViewXForm, Client->Angles.X);
		geXForm3d_RotateY(&Client->ViewXForm, Client->Angles.Y);
	
		geXForm3d_Translate(&Client->ViewXForm, Pos.X, Pos.Y, Pos.Z);
	}

	// HACK the view height till we get it in 
	// Just use a vakue that looks good for now...
	Client->ViewXForm.Translation.Y += 140.0f;
}

//===========================================================================
//	Client_UpdateSinglePlayer
//===========================================================================
geBoolean Client_UpdateSinglePlayer(Client_Client *Client, GPlayer *Player, float Ratio, float Time, geBoolean TempPlayer)
{
	geXForm3d		XForm;
	geWorld			*World;

	assert(Client_IsValid(Client) == GE_TRUE);

	World = GameMgr_GetWorld(Client->GMgr);

	// Can't mess with hardly anything if the world is not loaded
	// so just return and wait for the world to be loaded...
	if (!World)
		return GE_TRUE;

	// Took this out because it was preventing my bots from getting an actor
	//if (Client->ViewPlayer == -1)		// Don't do nothing till we know who represents our view
	//	return GE_TRUE;
	
	geXForm3d_SetEulerAngles(&XForm, &Player->Angles);
	XForm.Translation = Player->Pos;

	// Save the transform
	Player->XForm = XForm;

	// See what has changed recently about the players ViewFlags, ViewIndex, etc...
	CheckClientPlayerChanges(Client, Player, TempPlayer);

	// Update any valid world entities that may be attached to this player
	if (Player->Light)
	{
		GE_RGBA	RGBA = {255.0f, 55.0f, 15.0f, 255.0f};
		geWorld_SetLightAttributes(World, Player->Light, 
								   &Player->Pos, &RGBA, 320.0f, GE_FALSE);
	}

	if (Player->Mesh)
	{
		geXForm3d	TXForm;
		geVec3d		In, Up, Lf, Pos;

		Pos = Player->XForm.Translation;

		if (Player->ViewFlags & VIEW_TYPE_YAW_ONLY)
		{
			geVec3d_Set(&Up, 0.0f, 1.0f, 0.0f);

			geXForm3d_GetIn(&Player->XForm, &In);
			geXForm3d_GetLeft(&Player->XForm, &Lf);

			geVec3d_CrossProduct(&In, &Up, &Lf);
			geVec3d_CrossProduct(&Lf, &Up, &In);

			geVec3d_Normalize(&Lf);
			geVec3d_Normalize(&In);
		
			geXForm3d_SetFromLeftUpIn(&TXForm, &Lf, &Up, &In);
		}
		else
		{
			TXForm = Player->XForm;
		}

		TXForm.Translation = Pos;

#if 0
		geWorld_SetMeshXForm(World, Player->Mesh, &TXForm);
		geWorld_SetMeshFrame(World, Player->Mesh, (int32)Player->FrameTime);
#endif
	}

	if (Player->Actor)
	{
		geXForm3d				XForm;
		geVec3d					In, Up, Lf, Pos;
		geMotion				*Motion;
		char					*MotionName;
		GameMgr_MotionIndexDef	*MotionIndex;
		
		assert((Player->ViewFlags & VIEW_TYPE_ACTOR));

		Pos = Player->XForm.Translation;

		if (Player->ViewFlags & VIEW_TYPE_YAW_ONLY)
		{
			geVec3d_Set(&Up, 0.0f, 1.0f, 0.0f);

			geXForm3d_GetIn(&Player->XForm, &In);
			geXForm3d_GetLeft(&Player->XForm, &Lf);

			geVec3d_CrossProduct(&In, &Up, &Lf);
			geVec3d_CrossProduct(&Lf, &Up, &In);

			geVec3d_Normalize(&Lf);
			geVec3d_Normalize(&In);
		
			geXForm3d_SetFromLeftUpIn(&XForm, &Lf, &Up, &In);
		}
		else
		{
			XForm = Player->XForm;
		}
		
		if (Player->ViewFlags & VIEW_TYPE_ACTOR)
		{
			
			geXForm3d	RXForm;
			geXForm3d_SetXRotation(&RXForm, -(3.14159f/2.0f));
			geXForm3d_Multiply(&XForm, &RXForm, &XForm);
		}
		
		XForm.Translation = Pos;

		if (Player->MotionIndex == GAMEMGR_MOTION_INDEX_NONE)
		{
			if (!geActor_SetBoneAttachment(Player->Actor, NULL, &XForm))
			{
				GenVS_Error("Client_UpdateSinglePlayer:  geActor_SetBoneAttachment failed...");
			}
		}
		else
		{
			MotionIndex = GameMgr_GetMotionIndexDef(Client->GMgr, Player->MotionIndex);

			assert(MotionIndex);
			assert(MotionIndex->Active == GE_TRUE);

			// Set the base motion and transform
			if (MotionIndex->Active)
			{
				float FrameTime;
							
				if (Player->FrameTime > Player->OldFrameTime)
					FrameTime = Player->OldFrameTime + (Player->FrameTime - Player->OldFrameTime) * Ratio;
				else
					FrameTime = Player->FrameTime;

				MotionName = MotionIndex->MotionName;
			
				Motion = geActor_GetMotionByName(Player->ActorDef, MotionName);
				
				if (!Motion)
					GenVS_Error("Client_UpdateSinglePlayer:  geActor_GetMotionByName1 failed...");

				geActor_SetPose(Player->Actor, Motion, FrameTime, &XForm);
			}
		}

		// Go through and blend in all the motion hacks...
		{
			int32				i;
			GPlayer_MotionData	*pMotionData;

			pMotionData = Player->MotionData;

			for (i=0; i< Player->NumMotionData; i++, pMotionData++)
			{
				MotionIndex = GameMgr_GetMotionIndexDef(Client->GMgr, pMotionData->MotionIndex);

				assert(MotionIndex->Active == GE_TRUE);

				if (!MotionIndex->Active)
					continue;

				Motion = geActor_GetMotionByName(Player->ActorDef, MotionIndex->MotionName);

				if (!Motion)
					GenVS_Error("Client_UpdateSinglePlayer:  geActor_GetMotionByName2 failed...");

				geActor_BlendPose(Player->Actor, Motion, pMotionData->MotionTime, &XForm, 1.0f);
			}
		}

		// Go through and set all the xform hacks for the bones...
		{
			int32				i;
			GPlayer_XFormData	*pXFormData;
			GameMgr_BoneIndex	*pBoneIndex;
			geXForm3d			BoneXForm;

			pXFormData = Player->XFormData;

			for (i=0; i< Player->NumXFormData; i++, pXFormData++)
			{
				pBoneIndex = GameMgr_GetBoneIndex(Client->GMgr, pXFormData->BoneIndex);

				assert(pBoneIndex->Active == GE_TRUE);

				if (!pBoneIndex->Active)
					continue;

				geActor_GetBoneAttachment(Player->Actor, pBoneIndex->BoneName, &BoneXForm);

				pXFormData->XForm.Translation = BoneXForm.Translation;

				geActor_SetBoneAttachment(Player->Actor, pBoneIndex->BoneName, &pXFormData->XForm);
			}
		}
				
	}

	if (Player->Model)// && !Host->Server)
	{
		geWorld_SetModelXForm(World, Player->Model, &XForm);
	}

	if (Player->Poly && Player->ViewFlags & VIEW_TYPE_SPRITE)
	{
		GE_LVertex	Vert;

		Vert.X = Player->Pos.X;
		Vert.Y = Player->Pos.Y;
		Vert.Z = Player->Pos.Z;
		Vert.u = 0.0f;
		Vert.v = 0.0f;
		Vert.r = Vert.a = 255.0f;
		Vert.g = Vert.b = 255.0f;

		gePoly_SetLVertex(Player->Poly, 0, &Vert);
	}
			
	// Run this players effects
	if (TempPlayer)
	{
		if (!Fx_PlayerFrame(GameMgr_GetFxSystem(Client->GMgr), TEMP_PLAYER_TO_FXPLAYER(Client, Player), &Player->XForm, Time))
			GenVS_Error("[CLIENT] UpdateSinglePlayer:  Could not run Fx_Player frame.\n");
	}
	else
	{
		if (!Fx_PlayerFrame(GameMgr_GetFxSystem(Client->GMgr), PLAYER_TO_FXPLAYER(Client, Player), &Player->XForm, Time))
			GenVS_Error("[CLIENT] UpdateSinglePlayer:  Could not run Fx_Player frame.\n");
	}

	return GE_TRUE;
}

static void *g_ProcIndex[CLIENT_MAX_PROC_INDEX];

//===========================================================================
//	Client_GetProcIndex
//===========================================================================
void *Client_GetProcIndex(Client_Client *Client, uint16 Index)
{
	assert(Client_IsValid(Client) == GE_TRUE);

	if (Index == CONTROL_INDEX_NONE)
		return NULL;			// This is NOT an error

	assert( Index >= 0 );
	assert( Index < CLIENT_MAX_PROC_INDEX );

	return Client->ProcIndex[Index];
}

//===========================================================================
//	UpdatePlayers
//===========================================================================
static void UpdatePlayers(Client_Client *Client, float Time)
{
	int32			i;
	GPlayer			*Player;
	float			Ratio, Delta;
	geWorld			*World;

	assert(Client_IsValid(Client) == GE_TRUE);

	World = GameMgr_GetWorld(Client->GMgr);
	
	if (!World)				// If no world loaded, then don't do anything...
		return;

	if (Client->ViewPlayer == -1)	// Don't do nothing till we know who represents our view
		return;

	// Find the distance between the last 2 messages from the server
	Delta = Client->NetTime - Client->OldNetTime;

	// Try to predict where we will be in between the last 2 messages from the server
	if (Delta <= 0.0f)
		Ratio = 1.0f;
	else
		Ratio = Client->TempTime / Delta;

	if (Ratio < 0.0f)
		Ratio = 0.0f;
	else if (Ratio > 1.0f)
		Ratio = 1.0f;

	// Move from the start of the last message up to the current message...
	Client->TempTime += Time;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		Player = &Client->Players[i];

		// If player was not inluded in last update, remove all it's actors/lights/etc from the world
		// and reset this player slot
		if (Player->UpdateTime != Client->NetTime)	
		{
			// Make sure the fx are shutdown for this player
			if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), PLAYER_TO_FXPLAYER(Client, Player), 0))
				GenVS_Error("[CLIENT] UpdatePlayers:  Could not set Fx_Player flags.\n");

			if (Player->Actor)
			{
				geWorld_RemoveActor(World, Player->Actor);
				geActor_Destroy(&Player->Actor);
			}

			if (Player->Light)
				geWorld_RemoveLight(World, Player->Light);
			
			if (Player->Poly)
				geWorld_RemovePoly(World, Player->Poly);

			// Prepare the player for a fresh renewal later on...
			memset(Player, 0, sizeof(GPlayer));	// Clear the player

			// Reset player default
			SetPlayerDefaults(Player);
			continue;
		}
		
		Client_UpdateSinglePlayer(Client, Player, 1.0f, Time, GE_FALSE);

	#if 1
		Player->Control = static_cast<GPlayer_Control*>(Client_GetProcIndex(Client, Player->ControlIndex));

		// Call the control code for this player (if it's not this local client), and if not in demo mode
		if (	(Client->Demo.Mode != CLIENT_DEMO_PLAY) 
			&&  (Client->Mode == ClientMode_Proxy)
			&& 	(i != Client->ClientPlayer)
			&&  (Player->Control) )
		{
			Player->Control(&Client->GenVSI, Player, Time);
			Player->Pos = Player->XForm.Translation;
		}
	#endif
		
	
	}

	// Build the camera rotation from our local angles, so we can see where we are at...
	BuildClientViewXForm(Client);
}

//=====================================================================================
//	Client_ParsePlayerData
//	If AuxPlayer is not NULL, then that is where the data is read into
//=====================================================================================
GPlayer *Client_ParsePlayerData(Client_Client *Client, Buffer_Data *Buffer, GPlayer *AuxPlayer)
{
	uint16			PlayerIndex;
	uint16			SendFlags, PlayerIndexFlags;
	GPlayer			*pPlayer;

	assert(Client_IsValid(Client) == GE_TRUE);

	Buffer_GetShort(Buffer, &PlayerIndex);

	PlayerIndexFlags = (uint16)(PlayerIndex & (1<<15));
	PlayerIndex &= (uint16)0x7FFF;
	
	if (AuxPlayer)
		pPlayer = AuxPlayer;
	else
		pPlayer = &Client->Players[PlayerIndex];

	pPlayer->Active = GE_TRUE;

	pPlayer->UpdateTime = Client->NetTime;		// Make this player recent
	
	if (!(PlayerIndexFlags & (1<<15)) )
	{
		// Fast update, use all old known flags
		pPlayer->SpawnTime = pPlayer->OldSpawnTime;
		pPlayer->ViewFlags = pPlayer->OldViewFlags;
		pPlayer->ViewIndex = pPlayer->OldViewIndex;
		pPlayer->MotionIndex = pPlayer->OldMotionIndex;
		pPlayer->FxFlags = pPlayer->OldFxFlags;

		if (!AuxPlayer)
		{
			if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), PLAYER_TO_FXPLAYER(Client, pPlayer), pPlayer->FxFlags))
				GenVS_Error("Client_ParsePlayerData:  Fx_PlayerSetFxFlags failed.\n");
		}

		pPlayer->Pos = pPlayer->OldPos;
		pPlayer->Angles = pPlayer->OldAngles;
		pPlayer->FrameTime = pPlayer->OldFrameTime;
		pPlayer->Scale = pPlayer->OldScale;
		pPlayer->Velocity = pPlayer->OldVelocity;
		pPlayer->State = pPlayer->OldState;
		pPlayer->ControlIndex = pPlayer->OldControlIndex;
		pPlayer->TriggerIndex = pPlayer->OldTriggerIndex;
		pPlayer->Mins = pPlayer->OldMins;
		pPlayer->Maxs = pPlayer->OldMaxs;

		if (Client->Mode == ClientMode_Proxy)
		{
			// Set the control func here
			if (pPlayer->ControlIndex == 0xffff)
				pPlayer->Control = NULL;
			else
				pPlayer->Control = static_cast<GPlayer_Control*>(Client->ProcIndex[pPlayer->ControlIndex]);

			// Set the control func here
			if (pPlayer->TriggerIndex == 0xffff)
				pPlayer->Trigger = NULL;
			else
			{
				pPlayer->Trigger = static_cast<GPlayer_Trigger*>(Client->ProcIndex[pPlayer->TriggerIndex]);
			}
		}

		return pPlayer;		// Nothing to read, fast update
	}
	
	// See what we need to read for this player
	// If somthing changed, we grab it, and store it in the old variable.
	// If somthing did not change, we reset it back to the old one..
	Buffer_GetShort(Buffer, &SendFlags);

	// Get the data
	if (SendFlags & NETMGR_SEND_SPAWN_TIME)
	{
		Buffer_GetFloat(Buffer, &pPlayer->SpawnTime);
		pPlayer->OldSpawnTime = pPlayer->SpawnTime;
	}
	else
		pPlayer->SpawnTime = pPlayer->OldSpawnTime;

	if (SendFlags & NETMGR_SEND_VIEW_FLAGS)
	{
		Buffer_GetShort(Buffer, &pPlayer->ViewFlags);
		pPlayer->OldViewFlags = pPlayer->ViewFlags;
	}
	else
		pPlayer->ViewFlags = pPlayer->OldViewFlags;

	if (SendFlags & NETMGR_SEND_VIEW_INDEX)
	{
		Buffer_GetShort(Buffer, &pPlayer->ViewIndex);
		pPlayer->OldViewIndex = pPlayer->ViewIndex;
	}
	else
		pPlayer->ViewIndex = pPlayer->OldViewIndex;

	if (SendFlags & NETMGR_SEND_MOTION_INDEX)
	{
		Buffer_GetByte(Buffer, &pPlayer->MotionIndex);
		pPlayer->OldMotionIndex = pPlayer->MotionIndex;
	}
	else
		pPlayer->MotionIndex = pPlayer->OldMotionIndex;

	if (SendFlags & NETMGR_SEND_FX_FLAGS)
	{
		Buffer_GetShort(Buffer, &pPlayer->FxFlags);
		pPlayer->OldFxFlags = pPlayer->FxFlags;
	}
	else 
		pPlayer->FxFlags = pPlayer->OldFxFlags;

	if (SendFlags & NETMGR_SEND_POS)
	{
		Buffer_GetPos(Buffer, &pPlayer->Pos);
		pPlayer->OldPos = pPlayer->Pos;
	}
	else
		pPlayer->Pos = pPlayer->OldPos;

	if (SendFlags & NETMGR_SEND_ANGLES)
	{
		Buffer_GetAngle(Buffer, &pPlayer->Angles);
		pPlayer->OldAngles = pPlayer->Angles;
	}
	else
		pPlayer->Angles = pPlayer->OldAngles;

	if (SendFlags & NETMGR_SEND_FRAME_TIME)
	{
		Buffer_GetFloat2(Buffer, &pPlayer->FrameTime, 60.0f);
		pPlayer->OldFrameTime = pPlayer->FrameTime;
	}
	else
		pPlayer->FrameTime = pPlayer->OldFrameTime;

	if (SendFlags & NETMGR_SEND_SCALE)
	{
		Buffer_GetFloat2(Buffer, &pPlayer->Scale, 100.0f);
		pPlayer->OldScale = pPlayer->Scale;
	}
	else
		pPlayer->Scale = pPlayer->OldScale;

	if (SendFlags & NETMGR_SEND_VELOCITY)
	{
		Buffer_GetAngle(Buffer, &pPlayer->Velocity);
		pPlayer->OldVelocity = pPlayer->Velocity;
	}
	else
		pPlayer->Velocity = pPlayer->OldVelocity;

	if (SendFlags & NETMGR_SEND_STATE)
	{
		uint8	State;

		Buffer_GetByte(Buffer, &State);
		pPlayer->State = (GPlayer_PState)State;
		pPlayer->OldState = pPlayer->State;
	}
	else
		pPlayer->State = pPlayer->OldState;

	if (SendFlags & NETMGR_SEND_CONTROL_INDEX)
	{
		pPlayer->OldControlIndex = pPlayer->ControlIndex;
		Buffer_GetShort(Buffer, &pPlayer->ControlIndex);

		pPlayer->OldControlIndex = pPlayer->ControlIndex;
	}
	else
		pPlayer->ControlIndex = pPlayer->OldControlIndex;
	
	if (SendFlags & NETMGR_SEND_TRIGGER_INDEX)
	{
		pPlayer->OldTriggerIndex = pPlayer->TriggerIndex;
		Buffer_GetShort(Buffer, &pPlayer->TriggerIndex);

		pPlayer->OldTriggerIndex = pPlayer->TriggerIndex;
	}
	else
		pPlayer->TriggerIndex = pPlayer->OldTriggerIndex;

	//	Set control/trigger functions
	if (Client->Mode == ClientMode_Proxy)
	{
		// Set the control func here
		if (pPlayer->ControlIndex == 0xffff)
			pPlayer->Control = NULL;
		else
			pPlayer->Control = static_cast<GPlayer_Control*>(Client->ProcIndex[pPlayer->ControlIndex]);

		// Set the control func here
		if (pPlayer->TriggerIndex == 0xffff)
			pPlayer->Trigger = NULL;
		else
		{
			pPlayer->Trigger = static_cast<GPlayer_Trigger*>(Client->ProcIndex[pPlayer->TriggerIndex]);
		}
	}

	// Update the players fx system 
	if (!AuxPlayer)
	{
		if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), PLAYER_TO_FXPLAYER(Client, pPlayer), pPlayer->FxFlags))
			GenVS_Error("Client_ParsePlayerData:  Fx_PlayerSetFxFlags failed.\n");
	}

	if (SendFlags & NETMGR_SEND_MINS_MAXS)
	{
		Buffer_GetPos(Buffer, &pPlayer->Mins);
		Buffer_GetPos(Buffer, &pPlayer->Maxs);


		pPlayer->OldMins = pPlayer->Mins;
		pPlayer->OldMaxs = pPlayer->Maxs;
	}
	else
	{
		pPlayer->Mins = pPlayer->OldMins;
		pPlayer->Maxs = pPlayer->OldMaxs;
	}

#if 0
	// TOTAL hack for now... (this is a way to get extra xforms in a player for actors
	if (pPlayer->ViewFlags & VIEW_TYPE_HACK)
	{
		GPlayer_XFormData	*pXFormData;
		int32				i;
		uint8				NumXFormData;

		Buffer_GetByte(Buffer, &NumXFormData);

		pPlayer->NumXFormData = (uint8)NumXFormData;

		pXFormData = pPlayer->XFormData;
		
		for (i=0; i<pPlayer->NumXFormData; i++, pXFormData++)
		{
			geVec3d		EulerAngles;

			Buffer_GetAngle(Buffer, &EulerAngles);
			geXForm3d_SetEulerAngles(&pXFormData->XForm, &EulerAngles);
			Buffer_GetPos(Buffer, &pXFormData->XForm.Translation);
			Buffer_GetByte(Buffer, &pXFormData->BoneIndex);
		}
	}
	
	// TOTAL hack for now... (this is a way to get extra motions in a player for actors
	if (pPlayer->ViewFlags & VIEW_TYPE_HACK)
	{
		GPlayer_MotionData	*pMotionData;
		int32				i;
		uint8				NumMotionData;

		Buffer_GetByte(Buffer, &NumMotionData);

		pPlayer->NumMotionData = NumMotionData;

		pMotionData = pPlayer->MotionData;

		for (i=0; i<pPlayer->NumMotionData; i++, pMotionData++)
		{
			Buffer_GetFloat(Buffer, &pMotionData->MotionTime);
			Buffer_GetByte(Buffer, &pMotionData->MotionIndex);
		}
	}
	
#endif
	
	return pPlayer;
}

//=====================================================================================
//	ParsePlayerDataLocally
//=====================================================================================
static void ParsePlayerDataLocally(Client_Client *Client, Buffer_Data *Buffer, geBoolean Fake)
{
	GPlayer		*Player;

	assert(Client_IsValid(Client) == GE_TRUE);

	if (Fake)
	{
		GPlayer		FakePlayer;

		memset(&FakePlayer, 0, sizeof(GPlayer));

		Player = Client_ParsePlayerData(Client, Buffer, &FakePlayer);

		return;
	}

	Player = Client_ParsePlayerData(Client, Buffer, NULL);

	if (!Player)
		GenVS_Error("[CLIENT] ParsePlayerDataLocally:  Could not parse data.\n");

#ifdef PREDICT_CLIENT
	// Update the proxy players with this server update
	if (Client->Mode == ClientMode_Proxy)
		UpdateProxyPlayer(Client, Player);
#endif
}

//=====================================================================================
//	RenderWorld
//=====================================================================================
static geBoolean RenderWorld(Client_Client *Client, GameMgr *GMgr, float Time)
{
	GE_Rect		Rect;
	geCamera	*Camera;
	geEngine	*Engine;
	geWorld		*World;
	int			Width,Height;

	assert(Client_IsValid(Client) == GE_TRUE);

	World = GameMgr_GetWorld(GMgr);
	
	if (!World)
		return GE_TRUE;
		
	VidMode_GetResolution(Client->VidMode,&Width,&Height);

	Rect.Left = 0;
	Rect.Right = Width-1;
	Rect.Top = 0;
	
	if (Width<SMALL_CONSOLE_CUTOFF_WIDTH)
		Rect.Bottom = Height - 1 - CONSOLE_HEIGHT_320;
	else
		Rect.Bottom = Height - 1 - CONSOLE_HEIGHT_640;

	Engine = GameMgr_GetEngine(GMgr);
	assert(Engine);

	Camera = GameMgr_GetCamera(GMgr);
	assert(Camera);

	SetupCamera(Camera, &Rect, &Client->ViewXForm);

	// Run the coronas
	if (!Corona_Frame(World, &Client->ViewXForm, Time))
		GenVS_Error("Client_RenderWorld:  Corona_Frame failed.\n");

	// Run the electric stuff
	if (!Electric_Frame(World, &Client->ViewXForm, Time))
		GenVS_Error("Client_RenderWorld:  Electric_Frame failed.\n");

	// Run the DynLight stuff
	if (!DynLight_Frame(World, &Client->ViewXForm, Time))
		GenVS_Error("Client_RenderWorld:  DynLight_Frame failed.\n");

	// Run the ModelCtl stuff
	if (!ModelCtl_Frame(World, &Client->ViewXForm, Time))
		GenVS_Error("Client_RenderWorld:  ModelCtl_Frame failed.\n");

 	// Finally, render the world
	if (!geEngine_RenderWorld(Engine, World, Camera, Time))
		GenVS_Error("Client_RenderWorld:  geEngine_RenderWorld failed.\n");

	return GE_TRUE;
}

//=====================================================================================
//	SetupCamera
//=====================================================================================
static void SetupCamera(geCamera *Camera, geRect *Rect, geXForm3d *XForm)
{
	geCamera_SetWorldSpaceXForm(Camera, XForm);
	geCamera_SetAttributes(Camera, 2.0f, Rect);
}

//=====================================================================================
//	Client_ChangeNetState
//=====================================================================================
geBoolean Client_ChangeNetState(Client_Client *Client, NetMgr_NetState NewNetState)
{
	char		Data[128];
	Buffer_Data	Buffer;

	assert(Client_IsValid(Client) == GE_TRUE);

	Client->NetState = NewNetState;
								
	// Send the confirm msg to the server
	Buffer_Set(&Buffer, Data, 128);
	Buffer_FillByte(&Buffer, NETMGR_MSG_CLIENT_CONFIRM);
	Buffer_FillSLong(&Buffer, NewNetState);

	if (!NetMgr_SendServerMessage(Client->NMgr, &Buffer, GE_TRUE))
		GenVS_Error("Client_ChangeNetState:  NetMgr_SendServerMessage failed.\n");

	return GE_TRUE;
}

//===========================================================================
//	Client_SetDemo
//===========================================================================
geBoolean Client_SetDemo(Client_Client *Client, int32 DemoNum)
{
	assert(Client_IsValid(Client) == GE_TRUE);
	assert(DemoNum < Client->Demo.NumDemos);
	assert(Client->Demo.Mode == CLIENT_DEMO_PLAY);

	// Close down any old demo data files
	if (Client->Demo.File)
		fclose(Client->Demo.File);

	// Store current demo num
	Client->Demo.CurrentDemo = DemoNum;

	Client->Demo.File = fopen(Client->Demo.DemoNames[Client->Demo.CurrentDemo], "rb");
		
	if (!Client->Demo.File)
		GenVS_Error("Client_SetDemo:  Failed to open demo file: %s.\n", Client->Demo.DemoNames[Client->Demo.CurrentDemo]);

	return GE_TRUE;
}

char	DemoBuffer[NETMGR_LOCAL_MSG_BUFFER_SIZE];
//=====================================================================================
//	Client_ReadServerMessages
//	Keeps on returning true till the clients time passes the hosts current time.
//	This keeps frame rates the same on all machines...
//=====================================================================================
geBoolean Client_ReadServerMessages(Client_Client *Client, Buffer_Data *Buffer)
{
	geCSNetMgr_NetMsgType	MsgType;

	assert(Client_IsValid(Client) == GE_TRUE);

	// Reset the buffer pos
	Buffer->Pos = 0;

	if (Client->Demo.Mode == CLIENT_DEMO_PLAY)		// If demo play mode, get message from file
	{
		geBoolean	Good = GE_FALSE;

		Buffer->Data =(uint8*) DemoBuffer;

		assert(Client->Demo.File);
		
		// Make the demo run at it's original recorded speed...
		if (Client->Demo.OriginalSpeed)
			if (Client->Time <= Client->NetTime)
				return GE_FALSE;	// Wait till client catches up with the demo
		
		if (fread(&Buffer->Size, sizeof(int32), 1, Client->Demo.File) == 1)
		{
			if (Buffer->Size >= NETMGR_LOCAL_MSG_BUFFER_SIZE)
				GenVS_Error("Client_ReadServerMessages:  Demo Buffer size can't hold data.\n");

			if (fread(Buffer->Data, sizeof(char), Buffer->Size, Client->Demo.File) == (uint32)Buffer->Size)
				Good = GE_TRUE;
		}

		if (!Good)		// End of file, try the beginning...
		{
			Client->Time = 0.0f;
			Client->NetTime = 0.0f;
			
			// Reset the scoreboard between demo changes...
			memset(Client->ClientInfo, 0, sizeof(Client_ClientInfo)*NETMGR_MAX_CLIENTS);

			if (!Client_SetDemo(Client, (Client->Demo.CurrentDemo+1)%Client->Demo.NumDemos))
			{
				Client->Demo.Mode = CLIENT_DEMO_NONE;
				return GE_TRUE;
			}
			
			//GenVS_Error("End of demo.\n");
			fseek(Client->Demo.File, 0, SEEK_SET);

			if (fread(&Buffer->Size, sizeof(int32), 1, Client->Demo.File) != 1)
				GenVS_Error("Error reading the demo file.\n");

			if (fread(Buffer->Data, sizeof(char), Buffer->Size, Client->Demo.File) != (uint32)Buffer->Size)
				GenVS_Error("Error reading the demo file.\n");
		}

		return GE_TRUE;
	}

	while (1)
	{
		if (!NetMgr_ReceiveServerMessage(Client->NMgr, &MsgType, Buffer))
			GenVS_Error("Client_ReadServerMessages:  NetMgr_ReceiveServerMessage failed.\n");

		if (MsgType == NET_MSG_SESSIONLOST)
			GenVS_Error("Client_ReadServerMessages:  Session was lost.\n");

		if (MsgType == NET_MSG_NONE)
			return GE_FALSE;

		if (MsgType == NET_MSG_USER)
			break;
	}

	// Check to see if the buffer has data
	if (!Buffer->Size)
		return GE_FALSE;

	// Record the message
	if (Client->Demo.Mode == CLIENT_DEMO_RECORD)	// Record messages for DemoRecord mode
	{
		assert(Client->Demo.File);
		fwrite(&Buffer->Size, 1, sizeof(int32), Client->Demo.File);
		fwrite(Buffer->Data, Buffer->Size, 1, Client->Demo.File);
	}

	return GE_TRUE;
}

char	TempDemoName[CLIENT_MAX_DEMO_NAME_SIZE];

//===========================================================================
//	ParseDemoName
//===========================================================================
static const char *ParseDemoName(FILE *f)
{
	char	*pName;
	char	c;

	// Skip white space
	while (1)
	{
		if (fread(&c, 1, sizeof(char), f) != 1)
			return NULL;

		if (c != ' ' && c != 0x0d)
			break;
	}

	if (c == '\n')
		return NULL;

	pName = TempDemoName;

	*pName++ = c;			// store the first char

	while (1)
	{
		if (fread(&c, 1, sizeof(char), f) != 1)
			return NULL;

		if (c == 0x0d)
			continue;

		//if (c == ' ')
		//	continue;			// Skip white space

		*pName = c;			// store it...

		if ((int32)(pName - TempDemoName) >= CLIENT_MAX_DEMO_NAME_SIZE)
			return NULL;

		if (c == '\n')
		{
			*pName = 0;
			break;
		}

		pName++;

	}

	return TempDemoName;
}

//===========================================================================
//	ReadDemoIni
//===========================================================================
static geBoolean ReadDemoIni(Client_Client *Client)
{
	FILE	*f;

	assert(Client_IsValid(Client) == GE_TRUE);

	// Open, and parse out the demo names from the demo data file...
	f = fopen("Demo.Ini", "r");

	if (!f)
		return GE_FALSE;

	while (1)
	{
		const char	*Name;

		Name = ParseDemoName(f);

		if (!Name)
			break;

		strcpy(Client->Demo.DemoNames[Client->Demo.NumDemos], Name);
		Client->Demo.NumDemos++;

		if (Client->Demo.NumDemos >= CLIENT_MAX_DEMOS)
			break;		// No more demo slots available
	}

	// Close the file
	fclose(f);

	// Make sure there is at least one demo
	if (Client->Demo.NumDemos <= 0)
	{
		Client->Demo.Mode = CLIENT_DEMO_NONE;		// No demos names in demo file...
		return GE_FALSE;
	}

	return GE_TRUE;
}

//===========================================================================
//	Client_SetupDemos
//===========================================================================
geBoolean Client_SetupDemos(Client_Client *Client, int32 Mode, const char *DemoFile)
{
	assert(Client);

	assert(Client_IsValid(Client) == GE_TRUE);

	Client->Demo.Mode = Mode;
	Client->Demo.OriginalSpeed = GE_TRUE;

	Client->Demo.NumDemos = 0;
	Client->Demo.CurrentDemo = 0;

	Client->Demo.File = NULL;
	
	if (Client->Demo.Mode == CLIENT_DEMO_RECORD)
	{
		assert(DemoFile && DemoFile[0]);

		Client->Demo.File = fopen(DemoFile, "wb");

		if (!Client->Demo.File)
			return GE_FALSE;

		return GE_TRUE;
	}
	else if (Client->Demo.Mode == CLIENT_DEMO_PLAY && DemoFile[0])
	{
		Client->Demo.NumDemos = 1;
		Client->Demo.CurrentDemo = 0;
		strcpy(Client->Demo.DemoNames[0], DemoFile);
	}
	else if (Client->Demo.Mode == CLIENT_DEMO_PLAY && !DemoFile[0])
	{
		if (!ReadDemoIni(Client))
			return GE_FALSE;
	}

	if (Client->Demo.Mode == CLIENT_DEMO_PLAY)
	{
		if (!Client_SetDemo(Client, Client->Demo.CurrentDemo))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
//*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*


//=====================================================================================
//=====================================================================================
static float Client_GetTime(void *Client)
{
	assert(Client_IsValid(static_cast<Client_Client*>(Client)) == GE_TRUE);

	return GameMgr_GetTime(static_cast<Client_Client*>(Client)->GMgr);
}

//=====================================================================================
//	Client_GetWorld
//=====================================================================================
geWorld *Client_GetWorld(void *C)
{
	Client_Client *Client = static_cast<Client_Client*>(C);

	assert(Client_IsValid(Client) == GE_TRUE);

	return GameMgr_GetWorld(Client->GMgr);
}

//=====================================================================================
//	Client_GetClientMove
//=====================================================================================
static GenVSI_CMove *Client_GetClientMove(void *C, GenVSI_CHandle ClientHandle)
{
	Client_Client *Client = static_cast<Client_Client*>(C);

	assert(Client_IsValid(Client) == GE_TRUE);

	// Return NULL if it is not us we are controlling
	// For now, the game code should return on a NULL move...
	if (ClientHandle == Client->ClientIndex)
		return &Client->Move;
	else
		return NULL;
}

//=====================================================================================
//	Client_ProcIndex
//=====================================================================================
static void Client_ProcIndex(void *C, uint16 Index, void *Proc)
{
	Client_Client *Client = static_cast<Client_Client*>(C);

	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Index >= 0 && Index < 65535);
	assert(Index >= 0 && Index < CLIENT_MAX_PROC_INDEX);
	
	Client->ProcIndex[Index] = Proc;

	g_ProcIndex[Index] = Proc;
}

//=====================================================================================
//	Server_MovePlayerModel
//=====================================================================================
static geBoolean Client_MovePlayerModel(void *C, void *P, const geXForm3d *DestXForm)
{
	geBoolean	CanMove;
	int32		i;
	geWorld		*World;
	Client_Client *Client = static_cast<Client_Client*>(C);
	GPlayer *Player = static_cast<GPlayer*>(P);

	assert(Client_IsValid(Client) == GE_TRUE);

	World = GameMgr_GetWorld(Client->GMgr);
	assert(World);

	CanMove = GE_TRUE;

	for (i=0; i< NETMGR_MAX_PLAYERS; i++)
	{
		geVec3d		Mins, Maxs, Pos, Out;
		GPlayer		*Target;

		Target = &Client->Players[i];

		if (!Target->Active)			// Not an active player...
			continue;

		if (Target->ViewFlags & VIEW_TYPE_MODEL)	// Don't check other models, don't care...
			continue;

		if (i != Client->ClientPlayer)
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
				Player->Blocked(&Client->GenVSI, Player, Target);
			}

		}
		else
		{
			Target->XForm.Translation = Out;
			Target->Pos = Out;
			Target->LastGoodPos = Out;
		}
	}

	if (CanMove)
	{
		if (World)
			geWorld_SetModelXForm(World, Player->Model, DestXForm);
		else
			Console_Printf(GameMgr_GetConsole(Client->GMgr), "Client_MovePlayerModel:  No world!\n");

		Player->Pos = DestXForm->Translation;
		Player->XForm.Translation = DestXForm->Translation;
	}

	return CanMove;
}


//=====================================================================================
//	Client_SpawnTempPlayer
//=====================================================================================
static void *Client_SpawnTempPlayer(void *C, const char *ClassName)
{
	int32		i;
	Client_Client *Client = static_cast<Client_Client*>(C);

	assert(Client_IsValid(Client) == GE_TRUE);
	assert(strlen(ClassName) < MAX_CLASS_NAME_SIZE);

	for (i=0; i< CLIENT_MAX_TEMP_PLAYERS; i++)
	{
		if (!Client->TempPlayers[i].Active)
			break;
	}

	if (i >= CLIENT_MAX_TEMP_PLAYERS)
	{
		GenVS_Error("Client_SpawnTempPlayer: Failed to add player!!!\n");
		return NULL;
	}

	memset(&Client->TempPlayers[i], 0, sizeof(GPlayer));

	SetPlayerDefaults(&Client->TempPlayers[i]);

	Client->TempPlayers[i].Active = GE_TRUE;
	Client->TempPlayers[i].SpawnTime = Client->Move.MoveTime;

	return &Client->TempPlayers[i];
}

//=====================================================================================
//	Client_DestroyPlayerWorldObjects
//=====================================================================================
void Client_DestroyPlayerWorldObjects(Client_Client *Client, GPlayer *Player)
{
	geBoolean	Ret;
	geWorld		*World;

	World = GameMgr_GetWorld(Client->GMgr);

	if (Player->Actor)
	{
		assert(World);
		Ret = geWorld_RemoveActor(World, Player->Actor);
		assert(Ret == GE_TRUE);
		geActor_Destroy(&Player->Actor);
	}

	if (Player->Light)
	{
		assert(World);
		geWorld_RemoveLight(World, Player->Light);
	}
			
	if (Player->Poly)
	{
		assert(World);
		geWorld_RemovePoly(World, Player->Poly);
	}
}

//=====================================================================================
//	Client_DestroyTempPlayer
//=====================================================================================
void Client_DestroyTempPlayer(void *C, void *P)
{
	Client_Client *Client = static_cast<Client_Client*>(C);
	GPlayer *Player = static_cast<GPlayer*>(P);

	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Player);

	assert(Player->Active);

	//if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), TEMP_PLAYER_TO_FXPLAYER(Client, Player), 0))
	//	GenVS_Error("[CLIENT] UpdatePlayers:  Could not set Fx_Player flags.\n");

	Client_DestroyPlayerWorldObjects(Client, Player);

	memset(Player, 0, sizeof(GPlayer));
}

//=====================================================================================
//	Client_DestroyTempPlayer
//=====================================================================================
void Client_DestroyTempPlayer(Client_Client *Client, GPlayer *Player)
{
	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Player);

	assert(Player->Active);

	//if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), TEMP_PLAYER_TO_FXPLAYER(Client, Player), 0))
	//	GenVS_Error("[CLIENT] UpdatePlayers:  Could not set Fx_Player flags.\n");

	Client_DestroyPlayerWorldObjects(Client, Player);

	memset(Player, 0, sizeof(GPlayer));
}


//=====================================================================================
//	Client_GetPlayer
//=====================================================================================
GPlayer *Client_GetPlayer(Client_Client *Client, int32 Index)
{
	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Index >= 0);
	assert(Index < NETMGR_MAX_PLAYERS);

	return &Client->Players[Index];
}

//=====================================================================================
//	Client_DestroyPlayer
//=====================================================================================
void Client_DestroyPlayer(Client_Client *Client, GPlayer *Player)
{
	assert(Client_IsValid(Client) == GE_TRUE);
	assert(Player);

	assert(Player->Active);

	if (!Fx_PlayerSetFxFlags(GameMgr_GetFxSystem(Client->GMgr), PLAYER_TO_FXPLAYER(Client, Player), 0))
		GenVS_Error("[CLIENT] UpdatePlayers:  Could not set Fx_Player flags.\n");

	Client_DestroyPlayerWorldObjects(Client, Player);

	memset(Player, 0, sizeof(GPlayer));
}


//=====================================================================================
//	Client_SetupGenVSI
//=====================================================================================
static void Client_SetupGenVSI(Client_Client *Client)
{
	GenVSI	*VSI;

	assert(Client_IsValid(Client) == GE_TRUE);

	VSI = &Client->GenVSI;

	memset(VSI, 0, sizeof(GenVSI));

	VSI->Mode = MODE_SmartClient;

	VSI->UData = Client;

	VSI->GetTime = Client_GetTime;

	VSI->MovePlayerModel = Client_MovePlayerModel;
	
	VSI->GetWorld = Client_GetWorld;
	VSI->GetClientMove = Client_GetClientMove;

	VSI->ProcIndex = Client_ProcIndex;

	VSI->SpawnPlayer = Client_SpawnTempPlayer;
	VSI->DestroyPlayer = Client_DestroyTempPlayer;
}
