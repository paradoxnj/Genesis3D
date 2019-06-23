/****************************************************************************************/
/*  MATERIALSDLG.H																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor studio materials dialog handler.									*/
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
#if !defined(AFX_MATERIALSDLG_H__8A56D844_640F_11D2_B69D_004005424FA9__INCLUDED_)
#define AFX_MATERIALSDLG_H__8A56D844_640F_11D2_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MaterialsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMaterialsDlg dialog
#include "PropPage.h"
#include "resource.h"


class CMaterialsDlg : public CAStudioPropPage
{
	DECLARE_DYNCREATE(CMaterialsDlg)

// Construction
public:
	CMaterialsDlg();
	~CMaterialsDlg();

	virtual void SetProjectPointer (AProject *Project);
	virtual void GetDialogData ();
	virtual void SetCompileStatus (bool Status);

// Dialog Data
	//{{AFX_DATA(CMaterialsDlg)
	enum { IDD = IDD_MATERIALS };
	CButton	m_BrowseTexture;
	CButton	m_MaterialDefault;
	CButton	m_ChooseColor;
	CEdit	m_EditTextureFilename;
	CButton	m_CheckColor;
	CListBox	m_MaterialsList;
	BOOL	m_ColorFlag;
	CString	m_TextureFilename;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMaterialsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMaterialsDlg)
	afx_msg void OnSelchangeMaterialslist();
	afx_msg void OnAddmaterial();
	afx_msg void OnBrowsetexture();
	afx_msg void OnCheckcolor();
	afx_msg void OnChoosecolor();
	afx_msg void OnChangeEdittexture();
	afx_msg void OnMaterialdefault();
	virtual BOOL OnInitDialog();
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnRenamematerial();
	afx_msg void OnDeletematerial();
	afx_msg void OnKillfocusEdittexture();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnSetActive ();
	virtual BOOL OnKillActive ();

private:
	GE_RGBA m_Color;
	bool	m_FilenameChanged;
	int		m_CurrentIndex;

	void	FillListBox ();
	void	SetupCurrentItem (int Item);
	int		GetCurrentMaterialIndex ();
	void	SetMaterialFromDialog ();
	void	EnableControls ();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MATERIALSDLG_H__8A56D844_640F_11D2_B69D_004005424FA9__INCLUDED_)
