/****************************************************************************************/
/*  DrvList.h                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  Dialog control logic for driver/mode selection dialog                 */
/*                 (rewritten from previous version)                                    */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/

#ifndef	DRVLIST_H
#define DRVLIST_H

#include "ModeList.h"

#ifdef __cplusplus
extern "C" {
#endif

geBoolean DrvList_PickDriver(HINSTANCE hInstance, HWND hwndParent, 
		ModeList *List, int ListLength, int *ListSelection);

#ifdef __cplusplus
}
#endif

#endif

