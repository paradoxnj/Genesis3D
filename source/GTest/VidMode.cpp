/****************************************************************************************/
/*  VidMode.c                                                                           */
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

#include <assert.h>

#include "VidMode.h"



#define WIDTH(V)  ((V)>>16)
#define HEIGHT(V) ((V)&0xFFFF)
#define PACK(W,H)  (((W)<<16) + (H))

void VidMode_GetResolution (const VidMode V, int *Width, int *Height)
{
	int W,H;
	assert( Width  != NULL );
	assert( Height != NULL );
	W = WIDTH(V);
	H = HEIGHT(V);
	
	assert( W >= 320  );
	assert( W <  8000 );
	assert( H >= 200  );
	assert( H <  8000 ); 

	*Width = W;
	*Height = H;
}
	
geBoolean VidMode_SetResolution (VidMode *V, int Width, int Height)
{
	assert( V != NULL );
	
	if	(	 	( Width  < 320  ) 
			||  ( Width  > 8000 ) 
			||  ( Height < 200  ) 
			||  ( Height > 8000 )
		)
		return GE_FALSE;
	
	*V = PACK(Width,Height);
	return GE_TRUE;
}

