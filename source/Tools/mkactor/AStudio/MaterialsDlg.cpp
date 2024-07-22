/****************************************************************************************/
/*  MATERIALSDLG.CPP																	*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor studio materials dialog handler.									*/
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
#include "MaterialsDlg.h"
#include "MyFileDlg.h"
#include "TextInputDlg.h"
#include "MakeHelp.h"
#include "Filepath.h"

/////////////////////////////////////////////////////////////////////////////
// CMaterialsDlg property page

IMPLEMENT_DYNCREATE(CMaterialsDlg, CAStudioPropPage)

CMaterialsDlg::CMaterialsDlg() : CAStudioPropPage(CMaterialsDlg::IDD),
	m_FilenameChanged(false), m_CurrentIndex(-1)
{
	//{{AFX_DATA_INIT(CMaterialsDlg)
	m_ColorFlag = FALSE;
	m_TextureFilename = _T("");
	//}}AFX_DATA_INIT
}

CMaterialsDlg::~CMaterialsDlg()
{
}

void CMaterialsDlg::DoDataExchange(CDataExchange* pDX)
{
	CAStudioPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMaterialsDlg)
	DDX_Control(pDX, IDC_BROWSETEXTURE, m_BrowseTexture);
	DDX_Control(pDX, IDC_MATERIALDEFAULT, m_MaterialDefault);
	DDX_Control(pDX, IDC_CHOOSECOLOR, m_ChooseColor);
	DDX_Control(pDX, IDC_EDITTEXTURE, m_EditTextureFilename);
	DDX_Control(pDX, IDC_CHECKCOLOR, m_CheckColor);
	DDX_Control(pDX, IDC_MATERIALSLIST, m_MaterialsList);
	DDX_Check(pDX, IDC_CHECKCOLOR, m_ColorFlag);
	DDX_Text(pDX, IDC_EDITTEXTURE, m_TextureFilename);
	//}}AFX_DATA_MAP
}

int CMaterialsDlg::GetCurrentMaterialIndex ()
{
	int LbItem = m_MaterialsList.GetCurSel ();
	if (LbItem == LB_ERR)
	{
		m_CurrentIndex = -1;
	}
	else
	{
		m_CurrentIndex = (int)m_MaterialsList.GetItemData (LbItem);
	}
	return m_CurrentIndex;
}


void CMaterialsDlg::EnableControls ()
{
	const int Item = m_MaterialsList.GetCurSel ();
	const bool GoodItem = (Item != LB_ERR);

	m_BrowseTexture.EnableWindow (GoodItem && !m_ColorFlag && !m_Compiling);
	m_EditTextureFilename.EnableWindow (GoodItem && !m_ColorFlag && !m_Compiling);
	m_ChooseColor.EnableWindow (GoodItem && m_ColorFlag && !m_Compiling);

	m_CheckColor.EnableWindow (GoodItem && !m_Compiling);
	m_MaterialDefault.EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_RENAMEMATERIAL)->EnableWindow (GoodItem && !m_Compiling);
	GetDlgItem (IDC_DELETEMATERIAL)->EnableWindow (GoodItem && !m_Compiling);

	// Add available whenever we're not compiling
	GetDlgItem (IDC_ADDMATERIAL)->EnableWindow (!m_Compiling);

	// Enable/disable dropped files depending on compile flag
	DragAcceptFiles (!m_Compiling);
}

void CMaterialsDlg::FillListBox ()
{
	int Count = AProject_GetMaterialsCount (m_Project);

	// Fill listbox with material names
	m_MaterialsList.ResetContent ();
	for (int iMat = 0; iMat < Count; ++iMat)
	{
		// Get the material name and add it to the list box
		const char *Name	= AProject_GetMaterialName (m_Project, iMat);
		const int Index		= m_MaterialsList.AddString (Name);

		// if successfully added to the listbox, then add its index
		// to the listbox data
		if ((Index != LB_ERR) && (Index != LB_ERRSPACE))
		{
			m_MaterialsList.SetItemData (Index, iMat);
		}
	}
}

void CMaterialsDlg::SetupCurrentItem (int Item)
{
	// Set the dialog data for the current item.
	if (m_MaterialsList.SetCurSel (Item) != LB_ERR)
	{
		// enable defaults button....
		m_MaterialDefault.EnableWindow (TRUE);

		const int Index = (int)m_MaterialsList.GetItemData (Item);
		const ApjMaterialFormat Fmt = AProject_GetMaterialFormat (m_Project, Index);
		m_ColorFlag = (Fmt == ApjMaterial_Color) ? TRUE : FALSE;
		const char *Texture = AProject_GetMaterialTextureFilename (m_Project, Index);
		m_Color = AProject_GetMaterialTextureColor (m_Project, Index);

		m_CheckColor.SetCheck (m_ColorFlag);
		// Be sure to set the texture filename
		m_TextureFilename = Texture;
	}
	GetCurrentMaterialIndex ();
	m_FilenameChanged = false;
	EnableControls ();
}


BEGIN_MESSAGE_MAP(CMaterialsDlg, CAStudioPropPage)
	//{{AFX_MSG_MAP(CMaterialsDlg)
	ON_LBN_SELCHANGE(IDC_MATERIALSLIST, OnSelchangeMaterialslist)
	ON_BN_CLICKED(IDC_ADDMATERIAL, OnAddmaterial)
	ON_BN_CLICKED(IDC_BROWSETEXTURE, OnBrowsetexture)
	ON_BN_CLICKED(IDC_CHECKCOLOR, OnCheckcolor)
	ON_BN_CLICKED(IDC_CHOOSECOLOR, OnChoosecolor)
	ON_EN_CHANGE(IDC_EDITTEXTURE, OnChangeEdittexture)
	ON_BN_CLICKED(IDC_MATERIALDEFAULT, OnMaterialdefault)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_RENAMEMATERIAL, OnRenamematerial)
	ON_BN_CLICKED(IDC_DELETEMATERIAL, OnDeletematerial)
	ON_EN_KILLFOCUS(IDC_EDITTEXTURE, OnKillfocusEdittexture)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMaterialsDlg message handlers

BOOL CMaterialsDlg::OnInitDialog() 
{
	CAStudioPropPage::OnInitDialog();
	
	FillListBox ();
	SetupCurrentItem (0);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CMaterialsDlg::OnSetActive ()
{
	FillListBox ();
	SetupCurrentItem (0);
	UpdateData (FALSE);
	return TRUE;
}

void CMaterialsDlg::SetMaterialFromDialog ()
{
	if (m_CurrentIndex != -1)
	{
		AProject_SetMaterialTextureFilename (m_Project, m_CurrentIndex, m_TextureFilename);
		AProject_SetMaterialFormat (m_Project, m_CurrentIndex, (m_ColorFlag ? ApjMaterial_Color : ApjMaterial_Texture));
		AProject_SetMaterialTextureColor (m_Project, m_CurrentIndex, m_Color.r, m_Color.g, m_Color.b, m_Color.a);
	}
}

BOOL CMaterialsDlg::OnKillActive ()
{
	char OldTextureName[MAX_PATH];
	strcpy (OldTextureName, m_TextureFilename);

	UpdateData (TRUE);
	if (m_FilenameChanged)
	{
		m_FilenameChanged = false;
		if (!MakeHelp_GetRelativePath (m_Project, m_TextureFilename, OldTextureName))
		{
			m_TextureFilename = OldTextureName;
			UpdateData (FALSE);
			return FALSE;
		}
	}

	GetCurrentMaterialIndex ();
	SetMaterialFromDialog ();
	return TRUE;
}

void CMaterialsDlg::GetDialogData ()
{
	UpdateData (TRUE);
	SetMaterialFromDialog ();
}

void CMaterialsDlg::SetProjectPointer (AProject *Project)
{
	CAStudioPropPage::SetProjectPointer (Project);

	if (IsWindow (m_hWnd))
	{
		OnSetActive ();
	}
}

void CMaterialsDlg::SetCompileStatus (bool Status)
{
	CAStudioPropPage::SetCompileStatus (Status);

	if (IsWindow (m_hWnd))
	{
		EnableControls ();
	}
}

void CMaterialsDlg::OnSelchangeMaterialslist() 
{
	char OldTextureName[MAX_PATH];
	strcpy (OldTextureName, m_TextureFilename);

	UpdateData (TRUE);
	if (m_FilenameChanged)
	{
		m_FilenameChanged = false;
		if (!MakeHelp_GetRelativePath (m_Project, m_TextureFilename, OldTextureName))
		{
			m_TextureFilename = OldTextureName;
		}
	}

	SetMaterialFromDialog ();

	int CurSel = m_MaterialsList.GetCurSel ();

	SetupCurrentItem (CurSel);

	UpdateData (FALSE);
}


void CMaterialsDlg::OnAddmaterial() 
{
	// Prompt for material name.
	// Make sure it's not a duplicate of one that already exists.
	// Insert it into the project with default info.
	// Update listbox to include this item and focus this item.
	CTextInputDlg Dlg (IDS_MATERIALPROMPT, "", this);

	if (Dlg.DoModal () == IDOK)
	{
		// Update the current item
		UpdateData (TRUE);
		SetMaterialFromDialog ();
		CString MaterialName = Dlg.m_Text;
		// see if this material already exists in the list
		int Index = AProject_GetMaterialIndex (m_Project, MaterialName);
		if (Index == -1)
		{
			// Insert material into list
			if (!AProject_AddMaterial (m_Project, MaterialName, ApjMaterial_Texture, "", 0, 0, 0, 0, &Index))
			{
				// couldn't add it...
				AfxMessageBox (IDS_ERRORADDMATERIAL);
				return;
			}
			// And add to listbox
			int LbItem = m_MaterialsList.AddString (MaterialName);
			if ((LbItem == LB_ERR) && (LbItem != LB_ERRSPACE))
			{
				// couldn't add it to the listbox, so remove it from the project
				AProject_RemoveMaterial (m_Project, Index);
				AfxMessageBox (IDS_ERRORADDMATERIAL);
				return;
			}
			else
			{
				m_MaterialsList.SetItemData (LbItem, Index);
				SetupCurrentItem (LbItem);
				UpdateData (FALSE);
				SetModifiedFlag (true);
			}
		}
		else
		{
			// material exists in the project
			// Try to find it in the listbox.
			// If it doesn't exist in the listbox, then add it.
			int LbItem = m_MaterialsList.FindStringExact (-1, MaterialName);

			if (LbItem != LB_ERR)
			{
				// found it, so focus it
				SetupCurrentItem (LbItem);
				UpdateData (FALSE);
			}
			else
			{
				// wasn't found, so add it to the listbox and position there.
				LbItem = m_MaterialsList.AddString (MaterialName);
				if ((LbItem != LB_ERR) && (LbItem != LB_ERRSPACE))
				{
					m_MaterialsList.SetItemData (LbItem, Index);
					SetupCurrentItem (LbItem);
					UpdateData (FALSE);
				}
			}
		}
		GetCurrentMaterialIndex ();
	}
}


void CMaterialsDlg::OnBrowsetexture() 
{
	// Browse for texture name and if selected put in edit field
	CFileDialog *FileDlg = MyFileDialog_Create
	(
	  TRUE, IDS_TEXTUREPROMPT, IDS_BMPFILEEXT, IDS_BMPFILEFILTER,
	  m_TextureFilename, OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, this
	);

	if (FileDlg != NULL)
	{
		if (FileDlg->DoModal () == IDOK)
		{
			char NewTextureName[MAX_PATH];

			strcpy (NewTextureName, m_TextureFilename);
			SetModifiedFlag (true);

			if (MakeHelp_GetRelativePath (m_Project, FileDlg->GetPathName (), NewTextureName))
			{
				m_TextureFilename = NewTextureName;
				m_FilenameChanged = false;
			}
			UpdateData (FALSE);
		}
		delete FileDlg;
	}
	else
	{
		AfxMessageBox (IDS_OUTOFMEMORY);
	}

}

// Color checkbox clicked.  Enable/disable controls as appropriate.
void CMaterialsDlg::OnCheckcolor() 
{
	UpdateData (TRUE);
	if (m_ColorFlag)
	{
		m_ChooseColor.EnableWindow (TRUE);
		m_EditTextureFilename.EnableWindow (FALSE);
		m_BrowseTexture.EnableWindow (FALSE);
	}
	else
	{
		m_ChooseColor.EnableWindow (FALSE);
		m_EditTextureFilename.EnableWindow (TRUE);
		m_BrowseTexture.EnableWindow (TRUE);
	}
	SetModifiedFlag (true);
}

// Select color for this material.
void CMaterialsDlg::OnChoosecolor()
{
	COLORREF Color = RGB ((int)m_Color.r, (int)m_Color.g, (int)m_Color.b);

	CColorDialog Dlg (Color, CC_FULLOPEN, this);

	if (Dlg.DoModal () == IDOK)
	{
		Color = Dlg.GetColor ();
		m_Color.r = GetRValue (Color);
		m_Color.g = GetGValue (Color);
		m_Color.b = GetBValue (Color);

		SetModifiedFlag (true);
	}
}

void CMaterialsDlg::OnChangeEdittexture() 
{
	SetModifiedFlag (true);
	m_FilenameChanged = true;
}

// Defaults selected.  Set dialog fields and update material data.
void CMaterialsDlg::OnMaterialdefault() 
{
	m_TextureFilename = "";
	m_ColorFlag = FALSE;
	m_Color.r = 0;
	m_Color.g = 0;
	m_Color.b = 0;
	m_Color.a = 0;

	UpdateData (FALSE);

	SetMaterialFromDialog ();

	SetModifiedFlag (true);
	m_FilenameChanged = true;
}


void CMaterialsDlg::OnRenamematerial() 
{
	// Rename the material.
	// Get name.  Make sure it's unique.
	// Will have to remove and re-insert it in the listbox.
	int LbItem = m_MaterialsList.GetCurSel ();
	if (LbItem == LB_ERR)
	{
		// no current item
		return;
	}
	
	// get current name
	CString OldMaterialName = "";
	m_MaterialsList.GetText (LbItem, OldMaterialName);
	
	CTextInputDlg Dlg (IDS_NEWNAMEPROMPT, OldMaterialName, this);
	
	// Prompt user for new name
	if (Dlg.DoModal () == IDOK)
	{
		// Make sure the new name doesn't already exist
		const CString NewMaterialName = Dlg.m_Text;
		
		int Index = AProject_GetMaterialIndex (m_Project, NewMaterialName);
		if (Index == -1)
		{
			// Unique name entered	
			// Get item index from the listbox
			Index = m_MaterialsList.GetItemData (LbItem);
			// first try to add the new material to the listbox
			LbItem = m_MaterialsList.AddString (NewMaterialName);
			if ((LbItem == LB_ERR) || (LbItem == LB_ERRSPACE))
			{
				// couldn't add new material
				AfxMessageBox (IDS_ERRORADDMATERIAL);
			}
			else
			{
				// attach material to name
				m_MaterialsList.SetItemData (LbItem, Index);

				// Change the name of this material in the project
				AProject_SetMaterialName (m_Project, Index, NewMaterialName);

				// Set this as the new current selection
				m_MaterialsList.SetCurSel (LbItem);
				// and delete the old one...
				LbItem = m_MaterialsList.FindStringExact (-1, OldMaterialName);
				if (LbItem != LB_ERR)
				{
					m_MaterialsList.DeleteString (LbItem);
				}

				// refill the listbox 'cause we've shifted entries
				FillListBox ();

				// and set the new current item
				LbItem = m_MaterialsList.FindStringExact (-1, NewMaterialName);
				SetupCurrentItem (LbItem);
			}
		}
		else
		{
			// Material exists in the project.  Tell user about it.
			CString Msg;

			AfxFormatString1 (Msg, IDS_MATERIALEXISTS, NewMaterialName);
			AfxMessageBox (Msg);
		}
	}
}

void CMaterialsDlg::OnDeletematerial() 
{
	// remove a material from the project and from the listbox.
	int LbItem = m_MaterialsList.GetCurSel ();
	if (LbItem != LB_ERR)
	{
		int Index = m_MaterialsList.GetItemData (LbItem);
		// remove it from the project
		if (AProject_RemoveMaterial (m_Project, Index) == GE_FALSE)
		{
			AfxMessageBox (IDS_ERRORDELMATERIAL);
		}
		else
		{
			// remove it from the listbox
			m_MaterialsList.DeleteString (LbItem);
			// refill the listbox
			FillListBox ();
			// and set the new current item.
			if (LbItem >= m_MaterialsList.GetCount ())
			{
				--LbItem;
			}
			SetupCurrentItem (LbItem);
			UpdateData (FALSE);
		}
	}	
}

void CMaterialsDlg::OnDropFiles (HDROP hDrop)
{
	// Files dropped.  Add each one to the listbox.
	// The material name defaults to the name part of the full pathname

	// First make sure to update the current motion info
	OnSelchangeMaterialslist ();

	bool AddedOne = false;

	// get the count
	const UINT FileCount = DragQueryFile (hDrop, 0xffffffff, NULL, 0);
	for (UINT iFile = 0; iFile < FileCount; ++iFile)
	{
		// get the file and add it to the materials list
		char Filename[MAX_PATH];
		char MaterialName[MAX_PATH];
		char NewFilename[MAX_PATH];
		int Index;

		// get the filename
		DragQueryFile (hDrop, iFile, Filename, sizeof (Filename));

		// if we get a path that's not relative to the current path, then just exit
		strcpy (NewFilename, Filename);
		if (!MakeHelp_GetRelativePath (m_Project, Filename, NewFilename))
		{
			goto DropCleanup;
		}

		// Default material name will be the base filename
		FilePath_GetName (Filename, MaterialName);

		// Make sure that the name is unique
		if (m_MaterialsList.FindStringExact (-1, MaterialName) != LB_ERR)
		{
			CString Msg;

			AfxFormatString2 (Msg, IDS_CANTDROPMATERIAL, Filename, MaterialName);
			AfxMessageBox (Msg);
		}
		else
		{
			// Everything's cool, so add it to the listbox.
			if (AProject_AddMaterial (m_Project, MaterialName, ApjMaterial_Texture, NewFilename, 0, 0, 0, 0, &Index) != GE_FALSE)
			{
				// And add to listbox
				int LbItem = m_MaterialsList.AddString (MaterialName);
				if ((LbItem == LB_ERR) || (LbItem == LB_ERRSPACE))
				{
					// couldn't add it to listbox, so remove it
					CString Msg;

					AProject_RemoveMaterial (m_Project, Index);

					AfxFormatString1 (Msg, IDS_ERRORDROPMATERIAL, Filename);
					AfxMessageBox (Msg);
				}
				else
				{
					// Added it to the listbox, so update item data
					m_MaterialsList.SetItemData (LbItem, Index);
					SetModifiedFlag (true);
					AddedOne = true;
				}
			}
		}
	}

	// Let's focus at the beginning, 'cause I don't know what else to do...
	if (AddedOne)
	{
		SetupCurrentItem (0);
		UpdateData (FALSE);
	}

DropCleanup:
	// gotta do this or we'll leak memory and crash windows
	DragFinish (hDrop);
}

void CMaterialsDlg::OnKillfocusEdittexture() 
{
	if (m_FilenameChanged)
	{
		char OldMaterialName[MAX_PATH];
		
		// make sure that the new file is relative if required
		strcpy (OldMaterialName, m_TextureFilename);
		
		UpdateData (TRUE);
		if (!MakeHelp_GetRelativePath (m_Project, m_TextureFilename, OldMaterialName))
		{
			m_TextureFilename = OldMaterialName;
		}
		UpdateData (FALSE);
		m_FilenameChanged = false;
	}
}
