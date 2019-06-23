/****************************************************************************************/
/*  CreateConeDialog.h                                                                  */
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
#ifndef CREATECONEDIALOG_H
#define CREATECONEDIALOG_H

#include "resource.h"
#include "BrushTemplate.h"

class CCreateConeDialog : public CDialog
{
// Construction
public:
	virtual int DoModal(geBoolean ConvertToMetric, BrushTemplate_Cone *pConeTemplate);
	CCreateConeDialog(CWnd *pParent=NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateConeDialog)
	enum { IDD = IDD_CREATE_CONE };
	int		m_Style;
	float	m_Width;
	float	m_Height;
	int		m_VerticalStrips;
	float	m_Thickness;
	BOOL	m_TCut;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateConeDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void		CCreateConeDialog::dlgFieldsToCentimeters(void);
	void		CCreateConeDialog::dlgFieldsToTexels(void);
	geBoolean	m_ConvertToMetric;
	BrushTemplate_Cone *m_pConeTemplate;
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateConeDialog)
	afx_msg void OnDefault();
	afx_msg void OnSolid();
	afx_msg void OnHollow();
	afx_msg void OnFunnel();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
