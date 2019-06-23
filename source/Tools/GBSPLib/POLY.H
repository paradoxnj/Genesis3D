/****************************************************************************************/
/*  Poly.h                                                                              */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Various Poly routines (clipping, splitting, etc)                       */
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
#ifndef POLY_H
#define POLY_H

#include <Windows.h>

#include "BSP.h"
#include "MathLib.h"

extern geBoolean	gCountVerts;
extern int32		gTotalVerts;
extern int32		gPeekVerts;	

//
//	Polys
//
GBSP_Poly	*AllocPoly(int32 NumVerts);
void		FreePoly(GBSP_Poly *Poly);
GBSP_Poly	*CreatePolyFromPlane(GBSP_Plane *Plane);
geBoolean	ClipPoly(GBSP_Poly *InPoly, GBSP_Plane *Plane, geBoolean FlipTest, GBSP_Poly **OutPoly);
geBoolean	ClipPolyEpsilon(GBSP_Poly *InPoly, geFloat Epsilon, GBSP_Plane *Plane, geBoolean FlipTest, GBSP_Poly **OutPoly);
geBoolean	SplitPoly(GBSP_Poly *InPoly, GBSP_Plane *Plane, GBSP_Poly **Front, GBSP_Poly **Back, 
			   geBoolean FlipTest);
geBoolean	SplitPolyEpsilon(GBSP_Poly *InPoly, geFloat Epsilon, GBSP_Plane *Plane, GBSP_Poly **Front, GBSP_Poly **Back, 
			   geBoolean FlipTest);
float		PolyArea(GBSP_Poly *Poly);
geBoolean	CopyPoly(GBSP_Poly *In, GBSP_Poly **Out);
GBSP_Poly	*CopyPoly2(GBSP_Poly *In);
geBoolean	ReversePoly(GBSP_Poly *In, GBSP_Poly **Out);
void		RemoveDegenerateEdges(GBSP_Poly *Poly);
geBoolean	RemoveDegenerateEdges2(GBSP_Poly *Poly);
void		PolyCenter(GBSP_Poly *Poly, geVec3d *Center);
geBoolean	PolyIsTiny (GBSP_Poly *Poly);

//
//	Faces
//

GBSP_Face	*AllocFace(int32 NumVerts);
void		FreeFace(GBSP_Face *Face);
geBoolean	SplitFace(GBSP_Face *In, GBSP_Plane *Split, GBSP_Face **Front, GBSP_Face **Back, geBoolean FlipTest);
void		FreeFaceList(GBSP_Face *List);
int32		MergeFaceList(GBSP_Face *In, GBSP_Face **Out, geBoolean Mirror);
int32		EdgeExist(geVec3d *Edge1, GBSP_Poly *Poly, int32 *EdgeIndexOut);
GBSP_Face	*MergeFace(GBSP_Face *Face1, GBSP_Face *Face2);
geBoolean	CheckFace(GBSP_Face *Face, geBoolean Verb);
void		GetFaceListBOX(GBSP_Face *Faces, geVec3d *Mins, geVec3d *Maxs);

extern int32	NumSubdivides;
extern float	SubdivideSize;

#endif
