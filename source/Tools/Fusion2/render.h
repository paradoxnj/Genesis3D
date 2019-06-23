/****************************************************************************************/
/*  render.h                                                                            */
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
#ifndef _RENDER_H_
#define _RENDER_H_

#include "brush.h"
#include "bitmap.h"
#pragma warning(disable : 4201 4214 4115 4514)
#include <windows.h>
#pragma warning(default : 4201 4214 4115)

enum ViewFlags
{
	DIBDONE	=1,
};

enum ViewTypes
{
	VIEWSOLID	=1,
	VIEWTEXTURE	=2,
	VIEWWIRE	=4,
	VIEWTOP		=8,
	VIEWFRONT	=16,
	VIEWSIDE	=32
};

enum RenderFlags
{
	NORMAL	=1,
	ZFILL	=2,
	ZBUFFER	=4,
	FLIPPED	=8
};

enum
{
	GRID_TYPE_TEXEL,
	GRID_TYPE_METRIC
};

typedef struct ViewVarsTag ViewVars;
typedef struct GradientsTag Gradients;
typedef struct EdgeAsmTag EdgeAsm;

typedef struct SizeInfoTag
{
	long	TexWidth, TexHeight;
	long	ScreenWidth, ScreenHeight;
	uint8	*TexData, *ScreenData;
	uint32	*ZData;
} SizeInfo;

#ifdef __cplusplus
extern "C" {
#endif

geFloat	Render_GetFineGrid(const ViewVars *v, int GridType);
geFloat Render_ComputeGridDist (const ViewVars *v, int GridType);
SizeInfo	*Render_GetWadSizes(const ViewVars *v);
void		Render_SetWadSizes(ViewVars *v, SizeInfo *ws);
void		Render_ZoomChange(ViewVars *v, const geFloat factor);
void		Render_UpdateGridMetric(ViewVars *v);
void		Render_UpdateGridTexel(ViewVars *v);
int			Render_GetInidx(const ViewVars *v);
void		Render_ViewToWorld(const ViewVars *v, const int x, const int y, geVec3d *wp);
geFloat		Render_ViewDeltaToRadians( const ViewVars *v, const float dx ) ;
void		Render_ViewDeltaToRotation(const ViewVars *v, const float dx, geVec3d *VecRotate);
geVec3d		Render_GetViewCenter (const ViewVars *v);
void		Render_GetCameraPos(const ViewVars *v, geVec3d *pos);
void		Render_SetCameraPos(ViewVars *v, const geVec3d *pos);
void		Render_MoveCamPos(ViewVars *v, const geVec3d *pos);
void		Render_MoveCamPosOrtho(ViewVars *v, const geVec3d *pos);
void		Render_IncrementYaw(ViewVars *v, const geFloat incr);
void		Render_IncrementPitch(ViewVars *v, const geFloat incr);
geFloat	Render_GetXScreenScale(const ViewVars *v);
void		Render_SetViewType(ViewVars *v, const int vt);
void		Render_SetZoom(ViewVars *v, const geFloat vt);
void		Render_ResetSettings(ViewVars *, long vx, long vy);
void		Render_ResizeView (ViewVars *v, long vx, long vy);
ViewVars	*Render_AllocViewVars(void);
void		Render_FreeViewVars(ViewVars *v);
int			Render_GetWidth(const ViewVars *v);
int			Render_GetHeight(const ViewVars *v);
geFloat	Render_GetZoom(const ViewVars *v);
POINT		Render_OrthoWorldToView(const ViewVars *v, const geVec3d *wp);
geVec3d		Render_XFormVert(const ViewVars *v, const geVec3d *pin);
void		Render_UpdateTexInfoId(Plane *p, uint16 TId);
void		Render_GetPitchRollYaw( const ViewVars * v, geVec3d * pPRY ) ;
void		Render_SetPitchRollYaw( ViewVars * v, const geVec3d * pPRY ) ;
geBoolean	Render_UpIsDown (const ViewVars *v);
void		Render_RenderBrushSelFacesOrtho(ViewVars *v, Brush *b, HDC ViewDC);
void		Render_RenderBrushSheetFacesOrtho(ViewVars *Cam, Brush *b, HDC ViewDC);
void		Render_RenderOrthoGrid(ViewVars *Cam, HDC ViewDC);
void		Render_RenderOrthoGrid2(ViewVars *Cam, HDC ViewDC);
void		Render_RenderTreeHollowOrtho(ViewVars *Cam, Node *n, HDC ViewDC);
void		Render_RenderOrthoGridFromSize(ViewVars *v, geFloat Interval, HDC ViewDC);
void		Render_RenderTree(ViewVars *, Node *n, HDC, int RFlags);
void		Render_RenderTreeOrtho(ViewVars *, Node *n, HDC);
void		Render_RenderBrushFacesOrtho( const ViewVars *Cam, Brush *b, HDC);
void		Render_RenderBrushFaces(ViewVars *Cam, Brush *b, uint32 LineColor);
void		Render_RenderBrushFacesZBuffer(ViewVars *Cam, Brush *b, uint32 LineColor);
void		Render_RenderBrushSelFaces(ViewVars *Cam, Brush *b, uint32 LineColor);
void		Render_BackRotateVector(ViewVars *Cam, geVec3d *pin, geVec3d *pout);
BOOL		Render_PointInFrustum(ViewVars *Cam, geVec3d *v);
BOOL		Render_CastRay(Node *n, geVec3d *p1, geVec3d *p2);
int			Render_GetViewType(const ViewVars *v);
void		Render_BlitViewDib(ViewVars *v, HDC ViewDC);
void		Render_ClearViewDib (ViewVars *Cam);
void		Render_SetUpFrustum(ViewVars *v);
void		Render_UpdateViewPos(ViewVars *v);
void		Render_3DTextureZBuffer(ViewVars *Cam, const geVec3d *pos, const geBitmap *bmap);
void		Render_3DTextureZBufferOutline(ViewVars *Cam, const geVec3d *pos, const geBitmap *bmap, uint32 OutlineColor);
//void		Render_RenderBrushHintFacesOrtho(ViewVars *Cam, Brush *b, HDC ViewDC);

#ifdef __cplusplus
}
#endif

#endif
