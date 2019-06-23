/****************************************************************************************/
/*  NEWPRJDLG.H																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio new project dialog.										*/
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
#if !defined(AFX_NEWPRJDLG_H__D1299EE6_6D76_11D2_B69D_004005424FA9__INCLUDED_)
#define AFX_NEWPRJDLG_H__D1299EE6_6D76_11D2_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CNewPrjDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNewPrjDlg dialog

class CNewPrjDlg : public CDialog
{
// Construction
public:
	CNewPrjDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNewPrjDlg)
	enum { IDD = IDD_NEWPROJECT };
	CString	m_ProjectName;
	BOOL	m_UseProjectDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNewPrjDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNewPrjDlg)
	afx_msg void OnBrowseproject();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNEWPRJDLG_H__D1299EE6_6D76_11D2_B69D_004005424FA9__INCLUDED_)
