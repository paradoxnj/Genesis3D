/****************************************************************************************/
/*  Water.c                                                                             */
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

#include "Procedural.h"
#include "String.h"
#include "bitmap.h"
#include "RAM.H"

typedef struct Procedural
{
	float		PosX;
	float		PosY;
	float		Angle;
	float		TimeToSplashWater;

	int32		NPage;

	uint16		*OriginalBits;
	int16		*WaterData[2];

	geBitmap	*Bitmap;

	int32		Width;
	int32		Height;
	int32		Size;

	uint8		BlendLut[32][32];
} Procedural;

Procedural *Water_Create(geBitmap **Bitmap, const char *StrParms);
void Water_Destroy(Procedural *Water);
geBoolean Water_Animate(Procedural *Water, float ElapsedTime);
geBoolean Water_ApplyToBitmap(Procedural *Water);
void Water_Update(Procedural *Water, float Time);
void Water_BuildRGBLuts(Procedural *Water, float RScale, float GScale, float BScale);

//====================================================================================
//	Water_Create
//====================================================================================
Procedural *Water_Create(geBitmap **Bitmap, const char *StrParms)
{
	Procedural	*Water;

	assert(Bitmap);
	//assert(ParmStart);	// Unremark this when implemented!!!!!

	Water = GE_RAM_ALLOCATE_STRUCT(Procedural);

	if (!Water)
		goto ExitWithError;

	memset(Water, 0, sizeof(*Water));

	if (*Bitmap)
	{
		Water->Bitmap = *Bitmap;

		geBitmap_CreateRef(Water->Bitmap);
	}
	else
	{
		// Must make bitmap for tha caller!!!
		goto ExitWithError;
	}

	// We need this to be only 1 miplevel!!!
	if (!geBitmap_ClearMips(Water->Bitmap))
		goto ExitWithError;
	
	{
		// We need to change the format of this bitmap to a 565 in the world (hope he doesn't mind ;)
		gePixelFormat	Format;
		geBitmap_Info	Info;
		int32			i;

		Format = GE_PIXELFORMAT_16BIT_565_RGB;

		if (!geBitmap_SetFormat(Water->Bitmap, Format, GE_FALSE, 0, NULL))
			goto ExitWithError;

		if (!geBitmap_GetInfo(Water->Bitmap, &Info, NULL))
			goto ExitWithError;

		Water->Width = Info.Width;
		Water->Height = Info.Height;
		Water->Size = Water->Width*Water->Height;

		Water->OriginalBits = GE_RAM_ALLOCATE_ARRAY(uint16, Water->Size);

		if (!Water->OriginalBits)
			goto ExitWithError;

		for (i=0; i<2; i++)
		{
			Water->WaterData[i] = GE_RAM_ALLOCATE_ARRAY(int16, Water->Size);

			if (!Water->WaterData[i])
				goto ExitWithError;

			memset(Water->WaterData[i], 0, sizeof(int16)*Water->Size);
		}

	}

	// Get the original bits...
	{
		geBitmap		*Src;
		uint16			*pSrc16;
		int32			i;
		geBitmap_Info	Info;

		if (!geBitmap_GetInfo(Water->Bitmap, &Info, NULL))
			goto ExitWithError;

		if (!geBitmap_LockForRead(Water->Bitmap, &Src, 0, 0, Info.Format, GE_TRUE, 255))
			goto ExitWithError;
		
		pSrc16 = static_cast<uint16*>(geBitmap_GetBits(Src));

		for (i=0; i<Water->Size; i++)
			Water->OriginalBits[i] = pSrc16[i];

		if (!geBitmap_UnLock(Src))
			goto ExitWithError;
		
	}

	Water_BuildRGBLuts(Water, 1.0f, 1.0f, 1.0f);

	return Water;

	ExitWithError:
	{
		if (Water)
			Water_Destroy(Water);

		return NULL;
	}
}

//====================================================================================
//	Water_Destroy
//====================================================================================
void Water_Destroy(Procedural *Water)
{
	assert(Water);

	if (Water->Bitmap)
	{
		geBitmap_Destroy(&Water->Bitmap);
		Water->Bitmap = NULL;
	}
	
	if (Water->OriginalBits)
		geRam_Free(Water->OriginalBits);
	Water->OriginalBits = NULL;

	if (Water->WaterData[0])
		geRam_Free(Water->WaterData[0]);
	Water->WaterData[0] = NULL;

	if (Water->WaterData[1])
		geRam_Free(Water->WaterData[1]);
	Water->WaterData[1] = NULL;

	geRam_Free(Water);
}

//====================================================================================
//	Water_Animate
//====================================================================================
geBoolean Water_Animate(Procedural *Water, float ElapsedTime)
{
	if (!Water->Bitmap)
		return GE_TRUE;

	Water_Update(Water, ElapsedTime);

	if (!Water_ApplyToBitmap(Water))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//====================================================================================
geBoolean Water_ApplyToBitmap(Procedural *Water)
{
	geBitmap		*Dest;
	uint8			*pBlendLut;

	assert(Water->Bitmap);

#if 0 //@@ CB BUG Fix!
	{
	geBitmap_Info	MainInfo, Secondary;
	//if (!geBitmap_GetInfo(Water->Bitmap, &Secondary, &MainInfo))
 	if (!geBitmap_GetInfo(Water->Bitmap, &MainInfo, &Secondary))
		return GE_FALSE;

	assert(MainInfo.MaximumMip == 0);

	if (MainInfo.Format != GE_PIXELFORMAT_16BIT_565_RGB)
		return GE_TRUE;			// Oh well...
	
	if (!geBitmap_LockForWrite(Water->Bitmap, &Dest, 0, 0))
		return GE_FALSE;
	}
#else
	if (!geBitmap_LockForWriteFormat(Water->Bitmap, &Dest, 0, 0, GE_PIXELFORMAT_16BIT_565_RGB))
		return GE_FALSE;
		
#endif

	pBlendLut = &Water->BlendLut[0][0];
	
	{
		uint16			*pSrc16, *pDest16;
		int16			*pWSrc16, *pOriginalWSrc16;
		int32			w, h, Extra, WMask, HMask;
		geBitmap_Info	Info;

	//	if (!geBitmap_GetInfo(Dest, &Secondary, &Info)) // CB BUG Fix!
		if (!geBitmap_GetInfo(Dest, &Info, NULL))
			return GE_FALSE;
		
		assert(Info.Format == GE_PIXELFORMAT_16BIT_565_RGB);
		
		Extra = Info.Stride - Info.Width;

		WMask = Info.Width - 1;
		HMask = Info.Height - 1;
		
		pSrc16 = Water->OriginalBits;
		pOriginalWSrc16 = pWSrc16 = Water->WaterData[Water->NPage];
		pDest16 = static_cast<uint16*>(geBitmap_GetBits(Dest));

		// For the love of God, write this in assembly
		for (h=0; h< Info.Height; h++)
		{
			for (w=0; w< Info.Width; w++)
			{
				int32	x, y, Val;
				uint16	r, g, b;
				uint16	Color;

				Val = pWSrc16[w];
				 
				if (h < Info.Height-1)
					y = Val - pWSrc16[w+Info.Stride];
				else
					y = Val - pOriginalWSrc16[w];

				x = Val - pWSrc16[(w+1)&WMask];
			#if 1
				Val = 127 - (y<<4);

				if (Val < 0) 
					Val = 0;
				else if (Val > 255) 
					Val = 255;

				Val >>= 3;
				Val <<= 5;
			#endif
				
				x >>= 4;	 
				y >>= 4;

				Color = pSrc16[((h+y)&HMask)*Info.Stride + ((w+x)&WMask)];
				
			#if 1
				r = (uint16)pBlendLut[Val+((Color>>11)&31)];
				g = (uint16)pBlendLut[Val+((Color>>6)&31)];
				b = (uint16)pBlendLut[Val+(Color&31)];
				
				*pDest16++ = (r<<11) | (g<<6) | b;
			#else
				*pDest16++ = Color;
			#endif

			}

			pDest16 += Extra;
			pWSrc16 += Info.Stride;//Extra;
		}
		
		if (!geBitmap_UnLock(Dest))
		{
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//====================================================================================
//====================================================================================
static void CalcRippleData(int16 *Src, int16 *Dest, int16 Density, int32 W, int32 H)
{
	int32	i,j;
	int16	Val;

	//Bottom = W * (H-1);

	for(i=0; i< H; i++)
	{
		for(j=0; j< W; j++)
		{
			if (i > 0)							// Get top
				Val = *(Dest - W);
			else
				Val = *(Dest + W * (H-1));

			if (i < H-1)						// Get bottom
				Val += *(Dest + W);
			else
				Val += *(Dest - W * (H-1));

			if (j > 0)							// Get left
				Val += *(Dest - 1);
			else
				Val += *(Dest + (W-1));

			if (j < W-1)						// Get right
				Val += *(Dest + 1);
			else
				Val += *(Dest - (W-1));

			Val >>= 1;
			Val -= *Src;
			Val -= (Val >> Density);
			/*
			if (Val > 255)
				Val = 255;
			else if (Val < -255)
				Val = -255;
			*/
			*Src = Val;

			Src++;
			Dest++;

		}
	}
}


//====================================================================================
//	FloatMod
//====================================================================================
static float FloatMod(float In, float Wrap)
{
	Wrap = (float)floor(fabs(In)/Wrap)*Wrap - Wrap;

	if (In > 0)
		In -= Wrap;
	else if (In < 0)
		In += Wrap;

	return In;
}

//====================================================================================
//	Water_Update
//====================================================================================
void Water_Update(Procedural *Water, float Time)
{
	int16		*Page1, *Page2, *Page3;

	Page1 = Water->WaterData[Water->NPage];
	Page2 = Water->WaterData[!Water->NPage];

	Page3 = Page2;

	Water->TimeToSplashWater += Time;
#if 1
	if (Water->TimeToSplashWater > 0.8f)
	{
		int32	px, py, cx, cy, c;
		//int32		w, h;
		
		Water->TimeToSplashWater = 0.0f;
		
		
		for (c=0; c< 2; c++)
		{
			px=(1+(rand()%(Water->Width-1-10)));
			py=(1+(rand()%(Water->Height-1-10)));

			for(cy=py; cy< (py+8); cy++)
				for(cx=px; cx< (px+8); cx++)
					Water->WaterData[!Water->NPage][cy * Water->Width + cx]=255;
		}
		/*
		for (h=0; h<10; h++)
			for (w=0; w<10; w++)
				Water->WaterData[!Water->NPage][(((int32)Water->PosY+h)%Water->Height)*Water->Width + (((int32)Water->PosX+w)%Water->Width)] = 255;

		Water->PosX += 8.0f;
		Water->PosY += 2.0f;
		*/

	}
#else
	if (Water->TimeToSplashWater > 0.09f)
	{
		int32		w, h;

		Water->TimeToSplashWater = 0.0f;
		
		Water->PosX += (float)cos(Water->Angle)*16.0f;
		Water->PosY += (float)sin(Water->Angle)*16.0f;

		Water->Angle += ((float)rand()/RAND_MAX) * 0.5f - 0.1f;

		Water->Angle = FloatMod(Water->Angle, 3.14159f);

		Water->PosX = FloatMod(Water->PosX, (float)Water->Width);
		Water->PosY = FloatMod(Water->PosY, (float)Water->Height);

		//Water->WaterData[!Water->NPage][(int32)Water->PosY * Water->Width + (int32)Water->PosX]=255;
	
		for (h=0; h<10; h++)
			for (w=0; w<10; w++)
				Water->WaterData[!Water->NPage][(((int32)Water->PosY+h)%Water->Height)*Water->Width + (((int32)Water->PosX+w)%Water->Width)] = 255;
	}
#endif

	CalcRippleData(Page1, Page2, 4, Water->Width, Water->Height);

	Water->NPage = !Water->NPage;
}

//====================================================================================
//	Water_BuildRGBLuts
//====================================================================================
void Water_BuildRGBLuts(Procedural *Water, float RScale, float GScale, float BScale)
{
	int32		i, j;

	for (i=0; i<32; i++)		// Shade
	{
		for (j=0; j<32; j++)	// Color
		{
			int32	Val;

			Val = (int32)(((float)(31 - i)/14) * (float)j);

			//Val <<= 2;
			
			if (Val > 31)
				Val = 31;

			Water->BlendLut[i][j] = (uint8)Val;
		}
	}

}

//====================================================================================
//====================================================================================
static Procedural_Table Water_Table = 
{
	Procedurals_Version,Procedurals_Tag,
	"Water",
	Water_Create,
	Water_Destroy,
	Water_Animate
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
Procedural_Table *Water_GetProcedural_Table(void)
{
	return &Water_Table; 
}
