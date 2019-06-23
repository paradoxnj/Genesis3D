/****************************************************************************************/
/*  PATHSDLG.H																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio paths dialog.												*/
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
#if !defined(AFX_PATHSDLG_H__8A56D846_640F_11D2_B69D_004005424FA9__INCLUDED_)
#define AFX_PATHSDLG_H__8A56D846_640F_11D2_B69D_004005424FA9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PathsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPathsDlg dialog
#include "PropPage.h"
#include "resource.h"

class CPathsDlg : public CAStudioPropPage
{
	DECLARE_DYNCREATE(CPathsDlg)

// Construction
public:
	CPathsDlg();
	~CPathsDlg();
	virtual void SetProjectPointer (AProject *Project);
	virtual void GetDialogData ();
	virtual void SetCompileStatus (bool Status);

// Dialog Data
	//{{AFX_DATA(CPathsDlg)
	enum { IDD = IDD_PATHS };
	CString	m_MaterialsPath;
	CString	m_TempFilesDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPathsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPathsDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditmaterialspath();
	afx_msg void OnChangeEdittempfilesdir();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnSetActive ();
	virtual BOOL OnKillActive ();

private:
	void SetDialogData ();
	void EnableControls ();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PATHSDLG_H__8A56D846_640F_11D2_B69D_004005424FA9__INCLUDED_)
