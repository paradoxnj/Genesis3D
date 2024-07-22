/****************************************************************************************/
/*  KeyEditDlg.cpp                                                                      */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  This file contains all of the dialogs that edit entity properties.    */
/*                KeyEditDlg is subclassed to edit strings, ints, and floats.           */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/

#include "stdafx.h"
#include "KeyEditDlg.h"
#include <assert.h>
#include "RAM.H"
#include "util.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CKeyEditDlg dialog


CKeyEditDlg::CKeyEditDlg(CWnd* pParent, CString const KeyS, CString *pValS)
	: CDialog(CKeyEditDlg::IDD, pParent),
	  Key(KeyS), pValue(pValS)
{
	//{{AFX_DATA_INIT(CKeyEditDlg)
	//}}AFX_DATA_INIT
}


void CKeyEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CKeyEditDlg)
	DDX_Control(pDX, IDC_VALUE_EDIT, m_ValueEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CKeyEditDlg, CDialog)
	//{{AFX_MSG_MAP(CKeyEditDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyEditDlg message handlers

BOOL CKeyEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	this->SetWindowText (Key);
	m_ValueEdit.SetWindowText (*pValue);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CKeyEditDlg::OnOK() 
{
	CString Text;

	m_ValueEdit.GetWindowText (Text);

	// don't allow quotes in the string...
	if (strchr (Text, '"') == NULL)
	{
	    *pValue = Text;
		CDialog::OnOK();
	}
}


CColorKeyEditDlg::CColorKeyEditDlg
    (
	  CWnd *pParent,
	  CString const KeyS,
	  CString *pValS
	) : CColorDialog (0, (CC_ANYCOLOR | CC_FULLOPEN), pParent),
		Key(KeyS), pValue(pValS)
{
	// create RGB color...
	int r, g, b;

	r = 0;
	g = 0;
	b = 0;

	sscanf (*pValS, "%d %d %d", &r, &g, &b);

	this->m_cc.rgbResult = RGB (r, g, b);
	this->m_cc.Flags |= CC_RGBINIT;
}


BOOL CColorKeyEditDlg::OnInitDialog
    (
	  void
	)
{
	CColorDialog::OnInitDialog ();

	this->SetWindowText (Key);
	return TRUE;
}

INT_PTR CColorKeyEditDlg::DoModal
	(
	  void
	)
{
	/*
	  The MFC documentation for CColorDialog is incorrect.
	  It states that the selected color is available (through
	  GetColor()) when OnColorOK is fired.  Not true.  The only
	  way I could get the color was to do it at the end of DoModal.
	*/
    int rslt = CColorDialog::DoModal ();

	COLORREF  TheColor;
	int r, g, b;

	TheColor = this->GetColor ();

	r = GetRValue (TheColor);
	g = GetGValue (TheColor);
	b = GetBValue (TheColor);

	pValue->Format ("%d %d %d", r, g, b);

	return rslt;
}


//////////////////
// CIntKeyEditDlg

CIntKeyEditDlg::CIntKeyEditDlg 
	(
	  CWnd* pParent, 
	  CString const KeyS, 
	  CString *pValS
	) : CKeyEditDlg (pParent, KeyS, pValS)
{
}

void CIntKeyEditDlg::OnOK
    (
	  void
	)
{
	CString Text;
	int TheInt;

	this->m_ValueEdit.GetWindowText (Text);

	if (Util_IsValidInt (Text, &TheInt))
	{
		// in case it was a hex number
		// convert int to text and set...
		Text.Format ("%d", TheInt);
		m_ValueEdit.SetWindowText (Text);
	    CKeyEditDlg::OnOK ();
	}
	else
	{
	    m_ValueEdit.SetSel (0, -1, TRUE);
	}
}


//////////////////
// CFloatKeyEditDlg

CFloatKeyEditDlg::CFloatKeyEditDlg 
	(
	  CWnd* pParent, 
	  CString const KeyS, 
	  CString *pValS
	) : CKeyEditDlg (pParent, KeyS, pValS)
{
}

void CFloatKeyEditDlg::OnOK
    (
	  void
	)
{
	CString Text;
	float TheFloat;

	this->m_ValueEdit.GetWindowText (Text);

	if (Util_IsValidFloat (Text, &TheFloat))
	{
	    CKeyEditDlg::OnOK ();
	}
	else
	{
	    m_ValueEdit.SetSel (0, -1, TRUE);
	}
}


static BOOL ValidateFloat
    (
	  CEdit &EditControl
	)
{
	float TheFloat;
	CString Text;

	EditControl.GetWindowText (Text);
	if (!Util_IsValidFloat (Text, &TheFloat))
	{
		EditControl.SetSel (0, -1, TRUE);
		EditControl.SetFocus ();
	    return FALSE;
	}
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPointKeyEditDlg dialog


CPointKeyEditDlg::CPointKeyEditDlg(CWnd* pParent, CString const KeyS, CString *pValS)
	: CDialog(CPointKeyEditDlg::IDD, pParent),
	  Key(KeyS), pValue(pValS)
{
	//{{AFX_DATA_INIT(CPointKeyEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CPointKeyEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPointKeyEditDlg)
	DDX_Control(pDX, IDC_ZEDIT, m_ZEdit);
	DDX_Control(pDX, IDC_YEDIT, m_YEdit);
	DDX_Control(pDX, IDC_XEDIT, m_XEdit);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPointKeyEditDlg, CDialog)
	//{{AFX_MSG_MAP(CPointKeyEditDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPointKeyEditDlg message handlers

void CPointKeyEditDlg::OnOK() 
{
	if (ValidateFloat (m_XEdit) &&
	    ValidateFloat (m_YEdit) &&
		ValidateFloat (m_ZEdit))
	{
		CString cx, cy, cz;

		m_XEdit.GetWindowText (cx);
		m_YEdit.GetWindowText (cy);
		m_ZEdit.GetWindowText (cz);
		pValue->Format ("%s %s %s", cx, cy, cz);
	    CDialog::OnOK ();
	}
}

BOOL CPointKeyEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	static char *Zero = "0.0";

	char *c;
	char *temp;

	this->SetWindowText (Key);

	temp = Util_Strdup (*pValue);

	c = strtok (temp, " ");
	if (c == NULL)
	{
	    c = Zero;
	}
	m_XEdit.SetWindowText (c);

	c = strtok (NULL, " ");
	if (c == NULL)
	{
	    c = Zero;
	}
	m_YEdit.SetWindowText (c);

	c = strtok (NULL, " ");
	if (c == NULL)
	{
	    c = Zero;
	}
	m_ZEdit.SetWindowText (c);
	
	geRam_Free (temp);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/////////////////////////////////////////////////////////////////////////////
// CStructKeyEditDlg dialog


CStructKeyEditDlg::CStructKeyEditDlg
	(
	  CWnd* pParent, 
	  CEntity const &aEnt, 
	  CString const KeyS,
	  CEntityArray *Ents,
	  CString *pValS,
	  const EntityTable *pEntTable
	) : CDialog(CStructKeyEditDlg::IDD, pParent),
		Ent(aEnt), Key(KeyS), mEntityArray(Ents),
		pValue(pValS), m_pEntityTable(pEntTable)
{
	//{{AFX_DATA_INIT(CStructKeyEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CStructKeyEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CStructKeyEditDlg)
	DDX_Control(pDX, IDC_STRUCTLIST, m_StructList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CStructKeyEditDlg, CDialog)
	//{{AFX_MSG_MAP(CStructKeyEditDlg)
	ON_LBN_DBLCLK(IDC_STRUCTLIST, OnDblclkStructlist)
	ON_WM_VKEYTOITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CStructKeyEditDlg message handlers

static void stripQuotes
	(
	  CString &str
	)
/*
  This function is in this module, and also in EntitiesDialog.
  It should probably be in a utility module.
  It wouldn't be necessary here or in EntitiesDialog if the strings
  were de-quoted when they're loaded from the map file...
*/
{
	char *temp = Util_Strdup (str);
	
	if (*temp == '"')
	{
	    strcpy (temp, temp+1);
	}
	if (temp[strlen (temp) - 1] == '"')
	{
		temp[strlen (temp) - 1] = '\0';
	}
	str = temp;

	geRam_Free (temp);
}

BOOL CStructKeyEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	this->SetWindowText (Key);

	// get the type name of the current key...
	CString KeyTypeName = EntityTable_GetEntityPropertyTypeName (m_pEntityTable, Ent.GetClassname (), Key);

	// Fill the listbox with those values
	// And set the current selection to the current value (if selected).
	this->m_StructList.ResetContent ();

	{
	    // add <null> entry	
		CString EntityName = "<null>";
		int Index = m_StructList.AddString (EntityName);
		if (stricmp (*pValue, EntityName) == 0)
		{
		    m_StructList.SetCurSel (Index);
		}
	}

	// Get a list of all entities of this type...
	for (int Current = 0; Current < (*mEntityArray).GetSize(); ++Current)
	{
		CEntity &Ent = (*mEntityArray)[Current];
		if (KeyTypeName == Ent.GetClassname ())
		{
			CString EntityName;

			if (!Ent.GetKeyValue ("%name%", EntityName))
			{
			    EntityName = "Unnamed";
			}

			stripQuotes (EntityName);

		    // add the item....
			int Index = m_StructList.AddString (EntityName);
			if (stricmp (*pValue, EntityName) == 0)
			{
				m_StructList.SetCurSel (Index);
			}
			    
		}
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CStructKeyEditDlg::OnDblclkStructlist() 
{
	this->OnOK ();	
}

void CStructKeyEditDlg::OnOK() 
{
	int CurSel = m_StructList.GetCurSel ();

	if (CurSel != LB_ERR)
	{
	    m_StructList.GetText (CurSel, *pValue);
	}		

	CDialog::OnOK();
}

int CStructKeyEditDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex) 
{
	switch (nKey)
	{
		case VK_SPACE :
		    this->OnOK ();
			return -2;
		default :
			return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
	}
}
/////////////////////////////////////////////////////////////////////////////
// CModelKeyEditDlg dialog


CModelKeyEditDlg::CModelKeyEditDlg(CWnd* pParent, ModelList *pModels, CString const KeyS, CString *pValS)
	: CDialog(CModelKeyEditDlg::IDD, pParent),
	  Models(pModels), Key(KeyS), pValue(pValS)
{
	//{{AFX_DATA_INIT(CModelKeyEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CModelKeyEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModelKeyEditDlg)
	DDX_Control(pDX, IDC_MODELLIST, m_ModelList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModelKeyEditDlg, CDialog)
	//{{AFX_MSG_MAP(CModelKeyEditDlg)
	ON_LBN_DBLCLK(IDC_MODELLIST, OnDblclkModellist)
	ON_WM_VKEYTOITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelKeyEditDlg message handlers

BOOL CModelKeyEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	this->SetWindowText (Key);

	// get model ID of current model.
	int ValueModelId;
	Model *pModel;

	pModel = ModelList_FindByName (Models, *pValue);
	if (pModel == NULL)
	{
		ValueModelId = 0;
	}
	else
	{
		ValueModelId = Model_GetId (pModel);
	}

	// fill list box with existing models.
	// set current selection to item containing pValue
	m_ModelList.ResetContent ();
	m_ModelList.SetCurSel (0);

	{
	    // add <null> entry	
		CString EntityName = "<null>";
		int Index = m_ModelList.AddString (EntityName);
		m_ModelList.SetItemData (Index, 0);
		if (stricmp (*pValue, EntityName) == 0)
		{
		    m_ModelList.SetCurSel (Index);
		}
	}

	ModelIterator mi;

	pModel = ModelList_GetFirst (Models, &mi);
	while (pModel != NULL)
	{
		char const * ModelName;
		int Index;
		int ModelId;

		ModelId = Model_GetId (pModel);
		ModelName = Model_GetName (pModel);
		Index = m_ModelList.AddString (ModelName);
		m_ModelList.SetItemData (Index, ModelId);

		if (ModelId == ValueModelId)
		{
			m_ModelList.SetCurSel (Index);
		}

		pModel = ModelList_GetNext (Models, &mi);
	}

	return TRUE;
}

void CModelKeyEditDlg::OnDblclkModellist() 
{
	this->OnOK ();	
}

void CModelKeyEditDlg::OnOK() 
{
	int CurSel = m_ModelList.GetCurSel ();

	if (CurSel != LB_ERR)
	{
	    m_ModelList.GetText (CurSel, *pValue);
	}		

	CDialog::OnOK();
}

int CModelKeyEditDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex) 
{
	switch (nKey)
	{
		case VK_SPACE :
		    this->OnOK ();
			return -2;
		default :
			return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
	}
}
/////////////////////////////////////////////////////////////////////////////
// CBoolKeyEditDlg dialog


CBoolKeyEditDlg::CBoolKeyEditDlg
	(
	  CWnd* pParent, 
	  CString const KeyS,
	  CString *pValS
	) : CDialog(CBoolKeyEditDlg::IDD, pParent),
		Key(KeyS), pValue(pValS)
{
	//{{AFX_DATA_INIT(CBoolKeyEditDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CBoolKeyEditDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBoolKeyEditDlg)
	DDX_Control(pDX, IDC_BOOLLIST, m_BoolList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBoolKeyEditDlg, CDialog)
	//{{AFX_MSG_MAP(CBoolKeyEditDlg)
	ON_LBN_DBLCLK(IDC_BOOLLIST, OnDblclkBoollist)
	ON_WM_VKEYTOITEM()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBoolKeyEditDlg message handlers

void CBoolKeyEditDlg::OnDblclkBoollist() 
{
	this->OnOK ();
}

void CBoolKeyEditDlg::OnOK() 
{
	int CurSel = m_BoolList.GetCurSel ();

	if (CurSel != LB_ERR)
	{
		m_BoolList.GetText (CurSel, *pValue);
	}
	
	CDialog::OnOK();
}

BOOL CBoolKeyEditDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	this->m_BoolList.ResetContent ();

	m_BoolList.AddString ("False");
	m_BoolList.AddString ("True");

	if (stricmp (*pValue, "True") == 0)
	{
		m_BoolList.SetCurSel (1);
	}
	else
	{
		m_BoolList.SetCurSel (0);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

int CBoolKeyEditDlg::OnVKeyToItem(UINT nKey, CListBox* pListBox, UINT nIndex) 
{
	switch (nKey)
	{
		case VK_SPACE :
		    this->OnOK ();
			return -2;
		default :
			return CDialog::OnVKeyToItem(nKey, pListBox, nIndex);
	}
}
