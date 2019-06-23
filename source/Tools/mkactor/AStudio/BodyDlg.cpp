/****************************************************************************************/
/*  BODYDLG.CPP																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio's body dialog handler.									*/
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
#include "BodyDlg.h"
#include <assert.h>
#include "MyFileDlg.h"
#include "MakeHelp.h"

/////////////////////////////////////////////////////////////////////////////
// CBodyDlg property page

IMPLEMENT_DYNCREATE(CBodyDlg, CAStudioPropPage)

CBodyDlg::CBodyDlg() : CAStudioPropPage(CBodyDlg::IDD),
	m_FilenameChanged(false)
{
	//{{AFX_DATA_INIT(CBodyDlg)
	m_BodyName = _T("");
	m_BodyFormat = -1;
	//}}AFX_DATA_INIT
}

CBodyDlg::~CBodyDlg()
{
}

void CBodyDlg::DoDataExchange(CDataExchange* pDX)
{
	CAStudioPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBodyDlg)
	DDX_Control(pDX, IDC_EDITBODY, m_EditBody);
	DDX_Text(pDX, IDC_EDITBODY, m_BodyName);
	DDX_Radio(pDX, IDC_BODYMAX, m_BodyFormat);
	//}}AFX_DATA_MAP
}

static int IndexFromBodyFormat (ApjBodyFormat Fmt)
{
	switch (Fmt)
	{
		case ApjBody_Max : return 0;
		case ApjBody_Nfo : return 1;
		case ApjBody_Bdy : return 2;
		case ApjBody_Act : return 3;
		default :
			assert (0);		// can't happen??
	}
	return -1;	// keeps the compiler happy
}


void CBodyDlg::SetDialogData (void) 
{
	m_BodyName = AProject_GetBodyFilename (m_Project);
	m_BodyFormat = IndexFromBodyFormat (AProject_GetBodyFormat (m_Project));
}

void CBodyDlg::EnableControls (void)
{
	// enable/disable controls depending on state of m_Compiling flag
	m_EditBody.EnableWindow (!m_Compiling);
	GetDlgItem (IDC_BROWSEBODY)->EnableWindow (!m_Compiling);
	GetDlgItem (IDC_BODYACT)->EnableWindow (!m_Compiling);
	GetDlgItem (IDC_BODYBDY)->EnableWindow (!m_Compiling);
	GetDlgItem (IDC_BODYMAX)->EnableWindow (!m_Compiling);
	GetDlgItem (IDC_BODYNFO)->EnableWindow (!m_Compiling);
}

BEGIN_MESSAGE_MAP(CBodyDlg, CAStudioPropPage)
	//{{AFX_MSG_MAP(CBodyDlg)
	ON_EN_CHANGE(IDC_EDITBODY, OnChangeEditbody)
	ON_BN_CLICKED(IDC_BROWSEBODY, OnBrowsebody)
	ON_BN_CLICKED(IDC_BODYACT, OnBodyBtn)
	ON_BN_CLICKED(IDC_BODYBDY, OnBodyBtn)
	ON_BN_CLICKED(IDC_BODYMAX, OnBodyBtn)
	ON_BN_CLICKED(IDC_BODYNFO, OnBodyBtn)
	ON_EN_KILLFOCUS(IDC_EDITBODY, OnKillfocusEditbody)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBodyDlg message handlers

BOOL CBodyDlg::OnInitDialog() 
{
	assert (m_Project != NULL);

	CAStudioPropPage::OnInitDialog();
	
	SetDialogData ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CBodyDlg::OnSetActive ()
{
	SetDialogData ();
	EnableControls ();
	UpdateData (FALSE);
	return TRUE;
}

void CBodyDlg::SetBodyTypeFromFilename ()
{
	ApjBodyFormat Fmt;

	Fmt = AProject_GetBodyFormatFromFilename (m_BodyName);
	if (Fmt != ApjBody_Invalid)
	{
		m_BodyFormat = IndexFromBodyFormat (Fmt);
	}
}

BOOL CBodyDlg::OnKillActive ()
{
	if (m_Compiling)
	{
		return TRUE;
	}

	static const ApjBodyFormat Fmts[] = {ApjBody_Max, ApjBody_Nfo, ApjBody_Bdy, ApjBody_Act};

	// Save body name setting so we can go back if necessary
	char OldBodyName[MAX_PATH];
	strcpy (OldBodyName, m_BodyName);

	if (UpdateData (TRUE))
	{
		if (m_FilenameChanged)
		{
			// Have to set filename changed to false here before calling GetRelativePath.
			// Otherwise OnKillFocus will get called if an error message dialog box pops up, and
			// we end up with two of 'em.  The wonders of event-driven programming...
			m_FilenameChanged = false;
			if (MakeHelp_GetRelativePath (m_Project, m_BodyName, OldBodyName))
			{
				SetBodyTypeFromFilename ();
			}
			else
			{
				m_BodyName = OldBodyName;
				UpdateData (FALSE);
				return FALSE;
			}
		}
		AProject_SetBodyFilename (m_Project, m_BodyName);
		AProject_SetBodyFormat (m_Project, Fmts[m_BodyFormat]);
		return TRUE;
	}
	return FALSE;
}

void CBodyDlg::GetDialogData ()
{
	OnKillActive ();
}

void CBodyDlg::SetProjectPointer (AProject *Project)
{
	CAStudioPropPage::SetProjectPointer (Project);

	if (IsWindow (m_hWnd))
	{
		OnSetActive ();
	}
}

void CBodyDlg::SetCompileStatus (bool Status)
{
	CAStudioPropPage::SetCompileStatus (Status);

	if (IsWindow (m_hWnd))
	{
		EnableControls ();
	}
}

void CBodyDlg::OnChangeEditbody() 
{
	SetModifiedFlag (true);
	m_FilenameChanged = true;
}

// User clicked on browse button to locate body file
void CBodyDlg::OnBrowsebody() 
{
	CFileDialog *FileDlg = MyFileDialog_Create
	(
	  TRUE, IDS_BODYPROMPT, IDS_BDYFILEEXT, IDS_BDYFILEFILTER,
	  m_BodyName, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this
	);

	if (FileDlg != NULL)
	{
		if (FileDlg->DoModal () == IDOK)
		{
			char NewBodyName[MAX_PATH];

			SetModifiedFlag (true);
			strcpy (NewBodyName, m_BodyName);
			if (MakeHelp_GetRelativePath (m_Project, FileDlg->GetPathName (), NewBodyName))
			{
				m_BodyName = NewBodyName;
				SetBodyTypeFromFilename ();
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

void CBodyDlg::OnBodyBtn() 
{
	SetModifiedFlag (true);
	m_FilenameChanged = false;
}

void CBodyDlg::OnKillfocusEditbody() 
{
	UpdateData (true);

	// have to do this on KillFocus because we want to set the body type.
	if (m_FilenameChanged)
	{
		char NewBodyName[MAX_PATH];

		strcpy (NewBodyName, m_BodyName);
		UpdateData (TRUE);
		if (MakeHelp_GetRelativePath (m_Project, m_BodyName, NewBodyName))
		{
			SetBodyTypeFromFilename ();
			m_FilenameChanged = false;
		}
		else
		{
			m_BodyName = NewBodyName;
			m_FilenameChanged = false;
		}
		UpdateData (FALSE);
	}
}
