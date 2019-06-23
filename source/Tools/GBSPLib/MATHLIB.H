/****************************************************************************************/
/*  MathLib.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Various math functions not included in Vec3d.h, etc...                 */
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
#ifndef MATHLIB_H
#define MATHLIB_H

#include "Vec3d.h"

//#define	ON_EPSILON			(geFloat)0.05
#define		ON_EPSILON			(geFloat)0.1
//#define	ON_EPSILON			0.05f
#define		VCOMPARE_EPSILON	ON_EPSILON 

//#define	MIN_MAX_BOUNDS		8192.0f
#define		MIN_MAX_BOUNDS		15192.0f
#define		MIN_MAX_BOUNDS2		MIN_MAX_BOUNDS*2

#define		VectorToSUB(a, b)	(*(((geFloat*)&a) + b) )

#define		PLANE_X				0
#define		PLANE_Y				1
#define		PLANE_Z				2
#define		PLANE_ANYX			3
#define		PLANE_ANYY			4
#define		PLANE_ANYZ			5
#define		PLANE_ANY			6		

extern		geVec3d				VecOrigin;

void		ClearBounds(geVec3d *Mins, geVec3d *Maxs);
void		AddPointToBounds(geVec3d *v, geVec3d *Mins, geVec3d *Maxs);

geFloat		ColorNormalize(geVec3d *C1, geVec3d *C2);
geFloat		ColorClamp(geVec3d *C1, geFloat Clamp, geVec3d *C2);

int32		geVec3d_PlaneType(geVec3d *V1);

#endif
