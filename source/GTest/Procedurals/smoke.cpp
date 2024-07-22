/****************************************************************************************/
/*  Smoke.c                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
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
#include <Stdlib.h>

#include <Math.h>
#include <algorithm>

#include "Procedural.h"
#include "String.h"
#include "bitmap.h"
#include "RAM.H"
#include "gebmutil.h"

#define SMOOTH_WRAP GE_FALSE

//====================================================================================
//====================================================================================
// 3D Smoke effect
#define     NSMPARTICLES   1024

typedef struct
{
   // S15.16 fixed point
   int32		Velocity[3];
   int32		Position[3];
} Smoke_Data;

typedef struct Procedural
{
	geBitmap		*Bitmap;
	geBitmap_Info	BitmapInfo;

	int32		Width;
	int32		Height;
	int32		Size;

	// Buffer of data about smoke particles
	Smoke_Data	Smbuf[NSMPARTICLES];

	int32		*ZBuffer;				// 16.16 zbuffer
	float		*ZAge;					// ZBuffer age

	int32		NumParticles;
	float		x, y, z, vx, vy, vz;

	int32		PalIndex;
} Procedural;

void Smoke_Destroy(Procedural *Proc);
geBoolean Smoke_Animate(Procedural *Smoke, float ElapsedTime);

// Create a particle, based on Position and Velocity supplied, 
// but with a small reandom deviation
void  Smoke_FillParticle(Smoke_Data *particle,
                         float xp,
                         float yp,
                         float zp,
                         float xv,
                         float yv,
                         float zv);

geBoolean Smoke_Shade(Procedural *Smoke);

geBoolean Smoke_Update(	Procedural *Smoke,
					int nparticles,
					float xp,
					float yp,
					float zp,
					float xv,
					float yv,
					float zv,
					int lightdir);

int32 Smoke_GetPalIndexFromString(const char *Str);
geBoolean Smoke_InitBitmap(Procedural *Proc, geBitmap **ppBitmap);
geBoolean Smoke_InitPalette(Procedural *Proc);

//====================================================================================
//	Smoke_Create
//====================================================================================
Procedural *Smoke_Create(geBitmap **Bitmap, const char *StrParms)
{
	Procedural	*Proc;

	assert(Bitmap);
	//assert(ParmStart);	// Unremark this when implemented!!!!!

	Proc = GE_RAM_ALLOCATE_STRUCT(Procedural);

	if (!Proc)
		goto ExitWithError;

	memset(Proc, 0, sizeof(*Proc));

	{
		char		*Token;
		int32		TokenNum;
		char		Parms[1024];

		Proc->PalIndex = 0;

		Proc->NumParticles = 128;
		
		Proc->x = 6.0f;
		Proc->y = 64.0f;
		Proc->z = 25.0f;

		Proc->vx = 4.0f;
		Proc->vy = 5.0f;
		Proc->vz = 1.0f;

		strcpy(Parms, StrParms);
		
		TokenNum = 0;

		Token = strtok(Parms," \t,+\n\r");

		while(Token)
		{
			switch(TokenNum)
			{
				case 0:
					Proc->PalIndex = Smoke_GetPalIndexFromString(Token);
					break;
				case 1:
					Proc->NumParticles = atoi(Token);
					break;
				case 2:
					Proc->x = (float)atof(Token);
					break;
				case 3:
					Proc->y = (float)atof(Token);
					break;
				case 4:
					Proc->z = (float)atof(Token);
					break;
				case 5:
					Proc->vx = (float)atof(Token);
					break;
				case 6:
					Proc->vy = (float)atof(Token);
					break;
				case 7:
					Proc->vz = (float)atof(Token);
					break;
			}

			TokenNum++;

			if (TokenNum >= 8)
				break;

			Token = strtok(NULL," \t,+\n\r");

		}

	}

	if (!Smoke_InitBitmap(Proc, Bitmap))
		goto ExitWithError;

	if (Proc->x < 0.0f) 
		Proc->x = 0.0f;
	if (Proc->x > Proc->Width-1) 
		Proc->x = (float)Proc->Width-1;
	if (Proc->y < 0.0f) 
		Proc->x = 0.0f;
	if (Proc->y > Proc->Height-1) 
		Proc->y = (float)Proc->Height-1;

	if (!Smoke_Animate(Proc, 0.1f))
	{
		goto ExitWithError;
	}

	return Proc;

	ExitWithError:
	{
		if (Proc)
			Smoke_Destroy(Proc);

		return NULL;
	}
}

//====================================================================================
//	Smoke_Destroy
//====================================================================================
void Smoke_Destroy(Procedural *Proc)
{
	assert(Proc);

	if (Proc->ZBuffer)
	{
		geRam_Free(Proc->ZBuffer);
		Proc->ZBuffer = NULL;
	}
	
	if (Proc->ZAge)
	{
		geRam_Free(Proc->ZAge);
		Proc->ZAge = NULL;
	}

	if (Proc->Bitmap)
	{
		geBitmap_Destroy(&Proc->Bitmap);
		Proc->Bitmap = NULL;
	}

	geRam_Free(Proc);
}

//====================================================================================
//	Smoke_Animate
//====================================================================================
geBoolean Smoke_Animate(Procedural *Smoke, float ElapsedTime)
{
	if (!Smoke->Bitmap)
		return GE_TRUE;
	
	if (!Smoke_Update(Smoke, Smoke->NumParticles, Smoke->x, Smoke->y, Smoke->z, Smoke->vx, Smoke->vy, Smoke->vz, 1))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//====================================================================================
static Procedural_Table Smoke_Table = 
{
	Procedurals_Version,Procedurals_Tag,
	"Smoke",
	Smoke_Create,
	Smoke_Destroy,
	Smoke_Animate
};

/*
//====================================================================================
//====================================================================================
DllExport Procedural_Table *GetProcedural_Table()
{
	return Smoke_GetProcedural_Table();
}
*/

//====================================================================================
//====================================================================================
Procedural_Table *Smoke_GetProcedural_Table(void)
{
	return &Smoke_Table; 
}

//==================================================================================
// Create a particle, based on Position and Velocity supplied, 
// but with a small random deviation
//==================================================================================
void  Smoke_FillParticle(Smoke_Data *particle,
                         float xp,
                         float yp,
                         float zp,
                         float xv,
                         float yv,
                         float zv)
{
   int   tmp;

   particle->Position[0] = (int32)((xp * 0x10000) + ((rand()&1) ? (rand() * xp) : (-rand() * xp)));

   tmp = rand() << 3;
   if(rand() & 1)
      tmp = -tmp;

   particle->Position[1] = (int32)(yp * 0x10000) + tmp;

   tmp = rand() << 1;
   if(rand() & 1)
      tmp = -tmp;

   particle->Position[2] = (int32)(zp * 0x10000) + tmp;

   tmp = rand() << 1;
   if(rand() & 1)
      tmp = -tmp;
   particle->Velocity[0] = (int32)(xv * 0x10000) + tmp;
//   if(rand() & 1)
//      particle->Velocity[0] = -particle->Velocity[0];

   particle->Velocity[1] = (int32)(yv * rand() );
   if(rand() & 1)
      particle->Velocity[1] = -particle->Velocity[1];
   particle->Velocity[2] = (int32)(zv * (rand() << 1));
   if(rand() & 1)
      particle->Velocity[2] = -particle->Velocity[2];
}

//==================================================================================
//
//==================================================================================
geBoolean Smoke_Shade(Procedural *Smoke)
{
	int32	*ZBuffer;
	float	*ZAge;
	uint8	*Bits;
	int		i;

	geBitmap	*Lock;

	if (!geBitmap_LockForWriteFormat(Smoke->Bitmap, &Lock, 0, 0, GE_PIXELFORMAT_8BIT_PAL))
		goto Fail;

	if (!geBitmap_GetInfo(Lock, &(Smoke->BitmapInfo),NULL) )
		goto Fail;

	if (Smoke->BitmapInfo.Format != GE_PIXELFORMAT_8BIT_PAL )
		goto Fail;

	Bits = static_cast<uint8*>(geBitmap_GetBits(Lock));
	ZBuffer = Smoke->ZBuffer;
	ZAge = Smoke->ZAge;

	// Shade the data for the smoke using the z buffer to provide
	// occlusion information.
	for(i=0;i != Smoke->Size-5; i++, ZBuffer++, ZAge++)
	{
		int32	Result;
		int32	Val;

		if(*ZBuffer == 0)
			continue;

		Val = *ZBuffer;

		Result = Val - *(ZBuffer-1);
		Result >>= 16;

		Result += 128;
			
		if(Result > 255)
			Result = 255;
		else if(Result < 0)
			Result = 0;

		// Age the smoke particle
		Result *= (int32)*ZAge;

		Bits[i] = std::min<int>(Result + Bits[i], 255);
		
		if(Val > ZBuffer[1])
			Bits[i+1] = std::max<int>(Bits[i+1]-3,0);
		if(Val > ZBuffer[2])
			Bits[i+1] = std::max<int>(Bits[i+2]-7,0);
		if(Val > ZBuffer[3])
			Bits[i+3] = std::max<int>(Bits[i+3]-10,0);
		if(Val > ZBuffer[4])
			Bits[i+4] = std::max<int>(Bits[i+4]-15,0);
		if(Val > ZBuffer[5])
			Bits[i+5] = std::max<int>(Bits[i+5]-7,0);
	}

	// Smoth the bitmap
	for(i=0;i<2;i++)
	{
		geBitmapUtil_SmoothBits(&Smoke->BitmapInfo, Bits, Bits, 1, SMOOTH_WRAP);
	}

	if (!geBitmap_UnLock(Lock))
		goto Fail;

	return GE_TRUE;

	Fail:
	{
		if (Lock )		
			geBitmap_UnLock(Lock);

		return GE_FALSE;
	}
}

//==================================================================================
//==================================================================================
geBoolean Smoke_Update(	Procedural *Smoke,
					int nparticles,
					float xp,
					float yp,
					float zp,
					float xv,
					float yv,
					float zv,
					int lightdir)
{
	int32		*ZBuffer;
	float		*ZAge;
	Smoke_Data	*Smbuf;
	int32		i;

	ZBuffer = Smoke->ZBuffer;
	ZAge = Smoke->ZAge;
	Smbuf = Smoke->Smbuf;

	// Zero the destination buffer
	memset(ZBuffer,0, Smoke->Size*sizeof(int32));
	memset(ZAge,0, Smoke->Size*sizeof(float));

	// Update Positions for all the particles in the smoke
	// and update the data in the destination buffer
	for(i=0;i<nparticles;i++)
	{
		int   x,y;
		
		if(Smbuf[i].Velocity[0] < 0x1000)
		{
			// recreate the smoke particle if it is too old
			Smoke_FillParticle(&Smbuf[i],xp,yp,zp,xv,yv,zv);
			continue;
		}

		Smbuf[i].Position[0] += Smbuf[i].Velocity[0];
		Smbuf[i].Position[1] += Smbuf[i].Velocity[1];
		Smbuf[i].Position[2] += Smbuf[i].Velocity[2];

		// Damp the Velocity by a percentage of itself
		Smbuf[i].Velocity[0] -= (Smbuf[i].Velocity[0] >> 5);
		Smbuf[i].Velocity[1] -= (Smbuf[i].Velocity[1] >> 5);
		Smbuf[i].Velocity[1] -= 0x800;
		Smbuf[i].Velocity[2] -= (Smbuf[i].Velocity[2] >> 5);

		// Check if particle has moved off the texture map, and if so, recreate it
		if((Smbuf[i].Position[0]>>16) > (Smoke->Width-1))
			Smoke_FillParticle(&(Smbuf[i]),xp,yp,zp,xv,yv,zv);
		else if ((Smbuf[i].Position[0]>>16) < 0)
			Smoke_FillParticle(&(Smbuf[i]),xp,yp,zp,xv,yv,zv);
		else if((Smbuf[i].Position[1]>>16) > (Smoke->Height-1) || (Smbuf[i].Position[1]>>16) < 0)
			Smoke_FillParticle((&Smbuf[i]),xp,yp,zp,xv,yv,zv);

		// Now plot this particle into the destination buffer if its Z is
		// nearer than the one currently there
		// Get integer coordinate
		x = Smbuf[i].Position[0] >> 16;
		y = Smbuf[i].Position[1] >> 16;

		// Do Z buffer operation and age the particle.
		if(Smbuf[i].Position[2] > ZBuffer[(y*Smoke->Width) + x])
		{
			ZBuffer[(y*Smoke->Width) + x] = Smbuf[i].Position[2];
			ZAge[(y*Smoke->Width) + x] = Smbuf[i].Velocity[0] * (2.5f / 0x30000);
		}
	}

	// Shade the smoke
	if (!Smoke_Shade(Smoke))
		return GE_FALSE;

	return GE_TRUE;
}

typedef struct
{
	float	f, a, r, g, b;
} CPoint;

static char PalStr[][256] = {
	"Smoke_PalSlime",
	"Smoke_PalFire",
	"Smoke_PalOrange",
	"Smoke_PalBlue",
};

#define PalStrTableSize (sizeof(PalStr) / sizeof(char[256]))

static CPoint CPoints[PalStrTableSize][3] =
{
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
	30.0f, 70.0f, 0.0f, 30.0f, 0.0f, 
	180.0f, 255.0f, 255.0f, 255.0f, 25.0f,
	
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
	30.0f, 70.0f, 40.0f, 0.0f, 0.0f, 
	180.0f, 255.0f, 255.0f, 255.0f, 25.0f,

	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
	30.0f, 70.0f, 70.0f, 10.0f, 0.0f, 
	180.0f, 255.0f, 255.0f, 255.0f, 25.0f,
	
	0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 
	30.0f, 60.0f, 5.0f, 5.0f, 30.0f, 
	180.0f, 255.0f, 100.0f, 100.0f, 165.0f
};

static uint16 Smoke_LerpColor(float c1, float c2, float Ratio)
{
	float	Val;

	Val = c1+(c2 - c1)*Ratio;

	Val *= 8.0f;

	if (Val > 255.0f)
		Val = 255.0f;

	return (uint16)Val;
}

//====================================================================================
//	Smoke_GetPalIndexFromString
//====================================================================================
int32 Smoke_GetPalIndexFromString(const char *Str)
{
	int32		i;

	for (i=0; i<PalStrTableSize; i++)
	{
		if (!_stricmp(Str, PalStr[i]))
			return i;
	}

	return 0;
}


//====================================================================================
//====================================================================================
geBoolean Smoke_InitBitmap(Procedural *Proc, geBitmap **ppBitmap)
{
	assert( ppBitmap);

	if ( ! *ppBitmap )
	{
		*ppBitmap = geBitmap_Create(128, 128, 1, GE_PIXELFORMAT_8BIT_PAL);
		
		if ( ! *ppBitmap )
			return GE_FALSE;
	}
	else
	{
		geBitmap_CreateRef(*ppBitmap);
		geBitmap_SetColorKey(*ppBitmap,GE_FALSE,0,0);
	}

	if (!geBitmap_SetFormat(*ppBitmap,GE_PIXELFORMAT_8BIT_PAL,GE_FALSE,0,NULL) )
		return GE_FALSE;

	if (!geBitmap_ClearMips(*ppBitmap) )
		return GE_FALSE;

	if (!geBitmap_SetPreferredFormat(*ppBitmap,GE_PIXELFORMAT_16BIT_4444_ARGB) )
		return GE_FALSE;

	if (!geBitmapUtil_SetColor(*ppBitmap,0,0,0,0) )
		return GE_FALSE;

	if (!geBitmap_GetInfo(*ppBitmap, &Proc->BitmapInfo, NULL))
		return GE_FALSE;

	Proc->Bitmap = *ppBitmap;

	if (!Smoke_InitPalette(Proc))
		return GE_FALSE;

	Proc->Width = Proc->BitmapInfo.Width;
	Proc->Height = Proc->BitmapInfo.Height;
	Proc->Size = Proc->Width*Proc->Height;

	Proc->ZBuffer = GE_RAM_ALLOCATE_ARRAY(int32, Proc->Size);
	
	if (!Proc->ZBuffer)
		goto ExitWithError;

	Proc->ZAge = GE_RAM_ALLOCATE_ARRAY(float, Proc->Size);

	if (!Proc->ZAge)
		goto ExitWithError;

	// Clear the buffers out
	memset(Proc->ZBuffer,0, Proc->Size*sizeof(int32));
	memset(Proc->ZAge,0, Proc->Size*sizeof(float));
	memset(Proc->Smbuf, 0, NSMPARTICLES*sizeof(Smoke_Data));

	return GE_TRUE;
	
	ExitWithError:
	{
		if (Proc->ZBuffer)
		{
			geRam_Free(Proc->ZBuffer);
			Proc->ZBuffer = NULL;
		}
	
		if (Proc->ZAge)
		{
			geRam_Free(Proc->ZAge);
			Proc->ZAge = NULL;
		}
	}

	return GE_FALSE;
}

//====================================================================================
//====================================================================================
geBoolean Smoke_InitPalette(Procedural *Proc)
{
	geBitmap_Info	Info;
	geBitmap_Palette *Pal;
	void			*PalData = NULL;
	gePixelFormat	PalFormat;
	int				PalSize;

	Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_ARGB,256);
	if ( ! Pal )
		goto fail;
	if ( ! geBitmap_SetPalette(Proc->Bitmap,Pal) )
		goto fail;
	geBitmap_Palette_Destroy(&Pal);

	if (!geBitmap_GetInfo(Proc->Bitmap,&Info,NULL) )
		goto fail;

	if (Info.Format != GE_PIXELFORMAT_8BIT_PAL )
		goto fail;

	if (!(Pal = geBitmap_GetPalette(Proc->Bitmap)) )
		goto fail;

	if (!geBitmap_Palette_Lock(Pal,&PalData, &PalFormat, &PalSize) )
		goto fail;
	if ( PalSize < 256 )
		goto fail;

	{
		int32	i;
		int32	NumControlPoints, Current;
		CPoint	*pPoint;

		// Fill the palette
		NumControlPoints = 3;
		
		Current = 0;

		pPoint = &CPoints[Proc->PalIndex][0];

		for (i=0; i<256; i++)
		{
			float	Ratio;
			int32	Next;
			uint32	R,G,B,A;
			uint8	*PalPtr;

			Next = Current+1;

			if (Next > NumControlPoints-1)
				Next = NumControlPoints-1;

			Ratio = (float)(i-pPoint[Current].f)/pPoint[Next].f;

			if (Ratio > 1.0f)
				Ratio = 1.0f;
			else if (Ratio < 0.0f)
				Ratio = 0.0f;

			PalPtr = ((uint8 *)PalData) + i * gePixelFormat_BytesPerPel(PalFormat);

			A = (uint32)Smoke_LerpColor(pPoint[Current].a, pPoint[Next].a, Ratio);
			R = (uint32)Smoke_LerpColor(pPoint[Current].r, pPoint[Next].r, Ratio);
			G = (uint32)Smoke_LerpColor(pPoint[Current].g, pPoint[Next].g, Ratio);
			B = (uint32)Smoke_LerpColor(pPoint[Current].b, pPoint[Next].b, Ratio);

			gePixelFormat_PutColor(PalFormat,&PalPtr,R,G,B,A);

			if ((float)i >= pPoint[Next].f)
			{
				Current++;

				if (Current > NumControlPoints-1)
					Current = NumControlPoints-1;
			}
		}
	}

	PalData = NULL;

	if ( ! geBitmap_Palette_UnLock(Pal) )
		return GE_FALSE;

	return GE_TRUE;

fail:

	if ( PalData )
		geBitmap_Palette_UnLock(Pal);

	return GE_FALSE;

}
