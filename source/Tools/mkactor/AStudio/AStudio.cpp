/****************************************************************************************/
/*  ASTUDIO.CPP																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio application main module.									*/
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

/*******************************************
  The main dialog module is PropSheet.cpp.
********************************************/


#include "stdafx.h"
#include "AStudio.h"
#include "PropSheet.h"
#include "FilePath.h"
#include "rcstring.h"
#include "ram.h"
#pragma warning (disable:4201)		// namless struct/union
#include <shlobj.h>
#pragma warning (default:4201)		// namless struct/union

/////////////////////////////////////////////////////////////////////////////
// CAStudioCommandLineInfo -- Command line parser
class CAStudioCommandLineInfo : public CCommandLineInfo
{
public:
	CAStudioCommandLineInfo ();
    virtual void ParseParam( LPCTSTR lpszParam, BOOL bFlag, BOOL bLast );

	bool ErrFlag;
	int nParams;
	CString InputFile;
};

CAStudioCommandLineInfo::CAStudioCommandLineInfo () :
	CCommandLineInfo(), InputFile(""), ErrFlag(false), nParams(0)
{
}

void CAStudioCommandLineInfo::ParseParam (LPCTSTR lpszParam, BOOL bFlag, BOOL /*bLast*/)
{
	++nParams;

	if (!bFlag)
	{
		if (nParams == 1)
		{
			InputFile = lpszParam;
		}
		else
		{
			ErrFlag = true;
		}
	}
	else
	{
		ErrFlag = true;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CAStudioApp

BEGIN_MESSAGE_MAP(CAStudioApp, CWinApp)
	//{{AFX_MSG_MAP(CAStudioApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
//	ON_COMMAND(ID_HELP, CWinApp::OnHelp)	// commented so property sheet
											// help button not displayed.
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAStudioApp construction

CAStudioApp::CAStudioApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAStudioApp object

CAStudioApp theApp;

// Get long path name from short path name.
// assumes that LongPath has enough characters to hold the generated name
static bool ShortPathToLongPath (const char *ShortPath, char *LongPath)
{
// Man, this is a seriously disturbed function.
// Win32 could use a GetLongFileName function (NT 5 has one...)
	LPSHELLFOLDER psfDesktop = NULL;

	WCHAR szShortPathNameW[MAX_PATH];

	// convert path name to wide and copy to local storage
	ULONG chEaten = 0;
	LPITEMIDLIST pidlShellItem = NULL;

	mbstowcs (szShortPathNameW, ShortPath, MAX_PATH);

	// Get desktop's shell folder interface
	HRESULT hr = SHGetDesktopFolder (&psfDesktop);

	// request an ID list (relative to the desktop) for the short pathname
	hr = psfDesktop->ParseDisplayName (NULL, NULL, szShortPathNameW, &chEaten, &pidlShellItem, NULL);
	psfDesktop->Release ();		// release desktop's IShellFolder

	if (FAILED (hr))
	{
		return false;
	}

	// got an ID list.  Convert it to a long pathname
	SHGetPathFromIDListA (pidlShellItem, LongPath);

	// Free the ID list allocated by ParseDisplayName
	LPMALLOC pMalloc = NULL;
	SHGetMalloc (&pMalloc);
	pMalloc->Free (pidlShellItem);
	pMalloc->Release ();

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// CAStudioApp initialization

BOOL CAStudioApp::InitInstance()
{
	Enable3dControlsStatic();	// Call this when linking to MFC statically

	AOptions *Options = NULL;	// options structure holds INI file options
	char IniFilePath[MAX_PATH] ;

	// Set up INI file path
	{
		char AppPath[MAX_PATH];
		
		::GetModuleFileName (NULL, AppPath, MAX_PATH);
		::FilePath_GetDriveAndDir (AppPath, AppPath);

		::FilePath_AppendName (AppPath, rcstring_Load (AfxGetInstanceHandle (), IDS_INIFILE_NAME), IniFilePath);

		// Set our INI file
		::free ((void *)m_pszProfileName);
		m_pszProfileName = ::_tcsdup (IniFilePath);

	}

	// Parse command line for filename
	CAStudioCommandLineInfo cmdInfo;
	ParseCommandLine (cmdInfo);

	AProject *pProject = NULL;

	// load program options
	Options = AOptions_CreateFromFile (IniFilePath);
	bool KeepGoing = true;

	if (Options == NULL)
	{
		// couldn't load options.  Man, we're REALLY out of memory...
		AfxMessageBox (IDS_OPTIONSERR);
		return FALSE;
	}

	char LoadProjectName[2*MAX_PATH] = "";

	// if filename specified on command line, then try to load it
	if ((cmdInfo.ErrFlag == false) && (cmdInfo.InputFile != ""))
	{
		char Ext[MAX_PATH];

		if (::ShortPathToLongPath (cmdInfo.InputFile, LoadProjectName) == false)
		{
			// ShortPathToLongPath failed for some reason.  Use short filename.
			strcpy (LoadProjectName, cmdInfo.InputFile);
		}

		// if no extension, then append .apj
		if (FilePath_GetExt (LoadProjectName, Ext) == GE_FALSE)
		{
			FilePath_SetExt (LoadProjectName, ".apj", LoadProjectName);
		}

		pProject = AProject_CreateFromFilename (LoadProjectName);
		if (pProject == NULL)
		{
			// If unable to load, then tell user and ask for Ok/Cancel
			// Ok will create new project and launch program.
			// Cancel will exit program.
			CString ErrMsg;

			AfxFormatString1 (ErrMsg, IDS_READERROR, LoadProjectName);
			if (AfxMessageBox (ErrMsg, MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
			{
				KeepGoing = false;
			}
		}
	}

	if (KeepGoing)
	{
		CAStudioPropSheet dlg (NULL, &pProject, Options, LoadProjectName);

		m_pMainWnd = &dlg;
		dlg.DoModal ();

		// all done.  Save options.
		AOptions_WriteToFile (Options, IniFilePath);
	}

	// Cleanup

	// Destroy options structure
	if (Options != NULL)
	{
		AOptions_Destroy (&Options);
	}

	// Destroy project structure
	if (pProject != NULL)
	{
		AProject_Destroy (&pProject);
	}
		
	geRam_ReportAllocations();
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
