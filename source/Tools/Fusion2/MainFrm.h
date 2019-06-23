/****************************************************************************************/
/*  MainFrm.h                                                                           */
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

#ifndef	MAINFRM_H
#define MAINFRM_H

#include "brush.h"

class CFusionTabControls;
class CModelDialog;
class CFusionDoc;

class CStyleBar : public CToolBar
{
public:
	CComboBox   m_comboBox;
	CFont       m_font;
};

#define MAX_GRID_STRING	16
#define MAX_SNAP_STRING 16

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)

	char	szGridString[MAX_GRID_STRING] ;
	char	szSnapString[MAX_SNAP_STRING] ;

public:
	BOOL	IsDestroyingApp;
	int		IsStartingApp;
	const char *GetCurrentTexture();

	//	Clipboard registration format numbers...
	UINT m_CB_FUSION_BRUSH_FORMAT;
	UINT m_CB_FUSION_ENTITY_FORMAT;

	CWnd				*GetTabControlWindow();
	CFusionTabControls	*m_wndTabControls;
	CFont				TabFont ;

//	CModelDialog		*pModelDialog;

	BOOL CreateTabBar();
	void UpdateGridSize(geFloat GridSize, int SnapOn, int snapto, int gunits, int snapunits);
	CFrameWnd* CreateNewGameViewFrame(CRuntimeClass* pViewClass,  CDocTemplate* pTemplate, CDocument* pDoc, CFrameWnd* pOther);
	void MakeNewView( CRuntimeClass* pViewRuntimeClass);
	void LoadComboBox();
	BOOL CreateStyleBar();
	BOOL CreateGroupBar() ;
	CMainFrame();
	void SelectTab( int nTabIndex ) ;
	CFusionDoc *GetCurrentDoc (void);
	void UpdateActiveDoc (void);
	void UpdateModelsDialog (void);
	void UpdateSelectedModel (int MoveRotate, geVec3d const *pVecDelta);
	CModelDialog *GetModelTab (void);
	void DockControlBarLeftOf(CToolBar* Bar,CToolBar* LeftOf) ;

public:
	CStatusBar  m_wndStatusBar;  // s/b protected, but I'm hacking

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CToolBar    m_wndToolBar;
	CStyleBar	m_wndBrushToolBar;
	CStyleBar	m_wndGroupBar ;

	//	CHANGE!	04/03/97	John Moore
	//	This is the dockable control bar for our tabs...
	CDialogBar m_wndTabBar;
	//	End of CHANGE

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnWindowNew();
	afx_msg void OnUpdateSLock(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSelInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateWorldPos(CCmdUI *pCmdUI);
	afx_msg void OnUpdateCursorInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateGridInfo(CCmdUI *pCmdUI);
	afx_msg void OnUpdateSnapInfo(CCmdUI *pCmdUI);
	afx_msg void OnBrushGroupsMakenewgroup();
	afx_msg void OnModebar();
	afx_msg void OnUpdateModebar(CCmdUI* pCmdUI);
	afx_msg void OnGroupbar();
	afx_msg void OnUpdateGroupbar(CCmdUI* pCmdUI);
	afx_msg void OnTabbar();
	afx_msg void OnUpdateTabbar(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnViewLeakFinder();
	afx_msg void OnUpdateViewLeakFinder(CCmdUI* pCmdUI);
	afx_msg void OnSelchangeGroupList ();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );

private:
};

/////////////////////////////////////////////////////////////////////////////

#endif
