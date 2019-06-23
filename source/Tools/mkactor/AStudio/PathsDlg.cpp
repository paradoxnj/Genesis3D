/****************************************************************************************/
/*  PATHSDLG.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio paths dialog.												*/
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
#include "PathsDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CPathsDlg property page

IMPLEMENT_DYNCREATE(CPathsDlg, CAStudioPropPage)

CPathsDlg::CPathsDlg() : CAStudioPropPage(CPathsDlg::IDD)
{
	//{{AFX_DATA_INIT(CPathsDlg)
	m_MaterialsPath = _T("");
	m_TempFilesDir = _T("");
	//}}AFX_DATA_INIT
}

CPathsDlg::~CPathsDlg()
{
}

void CPathsDlg::DoDataExchange(CDataExchange* pDX)
{
	CAStudioPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPathsDlg)
	DDX_Text(pDX, IDC_EDITMATERIALSPATH, m_MaterialsPath);
	DDX_Text(pDX, IDC_EDITTEMPFILESDIR, m_TempFilesDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPathsDlg, CAStudioPropPage)
	//{{AFX_MSG_MAP(CPathsDlg)
	ON_EN_CHANGE(IDC_EDITMATERIALSPATH, OnChangeEditmaterialspath)
	ON_EN_CHANGE(IDC_EDITTEMPFILESDIR, OnChangeEdittempfilesdir)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPathsDlg message handlers

void CPathsDlg::SetProjectPointer (AProject *Project)
{
	CAStudioPropPage::SetProjectPointer (Project);

	if (IsWindow (m_hWnd) && (m_Project != NULL))
	{
		SetDialogData ();
		UpdateData (FALSE);
	}
}

void CPathsDlg::SetDialogData ()
{
	m_MaterialsPath = AProject_GetMaterialsPath (m_Project);
	m_TempFilesDir = AProject_GetObjPath (m_Project);
}

BOOL CPathsDlg::OnInitDialog() 
{
	CAStudioPropPage::OnInitDialog();
	
	SetDialogData ();
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPathsDlg::EnableControls ()
{
	GetDlgItem (IDC_EDITMATERIALSPATH)->EnableWindow (!m_Compiling);
	GetDlgItem (IDC_EDITTEMPFILESDIR)->EnableWindow (!m_Compiling);
}

BOOL CPathsDlg::OnSetActive ()
{
	SetDialogData ();
	UpdateData (FALSE);
	EnableControls ();
	return TRUE;
}

BOOL CPathsDlg::OnKillActive ()
{
	if (m_Compiling)
	{
		return TRUE;
	}
	if (UpdateData (TRUE))
	{
		AProject_SetMaterialsPath (m_Project, m_MaterialsPath);
		AProject_SetObjPath (m_Project, m_TempFilesDir);
		return TRUE;
	}
	return FALSE;
}


void CPathsDlg::SetCompileStatus (bool Status)
{
	CAStudioPropPage::SetCompileStatus (Status);
	if (IsWindow (m_hWnd))
	{
		EnableControls ();
	}
}

void CPathsDlg::GetDialogData ()
{
	OnKillActive ();
}

void CPathsDlg::OnChangeEditmaterialspath() 
{
	SetModifiedFlag (true);
}

void CPathsDlg::OnChangeEdittempfilesdir() 
{
	SetModifiedFlag (true);
}
