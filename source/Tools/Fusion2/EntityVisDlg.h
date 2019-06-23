/****************************************************************************************/
/*  EntityVisDlg.h                                                                      */
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
#if !defined(AFX_ENTITYVISDLG_H__9B251C00_C7BA_11D1_B69D_004005424FA9__INCLUDED_)
#define AFX_ENTITYVISDLG_H__9B251C00_C7BA_11D1_B69D_004005424FA9__INCLUDED_

#include "EntView.h"

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// EntityVisDlg.h : header file
//

class CMyCheckListBox : public CCheckListBox
{
};

/////////////////////////////////////////////////////////////////////////////
// CEntityVisDlg dialog

class CEntityVisDlg : public CDialog
{
// Construction
public:
	CEntityVisDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEntityVisDlg)
	enum { IDD = IDD_ENTITYVISIBILITY };
	CMyCheckListBox	m_EntsList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntityVisDlg)
	public:
	virtual int DoModal(EntityViewList *pList);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEntityVisDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	EntityViewList *pViewList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ENTITYVISDLG_H__9B251C00_C7BA_11D1_B69D_004005424FA9__INCLUDED_)
