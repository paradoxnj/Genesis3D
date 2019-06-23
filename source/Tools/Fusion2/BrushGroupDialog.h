/****************************************************************************************/
/*  BrushGroupDialog.h	                                                                */
/*                                                                                      */
/*  Author:       Jim Mischel, Jeff Lomax, John Moore                                   */
/*  Description:  Dialog code for brush groups                                          */
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
#ifndef BRUSHGROUPDIALOG_H_
#define BRUSHGROUPDIALOG_H_

#include "brush.h"
#include "colorbtn.h"
#include "resource.h"

class CFusionDoc;
class CFusionTabControls;
/////////////////////////////////////////////////////////////////////////////
// CBrushGroupDialog dialog

class CBrushGroupDialog : public CDialog
{
// Construction
public:
	CBrushGroupDialog(CFusionTabControls *pParent, CFusionDoc *apDoc);   // standard constructor
	void CBrushGroupDialog::UpdateTabDisplay(CFusionDoc* NewDoc);
	void CBrushGroupDialog::LoadComboBox(void);
	void UpdateGroupSelection( void ) ;

// Dialog Data
	//{{AFX_DATA(CBrushGroupDialog)
	enum { IDD = IDD_GROUPDIALOG };
	CButton	m_BrushLock;
	CButton	m_Visible;
	CListBox	m_BrushList;
	CColorButton	m_ColorButton;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrushGroupDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBrushGroupDialog)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelectbrushes();
	afx_msg void OnDeselectbrushes();
	virtual void OnCancel();
	afx_msg void OnCreateNewGroup();
	afx_msg void OnAddToCurrent();
	afx_msg void OnSelChangeGroupCombo();
	afx_msg void OnVisible();
	afx_msg void OnRemovefromcurrent();
	afx_msg void OnBrushlock();
	afx_msg void OnSelchangeBrushlist();
	afx_msg void OnRemovegroup();
	afx_msg LRESULT OnChangeColor(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
//	int					mCurrentGroup;	WHY, it was not updated!
	CFusionDoc			*pDoc;
	CFusionTabControls	*m_pParentCtrl;
};
/////////////////////////////////////////////////////////////////////////////


#endif
