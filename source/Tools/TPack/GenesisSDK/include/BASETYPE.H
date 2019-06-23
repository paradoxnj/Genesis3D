/****************************************************************************************/
/*  BASETYPE.H                                                                          */
/*                                                                                      */
/*  Author:                                                                             */
/*  Description: Basic type definitions and calling convention defines                  */
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
#ifndef GE_BASETYPE_H
#define GE_BASETYPE_H
 
#ifdef __cplusplus
extern "C" {
#endif

 
//
//  API wide optimization:
//
#define	GENESISCC	_fastcall

#if	defined(BUILDGENESIS) && defined(GENESISDLLVERSION)
  #define GENESISAPI	_declspec(dllexport)
#else
  #if	defined(GENESISDLLVERSION)
    #define GENESISAPI	_declspec(dllimport)
  #else
    #define GENESISAPI
  #endif
#endif

typedef signed int geBoolean;
#define GE_FALSE	(0)
#define GE_TRUE		(1)

typedef float geFloat;

#ifndef NULL
#define NULL	((void *)0)
#endif

#define	GE_PI	((geFloat)3.14159265358979323846f)

typedef signed long     int32;
typedef signed short    int16;
typedef signed char     int8 ;
typedef unsigned long  uint32;
typedef unsigned short uint16;
typedef unsigned char  uint8 ;

#ifdef __cplusplus
}
#endif

#endif
 
