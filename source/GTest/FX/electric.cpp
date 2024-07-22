/****************************************************************************/
/*    FILE: electric.c                                                      */
/*                                                                          */
/*    Copyright (c) 1999, WildTangent, Inc.; All rights reserved.       */
/*                                                                          */
/****************************************************************************/

#include	<windows.h>
#include	<math.h>
#include	<assert.h>

#include	"GENESIS.H"
#include	"Errorlog.h"

#include	"Electric.h"
#include	"..\\Procedurals\\gebmutil.h"

extern	geFloat		EffectScale;

static	geBitmap			*Bitmap;
static	geSound_Def *		LoopingDef;
static	geSound_Def *		SingleDef;
static	geSound_System *	SoundSys;

static geBoolean	Electric_SetWorld(geWorld *World, geVFile *Context);

typedef struct	Electric_BoltEffect
{
	int			beInitialized;
	int			beNumPoints;
	geFloat		beWildness;

	/* For rendering */
	geVec3d		beStart;
	geVec3d		beEnd;

	/* For generating the geometry */
	geVec3d	*	beCenterPoints;
	geVec3d *	beCurrentPoint;

	geFloat		beBaseColors[3];
	geFloat		beCurrentColors[3];
	geFloat		beBaseBlue;
	int			beDecayRate;
	int			beDominantColor;

	int			beWidth;

	geBitmap	*beBitmap;

}	Electric_BoltEffect;

Electric_BoltEffect * Electric_BoltEffectCreate(
	geBitmap				*Texture,	/* The texture we map onto the bolt */
	geBitmap				*Texture2,	/* The texture we map onto the bolt */
	int 					NumPolys,	/* Number of polys, must be power of 2 */
	int						Width,		/* Width in world units of the bolt */
	geFloat 				Wildness);	/* How wild the bolt is (0 to 1 inclusive) */

void Electric_BoltEffectDestroy(Electric_BoltEffect *Effect);

void Electric_BoltEffectAnimate(
	Electric_BoltEffect *	Effect,
	const geVec3d *			start,		/* Starting point of the bolt */
	const geVec3d *			end);		/* Ending point of the bolt */

void Electric_BoltEffectRender(
	geWorld *				World,		/* World to render for */
	Electric_BoltEffect *	Effect,		/* Bolt to render */
	const geXForm3d *		XForm);		/* Transform of our point of view */

void Electric_BoltEffectSetColorInfo(
	Electric_BoltEffect *	Effect,
	GE_RGBA *				BaseColor,		/* Base color of the bolt (2 colors should be the same */
	int						DominantColor);	/* Which color is the one to leave fixed */

static	int		logBase2(int n)
{
	int	i = 0;

	assert(n != 0);

	while	(!(n & 1))
	{
		n = n >> 1;
		i++;
	}

	assert((n & ~1) == 0);

	return i;
}

static	geBoolean	IsPowerOf2(int n)
{
	if	(n == 0)
		return GE_TRUE;

	while	(!(n & 1))
		n = n >> 1;

	if	(n & ~1)
		return GE_FALSE;

	return GE_TRUE;
}

Electric_BoltEffect * Electric_BoltEffectCreate(
	geBitmap		*Bitmap,
	geBitmap		*Bitmap2,
 	int NumPolys,
 	int Width,
	geFloat Wildness)
{
	Electric_BoltEffect *	be;
	GE_RGBA					color;

	assert(Wildness >= 0.0f && Wildness <= 1.0f);

	/* Asserts power of 2 */
	logBase2(NumPolys);

	be = (Electric_BoltEffect *)malloc(sizeof(*be));
	if	(!be)
		return be;

	memset(be, 0, sizeof(*be));

	be->beCenterPoints = (geVec3d *)malloc(sizeof(*be->beCenterPoints) * (NumPolys + 1));
	if	(!be->beCenterPoints)
		goto fail;

	be->beBitmap	= Bitmap;
//	be->beBitmap2	= Bitmap2;
	be->beNumPoints	= NumPolys;
	be->beWildness	= Wildness;
	be->beWidth		= Width;

//	color.r = 255.0f;
//	color.g = 60.0f;
//	color.b = 60.0f;
//	Electric_BoltEffectSetColorInfo(be, &color, ELECTRIC_BOLT_REDDOMINANT);

//	color.r = 60.0f;
//	color.g = 255.0f;
//	color.b = 60.0f;
//	Electric_BoltEffectSetColorInfo(be, &color, ELECTRIC_BOLT_GREENDOMINANT);

	color.r = 160.0f;
	color.g = 160.0f;
	color.b = 255.0f;
	Electric_BoltEffectSetColorInfo(be, &color, ELECTRIC_BOLT_BLUEDOMINANT);

	return be;

fail:
	if	(be->beCenterPoints)
		free(be->beCenterPoints);

	return NULL;
}

void Electric_BoltEffectDestroy(Electric_BoltEffect *Effect)
{
	free(Effect->beCenterPoints);
	free(Effect);
}

static	geFloat	GaussRand(void)
{
	int	i;
	int	r;

	r = 0;

	for	(i = 0; i < 6; i++)
		r = r + rand() - rand();

	return (geFloat)r / ((geFloat)RAND_MAX * 6.0f);
}

static	void	subdivide(
	Electric_BoltEffect *	be,
	const geVec3d *			start,
	const geVec3d *			end,
	geFloat 				s,
	int 					n)
{
	geVec3d	tmp;

	if	(n == 0)
	{
		be->beCurrentPoint++;
		*be->beCurrentPoint = *end;
		return;
	}
	
	tmp.X = (end->X + start->X) / 2 + s * GaussRand();
	tmp.Y = (end->Y + start->Y) / 2 + s * GaussRand();
	tmp.Z = (end->Z + start->Z) / 2 + s * GaussRand();
	subdivide(be,  start, &tmp, s / 2, n - 1);
	subdivide(be, &tmp,    end, s / 2, n - 1);
}

#define	LIGHTNINGWIDTH 8.0f

static	void	genLightning(
	Electric_BoltEffect *	be,
	int 					RangeLow,
	int 					RangeHigh,
	const geVec3d *			start,
	const geVec3d *			end)
{
	geFloat	length;
	int		seed;

	assert(be);
	assert(start);
	assert(end);
	assert(RangeHigh > RangeLow);
	assert(IsPowerOf2(RangeHigh - RangeLow));

	/* Manhattan length is good enough for this */
	length = (geFloat)(fabs(start->X - end->X) +
						fabs(start->Y - end->Y) +
						fabs(start->Z - end->Z));

	seed = rand();

	srand(seed);
	be->beCurrentPoint					= be->beCenterPoints + RangeLow;
	be->beCenterPoints[RangeLow]		= *start;
	be->beCenterPoints[RangeHigh] 		= *end;
//	be->beCenterPoints[be->beNumPoints] = *end;
//	subdivide(be, start, end, length * be->beWildness, logBase2(be->beNumPoints));
	subdivide(be, start, end, length * be->beWildness, logBase2(RangeHigh - RangeLow));
}

void Electric_BoltEffectSetColorInfo(
	Electric_BoltEffect *	Effect,
	GE_RGBA *				BaseColor,
	int						DominantColor)
{
	Effect->beBaseColors[0]		= BaseColor->r;
	Effect->beBaseColors[1]		= BaseColor->g;
	Effect->beBaseColors[2]		= BaseColor->b;
	Effect->beCurrentColors[0]	= BaseColor->r;
	Effect->beCurrentColors[1]	= BaseColor->g;
	Effect->beCurrentColors[2]	= BaseColor->b;
	Effect->beDominantColor 	= DominantColor;
}

void Electric_BoltEffectAnimate(
	Electric_BoltEffect *	Effect,
	const geVec3d *			start,
	const geVec3d *			end)
{
	int		dominant;
	int		nonDominant1;
	int		nonDominant2;
	geVec3d	SubdivideStart;
	geVec3d	SubdivideEnd;
	int		LowIndex;
	int		HighIndex;

	Effect->beStart = *start;
	Effect->beEnd	= *end;

	dominant = Effect->beDominantColor;
	nonDominant1 = (dominant + 1) % 3;
	nonDominant2 = (dominant + 2) % 3;
	if	(Effect->beBaseColors[nonDominant1] == Effect->beCurrentColors[nonDominant1])
	{
		int	DecayRate;
		int	Spike;

		DecayRate = rand() % (int)(Effect->beBaseColors[dominant] - Effect->beBaseColors[nonDominant1]);
		DecayRate = max(DecayRate, 5);
		Effect->beDecayRate = DecayRate;
		if	(Effect->beBaseColors[nonDominant1] >= 1.0f)
			Spike = rand() % (int)(Effect->beBaseColors[nonDominant1]);
		else
			Spike = 0;
		Effect->beCurrentColors[nonDominant1] -= Spike;
		Effect->beCurrentColors[nonDominant2] -= Spike;
	}
	else
	{
		Effect->beCurrentColors[nonDominant1] += Effect->beDecayRate;
		Effect->beCurrentColors[nonDominant2] += Effect->beDecayRate;
		if	(Effect->beCurrentColors[nonDominant1] > Effect->beBaseColors[nonDominant1])
		{
			Effect->beCurrentColors[nonDominant1] = Effect->beBaseColors[nonDominant1];
			Effect->beCurrentColors[nonDominant2] = Effect->beBaseColors[nonDominant2];
		}
	}

	if	(Effect->beInitialized && Effect->beNumPoints > 16)
	{
		int		P1;
		int		P2;
		int		P3;
		int		P4;

		switch	(rand() % 7)
		{
			case	0:
				genLightning(Effect, 0, Effect->beNumPoints, start, end);
				return;

			case	1:
			case	2:
			case	3:
				P1 = 0;
				P2 = Effect->beNumPoints / 2;
				P3 = P2 + Effect->beNumPoints / 4;
				P4 = Effect->beNumPoints;
				break;

			case	4:
			case	5:
			case	6:
				P1 = 0;
				P3 = Effect->beNumPoints / 2;
				P2 = P3 - Effect->beNumPoints / 4;
				P4 = Effect->beNumPoints;
				break;
		}
		SubdivideStart = Effect->beCenterPoints[P1];
		SubdivideEnd = Effect->beCenterPoints[P2];
		genLightning(Effect, P1, P2, &SubdivideStart, &SubdivideEnd);
		SubdivideStart = Effect->beCenterPoints[P2];
		SubdivideEnd = Effect->beCenterPoints[P3];
		genLightning(Effect, P2, P3, &SubdivideStart, &SubdivideEnd);
		SubdivideStart = Effect->beCenterPoints[P3];
		SubdivideEnd = Effect->beCenterPoints[P4];
		genLightning(Effect, P3, P4, &SubdivideStart, &SubdivideEnd);
	}
	else
	{
		Effect->beInitialized = 1;
		LowIndex = 0;
		HighIndex = Effect->beNumPoints;
		SubdivideStart = *start;
		SubdivideEnd   = *end;

		genLightning(Effect, LowIndex, HighIndex, &SubdivideStart, &SubdivideEnd);
	}
}

#if 0
static	void	DrawPoint(geWorld *world, geVec3d *pos, geBitmap *Bitmap, int r, int g, int b)
{
	GE_LVertex	vert;

	vert.X = pos->X;
	vert.Y = pos->Y;
	vert.Z = pos->Z;
	vert.r = (geFloat)r;
	vert.g = (geFloat)g;
	vert.b = (geFloat)b;
	vert.a = 255.0f;
	vert.u = vert.v = 0.0f;

	GE_WorldAddPolyOnce(world,
						&vert,
						1,
						Bitmap,
						GE_TEXTURED_POINT,
						GE_FX_TRANSPARENT,
						EffectScale);
}
#endif

#define	LIGHTNINGALPHA	160.0f

void Electric_BoltEffectRender(
	geWorld *				World,
	Electric_BoltEffect *	be,
	const geXForm3d *		XForm)
{
	geVec3d			perp;
	geVec3d			temp;
	geVec3d			in;
	GE_LVertex 		verts[4];
	int				i;

	geVec3d_Subtract(&be->beStart, &be->beEnd, &temp);
	geXForm3d_GetIn(XForm, &in);

	geVec3d_CrossProduct(&in, &temp, &perp);
	geVec3d_Normalize(&perp);

	geVec3d_Scale(&perp, be->beWidth / 2.0f, &perp);

	/*
		We've got the perpendicular to the camera in the
		rough direction of the electric bolt center.  Walk
		the left and right sides, constructing verts, then
		do the drawing.
	*/
	for	(i = 0; i < be->beNumPoints - 1; i++)
	{
		geVec3d	temp;

		geVec3d_Subtract(&be->beCenterPoints[i], &perp, &temp);
		verts[0].X = temp.X;
		verts[0].Y = temp.Y;
		verts[0].Z = temp.Z;
		verts[0].u = 0.0f;
		verts[0].v = 0.0f;
		verts[0].r = be->beCurrentColors[0];
		verts[0].g = be->beCurrentColors[1];
		verts[0].b = be->beCurrentColors[2];
		verts[0].a = LIGHTNINGALPHA;

		geVec3d_Subtract(&be->beCenterPoints[i + 1], &perp, &temp);
		verts[1].X = temp.X;
		verts[1].Y = temp.Y;
		verts[1].Z = temp.Z;
		verts[1].u = 0.0f;
		verts[1].v = 1.0f;
		verts[1].r = be->beCurrentColors[0];
		verts[1].g = be->beCurrentColors[1];
		verts[1].b = be->beCurrentColors[2];
		verts[1].a = LIGHTNINGALPHA;

		geVec3d_Add(&be->beCenterPoints[i + 1], &perp, &temp);
		verts[2].X = temp.X;
		verts[2].Y = temp.Y;
		verts[2].Z = temp.Z;
		verts[2].u = 1.0f;
		verts[2].v = 1.0f;
		verts[2].r = be->beCurrentColors[0];
		verts[2].g = be->beCurrentColors[1];
		verts[2].b = be->beCurrentColors[2];
		verts[2].a = LIGHTNINGALPHA;

		geVec3d_Add(&be->beCenterPoints[i], &perp, &temp);
		verts[3].X = temp.X;
		verts[3].Y = temp.Y;
		verts[3].Z = temp.Z;
		verts[3].u = 1.0f;
		verts[3].v = 0.0f;
		verts[3].r = be->beCurrentColors[0];
		verts[3].g = be->beCurrentColors[1];
		verts[3].b = be->beCurrentColors[2];
		verts[3].a = LIGHTNINGALPHA;

		geWorld_AddPolyOnce(World,
							verts,
							4,
							be->beBitmap,
							GE_TEXTURED_POLY,
							GE_RENDER_DO_NOT_OCCLUDE_OTHERS,
							1.0f);

//		DrawPoint(World, &be->beCenterPoints[i], be->beTexture, 255, 0, 0);
	}
}

geBoolean Electric_Init(geEngine *Engine, geWorld *World, geVFile * MainFS, geSound_System *SoundSystem)
{
	Engine;

	if	(SoundSystem)
	{
		geVFile *	File;

		SoundSys = SoundSystem;
		assert(SoundSys);
		File = geVFile_Open(MainFS, "wav\\loopbzzt.wav", GE_VFILE_OPEN_READONLY);
		if	(!File)
			return GE_FALSE;
		LoopingDef = geSound_LoadSoundDef(SoundSys, File);
		geVFile_Close(File);
		File = geVFile_Open(MainFS, "wav\\onebzzt.wav", GE_VFILE_OPEN_READONLY);
		if	(!File)
			return GE_FALSE;
		
		SingleDef = geSound_LoadSoundDef(SoundSys, File);
		
		geVFile_Close(File);
		if	(!LoopingDef || !SingleDef)
			return GE_FALSE;
	}

	if (!Electric_SetWorld(World, MainFS))
	{
		// FIXME: Free more stuff
		return GE_FALSE;
	}

	return GE_TRUE;
}

geBoolean Electric_Reset(geWorld *World)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;

	if	(!World)
		return GE_TRUE;
 
	Set = geWorld_GetEntitySet(World, "ElectricBolt");
	if	(Set == NULL)
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		ElectricBolt *		Bolt;

		Bolt = static_cast<ElectricBolt*>(geEntity_GetUserData(Entity));
		Bolt->LastTime = 0.0f;
		Bolt->LastBoltTime = 0.0f;

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}

static geBoolean Electric_SetWorld(geWorld *World, geVFile *MainFS)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;

	assert(World);
	assert(MainFS);

	assert(Bitmap == NULL);

	Set = geWorld_GetEntitySet(World, "ElectricBolt");

	if	(Set == NULL)
		return GE_TRUE;

	Bitmap = geBitmapUtil_CreateFromFileAndAlphaNames(MainFS, "Bmp\\Bolt.Bmp", "Bmp\\Bolt.Bmp");

	if (!Bitmap)
	{
		geErrorLog_AddString(-1, "Electric_SetWorld:  geBitmapUtil_CreateFromFileAndAlphaNames failed:", "Bmp\\Bolt.Bmp, Bmp\\Bolt.Bmp");
		return GE_FALSE;
	}

	if (!geWorld_AddBitmap(World, Bitmap))
	{
		geBitmap_Destroy(&Bitmap);
		return GE_FALSE;
	}

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		ElectricBolt *	Bolt;

		Bolt = static_cast<ElectricBolt*>(geEntity_GetUserData(Entity));
		if (Bolt->Terminus == NULL)
			{
				#define MAX_NAME 100
				char	EntityName[MAX_NAME];
				char    s[MAX_NAME + 200];
				geEntity_GetName(Entity, EntityName, MAX_NAME-1);
				EntityName[MAX_NAME-1]=0;
				sprintf(s,"Name='%s' Origin=%f,%f,%f",EntityName,Bolt->origin.X,Bolt->origin.Y,Bolt->origin.Z);
				geErrorLog_AddString(-1,"Electric_SetWorld:  ElectricBolt entity has no terminius.  ",s);
				geWorld_RemoveBitmap(World, Bitmap);
				geBitmap_Destroy(&Bitmap);
				return GE_FALSE;
			}
		Bolt->Bolt = Electric_BoltEffectCreate(Bitmap,
											   NULL,
											   Bolt->NumPoints,
											   Bolt->Width,
											   Bolt->Wildness);

		Electric_BoltEffectSetColorInfo(Bolt->Bolt, &Bolt->Color, Bolt->DominantColor);

		if	(Bolt->Bolt == NULL)
		{
			geWorld_RemoveBitmap(World, Bitmap);
			geBitmap_Destroy(&Bitmap);
			return GE_FALSE;
		}

		if	(!Bolt->Intermittent && SoundSys)
		{
			Bolt->LoopingSound = geSound_PlaySoundDef(SoundSys,
													  LoopingDef,
													  0.0f,
													  0.0f,
													  0.0f,
													  GE_TRUE);
			if	(!Bolt->LoopingSound)
			{
				geWorld_RemoveBitmap(World, Bitmap);
				geBitmap_Destroy(&Bitmap);
				return GE_FALSE;
			}
		}

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}

geBoolean Electric_Shutdown(void)
{
	if (LoopingDef)
	{
		geSound_FreeSoundDef(SoundSys, LoopingDef);
		LoopingDef = NULL;
	}
	
	if (SingleDef)
	{
		geSound_FreeSoundDef(SoundSys, SingleDef);
		SingleDef = NULL;
	}

	if (Bitmap)
	{
		//geWorld_RemoveBitmap(World, Bitmap);
		geBitmap_Destroy(&Bitmap);
		Bitmap = NULL;
	}

	return GE_TRUE;
}

static	geFloat	frand(geFloat Low, geFloat High)
{
	geFloat	Range;

	//assert(High > Low);

	Range = High - Low;

	return ((geFloat)(((rand() % 1000) + 1))) / 1000.0f * Range + Low;
}

#define	LIGHTNINGSTROKEDURATION	0.05f

geBoolean Electric_Frame(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;
 
	if	(!World)
		return GE_TRUE;

	Set = geWorld_GetEntitySet(World, "ElectricBolt");
	if	(Set == NULL)
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		ElectricBolt *	Bolt;
		geFloat			Volume;
		geFloat			Pan;
		geFloat			Frequency;
		geVec3d			MidPoint;
		int32			Leaf;

		Bolt = static_cast<ElectricBolt*>(geEntity_GetUserData(Entity));

		geVec3d_Subtract(&Bolt->Terminus->origin, &Bolt->origin, &MidPoint);
		geVec3d_AddScaled(&Bolt->origin, &MidPoint, 0.5f, &MidPoint);

		geWorld_GetLeaf(World, &MidPoint, &Leaf);
		
		if (geWorld_MightSeeLeaf(World, Leaf))
		{
		geSound3D_GetConfig(World,
							XForm,
							&MidPoint,
							600.0f,
							2.0f,
							&Volume,
							&Pan,
							&Frequency);

		Bolt->LastTime += DeltaTime;

		if	(!Bolt->Intermittent ||
			 (Bolt->LastTime - Bolt->LastBoltTime > frand(Bolt->MaxFrequency, Bolt->MinFrequency)))
		{
			Electric_BoltEffectAnimate(Bolt->Bolt,
									   &Bolt->origin,
									   &Bolt->Terminus->origin);

			if	(Bolt->Intermittent && SoundSys)
				geSound_PlaySoundDef(SoundSys, SingleDef, Volume, Pan, Frequency, GE_FALSE);

			Bolt->LastBoltTime = Bolt->LastTime;
		}

		if	(!Bolt->Intermittent && SoundSys)
			geSound_ModifySound(SoundSys, Bolt->LoopingSound, Volume, Pan, Frequency);

		if	(Bolt->LastTime - Bolt->LastBoltTime <= LIGHTNINGSTROKEDURATION)
			Electric_BoltEffectRender(World, Bolt->Bolt, XForm);
		}

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}

