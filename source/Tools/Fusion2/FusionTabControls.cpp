/****************************************************************************************/
/*  FusionTabControls.cpp                                                               */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Moore                        */
/*  Description:  Tab control ui stuff                                                  */
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

/*	This class will handle all coordination between the user tab interface
	and the current document.  If the user clicks on a tab, it has to update
	itself using information from the current document.  If the user makes
	changes in the tab, it has to make sure that the current document is
	updated in return... */

#include "stdafx.h"
#include "FusionTabControls.h"
#include "BrushEntityDialog.h"
#include "TextureDialog.h"
#include "ChildFrm.h"
#include "brushgroupdialog.h"
#include "FusionDoc.h"
#include "Model.h"
#include "ModelDialog.h"
#include "SkyDialog.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFusionTabControls

CFusionTabControls::CFusionTabControls( CDialogBar* pDBar)
{
	//	Create a pointer to the main window...
	m_pMainFrame		=(CMainFrame *)AfxGetMainWnd();
	LastView			=(CMainFrame *)AfxGetMainWnd();

	m_pBrushEntityDialog=NULL;
	m_pTextureDialog	=NULL;
	ConTab				=NULL;
	GrpTab				=NULL;
	ModelTab			=NULL;
	SkyTab				=NULL;

	//	Create a pointer to the main window's CDialogBar...
	m_pDialogBar		=pDBar;
}

CFusionTabControls::~CFusionTabControls()
{
	if( m_pBrushEntityDialog )
	{
		delete m_pBrushEntityDialog;
		m_pBrushEntityDialog = NULL;
	}
	if( m_pTextureDialog )
	{
		delete m_pTextureDialog;
		m_pTextureDialog = NULL;
	}
	if(ConTab)
	{
		delete ConTab;
		ConTab=NULL;
	}
	if(GrpTab)
	{
		delete GrpTab;
		GrpTab=NULL;
	}
	if( ModelTab)
	{
		delete ModelTab ;
		ModelTab = NULL ;
	}
	if (SkyTab)
	{
		delete SkyTab;
		SkyTab = NULL;
	}
}/* ~CFusionTabControls */


BEGIN_MESSAGE_MAP(CFusionTabControls, CTabCtrl)
	//{{AFX_MSG_MAP(CFusionTabControls)
	ON_NOTIFY_REFLECT(TCN_SELCHANGE, OnSelchange)
	ON_WM_SETFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFusionTabControls message handlers

struct TabItem
{
	UINT WhichTab;
	char *Text;
};

static const TabItem Tabs[] = 
{
	{BRUSH_ENTITY_TAB,	"Template"},
	{TEXTURE_TAB,		"Textures"},
	{GROUP_TAB,			"Groups"},
	{CONSOLE_TAB,		"Console"},
	{MODEL_TAB,			"Models"},
	{SKY_TAB,			"Sky"}
};
	 
static const int NumTabs = sizeof (Tabs) / sizeof (TabItem);

BOOL CFusionTabControls::CreateTabs()
{
	int tab;

	for (tab = 0; tab < NumTabs; ++tab)
	{
		TC_ITEM item;
	
		item.mask = TCIF_TEXT;
		item.pszText = Tabs[tab].Text;
		InsertItem (Tabs[tab].WhichTab, &item);
	}

	m_CurrentTab = GetCurSel();

	return TRUE;
}

#pragma warning (disable:4100)
void CFusionTabControls::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	//	Which tab is now currently selected?
	m_CurrentTab = GetCurSel();	

	UpdateTabs();

	*pResult = 0;
}
#pragma warning (default:4100)

//	This handles overall tab updates.  NEEDS TO BE CALLED RIGHT AFTER DOCUMENT CREATION!
void CFusionTabControls::UpdateTabs()
{
	CFusionDoc		*	pDoc = NULL;
	CChildFrame		*	pActiveChild ;
	int			UpdateBrushEntity	=1;
	int			UpdateTexture		=1;

	pActiveChild = (CChildFrame *)m_pMainFrame->MDIGetActive() ;
	if(pActiveChild != NULL)
	{
		pDoc = (CFusionDoc*) pActiveChild->GetActiveDocument() ;
	}
	if (pActiveChild == NULL || pDoc == NULL )
	{	//	There are no open documents right now...
		return;     // command failed
	}

	//	Let's create the tab dialogs for the first time if not done so...
	if( m_pBrushEntityDialog == NULL )
	{
		m_pBrushEntityDialog = new CBrushEntityDialog( this, pDoc );
		UpdateBrushEntity = 0;
	}
	else
	{
		//	Let's make it invisible...
		m_pBrushEntityDialog->ShowWindow( SW_HIDE );
	}

	if( m_pTextureDialog == NULL )
	{
		m_pTextureDialog = new CTextureDialog( this, pDoc );
		UpdateTexture = 0;
	}
	if(ConTab==NULL)
	{
		ConTab=new CConsoleTab(this, pDoc);
	}
	if(GrpTab==NULL)
	{
		GrpTab=new CBrushGroupDialog(this, pDoc);
	}
	if( ModelTab==NULL )
	{
		ModelTab=new CModelDialog( this ) ;
		ModelTab->Create( IDD_MODELKEY, this ) ;
	}
	if (SkyTab == NULL)
	{
		SkyTab = new CSkyDialog (this, pDoc);
	}

	ConTab->ShowWindow(SW_HIDE);
	ConTab->SetConHwnd();

	GrpTab->ShowWindow(SW_HIDE);
	ModelTab->ShowWindow(SW_HIDE);
	SkyTab->ShowWindow (SW_HIDE);

	//	This one should always be invisible.  Even to start out with...
	m_pTextureDialog->ShowWindow( SW_HIDE );

	//	Update all tabs...
	ModelTab->Update(pDoc, Level_GetModelInfo (pDoc->pLevel)) ;
	m_pBrushEntityDialog->Update( pDoc );
	m_pTextureDialog->Update( pDoc );
	GrpTab->UpdateTabDisplay(pDoc);
	SkyTab->Update (pDoc);

	//	Which tab do we show..?
	switch( m_CurrentTab )
	{
		case BRUSH_ENTITY_TAB:
			if( UpdateBrushEntity )
			{
				//	Make it visible...
				m_pBrushEntityDialog->ShowWindow( SW_SHOW );
			}
			break;

		case TEXTURE_TAB:
			if( UpdateTexture )
			{
				//	Make it visible...
				m_pTextureDialog->ShowWindow( SW_SHOW );
			}
			break;

		case GROUP_TAB:
			GrpTab->ShowWindow(SW_SHOW);
			break;

		case CONSOLE_TAB:
			ConTab->ShowWindow( SW_SHOW );
			break;

		case MODEL_TAB:
			ModelTab->ShowWindow( SW_SHOW ) ;
			break ;

		case SKY_TAB :
			SkyTab->ShowWindow (SW_SHOW);
			break;
	}

}


const char *CFusionTabControls::GetCurrentTexture()
{
	return m_pTextureDialog->GetCurrentTexture();
}

void	CFusionTabControls::SelectTexture(int SelNum)
{
	if(m_CurrentTab != TEXTURE_TAB)
	{
		m_CurrentTab	=TEXTURE_TAB;
		SetCurSel (TEXTURE_TAB);
		UpdateTabs();
	}
	m_pTextureDialog->SelectTexture(SelNum);
}

void CFusionTabControls::UpdateTextures (void)
{
	m_pTextureDialog->m_TxlibChanged = true;
	if (m_CurrentTab == TEXTURE_TAB)
	{
		UpdateTabs ();
	}
}


//	Use this whenever the user closes all documents...
void CFusionTabControls::DisableAllTabDialogs()
{
	if( m_pBrushEntityDialog )
		m_pBrushEntityDialog->ShowWindow( SW_HIDE );
	if(ConTab)
		ConTab->ShowWindow(SW_HIDE);
	if(GrpTab)
		ConTab->ShowWindow(SW_HIDE);
	if(ModelTab)
		ModelTab->ShowWindow(SW_HIDE);
	if( m_pTextureDialog )
	{
		m_pTextureDialog->Update( NULL );
		m_pTextureDialog->ShowWindow( SW_HIDE );
	}
	if (SkyTab != NULL)
	{
		SkyTab->ShowWindow (SW_HIDE);
	}
}/* CFusionTabControls::DisableAllTabDialogs */


void CFusionTabControls::OnSetFocus(CWnd* pOldWnd) 
{
//	CTabCtrl::OnSetFocus(pOldWnd);
	LastView=pOldWnd;
}
