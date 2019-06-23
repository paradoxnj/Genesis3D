/****************************************************************************************/
/*  MOTIONSDLG.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor studio motions dialog handler.									*/
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
#include "MotionsDlg.h"
#include <assert.h>
#include "MyFileDlg.h"
#include "TextInputDlg.h"
#include "FilePath.h"
#include "MakeHelp.h"
#include "motion.h"

/////////////////////////////////////////////////////////////////////////////
// CMotionsDlg property page

IMPLEMENT_DYNCREATE(CMotionsDlg, CAStudioPropPage)

CMotionsDlg::CMotionsDlg() : CAStudioPropPage(CMotionsDlg::IDD),
	m_FilenameChanged(false)
{
	//{{AFX_DATA_INIT(CMotionsDlg)
	m_MotionFilename = _T("");
	m_MotionFormat = -1;
	m_OptLevel = 0;
	m_Optimize = FALSE;
	m_BoneName = _T("");
	//}}AFX_DATA_INIT
}

CMotionsDlg::~CMotionsDlg()
{
}

void CMotionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CAStudioPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMotionsDlg)
	DDX_Control(pDX, IDC_EDITBONE, m_EditBone);
	DDX_Control(pDX, IDC_SPINOPTIMIZE, m_SpinOptLevel);
	DDX_Control(pDX, IDC_EDITOPTIMIZE, m_EditOptLevel);
	DDX_Control(pDX, IDC_EDITMOTION, m_EditMotionFilename);
	DDX_Control(pDX, IDC_BROWSEMOTION, m_BrowseMotion);
	DDX_Control(pDX, IDC_MOTIONDEFAULT, m_MotionDefault);
	DDX_Control(pDX, IDC_DELETEMOTION, m_DeleteMotion);
	DDX_Control(pDX, IDC_MOTIONSLIST, m_MotionsList);
	DDX_Text(pDX, IDC_EDITMOTION, m_MotionFilename);
	DDX_Radio(pDX, IDC_MOTIONMAX, m_MotionFormat);
	DDX_Text(pDX, IDC_EDITOPTIMIZE, m_OptLevel);
	DDV_MinMaxInt(pDX, m_OptLevel, 0, 9);
	DDX_Check(pDX, IDC_OPTIMIZE, m_Optimize);
	DDX_Text(pDX, IDC_EDITBONE, m_BoneName);
	//}}AFX_DATA_MAP
}

int CMotionsDlg::GetCurrentMotionIndex ()
{
	int LbItem = m_MotionsList.GetCurSel ();
	if (LbItem == LB_ERR)
	{
		m_CurrentIndex = -1;
	}
	else
	{
		m_CurrentIndex = (int)m_MotionsList.GetItemData (LbItem);
	}
	return m_CurrentIndex;
}

void CMotionsDlg::EnableControls ()
{
	const int Item = m_MotionsList.GetCurSel ();
	const bool GoodItem = (Item != LB_ERR);

	// most are disabled unless we have a good item and we're not compiling
	m_DeleteMotion.EnableWindow (GoodItem && !m_Compiling);
	m_MotionDefault.EnableWindow (GoodItem && !m_Compiling);
	m_EditMotionFilename.EnableWindow (GoodItem && !m_Compiling);
	m_BrowseMotion.EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_MOTIONMAX)->EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_MOTIONKEY)->EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_MOTIONMOT)->EnableWindow (GoodItem && !m_Compiling);
	// Actor button not yet supported...
//	GetDlgItem (IDC_MOTIONACT)->EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_OPTIMIZE)->EnableWindow (GoodItem && !m_Compiling);

	m_EditOptLevel.EnableWindow (GoodItem && !m_Compiling);
	m_SpinOptLevel.EnableWindow (GoodItem && !m_Compiling);

	m_EditBone.EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_RENAMEMOTION)->EnableWindow (GoodItem && !m_Compiling);

	// Add available whenever we're not compiling
	GetDlgItem (IDC_ADDMOTION)->EnableWindow (!m_Compiling);

	// Enable/disable dropped files depending on compile flag
	DragAcceptFiles (!m_Compiling);
}

void CMotionsDlg::FillListBox ()
{
	int Count = AProject_GetMotionsCount (m_Project);

	// file listbox with motion names
	m_MotionsList.ResetContent ();
	for (int iMotion = 0; iMotion < Count; ++iMotion)
	{
		// Get motion name and add to listbox
		const char *Name	= AProject_GetMotionName (m_Project, iMotion);
		const int Index		= m_MotionsList.AddString (Name);

		// if successfully added to the listbox, then add its index
		// to the listbox data.
		if ((Index != LB_ERR) && (Index != LB_ERRSPACE))
		{
			m_MotionsList.SetItemData (Index, iMotion);
		}
	}
}

void CMotionsDlg::SetupCurrentItem (int Item)
{
	// Set the dialog data for the currently selected motion
	if (m_MotionsList.SetCurSel (Item) != LB_ERR)
	{
		const int Index = (int)m_MotionsList.GetItemData (Item);
		const char *Filename = AProject_GetMotionFilename (m_Project, Index);
		const ApjMotionFormat Fmt = AProject_GetMotionFormat (m_Project, Index);
		const int OptLevel = AProject_GetMotionOptimizationLevel (m_Project, Index);
		const char *BoneName = AProject_GetMotionBone (m_Project, Index);
		const geBoolean OptFlag = AProject_GetMotionOptimizationFlag (m_Project, Index);

		m_MotionFilename = Filename;
		switch (Fmt)
		{
			case ApjMotion_Max : m_MotionFormat = 0; break;
			case ApjMotion_Key : m_MotionFormat = 1; break;
			case ApjMotion_Mot : m_MotionFormat = 2; break;
#pragma message ("Motions from actors not yet supported")
//			case ApjMotion_Act : m_MotionFormat = 3; break;
			default :
				assert (0);
		}

		m_Optimize = OptFlag;
		m_OptLevel = OptLevel;
		m_BoneName = BoneName;
	}
	GetCurrentMotionIndex ();
	m_FilenameChanged = false;
	EnableControls ();
}

BEGIN_MESSAGE_MAP(CMotionsDlg, CAStudioPropPage)
	//{{AFX_MSG_MAP(CMotionsDlg)
	ON_LBN_SELCHANGE(IDC_MOTIONSLIST, OnSelchangeMotionslist)
	ON_BN_CLICKED(IDC_ADDMOTION, OnAddmotion)
	ON_BN_CLICKED(IDC_DELETEMOTION, OnDeletemotion)
	ON_BN_CLICKED(IDC_MOTIONDEFAULT, OnMotiondefault)
	ON_EN_CHANGE(IDC_EDITMOTION, OnChangeEditmotion)
	ON_BN_CLICKED(IDC_BROWSEMOTION, OnBrowsemotion)
	ON_BN_CLICKED(IDC_MOTIONACT, OnMotionType)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINOPTIMIZE, OnDeltaposSpinoptimize)
	ON_BN_CLICKED(IDC_OPTIMIZE, OnOptimize)
	ON_EN_KILLFOCUS(IDC_EDITMOTION, OnKillfocusEditmotion)
	ON_WM_DROPFILES()
	ON_EN_CHANGE(IDC_EDITBONE, OnChangeEditbone)
	ON_BN_CLICKED(IDC_MOTIONKEY, OnMotionType)
	ON_BN_CLICKED(IDC_MOTIONMAX, OnMotionType)
	ON_BN_CLICKED(IDC_MOTIONMOT, OnMotionType)
	ON_BN_CLICKED(IDC_RENAMEMOTION, OnRenamemotion)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMotionsDlg message handlers

BOOL CMotionsDlg::OnInitDialog() 
{
	CAStudioPropPage::OnInitDialog();

	m_SpinOptLevel.SetRange (0, 9);

	FillListBox ();
	SetupCurrentItem (0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMotionsDlg::OnSetActive ()
{
	FillListBox ();
	SetupCurrentItem (0);
	UpdateData (FALSE);
	return TRUE;
}

static ApjMotionFormat MotionFormatFromInt (int Fmt)
{
	static const ApjMotionFormat MotionFormats[3] = {ApjMotion_Max, ApjMotion_Key, ApjMotion_Mot/*, ApjMotion_Act*/};
	
	assert ((Fmt >= 0) && (Fmt < 3));

	return MotionFormats[Fmt];
}

static int IndexFromMotionFormat (ApjMotionFormat Fmt)
{
	switch (Fmt)
	{
		case ApjMotion_Max : return 0;
		case ApjMotion_Key : return 1;
		case ApjMotion_Mot : return 2;
//		case ApjMotion_Act : return 3;
		default :
			assert (0);		// can't happen??
	}
	return -1;	// keeps the compiler happy
}

void CMotionsDlg::SetMotionFromDialog ()
{
	if (m_Compiling)
	{
		return;
	}
	if (m_CurrentIndex != -1)
	{
		if (m_FilenameChanged)
		{
			SetMotionTypeFromFilename ();
			m_FilenameChanged = false;
		}
		AProject_SetMotionFilename (m_Project, m_CurrentIndex, m_MotionFilename);
		AProject_SetMotionFormat (m_Project, m_CurrentIndex, MotionFormatFromInt (m_MotionFormat));
		AProject_SetMotionOptimizationLevel (m_Project, m_CurrentIndex, m_OptLevel);
		AProject_SetMotionOptimizationFlag (m_Project, m_CurrentIndex, m_Optimize);
		AProject_SetMotionBone (m_Project, m_CurrentIndex, m_BoneName);
	}
}


void CMotionsDlg::SetMotionTypeFromFilename ()
{
	ApjMotionFormat Fmt;

	Fmt = AProject_GetMotionFormatFromFilename (m_MotionFilename);
	if (Fmt != ApjMotion_Invalid)
	{
		m_MotionFormat = IndexFromMotionFormat (Fmt);
	}
}

BOOL CMotionsDlg::OnKillActive ()
{
	if (m_Compiling)
	{
		return TRUE;
	}
	char OldMotionName[MAX_PATH];
	strcpy (OldMotionName, m_MotionFilename);

	UpdateData (TRUE);
	if (m_FilenameChanged)
	{
		m_FilenameChanged = false;
		if (!MakeHelp_GetRelativePath (m_Project, m_MotionFilename, OldMotionName))
		{
			m_MotionFilename = OldMotionName;
			UpdateData (FALSE);
			return FALSE;
		}
	}

	GetCurrentMotionIndex ();
	SetMotionFromDialog ();
	return TRUE;
}

void CMotionsDlg::GetDialogData ()
{
	UpdateData (TRUE);
	SetMotionFromDialog ();
}

void CMotionsDlg::SetProjectPointer (AProject *Project)
{
	CAStudioPropPage::SetProjectPointer (Project);

	if (IsWindow (m_hWnd))
	{
		OnSetActive ();
	}
}

void CMotionsDlg::SetCompileStatus (bool Status)
{
	CAStudioPropPage::SetCompileStatus (Status);

	if (IsWindow (m_hWnd))
	{
		EnableControls ();
	}
}

void CMotionsDlg::OnSelchangeMotionslist() 
{
	char OldMotionName[MAX_PATH];
	strcpy (OldMotionName, m_MotionFilename);

	UpdateData (TRUE);
	if (m_FilenameChanged)
	{
		m_FilenameChanged = false;
		if (!MakeHelp_GetRelativePath (m_Project, m_MotionFilename, OldMotionName))
		{
			m_MotionFilename = OldMotionName;
		}
	}
	SetMotionFromDialog ();

	int CurSel = m_MotionsList.GetCurSel ();

	SetupCurrentItem (CurSel);

	UpdateData (FALSE);
}

void CMotionsDlg::OnAddmotion() 
{
	// Prompt for motion name
	// Make sure it's not a duplicate of one that already exists
	// Insert it into the project with default info
	// Update listbox to include this item and focus this item
	CTextInputDlg Dlg (IDS_MOTIONPROMPT, "", this);
	
	if (Dlg.DoModal () == IDOK)
	{
		// Update current item
		UpdateData (TRUE);
		SetMotionFromDialog ();

		CString MotionName = Dlg.m_Text;

		// see if this motion already exists in the list
		int Index = AProject_GetMotionIndex (m_Project, MotionName);
		if (Index == -1)
		{
			const geBoolean OptFlag = AOptions_GetMotionOptimizationFlag (m_Options);
			int OptLevel = AOptions_GetMotionOptimizationLevel (m_Options);

			// Insert motion into list
			if (!AProject_AddMotion (m_Project, MotionName, "", ApjMotion_Max, OptFlag, OptLevel, "", &Index))
			{
				// couldn't add it
				AfxMessageBox (IDS_ERRORADDMOTION);
				return;
			}
			// And add to listbox
			int LbItem = m_MotionsList.AddString (MotionName);
			if ((LbItem == LB_ERR) || (LbItem == LB_ERRSPACE))
			{
				// couldn't add it to listbox, so remove it
				AProject_RemoveMotion (m_Project, Index);
				AfxMessageBox (IDS_ERRORADDMOTION);
			}
			else
			{
				// Added it to the listbox, so update item data
				m_MotionsList.SetItemData (LbItem, Index);
				SetupCurrentItem (LbItem);
				UpdateData (FALSE);
				SetModifiedFlag (true);
			}
		}
		else
		{
			// Motion exists in the project.  Show it in the listbox.
			int LbItem = m_MotionsList.FindStringExact (-1, MotionName);

			if (LbItem != LB_ERR)
			{
				// Found it so focus it
				SetupCurrentItem (LbItem);
				UpdateData (FALSE);
			}
			else
			{
				// wasn't found in listbox (don't know how this could happen)
				assert (0);
			}
		}
		GetCurrentMotionIndex ();
	}
}

void CMotionsDlg::OnDeletemotion() 
{
	// remove a motion from the project and from the listbox
	// This will require us to re-initialize the listbox...
	int LbItem = m_MotionsList.GetCurSel ();
	if (LbItem != LB_ERR)
	{
		int Index = (int)m_MotionsList.GetItemData (LbItem);
		// remove it from the project
		if (AProject_RemoveMotion (m_Project, Index) == GE_FALSE)
		{
			AfxMessageBox (IDS_ERRORDELMOTION);
		}
		else
		{
			// Refill the listbox 'cause we've shifted motion entries
			FillListBox ();
			// and set the new current item
			if (LbItem >= m_MotionsList.GetCount ())
			{
				--LbItem;
			}
			SetupCurrentItem (LbItem);
			UpdateData (FALSE);
		}
	}
}

void CMotionsDlg::OnMotiondefault() 
{
	// Apply defaults to all motion fields
	m_Optimize = GE_FALSE;
	m_OptLevel = 0;
	m_MotionFilename = "";
	m_MotionFormat = 0;
	m_BoneName = "";

	UpdateData (FALSE);

	SetMotionFromDialog ();

	SetModifiedFlag (true);
}

void CMotionsDlg::OnChangeEditmotion() 
{
	SetModifiedFlag (true);
	m_FilenameChanged = true;
}

void CMotionsDlg::OnBrowsemotion() 
{
	CFileDialog *FileDlg = MyFileDialog_Create
	(
	  TRUE, IDS_MOTIONFILEPROMPT, IDS_MOTFILEEXT, IDS_MOTFILEFILTER,
	  m_MotionFilename, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this
	);
	if (FileDlg != NULL)
	{
		if (FileDlg->DoModal () == IDOK)
		{
			char NewMotionName[MAX_PATH];

			SetModifiedFlag (true);
			strcpy (NewMotionName, m_MotionFilename);

			if (MakeHelp_GetRelativePath (m_Project, FileDlg->GetPathName (), NewMotionName))
			{
				m_MotionFilename = NewMotionName;
				SetMotionTypeFromFilename ();
				m_FilenameChanged = false;
			}
			UpdateData (FALSE);
		}
		delete FileDlg;	
	}
	else
	{
		AfxMessageBox (IDS_OUTOFMEMORY);
	}
}

void CMotionsDlg::OnKillfocusEditmotion() 
{
	if (m_FilenameChanged)
	{
		char OldMotionName[MAX_PATH];

		// make sure that the new file is relative if required
		strcpy (OldMotionName, m_MotionFilename);

		UpdateData (TRUE);
		if (MakeHelp_GetRelativePath (m_Project, m_MotionFilename, OldMotionName))
		{
			SetMotionTypeFromFilename ();
		}
		else
		{
			m_MotionFilename = OldMotionName;
		}
		UpdateData (FALSE);
		m_FilenameChanged = false;
	}	
}

void CMotionsDlg::OnMotionType() 
{
	m_FilenameChanged = false;
	SetModifiedFlag (true);
}

void CMotionsDlg::OnDeltaposSpinoptimize(NMHDR* pNMHDR, LRESULT* pResult) 
{
//	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	pNMHDR;
	
	*pResult = 0;
	UpdateData (TRUE);
}

void CMotionsDlg::OnOptimize() 
{
	UpdateData (TRUE);
	SetModifiedFlag (true);
}

void CMotionsDlg::OnChangeEditbone() 
{
	SetModifiedFlag (true);
}

void CMotionsDlg::OnDropFiles (HDROP hDrop)
{
	// Files dropped.  Add each one to the listbox.
	// The motion name defaults to the name part of the full pathname

	// First make sure to update the current motion info
	OnSelchangeMotionslist ();

	bool AddedOne = false;

	// get the count
	const UINT FileCount = DragQueryFile (hDrop, 0xffffffff, NULL, 0);
	for (UINT iFile = 0; iFile < FileCount; ++iFile)
	{
		// get the file and add it to the motions list
		char Filename[MAX_PATH];
		char MotionName[MAX_PATH];
		char NewFilename[MAX_PATH];
		int Index;

		// get the filename
		DragQueryFile (hDrop, iFile, Filename, sizeof (Filename));

		// if we get a path that's not relative to the current path, then just exit
		strcpy (NewFilename, Filename);
		if (!MakeHelp_GetRelativePath (m_Project, Filename, NewFilename))
		{
			goto DropCleanup;
		}

		// Default motion name will be the base filename
		FilePath_GetName (Filename, MotionName);

		// Get motion type.
		// If the extension indicates that it's a Genesis3D motion file, then load
		// the motion and get its name.
		// We'll have to check for duplicates and display a message.
		ApjMotionFormat Fmt = AProject_GetMotionFormatFromFilename (Filename);
		if (Fmt == ApjMotion_Invalid)
		{
			// assume MAX
			Fmt = ApjMotion_Max;
		}
		else if (Fmt == ApjMotion_Mot)
		{
			// it's a Genesis3D motion file.  Try to load it.
			geVFile *MotFile = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_DOS, Filename, NULL, GE_VFILE_OPEN_READONLY);
			if (MotFile != NULL)
			{
				// opened...Let's try to load it.
				geMotion *Motion = geMotion_CreateFromFile (MotFile);
				if (Motion != NULL)
				{
					// get the name from the motion
					const char *Name = geMotion_GetName (Motion);
					if (Name != NULL)
					{
						strcpy (MotionName, Name);
					}
					geMotion_Destroy (&Motion);
				}
				geVFile_Close (MotFile);
			}
		}

		// Well, we've determined the motion name.  Make sure it's unique.
		if (m_MotionsList.FindStringExact (-1, MotionName) != LB_ERR)
		{
			CString Msg;

			AfxFormatString2 (Msg, IDS_CANTDROPMOTION, Filename, MotionName);
			AfxMessageBox (Msg);
		}
		else
		{
			const geBoolean OptFlag = AOptions_GetMotionOptimizationFlag (m_Options);
			int OptLevel = AOptions_GetMotionOptimizationLevel (m_Options);
			// Everything's cool, so add it to the listbox.
			if (AProject_AddMotion (m_Project, MotionName, NewFilename, Fmt, OptFlag, OptLevel, "", &Index) != GE_FALSE)
			{
				// And add to listbox
				int LbItem = m_MotionsList.AddString (MotionName);
				if ((LbItem == LB_ERR) || (LbItem == LB_ERRSPACE))
				{
					// couldn't add it to listbox, so remove it
					CString Msg;

					AProject_RemoveMotion (m_Project, Index);

					AfxFormatString1 (Msg, IDS_ERRORDROPMOTION, Filename);
					AfxMessageBox (Msg);
				}
				else
				{
					// Added it to the listbox, so update item data
					m_MotionsList.SetItemData (LbItem, Index);
					SetModifiedFlag (true);
					AddedOne = true;
				}
			}
		}
	}

	// Let's focus at the beginning, 'cause I don't know what else to do...
	if (AddedOne)
	{
		SetupCurrentItem (0);
		UpdateData (FALSE);
	}

DropCleanup:
	// gotta do this or we'll leak memory and crash windows
	DragFinish (hDrop);
}

void CMotionsDlg::OnRenamemotion() 
{
	// Rename the motion.
	// Get name.  Make sure it's unique.
	// Will have to remove and re-insert it in the listbox.
	int LbItem = m_MotionsList.GetCurSel ();
	if (LbItem == LB_ERR)
	{
		// no current item
		return;
	}

	// get current name
	CString OldMotionName = "";
	m_MotionsList.GetText (LbItem, OldMotionName);

	CTextInputDlg Dlg (IDS_NEWNAMEPROMPT, OldMotionName, this);

	// Prompt user for new name
	if (Dlg.DoModal () == IDOK)
	{
		// Make sure the new name doesn't already exist
		const CString NewMotionName = Dlg.m_Text;

		int Index = AProject_GetMotionIndex (m_Project, NewMotionName);
		if (Index == -1)
		{
			// valid name entered.
			// Get item index from the listbox.
			Index = m_MotionsList.GetItemData (LbItem);
			// First try to add the new motion to the listbox.
			LbItem = m_MotionsList.AddString (NewMotionName);
			if ((LbItem == LB_ERR) || (LbItem == LB_ERRSPACE))
			{
				// couldn't add new motion
				AfxMessageBox (IDS_ERRORADDMOTION);
			}
			else
			{
				// attach motion to name
				m_MotionsList.SetItemData (LbItem, Index);

				// Change the name of this motion in the project.
				AProject_SetMotionName (m_Project, Index, NewMotionName);

				// Set this as the new current selection
				m_MotionsList.SetCurSel (LbItem);
				// and delete the old one...
				LbItem = m_MotionsList.FindStringExact (-1, OldMotionName);
				if (LbItem != LB_ERR)
				{
					m_MotionsList.DeleteString (LbItem);
				}

				// refill the listbox 'cause we've shifted entries
				FillListBox ();

				// and set the new current item
				LbItem = m_MotionsList.FindStringExact (-1, NewMotionName);
				SetupCurrentItem (LbItem);
			}
		}
		else
		{
			// Motion exists in the project.  Tell user about it.
			CString Msg;

			AfxFormatString1 (Msg, IDS_MOTIONEXISTS, NewMotionName);
			AfxMessageBox (Msg);
		}
	}
}
