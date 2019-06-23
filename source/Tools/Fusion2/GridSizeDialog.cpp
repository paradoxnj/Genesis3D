/****************************************************************************************/
/*  GridSizeDialog.cpp                                                                  */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  UI for grid settings                                                  */
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
#include <afxcmn.h>
#include "GridSizeDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGridSizeDialog dialog


CGridSizeDialog::CGridSizeDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CGridSizeDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CGridSizeDialog)
	m_SnapDegrees = 0.0;
	m_UseSnap = FALSE;
	MetricOrTexelSnap = -1;
	MetricOrTexelGrid = -1;
	m_GridUnits = -1;
	//}}AFX_DATA_INIT
}


void CGridSizeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGridSizeDialog)
	DDX_Text(pDX, IDC_ROTSNAPDEGREES, m_SnapDegrees);
	DDV_MinMaxDouble(pDX, m_SnapDegrees, 0., 90.);
	DDX_Check(pDX, IDC_USEROTSNAP, m_UseSnap);
	DDX_Radio(pDX, IDC_1RADIO, m_GridUnits);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGridSizeDialog, CDialog)
	//{{AFX_MSG_MAP(CGridSizeDialog)
	ON_BN_CLICKED(IDC_SNAP15, OnSnap15)
	ON_BN_CLICKED(IDC_SNAP30, OnSnap30)
	ON_BN_CLICKED(IDC_SNAP45, OnSnap45)
	ON_BN_CLICKED(IDC_SNAP60, OnSnap60)
	ON_BN_CLICKED(IDC_USEROTSNAP, OnUsertosnap)
	ON_BN_CLICKED(IDC_SNAP90, OnSnap90)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//	ON_COMMAND_RANGE(ID_MYCMD_ONE, ID_MYCMD_TEN, OnDoSomething)

void CGridSizeDialog::OnSnap15() 
{
	UpdateData();
	m_SnapDegrees = 15.0;
	UpdateData(FALSE);
}
void CGridSizeDialog::OnSnap30() 
{
	UpdateData();
	m_SnapDegrees = 30.0;
	UpdateData(FALSE);
}

void CGridSizeDialog::OnSnap45() 
{
	UpdateData();
	m_SnapDegrees = 45.0;
	UpdateData(FALSE);
}
void CGridSizeDialog::OnSnap60() 
{
	UpdateData();
	m_SnapDegrees = 60.0;
	UpdateData(FALSE);
}

void CGridSizeDialog::OnSnap90() 
{
	UpdateData();
	m_SnapDegrees = 90.0;
	UpdateData(FALSE);
}


static UINT snapdisable[] =
{
	IDC_SNAP15,         
	IDC_SNAP30,
	IDC_SNAP45,
	IDC_SNAP60,
	IDC_SNAP90,
	IDC_ROTSNAPDEGREES,
	IDC_SPINROTSNAPDEGREES
};

BOOL CGridSizeDialog::OnInitDialog() 
{
	CWnd *	pWnd ;
	CSpinButtonCtrl	* pSpin ;
	int		i ;
	CDialog::OnInitDialog();
	
	for( i=0; i< sizeof( snapdisable ) / sizeof( UINT); i++ )
	{
		pWnd = GetDlgItem( snapdisable[i] ) ;
		pWnd->EnableWindow( m_UseSnap ) ;
	}
	pSpin = (CSpinButtonCtrl*)GetDlgItem( IDC_SPINROTSNAPDEGREES ) ;
	pSpin->SetRange( 0, 90 ) ;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGridSizeDialog::OnUsertosnap() 
{
	CWnd *	pWnd ;
	int i ;

	m_UseSnap = !m_UseSnap ;
	for( i=0; i< sizeof( snapdisable ) / sizeof( UINT); i++ )
	{
		pWnd = GetDlgItem( snapdisable[i] ) ;
		pWnd->EnableWindow( m_UseSnap ) ;
	}
}

