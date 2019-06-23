/****************************************************************************************/
/*  TextureDialog.h                                                                     */
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

#ifndef TEXTUREDIALOG_H
#define TEXTUREDIALOG_H

#include "resource.h"
#include "brush.h"

class CFusionDoc;
class CFusionTabControls;

// CTextureDialog dialog

class CTextureDialog : public CDialog
{
// Construction
public:
	const char	*GetCurrentTexture();
	void Update( CFusionDoc* pDoc );
	CTextureDialog(CFusionTabControls* pParent = NULL, CFusionDoc* pDoc = NULL);   // standard constructor
	void	SelectTexture(int SelNum);
	~CTextureDialog();
	bool m_TxlibChanged;
// Dialog Data
	//{{AFX_DATA(CTextureDialog)
	enum { IDD = IDD_TEXTUREDIALOG };
	CStatic	m_SizeText;
	CButton	m_ApplyButton;
	CStatic	m_TextureImage;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextureDialog)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CTextureDialog)
	afx_msg void OnApply();
	afx_msg void OnSelchangetexturelist();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void UpdateSize();
	void ClearTextureImage(CDC *pDC);
	RECT m_TextureRect;
	void UpdateBitmap();
	void FillTheListbox();

	CString				m_CurrentTexture;
	CListBox			m_TextureList;
	CFusionTabControls	*m_pParentCtrl;
	CFusionDoc			*m_pDoc;
};

#endif
