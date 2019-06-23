/****************************************************************************************/
/*  CreateCylDialog.cpp                                                                 */
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
#include "CreateCylDialog.h"
#include <math.h>
#include "units.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CCreateCylDialog::CCreateCylDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateCylDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateCylDialog)
	m_BotXOffset = 0.0;
	m_BotXSize = 128.0;
	m_BotZOffset = 0.0;
	m_BotZSize = 128.0;
	m_Solid = 0;
	m_TopXOffset = 0.0;
	m_TopXSize = 128.0;
	m_TopZOffset = 0.0;
	m_TopZSize = 128.0;
	m_YSize = 512.0;
	m_RingLength = 0.0;
	m_TCut = FALSE;
	m_VerticalStripes = 0;
	m_Thickness = 0.0f;
	//}}AFX_DATA_INIT
}


void CCreateCylDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateCylDialog)
	DDX_Text(pDX, IDC_BOTXOFF, m_BotXOffset);
	DDX_Text(pDX, IDC_BOTXSIZE, m_BotXSize);
	DDV_MinMaxFloat(pDX, m_BotXSize, 1.f, 4000.f);
	DDX_Text(pDX, IDC_BOTZOFF, m_BotZOffset);
	DDX_Text(pDX, IDC_BOTZSIZE, m_BotZSize);
	DDV_MinMaxFloat(pDX, m_BotZSize, 1.f, 4000.f);
	DDX_Radio(pDX, IDC_SOLID, m_Solid);
	DDX_Text(pDX, IDC_TOPXOFF, m_TopXOffset);
	DDX_Text(pDX, IDC_TOPXSIZE, m_TopXSize);
	DDV_MinMaxFloat(pDX, m_TopXSize, 1.f, 4000.f);
	DDX_Text(pDX, IDC_TOPZOFF, m_TopZOffset);
	DDX_Text(pDX, IDC_TOPZSIZE, m_TopZSize);
	DDV_MinMaxFloat(pDX, m_TopZSize, 1.f, 4000.f);
	DDX_Text(pDX, IDC_YSIZE, m_YSize);
	DDV_MinMaxFloat(pDX, m_YSize, 1.f, 4000.f);
	DDX_Text(pDX, IDC_RINGLENGTH, m_RingLength);
	DDX_Check(pDX, IDC_TCUT, m_TCut);
	DDX_Text(pDX, IDC_VERTSTRIPES, m_VerticalStripes);
	DDV_MinMaxInt(pDX, m_VerticalStripes, 3, 64);
	DDX_Text(pDX, IDC_THICKNESS, m_Thickness);
	DDV_MinMaxFloat(pDX, m_Thickness, 1.f, 2000.f);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateCylDialog, CDialog)
	//{{AFX_MSG_MAP(CCreateCylDialog)
	ON_BN_CLICKED(IDC_Defaults, OnDefaults)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int	CCreateCylDialog::DoModal(geBoolean ConvertToMetric, BrushTemplate_Cylinder *pCylTemplate)
{
	m_ConvertToMetric	=ConvertToMetric;
	m_pCylTemplate = pCylTemplate;

	m_BotXOffset	= m_pCylTemplate->BotXOffset;
	m_BotXSize		= m_pCylTemplate->BotXSize;
	m_BotZOffset	= m_pCylTemplate->BotZOffset;
	m_BotZSize		= m_pCylTemplate->BotZSize;
	m_Solid			= m_pCylTemplate->Solid;
	m_Thickness		= m_pCylTemplate->Thickness;
	m_TopXOffset	= m_pCylTemplate->TopXOffset;
	m_TopXSize		= m_pCylTemplate->TopXSize;
	m_TopZOffset	= m_pCylTemplate->TopZOffset;
	m_TopZSize		= m_pCylTemplate->TopZSize;
	m_VerticalStripes	= m_pCylTemplate->VerticalStripes;
	m_YSize			= m_pCylTemplate->YSize;
	m_RingLength	= m_pCylTemplate->RingLength;
	m_TCut			= m_pCylTemplate->TCut;

	if(m_ConvertToMetric)
	{
		dlgFieldsToCentimeters();
	}

	return CDialog::DoModal();
}

void	CCreateCylDialog::dlgFieldsToTexels(void)
{
	m_BotXOffset	=Units_CentimetersToEngine(m_BotXOffset);
	m_BotXSize		=Units_CentimetersToEngine(m_BotXSize);
	m_BotZOffset	=Units_CentimetersToEngine(m_BotZOffset);
	m_BotZSize		=Units_CentimetersToEngine(m_BotZSize);
	m_Thickness		=Units_CentimetersToEngine(m_Thickness);
	m_TopXOffset	=Units_CentimetersToEngine(m_TopXOffset);
	m_TopXSize		=Units_CentimetersToEngine(m_TopXSize);
	m_TopZOffset	=Units_CentimetersToEngine(m_TopZOffset);
	m_TopZSize		=Units_CentimetersToEngine(m_TopZSize);
	m_YSize			=Units_CentimetersToEngine(m_YSize);
	m_RingLength	=Units_CentimetersToEngine(m_RingLength);
}

void	CCreateCylDialog::dlgFieldsToCentimeters(void)
{
	m_BotXOffset	=Units_FRound(Units_EngineToCentimeters(m_BotXOffset));
	m_BotXSize		=Units_FRound(Units_EngineToCentimeters(m_BotXSize));
	m_BotZOffset	=Units_FRound(Units_EngineToCentimeters(m_BotZOffset));
	m_BotZSize		=Units_FRound(Units_EngineToCentimeters(m_BotZSize));
	m_Thickness		=Units_FRound(Units_EngineToCentimeters(m_Thickness));
	m_TopXOffset	=Units_FRound(Units_EngineToCentimeters(m_TopXOffset));
	m_TopXSize		=Units_FRound(Units_EngineToCentimeters(m_TopXSize));
	m_TopZOffset	=Units_FRound(Units_EngineToCentimeters(m_TopZOffset));
	m_TopZSize		=Units_FRound(Units_EngineToCentimeters(m_TopZSize));
	m_YSize			=Units_FRound(Units_EngineToCentimeters(m_YSize));
	m_RingLength	=Units_FRound(Units_EngineToCentimeters(m_RingLength));
}


void CCreateCylDialog::Serialize(CArchive& ar) 
{
	if (ar.IsStoring())
	{	
		ar << m_BotXOffset;
		ar << m_BotXSize;
		ar << m_BotZOffset;
		ar << m_BotZSize;
		ar << m_Solid;
		ar << m_Thickness;
		ar << m_TopXOffset;
		ar << m_TopXSize;
		ar << m_TopZOffset;
		ar << m_TopZSize;
		ar << m_VerticalStripes;
		ar << m_YSize;
	}
	else
	{	// loading code
		ar >> m_BotXOffset;
		ar >> m_BotXSize;
		ar >> m_BotZOffset;
		ar >> m_BotZSize;
		ar >> m_Solid;
		ar >> m_Thickness;
		ar >> m_TopXOffset;
		ar >> m_TopXSize;
		ar >> m_TopZOffset;
		ar >> m_TopZSize;
		ar >> m_VerticalStripes;
		ar >> m_YSize;
	}
}

void CCreateCylDialog::OnDefaults() 
{
	m_BotXOffset = 0.0;
	m_BotXSize = 128.0;
	m_BotZOffset = 0.0;
	m_BotZSize = 128.0;
	m_Solid = 0;
	m_Thickness = 16.0;
	m_TopXOffset = 0.0;
	m_TopXSize = 128.0;
	m_TopZOffset = 0.0;
	m_TopZSize = 128.0;
	m_VerticalStripes = 6;
	m_YSize = 512.0;
	UpdateData(FALSE);
}
/* EOF: CreateCylDialog.cpp */
void CCreateCylDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_ConvertToMetric)
	{
		dlgFieldsToTexels();
	}

	m_pCylTemplate->BotXOffset	= m_BotXOffset;
	m_pCylTemplate->BotXSize	= m_BotXSize;
	m_pCylTemplate->BotZOffset	= m_BotZOffset;
	m_pCylTemplate->BotZSize	= m_BotZSize;
	m_pCylTemplate->Solid		= m_Solid;
	m_pCylTemplate->Thickness	= m_Thickness;
	m_pCylTemplate->TopXOffset	= m_TopXOffset;
	m_pCylTemplate->TopXSize	= m_TopXSize;
	m_pCylTemplate->TopZOffset	= m_TopZOffset;
	m_pCylTemplate->TopZSize	= m_TopZSize;
	m_pCylTemplate->VerticalStripes	= m_VerticalStripes;
	m_pCylTemplate->YSize		= m_YSize;
	m_pCylTemplate->RingLength	= m_RingLength;
	m_pCylTemplate->TCut		= m_TCut;
	
	CDialog::OnOK();
}
