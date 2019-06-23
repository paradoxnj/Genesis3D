/****************************************************************************************/
/*  SkyDialog.h                                                                         */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Genesis world editor header file                                      */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#if !defined(AFX_SKYDIALOG_H__EDBC3365_E973_11D1_B69D_004005424FA9__INCLUDED_)
#define AFX_SKYDIALOG_H__EDBC3365_E973_11D1_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SkyDialog.h : header file
//

#include "resource.h"
class CFusionDoc;
class CFusionTabControls;

/////////////////////////////////////////////////////////////////////////////
// CSkyDialog dialog

class CSkyDialog : public CDialog
{
// Construction
public:
	CSkyDialog(CFusionTabControls *pParent, CFusionDoc *pDoc);
	~CSkyDialog (void);
	void Update (CFusionDoc *pDoc);

// Dialog Data
	//{{AFX_DATA(CSkyDialog)
	enum { IDD = IDD_SKYDIALOG };
	CEdit	m_EditSpeed;
	CButton	m_SkyTop;
	CButton	m_SkyRight;
	CButton	m_SkyLeft;
	CButton	m_SkyFront;
	CButton	m_SkyBottom;
	CButton	m_SkyBack;
	CComboBox	m_SkyTopCombo;
	CComboBox	m_SkyRightCombo;
	CComboBox	m_SkyLeftCombo;
	CComboBox	m_SkyFrontCombo;
	CComboBox	m_SkyBottomCombo;
	CComboBox	m_SkyBackCombo;
	int		m_RotationAxis;
	float	m_RotationSpeed;
	float	m_TextureScale;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSkyDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSkyDialog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSkyback();
	afx_msg void OnSkybottom();
	afx_msg void OnSkyfront();
	afx_msg void OnSkyleft();
	afx_msg void OnSkyright();
	afx_msg void OnSkytop();
	afx_msg void OnSelchangeCbskyleft();
	afx_msg void OnSelchangeCbskyright();
	afx_msg void OnSelchangeCbskytop();
	afx_msg void OnSelchangeCbskybottom();
	afx_msg void OnSelchangeCbskyfront();
	afx_msg void OnSelchangeCbskyback();
	afx_msg void OnKillfocusEditspeed();
	afx_msg void OnAxisButton();
	afx_msg void OnKillfocusEditscale();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CFusionTabControls	*m_pParentCtrl;
	CFusionDoc			*m_pFusionDoc;
	void UpdateSkyFaceUI (CButton &FaceButton, CComboBox &FaceCombo, int FaceIndex);
	void UpdateSkyFaceInfo (CButton &FaceButton, CComboBox &FaceCombo, int FaceIndex);
	void SetSkyFaceUI (CButton &FaceButton, CComboBox &FaceCombo, int FaceIndex);
	int FaceNameInCombo (char const *TextureName, CComboBox &FaceCombo);
	void UpdateFaceTextureName (CComboBox &FaceCombo, int FaceIndex);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SKYDIALOG_H__EDBC3365_E973_11D1_B69D_004005424FA9__INCLUDED_)
