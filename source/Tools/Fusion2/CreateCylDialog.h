/****************************************************************************************/
/*  CreateCylDialog.h                                                                   */
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
#ifndef CREATECYLDIALOG_H
#define CREATECYLDIALOG_H

#include "resource.h"
#include "BrushTemplate.h"

class CCreateCylDialog : public CDialog
{
public:
	virtual int DoModal(geBoolean ConvertToMetric, BrushTemplate_Cylinder *pCylTemplate);
	CCreateCylDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCreateCylDialog)
	enum { IDD = IDD_CREATE_CYL };
	float	m_BotXOffset;
	float	m_BotXSize;
	float	m_BotZOffset;
	float	m_BotZSize;
	int		m_Solid;
	float	m_TopXOffset;
	float	m_TopXSize;
	float	m_TopZOffset;
	float	m_TopZSize;
	float	m_YSize;
	float	m_RingLength;
	BOOL	m_TCut;
	int		m_VerticalStripes;
	float	m_Thickness;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateCylDialog)
	public:
	virtual void Serialize(CArchive& ar);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	void		dlgFieldsToCentimeters(void);
	void		dlgFieldsToTexels(void);
	geBoolean	m_ConvertToMetric;
	BrushTemplate_Cylinder *m_pCylTemplate;
protected:

	// Generated message map functions
	//{{AFX_MSG(CCreateCylDialog)
	afx_msg void OnDefaults();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif
