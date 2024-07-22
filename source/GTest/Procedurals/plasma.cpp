
// <> has jitter when CircleScale is very slow
//	tried some anti-aliasing, but didn't seem to help

//#define DO_TIMER

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>

#include "Plasma.h"
#include "GENESIS.H"
#include "RAM.H"
#include "gebmutil.h"
#include "Errorlog.h"
#include "proceng.h"
#include "procutil.h"

/*

params:

	( circle_step, roll_step, do_alpha, do_displacement, [palette_spec] )

example :

wbm:Wall3 Plasma(0.005, 0, f, f, trig, 128,128,128,255, 5.0,1.0,5.0,0.0, 0.0,0.5,0.0,0.0 );
wbm:Wall3 Plasma(0.01,  1, f, t );

*/

// work on these :
#define SIZE_SCALE	(150.0)

#define R_PAL_STEP	(0.07)
#define G_PAL_STEP	(0.05)
#define B_PAL_STEP	(0.1)

// Plasma               : 0.001983 : 6.3 %
//		about two millis
//		== 500 fps !

#ifdef DO_TIMER
#include "timer.h"

TIMER_VARS(Plasma);
#endif

#define BM_CREATE_WIDTH		(128)
#define BM_CREATE_HEIGHT	(128)
#define BM_MAX_WIDTH		(128)
#define BM_MAX_HEIGHT		(128)
#define BM_MIN_WIDTH		(128)
#define BM_MIN_HEIGHT		(128)
	// <> to support more widths we would need to make tables for each width;
	//	really, we should do this, and also dynamically allocate all tables
	//	(make a ref count in _Create and decrement it in _Destroy)

#define TAB_WIDTH	(BM_MAX_WIDTH * 2)
#define TAB_HEIGHT	(BM_MAX_HEIGHT* 2)

geBoolean PlasmaAnimator_CreatePalette(Procedural * Proc, double time);
geBoolean PlasmaAnimator_CreatePlasma(Procedural * Proc, double time);

/**************************************************************/

Procedural *Plasma_Create(geBitmap **ppBitmap, const char *ParmStart);
void		Plasma_Destroy(Procedural *Proc);
geBoolean	Plasma_Animate(Procedural *Plasma, float ElapsedTime);

static Procedural_Table Plasma_Table = 
{
	Procedurals_Version,Procedurals_Tag,
	"Plasma",
	Plasma_Create,
	Plasma_Destroy,
	Plasma_Animate
};

Procedural_Table * Plasma_GetProcedural_Table(void)
{
	return &Plasma_Table;
}

/**********************************************************/

typedef struct Procedural
{
	geBitmap * Bitmap;
	geBoolean PlasmaPalette,Displacer;
	geBitmap * DisplaceOriginal;
	
	double	circle1,circle2,circle3,circle4,
			circle5,circle6,circle7,circle8;
	double roll;
	double R,G,B;
	double CircleScale;
	double RollStep;
} Procedural;

/****/

Procedural *Plasma_Create(geBitmap **ppBitmap, const char *ParmStart)
{
Procedural * P = nullptr;
geBoolean DoAlpha = GE_FALSE;

	P = static_cast<Procedural*>(geRam_Allocate(sizeof(Procedural)));
	if ( ! P )
		return nullptr;
	assert(P);
	memset(P,0,sizeof(Procedural));

	if ( ! *ppBitmap )
	{
		*ppBitmap = geBitmap_Create(BM_CREATE_WIDTH,BM_CREATE_HEIGHT,1, GE_PIXELFORMAT_8BIT_PAL);
		if ( ! *ppBitmap )
		{
			geRam_Free(P);
			return nullptr;
		}
	}
	else
	{
		if ( ! geBitmap_SetFormat(*ppBitmap,GE_PIXELFORMAT_8BIT_PAL,GE_FALSE,0, nullptr) )
		{
			geRam_Free(P);
			return nullptr;
		}

		geBitmap_ClearMips(*ppBitmap);
		geBitmap_CreateRef(*ppBitmap);
	}

	if (geBitmap_Width( *ppBitmap) > BM_MAX_WIDTH  ||
		geBitmap_Height(*ppBitmap) > BM_MAX_HEIGHT ||
		geBitmap_Width( *ppBitmap) < BM_MIN_WIDTH  ||
		geBitmap_Height(*ppBitmap) < BM_MIN_HEIGHT )
	{
		geErrorLog_AddString(-1,"Plasm_Create : target Bitmap width not in supported ranged", nullptr);
		geRam_Free(P);
		return nullptr;
	}

	P->Bitmap = *ppBitmap;

	if ( ! ParmStart || strlen(ParmStart) < 10 )
	{
		P->PlasmaPalette = GE_TRUE;
		
		if ( ! PlasmaAnimator_CreatePalette(P,0.0) )
		{
			geRam_Free(P);
			return nullptr;
		}

		P->CircleScale = 1.0;
		P->RollStep = 1.0;

#if 0		
		P->R = 1.0/6.0*PI;
		P->G = 3.0/6.0*PI;
		P->B = 5.0/6.0*PI;
#else
		P->R = (rand() % 6)/6.0*PI;
		P->G = (rand() % 6)/6.0*PI;
		P->B = (rand() % 6)/6.0*PI;
#endif
	}
	else
	{
	char * pstr;
	char ParmWork[1024];

		P->PlasmaPalette = GE_FALSE;

		strcpy(ParmWork,ParmStart);
		pstr = strtok(ParmWork," ,\n\r\r");

		P->CircleScale = atof(pstr);
		pstr = strtok(nullptr," ,\n\r\r");

		P->RollStep = atof(pstr);
		pstr = strtok(nullptr," ,\n\r\r");

		DoAlpha = ((toupper(*pstr) == 'T') ? GE_TRUE : atol(pstr));
		pstr = strtok(nullptr," ,\n\r\r");

		P->Displacer = ((toupper(*pstr) == 'T') ? GE_TRUE : atol(pstr));
		pstr = strtok(nullptr," ,\n\r\r");

		if ( ! P->Displacer )
		{
			ParmStart += (int)(pstr - ParmWork);

			if ( ! ProcUtil_SetPaletteFromString(P->Bitmap,(char **)&ParmStart) )
			{
				geBitmap_Destroy(&(P->Bitmap));
				geRam_Free(P);
				return nullptr;
			}
		}
	}

	if ( P->Displacer )
	{
		P->DisplaceOriginal = geBitmap_Create(BM_CREATE_WIDTH,BM_CREATE_HEIGHT,1, GE_PIXELFORMAT_8BIT_PAL);
		if ( ! P->DisplaceOriginal )
		{
			geBitmap_Destroy(&(P->Bitmap));
			geRam_Free(P);
			return nullptr;
		}
		geBitmap_BlitBitmap(P->Bitmap,P->DisplaceOriginal);
	}

	if ( DoAlpha )
	{
		// this is the beauty of 'set preferred format'
		//	the only difference between Plasma with and without alpha
		//	is this one call !!!
		if (!geBitmap_SetPreferredFormat(P->Bitmap, GE_PIXELFORMAT_16BIT_4444_ARGB))
		{
			geBitmap_Destroy(&P->Bitmap);
			geRam_Free(P);
			return nullptr;
		}
	}

	P->circle1 = ( rand() % 10 ) * 0.1;
	P->circle2 = ( rand() % 10 ) * 0.1;
	P->circle3 = ( rand() % 10 ) * 0.1;
	P->circle4 = ( rand() % 10 ) * 0.1;
	P->circle5 = ( rand() % 10 ) * 0.1;
	P->circle6 = ( rand() % 10 ) * 0.1;
	P->circle7 = ( rand() % 10 ) * 0.1;
	P->circle8 = ( rand() % 10 ) * 0.1;
			
	#ifdef DO_TIMER
		timerFP = fopen("q:\\timer.log","at+");
		Timer_Start();
	#endif

return P;
}

void Plasma_Destroy(Procedural * P)
{
	if ( ! P )
		return;

	if ( P->Bitmap )
		geBitmap_Destroy(&(P->Bitmap));

	if ( P->DisplaceOriginal )
		geBitmap_Destroy(&(P->DisplaceOriginal));

	#ifdef DO_TIMER
	Timer_Stop();
	if ( timerFP )
	{
		TIMER_REPORT(Plasma);
	}
	#endif

	geRam_Free(P);
}

geBoolean Plasma_Animate(Procedural * P,float time)
{
	assert(P && P->Bitmap);

#ifdef DO_TIMER
	TIMER_P(Plasma);
#endif

	if ( P->PlasmaPalette )
	{
		if ( ! PlasmaAnimator_CreatePalette(P,time) )
			return GE_FALSE;
	}

	if ( ! PlasmaAnimator_CreatePlasma(P,time) )
		return GE_FALSE;
	
#ifdef DO_TIMER
	TIMER_Q(Plasma);
	TIMER_COUNT();
#endif

return GE_TRUE;
}

geBoolean PlasmaAnimator_CreatePalette(Procedural *P,double time)
{
geBitmap_Palette *Pal;
void * PalData = NULL;
gePixelFormat PalFormat;
int PalSize;

	if ( ! (Pal = geBitmap_GetPalette(P->Bitmap)) )
	{

		Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_ARGB,256);
		if ( ! Pal )
			goto fail;

		if ( ! geBitmap_SetPalette(P->Bitmap,Pal) )
			goto fail;

		geBitmap_Palette_Destroy(&Pal);
		
		if ( ! (Pal = geBitmap_GetPalette(P->Bitmap)) )
			return GE_FALSE;
	}

	if ( ! geBitmap_Palette_Lock(Pal,&PalData, &PalFormat, &PalSize) )
		goto fail;
	if ( PalSize < 256 )
		goto fail;

	{
	int p,r,g,b;
	uint8 * PalPtr;
	double R,G,B;
	float u;

		R = P->R;
		G = P->G;
		B = P->B;

		PalPtr = static_cast<uint8*>(PalData);
		for(p=0;p<256;p++)
		{
			u = (float)((PI/128.0) * p);

			// <>
			//#define mycol(u,a) (int)((max(0.0,cos((u)+(a))))*255.0)
			#define mycol(u,a) (int)((cos((u)+(a))+1)*127.0)

			r = mycol(u,R);
			g = mycol(u,G);
			b = mycol(u,B);

			gePixelFormat_PutColor(PalFormat,&PalPtr,r,g,b,0xFF);
		}

		P->R += R_PAL_STEP * time * 30.0;
		P->G -= G_PAL_STEP * time * 30.0;
		P->B += B_PAL_STEP * time * 30.0;
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

/*****

tab1 is the distance to the center of the (doubled) image (hence the corner of the original)
tab2 is basically sin(tab1)
(could save memory by making a sintable[255] instead of tab2)

sintab1[256] is too granular ; would have to use 
	int tab1[]
and sintab1[10000] or something

tables take 128k for a max size of 128x128

******/

static uint8 tab1[TAB_WIDTH * TAB_HEIGHT];
static uint8 tab2[TAB_WIDTH * TAB_HEIGHT];
static uint8 sintab1[256];
static signed char dtable[256];
static int calculated = 0;

void CalculateTables(Procedural * Proc)
{
int x,y;
uint8 *ptab1,*ptab2;
	if ( calculated )
		return;
	calculated = 1;
	ptab1 = tab1;
	ptab2 = tab2;
	for(y=0;y<TAB_HEIGHT;y++)
	{
		for(x=0;x<TAB_WIDTH;x++)
		{
		double d_to_center,r,dx,dy;

		#if 1	// TILE the texture correctly!
		#if 0
			dx = (x%128);
			dy = (y%128);
			dx = 64.0 - dx;
			dy = 64.0 - dy;
		#else
			dx = SIZE_SCALE * sin(x*PI/128.0);
			dy = SIZE_SCALE * cos(y*PI/128.0);
		#endif
		#else
			dx = (TAB_WIDTH *0.5) - x;
			dy = (TAB_HEIGHT*0.5) - y;
		#endif

		#if 0	// <> weak anti-alias
			dx += (( rand() % 50 ) * 0.01);
			dy += (( rand() % 50 ) * 0.01);
			r = (( rand() % 50 ) * 0.01);
		#else
			r = 0.4;
		#endif

			d_to_center = sqrt( 16.0 + dx*dx + dy*dy ) - 4.0;
			*ptab1++ = (uint8)(d_to_center * 5.0 + r );
			*ptab2++= (uint8)((sin(d_to_center/9.5)+1.0)*90.0 + r);
		}
	}

	for(x=0;x<256;x++)
	{
		sintab1[x] = (uint8)((sin(x/(5.0*9.5))+1.0)*90.0 + 0.5);
		dtable[x] = (signed char)(sin(x*2.0*PI/256) * 16.0);
	}
}

int DoCircle(double * pCircle, double step, int w, int doCos);

geBoolean PlasmaAnimator_CreatePlasma(Procedural * Proc,double time)
{
geBitmap *PlasmaLock=nullptr;
geBitmap_Info PlasmaInfo;
uint8 *Bits,*bptr;
int x,y,w,h,s;
geBitmap *Bitmap;

	Bitmap = Proc->Bitmap;
	assert(Bitmap);
	
	CalculateTables(Proc);

	geBitmap_SetGammaCorrection(Bitmap, 1.0f, GE_FALSE);

	if ( ! geBitmap_LockForWriteFormat(Bitmap,&PlasmaLock,0,0,GE_PIXELFORMAT_8BIT_PAL) )
		goto fail;

	if ( ! geBitmap_GetInfo(PlasmaLock,&PlasmaInfo,nullptr) )
		goto fail;

	Bits = static_cast<uint8*>(geBitmap_GetBits(PlasmaLock));

	if ( ! Bits )
		goto fail; 

	w = PlasmaInfo.Width;
	h = PlasmaInfo.Height;
	s = PlasmaInfo.Stride;

	// could do every other pixel & smooth
	// main advantage : smaller table

	{
	int	x1,y1,x2,y2,
		x3,y3,x4,y4,
		o1,o2,o3,o4;
	uint8 *ptab1,*ptab2;
	uint8 roll;
	double CircleScale;

		CircleScale = Proc->CircleScale * time * 2.0;

		x1 = DoCircle(&(Proc->circle1),0.3  * CircleScale,w,1);
		y1 = DoCircle(&(Proc->circle2),0.2  * CircleScale,h,0);
		y2 = DoCircle(&(Proc->circle4),0.1  * CircleScale,h,1);
		x2 = DoCircle(&(Proc->circle3),0.85 * CircleScale,w,0);
		x3 = DoCircle(&(Proc->circle5),0.4  * CircleScale,w,1);
		y3 = DoCircle(&(Proc->circle6),0.15 * CircleScale,h,0);
		x4 = DoCircle(&(Proc->circle7),0.35 * CircleScale,w,1);
		y4 = DoCircle(&(Proc->circle8),0.05 * CircleScale,h,0);

		o1 = x1 + TAB_WIDTH*y1;
		o2 = x2 + TAB_WIDTH*y2;
		o3 = x3 + TAB_WIDTH*y3;
		o4 = x4 + TAB_WIDTH*y4;

		roll = (uint8)( Proc->roll + 0.5);
		Proc->roll += Proc->RollStep * time * 30.0;
			// roll uses the fact that we work modulo 255

		// this is the inner loop;
		// it's very lean, but also not blazing fast, cuz
		//  this is very bad on the cache
		// we have we have FIVE different active segments of memory,
		//	(bptr,ptab1+o1,ptab2+o2,ptab2+o3,ptab2+o4) all of which are
		// too large to fit in the L1 cache, so each mem-hit is 4 clocks

		if ( Proc->Displacer )
		{
		geBitmap *Source,*SourceLock;
		geBitmap_Info SourceInfo;
		void * SourceBits;
		int ss;
		uint8 * sptr;
			// instensity is a displacement
		
			Source = Proc->DisplaceOriginal;
			assert(Source);

			if ( ! geBitmap_LockForReadNative(Source,&SourceLock,0,0) )
				goto fail;

			if ( ! geBitmap_GetInfo(SourceLock,&SourceInfo,nullptr) )
				goto fail;

			if ( ! (SourceBits = geBitmap_GetBits(SourceLock)) )
				goto fail; 

			assert( SourceInfo.Width == w);
			assert( SourceInfo.Height == h);
			ss = SourceInfo.Stride;

			bptr = Bits;
			sptr = static_cast<uint8*>(SourceBits);
			for(y=0;y<h;y++)
			{
				ptab1 =	tab1 + TAB_WIDTH*y;
				ptab2 =	tab2 + TAB_WIDTH*y;
				for(x=0;x<w;x++)
				{
				int o,sx;
					o = ptab1[o1] + ptab2[o2] + ptab2[o3] + ptab2[o4];
					o = (o + roll)&0xFF;
					o = dtable[o];
					ptab1++;
					ptab2++;

					sx = x + o;
					if ( sx < 0 ) sx += w;
					else if ( sx >= w ) sx -= w;
					assert( sx >= 0 && sx < w );
					*bptr++ = sptr[sx];
				}
				bptr += (s-w);
				sptr += ss;
			}

			geBitmap_UnLock(SourceLock);
		}
		else
		{
			// classic style

			bptr = Bits;
			if ( PlasmaInfo.HasColorKey )
			{
			uint8 CK,pel;
				CK = (uint8)PlasmaInfo.ColorKey;
				if ( CK > 200 )
				{
					for(y=0;y<h;y++)
					{
						ptab1 =	tab1 + TAB_WIDTH*y;
						ptab2 =	tab2 + TAB_WIDTH*y;
						for(x=w;x--;)
						{
							pel = roll + ptab1[o1] + 
									ptab2[o2] + ptab2[o3] + ptab2[o4];

							*bptr++ = pel%CK;
							ptab1++;
							ptab2++;
						}
						bptr += (s-w);
					}
				}
				else
				{
					for(y=0;y<h;y++)
					{
						ptab1 =	tab1 + TAB_WIDTH*y;
						ptab2 =	tab2 + TAB_WIDTH*y;
						for(x=w;x--;)
						{
							pel = roll + ptab1[o1] + 
									ptab2[o2] + ptab2[o3] + ptab2[o4];

							if ( pel == CK )
								pel ^= 1;

							*bptr++ = pel;
							ptab1++;
							ptab2++;
						}
						bptr += (s-w);
					}
				}
			}
			else
			{
				for(y=0;y<h;y++)
				{
					ptab1 =	tab1 + TAB_WIDTH*y;
					ptab2 =	tab2 + TAB_WIDTH*y;
					for(x=w;x--;)
					{
						*bptr++ = roll + ptab1[o1] + 
								ptab2[o2] + ptab2[o3] + ptab2[o4];
						ptab1++;
						ptab2++;
					}
					bptr += (s-w);
				}
			}
		}
	}

/*
	if ( ! geBitmapUtil_SmoothBits(&PlasmaInfo,Bits,Bits,1) )
		goto fail;
*/

	if ( ! geBitmap_UnLock(PlasmaLock) )
		goto fail;

	return GE_TRUE;
fail:

	if ( PlasmaLock )		geBitmap_UnLock(PlasmaLock);

	return GE_FALSE;
}


int DoCircle(double * pCircle, double step, int w, int doCos)
{
double cp,c,cn;
double xp,x,xn;
int ix,ixp,ixn;
	
	w--;

	// try to do temporal anti-aliasing
	// <> doesn't seem to help

	c = *pCircle;
	cp = c - step;
	cn = c + step;

	*pCircle += step;

	if ( doCos )
	{
		x  = w*0.5*(1.0 + cos(c));
		xp = w*0.5*(1.0 + cos(cp));
		xn = w*0.5*(1.0 + cos(cn));
	}
	else
	{
		x  = w*0.5*(1.0 + sin(c));
		xp = w*0.5*(1.0 + sin(cp));
		xn = w*0.5*(1.0 + sin(cn));
	}

	ixn = (int)xn;
	ixp = (int)xp;
	
	if ( ixp == ixn )
	{
		ix = ixp;
	}
	else
	{
		x += ( rand() % 50 ) * 0.01;
		ix = (int)((x + ixp + ixn)*(1.0/3.0));
	}

	assert( ix >= 0 && ix <= w );

return ix;
}
