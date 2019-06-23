/****************************************************************************************/
/*  INSTCHECK.C                                                                         */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Function to check for previous program instances.                     */
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
#include "InstCheck.h"

#pragma warning (disable:4514)

static UINT InstCheck_UniqueMessage = 0;

// data structure passed to previous instance callback function
typedef struct
{
	HWND prev_hwnd;
	const char *TitleText;
	HINSTANCE hinst;
} InstCheck_FindStruct;

// Callback called by EnumWindows to find previous instance of application.
static BOOL CALLBACK InstCheck_FindPrevInstance
	(
	  HWND hwnd,
	  LPARAM lParam
	)
{
	InstCheck_FindStruct *fs = (InstCheck_FindStruct *)lParam;
	const char *ProgramName;
	char WindowName[1000];

	// Since the window is a dialog that uses the standard dialog class,
	// we can't use FindWindow to find a window with the class name.
	// And we can't use FindWindow to find a window with a specific title
	// because the title changes based on the loaded file.
	// So we get each window's title and check the beginning of it for
	// the program name, which should be there.
	ProgramName = fs->TitleText;
	GetWindowText (hwnd, WindowName, sizeof (WindowName));

	if (strncmp (ProgramName, WindowName, strlen (ProgramName)) == 0)
	{
		// Found it.
		// Set the callback data and tell EnumWindows to stop enumerating.
		fs->prev_hwnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

HWND InstCheck_CheckForPreviousInstance
	(
	  HINSTANCE instance,
	  const char *TitleText,
	  const char *UniqueMessageString
	)
{
	// Previous instance check.

	// If an instance of this program is already running, then
	// bring it to the front and shut down this instance.
	InstCheck_FindStruct fs;
	BOOL FoundIt;

	fs.hinst = instance;
	fs.TitleText = TitleText;
	fs.prev_hwnd = NULL;

	// Register a unique windows message for this application
	InstCheck_UniqueMessage = RegisterWindowMessage (UniqueMessageString);

	// go see if there is already a window of this type
	// EnumWindows will return FALSE if our callback function finds the window.
	FoundIt = !(EnumWindows (InstCheck_FindPrevInstance, (LPARAM)&fs));

	return fs.prev_hwnd;
}

UINT InstCheck_GetUniqueMessageId (void)
{
	return InstCheck_UniqueMessage;
}
