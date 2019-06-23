/****************************************************************************************/
/*  SETTINGSDLG.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio project settings dialog.									*/
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
#include "AStudio.h"
#include "SettingsDlg.h"
#include "MyFileDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg property page

IMPLEMENT_DYNCREATE(CSettingsDlg, CPropertyPage)

CSettingsDlg::CSettingsDlg() : CPropertyPage(CSettingsDlg::IDD),
	m_Compiling (false)
{
	//{{AFX_DATA_INIT(CSettingsDlg)
	m_3DSMaxPath = _T("");
	m_ViewerPath = _T("");
	m_OptLevel = 0;
	m_OptCheck = FALSE;
	//}}AFX_DATA_INIT
}

CSettingsDlg::~CSettingsDlg()
{
}

void CSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSettingsDlg)
	DDX_Control(pDX, IDC_SPINOPTLEVEL, m_SpinOptLevel);
	DDX_Control(pDX, IDC_EDITOPTLEVEL, m_EditOptLevel);
	DDX_Text(pDX, IDC_EDIT3DSMAXPATH, m_3DSMaxPath);
	DDX_Text(pDX, IDC_EDITVIEWERPATH, m_ViewerPath);
	DDX_Text(pDX, IDC_EDITOPTLEVEL, m_OptLevel);
	DDV_MinMaxInt(pDX, m_OptLevel, 0, 9);
	DDX_Check(pDX, IDC_OPTCHECK, m_OptCheck);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSettingsDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CSettingsDlg)
	ON_BN_CLICKED(IDC_BROWSE3DSMAX, OnBrowse3dsmax)
	ON_BN_CLICKED(IDC_BROWSEVIEWER, OnBrowseviewer)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_OPTCHECK, OnOptcheck)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPINOPTLEVEL, OnDeltaposSpinoptlevel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg message handlers

void CSettingsDlg::EnableControls ()
{
	GetDlgItem (IDC_EDIT3DSMAXPATH)->EnableWindow (!m_Compiling);
	GetDlgItem (IDC_EDITVIEWERPATH)->EnableWindow (!m_Compiling);
	m_EditOptLevel.EnableWindow (!m_Compiling);
	m_SpinOptLevel.EnableWindow (!m_Compiling);
}

BOOL CSettingsDlg::OnSetActive ()
{
	m_3DSMaxPath = AOptions_Get3DSMaxPath (Options);
	m_ViewerPath = AOptions_GetViewerPath (Options);
	m_OptLevel = AOptions_GetMotionOptimizationLevel (Options);
	m_OptCheck = AOptions_GetMotionOptimizationFlag (Options);
	EnableControls ();

	return UpdateData (FALSE);
}

BOOL CSettingsDlg::OnKillActive ()
{
	if (UpdateData (TRUE))
	{
		AOptions_Set3DSMaxPath (Options, m_3DSMaxPath);
		AOptions_SetViewerPath (Options, m_ViewerPath);
		AOptions_SetMotionOptimizationLevel (Options, m_OptLevel);
		AOptions_SetMotionOptimizationFlag (Options, m_OptCheck);
		return TRUE;
	}

	return FALSE;
}

void CSettingsDlg::OnDestroy() 
{
	OnKillActive ();	

	CPropertyPage::OnDestroy();
}


BOOL CSettingsDlg::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_SpinOptLevel.SetRange (0, 9);

	return TRUE;
}

void CSettingsDlg::OnBrowse3dsmax() 
{
	CFileDialog *FileDlg = MyFileDialog_Create
	(
	  TRUE, IDS_3DSMAXPROMPT, IDS_EXEFILEEXT, IDS_EXEFILEFILTER,
	  m_3DSMaxPath, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this
	);

	if (FileDlg != NULL)
	{
		if (FileDlg->DoModal () == IDOK)
		{
			m_3DSMaxPath = FileDlg->GetPathName ();
			UpdateData (FALSE);
		}
		delete FileDlg;
	}
	else
	{
		AfxMessageBox (IDS_OUTOFMEMORY);
	}
}

void CSettingsDlg::OnBrowseviewer() 
{
	CFileDialog *FileDlg = MyFileDialog_Create
	(
	  TRUE, IDS_VIEWERPROMPT, IDS_EXEFILEEXT, IDS_EXEFILEFILTER,
	  m_ViewerPath, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this
	);

	if (FileDlg != NULL)
	{
		if (FileDlg->DoModal () == IDOK)
		{
			m_ViewerPath = FileDlg->GetPathName ();
			UpdateData (FALSE);
		}
		delete FileDlg;
	}
	else
	{
		AfxMessageBox (IDS_OUTOFMEMORY);
	}
}


void CSettingsDlg::SetCompileStatus (bool Status)
{
	m_Compiling = Status;
	if (IsWindow (m_hWnd))
	{
		EnableControls ();
	}
}

void CSettingsDlg::OnOptcheck() 
{
	// Nothing to do??	
}

void CSettingsDlg::OnDeltaposSpinoptlevel(NMHDR* pNMHDR, LRESULT* pResult) 
{
//	NM_UPDOWN* pNMUpDown = (NM_UPDOWN*)pNMHDR;
	pNMHDR;	
	// ?? Nothing to do??
	
	*pResult = 0;
}
