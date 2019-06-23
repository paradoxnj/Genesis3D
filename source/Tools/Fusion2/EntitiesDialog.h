/****************************************************************************************/
/*  EntitiesDialog.h                                                                    */
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
// CEntitiesDialog dialog
#ifndef ENTITIES_DIALOG_H
#define ENTITIES_DIALOG_H

#include "Entity.h"
#include "resource.h"

class CFusionDoc;

class CEntitiesDialog : public CDialog
{
// Construction
public:
	CFusionDoc *pDoc;
//	void CreateNewEntity(char** ClassnameList);
	void FillInKeyValuePairs(int Selection);
	void FillInDialog();
	int DoDialog( CEntityArray& Entities, CFusionDoc* Doc);
	int EditEntity( CEntityArray& Entities, int CurrentEntity, CFusionDoc* Doc);
	CEntitiesDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CEntitiesDialog)
	enum { IDD = IDD_ENTITIES };
	CListBox	m_PropertiesList;
	CComboBox	m_EntityCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CEntitiesDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	BOOL NeedTextNotify( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
	INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CEntitiesDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK ();
	afx_msg void OnSelchangeEntitylist();
	afx_msg void OnDblclkPropertieslist();
	afx_msg int OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex);
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	// our entity array
	CEntityArray* mEntityArray;
	int mCurrentEntity;
	int mCurrentKey;
	mutable UINT UglyGlobalItemId;
	BOOL MultiEntityFlag;
};


#endif
