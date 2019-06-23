/****************************************************************************************/
/*  KeyEditDlg.h                                                                        */
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
#if !defined(AFX_KEYEDITDLG_H__E9ABF084_A391_11D1_AC11_0060088D6C58__INCLUDED_)
#define AFX_KEYEDITDLG_H__E9ABF084_A391_11D1_AC11_0060088D6C58__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// KeyEditDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CKeyEditDlg dialog
#include "model.h"
#include "resource.h"
#include "entity.h"
#include "EntityTable.h"

class CKeyEditDlg : public CDialog
{
// Construction
public:
	CKeyEditDlg(CWnd* pParent, CString const pKeyS, CString *pValS);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CKeyEditDlg)
	enum { IDD = IDD_KEYEDIT };
	CEdit	m_ValueEdit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CKeyEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CKeyEditDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString const Key;
	CString *pValue;
};

/////////////////////////////////////////////////////////////////////////////
// CPointKeyEditDlg dialog

class CPointKeyEditDlg : public CDialog
{
// Construction
public:
	CPointKeyEditDlg(CWnd* pParent, CString const KeyS, CString *pValS);

// Dialog Data
	//{{AFX_DATA(CPointKeyEditDlg)
	enum { IDD = IDD_POINTKEYEDIT };
	CEdit	m_ZEdit;
	CEdit	m_YEdit;
	CEdit	m_XEdit;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPointKeyEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPointKeyEditDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CString const Key;
	CString *pValue;
};

/////////////////////////////////////////////////////////////////////////////
// CStructKeyEditDlg dialog

class CStructKeyEditDlg : public CDialog
{
// Construction
public:
	CStructKeyEditDlg(CWnd* pParent, CEntity const &aEnt, 
	  CString const KeyS, CEntityArray *Ents, CString *pValS, 
	  const EntityTable *pEntTable);

// Dialog Data
	//{{AFX_DATA(CStructKeyEditDlg)
	enum { IDD = IDD_STRUCTEDIT };
	CListBox	m_StructList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CStructKeyEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CStructKeyEditDlg)
	afx_msg void OnDblclkStructlist();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CEntity const &Ent;
	CString const Key;
	CEntityArray* mEntityArray;
	CString *pValue;
	const EntityTable *m_pEntityTable;
};
/////////////////////////////////////////////////////////////////////////////
// CModelKeyEditDlg dialog

class CModelKeyEditDlg : public CDialog
{
// Construction
public:
	CModelKeyEditDlg::CModelKeyEditDlg(CWnd* pParent, ModelList *pModels, CString const KeyS, CString *pValS);

// Dialog Data
	//{{AFX_DATA(CModelKeyEditDlg)
	enum { IDD = IDD_MODELEDIT };
	CListBox	m_ModelList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModelKeyEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CModelKeyEditDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnDblclkModellist();
	virtual void OnOK();
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	ModelList *Models;
	CString const Key;
	CString *pValue;
};
/////////////////////////////////////////////////////////////////////////////
// CBoolKeyEditDlg dialog

class CBoolKeyEditDlg : public CDialog
{
// Construction
public:
	CBoolKeyEditDlg(CWnd* pParent, CString const KeyS, CString *pValS);

// Dialog Data
	//{{AFX_DATA(CBoolKeyEditDlg)
	enum { IDD = IDD_BOOLEDIT };
	CListBox	m_BoolList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBoolKeyEditDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBoolKeyEditDlg)
	afx_msg void OnDblclkBoollist();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CString const Key;
	CString *pValue;
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.



// CColorKeyEditDlg -- Custom ColorDialog...
class CColorKeyEditDlg : public CColorDialog
{
public:
	CColorKeyEditDlg (CWnd* pParent, CString const KeyS, CString *pValS);
	virtual INT_PTR DoModal();
protected:
	virtual BOOL OnInitDialog ();
private:
	CString const Key;
	CString *pValue;
};


class CIntKeyEditDlg : public CKeyEditDlg
{
public:
	CIntKeyEditDlg (CWnd* pParent, CString const KeyS, CString *pValS);
protected:
	virtual void OnOK (void);
};

class CFloatKeyEditDlg : public CKeyEditDlg
{
public:
	CFloatKeyEditDlg (CWnd* pParent, CString const KeyS, CString *pValS);
protected:
	virtual void OnOK (void);
};

#endif // !defined(AFX_KEYEDITDLG_H__E9ABF084_A391_11D1_AC11_0060088D6C58__INCLUDED_)
