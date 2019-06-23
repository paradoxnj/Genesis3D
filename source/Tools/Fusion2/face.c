/****************************************************************************************/
/*  face.c                                                                              */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird                                                */
/*  Description:  Face csg, management, io, etc...                                      */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include "face.h"
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "basetype.h"
#include "units.h"
#include "box3d.h"
#include "typeio.h"
#include "consoletab.h"
#include "ram.h"
#include "quatern.h"
#include "util.h"

const geVec3d	VecOrigin	={ 0.0f, 0.0f, 0.0f };

//temp buffers for splitting faces
//use caution in recursive code that uses these...
static	geVec3d		spf[256], spb[256];

#define	VCOMPARE_EPSILON			(0.001f)
#define MAX_POINTS					64
#define	ON_EPSILON					(0.1f)
#define	VectorToSUB(a, b)			(*((((geFloat *)(&a))) + (b)))
#define FACE_DEFAULT_LIGHT			300
#define FACE_DEFAULT_BIAS			(1.0f)
#define FACE_DEFAULT_TRANSLUCENCY	(255.0f)
#define FACE_DEFAULT_REFLECTIVITY	(1.0f)

enum FaceFlags
{
	FACE_MIRROR		= (1<<0),
	FACE_FULLBRIGHT	= (1<<1),
	FACE_SKY		= (1<<2),
	FACE_LIGHT		= (1<<3),
	FACE_SELECTED	= (1<<4),
	FACE_FIXEDHULL	= (1<<5),		//doesn't expand (rings)
	FACE_GOURAUD	= (1<<6),
	FACE_FLAT		= (1<<7),
	FACE_TEXTURELOCKED = (1<<8),
	FACE_VISIBLE	= (1<<9),
	FACE_SHEET		= (1<<10),		//visible from both sides
	FACE_TRANSPARENT= (1<<11)		//use transparency value for something
};

enum OldFaceFlags
{
	oldINSOLID		=1,
	oldSELECTED		=2,
	oldLIGHT		=4,
	oldMIRROR		=8,
	oldFULLBRIGHT	=16,
	oldSKY			=32
};

enum SideFlags
{
	SIDE_FRONT	=0,
	SIDE_BACK	=1,
	SIDE_ON		=2
};

typedef struct TexInfoTag
{
	geVec3d VecNormal;
	geFloat xScale, yScale;
	int xShift, yShift;
	geFloat	Rotate;			// texture rotation angle in degrees
	TexInfo_Vectors TVecs;
	int Dib;				// index into the wad
	char Name[16];
	geBoolean DirtyFlag;
	geVec3d Pos;
	int txSize, tySize;		// texture size (not currently used)
	geXForm3d XfmFaceAngle;	// face rotation angle
} TexInfo;

typedef struct FaceTag
{
	int			NumPoints;
	int			Flags;
	Plane		Face_Plane;
	int			LightIntensity;
	geFloat		Reflectivity;
	geFloat		Translucency;
	geFloat		MipMapBias;
	geFloat		LightXScale, LightYScale;
	TexInfo		Tex;
	geVec3d		*Points;
} Face;

enum NewFaceFlags
{
	ffMirror		= (1<<0),	// face is a mirror
	ffFullBright	= (1<<1),	// give face full brightness
	ffSky			= (1<<2),	// face is sky
	ffLight			= (1<<3),	// face emits light
	ffTranslucent	= (1<<4),	// internal to tools
	ffGouraud		= (1<<5),	// shading
	ffFlat			= (1<<6)
};

static void Face_SetTexInfoPlane
	(
	  TexInfo *t,
	  geVec3d const *pNormal
	)
{
	assert (t != NULL);
	assert (pNormal != NULL);
	assert (geVec3d_IsNormalized (pNormal));

	t->VecNormal = *pNormal;

	t->DirtyFlag = GE_TRUE;
}

static void Face_InitFaceAngle
	(
	  TexInfo *t,
	  geVec3d const *pNormal
	)
{
	geVec3d VecDest;
	geVec3d VecAxis;
	geFloat cosv, Theta;
	geVec3d		PosNormal;

	PosNormal = *pNormal;

	if (fabs(pNormal->X) > fabs(pNormal->Y))
	{
		if (fabs(pNormal->X) > fabs(pNormal->Z))
		{
			if (pNormal->X > 0)
				geVec3d_Inverse(&PosNormal);
		}
		else
		{
			if (pNormal->Z > 0)
				geVec3d_Inverse(&PosNormal);
		}

	}
	else 
	{
		if (fabs(pNormal->Y) > fabs(pNormal->Z))
		{
			if (pNormal->Y > 0)
				geVec3d_Inverse(&PosNormal);
		}
		else
		{
			if (pNormal->Z > 0)
				geVec3d_Inverse(&PosNormal);
		}
	}
	
	// Create rotation matrix that will put this face into the X,Y plane.
	geVec3d_Set (&VecDest, 0.0f, 0.0f, 1.0f);
	geVec3d_CrossProduct (&VecDest, &PosNormal, &VecAxis);
	cosv = geVec3d_DotProduct (&VecDest, &PosNormal);
	if (cosv > 1.0f)
	{
		cosv = 1.0f;
	}
	Theta = (geFloat)acos (cosv);
	if (geVec3d_Normalize (&VecAxis) == 0.0f)
	{
		// If the resulting vector is 0 length, 
		// then a rotation about X will put us where we need to be.
		geXForm3d_SetIdentity (&t->XfmFaceAngle);
		geXForm3d_RotateX (&t->XfmFaceAngle, -Theta);
	}
	else
	{
		geQuaternion QRot;

		geQuaternion_SetFromAxisAngle (&QRot, &VecAxis, -Theta);
		geQuaternion_ToMatrix (&QRot, &t->XfmFaceAngle);
	}
}

static void Face_InitTexInfo
	(
	  TexInfo *t,
	  geVec3d const *pNormal
	)
{
	assert (t != NULL);
	assert (pNormal != NULL);

	t->Name[0] = '\0';
	t->xScale = 1.0f;
	t->yScale = 1.0f;
	t->xShift = 0;
	t->yShift = 0;
	t->Rotate = 0.0f;
	t->Dib = 0;
	t->DirtyFlag = GE_FALSE;
	t->txSize = 0;
	t->tySize = 0;
	geVec3d_Clear (&t->Pos);
	Face_SetTexInfoPlane (t, pNormal);
	Face_InitFaceAngle( t, pNormal );
}

static geBoolean	Face_SetPlaneFromFace(Face *f)
{
	int		i;
	geVec3d	v1, v2;

	assert(f != NULL);

	//catches colinear points now
	for(i=0;i < f->NumPoints;i++)
	{
		//gen a plane normal from the cross of edge vectors
		geVec3d_Subtract(&f->Points[i], &f->Points[(i+1) % f->NumPoints], &v1);
		geVec3d_Subtract(&f->Points[(i+2) % f->NumPoints], &f->Points[(i+1) % f->NumPoints], &v2);

		geVec3d_CrossProduct(&v1, &v2, &f->Face_Plane.Normal);
		if(!geVec3d_Compare(&f->Face_Plane.Normal, &VecOrigin, VCOMPARE_EPSILON))
		{
			break;
		}
		//try the next three if there are three
	}
	if(i >= f->NumPoints)
	{
		ConPrintf("Face with no normal!\n");
		return	GE_FALSE;
	}
	geVec3d_Normalize(&f->Face_Plane.Normal);
	f->Face_Plane.Dist	=geVec3d_DotProduct(&f->Points[1], &f->Face_Plane.Normal);

	Face_SetTexInfoPlane(&f->Tex, &f->Face_Plane.Normal);
	return	GE_TRUE;
}

static void Face_SetTexturePos (Face *f)
{
//	Face_GetCenter (f, &f->Tex.Pos);
	geVec3d_Clear (&f->Tex.Pos);
	f->Tex.DirtyFlag = GE_TRUE;
}


Face	*Face_Create(int NumPnts, const geVec3d *pnts, int DibId)
{
	Face	*f;

	assert(NumPnts > 0);
	assert(NumPnts < MAX_POINTS);
	assert(pnts != NULL);

	f	=geRam_Allocate(sizeof(Face));
	if(f)
	{
		memset(f, 0, sizeof(Face));

		f->NumPoints=NumPnts;
		f->LightIntensity = FACE_DEFAULT_LIGHT;
		f->MipMapBias = FACE_DEFAULT_BIAS;
		f->Translucency = FACE_DEFAULT_TRANSLUCENCY;
		f->Reflectivity = FACE_DEFAULT_REFLECTIVITY;
		f->LightXScale = 1.0f;
		f->LightYScale = 1.0f;

		Face_SetVisible(f, GE_TRUE);

		f->Points	=geRam_Allocate(sizeof(geVec3d) * NumPnts);
		if(f->Points)
		{
			memcpy(f->Points, pnts, sizeof(geVec3d) * NumPnts);

			if(Face_SetPlaneFromFace(f))
			{
				Face_InitTexInfo(&f->Tex, &f->Face_Plane.Normal);
				Face_SetTextureDibId (f, DibId);
				Face_SetTexturePos (f);
			}
			else
			{
				geRam_Free (f->Points);
				geRam_Free (f);
				f	=NULL;
			}
		}
		else
		{
			geRam_Free (f);
			f	=NULL;
		}
	}
	return	f;
}

//builds a large face based on p
Face	*Face_CreateFromPlane(const Plane *p, geFloat Radius, int DibId)
{
	geFloat	v;
	geVec3d	vup, vright, org, pnts[4];
	
	assert(p != NULL);

	//find the major axis of p->Normal
	geVec3d_Set (&vup, 0.0f, 0.0f, 1.0f);
	if((fabs(p->Normal.Z) > fabs(p->Normal.X))
		&&(fabs(p->Normal.Z) > fabs(p->Normal.Y)))
	{
	    geVec3d_Set(&vup, 1.0f, 0.0f, 0.0f);
	}

	v	=geVec3d_DotProduct(&vup, &p->Normal);
	geVec3d_AddScaled (&vup, &p->Normal, -v, &vup);
	geVec3d_Normalize(&vup);
		
	geVec3d_AddScaled (&VecOrigin, &p->Normal, p->Dist, &org);
	geVec3d_CrossProduct(&vup, &p->Normal, &vright);

	geVec3d_Scale(&vup, Radius, &vup);
	geVec3d_Scale(&vright, Radius, &vright);

	geVec3d_Subtract(&org, &vright, &pnts[0]);
	geVec3d_Add(&pnts[0], &vup, &pnts[0]);

	geVec3d_Add(&org, &vright, &pnts[1]);
	geVec3d_Add(&pnts[1], &vup, &pnts[1]);

	geVec3d_Add(&org, &vright, &pnts[2]);
	geVec3d_Subtract(&pnts[2], &vup, &pnts[2]);

	geVec3d_Subtract(&org, &vright, &pnts[3]);
	geVec3d_Subtract(&pnts[3], &vup, &pnts[3]);

	return	Face_Create(4, pnts, DibId);
}

void	Face_Destroy(Face **f)
{
	assert(f != NULL);
	assert(*f != NULL);

	if((*f)->Points)
	{
		geRam_Free ((*f)->Points);
	}

	geRam_Free (*f);
	*f	=NULL;
}

Face	*Face_Clone(const Face *src)
{
	Face	*dst;

	assert(src != NULL);
	assert(src->NumPoints > 0);
	assert(src->Points != NULL);

	dst	=Face_Create(src->NumPoints, src->Points, Face_GetTextureDibId (src));
	if(dst)
	{
		Face_CopyFaceInfo(src, dst);
	}
	return	dst;
}

Face	*Face_CloneReverse(const Face *src)
{
	int		i;
	Face	*dst;
	geVec3d	pt;

	assert(src != NULL);
	assert(src->NumPoints >0);
	assert(src->Points != NULL);

	dst	=Face_Clone(src);
	if(dst)
	{
		dst->Face_Plane.Dist=-dst->Face_Plane.Dist;
		geVec3d_Inverse(&dst->Face_Plane.Normal);

		for(i=0;i < dst->NumPoints/2;i++)
		{
			pt								=dst->Points[i];
			dst->Points[i]					=dst->Points[dst->NumPoints-i-1];
			dst->Points[dst->NumPoints-i-1] =pt;
		}
		Face_SetPlaneFromFace(dst);
	}
	return dst;
}

int	Face_GetNumPoints(const Face *f)
{
	assert(f != NULL);

	return	f->NumPoints;
}

const Plane	*Face_GetPlane(const Face *f)
{
	assert(f != NULL);

	return	&f->Face_Plane;
}

const geVec3d	*Face_GetPoints(const Face *f)
{
	assert(f != NULL);

	return	f->Points;
}

int Face_GetLightIntensity(const Face *f)
{
	assert (f != NULL);

	return f->LightIntensity;
}

void Face_GetLightScale(const Face *f, geFloat *pxScale, geFloat *pyScale)
{
	assert(f != NULL);
	assert(pxScale != NULL);
	assert(pyScale != NULL);

	*pxScale = f->LightXScale;
	*pyScale = f->LightYScale;
}


void	Face_GetTextureScale(const Face *f, geFloat *pxScale, geFloat *pyScale)
{
	assert(f != NULL);
	assert(pxScale != NULL);
	assert(pyScale != NULL);

	*pxScale = f->Tex.xScale;
	*pyScale = f->Tex.yScale;
}

void	Face_GetTextureShift(const Face *f, int *pxShift, int *pyShift)
{
	assert(f != NULL);
	assert(pxShift != NULL);
	assert(pyShift != NULL);

	*pxShift = f->Tex.xShift;
	*pyShift = f->Tex.yShift;
}

geFloat	Face_GetTextureRotate(const Face *f)
{
	assert(f != NULL);

	return	f->Tex.Rotate;
}


static void Face_UpdateLockedTextureVecs
	(
	  Face *f
	)
{
	geXForm3d XfmTexture;
	TexInfo *t = &f->Tex;

	assert (t != NULL);
//	assert (t->xScale != 0.0f);
//	assert (t->yScale != 0.0f);
#pragma message ("this is an ugly hack.  Values should never be == 0.")
	if (t->xScale == 0.0f) t->xScale = 1.0f;
	if (t->yScale == 0.0f) t->yScale = 1.0f;

	// the normal has to be normal, no?
	assert ((t->VecNormal.X != 0.0f) || 
			(t->VecNormal.Y != 0.0f) || 
			(t->VecNormal.Z != 0.0f));

	// Compute rotation
	geVec3d_Clear (&t->XfmFaceAngle.Translation);
	geXForm3d_SetZRotation (&XfmTexture, Units_DegreesToRadians (-t->Rotate));
	geXForm3d_Multiply (&t->XfmFaceAngle, &XfmTexture, &XfmTexture);

	// get info from transform into texture vectors.
	geVec3d_Set (&t->TVecs.uVec, XfmTexture.AX, XfmTexture.BX, XfmTexture.CX);
	geVec3d_Set (&t->TVecs.vVec, XfmTexture.AY, XfmTexture.BY, XfmTexture.CY);

	// and scale accordingly
	geVec3d_Scale (&t->TVecs.uVec, 1.0f/t->xScale, &t->TVecs.uVec);
	geVec3d_Scale (&t->TVecs.vVec, 1.0f/t->yScale, &t->TVecs.vVec);

	// compute offsets...
	{
		geFloat uOffset, vOffset;

		uOffset = geVec3d_DotProduct (&t->TVecs.uVec, &f->Tex.Pos);
		vOffset = geVec3d_DotProduct (&t->TVecs.vVec, &f->Tex.Pos);

		t->TVecs.uOffset = (float)(t->xShift - uOffset);
 		t->TVecs.vOffset = (float)(t->yShift - vOffset);
	}
}


static void Face_UpdateWorldTextureVecs
	(
	  Face *f
	)
{
	geFloat	ang, sinv, cosv;
	geVec3d uVec, vVec;
	int WhichAxis;
	TexInfo *t = &f->Tex;

	assert (t != NULL);
//	assert (t->xScale != 0.0f);
//	assert (t->yScale != 0.0f);
#pragma message ("this is an ugly hack.  Values should never be == 0.")
	if (t->xScale == 0.0f) t->xScale = 1.0f;
	if (t->yScale == 0.0f) t->yScale = 1.0f;

	ang = UNITS_DEGREES_TO_RADIANS (-t->Rotate);
	sinv = (geFloat)sin(ang);
	cosv = (geFloat)cos(ang);

	// the normal has to be normal, no?
	assert ((t->VecNormal.X != 0.0f) || 
			(t->VecNormal.Y != 0.0f) || 
			(t->VecNormal.Z != 0.0f));

	// Must check x, y, z in order to match tools.
	WhichAxis = 0;
	if (fabs (t->VecNormal.Y) > fabs (t->VecNormal.X))
	{
		if (fabs (t->VecNormal.Z) > fabs (t->VecNormal.Y))
		{
			WhichAxis = 2;
		}
		else
		{
			WhichAxis = 1;
		}
	}
	else if (fabs (t->VecNormal.Z) > fabs (t->VecNormal.X))
	{
		WhichAxis = 2;
	}

	switch (WhichAxis)
	{
		case 0:
			geVec3d_Set (&uVec, 0.0f, sinv, cosv);
			geVec3d_Set (&vVec, 0.0f, -cosv, sinv);
			break;
		case 1:
			geVec3d_Set (&uVec, cosv, 0.0f, sinv);
			geVec3d_Set (&vVec, -sinv, 0.0f, cosv);			
			break;
		case 2:
			geVec3d_Set (&uVec, cosv, sinv, 0.0f);
			geVec3d_Set (&vVec, sinv, -cosv, 0.0f);
			break;
	}

	//version 1.10 now has a one to one unit to texel ratio
	t->TVecs.uOffset = (geFloat)(t->xShift);
	t->TVecs.vOffset = (geFloat)(t->yShift);

	geVec3d_Scale(&uVec, (1.0f/t->xScale), &t->TVecs.uVec);
	geVec3d_Scale(&vVec, (1.0f/t->yScale), &t->TVecs.vVec);
}


static void Face_UpdateTextureVecs
	(
	  Face *f
	)
{
	if (Face_IsTextureLocked (f))
	{
		Face_UpdateLockedTextureVecs (f);
	}
	else
	{
		Face_UpdateWorldTextureVecs (f);
	}
}

const TexInfo_Vectors	*Face_GetTextureVecs(const Face *f)
{
	assert(f != NULL);

	// if info has been changed then we have to re-calculate the vecs...
	if (f->Tex.DirtyFlag)
	{
		//make sure the texinfos plane and vecs are good
		Face_SetPlaneFromFace((Face *)f);

		// The cast is kinda ugly, but we really want the parameter
		// to this function to be const!
		// mutable would be nice here, huh?
		Face_UpdateTextureVecs ((Face *)f);
		((Face *)f)->Tex.DirtyFlag = GE_FALSE;
	}

	return &(f->Tex.TVecs);
}

int	Face_GetTextureDibId(const Face *f)
{
	assert(f != NULL);

	return	f->Tex.Dib;
}

char const	*Face_GetTextureName(const Face *f)
{
	assert(f != NULL);

	return	f->Tex.Name;
}

geFloat	Face_GetMipMapBias(const Face *f)
{
	assert (f != NULL);

	return f->MipMapBias;
}

geFloat	Face_GetTranslucency(const Face *f)
{
	assert (f != NULL);

	return f->Translucency;
}

geFloat	Face_GetReflectivity(const Face *f)
{
	assert (f != NULL);

	return f->Reflectivity;
}

//check flags
geBoolean	Face_IsSelected(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_SELECTED)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsFixedHull(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_FIXEDHULL)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsLight(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_LIGHT)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsMirror(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_MIRROR)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsFullBright(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_FULLBRIGHT)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsSky(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_SKY)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsGouraud(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_GOURAUD)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsFlat(const Face *f)
{
	assert(f != NULL);

	return	(f->Flags & FACE_FLAT)?	GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsTextureLocked (const Face *f)
{
	return (f->Flags & FACE_TEXTURELOCKED) ? GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsVisible (const Face *f)
{
	return (f->Flags & FACE_VISIBLE) ? GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsSheet (const Face *f)
{
	return (f->Flags & FACE_SHEET) ? GE_TRUE : GE_FALSE;
}

geBoolean	Face_IsTransparent (const Face *f)
{
	return (f->Flags & FACE_TRANSPARENT) ? GE_TRUE : GE_FALSE;
}

void	Face_SetSelected(Face *f, const geBoolean bState)
{
	assert(f != NULL);

	f->Flags	=(bState)? f->Flags|FACE_SELECTED	: f->Flags&~FACE_SELECTED;
}

void	Face_SetFixedHull(Face *f, const geBoolean bState)
{
	assert(f != NULL);

	f->Flags	=(bState)? f->Flags|FACE_FIXEDHULL	: f->Flags&~FACE_FIXEDHULL;
}

void	Face_SetLight(Face *f, const geBoolean bState)
{
	assert(f != NULL);

	f->Flags	=(bState)? f->Flags|FACE_LIGHT	: f->Flags&~FACE_LIGHT;
}

void	Face_SetMirror(Face *f, const geBoolean bState)
{
	assert(f != NULL);

	f->Flags	=(bState)? f->Flags|FACE_MIRROR	: f->Flags&~FACE_MIRROR;
}


void	Face_SetSky(Face *f, const geBoolean bState)
{
	assert(f != NULL);

	f->Flags	=(bState)? f->Flags|FACE_SKY	: f->Flags&~FACE_SKY;
}

// Fullbright, flat, and gouraud are mutually exclusive
static void Face_SetShadingExclusive (Face *f, const int StateFlag)
{
	// clear all face shading flags
	f->Flags &= ~(FACE_FULLBRIGHT | FACE_GOURAUD | FACE_FLAT);

	// set the desired flag
	f->Flags |= StateFlag;
}

void	Face_SetFullBright(Face *f, const geBoolean bState)
{
	assert(f != NULL);

	if (bState)
	{
		Face_SetShadingExclusive (f, FACE_FULLBRIGHT);
	}
	else
	{
		f->Flags &= ~FACE_FULLBRIGHT;
	}
}

void	Face_SetGouraud(Face *f, const geBoolean bState)
{
	assert (f != NULL);

	if (bState)
	{
		Face_SetShadingExclusive (f, FACE_GOURAUD);
	}
	else
	{
		f->Flags &= ~FACE_GOURAUD;
	}
}

void	Face_SetFlat(Face *f, const geBoolean bState)
{
	assert (f != NULL);

	if (bState)
	{
		Face_SetShadingExclusive (f, FACE_FLAT);
	}
	else
	{
		f->Flags &= ~FACE_FLAT;
	}
}

void	Face_SetTextureLock (Face *f, const geBoolean bState)
{
	f->Flags = (bState) ? (f->Flags | FACE_TEXTURELOCKED) : (f->Flags & ~FACE_TEXTURELOCKED);
	Face_InitFaceAngle( &f->Tex, &f->Face_Plane.Normal );
	Face_SetTexturePos (f);
}

void	Face_SetVisible (Face *f, const geBoolean bState)
{
	f->Flags = (bState) ? (f->Flags | FACE_VISIBLE) : (f->Flags & ~FACE_VISIBLE);
}

void	Face_SetSheet (Face *f, const geBoolean bState)
{
	f->Flags = (bState) ? (f->Flags | FACE_SHEET) : (f->Flags & ~FACE_SHEET);
}

void	Face_SetTransparent (Face *f, const geBoolean bState)
{
	f->Flags = (bState) ? (f->Flags | FACE_TRANSPARENT) : (f->Flags & ~FACE_TRANSPARENT);
}

void	Face_SetLightIntensity(Face *f, const int Value)
{
	assert (f != NULL);

	f->LightIntensity = Value;
}

void	Face_SetLightScale(Face *f, const geFloat xScale, const geFloat yScale)
{
	assert (f != NULL);

	f->LightXScale = xScale;
	f->LightYScale = yScale;
}

void	Face_SetTextureScale(Face *f, const geFloat xScale, const geFloat yScale)
{
	assert(f != NULL);

	f->Tex.xScale = xScale;
	f->Tex.yScale = yScale;

	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_SetTextureShift(Face *f, const int xShift, const int yShift)
{
	assert(f != NULL);

	f->Tex.xShift = xShift;
	f->Tex.yShift = yShift;
	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_SetTextureRotate(Face *f, const geFloat Rotate)
{
	assert(f != NULL);


	f->Tex.Rotate = Rotate;
	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_SetTextureDibId(Face *f, const int Dib)
{
	assert(f != NULL);

	f->Tex.Dib = Dib;
}

void	Face_SetTextureName(Face *f, const char *pName)
{
	assert(f != NULL);

	// copy the name (safely), and then nul-terminate
	strncpy (f->Tex.Name, pName, sizeof (f->Tex.Name));
	f->Tex.Name[sizeof (f->Tex.Name)-1] = '\0';
}

void Face_XfmTexture (Face *f, const geXForm3d *pXfm)
{
	assert (f != NULL);
	assert (pXfm != NULL);

	geXForm3d_Multiply (pXfm, &f->Tex.XfmFaceAngle, &f->Tex.XfmFaceAngle);
}

void	Face_SetTextureSize (Face *f, const int txSize, const int tySize)
{
	assert (f != NULL);

	f->Tex.txSize = txSize;
	f->Tex.tySize = tySize;
}

void	Face_SetMipMapBias (Face *f, const geFloat Bias)
{
	assert (f != NULL);

	f->MipMapBias = Bias;
}

void	Face_SetTranslucency (Face *f, const geFloat trans)
{
	assert (f != NULL);

	f->Translucency = trans;
}

void	Face_SetReflectivity (Face *f, const geFloat rscale)
{
	assert (f != NULL);

	f->Reflectivity = rscale;
}


void	Face_CopyTexInfo(const Face *src, Face *dst)
{
	assert(src);
	assert(dst);

	dst->Tex	=src->Tex;

	//make sure the texinfos plane and vecs are good
	Face_SetPlaneFromFace(dst);
}

//copies texinfo, flags, light, value
void	Face_CopyFaceInfo(const Face *src, Face *dst)
{
	assert(src);
	assert(dst);

	dst->Flags			=src->Flags;
	dst->LightIntensity	=src->LightIntensity;
	dst->MipMapBias		=src->MipMapBias;
	dst->Translucency	=src->Translucency;
	dst->Reflectivity	=src->Reflectivity;
	dst->Tex			=src->Tex;
	dst->LightXScale	=src->LightXScale;
	dst->LightYScale	=src->LightYScale;

	//make sure the texinfos plane and vecs are good
	Face_SetPlaneFromFace(dst);
}

void	Face_Rotate(Face *f, const geXForm3d *pXfmRotate, const geVec3d *pCenter)
{
	int	i;
	geVec3d	*pPoint;

	assert(f != NULL);
	assert(pXfmRotate != NULL);
	assert(pCenter != NULL);

	for(i=0;i < f->NumPoints;i++)
	{

		pPoint	=&f->Points[i];
		geVec3d_Subtract(pPoint, pCenter, pPoint);
		geXForm3d_Rotate(pXfmRotate, pPoint, pPoint);
		geVec3d_Add(pPoint, pCenter, pPoint);
	}
	Face_SetPlaneFromFace(f);
	Face_XfmTexture (f, pXfmRotate);
	pPoint	=&f->Tex.Pos;
	geVec3d_Subtract(pPoint, pCenter, pPoint);
	geXForm3d_Rotate(pXfmRotate, pPoint, pPoint);
	geVec3d_Add(pPoint, pCenter, pPoint);
	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_Move(Face *f, const geVec3d *trans)
{
	int i;

	assert(f);
	assert(trans);

	for(i=0;i < f->NumPoints;i++)
	{
		geVec3d_Add(&f->Points[i], trans, &f->Points[i]);
	}
	Face_SetPlaneFromFace(f);

	// Update position...
	geVec3d_Add (&f->Tex.Pos, trans, &f->Tex.Pos);
	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_Transform(Face *f, const geXForm3d *pXfm)
{
	int	i;

	assert(f != NULL);
	assert(pXfm != NULL);

	for(i=0;i < f->NumPoints;i++)
	{
		geXForm3d_Transform(pXfm, &f->Points[i], &f->Points[i]);
	}
	Face_SetPlaneFromFace(f);
	Face_XfmTexture (f, pXfm);
	f->Tex.DirtyFlag = GE_TRUE;
}

static void Face_UpdateFaceAngle (Face *f, const geVec3d *OldNormal)
/*
  Compute rotation matrix from OldNormal to current normal (f->Tex.VecNormal),
  and include that rotation in the XfmFaceAngle transform.
*/
{
	geVec3d VecDest;
	geVec3d VecAxis;
	geFloat cosv, Theta;
	geXForm3d Xfm;

	// Compute rotation from 
	VecDest = f->Tex.VecNormal;
	geVec3d_CrossProduct (&VecDest, OldNormal, &VecAxis);
	cosv = geVec3d_DotProduct (&VecDest, OldNormal);
	// Now here's an interesting twist.
	// Due to floating point inaccuracies, the dot product of two supposedly
	// normal vectors may result in a number larger than 1.0.
	// If that happens, it's supposed to be 1.0, so we'll force it.
	if (cosv > 1.0f) 
	{
		cosv = 1.0f;
	}
	Theta = (geFloat)acos (cosv);
	if (geVec3d_Normalize (&VecAxis) == 0.0f)
	{
		// If the resulting vector is 0 length, 
		// then a rotation about X will put us where we need to be.
		geXForm3d_SetIdentity (&Xfm);
		geXForm3d_RotateX (&Xfm, -Theta);
	}
	else
	{
		geQuaternion QRot;

		geQuaternion_SetFromAxisAngle (&QRot, &VecAxis, -Theta);
		geQuaternion_ToMatrix (&QRot, &Xfm);
	}
	// and include it in the face's rotation...
	Face_XfmTexture (f, &Xfm);
}

void	Face_Scale(Face *f, const geVec3d *ScaleVec)
{
	int i;

	assert(f);
	assert(ScaleVec);

	for(i=0;i < f->NumPoints;i++)
	{
		//no magnitude operation in vec3d
		f->Points[i].X	*=ScaleVec->X;
		f->Points[i].Y	*=ScaleVec->Y;
		f->Points[i].Z	*=ScaleVec->Z;
	}
	{
		geVec3d OldNormal = f->Tex.VecNormal;
		Face_SetPlaneFromFace(f);

		Face_UpdateFaceAngle (f, &OldNormal);
	}
	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_Shear(Face *f, const geVec3d *ShearVec, const geVec3d *ShearAxis)
{
	int		i;

	assert(f);
	assert(ShearVec);
	assert(ShearAxis);

	for(i=0;i < f->NumPoints;i++)
	{
		geFloat	dot;

		dot	=geVec3d_DotProduct(&f->Points[i], ShearVec);
		geVec3d_MA(&f->Points[i], dot, ShearAxis, &f->Points[i]);
	}
	{
		geVec3d OldNormal = f->Tex.VecNormal;
		Face_SetPlaneFromFace(f);
		Face_UpdateFaceAngle (f, &OldNormal);
	}
	f->Tex.DirtyFlag = GE_TRUE;
}

void	Face_GetBounds(const Face *f, Box3d *b)
{
	int i;

	assert(f != NULL);
	assert(b != NULL);

	Box3d_SetBogusBounds(b);
	for(i=0;i < f->NumPoints;i++)
	{
		Box3d_AddPoint(b, f->Points[i].X, f->Points[i].Y, f->Points[i].Z);
	}
}

void	Face_GetCenter(const Face *f, geVec3d *pCenter)
{
	Box3d Box;

	assert(f != NULL);
	assert(pCenter != NULL);

	Face_GetBounds(f, &Box);
	Box3d_GetCenter(&Box, pCenter);
}

void	Face_WriteToMap(const Face *f, FILE *wf)
{
	char	szTemp[_MAX_PATH];
	
	assert(f);
	assert(wf);

	{
		int OutputFlags;

		OutputFlags = 0;
		if (f->Flags & FACE_MIRROR)		OutputFlags	|= ffMirror;
		if (f->Flags & FACE_FULLBRIGHT) OutputFlags	|= ffFullBright;
		if (f->Flags & FACE_SKY)		OutputFlags	|= ffSky;
		if (f->Flags & FACE_LIGHT)		OutputFlags	|= ffLight;
		if (f->Flags & FACE_GOURAUD)	OutputFlags	|= ffGouraud;
		if (f->Flags & FACE_FLAT)		OutputFlags	|= ffFlat;
		if (f->Flags & FACE_TRANSPARENT) OutputFlags |= ffTranslucent;
		TypeIO_WriteInt(wf, OutputFlags);
	}

	TypeIO_WriteFloat (wf, f->MipMapBias);
	TypeIO_WriteFloat (wf, f->Translucency);
	TypeIO_WriteFloat (wf, (geFloat)f->LightIntensity);	//engine expects float
	TypeIO_WriteFloat (wf, f->Reflectivity);

	strcpy(szTemp, Face_GetTextureName (f));
	TypeIO_WriteBlock(wf, szTemp, 32);
	{
		#pragma message ("New texture vector output!")
		const TexInfo_Vectors *TVecs = Face_GetTextureVecs (f);
		geVec3d uVec, vVec;
		const geFloat xScale = f->Tex.xScale/f->LightXScale;
		const geFloat yScale = f->Tex.yScale/f->LightYScale;

		/*
		  The texture vectors returned have the entire scale value
		  included.  We need to back out the scale and use the
		  LightScale instead.  The Scale values that we send to
		  the tools will be Scale/LightScale.
		*/
		// Back out original scale and scale by LightScale
		geVec3d_Scale (&TVecs->uVec, xScale, &uVec);
		geVec3d_Scale (&TVecs->vVec, yScale, &vVec);

		// u vector, scale, offset
		TypeIO_WriteVec3d (wf, &uVec);
		TypeIO_WriteFloat (wf, xScale);
		TypeIO_WriteFloat (wf, TVecs->uOffset);

		// v vector, scale, offset
		TypeIO_WriteVec3d (wf, &vVec);
		TypeIO_WriteFloat (wf, yScale);
		TypeIO_WriteFloat (wf, TVecs->vOffset);
	}

	TypeIO_WriteBlock (wf, &f->Face_Plane, sizeof (Plane));
}

void	Face_WriteToQuakeMap(const Face *f, FILE *wf)
{
	int		xShift, yShift;
	geFloat	xScale, yScale, Rotate;
	
	assert(f);
	assert(wf);

	fprintf(wf, "( %f %f %f ) ",
		(f->Points[0].X),
		-(f->Points[0].Z),
		(f->Points[0].Y));
	fprintf(wf, "( %f %f %f ) ",
		(f->Points[1].X),
		-(f->Points[1].Z),
		(f->Points[1].Y));
	fprintf(wf, "( %f %f %f ) ",
		(f->Points[2].X),
		-(f->Points[2].Z),
		(f->Points[2].Y));

	Face_GetTextureShift (f, &xShift, &yShift);
	Face_GetTextureScale (f, &xScale, &yScale);
	Rotate	= Face_GetTextureRotate (f);

	fprintf(wf, Face_GetTextureName (f));
	fprintf(wf, " %d %d %d %f %f\n", Rotate, xShift, yShift, xScale, yScale);
}

geBoolean Face_Write(const Face *f, FILE *wf)
{
	int		i, xShift, yShift, Rotate;
	geFloat xScale, yScale, rot;

	assert(f);
	assert(wf);

	if (fprintf(wf, "\t\tNumPoints %d\n", f->NumPoints) < 0) return GE_FALSE;
	if (fprintf(wf, "\t\tFlags %d\n", f->Flags) < 0) return GE_FALSE;
	if (fprintf(wf, "\t\tLight %d\n", f->LightIntensity) < 0) return GE_FALSE;
	if (fprintf(wf, "\t\tMipMapBias %f\n", f->MipMapBias) < 0) return GE_FALSE;
	if (fprintf(wf, "\t\tTranslucency %f\n", f->Translucency) < 0) return GE_FALSE;
	if (fprintf(wf, "\t\tReflectivity %f\n", f->Reflectivity) < 0) return GE_FALSE;

	for(i=0;i < f->NumPoints;i++)
	{
		if (fprintf(wf, "\t\t\tVec3d %f %f %f\n", f->Points[i].X, f->Points[i].Y, f->Points[i].Z) < 0) return GE_FALSE;
	}

	Face_GetTextureShift (f, &xShift, &yShift);
	Face_GetTextureScale (f, &xScale, &yScale);
	rot		=Face_GetTextureRotate (f);
	Rotate	=Units_Round(rot);

	{
		char QuotedValue[SCANNER_MAXDATA];

		// Quote the texture name
		Util_QuoteString (Face_GetTextureName (f), QuotedValue);
		if (fprintf(wf, "\t\t\tTexInfo Rotate %d Shift %d %d Scale %f %f Name %s\n",
			Rotate, xShift, yShift, xScale, yScale, QuotedValue) < 0) return GE_FALSE;
	}

	if (fprintf(wf, "\t\tLightScale %f %f\n", f->LightXScale, f->LightYScale) < 0) return GE_FALSE;

	if (fprintf (wf, "%s", "\tTransform\t") < 0) return GE_FALSE;	
	if (!TypeIO_WriteXForm3dText (wf, &(f->Tex.XfmFaceAngle))) return GE_FALSE;

	if (fprintf (wf, "%s", "\tPos\t") < 0) return GE_FALSE;	
	if( !TypeIO_WriteVec3dText(wf, &f->Tex.Pos )) return GE_FALSE;

	return GE_TRUE;
}

Face	*Face_CreateFromFile
	(
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	Face	*f = NULL;
	int		i, flg, NumPnts, xShift, yShift, tempint, Light;
	geFloat MipMapBias, Reflectivity, Translucency;
	geFloat	fVal, xScale, yScale;
	geVec3d	*tmpPnts = NULL;
	geBoolean LoadResult;
	char	szTemp[_MAX_PATH];

	assert(Parser != NULL);

	LoadResult = GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "NumPoints"), &NumPnts)) goto DoneLoad;

	// read face flags and value...
	if((VersionMajor == 1) && (VersionMinor < 8))
	{
		fVal=0.0f;
		flg	=0;
	}
	else
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "Flags"), &flg)) goto DoneLoad;
		if ((VersionMajor == 1) && (VersionMinor < 25))
		{
			if (!Parse3dt_GetFloat (Parser, (*Expected = "FaceValue"), &fVal)) goto DoneLoad;
			// FaceValue is ignored
			fVal	=0.0f;
		}
	}
	//xlate old flags
	if ((VersionMajor == 1) && (VersionMinor < 13))
	{
		int	flg2	=0;
		if(flg	&	oldLIGHT)		flg2	|=	FACE_LIGHT;
		if(flg	&	oldMIRROR)		flg2	|=	FACE_MIRROR;
		if(flg	&	oldFULLBRIGHT)	flg2	|=	FACE_FULLBRIGHT;
		if(flg	&	oldSKY)			flg2	|=	FACE_SKY;
		flg	=flg2;
	}

	// clear previously unused flag values
	if ((VersionMajor == 1) && (VersionMinor <= 10))
	{
		flg  &= ~(FACE_MIRROR | FACE_FULLBRIGHT | FACE_SKY);
	}
	if ((VersionMajor == 1) && (VersionMinor < 25))
	{
		// clear previously unused shading values
		flg &= ~(FACE_GOURAUD | FACE_FLAT);
	}
	if ((VersionMajor == 1) && (VersionMinor < 30))
	{
		flg &= ~FACE_TRANSPARENT;
	}

	// don't allow selected faces on load
	flg &= ~FACE_SELECTED;

	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 13)))
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "Light"), &Light)) goto DoneLoad;
	}
	else
	{
		Light = FACE_DEFAULT_LIGHT;
	}

	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 25)))
	{
		if (!Parse3dt_GetFloat (Parser, (*Expected = "MipMapBias"), &MipMapBias)) goto DoneLoad;
	}
	else
	{
		MipMapBias = FACE_DEFAULT_BIAS;
	}
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 27)))
	{
		if (!Parse3dt_GetFloat (Parser, (*Expected = "Translucency"), &Translucency)) goto DoneLoad;
		if (!Parse3dt_GetFloat (Parser, (*Expected = "Reflectivity"), &Reflectivity)) goto DoneLoad;
	}
	else
	{
		Translucency = FACE_DEFAULT_TRANSLUCENCY;
		Reflectivity = FACE_DEFAULT_REFLECTIVITY;
	}

	tmpPnts	=geRam_Allocate(sizeof(geVec3d) * NumPnts);
	if(tmpPnts)
	{
		geFloat	LightXScale = 1.0f;
		geFloat LightYScale = 1.0f;
		if((VersionMajor == 1) && (VersionMinor < 4))
		{
			/*
			  From version 1.3 to 1.4 we changed back to a right-handed coordinate system.
			  So, to load a 1.3 or earlier file, we need to reverse the windings
			  and flip the Z on every point.
			*/
			for(i=NumPnts-1;i >= 0;i--)
			{
				if (!Parse3dt_GetVec3d (Parser, (*Expected = "Vec3d"), &tmpPnts[i])) goto DoneLoad;

				//flip z
				tmpPnts[i].Z	=-tmpPnts[i].Z;
			}
		}
		else
		{
			for(i=0;i < NumPnts;i++)
			{
				if (!Parse3dt_GetVec3d (Parser, (*Expected = "Vec3d"), &tmpPnts[i])) goto DoneLoad;
			}
		}
		f	=Face_Create(NumPnts, tmpPnts, 0);
		geRam_Free (tmpPnts);
		tmpPnts = NULL;

		if(f)
		{
			f->Flags	=flg;
			f->LightIntensity = Light;
			f->MipMapBias = MipMapBias;
			f->Reflectivity = Reflectivity;
			f->Translucency = Translucency;
		}
		if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "TexInfo"))) goto DoneLoad;
		if (!Parse3dt_GetInt (Parser, (*Expected = "Rotate"), &tempint)) goto DoneLoad;
		if (!Parse3dt_GetInt (Parser, (*Expected = "Shift"), &xShift)) goto DoneLoad;
		if (!Parse3dt_GetInt (Parser, NULL, &yShift)) goto DoneLoad;
		if (!Parse3dt_GetFloat (Parser, (*Expected = "Scale"), &xScale)) goto DoneLoad;
		if (!Parse3dt_GetFloat (Parser, NULL, &yScale)) goto DoneLoad;
		if((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 24)))
		{
			// Version 1.24 and later we quote the texture names
			if (!Parse3dt_GetLiteral (Parser, (*Expected = "Name"), szTemp)) goto DoneLoad;
		}
		else
		{	
			if (!Parse3dt_GetIdentifier (Parser, (*Expected = "Name"), szTemp)) goto DoneLoad;
		}

		if (!((VersionMajor == 1) && (VersionMinor <= 16)))
		{
			if (!Parse3dt_GetFloat (Parser, (*Expected = "LightScale"), &LightXScale)) goto DoneLoad;
			if (!Parse3dt_GetFloat (Parser, NULL, &LightYScale)) goto DoneLoad;
		}
		if(f)
		{
			Face_InitTexInfo(&f->Tex, &f->Face_Plane.Normal);

			Face_SetTextureName (f, szTemp);
			Face_SetTextureRotate (f, (geFloat)tempint);
			Face_SetTextureShift (f, xShift, yShift);
			Face_SetTextureScale (f, xScale, yScale);
			Face_SetTexturePos (f);

			if ((VersionMajor == 1) && (VersionMinor <= 16))
			{
				f->LightXScale = xScale;
				f->LightYScale = yScale;
			}
			else
			{
				f->LightXScale = LightXScale;
				f->LightYScale = LightYScale;
			}
			if((VersionMajor == 1) && (VersionMinor < 29))
			{
				Face_SetVisible(f, GE_TRUE);
			}
			if (((VersionMajor == 1) && (VersionMinor > 31)))
			{
				if (!Parse3dt_GetXForm3d (Parser, (*Expected = "Transform"), &f->Tex.XfmFaceAngle)) goto DoneLoad;
				if (!Parse3dt_GetVec3d (Parser, (*Expected = "Pos"), &f->Tex.Pos)) goto DoneLoad;
			}
		}
		LoadResult = GE_TRUE;
	}
DoneLoad:
	if (LoadResult == GE_FALSE)
	{
		if (f != NULL)
		{
			Face_Destroy (&f);
		}
		if (tmpPnts != NULL)
		{
			geRam_Free (tmpPnts);
		}
	}
	return f;
}

void	Face_GetSplitInfo(const Face *f, const Plane *p, geFloat *dists, uint8 *sides, uint8 *cnt)
{
	int	i;

	assert(f);
	assert(dists);
	assert(sides);
	assert(cnt);

	cnt[0]=cnt[1]=cnt[2]=0;

	for(i=0;i < f->NumPoints;i++)
	{
		dists[i]=geVec3d_DotProduct(&f->Points[i], &p->Normal)-p->Dist;
		if(dists[i] > ON_EPSILON)
		{
			sides[i]=SIDE_FRONT;
		}
		else if(dists[i] < -ON_EPSILON)
		{
			sides[i]=SIDE_BACK;
		}
		else
		{
			sides[i]=SIDE_ON;
		}
		cnt[sides[i]]++;
	}
	sides[i]=sides[0];
	dists[i]=dists[0];
}

void	Face_Split(const Face	*f,		//original face
				   const Plane	*p,		//split plane
				   Face			**ff,	//front face (null)
				   Face			**bf,	//back face (null)
				   geFloat		*dists,	//plane dists per vert
				   uint8		*sides)	//plane sides per vert
{
	geVec3d	*p1, *p2, mid;
	int		nfp, nbp, i, j;
	geFloat	dot;

	assert(f);
	assert(p);
	assert(ff);
	assert(bf);
	assert(*ff==NULL);
	assert(*bf==NULL);
	assert(dists);
	assert(sides);

	p1	=f->Points;
	for(i=nfp=nbp=0;i < f->NumPoints;i++, p1++)
	{
		if(sides[i]==SIDE_ON)
		{
			geVec3d_Copy(p1, &spf[nfp]);
			geVec3d_Copy(p1, &spb[nbp]);
			nfp++;	nbp++;	//Dont ++ in params!
			continue;
		}
		if(sides[i]==SIDE_FRONT)
		{
			geVec3d_Copy(p1, &spf[nfp]);
			nfp++;
		}
		if(sides[i]==SIDE_BACK)
		{
			geVec3d_Copy(p1, &spb[nbp]);
			nbp++;
		}
		if(sides[i+1]==SIDE_ON || sides[i+1]==sides[i])
			continue;

		p2	=&f->Points[(i+1) % f->NumPoints];
		dot	=dists[i] / (dists[i]-dists[i+1]);
		for(j=0;j<3;j++)
		{
			if(VectorToSUB(p->Normal, j)==1)
			{
				VectorToSUB(mid, j)	=p->Dist;
			}
			else if(VectorToSUB(p->Normal, j)==-1)
			{
				VectorToSUB(mid, j)	=-p->Dist;
			}
			else
			{
				VectorToSUB(mid, j)	=VectorToSUB(*p1, j)+
					dot*(VectorToSUB(*p2, j)	-VectorToSUB(*p1, j));
			}
		}

		//split goes to both sides
		geVec3d_Copy(&mid, &spf[nfp]);
		nfp++;
		geVec3d_Copy(&mid, &spb[nbp]);
		nbp++;
	}
	*ff	=Face_Create(nfp, spf, 0);
	*bf	=Face_Create(nbp, spb, 0);

	if(*ff)
	{
		Face_CopyFaceInfo(f, *ff);
	}
	if(*bf)
	{
		Face_CopyFaceInfo(f, *bf);
	}
}

void	Face_Clip(Face *f, const Plane *p, geFloat *dists, uint8 *sides)
{
	geVec3d	*p1, *p2, mid;
	int		nfp, nbp, i, j;
	geFloat	dot;

	assert(f);
	assert(p);
	assert(dists);
	assert(sides);

	p1	=f->Points;
	for(i=nfp=nbp=0;i < f->NumPoints;i++, p1++)
	{
		if(sides[i]==SIDE_ON)
		{
			geVec3d_Copy(p1, &spb[nbp]);
			nbp++;
			continue;
		}
		if(sides[i]==SIDE_BACK)
		{
			geVec3d_Copy(p1, &spb[nbp]);
			nbp++;
		}
		if(sides[i+1]==SIDE_ON || sides[i+1]==sides[i])
			continue;

		p2	=&f->Points[(i+1) % f->NumPoints];
		dot	=dists[i] / (dists[i]-dists[i+1]);
		for(j=0;j<3;j++)
		{
			if(VectorToSUB(p->Normal, j)==1)
			{
				VectorToSUB(mid, j)	=p->Dist;
			}
			else if(VectorToSUB(p->Normal, j)==-1)
			{
				VectorToSUB(mid, j)	=-p->Dist;
			}
			else
			{
				VectorToSUB(mid, j)	=VectorToSUB(*p1, j)+
					dot*(VectorToSUB(*p2, j)	-VectorToSUB(*p1, j));
			}
		}
		geVec3d_Copy(&mid, &spb[nbp]);
		nbp++;
	}
	geRam_Free (f->Points);
	f->NumPoints	=nbp;
	f->Points		=(geVec3d *) geRam_Allocate(sizeof(geVec3d)*nbp);
	memcpy(f->Points, spb, sizeof(geVec3d)*nbp);
}

geFloat	Face_PlaneDistance(const Face *f, geVec3d *pos)
{
	assert(f);
	assert(pos);

	return	(geVec3d_DotProduct(&f->Face_Plane.Normal, pos)
				-f->Face_Plane.Dist);
}

void	Face_MostlyOnSide(const Face *f, const Plane *p, geFloat *max, int *side)
{
	int		i;
	geFloat	d;

	for(i=0;i < f->NumPoints;i++)
	{
		d	=geVec3d_DotProduct(&f->Points[i], &p->Normal) - p->Dist;
		if(d > *max)
		{
			*max	=d;
			*side	=SIDE_FRONT;
		}
		if(-d > *max)
		{
			*max	=-d;
			*side	=SIDE_BACK;
		}
	}
}
