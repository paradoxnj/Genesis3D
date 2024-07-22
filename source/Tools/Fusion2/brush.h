/****************************************************************************************/
/*  brush.h                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Pollard                      */
/*  Description:  Header for brush.c, important flags                                   */
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
#ifndef _BRUSH_H_
#define _BRUSH_H_

#include <math.h>
#include <stdio.h>
#include "VEC3D.H"
#include "XFORM3D.H"
#include "face.h"
#include "box3d.h"
#include "parse3dt.h"

#ifdef __cplusplus
extern "C" {
#endif

enum LastBrushAction
{
	BRUSH_MOVE,
	BRUSH_ROTATE,
	BRUSH_SCALE,
	BRUSH_SHEAR,
	BRUSH_RESET,
	BRUSH_DIALOG
};

enum BrushTypeFlags
{
	BRUSH_MULTI			=0x0001,
	BRUSH_LEAF			=0x0002, 
	BRUSH_CSG			=0x0004
};

#define BRUSH_COUNT_MULTI 1
#define BRUSH_COUNT_LEAF  2
#define BRUSH_COUNT_CSG   4
#define BRUSH_COUNT_ALL (BRUSH_COUNT_MULTI | BRUSH_COUNT_LEAF | BRUSH_COUNT_CSG)
#define BRUSH_COUNT_NORECURSE 8

typedef struct tag_FaceList FaceList;
typedef struct tag_BrushList BrushList;
typedef struct BrushTag Brush;

//instancing / init
Brush		*Brush_Create(int Type, const FaceList *fl, const BrushList *BList);
void		Brush_Destroy(Brush **b);
Brush		*Brush_Clone(Brush const *from);
Brush		*Brush_CreateHollowFromBrush(const Brush *b);

//access
int			Brush_GetNumFaces(const Brush *b);
Face		*Brush_GetFace(const Brush *b, int i);
int			Brush_GetModelId(const Brush *b);
int			Brush_GetGroupId(const Brush *b);
geFloat		Brush_GetHullSize(const Brush *b);
uint32		Brush_GetColor(const Brush *b);
int			Brush_GetType (const Brush *b);

const char *Brush_GetName(const Brush *b);
Face		*Brush_GetSelectedFace(const Brush *b);
const Box3d	*Brush_GetBoundingBox (const Brush *b);
const BrushList	*Brush_GetBrushList(const Brush *b);
geBoolean	Brush_GetParent(const BrushList	*pList,		//list to search
							const Brush		*b,			//brush to find
							Brush			**bParent);	//parent returned
Brush *		Brush_GetTopLevelParent (const BrushList	*pList, const Brush	*b);

//set elements
void		Brush_SetModelId(Brush *b, const int mid);
void		Brush_SetGroupId(Brush *b, const int gid);
void		Brush_SetHullSize(Brush *b, const geFloat HullSize);
void		Brush_SetColor(Brush *b, const uint32 Color);
void		Brush_SetName(Brush *b, const char *bname);

//check flags
geBoolean	Brush_IsSolid(const Brush *b);
geBoolean	Brush_IsWindow(const Brush *b);
geBoolean	Brush_IsWavy(const Brush *b);
geBoolean	Brush_IsDetail(const Brush *b);
geBoolean	Brush_IsSubtract(const Brush *b);
geBoolean	Brush_IsClip(const Brush *b);
geBoolean	Brush_IsHollow(const Brush *b);
geBoolean	Brush_IsHollowCut(const Brush *b);
geBoolean	Brush_IsVisible(const Brush *b);
geBoolean	Brush_IsLocked(const Brush *b);
geBoolean	Brush_IsHint(const Brush *b);
geBoolean	Brush_IsArea(const Brush *b);
geBoolean	Brush_IsTranslucent(const Brush *b);
geBoolean	Brush_IsEmpty(const Brush *b);
geBoolean	Brush_IsMulti(const Brush *b);
geBoolean	Brush_IsFlocking (const Brush *b);
geBoolean	Brush_IsSheet (const Brush *b);

//set/reset flags
void		Brush_SetSolid(Brush *b, const geBoolean state);
void		Brush_SetWindow(Brush *b, const geBoolean state);
void		Brush_SetWavy(Brush *b, const geBoolean state);
void		Brush_SetDetail(Brush *b, const geBoolean state);
void		Brush_SetSubtract(Brush *b, const geBoolean state);
void		Brush_SetClip(Brush *b, const geBoolean state);
void		Brush_SetHollow(Brush *b, const geBoolean state);
void		Brush_SetVisible(Brush *b, const geBoolean state);
void		Brush_SetLocked(Brush *b, const geBoolean state);
void		Brush_SetHint(Brush *b, const geBoolean state);
void		Brush_SetArea(Brush *b, const geBoolean state);
void		Brush_SetTranslucent(Brush *b, const geBoolean state);
void		Brush_SetEmpty(Brush *b, const geBoolean state);
void		Brush_SetHollowCut(Brush *b, const geBoolean bState);
void		Brush_SetFlocking (Brush *b, const geBoolean bState);
void		Brush_SetSheet (Brush *b, const geBoolean bState);
void		Brush_SetUserFlags (Brush *b, unsigned long Flags);
unsigned long Brush_GetUserFlags (const Brush *b);

//io
void		Brush_WriteToMap(const Brush *b, FILE *ofile, geBoolean VisDetail);
void		Brush_WriteToQuakeMap(const Brush *b, FILE *ofile);
geBoolean	Brush_Write(const Brush *b, FILE *ofile);
Brush		*Brush_CreateFromFile(Parse3dt *Parser, int VersionMajor, int VersionMinor, const char **Expected);

//operations
void		Brush_Resize(Brush *b, float dx, float dy, int sides, int inidx, geVec3d *fnscale, int *ScaleNum);
void		Brush_ResizeFinal(Brush *b, int sides, int inidx, geVec3d *fnscale);
//void		Brush_SnapNearest(Brush *b, geFloat gsize, int sides, int inidx);
void		Brush_SnapShearNearest(Brush *b, geFloat gsize, int sides, int inidx, int snapside);
void		Brush_SnapScaleNearest(Brush *b, geFloat gsize, int sides, int inidx, geVec3d *fnscale, int *ScaleNum);
Face		*Brush_RayCast(const Brush *b, geVec3d *BrushOrg, geVec3d *dir, geFloat *dist);
void		Brush_Move(Brush *b, const geVec3d *trans);
void		Brush_Scale3d(Brush *b, const geVec3d *scalevec);
void		Brush_Transform (Brush *b, const geXForm3d *pXfm);
void		Brush_Rotate (Brush *b, const geXForm3d *pXfmRotate, const geVec3d *pCenter);
void		Brush_Scale (Brush *b, float ScaleFactor);
void		Brush_Shear(Brush *b, const geVec3d *ShearVec, const geVec3d *ShearAxis);
void		Brush_ShearFixed(Brush *b, float dx, float dy, int sides, int inidx, geVec3d *fnscale, int *ScaleNum);
void		Brush_Bound(Brush *b);
geBoolean	Brush_TestBoundsIntersect(const Brush *b, const Box3d *pBox);
void		Brush_SealFaces(Brush **b);
void		Brush_UpdateChildFaces(Brush *b);
void		Brush_SetFaceListDirty(Brush *b);
geBoolean	Brush_SetNextSelectedFace(Brush *b);
geBoolean	Brush_SetPrevSelectedFace(Brush *b);
Face		*Brush_SelectFirstFace(Brush *b);
Face		*Brush_SelectLastFace(Brush *b);
Brush		*Brush_GetNextBrush(Brush *b, BrushList *pList);
Brush		*Brush_GetPrevBrush(Brush *b, BrushList *pList);
void		Brush_SetTextureScale (Brush *b, geFloat ScaleVal);

typedef geBoolean (*BrushList_CB)( Brush *pBrush, void * pVoid ) ;

// Brush list operations
BrushList	*BrushList_Create (void);
BrushList	*BrushList_CreateFromFile (Parse3dt *Parser, int VersionMajor, int VersionMinor, const char **Expected);
void		BrushList_MakeHollowsMulti(BrushList *inList);
void BrushList_Destroy (BrushList **ppList);
void BrushList_Append (BrushList *pList, Brush *pBrush);
void BrushList_InsertAfter(BrushList *pList, Brush *pBMarker, Brush *pBrush);
void BrushList_InsertBefore(BrushList *pList, Brush *pBMarker, Brush *pBrush);
void BrushList_Prepend (BrushList *pList, Brush *pBrush);
void BrushList_Remove (BrushList *pList, Brush *pBrush);
void BrushList_DeleteAll (BrushList *pList);
void BrushList_GetBounds(const BrushList *BList, Box3d *pBounds);
geBoolean BrushList_Write (BrushList *BList, FILE *ofile);
geBoolean BrushList_EnumAll
	(
		BrushList const *pList,
		void *			lParam,
		BrushList_CB	CallBack
	) ;
geBoolean BrushList_Enum 
	(
		BrushList const *pList,
		void *			lParam,
		BrushList_CB	CallBack
	) ;
int BrushList_EnumLeafBrushes
	(
		BrushList const *pList,
		void *				pVoid,
		BrushList_CB		CallBack
	) ;
int BrushList_EnumCSGBrushes
	(
		BrushList const *pList,
		void *				pVoid,
		BrushList_CB		CallBack
	) ;

typedef geBoolean (*Brush_CSGCallback)(const Brush *pBrush, void *lParam);

typedef Brush * BrushIterator;
Brush *BrushList_GetFirst (BrushList *bList, BrushIterator *bi);
Brush *BrushList_GetNext (BrushIterator *bi);
Brush *BrushList_GetLast (BrushList *bList, BrushIterator *bi);
Brush *BrushList_GetPrev (BrushIterator *bi);
int BrushList_Count	(BrushList const *pList, int CountFlags);
BrushList	*BrushList_Clone(BrushList *inList);
void	BrushList_DoCSG(BrushList *inList, int mid, Brush_CSGCallback Callback, void *lParam);
void	BrushList_RebuildHollowFaces(BrushList *inList, int mid, Brush_CSGCallback Callback, void *lParam);
void	BrushList_ClearAllCSG (BrushList *pList);
void	BrushList_ClearCSGAndHollows(BrushList *inList, int mid);
void	BrushList_Move(BrushList *pList, const geVec3d *trans);
void	BrushList_Scale (BrushList *pList, float ScaleFactor);
void	BrushList_Scale3d(BrushList *pList, const geVec3d *trans);
void	BrushList_Transform(BrushList *pList, const geXForm3d *pXfm);
void	BrushList_Rotate(BrushList *pList, const geXForm3d *pXfmRotate, const geVec3d *pCenter);
void	BrushList_Shear(BrushList *pList, const geVec3d *ShearVec, const geVec3d *ShearAxis);
void	Brush_ShearFinal(Brush *b, int sides, int inidx, geVec3d *fnscale);
Brush * BrushList_FindFaceParent (const BrushList *pList, const Face *pFace);
Brush * BrushList_FindTopLevelFaceParent (const BrushList *pList, const Face *pFace);

void	Brush_Center(const Brush *b, geVec3d *center);

typedef geBoolean (*Brush_FaceCallback)(Face *pFace, void *lParam);

void	Brush_EnumFaces (Brush *b, void *lParam, Brush_FaceCallback Callback);

#ifdef __cplusplus
}
#endif

#endif
