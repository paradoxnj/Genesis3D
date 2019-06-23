/****************************************************************************************/
/*  FusionView.cpp                                                                      */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Moore                        */
/*  Description:  MFC view stuff...  Alot of the editor UI functionality is here        */
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
#include "FUSIONView.h"
#include "FUSIONDoc.h"
#include "KeyEditDlg.h"
#include <assert.h>

#include "FusionTabControls.h"
#include "BrushEntityDialog.h"
#include "units.h"
#include "face.h"
#include "ram.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define AXIS_X	0x1
#define AXIS_Y	0x2
#define AXIS_Z	0x4

#define MAX_PIXEL_DRAG_START_DIST (12.0f)

int CFusionView::mCY_DRAG = 2 ;
int CFusionView::mCX_DRAG = 2 ;

IMPLEMENT_DYNCREATE(CFusionView, CView)

BEGIN_MESSAGE_MAP(CFusionView, CView)
	ON_MESSAGE(WM_USER_COMPILE_MSG, OnCompileMessage)
	ON_MESSAGE(WM_USER_COMPILE_ERR, OnCompileError)
	ON_MESSAGE(WM_USER_COMPILE_DONE, OnCompileDone)
	//{{AFX_MSG_MAP(CFusionView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_TOOLS_CAMERA, OnToolsCamera)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_CAMERA, OnUpdateToolsCamera)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_COMMAND(ID_AXIS_X, OnAxisX)
	ON_UPDATE_COMMAND_UI(ID_AXIS_X, OnUpdateAxisX)
	ON_COMMAND(ID_AXIS_Y, OnAxisY)
	ON_UPDATE_COMMAND_UI(ID_AXIS_Y, OnUpdateAxisY)
	ON_COMMAND(ID_AXIS_Z, OnAxisZ)
	ON_UPDATE_COMMAND_UI(ID_AXIS_Z, OnUpdateAxisZ)
	ON_COMMAND(ID_TOOLS_BRUSH_MOVEROTATEBRUSH, OnToolsBrushMoverotatebrush)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_MOVEROTATEBRUSH, OnUpdateToolsBrushMoverotatebrush)
	ON_COMMAND(ID_TOOLS_BRUSH_SCALEBRUSH, OnToolsBrushScalebrush)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_SCALEBRUSH, OnUpdateToolsBrushScalebrush)
	ON_COMMAND(ID_TOOLS_BRUSH_SHOWBRUSH, OnToolsBrushShowbrush)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_SHOWBRUSH, OnUpdateToolsBrushShowbrush)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_TOOLS_BRUSH_SHEARBRUSH, OnToolsBrushShearbrush)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_SHEARBRUSH, OnUpdateToolsBrushShearbrush)
	ON_UPDATE_COMMAND_UI(ID_GROUPS_MAKENEWGROUP, OnUpdateBrushGroupsMakenewgroup)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_GROUPS_ADDTOGROUP, OnUpdateBrushGroupsAddtogroup)
	ON_COMMAND(ID_TOOLS_BRUSH_ROTATE90, OnToolsBrushRotate45)
	ON_COMMAND(ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES, OnToolsBrushMoveselectedbrushes)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES, OnUpdateToolsBrushMoveselectedbrushes)
	ON_COMMAND(ID_TOOLS_TEMPLATE, OnToolsTemplate)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_TEMPLATE, OnUpdateToolsTemplate)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_ROTATE90, OnUpdateToolsBrushRotate45)
	ON_COMMAND(ID_BRUSH_REMOVESELECTEDFROMGROUP, OnBrushRemoveselectedfromgroup)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_REMOVESELECTEDFROMGROUP, OnUpdateBrushRemoveselectedfromgroup)
	ON_COMMAND(ID_BRUSH_GROUPS_ADDTOGROUP, OnBrushGroupsAddtogroup)
	ON_COMMAND(ID_DESELECTALL, OnDeselectall)
	ON_UPDATE_COMMAND_UI(ID_DESELECTALL, OnUpdateDeselectall)
	ON_COMMAND(ID_SELECTALL, OnSelectall)
	ON_UPDATE_COMMAND_UI(ID_SELECTALL, OnUpdateSelectall)
	ON_COMMAND(ID_TOOLS_SCALEWORLD, OnToolsScaleworld)
	ON_COMMAND(ID_TOOLS_BRUSH_MAKENEWEST, OnToolsBrushMakenewest)
	ON_COMMAND(ID_TOOLS_SETTEXTURESCALE, OnToolsSettexturescale)
	ON_COMMAND(ID_TOOLS_NEXTBRUSH, OnToolsNextbrush)
	ON_COMMAND(ID_TOOLS_PREVBRUSH, OnToolsPrevbrush)
	ON_COMMAND(ID_TOOLS_ADDTOLEVEL, OnToolsAddtolevel)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_ADDTOLEVEL, OnUpdateToolsAddtolevel)
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN, OnUpdateViewZoomin)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomout)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUT, OnUpdateViewZoomout)
	ON_COMMAND(ID_CENTERTHING, OnCenterthing)
	ON_WM_MBUTTONUP()
	ON_UPDATE_COMMAND_UI(ID_CENTERTHING, OnUpdateCenterthing)
	//}}AFX_MSG_MAP
	ON_COMMAND_RANGE( ID_VIEW_3DWIREFRAME, ID_VIEW_TEXTUREVIEW, OnViewType)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_3DWIREFRAME, ID_VIEW_TEXTUREVIEW, OnViewTypeCmdUi)
END_MESSAGE_MAP()


#define SIDE_LEFT 1
#define SIDE_RIGHT 2
#define SIDE_TOP 4
#define SIDE_BOTTOM 8

void CFusionView::OnUpdateCenterthing(CCmdUI* pCmdUI) 
{
	CFusionDoc *pDoc = GetDocument ();

	if ((pDoc->mModeTool == ID_TOOLS_TEMPLATE) &&
	    ((mViewType == ID_VIEW_TOPVIEW) || (mViewType == ID_VIEW_SIDEVIEW) || (mViewType == ID_VIEW_FRONTVIEW)))
	{
		pCmdUI->Enable (TRUE);
	}
	else
	{
		pCmdUI->Enable (FALSE);
	}
}

// Center the template brush or entity in the selected view.
// Doesn't work for 3d views...
void CFusionView::OnCenterthing()
{
	CFusionDoc *pDoc = GetDocument ();
	// only works on templates
	if ((pDoc->mModeTool != ID_TOOLS_TEMPLATE) ||
	    ((mViewType != ID_VIEW_TOPVIEW) && (mViewType != ID_VIEW_SIDEVIEW) && (mViewType != ID_VIEW_FRONTVIEW)))
	{
		return;
	}

	geVec3d NewWorldPos = Render_GetViewCenter (VCam);

	// Get the current thing's position...
	geVec3d CurrentThingPos;

	if (pDoc->TempEnt)
	{
		CurrentThingPos = pDoc->mRegularEntity.mOrigin;
	}
	else
	{
		if (pDoc->CurBrush == NULL)
		{
			return;
		}
		Brush_Center (pDoc->CurBrush, &CurrentThingPos);
	}

	// Compute delta required to get thing to NewWorldPos.
	// One dimension won't be changed (i.e. in the top view, the Y won't be modified)
	geVec3d MoveDelta;

	geVec3d_Subtract (&NewWorldPos, &CurrentThingPos, &MoveDelta);

	switch (mViewType)
	{
		case ID_VIEW_TOPVIEW :
			MoveDelta.Y = 0.0f;
			break;

		case ID_VIEW_SIDEVIEW :
			MoveDelta.X = 0.0f;
			break;

		case ID_VIEW_FRONTVIEW :
			MoveDelta.Z = 0.0f;
			break;

		default :
			// don't do nothin!
			assert (0);
	}

	// We've computed the delta, so move the thing...
	pDoc->MoveTemplateBrush (&MoveDelta);

	pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
}

static geBoolean IsKeyDown (int Key)
{
	int KeyState;

	KeyState = ::GetAsyncKeyState (Key);
	return (KeyState & 0x8000);
}

// Prevent axis movement.  View space clipping.
void CFusionView::LockAxisView (int *dx, int *dy)
{
	CFusionDoc *pDoc = GetDocument ();
	int mLockAxis = pDoc->GetLockAxis ();

	switch (mViewType)
	{
		case ID_VIEW_TOPVIEW :
			if (mLockAxis & AXIS_X) *dx = 0;
			if (mLockAxis & AXIS_Z) *dy = 0;
			break;

		case ID_VIEW_SIDEVIEW :
			if (mLockAxis & AXIS_X) *dx = 0;
			if (mLockAxis & AXIS_Y) *dy = 0;
			break;
			
		case ID_VIEW_FRONTVIEW :
			if (mLockAxis & AXIS_Z) *dx = 0;
			if (mLockAxis & AXIS_Y) *dy = 0;
			break;
	}
}

// Prevent axis movement.  World space clipping.
void CFusionView::LockAxis( geVec3d * pWP )
{
	int mLockAxis ;
	CFusionDoc* pDoc = GetDocument();

	mLockAxis = pDoc->GetLockAxis() ;

	if( mLockAxis & AXIS_X )	pWP->X = 0.0f ;
	if( mLockAxis & AXIS_Y )	pWP->Y = 0.0f ;
	if( mLockAxis & AXIS_Z )	pWP->Z = 0.0f ;
}/* CFusionView::LockAxis */

void CFusionView::OnToolsBrushRotate45()
{
	geVec3d	rp;

	CFusionDoc* pDoc = GetDocument();

	geVec3d_Clear (&rp);
	switch (mViewType)
	{
		case ID_VIEW_TOPVIEW:
			rp.Y = -(M_PI/4.0f);
			break;
		case ID_VIEW_FRONTVIEW :
			rp.Z = -(M_PI/4.0f);
			break;
		case ID_VIEW_SIDEVIEW:
			rp.X = (M_PI/4.0f);
			break;
		default :
			return;
	}

	if(GetModeTool()!=ID_GENERALSELECT)
	{
		pDoc->RotateTemplateBrush(&rp);
		pDoc->UpdateSelected();
		pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
	}
	else
	{
		pDoc->RotateSelectedBrushesDirect (&rp);
		pDoc->UpdateAllViews(UAV_ALL3DVIEWS | REBUILD_QUICK, NULL);
	}
}


int CFusionView::GetCursorBoxPos (const POINT *ptMousePos)
{
	CFusionDoc *pDoc;
	const Box3d *pBrushBox;
	POINT ptMin, ptMax;
	int dx, dy;
	int x, y;
	int horiz, vert;
	int lookup[4] = {1, 2, 2, 3};

	/*
	  Split the box up into 3 sections vertically and 3 horizontally.
	  The center sections are 1/2 the width and height, and the end sections are
	  each 1/4 of width and height.

	  What we're creating is:

			0	   1       2        3	  4
				------------------------
				|     |          |     |
			5	|  6  |    7     |  8  |  9
				------------------------
				|     |          |     |
				|     |          |     | 
		   10	|  11 |   12     |  13 |  14
				|     |          |     |
				------------------------
				|     |          |     | 
		   15	|  16 |   17     |  18 |  19
				------------------------
		   20      21	  22	    23	  24
	  
	  Then we determine which of the sections the cursor is closest to,
	  and return that index.
	*/

	pDoc = GetDocument ();

	pBrushBox = Brush_GetBoundingBox (pDoc->CurBrush);

	// obtain screen coordinates for bounding box min and max points
	ptMin = Render_OrthoWorldToView (VCam, &pBrushBox->Min);
	ptMax = Render_OrthoWorldToView (VCam, &pBrushBox->Max);

	// make sure the min and max points are correct...
	if (ptMin.x > ptMax.x)
	{
		int temp;

		temp = ptMin.x;
		ptMin.x = ptMax.x;
		ptMax.x = temp;
	}
	if (ptMin.y > ptMax.y)
	{
		int temp;

		temp = ptMin.y;
		ptMin.y = ptMax.y;
		ptMax.y = temp;
	}

	// compute horizontal first
	x = ptMousePos->x - ptMin.x;
	dx = (ptMax.x - ptMin.x);
	if (dx == 0) horiz = 0; else horiz = (4*x) / dx;
	if (horiz < 0) horiz = 0;
	else if (horiz > 3) horiz = 4;
	else horiz = lookup[horiz];

	// and vertical
	y = ptMousePos->y - ptMin.y;
	dy = (ptMax.y - ptMin.y);
	if (dy == 0) vert = 0; else vert = (4*y)/dy;
	if (vert < 0) vert = 0;
	else if (vert > 3) vert = 3;
	else vert = lookup[vert];

	// return index...
	return (vert * 5) + horiz;
}

void CFusionView::SetEditCursor (int Tool, const POINT *pMousePos)
{
	//for sizing stuff
	static const char *SizeCursors[25]=
	{
		IDC_SIZENWSE,	IDC_SIZENWSE,	IDC_SIZENS,		IDC_SIZENESW,	IDC_SIZENESW,
		IDC_SIZENWSE,	IDC_SIZENWSE,	IDC_SIZENS,		IDC_SIZENESW,	IDC_SIZENESW,	
		IDC_SIZEWE,		IDC_SIZEWE,		IDC_NO,			IDC_SIZEWE,		IDC_SIZEWE,
		IDC_SIZENESW,	IDC_SIZENESW,	IDC_SIZENS,		IDC_SIZENWSE,	IDC_SIZENWSE,
		IDC_SIZENESW,	IDC_SIZENESW,	IDC_SIZENS,		IDC_SIZENWSE,	IDC_SIZENWSE
	};

	static const char *ShearCursors[25]=
	{
		IDC_NO,			IDC_SIZEWE,		IDC_SIZEWE,		IDC_SIZEWE,		IDC_NO,
		IDC_SIZENS,		IDC_NO,			IDC_SIZEWE,		IDC_NO,			IDC_SIZENS,
		IDC_SIZENS,		IDC_SIZENS,		IDC_NO,			IDC_SIZENS,		IDC_SIZENS,
		IDC_SIZENS,		IDC_NO,			IDC_SIZEWE,		IDC_NO,			IDC_SIZENS,
		IDC_NO,			IDC_SIZEWE,		IDC_SIZEWE,		IDC_SIZEWE,		IDC_NO
	};

	const char *WhichCursor = NULL;
	int CursorIndex;

	assert ((Tool == ID_TOOLS_BRUSH_SCALEBRUSH) || (Tool == ID_TOOLS_BRUSH_SHEARBRUSH));

	// Determine where the cursor is on the box surrounding the selected brush,
	// and set the appropriate cursor.
	if (pMousePos->x < 0 || pMousePos->y < 0)
	{
		return;
	}
	CursorIndex = GetCursorBoxPos (pMousePos);
	switch (Tool)
	{
		case ID_TOOLS_BRUSH_SCALEBRUSH :
			// Scaling it's just a simple lookup
			WhichCursor = SizeCursors [CursorIndex];
			break;

		case ID_TOOLS_BRUSH_SHEARBRUSH :
			WhichCursor = ShearCursors[CursorIndex];
			break;
		default :
			assert (0);
			break;
	}
	SetCursor (AfxGetApp()->LoadStandardCursor (WhichCursor));
}

void CFusionView::Pan
	(
	  CFusionDoc *pDoc,
	  int dx, int dy,
	  const geVec3d *dv,
	  BOOL LButtonIsDown, 
	  BOOL RButtonIsDown
	)
{
	if(mViewIs3d)
	{
		geVec3d MoveVec;

		if(LButtonIsDown && RButtonIsDown)
		{
			geVec3d_Set (&MoveVec, (float)-dx, (float)-dy, 0.0f);
			Render_MoveCamPos( VCam, &MoveVec ) ;
			pDoc->UpdateCameraEntity( VCam ) ;
		}
		else if (LButtonIsDown)
		{
			geVec3d_Set (&MoveVec, 0.0f, 0.0f, (float)-dy);
			Render_MoveCamPos( VCam, &MoveVec ) ;
			if (Render_UpIsDown (VCam))
			{
				Render_IncrementYaw (VCam, (float)(-dx));
			}
			else
			{
				Render_IncrementYaw(VCam, (float)dx);
			}
			pDoc->UpdateCameraEntity( VCam ) ;
		}
		else if (RButtonIsDown)
		{
			if (Render_UpIsDown (VCam))
			{
				Render_IncrementYaw (VCam, (float)(-dx));
			}
			else
			{
				Render_IncrementYaw(VCam, (float)dx);
			}
			Render_IncrementPitch(VCam, (float)dy);
			pDoc->UpdateCameraEntity( VCam ) ;
		}
	}
	else
	{
		geVec3d dcamv;
		
		geVec3d_Scale (dv, -1.0f, &dcamv);
		if (LButtonIsDown)
		{
			Render_MoveCamPosOrtho(VCam, &dcamv);
		}
		else if (RButtonIsDown)
		{
			Render_ZoomChange(VCam, -(((float)dy) * 0.001f));
			pDoc->UpdateGridInformation ();
		}
	}
}

void CFusionView::ScaleSelected (CFusionDoc *pDoc, int dx, int dy)
{
	//smooth out the zoom scale curve with a scalar
	float	ZoomInv	=Render_GetZoom(VCam);

	ZoomInv	=(ZoomInv > .5)? 0.5f / ZoomInv : 1.0f;

	// negated here because Brush_Resize is still thinking weird
	pDoc->ResizeSelected (-(((float)dx)*ZoomInv), -(((float)dy)*ZoomInv), sides, Render_GetInidx(VCam));
}

void CFusionView::ShearSelected (CFusionDoc *pDoc, int dx, int dy)
{
	//smooth out the zoom scale curve with a scalar
	float	ZoomInv	=Render_GetZoom(VCam);

	ZoomInv	=(ZoomInv > .5)? 0.5f / ZoomInv : 1.0f;

	pDoc->ShearSelected(-(((float)dy)*ZoomInv), -(((float)dx)*ZoomInv), sides, Render_GetInidx(VCam));
}

#pragma warning (disable:4100)
void CFusionView::OnMouseMove (UINT nFlags, CPoint point)
{
	int			dx, dy;
	geVec3d		sp, wp, dv;
	CFusionDoc	*pDoc;
	geBoolean	ShiftHeld;
	geBoolean	ControlHeld;
	geBoolean	LButtonIsDown, RButtonIsDown;
	geBoolean	SpaceHeld;
	BOOL		ThisIsCaptured;
	int			ModeTool, Tool;
	fdocAdjustEnum AdjustMode;
	BOOL		DoRedraw = TRUE;
	POINT		RealCursorPosition;


	pDoc	=GetDocument();
	ThisIsCaptured = (this == GetCapture ());
	ModeTool = GetModeTool ();
	Tool		= GetTool ();
	AdjustMode  = GetAdjustMode ();

	/*
	  You'll notice here that we don't use the nFlags parameter to get these
	  settings.  Those flags tell what the state of things was when the
	  MouseMove message was received, which could have been several seconds
	  ago (due to delays caused by a mashed key, for example).  What we're
	  really interested is the current state, so we can throw out old messages.
	*/
	ShiftHeld = IsKeyDown (VK_SHIFT);
	ControlHeld = IsKeyDown (VK_CONTROL);
	LButtonIsDown = IsKeyDown (VK_LBUTTON);
	RButtonIsDown = IsKeyDown (VK_RBUTTON);
	SpaceHeld	= IsKeyDown (VK_SPACE);

	IsPanning	=((SpaceHeld || IsPanning) &&
				  (LButtonIsDown | RButtonIsDown) && ThisIsCaptured);

	if (!ThisIsCaptured)
	{
		// Mouse isn't captured.  So we're just moving the mouse around
		// in this view.  If we're not in camera mode and not panning,
		// then set the cursor accordingly and exit.
		if(!((ModeTool == ID_TOOLS_CAMERA) || IsPanning))
		{
			int Tool;

			Tool = GetTool ();
			if (mViewIs3d)
			{
				if (ShiftHeld)
				{
					SetCursor (AfxGetApp()->LoadCursor (IDC_EYEDROPPER));
				}
				else
				{
					SetCursor (AfxGetApp()->LoadStandardCursor (IDC_ARROW));
				}
			}
			else if ((Tool == ID_TOOLS_BRUSH_SCALEBRUSH) || (Tool == ID_TOOLS_BRUSH_SHEARBRUSH))
			{
				SetEditCursor (Tool, &point);
			}
		}
		return;
	}

	
	if(this==GetParentFrame()->GetActiveView())
	{
		pDoc->mActiveView	=mViewType;
	}

	/*
	  The point parameter to this message gives us the mouse position when the
	  message was received.  That could be old.  We really want the *current*
	  position, so we get it here and convert it to client coordinates.  This
	  prevents the panning runaway bug that plauged us for so long.
	*/
	::GetCursorPos (&RealCursorPosition);
	ScreenToClient (&RealCursorPosition);

	dx = (RealCursorPosition.x - mStartPoint.x);
	dy = (RealCursorPosition.y - mStartPoint.y);
	
	if ((dx == 0) && (dy == 0))	// don't do anything if no delta
	{
		return;
	}
	
	Render_ViewToWorld(VCam, mStartPoint.x, mStartPoint.y, &sp);
	Render_ViewToWorld(VCam, RealCursorPosition.x, RealCursorPosition.y, &wp);
	geVec3d_Subtract(&wp, &sp, &dv);	// delta in world space

	//camera and space hold panning
	if ((ModeTool == ID_TOOLS_CAMERA)||IsPanning)
	{
		Pan (pDoc, dx, dy, &dv, LButtonIsDown, RButtonIsDown);
	}
	else if (!mViewIs3d)
	// none of this stuff should be available in the 3d view.
	{
		switch (ModeTool)
		{
			case ID_GENERALSELECT :
				switch (Tool)
				{
					case CURTOOL_NONE :
						// no tool selected.  We're doing a drag select or clone operation
						if (AdjustMode == ADJUST_MODE_BRUSH)
						{
							// drag select or cloning
							if( IsDragging )
							{
								mDragCurrentPoint.x += (long)dx ;
								mDragCurrentPoint.y += (long)dy ;
							}
							else if( !IsCopying && !ShiftHeld && !RButtonIsDown )
							{
#pragma message ("Logic flawed here when space being held.  Don't know exactly what.")
								if((abs(dx) > mCX_DRAG) || (abs(dy) > mCY_DRAG))
								{
									mDragCurrentPoint = RealCursorPosition;
									IsDragging = TRUE ;
								}
							}// Drag Select
							else
							{	// Begin a copy operation			
								if((LButtonIsDown && ShiftHeld)&&(!IsCopying))
								{
									IsCopying	=TRUE;
									pDoc->CopySelectedBrushes();
									if (SelBrushList_GetSize (pDoc->pSelBrushes) > 0)
									{
										// make current brush point to first brush in list
										// so we can snap correctly.
										pDoc->CurBrush = SelBrushList_GetBrush (pDoc->pSelBrushes, 0);
									}
								}
								if(IsCopying)
								{
									LockAxis( &dv ) ;
									if(LButtonIsDown)
									{
										pDoc->MoveSelectedClone(&dv);
									}
									SetTool(CURTOOL_NONE);
								}
							}// Not Drag Select
						}
						break;

					case ID_TOOLS_BRUSH_MOVEROTATEBRUSH :
						// moving/rotating brushes and entities
						SetTool(ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES);

						if (LButtonIsDown)
						{
							LockAxis( &dv ) ;
							pDoc->MoveSelectedBrushes(&dv);
						}// LButtonDown
						if (RButtonIsDown)
						{
							if( pDoc->GetSelState() == ONEENTITYONLY )	// Angle,Arc,Radius control
							{
								if( !ShiftHeld && !ControlHeld )
								{
									pDoc->AdjustEntityAngle( VCam, (float)dx ) ;
								}
								else if( ShiftHeld && !ControlHeld )
								{
									pDoc->AdjustEntityArc( VCam, (float)dx ) ;
								}
								else if( !ShiftHeld && ControlHeld )
								{
									pDoc->AdjustEntityRadius( &dv ) ;
								}
							}
							else
							{			
								Render_ViewDeltaToRotation (VCam, (float)dx, &dv);
								pDoc->RotateSelectedBrushes(&dv);
							}
						}// RButtonDown
						SetTool(ID_TOOLS_BRUSH_MOVEROTATEBRUSH);
						break;

					case ID_TOOLS_BRUSH_SCALEBRUSH :
						if (LButtonIsDown)
						{
							LockAxisView (&dx, &dy);
							ScaleSelected (pDoc, dx, dy);
						}
						break;

					case ID_TOOLS_BRUSH_SHEARBRUSH :
						if (LButtonIsDown)
						{
							LockAxisView (&dx, &dy);
							ShearSelected (pDoc, dx, dy);
						}
						break;

					default :
						DoRedraw = FALSE;
						break;
				}
				break;

			case ID_TOOLS_TEMPLATE :
				switch (Tool)
				{
					case ID_TOOLS_BRUSH_MOVEROTATEBRUSH :
						if (LButtonIsDown)
						{
							LockAxis (&dv);
							pDoc->MoveTemplateBrush(&dv);
						}
						else if (RButtonIsDown)
						{
							Render_ViewDeltaToRotation (VCam, (float)dx, &dv);
							pDoc->RotateTemplateBrush(&dv);
						}
						break;

					case ID_TOOLS_BRUSH_SCALEBRUSH :
						if (LButtonIsDown)
						{
							LockAxisView (&dx, &dy);
							ScaleSelected (pDoc, dx, dy);
						}
						break;

					case ID_TOOLS_BRUSH_SHEARBRUSH :
						if (LButtonIsDown)
						{
							LockAxisView (&dx, &dy);
							ShearSelected (pDoc, dx, dy);
						}
						break;

					default :
						DoRedraw = FALSE;
						break;
				}
				break;

			default :
				DoRedraw = FALSE;
				break;
		} // Mode Tool
	}// Ortho Views

	if (DoRedraw)
	{
		RedrawWindow();
	}

	POINT pt = mStartPoint;	// The position works on the delta mStartPoint...
	ClientToScreen( &pt ) ;
	SetCursorPos( pt.x, pt.y );	
}
#pragma warning (default:4100)

// Snaps the closest edge to SnapSize.
static float ComputeSnap (float Cur, float Delta, float SnapSize)
{
	float Target;
	float SnapDelta;
	float Remainder;

	Target = Cur + Delta;
	Remainder = (float)fmod (Target, SnapSize);
	if (fabs (Remainder) < (SnapSize / 2.0f))
	{
		SnapDelta = -Remainder;
	}
	else
	{
		if (Target < 0.0f)
		{
			SnapDelta = -(SnapSize + Remainder);
		}
		else
		{
			SnapDelta = SnapSize - Remainder;
		}
	}
	return SnapDelta;
}

static float SnapSide (float CurMin, float CurMax, float Delta, float SnapSize)
{
	float MinDelta, MaxDelta;

	MinDelta = ComputeSnap (CurMin, Delta, SnapSize);
	MaxDelta = ComputeSnap (CurMax, Delta, SnapSize);

	return (fabs (MinDelta) < fabs (MaxDelta)) ? MinDelta : MaxDelta;
}

void CFusionView::DoneMovingBrushes ()
{
	CFusionDoc *pDoc = GetDocument ();
	int ModeTool = GetModeTool ();

	if (SelBrushList_GetSize (pDoc->pSelBrushes) > 0 || ModeTool == ID_TOOLS_TEMPLATE)
	{
		geFloat fSnapSize ;
		const geVec3d *vMin, *vMax;
		const Box3d *pBox;
		geVec3d SnapDelta;
		geBoolean SnapX, SnapY, SnapZ;

		fSnapSize = 1.0f;
		if (Level_UseGrid (pDoc->pLevel))
		{
			fSnapSize = Level_GetGridSnapSize (pDoc->pLevel);
		}
		// do the snap thing...
		pBox = Brush_GetBoundingBox (pDoc->CurBrush);
		vMin = Box3d_GetMin (pBox);
		vMax = Box3d_GetMax (pBox);
		geVec3d_Clear (&SnapDelta);
		/*
		  In template mode, the brush is moved directly, so we have to snap to
		  the current position, not current position plus delta.  Since we
		  clear the delta before computing the snap, we have to save these
		  flags.
		*/
		SnapX = (pDoc->FinalPos.X != 0.0f);
		SnapY = (pDoc->FinalPos.Y != 0.0f);
		SnapZ = (pDoc->FinalPos.Z != 0.0f);
		if ((ModeTool == ID_TOOLS_TEMPLATE) || IsCopying)
		{
			geVec3d_Clear (&pDoc->FinalPos);
		}
		if (SnapX)
		{
			SnapDelta.X = ::SnapSide (vMin->X, vMax->X, pDoc->FinalPos.X, fSnapSize);
		}
		if (SnapY)
		{
			SnapDelta.Y = ::SnapSide (vMin->Y, vMax->Y, pDoc->FinalPos.Y, fSnapSize);
		}
		if (SnapZ)
		{
			SnapDelta.Z = ::SnapSide (vMin->Z, vMax->Z, pDoc->FinalPos.Z, fSnapSize);
		}
		if (ModeTool == ID_TOOLS_TEMPLATE)
		{
			pDoc->FinalPos = SnapDelta;
		}
		else
		{
			geVec3d_Add (&pDoc->FinalPos, &SnapDelta, &pDoc->FinalPos);
		}
	}

	pDoc->DoneMove ();

	pDoc->UpdateSelected();

	if ((ModeTool == ID_TOOLS_TEMPLATE) ||
		((pDoc->GetSelState() & ANYENTITY) && (!(pDoc->GetSelState() & ANYBRUSH))) )
	{			
		pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
	}
	else
	{
		pDoc->UpdateAllViews( UAV_ALL3DVIEWS | REBUILD_QUICK, NULL );
	}
}


#pragma warning (disable:4100)
void CFusionView::OnLButtonUp(UINT nFlags, CPoint point)
{
	BOOL		bWasCaptured = FALSE ;
	CFusionDoc* pDoc;
	int ModeTool;
	int Tool;
	fdocAdjustEnum AdjustMode;

	ModeTool = GetModeTool ();
	Tool = GetTool ();
	pDoc = GetDocument ();
	AdjustMode = GetAdjustMode ();

	LMouseButtonDown = GE_FALSE;
	if (!RMouseButtonDown)
	{
		// right mouse button isn't down
		if(this == GetCapture ())
		{
			bWasCaptured = TRUE ;
			ReleaseCapture();
		}

		if (IsKeyDown (VK_SPACE) || IsPanning || ModeTool == ID_TOOLS_CAMERA)
		{
			/*
			  Ok, here's the scoop.
			  If we're in the middle of a move/rotate and the user mashes the space bar,
			  we all of a sudden end up in panning mode with no context information to
			  tell us that we were previously moving/rotating.  So we end up with the
			  original brushes in the world, and the temporary brushes that are selected
			  and being moved also in the world.  So here we do a TempDeleteSelected to
			  remove those temporary brushes.  Otherwise they'd hang around forever.

			  Ideally, the move/rotate would be separated from the panning so that we could
			  move and pan at the same time.  Of course, I'd like to get rid of the whole
			  temp selected thing, too, but that'll have to wait...
			*/
			pDoc->TempDeleteSelected();

			IsPanning = FALSE;
			ShowTheCursor ();

			RedrawWindow();
			return;
		}

		if (mViewIs3d)
		{
			if(ModeTool==ID_TOOLS_TEMPLATE && pDoc->TempEnt)
			{
				pDoc->PlaceTemplateEntity3D(point, VCam);	
			}
			else if (IsKeyDown (VK_SHIFT))
			{
				pDoc->SelectTextureFromFace3D (point, VCam);
			}
			else
			{
				switch (AdjustMode)
				{
					case ADJUST_MODE_BRUSH :
					case ADJUST_MODE_FACE :
						pDoc->SelectRay (point, VCam);
						pDoc->UpdateAllViews (UAV_ALL3DVIEWS, NULL);
						break;
					
					default :
						break;
				}
			}
		}
		else
		{
			switch (Tool)
			{
				case CURTOOL_NONE :
					if (AdjustMode == ADJUST_MODE_BRUSH)
					{
						if( IsDragging )
						{
							pDoc->SelectOrthoRect( mDragStartPoint, mDragCurrentPoint, VCam );
							pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
							IsDragging = FALSE ;
						}
						else if(IsCopying)
						{
							DoneMovingBrushes ();
							IsCopying = FALSE;
						}
						else
						{
							pDoc->SelectOrtho(point, VCam);
							pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
						}
					}
					else
					{
						MessageBeep ((UINT)(-1));
					}

					break;

				case ID_TOOLS_BRUSH_MOVEROTATEBRUSH :
				case ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES :

						DoneMovingBrushes ();
					break; 

				case ID_TOOLS_BRUSH_SCALEBRUSH :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
					pDoc->SnapScaleNearest(sides, Render_GetInidx(VCam), VCam);
					if(pDoc->mLastOp == BRUSH_SCALE)
					{
						pDoc->DoneResize(sides, Render_GetInidx(VCam));
					}

					pDoc->UpdateSelected();
					if((ModeTool == ID_TOOLS_TEMPLATE) ||
					   ((pDoc->GetSelState() & ANYENTITY) && (!(pDoc->GetSelState() & ANYBRUSH))) )
					{			
						pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
					}
					else
					{
						pDoc->UpdateAllViews( UAV_ALL3DVIEWS | REBUILD_QUICK, NULL );
					}
					break;

				case ID_TOOLS_BRUSH_SHEARBRUSH :
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
					if(pDoc->mLastOp==BRUSH_SHEAR)
						pDoc->DoneShear(sides, Render_GetInidx(VCam));

					pDoc->UpdateSelected();
					if((ModeTool == ID_TOOLS_TEMPLATE) ||
					   ((pDoc->GetSelState() & ANYENTITY) && (!(pDoc->GetSelState() & ANYBRUSH))) )
					{			
						pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
					}
					else
					{
						pDoc->UpdateAllViews( UAV_ALL3DVIEWS | REBUILD_QUICK, NULL );
					}
					break;

				default :
					break;
			}
		}

		if( bWasCaptured )
		{
			ClientToScreen( &mStartPoint ) ;
			SetCursorPos( mStartPoint.x, mStartPoint.y ) ;
		}

		ShowTheCursor ();
	}

	assert( IsCopying == FALSE ) ;
}
#pragma warning (default:4100)


#pragma warning (disable:4100)
void CFusionView::OnLButtonDown(UINT nFlags, CPoint point)
{
	/*
	  These tables convert the index values returned by GetCursorBoxPos
	  into the bitmapped values expected by the brush routines (the "sides" values).
	  These sides values are encode the cursor's position in relation to the box
	  using this:

				   4
	           ----------
			   |		|
			1  |		|  2
			   |		|
			   |		|
	           ----------
				   8

	  So the cursor in the top-left corner of the box will be 5 (left+top = 1+4 = 5).
	*/
	static const int SideLookup[25] =
	{
		5,	5,	4,	6,	6,
		5,	5,	4,	6,	6,
		1,	1,	0,	2,	2,
		9,	9,	8,	10,	10,
		9,	9,	8,	10,	10
	};
	static const int SideLookupShear[25] =
	{
		0,	4,	4,	4,	0,
		1,	0,	4,	0,	2,
		1,	1,	0,	2,	2,
		1,	0,	8,	0,	2,
		0,	8,	8,	8,	0
	};
	int Tool = GetTool ();
	int ModeTool = GetModeTool ();
//	int AdjustMode = GetAdjustMode ();

	CFusionDoc* pDoc = GetDocument();

	LMouseButtonDown = GE_TRUE;

	geBoolean SpaceIsDown = IsKeyDown (VK_SPACE);

	assert( IsCopying == FALSE ) ;

	if (!RMouseButtonDown)
	{
/*
		if ((mViewIs3d == FALSE) && (AdjustMode == ADJUST_MODE_FACE))
		{
			::MessageBeep( (uint32)-1 ) ;
			return ;
		}
*/

		SetCapture();
		HideTheCursor ();
		mStartPoint = point;

		if (mViewIs3d || IsPanning || SpaceIsDown)
			return ;

		if ((Tool == CURTOOL_NONE))// && (ModeTool == ID_GENERALSELECT) && (AdjustMode == ADJUST_MODE_BRUSH))
		{
			mDragStartPoint = point ;	// Drag mode is initiated in MouseMove
			mDragCurrentPoint = point ;
		}// Drag Select
		else if (mViewIs3d == FALSE)
		{
			int CursorSide;

			CursorSide = GetCursorBoxPos (&point);
			if (Tool == ID_TOOLS_BRUSH_SHEARBRUSH)
			{
				sides = SideLookupShear[CursorSide];
			}
			else
			{
				sides = SideLookup[CursorSide];
			}

			if(Tool == ID_TOOLS_BRUSH_SCALEBRUSH)
			{
				pDoc->ScaleNum	=0;
			
				geVec3d_Set (&pDoc->FinalScale, 1.0f, 1.0f, 1.0f);
				pDoc->TempCopySelectedBrushes();
			}
			else if(Tool == ID_TOOLS_BRUSH_SHEARBRUSH)
			{
				pDoc->ScaleNum	=0;
			
				geVec3d_Clear (&pDoc->FinalScale);
				if (ModeTool == ID_TOOLS_TEMPLATE)
				{
					pDoc->TempShearTemplate	=Brush_Clone(pDoc->CurBrush);
				}
				else
				{
					pDoc->TempCopySelectedBrushes();
				}
			}
			else if ((Tool == ID_TOOLS_BRUSH_MOVEROTATEBRUSH) || (Tool == ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES))
			{
				geVec3d_Clear (&pDoc->FinalPos);
				pDoc->TempCopySelectedBrushes();
			}
		}// Not Drag-select 
	}// LButtonDown only
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
void CFusionView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	CFusionDoc* pDoc = GetDocument();
	geBoolean SpaceIsDown = IsKeyDown (VK_SPACE);

	RMouseButtonDown = GE_TRUE;
	if (!LMouseButtonDown)
	{
		int Tool;

		SetCapture();

		HideTheCursor ();

		mStartPoint = point;

		if (mViewIs3d || IsPanning || SpaceIsDown)
			return ;

		Tool = GetTool ();

		if ((Tool == ID_TOOLS_BRUSH_MOVEROTATEBRUSH) || (Tool == ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES))
		{
			geVec3d_Set (&(pDoc->FinalRot), 0.0f, 0.0f, 0.0f);
			pDoc->TempCopySelectedBrushes();
		}

	}
}
#pragma warning (default:4100)

#pragma warning (disable:4100)
void CFusionView::OnRButtonUp(UINT nFlags, CPoint point)
{
	int Tool = GetTool ();

	RMouseButtonDown = GE_FALSE;
	if (!LMouseButtonDown)
	{
		CFusionDoc* pDoc = GetDocument();

		if(this==GetCapture())
		{
			ReleaseCapture();
		}


		if((IsKeyDown (VK_SPACE)) || IsPanning || GetModeTool()==ID_TOOLS_CAMERA)
		{
			pDoc->TempDeleteSelected();
			IsPanning	=FALSE;

			ShowTheCursor ();
			RedrawWindow();
			return;
		}

		ShowTheCursor ();

		if (!mViewIs3d)
		{
			if ((Tool == ID_TOOLS_BRUSH_MOVEROTATEBRUSH) || (Tool == ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES))
			{
				pDoc->UpdateSelected();
				if (GetModeTool () == ID_GENERALSELECT)
				{
					pDoc->DoneRotate ();
				}
			}

			if (SelBrushList_GetSize (pDoc->pSelBrushes) != 0)
				pDoc->UpdateAllViews(UAV_ALL3DVIEWS | REBUILD_QUICK, NULL);
			else
				pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		}
	}

	assert( IsCopying == FALSE ) ;
}
#pragma warning (disable:4100)



void CFusionView::OnDraw(CDC *c)
{
	CFusionDoc* pDoc = GetDocument();

	switch(mViewType)
	{
	case ID_VIEW_TEXTUREVIEW:
	case ID_VIEW_3DWIREFRAME:
		//don't draw before texinfos are valid
		if(Render_GetWadSizes(VCam))
		{
			pDoc->RenderWorld(VCam, c);
		}
		if(pDoc->IsLeakFileLoaded() && pDoc->bShowLeakFinder())
		{
			DrawLeakPoints3D(c->m_hDC, pDoc->GetLeakPoints(), pDoc->GetNumLeakPoints());
		}
		break;
	case ID_VIEW_TOPVIEW:
	case ID_VIEW_SIDEVIEW:
	case ID_VIEW_FRONTVIEW:
		pDoc->RenderOrthoView(VCam, c);
		if(pDoc->IsLeakFileLoaded() && pDoc->bShowLeakFinder())
		{
			DrawLeakPoints(c->m_hDC, pDoc->GetLeakPoints(), pDoc->GetNumLeakPoints());
		}
		break;
	}
	int bkMode=c->GetBkMode();
	int oldColor;
	c->SetBkMode(TRANSPARENT);
	if(this==GetParentFrame()->GetActiveView())
	{
		RECT	r ;

		GetClientRect( &r ) ;	// This seemed to be more accurate, it shouldn't be...
		CBrush RedBrush( RGB(255,0,0) ) ;
		c->FrameRect( &r, &RedBrush );
		oldColor=c->SetTextColor(RGB(255,255,255));
	}
	else
	{
		oldColor=c->SetTextColor(RGB(205,205,205));
	}
	switch( mViewType ) {
	case ID_VIEW_TEXTUREVIEW:
		c->TextOut(4,4,"Textured",8);
		break;
	case ID_VIEW_3DWIREFRAME:
		c->TextOut(4,4,"Wireframe",9);
		break;
	case ID_VIEW_TOPVIEW:
		c->TextOut(4,4,"Top",3);
		break;
	case ID_VIEW_SIDEVIEW:
		c->TextOut(4,4,"Side",4);
		break;
	case ID_VIEW_FRONTVIEW:
		c->TextOut(4,4,"Front",5);
		break;
	}
	c->SetBkMode(bkMode);
	c->SetTextColor(oldColor);

	if( IsDragging && this==GetCapture() )
	{
		// DrawDragRect here just didn't show up against our grid...
		CBrush SelBrush ;
		SelBrush.CreateSolidBrush( RGB(0,255,255) );
		CRect NewRect(mDragStartPoint, mDragCurrentPoint );
		NewRect.NormalizeRect();
		c->FrameRect( &NewRect, &SelBrush ) ;
	}
}

void CFusionView::OnInitialUpdate() 
{
	RECT r;
	CFusionDoc*	pDoc = (CFusionDoc*) GetDocument();
	SizeInfo *WadSizeInfos = Level_GetWadSizeInfos (pDoc->pLevel);
	int iView;

	CView::OnInitialUpdate();
	GetClientRect(&r);

	if(WadSizeInfos)
	{
		Render_SetWadSizes(VCam, WadSizeInfos);
		Render_ResetSettings(VCam, r.right, r.bottom);
	}

	switch(mViewType)
	{
	case ID_VIEW_TEXTUREVIEW:
		Render_SetViewType(VCam, VIEWTEXTURE);
		iView = 0;
		break;
	case ID_VIEW_3DWIREFRAME:
		Render_SetViewType(VCam, VIEWWIRE);
		iView = 0;
		break;
	case ID_VIEW_TOPVIEW:
		Render_SetViewType(VCam, VIEWTOP);
		iView = 1;
		break;
	case ID_VIEW_SIDEVIEW:
		Render_SetViewType(VCam, VIEWSIDE);
		iView = 3;
		break;
	case ID_VIEW_FRONTVIEW:
		Render_SetViewType(VCam, VIEWFRONT);
		iView = 2;
		break;
	default :
		iView = -1;
		break;
	}

	GetParentFrame()->SetWindowText(pDoc->GetTitle());

	if (iView != -1)
	{
		// Update view state information that was saved in level
		ViewStateInfo *pViewStateInfo;

		pViewStateInfo = Level_GetViewStateInfo (pDoc->pLevel, iView);
		if (pViewStateInfo->IsValid)
		{
			Render_SetZoom (VCam, pViewStateInfo->ZoomFactor);
			Render_SetPitchRollYaw (VCam, &pViewStateInfo->PitchRollYaw);
			Render_SetCameraPos (VCam, &pViewStateInfo->CameraPos);
		}
	}

	pDoc->UpdateGridInformation ();
//	pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL );
}

/////////////////////////////////////////////////////////////////////////////
// CFusionView construction/destruction
CFusionView::CFusionView()
{
	VCam	=Render_AllocViewVars();
	Render_SetWadSizes(VCam, NULL);

	mStartPoint.x=mStartPoint.y=0;

	mViewType = ID_VIEW_NEW;

	IsDragging = IsCopying = IsPanning = FALSE;
	LMouseButtonDown = GE_FALSE;
	RMouseButtonDown = GE_FALSE;

	mCY_DRAG = ::GetSystemMetrics( SM_CYDRAG ) ;
	mCX_DRAG = ::GetSystemMetrics( SM_CXDRAG ) ;
}

CFusionView::~CFusionView()
{
	Render_FreeViewVars(VCam);
	if (VCam != NULL)
	{
		geRam_Free (VCam);
	}
}

BOOL CFusionView::PreCreateWindow(CREATESTRUCT& cs)
{
	//get rid of default cursor
	cs.lpszClass = AfxRegisterWndClass( CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
		NULL, (HBRUSH)GetStockObject(GRAY_BRUSH));
	
	return CView::PreCreateWindow(cs);
}

CFusionDoc* CFusionView::GetDocument()
{
	CFusionDoc *pDoc;

	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFusionDoc)));
	pDoc = (CFusionDoc *)m_pDocument;
	ASSERT_VALID (pDoc);

	return pDoc;
}

void CFusionView::OnSize(UINT nType, int cx, int cy) 
{
	CFusionDoc* pDoc = GetDocument();
	SizeInfo *WadSizeInfos = Level_GetWadSizeInfos (pDoc->pLevel);

	// call our oldself
	CView::OnSize(nType, cx, cy);
	
	// make sure that our camera knows our current size
	if(WadSizeInfos)
	{
		Render_SetWadSizes(VCam, WadSizeInfos);
		Render_ResizeView (VCam, cx, cy);
	}
}

void CFusionView::OnToolsCamera() 
{
	CFusionDoc* pDoc = GetDocument();

	SetModeTool(ID_TOOLS_CAMERA);

	pDoc->ConfigureCurrentTool();
	pDoc->mpMainFrame->m_wndTabControls->m_pBrushEntityDialog->Update(pDoc);
}

void CFusionView::OnUpdateToolsCamera(CCmdUI* pCmdUI) 
{
	if( GetModeTool() == ID_TOOLS_CAMERA )
		pCmdUI->SetCheck();
	else
		pCmdUI->SetCheck(0);
}


void CFusionView::OnAxisX() 
{
	CFusionDoc* pDoc = GetDocument();

	pDoc->SetLockAxis( pDoc->GetLockAxis() ^ AXIS_X ) ;
}


void CFusionView::OnUpdateAxisX(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	if( pDoc->GetLockAxis() & AXIS_X )
		pCmdUI->SetCheck();
	else
		pCmdUI->SetCheck(0);
}


void CFusionView::OnAxisY() 
{
	CFusionDoc* pDoc = GetDocument();

	pDoc->SetLockAxis( pDoc->GetLockAxis() ^ AXIS_Y ) ;
}


void CFusionView::OnUpdateAxisY(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();
	
	if( pDoc->GetLockAxis() & AXIS_Y )
		pCmdUI->SetCheck();
	else
		pCmdUI->SetCheck(0);
}


void CFusionView::OnAxisZ() 
{
	CFusionDoc* pDoc = GetDocument();

	pDoc->SetLockAxis( pDoc->GetLockAxis() ^ AXIS_Z ) ;
}


void CFusionView::OnUpdateAxisZ(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();
	
	if( pDoc->GetLockAxis() & AXIS_Z )
		pCmdUI->SetCheck();
	else
		pCmdUI->SetCheck(0);
}


void CFusionView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	CFusionDoc* pDoc = GetDocument();

	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	// set our title
	GetParentFrame()->SetWindowText(pDoc->GetTitle());

	// make sure the bar is updated for our doc.
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

	//	If the application terminates after running a command line
	//	request, we don't want to update this combo box...
	if( pFrame->IsDestroyingApp )
		pFrame->LoadComboBox();
}


void CFusionView::OnToolsBrushMoverotatebrush() 
{
	CFusionDoc* pDoc = GetDocument();

	int mode=GetModeTool();

	if(mode==ID_TOOLS_TEMPLATE)
	{
		SetTool( ID_TOOLS_BRUSH_MOVEROTATEBRUSH );
		pDoc->ConfigureCurrentTool();
	} 
	else 
	{
		if(GetTool()==ID_TOOLS_BRUSH_MOVEROTATEBRUSH)
		{
			SetTool(CURTOOL_NONE);
			SetAdjustMode (ADJUST_MODE_BRUSH);
		} 
		else 
		{
			SetTool(ID_TOOLS_BRUSH_MOVEROTATEBRUSH);
		}
		pDoc->ConfigureCurrentTool();
	}
}

void CFusionView::OnUpdateToolsBrushMoverotatebrush(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	//that's a pretty big if
	if(GetModeTool()==ID_TOOLS_TEMPLATE ||
		(GetModeTool()==ID_GENERALSELECT &&
		GetAdjustMode()==ADJUST_MODE_BRUSH &&
		pDoc->GetSelState()!=NOSELECTIONS)) 
	{
		pCmdUI->Enable();
		if(GetTool()==ID_TOOLS_BRUSH_MOVEROTATEBRUSH)
		{
			pCmdUI->SetCheck();
		}
		else 
		{
			pCmdUI->SetCheck(0);
		}
	} 
	else 
	{
		pCmdUI->Enable(0);
		pCmdUI->SetCheck(0);
	}
}


void CFusionView::OnToolsBrushScalebrush() 
{
	CFusionDoc* pDoc = GetDocument();

	int mode=GetModeTool();

	if(mode==ID_TOOLS_TEMPLATE)
	{
		SetTool(ID_TOOLS_BRUSH_SCALEBRUSH);
		pDoc->ConfigureCurrentTool();
	} 
	else 
	{
		if(GetTool()==ID_TOOLS_BRUSH_SCALEBRUSH)
		{
			SetTool(CURTOOL_NONE);
			SetAdjustMode (ADJUST_MODE_BRUSH);
		} 
		else 
		{
			SetTool(ID_TOOLS_BRUSH_SCALEBRUSH);
		}
		pDoc->ConfigureCurrentTool();
	}
}

void CFusionView::OnUpdateToolsBrushScalebrush(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	//that's a very big if
	if((GetModeTool()==ID_TOOLS_TEMPLATE && !pDoc->TempEnt) ||
		(GetModeTool()==ID_GENERALSELECT &&
		GetAdjustMode ()==ADJUST_MODE_BRUSH &&
#pragma message ("Can't do multiple brush scaling due to Brush_Resize implementation.")
//		SelBrushList_GetSize (pDoc->pSelBrushes) > 0)) 
		SelBrushList_GetSize (pDoc->pSelBrushes) == 1))
	{
		pCmdUI->Enable();
		if(GetTool()==ID_TOOLS_BRUSH_SCALEBRUSH)
		{
			pCmdUI->SetCheck();
		}
		else 
		{
			pCmdUI->SetCheck(0);
		}
	} 
	else 
	{
		pCmdUI->Enable(0);
		pCmdUI->SetCheck(0);
	}
}

void CFusionView::OnToolsBrushShowbrush() 
{

	CFusionDoc* pDoc = GetDocument();

	// toggle brush
	pDoc->mShowBrush ^= 1;

	// redraw the screen
	pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionView::OnUpdateToolsBrushShowbrush(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	pCmdUI->SetCheck (pDoc->mShowBrush);
}


BOOL CFusionView::OnEraseBkgnd(CDC* pDC) 
{
	CRect rect ;
//	CFusionDoc* pDoc = GetDocument();

//	if(!pDoc->mWorldBsp || (mViewType != ID_VIEW_TEXTUREVIEW))
	if ((mViewType == ID_VIEW_TOPVIEW) || (mViewType == ID_VIEW_SIDEVIEW) || (mViewType == ID_VIEW_FRONTVIEW))
	{
		GetClientRect( &rect ) ;
		pDC->FillSolidRect( &rect, Prefs_GetBackgroundColor (((CFusionApp *)AfxGetApp ())->GetPreferences ()));
	}

	return TRUE;
}

// This is the range handler for the types of view that we have
// make sure when we add more view types that we update this.
void CFusionView::OnViewType(UINT nID)
{
	CFusionDoc	*pDoc	=GetDocument();
	mViewType	=nID;
	SizeInfo *WadSizeInfos = Level_GetWadSizeInfos (pDoc->pLevel);

	if(WadSizeInfos)
	{
		geVec3d SaveCameraPos;
		geVec3d SaveOrientation;
		geFloat ZoomFactor;
		int Width, Height;

		ZoomFactor = Render_GetZoom (VCam);
		Render_GetCameraPos (VCam, &SaveCameraPos);
		Render_GetPitchRollYaw (VCam, &SaveOrientation);
		Width = Render_GetWidth (VCam);
		Height = Render_GetHeight (VCam);

		Render_SetWadSizes(VCam, WadSizeInfos);
		Render_ResetSettings(VCam, Render_GetWidth(VCam), Render_GetHeight(VCam));
		Render_ResizeView (VCam, Width, Height);
		Render_SetCameraPos (VCam, &SaveCameraPos);
		Render_SetPitchRollYaw (VCam, &SaveOrientation);
		if (ZoomFactor != 0.0f)
		{
			Render_SetZoom (VCam, ZoomFactor);
		}
	}
	switch(mViewType)
	{
	case ID_VIEW_TEXTUREVIEW:
		Render_SetViewType(VCam, VIEWTEXTURE);
		mViewIs3d = TRUE ;
		break;
	case ID_VIEW_3DWIREFRAME:
		Render_SetViewType(VCam, VIEWWIRE);
		mViewIs3d = TRUE ;
		break;
	case ID_VIEW_TOPVIEW:
		Render_SetViewType(VCam, VIEWTOP);
		mViewIs3d = FALSE ;
		break;
	case ID_VIEW_SIDEVIEW:
		Render_SetViewType(VCam, VIEWSIDE);
		mViewIs3d = FALSE ;
		break;
	case ID_VIEW_FRONTVIEW:
		Render_SetViewType(VCam, VIEWFRONT);
		mViewIs3d = FALSE ;
		break;
	}
	pDoc->UpdateGridInformation ();

	RedrawWindow();
	GetParentFrame()->SetWindowText(pDoc->GetTitle());
}

void CFusionView::OnViewTypeCmdUi(CCmdUI* pCmdUI)
{
	BOOL	bEnable = TRUE ;

	if( !mViewIs3d )	// If this is an otho view, don't allow mutate to rendered
	{
		if( pCmdUI->m_nID == ID_VIEW_TEXTUREVIEW || 
			pCmdUI->m_nID == ID_VIEW_3DWIREFRAME )
			bEnable = FALSE ;
	}
	else				// If this is a rendered view, don't allow mutate to ortho
	{
		if( pCmdUI->m_nID == ID_VIEW_TOPVIEW || 
			pCmdUI->m_nID == ID_VIEW_SIDEVIEW || 
			pCmdUI->m_nID == ID_VIEW_FRONTVIEW )
			bEnable = FALSE ;
	}
	
	if( mViewType == pCmdUI->m_nID )
		pCmdUI->SetCheck();
	else
		pCmdUI->SetCheck(0);

	pCmdUI->Enable( bEnable ) ;
}

void CFusionView::SetTitle()
{
	switch(mViewType)
	{
	case ID_VIEW_NEW:
		GetParentFrame()->SetWindowText("-Creating New View-");
		break;
	case ID_VIEW_3DWIREFRAME:
		GetParentFrame()->SetWindowText("-3D Wireframe-");
		break;
	case ID_VIEW_TEXTUREVIEW:
		GetParentFrame()->SetWindowText("-Texture View-");
		break;
	case ID_VIEW_TOPVIEW:
		GetParentFrame()->SetWindowText("-Top View-");
		break;
	case ID_VIEW_FRONTVIEW:
		GetParentFrame()->SetWindowText("-Front View-");
		break;
	case ID_VIEW_SIDEVIEW:
		GetParentFrame()->SetWindowText("-Side View-");
		break;
	}
}

void CFusionView::OnToolsBrushShearbrush() 
{
	CFusionDoc* pDoc = GetDocument();

	int mode=GetModeTool();

	if(mode==ID_TOOLS_TEMPLATE)
	{
		SetTool( ID_TOOLS_BRUSH_SHEARBRUSH);
		pDoc->ConfigureCurrentTool();
	} 
	else 
	{
		if(GetTool()==ID_TOOLS_BRUSH_SHEARBRUSH)
		{
			SetTool(CURTOOL_NONE);
			SetAdjustMode (ADJUST_MODE_BRUSH);
		}
		else 
		{
			SetTool(ID_TOOLS_BRUSH_SHEARBRUSH);
		}
		pDoc->ConfigureCurrentTool();
	}
}


void CFusionView::OnUpdateToolsBrushShearbrush(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	//that's a very big if
	if((GetModeTool()==ID_TOOLS_TEMPLATE && !pDoc->TempEnt) ||
		(GetModeTool()==ID_GENERALSELECT &&
		GetAdjustMode()==ADJUST_MODE_BRUSH &&
#pragma message ("Can't do multiple brush shear due to Brush_Shear implementation.")
		SelBrushList_GetSize (pDoc->pSelBrushes) == 1)) 
//		SelBrushList_GetSize (pDoc->pSelBrushes) > 0)) 
	{
		pCmdUI->Enable();
		if(GetTool()==ID_TOOLS_BRUSH_SHEARBRUSH)
		{
			pCmdUI->SetCheck();
		}
		else 
		{
			pCmdUI->SetCheck(0);
		}
	} 
	else 
	{
		pCmdUI->Enable(0);
		pCmdUI->SetCheck(0);
	}
}

int CFusionView::GetTool(void)
{
	CFusionDoc* pDoc = GetDocument();

	return pDoc->mCurrentTool;
}

fdocAdjustEnum CFusionView::GetAdjustMode(void)
{
	CFusionDoc* pDoc = GetDocument();
	
	return pDoc->mAdjustMode;
}

int CFusionView::GetModeTool(void)
{
	CFusionDoc* pDoc = GetDocument();
	
	return pDoc->mModeTool;
}

void CFusionView::SetTool(int Tool)
{
	CFusionDoc* pDoc = GetDocument();
	
	pDoc->mCurrentTool = Tool;
}

void CFusionView::SetAdjustMode(fdocAdjustEnum Mode)
{
	CFusionDoc* pDoc = GetDocument();
	
	pDoc->mAdjustMode = Mode;
}

void CFusionView::SetModeTool(int Tool)
{
	CFusionDoc* pDoc = GetDocument();
	
	pDoc->mModeTool = Tool;
}


void CFusionView::OnUpdateBrushGroupsMakenewgroup(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable();
}

void CFusionView::OnBrushGroupsAddtogroup() 
{
	CFusionDoc* pDoc = GetDocument();
	
	pDoc->AddSelToGroup() ;

	pDoc->mpMainFrame->UpdateActiveDoc() ;
}

void CFusionView::OnUpdateBrushGroupsAddtogroup(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();
	
	pCmdUI->Enable (!(pDoc->GetSelState() == NOSELECTIONS));
}

void CFusionView::OnBrushRemoveselectedfromgroup() 
{
	CFusionDoc* pDoc = GetDocument();
	
	pDoc->RemovesSelFromGroup() ;
	pDoc->mpMainFrame->UpdateActiveDoc() ;

}/* CFusionView::OnBrushRemoveselectedfromgroup */

void CFusionView::OnUpdateBrushRemoveselectedfromgroup(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	if( (pDoc->mCurrentGroup == 0) || (pDoc->GetSelState() == NOSELECTIONS) )
		pCmdUI->Enable( FALSE ) ;
	else
		pCmdUI->Enable( TRUE ) ;
}

void CFusionView::OnToolsBrushMoveselectedbrushes() 
{
	SetTool(ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES);

	CFusionDoc* pDoc = GetDocument();
	
	pDoc->ConfigureCurrentTool();
}

void CFusionView::OnUpdateToolsBrushMoveselectedbrushes(CCmdUI* pCmdUI) 
{
	if(GetModeTool()!=ID_TOOLS_CAMERA) 
	{
		pCmdUI->Enable();
		pCmdUI->SetCheck (GetTool () == ID_TOOLS_BRUSH_MOVESELECTEDBRUSHES);
	} 
	else 
	{
		pCmdUI->Enable(0);
		pCmdUI->SetCheck(0);
	}
}

void CFusionView::OnToolsTemplate() 
{
	CFusionDoc* pDoc = GetDocument();

	pDoc->ResetAllSelectedEntities();
	pDoc->ResetAllSelectedFaces();
	pDoc->ResetAllSelectedBrushes();

	SetModeTool(ID_TOOLS_TEMPLATE);
	if(pDoc->TempEnt) 
	{
		SetTool( ID_TOOLS_BRUSH_MOVEROTATEBRUSH );
	}
	else 
	{
		SetTool(ID_TOOLS_BRUSH_SCALEBRUSH);
	}
	pDoc->SetAdjustmentMode( ADJUST_MODE_BRUSH ) ;
	pDoc->ConfigureCurrentTool();
	pDoc->mpMainFrame->m_wndTabControls->m_pBrushEntityDialog->Update(pDoc);
}

void CFusionView::OnUpdateToolsTemplate(CCmdUI* pCmdUI) 
{
	if( GetModeTool() == ID_TOOLS_TEMPLATE ) pCmdUI->SetCheck();
	else pCmdUI->SetCheck(0);
}

void CFusionView::OnUpdateToolsBrushRotate45(CCmdUI* pCmdUI) 
{
	CFusionDoc* pDoc = GetDocument();

	//that's a pretty big if
	if((GetModeTool()==ID_TOOLS_TEMPLATE && !pDoc->TempEnt) || 
	   (GetModeTool()==ID_GENERALSELECT &&
		GetAdjustMode()==ADJUST_MODE_BRUSH &&
		pDoc->GetSelState()!=NOSELECTIONS))
	{
		pCmdUI->Enable();
	}
	else 
	{
		pCmdUI->Enable(0);
	}
}

void CFusionView::DrawLeakPoints(HDC ViewDC, geVec3d *LeakPoints, int NumLeakPoints)
{
	POINT	pnt = {0,0};
	POINT	nullpnt;
	int		i;
	CPen	PenRed(PS_SOLID, 1, RGB(255,0,0));
	HPEN	oldpen;

	assert(LeakPoints != NULL);
	assert(NumLeakPoints > 0);

	oldpen	=(HPEN)SelectObject(ViewDC, (HPEN)PenRed);

	for(i=0;i < NumLeakPoints-1;i++)
	{
		pnt	=Render_OrthoWorldToView(VCam, &LeakPoints[i]);
		MoveToEx(ViewDC, pnt.x, pnt.y, &nullpnt);

		pnt	=Render_OrthoWorldToView(VCam, &LeakPoints[i+1]);
		LineTo(ViewDC, pnt.x, pnt.y);
	}
	LineTo(ViewDC, pnt.x, pnt.y);

	SelectObject(ViewDC, oldpen);
}

void CFusionView::DrawLeakPoints3D(HDC ViewDC, geVec3d *LeakPoints, int NumLeakPoints)
{
	POINT	nullpnt;
	int		i;
	CPen	PenRed(PS_SOLID, 1, RGB(255,0,0));
	HPEN	oldpen;
	geVec3d	pnt3;

	assert(LeakPoints != NULL);
	assert(NumLeakPoints > 0);

	oldpen	=(HPEN)SelectObject(ViewDC, (HPEN)PenRed);
	geVec3d_Clear (&pnt3);

	for(i=0;i < NumLeakPoints-1;i++)
	{
		pnt3	=Render_XFormVert(VCam, &LeakPoints[i]);

		if(pnt3.Z < 0.0f)
			continue;

		MoveToEx(ViewDC, Units_Round(pnt3.X), Units_Round(pnt3.Y), &nullpnt);

		pnt3	=Render_XFormVert(VCam, &LeakPoints[i+1]);
		LineTo(ViewDC, Units_Round(pnt3.X), Units_Round(pnt3.Y));
	}
	if(pnt3.Z > 1.0f)
		LineTo(ViewDC, Units_Round(pnt3.X), Units_Round(pnt3.Y));

	SelectObject(ViewDC, oldpen);
}


void CFusionView::OnDeselectall() 
{
	CFusionDoc* pDoc = GetDocument();

	pDoc->ResetAllSelections() ;
	pDoc->UpdateSelected();
	pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL ) ;
}

void CFusionView::OnUpdateDeselectall(CCmdUI* pCmdUI) 
{
	BOOL		bEnable ;
	CFusionDoc* pDoc = GetDocument();
	
	bEnable = ( pDoc->GetSelState() == NOSELECTIONS ) ? FALSE : TRUE ;
	pCmdUI->Enable( bEnable ) ;
}

void CFusionView::OnSelectall() 
{
	CFusionDoc* pDoc = GetDocument();
	
	pDoc->SelectAll () ;
	pDoc->UpdateAllViews( UAV_ALL3DVIEWS, NULL ) ;
}

void CFusionView::OnUpdateSelectall(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( TRUE ) ;
}


LRESULT CFusionView::OnCompileMessage (WPARAM wParam, LPARAM lParam)
{
	if (wParam == COMPILER_PROCESSID)
	{
		char *msg;

		msg = (char *)lParam;
		ConPrintf ("%s", msg);
		geRam_Free (msg);
	}
	return 0;
}

LRESULT CFusionView::OnCompileError (WPARAM wParam, LPARAM lParam)
{
	if (wParam == COMPILER_PROCESSID)
	{
		char *msg;

		msg = (char *)lParam;
		ConError ("%s", msg);
		geRam_Free (msg);
	}
	return 0;
}

LRESULT CFusionView::OnCompileDone (WPARAM wParam, LPARAM lParam)
{
	if (wParam == COMPILER_PROCESSID)
	{
		CFusionDoc *pDoc;

		pDoc = GetDocument ();

		if (pDoc != NULL)
		{
			pDoc->CompileDone ((CompilerErrorEnum)lParam);
		}
	}
	return 0;
}

void CFusionView::OnToolsScaleworld() 
{
	CString	szKey = "Enter world scale factor";
	CString szVal;
	int		ModalResult;
	CDialog	*pEditDialog;
	float scf;
	CFusionDoc* pDoc = GetDocument();
	
	pEditDialog = new CFloatKeyEditDlg(this, szKey, &szVal);
	if (pEditDialog != NULL)
	{
		ModalResult = pEditDialog->DoModal();
		delete pEditDialog;
		if(ModalResult == IDOK)
		{
			sscanf((LPCSTR)szVal, "%f", &scf);
			pDoc->ScaleWorld(scf);			
		}
	}	
}

void CFusionView::OnToolsBrushMakenewest() 
{
	CFusionDoc *pDoc = GetDocument ();

	pDoc->MakeSelectedBrushNewest();
}

void CFusionView::OnToolsSettexturescale() 
{
	CFusionDoc* pDoc = GetDocument();
	float	scf;
	CString	szKey, szVal;
	int		ModalResult;
	CDialog	*pEditDialog;

	szKey = "Enter texture scale for selected brushes";
	szVal = "1.0";
	pEditDialog = new CFloatKeyEditDlg(this, szKey, &szVal);
	if (pEditDialog != NULL)
	{
		ModalResult = pEditDialog->DoModal();
		delete pEditDialog;
		if(ModalResult == IDOK)
		{
			sscanf((LPCSTR)szVal, "%f", &scf);
			pDoc->SetAllFacesTextureScale(scf);
		}
	}
}

void CFusionView::OnToolsNextbrush() 
{
	CFusionDoc *pDoc = GetDocument ();
	BrushList *BList = Level_GetBrushes (pDoc->pLevel);

	if(GetModeTool()==ID_GENERALSELECT && !pDoc->IsSelectionLocked())
	{
		switch (pDoc->mAdjustMode)
		{
			case ADJUST_MODE_FACE :
			{
				int nSelectedFaces = SelFaceList_GetSize (pDoc->pSelFaces);
				Face *pFace;

				if (nSelectedFaces == 0)
				{
					BrushIterator bi;

					pDoc->CurBrush = BrushList_GetFirst (BList, &bi);
					pFace = Brush_SelectFirstFace (pDoc->CurBrush);
					SelBrushList_Add (pDoc->pSelBrushes, pDoc->CurBrush);
				}
				else
				{
					Brush *pBrush;

					// get first selected face
					pFace = SelFaceList_GetFace (pDoc->pSelFaces, nSelectedFaces-1);
					// Remove all face selections
					pDoc->ResetAllSelectedFaces ();

					Face_SetSelected (pFace, GE_TRUE);
					pBrush = BrushList_FindTopLevelFaceParent (Level_GetBrushes (pDoc->pLevel), pFace);

					// select next face
					if(!Brush_SetNextSelectedFace(pBrush))
					{
						pFace = Brush_SelectFirstFace(pBrush);
					}
					else
					{
						pFace = Brush_GetSelectedFace (pBrush);
					}
				}
				SelFaceList_Add (pDoc->pSelFaces, pFace);
				pDoc->UpdateSelected ();
									
				pDoc->UpdateFaceAttributesDlg ();
				pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
				break;
			}

			case ADJUST_MODE_BRUSH :
				if(pDoc->GetSelState()&ONEBRUSH)
				{
					SelBrushList_RemoveAll (pDoc->pSelBrushes);
					SelBrushList_Add (pDoc->pSelBrushes, Brush_GetNextBrush(pDoc->CurBrush, BList));
					pDoc->UpdateSelected();
				
					//update the brush attributes dialog...
					pDoc->UpdateBrushAttributesDlg ();
					pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
				}
				else if(!(pDoc->GetSelState() & ANYBRUSH))
				{
					Brush *pBrush;
					BrushIterator bi;

					pBrush = BrushList_GetFirst (BList, &bi);
					if(pBrush != NULL)
					{
						SelBrushList_Add (pDoc->pSelBrushes, pBrush);
						pDoc->UpdateSelected();
						pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
					}
				}
				break;

			default :
				assert (0);  // bad adjust mode
				break;
		}
	}
}

void CFusionView::OnToolsPrevbrush() 
{
	CFusionDoc *pDoc = GetDocument ();
	BrushList *BList = Level_GetBrushes (pDoc->pLevel);

	if(GetModeTool()==ID_GENERALSELECT && !pDoc->IsSelectionLocked())
	{
		switch (pDoc->mAdjustMode)
		{
			case ADJUST_MODE_FACE :
			{
				int nSelectedFaces = SelFaceList_GetSize (pDoc->pSelFaces);
				Face *pFace;

				if (nSelectedFaces == 0)
				{
					BrushIterator bi;

					pDoc->CurBrush = BrushList_GetFirst (BList, &bi);
					pFace = Brush_SelectFirstFace (pDoc->CurBrush);
					SelBrushList_Add (pDoc->pSelBrushes, pDoc->CurBrush);
				}
				else
				{
					Brush *pBrush;

					// get the last selected face
					pFace = SelFaceList_GetFace (pDoc->pSelFaces, 0);

					// Remove all face selections
					pDoc->ResetAllSelectedFaces ();

					// Select the next face in order, using selected brush list...
					pBrush = BrushList_FindTopLevelFaceParent (Level_GetBrushes (pDoc->pLevel), pFace);
					Face_SetSelected (pFace, GE_TRUE);

					// select next face
					if(!Brush_SetPrevSelectedFace(pBrush))
					{
						pFace = Brush_SelectLastFace(pBrush);
					}
					else
					{
						pFace = Brush_GetSelectedFace (pBrush);
					}
				}
				SelFaceList_Add (pDoc->pSelFaces, pFace);
				pDoc->UpdateSelected ();
									
				pDoc->UpdateFaceAttributesDlg ();
				pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
				break;
			}

			case ADJUST_MODE_BRUSH :
				if(pDoc->GetSelState()&ONEBRUSH)
				{
					SelBrushList_RemoveAll (pDoc->pSelBrushes);
					SelBrushList_Add (pDoc->pSelBrushes, Brush_GetPrevBrush(pDoc->CurBrush, BList));
					pDoc->UpdateSelected();
				
					//update the brush attributes dialog...
					pDoc->UpdateBrushAttributesDlg ();
					pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
				}
				else if(!(pDoc->GetSelState() & ANYBRUSH))
				{
					Brush *pBrush;
					BrushIterator bi;

					pBrush = BrushList_GetLast(BList, &bi);
					if (pBrush != NULL)
					{
						SelBrushList_Add (pDoc->pSelBrushes, pBrush);
						pDoc->UpdateSelected();
						pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
					}
				}
				break;

			default :
				assert (0);		// bad adjust mode
				break;
		}
	}
}

void CFusionView::OnToolsAddtolevel() 
{
	CFusionDoc *pDoc = GetDocument ();

	if(GetModeTool()==ID_TOOLS_TEMPLATE)
		pDoc->AddBrushToWorld();
}

void CFusionView::OnUpdateToolsAddtolevel(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CFusionView::DoZoom (float ZoomInc)
{
	if (!mViewIs3d)
	{
		CFusionDoc *pDoc = GetDocument ();

		Render_ZoomChange( VCam, ZoomInc);
		pDoc->UpdateGridInformation ();
		RedrawWindow();
	}
}

void CFusionView::OnViewZoomin() 
{
	DoZoom (0.1f);
}

void CFusionView::OnUpdateViewZoomin(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CFusionView::OnViewZoomout() 
{
	DoZoom (-0.1f);
}

void CFusionView::OnUpdateViewZoomout(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	
}

void CFusionView::HideTheCursor (void)
{
	while (ShowCursor (FALSE) >= 0)
	{
		;
	}
}

void CFusionView::ShowTheCursor (void)
{
	while (ShowCursor (TRUE) < 0)
	{
		;
	}
}
