/****************************************************************************************/
/*  SkyDialog.cpp                                                                       */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Sky settings                                                          */
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
#include "SkyDialog.h"
#include "wadfile.h"
#include "fusiondoc.h"
#include "fusiontabcontrols.h"
#include "ram.h"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSkyDialog dialog


CSkyDialog::CSkyDialog(CFusionTabControls *pParent, CFusionDoc *pDoc)
	: CDialog(CSkyDialog::IDD, (CWnd*)pParent)
{
	//{{AFX_DATA_INIT(CSkyDialog)
	m_RotationAxis = -1;
	m_RotationSpeed = 0.0f;
	m_TextureScale = 0.0f;
	//}}AFX_DATA_INIT
	m_pFusionDoc = pDoc;
	m_pParentCtrl = pParent;

	CDialog::Create (IDD, (CWnd*)pParent);
}

CSkyDialog::~CSkyDialog ()
{
}

void CSkyDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSkyDialog)
	DDX_Control(pDX, IDC_EDITSPEED, m_EditSpeed);
	DDX_Control(pDX, IDC_SKYTOP, m_SkyTop);
	DDX_Control(pDX, IDC_SKYRIGHT, m_SkyRight);
	DDX_Control(pDX, IDC_SKYLEFT, m_SkyLeft);
	DDX_Control(pDX, IDC_SKYFRONT, m_SkyFront);
	DDX_Control(pDX, IDC_SKYBOTTOM, m_SkyBottom);
	DDX_Control(pDX, IDC_SKYBACK, m_SkyBack);
	DDX_Control(pDX, IDC_CBSKYTOP, m_SkyTopCombo);
	DDX_Control(pDX, IDC_CBSKYRIGHT, m_SkyRightCombo);
	DDX_Control(pDX, IDC_CBSKYLEFT, m_SkyLeftCombo);
	DDX_Control(pDX, IDC_CBSKYFRONT, m_SkyFrontCombo);
	DDX_Control(pDX, IDC_CBSKYBOTTOM, m_SkyBottomCombo);
	DDX_Control(pDX, IDC_CBSKYBACK, m_SkyBackCombo);
	DDX_Radio(pDX, IDC_AXISX, m_RotationAxis);
	DDX_Text(pDX, IDC_EDITSPEED, m_RotationSpeed);
	DDV_MinMaxFloat(pDX, m_RotationSpeed, 0.f, 3600.f);
	DDX_Text(pDX, IDC_EDITSCALE, m_TextureScale);
	DDV_MinMaxFloat(pDX, m_TextureScale, 0.f, 255.f);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSkyDialog, CDialog)
	//{{AFX_MSG_MAP(CSkyDialog)
	ON_BN_CLICKED(IDC_SKYBACK, OnSkyback)
	ON_BN_CLICKED(IDC_SKYBOTTOM, OnSkybottom)
	ON_BN_CLICKED(IDC_SKYFRONT, OnSkyfront)
	ON_BN_CLICKED(IDC_SKYLEFT, OnSkyleft)
	ON_BN_CLICKED(IDC_SKYRIGHT, OnSkyright)
	ON_BN_CLICKED(IDC_SKYTOP, OnSkytop)
	ON_CBN_SELCHANGE(IDC_CBSKYLEFT, OnSelchangeCbskyleft)
	ON_CBN_SELCHANGE(IDC_CBSKYRIGHT, OnSelchangeCbskyright)
	ON_CBN_SELCHANGE(IDC_CBSKYTOP, OnSelchangeCbskytop)
	ON_CBN_SELCHANGE(IDC_CBSKYBOTTOM, OnSelchangeCbskybottom)
	ON_CBN_SELCHANGE(IDC_CBSKYFRONT, OnSelchangeCbskyfront)
	ON_CBN_SELCHANGE(IDC_CBSKYBACK, OnSelchangeCbskyback)
	ON_EN_KILLFOCUS(IDC_EDITSPEED, OnKillfocusEditspeed)
	ON_BN_CLICKED(IDC_AXISX, OnAxisButton)
	ON_BN_CLICKED(IDC_AXISY, OnAxisButton)
	ON_BN_CLICKED(IDC_AXISZ, OnAxisButton)
	ON_EN_KILLFOCUS(IDC_EDITSCALE, OnKillfocusEditscale)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSkyDialog message handlers

BOOL CSkyDialog::OnInitDialog() 
{
	RECT rect;
	RECT rect2;

	CDialog::OnInitDialog();

	m_pParentCtrl->GetClientRect (&rect);
	m_pParentCtrl->GetItemRect( 0, &rect2 );
	rect.top = rect2.bottom + FTC_BORDER_SIZE_TOP;
	rect.left = rect.left + FTC_BORDER_SIZE_LEFT;
	rect.right = rect.right - FTC_BORDER_SIZE_RIGHT ;
	rect.bottom = rect.bottom - FTC_BORDER_SIZE_BOTTOM ;

	SetWindowPos( NULL, rect.left,
			rect.top, rect.right, rect.bottom, SWP_NOZORDER );

	Update (m_pFusionDoc);	
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSkyDialog::UpdateSkyFaceInfo (CButton &FaceButton, CComboBox &FaceCombo, int FaceIndex)
{
	CWadFile *pWad;

	FaceCombo.ResetContent ();
	pWad = Level_GetWadFile (m_pFusionDoc->pLevel);
	for (int index = 0; index < pWad->mBitmapCount; ++index)
	{
		CString Name = pWad->mBitmaps[index].Name;
		FaceCombo.AddString (Name);
	}
	SetSkyFaceUI (FaceButton, FaceCombo, FaceIndex);
}

void CSkyDialog::Update (CFusionDoc *pDoc)
{
	m_pFusionDoc = pDoc;
	geVec3d Axis;
//	SkyFaceTexture *SkyInfo = 
	Level_GetSkyInfo (m_pFusionDoc->pLevel, &Axis, &m_RotationSpeed, &m_TextureScale);

	if (Axis.X != 0.0f) 
	{
		m_RotationAxis = 0;
	}
	else if (Axis.Y != 0.0f) 
	{
		m_RotationAxis = 1;
	}
	else
	{
		m_RotationAxis = 2;
	}

	UpdateSkyFaceInfo (m_SkyLeft, m_SkyLeftCombo, SkyFace_Left);
	UpdateSkyFaceInfo (m_SkyRight, m_SkyRightCombo, SkyFace_Right);
	UpdateSkyFaceInfo (m_SkyTop, m_SkyTopCombo, SkyFace_Top);
	UpdateSkyFaceInfo (m_SkyBottom, m_SkyBottomCombo, SkyFace_Bottom);
	UpdateSkyFaceInfo (m_SkyFront, m_SkyFrontCombo, SkyFace_Front);
	UpdateSkyFaceInfo (m_SkyBack, m_SkyBackCombo, SkyFace_Back);
	UpdateData (FALSE);
}

int CSkyDialog::FaceNameInCombo (char const *TextureName, CComboBox &FaceCombo)
{
	if (TextureName == NULL)
	{
		return CB_ERR;
	}
	return FaceCombo.FindStringExact (0, TextureName);
}

void CSkyDialog::SetSkyFaceUI (CButton &FaceButton, CComboBox &FaceCombo, int FaceIndex)
{
	if (m_pFusionDoc != NULL)
	{
		int Index;
		SkyFaceTexture *SkyFaces;
		geVec3d Axis;
		geFloat Speed, Scale;

		SkyFaces = Level_GetSkyInfo (m_pFusionDoc->pLevel, &Axis, &Speed, &Scale);

		// If the texture name is in the combo, then select that item.
		// Otherwise select -1
		Index = FaceNameInCombo (SkyFaces[FaceIndex].TextureName, FaceCombo);
		FaceCombo.SetCurSel (Index);
		FaceButton.SetCheck ((SkyFaces[FaceIndex].Apply) ? 1 : 0);
		FaceCombo.EnableWindow (SkyFaces[FaceIndex].Apply);
	}
}

void CSkyDialog::UpdateSkyFaceUI (CButton &FaceButton, CComboBox &FaceCombo, int FaceIndex)
{
	BOOL Checked;
	Checked = (FaceButton.GetCheck() == 1);
	FaceCombo.EnableWindow (Checked);
	if (m_pFusionDoc != NULL)
	{
		SkyFaceTexture *SkyFaces;
		geVec3d Axis;
		geFloat Speed, Scale;

		SkyFaces = Level_GetSkyInfo (m_pFusionDoc->pLevel, &Axis, &Speed, &Scale);
		SkyFaces[FaceIndex].Apply = Checked;
	}
}

void CSkyDialog::OnSkyback() 
{
	UpdateSkyFaceUI (m_SkyBack, m_SkyBackCombo, SkyFace_Back);
}

void CSkyDialog::OnSkybottom() 
{
	UpdateSkyFaceUI (m_SkyBottom, m_SkyBottomCombo, SkyFace_Bottom);
}

void CSkyDialog::OnSkyfront() 
{
	UpdateSkyFaceUI (m_SkyFront, m_SkyFrontCombo, SkyFace_Front);
}

void CSkyDialog::OnSkyleft() 
{
	UpdateSkyFaceUI (m_SkyLeft, m_SkyLeftCombo, SkyFace_Left);
}

void CSkyDialog::OnSkyright() 
{
	UpdateSkyFaceUI (m_SkyRight, m_SkyRightCombo, SkyFace_Right);
}

void CSkyDialog::OnSkytop() 
{
	UpdateSkyFaceUI (m_SkyTop, m_SkyTopCombo, SkyFace_Top);
}

void CSkyDialog::UpdateFaceTextureName (CComboBox &FaceCombo, int FaceIndex)
{
	int Index;

	assert (FaceIndex >= 0);
	assert (FaceIndex < 6);

	Index = FaceCombo.GetCurSel ();
	if (Index != CB_ERR)
	{
		CString FaceString;

		FaceCombo.GetLBText (Index, FaceString);
		if (m_pFusionDoc != NULL)
		{
			char *pFaceName;
			SkyFaceTexture *SkyFaces;
			geVec3d Axis;
			geFloat Speed, Scale;
	
			SkyFaces = Level_GetSkyInfo (m_pFusionDoc->pLevel, &Axis, &Speed, &Scale);
			pFaceName = SkyFaces[FaceIndex].TextureName;
			if (pFaceName != NULL)
			{
				geRam_Free (pFaceName);
				pFaceName = NULL;
			}
			pFaceName = Util_Strdup (FaceString);
			SkyFaces[FaceIndex].TextureName = pFaceName;
		}
	}
}

	
void CSkyDialog::OnSelchangeCbskyleft() 
{
	UpdateFaceTextureName (m_SkyLeftCombo, SkyFace_Left);
}

void CSkyDialog::OnSelchangeCbskyright() 
{
	UpdateFaceTextureName (m_SkyRightCombo, SkyFace_Right);
}

void CSkyDialog::OnSelchangeCbskytop() 
{
	UpdateFaceTextureName (m_SkyTopCombo, SkyFace_Top);
}

void CSkyDialog::OnSelchangeCbskybottom() 
{
	UpdateFaceTextureName (m_SkyBottomCombo, SkyFace_Bottom);
}

void CSkyDialog::OnSelchangeCbskyfront() 
{
	UpdateFaceTextureName (m_SkyFrontCombo, SkyFace_Front);
}

void CSkyDialog::OnSelchangeCbskyback() 
{
	UpdateFaceTextureName (m_SkyBackCombo, SkyFace_Back);
}

static const geVec3d AxisVectors[3] =
	{
		{1.0f, 0.0f, 0.0f},
		{0.0f, 1.0f, 0.0f},
		{0.0f, 0.0f, 1.0f}
	};

void CSkyDialog::OnAxisButton() 
{
	UpdateData (TRUE);
	Level_SetSkyRotationAxis (m_pFusionDoc->pLevel, &AxisVectors[m_RotationAxis]);
}

void CSkyDialog::OnKillfocusEditspeed() 
{
	UpdateData (TRUE);

	Level_SetSkyRotationSpeed (m_pFusionDoc->pLevel, m_RotationSpeed);
}

void CSkyDialog::OnKillfocusEditscale() 
{
	UpdateData (TRUE);

	Level_SetSkyTextureScale(m_pFusionDoc->pLevel, m_RotationSpeed);
}
