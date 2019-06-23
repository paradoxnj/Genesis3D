/****************************************************************************************/
/*  CreateArchDialog.cpp                                                                */
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
#include "CreateArchDialog.h"
#include "units.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCreateArchDialog::CCreateArchDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateArchDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateArchDialog)
	m_NumSlits	=3;
	m_Thickness	=150;
	m_Width		=100;
	m_Radius	=200;
	m_WallSize	=16;
	m_Style		=0;
	m_EndAngle	=180.0f;
	m_StartAngle=0.0f;
	m_TCut = FALSE;
	//}}AFX_DATA_INIT
}


void CCreateArchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateArchDialog)
	DDX_Text(pDX, IDC_NUMSLITS, m_NumSlits);
	DDV_MinMaxInt(pDX, m_NumSlits, 3, 64);
	DDX_Text(pDX, IDC_THICKNESS, m_Thickness);
	DDV_MinMaxFloat(pDX, m_Thickness, 1.f, 4000.f);
	DDX_Text(pDX, IDC_WIDTH, m_Width);
	DDV_MinMaxFloat(pDX, m_Width, 1.f, 4000.f);
	DDX_Text(pDX, IDC_RADIUS, m_Radius);
	DDV_MinMaxFloat(pDX, m_Radius, 1.f, 10000.f);
	DDX_Text(pDX, IDC_WALLSIZE, m_WallSize);
	DDV_MinMaxFloat(pDX, m_WallSize, 1.f, 4000.f);
	DDX_Radio(pDX, IDC_SOLID, m_Style);
	DDX_Text(pDX, IDC_ENDANGLE, m_EndAngle);
	DDV_MinMaxDouble(pDX, m_EndAngle, -360., 360.);
	DDX_Text(pDX, IDC_STARTANGLE, m_StartAngle);
	DDV_MinMaxDouble(pDX, m_StartAngle, -360., 360.);
	DDX_Check(pDX, IDC_TCUT, m_TCut);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateArchDialog, CDialog)
	//{{AFX_MSG_MAP(CCreateArchDialog)
	ON_BN_CLICKED(ID_DEFAULTS, OnDefaults)
	ON_BN_CLICKED(IDC_SOLID, OnSolid)
	ON_BN_CLICKED(IDC_HOLLOW, OnHollow)
	ON_BN_CLICKED(IDC_RING, OnRing)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCreateArchDialog message handlers
int	CCreateArchDialog::DoModal(geBoolean ConvertToMetric, BrushTemplate_Arch *pArchTemplate)
{
	m_ConvertToMetric	=ConvertToMetric;
	m_pArchTemplate = pArchTemplate;


	m_NumSlits	= pArchTemplate->NumSlits;
	m_Thickness	= pArchTemplate->Thickness;
	m_Width		= pArchTemplate->Width;
	m_Radius	= pArchTemplate->Radius;
	m_WallSize	= pArchTemplate->WallSize;
	m_Style		= pArchTemplate->Style;
	m_EndAngle	= pArchTemplate->EndAngle;
	m_StartAngle = pArchTemplate->StartAngle;
	m_TCut		= pArchTemplate->TCut;


	if(m_ConvertToMetric)
	{
		dlgFieldsToCentimeters();
	}

	return CDialog::DoModal();
}

void	CCreateArchDialog::dlgFieldsToTexels(void)
{
	m_Thickness	=Units_CentimetersToEngine(m_Thickness);
	m_Width		=Units_CentimetersToEngine(m_Width);
	m_Radius	=Units_CentimetersToEngine(m_Radius);
	m_WallSize	=Units_CentimetersToEngine(m_WallSize);
}

void	CCreateArchDialog::dlgFieldsToCentimeters(void)
{
	m_Thickness	=Units_FRound(Units_EngineToCentimeters(m_Thickness));
	m_Width		=Units_FRound(Units_EngineToCentimeters(m_Width));
	m_Radius	=Units_FRound(Units_EngineToCentimeters(m_Radius));
	m_WallSize	=Units_FRound(Units_EngineToCentimeters(m_WallSize));
}


void CCreateArchDialog::OnDefaults() 
{
	m_EndAngle = 180;
	m_NumSlits = 3;
	m_StartAngle = 0;
	m_Thickness = 150;
	m_Width = 100;
	m_Radius = 200;
	m_WallSize = 16;
	m_Style = 0;
	UpdateData( FALSE );
}

void CCreateArchDialog::OnSolid() 
{
}

void CCreateArchDialog::OnHollow() 
{
}

void CCreateArchDialog::OnRing() 
{
}
/* EOF: CreateArchDialog */

void CCreateArchDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_ConvertToMetric)
	{
		dlgFieldsToTexels();
	}

	m_pArchTemplate->NumSlits	= m_NumSlits;
	m_pArchTemplate->Thickness	= m_Thickness;
	m_pArchTemplate->Width		= m_Width;
	m_pArchTemplate->Radius		= m_Radius;
	m_pArchTemplate->WallSize	= m_WallSize;
	m_pArchTemplate->Style		= m_Style;
	m_pArchTemplate->EndAngle	= m_EndAngle;
	m_pArchTemplate->StartAngle	= m_StartAngle;
	m_pArchTemplate->TCut		= m_TCut;
	
	CDialog::OnOK();
}
