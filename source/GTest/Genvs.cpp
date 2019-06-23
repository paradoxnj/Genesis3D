/****************************************************************************************/
/*  GenVS.c                                                                             */
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
#include <Stdio.h>
#include <Dos.h>
#include <Math.h>
#include <Windows.H>
#include <MMSystem.H>
#include <StdLib.h>
#include <Assert.h>
#include <Time.h>
#include <direct.h>		//_chdir()

#include "Genesis.h"
#include "bitmap.h"
#include "Errorlog.h"

#include "GameMgr.h"
#include "NetMgr.h"

#include "Host.h"
#include "Client.h"
#include "Console.h"

#include "DrvList.h"

#include "Menu.h"
#include "GMenu.h"
#include "Text.h"
#include "AutoSelect.h"

#define CLIP_CURSOR
//#define DO_CAPTURE

void		GenVS_Error(const char *Msg, ...);

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

static char *GetCommandText(char *CmdLine, char *Data, geBoolean Cmd);

void ShutdownAll(void);

static geBoolean NewKeyDown(int KeyCode);
static void GetMouseInput(HWND hWnd, int Width, int Height);

#define STARTING_WIDTH  (500)
#define STARTING_HEIGHT (400)

// Misc global info
geBoolean				PopupD3DLog     = GE_FALSE;
BOOL					Running			= TRUE;
BOOL					GameRunning		= FALSE;
BOOL					ResetMouse		= TRUE;
char					AppName[256]	= "Genesis3D Sample Game";

#define DEFAULT_LEVEL	"Levels\\GenVS.BSP"

geFloat					EffectScale;
geBoolean				ChangingDisplayMode = GE_FALSE;	
int						ChangeDisplaySelection;

// Game Manager
static GameMgr			*GMgr = NULL;

//	Genesis Objects
geEngine				*Engine = NULL;

// Misc objects
static Console_Console	*Console = NULL;
static Host_Host		*Host = NULL;

geSound_System	*SoundSys = NULL;

static char				PlayerName[MENU_MAXSTRINGSIZE];
static char				IPAddress[MENU_MAXSTRINGSIZE];

float					MainTime;

static geBoolean		MenusCreated;
ModeList			   *DriverModeList;
int						DriverModeListLength;

geBoolean				g_FogEnable = GE_FALSE;
geBoolean				g_FarClipPlaneEnable = GE_FALSE;

//=====================================================================================
//	GetCommandText
//=====================================================================================
static char *GetCommandText(char *CmdLine, char *Data, geBoolean Cmd)
{
	geBoolean	Quote;
	int32		dp;
	char		ch;

	while (1)
	{
		ch = *CmdLine;
		
		if (ch == 0 || ch == '\n')
			return NULL;

		if (ch == ' ')				// Skip white space
		{
			CmdLine++;
			continue;
		}

		if (ch == '-')
		{
			if (!Cmd)
				return NULL;		// FIXME:  Return error!!!
			CmdLine++;
			break;
		}
		else 
		{
			if (Cmd)
				return NULL;			// FIXME:  Return error!!!
			break;
		}

		CmdLine++;
	}

	if (ch == 0 || ch == '\n')
		return NULL;

	if (ch == '"')
	{
		Quote = GE_TRUE;
		ch = *++CmdLine;
	}
	else
		Quote = GE_FALSE;

	dp = 0;

	while (1)
	{
		if (Quote)
		{
			if (ch == '"')
			{
				break;
			}
			
			if (ch == '\n' || ch == 0)
				return NULL;		// Quote not found!!!
		}
		else
		{
			if (ch == ' ' || ch == '\n' || ch == 0)
				break;
		}
		
		ch = *CmdLine++;
		Data[dp++] = ch;
	}

	Data[dp-1] = 0;

	return CmdLine;
		
}

void AdjustPriority(int adjustment)
{
	//HANDLE thread = GetCurrentThread();
	//SetThreadPriority(thread,adjustment);
}


float	GlobalMouseSpeedX;
float	GlobalMouseSpeedY;
uint32	GlobalMouseFlags;	

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

Host_Init		HostInit;
geBoolean		ShowStats,Mute;

geVFile *			MainFS;

//=====================================================================================
//	NewKeyDown
//=====================================================================================
static geBoolean NewKeyDown(int KeyCode)
{
	if (GetAsyncKeyState(KeyCode) & 1)
			return GE_TRUE;

	return GE_FALSE;
}

static void PickMode(HWND hwnd, HINSTANCE hInstance, geBoolean NoSelection, geBoolean ManualSelection, 
		ModeList *List, int ModeListLength, int *Selection);


//=====================================================================================
//	WinMain
//=====================================================================================
#pragma warning (disable: 4028)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpszCmdParam, int nCmdShow)
{
#pragma warning (default: 4028)
	geDriver		*Driver     = NULL;
	geDriver_Mode	*DriverMode = NULL;

	SYSTEMTIME		SystemTime;
	MSG				Msg;
	char			*CmdLine = lpszCmdParam;
	int32			i;
	LARGE_INTEGER	Freq, OldTick, CurTick;
	char			TempName[256];
	int32			TempNameLength;
	geBoolean		ManualPick=GE_FALSE;

	MainTime = 0.0f;

	AdjustPriority(THREAD_PRIORITY_NORMAL);

	for (i=0; i<255; i++)
	{
		NewKeyDown(i);		// Need to flush all the keys
	}

	GetSystemTime(&SystemTime);

#if 0
	if	(SystemTime.wYear > 1999 || SystemTime.wMonth > 11)
	{
		MessageBox(NULL,"The time limit on this demo has expired. \n" 
			"Please contact WildTangent for more information.",
			"Notice",MB_OK | MB_TASKMODAL);
		return 0;
	}
#endif


// set the currrent directory to where the exe is
	{
		int i;
		char	PathBuf[_MAX_PATH];

		// get the exe's path and name
		if (GetModuleFileName(hInstance,PathBuf,_MAX_PATH-1)==0)
		{
			GenVS_Error("Could not get exe file name.");
		}

		// strip off exe name to leave path
		for (i=strlen(PathBuf)-1; i>0; i--)
		{
			if (PathBuf[i]=='\\')
			{
				PathBuf[i]=0;
				break;
			}
		}
		if (i==0)
		{
			GenVS_Error("Could not parse exe's path from exe name.");
		}
		
		// move the current working directory to the executables directory.
		// this is a little rude
		if (_chdir(PathBuf)==-1)
		{
			GenVS_Error("Could not change current working directory to exe's path.");
		}
	}
	

	// Setup the appname
	sprintf(AppName, "GTest v1.0 %s, %s", __DATE__, __TIME__);

	// Get defaults
	ShowStats = Mute = GE_FALSE;

	strcpy(IPAddress, "");							// Deafult our IP address to blank (LAN game)
	TempNameLength = sizeof(TempName);
	if( GetUserName( TempName, (LPDWORD)&TempNameLength ) == 0 )
	{
		strcpy( PlayerName, "Player" );
	}
	else
	{
		strncpy( PlayerName, TempName, sizeof( PlayerName ) );
		PlayerName[ sizeof( PlayerName ) - 1 ] = '\0';
	}


	HostInit.Mode = HOST_MODE_SINGLE_PLAYER;
	HostInit.DemoMode = HOST_DEMO_NONE;
	HostInit.LevelHack[0] = 0;
	HostInit.DemoFile[0] = 0;

	GetCurrentDirectory(sizeof(TempName), TempName);
	MainFS = geVFile_OpenNewSystem(NULL,
								   GE_VFILE_TYPE_DOS,
								   TempName,
								   NULL,
								   GE_VFILE_OPEN_READONLY | GE_VFILE_OPEN_DIRECTORY);
	assert(MainFS);
	

	while (1)
	{	
		char			Data[MAX_PATH];

		if (!(CmdLine = GetCommandText(CmdLine, Data, GE_TRUE)))
			break;

		if (!_stricmp(Data, "Server"))
		{
			if (!HostInit.LevelHack[0])	
				strcpy(HostInit.LevelHack, DEFAULT_LEVEL);

			HostInit.Mode = HOST_MODE_SERVER_CLIENT;
		}
		else if (!_stricmp(Data, "Dedicated"))
		{
			HostInit.Mode = HOST_MODE_SERVER_DEDICATED;
		}
		else if (!_stricmp(Data, "Client"))
		{
			HostInit.Mode = HOST_MODE_CLIENT;
		}
		else if (!_stricmp(Data, "BotPathDebug"))
		{
			extern geBoolean PathLight; // hacked in
			PathLight = GE_TRUE;
		}
		else if (!_stricmp(Data, "Record"))
		{
			HostInit.DemoMode = HOST_DEMO_RECORD;

			if (!(CmdLine = GetCommandText(CmdLine, Data, GE_FALSE)))
				GenVS_Error("No demo name specified on command line.");

			strcpy(HostInit.DemoFile, Data);
		}
		else if (!_stricmp(Data, "Play"))
		{
			HostInit.DemoMode = HOST_DEMO_PLAY;

			if (!(CmdLine = GetCommandText(CmdLine, Data, GE_FALSE)))
				GenVS_Error("No demo name specified on command line.");

			strcpy(HostInit.DemoFile, Data);
		}
		else if (!_stricmp(Data, "Name"))
		{
			// Get name
			if (!(CmdLine = GetCommandText(CmdLine, Data, GE_FALSE)))
				GenVS_Error("No name specified on command line.");
			
			strcpy(PlayerName, Data);
		}
		else if (!_stricmp(Data, "IP"))
		{
			// Get name
			if (!(CmdLine = GetCommandText(CmdLine, Data, GE_FALSE)))
				GenVS_Error("No IP Address specified on command line.");
			
			if (strlen(Data) >= NETMGR_MAX_IP_ADDRESS)
				GenVS_Error("MaxIP Address string on command line.\n");

			strcpy(IPAddress, Data);		// User wants to override IP at command line
		}
		else if (!_stricmp(Data, "Map"))
		{
			// Get name
			if (!(CmdLine = GetCommandText(CmdLine, Data, GE_FALSE)))
				GenVS_Error("No map name specified on command line.");
			sprintf(HostInit.LevelHack, "Levels\\%s", Data);
			//sprintf(HostInit.UserLevel, "Levels\\%s", Data);
		}
		else if (!_stricmp(Data, "Gamma"))
		{
			// Get name
			if (!(CmdLine = GetCommandText(CmdLine, Data, GE_FALSE)))
				GenVS_Error("No gamma value specified on command line.");
			
			// <>
			UserGamma = (float)atof(Data);
		}
		else if (!_stricmp(Data, "ShowStats"))
		{
			ShowStats = GE_TRUE;
		}
		else if (!_stricmp(Data, "Mute"))
		{
			Mute = GE_TRUE;
		}
		else if (!_stricmp(Data, "VidMode"))
		{
			GenVS_Error("VidMode Parameter no longer supported");
		}
		else if (!_stricmp(Data,"PickMode"))
		{
			ManualPick=GE_TRUE;
		}
		else if (!_stricmp(Data,"Fog"))
		{
			g_FogEnable = GE_TRUE;
		}
		else if (!_stricmp(Data,"FarClip"))
		{
			g_FarClipPlaneEnable = GE_TRUE;
		}
		else
			GenVS_Error("Unknown Option: %s.", Data);
	}


	
	//
	//	Create the Game Mgr
	//
	GMgr = GameMgr_Create(hInstance, STARTING_WIDTH, STARTING_HEIGHT, AppName);
	
	if (!GMgr)
		GenVS_Error("Could not create the game mgr.");

	Engine = GameMgr_GetEngine(GMgr);
	if (!Engine)
		GenVS_Error("Failed to create the geEngine Object");

	DriverModeList = ModeList_Create(Engine,&DriverModeListLength);
	if (DriverModeList == NULL)
		{
			GenVS_Error("Failed to create a list of available drivers - Make sure the driver dll's are in the right directory.");
		}

	AutoSelect_SortDriverList(DriverModeList, DriverModeListLength);

	geEngine_EnableFrameRateCounter(Engine, ShowStats);

	
	do 
		{
			HWND hWnd;
			VidMode VidMode;

			// Pick mode
			PickMode(GameMgr_GethWnd(GMgr),hInstance,ChangingDisplayMode, ManualPick, 
					 DriverModeList, DriverModeListLength, &ChangeDisplaySelection);
			ChangingDisplayMode = 0;		

			// Text, and Menu should go in GameMgr to make the hierarchy clean
			// Client, and Host could then look down on Menu, and grab default items out of it...
			if (!Text_Create( Engine ))
				GenVS_Error("Text_Create failed.");

			if (!GMenu_Create( Engine , DriverModeList, DriverModeListLength, ChangeDisplaySelection))
				GenVS_Error("GMenu_Create failed.");
			
			MenusCreated = GE_TRUE;

			// Set the text in the menus for the name/ipaddress etc...
			{
				Menu_SetStringText( GMenu_GetMenu(GMenu_NameEntry), GMenu_NameEntry, PlayerName );
				Menu_SetStringText( GMenu_GetMenu(GMenu_IPEntry), GMenu_IPEntry, IPAddress );
			}

			// Get the sound system
			SoundSys = GameMgr_GetSoundSystem(GMgr);
			//assert(SoundSys);

			if (SoundSys)
				{
					if ( Mute )
						geSound_SetMasterVolume(SoundSys, 0.0f );
				}

			// Get the console
			Console = GameMgr_GetConsole(GMgr);
			assert(Console);
			
			HostInit.hWnd = GameMgr_GethWnd(GMgr);
			strcpy(HostInit.ClientName, PlayerName);

			if (!GameMgr_ClearBackground(GMgr, 0, 0, NULL))
				GenVS_Error("GameMgr_ClearBackground failed.");

			// If a map has not been set, and not recording a demo, then play demos...
			if (HostInit.DemoMode != HOST_DEMO_RECORD && !HostInit.LevelHack[0] && HostInit.Mode != HOST_MODE_CLIENT)
				HostInit.DemoMode = HOST_DEMO_PLAY;
			else
				GMenu_SetActive(GE_FALSE);			// Menu should start out as non active when recording a demo...

			// If the user wants to load a level at the command prompt, or wants to play back a demo
			// then load a host now, else just let them do it through the menus later...
			if (HostInit.LevelHack[0] || HostInit.DemoMode != HOST_DEMO_NONE || HostInit.Mode == HOST_MODE_CLIENT)
			{
				if (!HostInit.LevelHack[0])	
					strcpy(HostInit.LevelHack, DEFAULT_LEVEL);

				Host = Host_Create(Engine, &HostInit, GMgr, GameMgr_GetVidMode(GMgr));

				if (!Host)
					GenVS_Error("Could not create the host!\n");
			}
			else		// Make sure a default level is set
				strcpy(HostInit.LevelHack, DEFAULT_LEVEL);

		#if 1
			// <>
			geEngine_SetGamma(Engine, UserGamma);
		#endif

			// Setup the fog (if enabled)
			if (g_FogEnable)
				geEngine_SetFogEnable(Engine, GE_TRUE, 200.0f, 10.0f, 10.0f, 200.0f, 1300.0f);
			
			QueryPerformanceFrequency(&Freq);
			QueryPerformanceCounter(&OldTick);

			//SetCapture(GameMgr_GethWnd(GMgr));
			ShowCursor(FALSE);
			
			#ifdef CLIP_CURSOR	
			{
				RECT	ClipRect;
				RECT	ClientRect;
				POINT	RPoint;

				GetClientRect(GameMgr_GethWnd(GMgr), &ClientRect);
				RPoint.x		=ClientRect.left;
				RPoint.y		=ClientRect.top;
				ClientToScreen(GameMgr_GethWnd(GMgr), &RPoint);
				ClipRect.left	=RPoint.x;
				ClipRect.top	=RPoint.y;

				RPoint.x		=ClientRect.right;
				RPoint.y		=ClientRect.bottom;
				ClientToScreen(GameMgr_GethWnd(GMgr), &RPoint);
				ClipRect.right	=RPoint.x;
				ClipRect.bottom	=RPoint.y;
				ClipCursor(&ClipRect);
			}
			#endif
			
			Running = TRUE;
			VidMode = GameMgr_GetVidMode(GMgr);
			hWnd    = GameMgr_GethWnd(GMgr);


			while (Running)
			{
				LARGE_INTEGER	DeltaTick;
				float			ElapsedTime;
				geWorld			*World;
				geCamera		*Camera;

				GameRunning	=TRUE;

				// see runtime menu options for stats and video mode changing.  (Mike)

				SetCursor(NULL);
				#ifdef DO_CAPTURE
				SetCapture(hWnd);
				#endif

				// Get timing info
				QueryPerformanceCounter(&CurTick);

				SubLarge(&OldTick, &CurTick, &DeltaTick);

				OldTick = CurTick;

				if (DeltaTick.LowPart > 0)
					ElapsedTime =  1.0f / (((float)Freq.LowPart / (float)DeltaTick.LowPart));
				else 
					ElapsedTime = 0.001f;

				//MainTime += ElapsedTime;

				// Get the mouse input (FIXME:  Move this into client?)
				{
					int Width, Height;
					VidMode_GetResolution(VidMode,&Width,&Height);
					GetMouseInput(hWnd,Width,Height);
				}

				// Do a host frame (physics, etc)
				if (Host)
				{
					if (!Host_Frame(Host, ElapsedTime))
						GenVS_Error("Host_Frame failed...");
				}

				// Get the world, and the camera from the GameMgr
				World = GameMgr_GetWorld(GMgr);
				Camera = GameMgr_GetCamera(GMgr);;

				if (!GameMgr_Frame(GMgr, ElapsedTime))
					GenVS_Error("GameMgr_Frame failed...");

				// Begin frame
				if (g_FarClipPlaneEnable)		// If we are using a far clip plane, then clear the screen
				{
					if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
						GenVS_Error("GameMgr_BeginFrame failed.\n");
				}
				else if (!GameMgr_BeginFrame(GMgr, World, GE_FALSE))
				{
					if (!GameMgr_BeginFrame(GMgr, World, GE_TRUE))
						GenVS_Error("GameMgr_BeginFrame failed.\n");
				}

				if (!Host || !World)
					GameMgr_ClearBackground(GMgr, 0, 0, NULL);

				if (Host)
				{
					if (!Host_RenderFrame(Host, ElapsedTime))
						GenVS_Error("Host_RenderFrame failed in main game loop.\n");
				}

				Console_Frame(Console, ElapsedTime);

				/*
				if (Host)
				{
					if (!World)
						GameMgr_ClearBackground(GMgr, 0, 0, NULL);

					if (Host_RenderFrame(Host, ElapsedTime)==GE_FALSE)
						GenVS_Error("Host_RenderFrame failed in main game loop.\n");

					Console_Frame(Console, ElapsedTime);
				}
				else
					GameMgr_ClearBackground(GMgr, 0, 0, NULL);
				*/

				if (!ShowStats && World)
				{
					Console_XYPrintf(Console,0,0,0,"Driver: %s %s",
						DriverModeList[ChangeDisplaySelection].DriverNamePtr,
						DriverModeList[ChangeDisplaySelection].ModeNamePtr);
					
					if (!SoundSys)
					{
						Console_XYPrintf(Console,0,1,0,"(No sound device found)");
					}
				}
				
				// Draw manu
				GMenu_Draw();

				//	End Engine frame
				if (!GameMgr_EndFrame(GMgr))
				{
					GenVS_Error("GameMgr_EndFrame failed.\n");
				}

				// Do the'ol message pump
				
				while (PeekMessage( &Msg, NULL, 0, 0, PM_NOREMOVE))
				{
					if (!GetMessage(&Msg, NULL, 0, 0 ))
					{
						PostQuitMessage(0);
						Running=0;
						break;
					}

					TranslateMessage(&Msg); 
					DispatchMessage(&Msg);
					if (ChangingDisplayMode)
						{
							break;
						}
				}
				
				

				if (ChangingDisplayMode)
					{
						Running = 0;		
						GameRunning	= FALSE;
					}
			} 
			
				if (ChangingDisplayMode)
					{
						if (MenusCreated)
							{
								Text_Destroy();
								GMenu_Destroy();
								MenusCreated = GE_FALSE;
							}

						if (Host)
							{
								Host_Destroy(Host);
								Host = NULL;
							}
						
						if ( GetCapture() )
							{
								ReleaseCapture();
							}
						ShowCursor(TRUE);
					}

			
		}
	while (ChangingDisplayMode != GE_FALSE);

	ShutdownAll();
	return (0);
}


static void PickMode(HWND hwnd, HINSTANCE hInstance, geBoolean NoSelection, geBoolean ManualSelection, 
		ModeList *List, int ListLength, int *ListSelection)
{

	assert( hwnd != 0 );
	assert( hInstance != 0 );
	assert( List != NULL );
	assert( ListSelection != NULL );

	GameMgr_PrepareToChangeMode(GMgr);

	if (!NoSelection && !ManualSelection)
			{
				if (AutoSelect_PickDriver(GameMgr_GethWnd(GMgr),Engine,List, ListLength, ListSelection)==GE_FALSE)
					{
						geErrorLog_AddString(-1,"Automatic video mode selection failed to find good mode.  Trying manual selection.",NULL);
						ManualSelection = GE_TRUE;
					}
			}

	
	if (NoSelection || ManualSelection)
		{
			while (1)
				{
					if (!NoSelection)
						{
							if (DrvList_PickDriver(hInstance, GameMgr_GethWnd(GMgr), List, ListLength, ListSelection)==GE_FALSE)
								{
									geErrorLog_AddString(-1,"Driver pick dialog failed",NULL);
									ShutdownAll();
									exit(1);
								}
						}
					NoSelection = GE_FALSE;

					if ( *ListSelection < 0 )	// no selection made
						{
							ShutdownAll();
							exit(1);
						}

					geEngine_ShutdownDriver(Engine);

					if(List[*ListSelection].InAWindow)
						{
							GameMgr_ResetMainWindow(hwnd,List[*ListSelection].Width,List[*ListSelection].Height);
						}
					if ( (List[*ListSelection].DriverType == MODELIST_TYPE_D3D_PRIMARY)   ||
						 (List[*ListSelection].DriverType == MODELIST_TYPE_D3D_SECONDARY) ||
						 (List[*ListSelection].DriverType == MODELIST_TYPE_D3D_3DFX) )
						{
							PopupD3DLog = GE_TRUE;
						}
					else
						{
							PopupD3DLog = GE_FALSE;
						}
					if (!geEngine_SetDriverAndMode(Engine, List[*ListSelection].Driver, List[*ListSelection].Mode))
						{
							if ( GetCapture() )  // just in case
								{
									ReleaseCapture();
								}
							GameMgr_ResetMainWindow(hwnd,STARTING_WIDTH,STARTING_HEIGHT);
							geErrorLog_AddString(-1, "geEngine_SetDriverAndMode failed. (continuing)", NULL);
							MessageBox(GameMgr_GethWnd(GMgr), "Driver failed to properly set the selected mode.","Error:",MB_OK);
						}
					else
						{ 
							break;
						}
				}
		}



	// Set the driver and the mode through the game manager...
	if (!GameMgr_SetDriverAndMode(GMgr, List[*ListSelection].Driver, List[*ListSelection].Mode, 
										List[*ListSelection].Width,  List[*ListSelection].Height))
		GenVS_Error("GameMgr_SetDriverAndMode failed.");
	

	EffectScale	= 0.3f;	// this is always 0.3 ?

}

//=====================================================================================
//	ShutdownAll
//=====================================================================================
void ShutdownAll(void)
{
	if ( GetCapture() )
	{
		ReleaseCapture();
	}
	ShowCursor(TRUE);

	// Free each object (sub objects are freed by their parents...)
	if (MenusCreated)
	{
		Text_Destroy();
		GMenu_Destroy();
		MenusCreated = GE_FALSE;
	}

	if (Host)
	{
		Host_Destroy(Host);
		Host = NULL;
	}

	if (GMgr)
		{
			HWND hWnd = GameMgr_GethWnd(GMgr);
			if (hWnd)
				{
					ShowWindow(hWnd, SW_HIDE);
					UpdateWindow(hWnd);
				}
		}

	if (GMgr)
		GameMgr_Destroy(GMgr);
	
	geVFile_Close(MainFS);

	GMgr			= NULL;
	Engine			= NULL;
	Console			= NULL;
	SoundSys		= NULL;
	Host			= NULL;

	#ifdef _DEBUG
	{
		char	Str[1024];
		uint32	Count;

		Count = geBitmap_Debug_GetCount();

		sprintf(Str, "Final Bitmap count: %i\n", Count);
		OutputDebugString(Str);

		MessageBox(NULL, Str,
			"GenVS MSG",MB_OK | MB_TASKMODAL);

	}
	#endif

	#ifdef CLIP_CURSOR	
		ClipCursor(NULL);
	#endif

}

extern uint32	GlobalButtonBits;

//=====================================================================================
//	WndProc
//=====================================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch(iMessage)
	{
		case WM_SYSCOMMAND:
			if (wParam == SC_SCREENSAVE)
				return 1;
			break;
		case WM_DISPLAYCHANGE:
			{
				if (DriverModeList)
					{
						if (DriverModeList[ChangeDisplaySelection].InAWindow)
							ChangingDisplayMode=GE_TRUE;
					}
				break;
			}
		case WM_ACTIVATEAPP:
		{
			if(Engine)
			{
				geEngine_Activate(Engine, wParam);
			}

			if(GameRunning)
			{

			#ifdef CLIP_CURSOR	
				if(wParam && GMgr)
				{
					RECT	ClipRect;
					RECT	ClientRect;
					POINT	RPoint;
						  
					GetClientRect(GameMgr_GethWnd(GMgr), &ClientRect);
					RPoint.x		=ClientRect.left;
					RPoint.y		=ClientRect.top;
					ClientToScreen(GameMgr_GethWnd(GMgr), &RPoint);
					ClipRect.left	=RPoint.x;
					ClipRect.top	=RPoint.y;

					RPoint.x		=ClientRect.right;
					RPoint.y		=ClientRect.bottom;
					ClientToScreen(GameMgr_GethWnd(GMgr), &RPoint);
					ClipRect.right	=RPoint.x;
					ClipRect.bottom	=RPoint.y;
					ClipCursor(&ClipRect);
					ResetMouse	=TRUE;
				}
				else
				{
					ResetMouse	=FALSE;
					ClipCursor(NULL);
				}
			#endif
			}
			if ( wParam )
				{
					AdjustPriority(THREAD_PRIORITY_HIGHEST);
				}
			else
				{
					AdjustPriority(THREAD_PRIORITY_NORMAL);
				}
			return	0;
		}
		case WM_MOVE:
		{
			if(GameRunning)
			{
				geRect		ClipRect;
				RECT		ClientRect;
				POINT		RPoint;
				geBoolean	Ret;

				GetClientRect(GameMgr_GethWnd(GMgr), &ClientRect);
				RPoint.x		=ClientRect.left;
				RPoint.y		=ClientRect.top;
				ClientToScreen(GameMgr_GethWnd(GMgr), &RPoint);
				ClipRect.Left	=RPoint.x;
				ClipRect.Top	=RPoint.y;

				RPoint.x		=ClientRect.right;
				RPoint.y		=ClientRect.bottom;
				ClientToScreen(GameMgr_GethWnd(GMgr), &RPoint);
				ClipRect.Right	=RPoint.x;
				ClipRect.Bottom	=RPoint.y;
				Ret = geEngine_UpdateWindow(Engine);

				assert(Ret == GE_TRUE);
			}
			return	0;
		}
		case WM_MOUSEMOVE:
		{
			return FALSE;
			break;
		}

		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_LBUTTONDOWN:
		{
			switch ( iMessage )
			{
				case WM_LBUTTONDOWN:
					GlobalMouseFlags |= 1;
					lParam = VK_LBUTTON;
					break;
				case WM_RBUTTONDOWN:
					GlobalMouseFlags |= 2;
					lParam = VK_RBUTTON;
					break;
				case WM_LBUTTONUP:
					GlobalMouseFlags &= ~1;
					lParam = VK_LBUTTON;
					break;
				case WM_RBUTTONUP:
					GlobalMouseFlags &= ~2;
					lParam = VK_RBUTTON;
					break;
			}

			// intentionally falls through if required
			if ( GMenu_IsAMenuActive() == GE_FALSE )
			{
				break;
			}
		}

		case (WM_KEYDOWN):
		{
			// locals
			int32			Result;

			// process keystroke result //undone
			Result = GMenu_Key( wParam, lParam );

			switch ( Result )
			{
				
				case GMenu_DoNothing:
				{
					break;
				}

				case GMenu_UserSinglePlayerGame:
				case GMenu_SinglePlayerGame:
				case GMenu_SinglePlayerGame1:
				case GMenu_SinglePlayerGame2:
				case GMenu_SinglePlayerGame3:
				case GMenu_StartGame:
				case GMenu_StartGame1:
				case GMenu_StartGame2:
				case GMenu_Connect:
				{
					char	TempString[64];

					// Init the host struct
					if (HostInit.DemoMode == HOST_DEMO_PLAY)	// Make sure we are not in demo play mode...
						HostInit.DemoMode = HOST_DEMO_NONE;

					HostInit.hWnd = GameMgr_GethWnd(GMgr);
					
					// Make sure there is a levelname just in case we are not in demo mode or somthing...
					strcpy(HostInit.LevelHack, "Levels\\GenVS.BSP");

					// zap old host
					if ( Host != NULL )
					{ 
						Host_Destroy( Host );
						Host = NULL;
					}
				
					// set host type
					switch ( Result )
					{
						case GMenu_UserSinglePlayerGame:
						{
							strcpy(HostInit.LevelHack, HostInit.UserLevel);
							HostInit.Mode = HOST_MODE_SINGLE_PLAYER;
							break;
						}

						case GMenu_SinglePlayerGame:
						{
							HostInit.Mode = HOST_MODE_SINGLE_PLAYER;
							break;
						}

						case GMenu_SinglePlayerGame1:
						{
							strcpy(HostInit.LevelHack, "Levels\\GenVS.BSP");
							HostInit.Mode = HOST_MODE_SINGLE_PLAYER;
							break;
						}

						case GMenu_SinglePlayerGame2:
						{
							strcpy(HostInit.LevelHack, "Levels\\GenVS2.BSP");
							HostInit.Mode = HOST_MODE_SINGLE_PLAYER;
							break;
						}

						/*case GMenu_SinglePlayerGame3:
						{
							strcpy(HostInit.LevelHack, "Levels\\Gallery.BSP");
							HostInit.Mode = HOST_MODE_SINGLE_PLAYER;
							break;
						}*/

						case GMenu_StartGame:
						{
							HostInit.Mode = HOST_MODE_SERVER_CLIENT;
							break;
						}

						case GMenu_StartGame1:
						{
							strcpy(HostInit.LevelHack, "Levels\\GenVS.BSP");
							HostInit.Mode = HOST_MODE_SERVER_CLIENT;
							break;
						}

						case GMenu_StartGame2:
						{
							strcpy(HostInit.LevelHack, "Levels\\GenVS2.BSP");
							HostInit.Mode = HOST_MODE_SERVER_CLIENT;
							break;
						}

						case GMenu_Connect:
						{
							HostInit.Mode = HOST_MODE_CLIENT;
							break;
						}
					}

					// Get the client name form the menu
					Menu_GetStringText( GMenu_GetMenu(GMenu_NameEntry), GMenu_NameEntry, TempString );
					strcpy(HostInit.ClientName, TempString);

					// Get the Ip address from the menu...
					Menu_GetStringText( GMenu_GetMenu(GMenu_IPEntry), GMenu_IPEntry, TempString );
					strcpy(HostInit.IPAddress, TempString);

					// create new host
					Host = Host_Create( Engine, &HostInit, GMgr, GameMgr_GetVidMode(GMgr));
					assert( Host != NULL );
					break;
				}

				case GMenu_QuitGame:
				{
					Running = 0;
					break;
				}
			}

			switch(wParam)
			{

				case VK_F12:
				{
					int32		i;
					FILE		*f;
					char		Name[256];

					if (Engine)
					{
						for (i=0 ;i<999; i++)		// Only 999 bmps, oh well...
						{
							sprintf(Name, "Bmp%i.Bmp", i);

							f = fopen(Name, "rb");

							if (f)
							{
								fclose(f);
								continue;
							}
							
							geEngine_ScreenShot(Engine, Name);

							if (Console)
								Console_Printf(Console, "Writing Bmp: %s...\n", Name);

							break;
						}
					}
					break;
				}

				case 192:		// '~'

					if (Console)
						Console_ToggleActive(Console);
					break;

				default:
					if (Console)
						Console_KeyDown(Console, (char)wParam, GE_TRUE);
					break;
			}
			break;
		}
		
		case (WM_KEYUP):
		{
			break;
		}

		case WM_DESTROY:
			PostQuitMessage(0);
			return	0;


		default:
			return DefWindowProc(hWnd, iMessage, wParam, lParam);
	}
	return 0;
}


geBoolean IsAMenuActive(void)
{
	// intentionally falls through if required
	return(GMenu_IsAMenuActive());
}


//=====================================================================================
//=====================================================================================
static void GetMouseInput(HWND hWnd, int Width, int Height)
{
	POINT	Point;
	float	dx, dy;
	int32	x, y;
	
	assert( Width > 0 );
	assert( Height > 0 );
	GetCursorPos(&Point);
  
	if (ScreenToClient( hWnd, &Point)==0)
		{
			geErrorLog_AddString(0,"GetMouseInput:  ScreenToClient failed",NULL); 
			return;
		}
	x = Point.x;
	y = Point.y;

	dx = ((float) (((float)Width/2.0f) - Point.x) / 350.0f);
	dy = ((float) (((float)Height/2.0f) - Point.y) / 350.0f);

	Point.x = Width/2;
	Point.y = Height/2;

	if (ClientToScreen( hWnd, &Point)==0)
		{
			geErrorLog_AddString(0,"GetMouseInput:  ClientToScreen failed",NULL); 
			return;
		}
	if(ResetMouse)
	{
		SetCursorPos(Point.x,Point.y);
	}

	GlobalMouseSpeedX = dx;
	GlobalMouseSpeedY = dy;
}

//=====================================================================================
//	GenVS_Error
//=====================================================================================
extern geBoolean	PopupD3DLog;
static geBoolean	ErrorHandled = GE_FALSE;

void GenVS_Error(const char *Msg, ...)
{
	va_list		ArgPtr;
    char		TempStr[1024];
    char		TempStr2[1024];
	FILE		*f;

	if (ErrorHandled)
		return;

	ErrorHandled = GE_TRUE;

	va_start (ArgPtr, Msg);
    vsprintf (TempStr, Msg, ArgPtr);
	va_end (ArgPtr);

	ShutdownAll();

	f = fopen("GTest.Log", "wt");

	if (f)
	{
		int32		i, NumErrors;

		NumErrors = geErrorLog_Count();

		fprintf(f, "Error#:%3i, Code#:%3i, Info: %s\n", NumErrors, 0, TempStr);

		for (i=0; i<NumErrors; i++)
		{
			geErrorLog_ErrorClassType	Error;
			const char						*String;

			if (geErrorLog_Report(NumErrors-i-1, &Error, &String))
			{
				fprintf(f, "Error#:%3i, Code#:%3i, Info:%s\n", NumErrors-i-1, Error, String);
			}
		}

		fclose(f);
		
		sprintf(TempStr2, "%s\nPlease refer to GTest.Log for more info.", TempStr);

		MessageBox(0, TempStr2, "** Genesis3D Virtual System Error **", MB_OK);
		WinExec( "Notepad GTest.Log", SW_SHOW );
		if (PopupD3DLog)
			{
				WinExec( "Notepad d3ddrv.log", SW_SHOW);
			}
	}
	else
	{
		sprintf(TempStr2, "%s\nCould NOT output GTest.log!!!", TempStr);

		MessageBox(0, TempStr2, "** Genesis3D Virtual System Error **", MB_OK);
	}

	_exit(1);
}

