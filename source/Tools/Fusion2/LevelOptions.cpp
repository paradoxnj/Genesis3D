/****************************************************************************************/
/*  LevelOptions.cpp                                                                    */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Saves settings for levels                                             */
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
#include "fusion.h"
#include "LevelOptions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLevelOptions dialog


CLevelOptions::CLevelOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CLevelOptions::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLevelOptions)
	m_DrawScale = 0.0f;
	m_LightmapScale = 0.0f;
	m_TextureLib = _T("");
	m_TxlChanged = false;
	m_HeadersDir = _T("");
	//}}AFX_DATA_INIT
}


void CLevelOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLevelOptions)
	DDX_Text(pDX, IDC_DRAWSCALE, m_DrawScale);
	DDX_Text(pDX, IDC_LIGHTMAPSCALE, m_LightmapScale);
	DDX_Text(pDX, IDC_EDITTXL, m_TextureLib);
	DDX_Text(pDX, IDC_EDITHEADERSDIR, m_HeadersDir);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CLevelOptions, CDialog)
	//{{AFX_MSG_MAP(CLevelOptions)
	ON_BN_CLICKED(IDC_BROWSETXL, OnBrowsetxl)
	ON_EN_CHANGE(IDC_EDITTXL, OnChangeEdittxl)
	ON_EN_CHANGE(IDC_EDITHEADERSDIR, OnChangeEditheadersdir)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLevelOptions message handlers

void CLevelOptions::OnBrowsetxl() 
{
	CFileDialog FileDlg (TRUE, "txl", m_TextureLib, OFN_FILEMUSTEXIST, "Texture Library Files (*.txl)|*.txl||");
	FileDlg.m_ofn.lpstrTitle	="Open";

	if (FileDlg.DoModal () == IDOK)
	{
		m_TxlChanged = true;
		m_TextureLib = FileDlg.GetPathName ();
		UpdateData (FALSE);
	}
}


void CLevelOptions::OnChangeEdittxl() 
{
	m_TxlChanged = true;
}

void CLevelOptions::OnChangeEditheadersdir() 
{
	m_HeadersChanged = true;
}
