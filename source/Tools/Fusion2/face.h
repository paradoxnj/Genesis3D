/****************************************************************************************/
/*  face.h                                                                              */
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
#ifndef _FACE_H_
#define _FACE_H_

#include "Vec3d.h"
#include "xform3d.h"
#include "box3d.h"
#include <stdio.h>
#include "Parse3dt.h"


typedef struct
{
	geVec3d uVec, vVec;
	geFloat uOffset, vOffset;
} TexInfo_Vectors;


#ifdef __cplusplus
extern "C" {
#endif

typedef struct NodeTag Node;

typedef struct PlaneTag
{
	geVec3d		Normal;
	geFloat		Dist;
} Plane;

typedef struct FaceTag Face;

//instancing / init
Face	*Face_Create(int NumPnts, const geVec3d *pnts, int DibId);
Face	*Face_CreateFromPlane(const Plane *p, geFloat Radius, int DibId);
void	Face_Destroy(Face **);
Face	*Face_Clone(const Face *src);
Face	*Face_CloneReverse(const Face *src);

//access
int						Face_GetNumPoints(const Face *f);
geFloat					Face_GetValue(const Face *f);
const Plane				*Face_GetPlane(const Face *f);
const geVec3d			*Face_GetPoints(const Face *f);
int						Face_GetLightIntensity(const Face *f);
void					Face_GetLightScale(const Face *f, geFloat *pxScale, geFloat *pyScale);
void					Face_GetTextureScale(const Face *f, geFloat *pxScale, geFloat *pyScale);
void					Face_GetTextureShift(const Face *f, int *pxShift, int *pyShift);
geFloat					Face_GetTextureRotate(const Face *f);
const TexInfo_Vectors	*Face_GetTextureVecs(const Face *f);
int						Face_GetTextureDibId(const Face *f);
char const				*Face_GetTextureName(const Face *f);
geFloat					Face_GetMipMapBias(const Face *f);
geFloat					Face_GetTranslucency(const Face *f);
geFloat					Face_GetReflectivity(const Face *f);

//check flags
geBoolean	Face_IsSelected(const Face *f);
geBoolean	Face_IsLight(const Face *f);
geBoolean	Face_IsMirror(const Face *f);
geBoolean	Face_IsFullBright(const Face *f);
geBoolean	Face_IsSky(const Face *f);
geBoolean	Face_IsFixedHull(const Face *f);
geBoolean	Face_IsGouraud(const Face *f);
geBoolean	Face_IsFlat(const Face *f);
geBoolean	Face_IsTextureLocked (const Face *f);
geBoolean	Face_IsVisible (const Face *f);
geBoolean	Face_IsSheet(const Face *f);
geBoolean	Face_IsTransparent(const Face *f);

//set/reset flags
void	Face_SetSelected(Face *f, const geBoolean state);
void	Face_SetLight(Face *f, const geBoolean state);
void	Face_SetMirror(Face *f, const geBoolean state);
void	Face_SetFullBright(Face *f, const geBoolean state);
void	Face_SetSky(Face *f, const geBoolean state);
void	Face_SetFixedHull(Face *f, const geBoolean state);
void	Face_SetGouraud(Face *f, const geBoolean state);
void	Face_SetFlat(Face *f, const geBoolean state);
void	Face_SetTextureLock(Face *f, const geBoolean state);
void	Face_SetVisible(Face *f, const geBoolean state);
void	Face_SetSheet(Face *f, const geBoolean state);
void	Face_SetTransparent(Face *f, const geBoolean state);

//set elements
void	Face_SetLightIntensity(Face *f, const int Value);
void	Face_SetLightScale(Face *f, const geFloat xScale, const geFloat yScale);
void	Face_SetTextureScale(Face *f, const geFloat xScale, const geFloat yScale);
void	Face_SetTextureShift(Face *f, const int xShift, const int yShift);
void	Face_SetTextureRotate(Face *f, const geFloat Rotate);
void	Face_SetTextureDibId(Face *f, const int Dib);
void	Face_SetTextureName(Face *f, const char *pName);
void	Face_SetTextureSize (Face *f, const int txSize, const int tySize);
void	Face_SetMipMapBias (Face *f, const geFloat Bias);
void	Face_SetTranslucency (Face *f, const geFloat trans);
void	Face_SetReflectivity (Face *f, const geFloat rscale);
void	Face_CopyTexInfo(const Face *src, Face *dst);
void	Face_CopyFaceInfo(const Face *src, Face *dst);

//operations
void	Face_Rotate(Face *f, const geXForm3d *pXfmRotate, const geVec3d *pCenter);
void	Face_Move(Face *f, const geVec3d *trans);
void	Face_Transform(Face *f, const geXForm3d *pXfm);
void	Face_Scale(Face *f, const geVec3d *ScaleVec);
void	Face_Shear(Face *f, const geVec3d *ShearVec, const geVec3d *ShearAxis);
void	Face_GetBounds(const Face *f, Box3d *b);
void	Face_GetCenter(const Face *f, geVec3d *pCenter);
void	Face_GetSplitInfo(const Face *f, const Plane *p, geFloat *dists, uint8 *sides, uint8 *cnt);
void	Face_Split(const Face	*f,		//original face
				   const Plane	*p,		//split plane
				   Face			**ff,	//front face (null)
				   Face			**bf,	//back face (null)
				   geFloat		*dists,	//plane dists per vert
				   uint8		*sides);//plane sides per vert
void	Face_Clip(Face *f, const Plane *p, geFloat *dists, uint8 *sides);
geFloat	Face_PlaneDistance(const Face *f, geVec3d *pos);
void	Face_MostlyOnSide(const Face *f, const Plane *p, geFloat *max, int *side);

//io
void	Face_WriteToMap(const Face *f, FILE *wf);
void	Face_WriteToQuakeMap(const Face *f, FILE *wf);
geBoolean Face_Write(const Face *f, FILE *wf);
Face	*Face_CreateFromFile(Parse3dt *Parser, int VersionMajor, int VersionMinor, const char **Expected);

#ifdef __cplusplus
}
#endif

#endif