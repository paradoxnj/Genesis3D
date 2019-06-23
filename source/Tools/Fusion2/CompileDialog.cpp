/****************************************************************************************/
/*  CompileDialog.cpp                                                                   */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Dialog code for compile settings                                      */
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
#include "CompileDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCompileDialog dialog


CCompileDialog::CCompileDialog(CWnd* pParent, CompileParamsType *pParms)
	: CDialog(CCompileDialog::IDD, pParent),
	  pParms(pParms)
{
	//{{AFX_DATA_INIT(CCompileDialog)
	m_RunAvenger = pParms->RunPreview;
	m_FileName = _T(pParms->Filename);
	Rad = pParms->Light.Radiosity;
	BounceLimit = pParms->Light.NumBounce;
	MinLight = _T("");
	ReflectScale = _T("");
	PatchSize = (int)(pParms->Light.PatchSize);
	UseMinLight = pParms->UseMinLight;
	m_SuppressHidden = pParms->SuppressHidden;
	m_VisDetail = pParms->VisDetailBrushes;
	LightScale = _T("");
	FastPatch = pParms->Light.FastPatch;
	ExtraSamp = pParms->Light.ExtraSamples;
	m_FullVis = pParms->Vis.FullVis;
	m_VisVerbose = pParms->Vis.Verbose;
	m_GbspVerbose = pParms->Bsp.Verbose;
	m_EntityVerbose = pParms->Bsp.EntityVerbose;
	//}}AFX_DATA_INIT
	MinLight.Format ("%.2f %.2f %.2f", 
		pParms->Light.MinLight.X, 
		pParms->Light.MinLight.Y, 
		pParms->Light.MinLight.Z); 
	ReflectScale.Format ("%.2f", pParms->Light.ReflectiveScale);
	LightScale.Format ("%.2f", pParms->Light.LightScale);
}


void CCompileDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCompileDialog)
	DDX_Control(pDX, IDC_VIS, m_Vis);
	DDX_Control(pDX, IDC_RUNGBSP, m_RunGbsp);
	DDX_Control(pDX, IDC_RUNLIGHT, m_RunLight);
	DDX_Control(pDX, IDC_ENTITIESONLY, m_EntitiesOnly);
	DDX_Check(pDX, IDC_RunAvenger, m_RunAvenger);
	DDX_Text(pDX, IDC_EDIT1, m_FileName);
	DDV_MaxChars(pDX, m_FileName, 260);
	DDX_Check(pDX, IDC_RADBOOL, Rad);
	DDX_Text(pDX, IDC_BOUNCELIMIT, BounceLimit);
	DDX_Text(pDX, IDC_MINLIGHT, MinLight);
	DDX_Text(pDX, IDC_REFLECTSCALE, ReflectScale);
	DDX_Text(pDX, IDC_PATCHSIZE, PatchSize);
	DDX_Check(pDX, IDC_USEMINLIGHT, UseMinLight);
	DDX_Check(pDX, IDC_SUPPRESSHIDDEN, m_SuppressHidden);
	DDX_Check(pDX, IDC_VISDETAIL, m_VisDetail);
	DDX_Text(pDX, IDC_LIGHTSCALE, LightScale);
	DDX_Check(pDX, IDC_FASTPBOOL, FastPatch);
	DDX_Check(pDX, IDC_EXSAMPBOOL, ExtraSamp);
	DDX_Check(pDX, IDC_FULLVIS, m_FullVis);
	DDX_Check(pDX, IDC_VISVERBOSE, m_VisVerbose);
	DDX_Check(pDX, IDC_BSPVERBOSE, m_GbspVerbose);
	DDX_Check(pDX, IDC_ENTITYVERBOSE, m_EntityVerbose);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCompileDialog, CDialog)
	//{{AFX_MSG_MAP(CCompileDialog)
	ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
	ON_BN_CLICKED(IDC_ENTITIESONLY, OnEntitiesonly)
	ON_BN_CLICKED(IDC_RUNGBSP, OnRungbsp)
	ON_BN_CLICKED(IDC_VIS, OnVis)
	ON_BN_CLICKED(IDC_RUNLIGHT, OnRunlight)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCompileDialog message handlers

//	Browse for a new file...
void CCompileDialog::OnBrowse() 
{
	CString Filename;
	CFileFind Finder;

	Filename = "*.MAP";

	// make the dialog
	CFileDialog dlg(TRUE, "MAP", Filename);

	// show it to the user.
	if( dlg.DoModal() == IDOK )
	{
		UpdateData( TRUE );
		Filename = dlg.GetPathName();
		if( Finder.FindFile( Filename ) )
		{
			m_FileName = Filename;
		}
		UpdateData( FALSE );
	}
}

void CCompileDialog::OnOK() 
{
	CDialog::OnOK();
	
	pParms->EntitiesOnly = (m_EntitiesOnly.GetCheck () == 1);
	pParms->VisDetailBrushes = m_VisDetail;
	pParms->RunPreview = m_RunAvenger;
	pParms->DoLight = (m_RunLight.GetCheck () == 1);
	pParms->DoVis = (m_Vis.GetCheck () == 1);
	pParms->RunBsp = (m_RunGbsp.GetCheck () == 1);
	pParms->UseMinLight = UseMinLight;
	pParms->SuppressHidden = m_SuppressHidden;

	strcpy (pParms->Filename, m_FileName);

	pParms->Light.Radiosity = Rad;
	pParms->Light.FastPatch = FastPatch;
	pParms->Light.ExtraSamples = ExtraSamp;
	pParms->Light.NumBounce = BounceLimit;
	pParms->Light.PatchSize = (float)PatchSize;
	pParms->Light.ReflectiveScale = (float)atof (ReflectScale);
	pParms->Light.LightScale = (float)atof (LightScale);
	{
		float x, y, z;
		if (sscanf (MinLight, "%f %f %f", &x, &y, &z) == 3)
		{
			geVec3d_Set (&(pParms->Light.MinLight), x, y, z);
		}
	}

	pParms->Vis.FullVis = m_FullVis;
	pParms->Vis.Verbose = m_VisVerbose;

	pParms->Bsp.Verbose = m_GbspVerbose;
	pParms->Bsp.EntityVerbose = m_EntityVerbose;
}

void CCompileDialog::DoEnableWindow (int id, BOOL Enable)
{
	CWnd *pWnd;
	
	pWnd = GetDlgItem (id);

	if (pWnd != NULL)
	{
		pWnd->EnableWindow (Enable);
	}
}

void CCompileDialog::EnableControls (void)
{
	BOOL Checked;

	if (m_EntitiesOnly.GetCheck () == 1)
	{
		// disable BSP and VIS
		m_RunGbsp.EnableWindow (FALSE);
		m_Vis.EnableWindow (FALSE);
	}
	else
	{
		// enable BSP and VIS
		m_RunGbsp.EnableWindow (TRUE);
		m_Vis.EnableWindow (TRUE);
	}

	if (m_EntitiesOnly.GetCheck () == 0)
	{

		Checked = (m_RunGbsp.GetCheck () == 1);
		DoEnableWindow (IDC_BSPVERBOSE,	Checked);
		DoEnableWindow (IDC_ENTITYVERBOSE, Checked);

		Checked = (m_Vis.GetCheck () == 1);
		DoEnableWindow (IDC_FULLVIS, Checked);
		DoEnableWindow (IDC_VISVERBOSE, Checked);
	}
	else
	{
		DoEnableWindow (IDC_BSPVERBOSE,	FALSE);
		DoEnableWindow (IDC_ENTITYVERBOSE, FALSE);
		DoEnableWindow (IDC_FULLVIS, FALSE);
		DoEnableWindow (IDC_VISVERBOSE, FALSE);
	}

	Checked = (m_RunLight.GetCheck () == 1);
	DoEnableWindow (IDC_RADBOOL, Checked);
	DoEnableWindow (IDC_EXSAMPBOOL, Checked);
	DoEnableWindow (IDC_FASTPBOOL, Checked);
	DoEnableWindow (IDC_LIGHTSCALE, Checked);
	DoEnableWindow (IDC_REFLECTSCALE, Checked);
	DoEnableWindow (IDC_BOUNCELIMIT, Checked);
	DoEnableWindow (IDC_PATCHSIZE, Checked);
	DoEnableWindow (IDC_USEMINLIGHT, Checked);
	DoEnableWindow (IDC_MINLIGHT, Checked);
}

BOOL CCompileDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_EntitiesOnly.SetCheck (pParms->EntitiesOnly);
	m_RunGbsp.SetCheck (pParms->RunBsp);
	m_RunLight.SetCheck (pParms->DoLight);
	m_Vis.SetCheck (pParms->DoVis);

	EnableControls ();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCompileDialog::OnEntitiesonly() 
{
	EnableControls ();	
}

void CCompileDialog::OnRungbsp() 
{
	EnableControls ();	
}

void CCompileDialog::OnVis() 
{
	EnableControls ();	
}

void CCompileDialog::OnRunlight() 
{
	EnableControls ();	
}
