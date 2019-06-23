/****************************************************************************************/
/*  TARGETDLG.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio build target dialog.										*/
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
#include "TargetDlg.h"
#include <assert.h>
#include "MyFileDlg.h"
#include "MakeHelp.h"

/////////////////////////////////////////////////////////////////////////////
// CTargetDlg property page

IMPLEMENT_DYNCREATE(CTargetDlg, CAStudioPropPage)

CTargetDlg::CTargetDlg() : CAStudioPropPage(CTargetDlg::IDD),
	m_FilenameChanged(false)
{
	//{{AFX_DATA_INIT(CTargetDlg)
	m_TargetName = _T("");
	m_OutputFormat = -1;
	//}}AFX_DATA_INIT
}

CTargetDlg::~CTargetDlg()
{
	SetModified (TRUE);
}

void CTargetDlg::DoDataExchange(CDataExchange* pDX)
{
	CAStudioPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTargetDlg)
	DDX_Control(pDX, IDC_EDITTARGET, m_EditTarget);
	DDX_Text(pDX, IDC_EDITTARGET, m_TargetName);
	DDX_Radio(pDX, IDC_FORMATBINARY, m_OutputFormat);
	//}}AFX_DATA_MAP
}


void CTargetDlg::SetDialogData (void) 
{
	m_TargetName = AProject_GetOutputFilename (m_Project);
}

void CTargetDlg::EnableControls ()
{
	m_EditTarget.EnableWindow (!m_Compiling);
	GetDlgItem (IDC_BROWSETARGET)->EnableWindow (!m_Compiling);
//	GetDlgItem (IDC_FORMATBINARY)->EnableWindow (!m_Compiling);
//	GetDlgItem (IDC_FORMATTEXT)->EnableWindow (!m_Compiling);
}

BEGIN_MESSAGE_MAP(CTargetDlg, CAStudioPropPage)
	//{{AFX_MSG_MAP(CTargetDlg)
	ON_BN_CLICKED(IDC_BROWSETARGET, OnBrowsetarget)
	ON_EN_CHANGE(IDC_EDITTARGET, OnChangeEdittarget)
	ON_BN_CLICKED(IDC_FORMATBINARY, OnFormatChanged)
	ON_BN_CLICKED(IDC_FORMATTEXT, OnFormatChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTargetDlg message handlers

BOOL CTargetDlg::OnInitDialog() 
{
	CAStudioPropPage::OnInitDialog();
	
	SetDialogData ();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CTargetDlg::OnSetActive ()
{
	SetDialogData ();
	EnableControls ();
	UpdateData (FALSE);
	return TRUE;
}

BOOL CTargetDlg::OnKillActive ()
{
	if (m_Compiling)
	{
		return TRUE;
	}
	char OldTargetName[MAX_PATH];
	strcpy (OldTargetName, m_TargetName);
	if (UpdateData (TRUE))
	{
		if (m_FilenameChanged)
		{
			// Have to set filename changed to false here before calling GetRelativePath.
			// Otherwise OnKillFocus will get called if an error message dialog box pops up, and
			// we end up with two of 'em.  The wonders of event-driven programming...
			m_FilenameChanged = false;
			if (!MakeHelp_GetRelativePath (m_Project, m_TargetName, OldTargetName))
			{
				m_TargetName = OldTargetName;
				UpdateData (FALSE);
				return FALSE;
			}
		}
		AProject_SetOutputFilename (m_Project, m_TargetName);
		return TRUE;
	}
	return FALSE;
}

void CTargetDlg::GetDialogData ()
{
	OnKillActive ();
}

void CTargetDlg::SetProjectPointer (AProject *Project)
{
	CAStudioPropPage::SetProjectPointer (Project);

	if (IsWindow (m_hWnd))
	{
		OnSetActive ();
	}
}

void CTargetDlg::SetCompileStatus (bool Status)
{
	CAStudioPropPage::SetCompileStatus (Status);
	if (IsWindow (m_hWnd))
	{
		EnableControls ();
	}
}

void CTargetDlg::OnBrowsetarget() 
{
	CFileDialog *FileDlg = MyFileDialog_Create
	(
	  FALSE, IDS_TARGETPROMPT, IDS_ACTFILEEXT, IDS_ACTFILEFILTER,
	  m_TargetName, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST, this
	);
	
	if (FileDlg->DoModal () == IDOK)
	{
		char NewTargetName[MAX_PATH];

		SetModifiedFlag (true);
		strcpy (NewTargetName, m_TargetName);
		if (MakeHelp_GetRelativePath (m_Project, FileDlg->GetPathName (), NewTargetName))
		{
			m_TargetName = NewTargetName;
			m_FilenameChanged = false;
		}
		UpdateData (FALSE);
	}

	delete FileDlg;
}

void CTargetDlg::OnChangeEdittarget() 
{
	SetModifiedFlag (true);	
	m_FilenameChanged = true;
}

void CTargetDlg::OnFormatChanged() 
{
	SetModifiedFlag (true);
}
