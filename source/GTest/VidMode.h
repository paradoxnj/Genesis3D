/****************************************************************************************/
/*  VidMode.h                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:                                                                        */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#ifndef VIDMODE_H
#define VIDMODE_H

#include "BASETYPE.H"

#ifdef __cplusplus
extern "C" {
#endif


typedef int32 VidMode;		// changed this from an enum with minimal impact on existing data structures

void VidMode_GetResolution (const VidMode V, int *Width, int *Height);
geBoolean VidMode_SetResolution (VidMode *V, int Width, int Height);


#ifdef __cplusplus
}
#endif

#endif