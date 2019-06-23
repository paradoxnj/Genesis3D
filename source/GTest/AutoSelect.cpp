/****************************************************************************************/
/*  AutoSelect.c                                                                        */
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

#include    <stdlib.h>  // qsort 
#include	<assert.h>
#include    <Windows.h>


#include	"AutoSelect.h"

#include    "ErrorLog.h"
#include    "Ram.h"
#include	"Gamemgr.h"



static int AutoSelect_Compare( const void *arg1, const void *arg2 )
{
	/* used for quicksort comparison.  
		returns <0 if arg1 is 'smaller than' arg2
		returns >0 if arg1 is 'greater than' arg2
		returns =0 if arg1 is 'the same as'  arg2
		   since the list is sorted by most desirable mode/resolution first, the better mode is 'smaller than'
	*/
	#define A1_BETTER  (-1)
	#define A2_BETTER  ( 1)
	#define TIE        ( 0)

	int Compare = 0;
	ModeList *A1,*A2;
	assert(arg1);
	assert(arg2);
	A1 = (ModeList *)arg1;
	A2 = (ModeList *)arg2;

	// sort by DriverType		 (smaller enum value first)
	// then by windowed
	// then by Width             (larger width first)
	// then by Height            (larger height first) 
	
	if      ( A1->DriverType < A2->DriverType )
		return A1_BETTER;
	else if ( A2->DriverType < A1->DriverType )
		return A2_BETTER;

	if		( !A1->InAWindow && A2->InAWindow )
		return A1_BETTER;
	if		( A1->InAWindow && !A2->InAWindow )
		return A2_BETTER;

	if      ( A1->Width > A2->Width )
		return A1_BETTER;
	else if ( A2->Width > A1->Width )
		return A2_BETTER;

	if      ( A1->Height > A2->Height )
		return A1_BETTER;
	else if ( A2->Height > A1->Height )
		return A2_BETTER;

	return TIE;
}

void AutoSelect_SortDriverList(ModeList *MList, int ListLength)
{
	qsort( MList, ListLength, sizeof( MList[0] ), AutoSelect_Compare );
}
	
//extern 	void GameMgr_ResetMainWindow(HWND hWnd, int32 Width, int32 Height);

// assumes list is sorted!	
geBoolean AutoSelect_PickDriver(HWND hWnd, geEngine *Engine,ModeList *DriverList, int ListLength, int *Selection)
{
	int i;

	assert( Engine      != NULL );
	assert( DriverList  != NULL );
	assert( Selection   != NULL );

	for (i=0; i<ListLength; i++)
		{
			if (DriverList[i].Evaluation == MODELIST_EVALUATED_OK)
				{
					if (DriverList[i].InAWindow)
						{
							GameMgr_ResetMainWindow(hWnd, DriverList[i].Width, DriverList[i].Height);
						}

					if (!geEngine_SetDriverAndMode(Engine, DriverList[i].Driver, DriverList[i].Mode))
						{
							geErrorLog_AddString(-1, "AutoSelect_ModeAndDriver:  driver mode set failed.  continuing.  Driver:", DriverList[i].DriverNamePtr);
							geErrorLog_AddString(-1, "AutoSelect_ModeAndDriver:  driver mode set failed.  continuing.    Mode:", DriverList[i].ModeNamePtr);
						}
					else 
						{
							*Selection = i;
							return GE_TRUE;
						}
				}
		}

	return GE_FALSE;
}
