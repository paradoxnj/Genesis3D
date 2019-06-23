/****************************************************************************************/
/*  MainFrm.cpp                                                                         */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Moore                        */
/*  Description:  MFC frame stuff with a bit of other misc ui stuff                     */
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
#include "MainFrm.h"

#include "resource.h"

#include "FusionTabControls.h"
#include "ModelDialog.h"
#include "FUSIONDoc.h"
#include "FUSIONView.h"
#include "brushgroupdialog.h"
#include "units.h"

#include <assert.h>


#include "Fusion.h"		// major icko!!
#include "ChildFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT indicators[] =
{
	ID_SEPARATOR,				// Text Area on left status line indicator
	ID_INDICATOR_SLOCK,			// Lock pane
	ID_INDICATOR_SELINFO,		// Selection Info pane
	ID_INDICATOR_WORLDPOS,		// World Coordinates of cursor
	ID_INDICATOR_CURSORINFO,	// Cursor Info (What would select if you click)
	ID_INDICATOR_GRID,			// Grid Info 
	ID_INDICATOR_SNAP			// Snap Info
};

typedef enum eINDICATORS
{
	ID_SLOCK_PANE = 1,
	ID_SELINFO_PANE,
	ID_WORLDPOS_PANE,
	ID_CURSORINFO_PANE,
	ID_GRIDINFO_PANE,
	ID_SNAPINFO_PANE
} ;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_WINDOW_NEW, OnWindowNew)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SLOCK, OnUpdateSLock)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SELINFO, OnUpdateSelInfo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_WORLDPOS, OnUpdateWorldPos)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CURSORINFO, OnUpdateCursorInfo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_GRID, OnUpdateGridInfo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_SNAP, OnUpdateSnapInfo)
	ON_COMMAND(ID_GROUPS_MAKENEWGROUP, OnBrushGroupsMakenewgroup)
	ON_COMMAND(IDM_MODEBAR, OnModebar)
	ON_UPDATE_COMMAND_UI(IDM_MODEBAR, OnUpdateModebar)
	ON_COMMAND(IDM_GROUPBAR, OnGroupbar)
	ON_UPDATE_COMMAND_UI(IDM_GROUPBAR, OnUpdateGroupbar)
	ON_COMMAND(IDM_TABBAR, OnTabbar)
	ON_UPDATE_COMMAND_UI(IDM_TABBAR, OnUpdateTabbar)
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEWLEAKFINDER, OnViewLeakFinder)
	ON_UPDATE_COMMAND_UI(ID_VIEWLEAKFINDER, OnUpdateViewLeakFinder)
	ON_CBN_SELCHANGE(ID_TOOLBAR_COMBOBOX, OnSelchangeGroupList)
	//}}AFX_MSG_MAP
	// Global help commands
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame
	(
	  void
	) : IsStartingApp(1), IsDestroyingApp(0)
{
}


CMainFrame::~CMainFrame()
{
	if (m_wndTabControls)
		delete m_wndTabControls;

	//	Set the app's mainframe pointer to null...
//	CFusionApp* pApp = ( CFusionApp* ) AfxGetApp();
//	pApp->pMainFrame = NULL;
}

#pragma warning (disable:4100)
void CMainFrame::OnUpdateSLock(CCmdUI *pCmdUI)
{
	CFusionDoc *pDoc;

	pDoc = this->GetCurrentDoc ();
	if (pDoc != NULL)
	{
		if (pDoc->IsSelectionLocked()) 
		{
			m_wndStatusBar.SetPaneText(ID_SLOCK_PANE, "SLOCK");
		}
		else 
		{
			m_wndStatusBar.SetPaneText(ID_SLOCK_PANE, "");
		}
		return;
	}
	m_wndStatusBar.SetPaneText(ID_SLOCK_PANE, "");
}
#pragma warning (default:4100)

void CMainFrame::OnUpdateSelInfo(CCmdUI *pCmdUI)
{
	CString Text = "0 items selected";

	pCmdUI->Enable();

	CFusionDoc *pDoc;

	pDoc = this->GetCurrentDoc ();
	if(pDoc)
	{
		CString etxt, btxt, ftxt;
		int NumSelFaces = SelFaceList_GetSize (pDoc->pSelFaces);
		int NumSelBrushes = SelBrushList_GetSize (pDoc->pSelBrushes);

		etxt.Format("Ent: %d", pDoc->NumSelEntities);
		btxt.Format(" Brsh: %d", NumSelBrushes);
		ftxt.Format(" Face: %d", NumSelFaces);
		Text.Format("%s%s%s",
			pDoc->NumSelEntities ? etxt : "",
			NumSelBrushes ? btxt : "",
			NumSelFaces ? ftxt : "");

		switch (pDoc->mAdjustMode)
		{
			case ADJUST_MODE_BRUSH :
				Text = "BRUSHMODE:"+Text;
				break;
			case ADJUST_MODE_FACE :
				Text = "FACEMODE:"+Text;
				break;
			default :
				assert (0);	// can't happen??
//				pCmdUI->SetText( Text);
				break;
		}
	}
	pCmdUI->SetText (Text) ;
}

void CMainFrame::OnUpdateCursorInfo(CCmdUI *pCmdUI)
{
	char info[256];
	CFusionDoc* pDoc;

	pCmdUI->Enable();
	pDoc = this->GetCurrentDoc ();

	if (pDoc != NULL)
	{
		pDoc->GetCursorInfo(info, 15);
		pCmdUI->SetText( info );
		return;
	}
	pCmdUI->SetText( "" );
}

void CMainFrame::OnUpdateWorldPos(CCmdUI *pCmdUI)
{
	CPoint		CursorPos;
	char		stuff[100];
	CFusionView * pView ;
	CWnd		* pWnd ; 
	geVec3d		wp ;

	pCmdUI->Enable() ;
	GetCursorPos(&CursorPos);
	pWnd = WindowFromPoint( CursorPos );

	if( pWnd != NULL )
	{
		if( pWnd->IsKindOf( RUNTIME_CLASS (CFusionView)) )
		{
			pView = (CFusionView *)pWnd ;
			if( pView->mViewType == ID_VIEW_TOPVIEW ||
				pView->mViewType == ID_VIEW_FRONTVIEW ||
				pView->mViewType == ID_VIEW_SIDEVIEW )
			{
				pView->ScreenToClient( &CursorPos ) ; 
				Render_ViewToWorld( pView->VCam, CursorPos.x, CursorPos.y, &wp ) ;

				CFusionDoc	*pDoc	=pView->GetDocument();
				if(pDoc)
				{
					if (Level_GetGridType (pDoc->pLevel) == GridMetric)
					{
						geVec3d_Scale (&wp, Units_EngineToCentimeters (1.0f), &wp);
					}
				}
				sprintf( stuff, "% 4d,% 4d,% 4d", (int)wp.X, (int)wp.Y, (int)wp.Z );
				pCmdUI->SetText( stuff ) ;
				return ;
			}
		}
	}

	m_wndStatusBar.SetPaneText( ID_WORLDPOS_PANE, "" );
}

void CMainFrame::OnUpdateGridInfo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
	pCmdUI->SetText( szGridString );
}


void CMainFrame::OnUpdateSnapInfo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable();
	pCmdUI->SetText( szSnapString ) ;
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndToolBar.Create(this) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if( !CreateStyleBar() )
	{
		TRACE0("Failed to create brush toolbar\n");
		return -1;      // fail to create
	}

	if( !CreateGroupBar() )
	{
		TRACE0("Failed to create group toolbar\n");
		return -1;      // fail to create
	}

	//	CHANGE!	04/03/97	John Moore
	if( !CreateTabBar() )
	{
		TRACE0("Failed to create tab bar\n");
		return -1;
	}
	//	End of CHANGE

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	UINT pane_id;
	UINT pane_style;
	int pane_width;

	m_wndStatusBar.GetPaneInfo( ID_SLOCK_PANE,		pane_id, pane_style, pane_width );
	m_wndStatusBar.SetPaneInfo( ID_SLOCK_PANE,		pane_id, pane_style, 40);
	m_wndStatusBar.GetPaneInfo( ID_SELINFO_PANE,	pane_id, pane_style, pane_width );
	m_wndStatusBar.SetPaneInfo( ID_SELINFO_PANE,	pane_id, pane_style, 200);

	m_wndStatusBar.GetPaneInfo( ID_WORLDPOS_PANE,	pane_id, pane_style, pane_width );
	m_wndStatusBar.SetPaneInfo( ID_WORLDPOS_PANE,	pane_id, pane_style, 100);
	
	m_wndStatusBar.GetPaneInfo( ID_CURSORINFO_PANE, pane_id, pane_style, pane_width );
	m_wndStatusBar.SetPaneInfo( ID_CURSORINFO_PANE, pane_id, pane_style, 100);
	

	m_wndToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndBrushToolBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndGroupBar.SetBarStyle(m_wndToolBar.GetBarStyle() |
		CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

	m_wndToolBar.SetWindowText( "General" ) ;
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndBrushToolBar.SetWindowText( "Mode" ) ;
	m_wndBrushToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndGroupBar.SetWindowText( "Group" ) ;
	m_wndGroupBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndTabBar.SetWindowText( "Command Panel" ) ;
	m_wndTabBar.EnableDocking( CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT );

	EnableDocking(CBRS_ALIGN_ANY);

	DockControlBar(&m_wndTabBar, AFX_IDW_DOCKBAR_RIGHT);
	DockControlBar(&m_wndToolBar, AFX_IDW_DOCKBAR_TOP );
	DockControlBarLeftOf(&m_wndBrushToolBar, &m_wndToolBar );
	DockControlBarLeftOf(&m_wndGroupBar, &m_wndBrushToolBar );

	m_CB_FUSION_BRUSH_FORMAT = RegisterClipboardFormat( "FUSIONBRUSHDATA" );
	m_CB_FUSION_ENTITY_FORMAT = RegisterClipboardFormat( "FUSIONENTITYDATA" );
	LoadBarState( "DESKTOP" ) ;	

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	return CMDIFrameWnd::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::CreateStyleBar()
{
//	const int nDropHeight = 200;
							  
	if (!m_wndBrushToolBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_TOP|
			CBRS_TOOLTIPS|CBRS_FLYBY, IDR_BRUSHTOOLS ) ||
		!m_wndBrushToolBar.LoadToolBar(IDR_BRUSHTOOLS) )
	{
		TRACE0("Failed to create stylebar\n");
		return FALSE;       // fail to create
	}
/*
	// Create the combo box
	m_wndBrushToolBar.SetButtonInfo(20, ID_AXIS_X, TBBS_SEPARATOR, 110);

	// Design guide advises 12 pixel gap between combos and buttons
	m_wndBrushToolBar.SetButtonInfo(19, ID_SEPARATOR, TBBS_SEPARATOR, 12);
	CRect rect;
	m_wndBrushToolBar.GetItemRect(20, &rect);
	rect.top = 3;
	rect.bottom = rect.top + nDropHeight;
	if (!m_wndBrushToolBar.m_comboBox.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP,
			rect, &m_wndBrushToolBar, ID_TOOLBAR_COMBOBOX))
	{
		TRACE0("Failed to create combo-box\n");
		return FALSE;
	}

	//  Create a font for the combobox
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));

	if (!::GetSystemMetrics(SM_DBCSENABLED))
	{
		// Since design guide says toolbars are fixed height so is the font.
#ifndef _MAC
		logFont.lfHeight = -12;
#else
		logFont.lfHeight = -14;     // looks better than 12 on Mac
#endif
		logFont.lfWeight = FW_BOLD;
		logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lstrcpy(logFont.lfFaceName, "MS Sans Serif");
		if (!m_wndBrushToolBar.m_font.CreateFontIndirect(&logFont))
			TRACE0("Could Not create font for combo\n");
		else
			m_wndBrushToolBar.m_comboBox.SetFont(&m_wndBrushToolBar.m_font);
	}
	else
	{
		m_wndBrushToolBar.m_font.Attach(::GetStockObject(SYSTEM_FONT));
		m_wndBrushToolBar.m_comboBox.SetFont(&m_wndBrushToolBar.m_font);
	}
*/
	return TRUE;
}


BOOL CMainFrame::CreateGroupBar()
{					
	const int nDropHeight = 225 ;

	if (!m_wndGroupBar.Create(this, WS_CHILD|WS_VISIBLE|CBRS_TOP|
			CBRS_TOOLTIPS|CBRS_FLYBY, IDR_GROUPBAR ) ||
		!m_wndGroupBar.LoadToolBar(IDR_GROUPBAR) )
	{
		TRACE0("Failed to create groupbar\n");
		return FALSE;       // fail to create
	}


	// Create the combo box
	m_wndGroupBar.SetButtonInfo(7, ID_AXIS_X, TBBS_SEPARATOR, 110);

	// Design guide advises 12 pixel gap between combos and buttons
	m_wndGroupBar.SetButtonInfo(6, ID_SEPARATOR, TBBS_SEPARATOR, 12);
	CRect rect;
	m_wndGroupBar.GetItemRect(7, &rect);
	rect.top = 3;
	rect.bottom = rect.top + nDropHeight;
	if (!m_wndGroupBar.m_comboBox.Create(
			CBS_DROPDOWNLIST|WS_VISIBLE|WS_TABSTOP,
			rect, &m_wndGroupBar, ID_TOOLBAR_COMBOBOX))
	{
		TRACE0("Failed to create combo-box\n");
		return FALSE;
	}

	//  Create a font for the combobox
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));

	if (!::GetSystemMetrics(SM_DBCSENABLED))
	{
		// Since design guide says toolbars are fixed height so is the font.

		logFont.lfHeight = -12;
		logFont.lfWeight = FW_BOLD;
		logFont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
		lstrcpy(logFont.lfFaceName, "MS Sans Serif");
		if (!m_wndGroupBar.m_font.CreateFontIndirect(&logFont))
			TRACE0("Could Not create font for combo\n");
		else
			m_wndGroupBar.m_comboBox.SetFont(&m_wndGroupBar.m_font);
	}
	else
	{
		m_wndGroupBar.m_font.Attach(::GetStockObject(SYSTEM_FONT));
		m_wndGroupBar.m_comboBox.SetFont(&m_wndGroupBar.m_font);
	}

	return TRUE;
}


//	CHANGE!	04/03/97	John Moore
//	First create the tab bar and initialize the tab controls...
BOOL CMainFrame::CreateTabBar()
{
	if (!m_wndTabBar.Create(this, IDD_TABDIALOG, WS_CHILD | CBRS_RIGHT |
		CBRS_TOOLTIPS | CBRS_FLYBY, IDD_TABDIALOG) )
	{
		TRACE0("Failed to create tab bar\n");
		return FALSE;       // fail to create
	}

	//	Create the property sheet and its pages...
	m_wndTabControls = new CFusionTabControls( &m_wndTabBar );

	//	Now set the area for our tabs...
	RECT rect;
	CWnd* WndPtr = m_wndTabBar.GetDlgItem( IDC_GROUP1 );
	WndPtr->GetClientRect( &rect );

	if (!m_wndTabControls->Create( WS_CHILD|WS_VISIBLE|TCS_MULTILINE, rect, &m_wndTabBar, 0 ))
	{
		TRACE0( "CFusionTabControls: Create(...) Failed\n" );
		return FALSE;
	}

	TabFont.CreateFont
	(
		12, 0, 
		0, 0, 
		FW_BOLD,
		0, 0, 0,
		ANSI_CHARSET, 
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, 
		DRAFT_QUALITY,
		DEFAULT_PITCH|FF_DONTCARE,
		"MS Sans Serif"
	) ;
	m_wndTabControls->SetFont( &TabFont ) ;

	//	Now let's set the position of our tabs...
	WINDOWPLACEMENT	WndPlace;
	WndPtr->GetWindowPlacement( &WndPlace );
	m_wndTabControls->SetWindowPos( NULL, WndPlace.rcNormalPosition.left,
			WndPlace.rcNormalPosition.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
	m_wndTabControls->m_WindowPlacement = WndPlace;

	m_wndTabControls->CreateTabs();

	return TRUE;
}
//	End of CHANGE


// fill the combo box with the list of groups and ids
void CMainFrame::LoadComboBox()
{
	CFusionDoc *pDoc;

	pDoc = this->GetCurrentDoc ();
	m_wndGroupBar.m_comboBox.ResetContent() ;
	if (pDoc != NULL)
	{
		GroupListType *Groups = Level_GetGroups (pDoc->pLevel);
		if( Groups )
		{
			if( Group_GetCount( Groups ) )
			{
				pDoc->FillBrushGroupCombo( m_wndGroupBar.m_comboBox ) ;
			}// There are groups
		}// Groups Ptr
	}// Good Document
}/* CMainFrame::LoadComboBox*/

void CMainFrame::UpdateModelsDialog
	(
	  void
	)
{
	if ( m_wndTabControls->ModelTab != NULL)
	{
		CFusionDoc *pDoc;

		pDoc = this->GetCurrentDoc ();
		if(pDoc)
		{
			m_wndTabControls->ModelTab->Update (pDoc, Level_GetModelInfo (pDoc->pLevel));
		}
	}
}


CFrameWnd* CMainFrame::CreateNewGameViewFrame(CRuntimeClass* pViewClass,  CDocTemplate* pTemplate, CDocument* pDoc, CFrameWnd* pOther)
{
	// make sure we have a doc
	if (pDoc != NULL) 
	{
		ASSERT_VALID(pDoc);
	}

	// create a frame wired to the specified document
	CCreateContext context;
	context.m_pCurrentFrame = pOther;
	context.m_pCurrentDoc = pDoc;
	context.m_pNewViewClass = pViewClass;
	context.m_pNewDocTemplate = pTemplate;

	// make the object
	CFrameWnd* pFrame = (CFrameWnd*)(TEMPLATE_CHILD_FRAME_CLASS->CreateObject());
	if (pFrame == NULL)
	{
		TRACE1("Warning: Dynamic create of frame %hs failed.\n",
			TEMPLATE_CHILD_FRAME_CLASS->m_lpszClassName);
		return NULL;
	}
	ASSERT_KINDOF(CFrameWnd, pFrame);

	// create new from resource
	if (!pFrame->LoadFrame(TEMPLATE_RESOURCE_ID,
			WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE,   // default frame styles
			NULL, &context))
	{
		TRACE0("Warning: CDocTemplate couldn't create a frame.\n");
		// frame will be deleted in PostNcDestroy cleanup
		return NULL;
	}

	// it worked !
	return pFrame;
}

void CMainFrame::MakeNewView( CRuntimeClass* pViewRuntimeClass)
{
	CDocument* pDocument;
	CChildFrame *pActiveChild;

	pActiveChild =(CChildFrame *)this->MDIGetActive();

	pDocument = this->GetCurrentDoc ();
/*
	if (pActiveChild == NULL ||
	  (pDocument = pActiveChild->GetActiveDocument()) == NULL)
	{
		TRACE0("Warning: No active document for WindowNew command.\n");
		AfxMessageBox(AFX_IDP_COMMAND_FAILURE);
		return;     // command failed
	}
*/
	// otherwise we have a new frame !
	CDocTemplate* pTemplate = pDocument->GetDocTemplate();
	ASSERT_VALID(pTemplate);
	CFrameWnd* pFrame = CreateNewGameViewFrame(pViewRuntimeClass, pTemplate, pDocument, pActiveChild);
	if (pFrame == NULL)
	{
		TRACE0("Warning: failed to create new frame.\n");
		return;     // command failed
	}

	pTemplate->InitialUpdateFrame(pFrame, pDocument);
}


static int GetMag10
	(
	  int Num
	)
/*
  Returns the number of the multiple of 10 that is
  less than or equal to the number.  That is,
  0 thru 9 returns 0
  10 thru 99 returns 1
  100 thru 999 returns 2
  etc.

  This will work for negative numbers, although the
  return value is positive.  (i.e. -10 thru -99 returns 1)
*/
{
	int Mag;
	int MyNum;

	Mag = 0;
	MyNum = Num/10;
	while (MyNum != 0)
	{
		++Mag;
		MyNum /= 10;		
	}
	return Mag;
}

// update the size of the grid in the pane
void CMainFrame::UpdateGridSize(geFloat GridSize, int SnapOn, int snapto, int gunits, int snapunits)
{
	int	GridIndex;
	static const char *GridUnits[] =
		{"CENTI", "DECI", "METER", "10MTR", "100METER", "KILO" };

	assert (GridSize > 0);
	assert (GridSize < 1000000);
	assert (snapto > 0);
	assert (snapto < 1000000);

	if(!gunits)
	{
		//use plus one to compensate for fp error
		GridIndex = GetMag10((int)Units_EngineToCentimeters(GridSize)+1);

		// build grid units into string
		sprintf( szGridString, "Grid: %s", GridUnits[GridIndex]);
	}
	else
	{
		// build grid units into string
		sprintf( szGridString, "Grid: %d Texel", (int)GridSize);
	}

	// append the snap information.
	if(SnapOn)
	{
		int SnapIndex;

		// build snap units and append to grid string
		if(!snapunits)
		{
			SnapIndex = GetMag10 (snapto);
			sprintf( szSnapString, "Snap: %s", GridUnits[SnapIndex]);
		}
		else
		{
			sprintf( szSnapString, "Snap: %d Texel", snapto);
		}

	}
	else
	{
		strcpy( szSnapString, "Off" ) ;
	}
}

void CMainFrame::OnWindowNew() 
{
	CDocument *pDoc;

	pDoc = this->GetCurrentDoc ();
	if (pDoc != NULL)
	{
		CChildFrame* pActiveChild;
	
		pActiveChild =(CChildFrame *)this->MDIGetActive();
		// Create a new frame !
		CDocTemplate* pTemplate = pDoc->GetDocTemplate();
		ASSERT_VALID(pTemplate);

		CFrameWnd* pFrame = pTemplate->CreateNewFrame(pDoc, pActiveChild);
		if (pFrame == NULL)
		{
			TRACE0("Warning: failed to create new frame.\n");
			return;     // command failed
		}

		pTemplate->InitialUpdateFrame(pFrame, pDoc);
	}
}

CWnd* CMainFrame::GetTabControlWindow()
{
	return ( (CWnd*) m_wndTabControls );
}

const char	*CMainFrame::GetCurrentTexture()
{
	return m_wndTabControls->GetCurrentTexture();
}


void CMainFrame::OnBrushGroupsMakenewgroup() 
{
	/*
	  Things to do here:
	    1) Get the current document
		2) Create a new group in that document's list
		3) Update the combo box with the group info
		4) Set the document's current group to the newly created group
	*/
	CFusionDoc *pDoc;

	pDoc = this->GetCurrentDoc ();
	if (pDoc != NULL)
	{
		if (pDoc->MakeNewBrushGroup ( this ))
		{
			this->LoadComboBox ();
		}
	}
}/* CMainFrame::OnBrushGroupsMakenewgroup */

CFusionDoc *CMainFrame::GetCurrentDoc 
	(
	  void
	)
{
	CChildFrame* pActiveChild;
	CFusionDoc *pDoc;
	
	pDoc = NULL;
	pActiveChild =(CChildFrame *)this->MDIGetActive();

	if (pActiveChild != NULL)
	{
		pDoc =(CFusionDoc*)pActiveChild->GetActiveDocument();
	}
	return pDoc;
}

void CMainFrame::UpdateActiveDoc
	(
	  void
	)
{
	CChildFrame	*pActiveChild	=(CChildFrame *)this->MDIGetActive();
	if(pActiveChild)
	{
		CFusionDoc	*pDoc		=(CFusionDoc*)pActiveChild->GetActiveDocument();

		// update groups tab
		if(pDoc)
		{
			if(m_wndTabControls)
			{
				if(m_wndTabControls->GrpTab)
				{
					m_wndTabControls->GrpTab->UpdateTabDisplay(pDoc);
				}
			}
			LoadComboBox() ;
		}
	}
	// update models list box
	this->UpdateModelsDialog ();
}

void CMainFrame::UpdateSelectedModel 
	(
	  int MoveRotate,
	  geVec3d const *pVecDelta
	)
{
	CModelDialog *ModelTab = GetModelTab ();

	if (ModelTab != NULL)
	{
		ModelTab->UpdateSelectedModel( MoveRotate, pVecDelta );
	}
}

CModelDialog *CMainFrame::GetModelTab (void)
{
	return m_wndTabControls->ModelTab;
}

BOOL CMainFrame::OnNotify
	( 
	  WPARAM wParam, 
	  LPARAM lParam, 
	  LRESULT* pResult
	)
{
	LPNMHDR pHeader;

	pHeader = (LPNMHDR)lParam;
	if (pHeader != NULL)
	{
		// child frame sends a WM_SETFOCUS notification 
		// when it gets the focus
		if (pHeader->code == WM_SETFOCUS)
		{
			UpdateActiveDoc ();
		}
	}
	return CMDIFrameWnd::OnNotify (wParam, lParam, pResult);
}


void CMainFrame::SelectTab( int nTabIndex )
{
	CFusionTabControls *	pTabCtrl ;
	long					index ;

	pTabCtrl = (CFusionTabControls*) GetTabControlWindow() ;
	if( pTabCtrl != NULL )
	{
		if( pTabCtrl->m_CurrentTab != nTabIndex )
		{		
			pTabCtrl->SetCurSel( nTabIndex ) ;
			pTabCtrl->OnSelchange(NULL, (LRESULT*)&index);
		}
	}
}/* CMainFrame::SelectTab */

void CMainFrame::OnModebar() 
{
	CControlBar* pBar = (CControlBar*)&m_wndBrushToolBar ;

	if( pBar )
	{
		ShowControlBar( pBar, (pBar->GetStyle() & WS_VISIBLE) == 0, FALSE ) ;
	}
}

void CMainFrame::OnUpdateModebar(CCmdUI* pCmdUI) 
{
	CControlBar* pBar = (CControlBar*)&m_wndBrushToolBar ;
	if (pBar != NULL)
    {
		pCmdUI->SetCheck((pBar->GetStyle() & WS_VISIBLE) != 0);
	}
}

void CMainFrame::OnGroupbar() 
{
	CControlBar* pBar = (CControlBar*)&m_wndGroupBar ;

	if( pBar )
	{
		ShowControlBar( pBar, (pBar->GetStyle() & WS_VISIBLE) == 0, FALSE ) ;
	}
}

void CMainFrame::OnUpdateGroupbar(CCmdUI* pCmdUI) 
{
	CControlBar* pBar = (CControlBar*)&m_wndGroupBar ;
	if (pBar != NULL)
    {
		pCmdUI->SetCheck((pBar->GetStyle() & WS_VISIBLE) != 0);
	}
}

void CMainFrame::OnTabbar() 
{
	CControlBar* pBar = (CControlBar*)&m_wndTabBar ;

	if( pBar )
	{
		ShowControlBar( pBar, (pBar->GetStyle() & WS_VISIBLE) == 0, FALSE ) ;
	}
}

void CMainFrame::OnUpdateTabbar(CCmdUI* pCmdUI) 
{
	CControlBar* pBar = (CControlBar*)&m_wndTabBar ;
	if (pBar != NULL)
    {
		pCmdUI->SetCheck((pBar->GetStyle() & WS_VISIBLE) != 0);
	}
}

void CMainFrame::OnClose() 
{
	SaveBarState( "DESKTOP" ) ;	
	CMDIFrameWnd::OnClose();
}

void CMainFrame::DockControlBarLeftOf(CToolBar* Bar,CToolBar* LeftOf)
{
	CRect rect;
	DWORD dw;
	UINT n;

	// get MFC to adjust the dimensions of all docked ToolBars
	// so that GetWindowRect will be accurate
	RecalcLayout();
	LeftOf->GetWindowRect(&rect);
	rect.OffsetRect(1,0);
	dw=LeftOf->GetBarStyle();
	n = 0;
	n = (dw&CBRS_ALIGN_TOP) ? AFX_IDW_DOCKBAR_TOP : n;
	n = (dw&CBRS_ALIGN_BOTTOM && n==0) ? AFX_IDW_DOCKBAR_BOTTOM : n;
	n = (dw&CBRS_ALIGN_LEFT && n==0) ? AFX_IDW_DOCKBAR_LEFT : n;
	n = (dw&CBRS_ALIGN_RIGHT && n==0) ? AFX_IDW_DOCKBAR_RIGHT : n;

	// When we take the default parameters on rect, DockControlBar will dock
	// each Toolbar on a seperate line.  By calculating a rectangle, we in effect
	// are simulating a Toolbar being dragged to that location and docked.
	DockControlBar(Bar,n,&rect);
}

void CMainFrame::OnViewLeakFinder() 
{
	CFusionDoc	*pDoc	=this->GetCurrentDoc();

	if(pDoc != NULL)
	{
		pDoc->SetShowLeakFinder(!pDoc->bShowLeakFinder());
		pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
	}
}

void CMainFrame::OnUpdateViewLeakFinder(CCmdUI* pCmdUI) 
{
	geBoolean	bEnable	=GE_FALSE;
	CFusionDoc	*pDoc	=this->GetCurrentDoc();

	if(pDoc != NULL)
	{
		bEnable	=pDoc->IsLeakFileLoaded();
	}	
	pCmdUI->Enable(bEnable) ;

	//should this be enabled even if greyed?
	//so you know that it will display when loaded?
	pCmdUI->SetCheck(pDoc->bShowLeakFinder());
}

void CMainFrame::OnSelchangeGroupList ()
{
	CFusionDoc *pDoc = GetCurrentDoc ();
	int CurSel;

	CurSel = m_wndGroupBar.m_comboBox.GetCurSel ();
	if (CurSel != LB_ERR)
	{
		pDoc->mCurrentGroup = m_wndGroupBar.m_comboBox.GetItemData (CurSel);
		m_wndTabControls->GrpTab->UpdateTabDisplay (pDoc);
	}
}

