/****************************************************************************************/
/*  CreateBoxDialog.cpp                                                                 */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, John Moore                                    */
/*  Description:  Dialog code for templates                                             */
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
#include "CreateBoxDialog.h"
#include "units.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCreateBoxDialog::CCreateBoxDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateBoxDialog::IDD, pParent)
{
	// We want to initialize our data to something
	// reasonable

	//{{AFX_DATA_INIT(CCreateBoxDialog)
	m_YSize = 128.0f;
	m_Solid = 0;
	m_XSizeBot = 128.0f;
	m_XSizeTop = 128.0f;
	m_ZSizeBot = 128.0f;
	m_ZSizeTop = 128.0f;
	m_TCut = FALSE;
	m_Thickness = 16.0f;
	m_TSheet = FALSE;
	//}}AFX_DATA_INIT

	// we want to load up our bitmaps
	mHollowBitmap.LoadBitmap( IDB_HOLLOWBOX );
	mSolidBitmap.LoadBitmap( IDB_SOLIDBOX );

}

CCreateBoxDialog::~CCreateBoxDialog()
{
	// delete our stupid bitmaps
	mHollowBitmap.DeleteObject();
	mSolidBitmap.DeleteObject();
}


void CCreateBoxDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateBoxDialog)
	DDX_Control(pDX, IDC_TCUT, m_CutBtn);
	DDX_Control(pDX, IDC_TSHEET, m_SheetBtn);
	DDX_Control(pDX, IDC_PICTURE, m_Picture);
	DDX_Text(pDX, IDC_YSIZE, m_YSize);
	DDV_MinMaxFloat(pDX, m_YSize, 1.f, 4000.f);
	DDX_Radio(pDX, IDC_SOLID, m_Solid);
	DDX_Text(pDX, IDC_XSIZEBOT, m_XSizeBot);
	DDV_MinMaxFloat(pDX, m_XSizeBot, 1.f, 4000.f);
	DDX_Text(pDX, IDC_XSIZETOP, m_XSizeTop);
	DDV_MinMaxFloat(pDX, m_XSizeTop, 1.f, 4000.f);
	DDX_Text(pDX, IDC_ZSIZEBOT, m_ZSizeBot);
	DDV_MinMaxFloat(pDX, m_ZSizeBot, 1.f, 4000.f);
	DDX_Text(pDX, IDC_ZSIZETOP, m_ZSizeTop);
	DDV_MinMaxFloat(pDX, m_ZSizeTop, 1.f, 4000.f);
	DDX_Check(pDX, IDC_TCUT, m_TCut);
	DDX_Text(pDX, IDC_THICKNESS, m_Thickness);
	DDV_MinMaxFloat(pDX, m_Thickness, 1.f, 2000.f);
	DDX_Check(pDX, IDC_TSHEET, m_TSheet);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateBoxDialog, CDialog)
	//{{AFX_MSG_MAP(CCreateBoxDialog)
	ON_BN_CLICKED(IDC_SOLID, OnSolid)
	ON_BN_CLICKED(IDC_HOLLOW, OnHollow)
	ON_BN_CLICKED(IDC_Defaults, OnDefaults)
	ON_BN_CLICKED(IDC_TCUT, OnTcut)
	ON_BN_CLICKED(IDC_TSHEET, OnTsheet)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateBoxDialog message handlers
int	CCreateBoxDialog::DoModal(geBoolean ConvertToMetric, BrushTemplate_Box *pBoxTemplate)
{
	m_ConvertToMetric	=ConvertToMetric;
	m_pBoxTemplate = pBoxTemplate;

	m_Solid		= pBoxTemplate->Solid;
	m_TCut		= pBoxTemplate->TCut;
	m_Thickness = pBoxTemplate->Thickness;
	m_XSizeTop	= pBoxTemplate->XSizeTop;
	m_XSizeBot	= pBoxTemplate->XSizeBot;
	m_YSize		= pBoxTemplate->YSize;
	m_ZSizeTop	= pBoxTemplate->ZSizeTop;
	m_ZSizeBot	= pBoxTemplate->ZSizeBot;

	if(m_ConvertToMetric)
	{
		dlgFieldsToCentimeters();
	}

	return CDialog::DoModal();
}

void	CCreateBoxDialog::dlgFieldsToTexels(void)
{
	m_Thickness	=Units_CentimetersToEngine(m_Thickness);
	m_XSizeTop	=Units_CentimetersToEngine(m_XSizeTop);
	m_XSizeBot	=Units_CentimetersToEngine(m_XSizeBot);
	m_YSize		=Units_CentimetersToEngine(m_YSize);
	m_ZSizeTop	=Units_CentimetersToEngine(m_ZSizeTop);
	m_ZSizeBot	=Units_CentimetersToEngine(m_ZSizeBot);
}

void	CCreateBoxDialog::dlgFieldsToCentimeters(void)
{
	m_Thickness	=Units_FRound(Units_EngineToCentimeters(m_Thickness));
	m_XSizeTop	=Units_FRound(Units_EngineToCentimeters(m_XSizeTop));
	m_XSizeBot	=Units_FRound(Units_EngineToCentimeters(m_XSizeBot));
	m_YSize		=Units_FRound(Units_EngineToCentimeters(m_YSize));
	m_ZSizeTop	=Units_FRound(Units_EngineToCentimeters(m_ZSizeTop));
	m_ZSizeBot	=Units_FRound(Units_EngineToCentimeters(m_ZSizeBot));
}



void CCreateBoxDialog::OnSolid() 
{
	m_Picture.SetBitmap( mSolidBitmap );
	GetDlgItem (IDC_TSHEET)->EnableWindow (TRUE);
}

void CCreateBoxDialog::OnHollow() 
{
	m_Picture.SetBitmap( mHollowBitmap );
	m_TSheet = FALSE;
	m_SheetBtn.EnableWindow (FALSE);
	m_SheetBtn.SetCheck (0);
}

BOOL CCreateBoxDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if( m_Solid == 0 )
		m_Picture.SetBitmap( mSolidBitmap );
	else
		m_Picture.SetBitmap( mHollowBitmap );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCreateBoxDialog::Serialize(CArchive& ar) 
{
	if (ar.IsStoring())
	{	
	}
	else
	{	// loading code
	}
}

void CCreateBoxDialog::OnDefaults() 
{
	m_Picture.SetBitmap( mSolidBitmap );
	m_XSizeTop = 128.0;
	m_ZSizeTop = 128.0;
	m_XSizeBot = 128.0;
	m_ZSizeBot = 128.0;
	m_YSize = 128.0;
	m_Solid = 0;
	m_Thickness = 16.0;
	UpdateData(FALSE);
}

void CCreateBoxDialog::OnTcut() 
{
	if (m_CutBtn.GetCheck ())
	{
		m_SheetBtn.SetCheck (0);
	}
}

void CCreateBoxDialog::OnTsheet() 
{
	if (m_SheetBtn.GetCheck ())
	{
		m_CutBtn.SetCheck (0);
	}
}

/* EOF: CreateBoxDialog.cpp */
void CCreateBoxDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_ConvertToMetric)
	{
		dlgFieldsToTexels();
	}

	m_pBoxTemplate->Solid		= m_Solid;
	m_pBoxTemplate->TCut		= m_TCut;
	m_pBoxTemplate->TSheet		= m_TSheet;
	m_pBoxTemplate->Thickness	= m_Thickness;
	m_pBoxTemplate->XSizeTop	= m_XSizeTop;
	m_pBoxTemplate->XSizeBot	= m_XSizeBot;
	m_pBoxTemplate->YSize		= m_YSize;
	m_pBoxTemplate->ZSizeTop	= m_ZSizeTop;
	m_pBoxTemplate->ZSizeBot	= m_ZSizeBot;
	
	CDialog::OnOK();
}
