/****************************************************************************************/
/*  BrushEntityDialog.h                                                                 */
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

#ifndef BRUSHENTITYDIALOG_H
#define BRUSHENTITYDIALOG_H

#include "basetype.h"
#include "resource.h"

class CFusionDoc;
class CFusionTabControls;
class CPreMadeObject;

// CBrushEntityDialog dialog

class CBrushEntityDialog : public CDialog
{
// Construction
public:
	void Update( CFusionDoc* pDoc );
	CBrushEntityDialog(CFusionTabControls* pParent = NULL, CFusionDoc* pDoc = NULL);   // standard constructor

	// goes through all names in the object library and adds them to the list combo
	void SetupObjectListCombo();
	void AddObjectListName( char *name);

	// enables or disables (depending on state of flag) the place object button
	void EnablePlaceObjectButton( BOOL flag = TRUE );

	geBoolean GetCurrentEntityName (char *pEntityName);
	geBoolean GetCurrentObjectName (char *pObjName);

// Dialog Data
	//{{AFX_DATA(CBrushEntityDialog)
	enum { IDD = IDD_BRUSH_ENTITY_DIALOG };
	CStatic	m_LblBrushes;
	CStatic	m_LblEntities;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrushEntityDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual	BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CBrushEntityDialog)
	afx_msg void OnCubePrimitive();
	afx_msg void OnCylinderPrimitive();
	afx_msg void OnSpheroidPrimitive();
	afx_msg void OnStaircasePrimitive();
	afx_msg void OnArchPrimitive();
	afx_msg void OnConePrimitive();
	afx_msg void OnPlaceObject();
	afx_msg void OnEntities();
	afx_msg void OnSelchangeEntitycombo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	//	Brushes
	CButton m_ConeButton;
	CButton m_ArchButton;
	CButton m_StaircaseButton;
	CButton m_CylinderButton;
	CButton m_SpheroidButton;
	CButton m_CubeButton;
	
	//	Entitites
	CButton m_EntityButton;
	CComboBox m_EntityCombo;

	// combo box containing list of library objects
	CComboBox	m_ObjectListCombo;
	// button that user clicks to place an object within the level
	CButton	m_PlaceObjectButton;

	CFusionTabControls* m_pParentCtrl;
	CFusionDoc* m_pFusionDoc;
	void SetupDialog();
	void DisplayEntityButtonBitmap ();
	void FillEntitiesCombo ();
};

#endif
