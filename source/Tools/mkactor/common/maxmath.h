/****************************************************************************************/
/*  MAXMATH.H                                                                           */
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description:  3DS MAX compatible transformation functions.                          */
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
#ifndef MAXMATH_H
#define MAXMATH_H

#ifdef __cplusplus
extern "C"
{
#endif

void MaxMath_Transform(const geXForm3d* M, const geVec3d* v, geVec3d* r);
void MaxMath_GetInverse(const geXForm3d* A, geXForm3d* Inv);
void MaxMath_InverseMultiply(const geXForm3d* A, const geXForm3d* B, geXForm3d* M);
void MaxMath_Multiply(const geXForm3d* M1, const geXForm3d* M2, geXForm3d* MProduct);

#ifdef __cplusplus
}
#endif

#endif // MAXMATH_H