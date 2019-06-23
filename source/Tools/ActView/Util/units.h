/****************************************************************************************/
/*  UNITS.H		                                                                        */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Common constants and unit conversions.								*/
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
#ifndef UNITS_H
#define UNITS_H

#include "basetype.h"
#include <math.h>
 
#ifdef __cplusplus
	extern "C" {
#endif


#define PI2				((geFloat)(2.0f * (GE_PI)))
#define ONE_OVER_2PI	((geFloat)(1.0f/(PI2)))

// some useful unit conversions
#define UNITS_DEGREES_TO_RADIANS(d) Units_DegreesToRadians(d)
#define UNITS_RADIANS_TO_DEGREES(r) Units_RadiansToDegrees(r)

#define Units_DegreesToRadians(d) ((((geFloat)(d)) * GE_PI) / 180.0f)
#define Units_RadiansToDegrees(r) ((((geFloat)(r)) * 180.0f) / GE_PI)

#define Units_Round(n) ((int)Units_FRound((n)))
#define Units_Trunc(n) ((int)(n))
#define Units_FRound(n)	((geFloat)floor((n)+0.5f))

#define Units_MakePercent(fVal) (Units_Round(fVal*100.0f))
#define Units_FloatFromPercent(iVal) (((float)iVal)/100.0f)

#ifdef __cplusplus
	}
#endif

#endif
 
