/****************************************************************************************/
/*  PROPSHEET.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio main dialog.												*/
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
#include "resource.h"
#include "PropSheet.h"
#include <assert.h>
#include "rcstring.h"
#include "MyFileDlg.h"
#include "FilePath.h"
#include "MkUtil.h"
#include "MakeHelp.h"

#include <ShlObj.h>

#include "util.h"
#include "NewPrjDlg.h"
#include "make.h"


/////////////////////////////////////////////////////////////////////////////
// CAStudioPropSheet property sheet

CAStudioPropSheet::CAStudioPropSheet(CWnd* pWndParent, AProject **pProject, AOptions *pOptions, const char *ProjectFilename)
	: CPropertySheet(IDS_CAPTION, pWndParent),
	  m_hIcon(NULL), m_hAccel(NULL),
	  m_Modified(false), m_NewFile(true),
	  m_Filename(""), m_Project(*pProject), m_pProject(pProject), m_Options(pOptions),
	  m_MessagesVisible(false), m_FileLoaded(false), m_Compiling(false)
{
	// Remove help and apply buttons...
	// This is great.  Negative logic to remove one, and positive to remove the other.
	// And the help button is removed somewhere else entirely.
	// Although this doesn't seem to work for the help button.  Apparantly you have to ensure
	// that the main app doesn't handle the ID_HELP message.
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_hIcon = AfxGetApp()->LoadIcon (IDR_MAINFRAME);
	
	// load menu accelerators
	// The program can still run if accelerators aren't found, so don't check return.
	m_hAccel = ::LoadAccelerators (AfxGetInstanceHandle (), MAKEINTRESOURCE (IDR_ACCELERATOR1));

	AddPage (&m_LogoPage);

	if (*pProject != NULL)
	{
		m_NewFile = false;
		m_Filename = ProjectFilename;
		SetWorkingDirectory ();
	}
}


CAStudioPropSheet::~CAStudioPropSheet ()
{
}


void CAStudioPropSheet::ForcePageUpdates ()
{
	// make sure current page updates its data
	CPropertyPage *pPage = GetActivePage ();
	if ((pPage != &m_SettingsPage) && (pPage != &m_LogoPage))
	{
		CAStudioPropPage *pAStudioPage = reinterpret_cast<CAStudioPropPage *>(GetActivePage ());

		pAStudioPage->GetDialogData ();
	}
}

void CAStudioPropSheet::SetProjectPointer (AProject *Project)
{
	assert (m_pProject != NULL);

	// Kind of a hack.  As it happens, InitDialog has to call this function,
	// but m_Project is already set.  So if the passed Project parameter is
	// the same as the m_Project member, then don't delete the m_Project member.
	//
	// If m_Project and Project are not the same, then free the old project.
	if ((m_Project != NULL) && (m_Project != Project))
	{
		AProject_Destroy (&m_Project);
	}

	//
	m_Project = Project;
	*m_pProject = Project;

	// set the project pointer for each of the pages
	for (int iPage = 0; iPage < GetPageCount ()-2; ++iPage)  // count - 2 'cause settings and about are different
	{
		CAStudioPropPage *pPage = reinterpret_cast<CAStudioPropPage *>(GetPage (iPage));
		pPage->SetProjectPointer (m_Project);
	}
}



IMPLEMENT_DYNAMIC(CAStudioPropSheet, CPropertySheet)

BEGIN_MESSAGE_MAP(CAStudioPropSheet, CPropertySheet)
	//{{AFX_MSG_MAP(CModalShapePropSheet)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_FILENEW, OnFileNew)
	ON_COMMAND(ID_FILEOPEN, OnFileOpen)
	ON_COMMAND(ID_FILESAVE, OnFileSave)
	ON_COMMAND(ID_FILEEXIT, OnFileExit)
	ON_COMMAND(ID_PROJECTBUILD, OnProjectBuild)
	ON_COMMAND(ID_PROJECTBUILDALL, OnProjectBuildAll)
	ON_COMMAND(ID_PROJECTCLEAN, OnProjectClean)
	ON_COMMAND(ID_PROJECTACTORSUMMARY, OnProjectActorSummary)
	ON_COMMAND(ID_DUMMYESC, OnDummyEsc)
	ON_BN_CLICKED(IDC_MESSAGES, OnMessages)
	ON_UPDATE_COMMAND_UI(ID_FILENEW, OnUpdateFileNew)
	ON_UPDATE_COMMAND_UI(ID_FILEOPEN, OnUpdateFileOpen)
	ON_UPDATE_COMMAND_UI(ID_FILESAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_PROJECTBUILD, OnUpdateProjectBuild)
	ON_UPDATE_COMMAND_UI(ID_PROJECTBUILDALL, OnUpdateProjectBuildAll)
	ON_UPDATE_COMMAND_UI(ID_PROJECTCLEAN, OnUpdateProjectClean)
	ON_UPDATE_COMMAND_UI(ID_PROJECTACTORSUMMARY, OnUpdateProjectActorSummary)
	ON_MESSAGE (WM_USER_COMPILE_MESSAGE, OnCompileMessage)
	ON_MESSAGE (WM_USER_COMPILE_DONE, OnCompileDone)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CAStudioPropSheet::AddAButton
	(
	  int ButtonTextId,
	  int ButtonId,
	  CButton &TheButton,
	  const RECT &BtnRect
	)
{
	// Create the reset page button and position it
	TheButton.Create
	(
		rcstring_Load (AfxGetInstanceHandle (), ButtonTextId),
		BS_PUSHBUTTON,
		BtnRect,
		this,
		ButtonId
	);

	// Set the button's font so it's the same as the dialog's
	TheButton.SetFont (GetFont ());

	// Make the button visible and enabled
	TheButton.ShowWindow (TRUE);
	TheButton.EnableWindow (TRUE);
}

void CAStudioPropSheet::ReplaceAButton
	(
	  int ButtonTextId,
	  int ReplaceButtonId,
	  int ButtonId,
	  CButton &TheButton
	)
{
	// Create a button and position where ReplaceButtonId is...
	RECT WindowRect;
	CWnd *ReplaceBtn = GetDlgItem (ReplaceButtonId);

	ReplaceBtn->GetWindowRect (&WindowRect);

	POINT pt;

	pt.x = WindowRect.left;
	pt.y = WindowRect.top;
	ScreenToClient (&pt);

	WindowRect.right = pt.x + (WindowRect.right - WindowRect.left);
	WindowRect.bottom = pt.y + (WindowRect.bottom - WindowRect.top);
	WindowRect.left = pt.x;
	WindowRect.top = pt.y;

	AddAButton (ButtonTextId, ButtonId, TheButton, WindowRect);
}

BOOL CAStudioPropSheet::OnInitDialog()
{
	CPropertySheet::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// disable and hide OK and Cancel buttons...

	GetDlgItem (IDOK)->EnableWindow (FALSE);
	GetDlgItem (IDOK)->ShowWindow (FALSE);
	GetDlgItem (IDCANCEL)->EnableWindow (FALSE);
	GetDlgItem (IDCANCEL)->ShowWindow (FALSE);

	// Help button is disabled by commenting out the handling of ID_HELP
	// in the main program (AStudio.cpp).

	// Add controls to the dialog box...
	// Put the Messages button where the Cancel button is
	ReplaceAButton (IDS_MESSAGESSHOW, IDCANCEL, IDC_MESSAGES, m_MessagesBtn);
	// Add build button
	ReplaceAButton (IDS_BUILD, IDOK, ID_PROJECTBUILD, m_BuildBtn);
	// build button is initially disabled.
	m_BuildBtn.EnableWindow (FALSE);
	/*
	  The dialog is now in its default size and position.
	  Here we determine the size of the dialog required to hold the messages
	  window when the user presses the Show Messages button.  We place the
	  messages window in that area, but since the dialog is too small to contain
	  that area, the messages window will not show.

	  When the Show Messages button is pressed, we resize the dialog and,
	  like magic, the messages window appears.
	*/
	{
		RECT DialogRect;

		GetWindowRect (&DialogRect);

		// save normal height/width
		m_Width = DialogRect.right - DialogRect.left;
		m_NormalHeight = DialogRect.bottom - DialogRect.top;

		// Add 3/4 of the height for the new height...
		m_ExpandedHeight = (3*m_NormalHeight)/4;

		// create the messages window and place it in the expanded section.
		RECT ClientRect;

		GetClientRect (&ClientRect);
		const int ClientWidth = ClientRect.right - ClientRect.left;
		const int ClientHeight = ClientRect.bottom - ClientRect.top;

		RECT EditRect;

		EditRect.left	= 7;
		EditRect.top	= 7 + ClientHeight;
		EditRect.right	= EditRect.left + (ClientWidth - 14);
		EditRect.bottom	= EditRect.top + (m_ExpandedHeight - 14);

		BOOL rslt = m_EditMessages.Create 
		(
		  (WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | WS_VSCROLL | WS_HSCROLL |
		   ES_LEFT | ES_AUTOHSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY),
		  EditRect, this, IDC_MESSAGES
		);

		if (rslt == FALSE)
		{
			#pragma message ("Need to handle creation err here")
			// really don't know what to do about this...
			// probably should destroy myself
			assert (0);
		}
		// Set the control's font so it's the same as the dialog's
		m_EditMessages.SetFont (GetFont ());
	}

	SetupTitle ();

	if (m_Project != NULL)
	{
		AddPropertyPages ();
		SetProjectPointer (m_Project);
		SetActivePage (0);
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// Create the property sheet.
// After default creation, add a minimize box and the menu.
int CAStudioPropSheet::OnCreate (LPCREATESTRUCT lpCreateStruct) 
{
	if (CPropertySheet::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	// add  minimize box
	const int Style = ::GetWindowLong (m_hWnd, GWL_STYLE);
	::SetWindowLong (m_hWnd, GWL_STYLE, Style | WS_MINIMIZEBOX);

	// set up the menu
	CMenu *pmenu = new CMenu;
	
	pmenu->LoadMenu(IDR_MENU1);
	SetMenu(pmenu);
	pmenu->Detach();

	delete pmenu;

	// Adjust dialog size to include menu
	RECT WindowRect;

	GetWindowRect (&WindowRect);
	const int MenuHeight = ::GetSystemMetrics (SM_CYMENU);
	SetWindowPos 
	(
		NULL,		// no z order change
		WindowRect.left, WindowRect.top-MenuHeight/2,
		(WindowRect.right - WindowRect.left), (WindowRect.bottom - WindowRect.top + MenuHeight),
		SWP_NOZORDER
	);

	return 0;
}


// Handle any final cleanup
void CAStudioPropSheet::OnDestroy()
{
	WinHelp(0L, HELP_QUIT);	// shutdown WinHelp
	CPropertySheet::OnDestroy();
}

// Handle OnSysCommand so we can prompt the user to save a modified
// file before the application is closed.
void CAStudioPropSheet::OnSysCommand( UINT nID, LPARAM lParam )
{
	if (nID == SC_CLOSE)
	{
		if (!CanClose ())
		{
			return;
		}
	}
	CPropertySheet::OnSysCommand (nID, lParam);
}


// File has been modified.  Prompt user to save before continuing.
int CAStudioPropSheet::PromptForSave ()
{
	CString Filename;
	CString Message;

	// build prompt from current filename
	if (m_NewFile)
	{
		Filename.LoadString (IDS_UNTITLEDFILE);
	}
	else
	{
		Filename = m_Filename;
	}

	AfxFormatString1 (Message, IDS_MODIFIED, Filename);

	// display in message box and get response
	int rslt = AfxMessageBox (Message, MB_ICONQUESTION | MB_YESNOCANCEL);

	// only continue if user wants to save file
	if (rslt != IDYES)
	{
		return rslt;
	}

	// Attempt to save file.  If save fails, then return CANCEL to prevent
	// application from closing.
	if (!SaveFile ())
	{
		return IDCANCEL;
	}

	return rslt;
}

bool CAStudioPropSheet::SaveFile ()
{
	CString OutputFilename;

	if (m_NewFile)
	{
		char Name[MAX_PATH];

		*Name = '\0';
		FilePath_GetName (AProject_GetOutputFilename (m_Project), Name);
		OutputFilename = Name;
		if (!PromptForSaveFileName (OutputFilename))
		{
			return false;
		}
	}
	else
	{
		OutputFilename = m_Filename;
	}
	return SaveFileAs (OutputFilename);
}


bool CAStudioPropSheet::PromptForSaveFileName (CString &OutputFilename)
{
	CFileDialog *FileDlg = MyFileDialog_Create
	  (
	    FALSE, IDS_SAVEPROJECTPROMPT, IDS_ASPFILEEXT, IDS_ASPFILEFILTER,
		OutputFilename, OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, this
	  );

	int rslt = FileDlg->DoModal ();

	if (rslt == IDOK)
	{
		OutputFilename = FileDlg->GetPathName ();
	}

	delete FileDlg;

	return (rslt == IDOK);
}

/*
  SaveFileAs writes the project to the specified filename.
  Returns true if file successfully written.
  On error, displays an error message and returns false.

  On success, the m_Filename member variable is updated with the
  output filename, and the title is updated to reflect the new filename.
*/
bool CAStudioPropSheet::SaveFileAs (const CString &OutputFilename)
{
	ForcePageUpdates ();

	// output the file.
	if (WriteTheDamnFileAlready (OutputFilename))
	{
		// if successful writing the file, then set the new file name
		// and update the title accordingly.
		m_Filename = OutputFilename;
		SetupTitle ();
		return true;
	}

	// Error message here...
	{
		CString ErrorString;

		AfxFormatString1 (ErrorString, IDS_WRITEERROR, OutputFilename);
		AfxMessageBox (ErrorString, MB_ICONEXCLAMATION | MB_OK);
	}
	return false;
}


bool CAStudioPropSheet::WriteTheDamnFileAlready (const CString &Filename)
// Function actually opens the file and writes the data to it.
// Returns true if successful.  false if any error occurred.
//  This function does no error output--it's up to the calling function.
{
	bool rslt = (AProject_WriteToFilename (m_Project, Filename) == GE_FALSE) ? false : true;

	if (rslt)
	{
		SetModifiedFlag (false);
		m_NewFile = false;
	}

	return rslt;
}


bool CAStudioPropSheet::CanClose ()
{
	// Dialog is closing...
	// Need to prompt user before exiting if file has been modified.
	if (WasModified ())
	{
		if (PromptForSave () == IDCANCEL)
		{
			return false;
		}
	}
	return true;
}

void CAStudioPropSheet::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CPropertySheet::OnPaint();
	}
}

// Return true if any part of the project has been modified
bool	CAStudioPropSheet::WasModified ()
{
	for (int iPage = 0; (!m_Modified) && (iPage < GetPageCount ()-2); ++iPage)
	{
		// check each of the pages to see if it was modified
		CAStudioPropPage *pPage = reinterpret_cast<CAStudioPropPage *>(GetPage (iPage));
		m_Modified = pPage->WasModified ();
	}

	return m_Modified;
}

void	CAStudioPropSheet::SetModifiedFlag (bool flag)
{
	m_Modified = flag;
	if (!m_Modified)
	{
		// if setting to not modified, then clear all dialogs' modified flags
		for (int iPage = 0; iPage < GetPageCount ()-2; ++iPage)
		{
			CAStudioPropPage *pPage = reinterpret_cast<CAStudioPropPage *>(GetPage (iPage));
			pPage->SetModifiedFlag (false);
		}
	}
}

// Setup window title.  Title is caption + filename.
// Filename is "untitled" if it's a new file.
void	CAStudioPropSheet::SetupTitle ()
{
	CString ProjectFilename;
	CString Title;

	// format string with filename (if it exists)
	if (m_NewFile)
	{
		ProjectFilename.LoadString (IDS_UNTITLED);
	}
	else
	{
		ProjectFilename = m_Filename;
	}

	AfxFormatString1 (Title, IDS_CAPTION, ProjectFilename);
	SetTitle (Title);
}


// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAStudioPropSheet::OnQueryDragIcon()
{
	return (HCURSOR)m_hIcon;
}


void CAStudioPropSheet::AddPropertyPages ()
{
	// Make sure that the property sheet pages are displayed.
	if (!m_FileLoaded)
	{
		// remove the logo page...
		RemovePage (&m_LogoPage);

		// add the property pages
		AddPage (&m_TargetPage);
		AddPage (&m_BodyPage);
		AddPage (&m_MaterialsPage);
		AddPage (&m_MotionsPage);
		AddPage (&m_PathsPage);

		// set options for the Settings dialog
		m_SettingsPage.SetOptionsPointer (m_Options);
		AddPage (&m_SettingsPage);

		// and add the logo page again...
		AddPage (&m_LogoPage);
		// the file is loaded...

		m_FileLoaded = true;
		// enable the build button
		m_BuildBtn.EnableWindow (TRUE);

		// set options pointer for everybody else
		for (int iPage = 0; iPage < GetPageCount ()-2; ++iPage)  // count - 2 'cause settings and about are different
		{
			CAStudioPropPage *pPage = reinterpret_cast<CAStudioPropPage *>(GetPage (iPage));
			pPage->SetOptionsPointer (m_Options);
		}
	}
}

// Create a new file.
void CAStudioPropSheet::OnFileNew ()
{
	CNewPrjDlg Dlg;

	ForcePageUpdates ();

	// force relative paths by default
	Dlg.m_UseProjectDir = TRUE;

	// Get new project information
	if (Dlg.DoModal () != IDOK)
	{
		return;
	}

	// Prompt user for save before closing a modified file.
	if (!CanClose ())
	{
		return;
	}

	// Make sure the dialog's filename is fully qualified
	char ProjectName[MAX_PATH];
	char Ext[MAX_PATH];
	char NameOnly[MAX_PATH];

	GetFullPathName (Dlg.m_ProjectName, MAX_PATH, ProjectName, NULL);

	// Append the project file extension if there isn't an extension on the filename
	if (FilePath_GetExt (ProjectName, Ext) == GE_FALSE)
	{
		FilePath_SetExt (ProjectName, ".apj", ProjectName);
	}

	// Get just the base filename for the default body name
	// If no name, give it the Untitled string
	if (FilePath_GetName (ProjectName, NameOnly) == GE_FALSE)
	{
		strcpy (NameOnly, rcstring_Load (AfxGetResourceHandle (), IDS_UNTITLED));
	}

	// create the new project
	AProject *NewProject = AProject_Create (NameOnly);

	if (NewProject == NULL)
	{
		// Couldn't create the project.  Display error message and exit.
		AfxMessageBox (IDS_CREATEERROR, MB_ICONEXCLAMATION);
		return;
	}

	// Set the force relative flag accordingly
	AProject_SetForceRelativePaths (NewProject, (Dlg.m_UseProjectDir == TRUE) ? GE_TRUE : GE_FALSE);
	// Make sure property pages are there...
	AddPropertyPages ();
	// set new project pointer and update the display
	SetProjectPointer (NewProject);
	SetActivePage (0);
	// file hasn't been modified...
	SetModifiedFlag (true);
	m_NewFile = false;
	m_Filename = ProjectName;
	SetWorkingDirectory ();
	SetupTitle ();
}

// Open an existing file
void CAStudioPropSheet::OnFileOpen ()
{
	ForcePageUpdates ();

	// Prompt user for save before closing a modified file
	if (CanClose ())
	{
		CFileDialog *FileDlg = MyFileDialog_Create 
			(
			  TRUE, IDS_OPENPROJECTPROMPT, IDS_ASPFILEEXT, IDS_ASPFILEFILTER, 
			  NULL, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this
			);

		if (FileDlg->DoModal () == IDOK)
		{
			const CString InputFilename = FileDlg->GetPathName ();

			// Load an existing project and update current dialog page
			AProject *NewProject = AProject_CreateFromFilename (InputFilename);

			// If unable to load, show error message and exit
			if (NewProject == NULL)
			{
				CString ErrorString;

				AfxFormatString1 (ErrorString, IDS_READERROR, InputFilename);
				AfxMessageBox (ErrorString, MB_ICONEXCLAMATION | MB_OK);
				return;
			}

			// Make sure property pages are there...
			AddPropertyPages ();
			// set new project pointer and update the display
			SetProjectPointer (NewProject);
			SetActivePage (0);
			// file hasn't been modified
			SetModifiedFlag (false);
			m_NewFile = false;
			m_Filename = InputFilename;
			SetWorkingDirectory ();
			SetupTitle ();
		}
		delete FileDlg;
	}
}

void CAStudioPropSheet::OnFileSave ()
{
	ForcePageUpdates ();

	SaveFile ();
}

void CAStudioPropSheet::OnFileExit ()
{
	ForcePageUpdates ();

	// This should cancel the dialog, but first prompt user for save...
	EndDialog (IDCANCEL);
}

void CAStudioPropSheet::OnProjectBuild ()
{
	if (m_Compiling)
	{
		// cancel the compile
		MakeHelp_CancelCompile ();
		MakeHelp_Printf ("%s\n", rcstring_Load (AfxGetResourceHandle (), IDS_CANCELREQUESTED));
	}
	else
	{
		ForcePageUpdates ();

		// Point the printf function at us
		MakeHelp_SetMessagesWindow (this);

		// make sure the messages window is visible
		EnableMessagesWindow (true);

		// Clear messages window
		m_EditMessages.SetWindowText ("");

		// and start compile...
		const geBoolean rslt = MakeHelp_StartCompile (m_Project, m_Options, this);

		// If compile started OK, then lock out controls.
		// If it failed, then display message
		if (rslt == GE_FALSE)
		{
			AfxMessageBox (IDS_CANTSTARTCOMPILE, MB_ICONEXCLAMATION);
		}
		else
		{
			CString Msg;

			AfxFormatString1 (Msg, IDS_BUILDING, m_Filename);
			MakeHelp_Printf ("%s\n", Msg);
			SetCompilingStatus (true);
		}
	}
}

void CAStudioPropSheet::OnProjectBuildAll ()
{
	if (m_Compiling)
	{
		return;
	}

	ForcePageUpdates ();

	// point printf function at us
	MakeHelp_SetMessagesWindow (this);
	// enable it
	EnableMessagesWindow (true);
	// and clear it
	m_EditMessages.SetWindowText ("");

	// clean
	if (Make_Clean (m_Project, m_Options, MakeHelp_Printf) == GE_FALSE)
	{
		AfxMessageBox (IDS_COMPILEFAILED, MB_OK);
		return;
	}

	// and start compile...
	const geBoolean rslt = MakeHelp_StartCompile (m_Project, m_Options, this);

	// If compile started OK, then lock out controls.
	// If it failed, then display message
	if (rslt == GE_FALSE)
	{
		AfxMessageBox (IDS_CANTSTARTCOMPILE, MB_ICONEXCLAMATION);
	}
	else
	{
		SetCompilingStatus (true);
	}
}

void CAStudioPropSheet::OnProjectClean ()
{
	// point printf function at us
	MakeHelp_SetMessagesWindow (this);
	// enable it
	EnableMessagesWindow (true);
	// and clear it
	m_EditMessages.SetWindowText ("");

	// Display "cleaning" message
	CString Msg;
	
	AfxFormatString1 (Msg, IDS_CLEANING, m_Filename);
	MakeHelp_Printf ("%s\n", Msg);

	if (Make_Clean (m_Project, m_Options, MakeHelp_Printf) == GE_FALSE)
	{
		AfxMessageBox (IDS_COMPILEFAILED, MB_OK);
	}
}

void CAStudioPropSheet::OnProjectActorSummary ()
{
	ForcePageUpdates ();

	MakeHelp_SetMessagesWindow (this);
	EnableMessagesWindow (true);
	m_EditMessages.SetWindowText ("");
	if (Make_ActorSummary (m_Project, MakeHelp_Printf) == GE_FALSE)
	{
		AfxMessageBox (IDS_SUMMARYFAILED, MB_OK);
	}
}


void CAStudioPropSheet::OnDummyEsc ()
{
	// Do nothing.
	// This function just prevents ESC from closing the dialog box.
	// The Escape key is mapped in the accelerator table to generate the ID_DUMMYESC
	// message, which is then dispatched here by PreTranslateMessage.
}


// PreTranslateMessage lets me trap menu accelerators.
BOOL CAStudioPropSheet::PreTranslateMessage(MSG* pMsg) 
{
	if (!(m_hAccel && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg)))
	{
		return CPropertySheet::PreTranslateMessage (pMsg);
	}
	else
	{
		return TRUE;	
	}
}

// If messages button is pressed, toggle message visible state,
// show/hide messages box, and change button text
void CAStudioPropSheet::OnMessages ()
{
	EnableMessagesWindow (!m_MessagesVisible);
}

void CAStudioPropSheet::EnableMessagesWindow (bool Enabled)
{
	if (m_MessagesVisible != Enabled)
	{
		// if changing state then do it...
		int Height;

		m_MessagesVisible = Enabled;
		if (m_MessagesVisible)
		{
			
			// show messages edit box
			m_MessagesBtn.SetWindowText (rcstring_Load (AfxGetResourceHandle (), IDS_MESSAGESHIDE));
			Height = m_NormalHeight + m_ExpandedHeight;
		}
		else
		{
			// hide messages edit box
			m_MessagesBtn.SetWindowText (rcstring_Load (AfxGetResourceHandle (), IDS_MESSAGESSHOW));
			Height = m_NormalHeight;
		}

		// change window's size to show/hide messages window
		SetWindowPos (NULL, 0, 0, m_Width, Height, SWP_NOMOVE | SWP_NOZORDER);
	}
}

void CAStudioPropSheet::SetWorkingDirectory ()
{
	char pszPath[MAX_PATH];

	FilePath_GetDriveAndDir (m_Filename, pszPath);
	::SetCurrentDirectory (pszPath);
}

void CAStudioPropSheet::SetCompilingStatus (bool CompileFlag)
{
	m_Compiling = CompileFlag;
	// inform all views that we're compiling...
	m_TargetPage.SetCompileStatus (m_Compiling);
	m_BodyPage.SetCompileStatus (m_Compiling);
	m_MaterialsPage.SetCompileStatus (m_Compiling);
	m_MotionsPage.SetCompileStatus (m_Compiling);
	m_PathsPage.SetCompileStatus (m_Compiling);
	m_SettingsPage.SetCompileStatus (m_Compiling);

	if (m_Compiling)
	{
		m_BuildBtn.SetWindowText (rcstring_Load (AfxGetResourceHandle(), IDS_STOPBUILD));
	}
	else
	{
		m_BuildBtn.SetWindowText (rcstring_Load (AfxGetResourceHandle(), IDS_BUILD));
	}
}

void CAStudioPropSheet::OnCompileMessage (UINT /*wParam*/, LONG lParam)
{
	char *MessageText = (char *)lParam;

	if (MessageText != NULL)
	{
		// set current selection and put message into listbox
		int Length = m_EditMessages.GetWindowTextLength ();

		// Don't know how to set the caret pos except by selecting.
		// So select the last character.  Then unselect everything and replace the
		// current selection.  Hey, it works.
		m_EditMessages.SetSel (Length-1, Length, FALSE);
		m_EditMessages.SetSel (-1, Length, FALSE);
		m_EditMessages.ReplaceSel (MessageText, FALSE);

		// free the memory we were sent
		CoTaskMemFree (MessageText);
	}
}

void CAStudioPropSheet::OnCompileDone (UINT wParam, LONG lParam)
{
	if (wParam != MAKEHELP_PROCESSID)
	{
		// somebody passed us a bogus message...
		return;
	}

	SetCompilingStatus (false);
	if (((geBoolean)lParam) == GE_FALSE)
	{
		// compile failed
		AfxMessageBox (IDS_COMPILEFAILED, MB_OK);
	}
	else
	{
		// compile successful
		AfxMessageBox (IDS_COMPILESUCCESS, MB_OK);
	}
}

// Command UI updating
void CAStudioPropSheet::OnUpdateFileNew (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (!m_Compiling);
}

void CAStudioPropSheet::OnUpdateFileOpen (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (!m_Compiling);
}

void CAStudioPropSheet::OnUpdateFileSave (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (m_FileLoaded && !m_Compiling);
}

void CAStudioPropSheet::OnUpdateProjectBuild (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (m_FileLoaded && !m_Compiling);
}

void CAStudioPropSheet::OnUpdateProjectBuildAll (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (m_FileLoaded && !m_Compiling);
}

void CAStudioPropSheet::OnUpdateProjectClean (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (m_FileLoaded && !m_Compiling);
}

void CAStudioPropSheet::OnUpdateProjectActorSummary (CCmdUI* pCmdUI)
{
	pCmdUI->Enable (m_FileLoaded && !m_Compiling);
}
