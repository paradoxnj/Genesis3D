/****************************************************************************************/
/*  TEXTINPUTDLG.H																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Simple text input dialog box.											*/
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
#include "TextInputDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextInputDlg dialog


CTextInputDlg::CTextInputDlg(int TitleId, const CString &Text, CWnd* pParent /*=NULL*/)
	: CDialog(CTextInputDlg::IDD, pParent), m_TitleId(TitleId)
{
	//{{AFX_DATA_INIT(CTextInputDlg)
	m_Text = Text;
	//}}AFX_DATA_INIT
}


void CTextInputDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextInputDlg)
	DDX_Text(pDX, IDC_EDIT1, m_Text);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextInputDlg, CDialog)
	//{{AFX_MSG_MAP(CTextInputDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextInputDlg message handlers

void CTextInputDlg::OnOK() 
{
	CDialog::OnOK();
}

BOOL CTextInputDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Set title from ID
	CString Title;

	Title.LoadString (m_TitleId);
	SetWindowText (Title);
		
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
