/****************************************************************************************/
/*  AutoSelect.h                                                                        */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:    Attempts to automatically choose a good video mode                  */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#ifndef AUTOSELECT_H
#define AUTOSELECT_H

#include "GENESIS.H"
#include "ModeList.h"

#ifdef __cplusplus
extern "C" {
#endif

void AutoSelect_SortDriverList(ModeList *MList, int ListLength);
geBoolean AutoSelect_PickDriver(HWND hWnd, geEngine *Engine,ModeList *List, int ModeListLength, int *Selection);

#ifdef __cplusplus
}
#endif

#endif

