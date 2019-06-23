#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <algorithm>

#include "procutil.h"
#include "errorlog.h"

static uint32 lastRandomNumber = 0xA2A9; // that's a prime

void ProcUtil_Randomize( void )
{
uint32 cnt;
	cnt = clock();
	cnt &= 0xFFFF;
	while(cnt--)
	{
		lastRandomNumber = lastRandomNumber*65539+3;
		lastRandomNumber = lastRandomNumber*1009 +7;
	}
}

uint32 ProcUtil_Rand( uint32 max )
{
uint32 a;

	lastRandomNumber = lastRandomNumber*65539+3;

	a = lastRandomNumber>>16;

return a%max;
}

float ProcUtil_SinTable[TABLE_SIZE];
float ProcUtil_CosTable[TABLE_SIZE];

int   ProcUtil_ByteSinTable[TABLE_SIZE];
int   ProcUtil_ByteCosTable[TABLE_SIZE];

void ProcUtil_Init(void)
{
	int i;
	double radVal;
	
	for (i = 0; i < TABLE_SIZE; i ++)
	{
		radVal = i * (TWO_PI / TABLE_SIZE );
		
		ProcUtil_SinTable[i] = (float)sin(radVal);
		ProcUtil_CosTable[i] = (float)cos(radVal);
	}

	for (i = 0; i < 256; i ++)
	{
		radVal = i * (TWO_PI / 256.0);
		
		ProcUtil_ByteSinTable[i] = (int) (127.0 * (sin(radVal) + 1.0) + 0.49);
		ProcUtil_ByteCosTable[i] = (int) (127.0 * (cos(radVal) + 1.0) + 0.49);
	}
}


//====================================================================================
//	Utilities for many Procedurals
//====================================================================================

geBoolean ProcUtil_SetPaletteFromString(geBitmap * Bitmap,char ** pParams)
{
geBitmap_Info Info;
geBitmap_Palette *Pal;
void * PalData = NULL;
gePixelFormat PalFormat;
int PalSize;

char ParamWork[1024];
char *pstr;
const char * strbreakers = " ,\t\n\r\34\39\09";

#define nextparam()	do { if ( ! pstr ) { geErrorLog_AddString(-1,"ProcEng_CreatePalette : params insufficient",nullptr); goto fail; }; pstr = strtok(nullptr,strbreakers); } while(0)
#define getint()	( pstr == nullptr ) ? 0 : atol(pstr); nextparam();
#define getfloat()	( pstr == nullptr ) ? 0.0f : (float)atof(pstr); nextparam();

	if ( pParams )
	{
		assert(*pParams);
		strcpy(ParamWork,*pParams);
	}
	else
	{
		strcpy(ParamWork,"list, 3, 0,0,0,0, 200,50,0,100, 255,100,50,255");
	}

	pstr = strtok(ParamWork,strbreakers);

	assert(Bitmap);

	Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_ARGB,256);
	if ( ! Pal )
		goto fail;
	if ( ! geBitmap_SetPalette(Bitmap,Pal) )
		goto fail;
	geBitmap_Palette_Destroy(&Pal);

	geBitmap_GetInfo(Bitmap,&Info,nullptr);

	if ( ! (Pal = geBitmap_GetPalette(Bitmap)) )
		goto fail;

	if ( ! geBitmap_Palette_Lock(Pal,&PalData, &PalFormat, &PalSize) )
		goto fail;
	if ( PalSize < 256 )
		goto fail;

	if ( _strnicmp(pstr,"list",4) == 0 )
	{
	int p,lp,r,g,b,a;
	uint8 * PalPtr;
	int NumColors;
	int pr,pg,pb,pa;
	int nr,ng,nb,na;
	double cstep,nextc,icstep;

		nextparam();
		NumColors = getint();
		if ( NumColors == 0 )
		{
			*pParams += (int)(pstr - ParamWork);
			pParams = nullptr;
			strcpy(ParamWork,"list, 3, 0,0,0,0, 200,50,0,100, 255,100,50,255");
		}
		else if ( NumColors < 2 )
			goto fail;

		cstep = 256.0 / (NumColors - 1);
		nextc = 0.0;
		icstep = 1.0 / cstep;

		NumColors--;
		nr = getint();
		ng = getint();
		nb = getint();
		na = getint();
	
		PalPtr = static_cast<uint8*>(PalData);
		for(p=0;p<256;p++)
		{
			if ( p >= (int)nextc )
			{
				pr = nr;
				pg = ng;
				pb = nb;
				pa = na;
				nr = getint();
				ng = getint();
				nb = getint();
				na = getint();
				nextc += cstep;
				lp = p;
			}

			// <> use Spline or Bezier instead !

			r = (int)(pr + (nr - pr)*(p - lp)*icstep);
			g = (int)(pg + (ng - pg)*(p - lp)*icstep);
			b = (int)(pb + (nb - pb)*(p - lp)*icstep);
			a = (int)(pa + (na - pa)*(p - lp)*icstep);

			gePixelFormat_PutColor(PalFormat, &PalPtr, std::min<int>(r, 255), std::min<int>(g, 255), std::min<int>(b, 255), std::min<int>(a, 255));
		}	
	}
	else if ( _strnicmp(pstr,"pow",4) == 0 )
	{
	int p,r,g,b,a;
	uint8 * PalPtr;
	double pr,pg,pb,pa,frac;
	int nr,ng,nb,na;

		nextparam();

		nr = getint();
		ng = getint();
		nb = getint();
		na = getint();
		if ( pstr )
		{
			pr = getfloat();
			pg = getfloat();
			pb = getfloat();
			pa = getfloat();
		}
		else
		{
			pr = pg = pb = pa = 0.5;
		}

		PalPtr = static_cast<uint8*>(PalData);
		for(p=0;p<256;p++)
		{
			frac = (double)p * (1.0/256.0);
			r = (int)( nr * pow( frac, pr) );
			g = (int)( ng * pow( frac, pg) );
			b = (int)( nb * pow( frac, pb) );
			a = (int)( na * pow( frac, pa) );
			gePixelFormat_PutColor(PalFormat,&PalPtr,std::min<int>(r,255),std::min<int>(g,255),std::min<int>(b,255),std::min<int>(a,255));
		}
	}
	else if ( _strnicmp(pstr,"trig",4) == 0 )
	{
	int p,r,g,b,a;
	uint8 * PalPtr;
	float r_mult,g_mult,b_mult,a_mult;
	float r_freq,g_freq,b_freq,a_freq;
	float r_base,g_base,b_base,a_base;
	float frac;

		nextparam();

		r_mult = getfloat();
		g_mult = getfloat();
		b_mult = getfloat();
		a_mult = getfloat();
		r_freq = getfloat();
		g_freq = getfloat();
		b_freq = getfloat();
		a_freq = getfloat();
		r_base = getfloat(); r_base *= TWO_PI;
		g_base = getfloat(); g_base *= TWO_PI;
		b_base = getfloat(); b_base *= TWO_PI;
		a_base = getfloat(); a_base *= TWO_PI;

		PalPtr = static_cast<uint8*>(PalData);
		for(p=0;p<256;p++)
		{
			frac = (float)p * (TWO_PI/256.0f);
			r = (int)( r_mult * (cos( r_freq * frac + r_base ) + 1.0) );
			g = (int)( g_mult * (cos( g_freq * frac + g_base ) + 1.0) );
			b = (int)( b_mult * (cos( b_freq * frac + b_base ) + 1.0) );
			a = (int)( a_mult * (cos( a_freq * frac + a_base ) + 1.0) );
			gePixelFormat_PutColor(PalFormat,&PalPtr,std::min<int>(r,255),std::min<int>(g,255),std::min<int>(b,255),std::min<int>(a,255));
		}
	}
	else
	{
		geErrorLog_AddString(-1,"ProcEng_MakePalette : no palette type!",nullptr);
	}

	PalData = NULL;

	if ( ! geBitmap_Palette_UnLock(Pal) )
		return GE_FALSE;

	if ( pParams )
	{
		*pParams += (int)pstr - (int)ParamWork;
	}	

	return GE_TRUE;

fail:

	if ( PalData )
		geBitmap_Palette_UnLock(Pal);

	return GE_FALSE;
}
