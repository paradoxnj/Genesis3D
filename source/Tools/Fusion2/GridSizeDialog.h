/****************************************************************************************/
/*  GridSizeDialog.h                                                                    */
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
#ifndef GRIDSIZEDIALOG_H
#define GRIDSIZEDIALOG_H

#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CGridSizeDialog dialog

class CGridSizeDialog : public CDialog
{
// Construction
public:
	CGridSizeDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CGridSizeDialog)
	enum { IDD = IDD_GRIDSETTINGSDIALOG };
	double	m_SnapDegrees;
	BOOL	m_UseSnap;
	int		MetricOrTexelSnap;
	int		MetricOrTexelGrid;
	int		TexelGridUnits;
	int		m_GridUnits;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGridSizeDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGridSizeDialog)
	afx_msg void OnSnap15();
	afx_msg void OnSnap30();
	afx_msg void OnSnap45();
	afx_msg void OnSnap60();
	virtual BOOL OnInitDialog();
	afx_msg void OnUsertosnap();
	afx_msg void OnSnap90();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
