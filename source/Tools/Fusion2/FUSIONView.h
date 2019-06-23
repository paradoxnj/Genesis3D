/****************************************************************************************/
/*  FusionView.h                                                                        */
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
#ifndef FUSIONVIEW_H
#define FUSIONVIEW_H

#include "brush.h"
#include "render.h"
#include "resource.h"
#include "fusiondoc.h"

/* SELECTION MODES, part in VIEW, part in DOC
	mCurrentTool GetTool()			mAdjustMode GetAdjustMode()		mModeTool GetModeTool()
	ID_TOOLS_BRUSH_SCALEBRUSH		ID_TOOLS_BRUSH_ADJUSTMENTMODE		ID_TOOLS_TEMPLATE
	ID_TOOLS_BRUSH_MOVEROTATEBRUSH	ID_TOOLS_FACE_ADJUSTMENTMODE		ID_TOOLS_CAMERA
	ID_TOOLS_BRUSH_SHEARBRUSH											ID_GENERALSELECT
	CURTOOL_NONE
*/

typedef struct tag_EntityViewList EntityViewList;

class CFusionView : public CView
{
protected: // create from serialization only
	DECLARE_DYNCREATE(CFusionView)

// Attributes
public:
	UINT		mViewType;
	int			sides;
	ViewVars	*VCam;

	CFusionView();
	void SetTool(int Tool);
	void SetAdjustMode(fdocAdjustEnum Mode);
	void SetModeTool(int Tool);
	void SetTitle();
	int GetTool(void);
	fdocAdjustEnum GetAdjustMode(void);
	int GetModeTool(void);
	CFusionDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFusionView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFusionView();

protected:

// Generated message map functions
protected:
	LRESULT OnCompileMessage (WPARAM wParam, LPARAM lParam);
	LRESULT OnCompileError (WPARAM wParam, LPARAM lParam);
	LRESULT OnCompileDone (WPARAM wParam, LPARAM lParam);
	//{{AFX_MSG(CFusionView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnToolsCamera();
	afx_msg void OnUpdateToolsCamera(CCmdUI* pCmdUI);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnAxisX();
	afx_msg void OnUpdateAxisX(CCmdUI* pCmdUI);
	afx_msg void OnAxisY();
	afx_msg void OnUpdateAxisY(CCmdUI* pCmdUI);
	afx_msg void OnAxisZ();
	afx_msg void OnUpdateAxisZ(CCmdUI* pCmdUI);
	afx_msg void OnToolsBrushMoverotatebrush();
	afx_msg void OnUpdateToolsBrushMoverotatebrush(CCmdUI* pCmdUI);
	afx_msg void OnToolsBrushScalebrush();
	afx_msg void OnUpdateToolsBrushScalebrush(CCmdUI* pCmdUI);
	afx_msg void OnToolsBrushShowbrush();
	afx_msg void OnUpdateToolsBrushShowbrush(CCmdUI* pCmdUI);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnToolsBrushShearbrush();
	afx_msg void OnUpdateToolsBrushShearbrush(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBrushGroupsMakenewgroup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBrushGroupsAddtogroup(CCmdUI* pCmdUI);
	afx_msg void OnToolsBrushRotate45();
	afx_msg void OnToolsBrushMoveselectedbrushes();
	afx_msg void OnUpdateToolsBrushMoveselectedbrushes(CCmdUI* pCmdUI);
	afx_msg void OnToolsTemplate();
	afx_msg void OnUpdateToolsTemplate(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsBrushRotate45(CCmdUI* pCmdUI);
	afx_msg void OnBrushRemoveselectedfromgroup();
	afx_msg void OnUpdateBrushRemoveselectedfromgroup(CCmdUI* pCmdUI);
	afx_msg void OnBrushGroupsAddtogroup();
	afx_msg void OnDeselectall();
	afx_msg void OnUpdateDeselectall(CCmdUI* pCmdUI);
	afx_msg void OnSelectall();
	afx_msg void OnUpdateSelectall(CCmdUI* pCmdUI);
	afx_msg void OnToolsScaleworld();
	afx_msg void OnToolsBrushMakenewest();
	afx_msg void OnToolsSettexturescale();
	afx_msg void OnToolsNextbrush();
	afx_msg void OnToolsPrevbrush();
	afx_msg void OnToolsAddtolevel();
	afx_msg void OnUpdateToolsAddtolevel(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomin();
	afx_msg void OnUpdateViewZoomin(CCmdUI* pCmdUI);
	afx_msg void OnViewZoomout();
	afx_msg void OnUpdateViewZoomout(CCmdUI* pCmdUI);
	afx_msg void OnCenterthing();
	afx_msg void OnUpdateCenterthing(CCmdUI* pCmdUI);
	//}}AFX_MSG
public:
	afx_msg void OnViewType(UINT nID);
	BOOL IsPanning;
	void DoZoom (float ZoomInc);
protected:
	afx_msg void OnViewTypeCmdUi(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()
private:
	geBoolean	mViewIs3d ;
	BOOL		IsCopying;
	BOOL		IsDragging ;
	double		mZoomFactor;
	int			mShowBrush;
	int			mQuadrant;	// Current Quadrant of brush that mouse is in...
	CPoint		mStartPoint;
	CPoint		mDragStartPoint ;
	CPoint		mDragCurrentPoint ;

	// These flags are updated by the OnLButtonDown, OnLButtonUp,
	// OnRButtonDown, and OnRButtonUp messages handlers, and are
	// checked in those handlers to prevent problems associated with
	// other ways of checking mouse key position (either by checking the nFlags
	// parameter to the handlers, or using GetAsyncKeyState).
	// NOTE:  These variables track the state of mouse button message processing,
	// they do not reflect the current state of the mouse buttons.
	geBoolean LMouseButtonDown;
	geBoolean RMouseButtonDown;

	int mTextureViewIndex;
	int m3DWireframeViewIndex;
	int mSolidGonViewIndex;
	int mFrontViewIndex;
	int mSideViewIndex;
	int mTopViewIndex;

	static int mCY_DRAG ;
	static int mCX_DRAG ;

	void DrawLeakPoints(HDC ViewDC, geVec3d *LeakPoints, int NumLeakPoints);
	void DrawLeakPoints3D(HDC ViewDC, geVec3d *LeakPoints, int NumLeakPoints);
	void LockAxisView (int *dx, int *dy);
	void LockAxis( geVec3d * pWP ) ;
	void SetEditCursor (int Tool, const POINT *pMousePos);
	int GetCursorBoxPos (const POINT *pMousePos);
	void Pan (CFusionDoc *pDoc, int dx, int dy, const geVec3d *dv, BOOL LButtonIsDown, BOOL RButtonIsDown);
	void ScaleSelected (CFusionDoc *pDoc, int dx, int dy);
	void ShearSelected (CFusionDoc *pDoc, int dx, int dy);
	void HideTheCursor (void);
	void ShowTheCursor (void);
	void DoneMovingBrushes ();
	EntityViewList *pEntityView;
};

/////////////////////////////////////////////////////////////////////////////

#endif
