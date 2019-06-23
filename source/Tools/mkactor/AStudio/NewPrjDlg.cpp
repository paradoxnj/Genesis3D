/****************************************************************************************/
/*  NEWPRJDLG.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio new project dialog.										*/
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
#include "NewPrjDlg.h"
#include "MyFileDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CNewPrjDlg dialog


CNewPrjDlg::CNewPrjDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNewPrjDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNewPrjDlg)
	m_ProjectName = _T("");
	m_UseProjectDir = FALSE;
	//}}AFX_DATA_INIT
}


void CNewPrjDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNewPrjDlg)
	DDX_Text(pDX, IDC_EDITPROJECTNAME, m_ProjectName);
	DDX_Check(pDX, IDC_USEPROJECTDIR, m_UseProjectDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CNewPrjDlg, CDialog)
	//{{AFX_MSG_MAP(CNewPrjDlg)
	ON_BN_CLICKED(IDC_BROWSEPROJECT, OnBrowseproject)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNewPrjDlg message handlers

void CNewPrjDlg::OnBrowseproject() 
{
	CFileDialog *FileDlg = MyFileDialog_Create
	(
		FALSE, IDS_PROJECTNAMEPROMPT, IDS_ASPFILEEXT, IDS_ASPFILEFILTER,
		"", OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST, this
	);
	if (FileDlg != NULL)
	{
		if (FileDlg->DoModal () == IDOK)
		{
			m_ProjectName = FileDlg->GetPathName ();
			UpdateData (FALSE);
		}
		delete FileDlg;	
	}
	else
	{
		AfxMessageBox (IDS_OUTOFMEMORY);
	}
}
