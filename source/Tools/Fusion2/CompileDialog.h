/****************************************************************************************/
/*  CompileDialog.h                                                                     */
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

/////////////////////////////////////////////////////////////////////////////
// CCompileDialog dialog

#ifndef _COMPILEDIALOG_H
#define _COMPILEDIALOG_H

#include "resource.h"
#include "compiler.h"

class CCompileDialog : public CDialog
{
// Construction
public:
	CCompileDialog(CWnd* pParent, CompileParamsType *pParms);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCompileDialog)
	enum { IDD = IDD_COMPILEDIALOG };
	CButton	m_Vis;
	CButton	m_RunGbsp;
	CButton	m_RunLight;
	CButton	m_EntitiesOnly;
	BOOL	m_RunAvenger;
	CString	m_FileName;
	BOOL	Rad;
	int		BounceLimit;
	CString	MinLight;
	CString	ReflectScale;
	int		PatchSize;
	BOOL	UseMinLight;
	BOOL	m_SuppressHidden;
	BOOL	m_VisDetail;
	CString	LightScale;
	BOOL	FastPatch;
	BOOL	ExtraSamp;
	BOOL	m_FullVis;
	BOOL	m_VisVerbose;
	BOOL	m_GbspVerbose;
	BOOL	m_EntityVerbose;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCompileDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCompileDialog)
	afx_msg void OnBrowse();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnEntitiesonly();
	afx_msg void OnRungbsp();
	afx_msg void OnVis();
	afx_msg void OnRunlight();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CompileParamsType *pParms;
	void EnableControls (void);
	void DoEnableWindow (int id, BOOL Enable);
};


#endif
