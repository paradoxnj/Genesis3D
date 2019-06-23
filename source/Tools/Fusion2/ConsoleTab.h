/****************************************************************************************/
/*  ConsoleTab.h                                                                        */
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
#if !defined(AFX_CONSOLETAB_H__F4D701E1_7D42_11D1_9A1D_0000E824D625__INCLUDED_)
#define AFX_CONSOLETAB_H__F4D701E1_7D42_11D1_9A1D_0000E824D625__INCLUDED_

/*
	Okay, this is really ugly.  For the moment we want to have the console functions
	avalailable from C programs.  So we do that stupid #ifdef __cplusplus trick...
*/
#ifdef __cplusplus
	extern "C" {
#endif
//console stuff
//char *TranslateString(char *);
void ConClear(void);
void ConPrintf(char *, ...);
void ConError(char *, ...);	//wanna do this red eventually

#ifdef __cplusplus
	}
#endif


#ifdef __cplusplus

#include "resource.h"

class CFusionDoc;
class CFusionTabControls;

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ConsoleTab.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConsoleTab dialog

class CConsoleTab : public CDialog
{
// Construction
public:
	CFusionTabControls* m_pParentCtrl;
	CFusionDoc* m_pFusionDoc;

	CConsoleTab(CFusionTabControls* pParent = NULL, CFusionDoc* pDoc = NULL);
	void SetConHwnd(void);
// Dialog Data
	//{{AFX_DATA(CConsoleTab)
	enum { IDD = IDD_CONSOLE };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConsoleTab)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConsoleTab)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnMaxtextConedit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	HFONT Font;
	BOOL bNewFont;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif  // __cplusplus

#endif // !defined(AFX_CONSOLETAB_H__F4D701E1_7D42_11D1_9A1D_0000E824D625__INCLUDED_)
