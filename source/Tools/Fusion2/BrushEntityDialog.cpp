/****************************************************************************************/
/*  BrushEntityDialog.cpp                                                               */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Moore, Bruce Cooner          */
/*  Description:  Dialog code for templates                                             */
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
#include "BrushEntityDialog.h"
#include "FusionTabControls.h"
#include "FUSIONDoc.h"
#include "EntityTable.h"
#include "FilePath.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define DLG_BORDER_SIZE_TOP	5
#define DLG_BORDER_SIZE_LEFT	5
#define DLG_WIDTH	250
#define DLG_HEIGHT	500

#define BRUSH_BUTTON_BORDER_TOP	5
#define BRUSH_BUTTON_BORDER_LEFT	2
#define ENTITY_BUTTON_BORDER_TOP	2
#define BUTTON_SIZE	34 //40

#define COMBO_BOX_WIDTH 175
#define COMBO_BOX_HEIGHT 175
#define COMBO_BOX_EDIT_HEIGHT 25

/////////////////////////////////////////////////////////////////////////////
// CBrushEntityDialog dialog


CBrushEntityDialog::CBrushEntityDialog(CFusionTabControls* pParent /*=NULL*/, CFusionDoc* pDoc)
	: CDialog(CBrushEntityDialog::IDD, (CWnd*) pParent)
{
	//{{AFX_DATA_INIT(CBrushEntityDialog)
	//}}AFX_DATA_INIT

	m_pFusionDoc = pDoc;
	m_pParentCtrl = pParent;

	CDialog::Create( IDD, (CWnd*) pParent );

	SetupDialog();
}


void CBrushEntityDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrushEntityDialog)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrushEntityDialog, CDialog)
	//{{AFX_MSG_MAP(CBrushEntityDialog)
	ON_BN_CLICKED(IDC_CUBE_PRIMITIVE, OnCubePrimitive)
	ON_BN_CLICKED(IDC_CYLINDER_PRIMITIVE, OnCylinderPrimitive)
	ON_BN_CLICKED(IDC_SPHEROID_PRIMITIVE, OnSpheroidPrimitive)
	ON_BN_CLICKED(IDC_STAIRCASE_PRIMITIVE, OnStaircasePrimitive)
	ON_BN_CLICKED(IDC_ARCH_PRIMITIVE, OnArchPrimitive)
	ON_BN_CLICKED(IDC_CONE_PRIMITIVE, OnConePrimitive)
	ON_BN_CLICKED(IDC_PLACE_OBJECT_BUTTON, OnPlaceObject)
	ON_BN_CLICKED(IDC_ENTITIES, OnEntities)
	ON_CBN_SELCHANGE(IDC_ENTITY_COMBO, OnSelchangeEntitycombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CBrushEntityDialog message handlers
struct ButtonInfoType
{
    UINT IconId;
	CButton *Btn;
	int Cmd;
};


void CBrushEntityDialog::SetupDialog()
{
	DWORD ButtonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_ICON;
	RECT rect;
	HICON icon;

	//	Create the buttons for brushes
	static ButtonInfoType BtnInfo[] =
	{
		{IDI_CUBE_PRIMITIVE,	  &this->m_CubeButton,		IDC_CUBE_PRIMITIVE},
		{IDI_SPHEROID_PRIMITIVE,  &this->m_SpheroidButton,	IDC_SPHEROID_PRIMITIVE},
		{IDI_CYLINDER_PRIMITIVE,  &this->m_CylinderButton,	IDC_CYLINDER_PRIMITIVE},
		{IDI_STAIRCASE_PRIMITIVE, &this->m_StaircaseButton, IDC_STAIRCASE_PRIMITIVE},
		{IDI_ARCH_PRIMITIVE,	  &this->m_ArchButton,		IDC_ARCH_PRIMITIVE},
		{IDI_CONE_PRIMITIVE,	  &this->m_ConeButton,		IDC_CONE_PRIMITIVE}
	};
	static const int NUM_BUTTONS = sizeof (BtnInfo) / sizeof (ButtonInfoType);

	rect.top = DLG_BORDER_SIZE_TOP + BRUSH_BUTTON_BORDER_TOP;
	rect.left = DLG_BORDER_SIZE_LEFT + BRUSH_BUTTON_BORDER_LEFT;
	rect.bottom = rect.top + BUTTON_SIZE;
	rect.right = rect.left + BUTTON_SIZE;

	for (int btn = 0; btn < NUM_BUTTONS; btn++)
	{
	    ButtonInfoType *pInfo;

		pInfo = &BtnInfo[btn];
		pInfo->Btn->Create (NULL, ButtonStyle, rect, this, pInfo->Cmd);
		icon = AfxGetApp ()->LoadIcon (pInfo->IconId);
		pInfo->Btn->SetIcon (icon);

		rect.left = rect.right + BRUSH_BUTTON_BORDER_LEFT;
		rect.right = rect.left + BUTTON_SIZE;
	}

	//	Entities button and combo box...
	rect.top = rect.bottom + BUTTON_SIZE;
	rect.bottom = rect.top + BUTTON_SIZE;
	rect.left = DLG_BORDER_SIZE_LEFT + BRUSH_BUTTON_BORDER_LEFT;
	rect.right = rect.left + BUTTON_SIZE;

	m_EntityButton.Create (NULL, ButtonStyle, rect, this, IDC_ENTITIES);

	// combo box....
	int ComboStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST | CBS_SORT;
	rect.left = rect.right + (2*BRUSH_BUTTON_BORDER_LEFT);
	rect.right = rect.left + COMBO_BOX_WIDTH;
	rect.top = rect.bottom - COMBO_BOX_EDIT_HEIGHT;
	rect.bottom = rect.top + COMBO_BOX_HEIGHT;
	m_EntityCombo.Create (ComboStyle, rect, this, IDC_ENTITY_COMBO);
	
	FillEntitiesCombo ();
	DisplayEntityButtonBitmap ();

	// Library object selection button and combo box...
	rect.top = rect.top + BUTTON_SIZE + BRUSH_BUTTON_BORDER_TOP;
	rect.bottom = rect.top + BUTTON_SIZE;
	rect.left = DLG_BORDER_SIZE_LEFT + BRUSH_BUTTON_BORDER_LEFT;
	rect.right = rect.left + BUTTON_SIZE;

	m_PlaceObjectButton.Create( NULL, ButtonStyle, rect, this, IDC_PLACE_OBJECT_BUTTON );
	icon = AfxGetApp ()->LoadIcon (IDI_LIBOBJECT);
	m_PlaceObjectButton.SetIcon( icon );

	// object list combo box
	rect.left = rect.right + (2*BRUSH_BUTTON_BORDER_LEFT);
	rect.right = rect.left + COMBO_BOX_WIDTH;
	rect.top = rect.bottom - COMBO_BOX_EDIT_HEIGHT;
	rect.bottom = rect.top + COMBO_BOX_HEIGHT;
	m_ObjectListCombo.Create( ComboStyle, rect, this, IDC_OBJECT_SELECT_COMBO );

	// now add all of the object names to the combo list
	SetupObjectListCombo();
}
void CBrushEntityDialog::FillEntitiesCombo ()
{
	m_EntityCombo.ResetContent ();
	// here we need to add the entity types from the CDialog info to the combo box...
	EntityTypeList *pList = EntityTable_GetAvailableEntityTypes (Level_GetEntityDefs (m_pFusionDoc->pLevel));
	if (pList != NULL)
	{
		for (int i = 0; i < pList->nTypes; i++)
		{
			if( strcmp( pList->TypeNames[i], "Camera" ) )	// Exclude Camera entity
				m_EntityCombo.AddString (pList->TypeNames[i]);
		}
		EntityTable_ReleaseEntityTypes (pList);
	}
	m_EntityCombo.SetCurSel (0);
}

void CBrushEntityDialog::DisplayEntityButtonBitmap (void)
{
	m_EntityButton.SetIcon (AfxGetApp ()->LoadIcon (IDI_LIGHT_ENTITY));
/*
  There's something screwy here.  SetBitmap doesn't seem to work with 16-color or
  256-color bitmaps.  SetIcon works fine.  Dunno.  Windows!#$%@#%$
*/
/*
	HBITMAP hbmp = ::LoadBitmap (::AfxGetInstanceHandle(), "BMP_TESTO");
	m_EntityButton.SetBitmap (hbmp);
	return;
/*
	int CurSel;
	geBoolean NoErrors;
	const Bitmap *pBitmap;
	HICON LightIcon;
	HBITMAP hbmpOld, hbmpNew;

 	LightIcon = AfxGetApp ()->LoadIcon (IDI_LIGHT_ENTITY);

	CurSel = m_EntityCombo.GetCurSel ();
 	NoErrors = (CurSel >= 0);

	if (NoErrors)
	{
		CString cstr;

		m_EntityCombo.GetLBText (CurSel, cstr);
		pBitmap = EntityTable_GetBitmapPtr (cstr);
		NoErrors = (pBitmap != NULL);
	}
	
	if (NoErrors)
	{
		// Create a Windows bitmap out of this thing.
		// and then call m_EntityButton.SetBitmap (bmpHandle)
		BITMAPINFOHEADER *bmih;
		struct MyBitmapInfo
		{
			BITMAPINFOHEADER bmiHeader;
			RGBQUAD bmiColors[256];
		};
		MyBitmapInfo bmi;
		void *bits;
		HDC dcMem;

		bmih = &(bmi.bmiHeader);
		bmih->biSize = sizeof (BITMAPINFOHEADER);
		bmih->biWidth = Bitmap_GetWidth (pBitmap);
		bmih->biHeight = Bitmap_GetHeight (pBitmap);
		bmih->biPlanes = 1;
		bmih->biBitCount = 8;
		bmih->biCompression = BI_RGB;
		bmih->biSizeImage = 0;
		bmih->biXPelsPerMeter = 0;
		bmih->biYPelsPerMeter = 0;
		bmih->biClrUsed = 0;
		bmih->biClrImportant = 0;

		// This cast is probably a bad idea, but WTF.
		Bitmap_GetPalette (pBitmap, (Bitmap_Palette *)&(bmi.bmiColors));
		dcMem = ::CreateCompatibleDC (NULL);
		// so create the bitmap
		hbmpNew = ::CreateDIBSection (dcMem, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, &bits, NULL, 0);
		NoErrors = (hbmpNew != NULL);

		DeleteDC (dcMem);
		if (NoErrors)
		{
			// and copy the bits to it
			Bitmap_GetBits (pBitmap, bits, 0, Bitmap_8bit);
		}
	}
	if (NoErrors)
	{
		hbmpOld = m_EntityButton.SetBitmap (hbmpNew);
	}
	else
	{	
		hbmpOld = (HBITMAP)m_EntityButton.SetIcon (LightIcon);
	}

	if ((hbmpOld != NULL) && (hbmpOld != (HBITMAP)LightIcon))
	{
		::DeleteObject (hbmpOld);
	}
*/
}

BOOL CBrushEntityDialog::OnInitDialog()
{
	//	Position the new dialog on the tab...
	RECT rect;
	RECT rect2;

	m_pParentCtrl->GetClientRect( &rect );
	m_pParentCtrl->GetItemRect( 0, &rect2 );
	rect.top = rect2.bottom + DLG_BORDER_SIZE_TOP;
	rect.left = rect.left + DLG_BORDER_SIZE_LEFT;
	rect.right = rect.left + DLG_WIDTH;
	rect.bottom = rect.top + DLG_HEIGHT;

	SetWindowPos( NULL, rect.left,
			rect.top, rect.right, rect.bottom, SWP_NOZORDER );

	return TRUE;
}

void CBrushEntityDialog::Update( CFusionDoc* pDoc )
{
	m_pFusionDoc = pDoc;

	FillEntitiesCombo ();
	if(pDoc->mModeTool==ID_TOOLS_TEMPLATE)
	{
		//enable all
		m_CubeButton.EnableWindow(TRUE);
		m_SpheroidButton.EnableWindow(TRUE);
		m_CylinderButton.EnableWindow(TRUE);
		m_StaircaseButton.EnableWindow(TRUE);
		m_ArchButton.EnableWindow(TRUE);
		m_ConeButton.EnableWindow(TRUE);
		m_EntityButton.EnableWindow (TRUE);
		// enable the object placement button
		EnablePlaceObjectButton( TRUE );
	} 
	else 
	{
		//grey all
		m_CubeButton.EnableWindow(FALSE);
		m_SpheroidButton.EnableWindow(FALSE);
		m_CylinderButton.EnableWindow(FALSE);
		m_StaircaseButton.EnableWindow(FALSE);
		m_ArchButton.EnableWindow(FALSE);
		m_ConeButton.EnableWindow(FALSE);
		m_EntityButton.EnableWindow (FALSE);
		// disable object placement button
		m_PlaceObjectButton.EnableWindow( FALSE );
	}
}

void CBrushEntityDialog::OnCubePrimitive() 
{
	m_pFusionDoc->CreateCube();
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}

void CBrushEntityDialog::OnCylinderPrimitive() 
{
	m_pFusionDoc->CreateCylinder();
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}

void CBrushEntityDialog::OnSpheroidPrimitive() 
{
	m_pFusionDoc->CreateSpheroid();
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}

void CBrushEntityDialog::OnStaircasePrimitive() 
{
	m_pFusionDoc->CreateStaircase();
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}

void CBrushEntityDialog::OnEntities() 
{
	int Cur;

	// Entity button was clicked
	// Get current entity type from combo box and create one...
	Cur = m_EntityCombo.GetCurSel ();
	if (Cur != LB_ERR)
	{
		char EntityName[_MAX_PATH];
		CWnd *LastView;

		m_EntityCombo.GetLBText (Cur, EntityName);
		/*
		  We create a "light" entity as the template entity.
		  In future, we'll have a "template" entity.
		*/
		m_pFusionDoc->CreateEntity ("light");//EntityName);
		LastView = m_pParentCtrl->LastView;
		if ((LastView != NULL) && ::IsWindow (LastView->m_hWnd))
		{
			LastView->SetFocus ();
		}
	}
}

void CBrushEntityDialog::OnArchPrimitive() 
{
	m_pFusionDoc->CreateArch();
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}

void CBrushEntityDialog::OnConePrimitive() 
{
	m_pFusionDoc->CreateCone();	
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}


void CBrushEntityDialog::OnSelchangeEntitycombo
	(
	  void
	)
{
    int Cur;

	Cur = m_EntityCombo.GetCurSel ();
	DisplayEntityButtonBitmap ();	
}

geBoolean CBrushEntityDialog::GetCurrentEntityName 
	(
	  char *pEntityName
	)
{
	int Cur;

	Cur = m_EntityCombo.GetCurSel ();
	if (Cur != LB_ERR)
	{
		m_EntityCombo.GetLBText (Cur, pEntityName);
		return GE_TRUE;
	}
	return GE_FALSE;
}

geBoolean CBrushEntityDialog::GetCurrentObjectName (char *pObjName)
{
	int Cur;

	Cur = m_ObjectListCombo.GetCurSel ();
	if (Cur != LB_ERR)
	{
		m_ObjectListCombo.GetLBText (Cur, pObjName);
		return GE_TRUE;
	}
	return GE_FALSE;
}


// -------------------------------------------------------------------------------------
// called when the user presses the Place Object push button on the Brush/Entity dialog
// and is charged with placing the currently selected library object down into the level
// where the user wishes it to go
// TODO : need some method by which objects are placed at a user specified location,  instead
//        of just plopping them down at the origin
void CBrushEntityDialog::OnPlaceObject() 
{
	CWnd *mainWnd = AfxGetMainWnd();
	ASSERT( mainWnd );


	// tell the fusion doc to create a template entity for the user to specify the location
	// of the objects with
	m_pFusionDoc->CreateObjectTemplate();
	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();
}

// ------------------------------------------------------------------------------------
// first removes all items currently in the list box and then
// goes through all names in the object library and adds them to the list combo
void CBrushEntityDialog::SetupObjectListCombo()
{
	// add object names to the object combo box
//	int NumObjectsAdded = 0;	// counts the number of library objects that were added to 
								// the combo box
	char ObjectsDir[MAX_PATH];
	const char *pObjDir;
	CString strObjectsDir;
	BOOL FileWasFound;
	char Name[MAX_PATH];

	WIN32_FIND_DATA FindData;
	HANDLE FindHandle;
	const Prefs *pPrefs = m_pFusionDoc->GetPrefs ();

	// get Objects directory from INI
	pObjDir = Prefs_GetObjectsDir (pPrefs);
	::FilePath_AppendName (pObjDir, "*.3dt", ObjectsDir);

	m_ObjectListCombo.ResetContent ();
	FindHandle = ::FindFirstFile (ObjectsDir, &FindData);

	FileWasFound = (FindHandle != INVALID_HANDLE_VALUE);

	while (FileWasFound)
	{
		::FilePath_GetName (FindData.cFileName, Name);
		m_ObjectListCombo.AddString (Name);
		FileWasFound = ::FindNextFile (FindHandle, &FindData);
	}

	::FindClose (FindHandle);
	if (m_ObjectListCombo.SetCurSel (0) == CB_ERR)
	{
		// couldn't set it...probably nothing in there
		EnablePlaceObjectButton (FALSE);
	}
}

// -------------------------------------------------------------------------
// adds a single new name to the object list combo box
void CBrushEntityDialog::AddObjectListName( char *name)
{
	// add this object's name to combo box
	int AddReturnValue = m_ObjectListCombo.AddString( name );

	if ((AddReturnValue != CB_ERR) && (AddReturnValue != CB_ERRSPACE))
	{
		m_ObjectListCombo.SetCurSel (AddReturnValue);
		EnablePlaceObjectButton (TRUE);
	}
}

// enables or disables (depending on state of flag) the place object button
void CBrushEntityDialog::EnablePlaceObjectButton( BOOL flag )
{
	int nItems;

	nItems = m_ObjectListCombo.GetCount ();

	// if there are no objects in the library don't allow setting the flag
	m_PlaceObjectButton.EnableWindow (flag & (nItems > 0));
}
