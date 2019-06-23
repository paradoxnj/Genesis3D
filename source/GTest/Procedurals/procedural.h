/****************************************************************************/
/*    FILE: Procedural.h												    */
/*                                                                          */
/*    Copyright (c) 1999, WildTangent, Inc.; All rights reserved.       */
/*                                                                          */
/****************************************************************************/
#ifndef PROCEDURAL_H
#define PROCEDURAL_H

#include "Bitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

//====================================================================================

typedef struct Procedural	Procedural;

//====================================================================================

typedef Procedural *	PROC_CREATE(geBitmap **Bitmap, const char *ParmStart);
typedef void 			PROC_DESTROY(Procedural *Proc);
typedef geBoolean 		PROC_ANIMATE(Procedural *Proc, float ElapsedTime); // ElapsedTime in Millisecs

#define Procedurals_Version		(0)
#define Procedurals_Tag			(0x50724F63)	//PrOc

	// when you define a procedural table, the first two lines are
	//	Procedurals_Version,Procedurals_Tag

typedef struct
{
	uint32			Version;
	uint32			Tag;

	const char		Name[256];

	// Init/Destroy funcs
	PROC_CREATE		*Create;
	PROC_DESTROY	*Destroy;

	// Access funcs
	PROC_ANIMATE	*Animate;

	// Edit / Interactive interface functions
	// PROC_*

} Procedural_Table;

#ifdef __cplusplus
}
#endif

#endif
