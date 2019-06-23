/****************************************************************************************/
/*  CreateStaircaseDialog.cpp                                                           */
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
#include "CreateStaircaseDialog.h"
#include "units.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CCreateStaircaseDialog::CCreateStaircaseDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CCreateStaircaseDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCreateStaircaseDialog)
	m_Height = 128.0;
	m_Length = 128.0;
	m_Width = 64.0;
	m_MakeRamp = FALSE;
	m_TCut = FALSE;
	m_NumberOfStairs = 0;
	//}}AFX_DATA_INIT
}


void CCreateStaircaseDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCreateStaircaseDialog)
	DDX_Text(pDX, IDC_HEIGHT, m_Height);
	DDX_Text(pDX, IDC_LENGTH, m_Length);
	DDX_Text(pDX, IDC_WIDTH, m_Width);
	DDX_Check(pDX, IDC_CHECK1, m_MakeRamp);
	DDX_Check(pDX, IDC_TCUT, m_TCut);
	DDX_Text(pDX, IDC_NUMBEROFSTAIRS, m_NumberOfStairs);
	DDV_MinMaxInt(pDX, m_NumberOfStairs, 1, 64);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCreateStaircaseDialog, CDialog)
	//{{AFX_MSG_MAP(CCreateStaircaseDialog)
	ON_BN_CLICKED(ID_DEFAULTS, OnDefaults)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

int	CCreateStaircaseDialog::DoModal(geBoolean ConvertToMetric, BrushTemplate_Staircase *pStairTemplate)
{
	m_ConvertToMetric	=ConvertToMetric;
	m_pStairTemplate = pStairTemplate;

	m_Height		= m_pStairTemplate->Height;
	m_Length		= m_pStairTemplate->Length;
	m_NumberOfStairs = m_pStairTemplate->NumberOfStairs;
	m_Width			= m_pStairTemplate->Width;
	m_MakeRamp		= m_pStairTemplate->MakeRamp;
	m_TCut			= m_pStairTemplate->TCut;

	if(m_ConvertToMetric)
	{
		dlgFieldsToCentimeters();
	}

	return CDialog::DoModal();
}

void	CCreateStaircaseDialog::dlgFieldsToTexels(void)
{
	m_Height	=Units_CentimetersToEngine(m_Height);
	m_Width		=Units_CentimetersToEngine(m_Width);
	m_Length	=Units_CentimetersToEngine(m_Length);
}

void	CCreateStaircaseDialog::dlgFieldsToCentimeters(void)
{
	m_Height	=Units_FRound(Units_EngineToCentimeters(m_Height));
	m_Width		=Units_FRound(Units_EngineToCentimeters(m_Width));
	m_Length	=Units_FRound(Units_EngineToCentimeters(m_Length));
}


void CCreateStaircaseDialog::OnDefaults()
{
	m_Height = 128.0;
	m_Length = 128.0;
	m_NumberOfStairs = 8;
	m_Width = 64.0;
	m_MakeRamp = FALSE;
	m_TCut = FALSE;
	UpdateData( FALSE );
}/* CCreateStaircaseDialog::OnDefaults */

/* EOF: CreateStaircaseDialog */

void CCreateStaircaseDialog::OnOK() 
{
	UpdateData(TRUE);
	if(m_ConvertToMetric)
	{
		dlgFieldsToTexels();
	}

	m_pStairTemplate->Height		= m_Height;
	m_pStairTemplate->Length		= m_Length;
	m_pStairTemplate->NumberOfStairs = m_NumberOfStairs;
	m_pStairTemplate->Width			= m_Width;
	m_pStairTemplate->MakeRamp		= m_MakeRamp;
	m_pStairTemplate->TCut			= m_TCut;
	
	CDialog::OnOK();
}
