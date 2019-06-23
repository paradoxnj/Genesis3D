/****************************************************************************************/
/*  PreferencesDialog.h                                                                 */
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
#if !defined(AFX_PREFERENCESDIALOG_H__14A85704_E5AB_11D1_A2EA_0000E823F3AA__INCLUDED_)
#define AFX_PREFERENCESDIALOG_H__14A85704_E5AB_11D1_A2EA_0000E823F3AA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PreferencesDialog.h : header file
//
#include "colorbtn.h"


/////////////////////////////////////////////////////////////////////////////
// CPreferencesDialog dialog

class CPreferencesDialog : public CDialog
{
// Construction
public:
	CPreferencesDialog(CWnd* pParent = NULL);   // standard constructor
	COLORREF	coBackground ;
	COLORREF	coGrid ;
	COLORREF	coSnapGrid ;

// Dialog Data
	//{{AFX_DATA(CPreferencesDialog)
	enum { IDD = IDD_PREFERENCES };
	CColorButton	m_SnapGrid;
	CColorButton	m_Grid;
	CColorButton	m_GridBackground;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPreferencesDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnPathprefs();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFERENCESDIALOG_H__14A85704_E5AB_11D1_A2EA_0000E823F3AA__INCLUDED_)
