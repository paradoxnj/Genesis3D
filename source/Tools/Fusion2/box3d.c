/****************************************************************************************/
/*  box3d.c                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Bounding box related stuff                                            */
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
#include "box3d.h"
#include <assert.h>
#include <float.h>
#include <stdlib.h>

void Box3d_Clear
	(
	  Box3d *b
	)
{
	assert (b != NULL);

	Box3d_Set (b, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

void Box3d_SetBogusBounds
	(
	  Box3d *b
	)
{
	assert (b != NULL);

	geVec3d_Set (&b->Min, FLT_MAX, FLT_MAX, FLT_MAX);
	geVec3d_Set (&b->Max, -FLT_MAX, -FLT_MAX, -FLT_MAX);
}


void Box3d_Set
	(
	  Box3d *b,
	  geFloat x1,
	  geFloat y1,
	  geFloat z1,
	  geFloat x2,
	  geFloat y2,
	  geFloat z2
	)
{
	assert (b != NULL);

	geVec3d_Set 
	(
		&b->Min, 
		min (x1, x2),
		min (y1, y2),
		min (z1, z2)
	);
	geVec3d_Set
	(
		&b->Max,
		max (x1, x2),
		max (y1, y2),
		max (z1, z2)
	);
}

void Box3d_SetSize
	(
	  Box3d *b,
	  geFloat sx,
	  geFloat sy,
	  geFloat sz
	)
{
	// don't allow boxes with negative bounds...
	assert (sx >= 0.0f);
	assert (sy >= 0.0f);
	assert (sz >= 0.0f);

	Box3d_Set (b, -sx/2, -sy/2, -sz/2, sx/2, sy/2, sz/2);
}

void Box3d_AddPoint
	(
	  Box3d *b,
	  geFloat px,
	  geFloat py,
	  geFloat pz
	)
{
	if (px < b->Min.X) b->Min.X = px;
	if (px > b->Max.X) b->Max.X = px;
	if (py < b->Min.Y) b->Min.Y = py;
	if (py > b->Max.Y) b->Max.Y = py;
	if (pz < b->Min.Z) b->Min.Z = pz;
	if (pz > b->Max.Z) b->Max.Z = pz;
}

#ifndef NDEBUG
static geBoolean Box3d_IsValid
	(
	  const Box3d *b
	)
{
	assert (b != NULL);

	return ((b->Min.X <= b->Max.X) &&
			(b->Min.Y <= b->Max.Y) &&
			(b->Min.Z <= b->Max.Z));
}
#endif

static geBoolean Box3d_Intersects
	(
	  const Box3d *b1,
	  const Box3d *b2
	)
{
	assert (Box3d_IsValid (b1));
	assert (Box3d_IsValid (b2));

	if ((b1->Min.X > b2->Max.X) || (b1->Max.X < b2->Min.X)) return GE_FALSE;
	if ((b1->Min.Y > b2->Max.Y) || (b1->Max.Y < b2->Min.Y)) return GE_FALSE;
	if ((b1->Min.Z > b2->Max.Z) || (b1->Max.Z < b2->Min.Z)) return GE_FALSE;

	return GE_TRUE;
}

geBoolean Box3d_Intersection
	(
	  const Box3d *b1,
	  const Box3d *b2,
	  Box3d *bResult
	)
{
	geBoolean rslt;

	assert (Box3d_IsValid (b1));
	assert (Box3d_IsValid (b2));

	rslt = Box3d_Intersects (b1, b2);
	if (rslt && (bResult != NULL))
	{
		Box3d_Set
		(
			bResult,
			max (b1->Min.X, b2->Min.X),
			max (b1->Min.Y, b2->Min.Y),
			max (b1->Min.Z, b2->Min.Z),
			min (b1->Max.X, b2->Max.X),
			min (b1->Max.Y, b2->Max.Y),
			min (b1->Max.Z, b2->Max.Z)
		);
	}
	return rslt;
}

void Box3d_Union
	(
	  const Box3d *b1,
	  const Box3d *b2,
	  Box3d *bResult
	)
{
	assert (Box3d_IsValid (b1));
	assert (Box3d_IsValid (b2));
	assert (bResult != NULL);

	Box3d_Set
	(
		bResult,
		min (b1->Min.X, b2->Min.X),
		min (b1->Min.Y, b2->Min.Y),
		min (b1->Min.Z, b2->Min.Z),
		max (b1->Max.X, b2->Max.X),
		max (b1->Max.Y, b2->Max.Y),
		max (b1->Max.Z, b2->Max.Z)
	);
}

geBoolean Box3d_ContainsPoint
	(
	  const Box3d *b,
	  geFloat px,
	  geFloat py,
	  geFloat pz
	)
{
	assert (Box3d_IsValid (b));

	return ((px >= b->Min.X) && (px <= b->Max.X) &&
			(py >= b->Min.Y) && (py <= b->Max.Y) &&
			(pz >= b->Min.Z) && (pz <= b->Max.Z));
}

const geVec3d *Box3d_GetMin
	(
	  const Box3d *b
	)
{
	assert (Box3d_IsValid (b));

	return &b->Min;
}

const geVec3d *Box3d_GetMax
	(
	  const Box3d *b
	)
{
	assert (Box3d_IsValid (b));

	return &b->Max;
}

void Box3d_GetCenter
	(
	  const Box3d *b,
	  geVec3d *pCenter
	)
{
	assert (Box3d_IsValid (b));
	assert (pCenter != NULL);

	geVec3d_Set 
	(
		pCenter,
		(b->Min.X + b->Max.X)/2,
		(b->Min.Y + b->Max.Y)/2,
		(b->Min.Z + b->Max.Z)/2
	);
}

geFloat Box3d_GetWidth
	(
	  const Box3d *b
	)
{
	assert (Box3d_IsValid (b));

	return (b->Max.X - b->Min.X + 1);
}

geFloat Box3d_GetHeight
	(
	  const Box3d *b
	)
{
	assert (Box3d_IsValid (b));

	return (b->Max.Y - b->Min.Y + 1);
}

geFloat Box3d_GetDepth
	(
	  const Box3d *b
	)
{
	assert (Box3d_IsValid (b));

	return (b->Max.Z - b->Min.Z + 1);
}

void Box3d_GetSize
	(
	  const Box3d *b,
	  geVec3d *pSize
	)
{
	assert (Box3d_IsValid (b));
	assert (pSize != NULL);

	geVec3d_Set
	(
	  pSize,
	  (b->Max.X - b->Min.X + 1),
	  (b->Max.Y - b->Min.Y + 1),
	  (b->Max.Z - b->Min.Z + 1)
	);
}

void Box3d_Scale
	(
	  Box3d *b,
	  geFloat Scale
	)
{
	assert (Box3d_IsValid (b));
	assert (Scale >= 0.0f);

	geVec3d_Scale (&b->Min, Scale, &b->Min);
	geVec3d_Scale (&b->Max, Scale, &b->Max);

	assert (Box3d_IsValid (b));
}

void Box3d_Move
	(
	  Box3d *b,
	  geFloat dx,
	  geFloat dy,
	  geFloat dz
	)
{
	geVec3d VecDelta;

	assert (Box3d_IsValid (b));

	geVec3d_Set (&VecDelta, dx, dy, dz);
	geVec3d_Add (&b->Min, &VecDelta, &b->Min);
	geVec3d_Add (&b->Max, &VecDelta, &b->Max);
}

void Box3d_Inflate
	(
	  Box3d *b,
	  geFloat dx,
	  geFloat dy,
	  geFloat dz
	)
{
	geVec3d VecDelta;

	assert (Box3d_IsValid (b));

	geVec3d_Set (&VecDelta, dx, dy, dz);
	geVec3d_Subtract (&b->Min, &VecDelta, &b->Min);
	geVec3d_Add (&b->Max, &VecDelta, &b->Max);

	assert (Box3d_IsValid (b));
}
