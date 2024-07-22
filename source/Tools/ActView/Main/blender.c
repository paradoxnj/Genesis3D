/****************************************************************************************/
/*  BLENDER.C                                                                           */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Actor Viewer's motion blender dialog box.								*/
/*                                                                                      */
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

#include "Blender.h"
#pragma warning(disable : 4201 4214 4115)
#include <commctrl.h>
#pragma warning(default : 4201 4214 4115)
#include "resource.h"
#include "RAM.H"
#include "rcstring.h"
#include "units.h"
#include <assert.h>
#include "motion.h"
#include "winutil.h"

#pragma warning (disable:4514)		// unreferenced inline function (stupid compiler)

// Margins and motion spacing in window
#define TOP_MARGIN 5
#define DOT_MARGIN 2
#define LEFT_MARGIN 20
#define LINE_HEIGHT 15
#define SPACE_BETWEEN_LINES 20
#define HANDLE_SIZE (LINE_HEIGHT/2)

#define NO_MOTION (-1)

// For first cut, MAX_SUBMOTIONS is as many motions as will fit in the rendering window.
// Since we don't have a scrollbar right now, we limit a blend to this many submotions.
//
#define MAX_SUBMOTIONS 7

typedef enum
{
	mieNONE,		// not on item
	mieMOTION,		// within motion bounds
	mieSTARTBLEND,	// on start handle
	mieENDBLEND,	// on end handle
	mieMOTIONOFFSET,// on left-hand scaling handle
	mieMOTIONEND	// on right-hand scaling handle
} MotionItemEnum;


// Data attached to window.
typedef struct
{
	HWND hwnd;					// my window handle
	HWND hwndParent;			// parent window handle
	HINSTANCE Instance;			// instance
	geActor_Def *ActorDef;		// Current actor def
	geMotion *BlendedMotion;	// Compound motion being constructed
	int CurrentBlendItem;		// Index of current submotion
	MotionItemEnum MouseMode;	// Mouse state
	POINT StartDragPoint;		// Point at which drag was started
	POINT LastDragPoint;		// Last point where drag processed
	geFloat PixelsPerSecond;	// Pixels per second scale in window
} Blender_WindowData;


// Submotion structure holds all pertinent information about a submotion.
typedef struct
{
	geFloat TimeOffset;			// motion starting time (parent-relative)
	geFloat StartExtent;		// Time of first key in submotion (submotion relative)
	geFloat EndExtent;			// Time of last key in submotion
	geFloat TimeScale;			// Scale to apply to submotion-relative times
	geFloat StartBlendTime;		// Time (submotion relative) to start blend
	geFloat EndBlendTime;		// Time (submotion relative) to stop blend
	geFloat StartBlend;			// Start blend value (percent 0..1)
	geFloat EndBlend;			// End blend value (0..1)
	geBoolean SmoothBlend;		// Set to TRUE to automatically create blend path.
								// Currently this is forced
	gePath *BlendPath;			// Blend path
	geMotion *Motion;			// Motion information
	geXForm3d BaseTransform;	// Submotion base transform
} Blender_Submotion;


// Usage counter used for initialization/destruction of common resources
static int Blender_UsageCount = 0;

// Pens and brushes used for drawing
static HPEN RedPen = NULL;
static HPEN GreenPen = NULL;
static HPEN BluePen = NULL;
static HBRUSH RedBrush = NULL;
static HBRUSH GreenBrush = NULL;
static HBRUSH BlueBrush = NULL;
static HFONT Blender_Font = NULL;

// return pointer to Blender window's local data
static Blender_WindowData *Blender_GetWindowData (HWND hwnd)
{
	return (Blender_WindowData *)GetWindowLong (hwnd, GWL_USERDATA);
}

// shutdown the blender window...
static void Blender_ShutdownAll (Blender_WindowData *pData)
{
	SendMessage (pData->hwndParent, WM_BLENDERCLOSING, (WPARAM)pData->hwnd, 0);

	SetWindowLong (pData->hwnd, GWL_USERDATA, (LONG)NULL);
	geRam_Free (pData);

	--Blender_UsageCount;
	if (Blender_UsageCount == 0)
	{
		// destroy pens and brushes if we're the last user
		DeleteObject (RedPen);
		DeleteObject (GreenPen);
		DeleteObject (BluePen);
		DeleteObject (RedBrush);
		DeleteObject (GreenBrush);
		DeleteObject (BlueBrush);
		if (Blender_Font != NULL)
		{
			DeleteObject (Blender_Font);
		}
	}
}

// Gets all data about a submotion and returns it in a Blender_Submotion structure
static Blender_Submotion Blender_GetCompleteSubmotion
	(
	  geMotion *BlendedMotion,
	  int SubmotionIndex
	)
{
	Blender_Submotion SubInfo;

	assert (SubmotionIndex >= 0);
	assert (SubmotionIndex < geMotion_GetSubMotionCount (BlendedMotion));

	SubInfo.TimeOffset		= 0.0f;
	SubInfo.StartExtent		= 0.0f;
	SubInfo.EndExtent		= 0.0f;
	SubInfo.StartBlendTime	= 0.0f;
	SubInfo.EndBlendTime	= 0.0f;
	SubInfo.TimeScale		= 1.0f;
	SubInfo.StartBlend		= 0.0f;
	SubInfo.EndBlend		= 1.0f;
	SubInfo.SmoothBlend		= GE_TRUE;
	SubInfo.BlendPath		= NULL;
	SubInfo.Motion			= NULL;

	// Get selected submotion from blended motion
	SubInfo.Motion = geMotion_GetSubMotion (BlendedMotion, SubmotionIndex);

	if (SubInfo.Motion != NULL)
	{
		SubInfo.TimeOffset = geMotion_GetTimeOffset (BlendedMotion, SubmotionIndex);
		geMotion_GetTimeExtents (SubInfo.Motion, &SubInfo.StartExtent, &SubInfo.EndExtent);

		SubInfo.TimeScale = geMotion_GetTimeScale (BlendedMotion, SubmotionIndex);
		SubInfo.BlendPath = geMotion_GetBlendPath (BlendedMotion, SubmotionIndex);
		if (SubInfo.BlendPath != NULL)
		{
			// get starting and ending times and keyframes
			int nKeyframes;
			geXForm3d XfmFrame;
			geFloat KeyTime;

			nKeyframes = gePath_GetKeyframeCount (SubInfo.BlendPath, GE_PATH_TRANSLATION_CHANNEL);
#pragma todo ("Smooth blends only, for now")
			assert ((nKeyframes == 1) || (nKeyframes == 2));

			SubInfo.SmoothBlend = GE_TRUE;

			// Motion start and end times
			gePath_GetTimeExtents (SubInfo.BlendPath, &SubInfo.StartBlendTime, &SubInfo.EndBlendTime);

			// get blend amount at first keyframe
			gePath_GetKeyframe (SubInfo.BlendPath, 0, GE_PATH_TRANSLATION_CHANNEL, &KeyTime, &XfmFrame);
			SubInfo.StartBlend = XfmFrame.Translation.X;

			// and at last keyframe
			gePath_GetKeyframe (SubInfo.BlendPath, nKeyframes-1, GE_PATH_TRANSLATION_CHANNEL, &KeyTime, &XfmFrame);
			SubInfo.EndBlend = XfmFrame.Translation.X;
		}
		SubInfo.BaseTransform = *geMotion_GetBaseTransform (BlendedMotion, SubmotionIndex);
	}
	return SubInfo;
}

// get current blend listbox item
static int Blender_GetCurrentBlendItem (HWND hwnd)
{
	Blender_WindowData *pData = Blender_GetWindowData (hwnd);

	return pData->CurrentBlendItem;
}

// returns data for current submotion
static Blender_Submotion Blender_GetCurrentSubmotion (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo;
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);

	// If there's no current submotion, then give some reasonable defaults.
	if (SubmotionIndex == LB_ERR)
	{
		SubInfo.TimeOffset		= 0.0f;
		SubInfo.StartBlendTime	= 0.0f;
		SubInfo.EndBlendTime	= 0.0f;
		SubInfo.TimeScale		= 1.0f;
		SubInfo.StartBlend		= 0.0f;
		SubInfo.EndBlend		= 1.0f;
		SubInfo.SmoothBlend		= GE_TRUE;
		SubInfo.BlendPath		= NULL;
		SubInfo.Motion			= NULL;
	}
	else
	{
		SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, SubmotionIndex);
	}
	return SubInfo;
}

// Return Y position of item's top pixel
static int GetTopPixelPos (int Index)
{
	return TOP_MARGIN + ((Index + 1) * SPACE_BETWEEN_LINES);
}

// Return Y position of item's bottom pixel
static int GetBottomPixelPos (int Index)
{
	return GetTopPixelPos (Index) + LINE_HEIGHT;
}

// Return X pixel position that corresponds to item's start time
static int GetStartPixelPos (geMotion *pMotion, int Index, geFloat PixelScale)
{
	geFloat MotionStart, MotionEnd;
	Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pMotion, Index);
	geMotion_GetTimeExtents (pMotion, &MotionStart, &MotionEnd);

	return LEFT_MARGIN + Units_Round((SubInfo.TimeOffset - MotionStart) * PixelScale);
}

// Return X pixel position that corresponds to item's end time
static int GetEndPixelPos (geMotion *pMotion, int Index, geFloat PixelScale)
{
	geFloat MotionStart, MotionEnd;
	Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pMotion, Index);
	geMotion_GetTimeExtents (pMotion, &MotionStart, &MotionEnd);

	return GetStartPixelPos (pMotion, Index, PixelScale) +
		Units_Round (((SubInfo.EndExtent - SubInfo.StartExtent) / SubInfo.TimeScale) * PixelScale);
}

// return starting scale handle rectangle
static RECT GetStartHandleRect
	(
	  geMotion *pMotion,
	  int Index,
	  geFloat PixelScale
	)
{
	RECT ItemRect;

	ItemRect.left = GetStartPixelPos (pMotion, Index, PixelScale) - HANDLE_SIZE/2;
	ItemRect.right = ItemRect.left + HANDLE_SIZE;
	ItemRect.bottom = GetBottomPixelPos (Index);
	ItemRect.top = ItemRect.bottom - HANDLE_SIZE;

	return ItemRect;
}


// return ending scale handle rectangle
static RECT GetEndHandleRect
	(
	  geMotion *pMotion,
	  int Index,
	  geFloat PixelScale
	)
{
	RECT ItemRect;

	ItemRect.left = GetEndPixelPos (pMotion, Index, PixelScale) - HANDLE_SIZE/2;
	ItemRect.right = ItemRect.left + HANDLE_SIZE;
	ItemRect.bottom = GetBottomPixelPos (Index);
	ItemRect.top = ItemRect.bottom - HANDLE_SIZE;

	return ItemRect;
}


// Return rectangle that encloses the entire item
static RECT GetItemRect
	(
	  geMotion *pMotion,
	  int Index,
	  geFloat PixelScale
	)
{
	RECT ItemRect;

	ItemRect.left = GetStartPixelPos (pMotion, Index, PixelScale);
	ItemRect.top = GetTopPixelPos (Index);
	ItemRect.right = GetEndPixelPos (pMotion, Index, PixelScale);
	ItemRect.bottom = GetBottomPixelPos (Index);

	if (ItemRect.left > ItemRect.right)
	{
		int Temp = ItemRect.left;
		ItemRect.left = ItemRect.right;
		ItemRect.right = Temp;
	}
	return ItemRect;
}

// Get X pixel position that corresponds to item's start blend time
int GetStartBlendPixelPos (geMotion *pMotion, int Index, geFloat PixelScale)
{
	Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pMotion, Index);
	geFloat MotionStart, MotionEnd;

	geMotion_GetTimeExtents (pMotion, &MotionStart, &MotionEnd);
	return LEFT_MARGIN + Units_Round (((SubInfo.TimeOffset - MotionStart) + (SubInfo.StartBlendTime/SubInfo.TimeScale)) * PixelScale);
}

// Return rectangle of the start "handle"
static RECT GetStartBlendHandleRect
	(
	  geMotion *pMotion,
	  int Index,
	  geFloat PixelScale
	)
{
	RECT ItemRect;

	ItemRect.left = GetStartBlendPixelPos (pMotion, Index, PixelScale) - HANDLE_SIZE/2;
	ItemRect.top = TOP_MARGIN + ((Index + 1) * SPACE_BETWEEN_LINES);
	ItemRect.right = ItemRect.left + HANDLE_SIZE;
	ItemRect.bottom = ItemRect.top + HANDLE_SIZE;

	return ItemRect;
}

// Get X pixel position that corresponds to item's end blend time
int GetEndBlendPixelPos (geMotion *pMotion, int Index, geFloat PixelScale)
{
	Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pMotion, Index);
	geFloat MotionStart, MotionEnd;

	geMotion_GetTimeExtents (pMotion, &MotionStart, &MotionEnd);
	return LEFT_MARGIN + Units_Round (((SubInfo.TimeOffset - MotionStart) + (SubInfo.EndBlendTime/SubInfo.TimeScale)) * PixelScale);
}

// return end "handle" rectangle
static RECT GetEndBlendHandleRect
	(
	  geMotion *pMotion,
	  int Index,
	  geFloat PixelScale
	)
{
	RECT ItemRect;

	ItemRect.right = GetEndBlendPixelPos (pMotion, Index, PixelScale) + HANDLE_SIZE/2;
	ItemRect.top = TOP_MARGIN + ((Index + 1) * SPACE_BETWEEN_LINES);
	ItemRect.left = ItemRect.right - HANDLE_SIZE;
	ItemRect.bottom = ItemRect.top + HANDLE_SIZE;

	return ItemRect;
}

// Return rectangle for the specified item.
static RECT GetHandleRect
	(
	  geMotion *pMotion, 
	  int Index, 
	  geFloat PixelScale,
	  MotionItemEnum Item
	)
{
	switch (Item)
	{
		case mieMOTION :		return GetItemRect (pMotion, Index, PixelScale);
		case mieSTARTBLEND :	return GetStartBlendHandleRect (pMotion, Index, PixelScale);
		case mieENDBLEND :		return GetEndBlendHandleRect (pMotion, Index, PixelScale);
		case mieMOTIONOFFSET :	return GetStartHandleRect (pMotion, Index, PixelScale);
		case mieMOTIONEND:		return GetEndHandleRect (pMotion, Index, PixelScale);
		default :
		{
			RECT Trash = {-1, -1, -1, -1};
			assert (0);
			return Trash;
		}
	}
}

// this draws the time line for a single motion.
// This includes the start and ending bars, the line between them,
// and the start and ending times.
static void Blender_DrawTimeLine
	(
	  HDC dc,
	  float StartTime,
	  float EndTime,
	  geFloat PixelScale
	)
{
	const int LineLength = Units_Round((EndTime - StartTime) * PixelScale);
	const int LineStartX = LEFT_MARGIN;
	const int LineStartY = TOP_MARGIN;
	HPEN OldPen = SelectObject (dc, GetStockObject (BLACK_PEN));

	// draw vertical bars at start and end of time line
	MoveToEx (dc, LineStartX, LineStartY, NULL);
	LineTo (dc, LineStartX, LineStartY + LINE_HEIGHT);

	MoveToEx (dc, LineStartX + LineLength, LineStartY, NULL);
	LineTo (dc, LineStartX + LineLength, LineStartY + LINE_HEIGHT);

	// draw horizontal line between vertical bars
	MoveToEx (dc, LineStartX, LineStartY + LINE_HEIGHT/2, NULL);
	LineTo (dc, LineStartX + LineLength, LineStartY + LINE_HEIGHT/2);

	// Display starting and ending times next to bars
	{
		char sTime[100];
		SIZE tSize;
		#define RED (RGB (255, 0, 0))
		#define BLACK (RGB (0, 0, 0))
		COLORREF OldColor = SetTextColor (dc, BLACK);

		sprintf (sTime, "%.2f", StartTime);
		GetTextExtentPoint32 (dc, sTime, lstrlen (sTime), &tSize);

		SetTextColor (dc, (StartTime < 0) ? RED : BLACK);
		TextOut (dc, LineStartX + 2, LineStartY, sTime, lstrlen (sTime));

		sprintf (sTime, "%.2f", EndTime);
		GetTextExtentPoint32 (dc, sTime, lstrlen (sTime), &tSize);

		SetTextColor (dc, (EndTime < 0) ? RED : BLACK);
		TextOut (dc, LineStartX + LineLength - 2 - tSize.cx, LineStartY, sTime, lstrlen (sTime));

		SetTextColor (dc, OldColor);
	}

	SelectObject (dc, OldPen);
}

// Draw a specified handle.
static void Blender_DrawHandle
	(
	  HDC dc,
	  geMotion *pMotion,
	  int Index,
	  geFloat PixelScale,
	  MotionItemEnum Item,
	  HBRUSH Brush,
	  HPEN Pen
	)
{
	RECT HandleRect = GetHandleRect (pMotion, Index, PixelScale, Item);

	if ((Item == mieSTARTBLEND) || (Item == mieENDBLEND))
	{
		// draw the line...
		SelectObject (dc, Pen);
		MoveToEx (dc, HandleRect.left + HANDLE_SIZE/2, HandleRect.top, NULL);
		LineTo (dc, HandleRect.left + HANDLE_SIZE/2, HandleRect.top + LINE_HEIGHT);
	}

	// and the rectangle
	SelectObject (dc, Brush);
	SelectObject (dc, GetStockObject (BLACK_PEN));
	Rectangle (dc, HandleRect.left, HandleRect.top, HandleRect.right, HandleRect.bottom);
}

// Draw a single submotion
static void Blender_DrawMotion
	(
	  HDC dc,
	  geMotion *BlendedMotion,
	  int Index,
	  geFloat PixelScale
	)
{
	RECT ItemRect = GetItemRect (BlendedMotion, Index, PixelScale);
	geMotion *pSubmotion = geMotion_GetSubMotion (BlendedMotion, Index);

	SelectObject (dc, GetStockObject (BLACK_PEN));
	SelectObject (dc, GetStockObject (WHITE_BRUSH));

	// Display vertical lines at start and end of motion
	MoveToEx (dc, ItemRect.left, ItemRect.top, NULL);
	LineTo (dc, ItemRect.left, ItemRect.bottom);

	MoveToEx (dc, ItemRect.right, ItemRect.top, NULL);
	LineTo (dc, ItemRect.right, ItemRect.bottom);

	// draw horizontal line between vertical bars
	MoveToEx (dc, ItemRect.left, (ItemRect.top + ItemRect.bottom)/2, NULL);
	LineTo (dc, ItemRect.right, (ItemRect.top + ItemRect.bottom)/2);

	// Center motion name on line...
	{
		RECT TextRect;
		const char *SubmotionName = geMotion_GetName (pSubmotion);
		TextRect.left = ItemRect.left + 1;
		TextRect.top = ItemRect.top;
		TextRect.right = ItemRect.right - 1;
		TextRect.bottom = ItemRect.bottom - 1;

		DrawText (dc, SubmotionName, lstrlen (SubmotionName), &TextRect, DT_CENTER | DT_VCENTER);
   	}

	{
		// Draw arrow indicating direction
		#define ARROW_HEIGHT HANDLE_SIZE
		#define ARROW_WIDTH HANDLE_SIZE
		#define ARROW_SHAFT_LENGTH 10

		const int Start = GetStartPixelPos (BlendedMotion, Index, PixelScale);
		const int End = GetEndPixelPos (BlendedMotion, Index, PixelScale);
		const int Sign = (Start <= End) ? 1 : -1;
		const int ArrowBaseX = Start + (Sign * ARROW_SHAFT_LENGTH);
		const int ArrowPointX = ArrowBaseX + (Sign * ARROW_WIDTH);
		const int ShaftY = GetBottomPixelPos (Index) - ARROW_HEIGHT/2;
		POINT ArrowPoints[3];

		ArrowPoints[0].x = ArrowBaseX;
		ArrowPoints[0].y = ShaftY - ARROW_HEIGHT/2;
		ArrowPoints[1].x = ArrowPointX;
		ArrowPoints[1].y = ShaftY;
		ArrowPoints[2].x = ArrowBaseX;
		ArrowPoints[2].y = ShaftY + ARROW_HEIGHT/2;

		MoveToEx (dc, Start, ShaftY, NULL);
		LineTo (dc, ArrowBaseX, ShaftY);
		Polygon (dc, ArrowPoints, 3);
	}

	// and draw the handles that can be grabbed by the mouse
	Blender_DrawHandle (dc, BlendedMotion, Index, PixelScale, mieSTARTBLEND, GreenBrush, GreenPen);
	Blender_DrawHandle (dc, BlendedMotion, Index, PixelScale, mieENDBLEND, RedBrush, RedPen);
	Blender_DrawHandle (dc, BlendedMotion, Index, PixelScale, mieMOTIONOFFSET, GetStockObject (WHITE_BRUSH), GetStockObject (BLACK_PEN));
	Blender_DrawHandle (dc, BlendedMotion, Index, PixelScale, mieMOTIONEND, GetStockObject (WHITE_BRUSH), GetStockObject (BLACK_PEN));
}


// Paints the motion window
static void Blender_PaintMotionWindow
	(
	  Blender_WindowData *pData
	)
{
	HDC dc;
	RECT ClientRect;
	HWND hwndChild;
	HBRUSH OldBrush;
	HPEN OldPen;
	HFONT OldFont = NULL;
	int iMotion, NumMotions;
	geFloat CompoundMotionStart, CompoundMotionEnd;
	HRGN ClipRgn;

	// Get motion list window handle and DC
	hwndChild = GetDlgItem (pData->hwnd, IDC_STATICBLEND);

	GetClientRect (hwndChild, &ClientRect);
	dc = GetDC (hwndChild);
	if (Blender_Font != NULL)
	{
		OldFont = SelectObject (dc, Blender_Font);
	}

	// set clipping region so we don't draw outside here...
	ClipRgn = CreateRectRgn (ClientRect.left, ClientRect.top, ClientRect.right, ClientRect.bottom);
	SelectClipRgn (dc, ClipRgn);

	// Fill with white
	OldBrush = SelectObject (dc, GetStockObject (WHITE_BRUSH));
	OldPen = SelectObject (dc, GetStockObject (WHITE_PEN));
	Rectangle (dc, ClientRect.left, ClientRect.top, ClientRect.right, ClientRect.bottom);

	// Draw compound motion time line at the top of the window
	if (!geMotion_GetTimeExtents (pData->BlendedMotion, &CompoundMotionStart, &CompoundMotionEnd))
	{
		CompoundMotionStart = 0.0f;
		CompoundMotionEnd = 0.0f;
	}
	Blender_DrawTimeLine (dc, CompoundMotionStart, CompoundMotionEnd, pData->PixelsPerSecond);

	NumMotions = geMotion_GetSubMotionCount (pData->BlendedMotion);

	// Draw the individual motions
	for (iMotion = 0; iMotion < NumMotions; ++iMotion)
	{
		Blender_DrawMotion (dc, pData->BlendedMotion, iMotion, pData->PixelsPerSecond);
	}

	// Draw a circle to the left of the current motion
	if (pData->CurrentBlendItem != NO_MOTION)
	{
		int CurPos = ((1 + pData->CurrentBlendItem) * SPACE_BETWEEN_LINES) + LINE_HEIGHT;

		SelectObject (dc, GetStockObject (WHITE_BRUSH));
		SelectObject (dc, GetStockObject (BLACK_PEN));

		Ellipse (dc, DOT_MARGIN, CurPos-HANDLE_SIZE/2, DOT_MARGIN+HANDLE_SIZE, CurPos+HANDLE_SIZE/2);
	}

	// restore the DC
	SelectClipRgn (dc, NULL);
	DeleteObject (ClipRgn);

	SelectObject (dc, OldBrush);
	SelectObject (dc, OldPen);
	if (OldFont != NULL)
	{
		SelectObject (dc, OldFont);
	}
	ReleaseDC (hwndChild, dc);
}

// Display current submotion information in the edit fields
static void Blender_SetBlendMotionData (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	geFloat BlendedMotionStart, BlendedMotionEnd;

	// get time extents for blended motion...
	geMotion_GetTimeExtents (pData->BlendedMotion, &BlendedMotionStart, &BlendedMotionEnd);

	// compute pixels per second
	// Rather than allowing motion to always fill window, we'll do it by halving, with
	// the default being 100 pixels per second.
	{
		const geFloat BlendedMotionLength = BlendedMotionEnd - BlendedMotionStart;
		RECT ClientRect;

		GetClientRect (GetDlgItem (pData->hwnd, IDC_STATICBLEND), &ClientRect);

		pData->PixelsPerSecond = 100.0f;
		while ((pData->PixelsPerSecond * BlendedMotionLength) > (geFloat)(ClientRect.right - LEFT_MARGIN))
		{
			if (pData->PixelsPerSecond <= 2.0f)
			{
				// we need some limit, don't you think?
				break;
			}
			pData->PixelsPerSecond /= 2.0f;
		} 
	}

	// Set the values in the dialog controls
	WinUtil_SetDlgItemFloat (pData->hwnd, IDC_EDITTIMEOFFSET, SubInfo.TimeOffset);
	WinUtil_SetDlgItemFloat (pData->hwnd, IDC_EDITSTARTTIME, SubInfo.StartBlendTime);
	WinUtil_SetDlgItemFloat (pData->hwnd, IDC_EDITENDTIME, SubInfo.EndBlendTime);
	WinUtil_SetDlgItemFloat (pData->hwnd, IDC_EDITTIMESCALE, SubInfo.TimeScale);

	// start and end magnitude values are in percent
	SetDlgItemInt (pData->hwnd, IDC_EDITSTARTBLEND, Units_MakePercent (SubInfo.StartBlend), FALSE);
	SetDlgItemInt (pData->hwnd, IDC_EDITENDBLEND, Units_MakePercent (SubInfo.EndBlend), FALSE);

	{
		// display current submotion name
		const char *SubmotionName = (SubInfo.Motion == NULL) ? "" : geMotion_GetName (SubInfo.Motion);
		char *Name = geRam_Allocate (lstrlen (SubmotionName) + 100);

		if (Name == NULL)
		{
			SetDlgItemText (pData->hwnd, IDC_STATICSUBMOTION, SubmotionName);
		}
		else
		{
			sprintf (Name, "%s  (%.2f sec.)", SubmotionName, fabs(SubInfo.EndExtent - SubInfo.StartExtent));
			SetDlgItemText (pData->hwnd, IDC_STATICSUBMOTION, Name);
			geRam_Free (Name);
		}
	}
		
#pragma todo ("Add path display")
	// here we'll do the path...

	Blender_PaintMotionWindow (pData);

	// Enable/disable buttons as appropriate
	{
		int nSubmotions = geMotion_GetSubMotionCount (pData->BlendedMotion);
		WinUtil_EnableDlgItem (pData->hwnd, IDC_ADD, (nSubmotions < MAX_SUBMOTIONS));
		WinUtil_EnableDlgItem (pData->hwnd, IDC_REMOVE, (nSubmotions > 0));
		WinUtil_EnableDlgItem (pData->hwnd, IDC_MOVEUP, (pData->CurrentBlendItem > 0));
		WinUtil_EnableDlgItem (pData->hwnd, IDC_MOVEDOWN, ((pData->CurrentBlendItem >= 0) && (pData->CurrentBlendItem < nSubmotions-1)));
	}
}

// Called to notify parent of a change in the motion
static void Blender_NotifyParentOfChange (Blender_WindowData *pData)
{
	SendMessage (pData->hwndParent, WM_BLENDERMOTIONCHANGED, 0, 0);
}

// Add a submotion to the blended motion.
// This adds a copy of the passed motion.
// returns listbox index of added item
static int Blender_AddMotionToBlend
	(
	  geMotion *BlendedMotion,
	  geMotion *NewMotion
	)
{
	int SubmotionIndex;
	geXForm3d XfmIdentity;
	geFloat StartTime, EndTime;
	geBoolean rslt;

	assert (geMotion_GetSubMotionCount (BlendedMotion) < MAX_SUBMOTIONS);

	if (NewMotion == NULL)
	{
		// do I need an error message here?
		return GE_FALSE;
	}

	// add it to the motion...
	StartTime = 0.0f;
	EndTime = 0.0f;
	geMotion_GetTimeExtents (NewMotion, &StartTime, &EndTime);
	geXForm3d_SetIdentity (&XfmIdentity);		// Base transform is identity
	rslt = geMotion_AddSubMotion 
	(
		BlendedMotion,			// parent motion
		1.0f,					// time scale
		0.0f,					// motion offset (parent relative)
		NewMotion,				// motion to be added
		StartTime, 0.0f,		// start blend time (submotion relative) and blend value
		EndTime, 1.0f,			// end blend time (submotion relative) and blend value
		&XfmIdentity,			// Base transform
		&SubmotionIndex			// returned submotion index
	);

	return (rslt == GE_FALSE) ? -1 : SubmotionIndex;
}

// Add the motion that's selected in the Motion list box to the list
// of motions to blend in the Blend list box.
static void Blender_AddSelectedMotionToBlend
	(
	  Blender_WindowData *pData
	 )
{
	// get current selection
	int CurSel = SendDlgItemMessage (pData->hwnd, IDC_MOTIONSLIST, LB_GETCURSEL, 0, 0);
	if (CurSel != LB_ERR)
	{
		int SubmotionIndex;
		// motion index is stored in list box's item data.
		// get the index and then obtain the motion from the actor.
		int MotionIndex = SendDlgItemMessage (pData->hwnd, IDC_MOTIONSLIST, LB_GETITEMDATA, CurSel, 0);
		geMotion *Motion = geActor_GetMotionByIndex (pData->ActorDef, MotionIndex);

		assert (Motion != NULL);
		// Add it to the blended motion
		SubmotionIndex = Blender_AddMotionToBlend (pData->BlendedMotion, Motion);
		if (SubmotionIndex != -1)
		{
			// set new item as current and update the UI
			pData->CurrentBlendItem = SubmotionIndex;
			Blender_SetBlendMotionData (pData);
			Blender_NotifyParentOfChange (pData);
		}
	}
}


// remove from the compound motion the submotion that's selected in the Blend window
void Blender_RemoveSelectedMotionFromBlend 
	(
	  Blender_WindowData *pData
	)
{
	// get currently-selected item (if any)
	
	int CurSel = Blender_GetCurrentBlendItem (pData->hwnd);

	if (CurSel != NO_MOTION)
	{
		int nItems;

		// remove it from the compound motion
		geMotion_RemoveSubMotion (pData->BlendedMotion, CurSel);

		nItems = geMotion_GetSubMotionCount (pData->BlendedMotion);
		// if there are still items, set the new selection
		if (nItems > 0)
		{
			if (CurSel >= nItems)
			{
				CurSel = nItems - 1;
			}
		}
		else
		{
			CurSel = NO_MOTION;
		}
		pData->CurrentBlendItem = CurSel;
		Blender_SetBlendMotionData (pData);
		Blender_NotifyParentOfChange (pData);
	}
}

// return the time offset of the currently-selected item
// returns 0.0f if there is no current item
// Time is parent motion relative
static geFloat Blender_GetTimeOffset (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	return SubInfo.TimeOffset;
}

// returns the starting time of the currently-selected submotion
// This is submotion relative
static geFloat Blender_GetStartTime (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	return SubInfo.StartBlendTime;
}

// returns the ending time of the currently-selected submotion
// submotion relative
static geFloat Blender_GetEndTime (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	return SubInfo.EndBlendTime;
}

// Returns time scale for current submotion
static geFloat Blender_GetTimeScale (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	return SubInfo.TimeScale;
}

// Returns current submotion start blend value
static geFloat Blender_GetStartBlend (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	return SubInfo.StartBlend;
}

// Returns current submotion end blend value
static geFloat Blender_GetEndBlend (Blender_WindowData *pData)
{
	Blender_Submotion SubInfo = Blender_GetCurrentSubmotion (pData);
	return SubInfo.EndBlend;
}

// Set the time offset for a submotion, and update UI to reflect new value.
// Time offset is parent relative
static void Blender_SetTimeOffset
	(
	  Blender_WindowData *pData,
	  geFloat TimeOffset
	)
{
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);
	if (SubmotionIndex != LB_ERR)
	{
		// set it and update the UI
		geMotion_SetTimeOffset (pData->BlendedMotion, SubmotionIndex, TimeOffset);
	}
}

// Create new smooth blending path from StartTime to EndTime with
// given starting and ending magnitudes.
// Times are submotion relative
static void Blender_CreateNewPath
	(
		geMotion *BlendedMotion,
		int SubmotionIndex,
		geFloat StartTime, geFloat StartMagnitude,
		geFloat EndTime, geFloat EndMagnitude
	)
{
	geXForm3d XfmBlend;
	gePath *Path;

	// if times are identical, then we just can't do this...
	if (StartTime == EndTime)
	{
		return;
	}

	Path = gePath_Create 
	(
		GE_PATH_INTERPOLATE_HERMITE_ZERO_DERIV,	// default for blending
		GE_PATH_INTERPOLATE_SQUAD,				// anything...not used for blending
		GE_FALSE	// not looped
	);

	// Program's probably going to not like it if the path is NULL.
	// But there isn't much I can do about it...
	if (Path != NULL)
	{
		// add starting and ending keyframes

		// only the X value of the translation part of the transform is used.
		XfmBlend.Translation.X = StartMagnitude;
		gePath_InsertKeyframe (Path, GE_PATH_TRANSLATION_CHANNEL, StartTime, &XfmBlend);

		// and the end keyframe
		XfmBlend.Translation.X = EndMagnitude;
		gePath_InsertKeyframe (Path, GE_PATH_TRANSLATION_CHANNEL, EndTime, &XfmBlend);

		// set blend path for this submotion
		geMotion_SetBlendPath (BlendedMotion, SubmotionIndex, Path);

		// and delete our local copy
		gePath_Destroy (&Path);
	}
}

// Set submotion start blend time
static void Blender_SetStartTime
	(
	  Blender_WindowData *pData,
	  geFloat StartBlendTime
	)
{
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);
	if (SubmotionIndex != LB_ERR)
	{
		// Get ending time and create new blend path
		Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, SubmotionIndex);
		Blender_CreateNewPath (pData->BlendedMotion, SubmotionIndex, StartBlendTime, SubInfo.StartBlend, SubInfo.EndBlendTime, SubInfo.EndBlend);
	}
}

// Set submotion end blend time
static void Blender_SetEndTime
	(
	  Blender_WindowData *pData,
	  geFloat EndBlendTime
	)
{
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);
	if (SubmotionIndex != LB_ERR)
	{
		// Get starting time and create new blend path
		Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, SubmotionIndex);
		Blender_CreateNewPath (pData->BlendedMotion, SubmotionIndex, SubInfo.StartBlendTime, SubInfo.StartBlend, EndBlendTime, SubInfo.EndBlend);
	}
}

// Set submotion time scale
static void Blender_SetTimeScale
	(
	  Blender_WindowData *pData,
	  geFloat TimeScale
	)
{
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);
	if (SubmotionIndex != LB_ERR)
	{
		geMotion_SetTimeScale (pData->BlendedMotion, SubmotionIndex, TimeScale);
	}
	
}

// Set submotion start blend value
static void Blender_SetStartBlend
	(
	  Blender_WindowData *pData,
	  geFloat StartBlend
	)
{
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);
	if (SubmotionIndex != LB_ERR)
	{
		Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, SubmotionIndex);
		Blender_CreateNewPath (pData->BlendedMotion, SubmotionIndex, SubInfo.StartBlendTime, StartBlend, SubInfo.EndBlendTime, SubInfo.EndBlend);
	}
}

// Set submotion end blend value
static void Blender_SetEndBlend
	(
	  Blender_WindowData *pData,
	  geFloat EndBlend
	)
{
	int SubmotionIndex = Blender_GetCurrentBlendItem (pData->hwnd);
	if (SubmotionIndex != LB_ERR)
	{
		Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, SubmotionIndex);
		Blender_CreateNewPath (pData->BlendedMotion, SubmotionIndex, SubInfo.StartBlendTime, SubInfo.StartBlend, SubInfo.EndBlendTime, EndBlend);
	}
}


// Output test data to blender.txt.
// This is a debugging function.
static void Blender_OutputTestData (Blender_WindowData *pData)
{
	FILE *f;
	int nMotions, iSubmotion;
	geFloat StartExtent, EndExtent;
	static const geFloat TimeDelta = 0.10f;

	f = fopen ("blender.txt", "wt");
	fprintf (f, "Blender data output\n");
	
	nMotions = geMotion_GetSubMotionCount (pData->BlendedMotion);
	fprintf (f, "Number of submotions = %d\n", nMotions);

	geMotion_GetTimeExtents (pData->BlendedMotion, &StartExtent, &EndExtent);
	fprintf (f, "Extents:  %.2f to %.2f\n", StartExtent, EndExtent);

	for (iSubmotion = 0; iSubmotion < nMotions; ++iSubmotion)
	{
		geFloat mStart, mEnd;
		geMotion *Motion = geMotion_GetSubMotion (pData->BlendedMotion, iSubmotion);
		Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, iSubmotion);

		fprintf (f, "Submotion %d (%s)\n", iSubmotion, geMotion_GetName (Motion));
		geMotion_GetTimeExtents (Motion, &mStart, &mEnd);
		fprintf (f, "  Extents:  %.2f to %.2f\n", mStart, mEnd);
		fprintf (f, "  Time Offset = %.2f\n", SubInfo.TimeOffset);
		fprintf (f, "  Time Scale = %.2f\n", SubInfo.TimeScale);
		fprintf (f, "  Start Blend Time = %.2f\n", SubInfo.StartBlendTime);
		fprintf (f, "  Start Blend = %.2f\n", SubInfo.StartBlend);
		fprintf (f,	"  End Blend Time = %.2f\n", SubInfo.EndBlendTime);
		fprintf (f, "  End Blend = %.2f\n", SubInfo.EndBlend);
		fprintf (f, "  Num path keys = %d\n", gePath_GetKeyframeCount (SubInfo.BlendPath, GE_PATH_TRANSLATION_CHANNEL));
		fprintf (f, "  Blend amounts at %.2f second intervals\n", TimeDelta);
		{
			geFloat CurrentTime;

			CurrentTime = StartExtent;

			while (CurrentTime <= EndExtent)
			{
				geFloat BlendAmount;

				BlendAmount = geMotion_GetBlendAmount (pData->BlendedMotion, iSubmotion, CurrentTime);
				fprintf (f, "\t%.2f\t\t%.2f\n", CurrentTime, BlendAmount);
				CurrentTime += TimeDelta;
			}
		}
	}
	fclose (f);
	MessageBox (pData->hwnd, "Data output to blender.txt", "Blender", MB_OK);
}

// Move a submotion up or down in the list
static void Blender_MoveSubMotion
	(
	  Blender_WindowData *pData,
	  int MoveFrom,
	  int MoveTo
	)
{
	// Move a submotion from its current place in the compound motion to
	// a new place.
	// Since the motion API doesn't have the primitives to insert submotions,
	// we'll get info for all the submotions, remove them, and then add
	// them in the proper order.
	const int SubmotionCount = geMotion_GetSubMotionCount (pData->BlendedMotion);
	Blender_Submotion *SubInfo = GE_RAM_ALLOCATE_ARRAY (Blender_Submotion, SubmotionCount);
	int iMotion;

	if (SubInfo == NULL)
	{
		return;
	}

	// get each submotion, and then remove it from the compound motion
	for (iMotion = SubmotionCount; iMotion > 0; --iMotion)
	{
		SubInfo[iMotion-1] = Blender_GetCompleteSubmotion (pData->BlendedMotion, iMotion-1);
		// need to copy the path because RemoveSubMotion is going to trash it...
		SubInfo[iMotion-1].BlendPath = gePath_CreateCopy (SubInfo[iMotion-1].BlendPath);

		geMotion_RemoveSubMotion (pData->BlendedMotion, iMotion-1);
	}

	assert (geMotion_GetSubMotionCount (pData->BlendedMotion) == 0);

	// re-order the SubInfo in the array...
	{
		Blender_Submotion SubInfoTemp;

		// Yeah, memmove is ugly, but it's easier than 2 separate loops...
		SubInfoTemp = SubInfo[MoveFrom];
		memmove (&SubInfo[MoveFrom], &SubInfo[MoveTo], sizeof (Blender_Submotion) * abs (MoveFrom - MoveTo));
		SubInfo[MoveTo] = SubInfoTemp;
	}

	// now add the newly-ordered submotions back to the compound motion
	for (iMotion = 0; iMotion < SubmotionCount; ++iMotion)
	{
		Blender_Submotion *pInfo = &SubInfo[iMotion];
		int Index;

		geMotion_AddSubMotion 
		(
			pData->BlendedMotion,
			pInfo->TimeScale,
			pInfo->TimeOffset,
			pInfo->Motion,
			pInfo->StartBlendTime, pInfo->StartBlend,
			pInfo->EndBlendTime, pInfo->EndBlend,
			&pInfo->BaseTransform,
			&Index
		);
		assert (Index == iMotion);
		// Set the blend path and then destroy our copy of the path
		geMotion_SetBlendPath (pData->BlendedMotion, iMotion, pInfo->BlendPath);
		gePath_Destroy (&(pInfo->BlendPath));
	}

	assert (geMotion_GetSubMotionCount (pData->BlendedMotion) == SubmotionCount);

	geRam_Free (SubInfo);
}

// Don't you just love these big switch statements?
#pragma warning (disable:4100)
static BOOL wm_Command
	(
	  Blender_WindowData *pData,
	  WORD wNotifyCode,
	  WORD wID,
	  HWND hwndCtl
	)
{
	switch (wID)
	{
		geFloat TimeValue;
		BOOL rslt;

		case IDC_TESTOUT :
			Blender_OutputTestData (pData);
			return 0;

		case IDCANCEL :
			DestroyWindow (pData->hwnd);
			return 0;

		case IDC_MOTIONSLIST :
			// if double-click an item in the motions list, add it to 
			// the blended list.
			if (wNotifyCode == LBN_DBLCLK)
			{
				Blender_AddSelectedMotionToBlend (pData);
			}
			return 0;

		case IDC_ADD :
			Blender_AddSelectedMotionToBlend (pData);
			return 0;

		case IDC_REMOVE :
			// remove currently-selected item from blended motion
			Blender_RemoveSelectedMotionFromBlend (pData);
			return 0;

		case IDC_MOVEUP :
			if (pData->CurrentBlendItem > 0)
			{
				Blender_MoveSubMotion (pData, pData->CurrentBlendItem, pData->CurrentBlendItem-1);
				--(pData->CurrentBlendItem);
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_MOVEDOWN :
			if ((pData->CurrentBlendItem >= 0) && 
			    (pData->CurrentBlendItem < geMotion_GetSubMotionCount (pData->BlendedMotion)-1))
			{
				Blender_MoveSubMotion (pData, pData->CurrentBlendItem, pData->CurrentBlendItem+1);
				++(pData->CurrentBlendItem);
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_EDITTIMEOFFSET :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				rslt = WinUtil_GetDlgItemFloat (pData->hwnd, IDC_EDITTIMEOFFSET, &TimeValue);
				if (!rslt)
				{
					TimeValue = Blender_GetTimeOffset (pData);
				}
				Blender_SetTimeOffset (pData, TimeValue);
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_EDITSTARTTIME :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				rslt = WinUtil_GetDlgItemFloat (pData->hwnd, IDC_EDITSTARTTIME, &TimeValue);
				// don't allow Start > End
				if (!rslt || (TimeValue > Blender_GetEndTime (pData)))
				{
					TimeValue = Blender_GetStartTime (pData);
				}
				Blender_SetStartTime (pData, TimeValue);
				// update UI
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_EDITENDTIME :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				rslt = WinUtil_GetDlgItemFloat (pData->hwnd, IDC_EDITENDTIME, &TimeValue);
				// don't allow End < Start
				if (!rslt || (TimeValue < Blender_GetStartTime (pData)))
				{
					TimeValue = Blender_GetEndTime (pData);
				}
				Blender_SetEndTime (pData, TimeValue);
				// update UI
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_EDITTIMESCALE :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				geFloat Scale;

				rslt = WinUtil_GetDlgItemFloat (pData->hwnd, IDC_EDITTIMESCALE, &Scale);
				if (!rslt || (Scale == 0.0f))
				{
					Scale = Blender_GetTimeScale (pData);
				}
				Blender_SetTimeScale (pData, Scale);
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_EDITSTARTBLEND :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				BOOL IsOk;
				UINT NewValue = GetDlgItemInt (pData->hwnd, IDC_EDITSTARTBLEND, &IsOk, FALSE);
				geFloat fValue;

				if (IsOk && (NewValue >= 0) && (NewValue <= 100))
				{
					fValue = Units_FloatFromPercent (NewValue);
				}
				else
				{
					fValue = Blender_GetStartBlend (pData);
				}
				Blender_SetStartBlend (pData, fValue);
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		case IDC_EDITENDBLEND :
			if (wNotifyCode == EN_KILLFOCUS)
			{
				BOOL IsOk;
				UINT NewValue = GetDlgItemInt (pData->hwnd, IDC_EDITENDBLEND, &IsOk, FALSE);
				geFloat fValue;

				if (IsOk && (NewValue >= 0) && (NewValue <= 100))
				{
					fValue = Units_FloatFromPercent (NewValue);
				}
				else
				{
					fValue = Blender_GetEndBlend (pData);
				}
				Blender_SetEndBlend (pData, fValue);
				Blender_SetBlendMotionData (pData);
				Blender_NotifyParentOfChange (pData);
			}
			return 0;

		default :
			break;
	}
	return 0;
}
#pragma warning (default:4100)


// Initialize the dialog
static BOOL Blender_Initialize (Blender_WindowData *pData)
{
	if (Blender_UsageCount == 0)
	{
		// Create pens & brushes
		RedPen = CreatePen (PS_SOLID, 1, RGB (255, 0, 0));
		GreenPen = CreatePen (PS_SOLID, 1, RGB (0, 255, 0));
		BluePen = CreatePen (PS_SOLID, 1, RGB (0, 0, 255));
		RedBrush = CreateSolidBrush (RGB (255, 0, 0));
		GreenBrush = CreateSolidBrush (RGB (0, 255, 0));
		BlueBrush = CreateSolidBrush (RGB (0, 0, 255));

		// Create font for motion names
		Blender_Font = CreateFont
		(
			6,						// logical height of font
			0,						// logical average character width
			0,						// angle of escapement
			0,						// base-line orientation angle
			FW_NORMAL,				// font weight
			0,						// italic attribute flag
			0,						// underline attribute flag
			0,						// strikeout attribute flag
			ANSI_CHARSET,			// character set identifier
			OUT_DEFAULT_PRECIS,		// output precision
			CLIP_DEFAULT_PRECIS,	// clipping precision
			DRAFT_QUALITY,			// output quality
			DEFAULT_PITCH,			// pitch and family
			"MS Sans Serif"			// pointer to typeface name string
		);
	}
	++Blender_UsageCount;

	// set window icon
	SendMessage (pData->hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon (pData->Instance, MAKEINTRESOURCE (IDI_BLEND)));

	// Fill motions list box with actor's motions
	{
		int iMotion;
		HWND hwndListbox;
		int NumMotions;

		hwndListbox = GetDlgItem (pData->hwnd, IDC_MOTIONSLIST);
		SendMessage (hwndListbox, LB_RESETCONTENT, 0, 0);

		NumMotions = geActor_GetMotionCount (pData->ActorDef);
		for (iMotion = 0; iMotion < NumMotions; ++iMotion)
		{
			const char *MotionName = geActor_GetMotionName (pData->ActorDef, iMotion);
			int Index;

			// Although the motion API allows unnamed motions, our code that sets
			// up the combo box in ActView is supposed to give names to any unnamed motions.
			// So MotionName should never come back NULL here.
			assert (MotionName != NULL);

			Index = SendMessage (hwndListbox, LB_ADDSTRING, 0, (LONG)MotionName);
			if (Index != LB_ERR)
			{
				SendMessage (hwndListbox, LB_SETITEMDATA, Index, iMotion);
			}
		}
		// Set selection to first motion
		SendMessage (hwndListbox, LB_SETCURSEL, 0, 0);
	}
	Blender_SetBlendMotionData (pData);

	return TRUE;
}

// Returns a value indicating the type of item the mouse is over
static MotionItemEnum ItemMousePos
	(
	  geMotion *pMotion,
	  int iMotion,
	  geFloat PixelScale,
	  int MouseX,
	  int MouseY
	)
{
	RECT ItemRect;
	POINT pt;

	pt.x = MouseX;
	pt.y = MouseY;

	ItemRect = GetStartBlendHandleRect (pMotion, iMotion, PixelScale);
	if (PtInRect (&ItemRect, pt))
	{
		return mieSTARTBLEND;
	}

	ItemRect = GetEndBlendHandleRect (pMotion, iMotion, PixelScale);
	if (PtInRect (&ItemRect, pt))
	{
		return mieENDBLEND;
	}

	ItemRect = GetStartHandleRect (pMotion, iMotion, PixelScale);
	if (PtInRect (&ItemRect, pt))
	{
		return mieMOTIONOFFSET;
	}

	ItemRect = GetEndHandleRect (pMotion, iMotion, PixelScale);
	if (PtInRect (&ItemRect, pt))
	{
		return mieMOTIONEND;
	}

	// then check the entire item
	ItemRect = GetItemRect (pMotion, iMotion, PixelScale);
	if (PtInRect (&ItemRect, pt))
	{
		return mieMOTION;
	}

	return mieNONE;
}

// Determine which item (if any) the passed X,Y coordinates is on.
// Returns NO_MOTION if none.
// If on an item, returns the item index, and *pMouseItem describes which
// of the parts of the item (handle, etc.)
static int GetMouseItem
	(
	  Blender_WindowData *pData,
	  int MouseX,
	  int MouseY,
	  MotionItemEnum *pMouseItem
	)
{
	int iMotion;
	const int NumMotions = geMotion_GetSubMotionCount (pData->BlendedMotion);

	for (iMotion = 0; iMotion < NumMotions; ++iMotion)
	{
		const MotionItemEnum MousePos = ItemMousePos (pData->BlendedMotion, iMotion, pData->PixelsPerSecond, MouseX, MouseY);
		if (MousePos != mieNONE)
		{
			*pMouseItem = MousePos;
			return iMotion;
		}
	}
	return NO_MOTION;
}

// Convert mouse position, which is dialog-relative to blender window coordinates and return.
static POINT MousePosToBlender (HWND hwnd, int xPos, int yPos)
{
	POINT ptMouse;

	ptMouse.x = xPos;
	ptMouse.y = yPos;
	// convert to screen
	ClientToScreen (hwnd, &ptMouse);
	// and then to blender window
	ScreenToClient (GetDlgItem (hwnd, IDC_STATICBLEND), &ptMouse);

	return ptMouse;
}

// Check for and respond to mouse down messages in blender window
static void wm_LButtonDown
	(
	  Blender_WindowData *pData,
	  int xPos,
	  int yPos
	)
{
	const POINT ptMouse = MousePosToBlender (pData->hwnd, xPos, yPos);
	RECT BlenderClientRect;
	int Index;
	MotionItemEnum MouseItem;

	GetClientRect (GetDlgItem (pData->hwnd, IDC_STATICBLEND), &BlenderClientRect);

	// quick test to see if it's in the rect
	if (!PtInRect (&BlenderClientRect, ptMouse))
	{
		return;
	}

	Index = GetMouseItem (pData, ptMouse.x, ptMouse.y, &MouseItem);
	if (Index != NO_MOTION)
	{
		// selecting an item...change current pos
		if (Index != pData->CurrentBlendItem)
		{
			pData->CurrentBlendItem = Index;
			Blender_SetBlendMotionData (pData);
		}
		pData->MouseMode = MouseItem;
		pData->StartDragPoint = ptMouse;
		pData->LastDragPoint = ptMouse;
	}
}

#pragma warning (disable:4100)
static void wm_LButtonUp
	(
	  Blender_WindowData *pData,
	  int xPos,
	  int yPos
	)
{
	// When the left button is released, clear movement mode.
	if (pData->MouseMode != mieNONE)
	{
		pData->MouseMode = mieNONE;
		if (pData->CurrentBlendItem != NO_MOTION)
		{
			Blender_SetBlendMotionData (pData);
		}
	}
}
#pragma warning (default:4100)


// Determine new scale factor given old scale factor, extents, and new delta in pixels
static geFloat ComputeNewTimeScale
	(
	  geFloat StartExtent,
	  geFloat EndExtent,
	  geFloat OldScale,
	  int DeltaPixels,
	  geFloat PixelScale
	)
{
	geFloat Length = EndExtent - StartExtent;		// Length of motion (seconds)
	int ScaledPixelLength = Units_Round (Length * PixelScale / OldScale);
	int NewPixelLength = ScaledPixelLength + DeltaPixels;
	geFloat NewScaledLength = ((geFloat)NewPixelLength) / PixelScale;
	geFloat NewScaleValue = Length / NewScaledLength;

	return NewScaleValue;
}


// Handle mouse movements
// We're really only interested in mouse movements when one of the handles has been grabbed.
//
static void wm_MouseMove
	(
	  Blender_WindowData *pData,
	  int xPos,
	  int yPos
	)
{
	const POINT ptMouse = MousePosToBlender (pData->hwnd, xPos, yPos);
	RECT BlenderClientRect;
	HWND hwndBlender = GetDlgItem (pData->hwnd, IDC_STATICBLEND);
	GetClientRect (hwndBlender, &BlenderClientRect);

	// just ignore it if not dragging.
	if (pData->MouseMode == mieNONE)
	{
		return;
	}

	// If the mouse has been dragged outside the window, then clear the current mode and return.
	if (!PtInRect (&BlenderClientRect, ptMouse))
	{
		pData->MouseMode = mieNONE;
		if (pData->CurrentBlendItem != NO_MOTION)
		{
			Blender_SetBlendMotionData (pData);
		}
		return;
	}


	// If there's a current motion, then update the value based on which handle is being dragged.
	if (pData->CurrentBlendItem != NO_MOTION)
	{
		int DeltaX = ptMouse.x - pData->LastDragPoint.x;
		geFloat DeltaTime = ((geFloat)DeltaX)/pData->PixelsPerSecond;
		Blender_Submotion SubInfo = Blender_GetCompleteSubmotion (pData->BlendedMotion, pData->CurrentBlendItem);

		switch (pData->MouseMode)
		{
			case mieMOTION :	// moving motion
				Blender_SetTimeOffset (pData, (SubInfo.TimeOffset+DeltaTime));
				Blender_NotifyParentOfChange (pData);
				break;

			case mieMOTIONOFFSET :	// moving left-hand scaling handle
			{
				// make sure we're not moving beyond right boundary...
				RECT ItemRect = GetItemRect (pData->BlendedMotion, pData->CurrentBlendItem, pData->PixelsPerSecond);
				if (ptMouse.x != ItemRect.right)
				{
					const geFloat NewScaleValue = ComputeNewTimeScale (SubInfo.StartExtent, SubInfo.EndExtent, SubInfo.TimeScale, -DeltaX, pData->PixelsPerSecond);
					Blender_SetTimeScale (pData, NewScaleValue);

					// left handle also moves time offset...
					Blender_SetTimeOffset (pData, (SubInfo.TimeOffset+DeltaTime));
					Blender_NotifyParentOfChange (pData);
				}
				break;
			}

			case mieMOTIONEND :// moving right-hand scaling handle
			{
				// make sure we're not moving beyond left boundary...
				RECT ItemRect = GetItemRect (pData->BlendedMotion, pData->CurrentBlendItem, pData->PixelsPerSecond);
				if (ptMouse.x != ItemRect.left)
				{
					const geFloat NewScaleValue = ComputeNewTimeScale (SubInfo.StartExtent, SubInfo.EndExtent, SubInfo.TimeScale, DeltaX, pData->PixelsPerSecond);
					Blender_SetTimeScale (pData, NewScaleValue);
					Blender_NotifyParentOfChange (pData);
				}
				break;
			}

			case mieSTARTBLEND :		// moving start handle
			{
				// Make sure we're not moving the start position beyond the end position
				const geFloat NewStartTime = SubInfo.StartBlendTime + (DeltaTime * SubInfo.TimeScale);
				if (NewStartTime < SubInfo.EndBlendTime)
				{
					Blender_CreateNewPath (pData->BlendedMotion, pData->CurrentBlendItem, NewStartTime, SubInfo.StartBlend, SubInfo.EndBlendTime, SubInfo.EndBlend);
					Blender_NotifyParentOfChange (pData);
				}
				break;
			}
			case mieENDBLEND :		// moving end handle
			{
				// Make sure we're not moving the end position to a point before the start position
				const geFloat NewEndTime = SubInfo.EndBlendTime + (DeltaTime * SubInfo.TimeScale);
				if (NewEndTime > SubInfo.StartBlendTime)
				{
					Blender_CreateNewPath (pData->BlendedMotion, pData->CurrentBlendItem, SubInfo.StartBlendTime, SubInfo.StartBlend, NewEndTime, SubInfo.EndBlend);
					Blender_NotifyParentOfChange (pData);
				}
				break;
			}

			default :
				assert (0);
		}
		// update the UI to reflect the change
		Blender_SetBlendMotionData (pData);		
		pData->LastDragPoint = ptMouse;
	}
}

// Another cool switch statement.
static BOOL CALLBACK Blender_DlgProc
	(
	  HWND hwnd,
	  UINT msg,
	  WPARAM wParam,
	  LPARAM lParam
	)
{
	// Get window data.
	Blender_WindowData *pData = Blender_GetWindowData (hwnd);

	switch (msg)
	{
		case WM_INITDIALOG :
		{
			// Need to set up data structure in WM_INITDIALOG.
			pData = (Blender_WindowData *)lParam;
			SetWindowLong (hwnd, GWL_USERDATA, (LONG)pData);
			pData->hwnd = hwnd;

			return Blender_Initialize (pData);
		}

		case WM_DESTROY :
			Blender_ShutdownAll (pData);
			break;

		case WM_PAINT :
			Blender_PaintMotionWindow (pData);
			return 0;

		case WM_COMMAND :
		{
			WORD wNotifyCode = HIWORD(wParam); // notification code 
			WORD wID = LOWORD(wParam);         // item, control, or accelerator identifier 
			HWND hwndCtl = (HWND) lParam;      // handle of control 
 
			return wm_Command (pData, wNotifyCode, wID, hwndCtl);
		}

		case WM_VKEYTOITEM :
		{
			// respond to space pressed in list box by adding that item
			WORD nKey = LOWORD(wParam);          // key value 
			HWND hwndListBox = (HWND) lParam;    // handle of list box 
			
			if (hwndListBox == GetDlgItem (hwnd, IDC_MOTIONSLIST))
			{
				if (nKey == VK_SPACE)	// don't quite know why VK_RETURN won't work
				{
					if (geMotion_GetSubMotionCount (pData->BlendedMotion) < MAX_SUBMOTIONS)
					{
						Blender_AddSelectedMotionToBlend (pData);
					}
				}
			}
			return -1;
		}

		case WM_LBUTTONDOWN :
			wm_LButtonDown (pData, LOWORD (lParam), HIWORD (lParam));
			return 0;

		case WM_LBUTTONUP :
			wm_LButtonUp (pData, LOWORD (lParam), HIWORD (lParam));
			return 0;

		case WM_MOUSEMOVE :
			wm_MouseMove (pData, LOWORD (lParam), HIWORD (lParam));
			return 0;

		default :
			break;
	}
	return FALSE;
}


// Create a blender window and return its window handle.
HWND Blender_Create
	(
	  HWND hwndParent,
	  HINSTANCE hinst,
	  geActor_Def *ActorDef,
	  geMotion *BlendedMotion
	)
{
	HWND hwndBlender;
	Blender_WindowData *pData;

	// allocate and initialize the window's local data
	pData = GE_RAM_ALLOCATE_STRUCT (Blender_WindowData);
	if (pData == NULL)
	{
		return NULL;
	}
	pData->hwnd = NULL;
	pData->hwndParent = hwndParent;
	pData->Instance = hinst;
	pData->ActorDef = ActorDef;
	pData->BlendedMotion = BlendedMotion;
	if (geMotion_GetSubMotionCount (BlendedMotion) > 0)
	{
		pData->CurrentBlendItem = 0;
	}
	else
	{
		pData->CurrentBlendItem = NO_MOTION;
	}
	pData->MouseMode = mieNONE;

	// and create the dialog
	hwndBlender = CreateDialogParam 
	(
	  hinst,
	  MAKEINTRESOURCE (IDD_BLENDER),
	  hwndParent,
	  Blender_DlgProc,
	  (LONG)pData
	);

	return hwndBlender;
}

// Called by parent when a new actor is loaded.
// This function clears current context and displays new blended motion
void Blender_UpdateActor
	(
	  HWND hwndBlender,
	  geActor_Def *ActorDef,
	  geMotion *BlendedMotion
	)
{
	Blender_WindowData *pData = Blender_GetWindowData (hwndBlender);

	pData->ActorDef = ActorDef;
	pData->BlendedMotion = BlendedMotion;
	pData->CurrentBlendItem = NO_MOTION;
	pData->MouseMode = mieNONE;
	Blender_Initialize (pData);
}
