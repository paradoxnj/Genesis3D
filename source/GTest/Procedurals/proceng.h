/****************************************************************************/
/*    FILE: ProcEng.h    												    */
/*                                                                          */
/*    Copyright (c) 1999, WildTangent, Inc.; All rights reserved.       */
/*                                                                          */
/****************************************************************************/
#ifndef PROCENG_H
#define PROCENG_H

#include "GENESIS.H"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct	ProcEng			ProcEng;


ProcEng	*	ProcEng_Create(geVFile *CfgFile, geWorld *World);
void		ProcEng_Destroy(ProcEng **pPEng);
geBoolean	ProcEng_AddProcedural(ProcEng *PEng, const char *ProcName, geBitmap **Bitmap, const char * Params);
geBoolean	ProcEng_Animate(ProcEng *PEng, float ElapsedTime);
				// only animates visible procedurals

geBoolean	ProcEng_Minimize(ProcEng *PEng);
				// flush out unused procedurals

#ifdef __cplusplus
}
#endif

#endif
