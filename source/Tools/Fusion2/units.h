/****************************************************************************************/
/*  units.h                                                                             */
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
#ifndef UNITS_H
#define UNITS_H

#include "BASETYPE.H"
#include <math.h>
 
#ifdef __cplusplus
	extern "C" {
#endif


#ifndef M_PI
	#define	M_PI		((geFloat)3.14159265358979323846f)
#endif

#define PI2				((geFloat)(2.0f * (M_PI)))
#define ONE_OVER_2PI	((geFloat)(1.0f/(PI2)))

// some useful unit conversions
#define UNITS_DEGREES_TO_RADIANS(d) Units_DegreesToRadians(d)
#define UNITS_RADIANS_TO_DEGREES(r) Units_RadiansToDegrees(r)

#define Units_DegreesToRadians(d) ((((geFloat)(d)) * M_PI) / 180.0f)
#define Units_RadiansToDegrees(r) ((((geFloat)(r)) * 180.0f) / M_PI)

// Engine <--> Centimeter conversions
#define Units_CentimetersToEngine(c) (((float)(c)) / 2.54f)
#define Units_EngineToCentimeters(i) (((float)(i)) * 2.54f)


#define CENTIMETERS_TO_ENGINE(c) Units_CentimetersToEngine(c)
#define ENGINE_TO_CENTIMETERS(e) Units_EngineToCentimeters(e)

#define Units_Round(n) ((int)Units_FRound((n)))
#define Units_Trunc(n) ((int)(n))
#define Units_FRound(n)	((geFloat)floor((n)+0.5f))

#ifdef __cplusplus
	}
#endif

#endif
 
