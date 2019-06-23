/****************************************************************************************/
/*  MOTIONSDLG.H																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor studio motions dialog handler.									*/
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
#if !defined(AFX_MOTIONSDLG_H__8A56D845_640F_11D2_B69D_004005424FA9__INCLUDED_)
#define AFX_MOTIONSDLG_H__8A56D845_640F_11D2_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MotionsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMotionsDlg dialog
#include "PropPage.h"
#include "resource.h"


class CMotionsDlg : public CAStudioPropPage
{
	DECLARE_DYNCREATE(CMotionsDlg)

// Construction
public:
	CMotionsDlg();
	~CMotionsDlg();

	virtual void SetProjectPointer (AProject *Project);
	virtual void GetDialogData ();
	virtual void SetCompileStatus (bool Status);
// Dialog Data
	//{{AFX_DATA(CMotionsDlg)
	enum { IDD = IDD_MOTIONS };
	CEdit	m_EditBone;
	CSpinButtonCtrl	m_SpinOptLevel;
	CEdit	m_EditOptLevel;
	CEdit	m_EditMotionFilename;
	CButton	m_BrowseMotion;
	CButton	m_MotionDefault;
	CButton	m_DeleteMotion;
	CListBox	m_MotionsList;
	CString	m_MotionFilename;
	int		m_MotionFormat;
	int		m_OptLevel;
	BOOL	m_Optimize;
	CString	m_BoneName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMotionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMotionsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangeMotionslist();
	afx_msg void OnAddmotion();
	afx_msg void OnDeletemotion();
	afx_msg void OnMotiondefault();
	afx_msg void OnChangeEditmotion();
	afx_msg void OnBrowsemotion();
	afx_msg void OnMotionType();
	afx_msg void OnDeltaposSpinoptimize(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnOptimize();
	afx_msg void OnKillfocusEditmotion();
	afx_msg void OnDropFiles( HDROP hDropInfo );
	afx_msg void OnChangeEditbone();
	afx_msg void OnRenamemotion();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnSetActive ();
	virtual BOOL OnKillActive ();

private:
	bool	m_FilenameChanged;
	int		m_CurrentIndex;

	void	FillListBox ();
	void	SetupCurrentItem (int Item);
	int		GetCurrentMotionIndex ();
	void	SetMotionFromDialog ();
	void	SetMotionTypeFromFilename ();
	void	EnableControls ();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MOTIONSDLG_H__8A56D845_640F_11D2_B69D_004005424FA9__INCLUDED_)
