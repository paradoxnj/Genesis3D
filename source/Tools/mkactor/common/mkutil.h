/****************************************************************************************/
/*  MKUTIL.H																			*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Actor make process utility functions.									*/
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
#ifndef __MKUTIL_H__
#define __MKUTIL_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*MkUtil_Printf)(const char *Fmt, ...);

// These are the return codes for command line apps as defined by Eli
typedef enum
{
	RETURN_SUCCESS = 0,
	RETURN_WARNING = 1,
	RETURN_ERROR = 2,
	RETURN_USAGE = 999, // SLB added this
	RETURN_NOACTION = 9999, // SLB added this
} ReturnCode;

void MkUtil_AdjustReturnCode(ReturnCode* pToAdjust, ReturnCode AdjustBy);
int MkUtil_Interrupt(void);

// Gotta have my booleans
typedef unsigned char MK_Boolean;
#define MK_TRUE (MK_Boolean)1
#define MK_FALSE (MK_Boolean)0


#ifdef __cplusplus
}
#endif

#endif // __MKUTIL_H__