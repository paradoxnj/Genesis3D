/****************************************************************************************/
/*  MAKEHELP.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Helper functions for interface between GUI and actor make subsystem.	*/
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
#include "stdafx.h"
#include "MakeHelp.h"
#include "Filepath.h"
#include <string.h>
#include "resource.h"
#include <assert.h>
#include "make.h"
#include <objbase.h>

static CWnd *GlobalMessagesWindow = NULL;


void MakeHelp_SetMessagesWindow (CWnd *MsgWindow)
{
	GlobalMessagesWindow = MsgWindow;
}


extern "C" void MakeHelp_Printf (const char *fmt, ...)
{
	va_list argptr;
	char	buf[4096];		// that's gotta be big enough...

	va_start (argptr, fmt);
	vsprintf (buf, fmt, argptr);
	va_end (argptr);

	// buf now contains the formatted string.
	// Need to go through and convert all \n to \r\n
	// We do this by making a single forward pass to count the number of \n characters.
	// Then a reverse pass does the translation
	char *chFrom, *chTo;
	int NumLines = 0;
	for (chFrom = buf; *chFrom != '\0'; ++chFrom)
	{
		if (*chFrom == '\n')
		{
			++NumLines;
		}
	}
	chTo = chFrom + NumLines;
	for (chTo = chFrom + NumLines; chTo > chFrom; --chTo,--chFrom)
	{
		*chTo = *chFrom;
		if (*chFrom == '\n')
		{
			--chTo;
			*chTo = '\r';
		}
	}

	if (GlobalMessagesWindow != NULL)
	{
		// Since this is calling on a different thread, we'll put the message
		// in allocated memory and post a message to the window.  It'll have
		// to handle the message and deallocate the memory.
		char *TheMessage = (char *)CoTaskMemAlloc (lstrlen (buf)+1);
		if (TheMessage != NULL)
		{
			lstrcpy (TheMessage, buf);
#pragma todo ("This probably should use PostThreadMessage.")
			GlobalMessagesWindow->PostMessage (WM_USER_COMPILE_MESSAGE, 0, (LPARAM)TheMessage);
		}
	}
}



bool MakeHelp_GetRelativePath (const AProject *Project, const char *NewPath, char *Dest)
{
	char ProjectDir[MAX_PATH];
	char NewFileDir[MAX_PATH];

	*ProjectDir = '\0';
	*NewFileDir = '\0';

	if (AProject_GetForceRelativePaths (Project))
	{
		// see if NewPath is a subdir of the project path...

		// Get project path, which just happens to be the current directory...
		GetCurrentDirectory (MAX_PATH, ProjectDir);
		if (FilePath_GetDriveAndDir (NewPath, NewFileDir) != GE_FALSE)
		{
			// There's a path on the filename.
			// If there's no drive or leading slash, then it must be relative
			char PathWork[MAX_PATH];

			if (FilePath_GetDrive (NewPath, PathWork) == GE_FALSE)
			{
				// no drive, must be a dir
				FilePath_GetDir (NewPath, PathWork);
				if (*PathWork != '\\')
				{
					// no leading slash, so it's relative.
					strcpy (Dest, NewPath);
					return true;
				}
			}
			// NewPath's directory has to contain ProjectDir at the beginning of it
			if (_strnicmp (ProjectDir, NewFileDir, strlen (ProjectDir)) == 0)
			{
				// it's probably good
				// copy the remaining part to the destination
				strcpy (Dest, &NewPath[strlen (ProjectDir)]);
				if (*Dest == '\\')
				{
					// don't want a leading slash, if ya know what I mean...
					strcpy (Dest, Dest+1);
				}
				return true;
			}
		}
		else
		{
			// no path information on new filename, so it's relative by default
			strcpy (Dest, NewPath);
			return true;
		}
	}
	else
	{
		// no relative path required, so copy the whole thing.
		strcpy (Dest, NewPath);
		return true;
	}

	// The string is not relative, and it's supposed to be.
	// Display an error and return false.
	CString ErrMsg;

	AfxFormatString2 (ErrMsg, IDS_RELATIVEPATH, NewPath, ProjectDir);
	AfxMessageBox (ErrMsg, MB_ICONEXCLAMATION | MB_OK);

	return false;
}


struct MakeHelp_CompileInfo
{
	AProject *Project;
	AOptions *Options;
	CWnd *Parent;
};

static bool MakeHelp_CompileActive = false;

static UINT MakeHelp_ThreadProc (void *pParam)
{
	MakeHelp_CompileInfo CompileParams;

	assert (MakeHelp_CompileActive == false);

	MakeHelp_CompileActive = true;

	// clear compile interrupt flag
	Make_SetInterruptFlag (GE_FALSE);
	// get compile params
	CompileParams = *((MakeHelp_CompileInfo *)pParam);
	// and do the compile
	const geBoolean rslt = Make_Actor(CompileParams.Project, CompileParams.Options, MakeHelp_Printf);
	
	// notify the app that we finished...
	CompileParams.Parent->PostMessage (WM_USER_COMPILE_DONE, MAKEHELP_PROCESSID, (LPARAM)rslt);

	MakeHelp_CompileActive = false;

	return 0;
}

// Start the compile...
// This function spawns a thread.
// It returns true if the thread started OK.
bool MakeHelp_StartCompile (AProject *Project, AOptions *Options, CWnd *Parent)
{
	CWinThread *Thread;
	// static...so there can only be one of these...
	static MakeHelp_CompileInfo CompileParams;

	assert (MakeHelp_CompileActive == false);

	CompileParams.Project = Project;
	CompileParams.Options = Options;
	CompileParams.Parent = Parent;

	Thread = AfxBeginThread (MakeHelp_ThreadProc, (void *)&CompileParams, THREAD_PRIORITY_NORMAL, 0, 0, NULL);

	return (Thread != NULL);
}

void MakeHelp_CancelCompile (void)
{
	// Request that compile be canceled
	Make_SetInterruptFlag (GE_TRUE);
}
