/****************************************************************************************/
/*  PROPSHEET.H																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio main dialog.												*/
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
#ifndef PROPSHEET_H
#define PROPSHEET_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "TargetDlg.h"
#include "BodyDlg.h"
#include "MaterialsDlg.h"
#include "MotionsDlg.h"
#include "PathsDlg.h"
#include "AProject.h"
#include "SettingsDlg.h"
#include "AOptions.h"
#include "LogoPage.h"

class CAStudioPropSheet : public CPropertySheet
{
public:
	DECLARE_DYNAMIC(CAStudioPropSheet)
	CAStudioPropSheet(CWnd* pWndParent, AProject **pProject, AOptions *pOptions, const char *ProjectFilename);
	~CAStudioPropSheet ();

// Overrides
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
// Message Handlers
protected:
	//{{AFX_MSG(CAStudioPropSheet)
	afx_msg int OnCreate (LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy ();
	afx_msg void OnPaint ();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSysCommand( UINT nID, LPARAM lParam );
	afx_msg void OnFileNew ();
	afx_msg void OnFileOpen ();
	afx_msg void OnFileSave ();
	afx_msg void OnProjectBuild ();
	afx_msg void OnProjectBuildAll ();
	afx_msg void OnProjectClean ();
	afx_msg void OnProjectActorSummary ();
	afx_msg void OnFileExit ();
	afx_msg void OnDummyEsc ();
	afx_msg void OnMessages ();
	afx_msg void OnCompileMessage (UINT wParam, LONG lParam);
	afx_msg void OnCompileDone (UINT wParam, LONG lParam);
	// Update UI handlers
	afx_msg void OnUpdateFileNew (CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileOpen (CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave (CCmdUI* pCmdUI);
	afx_msg void OnUpdateProjectBuild (CCmdUI* pCmdUI);
	afx_msg void OnUpdateProjectBuildAll (CCmdUI* pCmdUI);
	afx_msg void OnUpdateProjectClean (CCmdUI* pCmdUI);
	afx_msg void OnUpdateProjectActorSummary (CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	HICON			m_hIcon;	// program icon
	HACCEL			m_hAccel;	// menu accelerators

	bool			m_FileLoaded;	// false at program startup until file
									// loaded or new file created
	bool			m_Compiling;	// true if currently compiling
	// Property sheet pages.
	// Automatically constructed
	CTargetDlg		m_TargetPage;
	CBodyDlg		m_BodyPage;
	CMaterialsDlg	m_MaterialsPage;
	CMotionsDlg		m_MotionsPage;
	CPathsDlg		m_PathsPage;
	CSettingsDlg	m_SettingsPage;
	CLogoPage		m_LogoPage;

	bool			m_Modified;
	bool			m_NewFile;
	bool			m_MessagesVisible;
	CString			m_Filename;
	CButton			m_MessagesBtn;
	CButton			m_BuildBtn;

	CEdit			m_EditMessages;		// messages window
	int				m_Width;			// dialog width
	int				m_NormalHeight;
	int				m_ExpandedHeight;

	// Parent passes a pointer-to-pointer to structure so we can return it.
	// But we use m_Project.
	AProject		**m_pProject;	// For communication to parent
	AProject		*m_Project;		// Project options structure
	AOptions		*m_Options;

	void	AddAButton (int ButtonTextId, int ButtonId, CButton &TheButton, const RECT &BtnRect);
	void	ReplaceAButton (int ButtonTextId, int ReplaceButtonId, int ButtonId, CButton &TheButton);
	bool	CanClose ();
	bool	WasModified ();
	void	SetModifiedFlag (bool flag);
	void	SetupTitle ();
	int		PromptForSave ();
	bool	PromptForSaveFileName (CString &OutputFilename);
	bool	SaveFileAs (const CString &OutputFilename);
	bool	SaveFile ();
	bool	WriteTheDamnFileAlready (const CString &Filename);
	void	AddPropertyPages ();

	void	ForcePageUpdates ();
	void	SetProjectPointer (AProject *Project);
	void	UpdateCurrentPage ();
	void	EnableMessagesWindow (bool Enabled);
	void	SetWorkingDirectory ();
	void	SetCompilingStatus (bool CompileFlag);
};

#endif
