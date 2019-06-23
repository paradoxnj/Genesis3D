/****************************************************************************************/
/*  CreateSpheroidDialog.cpp                                                            */
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
#include "CreateSpheroidDialog.h"
#include "units.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreateSpheroidDialog::CCreateSpheroidDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateSpheroidDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateSpheroidDialog)
	m_HorizontalBands = 4;
	m_VerticalBands = 8;
	m_YSize = 256.0;
	m_Solid = 0;
	m_Thickness = 16;
	m_TCut = FALSE;
	//}}AFX_DATA_INIT

	mSolidSphere.LoadBitmap(IDB_SOLIDSPHERE);
	mHollowSphere.LoadBitmap(IDB_HOLLOWSPHERE);
}

CCreateSpheroidDialog::~CCreateSpheroidDialog()
{
	mSolidSphere.DeleteObject();
	mHollowSphere.DeleteObject();
}

void CCreateSpheroidDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateSpheroidDialog)
	DDX_Control(pDX, IDC_PICTURE, m_Picture);
	DDX_Text(pDX, IDC_HORSTRIPES, m_HorizontalBands);
	DDV_MinMaxInt(pDX, m_HorizontalBands, 2, 100);
	DDX_Text(pDX, IDC_VERTSTRIPES, m_VerticalBands);
	DDV_MinMaxInt(pDX, m_VerticalBands, 3, 100);
	DDX_Text(pDX, IDC_YSIZE, m_YSize);
	DDV_MinMaxFloat(pDX, m_YSize, 1.f, 4000.f);
	DDX_Radio(pDX, IDC_SOLID, m_Solid);
	DDX_Text(pDX, IDC_THICKNESS, m_Thickness);
	DDV_MinMaxFloat(pDX, m_Thickness, 1.f, 2000.f);
	DDX_Check(pDX, IDC_TCUT, m_TCut);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateSpheroidDialog, CDialog)
	//{{AFX_MSG_MAP(CCreateSpheroidDialog)
	ON_BN_CLICKED(IDC_HOLLOW, OnHollow)
	ON_BN_CLICKED(IDC_SOLID, OnSolid)
	ON_BN_CLICKED(IDC_Defaults, OnDefaults)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int	CCreateSpheroidDialog::DoModal(geBoolean ConvertToMetric, BrushTemplate_Spheroid *pTemplate)
{
	m_ConvertToMetric	=ConvertToMetric;
	m_pTemplate = pTemplate;

	m_HorizontalBands	= m_pTemplate->HorizontalBands;
	m_VerticalBands		= m_pTemplate->VerticalBands;
	m_YSize				= m_pTemplate->YSize;
	m_Solid				= m_pTemplate->Solid;
	m_Thickness			= m_pTemplate->Thickness;
	m_TCut				= m_pTemplate->TCut;

	if(m_ConvertToMetric)
	{
		dlgFieldsToCentimeters();
	}

	return CDialog::DoModal();
}

void CCreateSpheroidDialog::OnHollow() 
{
	m_Picture.SetBitmap(mHollowSphere);
}

void CCreateSpheroidDialog::OnSolid() 
{
	m_Picture.SetBitmap(mSolidSphere);
}

BOOL CCreateSpheroidDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if(m_Solid==0)
		m_Picture.SetBitmap(mSolidSphere);
	else
		m_Picture.SetBitmap(mHollowSphere);
	
	return	TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

//#define PI2 (2.0 * M_PI)

void	CCreateSpheroidDialog::dlgFieldsToTexels(void)
{
	m_YSize			=Units_CentimetersToEngine(m_YSize);
}

void	CCreateSpheroidDialog::dlgFieldsToCentimeters(void)
{
	m_YSize			=Units_FRound(Units_EngineToCentimeters(m_YSize));
}

void CCreateSpheroidDialog::OnDefaults() 
{
	m_HorizontalBands	=4;
	m_VerticalBands		=8;
	m_YSize				=256.0;
	m_Solid				=0;
	m_Thickness			=10;
	UpdateData(FALSE);
	m_Picture.SetBitmap(mSolidSphere);
}

void CCreateSpheroidDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_ConvertToMetric)
	{
		dlgFieldsToTexels();
	}

	m_pTemplate->HorizontalBands	= m_HorizontalBands;
	m_pTemplate->VerticalBands		= m_VerticalBands;
	m_pTemplate->YSize				= m_YSize;
	m_pTemplate->Solid				= m_Solid;
	m_pTemplate->Thickness			= m_Thickness;
	m_pTemplate->TCut				= m_TCut;
	
	CDialog::OnOK();
}

void CCreateSpheroidDialog::Serialize(CArchive& ar) 
{
	if (ar.IsStoring())
	{	// storing code
		ar << m_HorizontalBands;
		ar << m_Solid;
		ar << m_Thickness;
		ar << m_VerticalBands;
		ar << m_YSize;
	}
	else
	{	// loading code
		ar >> m_HorizontalBands;
		ar >> m_Solid;
		ar >> m_Thickness;
		ar >> m_VerticalBands;
		ar >> m_YSize;
	}
}
