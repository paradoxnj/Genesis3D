/****************************************************************************************/
/*  CreateConeDialog.cpp                                                                */
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
#include "CreateConeDialog.h"
#include "units.h"
#include "facelist.h"
#include <math.h>
#include "ram.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreateConeDialog::CCreateConeDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateConeDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateConeDialog)
	m_Style = 0;
	m_Width = 200;
	m_Height = 300;
	m_VerticalStrips = 4;
	m_Thickness = 16;
	m_TCut = FALSE;
	//}}AFX_DATA_INIT
}

void CCreateConeDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateConeDialog)
	DDX_Radio(pDX, IDC_SOLID, m_Style);
	DDX_Text(pDX, IDC_EDIT1, m_Width);
	DDV_MinMaxFloat(pDX, m_Width, 1.f, 4000.f);
	DDX_Text(pDX, IDC_EDIT10, m_Height);
	DDV_MinMaxFloat(pDX, m_Height, 1.f, 4000.f);
	DDX_Text(pDX, IDC_EDIT11, m_VerticalStrips);
	DDV_MinMaxInt(pDX, m_VerticalStrips, 1, 100);
	DDX_Text(pDX, IDC_EDIT12, m_Thickness);
	DDV_MinMaxFloat(pDX, m_Thickness, 1.f, 2000.f);
	DDX_Check(pDX, IDC_TCUT, m_TCut);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateConeDialog, CDialog)
	//{{AFX_MSG_MAP(CCreateConeDialog)
	ON_BN_CLICKED(ID_DEFAULT, OnDefault)
	ON_BN_CLICKED(IDC_SOLID, OnSolid)
	ON_BN_CLICKED(IDC_HOLLOW, OnHollow)
	ON_BN_CLICKED(IDC_FUNNEL, OnFunnel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int	CCreateConeDialog::DoModal(geBoolean ConvertToMetric, BrushTemplate_Cone *pConeTemplate)
{
	m_ConvertToMetric	=ConvertToMetric;
	m_pConeTemplate = pConeTemplate;

	m_Style		= pConeTemplate->Style;
	m_Width		= pConeTemplate->Width;
	m_Height		= pConeTemplate->Height;
	m_VerticalStrips = pConeTemplate->VerticalStrips;
	m_Thickness	= pConeTemplate->Thickness;
	m_TCut		= pConeTemplate->TCut;


	if(m_ConvertToMetric)
	{
		dlgFieldsToCentimeters();
	}

	return CDialog::DoModal();
}

void	CCreateConeDialog::dlgFieldsToTexels(void)
{
	m_Width		=Units_CentimetersToEngine(m_Width);
	m_Height	=Units_CentimetersToEngine(m_Height);
	m_Thickness	=Units_CentimetersToEngine(m_Thickness);
}

void	CCreateConeDialog::dlgFieldsToCentimeters(void)
{
	m_Width		=Units_FRound(Units_EngineToCentimeters(m_Width));
	m_Height	=Units_FRound(Units_EngineToCentimeters(m_Height));
	m_Thickness	=Units_FRound(Units_EngineToCentimeters(m_Thickness));
}

void CCreateConeDialog::OnDefault()
{
	m_Style = 0;
	m_Width = 200;
	m_Height = 300;
	m_VerticalStrips = 4;
	m_Thickness = 16;
	m_TCut = FALSE;
	UpdateData( FALSE ) ;
}/* CCreateConeDialog::OnDefault */

void CCreateConeDialog::OnSolid() 
{	
}

void CCreateConeDialog::OnHollow() 
{	
}

void CCreateConeDialog::OnFunnel() 
{
}
/* EOF: CreateConeDialog.cpp */
void CCreateConeDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_ConvertToMetric)
	{
		dlgFieldsToTexels();
	}

	m_pConeTemplate->Style		= m_Style;
	m_pConeTemplate->Width		= m_Width;
	m_pConeTemplate->Height		= m_Height;
	m_pConeTemplate->VerticalStrips = m_VerticalStrips;
	m_pConeTemplate->Thickness	= m_Thickness;
	m_pConeTemplate->TCut		= m_TCut;

	CDialog::OnOK();
}
