
#define SMOOTH_WRAP (GE_FALSE)

//#define DO_TIMER

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>

#include "Fire.h"
#include "genesis.h"
#include "ram.h"
#include "gebmutil.h"
#include "proceng.h"
#include "procutil.h"

#ifdef DO_TIMER
#include "timer.h"
TIMER_VARS(Fire);
TIMER_VARS(Fire_Smooth);
TIMER_VARS(Fire_UnLock);
TIMER_VARS(Fire_Rands);
#endif

#define BM_WIDTH	(128)
#define BM_HEIGHT	(128)

#define SeedLines	(1)

geBoolean FireAnimator_CreateFire(Procedural * Proc);

/**************************************************************/

// optimized with ProcUtil_Rand() :
//Fire                 : 0.000820 = 1221 fps
//
// about 700 fps with ProcUtil_Rand()
// about 200 with the clib rand()

#define myrand(max)	ProcUtil_Rand(max)

/**************************************************************/

Procedural *Fire_Create(geBitmap **ppBitmap, const char *ParmStart);
void		Fire_Destroy(Procedural *Proc);
geBoolean	Fire_Animate(Procedural *Fire, float ElapsedTime);

static Procedural_Table Fire_Table = 
{
	Procedurals_Version,Procedurals_Tag,
	"Fire",
	Fire_Create,
	Fire_Destroy,
	Fire_Animate
};

Procedural_Table * Fire_GetProcedural_Table(void)
{
	return &Fire_Table;
}

/**********************************************************/

typedef struct Procedural
{
	geBitmap * Bitmap;
	uint32 Frame;
	geBoolean DoJets;
} Procedural;

/****/

Procedural *Fire_Create(geBitmap **ppBitmap, const char *ParmStart)
{
Procedural * P;

	P = static_cast<Procedural*>(geRam_Allocate(sizeof(Procedural)));
	if ( ! P )
		return NULL;
	assert(P);
	memset(P,0,sizeof(Procedural));

	P->Frame = 0;

	if ( strlen(ParmStart) < 10 )
	{
		ParmStart = "F, pow, 400,280,200,530, 0.3,0.6,1.0,0.8";
		//ParmStart = "F, pow, 230,230,255,300, 0.2,0.5,1.0,0.4";
	}

	if ( ! *ppBitmap )
	{
		*ppBitmap = geBitmap_Create(BM_WIDTH,BM_HEIGHT,1, GE_PIXELFORMAT_8BIT_PAL);
		if ( ! *ppBitmap )
		{
			geRam_Free(P);
			return NULL;
		}
	}
	else
	{
		geBitmap_CreateRef(*ppBitmap);
	}

	if ( ! geBitmap_SetFormat(*ppBitmap,GE_PIXELFORMAT_8BIT_PAL,GE_TRUE,255,nullptr) )
	{
		geRam_Free(P);
		return NULL;
	}

	geBitmap_ClearMips(*ppBitmap);

	geBitmap_SetPreferredFormat(*ppBitmap,GE_PIXELFORMAT_16BIT_4444_ARGB);

	P->Bitmap = *ppBitmap;

	{
	char ParmWork[100];
	char *ptr;
	int len;
		strncpy(ParmWork,ParmStart,100);
		ptr = strtok(ParmWork," \t\n\r,");
		if ( toupper(*ptr) == 'T' )
			P->DoJets = 1;
		else
			P->DoJets = atol(ptr);
		ptr = strtok(nullptr," \t\n\r,");
		len = (int)(ptr - ParmWork);
		ParmStart += len;
	}

	if ( ! ProcUtil_SetPaletteFromString(P->Bitmap,(char **)&ParmStart) )
	{
		geRam_Free(P);
		return nullptr;
	}

	assert( SeedLines == 1 ||SeedLines == 2 );

	#ifdef DO_TIMER
		timerFP = fopen("q:\\timer.log","at+");
		Timer_Start();
	#endif

return P;
}

void Fire_Destroy(Procedural * P)
{
	if ( ! P )
		return;
		
	#ifdef DO_TIMER
	Timer_Stop();
	if ( timerFP )
	{
		TIMER_REPORT(Fire);
		TIMER_REPORT(Fire_Smooth);
		TIMER_REPORT(Fire_UnLock);
		TIMER_REPORT(Fire_Rands);
	}
	#endif

	if ( P->Bitmap )
		geBitmap_Destroy(&(P->Bitmap));

	geRam_Free(P);
}

geBoolean Fire_Animate(Procedural * P,float time)
{

#ifdef DO_TIMER
	TIMER_P(Fire);
#endif

	if ( ! FireAnimator_CreateFire(P) )
		return GE_FALSE;

#ifdef DO_TIMER
	TIMER_Q(Fire);
	TIMER_COUNT();
#endif
	
	P->Frame ++;

return GE_TRUE;
}

geBoolean FireAnimator_CreateFire(Procedural * Proc)
{
geBitmap *FireLock = nullptr;
geBitmap_Info FireInfo;
uint8 *FireBits,*bPtr;
int x,y,w,h,s;
geBitmap *Bitmap;

	Bitmap = Proc->Bitmap;
	assert(Bitmap);

	geBitmap_SetGammaCorrection(Bitmap, 1.0f, GE_FALSE);

	if ( ! geBitmap_LockForWriteFormat(Bitmap,&FireLock,0,0,GE_PIXELFORMAT_8BIT_PAL) )
		goto fail;

	if ( ! geBitmap_GetInfo(FireLock,&FireInfo, nullptr) )
		goto fail;

	FireBits = static_cast<uint8*>(geBitmap_GetBits(FireLock));

	if ( ! FireBits )
		goto fail; 

	w = FireInfo.Width;
	h = FireInfo.Height;
	s = FireInfo.Stride;

	if ( Proc->Frame == 0 )
		memset(FireBits,0,s*h);

#ifdef DO_TIMER
	TIMER_P(Fire_Rands);
#endif

	{
	int r;
	uint8 * bPtr;
		bPtr = FireBits + (h - SeedLines)*s;
		// seed hot points
		memset(bPtr,0,s* SeedLines);
		for(r=20*SeedLines;r--;)
		{
			x = myrand(s*SeedLines);
			bPtr[x] = 254;
		}
	}

#if 1	// clear some points to avoid static junk
	{
	int r;
	uint8 * bPtr;
		for(y=0;y<h;y++)
		{
			bPtr = FireBits + y*s;
			for(r=10;r--;)
			{
				x = myrand(w);
				if ( bPtr[x] )
				{
					bPtr[x] -= ((bPtr[x])>>4) + 1;
				}
			}
		}
	}
#endif

#ifdef DO_TIMER
	TIMER_Q(Fire_Rands);
#endif

	if ( Proc->DoJets )
	{
	uint8 *pbPtr,*ppbPtr;
	int a;
	int randval,randbits;
		
		// now do a diffusion bleed

		randbits = 0;

		if ( SeedLines < 2 )
		{
			memcpy(FireBits + (h-2)*s,FireBits + (h-1)*s,s);
		}

		bPtr = FireBits + s;
		for(y=h-3;y--;)
		{
			 pbPtr =  bPtr + s;
			ppbPtr = pbPtr + s;
			for(x=w;x--;)
			{
				a = (bPtr[-1] + pbPtr[0] + pbPtr[1] + pbPtr[-1] + pbPtr[-2] +
					ppbPtr[0] + ppbPtr[1] + ppbPtr[-1])>>3;
				if ( a )
				{
					if ( ! randbits )
					{
						randval = myrand(1024);
						randbits = 10;
					}
					switch( randval & 1 )
					{
						case 0:
							*bPtr = (a + pbPtr[1])>>1;
							break;
						case 1:
							*bPtr = (a + pbPtr[-1])>>1;
							break;
					}
					randval >>= 1;
					randbits --;
				}
				else
				{
					*bPtr = 0;
				}
				bPtr++;
				pbPtr++;
				ppbPtr++;
			}
			bPtr += s - w;
		}
	}
	else
	{
	
#ifdef DO_TIMER
		TIMER_P(Fire_Smooth);
#endif

		FireInfo.Height --;
		if ( ! geBitmapUtil_SmoothBits(&FireInfo,FireBits + s,FireBits,2,SMOOTH_WRAP) )
			goto fail;
			
#ifdef DO_TIMER
		TIMER_Q(Fire_Smooth);
#endif
	}
	
	// fix up top lines
	{
	uint8 *sPtr;
	int passes,MaxY;

		passes = 2;

		if ( Proc->DoJets )
			MaxY = 4;
		else
			MaxY = 2;
	
		while(passes--)
		{
			for(y=1;y<= MaxY;y++)
			{
				bPtr = FireBits + (h-y)*s;
				sPtr = bPtr - s;
				*bPtr++ = (bPtr[0] + bPtr[1] + sPtr[0] + sPtr[1])>>2;
				sPtr++;
				for(x=w-2;x--;)
				{
					*bPtr++ = (bPtr[0] + bPtr[1] + bPtr[-1] + sPtr[0] + sPtr[1] + sPtr[-1] + sPtr[-2] + sPtr[2])>>3;
					sPtr++;
				}
				*bPtr++ = (bPtr[0] + bPtr[-1] + sPtr[0] + sPtr[-1])>>2;
				sPtr++;
			}
		}
	}

#ifdef DO_TIMER
		TIMER_P(Fire_UnLock);
#endif

	if ( ! geBitmap_UnLock(FireLock) )
		goto fail;

#ifdef DO_TIMER
		TIMER_Q(Fire_UnLock);
#endif

	return GE_TRUE;
fail:

	if ( FireLock )		geBitmap_UnLock(FireLock);

	return GE_FALSE;
}

