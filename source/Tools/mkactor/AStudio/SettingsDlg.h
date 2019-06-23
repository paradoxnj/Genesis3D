/****************************************************************************************/
/*  SETTINGSDLG.H																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio project settings dialog.									*/
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
#if !defined(AFX_SETTINGSDLG_H__8CB97C20_69BC_11D2_B69D_004005424FA9__INCLUDED_)
#define AFX_SETTINGSDLG_H__8CB97C20_69BC_11D2_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SettingsDlg.h : header file
//
#include "AOptions.h"

/////////////////////////////////////////////////////////////////////////////
// CSettingsDlg dialog

class CSettingsDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CSettingsDlg)

// Construction
public:
	CSettingsDlg();
	~CSettingsDlg();

	void SetOptionsPointer (AOptions *pOptions) {Options = pOptions;}
	void SetCompileStatus (bool Status);

// Dialog Data
	//{{AFX_DATA(CSettingsDlg)
	enum { IDD = IDD_SETTINGS };
	CSpinButtonCtrl	m_SpinOptLevel;
	CEdit	m_EditOptLevel;
	CString	m_3DSMaxPath;
	CString	m_ViewerPath;
	int		m_OptLevel;
	BOOL	m_OptCheck;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSettingsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSettingsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse3dsmax();
	afx_msg void OnBrowseviewer();
	afx_msg void OnDestroy();
	afx_msg void OnOptcheck();
	afx_msg void OnDeltaposSpinoptlevel(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnSetActive ();
	virtual BOOL OnKillActive ();

private :
	AOptions	*Options;
	bool		m_Compiling;
	void		EnableControls ();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SETTINGSDLG_H__8CB97C20_69BC_11D2_B69D_004005424FA9__INCLUDED_)
