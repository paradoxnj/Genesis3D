/****************************************************************************************/
/*  box3d.h                                                                             */
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
#ifndef BOX3D_H
#define BOX3D_H

#include "BASETYPE.H"
#include "VEC3D.H"

#ifdef __cplusplus
	extern "C" {
#endif



struct tag_Box3d
{
	geVec3d Min;
	geVec3d Max;
};

typedef struct tag_Box3d Box3d;

// Set all vectors to 0.
void Box3d_Clear
	(
	  Box3d *b
	);

// Create an inverted box (FLT_MAX, FLT_MAX, FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX)
// for use in creating a bounding box.
void Box3d_SetBogusBounds
	(
	  Box3d *b
	);

// Set the values in a box
void Box3d_Set
	(
	  Box3d *b,
	  geFloat x1,
	  geFloat y1,
	  geFloat z1,
	  geFloat x2,
	  geFloat y2,
	  geFloat z2
	);

// Extend a box to encompas the passed point
void Box3d_AddPoint
	(
	  Box3d *b,
	  geFloat px,
	  geFloat py,
	  geFloat pz
	);

// Return result of box intersection.
// If no intersection, returns GE_FALSE and bResult is not modified.
// If intersection, returns GE_TRUE and fills bResult (if not NULL)
// with the intersected box,
// bResult may be one of b1 or b2.
// 
geBoolean Box3d_Intersection
	(
	  const Box3d *b1,
	  const Box3d *b2,
	  Box3d *bResult
	);

// computes union of b1 and b2 and returns in bResult.
// bResult may be one of b1 or b2.
void Box3d_Union
	(
	  const Box3d *b1,
	  const Box3d *b2,
	  Box3d *bResult
	);

geBoolean Box3d_ContainsPoint
	(
	  const Box3d *b,
	  geFloat px,
	  geFloat py,
	  geFloat pz
	);

const geVec3d *Box3d_GetMin
	(
	  const Box3d *b
	);

const geVec3d *Box3d_GetMax
	(
	  const Box3d *b
	);

void Box3d_GetCenter
	(
	  const Box3d *b,
	  geVec3d *pCenter
	);

geFloat Box3d_GetWidth
	(
	  const Box3d *b
	);

geFloat Box3d_GetHeight
	(
	  const Box3d *b
	);

geFloat Box3d_GetDepth
	(
	  const Box3d *b
	);

void Box3d_GetSize
	(
	  const Box3d *b,
	  geVec3d *pSize
	);

// Scale the box
void Box3d_Scale
	(
	  Box3d *b,
	  geFloat Scale
	);

void Box3d_SetSize
	(
	  Box3d *b,
	  geFloat sx,
	  geFloat sy,
	  geFloat sz
	);

void Box3d_Move
	(
	  Box3d *b,
	  geFloat dx,
	  geFloat dy,
	  geFloat dz
	);

void Box3d_Inflate
	(
	  Box3d *b,
	  geFloat dx,
	  geFloat dy,
	  geFloat dz
	);

#ifdef __cplusplus
	}
#endif



#endif

