/****************************************************************************************/
/*  ACTVIEW.C                                                                           */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Actor Viewer and Motion Blender.  Main module.						*/
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#include <windows.h>
#pragma warning(disable : 4201 4214 4115)
#include <commctrl.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <shellapi.h>
#pragma warning(default : 4201 4214 4115)

#include <stdio.h>
#include <io.h>
#include <math.h>
#include "GENESIS.H"
#include "rcstring.h"
#include "resource.h"
#include "InstCheck.h"
#include "Filepath.h"
#include "RAM.H"
#include <assert.h>
#include "drvlist.h"
#include "about.h"
#include "units.h"
#include "blender.h"

#pragma warning (disable:4514)		// unreferenced inline function

#pragma todo ("Need to disable buttons when no actor selected")

#define	USER_ALL	0xffffffff

#define ACTOR_WINDOW_WIDTH	480
#define ACTOR_WINDOW_HEIGHT 360

#define RENDER_BORDER 5

#define DEFAULT_POSE_INDEX		-1
#define BLENDED_MOTION_INDEX	-2

// Message string used to create a custom message for the multimedia timer stuff.
static const char ActViewMessageString[] = "ActorViewerMessageString";

#define DESIRED_MMTIMER_FREQUENCY	33
#define DESIRED_MMTIMER_RESOLUTION  (DESIRED_MMTIMER_FREQUENCY/5)
#define LORES_TIMER_NUMBER 1

typedef enum
{
	AWM_NONE,
	AWM_PAN,
	AWM_ROTATE,
	AWM_ZOOM
} ActView_Mode;

typedef enum
{
	PLAYMODE_STOP,
	PLAYMODE_PLAY,
	PLAYMODE_PAUSE
} ActView_PlayMode;

typedef struct
{
	HINSTANCE	Instance;
	HWND		hwnd;
	char		ActorFilename[MAX_PATH];
	char		IniFilename[MAX_PATH];
	char		LastDir[MAX_PATH];
	ActView_Mode	MouseMode;			// Mouse mode (none, pan, rotate, zoom)
	HWND		hwndTT;					// tooltips window
	HWND		hwndBlender;			// motion blender dialog

	// Engine and such
	geEngine	*Engine;
	geWorld		*World;
	geCamera	*Camera;
	GE_Rect		Rect;
	float		XRotCam, YRotCam, Dist, Height;
	BOOL		ShowFrameRate;

	// animation stuff
	DWORD		MotionStartTime;
	float		LastFrameTime;
	ActView_PlayMode PlayMode;
	int			Scale;
	int			Speed;
	int			FrameDelta;
	float		StartExtent, EndExtent, MotionLength;
	geBoolean	Loop;

	// actor stuff
	geActor_Def	*ActorDef;
	geActor		*Actor;
	float		InitialXRot, InitialYRot;
	float		XRotActor, YRotActor;
	geVec3d		ActorPos, ActorDefaultPos;
	geMotion	*Motion;
	int			LastX, LastY;			// for mouse movement...
	int			CurrentMotion;
	geMotion	*BlendedMotion;

	// timer stuff
	UINT		loresTimer;
	MMRESULT	hiresTimer;
	UINT		TimerMessageId;
	BOOL		ProcessingTimerMessage;
} ActView_WindowData;

/*
  There's some serious ugliness here, but I don't know a way around it.
  The ActView_ToolHook and ActView_DlgHandle variables must be global
  in order for the tooltips to work.  I don't have to like it...
*/
static HHOOK ActView_ToolHook = NULL;
static HWND ActView_DlgHandle = NULL;

static ActView_WindowData *ActView_GetWindowData (HWND hwnd)
{
	return (ActView_WindowData *)GetWindowLong (hwnd, GWL_USERDATA);
}

// Turn an integer value into a percentage expressed in floating point.  (Divides by 100)
static float floatPercent (int Val)
{
	float fPercent = ((float)Val)/100.0f;

	return fPercent;
}

// Turn a floating point value into an integer percentage.  (Multiplies by 100).
static int MakePercent (float fVal)
{
	int iPercent;

	iPercent = (int)floor((fVal * 100.0f) + 0.5f);
	return iPercent;
}

// returns GE_TRUE if the requested key is currently being pressed
static geBoolean IsKeyDown(int KeyCode)
{
	return (GetAsyncKeyState (KeyCode) & 0x8000) ? GE_TRUE : GE_FALSE;
}

/*
  Formats an error message and displays a message box with that error.
  The passed FormatStringID is the resource identifier of the format string.
*/
static void MyError
	(
	  HWND hwnd,
	  HINSTANCE hinst,
	  int FormatStringID,
	  ...
	)
{
	char Buffer[1024];
	const char *FormatString;
	va_list argptr;

	va_start (argptr, FormatStringID);
	FormatString = rcstring_Load (hinst, FormatStringID);
	vsprintf (Buffer, FormatString, argptr);
	va_end (argptr);

	MessageBox (hwnd, Buffer, rcstring_Load (hinst, IDS_PROGRAMNAME), MB_ICONEXCLAMATION | MB_OK);
}

// Set up the dialog's title bar.
static void ActView_SetupTitle
	(
	  ActView_WindowData *pData
	)
{
	char Title[MAX_PATH+100];
	const char *ProgramName = rcstring_Load (pData->Instance, IDS_PROGRAMNAME);

	// If there's an actor loaded, display the file name in the title bar.
	if (pData->ActorFilename[0] == '\0')
	{
		strcpy (Title, ProgramName);
	}
	else
	{
		sprintf (Title, "%s - %s", ProgramName, pData->ActorFilename);
	}
	SetWindowText (pData->hwnd, Title);
}

// Sets the menu option to indicate state of frame rate display.
// Instructs engine to enable/disable frame rate counter.
static void ActView_SetFrameRateToggle
	(
	  ActView_WindowData *pData
	)
{
	HMENU Menu = GetMenu (pData->hwnd);
	if (Menu != NULL)
	{
		CheckMenuItem 
		(
			Menu, 
			ID_OPTIONS_FRAMERATE, 
			MF_BYCOMMAND | (pData->ShowFrameRate ? MF_CHECKED : MF_UNCHECKED)
		);
	}
	// tell engine not to show frame rate
	geEngine_EnableFrameRateCounter (pData->Engine, pData->ShowFrameRate);
}

// Hook procedure for checking on mouse messages passed to the dialog box.
// This is required in order to make the tooltips work.
static LRESULT CALLBACK ActView_HookProc 
	(
	  int nCode, 
	  WPARAM wParam, 
	  LPARAM lParam
	)
{
	MSG *pMsg;
	
	pMsg = (MSG *)lParam;
	// We're only interested in mouse messages for windows that are children of the main dialog.
	if ((nCode >= 0) && (IsChild (ActView_DlgHandle, pMsg->hwnd)))
	{
		switch (pMsg->message)
		{
			case WM_MOUSEMOVE :
			case WM_LBUTTONDOWN :
			case WM_LBUTTONUP :
			case WM_RBUTTONDOWN :
			case WM_RBUTTONUP :
			{
				ActView_WindowData *pData = (ActView_WindowData *)GetWindowLong (ActView_DlgHandle, GWL_USERDATA);

				// if we find such a message, we relay it to the tooltip control,
				// which in turn sends a TTN_NEEDTEXT query to the dialog 
				if (pData->hwndTT != NULL)
				{
					MSG msg;

					msg.lParam = pMsg->lParam;
					msg.wParam = pMsg->wParam;
					msg.message = pMsg->message;
					msg.hwnd = pMsg->hwnd;
					SendMessage (pData->hwndTT, TTM_RELAYEVENT, 0, (LPARAM)&msg);
				}
				break;
			}

			default :
				break;
		}
	}

	return CallNextHookEx (ActView_ToolHook, nCode, wParam, lParam);
}

static void SetSpinnerRange (HWND hwnd, int Spinner, short low, short high)
{
	SendDlgItemMessage (hwnd, Spinner, UDM_SETRANGE, 0, MAKELONG (high, low));
}

// Sets the value of an edit control and updates the associated spinner's value.
static void SetEditControlValue (HWND hwnd, int Edit, int Spinner, int ValuePercent)
{
	int Pos = ValuePercent;

	SetDlgItemInt (hwnd, Edit, Pos, FALSE);
	SendDlgItemMessage (hwnd, Spinner, UDM_SETPOS, 0, MAKELONG (Pos, 0));
}

// Sets up the slider's range and displays the the ending time value
static void SetSliderRange (ActView_WindowData *pData, int SliderId)
{
	int StartRange, EndRange;
	char sTime[100];

	StartRange = MakePercent (pData->StartExtent);
	EndRange = MakePercent (pData->EndExtent);
	SendDlgItemMessage (pData->hwnd, SliderId, TBM_SETRANGEMIN, FALSE, StartRange);
	SendDlgItemMessage (pData->hwnd, SliderId, TBM_SETRANGEMAX, TRUE, EndRange);

	sprintf (sTime, "%.2f", pData->StartExtent);
	SetDlgItemText (pData->hwnd, IDC_STATICSTART, sTime);

	sprintf (sTime, "%.2f", pData->EndExtent);
	SetDlgItemText (pData->hwnd, IDC_STATICEND, sTime);
}

// Sets the slider's current position and displays the current slider time
// in the box above the slider.
static void SetSliderTime (HWND hwnd, int SliderId, float FTime)
{
	char sTime[100];
	int iPercent;

	iPercent = MakePercent (FTime);
	SendDlgItemMessage (hwnd, SliderId, TBM_SETPOS, TRUE, iPercent);

	sprintf (sTime, "%.2f", FTime);
	SetDlgItemText (hwnd, IDC_STATICCURRENTTIME, sTime);
}

// Set the engine's camera attributes from the passed transform and rectangle
static void SetupCamera(geCamera *Camera, GE_Rect *Rect, geXForm3d *XForm)
{
	geCamera_SetWorldSpaceXForm (Camera, XForm);
	geCamera_SetAttributes (Camera, 2.0f, Rect);
}

// Create a transform from the passed rotations and distance
static void SetupXForm (geXForm3d *XForm, float XRot, float YRot, float Dist)
{
	geXForm3d_SetTranslation (XForm, 0.0f, 0.0f, Dist);

	geXForm3d_RotateX(XForm, XRot);
	geXForm3d_RotateY(XForm, YRot);
}

static void ActView_UpdateFrame (ActView_WindowData *pData)
{
	// do frame update
	if (pData == NULL)
	{
		return;
	}

	if ((pData->Engine == NULL) || (pData->World == NULL) || (pData->Camera == NULL))
	{
		return;
	}

	// Always begin frame (last parameter is clear screen flag)
	if (!geEngine_BeginFrame (pData->Engine, pData->Camera, GE_TRUE))
	{
		MyError (pData->hwnd, pData->Instance, IDS_BEGINFRAMEFAIL);
	}

	{
		geXForm3d XForm;

		// Set up the render camera
		SetupXForm (&XForm, pData->XRotCam, pData->YRotCam, pData->Dist);
		geXForm3d_Translate (&XForm, 0.0f, pData->Height, 0.0f);
		SetupCamera (pData->Camera, &pData->Rect, &XForm);

		// Set up transform for actor motion
		SetupXForm (&XForm, pData->XRotActor, pData->YRotActor, 0.0f);

		// adjust position based on default pos, current rotation, and X,Y panning...
		{
			geXForm3d XFormPos;

			geXForm3d_SetTranslation (&XFormPos, pData->ActorDefaultPos.X, pData->ActorDefaultPos.Y, pData->ActorDefaultPos.Z);
			geXForm3d_Translate (&XForm, pData->ActorPos.X, pData->ActorPos.Y, pData->ActorPos.Z);
			geXForm3d_Multiply (&XForm, &XFormPos, &XForm);
		}

		// If we have a current motion, then display the actor at
		// the proper key time.  Otherwise clear the pose.
		if (pData->Actor != NULL)
		{
			if (pData->Motion != NULL)
			{
				geFloat MotionTime = 0.0f;
				switch (pData->PlayMode)
				{
					case PLAYMODE_PLAY :
					{
						// playing, so update current frame time...
						DWORD CurrentTime;
						float ElapsedSeconds;

						CurrentTime = timeGetTime ();
						// Convert elapsed milliseconds from timeGetTime to seconds.
						ElapsedSeconds = ((float)(CurrentTime - pData->MotionStartTime))/1000.0f;
						// And then apply speed factor
						ElapsedSeconds *= ((float)pData->Speed)/100.0f;

						// check for looping...
						if (pData->Loop)
						{
							MotionTime = pData->StartExtent + (geFloat)fmod (ElapsedSeconds, pData->MotionLength);
						}
						else
						{
							// and stop animation if not looping
							if (ElapsedSeconds > pData->MotionLength)
							{
								pData->PlayMode = PLAYMODE_STOP;
								MotionTime = pData->EndExtent;
							}
							else
							{
								MotionTime = pData->StartExtent + ElapsedSeconds;
							}
						}
						break;
					}

					case PLAYMODE_STOP :
					case PLAYMODE_PAUSE :
						MotionTime = pData->LastFrameTime;
						break;
				}

				// Update frame time
				pData->LastFrameTime = MotionTime;
				// Set actor's pose
				geActor_SetPose (pData->Actor, pData->Motion, MotionTime, &XForm);
				// and update the slider
				SetSliderTime (pData->hwnd, IDC_SLIDERTIME, pData->LastFrameTime);
			}
			else
			{
				geActor_ClearPose (pData->Actor, &XForm);
			}
		}
	}

	// All world changes are made...render the results.
 	if (!geEngine_RenderWorld (pData->Engine, pData->World, pData->Camera, 0.0f))
	{
		MyError (pData->hwnd, pData->Instance, IDS_RENDERFAIL);
	}

	// Always end frame
	if (!geEngine_EndFrame (pData->Engine))
	{
		MyError (pData->hwnd, pData->Instance, IDS_ENDFRAMEFAIL);
	}
}

// Load an actor from the passed filename.
// returns TRUE if the actor is loaded successfully.
// This will unload any currently-loaded actor.
BOOL ActView_LoadActor
	(
	  HWND hwnd,
	  const char *ActorFilename
	)
{
	ActView_WindowData *pData = (ActView_WindowData *)GetWindowLong (hwnd, GWL_USERDATA);

	{
		geActor_Def	*ActorDef = NULL;
		geActor		*Actor;

		{
			// Open VFS directory...
			geVFile *ActorDir;
			char ActorPath[MAX_PATH];

			FilePath_GetDriveAndDir (ActorFilename, ActorPath);
			// OpenNewSystem doesn't like the path to have a trailing '\'
			if (ActorPath [strlen (ActorPath)-1] =='\\')
			{
				ActorPath[strlen (ActorPath)-1] = '\0';
			}

			ActorDir = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_DOS, ActorPath, NULL, GE_VFILE_OPEN_DIRECTORY | GE_VFILE_OPEN_READONLY);

			if (ActorDir != NULL)
			{
				// open actor file
				geVFile *ActorFile;
				char ActorName[MAX_PATH];

				FilePath_GetNameAndExt (ActorFilename, ActorName);
				ActorFile = geVFile_Open (ActorDir, ActorName, GE_VFILE_OPEN_READONLY);
				if (ActorFile != NULL)
				{
					// and load the actor from that file
					ActorDef = geActor_DefCreateFromFile (ActorFile);
					// close the VFS file
					geVFile_Close (ActorFile);
				}
			}

			if (ActorDef == NULL)
			{
				return GE_FALSE;
			}
		}

		// Actor definition is loaded.
		// Create it and place it in the world.
		Actor = geActor_Create (ActorDef);
		if (Actor == NULL)
		{
			geActor_DefDestroy (&ActorDef);
			return GE_FALSE;
		}

		// and add the new actor to the world...
		if (geWorld_AddActor (pData->World, Actor, GE_ACTOR_RENDER_ALWAYS | GE_ACTOR_COLLIDE, USER_ALL) == GE_FALSE)
		{
			geActor_Destroy (&Actor);
			geActor_DefDestroy (&ActorDef);
			return GE_FALSE;
		}

		// got the actor.  Remove previous actor from world.
		if (pData->Actor != NULL)
		{
			geWorld_RemoveActor (pData->World, pData->Actor);
			geActor_Destroy (&pData->Actor);
			pData->Actor		= NULL;
		}

		if (pData->ActorDef != NULL)
		{
			geActor_DefDestroy (&pData->ActorDef);
			pData->ActorDef		= NULL;
		}

		pData->ActorDef = ActorDef;
		pData->Actor = Actor;

		pData->Motion = NULL;

		// copy the new filename
		strcpy (pData->ActorFilename, ActorFilename);
	}


	{
		// Set the actor's position and default pose.

		geVec3d BoxPos;
		geXForm3d XForm;
		geExtBox ExtBox;
		
		pData->XRotActor = pData->InitialXRot;
		pData->YRotActor = pData->InitialYRot;
		
		// Get the actor's bounding box,
		// and place the center of the bounding box at the origin...
		geXForm3d_SetIdentity (&XForm);

		// set the actor's default pose
		geActor_ClearPose (pData->Actor, &XForm);
		geActor_GetDynamicExtBox (pData->Actor, &ExtBox);

		geVec3d_Add (&ExtBox.Min, &ExtBox.Max, &BoxPos);
		// save position for centering...
		geVec3d_Scale (&BoxPos, -0.5f, &pData->ActorDefaultPos);
		// current offsets from center are 0
		geVec3d_Clear (&pData->ActorPos);
	}

	{
		// Fill motion combo box with the actor's motions.
		int NumMotions;
		int iMotion;
		HWND hwndCombo;
		int Index;

		hwndCombo = GetDlgItem (hwnd, IDC_MOTIONCOMBO);
		SendMessage (hwndCombo, CB_RESETCONTENT, 0, 0);

		NumMotions = geActor_GetMotionCount (pData->ActorDef);
		for (iMotion = 0; iMotion < NumMotions; ++iMotion)
		{
			const char *MotionName = geActor_GetMotionName (pData->ActorDef, iMotion);
			char TempName[MAX_PATH];

			// The motion API allows unnamed motions.
			// So if MotionName comes back NULL, we have to create a name for this item
			if (MotionName == NULL)
			{
				geMotion *MyMotion;

				MyMotion = geActor_GetMotionByIndex (pData->ActorDef, iMotion);
				sprintf (TempName, rcstring_Load (pData->Instance, IDS_UNNAMED), iMotion);
				MotionName = TempName;
				geMotion_SetName (MyMotion, MotionName);
			}

			Index = SendMessage (hwndCombo, CB_ADDSTRING, 0, (LONG)MotionName);
			SendMessage (hwndCombo, CB_SETITEMDATA, Index, iMotion);
		}

		// Add blended motion to list.
		if (pData->BlendedMotion != NULL)
		{
			Index = SendMessage (hwndCombo, CB_ADDSTRING, 0, (LONG)rcstring_Load (pData->Instance, IDS_BLENDEDMOTION));
			SendMessage (hwndCombo, CB_SETITEMDATA, Index, BLENDED_MOTION_INDEX);
		}

		// Add Default Pose selection
		pData->CurrentMotion = DEFAULT_POSE_INDEX;
		Index = SendMessage (hwndCombo, CB_ADDSTRING, 0, (LONG)rcstring_Load (pData->Instance, IDS_DEFAULTPOSE));
		SendMessage (hwndCombo, CB_SETITEMDATA, Index, DEFAULT_POSE_INDEX);

		// and select it as current item
		SendMessage (hwndCombo, CB_SETCURSEL, Index, 0);
	}

	return TRUE;
}

static void ActView_DoLoadActor 
	(
	  ActView_WindowData *pData,
	  const char *Filename
	)
{
	// show wait cursor while loading...
	HCURSOR OldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

	// We got a filename, so load the actor.
	if (!ActView_LoadActor (pData->hwnd, Filename))
	{
		MyError (pData->hwnd, pData->Instance, IDS_CANTLOADACTOR, Filename);
		return;
	}
	{
		// give the actor some cool(?) lighting effects
		geVec3d FillLightNormal;

		geVec3d_Set (&FillLightNormal, -0.3f, 1.0f, 0.4f);
		geVec3d_Normalize (&FillLightNormal);

		geActor_SetLightingOptions 
		(
			pData->Actor, GE_TRUE, &FillLightNormal,
			128.0f, 128.0f, 128.0f,		// Fill light
			128.0f, 128.0f, 128.0f,		// Ambient light
			GE_FALSE,					// Ambient light from floor
			0,		// no dynamic lights,
			NULL, FALSE
		);
	}

	ActView_SetupTitle (pData);

	// Set last directory...
	FilePath_GetDriveAndDir (Filename, pData->LastDir);

	// Clear blended motion and notify blend dialog (if one)
	// that the actor has changed.
	if (pData->BlendedMotion != NULL)
	{
		geMotion_Destroy (&pData->BlendedMotion);
	}
	pData->BlendedMotion = geMotion_Create (GE_TRUE);
	if (pData->hwndBlender != NULL)
	{
		Blender_UpdateActor (pData->hwndBlender, pData->ActorDef, pData->BlendedMotion);
	}
	SetCursor (OldCursor);
}

/*
  Prompts user for actor filename, and creates a new window
  in which the actor is displayed.
*/
static void PromptForActorWindow (ActView_WindowData *pData)
{
	OPENFILENAME ofn;	// Windows open filename structure...
	char Filename[MAX_PATH];
	char Filter[MAX_PATH];

	Filename[0] = '\0';

	ofn.lStructSize = sizeof (OPENFILENAME);
	ofn.hwndOwner = pData->hwnd;
	ofn.hInstance = pData->Instance;
	{
		char *c;

		// build actor file filter string
		strcpy (Filter, rcstring_Load (ofn.hInstance, IDS_ACTFILEFILTERDESC));
		c = &Filter[strlen (Filter)] + 1;
		// c points one beyond end of string
		strcpy (c, rcstring_Load (ofn.hInstance, IDS_ACTFILEFILTER));
		c = &c[strlen (c)] + 1;
		*c = '\0';	// 2nd terminating nul character
	}
	ofn.lpstrFilter = Filter;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = Filename;
	ofn.nMaxFile = sizeof (Filename);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = pData->LastDir;
	ofn.lpstrTitle = NULL;
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = rcstring_Load (ofn.hInstance, IDS_ACTFILEEXT);
	ofn.lCustData = 0;
	ofn.lpfnHook = NULL;
	ofn.lpTemplateName = NULL;

	if (GetOpenFileName (&ofn))
	{
		ActView_DoLoadActor (pData, Filename);
	}
}

// Load program options.
// Searches the current directory, and then the executable's directory
// for the ini file.
// Saves the filename and loads the options from that file.
static void ActView_LoadOptions
	(
	  ActView_WindowData *pData
	)
{
	char FName[MAX_PATH];

	// Rotation options defaults.
	pData->InitialXRot = 0.0f;
	pData->InitialYRot = 0.0f;

	// search current directory first...
	GetCurrentDirectory (sizeof (FName), FName);
	FilePath_AppendName (FName, rcstring_Load (pData->Instance, IDS_INIFILENAME), FName);
	strcpy (pData->IniFilename, FName);

	if (_access (FName, 0) != 0)
	{
		char ModuleFilename[MAX_PATH];

		GetModuleFileName (NULL, ModuleFilename, sizeof (ModuleFilename));
		FilePath_GetDriveAndDir (ModuleFilename, FName);
		FilePath_AppendName (FName, rcstring_Load (pData->Instance, IDS_INIFILENAME), FName);
		if (_access (FName, 0) != 0)
		{
			// no ini file.  Set defaults and exit.
			return;
		}
		strcpy (pData->IniFilename, FName);
	}
	// pData->IniFilename has full path to .INI file

	// load rotation values
	{
		char sRot[100];

		GetPrivateProfileString ("Options", "XRot", "0", sRot, sizeof (sRot), FName);
		pData->InitialXRot = (float)atof (sRot);

		GetPrivateProfileString ("Options", "YRot", "0", sRot, sizeof (sRot), FName);
		pData->InitialYRot = (float)atof (sRot);

		GetPrivateProfileString ("Options", "LastDir", "", pData->LastDir, sizeof (pData->LastDir), FName);
	}
}

// Saves the current rotations to the ini file as the "Front" rotations.
static void ActView_SaveFront
	(
	  ActView_WindowData *pData
	)
{
	// Set front rotations...
	pData->InitialXRot = pData->XRotActor;
	pData->InitialYRot = pData->YRotActor;

	// IniFilename should have been setup by LoadOptions function...
	if (pData->IniFilename[0] != '\0')
	{
		char sRot[100];

		sprintf (sRot, "%f", pData->InitialXRot);
		WritePrivateProfileString ("Options", "XRot", sRot, pData->IniFilename);
		sprintf (sRot, "%f", pData->InitialYRot);
		WritePrivateProfileString ("Options", "YRot", sRot, pData->IniFilename);
	}
}

static BOOL SetRenderCursor (ActView_WindowData *pData, int Mode)
{
	// If the mouse is in the render window, then set the cursor to reflect
	// the current mouse mode.
	RECT WindowRect;
	POINT pt;

	pData->MouseMode = Mode;

	GetCursorPos (&pt);

	GetWindowRect (GetDlgItem (pData->hwnd, IDC_RENDERWIN), &WindowRect);
	if (PtInRect (&WindowRect, pt))
	{
		int CursorId = -1;

		// determine which cursor to load
		switch (pData->MouseMode)
		{
			case AWM_ZOOM :
				CursorId = IDC_ZOOM;
				break;
			case AWM_PAN :
				CursorId = IDC_PAN;
				break;
			case AWM_ROTATE :
				CursorId = IDC_ROTATE;
				break;
			default :
				break;
		}
		// If mouse mode is set, then load and display the proper cursor.
		if (CursorId != -1)
		{
			SetCursor ((HCURSOR)LoadCursor (pData->Instance, MAKEINTRESOURCE (CursorId)));
			return TRUE;
		}
	}

	return FALSE;
}

// Sets the current frame time to NewFrameTime
// This will clamp the values to the range StartExtent..EndExtent
static void ActView_SetFrameTime (ActView_WindowData *pData, const float fTime)
{
	float NewFrameTime = fTime;

	if (NewFrameTime < pData->StartExtent)
	{
		NewFrameTime = pData->StartExtent;
	}
	if (NewFrameTime > pData->EndExtent)
	{
		NewFrameTime = pData->EndExtent;
	}
	// Set current frame time
	pData->LastFrameTime = NewFrameTime;
	// adjust motion start time so that it's consistent with current time
	pData->MotionStartTime = 
		(DWORD)(timeGetTime() - 
		(1000.0f * (pData->LastFrameTime - pData->StartExtent)/floatPercent (pData->Speed)));
	// and set the slider to reflect the new time
	SetSliderTime (pData->hwnd, IDC_SLIDERTIME, pData->LastFrameTime);
}

// Update the speed value
// Gets the value from the edit control, validates it, and if good sets the value
// in the spinner and in the program data.
static void ActView_UpdateSpeed (ActView_WindowData *pData)
{
	UINT NewValue;
	BOOL IsOk;

	// Get the value
	NewValue = GetDlgItemInt (pData->hwnd, IDC_EDITSPEED, &IsOk, FALSE);
	if (IsOk && (NewValue > 0) && (NewValue <= 10000))
	{
		// Got a new speed value.
		// We need to adjust the speed, but keep the current time the same.
		// So update the speed value and then change the motion start time
		// to reflect the new value.
		pData->Speed = NewValue;
		ActView_SetFrameTime (pData, pData->LastFrameTime);
	}
	// Be sure that the edit control matches stored data values
	SetEditControlValue (pData->hwnd, IDC_EDITSPEED, IDC_SPINSPEED, pData->Speed);
}

// Add TimeDelta (expressed in 100ths of a second) to the current frame time
static void ActView_AdjustFrameTime (ActView_WindowData *pData, int TimeDelta)
{
	float fDelta = floatPercent (TimeDelta);

	ActView_SetFrameTime (pData, pData->LastFrameTime + fDelta);
}

// Update the actor scale value.
// Gets the value from the edit control, validates it, and if good sets the value
// in the spinner and in the program data, and changes the actor's scale.
static void ActView_UpdateScale (ActView_WindowData *pData)
{
	UINT NewValue;
	BOOL IsOk;

	// Get the value
	NewValue = GetDlgItemInt (pData->hwnd, IDC_EDITSCALE, &IsOk, FALSE);
	if (IsOk && (NewValue > 0) && (NewValue <= 10000))
	{
		// if it's good, update everything
		pData->Scale = NewValue;
		// If an actor is loaded, change its size
		if (pData->Actor != NULL)
		{
			float fScale = floatPercent (pData->Scale);
			geActor_SetScale (pData->Actor, fScale, fScale, fScale);
		}
	}
	// in any case, make sure that the edit control matches our value.
	SetEditControlValue (pData->hwnd, IDC_EDITSCALE, IDC_SPINSCALE, pData->Scale);
}

// Updates the FrameDelta value
static void ActView_UpdateFrameDelta (ActView_WindowData *pData)
{
	UINT NewValue;
	BOOL IsOk;

	NewValue = GetDlgItemInt (pData->hwnd, IDC_EDITFRAMETIME, &IsOk, FALSE);
	if (IsOk && (NewValue > 0) && (NewValue <= 100))
	{
		pData->FrameDelta = NewValue;
	}
	// make sure that edit control always reflects current data value
	SetEditControlValue (pData->hwnd, IDC_EDITFRAMETIME, IDC_SPINFRAMETIME, pData->FrameDelta);
}


// Select a new motion.
static void ActView_SelectMotion 
	(
	  ActView_WindowData *pData,
	  int NewMotion
	)
{
	pData->CurrentMotion = NewMotion;
	switch (pData->CurrentMotion)
	{
		case DEFAULT_POSE_INDEX :
			// selected the default pose
			pData->Motion = NULL;
			break;

		case BLENDED_MOTION_INDEX :
			pData->Motion = pData->BlendedMotion;
			break;

		default :
			pData->Motion = geActor_GetMotionByIndex (pData->ActorDef, pData->CurrentMotion);
			break;
	}
	if (pData->Motion != NULL)
	{					
		geMotion_GetTimeExtents (pData->Motion, &pData->StartExtent, &pData->EndExtent);
	}
	else
	{
		pData->StartExtent = 0.0f;
		pData->EndExtent = 0.0f;
	}
	pData->MotionLength = pData->EndExtent - pData->StartExtent;
	SetSliderRange (pData, IDC_SLIDERTIME);
}

#pragma warning (disable:4100)
static LRESULT wm_Command
	(
	  HWND hwnd,
	  ActView_WindowData *pData,
	  WORD wNotifyCode,
	  WORD wID,
	  HWND hwndCtl
	)
{
	switch (wID)
	{
		case ID_FILE_OPEN :
			PromptForActorWindow (pData);
			return 0;

		case ID_FILE_EXIT :
		case IDCANCEL :
			DestroyWindow (hwnd);
			return 0;

		case ID_OPTIONS_FRONT :
			ActView_SaveFront (pData);
			return 0;

		case ID_OPTIONS_FRAMERATE :
			pData->ShowFrameRate = !(pData->ShowFrameRate);
			ActView_SetFrameRateToggle (pData);
			return 0;

		case ID_HELP_CONTENTS :
			WinHelp (hwnd, rcstring_Load (pData->Instance, IDS_HELPFILENAME), HELP_FINDER, 0);
			return 0;

		case ID_HELP_ABOUT :
			About_DoDialog (hwnd, pData->Instance);
			return 0;

		// Pan, Rotate, and Zoom just set the mouse mode.
		// No processing is done with these modes until a click+mouse move in the render window.
		case IDC_PAN :
			SetRenderCursor (pData, AWM_PAN);
			return 0;

		case IDC_ROTATE :
			SetRenderCursor (pData, AWM_ROTATE);
			return 0;

		case IDC_ZOOM :
			SetRenderCursor (pData, AWM_ZOOM);
			return 0;

		// If a new motion is selected, then get the extents and reset the slider
		case IDC_MOTIONCOMBO :
			if (wNotifyCode == CBN_SELENDOK)
			{
				int Index = SendMessage (hwndCtl, CB_GETCURSEL, 0, 0);
				int NewMotion = SendMessage (hwndCtl, CB_GETITEMDATA, Index, 0);
				ActView_SelectMotion (pData, NewMotion);
				ActView_SetFrameTime (pData, pData->StartExtent);
			}
			return 0;

		case IDC_LOOPED :
			pData->Loop = (SendDlgItemMessage (hwnd, IDC_LOOPED, BM_GETCHECK, 0, 0) == 1) ? GE_TRUE : GE_FALSE;
			return 0;

		case IDC_EDITSPEED :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				ActView_UpdateSpeed (pData);
			}
			return 0;

		case IDC_EDITSCALE :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				ActView_UpdateScale (pData);
			}
			return 0;

		case IDC_EDITFRAMETIME :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				ActView_UpdateFrameDelta (pData);
			}
			return 0;

		case IDI_PLAY :
			if (pData->Motion != NULL)
			{
				switch (pData->PlayMode)
				{
					case PLAYMODE_STOP :
						// if stopped, then restart at the beginning
						pData->PlayMode = PLAYMODE_PLAY;
						ActView_SetFrameTime (pData, pData->StartExtent);
						break;

					case PLAYMODE_PAUSE :
						// if paused, then continue...
						pData->PlayMode = PLAYMODE_PLAY;
						ActView_SetFrameTime (pData, pData->LastFrameTime);
						break;
				}
			}
			return 0;

		case IDI_STOP :
			if (pData->Motion != NULL)
			{
				if (pData->PlayMode != PLAYMODE_STOP)
				{
					pData->PlayMode = PLAYMODE_STOP;
				}
			}
			return 0;

		case IDI_PAUSE :
			if (pData->Motion != NULL)
			{
				switch (pData->PlayMode)
				{
					case PLAYMODE_PLAY :		// animation playing...pause it
						pData->PlayMode = PLAYMODE_PAUSE;
						break;

					case PLAYMODE_STOP :
					case PLAYMODE_PAUSE :		// animation paused...restart it
						pData->PlayMode = PLAYMODE_PLAY;
						ActView_SetFrameTime (pData, pData->LastFrameTime);
						break;
				}
			}
			return 0;

		case IDI_RRFRAME :
			ActView_AdjustFrameTime (pData, -(pData->FrameDelta));
			return 0;

		case IDI_FFFRAME :
			ActView_AdjustFrameTime (pData, pData->FrameDelta);
			return 0;

		case IDI_RRSTART :		// rewind to start of animation
			ActView_SetFrameTime (pData, pData->StartExtent);
			return 0;

		case IDI_FFEND :		// go to end of animation
			ActView_SetFrameTime (pData, pData->EndExtent);
			return 0;

		// Set predefined orientation.
		// All of these are rotational only...translations remain
		case IDC_FRONT :
			pData->XRotActor = pData->InitialXRot;
			pData->YRotActor = pData->InitialYRot;
			return 0;

		case IDC_BACK :
			pData->XRotActor = pData->InitialXRot;
			pData->YRotActor = pData->InitialYRot + GE_PI;
			return 0;

		case IDC_LEFT :
			pData->XRotActor = pData->InitialXRot;
			pData->YRotActor = pData->InitialYRot - GE_PI/2.0f;
			return 0;

		case IDC_RIGHT :
			pData->XRotActor = pData->InitialXRot;
			pData->YRotActor = pData->InitialYRot + GE_PI/2.0f;
			return 0;

		case IDC_TOP :
			pData->XRotActor = pData->InitialXRot + GE_PI/2.0f;
			pData->YRotActor = pData->InitialYRot;
			return 0;

		case IDC_BOTTOM :
			pData->XRotActor = pData->InitialXRot - GE_PI/2.0f;
			pData->YRotActor = pData->InitialYRot;
			return 0;

		case IDC_CENTER :
			// Put actor back at default position
			geVec3d_Clear (&pData->ActorPos);
			return 0;

		case IDC_BLEND :
			if ((pData->ActorDef != NULL) && (pData->Actor != NULL))
			{
				// if blender window exists, then display it.
				if (pData->hwndBlender != NULL)
				{
					SetForegroundWindow (pData->hwndBlender);
				}
				else
				{
					// otherwise create it
					pData->hwndBlender = Blender_Create (pData->hwnd, pData->Instance, pData->ActorDef, pData->BlendedMotion);
					if (pData->hwndBlender != NULL)
					{
						ShowWindow (pData->hwndBlender, SW_SHOW);
						UpdateWindow (pData->hwndBlender);
					}
				}
				// and select the blended motion if not already selected
				if (pData->CurrentMotion != BLENDED_MOTION_INDEX)
				{
					int Item, nItems;
					HWND hwndCombo = GetDlgItem (pData->hwnd, IDC_MOTIONCOMBO);

					nItems = SendMessage (hwndCombo, CB_GETCOUNT, 0, 0);
					for (Item = 0; Item < nItems; ++Item)
					{
						int iMotion = SendMessage (hwndCombo, CB_GETITEMDATA, 0, 0);
						if (iMotion == BLENDED_MOTION_INDEX)
						{
							SendMessage (hwndCombo, CB_SETCURSEL, Item, 0);
							ActView_SelectMotion (pData, BLENDED_MOTION_INDEX);
							ActView_SetFrameTime (pData, pData->StartExtent);
							break;
						}
					}
				}
			}
			return 0;
	}
	return 0;
}
#pragma warning (default:4100)

// Moves the control specified by id by changing the control's vertical position
// as indicated by the DeltaY value.
static void ActView_MoveControl
	(
	  HWND hdlg,
	  int id,
	  int DeltaY,
	  int *MaxX
	)
{
	HWND hwndCtrl;

	// get the control's window handle.
	hwndCtrl = GetDlgItem (hdlg, id);
	if (hwndCtrl != NULL)
	{
		RECT WindowRect;
		POINT pt;

		// get its current position
		GetWindowRect (hwndCtrl, &WindowRect);
		// position is in screen coordinates, and we need dialog window relative
		pt.x = WindowRect.left;
		pt.y = WindowRect.top + DeltaY;
		ScreenToClient (hdlg, &pt);

		// and move the control
		SetWindowPos (hwndCtrl, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

		// and check right side for max X
		pt.x = WindowRect.right;
		pt.y = WindowRect.bottom + DeltaY;
		ScreenToClient (hdlg, &pt);
		if (pt.x > *MaxX)
		{
			*MaxX = pt.x;
		}
	}
}

// Creates tooltips window and returns its handle
static HWND CreateTooltipsWindow (HWND hwnd)
{

	// Controls that will be tipped...
	static const int TooltipButtons[] =
	{
		IDI_PLAY,		IDI_PAUSE,	IDI_STOP,	IDI_RRSTART,	IDI_RRFRAME, 
		IDI_FFFRAME,	IDI_FFEND,	IDC_ZOOM,	IDC_PAN,		IDC_ROTATE,
		IDC_FRONT,		IDC_BACK,	IDC_LEFT,	IDC_RIGHT,		IDC_TOP, 
		IDC_BOTTOM,		IDC_CENTER,	IDC_STATICCURRENTTIME,		IDC_STATICEND,
		IDC_EDITSCALE,	IDC_EDITSPEED,	IDC_EDITFRAMETIME,		IDC_BLEND,
		IDC_STATICSTART
	};
	static const int nTooltipButtons = sizeof (TooltipButtons) / sizeof (int);

	int iButton;
	HWND hwndTT;
	HINSTANCE hInst = (HINSTANCE)GetWindowLong (hwnd, GWL_HINSTANCE);

	// Create tooltip control's window
	hwndTT = CreateWindowEx 
	(
		0,
		TOOLTIPS_CLASS, 
		NULL, 
		TTS_ALWAYSTIP,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		hwnd,
		(HMENU)NULL,
		hInst,
		NULL
	);

	if (hwndTT == NULL)
	{
		return NULL;
	}

	// Add each of the controls in the array to the tooltip
	for (iButton = 0; iButton < nTooltipButtons; ++iButton)
	{
		TOOLINFO ti;
		char TipBuffer[MAX_PATH];

		TipBuffer[0] = '\0';
		LoadString (hInst, TooltipButtons[iButton], TipBuffer, sizeof (TipBuffer));
		ti.cbSize = sizeof (TOOLINFO);
		ti.uFlags = TTF_IDISHWND;
		ti.hwnd = hwnd;
		ti.uId = (UINT)GetDlgItem (hwnd, TooltipButtons[iButton]);
		ti.hinst = 0;
		ti.lpszText = LPSTR_TEXTCALLBACK;
		SendMessage (hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
	}
	return hwndTT;
}

// Process hi resolution timer messages.
#pragma warning (disable:4100)
static void CALLBACK hiresTimerProc (UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	// The user data should be the window handle.
	HWND hwnd = (HWND)dwUser;
	// get the data
	ActView_WindowData *pData = (ActView_WindowData *)GetWindowLong (hwnd, GWL_USERDATA);

	// if there's no user data, then this message isn't for us.
	// Don't know how to handle it, so just quit.
	if (pData == NULL)
	{
		return;
	}

	// Well, if the timer id matches the timer id we created, then go for it.
	if ((pData->hiresTimer != 0) && (uID == (UINT)pData->hiresTimer))
	{
		if (pData->ProcessingTimerMessage == FALSE)
		{
			// we use a flag to prevent overflowing the queue on a slow machine
			pData->ProcessingTimerMessage = TRUE;
			PostMessage (hwnd, pData->TimerMessageId, uID, 0);
		}
	}
}
#pragma warning (default:4100)

static BOOL ActView_InitializeDialog (HWND hwnd)
{
	ActView_WindowData *pData;

	// allocate window local data structure
	pData = GE_RAM_ALLOCATE_STRUCT (ActView_WindowData);
	if (pData == NULL)
	{
		DestroyWindow (hwnd);
		return TRUE;
	}

	// and initialize it
	pData->Instance		= (HINSTANCE)GetWindowLong (hwnd, GWL_HINSTANCE);
	pData->hwnd			= hwnd;
	*pData->ActorFilename = '\0';
	*pData->LastDir		= '\0';
	pData->Camera		= NULL;
	pData->World		= NULL;
	pData->Engine		= NULL;
	pData->Actor		= NULL;
	pData->ActorDef		= NULL;
	pData->LastX		= 0;
	pData->LastY		= 0;
	pData->Motion		= 0;
	pData->PlayMode		= PLAYMODE_STOP;
	pData->MouseMode	= AWM_NONE;
	pData->Scale		= 100;
	pData->Speed		= 100;
	pData->FrameDelta	= 5;
	pData->LastFrameTime = 0.0f;
	pData->Loop			= GE_FALSE;
	pData->hwndTT		= NULL;
	pData->hwndBlender	= NULL;
	pData->ShowFrameRate = FALSE;
	pData->loresTimer	= 0;
	pData->hiresTimer	= 0;
	pData->ProcessingTimerMessage = FALSE;

	// create an empty motion for the blender
	pData->BlendedMotion = geMotion_Create (GE_TRUE);
	if (pData->BlendedMotion == NULL)
	{
		return FALSE;
	}

	// Load program options.
	ActView_LoadOptions (pData);

	// Set the window data pointer in the GWL_USERDATA field
	SetWindowLong (hwnd, GWL_USERDATA, (LPARAM)pData);

	// set the program icon on the dialog window
	SendMessage (hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon (pData->Instance, MAKEINTRESOURCE (IDI_MAINICON)));


	// Set scale, speed, and frame time values
	SetSpinnerRange (hwnd, IDC_SPINSCALE, 1, 10000);
	SetEditControlValue (hwnd, IDC_EDITSCALE, IDC_SPINSCALE, pData->Scale);

	SetSpinnerRange (hwnd, IDC_SPINSPEED, 1, 10000);
	SetEditControlValue (hwnd, IDC_EDITSPEED, IDC_SPINSPEED, pData->Speed);

	SetSpinnerRange (hwnd, IDC_SPINFRAMETIME, 1, 100);
	SetEditControlValue (hwnd, IDC_EDITFRAMETIME, IDC_SPINFRAMETIME, pData->FrameDelta);

	// Setup slider
	SetSliderRange (pData, IDC_SLIDERTIME);
	SetSliderTime (hwnd, IDC_SLIDERTIME, 0.0f);

	{
		// position render window and the rest of the controls.
		int DeltaY;
		int RenderWindowWidth;

		HWND hwndRender = GetDlgItem (hwnd, IDC_RENDERWIN);

		// Initialize the dialog box.
		// Set render window to proper size and position.
		// Resize dialog box to fit
		// Move all the other controls to the bottom of the dialog
		// Initialize the engine in the render window
		{
			// Size the render window.
			// When done, ClientRect will contain the required width of
			// the main window's client rect.
			int Style = GetWindowLong (hwndRender, GWL_STYLE);
			int OldBottom;
			RECT WindowRect, ClientRect;

			GetWindowRect (hwndRender, &WindowRect);
			OldBottom = WindowRect.bottom;

			// compute window size for client rect
			ClientRect.left = 0;
			ClientRect.top = 0;
			ClientRect.right = ACTOR_WINDOW_WIDTH - 1;
			ClientRect.bottom = ACTOR_WINDOW_HEIGHT - 1;

			AdjustWindowRect (&ClientRect, Style, FALSE);	// FALSE == No menu
			{
				int WindowWidth = ClientRect.right - ClientRect.left + 1;
				int WindowHeight = ClientRect.bottom - ClientRect.top + 1;

				SetWindowPos 
				(
					hwndRender, 0,
					RENDER_BORDER, RENDER_BORDER, WindowWidth, WindowHeight,
					SWP_NOCOPYBITS | SWP_NOZORDER
				);

				// Compute DeltaY--the vertical position difference between the old
				// bottom of render window and new bottom of render window.
				GetWindowRect (hwndRender, &WindowRect);
				DeltaY = WindowRect.bottom - OldBottom;
				RenderWindowWidth = WindowRect.right - WindowRect.left - 1;
			}
		}


		{
			RECT ClientRect;
			int ClientWidth, ClientHeight;
			int Style;
			static int MoveableControls[] =
			{
				IDC_STATICSTART, IDC_STATICCURRENTTIME, IDC_STATICEND, IDC_SLIDERTIME,
				IDC_PAN, IDC_ROTATE, IDC_ZOOM,
				IDC_STATICMOTION, IDC_MOTIONCOMBO,
				IDC_LOOPED,
				IDC_STATICSCALE, IDC_EDITSCALE, IDC_SPINSCALE,
				IDC_STATICSPEED, IDC_EDITSPEED, IDC_SPINSPEED,
				IDC_STATICFRAME, IDC_EDITFRAMETIME, IDC_SPINFRAMETIME,
				IDC_FRONT, IDC_BACK, IDC_LEFT, IDC_RIGHT, IDC_TOP, IDC_BOTTOM, IDC_CENTER,
				IDI_PLAY, IDI_STOP, IDI_PAUSE, IDI_RRFRAME, IDI_FFFRAME, IDI_RRSTART, IDI_FFEND,
				IDC_BLEND
			};
			static int NumControls = sizeof (MoveableControls) / sizeof (int);
			int i, MaxX;

			MaxX = RenderWindowWidth;
			// Add the DeltaY value (computed above) to the position of all dialog controls
			// except the render window.
			for (i = 0; i < NumControls; ++i)
			{
				ActView_MoveControl (hwnd, MoveableControls[i], DeltaY, &MaxX);
			}

			GetClientRect (hwnd, &ClientRect);
			// add border for render window
			ClientRect.bottom += DeltaY + RENDER_BORDER;
			ClientRect.right = 2*RENDER_BORDER + MaxX;

			// adjust the dialog window's size.
			Style = GetWindowLong (hwnd, GWL_STYLE);
			AdjustWindowRect (&ClientRect, Style, TRUE);
			ClientWidth = ClientRect.right - ClientRect.left + 1;
			ClientHeight = ClientRect.bottom - ClientRect.top + 1;
			{
				int left, top;

				// Center the dialog on the screen
				left = (GetSystemMetrics (SM_CXSCREEN) - ClientWidth)/2;
				top = (GetSystemMetrics (SM_CYSCREEN) - ClientHeight)/2;

				SetWindowPos
				(
					hwnd, NULL,
					left, top, ClientWidth, ClientHeight,
					SWP_NOCOPYBITS | SWP_NOZORDER
				);
			}
			
			// Now center the rendered window horizontally in the dialog
			{
				RECT rectRender;
				RECT rectParent;
				POINT pt;

				GetWindowRect (hwnd, &rectParent);
				GetWindowRect (hwndRender, &rectRender);

				pt.x = rectRender.left;
				pt.y = rectRender.top;
				ScreenToClient (hwnd, &pt);

				pt.x = (rectParent.right - rectParent.left)/2 - RenderWindowWidth/2;
				SetWindowPos (hwndRender, NULL, pt.x, pt.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			}
		}

		{
			// Add images to the buttons.

			// Array of button ids and image identifiers
			static const int ButtonImages[][2] =
			{
				{IDI_PLAY,		IDI_PLAY},		// Button ID, image ID
				{IDI_PAUSE,		IDI_PAUSE},
				{IDI_STOP,		IDI_STOP},
				{IDI_RRFRAME,	IDI_RRFRAME},
				{IDI_FFFRAME,	IDI_FFFRAME},
				{IDI_RRSTART,	IDI_RRSTART},
				{IDI_FFEND,		IDI_FFEND},
				{IDC_ZOOM,		IDI_ZOOM},
				{IDC_PAN,		IDI_PAN},
				{IDC_ROTATE,	IDI_ROTATE},
				{IDC_BLEND,		IDI_BLEND}
			};
			static const int nButtons = sizeof (ButtonImages) / (2*sizeof (int));
			int iButton;

			for (iButton = 0; iButton < nButtons; ++iButton)
			{
				// set the button image
				SendDlgItemMessage 
				(
					hwnd,
					ButtonImages[iButton][0],
					BM_SETIMAGE, 
					IMAGE_ICON, 
					(LPARAM)LoadIcon (pData->Instance, MAKEINTRESOURCE (ButtonImages[iButton][1]))
				);
			}
		}

		/*
		  Create the tooltips window so we can do flyby hints.
		  If the window is created successfully, then we install a message filter hook 
		  so the dialog can capture and respond to mouse movement messages and thus
		  provide the tooltips.
		*/
		pData->hwndTT = CreateTooltipsWindow (hwnd);
		if (pData->hwndTT != NULL)
		{
			ActView_ToolHook = SetWindowsHookEx (WH_GETMESSAGE, (HOOKPROC)ActView_HookProc, NULL, GetCurrentThreadId ());
		}
	}

	DragAcceptFiles (hwnd, TRUE);

	return TRUE;
}

static void ActView_ShutdownAll
	(
	  HWND hwnd,
	  ActView_WindowData *pData
	)
{
	// don't want no more files.
	DragAcceptFiles (hwnd, FALSE);

	// Uninstall the message hook (ugliness ensues if you forget this)
	if (ActView_ToolHook != NULL)
	{
		UnhookWindowsHookEx (ActView_ToolHook);
		ActView_ToolHook = NULL;
	}

	// save last directory
	if (*pData->IniFilename != '\0')
	{
		WritePrivateProfileString ("Options", "LastDir", pData->LastDir, pData->IniFilename);
	}

	// When the window is closed, we need to notify the parent
	// so that it can remove us from its list.
	// We also need to free our local data.
	if (pData != NULL)
	{
		// Free all allocated data

		// shut down timers first thing...
		if (pData->loresTimer != 0)
		{
			KillTimer (hwnd, pData->loresTimer);
			pData->loresTimer = 0;
		}
		if (pData->hiresTimer != 0)
		{
			timeKillEvent (pData->hiresTimer);
			pData->hiresTimer = 0;
			timeEndPeriod (DESIRED_MMTIMER_RESOLUTION);
		}

		// Shut down the engine and all its associated stuff
		if (pData->BlendedMotion != NULL)
		{
			geMotion_Destroy (&pData->BlendedMotion);
			pData->BlendedMotion = NULL;
		}

		if (pData->Actor != NULL)
		{
			geWorld_RemoveActor (pData->World, pData->Actor);
			geActor_Destroy (&pData->Actor);
			pData->Actor = NULL;
		}

		if (pData->ActorDef != NULL)
		{
			geActor_DefDestroy (&pData->ActorDef);
			pData->ActorDef	= NULL;
		}

		if (pData->Camera != NULL)
		{
			geCamera_Destroy (&pData->Camera);
			pData->Camera = NULL;
		}
		if (pData->World != NULL)
		{
			geWorld_Free (pData->World);
			pData->World = NULL;
		}

		if (pData->Engine != NULL)
		{
			geEngine_Free (pData->Engine);
			pData->Engine		= NULL;
		}
	}

	if (pData != NULL)
	{
		geRam_Free (pData);
	}
	SetWindowLong (hwnd, GWL_USERDATA, (LPARAM)NULL);
}

#pragma warning (disable:4100)
static BOOL CALLBACK ActView_DlgProc
	(
	  HWND hwnd,
	  UINT msg,
	  WPARAM wParam,
	  LPARAM lParam
	)
{
	ActView_WindowData *pData = ActView_GetWindowData (hwnd);

	// if the message is a wakeup call, then wake up and return.
	if (msg == InstCheck_GetUniqueMessageId ())
	{
		if (IsIconic(hwnd))
		{
			ShowWindow(hwnd, SW_RESTORE);
		}
		SetForegroundWindow (hwnd);
		return 0;
	}

	// Process our timer messages.
	if ((pData != NULL) && (msg == pData->TimerMessageId))
	{
		if (((pData->hiresTimer != 0) && ((MMRESULT)wParam == pData->hiresTimer)) ||
			((pData->loresTimer != 0) && ((UINT)wParam == pData->loresTimer)))
		{
			ActView_UpdateFrame (pData);
			// clear the flag so another timer message can come through
			pData->ProcessingTimerMessage = FALSE;
			return FALSE;
		}
	}

	switch (msg)
	{
		case WM_INITDIALOG :
			return ActView_InitializeDialog (hwnd);

		case WM_DESTROY :
			ActView_ShutdownAll (hwnd, pData);
			PostQuitMessage (0);
			break;

		case WM_COMMAND :
		{
			WORD wNotifyCode = HIWORD (wParam);
			WORD wID = LOWORD (wParam);
			HWND hwndCtl = (HWND)lParam;

			return wm_Command (hwnd, pData, wNotifyCode, wID, hwndCtl);
		}

		case WM_DROPFILES :
		{
			// Files dropped.  Only take the first one.
			HDROP hDrop;
			UINT FileCount;
			char Buff[MAX_PATH];

			hDrop = (HDROP)wParam;
			FileCount = DragQueryFile (hDrop, 0xffffffff, Buff, sizeof (Buff));
			if (FileCount > 0)
			{
				// get the first one and open it...
				DragQueryFile (hDrop, 0, Buff, sizeof (Buff));
			}
			DragFinish (hDrop);
			ActView_DoLoadActor (pData, Buff);
			return 0;
		}

		case WM_MOUSEMOVE :
		{
			/*
			  If the mouse moves in the render window and the left mouse button
			  is down, then we need to move the actor or the camera based on
			  the MouseMode state (Pan, Rotate, Zoom).
			*/
			RECT WindowRect;
			POINT pt;

			pt.x = LOWORD (lParam);
			pt.y = HIWORD (lParam);
			/*
			  The mouse coordinates we're given are window-relative.
			  But the window rect that we use to determine if we're within the
			  render window is in screen coordinates, so we convert the mouse pos.
			*/
			ClientToScreen (hwnd, &pt);
			GetWindowRect (GetDlgItem (hwnd, IDC_RENDERWIN), &WindowRect);
			if (PtInRect (&WindowRect, pt) && IsKeyDown (VK_LBUTTON))
			{
				static float Velocity = 1.0f;
				static float ActorAdjust = 1.0f;
				// compute movement deltas.
				float dx = (float)(pt.x - pData->LastX);
				float dy = (float)(pt.y - pData->LastY);

				switch (pData->MouseMode)
				{
					case AWM_PAN :
						// pan moves the actor's position
						pData->ActorPos.X += (ActorAdjust * dx);
						pData->ActorPos.Y -= (ActorAdjust * dy);
						break;

					case AWM_ROTATE :
						// Rotate changes the actor's orientation.
						// 150.0f is an emperically derived value.
						pData->XRotActor += (dy/150.0f);
						pData->YRotActor += (dx/150.0f);
						break;

					case AWM_ZOOM :
						// zoom just moves the camera.
						pData->Dist += (Velocity * dy);
						// don't let camera go beyond origin...
						if (pData->Dist < Velocity)
						{
							pData->Dist = Velocity;
						}
						break;
				}
			}
			// in any case, we update the current position
			pData->LastX = pt.x;
			pData->LastY = pt.y;

			return 0;
		}

		case WM_SETCURSOR :
			return SetRenderCursor (pData, pData->MouseMode);

		case WM_VSCROLL :
		{
			// handle spinners
			HWND hwndScrollbar = (HWND)lParam;
//			int nPos = HIWORD (wParam);

			if (hwndScrollbar == GetDlgItem (hwnd, IDC_SPINSCALE))
			{
				ActView_UpdateScale (pData);
			}
			else if (hwndScrollbar == GetDlgItem (hwnd, IDC_SPINSPEED))
			{
				ActView_UpdateSpeed (pData);
			}
			else if (hwndScrollbar == GetDlgItem (hwnd, IDC_SPINFRAMETIME))
			{
				ActView_UpdateFrameDelta (pData);
			}
			return 0;
		}

		case WM_HSCROLL :
		{
			// handle slider
			HWND hwndScrollbar = (HWND)lParam;

			if (hwndScrollbar == GetDlgItem (hwnd, IDC_SLIDERTIME))
			{
				int nPos = SendMessage (hwndScrollbar, TBM_GETPOS, 0, 0);
				ActView_SetFrameTime (pData, floatPercent (nPos));
			}
			return 0;
		}

		case WM_NOTIFY :
		{
			NMHDR  *pHdr = (NMHDR *)lParam;


			if (pHdr->code == TTN_NEEDTEXT)
			{
				// The tooltip control will pass a TTN_NEEDTEXT message to the
				// parent (us) when the mouse passes over one of the controls that
				// has a hint associated with it.
				TOOLTIPTEXT *ptt = (TOOLTIPTEXT *)lParam;
				int idCtrl = GetDlgCtrlID ((HWND)pHdr->idFrom);

				ptt->lpszText = (char *)rcstring_Load (pData->Instance, idCtrl);
			}
			return 0;
		}

		case WM_BLENDERCLOSING :
			// Message passed when the child dialog is closed.
			// If this is the same as the blender dialog, then clear our blender
			// dialog value.
			if ((HWND)wParam == pData->hwndBlender)
			{
				pData->hwndBlender = NULL;
			}
			return 0;

		case WM_BLENDERMOTIONCHANGED :
			// Message passed when the blender dialog changes the motion.
			// Parent must reload motion because its time extents may have changed.
			if (pData->CurrentMotion == BLENDED_MOTION_INDEX)
			{
				// load blended motion and redraw the UI
				ActView_SelectMotion (pData, BLENDED_MOTION_INDEX);
			}
			return 0;

		default :
			break;

	}
	return FALSE;
}
#pragma warning (default:4100)


// Loads the Genesis3D engine.  Returns GE_TRUE if successful.
// returns GE_FALSE if something failed.
static geBoolean StartTheEngine
	(
	  HWND hwndParent,
	  int ControlID,
	  ActView_WindowData *pData
	)
{
	HWND hwnd = GetDlgItem (hwndParent, ControlID);
	assert (hwnd != NULL);

	//
	// Start up the engine
	//
	pData->Engine = geEngine_Create 
	(
		hwnd, 
		rcstring_Load (pData->Instance, IDS_PROGRAMBASENAME),
		"."		// driver directory
	);
	
	if (pData->Engine == NULL)
	{
		MyError (hwndParent, pData->Instance, IDS_CANTCREATEENGINE);
		return GE_FALSE;
	}

	//
	//	Setup the viewing rect
	//
	pData->Rect.Left = 0;
	pData->Rect.Right = ACTOR_WINDOW_WIDTH - 1;
	pData->Rect.Top = 0;
	pData->Rect.Bottom = ACTOR_WINDOW_HEIGHT - 1;

	// Initial position is 100 from origin along the Z axis...
	pData->XRotCam = 0.0f;
	pData->YRotCam = 0.0f;
	pData->Dist = 100.0f;
	pData->Height = 25.0f;

	//
	//	Create a camera
	//
	pData->Camera = geCamera_Create(GE_PI/2.0f, &pData->Rect);

	if (pData->Camera == NULL)
	{
		MyError (hwndParent, pData->Instance, IDS_CANTCREATECAMERA);
		return GE_FALSE;
	}

	// Create an empty world (this is cool, huh?)
	pData->World = geWorld_Create (NULL);

	if (pData->World == NULL)
	{
		MyError (hwndParent, pData->Instance, IDS_CANTCREATEWORLD);
		return GE_FALSE;
	}

	if (geEngine_AddWorld (pData->Engine, pData->World) == GE_FALSE)
	{
		MyError (hwndParent, pData->Instance, IDS_CANTADDWORLD);
		return GE_FALSE;
	}

	// Set some light type defaults
	geWorld_SetLTypeTable (pData->World, 0, "z");
	geWorld_SetLTypeTable (pData->World, 1, "mmnmmommommnonmmonqnmmo");
	geWorld_SetLTypeTable (pData->World, 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	geWorld_SetLTypeTable (pData->World, 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	geWorld_SetLTypeTable (pData->World, 4, "mamamamamama");
	geWorld_SetLTypeTable (pData->World, 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	geWorld_SetLTypeTable (pData->World, 6, "nmonqnmomnmomomno");
	geWorld_SetLTypeTable (pData->World, 7, "mmmaaaabcdefgmmmmaaaammmaamm");
	geWorld_SetLTypeTable (pData->World, 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	geWorld_SetLTypeTable (pData->World, 9, "aaaaaaaazzzzzzzz");
	geWorld_SetLTypeTable (pData->World, 10,"mmamammmmammamamaaamammma");
	geWorld_SetLTypeTable (pData->World, 11,"abcdefghijklmnopqrrqponmlkjihgfedcba");

	{
		// Select driver from list.
		// In this case, DrvList has been modified to present only
		// those drivers that have a windowed mode.
		geDriver *Driver;
		geDriver_Mode *Mode;

		DrvList_PickDriver (pData->Instance, hwndParent, pData->Engine, &Driver, &Mode);
		if ((Driver == NULL) || (Mode == NULL))
		{
			MyError (hwndParent, pData->Instance, IDS_CANTLOADDRIVER);
			return GE_FALSE;
		}
		// disable frame rate when displaying splash screen...
		geEngine_EnableFrameRateCounter (pData->Engine, GE_FALSE);

		if (geEngine_SetDriverAndMode (pData->Engine, Driver, Mode) == GE_FALSE)
		{
			MyError (hwndParent, pData->Instance, IDS_CANTLOADDRIVER);
			return GE_FALSE;
		}

	}

	return GE_TRUE;
}


#pragma warning (disable:4100)
int CALLBACK WinMain
      (
        HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR cmd_line,
        int cmd_show
      )
{
	ActView_WindowData *pData;

	{
		HWND prev_hwnd;

		// Check for previous instance and exit if one found
		prev_hwnd = InstCheck_CheckForPreviousInstance
		(
			instance,
			rcstring_Load (instance, IDS_PROGRAMNAME),
			rcstring_Load (instance, IDS_UNIQUEMESSAGE)
		);

		if (prev_hwnd != NULL)
		{
			// send wakeup call to the previous instance
			SendMessage (prev_hwnd, InstCheck_GetUniqueMessageId (), 0, 0);
			return 0;
		}
	}

	InitCommonControls ();

    // create main window.
	// This is the controlling dialog.
    ActView_DlgHandle = CreateDialog 
	(
		instance, 
		MAKEINTRESOURCE (IDD_ACTOR),
		NULL, 
		ActView_DlgProc
	);

    if (ActView_DlgHandle == NULL)
    {
        return 0;
    }

	pData = ActView_GetWindowData (ActView_DlgHandle);
	
	ShowWindow (ActView_DlgHandle, SW_SHOWNORMAL);
	UpdateWindow (ActView_DlgHandle);

	// The window's created the right size, so startup the engine
	if (!StartTheEngine (ActView_DlgHandle, IDC_RENDERWIN, pData))
	{
		MyError (ActView_DlgHandle, pData->Instance, IDS_CANTINITENGINE);
		DestroyWindow (ActView_DlgHandle);
		return 0;
	}

	// clear render window's text
	// (otherwise it might show through at times...trust me)
	SetDlgItemText (ActView_DlgHandle, IDC_RENDERWIN, "");

	ActView_SetupTitle (pData);
	ActView_SetFrameRateToggle (pData);

	// Check the command line.  If a filename specified, try to load it.
	{
		if (*cmd_line != '\0')
		{
			ActView_DoLoadActor (pData, cmd_line);
		}
	}

	// Set up timer stuff
	// Register custom window message for timer stuff
	pData->TimerMessageId = RegisterWindowMessage (ActViewMessageString);
	if (pData->TimerMessageId != 0)
	{
		if (timeBeginPeriod (DESIRED_MMTIMER_RESOLUTION) == TIMERR_NOERROR)
		{
			pData->hiresTimer = timeSetEvent (DESIRED_MMTIMER_FREQUENCY, DESIRED_MMTIMER_RESOLUTION, hiresTimerProc, (DWORD)ActView_DlgHandle, TIME_PERIODIC);
			if (pData->hiresTimer == 0)
			{
				timeEndPeriod (DESIRED_MMTIMER_RESOLUTION);
			}
		}
	}
	
	if (pData->hiresTimer == 0)
	{
		pData->TimerMessageId = WM_TIMER;
		pData->loresTimer = SetTimer (ActView_DlgHandle, LORES_TIMER_NUMBER, 50, NULL);
		if (pData->loresTimer == 0)
		{
			MyError (ActView_DlgHandle, pData->Instance, IDS_CANTSTARTTIMER);
		}
	}

	{
		// the application's message loop
		MSG Msg;
		HACCEL accel = LoadAccelerators (instance, MAKEINTRESOURCE (IDR_ACCELERATOR1));

		// do the message loop thing...
		while (GetMessage( &Msg, NULL, 0, 0))
		{
			/*
			  This is kind of weird.  
			  The main window is a dialog that has a menu.
			  In order for accelerators to work, we have to check the accelerators
			  first, then IsDialogMessage, then standard Windows message processing.
			  Any other order causes some weird behavior.
			*/
			if ((pData->hwndBlender == NULL) || !IsDialogMessage (pData->hwndBlender, &Msg))
			{
				if ((accel == NULL) || !TranslateAccelerator (ActView_DlgHandle, accel, &Msg))
				{
					if (!IsDialogMessage (ActView_DlgHandle, &Msg))
					{
						TranslateMessage(&Msg);
						DispatchMessage(&Msg);
					}
				}
			}
		}
	}	

	return 0;
}

#pragma warning (default:4100)
