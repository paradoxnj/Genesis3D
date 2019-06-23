/****************************************************************************************/
/*  FusionDoc.cpp                                                                       */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Moore, Bruce Cooner          */
/*  Description:  A very large file that does too much to describe                      */
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
#include "FUSIONDoc.h"
#include <float.h>

#include "CreateArchDialog.h"
#include "CreateBoxDialog.h"
#include "CreateConeDialog.h"
#include "CreateCylDialog.h"
#include "CreateSpheroidDialog.h"
#include "CreateStaircaseDialog.h"
#include "CompileDialog.h"
#include "BrushGroupDialog.h"
#include "gridsizedialog.h"
#include "Entity.h"
#include "entitiesdialog.h"
#include "FaceAttributesDialog.h"
#include "BrushAttributesDialog.h"
#include "KeyEditDlg.h"
#include "BrushEntityDialog.h"
#include "FusionTabControls.h"
#include "EntityVisDlg.h"
#include "LevelOptions.h"

#include "FUSIONView.h"
#include "wadfile.h"
#include "XForm3d.h"
#include "gbsplib.h"
#include "render.h"
#include "EntityTable.h"
#include "brush.h"
#include "node.h"
#include "facelist.h"
#include "ModelDialog.h"

#include <afxole.h>
#pragma warning(disable : 4201 4214 4115)
#include <mmsystem.h>
#pragma warning(default : 4201 4214 4115)
#include <errno.h>
#include <direct.h>
#include <assert.h>
#include "basetype.h"
#include "Vec3d.h"
#include "brush.h"
#include "typeio.h"
#include "units.h"
#include "FilePath.h"
#include <io.h>		// for _access
#include "ram.h"
#include "util.h"
#include "BrushTemplate.h"
#include "TextureDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Must be within this distance (in pixels) to select anything.
// (Yes, 10,000 is a big number.  Right now we want to select the closest thing,
// no matter how far away it is...)
#define MAX_PIXEL_SELECT_DIST (10000)
#define MIN_ENTITY_SELECT_DIST (8.0f)

// Maximum distance from entity in order for it to be selected.
// This is in world space coordinates and is used in rendered view only.
#define MAX_ENTITY_SELECT_DIST (16.0f)

IMPLEMENT_SERIAL(CFusionDoc, CDocument, 0);
BEGIN_MESSAGE_MAP(CFusionDoc, CDocument)
	//{{AFX_MSG_MAP(CFusionDoc)
	ON_COMMAND(ID_TOOLS_USEGRID, OnToolsUsegrid)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_USEGRID, OnUpdateToolsUsegrid)
	ON_COMMAND(ID_TOOLS_GRIDSETTINGS, OnToolsGridsettings)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_ENTITIES_EDITOR, OnEntitiesEditor)
	ON_COMMAND(ID_ENTITIES_SHOW, OnEntitiesShow)
	ON_UPDATE_COMMAND_UI(ID_ENTITIES_SHOW, OnUpdateEntitiesShow)
	ON_COMMAND(ID_VIEW_SHOW_ALLGROUPS, OnViewShowAllGroups)
	ON_COMMAND(ID_VIEW_SHOW_CURRENTGROUP, OnViewShowCurrentGroup)
	ON_COMMAND(ID_VIEW_SHOW_VISIBLEGROUPS, OnViewShowVisibleGroups)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_VISIBLEGROUPS, OnUpdateViewShowVisibleGroups)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_ALLGROUPS, OnUpdateViewShowAllGroups)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_CURRENTGROUP, OnUpdateViewShowCurrentGroup)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_ADJUSTMENTMODE, OnUpdateBrushAdjustmentmode)
	ON_COMMAND(ID_BRUSH_SELECTED_COPYTOCURRENT, OnBrushSelectedCopytocurrent)
	ON_COMMAND(ID_BRUSH_SELECTED_DELETE, OnBrushSelectedDelete)
	ON_COMMAND(ID_GBSPNOWATER, OnGbspnowater)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_FACE_ADJUSTMENTMODE, OnUpdateFaceAdjustmentmode)
	ON_COMMAND(ID_CONSTRAINHOLLOWS, OnConstrainhollows)
	ON_UPDATE_COMMAND_UI(ID_CONSTRAINHOLLOWS, OnUpdateConstrainhollows)
	ON_COMMAND(ID_GENERALSELECT, OnGeneralselect)
	ON_UPDATE_COMMAND_UI(ID_GENERALSELECT, OnUpdateGeneralselect)
	ON_COMMAND(ID_THING_ATTRIBUTES, OnThingAttributes)
	ON_COMMAND(ID_BRUSH_SUBTRACTFROMWORLD, OnBrushSubtractfromworld)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_COMMAND(ID_COMPILE, OnCompile)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_SUBTRACTFROMWORLD, OnUpdateBrushSubtractfromworld)
	ON_UPDATE_COMMAND_UI(ID_THING_ATTRIBUTES, OnUpdateThingAttributes)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_SHOWASSOCIATEDENTITY, OnUpdateToolsBrushShowassociatedentity)
	ON_UPDATE_COMMAND_UI(ID_ENTITIES_EDITOR, OnUpdateEntitiesEditor)
	ON_COMMAND(ID_NEW_LIB_OBJECT, OnNewLibObject)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_TOOLS_BRUSH_ADJUSTMENTMODE, OnToolsBrushAdjustmentmode)
	ON_COMMAND(ID_TOOLS_FACE_ADJUSTMENTMODE, OnToolsFaceAdjustmentmode)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_PRIMITIVES_CUBE, OnUpdateBrushPrimitives)
	ON_COMMAND(ID_BRUSH_PRIMITIVES_CUBE, OnBrushPrimitivesCube)
	ON_COMMAND(ID_BRUSH_PRIMITIVES_SPHEROID, OnBrushPrimitivesSpheroid)
	ON_COMMAND(ID_BRUSH_PRIMITIVES_CYLINDER, OnBrushPrimitivesCylinder)
	ON_COMMAND(ID_BRUSH_PRIMITIVES_STAIRCASE, OnBrushPrimitivesStaircase)
	ON_COMMAND(ID_VIEW_SHOW_CLIP, OnViewShowClip)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_CLIP, OnUpdateViewShowClip)
	ON_COMMAND(ID_VIEW_SHOW_DETAIL, OnViewShowDetail)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_DETAIL, OnUpdateViewShowDetail)
	ON_COMMAND(ID_VIEW_SHOW_HINT, OnViewShowHint)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOW_HINT, OnUpdateViewShowHint)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_BRUSH_PRIMITIVES_ARCH, OnBrushPrimitivesArch)
	ON_COMMAND(ID_BRUSH_PRIMITIVES_CONE, OnBrushPrimitivesCone)
	ON_COMMAND(ID_FILE_IMPORT, OnFileImport)
	ON_COMMAND(ID_TOOLS_BRUSH_ATTRIBUTES, OnToolsBrushAttributes)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_BRUSH_ATTRIBUTES, OnUpdateToolsBrushAttributes)
	ON_COMMAND(ID_TOOLS_FACE_ATTRIBUTES, OnToolsFaceAttributes)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_FACE_ATTRIBUTES, OnUpdateToolsFaceAttributes)
	ON_COMMAND(ID_ENTITYVISIBILITY, OnEntityVisibility)
	ON_COMMAND(IDM_REBUILD_BSP, OnRebuildBsp)
	ON_UPDATE_COMMAND_UI(IDM_REBUILD_BSP, OnUpdateRebuildBsp)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_TOOLS_TOGGLEADJUSTMODE, OnToolsToggleadjustmode)
	ON_UPDATE_COMMAND_UI(ID_TOOLS_TOGGLEADJUSTMODE, OnUpdateToolsToggleadjustmode)
	ON_COMMAND(IDM_LEVELOPTIONS, OnLeveloptions)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_PRIMITIVES_SPHEROID, OnUpdateBrushPrimitives)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_PRIMITIVES_CYLINDER, OnUpdateBrushPrimitives)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_PRIMITIVES_STAIRCASE, OnUpdateBrushPrimitives)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_PRIMITIVES_ARCH, OnUpdateBrushPrimitives)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_PRIMITIVES_CONE, OnUpdateBrushPrimitives)
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI_RANGE(  ID_BRUSH_SELECTED_DELETE, ID_BRUSH_SELECTED_COPYTOCURRENT, OnSelectedTypeCmdUI)
END_MESSAGE_MAP()



#pragma warning (disable:4100)
void CFusionDoc::Serialize(CArchive& ar)
{
	_ASSERTE(0);
}
#pragma warning (default:4100)


WadFileEntry* CFusionDoc::GetDibBitmap(const char *Name)
{
	return Level_GetWadBitmap (pLevel, Name);
}

const char *CFusionDoc::FindTextureLibrary (char const *WadName)
{
	static char WorkPath[MAX_PATH];
	const Prefs  *pPrefs;

	::FilePath_AppendName (LastPath, WadName, WorkPath);
	if (_access (WorkPath, 0) == 0)
	{
		return WorkPath;
	}

	pPrefs = GetPrefs ();
	if (FilePath_SearchForFile (WadName, Prefs_GetTxlSearchPath (pPrefs), WorkPath))
	{
		return WorkPath;
	}
	return NULL;
}

CFusionDoc::CFusionDoc() : CDocument (), 
	SelectLock (FALSE), TempEnt (FALSE), SelState (NOSELECTIONS),
	mShowSelectedFaces (FALSE), mShowSelectedBrushes (FALSE),
	LeakPoints (NULL), NumLeakPoints (0), bLeakLoaded (FALSE), bShowLeak (TRUE),
	IsNewDocument (1), mShowEntities (GE_TRUE), mCurTextureSelection (1),
	bShowClipBrushes (GE_TRUE), bShowDetailBrushes (GE_TRUE), bShowHintBrushes (GE_TRUE),
	mpActiveViewFrame (NULL), mpBrushAttributes (NULL), mpFaceAttributes (NULL),
	/*mpTextureView (NULL), */mWorldBsp (NULL), mActiveView (-1), mCurrentEntity (-1),
	mModeTool (ID_TOOLS_TEMPLATE), mAdjustMode (ADJUST_MODE_BRUSH),
	mCurrentTool (ID_TOOLS_BRUSH_SCALEBRUSH), mShowBrush (TRUE), mConstrainHollows (GE_TRUE),
	mCurrentBitmap (0), NumSelEntities (0), //mTextureBrowserOpen (0), 
	mCurrentGroup (0), TempShearTemplate (NULL), PlaceObjectFlag (FALSE),
	pSelFaces (NULL), pSelBrushes (NULL), pTempSelBrushes (NULL)
{
	const char *DefaultWadName;
	const Prefs  *pPrefs = GetPrefs ();


	DefaultWadName = Prefs_GetTxlName (pPrefs);

	strcpy (LastPath, Prefs_GetProjectDir (pPrefs));

	const char *WadPath = FindTextureLibrary (DefaultWadName);

	if (WadPath == NULL)
	{
		AfxMessageBox ("Can't find texture library");
	}

	pLevel = Level_Create (WadPath, Prefs_GetHeadersList (pPrefs));
	if (!Level_LoadWad (pLevel))
	{
		CString Msg;

		AfxFormatString1 (Msg, IDS_CANTLOADTXL, WadPath);
		AfxMessageBox (Msg);
	}

	mpMainFrame=(CMainFrame*)AfxGetMainWnd();

	pSelBrushes = SelBrushList_Create ();
	pTempSelBrushes = SelBrushList_Create ();
	pSelFaces = SelFaceList_Create ();

	SetLockAxis( 0 ) ;	// Start with no axis locked
	
	{
		// create our default box
		BrushTemplate_Box *pBoxTemplate;
		pBoxTemplate = Level_GetBoxTemplate (pLevel);

		BTemplate = BrushTemplate_CreateBox (pBoxTemplate);
	}

	Brush_Bound(BTemplate);
	CurBrush	=BTemplate;

	geVec3d_Clear(&SelectedGeoCenter);

	AddCameraEntityToLevel ();

	pUndoStack = UndoStack_Create ();
}/* CFusionDoc::CFusionDoc */

void CFusionDoc::AddCameraEntityToLevel (void)
{
	// Make default camera entity
	CEntity CameraEntity ;
	CString cstr;

	CreateEntityFromName( "Camera", CameraEntity ) ;
	cstr.LoadString( IDS_CAMERAENTITYNAME ) ;
	CameraEntity.SetKeyValue ("%name%", cstr );
	CameraEntity.SetOrigin ( 0.0f, 0.0f, 0.0f, Level_GetEntityDefs (pLevel) );
	Level_AddEntity (pLevel, CameraEntity);
}

CFusionDoc::~CFusionDoc()
{
	if (mWorldBsp != NULL)		Node_ClearBsp(mWorldBsp);
	if (pLevel != NULL)			Level_Destroy (&pLevel);
	if (BTemplate != NULL)		Brush_Destroy (&BTemplate);
	if (pUndoStack != NULL)		UndoStack_Destroy (&pUndoStack);
	if (pSelBrushes != NULL)	SelBrushList_Destroy (&pSelBrushes);
	if (pTempSelBrushes != NULL)SelBrushList_Destroy (&pTempSelBrushes);
	if (LeakPoints != NULL)		geRam_Free(LeakPoints);
	if (pSelFaces != NULL)		SelFaceList_Destroy (&pSelFaces);

	OpenClipboard(mpMainFrame->GetSafeHwnd());
	EmptyClipboard();
	CloseClipboard();
}

void CFusionDoc::DeleteFaceAttributes (void)
{
	if (mpFaceAttributes != NULL)
	{
		delete mpFaceAttributes;
		mpFaceAttributes = NULL;
	}
}

void CFusionDoc::DeleteBrushAttributes (void)
{
	if (mpBrushAttributes != NULL)
	{
		delete mpBrushAttributes;
		mpBrushAttributes = NULL;
	}
}

static void fdocDrawEntity
	(
	  CEntity const *	pEnt,
	  ViewVars  const *	v,
	  CDC			*	pDC,
	  EntityTable const *pEntityDefs,
	  BOOL				bShowUI
	)
{
#define ENTITY_SIZE (8.0f)  // 16" across
	geVec3d VecOrigin;
	geVec3d EntSizeWorld;	// entity size in world space

	POINT EntPosView;
	POINT EntSizeView;
	POINT EntWidthHeight ;
	POINT OriginView;

	POINT TopLeft, BottomRight;
	POINT TopRight, BottomLeft;

	static const float COS45	= (float)cos (M_PI/4.0f);
	static const float SIN45	= (float)sin (M_PI/4.0f);
	static const float MCOS45	= (float)cos (-(M_PI/4.0f));
	static const float MSIN45	= (float)sin (-(M_PI/4.0f));

	// compute entity size in view coordinates
	geVec3d_Set (&EntSizeWorld, ENTITY_SIZE, ENTITY_SIZE, ENTITY_SIZE);
	EntSizeView = Render_OrthoWorldToView ( v, &EntSizeWorld);
	geVec3d_Clear (&VecOrigin);
	OriginView = Render_OrthoWorldToView ( v, &VecOrigin);
	// This one is the width and height of the Entity
	EntWidthHeight.x = max( OriginView.x, EntSizeView.x ) - min( OriginView.x, EntSizeView.x ) ;
	EntWidthHeight.y = max( OriginView.y, EntSizeView.y ) - min( OriginView.y, EntSizeView.y ) ;
	
	// This can have negative numbers
	EntSizeView.x -= OriginView.x;
	EntSizeView.y -= OriginView.y;

	// entity's position in the view
	EntPosView = Render_OrthoWorldToView ( v, &(pEnt->mOrigin));

	{
		// Draw an X at the entity's position...
		TopLeft.x = EntPosView.x - EntSizeView.x;
		TopLeft.y = EntPosView.y - EntSizeView.y;
		BottomRight.x = EntPosView.x + EntSizeView.x;
		BottomRight.y = EntPosView.y + EntSizeView.y;
		TopRight.x = BottomRight.x;
		TopRight.y = TopLeft.y;
		BottomLeft.x = TopLeft.x;
		BottomLeft.y = BottomRight.y;

		pDC->MoveTo (TopLeft);
		pDC->LineTo (BottomRight);
		pDC->MoveTo (TopRight);
		pDC->LineTo (BottomLeft);
	}

	// and then show the aiming arrow and arc stuff...
	if( bShowUI )
	{
		POINT		ArcTopLeft, ArcBottomRight;
		POINT		ptDirSlope ;		// Slope of the "Direction" line
		POINT		ptRotationPoint ;	// Point near end of "Direction" line we rotate to get arrowhead points
		POINT		ptRelRotatePoint ;	// Rotation points about zero
		POINT		ptPlus45 ;			// Final Arrowhead point
		POINT		ptMinus45 ;			// Final Arrowhead point
		POINT		ptStart ;			// Start point for Arc
		POINT		ptEnd ;				// End point of Arc
		float		fPercentIntoLine ;	// Distance into Direction line for rotation point
		float		fDirLength ;		// Direction line length
		float		fEntityLength ;		// Entity length
		float		fAngleToTarget ;	// Radians of arc midpoint
		geFloat		fRadius ;
		geVec3d		Angles ;
		geXForm3d	Xfm ;
		geVec3d		VecTarg ;
		float		fArc ;
		POINT		LineEndView;
		geBoolean	bUIAvailable ;

		// Get the Radius and the Angle  ONE of these must be present to show UI
		bUIAvailable = GE_FALSE ;
		if( pEnt->GetRadius( &fRadius, pEntityDefs ) == GE_FALSE )
			fRadius = 100.0f ;
		else
			bUIAvailable = GE_TRUE ;

		if( pEnt->GetAngles( &Angles, pEntityDefs ) == GE_FALSE )
			geVec3d_Clear( &Angles ) ;
		else
			bUIAvailable = GE_TRUE ;

		if( bUIAvailable == GE_FALSE )
			return ;

		// The camera angles are given in camera coordinates rather than
		// world coordinates (don't ask).
		// So we convert them here.
		if (pEnt->IsCamera ())
		{
			geVec3d_Set(&Angles, Angles.Z, (-Angles.Y-M_PI/2.0f), Angles.X);
		}

		geXForm3d_SetEulerAngles( &Xfm, &Angles ) ;
		geVec3d_Set( &VecTarg, fRadius, 0.0f, 0.0f ) ;
		geXForm3d_Transform( &Xfm, &VecTarg, &VecTarg ) ;
		geVec3d_Add( &(pEnt->mOrigin), &VecTarg, &VecTarg ) ;

		LineEndView = Render_OrthoWorldToView ( v, &VecTarg );

		// Draw to the end point
		pDC->MoveTo( EntPosView ) ;
		pDC->LineTo( LineEndView ) ;

		ptDirSlope.x = LineEndView.x - EntPosView.x ;	// Slope of Direction line
		ptDirSlope.y = LineEndView.y - EntPosView.y ;
				
		fDirLength = (float)sqrt( (ptDirSlope.x*ptDirSlope.x) + (ptDirSlope.y*ptDirSlope.y)) ;	// Length of Direction line
		fEntityLength = (float)sqrt( (EntSizeView.x*EntSizeView.x)+(EntSizeView.y*EntSizeView.y)) ;
		fEntityLength *= 2 ;	// Arrow 2x entity size
		fPercentIntoLine = 1.0f - (fEntityLength / fDirLength ) ;
		ptRotationPoint.x = (long)(ptDirSlope.x * fPercentIntoLine) ;
		ptRotationPoint.y = (long)(ptDirSlope.y * fPercentIntoLine) ;
		ptRotationPoint.x += EntPosView.x ;
		ptRotationPoint.y += EntPosView.y ;

		ptRelRotatePoint.x = ptRotationPoint.x - LineEndView.x ;
		ptRelRotatePoint.y = ptRotationPoint.y - LineEndView.y ;

		ptPlus45.x = (long)(ptRelRotatePoint.x * COS45 - ptRelRotatePoint.y * SIN45 ) ;
		ptPlus45.y = (long)(ptRelRotatePoint.y * COS45 + ptRelRotatePoint.x * SIN45 ) ;
		ptMinus45.x = (long)(ptRelRotatePoint.x * MCOS45 - ptRelRotatePoint.y * MSIN45 ) ;
		ptMinus45.y = (long)(ptRelRotatePoint.y * MCOS45 + ptRelRotatePoint.x * MSIN45 ) ;

		ptPlus45.x += LineEndView.x ;
		ptPlus45.y += LineEndView.y ;
		ptMinus45.x += LineEndView.x ;
		ptMinus45.y += LineEndView.y ;

		pDC->LineTo( ptPlus45 ) ;
		pDC->LineTo( ptMinus45 ) ;
		pDC->LineTo( LineEndView ) ;

		if( pEnt->GetArc( &fArc, pEntityDefs ) == GE_FALSE )
		{
			fArc = 0.0f ;	// All Directions
		}
		if( fArc != 0.0f )			// Draw the arc
		{
			fArc = 2*M_PI - fArc;
			fArc /= 2.0f ;	// We need half the angle
			EntSizeView.x *= 3; 
			EntSizeView.y *= 3;
			EntWidthHeight.x *= 3 ;
			EntWidthHeight.y *= 3 ;
			// Arc BB is an enlarged Entity BB
			ArcTopLeft.x		= EntPosView.x - EntSizeView.x;
			ArcTopLeft.y		= EntPosView.y - EntSizeView.y;
			ArcBottomRight.x	= EntPosView.x + EntSizeView.x;
			ArcBottomRight.y	= EntPosView.y + EntSizeView.y;
		
			fAngleToTarget = (float)atan2( ptDirSlope.y, ptDirSlope.x ) ;	// Angle line leaves
			fAngleToTarget += M_PI ;	// The other side is where the angle starts
			
			ptStart.x = (long)((EntWidthHeight.x) * cos( fAngleToTarget + fArc )) ;
			ptStart.y = (long)((EntWidthHeight.y) * sin( fAngleToTarget + fArc )) ;
			ptEnd.x = (long)((EntWidthHeight.x) * cos( fAngleToTarget - fArc )) ;
			ptEnd.y = (long)((EntWidthHeight.y) * sin( fAngleToTarget - fArc )) ;
			ptStart.x += EntPosView.x ;
			ptStart.y += EntPosView.y ;
			ptEnd.x += EntPosView.x ;
			ptEnd.y += EntPosView.y ;

			// If Start and end point are different
			if( !(ptStart.x == ptEnd.x && ptStart.y == ptEnd.y) )
			{
				pDC->Arc
				( 
					ArcTopLeft.x, ArcTopLeft.y, ArcBottomRight.x, ArcBottomRight.y, 
					ptStart.x, ptStart.y, 
					ptEnd.x, ptEnd.y
				);
			}

			// Draw the two rays out the same distance as the Direction
			ptStart.x = (long)((fDirLength) * cos( fAngleToTarget + fArc )) ;
			ptStart.y = (long)((fDirLength) * sin( fAngleToTarget + fArc )) ;
			ptStart.x += EntPosView.x ;
			ptStart.y += EntPosView.y ;
			pDC->MoveTo( EntPosView ) ;
			pDC->LineTo( ptStart ) ;

			ptEnd.x = (long)((fDirLength) * cos( fAngleToTarget - fArc )) ;
			ptEnd.y = (long)((fDirLength) * sin( fAngleToTarget - fArc )) ;
			ptEnd.x += EntPosView.x ;
			ptEnd.y += EntPosView.y ;
			pDC->MoveTo( EntPosView ) ;
			pDC->LineTo( ptEnd ) ;
		}// Arc for this entity exists
	}
}/* fdocDrawEntity */


// ENUMERATION FUNCTIONS (File Scope)


	
static geBoolean fdocBrushCSGCallback (const Brush *pBrush, void *lParam)
{
	CFusionDoc *pDoc = (CFusionDoc *)lParam;

	return (pDoc->BrushIsVisible (pBrush) && (!Brush_IsHint(pBrush)) && (!Brush_IsClip(pBrush)));
}



typedef struct
{
	BOOL Select;
	int WhichGroup;
	CFusionDoc *pDoc;
} BrushSelectCallbackData;

// ::BrushList_CB Enumeration function to select/deselect brushes
static geBoolean BrushSelect( Brush *pBrush, void *lParam)
{
	BrushSelectCallbackData *pData;

	pData = (BrushSelectCallbackData *)lParam;

	// select/deselect all group brushes
	if( Brush_GetGroupId( pBrush ) == pData->WhichGroup )
	{
		if( pData->Select )
		{	// add this brush to the selected list
			SelBrushList_Add (pData->pDoc->pSelBrushes, pBrush);
		}
		else
		{
			// remove this brush from the selection list
			SelBrushList_Remove (pData->pDoc->pSelBrushes, pBrush);
		}
	}
	return GE_TRUE ;	// Continue enumeration
}/* ::BrushSelect */

// ::EntityList_CB Enumeration function to select/deselect brushes
static geBoolean EntitySelect( CEntity& Entity, void * lParam )
{
	BrushSelectCallbackData *pData;
	CFusionDoc *pDoc;

	pData = (BrushSelectCallbackData *)lParam;
	pDoc = pData->pDoc;

	if( Entity.GetGroupId () == pData->WhichGroup )
	{
		if( pData->Select )
		{
			pDoc->SelectEntity( &Entity );
		}
		else
		{
			pDoc->DeselectEntity( &Entity );
		}
	}
	return GE_TRUE ;	// Continue enumeration
}/* ::EntitySelect */

#define fdoc_SHOW_ALL_GROUPS -1

typedef struct tagBrushDrawData
{
	const Box3d	*	pViewBox ;
	CDC *		pDC ;
	ViewVars *	v ;
	int GroupId;
	CFusionDoc *pDoc;
	BrushFlagTest FlagTest;
	uint32		Color;
} BrushDrawData ;

static geBoolean BrushDraw( Brush *pBrush, void *lParam)
{
	BrushDrawData * pData = (BrushDrawData*)lParam ;
	CFusionDoc * pDoc = pData->pDoc;

	if ((pData->GroupId == fdoc_SHOW_ALL_GROUPS) || (Brush_GetGroupId (pBrush) == pData->GroupId))
	{
		if ((pData->FlagTest == NULL) || pData->FlagTest (pBrush))
		{
			if (pDoc->fdocShowBrush (pBrush, pData->pViewBox))
			{
				Render_RenderBrushFacesOrtho(pData->v, pBrush, pData->pDC->m_hDC);
			}
		}
	}
	return GE_TRUE ;
}/* ::BrushDraw */


static geBoolean BrushDrawSelFacesOrtho(Brush *pBrush, void *lParam)
{
	BrushDrawData	*pData;

	pData	=(BrushDrawData *)lParam;

	Render_RenderBrushSelFacesOrtho(pData->v, pBrush, pData->pDC->m_hDC);

	return	GE_TRUE;
}

static geBoolean BrushDrawSheetFacesOrtho(Brush *pBrush, void *lParam)
{
	BrushDrawData	*pData;

	if(Brush_IsSheet(pBrush))
	{
		pData	=(BrushDrawData *)lParam;

		Render_RenderBrushSheetFacesOrtho(pData->v, pBrush, pData->pDC->m_hDC);
	}
	return	GE_TRUE;
}

static geBoolean EntityDraw( CEntity& Entity, void * lParam )
{
	BrushDrawData *pData;

	pData = (BrushDrawData *)lParam;

	if( Entity.GetGroupId () != pData->GroupId )
		return GE_TRUE ;

	if ( (Entity.IsSelected() == GE_FALSE ) && pData->pDoc->EntityIsVisible( &Entity ) )
	{
		fdocDrawEntity (&Entity, pData->v, pData->pDC, Level_GetEntityDefs (pData->pDoc->pLevel), GE_FALSE );
	}
	return GE_TRUE ;
}/* ::EntityDraw */

static geBoolean	BrushDrawWire3dCB(Brush *pBrush, void *lParam)
{
	BrushDrawData *pData = (BrushDrawData *)lParam;
	CFusionDoc *pDoc = pData->pDoc;

	if ((pData->GroupId == fdoc_SHOW_ALL_GROUPS) || (Brush_GetGroupId (pBrush) == pData->GroupId))
	{
		if ((pData->FlagTest == NULL) || pData->FlagTest (pBrush))
		{
			if (pDoc->BrushIsVisible (pBrush))
			{
				Render_RenderBrushFaces (pData->v, pBrush, pData->Color);
			}
		}
	}
	return	GE_TRUE ;
}

static geBoolean	BrushDrawWire3dZBufferCB(Brush *pBrush, void *lParam)
{
	BrushDrawData *pData = (BrushDrawData *)lParam;
	CFusionDoc *pDoc = pData->pDoc;

	if ((pData->GroupId == fdoc_SHOW_ALL_GROUPS) || (Brush_GetGroupId (pBrush) == pData->GroupId))
	{
		if ((pData->FlagTest == NULL) || pData->FlagTest (pBrush))
		{
			if (pDoc->BrushIsVisible (pBrush))
			{
				Render_RenderBrushFacesZBuffer(pData->v, pBrush, pData->Color);
			}
		}
	}
	return	GE_TRUE ;
}

static geBoolean	BrushDrawWireSel3dCB(Brush *b, void *lParam)
{
	BrushDrawData *pData;

	pData = (BrushDrawData *)lParam;
	Render_RenderBrushSelFaces(pData->v, b, pData->Color);
	return	GE_TRUE ;
}

// END ENUMERATION FUNCTIONS


geBoolean CFusionDoc::fdocShowBrush
	(
	  Brush const *b,
	  Box3d const *ViewBox
	)
{
	return (BrushIsVisible (b) && Brush_TestBoundsIntersect(b, ViewBox));
}/* CFusionDoc::fdocShowBrush */


BOOL CFusionDoc::OnNewDocument()
{
	if(!CDocument::OnNewDocument())
	{
		return FALSE;
	}

	SetupDefaultFilename ();

	UpdateGridInformation();

	UpdateAllViews( UAV_ALL3DVIEWS, NULL );
	return TRUE;
}


geBoolean CFusionDoc::Save(const char *FileName)
{
	{
		// update view information in level
		ViewStateInfo *pViewStateInfo;
		POSITION		pos;
		CFusionView	*	pView;
		int iView;

		pos = GetFirstViewPosition();
		while( pos != NULL )
		{
			pView = (CFusionView*)GetNextView(pos) ;
			switch (Render_GetViewType (pView->VCam))
			{
				case VIEWSOLID :
				case VIEWTEXTURE :
				case VIEWWIRE :
					iView = 0;
					break;
				case VIEWTOP :
					iView = 1;
					break;
				case VIEWFRONT :
					iView = 2;
					break;
				case VIEWSIDE :
					iView = 3;
					break;
				default :
					iView = -1;
			}
			if (iView != -1)
			{
				pViewStateInfo = Level_GetViewStateInfo (pLevel, iView);
				pViewStateInfo->IsValid = GE_TRUE;
				pViewStateInfo->ZoomFactor = Render_GetZoom (pView->VCam);
				Render_GetPitchRollYaw (pView->VCam, &pViewStateInfo->PitchRollYaw);
				Render_GetCameraPos (pView->VCam, &pViewStateInfo->CameraPos);
			}
		}
	}

	// and then write the level info to the file
	return Level_WriteToFile (pLevel, FileName);
}

static geBoolean fdocSetEntityVisibility (CEntity &Ent, void *lParam)
{
	EntityViewEntry *pEntry = (EntityViewEntry *)lParam;

	if (Ent.GetClassname () == pEntry->pName)
	{
		Ent.SetVisible (pEntry->IsVisible);
	}
	return GE_TRUE;
}

/*
	Load file versions later than 1.0
*/
geBoolean CFusionDoc::Load(const char *FileName)
{
	const char		*Errmsg, *WadPath;
	int				i;
	Level			*NewLevel;
	EntityViewList	*pEntityView;
	const Prefs *pPrefs = GetPrefs ();

	{
		char WorkingDir[MAX_PATH];

		FilePath_GetDriveAndDir (FileName, WorkingDir);
		::SetCurrentDirectory (WorkingDir);
	}

	NewLevel = Level_CreateFromFile (FileName, &Errmsg, Prefs_GetHeadersList (pPrefs));
	if (NewLevel == NULL)
	{
		goto LoadError;
	}
	// get fully-qualified path name to texture library
	WadPath = Level_GetWadPath (NewLevel);

	if (!Level_LoadWad (NewLevel))
	{
		CString Msg;

		AfxFormatString1 (Msg, IDS_CANTLOADTXL, WadPath);
		AfxMessageBox (Msg);
	}
	Level_EnumLeafBrushes (NewLevel, NewLevel, Level_FaceFixupCallback);

	if (pLevel != NULL)
	{
		Level_Destroy (&pLevel);
	}
	pLevel = NewLevel;

	// Validate data, groups are read after entities and brushes, so this must be last
	if( ValidateEntities( ) == FALSE || ValidateBrushes( ) == FALSE )
	{
		SelectTab( CONSOLE_TAB ) ;
		AfxMessageBox( IDS_LOAD_WARNING ) ;
	}

	GroupIterator gi;
	GroupListType *Groups;

	Groups = Level_GetGroups (pLevel);
	mCurrentGroup = Group_GetFirstId (Groups, &gi);
	AddCameraEntityToLevel ();

	{
		Brush *pBox = BrushTemplate_CreateBox (Level_GetBoxTemplate (pLevel));
		if (pBox != NULL)
		{
			CreateNewTemplateBrush (pBox);
		}
	}

	// update entity visibility info
	pEntityView	=Level_GetEntityVisibilityInfo (pLevel);
	for (i = 0; i < pEntityView->nEntries; ++i)
	{
		Level_EnumEntities (pLevel, &pEntityView->pEntries[i], ::fdocSetEntityVisibility);
	}

	return GE_TRUE;
LoadError:
	if (NewLevel != NULL)
	{
		Level_Destroy (&NewLevel);
	}
	AfxMessageBox (Errmsg);
	return GE_FALSE;
}

struct fdocFaceScales
{
	float DrawScale;
	float LightmapScale;
};

static geBoolean fdocSetFaceScales (Face *pFace, void *lParam)
{
	fdocFaceScales *pScales = (fdocFaceScales *)lParam;

	Face_SetTextureScale (pFace, pScales->DrawScale, pScales->DrawScale);
	Face_SetLightScale (pFace, pScales->LightmapScale, pScales->LightmapScale);
	return GE_TRUE;
}


// play a sound from a resource
static geBoolean PlayResource (char const * pName) 
{     
	BOOL bRtn;
	char * lpRes; 
	HGLOBAL hRes;
	HRSRC hResInfo;
	HINSTANCE hInst = AfxGetInstanceHandle ();

	// Find the WAVE resource.
    hResInfo = ::FindResource (hInst, pName, "WAVE");
	if (hResInfo == NULL)
	{
        return FALSE;
	}

	// Load the WAVE resource.  
    hRes = ::LoadResource(hInst, hResInfo);     
	if (hRes == NULL)
	{
        return FALSE;
	}

	// Lock the WAVE resource and play it.  
    lpRes = (char *)::LockResource(hRes);     
	if (lpRes != NULL) 
	{
        bRtn = (::sndPlaySound(lpRes, SND_MEMORY | SND_SYNC | SND_NODEFAULT) != 0);
		::UnlockResource(hRes);
	}
	else 
	{
        bRtn = GE_FALSE;
	}

	// Free the WAVE resource and return success or failure. 
    FreeResource (hRes);
	return bRtn; 
}

void CFusionDoc::OnBrushAddtoworld() 
{
	geBoolean Placed;

	if(mModeTool!=ID_TOOLS_TEMPLATE)
		return;

	Placed = GE_FALSE;
	if(TempEnt)
	{
		// here we check to see if the user is placing library objects down
		if ( PlaceObjectFlag )
		{
			geBoolean GotName;
			char ObjectName[MAX_PATH];

			// get the current object from the library and place it down
			GotName = mpMainFrame->m_wndTabControls->m_pBrushEntityDialog->GetCurrentObjectName (ObjectName);
			if (GotName)
			{
				Placed = PlaceObject (ObjectName, &mRegularEntity.mOrigin);
				::PlayResource ("SND_WHOOSH");
				UpdateAllViews(UAV_ALL3DVIEWS, NULL);
			}
		}
		else
		{
			geBoolean GotName;
			CEntity NewEnt;
			char EntityName[MAX_PATH];

			mRegularEntity.DoneMove (1, Level_GetEntityDefs (pLevel));

			// now create a new entity of the currently-selected type
			GotName = mpMainFrame->m_wndTabControls->m_pBrushEntityDialog->GetCurrentEntityName (EntityName);
			if (GotName)
			{	
				if (CreateEntityFromName (EntityName, NewEnt))
				{
					geVec3d org;
					org = mRegularEntity.mOrigin;
					// set the new entity's origin
					NewEnt.SetOrigin (org.X, org.Y, org.Z, Level_GetEntityDefs (pLevel));
					// add to current group
					NewEnt.SetGroupId (mCurrentGroup);
					Level_AddEntity (pLevel, NewEnt);
					Placed = GE_TRUE;
					::PlayResource ("SND_WHOOSH");
					UpdateAllViews(UAV_ALL3DVIEWS, NULL);
				}
			}
		}
	}
	else
	{
		Brush	*nb;
		geVec3d *pTemplatePos;

		nb	=Brush_Clone(CurBrush);

		SetDefaultBrushTexInfo(nb);
		Brush_Bound (nb);
		pTemplatePos = Level_GetTemplatePos (pLevel);
		Brush_Center (nb, pTemplatePos);
		// add to current group
		Brush_SetGroupId (nb, mCurrentGroup);

		{
			// set draw scale and lightmap scale defaults for all faces
			fdocFaceScales Scales;

			Scales.DrawScale = Level_GetDrawScale (pLevel);
			Scales.LightmapScale = Level_GetLightmapScale (pLevel);
			Brush_EnumFaces (nb, &Scales, ::fdocSetFaceScales);

		}

		Level_AppendBrush (pLevel, nb);

		::PlayResource ("SND_WHOOSH");
		if(!Brush_IsHollow(nb) && !Brush_IsMulti(nb))
		{
			mWorldBsp	=Node_AddBrushToTree(mWorldBsp, nb);
			UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		}
		else
		{
			UpdateAllViews(UAV_ALL3DVIEWS | REBUILD_QUICK, NULL, TRUE);
		}

		Placed = GE_TRUE;
	}
	if (Placed)
	{
		SetModifiedFlag();
	}
}

// This code is STILL called from the toolbar button, but the "correct"
// interface is now on the Brush Attributes Dialog
void CFusionDoc::OnBrushSubtractfromworld() 
{
	Brush	*nb;
	BrushList *BList = Level_GetBrushes (pLevel);

	if ((mModeTool==ID_GENERALSELECT) && (BrushList_Count (BList, BRUSH_COUNT_MULTI | BRUSH_COUNT_LEAF) < 2))
	{
		// cuts shouldn't start the list
		return;
	}

	SetModifiedFlag();

	if(mModeTool==ID_GENERALSELECT)
	{
		// put the brush at the very end of the list
		BrushList_Remove (BList, CurBrush);
		Brush_SetSubtract(CurBrush, GE_TRUE);

		SelBrushList_RemoveAll (pSelBrushes);
		BrushList_Append (BList, CurBrush);
	}
	else
	{
		nb	=Brush_Clone(CurBrush);

		SetDefaultBrushTexInfo(nb);
		Brush_Bound (nb);
		BrushList_Append (BList, nb);
	}
	UpdateSelected();
	UpdateAllViews(UAV_ALL3DVIEWS | REBUILD_QUICK, NULL, TRUE);
}

void CFusionDoc::CopySelectedBrushes(void)
{
	int		i;
	int		NumSelBrushes;

	NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	if( NumSelBrushes )
	{
		// create copies of the selected brushes
		for(i=0;i < NumSelBrushes;i++)
		{
			// clone the brush, add copy to level,
			// add copy to select list, and remove original from
			// select list.
			Brush *pBrush;
			Brush *pClone;

			pBrush = SelBrushList_GetBrush (pSelBrushes, 0);
			pClone = Brush_Clone (pBrush);
			Level_AppendBrush (pLevel, pClone);
			SelBrushList_Add (pSelBrushes, pClone);
			SelBrushList_Remove (pSelBrushes, pBrush);
		}
	}// Duplicate selected and delesect original Brushes


	CEntity  TEnt;
	CEntityArray *Entities;
	int cnt;

	Entities = Level_GetEntities (pLevel);

	cnt = Entities->GetSize() ;
	for( i=0 ; i < cnt; i++ )
	{
		CEntity *pEnt = &(*Entities)[i];
		if( pEnt->IsCamera() == GE_FALSE  )	// Exclude Cameras
		{
			if (pEnt->IsSelected ())
			{
				DeselectEntity (pEnt);
				/*
				  There really is a reason for this oddity.
				  Because Level_AddEntity might cause the entities array to grow,
				  dereferencing the data in the array can cause problems because
				  growing the array might cause the dereferenced data to move.
				  (Fun, huh?)  So I create a temporary entity, copy the contents
				  there, and then use that as cloning material.
				*/
				CEntity WorkingEntity;
				int		Index ;

				WorkingEntity = *pEnt;
				Index = Level_AddEntity (pLevel, WorkingEntity);

				SelectEntity( &(*Entities)[Index] );
			}
		}
	}

	ConPrintf("Cloned %d Brushes, %d Entities.\n", NumSelBrushes, NumSelEntities);

	// Copying items places the new items in the same group, so we must update the UI
	mpMainFrame->m_wndTabControls->GrpTab->UpdateTabDisplay( this ) ;
	UpdateSelected();
	UpdateAllViews( UAV_ALL3DVIEWS, NULL );
}

void CFusionDoc::MakeSelectedBrushNewest(void)
{
	if(GetSelState()==ONEBRUSHONLY)
	{
		Level_RemoveBrush (pLevel, CurBrush);
		Level_AppendBrush (pLevel, CurBrush);
	}
}


void CFusionDoc::TempCopySelectedBrushes(void)
{
	int		i;
	int NumSelBrushes;

	NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	SelBrushList_RemoveAll (pTempSelBrushes);

	// make copies of selected brushes
	for(i=0;i < NumSelBrushes;i++)
	{
		Brush	*pBrush, *pClone;

		pBrush = SelBrushList_GetBrush (pSelBrushes, i);
		pClone = Brush_Clone (pBrush);
		Level_AppendBrush (pLevel, pClone);
		SelBrushList_Add (pTempSelBrushes, pClone);
	}
}

typedef struct
{
	CFusionDoc *pDoc;
	const char *TexName;
} BrushTexSetData;

static geBoolean	BrushTexSetCB (Brush *b, void *lParam)
{
	int			i;
	BrushTexSetData *pData;

	pData = (BrushTexSetData *)lParam;

	Brush_SetName(b, pData->TexName);
	char const * const BrushName = Brush_GetName (b);
	const int NumFaces = Brush_GetNumFaces (b);

	//copy face TexInfos
	for(i=0;i < NumFaces;i++)
	{
		Face	*f	=Brush_GetFace(b, i);
		WadFileEntry *pbmp;

		Face_SetTextureName(f, BrushName);
		Face_SetTextureDibId(f, Level_GetDibId (pData->pDoc->pLevel, BrushName));
		pbmp = Level_GetWadBitmap (pData->pDoc->pLevel, BrushName);
		if (pbmp != NULL)
		{
			Face_SetTextureSize (f, pbmp->Width, pbmp->Height);
		}
	}
	Brush_SetFaceListDirty(b);
	return GE_TRUE ;
}

void CFusionDoc::SetDefaultBrushTexInfo(Brush *b)
{
	const char *TexName;
	BrushTexSetData CallbackData;

	TexName = mpMainFrame->m_wndTabControls->GetCurrentTexture();
	CallbackData.pDoc = this;
	CallbackData.TexName = TexName;

	Brush_SetName(b, TexName);
	if(Brush_IsMulti(b))
	{
		BrushList_EnumLeafBrushes (Brush_GetBrushList(b), &CallbackData, ::BrushTexSetCB) ;
	}
	else
	{
		::BrushTexSetCB (b, &CallbackData);
	}
}

// Test BOTH group and brush visiblity--group overrides
// User flags for All, Visible, and Current group must be considered
geBoolean CFusionDoc::BrushIsVisible( const Brush * pBrush ) const
{
	int			GroupId ;

	if (!Brush_IsVisible (pBrush))
	{
		return GE_FALSE;
	}
	GroupId = Brush_GetGroupId (pBrush);

	switch (Level_GetGroupVisibility (pLevel))
	{
		case Group_ShowAll :
			return GE_TRUE;

		case Group_ShowCurrent :
		    return (GroupId == mCurrentGroup);
	
		case Group_ShowVisible :
			return Group_IsVisible (Level_GetGroups (pLevel), GroupId);
	
		default :
			assert (0);
			return GE_FALSE;
	}
}/* CFusionDoc::BrushIsVisible */


// Test BOTH group and entity visiblity--group overrides
// User flags for All, Visible, and Current group must be considered
geBoolean CFusionDoc::EntityIsVisible( const CEntity *pEntity ) const
{
	int			GroupId ;

	if (pEntity->IsCamera ())
	{
		return pEntity->IsVisible ();
	}
	if ((mShowEntities == GE_FALSE) || !pEntity->IsVisible ())
	{
		return GE_FALSE ;
	}

	GroupId = pEntity->GetGroupId( );

	switch (Level_GetGroupVisibility (pLevel))
	{
	    case Group_ShowAll :
		    return GE_TRUE;

		case Group_ShowCurrent :
		    return (GroupId == mCurrentGroup);

		case Group_ShowVisible :
		    return Group_IsVisible (Level_GetGroups (pLevel), GroupId);

		default :
		    assert (0);
			return GE_FALSE;
	}
}/* CFusionDoc::EntityIsVisible */

void CFusionDoc::CreateNewTemplateBrush
	(
	  Brush *pBrush
	)
{
	geVec3d *pTemplatePos;
	geVec3d MoveVec;
	geVec3d BrushPos;

	assert (pBrush != NULL);

	if (BTemplate != NULL)
	{
		Brush_Destroy (&BTemplate);
	}
	BTemplate = pBrush;
	CurBrush = pBrush;

	TempEnt	= FALSE;
	SetDefaultBrushTexInfo (CurBrush);
	Brush_Bound (CurBrush);
	Brush_Center (CurBrush, &BrushPos);

	pTemplatePos = Level_GetTemplatePos (pLevel);
	geVec3d_Subtract (pTemplatePos, &BrushPos, &MoveVec);
	Brush_Move (CurBrush, &MoveVec);

	UpdateAllViews (UAV_ALL3DVIEWS, NULL);
	SetModifiedFlag ();
}

void CFusionDoc::OnUpdateBrushPrimitives (CCmdUI *pCmdUI)
{
	// This function is used by all the primitive UI OnUpdateXXX's
	pCmdUI->Enable( (mModeTool == ID_TOOLS_TEMPLATE ) ? TRUE : FALSE ) ;
}

void CFusionDoc::OnBrushPrimitivesCube() 
{
	CreateCube() ;
}


void CFusionDoc::CreateCube() 
{
	BrushTemplate_Box *pBoxTemplate = Level_GetBoxTemplate (pLevel);
	CCreateBoxDialog mBoxCreation;

	if( mBoxCreation.DoModal((Level_GetGridType (pLevel) == GridMetric), pBoxTemplate) == IDOK )
	{
		Brush *pCube;

		pCube = ::BrushTemplate_CreateBox (pBoxTemplate);
		if (pCube != NULL)
		{
			CreateNewTemplateBrush (pCube);
		}
	}
}/* CFusionDoc::CreateCube */

void CFusionDoc::OnBrushPrimitivesSpheroid() 
{
	CreateSpheroid() ;
}


void CFusionDoc::CreateSpheroid() 
{
	BrushTemplate_Spheroid *pTemplate = Level_GetSpheroidTemplate (pLevel);
	CCreateSpheroidDialog mSpheroidCreation;

	if( mSpheroidCreation.DoModal(Level_GetGridType (pLevel) == GridMetric, pTemplate) == IDOK )
	{
		Brush *pBrush;

		pBrush = BrushTemplate_CreateSpheroid (pTemplate);
		if (pBrush != NULL)
		{
			CreateNewTemplateBrush (pBrush);
		}
	}
}/* CFusionDoc::CreateSpheroid */

void CFusionDoc::OnBrushPrimitivesCylinder() 
{
	CreateCylinder() ;
}


void CFusionDoc::CreateCylinder() 
{
	BrushTemplate_Cylinder *pCylTemplate = Level_GetCylinderTemplate (pLevel);
	CCreateCylDialog mCylCreation;

	if( mCylCreation.DoModal ((Level_GetGridType (pLevel) == GridMetric), pCylTemplate) == IDOK )
	{
		Brush *pCyl;

		pCyl = BrushTemplate_CreateCylinder (pCylTemplate);
		if (pCyl != NULL)
		{
			CreateNewTemplateBrush (pCyl);
		}
	}
}/* CFusionDoc::CreateCylinder */

void CFusionDoc::OnBrushPrimitivesStaircase() 
{
	CreateStaircase() ;
}


void CFusionDoc::CreateStaircase()
{
	CCreateStaircaseDialog mStairCreation;
	BrushTemplate_Staircase *pStairTemplate = Level_GetStaircaseTemplate (pLevel);

	if( mStairCreation.DoModal(Level_GetGridType (pLevel) == GridMetric, pStairTemplate) == IDOK )
	{
		Brush *pStair;

		pStair = BrushTemplate_CreateStaircase (pStairTemplate);
		if (pStair != NULL)
		{
			CreateNewTemplateBrush (pStair);
		}
	}
}/* CFusionDoc::CreateStaircase */

void CFusionDoc::OnBrushPrimitivesArch() 
{
	CreateArch ();
}

void CFusionDoc::CreateArch()
{
	CCreateArchDialog mArchCreation;
	BrushTemplate_Arch *pArchTemplate = Level_GetArchTemplate (pLevel);

	if( mArchCreation.DoModal((Level_GetGridType (pLevel) == GridMetric), pArchTemplate) == IDOK )
	{
		Brush *pArch;

		pArch = BrushTemplate_CreateArch (pArchTemplate);

		if (pArch != NULL)
		{
			CreateNewTemplateBrush (pArch);
		}
	}
}/* CFusionDoc::CreateArch */

	
void CFusionDoc::OnBrushPrimitivesCone() 
{
	CreateCone ();
}
void CFusionDoc::CreateCone()
{
	CCreateConeDialog mConeCreation;
	BrushTemplate_Cone *pConeTemplate = Level_GetConeTemplate (pLevel);

	if( mConeCreation.DoModal ((Level_GetGridType (pLevel) == GridMetric), pConeTemplate) == IDOK )
	{
		Brush *pCone;

		pCone = BrushTemplate_CreateCone (pConeTemplate);
		if (pCone != NULL)
		{
			CreateNewTemplateBrush (pCone);
		}
	}
}

void CFusionDoc::BrushSelect
	(
	  Brush *pBrush
	)
{
	// if the brush is already selected, then unselect it.
	// if not currently selected, then select it.
	if (!SelBrushList_Remove (pSelBrushes, pBrush))
	{
		SelBrushList_Add (pSelBrushes, pBrush);
	}
}

geBoolean CFusionDoc::BrushIsSelected
	(
	  Brush const *pBrush
	)
{
	assert (pBrush != NULL);

	return SelBrushList_Find (pSelBrushes, pBrush);
}



void CFusionDoc::OnToolsUsegrid() 
{
	GridInfo *pGridInfo = Level_GetGridInfo (pLevel);

	pGridInfo->UseGrid = !(pGridInfo->UseGrid);

	UpdateGridInformation();
	UpdateAllViews(UAV_GRID_ONLY, NULL);
}

void CFusionDoc::OnUpdateToolsUsegrid(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( Level_UseGrid (pLevel));
}

void CFusionDoc::OnToolsGridsettings() 
{
	int				gdex[3]={GridSize_Centimeter, GridSize_Decimeter, GridSize_Meter};
	CGridSizeDialog dlg;
	GridInfo *pGridInfo;
	static int TexelSnapValues[] = {1, 2, 4, 8, 16, 32};
	static int nSnapValues = sizeof (TexelSnapValues)/sizeof (int);

	pGridInfo = Level_GetGridInfo (pLevel);

	dlg.m_UseSnap			=pGridInfo->UseGrid;
	dlg.m_SnapDegrees		=pGridInfo->RotationSnap;
	
	dlg.MetricOrTexelSnap	=pGridInfo->SnapType;
	dlg.MetricOrTexelGrid	=pGridInfo->GridType;

	if( dlg.MetricOrTexelSnap == 0 )
	{
		switch(pGridInfo->MetricSnapSize)
		{
			case GridSize_Centimeter:
				dlg.m_GridUnits	=0;
				break;
			case GridSize_Decimeter:
				dlg.m_GridUnits	=1;
				break;
			case GridSize_Meter:
				dlg.m_GridUnits	=2;
				break ;
		}
	}
	else
	{
		int i;

		dlg.m_GridUnits = 0;
		for (i = 0; i < nSnapValues; ++i)
		{
			if (pGridInfo->TexelSnapSize == TexelSnapValues[i])
			{
				dlg.m_GridUnits = i;
				break;
			}
		}
// commented here because metric is disabled...
//		dlg.m_GridUnits += 3;	// metric takes up the first 3
	}
	if(dlg.DoModal()==IDOK)
	{
		pGridInfo->GridType = dlg.MetricOrTexelGrid;	// Main Visible Grid mode 
		// must add here 'cause metric grid is disabled
		dlg.m_GridUnits += 3;
		switch( dlg.m_GridUnits ) // SnapGrid RB's 0-2 METRIC 3-5 TEXEL
		{
			case 0:// Centimeter
			case 1:// Decimeter
			case 2:// Meter
				pGridInfo->MetricSnapSize	=gdex[dlg.m_GridUnits];
				pGridInfo->SnapType = GridMetric;
				break ;
			case 3:// 1
			case 4:// 2
			case 5:// 4
			case 6:
			case 7:
			case 8:
				pGridInfo->TexelSnapSize = TexelSnapValues[dlg.m_GridUnits - 3];
				pGridInfo->SnapType = GridTexel;
				break ;
			default :
				assert (0);	// unexpected grid units value!
				break;
		}
		
		pGridInfo->UseGrid = dlg.m_UseSnap;
		pGridInfo->RotationSnap = (int)(dlg.m_SnapDegrees);
		UpdateGridInformation();
		UpdateAllViews(UAV_GRID_ONLY, NULL);
	}// DoModal OK
}/* CFusionDoc::OnToolsGridsettings */

void CFusionDoc::OnEditUndo() 
{
#if 0
	// Well, here's the basic idea of undo.
	// It doesn't work 'cause the stack isn't being built.
	UndoStackEntry *pEntry;
	UndoEntryType EntryType;

	pEntry = UndoStack_Pop (pUndoStack);

	EntryType = UndoStackEntry_GetType (pEntry);
	switch (EntryType)
	{
		case UNDO_MOVE :
		{
			int i;
			UndoMoveEntry *pMove;
			geVec3d UndoVec;

			ResetAllSelections ();
			pMove = UndoStackEntry_GetMoveInfo (pEntry);
			geVec3d_Scale (&(pMove->MoveDelta), -1.0f, &UndoVec);
			for (i = 0; i < Array_GetSize (pMove->BrushArray); ++i)
			{
				Brush *pBrush;

				pBrush = (Brush *)Array_ItemPtr (pMove->BrushArray, i);
				Brush_Move (pBrush, &UndoVec);
				SelListAddBrush (pBrush);
			}

			for (i = 0; i < Array_GetSize (pMove->EntityArray); ++i)
			{
				CEntity *pEnt;

				pEnt = (CEntity *)Array_ItemPtr (pMove->EntityArray, i);
				SelectEntity (pEnt);
			}

			UpdateSelected ();

			if (GetSelState() & ANYENTITY)
			{
				DoneMoveEntity();
			}

			UpdateAllViews( UAV_ALL3DVIEWS, NULL );

			break;
		}

		default :
			assert (0);
			break;
	}
	UndoStackEntry_Destroy (&pEntry);
#endif
}

void CFusionDoc::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (CanUndo ());
}

//unimplemented currently
void CFusionDoc::OnEditRedo() 
{
//	CurBrush->RedoLast();
//	UpdateAllViews(UAV_ALLVIEWS, NULL);
}

void CFusionDoc::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (CanRedo ());
}


void CFusionDoc::OnEntitiesEditor() 
{
	CEntitiesDialog Dialog (AfxGetMainWnd ());
	CEntityArray *Entities = Level_GetEntities (pLevel);

	Dialog.EditEntity( *Entities, mCurrentEntity, this);
	UpdateEntityOrigins();
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
	SetModifiedFlag();
}

geBoolean CFusionDoc::CreateEntityFromName
    (
	  char const *pEntityType,
	  CEntity &NewEnt
	)
{
	assert (pEntityType != NULL);
	// get all properties for this entity type...
	EntityPropertiesList *pProps;

	pProps = EntityTable_GetEntityPropertiesFromName (Level_GetEntityDefs (pLevel), pEntityType, ET_ALL);
	if (pProps == NULL)
	{
	    return FALSE;
	}

	// Add key/value pairs for all of the properties...
	for (int PropNo = 0; PropNo < pProps->NumProps; ++PropNo)
	{
	    EntityProperty *p = &(pProps->Props[PropNo]);
		
		NewEnt.SetKeyValue (p->pKey, p->pValue);

	}

	EntityTable_ReleaseEntityProperties (pProps);

	NewEnt.SetGroupId ( 0 );
	NewEnt.UpdateOrigin (Level_GetEntityDefs (pLevel));
	return TRUE;
}

void CFusionDoc::CreateEntity
    (
	  char const *pEntityType
	)
{
	CEntity NewEnt;
	
	if (CreateEntityFromName (pEntityType, NewEnt))
	{
		TempEnt = TRUE;

		mRegularEntity = NewEnt;
		mCurrentEntity = -1;
		// set this flag so that doc knows when enter is pressed that user is NOT adding 
		// objects to level
		PlaceObjectFlag = FALSE;

		mCurrentTool=ID_TOOLS_BRUSH_MOVEROTATEBRUSH;
		ConfigureCurrentTool();

		UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		SetModifiedFlag();
	}
}

// creates a template entity with which the user specifies a location for any 
// objects they place
void CFusionDoc::CreateObjectTemplate()
{
	CEntity NewEnt;

	// make an origin
	if (CreateEntityFromName ("ModelOrigin", NewEnt))
	{
		TempEnt=TRUE;

		NewEnt.SetOrigin (0.0f, 0.0f, 0.0f, Level_GetEntityDefs (pLevel));

		mRegularEntity=NewEnt;
		mCurrentEntity=-1;

		// set this flag so that doc knows when enter is pressed that user is adding objects
		// to level
		PlaceObjectFlag = TRUE;

		mCurrentTool=ID_TOOLS_BRUSH_MOVEROTATEBRUSH;
		ConfigureCurrentTool();

		UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		SetModifiedFlag();
	}
}

void CFusionDoc::ResetAllSelections (void)
{
	ResetAllSelectedFaces();
	ResetAllSelectedBrushes();
	ResetAllSelectedEntities();
}


void CFusionDoc::OnEntitiesShow(void)
{
	mShowEntities = !mShowEntities;
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::OnUpdateEntitiesShow(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( mShowEntities );
}

void CFusionDoc::OnViewShowAllGroups() 
{
	CurBrush	=BTemplate;
	Level_SetGroupVisibility (pLevel, Group_ShowAll);

	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}


void CFusionDoc::OnViewShowCurrentGroup() 
{
	CurBrush = BTemplate;
	Level_SetGroupVisibility (pLevel, Group_ShowCurrent);

	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::OnViewShowVisibleGroups() 
{
	CurBrush	=BTemplate;
	Level_SetGroupVisibility (pLevel, Group_ShowVisible);
	
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::OnUpdateViewShowVisibleGroups(CCmdUI* pCmdUI) 
{
	BOOL	bEnable ;
	int Setting;

	Setting = Level_GetGroupVisibility (pLevel);
	bEnable = ( Group_GetCount( Level_GetGroups (pLevel)) ) ? TRUE : FALSE ;
	
	if (((pCmdUI->m_nID == ID_VIEW_SHOW_ALLGROUPS) && (Setting == Group_ShowAll)) ||
		((pCmdUI->m_nID == ID_VIEW_SHOW_CURRENTGROUP) && (Setting == Group_ShowCurrent)) ||
		((pCmdUI->m_nID == ID_VIEW_SHOW_VISIBLEGROUPS) && (Setting == Group_ShowVisible)))
	{
		pCmdUI->SetCheck();
	}
	else
	{
		pCmdUI->SetCheck(0);
	}

	pCmdUI->Enable( bEnable ) ;
}

void CFusionDoc::OnUpdateViewShowAllGroups(CCmdUI* pCmdUI) 
{
	OnUpdateViewShowVisibleGroups( pCmdUI ) ;
}


void CFusionDoc::OnUpdateViewShowCurrentGroup(CCmdUI* pCmdUI) 
{
	OnUpdateViewShowVisibleGroups( pCmdUI ) ;
}


int CFusionDoc::CanUndo()
{
	return !(UndoStack_IsEmpty (pUndoStack));
}

void CFusionDoc::SaveBrushUndo()
{
}

int CFusionDoc::CanRedo()
{
	return 0;
}

void CFusionDoc::UpdateEntityOrigins()
{
	int	i;
	CEntityArray *Entities;

	Entities = Level_GetEntities (pLevel);

	for(i=0;i < Entities->GetSize();i++)
	{
		(*Entities)[i].UpdateOrigin(Level_GetEntityDefs (pLevel));
	}
}


void CFusionDoc::MoveEntity(geVec3d *v)
{
	assert (v != NULL);

	if(mCurrentEntity < 0) //template
	{
		mRegularEntity.Move (v);
	}
	else
	{
		CEntityArray *Entities = Level_GetEntities (pLevel);

		(*Entities)[mCurrentEntity].Move(v);
		SetModifiedFlag();
	}
}

static geBoolean fdocBrushNotDetail (const Brush *b)
{
	return !Brush_IsDetail (b);
}

static geBoolean fdocBrushIsSubtract (const Brush *b)
{
	return (Brush_IsSubtract (b) && !Brush_IsHollowCut (b));
}

CEntity *CFusionDoc::FindCameraEntity (void)
{
	CEntityArray *Entities = Level_GetEntities (pLevel);
	int i;

	for (i = 0; i < Entities->GetSize (); ++i)
	{
		CEntity *pEnt;

		pEnt = &(*Entities)[i];
		if (pEnt->IsCamera ())
		{
			return pEnt;
		}
	}
	return NULL;
}

//need to put this somewhere
#define	VectorToSUB(a, b)			(*((((geFloat *)(&a))) + (b)))

void CFusionDoc::RenderOrthoView(ViewVars *v, CDC *pDC)
{
	int				inidx, i;
	GroupIterator	gi;
	int				GroupId;
	BrushDrawData	brushDrawData ;
	BrushList		*BList;

	if (pLevel == NULL)
	{
		// must not have been loaded.  Probably an assert popping up....
		return;
	}

	/*
	  We could declare these pens so that they have limited scope, but
	  then we'd have to remember to select oldpen back into the DC before
	  exiting the scope.  Otherwise, if the pen remains selected then
	  Windows won't free the resource.  Bad things ensue.
	*/
	CPen	PenAllItems (PS_SOLID, 1, RGB(255,255,255));
	CPen	PenCutBrush (PS_SOLID, 1, RGB(255,155,0));
	CPen	PenDetailBrush (PS_DASH, 1, RGB(255, 255, 255));
	CPen	PenSelected (PS_SOLID, 1, RGB(0,255,255));
	CPen	PenTemplate (PS_SOLID, 1, RGB(0,0,255));
	CPen	PenHintBrush (PS_SOLID, 1, RGB(0, 100, 0));
	CPen	PenClipBrush (PS_SOLID, 1, RGB(128, 0, 128));
	CPen	PenSheetFaces (PS_SOLID, 1, RGB(255, 255, 0));
	CPen	PenSelectedFaces (PS_SOLID, 1, RGB(255,0,255));

	geVec3d		XTemp;
	Box3d ViewBox;
	int GroupVis = Level_GetGroupVisibility (pLevel);

	BList = Level_GetBrushes (pLevel);
	inidx	=Render_GetInidx(v);
	
	Box3d_SetBogusBounds (&ViewBox);

	Render_ViewToWorld(v, 0, 0, &XTemp);
	Box3d_AddPoint (&ViewBox, XTemp.X, XTemp.Y, XTemp.Z);

	Render_ViewToWorld(v, Render_GetWidth(v), Render_GetHeight(v), &XTemp);
	Box3d_AddPoint (&ViewBox, XTemp.X, XTemp.Y, XTemp.Z);

	VectorToSUB(ViewBox.Min, inidx)	=-FLT_MAX;
	VectorToSUB(ViewBox.Max, inidx)	=FLT_MAX;

	brushDrawData.pViewBox = &ViewBox ;
	brushDrawData.pDC = pDC ;
	brushDrawData.v = v ;
	brushDrawData.pDoc = this;
	brushDrawData.GroupId = 0;
	brushDrawData.FlagTest = NULL;

	// Initialize oldpen.  Don't initialize it anywhere else!
	CPen BlackPen (PS_SOLID, 1, RGB (0, 0, 0));
	CPen * const oldpen = pDC->SelectObject (&BlackPen);

	{
		float GridSize, GridSnapSize;
		const Prefs *pPrefs = GetPrefs ();

		CPen	PenGrid (PS_SOLID, 1, Prefs_GetGridColor (pPrefs));
		CPen	PenSnapGrid (PS_SOLID, 1, Prefs_GetSnapGridColor (pPrefs));


		GridSize = Render_GetFineGrid(v, (Level_GetGridType (pLevel) == GridTexel) ? GRID_TYPE_TEXEL : GRID_TYPE_METRIC);
		if (Level_GetGridType (pLevel) == GridMetric)
		{
			GridSize /= 2.54f;
		}

		GridSnapSize = Level_GetGridSnapSize (pLevel);

		/*
		  If the grid size and the snap size are the same, then just render
		  the snap grid.
		  Otherwise we always want to render the regular grid.  If the
		  snap grid is larger than the regular grid, then render it, too.
		*/
		if (GridSize == GridSnapSize)
		{
			pDC->SelectObject (&PenSnapGrid);
			Render_RenderOrthoGridFromSize (v, GridSize, pDC->m_hDC);
		}
		else
		{
			pDC->SelectObject (&PenGrid);
			Render_RenderOrthoGridFromSize (v, GridSize, pDC->m_hDC);
			if (GridSnapSize > GridSize)
			{
				// render snap grid
				pDC->SelectObject (&PenSnapGrid);
				Render_RenderOrthoGridFromSize (v, GridSnapSize, pDC->m_hDC);
			}
		}
		pDC->SelectObject (oldpen);
	}

	// Step through groups, show items by color
	// All - Show all regardless of visiblity, local invisible attributes still over-ride
	// Vis - Show only groups marked visible, local invisible attributes still over-ride
	// Cur - Show current regardless of visible, local invisible attributes still over-ride

	GroupListType *Groups = Level_GetGroups (pLevel);

	GroupId	=Group_GetFirstId(Groups, &gi);
	while(GroupId != NO_MORE_GROUPS )
	{
	    brushDrawData.FlagTest = ::fdocBrushNotDetail;
		brushDrawData.GroupId = GroupId;
		if( (GroupVis == Group_ShowAll) ||
			((GroupVis == Group_ShowCurrent) && (GroupId == mCurrentGroup)) ||
			((GroupVis == Group_ShowVisible) && (Group_IsVisible (Groups, GroupId)))
		)
		{
			const GE_RGBA * pRGB ;
			pRGB = Group_GetColor( Groups, GroupId ) ;
			CPen	PenThisGroup(PS_SOLID, 1, RGB(pRGB->r,pRGB->g,pRGB->b));

			pDC->SelectObject (&PenThisGroup);
			Level_EnumLeafBrushes (pLevel, &brushDrawData, ::BrushDraw);
			if( mShowEntities == GE_TRUE )
			{
				Level_EnumEntities (pLevel, &brushDrawData, ::EntityDraw);
			}

			// render cut brushes
			pDC->SelectObject (&PenCutBrush);
			brushDrawData.FlagTest = fdocBrushIsSubtract;
			Level_EnumLeafBrushes (pLevel, &brushDrawData, ::BrushDraw);

			// details
			if (bShowDetailBrushes)
			{
				// detail brushes
				pDC->SelectObject (&PenDetailBrush);
				brushDrawData.FlagTest = Brush_IsDetail;
				Level_EnumLeafBrushes (pLevel, &brushDrawData, ::BrushDraw);
			}

			// hints
			if(bShowHintBrushes)
			{
				// Hint brushes
				pDC->SelectObject (&PenHintBrush);
				brushDrawData.FlagTest = Brush_IsHint;
				Level_EnumLeafBrushes (pLevel, &brushDrawData, ::BrushDraw);
			}


			// clip
			if(bShowClipBrushes)
			{
				// Hint brushes
				pDC->SelectObject (&PenClipBrush);
				brushDrawData.FlagTest = Brush_IsClip;
				Level_EnumLeafBrushes (pLevel, &brushDrawData, ::BrushDraw);
			}

			pDC->SelectObject (&PenAllItems);
		}
		GroupId	=Group_GetNextId(Groups, &gi);	
	}

	// find and render the camera entity
	CEntity *pCameraEntity = FindCameraEntity ();
	if ((pCameraEntity != NULL) && pCameraEntity->IsVisible ())
	{
		pDC->SelectObject (&PenAllItems);
		fdocDrawEntity (pCameraEntity, v, pDC, Level_GetEntityDefs (pLevel), GE_FALSE);
	}

	brushDrawData.GroupId = fdoc_SHOW_ALL_GROUPS;
	brushDrawData.FlagTest = NULL;
	{
		CEntityArray *Entities = Level_GetEntities (pLevel);
		int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
				
		// render selected brushes and entities
		pDC->SelectObject(&PenSelected);

		for(i=0;i < NumSelBrushes;i++)
		{
			Brush *pBrush;

			pBrush = SelBrushList_GetBrush (pSelBrushes, i);
			if (fdocShowBrush (pBrush, &ViewBox))
			{
				if(Brush_IsMulti (pBrush))
				{
					BrushList_EnumLeafBrushes(Brush_GetBrushList (pBrush), &brushDrawData, ::BrushDraw);
				}
				else
				{
					Render_RenderBrushFacesOrtho(v, pBrush, pDC->m_hDC);
				}
			}
		}


		for(i=0;i < Entities->GetSize();i++)
		{
			CEntity *pEnt;

			pEnt = &(*Entities)[i];

			if (pEnt->IsSelected ())
			{
				fdocDrawEntity (pEnt, v, pDC, Level_GetEntityDefs (pLevel), (i==mCurrentEntity) ? GE_TRUE : GE_FALSE ) ;
			}
		}
	}

	{
		// render sheet faces
		pDC->SelectObject (&PenSheetFaces);
		BrushList_EnumLeafBrushes(BList, &brushDrawData, ::BrushDrawSheetFacesOrtho);
	}
	{
		// render selected faces
		pDC->SelectObject (&PenSelectedFaces);
		BrushList_EnumLeafBrushes(BList, &brushDrawData, ::BrushDrawSelFacesOrtho);
	}
	{
		// template brush/entity
		pDC->SelectObject (&PenTemplate);

		if((mModeTool==ID_TOOLS_TEMPLATE)||
			(mModeTool==ID_TOOLS_CAMERA && GetSelState()==NOSELECTIONS))
		{
			if(!TempEnt)
			{
				if (Brush_TestBoundsIntersect(CurBrush, &ViewBox))
				{
					if(Brush_IsMulti(CurBrush))
					{
						BrushList_EnumLeafBrushes(Brush_GetBrushList(CurBrush), &brushDrawData, ::BrushDraw);
					}
					else
					{
						Render_RenderBrushFacesOrtho(v, CurBrush, pDC->m_hDC);
					}
				}
			}
			else
			{
				fdocDrawEntity (&mRegularEntity, v, pDC, Level_GetEntityDefs (pLevel), GE_FALSE );
			}
		}
	}

	pDC->SelectObject(oldpen);
}/* CFusionDoc::RenderOrthoView */

void CFusionDoc::OnUpdateBrushAdjustmentmode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( TRUE); //( mModeTool == ID_GENERALSELECT ) ? TRUE : FALSE ) ;
	pCmdUI->SetCheck( ( mAdjustMode == ADJUST_MODE_BRUSH ) ? TRUE : FALSE ) ;
}

void CFusionDoc::OnToolsBrushAdjustmentmode() 
{
	if( /*mModeTool == ID_GENERALSELECT && 
		!IsSelectionLocked() && */
		mCurrentTool == CURTOOL_NONE )
	{
		SetAdjustmentMode( ADJUST_MODE_BRUSH ) ;
	}
}

void CFusionDoc::OnUpdateFaceAdjustmentmode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( TRUE); //( mModeTool == ID_GENERALSELECT ) ? TRUE : FALSE ) ;
	pCmdUI->SetCheck( (mAdjustMode==ADJUST_MODE_FACE) ? TRUE : FALSE ) ;
}

void CFusionDoc::OnToolsFaceAdjustmentmode() 
{
	if( /*mModeTool == ID_GENERALSELECT && 
		!IsSelectionLocked() && */
		mCurrentTool == CURTOOL_NONE )
	{
		SetAdjustmentMode( ADJUST_MODE_FACE ) ;
	}
}

void CFusionDoc::OnToolsToggleadjustmode() 
{
	if( /*mModeTool == ID_GENERALSELECT && 
		!IsSelectionLocked() && */
		mCurrentTool == CURTOOL_NONE )
	{
		SetAdjustmentMode( ADJUST_MODE_TOGGLE ) ;	// Flip between Brush & face 
	}
	
}

void CFusionDoc::OnUpdateToolsToggleadjustmode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable (TRUE);//(mModeTool == ID_GENERALSELECT) ? TRUE : FALSE);
}


static geFloat PointToLineDist
	(
	  POINT const *ptFrom,
	  POINT const *ptLine1,
	  POINT const *ptLine2
	)
{
	geFloat xkj, ykj;
	geFloat xlk, ylk;
	geFloat denom;
	geFloat dist;

	xkj = (geFloat)(ptLine1->x - ptFrom->x);
	ykj = (geFloat)(ptLine1->y - ptFrom->y);
	xlk = (geFloat)(ptLine2->x - ptLine1->x);
	ylk = (geFloat)(ptLine2->y - ptLine1->y);
	denom = (xlk*xlk) + (ylk*ylk);
	if (denom < .0005f)
	{
		// segment ends coincide
		dist = xkj*xkj + ykj*ykj;
	}
	else
	{
		geFloat t;
		geFloat xfac, yfac;

		t = -(xkj*xlk + ykj*ylk)/denom;
		t = max (t, 0.0f);
		t = min (t, 1.0f);
		xfac = xkj + t*xlk;
		yfac = ykj + t*ylk;
		dist = xfac*xfac + yfac*yfac;
	}
	return (geFloat)sqrt (dist);
}


typedef struct FindClosestInfoTag
{
	CFusionDoc	*pDoc;
	ViewVars	*v;
	Brush		**ppFoundBrush;
	geFloat		*pMinEdgeDist;
	const POINT	*ptFrom;
} FindClosestInfo;

static geBoolean	FindClosestBrushCB(Brush *pBrush, void *pVoid)
{
	FindClosestInfo	*fci	=(FindClosestInfo *)pVoid;

	if(fci->pDoc->BrushIsVisible(pBrush))
	{
		// for each face...
		for (int iFace = 0; iFace < Brush_GetNumFaces(pBrush); ++iFace)
		{
			POINT			pt1, pt2;
			Face			*pFace		=Brush_GetFace(pBrush, iFace);
			const geVec3d	*FacePoints	=Face_GetPoints(pFace);
			int				NumPoints	=Face_GetNumPoints(pFace);

			// Starting with the edge formed by the last point and the first point,
			// determine distance from mouse cursor pos to the edge.
			pt1 = Render_OrthoWorldToView(fci->v, &FacePoints[NumPoints-1]);
			for(int iPoint = 0; iPoint < NumPoints; ++iPoint)
			{
				geFloat Dist;

				pt2 = Render_OrthoWorldToView (fci->v, &FacePoints[iPoint]);
				Dist = PointToLineDist (fci->ptFrom, &pt1, &pt2);
				if (Dist < *fci->pMinEdgeDist)
				{
					*fci->pMinEdgeDist = Dist;
					*fci->ppFoundBrush = pBrush;
				}
				pt1 = pt2;	// next edge...
			}
		}
	}
	return GE_TRUE ;
}

geBoolean CFusionDoc::FindClosestBrush
	(
	  POINT const *ptFrom,
	  ViewVars *v,
	  Brush **ppFoundBrush,
	  geFloat *pMinEdgeDist
	)
{
	// determine the distance to the closest brush edge in the current view.
	FindClosestInfo	fci;

	*pMinEdgeDist = FLT_MAX;
	*ppFoundBrush = NULL;

	fci.pDoc			=this;
	fci.v				=v;
	fci.ppFoundBrush	=ppFoundBrush;
	fci.pMinEdgeDist	=pMinEdgeDist;
	fci.ptFrom			=ptFrom;

	BrushList_EnumLeafBrushes(Level_GetBrushes (pLevel), &fci, ::FindClosestBrushCB);

	return	(*ppFoundBrush)? GE_TRUE : GE_FALSE;
}

geBoolean CFusionDoc::FindClosestEntity
	(
	  POINT const *ptFrom,
	  ViewVars *v,
	  CEntity **ppMinEntity,
	  geFloat *pMinEntityDist
	)
{
	geBoolean rslt;
	CEntityArray *Entities;

	Entities = Level_GetEntities (pLevel);
	rslt = GE_FALSE;
	// determine distance to closest entity in the current view
	*pMinEntityDist = FLT_MAX;
	*ppMinEntity = NULL;
	for (int i = 0; i < Entities->GetSize(); ++i)
	{
		CEntity *pEnt;
		POINT EntPosView;
		geFloat Dist;
		int dx, dy;

		pEnt = &(*Entities)[i];
		if (EntityIsVisible( pEnt ))
		{
			EntPosView = Render_OrthoWorldToView (v, &pEnt->mOrigin);
			dx = EntPosView.x - ptFrom->x;
			dy = EntPosView.y - ptFrom->y;

			Dist = (geFloat)((dx*dx) + (dy*dy));
			if (Dist < *pMinEntityDist)
			{
				*pMinEntityDist = Dist;
				*ppMinEntity = pEnt;
				rslt = GE_TRUE;
			}
		}
	}

	if (rslt)
	{
		*pMinEntityDist = (geFloat)sqrt (*pMinEntityDist);
	}
	return rslt;
}

int CFusionDoc::FindClosestThing
	(
	  POINT const *ptFrom,
	  ViewVars *v,
	  Brush **ppMinBrush,		// NULL OK
	  CEntity **ppMinEntity,	// NULL OK
	  geFloat *pDist
	)
{
	int rslt;

	geBoolean FoundBrush;
	geFloat MinEdgeDist;
	Brush *pMinBrush;
		
	geBoolean FoundEntity;
	geFloat MinEntityDist;
	CEntity *pMinEntity;

	rslt = fctNOTHING;
	FoundBrush = FindClosestBrush (ptFrom, v, &pMinBrush, &MinEdgeDist);
	FoundEntity = FindClosestEntity (ptFrom, v, &pMinEntity, &MinEntityDist);


	if (FoundEntity)
	{
		if ((!FoundBrush) || (MinEntityDist < MinEdgeDist))
		{
			*pDist = MinEntityDist;
			if( ppMinEntity != NULL )
				*ppMinEntity = pMinEntity;
			rslt = fctENTITY;
		}
		else
		{
			*pDist = MinEdgeDist;
			if( ppMinBrush != NULL )
				*ppMinBrush = pMinBrush;
			rslt = fctBRUSH;
		}
	}
	else if (FoundBrush)
	{
		*pDist = MinEdgeDist;
		if( ppMinBrush != NULL )
			*ppMinBrush = pMinBrush;
		rslt = fctBRUSH;
	}
	return rslt;
}

void CFusionDoc::DoBrushSelection
	(
		Brush	*	pBrush,
		BrushSel	nSelType //	brushSelToggle | brushSelAlways
	)
{
	int ModelId = 0;
	geBoolean ModelLocked;
	ModelInfo_Type *ModelInfo;
	GroupListType *Groups;
	int GroupId = 0;
	geBoolean GroupLocked;
	BrushList *BList;
	Brush	*pBParent=NULL;

	ModelInfo = Level_GetModelInfo (pLevel);
	Groups = Level_GetGroups (pLevel);
	BList = Level_GetBrushes (pLevel);

	if(Brush_GetParent(BList, pBrush, &pBParent))
	{
		pBrush	=pBParent;
	}

	ModelLocked = GE_FALSE;
	GroupLocked = FALSE;
	if(mAdjustMode != ADJUST_MODE_FACE)
	{
		// don't do this stuff if we're in face mode...
		ModelId = Brush_GetModelId (pBrush);
		if (ModelId != 0)
		{
			Model *pModel;

			pModel = ModelList_FindById (ModelInfo->Models, ModelId);
			if (pModel != NULL)
			{
				ModelLocked = Model_IsLocked (pModel);
			}
		}

		if (!ModelLocked)
		{
			GroupId = Brush_GetGroupId (pBrush);
			if (GroupId != 0)
			{
				GroupLocked = Group_IsLocked (Groups, GroupId);
			}
		}
	}

	if( nSelType == brushSelToggle && BrushIsSelected (pBrush) )
	{
		if (ModelLocked)
		{
			// model is locked, so deselect everything in the model
			SelectModelBrushes (FALSE, ModelId);
		}
		else if (GroupLocked)
		{
			// group is locked, so deselect entire group
			SelectGroupBrushes (FALSE, GroupId);
		}
		else
		{
			SelBrushList_Remove (pSelBrushes, pBrush);
		}
	}
	else
	{
		if (ModelLocked)
		{
			// model is locked, so select everything in the model
			SelectModelBrushes (TRUE, ModelId);
		}
		else if (GroupLocked)
		{
			// group is locked.  Select everything in the group
			SelectGroupBrushes (TRUE, GroupId);
		}
		else
		{
			SelBrushList_Add (pSelBrushes, pBrush);
		}
	}
}/* CFusionDoc::DoBrushSelection */

void CFusionDoc::SelectEntity
	(
	  CEntity *pEntity
	)
{
	assert (pEntity != NULL);

	if (!pEntity->IsSelected ())
	{
		pEntity->Select ();
		++NumSelEntities;
	}
}

void CFusionDoc::DeselectEntity
	(
	  CEntity *pEntity
	)
{
	assert (pEntity != NULL);

	if (pEntity->IsSelected ())
	{
		pEntity->DeSelect ();
		--NumSelEntities;
		assert (NumSelEntities >= 0);
	}
}

void CFusionDoc::DoEntitySelection
	(
	  CEntity *pEntity
	)
{
	// an entity is closest.  Select/deselect it.
	int GroupId;
	geBoolean GroupLocked;
	GroupListType *Groups = Level_GetGroups (pLevel);

	assert (pEntity != NULL);

	GroupLocked = FALSE;
	GroupId = pEntity->GetGroupId ();
	if (GroupId != 0)
	{
		GroupLocked = Group_IsLocked (Groups, GroupId);
	}


	if (pEntity->IsSelected ())
	{
		if (GroupLocked)
		{
			// deselect entire group
			SelectGroupBrushes (FALSE, GroupId);
		}
		else
		{
			DeselectEntity (pEntity);
		}
	}
	else
	{
		if (GroupLocked)
		{
			// select entire group
			SelectGroupBrushes (TRUE, GroupId);
		}
		else
		{
			SelectEntity (pEntity);
		}
	}
}

static geBoolean fdocDeselectEntity (CEntity &Ent, void *lParam)
{
	CFusionDoc *pDoc = (CFusionDoc *)lParam;

	pDoc->DeselectEntity (&Ent);
	return GE_TRUE;
}

void CFusionDoc::ResetAllSelectedEntities()
{
	DoGeneralSelect ();

	Level_EnumEntities (pLevel, this, ::fdocDeselectEntity);
}


static geBoolean fdocSelectEntity (CEntity &Ent, void *lParam)
{
	CFusionDoc *pDoc = (CFusionDoc *)lParam;

	Ent.DeSelect ();
	pDoc->SelectEntity (&Ent);
	return GE_TRUE;
}

static geBoolean fdocSelectBrush (Brush *pBrush, void *lParam)
{
	CFusionDoc *pDoc = (CFusionDoc *)lParam;

	SelBrushList_Add (pDoc->pSelBrushes, pBrush);
	return GE_TRUE;
}

void CFusionDoc::SelectAll (void)
{
	DoGeneralSelect ();

	NumSelEntities = 0;
	Level_EnumEntities (pLevel, this, ::fdocSelectEntity);
	Level_EnumBrushes (pLevel, this, ::fdocSelectBrush);	

	UpdateSelected();
}

BOOL CFusionDoc::IsEntitySelected(void)
{
	CEntityArray *Entities;

	Entities = Level_GetEntities (pLevel);

	for( int Ent = 0; Ent < Entities->GetSize(); Ent++ )
	{
		if ((*Entities)[ Ent ].IsSelected ())
		{
			return TRUE;
		}
	}

	return FALSE;
}

//	CHANGE!	04/16/97	John Moore
//	Sets the entity with the designated ID as the current entity...
void CFusionDoc::SetSelectedEntity( int ID )
{
	CEntityArray *Entities;

	Entities = Level_GetEntities (pLevel);
	ResetAllSelectedEntities();
	mCurrentEntity = ID;
	SelectEntity (&(*Entities)[ID]);
	UpdateSelected() ;
}
//	End of CHANGE

void CFusionDoc::DeleteEntity(int EntityIndex)
{
	CEntityArray *Entities;

	// now delete the entity, we'll do the fixups later
	Entities = Level_GetEntities (pLevel);

	DeselectEntity (&(*Entities)[EntityIndex]);
	Entities->RemoveAt( EntityIndex );
	SelState&=(~ENTITYCLEAR);
	SelState|=(NumSelEntities >1)? MULTIENTITY : (NumSelEntities+1)<<7;
}

void CFusionDoc::AdjustEntityAngle( const ViewVars * v, const geFloat dx )
{
	CEntity *	pEnt ;
	geVec3d		Vec ;
	geVec3d		Angles ;
	CEntityArray *Entities;

	Entities = Level_GetEntities (pLevel);

	pEnt = &(*Entities)[mCurrentEntity];

	
	if (pEnt->IsCamera ())
	{
		Render_ViewDeltaToRotation ( v, -dx, &Vec);
		// disallow roll for camera
		Vec.Z = 0.0f;
		// if the camera is upside down, then negate yaw
		if (Render_UpIsDown (v))
		{
			Vec.Y = -Vec.Y;
		}
	}
	else
	{
		Render_ViewDeltaToRotation ( v, dx, &Vec);
	}

	pEnt->GetAngles( &Angles, Level_GetEntityDefs (pLevel) ) ;
	geVec3d_Add (&Angles, &Vec, &Angles);
	pEnt->SetAngles( &Angles, Level_GetEntityDefs (pLevel) ) ;
}

void CFusionDoc::AdjustEntityArc( const ViewVars * v, const geFloat dx )
{
	CEntity *	pEnt ;
	geFloat		fArc ;
	geFloat		fArcDelta ;
	CEntityArray *Entities = Level_GetEntities (pLevel);

	pEnt = &(*Entities)[mCurrentEntity];

	pEnt->GetArc( &fArc, Level_GetEntityDefs (pLevel) ) ;
	
	fArcDelta = Render_ViewDeltaToRadians( v, dx ) ;
	fArc -= fArcDelta;
	if (fArc > 2*M_PI)
	{
		fArc -= 2*M_PI;
	}
	if (fArc < 0)
	{
		fArc += 2*M_PI;
	}

	pEnt->SetArc( fArc, Level_GetEntityDefs (pLevel) ) ;
}

void  CFusionDoc::AdjustEntityRadius( const geVec3d *pVec )
{
	CEntity *	pEnt ;
	geFloat		fRadius ;
	CEntityArray *Entities = Level_GetEntities (pLevel);

	pEnt = &(*Entities)[mCurrentEntity];

	pEnt->GetRadius( &fRadius, Level_GetEntityDefs (pLevel) ) ;
	fRadius += geVec3d_Length (pVec);
	if( fRadius < 0.0f )
	{
		fRadius = 0.0f ;
	}
	pEnt->SetRadius( fRadius, Level_GetEntityDefs (pLevel) ) ;
}


void CFusionDoc::SelectOrtho(CPoint point, ViewVars *v)
{
	Brush *pMinBrush;
	CEntity *pMinEntity;
	geFloat Dist;
	int FoundThingType;

	if(IsSelectionLocked())
	{
		return;
	}

	// if Control key isn't pressed, then clear all current selections
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0)
	{
		ResetAllSelections ();
	}

	FoundThingType = FindClosestThing (&point, v, &pMinBrush, &pMinEntity, &Dist);
	if ((FoundThingType != fctNOTHING) && (Dist <= MAX_PIXEL_SELECT_DIST))
	{
		switch (FoundThingType)
		{
			case fctBRUSH :
				DoBrushSelection (pMinBrush, brushSelToggle);
				UpdateBrushAttributesDlg ();
				break;
			case fctENTITY :
				DoEntitySelection (pMinEntity);
				break;
			default :
				// bad value returned from FindClosestThing
				assert (0);
		}
	}
	if (SelBrushList_GetSize (pSelBrushes) == 0)
	{
		DeleteBrushAttributes ();
	}

	UpdateSelected ();
}


void CFusionDoc::SelectOrthoRect(CPoint ptStart, CPoint ptEnd, ViewVars *v)
{
	Brush		*	pBrush;
	CEntity		*	pEnt;
	int				i ;
	BrushIterator	bi;
	POINT			Min ;
	POINT			Max ;
	POINT			EntPosView;
	geBoolean		bSelectedSomething = GE_FALSE ;
	CRect			viewRect( ptStart, ptEnd ) ;		//Selection Rect in view coords
	BrushList	*	BList = Level_GetBrushes (pLevel);

	viewRect.NormalizeRect() ;

	if(IsSelectionLocked())
	{
		return;
	}

	// if Control key isn't pressed, then clear all current selections
	if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0)
	{
		ResetAllSelections ();
	}

	// Should use ENUM function here
	pBrush = BrushList_GetFirst (BList, &bi);
	while (pBrush != NULL)
	{
		if (BrushIsVisible (pBrush))
		{
			const Box3d	*bbox	=Brush_GetBoundingBox(pBrush);
			Min = Render_OrthoWorldToView ( v, &bbox->Min ); // Get Brush BB in View Coords
			Max = Render_OrthoWorldToView ( v, &bbox->Max );
			if( viewRect.PtInRect( Min ) && viewRect.PtInRect( Max ) )		// If Brush ENTIRELY in rect...
			{			
				DoBrushSelection( pBrush, brushSelAlways ) ;
				bSelectedSomething = GE_TRUE ;
			}
		}
		pBrush = BrushList_GetNext (&bi);
	}

	CEntityArray *Entities = Level_GetEntities (pLevel);

	for( i = 0; i < Entities->GetSize(); ++i)
	{

		pEnt = &(*Entities)[i];
		if (EntityIsVisible( pEnt) )
		{
			EntPosView = Render_OrthoWorldToView (v, &pEnt->mOrigin);
			if( pEnt->IsSelected () == GE_FALSE )
			{
				if( viewRect.PtInRect( EntPosView ) )
				{
					DoEntitySelection( pEnt );
					bSelectedSomething = GE_TRUE ;
				}
			}
		}
	}

	if( bSelectedSomething )
		UpdateSelected ();

}/* CFusionDoc::SelectOrthoRect */

void CFusionDoc::ResizeSelected(float dx, float dy, int sides, int inidx)
{
	mLastOp				=BRUSH_SCALE;

	if(mModeTool == ID_TOOLS_TEMPLATE)
	{
		Brush_Resize(CurBrush, dx, dy, sides, inidx, &FinalScale, &ScaleNum);
		if(Brush_IsMulti(CurBrush))
		{
			BrushList_ClearCSGAndHollows((BrushList *)Brush_GetBrushList(CurBrush), Brush_GetModelId(CurBrush));
			BrushList_RebuildHollowFaces((BrushList *)Brush_GetBrushList(CurBrush), Brush_GetModelId(CurBrush), ::fdocBrushCSGCallback, this);
		}
	}
	else
	{
		int i;
		int NumBrushes;

		NumBrushes = SelBrushList_GetSize (pTempSelBrushes);

		for (i = 0; i < NumBrushes; ++i)
		{
			Brush *pBrush;
			
			pBrush = SelBrushList_GetBrush (pTempSelBrushes, i);

			Brush_Resize (pBrush, dx, dy, sides, inidx, &FinalScale, &ScaleNum);
			if (Brush_IsMulti(pBrush))
			{
				BrushList_ClearCSGAndHollows((BrushList *)Brush_GetBrushList(pBrush), Brush_GetModelId(pBrush));
				BrushList_RebuildHollowFaces((BrushList *)Brush_GetBrushList(pBrush), Brush_GetModelId(pBrush), ::fdocBrushCSGCallback, this);
			}
		}
	}
}

typedef struct
{
	geVec3d	vp, wp;
	geFloat	MinBDist, CurDist;
	Face	*HitFace, *CurFace;
	Brush	*CurBrush;
	CFusionDoc *pDoc;
} SelectBrush3DCBData;

static geBoolean SelectBrush3DCB(Brush *b, void * lParam)
{
	SelectBrush3DCBData	*pData	=(SelectBrush3DCBData *)lParam;

	if (pData->pDoc->BrushIsVisible (b))
	{
		if(!(Brush_IsSubtract(b)))
		{
			Face *HitFace;
			float CurDist;

			HitFace	=Brush_RayCast(b, &pData->vp, &pData->wp, &CurDist);
			if (HitFace != NULL)
			{
				pData->HitFace = HitFace;
				if(CurDist < pData->MinBDist)
				{
					pData->CurDist = CurDist;
					pData->MinBDist	=pData->CurDist;
					pData->CurBrush	=b;
					pData->CurFace	=pData->HitFace;
				}
			}
		}
	}
	return	GE_TRUE;
}

void CFusionDoc::PlaceTemplateEntity3D(CPoint point, ViewVars *v)
{
	BrushList	*BList = Level_GetBrushes (pLevel);
	SelectBrush3DCBData	bdat;
	geVec3d ClickPosWorld;
	const Plane	*p;

	Render_ViewToWorld(v, point.x, point.y, &bdat.vp);
	Render_BackRotateVector(v, &bdat.vp, &bdat.wp);
	Render_GetCameraPos(v, &bdat.vp);

	bdat.MinBDist	=999999.0f;
	bdat.CurBrush	=NULL;
	bdat.pDoc		= this;
	BrushList_EnumCSGBrushes(BList, &bdat, SelectBrush3DCB);
	
	if(bdat.CurBrush)
	{
		if(bdat.CurFace)
		{
			geFloat	pDist;

			//find the dist ratio from click ray to plane normal
			p		=Face_GetPlane(bdat.CurFace);
			pDist	=geVec3d_DotProduct(&bdat.wp, &p->Normal);
			if(pDist != 0.0f)
			{
				//grab plane distance and move inward by 16
				pDist	=(Face_PlaneDistance(bdat.CurFace, &bdat.vp) - 16.0f) / pDist;
			}

			geVec3d_Scale(&bdat.wp, pDist, &ClickPosWorld);

			//add in distances from the camera, and current entity position
			geVec3d_Subtract(&ClickPosWorld, &bdat.vp, &ClickPosWorld);

			geVec3d_Inverse (&ClickPosWorld);
			mRegularEntity.mOrigin = ClickPosWorld;
			UpdateAllViews( UAV_ALL3DVIEWS, NULL );
		}
	}
}

struct FaceSearchCallbackData
{
	const Plane *pFacePlane;
	geBoolean Found;
	geVec3d ImpactPoint;
	Face *pFoundFace;
	Brush *pFoundBrush;
};
static geBoolean fdocPointOnFace 
	(
	  const Face *pFace,
	  const geVec3d *pPoint
	)
{
	int pt1, pt2;
	int iVert, NumVerts;
	geVec3d const *Verts;

	NumVerts = Face_GetNumPoints (pFace);
	Verts = Face_GetPoints (pFace);
	pt1 = NumVerts-1;
	pt2 = 0;
	for (iVert = 0; iVert < NumVerts; ++iVert)
	{
		geFloat Dot;
		geVec3d VertToPoint, VertToVert;

		geVec3d_Subtract (&Verts[pt2], &Verts[pt1], &VertToVert);
		geVec3d_Subtract (pPoint, &Verts[pt1], &VertToPoint);

		Dot = geVec3d_DotProduct (&VertToVert, &VertToPoint);
		if (Dot < 0)
		{
			return GE_FALSE;
		}
		pt1 = pt2;
		++pt2;
	}
	return GE_TRUE;
}

static geBoolean fdocPointInBrushFace
	(
	  Brush *pBrush,
	  const Plane *pPlane,
	  const geVec3d *pPoint,
	  Face **ppFoundFace
	)
{
	int NumFaces;
	int i;

	NumFaces = Brush_GetNumFaces (pBrush);
	for (i = 0; i < NumFaces; ++i)
	{
		Face *pFace;
		const Plane *pFacePlane;

		pFace = Brush_GetFace (pBrush, i);
		pFacePlane = Face_GetPlane (pFace);

		if(geVec3d_Compare(&pPlane->Normal, &pFacePlane->Normal, 0.01f) &&
		   (fabs (pPlane->Dist - pFacePlane->Dist) < 0.01f))
		{
			// if the face plane matches the passed plane
			// then see if the ImpactPoint is within the bounds of the face
			if (::fdocPointOnFace (pFace, pPoint))
			{
				*ppFoundFace = pFace;
				return GE_TRUE;
			}
		}
	}
	return GE_FALSE;
}

static geBoolean fdocFindLeafFace (Brush *pBrush, void *lParam)
{
	if (!Brush_IsSubtract (pBrush))		// don't want cut brushes
	{
		FaceSearchCallbackData *pData;

		pData = (FaceSearchCallbackData *)lParam;
		if (::fdocPointInBrushFace (pBrush, pData->pFacePlane, &(pData->ImpactPoint), &(pData->pFoundFace)))
		{
			pData->pFoundBrush = pBrush;
			pData->Found = GE_TRUE;
			return GE_FALSE;	// found it, so quit...
		}
	}
	return GE_TRUE;
}

static geBoolean fdocFindCutFace (Brush *pBrush, void *lParam)
{
	if (Brush_IsSubtract (pBrush))		// want only cut brushes
	{
		FaceSearchCallbackData *pData;
		Plane PlaneInv;

		pData = (FaceSearchCallbackData *)lParam;
#pragma message ("Need to reverse plane?")
		PlaneInv = *pData->pFacePlane;
		geVec3d_Inverse (&PlaneInv.Normal);
		PlaneInv.Dist = -PlaneInv.Dist;

		pData = (FaceSearchCallbackData *)lParam;
		if (::fdocPointInBrushFace (pBrush, &PlaneInv, &(pData->ImpactPoint), &(pData->pFoundFace)))
		{
			pData->pFoundBrush = pBrush;
			pData->Found = GE_TRUE;
			return GE_FALSE;	// found it, so quit...
		}
	}
	return GE_TRUE;
}

//slow version checking brushes
void CFusionDoc::SelectRay(CPoint point, ViewVars *v)
{
	int			CurEnt = 0;
	geFloat		MinEDist;
	CEntityArray *Entities = Level_GetEntities (pLevel);
	BrushList	*BList = Level_GetBrushes (pLevel);
	SelectBrush3DCBData	bdat;
	geVec3d ClickPosWorld;
	geBoolean	EntitySelected	=FALSE;

	Render_ViewToWorld(v, point.x, point.y, &bdat.vp);
	Render_BackRotateVector(v, &bdat.vp, &bdat.wp);
	Render_GetCameraPos(v, &bdat.vp);

	Render_ViewToWorld (v, point.x, point.y, &ClickPosWorld);

	MinEDist = bdat.MinBDist = 999999.0f;

	for(int i=0;i < Entities->GetSize();i++)
	{
		CEntity *pEnt;

		pEnt = &(*Entities)[i];
		if (EntityIsVisible (pEnt))
		{
			bdat.CurDist=pEnt->RayDistance (point, v);
			if ((bdat.CurDist < 900.0f)
				&& (bdat.CurDist < MinEDist)
				&& (bdat.CurDist > MIN_ENTITY_SELECT_DIST))
			{
				MinEDist=bdat.CurDist;
				CurEnt	=i;
			}
		}
	}
	bdat.CurBrush	= NULL;
	bdat.CurFace	= NULL;
	bdat.pDoc		= this;
	BrushList_EnumCSGBrushes(BList, &bdat, SelectBrush3DCB);
		
	if((bdat.MinBDist < 999999.0f) && (MinEDist > MIN_ENTITY_SELECT_DIST))
	{
		//check the distance of the hit wall
		//see if the closest entity is occluded
		if(bdat.CurBrush)
		{
			if(bdat.CurFace)
			{
				geFloat		pDist;
				const Plane	*p;

				//find the dist ratio from click ray to plane normal
				p		=Face_GetPlane(bdat.CurFace);
				pDist	=geVec3d_DotProduct(&bdat.wp, &p->Normal);

				if(pDist != 0.0f)
				{
					pDist	=(Face_PlaneDistance(bdat.CurFace, &bdat.vp)  / pDist);
				}
				geVec3d_Scale(&bdat.wp, pDist, &ClickPosWorld);
				pDist	=geVec3d_Length(&ClickPosWorld);

				if(MinEDist < pDist)
				{
					if((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0)
					{
						ResetAllSelections ();
					}
					DoEntitySelection(&(*Entities)[CurEnt]);
					EntitySelected	=GE_TRUE;
				}
			}
		}

	}
	if(!EntitySelected && bdat.CurBrush)
	{
		// if Control key isn't pressed, then clear all current selections
		if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) == 0)
		{
			ResetAllSelections ();
		}
		/*
		  At this point, bdat.CurBrush points to the CSG brush that contains
		  the hit face.  We want the leaf brush that contains the CSG face
		  that was hit.

		  So, we search the entire brush list looking for a leaf face (non-cut)
		  that has a face that's coplanar with the hit face, and that contains
		  the impact point.  If none is found, we then search for a matching 
		  cut brush face (again, in leafs) that contains the point.
		*/
		{
			const Plane *p;
			geFloat pDist;
			geVec3d ClickPosWorld;
			FaceSearchCallbackData fsData;

			if (Brush_GetType (bdat.CurBrush) == BRUSH_LEAF)
			{
				// if the found face is on a leaf brush, then skip the rest of the search
				fsData.pFoundFace = bdat.CurFace;
				fsData.Found = GE_TRUE;
				fsData.pFoundBrush = bdat.CurBrush;
			}
			else
			{
				// determine the impact point
				p		=Face_GetPlane(bdat.CurFace);
				pDist	=geVec3d_DotProduct(&bdat.wp, &p->Normal);
				if(pDist != 0.0f)
				{
					//grab plane distance and move inward by 16
					pDist	=(Face_PlaneDistance(bdat.CurFace, &bdat.vp)) / pDist;
				}

				geVec3d_Scale(&bdat.wp, pDist, &ClickPosWorld);

				//add in distance from the camera
				geVec3d_Subtract(&ClickPosWorld, &bdat.vp, &ClickPosWorld);
				geVec3d_Inverse (&ClickPosWorld);

				// OK, now search list for a face that contains this point and
				// is coplanar with the matched face.
				fsData.pFacePlane = p;
				fsData.Found = GE_FALSE;
				fsData.pFoundFace = NULL;
				fsData.pFoundBrush = NULL;
				fsData.ImpactPoint = ClickPosWorld;

				BrushList_EnumLeafBrushes (BList, &fsData, ::fdocFindLeafFace);
				if (!fsData.Found)
				{
					BrushList_EnumLeafBrushes (BList, &fsData, ::fdocFindCutFace);
				}
			}
			if (fsData.Found)
			{
				// We found the hit face.
				if (mAdjustMode == ADJUST_MODE_BRUSH)
				{
					DoBrushSelection (fsData.pFoundBrush, brushSelToggle);
				}
				else
				{
					DoBrushSelection (fsData.pFoundBrush, brushSelAlways);
					// if the face is already in the list, then remove it
					if (SelFaceList_Remove (pSelFaces, fsData.pFoundFace))
					{
						Face_SetSelected (fsData.pFoundFace, GE_FALSE);
					}
					else
					{
						Face_SetSelected (fsData.pFoundFace, GE_TRUE);
						SelFaceList_Add (pSelFaces, fsData.pFoundFace);
					}
					// Deselect any brush that doesn't have selected faces
					int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
					for (int i = NumSelBrushes-1; i >= 0; --i)
					{
						Brush *pBrush;

						pBrush = SelBrushList_GetBrush (pSelBrushes, i);

						if (Brush_GetSelectedFace (pBrush) == NULL)
						{
							SelBrushList_Remove (pSelBrushes, pBrush);
						}
					}
				}
			}
		}
	}

	UpdateSelected();

	if ((bdat.CurFace == NULL) ||
	    ((mAdjustMode == ADJUST_MODE_FACE) && (SelFaceList_GetSize (pSelFaces) == 0)))
	{
		DeleteFaceAttributes ();
		DeleteBrushAttributes ();
	}
	else
	{
		UpdateFaceAttributesDlg ();
		UpdateBrushAttributesDlg ();
	}
}

//selects the texture of the face clicked (doesn't select the face)
void CFusionDoc::SelectTextureFromFace3D(CPoint point, ViewVars *v)
{
	BrushList	*BList = Level_GetBrushes (pLevel);
	SelectBrush3DCBData	bdat;

	Render_ViewToWorld(v, point.x, point.y, &bdat.vp);
	Render_BackRotateVector(v, &bdat.vp, &bdat.wp);
	Render_GetCameraPos(v, &bdat.vp);

	bdat.MinBDist	=999999.0f;
	bdat.CurBrush	=NULL;
	bdat.pDoc		= this;
	BrushList_EnumCSGBrushes(BList, &bdat, SelectBrush3DCB);
	
	if(bdat.CurBrush)
	{
		if(bdat.CurFace)
		{
			mpMainFrame->m_wndTabControls->SelectTexture(Face_GetTextureDibId(bdat.CurFace));
		}
	}
}

void CFusionDoc::UpdateFaceAttributesDlg (void)
{
	if (mpFaceAttributes != NULL)
	{
		mpFaceAttributes->UpdatePolygonFocus ();
	}
}

void CFusionDoc::UpdateBrushAttributesDlg (void)
{
	if (mpBrushAttributes != NULL)
	{
		mpBrushAttributes->UpdateBrushFocus ();
	}
}

void CFusionDoc::UpdateCameraEntity( const ViewVars *v )
{
	CEntity *	pEnt ;
	geVec3d		Vec ;

	pEnt = EntityList_FindByClassName( Level_GetEntities (pLevel), "Camera" ) ;
	if( pEnt )
	{
		Render_GetCameraPos( v, &Vec ) ;
		pEnt->SetOrigin( Vec.X, Vec.Y, Vec.Z, Level_GetEntityDefs (pLevel) ) ;
		Render_GetPitchRollYaw( v, &Vec ) ;
		pEnt->SetAngles( &Vec, Level_GetEntityDefs (pLevel) ) ;
		UpdateAllViews( UAV_ALL3DVIEWS, NULL );
	}

}/* CFusionDoc::MoveCamera */

void CFusionDoc::SetRenderedViewCamera( const geVec3d * pVec, const geVec3d * pPRY )
{
	POSITION		pos;
	CFusionView	*	pView;

	pos = GetFirstViewPosition();
	while( pos != NULL )
	{
		pView = (CFusionView*)GetNextView(pos) ;
		if( Render_GetViewType( pView->VCam ) & (VIEWSOLID|VIEWTEXTURE|VIEWWIRE) )
		{
			Render_SetPitchRollYaw( pView->VCam, pPRY ) ;
			Render_SetCameraPos( pView->VCam, pVec ) ;
			break ;	// Only 1 rendered view for now
		}
	}
}/* CFusionDoc::SetRenderedViewCamera */

void CFusionDoc::GetCursorInfo(char *info, int MaxSize)
{
	CFusionView	*pView;
	CPoint		CursorPos, ViewCursorPos;
	CRect		ClientRect;
	POSITION	pos;

	pos		=GetFirstViewPosition();
	info[0]	=0;

	GetCursorPos(&CursorPos);

	while(pos!=NULL)
	{
		pView	=(CFusionView*)GetNextView(pos);
		pView->GetClientRect(&ClientRect);
		pView->ClientToScreen(&ClientRect);
		if(ClientRect.PtInRect(CursorPos))
		{
			if(pView->mViewType!=(unsigned)mActiveView)
			{
				if(!pView->IsPanning && mCurrentTool!=ID_TOOLS_BRUSH_SCALEBRUSH
					&& mCurrentTool!=ID_TOOLS_BRUSH_SHEARBRUSH)
				{
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
				}
				return;
			}
			ViewCursorPos=CursorPos;
			pView->ScreenToClient(&ViewCursorPos);


			if(Render_GetViewType(pView->VCam) < VIEWTOP) //3d?
			{
				if(!pView->IsPanning)
				{
					SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
				}
			}
			else
			{
				if(!pView->IsPanning && mCurrentTool!=ID_TOOLS_BRUSH_SCALEBRUSH
					&& mCurrentTool!=ID_TOOLS_BRUSH_SHEARBRUSH)
				{
					int FoundThingType;
					Brush *pMinBrush;
					CEntity *pMinEntity;
					geFloat Dist;
					FoundThingType = FindClosestThing (&ViewCursorPos, pView->VCam, &pMinBrush, &pMinEntity, &Dist);
					if ((FoundThingType == fctNOTHING) || (Dist > MAX_PIXEL_SELECT_DIST) || (mCurrentTool != CURTOOL_NONE))
					{
						SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));
					}
					else
					{
						switch (FoundThingType)
						{
							case fctBRUSH :
								SetCursor(AfxGetApp()->LoadCursor(IDC_ARROWBRUSH));
								strncpy (info, Brush_GetName(pMinBrush), MaxSize);
								break;
							case fctENTITY :
								SetCursor(AfxGetApp()->LoadCursor(IDC_ARROWENTITY));
								strncpy (info, pMinEntity->GetName (), MaxSize);
								break;
							default :
								// bad value returned from FindClosestThing
								assert (0);
						}
					}
				}
			}
		}
	}
}

void CFusionDoc::OnSelectedTypeCmdUI(CCmdUI* pCmdUI)
{
	if(mModeTool==ID_TOOLS_BRUSH_ADJUSTMENTMODE) pCmdUI->Enable();
	else pCmdUI->Enable(0);
}

// NO UI EXISTS FOR THIS FUNCTION
void CFusionDoc::OnBrushSelectedCopytocurrent() 
{
	ConPrintf("Temporarily Disabled...\n");
	return;
	// make the current brush a copy of the other brush 
//	BTemplate = *CurBrush;
//	CurBrush = &BTemplate;
//	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
//	mModeTool=ID_TOOLS_TEMPLATE;
//	ConfigureCurrentTool();
}

BOOL CFusionDoc::DeleteSelectedBrushes(void)
{
	geBoolean	bAlteredCurrentGroup = GE_FALSE ;
	CEntityArray *Entities = Level_GetEntities (pLevel);

	for(int Ent=0;Ent < Entities->GetSize() && (!(GetSelState() & NOENTITIES)); Ent++)
	{
		if((*Entities)[Ent].IsSelected ())
		{
			if( (*Entities)[Ent].IsCamera() == GE_FALSE )	// Exclude Cameras
			{
				if( (*Entities)[Ent].GetGroupId() == mCurrentGroup )
				{
					bAlteredCurrentGroup = GE_TRUE ;
				}
				
				DeleteEntity(Ent--);
				SetModifiedFlag();
			}
		}
	}

	if(GetSelState() & ANYBRUSH)
	{
		int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
		for(int i=0;i < NumSelBrushes;i++)
		{
			Brush *pBrush;

			pBrush = SelBrushList_GetBrush (pSelBrushes, 0);
			if( Brush_GetGroupId(pBrush) == mCurrentGroup )
			{
				bAlteredCurrentGroup = GE_TRUE ;
			}
			
			Level_RemoveBrush (pLevel, pBrush);
			SelBrushList_Remove (pSelBrushes, pBrush);
			Brush_Destroy (&pBrush);
		}

		//turn off any operation tools
		mCurrentTool = CURTOOL_NONE ;

		SetModifiedFlag();
	}

	// Deleting items removed group members so we must update the UI
	if( bAlteredCurrentGroup )
	{
		mpMainFrame->m_wndTabControls->GrpTab->UpdateTabDisplay( this ) ;
	}

	UpdateSelected();

	return FALSE;
}

BOOL CFusionDoc::TempDeleteSelected(void)
{
	BOOL	ret;
	int		i;
	int		NumTSelBrushes = SelBrushList_GetSize (pTempSelBrushes);

	for(ret=FALSE, i=0;i < NumTSelBrushes;i++)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pTempSelBrushes, 0);

		Level_RemoveBrush (pLevel, pBrush);
		SelBrushList_Remove (pTempSelBrushes, pBrush);
		Brush_Destroy(&pBrush);
		ret	=TRUE;
	}
	return	ret;
}

void CFusionDoc::OnBrushSelectedDelete() 
{
	DeleteSelectedBrushes();
}

void CFusionDoc::SetupDefaultFilename (void)
{
	const CString DocPath = GetPathName ();

	if (DocPath == "")
	{
		// new file...
		char NewFileName[MAX_PATH];
		CString titl = GetTitle ();
		strcpy (NewFileName, LastPath);
		::FilePath_AppendName (NewFileName, titl, NewFileName);
		::FilePath_SetExt (NewFileName, ".3dt", NewFileName);
		strlwr (NewFileName);
		SetPathName (NewFileName, FALSE);
	}
}


void CFusionDoc::OnFileSave() 
{
	if (IsNewDocument)
	{
		OnFileSaveAs ();
	}
	else
	{
		SetupDefaultFilename ();
		CDocument::OnFileSave ();
	}
}

void CFusionDoc::OnFileSaveAs() 
{
	SetupDefaultFilename ();
	CDocument::OnFileSaveAs ();
}

BOOL CFusionDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	::FilePath_GetDriveAndDir (lpszPathName, LastPath);

	if (Save( lpszPathName ) == GE_FALSE)
	{
		AfxMessageBox ("Error saving file.\rThe disk may be full.");
		return FALSE;
	}
	IsNewDocument = 0;
	SetModifiedFlag (FALSE);
	return TRUE;
}

BOOL CFusionDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	::FilePath_GetDriveAndDir (lpszPathName, LastPath);	

	if( Load( lpszPathName ) == GE_FALSE )
		return FALSE ;

	UpdateGridInformation();
	IsNewDocument = 0;

	return TRUE;
}


typedef struct
{
	CFusionDoc	*pDoc;
	int			CurId;
	Node		**pTree;
	geBoolean	bAddFlocking;	//set for adding flocking brushes
} AddBrushCBData;

static geBoolean AddBrushToBspCB(Brush *pBrush, void * lParam)
{
	AddBrushCBData *pData;
	Node **tree;
	CFusionDoc *pDoc;

	pData = (AddBrushCBData *)lParam;
	tree = pData->pTree;
	pDoc = pData->pDoc;

	assert(tree);

	if(pData->bAddFlocking ^ Brush_IsFlocking(pBrush))
	{
		if((Brush_GetModelId(pBrush)==pData->CurId) && !Brush_IsSubtract(pBrush))
		{
			if(pDoc)
			{
				if(pDoc->BrushIsVisible(pBrush)
					&& (!Brush_IsClip(pBrush)) && (!Brush_IsHint(pBrush)))
				{
					*tree	=Node_AddBrushToTree(*tree, pBrush);
				}
			}
			else
			{
				*tree	=Node_AddBrushToTree(*tree, pBrush);
			}
		}
	}
	return	GE_TRUE;
}

//blasts away the OGFaces of the changed trees (makes em draw solid)
void	CFusionDoc::InvalidateDrawTreeOriginalFaces(void)
{
	ModelIterator	mi;
	int				i;
	Node			*n;
	ModelInfo_Type	*ModelInfo;
	Model			*pMod;

	Node_InvalidateTreeOGFaces(mWorldBsp);

	ModelInfo	=Level_GetModelInfo(pLevel);
	pMod		=ModelList_GetFirst(ModelInfo->Models, &mi);
	n			=NULL;
	for(i=0;i < ModelList_GetCount(ModelInfo->Models);i++, n=NULL)
	{
		n	=Model_GetModelTree(pMod);
		if(n)
		{
			Node_InvalidateTreeOGFaces(n);
		}
		pMod	=ModelList_GetNext(ModelInfo->Models, &mi);
	}
}

void	CFusionDoc::RebuildTrees(void)
{
	ModelIterator	mi;
	int				i, CurId = 0;
	Node			*n;
	AddBrushCBData	BspCallbackData;
	ModelInfo_Type	*ModelInfo;
	BrushList		*BList;
	Model			*pMod;

	BList = Level_GetBrushes (pLevel);
	SetModifiedFlag();

	//do the world csg list and tree first
	Node_ClearBsp(mWorldBsp);
	mWorldBsp	=NULL;

	BrushList_ClearAllCSG (BList);

	BrushList_DoCSG(BList, CurId, ::fdocBrushCSGCallback, this);

	BspCallbackData.pDoc		=this;
	BspCallbackData.CurId		=CurId;
	BspCallbackData.pTree		=&mWorldBsp;
	BspCallbackData.bAddFlocking=GE_FALSE;

	BrushList_EnumCSGBrushes (BList, &BspCallbackData, ::AddBrushToBspCB) ;
	BspCallbackData.bAddFlocking=GE_TRUE;
	BrushList_EnumCSGBrushes (BList, &BspCallbackData, ::AddBrushToBspCB) ;

	//build individual model mini trees
	ModelInfo = Level_GetModelInfo (pLevel);
	pMod = ModelList_GetFirst (ModelInfo->Models, &mi);
	n		=NULL;
	for(i=0;i < ModelList_GetCount(ModelInfo->Models);i++, n=NULL)
	{
		CurId = Model_GetId (pMod);

		BrushList_DoCSG(BList, CurId, ::fdocBrushCSGCallback, this);

		//change pvoid from this to null to skip the BrushIsVisible check
		BspCallbackData.CurId		=CurId;
		BspCallbackData.pTree		=&n;
		BspCallbackData.bAddFlocking=GE_FALSE;
		BrushList_EnumCSGBrushes (BList, &BspCallbackData, ::AddBrushToBspCB) ;

		BspCallbackData.bAddFlocking=GE_TRUE;
		BrushList_EnumCSGBrushes (BList, &BspCallbackData, ::AddBrushToBspCB) ;

		Node_ClearBsp (Model_GetModelTree(pMod));
		Model_SetModelTree(pMod, n);
		pMod = ModelList_GetNext(ModelInfo->Models, &mi);
	}
	if(mAdjustMode==ADJUST_MODE_FACE)
	{
		UpdateFaceAttributesDlg ();
	}
}

void CFusionDoc::OnGbspnowater() 
{
	RebuildTrees();
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}


//==============================================================
// This function will delete our current thing if we 
// have something. e.g. entity or brush
//==============================================================
void CFusionDoc::DeleteCurrentThing()
{
	BOOL	ReBuild;

	if(mAdjustMode==ADJUST_MODE_BRUSH
		&& mModeTool==ID_GENERALSELECT)
	{
		ResetAllSelectedFaces();
		ReBuild	=(GetSelState() & ANYBRUSH);
		DeleteSelectedBrushes();

		if(ReBuild && Level_RebuildBspAlways(pLevel))
		{
			UpdateAllViews(UAV_ALL3DVIEWS | REBUILD_QUICK, NULL, TRUE );
		}
		else
		{
			UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		}
	}
}


//===============================================
// this function will update the information on the status bar
// about the grid
//===============================================
void CFusionDoc::UpdateGridInformation()
{
	if( mpMainFrame )
	{
		CMDIChildWnd	*pMDIChild	=(CMDIChildWnd *)mpMainFrame->MDIGetActive();
		if(pMDIChild)
		{
			CFusionView	*cv	=(CFusionView *)pMDIChild->GetActiveView();
			if(cv)
			{
				GridInfo *pGridInfo = Level_GetGridInfo (pLevel);

				mpMainFrame->UpdateGridSize(
				    Render_GetFineGrid(cv->VCam, (pGridInfo->GridType == GridMetric) ? GRID_TYPE_METRIC : GRID_TYPE_TEXEL),
					pGridInfo->UseGrid,
					((pGridInfo->SnapType == GridMetric) ? pGridInfo->MetricSnapSize : pGridInfo->TexelSnapSize),
					pGridInfo->GridType,
					pGridInfo->SnapType);
			}
		}
	}
}/* CFusionDoc::UpdateGridInformation */

static geBoolean	IsBrushNotClipOrHint(const Brush *b)
{
	assert(b != NULL);
	
	return	(Brush_IsHint(b) || Brush_IsClip(b))? GE_FALSE : GE_TRUE;
}


static void DrawEntity (CEntity *pEnt, ViewVars *v, const EntityTable *pEntityDefs)
{
	const geBitmap *pBitmap;

	pBitmap = pEnt->GetBitmapPtr (pEntityDefs);

	Render_3DTextureZBuffer(v, &pEnt->mOrigin, pBitmap);
}

void CFusionDoc::RenderWorld(ViewVars *v, CDC *pDC)
{
#define PEN_WHITE_COLOR RGB(255,255,255)
#define PEN_CYAN_COLOR  RGB(0,255,0)
#define PEN_BLUE_COLOR  RGB(0,0,255)
#define PEN_PURPLE_COLOR RGB(255,0,255)
#define PEN_YELLOW_COLOR RGB(255,255,0)
#define PEN_HINT_COLOR  RGB(0,100,0)
#define PEN_CLIP_COLOR  RGB(128,0,128)

	int				i;
	Node			*n;
	ModelIterator	mi;
	Model			*pModel;
	geBoolean		DoBlit	=GE_FALSE;
	BrushDrawData	brushDrawData;
	BrushList		*BList;


	// Temporary Hack for not updating
//#pragma message ("This temporary hack needs to be fixed.  Perhaps a 'force redraw' flag?")
/*
	if (Level_RebuildBspAlways (pLevel) == GE_FALSE)
	{
		return ;
	}
*/
	if (pLevel == NULL)
	{
		// must not be loaded yet...
		return ;
	}

	BList = Level_GetBrushes (pLevel);
    Render_UpdateViewPos(v);
	Render_SetUpFrustum(v);

	brushDrawData.pViewBox = NULL ;
	brushDrawData.pDC = pDC ;
	brushDrawData.v = v ;
	brushDrawData.pDoc = this;
	brushDrawData.GroupId = fdoc_SHOW_ALL_GROUPS;
	brushDrawData.FlagTest = NULL;
	brushDrawData.Color = PEN_WHITE_COLOR;

	if((Render_GetViewType(v) & (VIEWSOLID|VIEWTEXTURE)) && mWorldBsp)
	{
		Render_RenderTree(v, mWorldBsp, pDC->m_hDC, ZFILL);
		DoBlit	=TRUE;
	}
	else
	{
		Render_ClearViewDib (v);
		brushDrawData.FlagTest = IsBrushNotClipOrHint;
		BrushList_EnumLeafBrushes(BList, &brushDrawData, ::BrushDrawWire3dCB);
	}
	if(bShowClipBrushes)
	{
		brushDrawData.Color	=PEN_CLIP_COLOR;
		brushDrawData.FlagTest = Brush_IsClip;
		BrushList_EnumLeafBrushes(BList, &brushDrawData, ::BrushDrawWire3dZBufferCB);
	}
	if(bShowHintBrushes)
	{
		brushDrawData.Color	=PEN_HINT_COLOR;
		brushDrawData.FlagTest = Brush_IsHint;
		BrushList_EnumLeafBrushes(BList, &brushDrawData, ::BrushDrawWire3dZBufferCB);
	}

	CEntityArray *Entities = Level_GetEntities (pLevel);

	for(i=0;i < Entities->GetSize();i++)
	{
		CEntity *pEnt;

		pEnt = &(*Entities)[i];

		if( !Render_PointInFrustum(v, &pEnt->mOrigin ) )
			continue ;

		if( EntityIsVisible( pEnt ) == GE_FALSE )
			continue;

		if (pEnt->IsSelected ())
			continue;

		::DrawEntity (pEnt, v, Level_GetEntityDefs (pLevel));
	}

	if(!(GetSelState() & NOENTITIES))
	{
		for(i=0;i < Entities->GetSize();i++)
		{
			CEntity *pEnt = &(*Entities)[i];

			if(!(pEnt->IsSelected () && EntityIsVisible( pEnt )) )
				continue;

			if(!Render_PointInFrustum(v, &pEnt->mOrigin))
				continue;

			const geBitmap *pBitmap;

			pBitmap = pEnt->GetBitmapPtr (Level_GetEntityDefs (pLevel));

			Render_3DTextureZBufferOutline(v, &pEnt->mOrigin, pBitmap, RGB(0,255,255));
		}
	}

	if(DoBlit)
	{
		ModelInfo_Type *ModelInfo = Level_GetModelInfo (pLevel);

		//render the models
		pModel = ModelList_GetFirst (ModelInfo->Models, &mi);
		while (pModel != NULL)
		{
			n = Model_GetModelTree (pModel);
			if (n != NULL)
			{
				Render_RenderTree(v, n, pDC->m_hDC, ZBUFFER);
				DoBlit	=TRUE;
			}
			pModel = ModelList_GetNext (ModelInfo->Models, &mi);
		}
	}


	brushDrawData.FlagTest = NULL;
	brushDrawData.Color = PEN_CYAN_COLOR;

	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
	for(i=0;i < NumSelBrushes;i++)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pSelBrushes, i);
		if(Brush_IsMulti(pBrush))
		{
			BrushList_EnumLeafBrushes(Brush_GetBrushList(pBrush), &brushDrawData, ::BrushDrawWire3dCB);
		}
		else
		{
			Render_RenderBrushFaces(v, pBrush, brushDrawData.Color);
		}
	}

	if(!(GetSelState() & NOFACES))
	{
		brushDrawData.Color = PEN_PURPLE_COLOR;
		BrushList_EnumLeafBrushes(BList, &brushDrawData, ::BrushDrawWireSel3dCB);
	}

	if((mModeTool==ID_TOOLS_TEMPLATE)||
		(mModeTool==ID_TOOLS_CAMERA && GetSelState()==NOSELECTIONS))
	{
		brushDrawData.Color = PEN_BLUE_COLOR;
		if(!TempEnt)
		{
			if(Brush_IsMulti(CurBrush))
			{
				BrushList_EnumLeafBrushes(Brush_GetBrushList(CurBrush), &brushDrawData, ::BrushDrawWire3dCB);
			}
			else
			{
				Render_RenderBrushFaces(v, CurBrush, brushDrawData.Color);
			}
		}
		else
		{
			if(Render_PointInFrustum(v, &mRegularEntity.mOrigin))
			{
				::DrawEntity (&mRegularEntity, v, Level_GetEntityDefs (pLevel));
			}
		}
	}

//	if (DoBlit)
	{
		Render_BlitViewDib(v, pDC->m_hDC);
	}
}

void CFusionDoc::MoveSelectedBrushList
	(
	  SelBrushList *pList,
	  geVec3d const *v
	)
{
	int		i;
	CEntityArray *Entities = Level_GetEntities (pLevel);
	int NumBrushes;

	mLastOp	=BRUSH_MOVE;

	geVec3d_Add (&SelectedGeoCenter, v, &SelectedGeoCenter);
	geVec3d_Add(v, &FinalPos, &FinalPos);

	NumBrushes = SelBrushList_GetSize (pList);
	for(i=0;i < NumBrushes;i++)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pList, i);
		Brush_Move (pBrush, v);
	}

	for(i=0;i < Entities->GetSize();i++)
	{
		if ((*Entities)[i].IsSelected ())
		{
			(*Entities)[i].Move (v);
		}
	}
}

void CFusionDoc::MoveSelectedBrushes(geVec3d const *v)
{
	MoveSelectedBrushList (pTempSelBrushes, v);
}

void CFusionDoc::MoveSelectedClone(geVec3d const *v)
{
	MoveSelectedBrushList (pSelBrushes, v);
}

void CFusionDoc::MoveTemplateBrush(geVec3d *v)
{
	mLastOp	=BRUSH_MOVE;

	geVec3d_Add (&SelectedGeoCenter, v, &SelectedGeoCenter);
	geVec3d_Add (v, &FinalPos, &FinalPos);

	if(TempEnt)
	{
		mRegularEntity.Move (v);
	}
	else
	{
		geVec3d *pTemplatePos;

		Brush_Move (CurBrush, v);
		pTemplatePos = Level_GetTemplatePos (pLevel);
		Brush_Center (CurBrush, pTemplatePos);
	}
}

void CFusionDoc::RotateTemplateBrush(geVec3d *v)
{
	geXForm3d	rm;

	mLastOp	=BRUSH_ROTATE;

	geXForm3d_SetEulerAngles(&rm, v);
	Brush_Rotate (CurBrush, &rm, &SelectedGeoCenter);
}

void CFusionDoc::GetRotationPoint
	(
	  geVec3d *pVec
	)
{
	Model *pModel;
	ModelInfo_Type *ModelInfo = Level_GetModelInfo (pLevel);

	pModel = ModelList_GetAnimatingModel (ModelInfo->Models);
	if (pModel != NULL)
	{
		CModelDialog *ModelTab;
		geVec3d Xlate;

		// we're animating a model, so use its current position
		Model_GetCurrentPos (pModel, pVec);
		// We have to add the current move translation
		ModelTab = mpMainFrame->GetModelTab ();
		if (ModelTab != NULL)
		{
			ModelTab->GetTranslation (&Xlate);
		}
		else
		{
			geVec3d_Clear (&Xlate);
		}
		geVec3d_Add (pVec, &Xlate, pVec);
	}
	else
	{
		*pVec = SelectedGeoCenter;
	}
}

void CFusionDoc::RotateSelectedBrushList
	(
	  SelBrushList *pList,
	  geVec3d const *v
	)
{
	int i;
	geXForm3d rm;
	geVec3d RotationPoint;
	int NumBrushes = SelBrushList_GetSize (pList);

	mLastOp = BRUSH_ROTATE;

	// if we're animating a model, we want to rotate about the
	// model's current rotation origin--taking into account any
	// current translations.
	GetRotationPoint (&RotationPoint);

	
	geVec3d_Add(v, &FinalRot, &FinalRot);
	geXForm3d_SetEulerAngles(&rm, v);

	for(i=0;i < NumBrushes;i++)
	{
		Brush *pBrush = SelBrushList_GetBrush (pList, i);
		Brush_Rotate (pBrush, &rm, &RotationPoint);
	}
}/* CFusionDoc::RotateSelectedBrushList */

void CFusionDoc::RotateSelectedBrushes(geVec3d const *v)
{
	RotateSelectedBrushList (pTempSelBrushes, v);
}

void CFusionDoc::RotateSelectedBrushesDirect(geVec3d const *v)
{
	RotateSelectedBrushList (pSelBrushes, v);
	UpdateSelectedModel (BRUSH_ROTATE, v);
	geVec3d_Clear (&FinalRot);
}

static geBoolean fdocBrushTextureScaleCallback (Brush *pBrush, void *lParam)
{
	const geFloat *pScaleVal = (geFloat *)lParam;

	Brush_SetTextureScale (pBrush, *pScaleVal);
	return GE_TRUE;
}

// Sets texture scale on all faces of all selected brushes.
void CFusionDoc::SetAllFacesTextureScale(geFloat ScaleVal)
{
	if (SelBrushList_GetSize (pSelBrushes) > 0)
	{
		SelBrushList_Enum (pSelBrushes, fdocBrushTextureScaleCallback, &ScaleVal);
		if (Level_RebuildBspAlways (pLevel))
		{
			RebuildTrees();
			UpdateAllViews (UAV_ALL3DVIEWS, NULL);
		}
	}
}

void CFusionDoc::DoneRotate(void)
{
	int			i;
	geFloat	RSnap;
	geXForm3d		rm;
	geVec3d RotationPoint;

	mLastOp		=BRUSH_ROTATE;

	TempDeleteSelected();
	TempCopySelectedBrushes();

	GetRotationPoint (&RotationPoint);

	if((SelState & NOENTITIES) && Level_UseGrid (pLevel))
	{
		RSnap		=Units_DegreesToRadians ((float)Level_GetRotationSnap (pLevel));
		FinalRot.X	=((float)((int)(FinalRot.X / RSnap))) * RSnap;
		FinalRot.Y	=((float)((int)(FinalRot.Y / RSnap))) * RSnap;
		FinalRot.Z	=((float)((int)(FinalRot.Z / RSnap))) * RSnap;
	}

	geXForm3d_SetEulerAngles(&rm, &FinalRot);

	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	for(i=0;i < NumSelBrushes;i++)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pTempSelBrushes, i);

		Brush_Rotate (pBrush, &rm, &RotationPoint);
	}
	if(i < NumSelBrushes)
	{
		TempDeleteSelected();
	}
	else
	{
		BrushList *BList = Level_GetBrushes (pLevel);
		for(i=0;i < NumSelBrushes;i++)
		{
			// Replace the sel list brushes with the TSelList brushes
			Brush *TempBrush, *OldBrush;

			TempBrush = SelBrushList_GetBrush (pTempSelBrushes, 0);
			OldBrush = SelBrushList_GetBrush (pSelBrushes, 0);

			BrushList_Remove (BList, TempBrush);
			BrushList_InsertAfter (BList, OldBrush, TempBrush);
			BrushList_Remove (BList, OldBrush);

			SelBrushList_Remove (pSelBrushes, OldBrush);
			SelBrushList_Remove (pTempSelBrushes, TempBrush);

			SelBrushList_Add (pSelBrushes, TempBrush);

			Brush_Destroy (&OldBrush);
		}
	}
	UpdateSelected();

	UpdateSelectedModel (BRUSH_ROTATE, &FinalRot);

	geVec3d_Clear (&FinalRot);

	// Find the camera entity and update the rendered view's camera position
	{
		CEntity *pCameraEntity = FindCameraEntity ();

		if (pCameraEntity != NULL)
		{
			geVec3d Angles;

			pCameraEntity->GetAngles( &Angles, Level_GetEntityDefs (pLevel) ) ;
			SetRenderedViewCamera( &(pCameraEntity->mOrigin), &Angles) ;
		}
	}
}

void CFusionDoc::UpdateSelectedModel
	(
	  int MoveRotate,
	  geVec3d const *pVecDelta
	)
{
	ModelIterator mi;
	Model *pModel;
	ModelInfo_Type *ModelInfo = Level_GetModelInfo (pLevel);
	BrushList *BList = Level_GetBrushes (pLevel);

	// notify model dialog so that it can update animation deltas if required
	mpMainFrame->UpdateSelectedModel (MoveRotate, pVecDelta);

	// For each model that is fully selected, update its reference position.
	pModel = ModelList_GetFirst (ModelInfo->Models, &mi);
	while (pModel != NULL)
	{
		// don't mess with models that are animating
		if (Model_IsAnimating (pModel) != GE_TRUE)
		{
			if (Model_IsSelected (pModel, pSelBrushes, BList))
			{
				Model_UpdateOrigin (pModel, MoveRotate, pVecDelta);
			}
		}
		pModel = ModelList_GetNext (ModelInfo->Models, &mi);
	}
}

void CFusionDoc::DoneResize(int sides, int inidx)
{
	mLastOp	=BRUSH_SCALE;

	TempDeleteSelected();

	if (mModeTool==ID_TOOLS_TEMPLATE)
	{
		if(Brush_IsMulti(CurBrush))
		{
			BrushList_ClearCSGAndHollows((BrushList *)Brush_GetBrushList(CurBrush), Brush_GetModelId(CurBrush));
			BrushList_RebuildHollowFaces((BrushList *)Brush_GetBrushList(CurBrush), Brush_GetModelId(CurBrush), ::fdocBrushCSGCallback, this);
		}
		return;
	}

	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
	for (int i = 0; i < NumSelBrushes; ++i)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pSelBrushes, i);

		Brush_ResizeFinal(pBrush, sides, inidx, &FinalScale);
		if(Brush_IsMulti(pBrush))
		{
			BrushList_ClearCSGAndHollows((BrushList *)Brush_GetBrushList(pBrush), Brush_GetModelId(pBrush));
			BrushList_RebuildHollowFaces((BrushList *)Brush_GetBrushList(pBrush), Brush_GetModelId(pBrush), ::fdocBrushCSGCallback, this);
		}
	}
	UpdateSelected();
}

void CFusionDoc::DoneMove (void)
{
	int	i;
//	BrushList *BList = Level_GetBrushes (pLevel);

	mLastOp	=BRUSH_MOVE;

	TempDeleteSelected();

	if(mModeTool == ID_TOOLS_TEMPLATE)
	{
		if (TempEnt)
		{
			DoneMoveEntity ();
		}
		else
		{
			Brush_Move (CurBrush, &FinalPos);
		}
		return;
	}
	else
	{
		int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
		for(i=0;i < NumSelBrushes;i++)
		{
			Brush *pBrush;

			pBrush = SelBrushList_GetBrush (pSelBrushes, i);
			Brush_Move (pBrush, &FinalPos);
		}

		if (GetSelState() & ANYENTITY)
		{
			DoneMoveEntity();
		}

		UpdateSelected();

		UpdateSelectedModel (BRUSH_MOVE, &FinalPos);
	}
//	UndoStack_Move (pUndoStack, &FinalPos, NumSelBrushes, SelListBrush, Level_GetEntities (pLevel));

	geVec3d_Clear (&FinalPos);
}


void CFusionDoc::DoneMoveEntity(void)
{
	int		i;
	float	SnapSize;
	CEntityArray *Entities = Level_GetEntities (pLevel);
	CEntity *pEnt;

	if (mCurrentTool==ID_TOOLS_BRUSH_MOVEROTATEBRUSH)
	{
		if ((GetSelState()==ONEENTITYONLY) && Level_UseGrid (pLevel))
		{
			SnapSize = Level_GetGridSnapSize (pLevel);
		}
		else
		{
			SnapSize = 1.0f;
		}

		for(i=0;i < Entities->GetSize();i++)
		{
			pEnt = &(*Entities)[i];

			if (pEnt->IsSelected ())
			{
				pEnt->DoneMove (SnapSize, Level_GetEntityDefs (pLevel));
				if( pEnt->IsCamera() == GE_TRUE )	// Camera Entity?
				{				
					geVec3d	PitchRollYaw ;

					pEnt->GetAngles( &PitchRollYaw, Level_GetEntityDefs (pLevel) ) ;
					SetRenderedViewCamera( &(pEnt->mOrigin), &PitchRollYaw ) ;
					UpdateAllViews(UAV_RENDER_ONLY, NULL);		
				}// Camera entity, update camera
			}// Entity Selected
		}// Loop thru Entities
	}
	else
	{
		if(mCurrentEntity < 0) //template
		{
			pEnt = &mRegularEntity;
		}
		else
		{
			pEnt = &(*Entities)[mCurrentEntity];
		}

		SnapSize = 1.0f;
		if( Level_UseGrid (pLevel))
		{
			SnapSize = Level_GetGridSnapSize (pLevel);
		}
		pEnt->DoneMove(SnapSize, Level_GetEntityDefs (pLevel));
	}
	SetModifiedFlag();
}/* CFusionDoc::DoneMoveEntity */


void CFusionDoc::UpdateAllViews(int Mode, CView* pSender, BOOL Override)
{
	if(IsModified() && ((Mode & REBUILD_QUICK ) && (Level_RebuildBspAlways (pLevel)))||(Override))
	{
		RebuildTrees();
	}
	else if((Mode & REBUILD_QUICK) && (!Level_RebuildBspAlways (pLevel)))
	{
		InvalidateDrawTreeOriginalFaces();
	}

	if(Mode & REBUILD_QUICK)
		Mode &= ~REBUILD_QUICK;

	//	Do we want to redraw everything?
	if (Mode & UAV_ALLVIEWS)
	{
		CDocument::UpdateAllViews(pSender);
		return;
	}

	POSITION pos = GetFirstViewPosition();

	while (pos != NULL)
	{
		CView* pView = GetNextView(pos);

		if ( pView->IsKindOf( RUNTIME_CLASS (CFusionView)) )
		{
			CFusionView* pFusionView = (CFusionView*) pView;
			CDC* pDC = pFusionView->GetDC();

			switch(Mode)
			{
			case UAV_ACTIVE3DVIEW_ONLY:

				if( pFusionView->GetParentFrame() == mpActiveViewFrame )
					pFusionView->Invalidate( TRUE );
				break;

			case UAV_NONACTIVE3DVIEWS_ONLY:

				if( pFusionView->GetParentFrame() != mpActiveViewFrame )
					pFusionView->Invalidate( TRUE );
				break;

			case UAV_TEXTUREVIEW_ONLY:

				if (pFusionView->mViewType == ID_VIEW_TEXTUREVIEW)
					pFusionView->Invalidate(TRUE);
				break;

			case UAV_RENDER_ONLY:

				switch(pFusionView->mViewType)
				{
				case ID_VIEW_3DWIREFRAME:
				case ID_VIEW_TEXTUREVIEW:

					pFusionView->Invalidate(TRUE);
					break;

				default:
					break;
				}
				break;

			case UAV_GRID_ONLY:

				switch(pFusionView->mViewType)
				{
				case ID_VIEW_TOPVIEW:
				case ID_VIEW_SIDEVIEW:
				case ID_VIEW_FRONTVIEW:

					pFusionView->Invalidate(TRUE);
					break;
				}
				break;


			case UAV_THIS_GRID_ONLY:
				if( pFusionView == pSender )
				{
					switch(pFusionView->mViewType)
					{
					case ID_VIEW_TOPVIEW:
					case ID_VIEW_SIDEVIEW:
					case ID_VIEW_FRONTVIEW:
						pFusionView->Invalidate(TRUE);
						break;
					}
				}
				break;

			case UAV_ALL3DVIEWS:

				pFusionView->Invalidate(TRUE);
				break;

			default:
				break;
			}

			pFusionView->ReleaseDC(pDC);
		}
	}
}

void CFusionDoc::FaceAttributesDialog ()
{
	//	Has a face been selected?  Are we selecting faces...
	if ((mAdjustMode == ADJUST_MODE_FACE) && (SelFaceList_GetSize (pSelFaces) > 0))
	{
		if (mpFaceAttributes == NULL)
		{
			mpFaceAttributes = new CFaceAttributesDialog (this);
		}
	}
}

void CFusionDoc::NullFaceAttributes()
{
	mpFaceAttributes = NULL;
}

//	This function basically sets up everything required
//  for each tool before its use...
void CFusionDoc::ConfigureCurrentTool(void)
{
	BOOL	Redraw	=FALSE;

	if(mModeTool==ID_TOOLS_CAMERA)
	{
		mCurrentTool		=CURTOOL_NONE;
		mShowSelectedFaces	=(mAdjustMode==ADJUST_MODE_FACE);
		mShowSelectedBrushes=(mAdjustMode==ADJUST_MODE_BRUSH);
		UpdateAllViews(UAV_ALL3DVIEWS, NULL);
		return;
	}

	switch(mAdjustMode)
	{
		case ADJUST_MODE_BRUSH :
			mShowSelectedFaces	=FALSE;
			mShowSelectedBrushes=TRUE;
			
			UpdateSelected();
			Redraw	=TRUE;
			break;

		case ADJUST_MODE_FACE :
			mShowSelectedFaces	=TRUE;
			mShowSelectedBrushes=FALSE;

			UpdateSelected();
			Redraw	=TRUE;
			break;

		default :
			assert (0);	// bad adjustment mode
			break;
	}

	switch(mCurrentTool)
	{
	case ID_TOOLS_BRUSH_MOVEROTATEBRUSH:
		if(mModeTool!=ID_TOOLS_TEMPLATE)
		{
			mShowSelectedFaces	=FALSE;
			mShowSelectedBrushes=TRUE;
			Redraw				=TRUE;
		}
		else
		{
			mShowSelectedFaces	=FALSE;
			mShowSelectedBrushes=FALSE;
		}
		break;

	case ID_TOOLS_BRUSH_SCALEBRUSH:
		if(mModeTool!=ID_TOOLS_TEMPLATE)
		{
			Redraw				=TRUE;
			mShowSelectedFaces	=FALSE;
			mShowSelectedBrushes=TRUE;
		}
		else
		{
			mShowSelectedFaces	=FALSE;
			mShowSelectedBrushes=FALSE;
		}
		break;

	}
	if(mModeTool==ID_TOOLS_TEMPLATE && TempEnt)
		mCurrentEntity=-1;

	Redraw	=TRUE;
	if(Redraw)
		UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::UpdateSelected(void)
{
	int		i;
	int NumSelFaces = SelFaceList_GetSize (pSelFaces);
	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	SelState=(NumSelBrushes > 1)? MULTIBRUSH : NumSelBrushes;
	SelState|=(NumSelFaces >1)? MULTIFACE : (NumSelFaces+1)<<3;
	SelState|=(NumSelEntities >1)? MULTIENTITY : (NumSelEntities+1)<<7;


	if(mModeTool==ID_GENERALSELECT)
	{
		if(GetSelState() & ONEBRUSH)
			CurBrush	=SelBrushList_GetBrush (pSelBrushes, 0);
		else
			CurBrush	=BTemplate;
	}

	geVec3d_Clear (&SelectedGeoCenter);

	if (mModeTool==ID_TOOLS_TEMPLATE)
	{
		Brush_Center (CurBrush, &SelectedGeoCenter);
	}
	else if((SelState & MULTIBRUSH) || (SelState & ONEBRUSH))
	{
		Model *pModel;
		ModelInfo_Type *ModelInfo = Level_GetModelInfo (pLevel);

		pModel = ModelList_GetAnimatingModel (ModelInfo->Models);
		if (pModel != NULL)
		{
			// we're animating a model, so use its current position
			Model_GetCurrentPos (pModel, &SelectedGeoCenter);
		}
		else
		{
			Box3d ViewBox;
			Brush *pBrush;
			
			pBrush = SelBrushList_GetBrush (pSelBrushes, 0);
			ViewBox = *Brush_GetBoundingBox (pBrush);
			for(i = 1;i < NumSelBrushes;i++)
			{
				pBrush = SelBrushList_GetBrush (pSelBrushes, i);
				Box3d_Union (Brush_GetBoundingBox(pBrush), &ViewBox, &ViewBox);
			}
			Box3d_GetCenter (&ViewBox, &SelectedGeoCenter);
		}
	}
	if(SelState & ONEENTITY)
	{
		CEntityArray *Entities = Level_GetEntities (pLevel);

		for(i=0;i < Entities->GetSize() && !((*Entities)[i].IsSelected ());i++);
		mCurrentEntity	=i;
	}
	else
		mCurrentEntity	=-1;

	assert( mpMainFrame->m_wndTabControls ) ;
	assert( mpMainFrame->m_wndTabControls->GrpTab ) ;
	mpMainFrame->m_wndTabControls->GrpTab->UpdateGroupSelection( ) ;

}/* CFusionDoc::UpdateSelected */

void CFusionDoc::NullBrushAttributes(void){  mpBrushAttributes=NULL;  }

void CFusionDoc::AddBrushToWorld(void)
{
	if(TempEnt || !Brush_IsSubtract (CurBrush))
		OnBrushAddtoworld();
	else
		OnBrushSubtractfromworld();
}

void CFusionDoc::ResetAllSelectedBrushes(void)
{
	SelBrushList_RemoveAll (pSelBrushes);
	CurBrush		=BTemplate;
}

#pragma warning (disable:4100)
static geBoolean	ResetSelectedFacesCB(Brush *b, void *pVoid)
{
	int	i;

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		Face	*pFace;

		pFace	=Brush_GetFace(b, i);
		Face_SetSelected(pFace, GE_FALSE);
	}
	return GE_TRUE ;
}
#pragma warning (default:4100)

void CFusionDoc::ResetAllSelectedFaces(void)
{
	BrushList_EnumLeafBrushes(Level_GetBrushes (pLevel), NULL, ::ResetSelectedFacesCB) ;
	SelFaceList_RemoveAll (pSelFaces);
}

static geBoolean FindSelectedFaceCB (Brush *b, void *lParam)
{
	Face **ppSelectedFace = (Face **)lParam;
	int i;

	for (i = 0; i < Brush_GetNumFaces (b); ++i)
	{
		Face *pFace;

		pFace = Brush_GetFace (b, i);
		if (Face_IsSelected (pFace))
		{
			*ppSelectedFace = pFace;
			return GE_FALSE;
		}
	}
	return GE_TRUE;
}

Face *CFusionDoc::FindSelectedFace (void)
{
	Face *pSelectedFace;

	pSelectedFace = NULL;
	BrushList_EnumLeafBrushes (Level_GetBrushes (pLevel), &pSelectedFace, ::FindSelectedFaceCB);
	return pSelectedFace;
}

void CFusionDoc::OnConstrainhollows()
{
	mConstrainHollows = ! mConstrainHollows ;
}

void CFusionDoc::OnUpdateConstrainhollows(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck( mConstrainHollows );
}

void CFusionDoc::OnGeneralselect()
{
	DoGeneralSelect ();
}

void CFusionDoc::DoGeneralSelect (void)
{
	mCurrentTool	=CURTOOL_NONE;
	mModeTool		=ID_GENERALSELECT;
	ConfigureCurrentTool();
	mpMainFrame->m_wndTabControls->m_pBrushEntityDialog->Update(this);
}

void CFusionDoc::OnUpdateGeneralselect(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck ((mModeTool == ID_GENERALSELECT) ? 1 : 0);
}

static geBoolean SelAllBrushFaces (Brush *pBrush, void *lParam)
{
	int iFace, nFaces;
	CFusionDoc *pDoc = (CFusionDoc *)lParam;

	nFaces = Brush_GetNumFaces (pBrush);
	for (iFace = 0; iFace < nFaces; ++iFace)
	{
		Face *pFace;

		pFace = Brush_GetFace (pBrush, iFace);
		Face_SetSelected (pFace, GE_TRUE);
		SelFaceList_Add (pDoc->pSelFaces, pFace);
	}
	return GE_TRUE;
}

static geBoolean fdocSelectBrushesFromFaces (Brush *pBrush, void *lParam)
{
	CFusionDoc *pDoc = (CFusionDoc *)lParam;
	int iFace, nFaces;

	// if any of the brush's faces is selected, then select the brush.
	nFaces = Brush_GetNumFaces (pBrush);
	for (iFace = 0; iFace < nFaces; ++iFace)
	{
		Face *pFace;

		pFace = Brush_GetFace (pBrush, iFace);
		if (Face_IsSelected (pFace))
		{
			pDoc->DoBrushSelection (pBrush, brushSelAlways);
			break;
		}
	}
	return GE_TRUE;
}

void CFusionDoc::SetAdjustmentMode( fdocAdjustEnum nCmdIDMode )
{
	if( mAdjustMode == nCmdIDMode )
		return ;

	if (nCmdIDMode == ADJUST_MODE_TOGGLE)
	{
		nCmdIDMode = (mAdjustMode == ADJUST_MODE_BRUSH) ? ADJUST_MODE_FACE : ADJUST_MODE_BRUSH;
	}

	switch (nCmdIDMode)
	{
		case ADJUST_MODE_BRUSH :
			mAdjustMode = nCmdIDMode;

			// go through brush list and select any brush that has selected faces.
			// Ensure that all brushes in a locked group or model set are selected...
			Level_EnumLeafBrushes (pLevel, this, ::fdocSelectBrushesFromFaces);

			ResetAllSelectedFaces();
			UpdateSelected();

			//remove face attributes dialog if present...
			DeleteFaceAttributes ();
			ConfigureCurrentTool();
			break;

		case ADJUST_MODE_FACE :
		{
			mAdjustMode = nCmdIDMode;
			
			// Select all faces on all selected brushes
			int iBrush;
			int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

			for (iBrush = 0; iBrush < NumSelBrushes; ++iBrush)
			{
				Brush *pBrush;

				pBrush = SelBrushList_GetBrush (pSelBrushes, iBrush);
				if (Brush_IsMulti (pBrush))
				{
					BrushList_EnumLeafBrushes (Brush_GetBrushList (pBrush), this, ::SelAllBrushFaces);
				}
				else
				{
					::SelAllBrushFaces (pBrush, this);
				}
			}
			UpdateSelected ();
			//remove brush attributes dialog if present...
			DeleteBrushAttributes ();
			ConfigureCurrentTool();
			break;
		}
		default :
			assert (0);		// bad mode (can't happen?)
			break;
	}
}


void CFusionDoc::OnCloseDocument() 
{
	CDocument::OnCloseDocument();
}

void CFusionDoc::OnThingAttributes() 
{
	switch( mAdjustMode )
	{
		case ADJUST_MODE_BRUSH :
			OnToolsBrushAttributes ();
			break;

		case ADJUST_MODE_FACE :
			OnToolsFaceAttributes ();
			break;

		default :
			assert (0);  // bad...
			break;
	}
}

void CFusionDoc::OnUpdateEditCopy(CCmdUI* pCmdUI) 
{
	if(GetSelState()!=NOSELECTIONS)	pCmdUI->Enable( TRUE );
	else pCmdUI->Enable( FALSE );
}

void CFusionDoc::OnUpdateEditPaste(CCmdUI* pCmdUI) 
{
	COleDataObject dobj;

	dobj.AttachClipboard();

	//	Is there any data available?
	if( dobj.IsDataAvailable( (unsigned short)(mpMainFrame->m_CB_FUSION_BRUSH_FORMAT) ) ||
		dobj.IsDataAvailable( (unsigned short)(mpMainFrame->m_CB_FUSION_ENTITY_FORMAT) ) )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( FALSE );	
}

//	Place selected brushes on clipboard and then delete them...
void CFusionDoc::OnEditCut() 
{
	BOOL	Flag	=(GetSelState() & ANYBRUSH);

	OnEditCopy();
	DeleteSelectedBrushes();

	if( Flag )
		UpdateAllViews(UAV_ALL3DVIEWS | REBUILD_QUICK, NULL);
	else
		UpdateAllViews( UAV_ALL3DVIEWS, NULL );
}

void CFusionDoc::OnEditDelete() 
{
	DeleteCurrentThing ();	
}

void CFusionDoc::OnUpdateEditCut(CCmdUI* pCmdUI) 
{
	if(GetSelState()!=NOSELECTIONS)	pCmdUI->Enable( TRUE );
	else pCmdUI->Enable( FALSE );
}


void CFusionDoc::ShearSelected(float dx, float dy, int sides, int inidx)
{
	mLastOp				=BRUSH_SHEAR;

	if(mModeTool == ID_TOOLS_TEMPLATE)
	{
		Brush_Destroy(&CurBrush);
		CurBrush	=BTemplate	=Brush_Clone(TempShearTemplate);
		Brush_ShearFixed(CurBrush, dx, dy, sides, inidx, &FinalScale, &ScaleNum);
	}
	else
	{
		int i;
		int NumSelBrushes;

		TempDeleteSelected();
		TempCopySelectedBrushes();
		NumSelBrushes = SelBrushList_GetSize (pTempSelBrushes);
		for (i = 0; i < NumSelBrushes; ++i)
		{
			Brush *pBrush;

			pBrush = SelBrushList_GetBrush (pTempSelBrushes, i);
			Brush_ShearFixed(pBrush, dx, dy, sides, inidx, &FinalScale, &ScaleNum);
		}
	}
}

void CFusionDoc::DoneShear(int sides, int inidx)
{
//	BrushList	*BList = Level_GetBrushes (pLevel);
	const Box3d	*bx1, *bx2;
	int			snapside	=0;
	geFloat		bsnap;
	
	mLastOp	=BRUSH_SHEAR;

	bsnap = 1.0f;
	if(Level_UseGrid (pLevel))
	{
		bsnap = Level_GetGridSnapSize (pLevel);
	}

	if(mModeTool==ID_TOOLS_TEMPLATE)
	{
		if(TempShearTemplate)	//can get here without shearing
		{						//by rapid clicking
			Brush_Destroy(&CurBrush);
			CurBrush	=BTemplate	=Brush_Clone(TempShearTemplate);
			Brush_ShearFinal(CurBrush, sides, inidx, &FinalScale);

			//check which side of the bounds changed
			bx1	=Brush_GetBoundingBox(CurBrush);
			bx2	=Brush_GetBoundingBox(TempShearTemplate);

			if(bx1->Max.X != bx2->Max.X)	snapside	|=2;
			if(bx1->Max.Y != bx2->Max.Y)	snapside	|=8;
			if(bx1->Max.Z != bx2->Max.Z)	snapside	|=32;
			if(bx1->Min.X != bx2->Min.X)	snapside	|=1;
			if(bx1->Min.Y != bx2->Min.Y)	snapside	|=4;
			if(bx1->Min.Z != bx2->Min.Z)	snapside	|=16;
			Brush_SnapShearNearest(CurBrush, bsnap, sides, inidx, snapside);
			Brush_Destroy(&TempShearTemplate);
		}
		return;
	}

	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	TempDeleteSelected();
	TempCopySelectedBrushes();

	int i;
	
	for (i = 0; i < NumSelBrushes; ++i)
	{
		Brush *pBrush;
		Brush *tBrush;

		pBrush = SelBrushList_GetBrush (pSelBrushes, i);
		tBrush = SelBrushList_GetBrush (pTempSelBrushes, i);

		Brush_ShearFinal(pBrush, sides, inidx, &FinalScale);

		//check which side of the bounds changed
		bx1	=Brush_GetBoundingBox(pBrush);
		bx2	=Brush_GetBoundingBox(tBrush);

		if(bx1->Max.X != bx2->Max.X)	snapside	|=2;
		if(bx1->Max.Y != bx2->Max.Y)	snapside	|=8;
		if(bx1->Max.Z != bx2->Max.Z)	snapside	|=32;
		if(bx1->Min.X != bx2->Min.X)	snapside	|=1;
		if(bx1->Min.Y != bx2->Min.Y)	snapside	|=4;
		if(bx1->Min.Z != bx2->Min.Z)	snapside	|=16;

		Brush_SnapShearNearest(pBrush, bsnap, sides, inidx, snapside);
	}
	TempDeleteSelected();
	UpdateSelected();
}

int CFusionDoc::DoCompileDialog
	(
	  void
	)
{
	CompileParamsType *CompileParams;

	CompileParams = Level_GetCompileParams (pLevel);
	// Build output file if none there...
	if (CompileParams->Filename[0] == '\0')
	{
		FilePath_SetExt (GetPathName (), ".map", CompileParams->Filename);
	}

	CCompileDialog CompileDlg (AfxGetMainWnd (), CompileParams);

	return CompileDlg.DoModal ();
}

static char *SkyFaceNames[6] =
	{"SkyLeft", "SkyRight", "SkyTop", "SkyBottom", "SkyFront", "SkyBack"};

typedef struct
{
	geBoolean SuppressHidden;
	geBoolean VisDetail;
	CFusionDoc *pDoc;
	int BrushCount;
	FILE *f;
} fdocBrushEnumData;

static geBoolean fdocBrushCountCallback(Brush *b, void * pVoid)
{
	fdocBrushEnumData *pData;

	pData = (fdocBrushEnumData *)pVoid;

	if ((Brush_GetModelId(b) == 0) && (!Brush_IsSubtract(b)) &&
	    ((pData->SuppressHidden == GE_FALSE) || (pData->pDoc->BrushIsVisible (b))))
	{
		++(pData->BrushCount);
	}
	return	GE_TRUE;
}


static geBoolean fdocBrushWriteCallback(Brush *b, void * pVoid)
{
	fdocBrushEnumData *pData;

	pData = (fdocBrushEnumData *)pVoid;

	if ((Brush_GetModelId(b) == 0) && (!Brush_IsSubtract(b)) &&
	    ((pData->SuppressHidden == GE_FALSE) || (pData->pDoc->BrushIsVisible (b))))
	{
		Brush_WriteToMap (b, pData->f, pData->VisDetail);
	}
	return	GE_TRUE;
}

typedef struct
{
	FILE *f;
	ModelList *Models;
	CEntityArray *Entities;
	CFusionDoc	*pDoc;
} fdocEntityToMapData;

static geBoolean fdocEntityToMap (CEntity &Ent, void *lParam)
{
	fdocEntityToMapData *pData;

	pData = (fdocEntityToMapData *)lParam;
	if (!Ent.IsCamera ())
	{
		CompileParamsType *CompileParams = Level_GetCompileParams (pData->pDoc->pLevel);
		if(CompileParams->SuppressHidden)
		{
			if(pData->pDoc->EntityIsVisible(&Ent))
			{
				Ent.WriteToMap (pData->f, pData->Models, pData->Entities, Level_GetEntityDefs (pData->pDoc->pLevel));
			}
		}
		else
		{
			Ent.WriteToMap (pData->f, pData->Models, pData->Entities, Level_GetEntityDefs (pData->pDoc->pLevel));
		}
	}
	return GE_TRUE;
}

typedef geBoolean (*EntityCount_CB)(const class CEntity *);
typedef struct fdocEntityCountDataTag
{
	int				Count;
	CFusionDoc		*pDoc;
} fdocEntityCountData;

static geBoolean fdocCountNonCameraEntities (CEntity &Ent, void *lParam)
{
	fdocEntityCountData	*ecnt	=(fdocEntityCountData *)lParam;
	if(!Ent.IsCamera())
	{
		CompileParamsType *CompileParams = Level_GetCompileParams (ecnt->pDoc->pLevel);
		if(CompileParams->SuppressHidden)
		{
			if(ecnt->pDoc->EntityIsVisible(&Ent))
			{
				ecnt->Count++;
			}
		}
		else
		{
			ecnt->Count++;
		}
	}
	return GE_TRUE;
}

// Write level data to map file for compiling
geBoolean CFusionDoc::WriteLevelToMap
	(
	  const char *Filename
	)
{
	static const int Version = 1;
	static const int ftag = 0x4642434E;
	int NumEntities;
	FILE *exfile;
	ModelInfo_Type *ModelInfo;
	BrushList *BList;
	fdocEntityCountData	ecnt;
	CompileParamsType *CompileParams = Level_GetCompileParams (pLevel);

	BList = Level_GetBrushes (pLevel);

	assert (BList != NULL);


	exfile = fopen (Filename, "wb");
	if (exfile == NULL)
	{
		return GE_FALSE;
	}

	ModelInfo = Level_GetModelInfo (pLevel);
	// write header information
	TypeIO_WriteInt (exfile, Version);
	TypeIO_WriteInt (exfile, ftag);
    /*
	  Number of entities to be written is the sum of:
	    World Entity		one
		Regular entities	one for each regular entity that isn't a camera
		Models				one for each model
		Type Information	one entity for each type
		----------------
	*/
	ecnt.Count	= 1;		// world entity
	ecnt.pDoc	= this;
	Level_EnumEntities(pLevel, &ecnt, ::fdocCountNonCameraEntities);

	NumEntities = ecnt.Count + ModelList_GetCount (ModelInfo->Models) +	// models
				   EntityTable_GetTypeCount (Level_GetEntityDefs (pLevel));			// type information

	TypeIO_WriteInt (exfile, NumEntities);

	//write the world entity first
	{
		BrushList *BList;

		// count brushes and write count to file
		fdocBrushEnumData EnumData;
			
		EnumData.SuppressHidden = CompileParams->SuppressHidden;
		EnumData.VisDetail = CompileParams->VisDetailBrushes;
		EnumData.pDoc = this;
		EnumData.BrushCount = 0;
		EnumData.f = exfile;

		BList = Level_GetBrushes (pLevel);

		BrushList_EnumCSGBrushes (BList, &EnumData, ::fdocBrushCountCallback);
	
		TypeIO_WriteInt (exfile, EnumData.BrushCount);

		// and write the model 0 brushes
		BrushList_EnumCSGBrushes (BList, &EnumData, ::fdocBrushWriteCallback);
	}

	{
		int nSkyFaces;
		int iFace;
		SkyFaceTexture *SkyFaces;
		geVec3d SkyRotationAxis;
		geFloat SkyRotationSpeed, SkyTextureScale;

		SkyFaces = Level_GetSkyInfo (pLevel, &SkyRotationAxis, &SkyRotationSpeed, &SkyTextureScale);

		// determine how many sky faces to write...
		nSkyFaces = 0;
		for (iFace = 0; iFace < 6; ++iFace)
		{
			SkyFaceTexture *pFace;

			pFace = &SkyFaces[iFace];
			if (pFace->Apply && (pFace->TextureName != NULL) && (*pFace->TextureName != '\0'))
			{
				++nSkyFaces;
			}
		}

		TypeIO_WriteInt (exfile, 0);  // no motion data for this model
		TypeIO_WriteInt (exfile, 4+nSkyFaces);  // numfields = #sky faces + TextureLibrary + SkyAxis + SkyRotation + SkyScaling
		TypeIO_WriteString (exfile, "TextureLib");
		TypeIO_WriteString (exfile, Level_GetWadPath (pLevel));
		
		// write sky information
		{
			char s[100];

			sprintf (s, "%f %f %f", SkyRotationAxis.X, SkyRotationAxis.Y, SkyRotationAxis.Z);
			TypeIO_WriteString (exfile, "SkyAxis");
			TypeIO_WriteString (exfile, s);

			sprintf (s, "%f", SkyRotationSpeed);
			TypeIO_WriteString (exfile, "SkyRotation");
			TypeIO_WriteString (exfile, s);

			TypeIO_WriteString (exfile, "SkyDrawScale");
			sprintf (s, "%f", SkyTextureScale);
			TypeIO_WriteString (exfile, s);
		}

		for (iFace = 0; iFace < 6; ++iFace)
		{
			SkyFaceTexture *pFace;

			pFace = &SkyFaces[iFace];
			if (pFace->Apply && (pFace->TextureName != NULL) && (*pFace->TextureName != '\0'))
			{
				TypeIO_WriteString (exfile, SkyFaceNames[iFace]);
				TypeIO_WriteString (exfile, pFace->TextureName);
			}
		}
	}

	// write the models
	ModelList_WriteToMap (ModelInfo->Models, exfile, BList, CompileParams->SuppressHidden, CompileParams->VisDetailBrushes);

	// and the entities
	{
		fdocEntityToMapData etmData;

		etmData.f = exfile;
		etmData.Models = ModelInfo->Models;
		etmData.Entities = Level_GetEntities (pLevel);
		etmData.pDoc	=this;

		Level_EnumEntities (pLevel, &etmData, ::fdocEntityToMap);
	}
		
	EntityTable_WriteTypesToMap (Level_GetEntityDefs (pLevel), exfile);
	fclose(exfile);


	return GE_TRUE;
}


void CFusionDoc::OnCompile() 
{
	geBoolean		NoErrors;
	CompilerErrorEnum CompileRslt;
	CompileParamsType *CompileParams = Level_GetCompileParams (pLevel);

	if (Compiler_CompileInProgress ())
	{
		if (AfxMessageBox ("Abort current compile?", MB_YESNO) == IDYES)
		{
			Compiler_CancelCompile ();
		}
		return;
	}

	// adjust tab dialog stuff...
	ConClear();
	SelectTab( CONSOLE_TAB ) ;

	BOOL KeepGoing;
	do
	{
		KeepGoing = FALSE;
		// display the dialog.  Exit if user cancels
		if (DoCompileDialog () != IDOK)
		{
			return;
		}

		NoErrors = GE_TRUE;

		// set wait cursor
		SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

		//ensure lists and trees are updated
		RebuildTrees();

		NoErrors = WriteLevelToMap (CompileParams->Filename);
		if (!NoErrors)
		{
			CString MsgString;

			MsgString.LoadString (IDS_MAPERROR);
			int rslt = AfxMessageBox (MsgString, MB_OKCANCEL);
			KeepGoing = (rslt == IDOK);
		}
	} while (KeepGoing);

	/*
	  The idea here is that I want to spawn a thread to do the compile.
	  When the compile is finished, we'll prompt the user and ask if he wants to
	  run preview.
	*/
	if (NoErrors)
	{
		CMDIChildWnd	*pMDIChild	=(CMDIChildWnd *)mpMainFrame->MDIGetActive();
		CFusionView	*cv	=(CFusionView *)pMDIChild->GetActiveView();

		CompileRslt = Compiler_StartThreadedCompile (CompileParams, cv->m_hWnd);
		NoErrors = (CompileRslt == COMPILER_ERROR_NONE);
	}

	// put cursor back
	SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

	if (!NoErrors)
	{
		AfxMessageBox ("Unable to start compile");
	}
}

void CFusionDoc::CompileDone (CompilerErrorEnum CompileRslt)
{
	static const char SuccessMessage[] = "Level compile completed successfully.";
	static const char ErrorMessage[] = "Error compiling level.";
	static const char PreviewMessage[] = "Run preview?";
	static const char CancelMessage[] = "Level compile cancelled by user";
	CompileParamsType *CompileParams = Level_GetCompileParams (pLevel);

	switch (CompileRslt)
	{
		case COMPILER_ERROR_NONE :
		{
			if (CompileParams->RunPreview)
			{
				char BigMessage[sizeof (SuccessMessage) + sizeof (PreviewMessage)];
				sprintf (BigMessage, "%s\r%s", SuccessMessage, PreviewMessage);
				if (AfxMessageBox (BigMessage, MB_YESNO) == IDYES)
				{
					char BspFilename[_MAX_PATH];
					char MotionFilename[_MAX_PATH];

					FilePath_SetExt (CompileParams->Filename, ".bsp", BspFilename);
					FilePath_SetExt (CompileParams->Filename, ".mot", MotionFilename);

					Compiler_RunPreview (BspFilename, MotionFilename, Prefs_GetPreviewPath (GetPrefs ()));
				}
			}
			else
			{
				AfxMessageBox (SuccessMessage);
			}
			break;
		}
		case COMPILER_ERROR_USERCANCEL :
			AfxMessageBox (CancelMessage);
			break;
		default :
			AfxMessageBox (ErrorMessage);
			break;
	}
}

struct ScaleEntityInfo
{
	const EntityTable *pEntityDefs;
	float ScaleValue;
};

static geBoolean fdocScaleEntityCallback (CEntity &Ent, void *lParam)
{
	ScaleEntityInfo *pInfo = (ScaleEntityInfo *)lParam;

	Ent.Scale (pInfo->ScaleValue, pInfo->pEntityDefs);
	return GE_TRUE;
}

void CFusionDoc::ScaleWorld(geFloat ScaleFactor)
{
	assert (ScaleFactor > 0.0f);

	// scale the brushes
	BrushList_Scale (Level_GetBrushes (pLevel), ScaleFactor);

	// scale the entity positions
	ScaleEntityInfo ScaleInfo;

	ScaleInfo.pEntityDefs = Level_GetEntityDefs (pLevel);
	ScaleInfo.ScaleValue = ScaleFactor;
	Level_EnumEntities (pLevel, &ScaleInfo, ::fdocScaleEntityCallback);


	// and scale the models
	ModelList_ScaleAll (Level_GetModelInfo(pLevel)->Models, ScaleFactor);

	UpdateAllViews( UAV_ALL3DVIEWS, NULL );
}


void CFusionDoc::OnEditPaste(){}
void CFusionDoc::OnEditCopy(){}

typedef struct
{
	int ModelId;
	BOOL Select;
	CFusionDoc *pDoc;
} fdocBrushSelectData;

static geBoolean fdocSelectBrushCallback (Brush *pBrush, void *lParam)
{
	fdocBrushSelectData *pData;

	pData = (fdocBrushSelectData *)lParam;
	if (Brush_GetModelId (pBrush) == pData->ModelId)
	{
		if (pData->Select)
		{
			SelBrushList_Add (pData->pDoc->pSelBrushes, pBrush);
		}
		else
		{
			SelBrushList_Remove (pData->pDoc->pSelBrushes, pBrush);
		}
	}
	return GE_TRUE;
}

void CFusionDoc::SelectModelBrushes
	(
	  BOOL Select,
	  int ModelId
	)
{
	fdocBrushSelectData bsData;

	bsData.Select = Select;
	bsData.ModelId = ModelId;
	bsData.pDoc = this;

	// Go through the brush list and add all brushes that have
	// this model's id to the selection list.
	Level_EnumBrushes (pLevel, &bsData, ::fdocSelectBrushCallback);

	UpdateSelected ();
}

/******************************************************
     BRUSH GROUP SUPPORT
*******************************************************/
BOOL CFusionDoc::MakeNewBrushGroup
	(
		CWnd * pParent
	)
// create a new brush group and add all currently-selected
// brushes and entities to the new group.
{
	CString	szName ;
	CString	szDefault ;
	int		NewGroupId = 0 ;
	int		ModalResult ;
	GroupListType *Groups = Level_GetGroups (pLevel);

	szDefault.LoadString( IDS_NEWGROUP ) ;
	CKeyEditDlg * GNameDlg = new CKeyEditDlg( pParent, szDefault, &szName ) ;

	ModalResult = GNameDlg->DoModal() ;
	delete GNameDlg ;

	if( ModalResult == IDOK )
	{
		szName.TrimLeft() ;
		szName.TrimRight() ;
		if( szName.IsEmpty() == FALSE )
		{
			NewGroupId = Group_AddToList( Groups );
			if( NewGroupId != 0 ) // You can't add the default group
			{
				Group_SetName( Groups, NewGroupId, szName ) ;
				mCurrentGroup = NewGroupId;
				AddSelToGroup() ;
			}//Good new Group ID
			else
			{
				AfxMessageBox( IDS_CREATEGROUPFAILED ) ;
			}
		}//Not empty name
	}// OK return from name dialog

	return NewGroupId ; // 0 for failed or new non-default group number

}/* CFusionDoc::MakeNewBrushGroup */

int CFusionDoc::FillBrushGroupCombo 
	(
	  CComboBox &cb
	)
{// Fill brush group combo box with group names and IDs
	int CurSel = LB_ERR;
	GroupList_FillCombobox( Level_GetGroups (pLevel), &cb, mCurrentGroup ) ;
	return CurSel;
}

typedef struct
{
	GroupListType *Groups;
	int CurrentGroup;
} fdocAddEntityData;
geBoolean fdocAddEntityToGroupCallback (CEntity &Ent, void *lParam)
{
	if (Ent.IsSelected ())
	{
		fdocAddEntityData *pData;

		pData = (fdocAddEntityData *)lParam;
		Group_AddEntity (pData->Groups, pData->CurrentGroup, &Ent);
	}
	return GE_TRUE;
}


void CFusionDoc::AddSelToGroup
	(
	  void
	)
// add selected brushes/entities to current group, no UI
{
	fdocAddEntityData entData;
	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	entData.Groups = Level_GetGroups (pLevel);
	entData.CurrentGroup = mCurrentGroup;

	// tag all selected brushes with this group id...
	for (int i = 0; i < NumSelBrushes; i++)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pSelBrushes, i);
		Group_AddBrush (entData.Groups, mCurrentGroup, pBrush);
	}
	// tag all selected entities with this group id...
	Level_EnumEntities (pLevel, &entData, ::fdocAddEntityToGroupCallback);
}

static geBoolean fdocRemoveEntityFromGroupCallback (CEntity &Ent, void *lParam)
{
	fdocAddEntityData *pData;

	pData = (fdocAddEntityData *)lParam;
	if (Ent.IsSelected () && (Ent.GetGroupId () == pData->CurrentGroup))
	{
		Group_RemoveEntity (pData->Groups, pData->CurrentGroup, &Ent);
	}
	return GE_TRUE;
}

void CFusionDoc::RemovesSelFromGroup
	(
	  void
	)
// removed selected brushes/entities from current group, do no UI
{
//	Brush	*	b;
	fdocAddEntityData entData;
	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);

	entData.Groups = Level_GetGroups (pLevel);
	entData.CurrentGroup = mCurrentGroup;

	for(int i = 0; i < NumSelBrushes; i++)
	{
		Brush *pBrush;

		pBrush = SelBrushList_GetBrush (pSelBrushes, i);
		if( Brush_GetGroupId(pBrush) == mCurrentGroup )
		{
			Group_RemoveBrush( entData.Groups, mCurrentGroup, pBrush );
		}
	}

	Level_EnumEntities (pLevel, &entData, ::fdocRemoveEntityFromGroupCallback);
}

void CFusionDoc::SelectGroupBrushes
	(
	  BOOL Select,
	  int WhichGroup
	) // select/unselect all brushes & entities in the current group
{
	BrushSelectCallbackData SelectData;

	SelectData.Select = Select;
	SelectData.WhichGroup = WhichGroup;
	SelectData.pDoc = this;

	Level_EnumBrushes (pLevel, &SelectData, ::BrushSelect ) ;
	Level_EnumEntities (pLevel, &SelectData, ::EntitySelect ) ;
	UpdateSelected ();		// update selection information
}


BOOL CFusionDoc::OneBrushSelectedOnly(void)
{
	return ((mModeTool==ID_TOOLS_TEMPLATE) || 
			((SelBrushList_GetSize (pSelBrushes)==1) && (NumSelEntities == 0) &&
			 (SelFaceList_GetSize (pSelFaces) == 0)));
}

void CFusionDoc::OnUpdateBrushSubtractfromworld(CCmdUI* pCmdUI) 
{
	BOOL EnableFlag;

	EnableFlag = ((mModeTool==ID_GENERALSELECT) && 
				  (SelBrushList_GetSize (pSelBrushes)==1) &&
				  (SelFaceList_GetSize (pSelFaces) == 0) && 
				  (NumSelEntities == 0));
	pCmdUI->Enable (EnableFlag);
}

void CFusionDoc::OnUpdateThingAttributes(CCmdUI* pCmdUI) 
{
	if(mModeTool==ID_GENERALSELECT)
	{
		switch (mAdjustMode)
		{
			case ADJUST_MODE_BRUSH :
				pCmdUI->Enable (SelBrushList_GetSize (pSelBrushes) > 0);
				break;
			case ADJUST_MODE_FACE :
				pCmdUI->Enable (SelFaceList_GetSize (pSelFaces) > 0);
				break;
			default :
				assert (0);
				break;
		}
	} 
	else 
	{
		pCmdUI->Enable(0);
	}
}

void CFusionDoc::OnUpdateToolsBrushShowassociatedentity(CCmdUI* pCmdUI) 
{
	if((mModeTool==ID_GENERALSELECT) && (mAdjustMode==ADJUST_MODE_BRUSH))
	{
		pCmdUI->Enable (SelBrushList_GetSize (pSelBrushes)==1);
	}
	else 
	{
		pCmdUI->Enable(0);
	}
}

void CFusionDoc::OnUpdateEntitiesEditor(CCmdUI* pCmdUI) 
{
	BOOL	bEnable ;

	bEnable = (	Level_GetEntities (pLevel)->GetSize() ) ? TRUE : FALSE ;
	pCmdUI->Enable( bEnable ) ;
}

static geBoolean GetObjectName (char *Name, char *Path, CFusionDoc *pDoc)
{
	CKeyEditDlg *pEditDlg;
	CString ObjName;
	char ObjectsDir[MAX_PATH];
	char WorkPath[MAX_PATH];
	int rslt;

	strcpy (ObjectsDir, Prefs_GetObjectsDir (pDoc->GetPrefs ()));

	do
	{
		rslt = IDYES;
		pEditDlg = new CKeyEditDlg (AfxGetMainWnd (), "Enter object name", &ObjName);
		if (pEditDlg->DoModal () != IDOK)
		{
			return GE_FALSE;
		}
		// see if an object of this name already exists
		::FilePath_AppendName (ObjectsDir, ObjName, WorkPath);
		::FilePath_SetExt (WorkPath, ".3dt", WorkPath);
		if (_access (WorkPath, 0) == 0)
		{
			static const char Prompt[] =
				"An object of that name already exists.\r"
				"Do you want to replace it?";

			rslt = AfxMessageBox (Prompt, MB_YESNOCANCEL);
		}
	} while (rslt == IDNO);

	if (rslt == IDYES)
	{
		strcpy (Path, WorkPath);
		::FilePath_GetName (Path, Name);
		return GE_TRUE;
	}
	else
	{
		return GE_FALSE;
	}
}	

typedef struct
{
	Level *NewLevel;
	Level *OldLevel;
} AddPremadeEnumData;

static void fdocAddReferencedGroup (AddPremadeEnumData *pData, int GroupId)
{

	if (Level_GetGroup (pData->NewLevel, GroupId) == NULL)
	{
		// group doesn't exist in the new level, so add it
		Group *OldGroup, *NewGroup;

		OldGroup = Level_GetGroup (pData->OldLevel, GroupId);
		NewGroup = Group_Clone (OldGroup);
		Level_AddGroup (pData->NewLevel, NewGroup);
	}
}

static void fdocAddReferencedModel (AddPremadeEnumData *pData, int ModelId)
{
	if ((ModelId != 0) && (Level_GetModel (pData->NewLevel, ModelId) == NULL))
	{
		Model *OldModel, *NewModel;

		OldModel = Level_GetModel (pData->OldLevel, ModelId);
		NewModel = Model_Clone (OldModel);
		Level_AddModel (pData->NewLevel, NewModel);
	}
}

static geBoolean fdocAddSelectedEntities (CEntity &Ent, void *lParam)
{
	AddPremadeEnumData *pData;

	pData = (AddPremadeEnumData *)lParam;

	if (Ent.IsSelected ())
	{
		Level_AddEntity (pData->NewLevel, Ent);
		::fdocAddReferencedGroup (pData, Ent.GetGroupId ());
	}
	return GE_TRUE;
}

void CFusionDoc::OnNewLibObject() 
{
	// save currently-selected stuff to named lib object.
	Level *NewLevel;
	char NewObjectName[MAX_PATH];
	char NewObjectFullPath[MAX_PATH];
	int i;
	AddPremadeEnumData EnumData;

	if (!::GetObjectName (NewObjectName, NewObjectFullPath, this))
	{
		return;
	}

	NewLevel = Level_Create (Level_GetWadPath (pLevel), Level_GetHeadersDirectory (pLevel));
	if (NewLevel == NULL)
	{
		AfxMessageBox ("Error creating object.");
		return;
	}

	EnumData.NewLevel = NewLevel;
	EnumData.OldLevel = pLevel;

	int NumSelBrushes = SelBrushList_GetSize (pSelBrushes);
	// add all selected brushes and entities to the new level
	for (i = 0; i < NumSelBrushes; ++i)
	{
		Brush *NewBrush;
		Brush *OldBrush;

		OldBrush = SelBrushList_GetBrush (pSelBrushes, i);
		NewBrush = Brush_Clone (OldBrush);
		Level_AppendBrush (NewLevel, NewBrush);

		// add any group or model that's referenced
		::fdocAddReferencedModel (&EnumData, Brush_GetModelId (NewBrush));
		::fdocAddReferencedGroup (&EnumData, Brush_GetGroupId (NewBrush));
	}

	Level_EnumEntities (pLevel, &EnumData, ::fdocAddSelectedEntities);

	// SelectedGeoCenter contains center.  We're going to translate
	// the center to the origin.  That means that every brush and
	// entity will be moved so that the SelectedGeoCenter of the premade
	// object will be the origin.
	{
		geVec3d VecXlate;

		geVec3d_Scale (&SelectedGeoCenter, -1.0f, &VecXlate);
		Level_TranslateAll (NewLevel, &VecXlate);
	}

	// ok, everything's added.  Write it to the file.
	if (!Level_WriteToFile (NewLevel, NewObjectFullPath))
	{
		AfxMessageBox ("Error writing object to file.");
	}
	else
	{
		// add level name to objects list...
		mpMainFrame->m_wndTabControls->m_pBrushEntityDialog->SetupObjectListCombo ();
	}

	Level_Destroy (&NewLevel);
}

typedef struct
{
	CFusionDoc *pDoc;
	Level *NewLevel;
} fdocAddPremadeData;

static geBoolean fdocAddPremadeEntity (CEntity &Ent, void *lParam)
{
	fdocAddPremadeData *pData;
	CEntityArray *Entities;
	CEntity *NewEnt;
	int Index;

	pData = (fdocAddPremadeData *)lParam;

	Index = Level_AddEntity (pData->pDoc->pLevel, Ent);
	Entities = Level_GetEntities (pData->pDoc->pLevel);
	NewEnt = &((*Entities)[Index]);

	pData->pDoc->SelectEntity (NewEnt);

	return GE_TRUE;
}

// -------------------------------------------------------------------------------
// places the specified object down into the level at the specified location
// TODO: later want to make it so that newly added brushes/entities are all placed
// in a single group and selected so the user can immediately begin to move
// and adjust the object within the level		
geBoolean CFusionDoc::PlaceObject( const char *ObjectName, const geVec3d *location )
{
	char WorkPath[MAX_PATH];

	::FilePath_AppendName (Prefs_GetObjectsDir (GetPrefs ()), ObjectName, WorkPath);
	::FilePath_SetExt (WorkPath, ".3dt", WorkPath);

	return ImportFile (WorkPath, location);
}

geBoolean CFusionDoc::ImportFile (const char *PathName, const geVec3d *location)
{
	const char		*ErrMsg = "#ERROR#";
	Level			*NewLevel;
	EntityViewList	*pEntityView;
	int				i;
	const Prefs *pPrefs = GetPrefs ();

	NewLevel = Level_CreateFromFile (PathName, &ErrMsg, Prefs_GetHeadersList (pPrefs));
	if (NewLevel == NULL)
	{
		AfxMessageBox (ErrMsg);
		return GE_FALSE;
	}


	// Unselects everything so that brushes/entities can be added
	ResetAllSelections ();

	// move the object to the new position
	Level_TranslateAll (NewLevel, location);

	{
		// copy any groups (except default...)
		int GroupId;
		GroupListType *NewGroups;
		GroupListType *OldGroups;
		GroupIterator gi;

		OldGroups = Level_GetGroups (pLevel);
		NewGroups = Level_GetGroups (NewLevel);

		Level_CollapseGroups (pLevel, 1);
		Level_CollapseGroups (NewLevel, Group_GetCount (OldGroups));

		GroupId = Group_GetFirstId (NewGroups, &gi);
		while (GroupId != NO_MORE_GROUPS)
		{
			if (GroupId != 0)	// don't add default group
			{
				Group *pGroup;
				Group *NewGroup;
				char GroupName[MAX_PATH];

				pGroup = GroupList_GetFromId (NewGroups, GroupId);
				strcpy (GroupName, Group_GetName (pGroup));

				// make sure that group name doesn't conflict
				while (Group_GetIdFromName (OldGroups, GroupName) != NO_MORE_GROUPS)
				{
					strcat (GroupName, "x");
				}

				NewGroup = Group_Clone (pGroup);
				Group_SetGroupName (NewGroup, GroupName);

				GroupList_Add (OldGroups, NewGroup);
			}
			GroupId = Group_GetNextId (NewGroups, &gi);
		}
	}

	{
		// copy models
		ModelList *NewModels, *OldModels;
		ModelIterator mi;
		Model *pModel;

		OldModels = Level_GetModelInfo (pLevel)->Models;
		NewModels = Level_GetModelInfo (NewLevel)->Models;

		Level_CollapseModels (pLevel, 1);
		Level_CollapseModels (NewLevel, ModelList_GetCount (OldModels) + 1);

		pModel = ModelList_GetFirst (NewModels, &mi);
		while (pModel != NULL)
		{
			Model *NewModel;
			char ModelName[MAX_PATH];

			// make sure names don't clash
			strcpy (ModelName, Model_GetName (pModel));
			while (ModelList_FindByName (OldModels, ModelName) != NULL)
			{
				strcat (ModelName, "x");
			}
			NewModel = Model_Clone (pModel);
			Model_SetName (NewModel, ModelName);
			ModelList_AddModel (OldModels, NewModel);

			pModel = ModelList_GetNext (NewModels, &mi);
		}

	}


	{
		fdocAddPremadeData AddPremadeData;

		AddPremadeData.pDoc = this;
		AddPremadeData.NewLevel = NewLevel;

		// add entities
		Level_EnumEntities (NewLevel, &AddPremadeData, ::fdocAddPremadeEntity);
	}

	{
		// add brushes
		Brush *pBrush, *NewBrush;
		BrushList *NewBrushes;
		BrushIterator bi;

		// fixup DIB ids on brush faces
		Level_EnumLeafBrushes (NewLevel, pLevel, Level_FaceFixupCallback);

		// Move brushes from loaded level to doc's level
		NewBrushes = Level_GetBrushes (NewLevel);
		pBrush = BrushList_GetFirst (NewBrushes, &bi);
		while (pBrush != NULL)
		{
			NewBrush = pBrush;
			pBrush = BrushList_GetNext (&bi);
			Level_RemoveBrush (NewLevel, NewBrush);
			Level_AppendBrush (pLevel, NewBrush);
		}
	}

	// update entity visibility info
	pEntityView	=Level_GetEntityVisibilityInfo (pLevel);
	for (i = 0; i < pEntityView->nEntries; ++i)
	{
		Level_EnumEntities (pLevel, &pEntityView->pEntries[i], ::fdocSetEntityVisibility);
	}

	Level_Destroy (&NewLevel);
	return GE_TRUE;
}


// -----------------------------------------------------------------------------
// insures all of the Dib ID's in the brush are correct
// I would like to have put this in the brush.cpp module,  but fixing up the dib ID's requires
// a function here in the doc and I wanted to avoid coupling brush.cpp to 
// fusiondoc.cpp
// returns 1 on success,  0 on failure
geBoolean CFusionDoc::FixUpBrushDibIDs(Brush *b)
{
	ASSERT(b);
	geBoolean NoErrors;

	NoErrors = GE_TRUE;
	// flags failure to find a particular texture

	// loop over all the faces in the brush
	for (int i = 0; i < Brush_GetNumFaces(b); i++ )
	{
		Face *pFace;
		const char *pName;
		uint16 DibId;

		pFace = Brush_GetFace (b, i);
		pName = Face_GetTextureName (pFace);
		DibId = Level_GetDibId (pLevel, pName);
		if (DibId == 0xffff)
		{
			DibId = 1;
			NoErrors = GE_FALSE;
		}
		Face_SetTextureDibId (pFace, DibId);
		if (NoErrors)
		{
			const WadFileEntry * const pbmp = Level_GetWadBitmap (pLevel, pName);

			Face_SetTextureSize (pFace, pbmp->Width, pbmp->Height);
		}
	}

	return NoErrors;
}

#pragma warning (disable:4100)
void CFusionDoc::SnapScaleNearest(int sides, int inidx, ViewVars *v)
{
	geFloat	bsnap;

	mLastOp		=BRUSH_SCALE;


	bsnap = 1.0f ;
	if (Level_UseGrid (pLevel))
	{
		bsnap = Level_GetGridSnapSize (pLevel);
	}

	if(mModeTool == ID_TOOLS_TEMPLATE)
	{
		Brush_SnapScaleNearest(CurBrush, bsnap, sides, inidx, &FinalScale, &ScaleNum);
	}
	else
	{
		int i;
		int NumBrushes = SelBrushList_GetSize (pTempSelBrushes);

		for (i = 0; i < NumBrushes; ++i)
		{
			Brush *pBrush = SelBrushList_GetBrush (pTempSelBrushes, i);
			Brush_SnapScaleNearest(pBrush, bsnap, sides, inidx, &FinalScale, &ScaleNum);
		}
	}
}
#pragma warning (default:4100)

typedef struct
{
	GroupListType *Groups;
	geBoolean AllGood;
	const EntityTable *pEntityTable;
} fdocEntityValidateData;

static geBoolean fdocValidateEntity (CEntity &Ent, void *lParam)
{
	fdocEntityValidateData *pData;
	int			nGroupId ;
	CString		cstr ;

	pData = (fdocEntityValidateData *)lParam;

	// Validate the entity type (Class)	
	cstr = Ent.GetClassname() ;
	if (!EntityTable_IsValidEntityType (pData->pEntityTable, cstr))
	{
		cstr = Ent.GetName() ;
		ConPrintf ("Entity %s: type not found.\n", cstr );
		pData->AllGood = GE_FALSE ;
	}

	nGroupId = Ent.GetGroupId() ;
	if (nGroupId != 0)
	{
		if (GroupList_IsValidId (pData->Groups, nGroupId) == FALSE)
		{
			cstr = Ent.GetName() ;
			ConPrintf("Entity %s: Group %d missing--set to no group\n", cstr, nGroupId );
			Ent.SetGroupId( 0 ) ;
			pData->AllGood = GE_FALSE;
		}
	}

	return GE_TRUE;
}

geBoolean CFusionDoc::ValidateEntities( void )
{
	fdocEntityValidateData evData;

	evData.Groups = Level_GetGroups (pLevel);
	evData.AllGood = GE_TRUE;
	evData.pEntityTable = Level_GetEntityDefs (pLevel);

	Level_EnumEntities (pLevel, &evData, ::fdocValidateEntity);

	return evData.AllGood;
}

typedef struct
{
	GroupListType *Groups;
	geBoolean AllGood;
} fdocBrushValidateData;

geBoolean fdocValidateBrush (Brush *pBrush, void *lParam)
{
	fdocBrushValidateData *pData;
	int nGroupId;

	pData = (fdocBrushValidateData *)lParam;
	nGroupId = Brush_GetGroupId( pBrush ) ;
	if( nGroupId != 0 )
	{
		if( GroupList_IsValidId( pData->Groups, nGroupId ) == FALSE )
		{
			char const *pName;

			pName = Brush_GetName (pBrush);
			ConPrintf("Brush %s: Group %d missing--set to no group\n", pName, nGroupId );
			Brush_SetGroupId( pBrush, 0 ) ;
			pData->AllGood = GE_FALSE ;
		}
	}
	return GE_TRUE;
}

geBoolean CFusionDoc::ValidateBrushes( void )
{
	fdocBrushValidateData bvData;
	
	bvData.Groups = Level_GetGroups (pLevel);
	bvData.AllGood = GE_TRUE;
		
	Level_EnumBrushes (pLevel, &bvData, ::fdocValidateBrush);

	return bvData.AllGood ;
}


void CFusionDoc::SelectTab( int nTabIndex )
{
	if( mpMainFrame )
	{
		mpMainFrame->SelectTab( nTabIndex ) ;
	}
}/* CFusionDoc::SelectTab */

/* EOF: FusionDoc.cpp */

void CFusionDoc::OnFileOpen() 
{
	static const char	FDTitle[]	="Open";

	CFileDialog dlg(TRUE, "3dt", NULL, OFN_HIDEREADONLY	| OFN_OVERWRITEPROMPT,
		"GEDIT Files (*.3DT)|*.3DT|MAP Files (*.MAP)|*.MAP|Leak Files (*.PNT)|*.PNT||");

	dlg.m_ofn.lpstrTitle	=FDTitle;
	dlg.m_ofn.lpstrInitialDir = LastPath;
	if(dlg.DoModal()==IDOK)
	{
		switch(dlg.m_ofn.nFilterIndex)
		{
		case 1 :
			AfxGetApp()->OpenDocumentFile(dlg.GetPathName());
			break;
		case 2 :
			LoadMapFile(dlg.GetPathName());
			break;
		case 3 :
			bLeakLoaded	=LoadLeakFile(dlg.GetPathName());
			break;
		default:
			assert(0);
		}
	}
}

void CFusionDoc::OnFileImport() 
{
	static const char FDTitle[] = "Import";
	CFileDialog dlg(TRUE, "3dt", NULL, OFN_OVERWRITEPROMPT,	"GEDIT Files (*.3DT)|*.3DT||");

	dlg.m_ofn.lpstrTitle = FDTitle;	
	if (dlg.DoModal () == IDOK)
	{
		if (dlg.m_ofn.nFilterIndex == 1)
		{
			char Name[MAX_PATH];
			geVec3d loc;

			geVec3d_Clear (&loc);
			::FilePath_SetExt (dlg.GetPathName (), ".3dt", Name);
			ImportFile (Name, &loc);
		}
	}
}

geBoolean	CFusionDoc::LoadLeakFile(const char *Filename)
{
	CFile	Infile;
	int		sig;
	int		PointsToRead;

	if(!Infile.Open(Filename, CFile::modeRead))
	{
		AfxMessageBox("Error opening the Leak File!", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	Infile.Read(&sig, sizeof(int));

	if(sig!=0x4b41454c)
	{
		AfxMessageBox("File is not a leakfile!", MB_OK|MB_ICONEXCLAMATION);
		return FALSE;
	}

	Infile.Read(&PointsToRead, sizeof(int));

	ConPrintf("Loaded leak file %s with %d Points...\n", Filename, PointsToRead);

	NumLeakPoints = (PointsToRead > 1) ? PointsToRead : 2;
	LeakPoints	=(geVec3d *)geRam_Allocate(sizeof(geVec3d)*NumLeakPoints);

	if (!LeakPoints)
		return FALSE;

	Infile.Read(LeakPoints, sizeof(geVec3d)*PointsToRead);
	if (PointsToRead == 1)
	{
		geVec3d_Clear (&LeakPoints[1]);
	}

	return TRUE;
}
// Quake 2 flag sets for importing
enum	Q2ContentsFlags
{
	CONTENTS_SOLID			=1,
	CONTENTS_WINDOW			=2,
	CONTENTS_AUX			=4,
	CONTENTS_LAVA			=8,
	CONTENTS_SLIME			=16,
	CONTENTS_WATER			=32,
	CONTENTS_MIST			=64,
	LAST_VISIBLE_CONTENTS	=64,
	CONTENTS_PLAYERCLIP		=0x10000,
	CONTENTS_MONSTERCLIP	=0x20000,
	CONTENTS_CURRENT_0		=0x40000,
	CONTENTS_CURRENT_90		=0x80000,
	CONTENTS_CURRENT_180	=0x100000,
	CONTENTS_CURRENT_270	=0x200000,
	CONTENTS_CURRENT_UP		=0x400000,
	CONTENTS_CURRENT_DOWN	=0x800000,
	CONTENTS_ORIGIN			=0x1000000,
	CONTENTS_MONSTER		=0x2000000,
	CONTENTS_DEADMONSTER	=0x4000000,
	CONTENTS_DETAIL			=0x8000000,
	CONTENTS_TRANSLUCENT	=0x10000000
};

enum	Q2SurfaceFlags
{
	SURF_LIGHT		=0x1,		// value will hold the light strength
	SURF_SLICK		=0x2,		// effects game physics
	SURF_SKY		=0x4,		// don't draw, but add to skybox
	SURF_WARP		=0x8,		// turbulent water warp
	SURF_TRANS33	=0x10,
	SURF_TRANS66	=0x20,
	SURF_FLOWING	=0x40,	// scroll towards angle
	SURF_NODRAW		=0x80	// don't bother referencing the texture
};

static geBoolean fdocCheckAddFace (FaceList **ppList, Face *f)
{
	if (FaceList_GetNumFaces (*ppList) >= FaceList_GetFaceLimit (*ppList))
	{
		if (!FaceList_Grow (ppList))
		{
			return GE_FALSE;
		}
	}
	FaceList_AddFace (*ppList, f);
	return GE_TRUE;
}


geBoolean	CFusionDoc::LoadMapFile(const char *FileName)
{
	FaceList	*fl, *mfl;
	Face		*f;
	Brush		*b;
	FILE		*mf;

	geVec3d		FaceVerts[3];
	int			i, k, bc, sx, sy, rot, contents, surfflags, value;
	geFloat		scx, scy;
	char		*sp = NULL;
	char		szTex[_MAX_PATH];
	uint16		dibid;

	assert (FileName != NULL);

	mf	=fopen(FileName, "r");
	mfl	=FaceList_Create(64);

	if(!mf)
	{
		AfxMessageBox (IDS_CANT_OPEN_FILE);
		return GE_FALSE;
	}
	//get open brace
	for(bc=fgetc(mf);bc!=EOF && bc!='{';bc=fgetc(mf));
	if(bc=='{')
	{
		for(;;)
		{
			//get open brace
			for(bc=fgetc(mf);bc!=EOF && bc!='{' && bc!='}';bc=fgetc(mf));
			if(bc=='{')
			{
				surfflags	=value=0;
				contents	=CONTENTS_SOLID;	//default
				for(k=0;;)
				{

					for(i=0;i<3;i++)
					{
						//get ( or }
						for(bc=fgetc(mf);bc!=EOF && bc!='}' && bc!='(';bc=fgetc(mf));
						if(bc=='(')
						{
							fscanf(mf, "%f%f%f", &FaceVerts[i].X, &FaceVerts[i].Z, &FaceVerts[i].Y);
							FaceVerts[i].Z	=-FaceVerts[i].Z;
						}
						else
						{
							break;
						}
					}
					if(i==3)
					{
						//get )
						for(bc=fgetc(mf);bc!=EOF && bc!=')';bc=fgetc(mf));
						fscanf(mf, "%s %i %i %i %f %f", szTex, &sx, &sy, &rot, &scx, &scy);
						dibid	=Level_GetDibId(pLevel, szTex);
						if(dibid == 0xffff)
						{
							//try stripping /
							sp	=strrchr(szTex, '/');
							if(sp)
							{
								sp++;
								dibid	=Level_GetDibId(pLevel, sp);
							}
						}
						if(!sp)
						{
							sp	=szTex;
						}
						f	=Face_Create(3, FaceVerts, dibid);
						for(;*sp=='#' || *sp=='*' || *sp=='+';sp++);	//strip illegal chars
						if(f)
						{
							Face_SetTextureName(f, sp);
							Face_SetTextureScale(f, scx, scy);
							Face_SetTextureShift(f, sx, sy);
							Face_SetTextureRotate(f, (float)rot);
						}
						//look for flags
						for(bc=fgetc(mf);bc!=EOF && bc <=32 && bc && bc!='\n';bc=fgetc(mf));
						if(bc!='\n' && bc)
						{
							ungetc(bc, mf);
							fscanf(mf, "%i %i %i", &contents, &surfflags, &value);
							if(f)
							{
								Face_SetLight(f, (surfflags	&	SURF_LIGHT));
								Face_SetSky(f, (surfflags	&	SURF_SKY));
								Face_SetLightIntensity(f, value);
							}
						}
						if(f)
						{
							if (!fdocCheckAddFace (&mfl, f))
							{
								// this is pretty ugly.
								// If we can't add the face, then set num faces to 0 and exit.
								k = 0;
								break;
							}
//							FaceList_AddFace(mfl, f);
							k++;
						}
					}
					else
					{
						break;
					}
				}
				if(k > 3)
				{
					fl	=FaceList_Create(k);
					for(i=0;i<k;i++)
					{
						FaceList_AddFace(fl, Face_Clone(FaceList_GetFace(mfl, i)));
					}
					FaceList_Destroy(&mfl);
					mfl	=FaceList_Create(64);
					b	=Brush_Create(BRUSH_LEAF, fl, NULL);
					Brush_SealFaces(&b);
					if(b)
					{
						Level_AppendBrush	(pLevel, b);
						Brush_SetSolid		(b, (contents	&	CONTENTS_SOLID));
						Brush_SetWindow		(b, (contents	&	CONTENTS_WINDOW));
						Brush_SetWavy		(b, (contents	&	CONTENTS_WATER));
						Brush_SetTranslucent(b, (contents	&	CONTENTS_WATER));
						Brush_SetClip		(b, (contents	&	CONTENTS_PLAYERCLIP));
						Brush_SetClip		(b, (contents	&	CONTENTS_MONSTERCLIP));
						Brush_SetDetail		(b, (contents	&	CONTENTS_DETAIL));
						Brush_SetTranslucent(b, (contents	&	CONTENTS_TRANSLUCENT));
					}
				}
			}
			else if(bc=='}')
			{
				break;
			}
		}
	}
	char	Key[255], Value[255];
	//get open brace
	for(bc=fgetc(mf);bc!=EOF && bc!='{';bc=fgetc(mf));
	if(bc=='{')
	{
		for(;;)
		{
			CEntity	NewEnt;
			for(;;)
			{
				for(bc=fgetc(mf);bc!=EOF && bc!='"' && bc!='}' && bc!='{';bc=fgetc(mf));
				if(bc=='"')
				{
					for(i=0;;i++)
					{
						bc		=fgetc(mf);
						if(bc!='"')
						{
							Key[i]	=(char)bc;	//hope theres no high nibble info
						}
						else
						{
							Key[i]	=(char)0;
							break;
						}
					}
					for(bc=fgetc(mf);bc!=EOF && bc!='"' && bc!='}';bc=fgetc(mf));
					if(bc!='"')
					{
						break;
					}
					for(i=0;;i++)
					{
						bc		=fgetc(mf);
						if(bc!='"')
						{
							Value[i]=(char)bc;
						}
						else
						{
							Value[i]=(char)0;
							break;
						}
					}
//					ConPrintf("Key: %s : Value: %s\n", Key, Value);
					if(!stricmp(Key, "_color"))
					{
						strcpy(Key, "color");
						geVec3d	TempVec;
						sscanf((char const *)Value, "%f %f %f", &TempVec.X, &TempVec.Y, &TempVec.Z);
						sprintf(Value, "%d %d %d",
							Units_Round(TempVec.X * 255.0f),
							Units_Round(TempVec.Y * 255.0f),
							Units_Round(TempVec.Z * 255.0f));					
					}
					else if(!strnicmp(Value, "info_player", 11))
					{
						strcpy(Value, "PlayerStart");
					}
					else if(!strnicmp(Value, "monster_", 8))
					{
						strcpy(Value, "AIPlayerStart");
					}
					else if(!stricmp(Key, "origin"))
					{
						geVec3d	TempVec;
						sscanf((char const *)Value, "%f %f %f", &TempVec.X, &TempVec.Z, &TempVec.Y);
						TempVec.Z	=-TempVec.Z;
						sprintf(Value, "%d %d %d", Units_Round(TempVec.X), Units_Round(TempVec.Y), Units_Round(TempVec.Z));					
					}
					else if(!strnicmp(Value, "weapon_", 7))
					{
						strcpy(Value, "PowerUp");
					}
					else if(!strnicmp(Value, "path_", 5))
					{
						strcpy(Value, "AIMapWayPoint");
					}
					NewEnt.SetKeyValue(Key, Value);
				}
				else if(bc=='{' && NewEnt.GetNumKeyValuePairs())	//brush models
				{
					geXForm3d	XfmDelta;
					for(;;)
					{
						if(bc=='{')
						{
							surfflags	=value=0;
							contents	=CONTENTS_SOLID;	//default
							for(k=0;;)
							{
								for(i=0;i<3;i++)
								{
									//get ( or }
									for(bc=fgetc(mf);bc!=EOF && bc!='}' && bc!='(';bc=fgetc(mf));
									if(bc=='(')
									{
										fscanf(mf, "%f%f%f", &FaceVerts[i].X, &FaceVerts[i].Z, &FaceVerts[i].Y);
										FaceVerts[i].Z	=-FaceVerts[i].Z;
									}
									else
									{
										break;
									}
								}
								if(i==3)
								{
									//get )
									for(bc=fgetc(mf);bc!=EOF && bc!=')';bc=fgetc(mf));
									fscanf(mf, "%s %i %i %i %f %f", szTex, &sx, &sy, &rot, &scx, &scy);
									dibid	=Level_GetDibId(pLevel, szTex);
									if(dibid == 0xffff)
									{
										//try stripping /
										sp	=strrchr(szTex, '/');
										if(sp)
										{
											sp++;
											dibid	=Level_GetDibId(pLevel, sp);
										}
									}
									if(!sp)
									{
										sp	=szTex;
									}
									f	=Face_Create(3, FaceVerts, dibid);
									for(;*sp=='#' || *sp=='*' || *sp=='+';sp++);	//strip illegal chars
									Face_SetTextureName(f, sp);
									Face_SetTextureScale(f, scx, scy);
									Face_SetTextureShift(f, sx, sy);
									Face_SetTextureRotate(f, (float)rot);

									//look for flags
									for(bc=fgetc(mf);bc!=EOF && bc <=32 && bc && bc!='\n';bc=fgetc(mf));
									if(bc!='\n' && bc)
									{
										ungetc(bc, mf);
										fscanf(mf, "%i %i %i", &contents, &surfflags, &value);
										Face_SetLight(f, (surfflags	&	SURF_LIGHT));
										Face_SetSky(f, (surfflags	&	SURF_SKY));
										Face_SetLightIntensity(f, value);
									}
									if (!fdocCheckAddFace (&mfl, f))
									{
										k = 0;
										break;
									}
									k++;
								}
								else
								{
									break;
								}
							}
							if(k > 3)
							{
								fl	=FaceList_Create(k);
								for(i=0;i<k;i++)
								{
									FaceList_AddFace(fl, Face_Clone(FaceList_GetFace(mfl, i)));
								}
								FaceList_Destroy(&mfl);
								mfl	=FaceList_Create(64);
								b	=Brush_Create(BRUSH_LEAF, fl, NULL);
								Brush_SealFaces(&b);
								Brush_SealFaces(&b);
								if(b)
								{
									Level_AppendBrush	(pLevel, b);
									Brush_SetSolid		(b, (contents	&	CONTENTS_SOLID));
									Brush_SetWindow		(b, (contents	&	CONTENTS_WINDOW));
									Brush_SetWavy		(b, (contents	&	CONTENTS_WATER));
									Brush_SetTranslucent(b, (contents	&	CONTENTS_WATER));
									Brush_SetClip		(b, (contents	&	CONTENTS_PLAYERCLIP));
									Brush_SetClip		(b, (contents	&	CONTENTS_MONSTERCLIP));
									Brush_SetDetail		(b, (contents	&	CONTENTS_DETAIL));
									Brush_SetTranslucent(b, (contents	&	CONTENTS_TRANSLUCENT));
								}
								SelBrushList_Add (pSelBrushes, b);
							}
						}
						else if(bc=='}')
						{
							break;
						}
					}
					strcpy(Key, NewEnt.GetClassname());
					i	=0;

					ModelInfo_Type *ModelInfo = Level_GetModelInfo (pLevel);
					while (ModelList_FindByName (ModelInfo->Models, Key) != NULL)
					{
						sprintf(Key, "%s%d", NewEnt.GetClassname(), i++);
					}
					// add the new model to the list.
					// This will set the model id fields in the model's brushes
					if (ModelList_Add (ModelInfo->Models, Key, pSelBrushes))
					{
						Model *m;

						// get current model and update its object-to-world transform
						m = ModelList_FindByName (ModelInfo->Models, Key);
						assert (m != NULL);
						ModelInfo->CurrentModel	= Model_GetId (m);

//						UpdateModelsList ();
						geXForm3d_SetIdentity (&XfmDelta);
						// we add the first key (identity)
						// (and don't allow it to be edited/deleted)
						Model_AddKeyframe(m, 0.0f, &XfmDelta);
					}
					SelBrushList_RemoveAll (pSelBrushes);
				}
				else if(bc==EOF || bc=='}')
				{
					break;
				}
			}
			if(bc==EOF)
			{
				break;
			}
			else if(bc=='}')
			{
				NewEnt.SetGroupId ( 0 );
				NewEnt.UpdateOrigin (Level_GetEntityDefs (pLevel));
				Level_AddEntity (pLevel, NewEnt);
			}
		}
	}

	fclose(mf);
	if (mfl != NULL)
	{
		FaceList_Destroy (&mfl);
	}

	return	TRUE;
}

void CFusionDoc::OnViewShowClip() 
{
	bShowClipBrushes	=!(bShowClipBrushes);	
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::OnUpdateViewShowClip(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(bShowClipBrushes);
}

void CFusionDoc::OnViewShowDetail() 
{
	bShowDetailBrushes	=!(bShowDetailBrushes);	
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::OnUpdateViewShowDetail(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(bShowDetailBrushes);
}

void CFusionDoc::OnViewShowHint() 
{
	bShowHintBrushes	=!(bShowHintBrushes);	
	UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}

void CFusionDoc::OnUpdateViewShowHint(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck(bShowHintBrushes);
}

void CFusionDoc::OnToolsBrushAttributes() 
{
	if ((mModeTool == ID_GENERALSELECT) && (mAdjustMode == ADJUST_MODE_BRUSH))
	{
		if ((SelBrushList_GetSize (pSelBrushes) > 0) && 
		    (mAdjustMode == ADJUST_MODE_BRUSH) && (mpBrushAttributes == NULL))
		{
			mpBrushAttributes = new CBrushAttributesDialog (this);
		}
		SetModifiedFlag ();
	}	
}

void CFusionDoc::OnUpdateToolsBrushAttributes(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable ((mModeTool == ID_GENERALSELECT) && 
					(mAdjustMode == ADJUST_MODE_BRUSH) && 
					(SelBrushList_GetSize (pSelBrushes) > 0));
}

void CFusionDoc::OnToolsFaceAttributes() 
{
	if ((mModeTool == ID_GENERALSELECT) && (mAdjustMode == ADJUST_MODE_FACE))
	{
		FaceAttributesDialog ();
		SetModifiedFlag ();
	}	
	
}

void CFusionDoc::OnUpdateToolsFaceAttributes(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable ((mModeTool == ID_GENERALSELECT) && 
					(mAdjustMode == ADJUST_MODE_FACE) && 
					(SelFaceList_GetSize (pSelFaces) > 0));
	
}

void CFusionDoc::OnEntityVisibility() 
{
	EntityViewList *pListCopy;
	EntityViewList *pEntityView;

	pEntityView = Level_GetEntityVisibilityInfo (pLevel);

	// copy existing...
	pListCopy = EntityViewList_Copy (pEntityView);
	
	if (pListCopy != NULL)
	{
		CEntityVisDlg Dlg;
		geBoolean Changed;

		Changed = GE_FALSE;

		// call dialog...
		if (Dlg.DoModal (pEntityView) == IDOK)
		{
			int i;

			// update changes
			for (i = 0; i < pEntityView->nEntries; ++i)
			{
				EntityViewEntry *pEntry;

				pEntry = &pEntityView->pEntries[i];
				if ((pEntry->IsVisible ^ pListCopy->pEntries[i].IsVisible) != 0)
				{
					Changed = GE_TRUE;

					Level_EnumEntities (pLevel, pEntry, ::fdocSetEntityVisibility);
				}
			}
		}

		// get rid of copy
		EntityViewList_Destroy (&pListCopy);

		// redraw views if any changes made
		if (Changed)
		{
			UpdateAllViews (UAV_ALL3DVIEWS, NULL);
		}
	}
}

void CFusionDoc::OnRebuildBsp() 
{
	Level_SetBspRebuild (pLevel, !Level_RebuildBspAlways (pLevel));
}

void CFusionDoc::OnUpdateRebuildBsp(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck (Level_RebuildBspAlways (pLevel));
}

const Prefs *CFusionDoc::GetPrefs (void)
{
	return ((CFusionApp *)AfxGetApp ())->GetPreferences ();
}

typedef geBoolean (*Brush_FaceCallback)(Face *pFace, void *lParam);

static geBoolean fdocUpdateFaceTextures (Face *pFace, void *lParam)
{
	CFusionDoc *pDoc = (CFusionDoc *)lParam;


	Face_SetTextureDibId (pFace, Level_GetDibId (pDoc->pLevel, Face_GetTextureName (pFace)));
	return GE_TRUE;
}

static geBoolean fdocUpdateBrushFaceTextures (Brush *pBrush, void *pVoid)
{
	Brush_EnumFaces (pBrush, pVoid, ::fdocUpdateFaceTextures);
	return GE_TRUE;
}

void CFusionDoc::OnLeveloptions() 
{
	CLevelOptions  Dlg;

	Dlg.m_DrawScale = Level_GetDrawScale (pLevel);
	Dlg.m_LightmapScale = Level_GetLightmapScale (pLevel);
	Dlg.m_TextureLib = Level_GetWadPath (pLevel);
	Dlg.m_HeadersDir = Level_GetHeadersDirectory (pLevel);

	if (Dlg.DoModal () == IDOK)
	{
		Level_SetDrawScale (pLevel, Dlg.m_DrawScale);
		Level_SetLightmapScale (pLevel, Dlg.m_LightmapScale);
		if (Dlg.m_TxlChanged)
		{
			Level_SetWadPath (pLevel, Dlg.m_TextureLib);
			if (!Level_LoadWad (pLevel))
			{
				CString Msg;

				AfxFormatString1 (Msg, IDS_CANTLOADTXL, Dlg.m_TextureLib);
				AfxMessageBox (Msg);
			}

			// update textures tab
			mCurTextureSelection = 0;
			mpMainFrame->m_wndTabControls->UpdateTextures ();

			// update all brush faces
			BrushList_EnumLeafBrushes (Level_GetBrushes (pLevel), this, ::fdocUpdateBrushFaceTextures);
			{
				// find the rendered view and set the wad size infos for it
				POSITION		pos;
				CFusionView	*	pView;

				pos = GetFirstViewPosition();
				while( pos != NULL )
				{
					pView = (CFusionView*)GetNextView(pos) ;
					if( Render_GetViewType( pView->VCam ) & (VIEWSOLID|VIEWTEXTURE|VIEWWIRE) )
					{
						Render_SetWadSizes (pView->VCam, Level_GetWadSizeInfos (pLevel));
						break ;	// Only 1 rendered view for now
					}
				}
			}

			if (Level_RebuildBspAlways (pLevel))
			{
				RebuildTrees();
				UpdateAllViews (UAV_ALL3DVIEWS, NULL);
			}
		}
		if (Dlg.m_HeadersChanged)
		{
			Level_LoadEntityDefs (pLevel, Dlg.m_HeadersDir);
			if (ValidateEntities( ) == FALSE)
			{
				SelectTab( CONSOLE_TAB ) ;
				AfxMessageBox( IDS_ENTITY_WARNING ) ;
			}
			mpMainFrame->m_wndTabControls->UpdateTabs ();
		}

	}
}
