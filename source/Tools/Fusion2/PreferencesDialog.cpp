/****************************************************************************************/
/*  PreferencesDialog.cpp                                                               */
/*                                                                                      */
/*  Author:       Jim Mischel, Jeff Lomax                                               */
/*  Description:  Preferences for grid and other stuff                                  */
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
#include "PreferencesDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDialog dialog


CPreferencesDialog::CPreferencesDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPreferencesDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPreferencesDialog)
	//}}AFX_DATA_INIT
}


void CPreferencesDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPreferencesDialog)
	DDX_Control(pDX, IDC_PREFSSNAPGRID, m_SnapGrid);
	DDX_Control(pDX, IDC_PREFSGRID, m_Grid);
	DDX_Control(pDX, IDC_PREFSGRIDBG, m_GridBackground);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDialog, CDialog)
	//{{AFX_MSG_MAP(CPreferencesDialog)
	ON_BN_CLICKED(IDC_PATHPREFS, OnPathprefs)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDialog message handlers

BOOL CPreferencesDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_GridBackground.SetColor( coBackground ) ;
	m_Grid.SetColor( coGrid ) ;
	m_SnapGrid.SetColor( coSnapGrid ) ;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPreferencesDialog::OnOK() 
{
	// #pragma todo: Add extra validation here
	coBackground = m_GridBackground.GetColor( ) ;
	coSnapGrid = m_SnapGrid.GetColor( ) ;
	coGrid = m_Grid.GetColor( ) ;

	CDialog::OnOK();
}

void CPreferencesDialog::OnPathprefs() 
{
	// #pragma todo: Add your control notification handler code here
	
}
