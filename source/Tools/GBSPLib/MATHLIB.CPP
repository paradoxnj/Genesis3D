/****************************************************************************************/
/*  MathLib.cpp                                                                         */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>

#include "Math.h"
#include "MathLib.h"

geVec3d	VecOrigin = {0.0f, 0.0f, 0.0f};

//====================================================================================
// ClearBounds
//====================================================================================
void ClearBounds(geVec3d *Mins, geVec3d *Maxs)
{
	Mins->X =  MIN_MAX_BOUNDS;
	Mins->Y =  MIN_MAX_BOUNDS;
	Mins->Z =  MIN_MAX_BOUNDS;

	Maxs->X = -MIN_MAX_BOUNDS;
	Maxs->Y = -MIN_MAX_BOUNDS;
	Maxs->Z = -MIN_MAX_BOUNDS;
}

//=======================================================================================
//	AddPointToBounds
//=======================================================================================
void AddPointToBounds(geVec3d *v, geVec3d *Mins, geVec3d *Maxs)
{
	int32	i;
	float 	Val;

	for (i=0 ; i<3 ; i++)
	{
		Val = VectorToSUB(*v, i);

		if (Val < VectorToSUB(*Mins, i))
			VectorToSUB(*Mins, i) = Val;
		if (Val > VectorToSUB(*Maxs, i))
			VectorToSUB(*Maxs, i) = Val;
	}
}

//=======================================================================================
//	ColorNormalize
//=======================================================================================
geFloat ColorNormalize(geVec3d *C1, geVec3d *C2)
{
	float	Max;

	Max = C1->X;
	if (C1->Y > Max)
		Max = C1->Y;
	if (C1->Z > Max)
		Max = C1->Z;

	if (Max == 0.0f)
		return 0.0f;
	
	geVec3d_Scale(C1, 1.0f/Max, C2);

	return Max;
}

//=======================================================================================
//	ColorClamp
//=======================================================================================
geFloat ColorClamp(geVec3d *C1, geFloat Clamp, geVec3d *C2)
{
	int32	i;
	float	Max, Max2;
	geVec3d	C3;

	Max = -1.0f;

	C3 = *C1;

	for (i=0; i<3; i++)
	{
		if (VectorToSUB(C3, i) < 1.0f)
			VectorToSUB(C3, i) = 1.0f;

		if (VectorToSUB(C3, i) > Max)
			Max = VectorToSUB(C3, i);
	}
	
	Max2 = Max;

	if (Max2 > Clamp)
		Max2 = Clamp;

	geVec3d_Scale(C1, Max2/Max, C2);

	return Max;
}

//=======================================================================================
//	geVec3d_PlaneType
//=======================================================================================
int32 geVec3d_PlaneType(geVec3d *V1)
{
	float	X, Y, Z;

	X = (float)fabs(V1->X);
	Y = (float)fabs(V1->Y);
	Z = (float)fabs(V1->Z);

	if (X == 1.0f)
		return PLANE_X;

	else if (Y == 1.0f)
		return PLANE_Y;

	else if (Z == 1.0f)
		return PLANE_Z;

	if (X >= Y && X >= Z)
		return PLANE_ANYX;

	else if (Y >= X && Y >= Z)
		return PLANE_ANYY;

	else
		return PLANE_ANYZ;
}


