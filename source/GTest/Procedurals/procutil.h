#ifndef PROCUTIL_H
#define PROCUTIL_H

#include "BASETYPE.H"
#include "bitmap.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PI
#undef PI
#endif
#ifdef TWO_PI
#undef TWO_PI
#endif

#define PI							(3.14159265359f)
#define PI_OVER_2					(1.570796326795f)
#define	TWO_PI						(6.28318530718f)
#define PI_OVER_180					(0.01745329251994f)
#define RAD_TO_DEG					(57.29577951308f)	//180_over_pi
#define ONE_OVER_360				(0.002777777777778f)

#define ProcUtil_Deg2Rad(a)	((a) * PI_OVER_180)
#define ProcUtil_Rad2Deg(a) ((a) * RAD_TO_DEG)

#define TABLE_SHIFT					(10)
#define TABLE_SIZE					((1UL)<<TABLE_SHIFT)
#define TABLE_MASK					(TABLE_SIZE - 1)

#define TABLE_SIZE_OVER_TWO_PI		((float)TABLE_SIZE / TWO_PI)

/****** you must call _Init before using ProcUtil functions ***/

void ProcUtil_Init(void);

/****** Stuff ***/

geBoolean ProcUtil_SetPaletteFromString(geBitmap * Bitmap,char ** pParams);

/*** fast Rand() utilities *****/

extern void		ProcUtil_Randomize( void );
extern uint32	ProcUtil_Rand( uint32 max );

#define ProcUtil_RandSigned(max)			( ProcUtil_Rand(max+max+1) - (max) )
#define ProcUtil_RandUnitFloat()			( ProcUtil_Rand(13729) * (1.0f/13729.0f))
#define ProcUtil_RandFloat(max)				( max * ProcUtil_RandUnitFloat())
#define ProcUtil_RandSignedUnitFloat()		( ProcUtil_RandSigned(13729) * (1.0f/13729.0f))
#define ProcUtil_RandSignedFloat(max)		( max * ProcUtil_RandSignedUnitFloat())

/*** fast Sin/Cos with tables *****/

extern float ProcUtil_SinTable[];
extern float ProcUtil_CosTable[];
extern int   ProcUtil_ByteSinTable[];
extern int   ProcUtil_ByteCosTable[];

#define ProcUtil_Sin(a)	( ProcUtil_SinTable[((int)((a) * TABLE_SIZE_OVER_TWO_PI) + (1UL<<20)) & TABLE_MASK] )
#define ProcUtil_Cos(a)	( ProcUtil_CosTable[((int)((a) * TABLE_SIZE_OVER_TWO_PI) + (1UL<<20)) & TABLE_MASK] )

/** takes integer in (0-255) to (Trig * 127 + 128)  **/

#define ProcUtil_ByteSin(a)	( ProcUtil_ByteSinTable[(int)((a) + (1UL<<20)) & 0xFF] )
#define ProcUtil_ByteCos(a)	( ProcUtil_ByteCosTable[(int)((a) + (1UL<<20)) & 0xFF] )

/******* __inline functions ****************/

float __inline ProcUtil_Sqrt(float val)
{
	__asm {
		FLD val
		FSQRT
		FSTP val
	}
return val;
}

#ifdef __cplusplus
}
#endif

#endif
