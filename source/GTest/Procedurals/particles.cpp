
//#define DO_TIMER

/*}{**************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <algorithm>
#include <ctype.h>

#include "Particles.h"
#include "GENESIS.H"
#include "RAM.H"
#include "Errorlog.h"
#include "procutil.h"
#include "gebmutil.h"
#include "VEC3D.H"

#ifdef DO_TIMER
#include "timer.h"

TIMER_VARS(Particles);
#endif

// optimized :
// Particles            : 0.000759 = 1313 fps !
//

/*****

Particles (
	parts, sources, 
	coloring_scheme, 
	smoothes, smoothradius,
	capper,
	do_magentism {field},
	do_attractor {strength, is_vec, {vec,} pos}
[for all sources:] {
[base:]		pos, velocity, deathtime, color,
[variance:] pos, velocity, deathtime, color, shade,
			emission_delay, drag,
					}
	"end"
	)
// [] are comments
// {} are conditional fields

*******/

static const char * Params_Oil = "100, 1, oil,	1, 1, wrap, t, 0, 1, 0,	f,"
			"0.5, 0.5, 0.5, 0,0,0, 1.0, 0,											"					
			"0,0,0, 0.3,0.2,0, 0, 99,0,	0.01,0.0,							"
			"end";
static const char * Params_Jet = "200, 1, fire,	1, 1, hard, f, t, 100, t, 0, 1, 0, 0.5, 0, 0.5,"
			"0.5, 0, 0.5, 0,0.6,0, 0.03, 0,											"
			"0,0,0, 0.5,0.4,0, 0, 99, 0,	0.01,0.0,							"
			"end)";
static const char * Params_Steam = "50, 1, steam,	2, 1, hard, f, f,		"															
			"0.5, 0, 0.5, 0,0.25,0, 0.7, 0,											"
			"0,0,0, 0.15,0.10,0,0.3,99,0,0.02,0.0005,						"
			"end";
static const char * Params_Explosion = "300, 1,fire,	1, 1, bounce, f, f,"
			"0.5, 0.5, 0.5,	0,0,0, 			0.1, 0,"
			"0,0,0, 			0.3,0.3,0.3,	0.05, 0, 0,	0.01, 0.0003,"
			"end)";

#define DefaultParams Params_Steam

#include "gebmutil.h"

#define	NUM_CLEAR_POINTS	(0)
	/* the idea of clear points is that the "smoothing" bleads in clear points
	*	from the outside, but sometimes you get solid pockets in the middle
	*	with no particles that don't get cleared.
	*/

#define BM_CREATE_WIDTH		(128)
#define BM_CREATE_HEIGHT	(128)

/*}{**************/

#define PIXEL_INDEX(c,s,nc)	(((s)*(nc)) + (c))
#define PIXEL_SHADE(pel,nc)	((pel)/(nc))
#define PIXEL_COLOR(pel,nc)	((pel)%(nc))

typedef void (*CoordCapperFunc) (float *x,float *v, int size);
typedef void (*PixelRGBAFunc) (Procedural *Proc,int c,int s,int *R,int *G,int *B,int *);

typedef struct
{
	float	p[3],	//position in pixels
			v[3];	//velocity in pixels per second
	int color,shade; // shade < 0 for inactive
	float DeathTime,CurDeathTime;
	float Drag;
} Particle;

typedef struct
{
	Particle Base;	
	Particle Random;
	float	RandomVMagnitude;
	float 	Delay,CurTime;
} ParticleSource;

typedef struct Procedural
{
	geBitmap * Bitmap;
	Particle * Particles;
	ParticleSource * Sources;

	int NumActiveParticles;
	int SizeX,SizeY,SizeZ;
	geBitmap_Info BmInfo;

	int NumParticles,NumSources;
	int NumColors,NumShades;
	int NumSmoothes,SmoothRadius;
	geBoolean SmoothWrap;
	
	CoordCapperFunc Capper;
	PixelRGBAFunc PixelRGBA;
	
	geBoolean DoMagnetic;
	geVec3d MagneticField;

	geBoolean DoAttractor;
	geBoolean AttractorIsAxis;
	geVec3d	AttractorPos,AttractorAxis;
	float AttractorStrength;
} Procedural;

/*}{**************/

geBoolean Particles_InitBitmap(geBitmap **ppBitmap);
geBoolean Particles_InitPalette(Procedural * Proc);
void Particles_Destroy(Procedural * Proc);

Particle * Particles_NewParticle(Procedural *Proc);
void Particles_EmitSources(Procedural *Proc,float time);
void Particles_MoveParticles(Procedural * Proc,float time);
geBoolean Particles_Draw(Procedural *Proc);

void Capper_Wrap(float *x,float *v, int size);
void Capper_Hard(float *x,float *v, int size);
void Capper_Bounce(float *x,float *v, int size);

void PixelRGBA_OilColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A);
void PixelRGBA_FireColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A);
void PixelRGBA_OpaqueFireColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A);
void PixelRGBA_SteamColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A);

/*}{**************/

#define ABS(x)	( (x) < 0 ? (-(x)) : (x) )
#define minmax(x,lo,hi) ( (x)<(lo)?(lo):( (x)>(hi)?(hi):(x)) )
#define putminmax(x,lo,hi) x = minmax(x,lo,hi)

static const char * strbreakers = " ,`\t\n\r\034\009";

#define nextparam(pstr)		do { pstr = strtok(nullptr,strbreakers); if ( ! pstr ) { Particles_Destroy(Proc); geErrorLog_AddString(-1,"Particle Procedural : params insufficient", nullptr); return nullptr; } } while(0)
#define getint(pstr)	atol(pstr); nextparam(pstr);
#define getbool(pstr)	( toupper(*pstr) == 'T' ? GE_TRUE : (toupper(*pstr) == 'F' ? GE_FALSE : (atol(pstr)) ) ); nextparam(pstr);
#define getfloat(pstr)	(float)atof(pstr); nextparam(pstr);
#define getvec(pstr,pvec)	do { ((geVec3d *)pvec)->X = getfloat(pstr); ((geVec3d *)pvec)->Y = getfloat(pstr); ((geVec3d *)pvec)->Z = getfloat(pstr); } while(0)
#define scalevec(pvec)	do { ((geVec3d *)pvec)->X *= Proc->SizeX; ((geVec3d *)pvec)->Y *= Proc->SizeY; ((geVec3d *)pvec)->Z *= Proc->SizeZ; } while(0)
#define absvec(pvec)	do { ((geVec3d *)pvec)->X = ABS(((geVec3d *)pvec)->X); ((geVec3d *)pvec)->Y = ABS(((geVec3d *)pvec)->Y); ((geVec3d *)pvec)->Z = ABS(((geVec3d *)pvec)->Z); } while(0)
#define matchstr(str,vs)	( _strnicmp(str,vs,strlen(vs)) == 0 )

/*}{**************/

Procedural * Particles_Create(geBitmap **ppBitmap, const char *InputParams)
{
Procedural * Proc;
int i;
char ParamWork[8192],*pstr;

	if ( ! (Proc = static_cast<Procedural*>(geRam_AllocateClear(sizeof(Procedural)))) )
		return nullptr;

	/**** read Params *****/

	if ( strlen(InputParams) < 20 )
	{
		strcpy(ParamWork,DefaultParams);
		geErrorLog_AddString(-1,"Particle Procedural : no params : using default ", nullptr);
	}
	else
	{
		strcpy(ParamWork,InputParams);
	}

	pstr = strtok(ParamWork,strbreakers);

	Proc->NumParticles = getint(pstr);
	Proc->NumSources = getint(pstr);

	/**** make Bitmap *****/

	if ( ! Particles_InitBitmap(ppBitmap) )
		goto fail;
	Proc->Bitmap = *ppBitmap;

	Proc->SizeX  = geBitmap_Width(Proc->Bitmap);
	Proc->SizeY = geBitmap_Height(Proc->Bitmap);
	geBitmap_GetInfo(Proc->Bitmap,&(Proc->BmInfo),nullptr);
	Proc->SizeZ = std::min<int>(Proc->SizeX, Proc->SizeY);

	/**** make Particles *****/

	Proc->Particles = static_cast<Particle*>(geRam_AllocateClear(Proc->NumParticles * sizeof(Particle)));
	if (! (Proc->Particles) )
		goto fail;
	for(i=0;i<Proc->NumParticles;i++)
	{
		Proc->Particles[i].shade = -1;
	}
	Proc->NumActiveParticles = 0;

	/**** make Sources *****/

	Proc->Sources = static_cast<ParticleSource*>(geRam_AllocateClear(Proc->NumSources * sizeof(ParticleSource)));
	if (! (Proc->Sources) )
		goto fail;

	/**** More Params *****/

	// <> for stock schemes, take a few colors as parameters

	if ( matchstr(pstr,"oil") )
	{
		nextparam(pstr);
		Proc->NumColors = Proc->NumShades = 16;
		Proc->PixelRGBA = PixelRGBA_OilColor;
	}
	else if ( matchstr(pstr,"fire") )
	{
		nextparam(pstr);
		Proc->NumColors = 1;
		Proc->NumShades = 256;
		Proc->PixelRGBA = PixelRGBA_FireColor;
	}
	else if ( matchstr(pstr,"opaquefire") )
	{
		nextparam(pstr);
		Proc->NumColors = 1;
		Proc->NumShades = 256;
		Proc->PixelRGBA = PixelRGBA_OpaqueFireColor;
	}
	else if ( matchstr(pstr,"steam") )
	{
		nextparam(pstr);
		Proc->NumColors = 1;
		Proc->NumShades = 256;
		Proc->PixelRGBA = PixelRGBA_SteamColor;
	}
	else
	{
		Proc->NumColors = getint(pstr);
		Proc->NumShades = getint(pstr);
		Proc->PixelRGBA = PixelRGBA_OilColor; 
		// <>
		// way to read in general spline of colors ?
	}

	if ( (Proc->NumColors * Proc->NumShades) < 256 )
	{
		Proc->NumColors = std::min<int>(Proc->NumColors,256);
		Proc->NumShades = 256 / Proc->NumColors;
	}

	Proc->NumSmoothes	= getint(pstr);
	Proc->SmoothRadius	= getint(pstr);

	if ( matchstr(pstr,"bounce") )
	{
		Proc->Capper = Capper_Bounce;
		Proc->SmoothWrap = GE_FALSE;
	}
	else if ( matchstr(pstr,"wrap") )
	{
		Proc->Capper = Capper_Wrap;
		Proc->SmoothWrap = GE_TRUE;
	}
	else if ( matchstr(pstr,"hard") )
	{
		Proc->Capper = Capper_Hard;
		Proc->SmoothWrap = GE_FALSE;
	}
	else
	{
		goto fail;
	}
	nextparam(pstr);

	Proc->DoMagnetic = getbool(pstr);
	if ( Proc->DoMagnetic )
	{
		getvec(pstr,&(Proc->MagneticField));
	}

	Proc->DoAttractor = getbool(pstr);
	if ( Proc->DoAttractor )
	{
		Proc->AttractorStrength = getfloat(pstr); // negative for repulsive
		Proc->AttractorIsAxis = getbool(pstr);
		if ( Proc->AttractorIsAxis )
		{
			getvec(pstr,&(Proc->AttractorAxis));
			geVec3d_Normalize(&(Proc->AttractorAxis));
		}
		getvec(pstr,&(Proc->AttractorPos));
		scalevec(&(Proc->AttractorPos));
	}

	/**** LUT's *****/

	if ( ! Particles_InitPalette(Proc) )
		goto fail;

	for(i=0;i< Proc->NumSources; i++)
	{
	ParticleSource * pSource;
	Particle * pParticle;

		pSource = Proc->Sources + i;

		pParticle = &(pSource->Base);

		getvec(pstr,pParticle->p);
		getvec(pstr,pParticle->v);
		scalevec(pParticle->p);
		scalevec(pParticle->v);
		pParticle->DeathTime = getfloat(pstr);
		pParticle->color = getint(pstr);
		pParticle->shade = Proc->NumShades - 1;
		pParticle->CurDeathTime = 0.0f;

		pParticle = &(pSource->Random);

		getvec(pstr,pParticle->p);
		getvec(pstr,pParticle->v);
		scalevec(pParticle->p);
		scalevec(pParticle->v);
		absvec(pParticle->p);
		absvec(pParticle->v);

		pParticle->DeathTime = getfloat(pstr);
		
		pParticle->color = getint(pstr);
		pParticle->shade = getint(pstr);
		pParticle->CurDeathTime = 0.0f;

		pSource->RandomVMagnitude = geVec3d_Normalize( (geVec3d *) (pParticle->v) );

		pSource->Delay = getfloat(pstr); // seconds
		pSource->Base.Drag = getfloat(pstr);
		pSource->CurTime = pSource->Delay;
	}

	if ( _strnicmp(pstr,"end",3) != 0 )
	{
		//geErrorLog_AddString(-1,"Particles_Create : non-fatal : Didn't get 'end' of Particle Prodedural config");
	}

	#ifdef DO_TIMER
		timerFP = fopen("q:\\timer.log","at+");
		Timer_Start();
	#endif

	return Proc;

	fail:

	Particles_Destroy(Proc);
	return NULL;
}

/*}{**************/

void Particles_Destroy(Procedural * Proc)
{
	if ( ! Proc )
		return;

	if ( Proc->Bitmap )
		geBitmap_Destroy(&(Proc->Bitmap)); // we did creatref
	if ( Proc->Particles )
		geRam_Free(Proc->Particles);
	if ( Proc->Sources )
		geRam_Free(Proc->Sources);
		
	#ifdef DO_TIMER
	Timer_Stop();
	if ( timerFP )
	{
		TIMER_REPORT(Particles);
	}
	#endif

	geRam_Free(Proc);
}

/*}{**************/

geBoolean Particles_Animate(Procedural * Proc,float time)
{

	#ifdef DO_TIMER
	TIMER_P(Particles);
	#endif

	Particles_EmitSources(Proc,time);

	Particles_MoveParticles(Proc,time);

	Particles_Draw(Proc);

	#ifdef DO_TIMER
	TIMER_Q(Particles);
	TIMER_COUNT();
	#endif

return GE_TRUE;
}

/*}{**************/

Particle * Particles_NewParticle(Procedural *Proc)
{
Particle * pP;
int p;
	pP = Proc->Particles;
	for(p= Proc->NumParticles;p--;pP++)
	{
		if ( pP->shade < 0 )
		{
			Proc->NumActiveParticles++;
			return pP;
		}
	}
	pP = NULL;
	assert(pP);
return pP;
}
void Particles_DeleteParticle(Procedural *Proc,Particle *pP)
{
Particle * pPNew;
	pPNew = Proc->Particles + (Proc->NumActiveParticles - 1);
	Proc->NumActiveParticles --;
	*pP = *pPNew;
	pPNew->shade = -1;
}

void Particles_EmitSources(Procedural *Proc,float time)
{
int cnt;
ParticleSource * pSource;
Particle *pP,*pR;

	pSource = Proc->Sources;
	for(cnt=Proc->NumSources;cnt--;pSource++)
	{
		pSource->CurTime += time;
		while ( pSource->CurTime > pSource->Delay &&
			Proc->NumActiveParticles < Proc->NumParticles )
		{
		float rv,t;

			pSource->CurTime -= pSource->Delay;

			pP = Particles_NewParticle(Proc);
			*pP = pSource->Base;
			pR = &(pSource->Random);
			rv = ProcUtil_RandSignedFloat( pSource->RandomVMagnitude );
			t = ProcUtil_RandSignedUnitFloat();
			pP->v[0] += rv * pR->v[0] * t;
			t = ProcUtil_RandSignedUnitFloat();
			pP->v[1] += rv * pR->v[1] * t;
			t = ProcUtil_RandSignedUnitFloat();
			pP->v[2] += rv * pR->v[2] * t;
			pP->p[0] += ProcUtil_RandSignedFloat(pR->p[0]);
			pP->p[1] += ProcUtil_RandSignedFloat(pR->p[1]);
			pP->p[2] += ProcUtil_RandSignedFloat(pR->p[2]);
			pP->color += ProcUtil_RandSigned(pR->color);
			pP->shade += ProcUtil_RandSigned(pR->shade);
			putminmax(pP->color,0,Proc->NumColors - 1);
			putminmax(pP->shade,0,Proc->NumShades - 1);
			
			assert( geVec3d_IsValid((geVec3d *)pP->p) );
			assert( geVec3d_IsValid((geVec3d *)pP->v) );
		}
	}

}

void Particles_MoveParticles(Procedural * Proc,float time)
{
int cnt;
Particle * pP;

	pP = Proc->Particles;
	for(cnt=0;cnt<Proc->NumActiveParticles;cnt++,pP++)
	{
	float mul,vsqr;

		assert( pP->shade >= 0 );
		assert( geVec3d_IsValid((geVec3d *)pP->p) );
		assert( geVec3d_IsValid((geVec3d *)pP->v) );

		pP->CurDeathTime += time;
		if ( pP->CurDeathTime > pP->DeathTime )
		{
			pP->shade -= (Proc->NumShades + 15)>>4;
			pP->CurDeathTime -= pP->DeathTime;
		}

		if ( pP->shade < 0 )
		{
			Particles_DeleteParticle(Proc,pP);
			if ( cnt >= Proc->NumActiveParticles )
				break;
		}

		assert( geVec3d_IsValid((geVec3d *)pP->p) );
		assert( geVec3d_IsValid((geVec3d *)pP->v) );

		pP->p[0] += pP->v[0] * time;
		Proc->Capper( pP->p + 0, pP->v + 0, Proc->SizeX);

		pP->p[1] += pP->v[1] * time;
		Proc->Capper( pP->p + 1, pP->v + 1, Proc->SizeY);

		pP->p[2] += pP->v[2] * time;
		Proc->Capper( pP->p + 2, pP->v + 2, Proc->SizeZ);

		assert( geVec3d_IsValid((geVec3d *)pP->p) );
		assert( geVec3d_IsValid((geVec3d *)pP->v) );

		vsqr = geVec3d_LengthSquared((geVec3d *)pP->v);
		assert( vsqr >= 0.0f );
		if ( vsqr > 100.0f ) vsqr = 100.0f;
		mul = vsqr * pP->Drag;
		if ( mul > 0.05f ) mul = 0.05f;
		pP->v[0] -= pP->v[0] * mul;
		pP->v[1] -= pP->v[1] * mul;
		pP->v[2] -= pP->v[2] * mul;

		// change velocity based on some forces

		assert( geVec3d_IsValid((geVec3d *)pP->p) );
		assert( geVec3d_IsValid((geVec3d *)pP->v) );

		if ( Proc->DoMagnetic )
		{
		geVec3d A;

			geVec3d_CrossProduct( (geVec3d *) (pP->v) , &(Proc->MagneticField), &A);
			pP->v[0] += time * A.X;
			pP->v[1] += time * A.Y;
			pP->v[2] += time * A.Z;

			assert( geVec3d_IsValid((geVec3d *)pP->p) );
			assert( geVec3d_IsValid((geVec3d *)pP->v) );
		}

		if ( Proc->DoAttractor )
		{
		float ax,ay,az,q;

			if ( Proc->AttractorIsAxis )
			{
			geVec3d Diff;
				geVec3d_Subtract( (geVec3d *) pP->p , &(Proc->AttractorPos), &Diff);
				q = geVec3d_DotProduct( &(Proc->AttractorAxis), &Diff );
				ax = q * Proc->AttractorAxis.X - Diff.X;
				ay = q * Proc->AttractorAxis.Y - Diff.Y;
				az = q * Proc->AttractorAxis.Z - Diff.Z;
			}
			else
			{
				ax = Proc->AttractorPos.X - pP->p[0];
				ay = Proc->AttractorPos.Y - pP->p[1];
				az = Proc->AttractorPos.Z - pP->p[2];
			}

			#if 0 // Electric Attractor
			q = (float)sqrt(ax*ax + ay*ay + az*az + 1.0);
			q = Proc->AttractorStrength * 100.0f * time / (q*q*q);
			#else // Spring Attractor
			q = Proc->AttractorStrength * time;
			#endif
			ax *= q;
			ay *= q;
			az *= q;

			pP->v[0] += ax;
			pP->v[1] += ay;
			pP->v[2] += az;
		}
		
		assert( geVec3d_IsValid((geVec3d *)pP->p) );
		assert( geVec3d_IsValid((geVec3d *)pP->v) );
	}
}

geBoolean Particles_Draw(Procedural *Proc)
{
geBitmap *Lock=NULL;
uint8 *Bits;
int w,h,s,cnt,x,y;
Particle * pP;
uint8 *pBits,pel;

	assert(Proc->Bitmap);

	geBitmap_SetGammaCorrection(Proc->Bitmap, 1.0f, GE_FALSE);

	if ( ! geBitmap_LockForWriteFormat(Proc->Bitmap,&Lock,0,0,GE_PIXELFORMAT_8BIT_PAL) )
		goto fail;

	if ( ! geBitmap_GetInfo(Lock,&(Proc->BmInfo),NULL) )
		goto fail;

	if ( Proc->BmInfo.Format != GE_PIXELFORMAT_8BIT_PAL )
		goto fail;

	Bits = static_cast<uint8*>(geBitmap_GetBits(Lock));

	if ( ! Bits )
		goto fail; 

	w = Proc->BmInfo.Width;
	h = Proc->BmInfo.Height;
	s = Proc->BmInfo.Stride;

	for( cnt = NUM_CLEAR_POINTS; cnt --; )
	{
	int shade,color;
	int nc = Proc->NumColors;
		x = ProcUtil_Rand(w);
		y = ProcUtil_Rand(h);
		pBits = Bits + y*s + x;
		shade = PIXEL_SHADE(*pBits,nc);
		color = PIXEL_COLOR(*pBits,nc);
		shade >>= 1;
		*pBits = PIXEL_INDEX(color,shade,nc);
	}

	for(	cnt=Proc->NumActiveParticles,	pP = Proc->Particles;
			cnt > 0 ;
			pP++	)
	{
		assert( pP->shade >= 0 );

		x = (int)(pP->p[0]);
		y = (int)(pP->p[1]);
		putminmax(x, 1, w-2);
		putminmax(y, 1, h-2);
		pBits = Bits + (y * s) + x;
		pel = PIXEL_INDEX(pP->color,pP->shade,Proc->NumColors);
		pBits[0] = pBits[1] = pBits[-1] = pBits[s] = pBits[-s] = pel;

		cnt--;
	}

	for( cnt = Proc->NumSmoothes; cnt--; )
	{
		if ( ! geBitmapUtil_SmoothBits(&(Proc->BmInfo),Bits,Bits,Proc->SmoothRadius,Proc->SmoothWrap) )
			goto fail;
	}

	if ( ! geBitmap_UnLock(Lock) )
		goto fail;

	return GE_TRUE;
fail:

	if ( Lock )		geBitmap_UnLock(Lock);

	return GE_FALSE;
}

/*}{**************/

geBoolean Particles_InitBitmap(geBitmap **ppBitmap)
{
	assert( ppBitmap);
	if ( ! *ppBitmap )
	{
		*ppBitmap = geBitmap_Create(BM_CREATE_WIDTH,BM_CREATE_HEIGHT,1, GE_PIXELFORMAT_8BIT_PAL);
		if ( ! *ppBitmap )
			return GE_FALSE;
	}
	else
	{
		geBitmap_CreateRef(*ppBitmap);
	}

	if ( ! geBitmap_SetFormat(*ppBitmap,GE_PIXELFORMAT_8BIT_PAL,GE_FALSE,0,NULL) )
		return GE_FALSE;

	if ( ! geBitmap_ClearMips(*ppBitmap) )
		return GE_FALSE;

	if ( ! geBitmap_SetPreferredFormat(*ppBitmap,GE_PIXELFORMAT_16BIT_4444_ARGB) )
		return GE_FALSE;

	if ( ! geBitmapUtil_SetColor(*ppBitmap,0,0,0,0) )
		return GE_FALSE;

	return GE_TRUE;
}

geBoolean Particles_InitPalette(Procedural * Proc)
{
geBitmap_Info Info;
geBitmap_Palette *Pal;
void * PalData = NULL;
gePixelFormat PalFormat;
int PalSize;

	Pal = geBitmap_Palette_Create(GE_PIXELFORMAT_32BIT_ARGB,256);
	if ( ! Pal )
		goto fail;
	if ( ! geBitmap_SetPalette(Proc->Bitmap,Pal) )
		goto fail;
	geBitmap_Palette_Destroy(&Pal);

	if ( ! geBitmap_GetInfo(Proc->Bitmap,&Info,NULL) )
		goto fail;

	if ( Info.Format != GE_PIXELFORMAT_8BIT_PAL )
		goto fail;

	if ( ! (Pal = geBitmap_GetPalette(Proc->Bitmap)) )
		goto fail;

	if ( ! geBitmap_Palette_Lock(Pal,&PalData, &PalFormat, &PalSize) )
		goto fail;
	if ( PalSize < 256 )
		goto fail;

	{
	int p;
	uint8 * PalPtr;
	int R,G,B,A;
	int c,s;

		for(c=0;c<(Proc->NumColors);c++)
		{
			for(s=0;s<(Proc->NumShades);s++)
			{
				p = PIXEL_INDEX(c,s,Proc->NumColors);
				Proc->PixelRGBA(Proc,c,s,&R,&G,&B,&A);
				PalPtr = ((uint8 *)PalData) + p * gePixelFormat_BytesPerPel(PalFormat);
				gePixelFormat_PutColor(PalFormat,&PalPtr,R,G,B,A);
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

/*}{**************/

void Capper_Wrap(float *x,float *v, int size)
{
	if ( *x < 0 )
	{
		*x += size;
	}
	else if ( *x >= size )
	{
		*x -= size;
	}
}

void Capper_Hard(float *x,float *v, int size)
{
	if ( *x < 0 )
	{
		*x = 0;
	}
	else if ( *x >= size )
	{
		*x = (float)(size - 1);
	}
}

void Capper_Bounce(float *x,float *v, int size)
{
	if ( *x < 0 )
	{
		*x = 0;
		*v *= -1;
	}
	else if ( *x >= size )
	{
		*x = (float)(size - 1);
		*v *= -1;
	}
}

/*}{**************/

static Procedural_Table Particles_Table = 
{
	Procedurals_Version,Procedurals_Tag,
	"Particles",
	Particles_Create,
	Particles_Destroy,
	Particles_Animate
};

Procedural_Table * Particles_GetProcedural_Table(void)
{
	return &Particles_Table;
}

/*}{**************/

void PixelRGBA_OilColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A)
{
/*
	*R = *B = (255 * c) / (Proc->NumColors - 1);	
	*G = 255 - *B;
*/
	if ( Proc->NumColors == 1 ) *B = 0;
	else *B = (255 * c) / (Proc->NumColors - 1);	
	*G = *R = 255 - *B;
	*A = (255 * s) / (Proc->NumShades - 1);
	if ( *A > 200 ) *A = 255;
	else *A = (int)(300 * pow((*A)/255.0,0.7));
}

void PixelRGBA_FireColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A)
{
float sfrac;
	sfrac = (float)s / (Proc->NumShades - 1);	
	*R = (int)(255 * pow(sfrac, 0.3));
	*G = (int)(255 * pow(sfrac, 0.7));
	*B = (int)(128 * pow(sfrac, 3.0));
	if ( sfrac > 0.4 )	*A = 255;
	else				*A = (int)(530 * pow(sfrac, 0.8));
}

void PixelRGBA_OpaqueFireColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A)
{
float sfrac;
	sfrac = (float)s / (Proc->NumShades - 1);	
	*R = (int)(255 * pow(sfrac, 0.3));
	*G = (int)(255 * pow(sfrac, 0.7));
	*B = (int)(128 * pow(sfrac, 3.0));
	*A = std::min<int>((int)(400 * pow(sfrac, 0.3)),255);
}

void PixelRGBA_SteamColor(Procedural *Proc,int c,int s,int *R,int *G,int *B,int *A)
{
float sfrac;
	sfrac = (float)s / (Proc->NumShades - 1);	
	*A = (int)(255 * pow(sfrac, 0.5));
	*R = *G = *B = *A;
}
