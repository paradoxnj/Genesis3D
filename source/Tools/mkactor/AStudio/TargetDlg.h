/****************************************************************************************/
/*  TARGETDLG.H																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio build target dialog.										*/
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
#if !defined(AFX_TARGETDLG_H__8A56D847_640F_11D2_B69D_004005424FA9__INCLUDED_)
#define AFX_TARGETDLG_H__8A56D847_640F_11D2_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TargetDlg.h : header file
//

#include "PropPage.h"
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CTargetDlg dialog

class CTargetDlg : public CAStudioPropPage
{
	DECLARE_DYNCREATE(CTargetDlg)

// Construction
public:
	CTargetDlg();
	~CTargetDlg();

	virtual void SetProjectPointer (AProject *Project);
	virtual void GetDialogData ();
	virtual void SetCompileStatus (bool bStatus);

// Dialog Data
	//{{AFX_DATA(CTargetDlg)
	enum { IDD = IDD_TARGET };
	CEdit	m_EditTarget;
	CString	m_TargetName;
	int		m_OutputFormat;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTargetDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTargetDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowsetarget();
	afx_msg void OnChangeEdittarget();
	afx_msg void OnFormatChanged();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnSetActive ();
	virtual BOOL OnKillActive ();

private:
	bool	m_FilenameChanged;
	void	SetDialogData (void);
	void	EnableControls ();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TARGETDLG_H__8A56D847_640F_11D2_B69D_004005424FA9__INCLUDED_)
