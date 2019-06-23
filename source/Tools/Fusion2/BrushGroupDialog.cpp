/****************************************************************************************/
/*  BrushGroupDialog.cpp                                                                */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include "stdafx.h"
#include "BrushGroupDialog.h"
#include "fusiontabcontrols.h"
#include "keyeditdlg.h"
#include "assert.h"

#include "fusiondoc.h"  // bad, bad, bad!
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBrushGroupDialog dialog

CBrushGroupDialog::CBrushGroupDialog(CFusionTabControls *pParent, CFusionDoc *apDoc)
: CDialog(CBrushGroupDialog::IDD, (CWnd *)pParent),
	  pDoc(apDoc)
{
	//{{AFX_DATA_INIT(CBrushGroupDialog)
	//}}AFX_DATA_INIT
	m_pParentCtrl	= pParent;

	CDialog::Create(IDD, (CWnd *)pParent);
}


void CBrushGroupDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrushGroupDialog)
	DDX_Control(pDX, IDC_BRUSHLOCK, m_BrushLock);
	DDX_Control(pDX, IDC_VISIBLE, m_Visible);
	DDX_Control(pDX, IDC_BRUSHLIST, m_BrushList);
	DDX_Control(pDX, IDC_COLOURBUTTON, m_ColorButton);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CBrushGroupDialog, CDialog)
	//{{AFX_MSG_MAP(CBrushGroupDialog)
	ON_BN_CLICKED(IDC_SELECTBRUSHES, OnSelectbrushes)
	ON_BN_CLICKED(IDC_DESELECTBRUSHES, OnDeselectbrushes)
	ON_BN_CLICKED(IDC_CREATENEWGROUP, OnCreateNewGroup)
	ON_BN_CLICKED(IDC_ADDTOCURRENT, OnAddToCurrent)
	ON_CBN_SELCHANGE(IDC_GROUPCOMBO, OnSelChangeGroupCombo)
	ON_BN_CLICKED(IDC_VISIBLE, OnVisible)
	ON_BN_CLICKED(IDC_REMOVEFROMCURRENT, OnRemovefromcurrent)
	ON_BN_CLICKED(IDC_BRUSHLOCK, OnBrushlock)
	ON_LBN_SELCHANGE(IDC_BRUSHLIST, OnSelchangeBrushlist)
	ON_BN_CLICKED(IDC_REMOVEGROUP, OnRemovegroup)
	ON_MESSAGE(WM_UPDATEGROUPCOLOR, OnChangeColor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrushGroupDialog message handlers
/////////////////////////////////////////////////////////////////////////////

// when we are making the dialog
BOOL CBrushGroupDialog::OnInitDialog() 
{
	BOOL	bEnableWindow ;
	
	CDialog::OnInitDialog();

	//	Position the new dialog on the tab...
	RECT rect;
	RECT rect2;

	m_pParentCtrl->GetClientRect( &rect );
	m_pParentCtrl->GetItemRect( 0, &rect2 );
	rect.top = rect2.bottom + FTC_BORDER_SIZE_TOP;
	rect.left = rect.left + FTC_BORDER_SIZE_LEFT;
	rect.right = rect.right - FTC_BORDER_SIZE_RIGHT ;
	rect.bottom = rect.bottom - FTC_BORDER_SIZE_BOTTOM ;

	SetWindowPos(NULL, rect.left,
			rect.top, rect.right, rect.bottom, SWP_NOZORDER );

	CButton * pBut = (CButton*)GetDlgItem( IDC_ADDTOCURRENT ) ;
	bEnableWindow = ( pDoc->mCurrentGroup == 0 ) ? FALSE : TRUE ;
	pBut->EnableWindow( bEnableWindow ) ;

	UpdateTabDisplay(pDoc);
	LoadComboBox();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

typedef struct
{
	CListBox *BrushListbox;
	CFusionDoc *pDoc;
	geBoolean ItemsInLB;
} lbAddData;

static geBoolean ListboxAddBrushes (Brush *pBrush, void *lParam)
{
	lbAddData *pData;
	char	szTempString[32];
	int Index;
	pData = (lbAddData *)lParam;

	if (Brush_GetGroupId (pBrush) == pData->pDoc->mCurrentGroup)
	{
		sprintf( szTempString, "(B) %s", Brush_GetName( pBrush ) ) ;
		Index = pData->BrushListbox->AddString( szTempString ) ;
		pData->ItemsInLB = GE_TRUE;
		if( Index >= 0 ) /* LB_ERR is -1, LB_ERRSPACE  is -2 */
		{
			pData->BrushListbox->SetItemData( Index, (DWORD)pBrush ) ;
			if( pData->pDoc->BrushIsSelected( pBrush ) )
			{
				pData->BrushListbox->SetSel( Index, TRUE );
			}
		}
	}
	return GE_TRUE;
}

static geBoolean ListboxAddEntities (CEntity &Ent, void *lParam)
{
	lbAddData *pData = (lbAddData *)lParam;
			
	if (Ent.GetGroupId () == pData->pDoc->mCurrentGroup)
	{
		char	szTempString[32];
		int Index;

		sprintf( szTempString, "(E) %s", Ent.GetName() ) ;

		Index = pData->BrushListbox->AddString (szTempString);
		pData->ItemsInLB = GE_TRUE;

		if( Index >= 0 )	/* LB_ERR is -1, LB_ERRSPACE  is -2 */
		{
			pData->BrushListbox->SetItemData( Index, (DWORD)&Ent ) ;
			if( Ent.IsSelected() )
			{
				pData->BrushListbox->SetSel( Index, TRUE );
			}
		}
	}
	return GE_TRUE;
}

void CBrushGroupDialog::UpdateTabDisplay(CFusionDoc* NewDoc)
{
	BOOL	bItemsInLB = FALSE ;
	GroupListType *Groups;
	lbAddData CallbackData;


	pDoc	=NewDoc;
	Groups = Level_GetGroups (pDoc->pLevel);

	if( Group_GetCount( Groups ) == 0 )	// Hack for while doc's are switching without built groups
		return ;

	m_Visible.EnableWindow(1);
	GetDlgItem(IDC_SELECTBRUSHES)->EnableWindow(1);
	GetDlgItem(IDC_DESELECTBRUSHES)->EnableWindow(1);

	if(pDoc->mCurrentGroup)
	{
		GetDlgItem(IDC_REMOVEGROUP)->EnableWindow(1);
		m_ColorButton.EnableWindow(1);
	}
	else
	{
		m_BrushLock.EnableWindow(0);
		GetDlgItem(IDC_REMOVEGROUP)->EnableWindow(0);
	}

	// fill in name
//	char const *Name = Group_GetNameFromId (Groups, pDoc->mCurrentGroup);

	if(Group_IsVisible (Groups, pDoc->mCurrentGroup))
	{
		m_Visible.SetCheck (1);
	}
	else
	{
		m_Visible.SetCheck (0);
	}

	if(Group_IsLocked (Groups, pDoc->mCurrentGroup))
	{
		m_BrushLock.SetCheck (1);
	}
	else
	{
		m_BrushLock.SetCheck (0);
	}

	// fill brush list box with names of current brushes and entities
	m_BrushList.ResetContent ();

	CallbackData.BrushListbox = &m_BrushList;
	CallbackData.pDoc = pDoc;
	CallbackData.ItemsInLB = GE_FALSE;

	Level_EnumBrushes (pDoc->pLevel, &CallbackData, ::ListboxAddBrushes);
	Level_EnumEntities (pDoc->pLevel, &CallbackData, ::ListboxAddEntities);

	LoadComboBox() ;

	if( pDoc->mCurrentGroup == 0 )
		bItemsInLB = FALSE ;	// Never let them remove from the default group

	((CButton*)GetDlgItem( IDC_ADDTOCURRENT ))->EnableWindow( TRUE ) ;
	((CButton*)GetDlgItem( IDC_REMOVEFROMCURRENT ))->EnableWindow( bItemsInLB ) ;

}


// fill the combo box with the list of groups and ids
void CBrushGroupDialog::LoadComboBox(void)
{
	CComboBox		*GCombo	=(CComboBox *)GetDlgItem(IDC_GROUPCOMBO);
	COLORREF		Color ;
	const GE_RGBA * pGeColor ;
	GroupListType	*Groups = Level_GetGroups (pDoc->pLevel);

	assert(GCombo);

	GroupList_FillCombobox (Groups, GCombo, pDoc->mCurrentGroup);

	pGeColor = Group_GetColor( Groups, pDoc->mCurrentGroup ) ;
	Color = RGB( pGeColor->r, pGeColor->g, pGeColor->b ) ;
//		Color = RGB( 255, 255, 255 ) ;

	m_ColorButton.SetColor( Color ) ;

}/* CBrushGroupDialog::LoadComboBox */


void CBrushGroupDialog::OnOK() 
{
	GroupListType *Groups = Level_GetGroups (pDoc->pLevel);
	BrushList *BList = Level_GetBrushes (pDoc->pLevel);
	CEntityArray *Entities = Level_GetEntities (pDoc->pLevel);

	// Set visible and lock state on all group brushes/entities
	if (m_Visible.GetCheck ())
	{
		Group_Show (Groups, pDoc->mCurrentGroup, BList, Entities);
	}
	else
	{
		Group_Hide (Groups, pDoc->mCurrentGroup, BList, Entities);
	}

	if (m_BrushLock.GetCheck ())
	{
		Group_Lock (Groups, pDoc->mCurrentGroup, BList, Entities);
	}
	else
	{
		Group_Unlock (Groups, pDoc->mCurrentGroup, BList, Entities);
	}
	pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);

	// Temporary Location -- Restore focus to active view
	CMDIChildWnd *pMDIChild	=(CMDIChildWnd *)pDoc->mpMainFrame->MDIGetActive();
	if(pMDIChild)
	{
		CView	*cv	=(CView *)pMDIChild->GetActiveView();
		if( cv )
			cv->SetFocus() ;
	}
//	CDialog::OnOK();
}

void CBrushGroupDialog::OnSelectbrushes()
{
	// Add all brushes/entities in the current group to the selected list
	pDoc->SelectGroupBrushes (TRUE, pDoc->mCurrentGroup);
	m_BrushList.SetSel( -1, TRUE );
	OnOK();
}

void CBrushGroupDialog::OnDeselectbrushes()
{
	// Remove all brushes/entities in the current group from the selected list
	pDoc->SelectGroupBrushes (FALSE, pDoc->mCurrentGroup);
	m_BrushList.SetSel( -1, FALSE );
	OnOK();
}

void CBrushGroupDialog::OnCancel() 
{
	UpdateTabDisplay(pDoc);
}

// create a new brush group and add all currently-selected
// brushes and entities to the new group.
void CBrushGroupDialog::OnCreateNewGroup(void) 
{
	int	NewGroupId ;

	NewGroupId = pDoc->MakeNewBrushGroup( this ) ;
	if( NewGroupId != 0 ) 
	{
		m_Visible.SetCheck( TRUE ) ;
		m_BrushLock.SetCheck( FALSE ) ;
		OnOK( );
		UpdateTabDisplay( pDoc ) ;
		pDoc->mpMainFrame->UpdateActiveDoc() ;
	}

}/* CBrushGroupDialog::OnCreateNewGroup */

void CBrushGroupDialog::OnAddToCurrent(void)
{
	pDoc->AddSelToGroup() ;
	OnOK();
	UpdateTabDisplay(pDoc);
}

void CBrushGroupDialog::OnSelChangeGroupCombo() 
{
	CComboBox		*GCombo	=(CComboBox *)GetDlgItem(IDC_GROUPCOMBO);

	assert(GCombo);

	if (pDoc != NULL)
	{
		int Id = GCombo->GetCurSel();

		if (Id != LB_ERR)
		{
			pDoc->mCurrentGroup = GCombo->GetItemData(Id);
		}

		//	CHANGE!	03/24/97	John Moore
		//	Go ahead and reset the currentbrush
		pDoc->CurBrush = pDoc->BTemplate;
		//	End of CHANGE
		pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		// Update the Toolbar
		pDoc->mpMainFrame->LoadComboBox() ;
	}	
	UpdateTabDisplay(pDoc);

}/* CBrushGroupDialog::OnSelChangeGroupCombo */

void CBrushGroupDialog::OnVisible() 
{
	OnOK();
}

// removed selected brushes/entities from current group
void CBrushGroupDialog::OnRemovefromcurrent(void) 
{
	pDoc->RemovesSelFromGroup() ;
	OnOK();
	UpdateTabDisplay(pDoc);
}

void CBrushGroupDialog::OnBrushlock() 
{
	int c ;
	
	if( m_BrushLock.GetCheck () )
	{
		// If ANY items in the group are selected, select them all
		c = m_BrushList.GetSelCount() ;
		if( c > 0 )
		{
			OnSelectbrushes() ;
		}
		// Then disable the LB until the user unlocks the group
		m_BrushList.EnableWindow( FALSE ) ;
	}
	else
	{
		m_BrushList.EnableWindow( TRUE ) ;
	}
	
	OnOK();
}/* CBrushGroupDialog::OnBrushlock */

void CBrushGroupDialog::OnSelchangeBrushlist() 
{
	int			c ;
	int			i ;
	Brush	*	b;
	CEntity *	pEnt;
	char		szBuffer[64]; // Should have a limit for entity/brush name
	geBoolean	bChanged = FALSE ;	// You get selchanges on cursor movements that don't change selection...

	c = m_BrushList.GetCount() ;
	if( c > 0 )
	{
		for( i=0; i<c; i++ )
		{
			m_BrushList.GetText( i, szBuffer ) ;
			if( szBuffer[1] == 'B' )
			{
				b = (Brush *)m_BrushList.GetItemData( i );
				if( m_BrushList.GetSel( i ) )
				{	// User says brush should be selected
					if( pDoc->BrushIsSelected( b ) == GE_FALSE )
					{
						pDoc->DoBrushSelection( b, brushSelToggle ) ;
						bChanged = GE_TRUE ;
					}
				}
				else
				{	// User says don't select
					if( pDoc->BrushIsSelected( b ) == GE_TRUE )
					{
						pDoc->DoBrushSelection( b, brushSelToggle ) ;
						bChanged = GE_TRUE ;
					}
				}
			}// Item is Brush
			else if( szBuffer[1] == 'E' )
			{
				pEnt = (CEntity *)m_BrushList.GetItemData( i );	
				if( m_BrushList.GetSel( i ) )
				{
					if( pEnt->IsSelected() == GE_FALSE )
					{	// User says entity should be selected
						pDoc->DoEntitySelection( pEnt ) ;
						bChanged = GE_TRUE ;
					}
				}
				else
				{
					if( pEnt->IsSelected() == GE_TRUE )
					{	// User says entity should not be selected
						pDoc->DoEntitySelection( pEnt ) ;
						bChanged = GE_TRUE ;
					}
				}

			}// Item is Entity	
		}// Loop thru Listbox
	}// There are items in the Listbox

	if( bChanged )
	{
		pDoc->UpdateSelected() ;
		pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
	}

}/* CBrushGroupDialog::OnSelchangeBrushlist */


void CBrushGroupDialog::UpdateGroupSelection( void ) 
{
#if 1
	#pragma message ("Group selection stuff badly farkled")
#else
	int			c ;
	int			i ;
	Brush	*	b;
	CEntity *	pEnt;
	char		szBuffer[64]; // Should have a limit for entity/brush name
	BOOL		bSelect ;

	c = m_BrushList.GetCount() ;
	if( c > 0 )
	{
		for( i=0; i<c; i++ )
		{
			m_BrushList.GetText( i, szBuffer ) ;
			if( szBuffer[1] == 'B' )
			{
				b = (Brush *)m_BrushList.GetItemData( i );
				if( Brush_GetGroupId(b) == pDoc->mCurrentGroup )
				{
					bSelect = pDoc->BrushIsSelected( b ) ;
					m_BrushList.SetSel( i, bSelect );
				}
			}// Item is Brush
			else if( szBuffer[1] == 'E' )
			{
				pEnt = (CEntity *)m_BrushList.GetItemData( i );	
				if( pEnt->GetGroupId () == pDoc->mCurrentGroup )
				{
					bSelect = pEnt->IsSelected() ;
					m_BrushList.SetSel( i, bSelect );
				}
			}// Item is Entity	
		}// Loop thru Listbox
	}// There are items in the Listbox
#endif
}/* CBrushGroupDialog::UpdateGroupSelection */


void CBrushGroupDialog::OnRemovegroup() 
{
	GroupIterator	gi;
	GroupListType *Groups = Level_GetGroups (pDoc->pLevel);
	BrushList *BList = Level_GetBrushes (pDoc->pLevel);
	CEntityArray *Entities = Level_GetEntities (pDoc->pLevel);

	// Nuke the current group
	Group_RemoveFromList (Groups, pDoc->mCurrentGroup, BList, Entities);

	// Find another group for our current one
	pDoc->mCurrentGroup = Group_GetFirstId( Groups, &gi ) ;
	pDoc->SetModifiedFlag() ;

	m_BrushList.ResetContent() ;
	UpdateTabDisplay(pDoc);
	LoadComboBox( );

}/* CBrushGroupDialog::OnRemovegroup */


LRESULT CBrushGroupDialog::OnChangeColor( WPARAM wParam, LPARAM lParam )
{
	COLORREF	color ;
	GE_RGBA		geColor ;
	GroupListType *Groups = Level_GetGroups (pDoc->pLevel);

	color = m_ColorButton.GetColor() ;
	geColor.r = GetRValue( color ) ;
	geColor.g = GetGValue( color ) ;
	geColor.b = GetBValue( color ) ;
	Group_SetColor ( Groups, pDoc->mCurrentGroup, &geColor ) ;
	pDoc->SetModifiedFlag() ;
	pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
	return TRUE ;
	wParam;
	lParam;
}/* CBrushGroupDialog::OnChangeColor */
