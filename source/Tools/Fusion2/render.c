/****************************************************************************************/
/*  render.c                                                                            */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, Eli Boling                        */
/*  Description:  Tons of render stuff                                                  */
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

/*
Code fragments from Chris Hecker's texture mapping articles used with
permission.  http://www.d6.com/users/checker 
*/

#include "render.h"
#include "node.h"
#include "basetype.h"
#include "units.h"
#include "consoletab.h"
#include "ram.h"
#include "bitmap.h"

#include <math.h>
#include <assert.h>
#include <float.h>


#pragma warning (disable:4725)  // FDIV warning...whoopee!!

static void	Render_LineZBuffer(int xa, int ya, geFloat za,
							   int xb, int yb, geFloat zb,
							   uint16 color, ViewVars *Cam);

static void	Render_Line(int xa, int ya,
						int xb, int yb,
						uint16 color, ViewVars *Cam);

static const geVec3d	VecOrigin	={ 0.0f, 0.0f, 0.0f };
#define MAX_SPANS           100000
#define MAX_SURFS           100000
#define MAX_EDGES           100000
#define	ON_EPSILON			((geFloat)0.02)
//need to put this somewhere
#define	VectorToSUB(a, b)			(*((((geFloat *)(&a))) + (b)))

//screenspace deltas needed by the asm raster code
typedef struct GradientsTag
{
	geFloat dOneOverZdX, dOneOverZdX16;	// d(1/z)/dX
	geFloat dUOverZdX, dUOverZdX16;		// d(u/z)/dX
	geFloat dVOverZdX, dVOverZdX16;		// d(v/z)/dX
} Gradients;

typedef struct EdgeAsmTag
{
	long		X, XStep, Y;	// DDA info
	geFloat	OneOverZ, UOverZ, VOverZ;
} EdgeAsm;

typedef struct SpanTag
{
	int			x, y, count, color, RFlag;
	geFloat	zinvLeft, zinvuLeft, zinvvLeft;
	geFloat	zinvRight, zinvuRight, zinvvRight;
	struct		SpanTag	*next;
} Span;

typedef struct SurfTag
{
	struct	SurfTag	*pnext, *pprev;
	int		color, visxstart;
	int		state, Key, sfIdx, RFlag;
} Surf;

typedef struct EdgeTag
{
	int		x, xstep, leading, Key;
    Surf	*psurf;
    struct	EdgeTag	*pnext, *pprev, *pnextremove;
} Edge;

//For clipping and drawing
typedef struct RenderFaceTag
{
	int32		NumPoints;
	geVec3d		Points[32];
} RenderFace;

typedef struct ViewVarsTag
{
	struct
	{
		BITMAPINFOHEADER	bmiHeader;
		RGBQUAD				bmiColors[256];
	} ViewBMI;

	HBITMAP		hDibSec;
	uint32		Flags;
	uint8		*pBits;
	uint32		*pZBuffer;
	uint32		ViewType;
	geFloat	ZoomFactor;//, GridInterval;

	geVec3d		Vpn, Vright, Vup, CamPos;
	geFloat	roll, pitch, yaw;
	Plane		FrustPlanes[4];
	geFloat	MaxScreenScaleInv, FieldOfView;
	geFloat	XCenter, YCenter, MaxScale;
	geFloat	SpeedScale, YScreenScale, XScreenScale;
	long		Width, Height;
	SizeInfo	*WadSizes;
	Edge		*NewEdges, **RemoveEdges;
	long		FacesDone;
} ViewVars;

geFloat Render_ComputeGridDist (const ViewVars *v, int GridType)
{
	geVec3d left, right;
	float dist;

	// determine grid size for minimum 10 pixels between grid lines
	Render_ViewToWorld (v, 0, 0, &left);
	Render_ViewToWorld (v, 10, 0, &right);
	switch (v->ViewType)
	{
		case VIEWTOP :
			dist = right.X - left.X;
			break;
		case VIEWFRONT :
			dist = right.X - left.X;
			break;
		case VIEWSIDE :
			dist = right.Z - left.Z;
			break;
		default :
			dist = 0.0f;
#if 1
			/*
			  This function should be called only by ortho views.
			  Currently, all views call it so this assertion will cause a problem.
			  Need to fix this...
			*/
			#pragma message ("This function should not be called by non-ortho views.")
#else
			assert (0);
#endif			
			break;
	}

	dist = (float)fabs (dist);
	if (dist < 1.0f)
		dist = 1.0f;
	if (GridType == GRID_TYPE_METRIC)
	{
		dist *= 2.54f;
	}
	return dist;
}

/*static geFloat log2 (geFloat f)
{
	return (geFloat)(log (f)/log (2.0f));
}*/

geFloat	Render_GetFineGrid(const ViewVars *v, int GridType)
{
	float dist;
	double Interval;

	assert(v);

	dist = Render_ComputeGridDist (v, GridType);

	switch (GridType)
	{
		case GRID_TYPE_METRIC :
			Interval = pow (10, (int)(log10 (dist)));
			break;
		case GRID_TYPE_TEXEL :
			Interval = pow (2, (int)(log2 (dist)));
			break;
		default :
			assert (0);
			Interval = 1.0f;
			break;
	}
	return (geFloat)Interval;
}

geFloat	Render_GetZoom(const ViewVars *v)
{
	assert(v);

	return	v->ZoomFactor;
}

geFloat	Render_GetXScreenScale(const ViewVars *v)
{
	assert(v);

	return	v->XScreenScale;
}

int	Render_GetInidx(const ViewVars *v)
{
	assert(v);

	return	(v->ViewType>>3)&0x3;
}

int	Render_GetViewType(const ViewVars *v)
{
	assert(v);

	return	v->ViewType;
}

int	Render_GetWidth(const ViewVars *v)
{
	assert(v);

	return	v->Width;
}

int	Render_GetHeight(const ViewVars *v)
{
	assert(v);

	return	v->Height;
}

void	Render_GetCameraPos(const ViewVars *v, geVec3d *pos)
{
	assert(v);
	assert(pos);

	geVec3d_Copy(&v->CamPos, pos);
}

void	Render_GetPitchRollYaw( const ViewVars * v, geVec3d * pPRY )
{
	assert( v ) ;
	assert( pPRY ) ;

	pPRY->X = v->pitch ;
	pPRY->Y = v->yaw ;
	pPRY->Z = v->roll ;
}


geBoolean Render_UpIsDown (const ViewVars *v)
{
	return ((v->pitch < M_PI/2.0f) || (v->pitch > 3.0f*M_PI/2.0f));
}

SizeInfo	*Render_GetWadSizes(const ViewVars *v)
{
	assert(v);

	return	v->WadSizes;
}

void	Render_SetWadSizes(ViewVars *v, SizeInfo *ws)
{
	assert(v);

	v->WadSizes	=ws;
}

void	Render_SetZoom(ViewVars *v, const geFloat zf)
{
	assert(v);

	v->ZoomFactor	=zf;
	// compute grid interval here...
	// we'll have to know if we're metric or texel.
}

void	Render_SetViewType(ViewVars *v, const int vt)
{
	assert(v);

	v->ViewType	=vt;
}

static geFloat Render_NormalizeAngle (float Rads)
{
	geFloat NewAngle;
	
	// first make it in the range -2PI..2PI
	NewAngle = (float)fmod (Rads, 2*M_PI);

	// and then convert to 0..2PI
	if (NewAngle < 0.0f)
	{
		NewAngle += 2*M_PI;
	}
	return NewAngle;	
}

void	Render_SetPitchRollYaw( ViewVars * v, const geVec3d * pPRY )
{
	assert( v ) ;
	assert( pPRY ) ;

	v->pitch	= Render_NormalizeAngle (pPRY->X) ;
	v->yaw		= Render_NormalizeAngle (pPRY->Y) ;
	v->roll		= Render_NormalizeAngle (pPRY->Z) ;
}



ViewVars	*Render_AllocViewVars(void)
{
	ViewVars	*v;

	v	=(ViewVars *) geRam_Allocate(sizeof(ViewVars));
	
	if(!v)
		ConPrintf("WARNING:  Allocation failure in Render_AllocViewVars()\n");

	memset(v, 0, sizeof(ViewVars));

	return	v;
}

void	Render_FreeViewVars(ViewVars *v)
{
	assert(v);

	if(v->NewEdges)
	{
		geRam_Free (v->NewEdges);
		v->NewEdges = NULL;
	}

	if(v->RemoveEdges)
	{
		geRam_Free (v->RemoveEdges);
		v->NewEdges = NULL;
	}

	if(v->Flags & DIBDONE)
	{
		DeleteObject(v->hDibSec);
		v->hDibSec = NULL;
	}
	if (v->pZBuffer != NULL)
	{
		geRam_Free (v->pZBuffer);
		v->pZBuffer = NULL;
	}
}

void	Render_SetCameraPos(ViewVars *v, const geVec3d *pos)
{
	assert(v);

	geVec3d_Copy(pos, &v->CamPos);
}

void	Render_ZoomChange(ViewVars *v, const geFloat factor)
{
	geFloat NewZoom;
	geFloat dist;

	assert(v);

	NewZoom = v->ZoomFactor * (1.0f + factor);

	dist = NewZoom * (geFloat)v->Width;
	if (((dist < 1.0f) && (NewZoom < v->ZoomFactor)) ||
		((dist > 100000.0f) && (NewZoom > v->ZoomFactor)))
	{
		// either way too small or way too big,
		// and trying to make it worse
		MessageBeep ((UINT)-1);
	}
	else
	{
		v->ZoomFactor = NewZoom;
	}
}

void	Render_MoveCamPos(ViewVars *v, const geVec3d *dv)
{
	assert(v);
	assert(dv);

	geVec3d_AddScaled (&v->CamPos, &v->Vright, dv->X, &v->CamPos);
	geVec3d_AddScaled (&v->CamPos, &v->Vup, dv->Y, &v->CamPos);
	geVec3d_AddScaled (&v->CamPos, &v->Vpn, dv->Z, &v->CamPos);
}

void	Render_MoveCamPosOrtho(ViewVars *v, const geVec3d *dv)
{
	assert(v);
	assert(dv);

	geVec3d_Add(&v->CamPos, dv, &v->CamPos);
}

void	Render_IncrementYaw(ViewVars *v, const geFloat YawIncr)
{
	assert(v);

	//this would be a nice place to put a range validation
	v->yaw	= Render_NormalizeAngle (v->yaw + (v->MaxScreenScaleInv * YawIncr));
}

void	Render_IncrementPitch(ViewVars *v, const geFloat PitchIncr)
{
	assert(v);

	//this would be a nice place to put a range validation
	v->pitch = Render_NormalizeAngle (v->pitch + (v->MaxScreenScaleInv * PitchIncr));
}

void	Render_IncrementRoll(ViewVars *v, const geFloat RollIncr)
{
	assert(v);

	//this would be a nice place to put a range validation
	v->roll	= Render_NormalizeAngle (v->roll + (v->MaxScreenScaleInv * RollIncr));
}

typedef struct SpanSurfaceTag
{
	Gradients	Grads;
	geFloat	zinv00, zinvu00, zinvv00;
	geFloat	zinvstepy, zinvustepy, zinvvstepy;
	Span		*head, *cur;
	SizeInfo	sizes;
} SpanSurface;

//global things
static	Edge		MaxEdge		={0x6FFFFFFF};
static	Edge		edgeHead, edgeTail;
static	Surf		SurfStack, *pAvailSurf;
static	Edge		*pAvailEdge;
static	SpanSurface	SpanFaces[MAX_SURFS];
static	Span		spanz[MAX_SPANS];
static	Edge		edgez[MAX_EDGES];
static	Surf		surfz[MAX_SURFS];
static	geFloat const Magic	=12582912.0f;

//These are built and tossed... I use vecs for everything
static	geFloat		mRoll[3][3], mPitch[3][3], mYaw[3][3];
static	geVec3d		XAxis={ 1.0, 0.0, 0.0 };
static	geVec3d		YAxis={ 0.0, 1.0, 0.0 };
static	geVec3d		ZAxis={ 0.0, 0.0, 1.0 };

//statics used mainly in asm
static geFloat	FixedScale	=65536.0f;
static geFloat	FixedScale16=4096.0f;		//2^16 / 16
//static geFloat	FixedScale24=16777216.0f;	//2^24
static geFloat	FixedScale28=268435456.0f;	//2^28
static geFloat	One			=1.0f;
static geFloat	geFloatTemp;

static uint32		NumASpans, RemainingCount;
static uint32		DeltaU, DeltaV, DeltaW;
static uint32		UFixed, VFixed, WLeft, WRight;
static uint8		*pTex;
static uint32		*pZBuf;						//32 bit zbuffer

//zcan jump pointers
static unsigned long SCanZ[2];
static unsigned long SCan[2];

//zcan storage area... must be 32 in length
static unsigned long can0[32];


#define SV_ACCELERATE		10.0f
#define SV_FRICTION			8.0f
#define CL_FORWARDSPEED		200.0f
#define CL_SIDESPEED		320.0f
#define CLIP_PLANE_EPSILON	0.001f

static void	DrawScanLine128(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine256(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine64(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine32(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine16(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine128_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine256_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine64_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine32_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine16_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine128_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine256_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine64_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine32_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

static void	DrawScanLine16_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight);

void Render_BackRotateVector(ViewVars *Cam, geVec3d *pin, geVec3d *pout)
{
	pout->X=(pin->X*Cam->Vright.X)+(pin->Y*Cam->Vup.X)+(pin->Z*Cam->Vpn.X);
	pout->Y=(pin->X*Cam->Vright.Y)+(pin->Y*Cam->Vup.Y)+(pin->Z*Cam->Vpn.Y);
	pout->Z=(pin->X*Cam->Vright.Z)+(pin->Y*Cam->Vup.Z)+(pin->Z*Cam->Vpn.Z);
}

void MConcat(geFloat in1[3][3], geFloat in2[3][3], geFloat out[3][3])
{
	int     i, j;
	for(i=0 ; i<3 ; i++) {
		for (j=0 ; j<3 ; j++) {
			out[i][j] = in1[i][0] * in2[0][j] +
				in1[i][1] * in2[1][j] +
				in1[i][2] * in2[2][j];
		}
	}
}

void MIdent(geFloat in[3][3])
{
	in[0][0]=1;	in[0][1]=0;	in[0][2]=0;
	in[1][0]=0;	in[1][1]=1;	in[1][2]=0;
	in[2][0]=0;	in[2][1]=0;	in[2][2]=1;
}

void ClearEdgeLists(ViewVars *v)
{
	int i;

	for(i=0;i < v->Height;i++)
	{
		v->NewEdges[i].pnext	=&MaxEdge;
		v->RemoveEdges[i]		=NULL;
	}
	for(i=0;i < v->FacesDone;i++)
	{
		SpanFaces[i].cur	=NULL;
		SpanFaces[i].head	=NULL;
	}
	v->FacesDone	=0;
}

void	Render_UpdateViewPos(ViewVars *v)
{
    geFloat	s, c, mtemp1[3][3], mtemp2[3][3];
	long	i;

	s			=(geFloat)sin(-v->yaw);
	c			=(geFloat)cos(-v->yaw);
	mYaw[0][0]	=c;
	mYaw[0][2]	=-s;
	mYaw[2][0]	=s;
	mYaw[2][2]	=c;

	s			=(geFloat)sin(v->pitch);
	c			=(geFloat)cos(v->pitch);
	mPitch[1][1]=c;
	mPitch[1][2]=s;
	mPitch[2][1]=-s;
	mPitch[2][2]=c;

	s			=(geFloat)sin(v->roll);
	c			=(geFloat)cos(v->roll);
	mRoll[0][0] =-c;
	mRoll[0][1] =-s;
	mRoll[1][0] =s;
	mRoll[1][1] =-c;

	MConcat(mRoll, mYaw, mtemp1);
	MConcat(mPitch, mtemp1, mtemp2);

	for(i=0;i<3;i++)
	{
		VectorToSUB(v->Vright, i)	=mtemp2[0][i];
		VectorToSUB(v->Vup, i)		=mtemp2[1][i];
		VectorToSUB(v->Vpn, i)		=mtemp2[2][i];
	}

	geVec3d_Normalize(&v->Vright);
	geVec3d_Normalize(&v->Vpn);
	geVec3d_Normalize(&v->Vup);
}

geFloat	RayDist;
Node	*RayNode;

void	Render_SetUpFrustum(ViewVars *v)
{
	geFloat	angle, s, c;
	geVec3d	n;

	angle	=(geFloat)atan(2.0f / v->FieldOfView * v->MaxScale / v->XScreenScale);
	s		=(geFloat)sin(angle);
	c		=(geFloat)cos(angle);

	//Left clip plane
	n.X	=s;
	n.Y	=0;
	n.Z	=c;
	geVec3d_Normalize(&n);
	Render_BackRotateVector(v, &n, &v->FrustPlanes[0].Normal);
	v->FrustPlanes[0].Dist	=geVec3d_DotProduct(&v->CamPos, &v->FrustPlanes[0].Normal)+CLIP_PLANE_EPSILON;
	
	// Right clip plane
	n.X	=-s;
	geVec3d_Normalize(&n);
	Render_BackRotateVector(v, &n, &v->FrustPlanes[1].Normal);
	v->FrustPlanes[1].Dist	=geVec3d_DotProduct(&v->CamPos, &v->FrustPlanes[1].Normal)+CLIP_PLANE_EPSILON;

	angle	=(geFloat)atan(2.0f / v->FieldOfView * v->MaxScale / v->YScreenScale);
	s		=(geFloat)sin(angle);
	c		=(geFloat)cos(angle);

	// Bottom clip plane
	n.X	=0;
	n.Y	=s;
	n.Z	=c;
	geVec3d_Normalize(&n);
	Render_BackRotateVector(v, &n, &v->FrustPlanes[2].Normal);
	v->FrustPlanes[2].Dist	=geVec3d_DotProduct(&v->CamPos, &v->FrustPlanes[2].Normal)+CLIP_PLANE_EPSILON;
	
	// Top clip plane
	n.Y	=-s;
	geVec3d_Normalize(&n);
	Render_BackRotateVector(v, &n, &v->FrustPlanes[3].Normal);
	v->FrustPlanes[3].Dist	=geVec3d_DotProduct(&v->CamPos, &v->FrustPlanes[3].Normal)+CLIP_PLANE_EPSILON;
}

int ClipToPlane(RenderFace *pin, Plane *pplane, RenderFace *pout)
{
	int		i, j, nextvert, curin, nextin;
	geFloat	curdot, nextdot, scale;
	geVec3d	*pinvert, *poutvert;
	
	pinvert	=pin->Points;
	poutvert=pout->Points;
	curdot	=geVec3d_DotProduct(&pin->Points[0], &pplane->Normal);
	curin	=(curdot >= pplane->Dist);

	for(i=0;i < pin->NumPoints;i++)
	{
		nextvert=(i+1) % pin->NumPoints;
		if(curin)
		{
			geVec3d_Copy(pinvert, poutvert);
			poutvert++;
		}
		nextdot	=geVec3d_DotProduct(&pin->Points[nextvert], &pplane->Normal);
		nextin	=(nextdot >= pplane->Dist);
		if(curin != nextin)
		{
			scale	=(pplane->Dist - curdot)/(nextdot-curdot);

			for(j=0;j<3;j++)
			{
				VectorToSUB(*poutvert, j)	=VectorToSUB(*pinvert, j)+
					((VectorToSUB(pin->Points[nextvert], j) - VectorToSUB(*pinvert, j))*scale);
			}

			poutvert++;
		}
		curdot	=nextdot;
		curin	=nextin;
		pinvert++;
	}
	pout->NumPoints=poutvert-pout->Points;
	if(pout->NumPoints >= 32 || pout->NumPoints < 3)
		return 0;
	else
		return 1;
}

int ClipToFrustum(Face *pin, RenderFace *pout, ViewVars *v)
{
	RenderFace	poly, cpoly;

	poly.NumPoints	=Face_GetNumPoints(pin);
	memcpy(poly.Points, Face_GetPoints(pin), sizeof(geVec3d)*poly.NumPoints);

	if(!ClipToPlane(&poly, &v->FrustPlanes[0], &cpoly)) return 0;
	if(!ClipToPlane(&cpoly, &v->FrustPlanes[1], &poly)) return 0;
	if(!ClipToPlane(&poly, &v->FrustPlanes[2], &cpoly)) return 0;
	if(!ClipToPlane(&cpoly, &v->FrustPlanes[3], pout)) return 0;
	return -1;
}

geVec3d Render_XFormVert(const ViewVars *v, const geVec3d *pin)
{
	geVec3d	out, work;
	geFloat	zinv;

	geVec3d_Subtract(pin, &v->CamPos, &work);

	out.X	=geVec3d_DotProduct(&work, &v->Vright);
	out.Y	=geVec3d_DotProduct(&work, &v->Vup);
	out.Z	=geVec3d_DotProduct(&work, &v->Vpn);

	zinv	=1.0f/out.Z;

//	out.X	=(out.X * zinv * v->MaxScale + v->XCenter);
//	out.X	=(out.X * zinv * v->MaxScale + v->XCenter);
	out.X	=(v->XCenter -(out.X * zinv * v->MaxScale));
	out.Y	=(v->YCenter -(out.Y * zinv * v->MaxScale));

	return	out;
}

void Render_ResizeView (ViewVars *v, long vx, long vy)
{
	HDC			ViewDC;

	vx=(vx+3)&~3;	//Align scan delta

	if(vx && vy)
	{
		if(v->Flags & DIBDONE)
		{
			DeleteObject(v->hDibSec);
			geRam_Free (v->pZBuffer);
		}
		else
			v->Flags|=DIBDONE;

		//Force top-down 8-bit bitmap of size WINDOW_WIDTH*WINDOW_HEIGHT.
		v->ViewBMI.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
		v->ViewBMI.bmiHeader.biPlanes       = 1;
		v->ViewBMI.bmiHeader.biBitCount     = 16;
		v->ViewBMI.bmiHeader.biCompression  = BI_RGB;
		v->ViewBMI.bmiHeader.biSizeImage    = 0;
		v->ViewBMI.bmiHeader.biClrUsed      = 0;
		v->ViewBMI.bmiHeader.biClrImportant = 0;
		v->ViewBMI.bmiHeader.biWidth        = vx;
		v->ViewBMI.bmiHeader.biHeight       = -vy;    // Minus for top-down.

		ViewDC=CreateCompatibleDC(NULL);
		assert(ViewDC);

		v->hDibSec=CreateDIBSection(ViewDC, (BITMAPINFO *)&v->ViewBMI, DIB_RGB_COLORS, (void **)&v->pBits, NULL, 0);
		assert(v->hDibSec);

		DeleteDC(ViewDC);

		//allocate a 32 bit zbuffer
		v->pZBuffer	=(uint32 *)	geRam_Allocate(sizeof(uint32) * (vx*vy));
	}

	v->FieldOfView		=2.0f;	//fixed for now?
	v->XScreenScale		=((geFloat)vx) / v->FieldOfView;
	v->YScreenScale		=((geFloat)vy) / v->FieldOfView;
	v->MaxScale			=max(v->XScreenScale, v->YScreenScale);
	v->MaxScreenScaleInv=1.0f / v->MaxScale;
	v->XCenter			=((geFloat)vx) / 2.0f - 0.5f;
	v->YCenter			=((geFloat)vy) / 2.0f - 0.5f;

	if(v->ViewType < VIEWTOP)
	{
		if(v->NewEdges)
			geRam_Free (v->NewEdges);
		if(v->RemoveEdges)
			geRam_Free (v->RemoveEdges);

		v->NewEdges			=(Edge *)geRam_Allocate(vy*sizeof(Edge));
		v->RemoveEdges		=(Edge **)geRam_Allocate(vy*sizeof(Edge *));
	}
	v->Width			=vx;
	v->Height			=vy;
}

void Render_ResetSettings(ViewVars *v, long vx, long vy)
{
	Render_ResizeView (v, vx, vy);

	// Compute and set zoom factor
	Render_SetZoom (v, (v->Width / 640.0f));

	geVec3d_Clear (&v->Vpn);
	geVec3d_Clear (&v->Vright);
	geVec3d_Clear (&v->Vup);

	v->roll = 0.0f;
	v->pitch	=M_PI;
	v->yaw	=0.0f;

	mRoll[0][0]=1;  mRoll[0][1]=0;  mRoll[0][2]=0;
	mRoll[1][0]=0;  mRoll[1][1]=1;  mRoll[1][2]=0;
	mRoll[2][0]=0;	mRoll[2][1]=0;  mRoll[2][2]=1;
	mPitch[0][0]=1; mPitch[0][1]=0;	mPitch[0][2]=0;
	mPitch[1][0]=0; mPitch[1][1]=1; mPitch[1][2]=0;
	mPitch[2][0]=0; mPitch[2][1]=0; mPitch[2][2]=1;
	mYaw[0][0]=1;	mYaw[0][1]=0;	mYaw[0][2]=0;
	mYaw[1][0]=0;	mYaw[1][1]=1;	mYaw[1][2]=0;
	mYaw[2][0]=0;	mYaw[2][1]=0;	mYaw[2][2]=1;

	geVec3d_Clear (&v->CamPos);
}

static void AddNodeEdges(Face *NodeFace,	//node face
						 const Face *OGFace,//original brush face
						 RenderFace *sf,	//screenspace clipped
						 RenderFace *sfnc,	//screenspace unclipped
						 uint32 Key,		//bspsortkey
						 ViewVars *Cam,		//viewvars
						 int RFlags);		//render flags

static void ScanEdges(ViewVars *);
static void DrawSpans(ViewVars *, HDC);

static POINT plist[64];

/*
void Render_RenderTreeOrtho(ViewVars *Cam, Node *n, HDC ViewDC)
{
	int32	i, j;

	if(!n)
		return;

	Render_RenderTreeOrtho(Cam, n->Front, ViewDC);
	Render_RenderTreeOrtho(Cam, n->Back, ViewDC);

	for(i=0;i < n->NumFaces;i++)
	{
		if((n->Flags & CLIPPEDOUT) || (n->Faces[i].Flags & INSOLID))
			continue;

		for(j=0;j < n->Faces[i].NumPoints;j++)
		{
			plist[j]	=Render_OrthoWorldToView(Cam, &n->Faces[i].Points[j]);
		}
		plist[j]	=Render_OrthoWorldToView(Cam, &n->Faces[i].Points[0]);
		Polyline(ViewDC, plist, j+1);
	}
}

void Render_RenderTreeHollowOrtho(ViewVars *Cam, Node *n, HDC ViewDC)
{
	int32	i, j;

	if(!n)
		return;

	Render_RenderTreeHollowOrtho(Cam, n->Front, ViewDC);
	Render_RenderTreeHollowOrtho(Cam, n->Back, ViewDC);

//	if(TexInf[n->TexId].Brush->bd.Flags & HOLLOW)
	{
		for(i=0;i < n->NumFaces;i++)
		{
			if((n->Flags & CLIPPEDOUT) || (n->Faces[i].Flags & INSOLID))
				continue;

			for(j=0;j < n->Faces[i].NumPoints;j++)
			{
				plist[j]	=Render_OrthoWorldToView(Cam, &n->Faces[i].Points[j]);
			}
			plist[j]	=Render_OrthoWorldToView(Cam, &n->Faces[i].Points[0]);
			Polyline(ViewDC, plist, j+1);
		}
	}
}
*/
void Render_RenderBrushFacesOrtho( const ViewVars *Cam, Brush *b, HDC ViewDC)
{
	int	i, j;

	assert (b != NULL);

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		Face			*f		=Brush_GetFace(b, i);
		const geVec3d	*pnts	=Face_GetPoints(f);
		for(j=0;j < Face_GetNumPoints(f);j++)
		{
			plist[j]	=Render_OrthoWorldToView(Cam, &pnts[j]);
		}
		plist[j]	=plist[0]; //Render_OrthoWorldToView(Cam, &b->Faces[i].Points[0]);
		Polyline(ViewDC, plist, j+1);
	}
}

static void Render_RenderOneBrushFaceZBuffer (ViewVars *Cam, RenderFace *pFace, uint32 LineColor)
{
	int j;
	RenderFace Clipped;

	for(j=0;j < pFace->NumPoints;j++)
	{
		Clipped.Points[j]	=Render_XFormVert(Cam, &pFace->Points[j]);
	}
	for(j=0;j < pFace->NumPoints;j++)
	{
		int	k				=(j+1) % pFace->NumPoints;

		Render_LineZBuffer( Units_Round(Clipped.Points[j].X),
							Units_Round(Clipped.Points[j].Y),
							Clipped.Points[j].Z,
							Units_Round(Clipped.Points[k].X),
							Units_Round(Clipped.Points[k].Y),
							Clipped.Points[k].Z,
							((uint16)(((LineColor & 0xf8)<<7) | ((LineColor & 0xf800)>>6) | ((LineColor & 0xf80000)>>19)))
							, Cam);
	}

}

static void Render_RenderOneBrushFace (ViewVars *Cam, RenderFace *pFace, uint32 LineColor)
{
	int j;
	RenderFace Clipped;

	for(j=0;j < pFace->NumPoints;j++)
	{
		Clipped.Points[j]	=Render_XFormVert(Cam, &pFace->Points[j]);
	}
	for(j=0;j < pFace->NumPoints;j++)
	{
		int	k				=(j+1) % pFace->NumPoints;

		Render_Line 
		(
			Units_Round(Clipped.Points[j].X),
			Units_Round(Clipped.Points[j].Y),
			Units_Round(Clipped.Points[k].X),
			Units_Round(Clipped.Points[k].Y),
			((uint16)(((LineColor & 0xf8)<<7) | ((LineColor & 0xf800)>>6) | ((LineColor & 0xf80000)>>19))),
			Cam
		);
	}

}

void Render_RenderBrushFaces(ViewVars *Cam, Brush *b, uint32 LineColor)
{
	int	i;
	RenderFace	TempFace;

	if(!b)
		return;

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		Face	*f	=Brush_GetFace(b, i);
		if(ClipToFrustum(f, &TempFace, Cam))
		{
			Render_RenderOneBrushFace (Cam, &TempFace, LineColor);
		}
	}
}

void Render_RenderBrushSelFaces(ViewVars *Cam, Brush *b, uint32 LineColor)
{
	int	i;
	RenderFace	TempFace;

	if(!b)
		return;

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		Face	*f	=Brush_GetFace(b, i);

		if(!Face_IsSelected(f))
			continue;

		if(ClipToFrustum(f, &TempFace, Cam))
		{
			Render_RenderOneBrushFace (Cam, &TempFace, LineColor);
		}
	}
}

void Render_RenderBrushFacesZBuffer(ViewVars *Cam, Brush *b, uint32 LineColor)
{
	int			i;
	RenderFace	TempFace;

	if(!b)
		return;

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		Face	*f	=Brush_GetFace(b, i);
		if(ClipToFrustum(f, &TempFace, Cam))
		{
			Render_RenderOneBrushFaceZBuffer (Cam, &TempFace, LineColor);
		}
	}
}


void	Render_3DTextureZBuffer(ViewVars *Cam, const geVec3d *pos, const geBitmap *bmap)
{
	int		x, y, i, Width, Height;
	int		xroll, yroll;
	int		sx, sy, ex, ey;
	geVec3d	cpos;
	uint16	*src;
	uint16	*dst;
	uint32	*zbuf, zval;
	geVec3d box[8];
	geFloat	x0, x1, y0, y1, z0, z1;
	int		dxi, dxf, dyi, dyf, xacc;

//	geBitmap_Palette	*bpal;
//	WORD WorkingPalette[256];

	//get the center position on the screen
	float xPlus8  = (float)(pos->X + 8.0);
	float xMinus8 = (float)(pos->X - 8.0);
	float yPlus8  = (float)(pos->Y + 8.0);
	float yMinus8 = (float)(pos->Y - 8.0);
	float zPlus8  = (float)(pos->Z + 8.0);
	float zMinus8 = (float)(pos->Z - 8.0);

	geBitmap *LockedBitmap;

	x0=y0=z0	=99999.0f;
	x1=y1=z1	=-99999.0f;

	{
		geBitmap_Info info, info2;

		geBitmap_GetInfo (bmap, &info, &info2);
		Width = info.Width;
		Height = info.Height;
	}
//	Width	=Bitmap_GetWidth(bmap);
//	Height	=Bitmap_GetHeight(bmap);

	geVec3d_Set (&box[0], xPlus8,  yPlus8,  zPlus8);
	geVec3d_Set (&box[1], xMinus8, yPlus8,  zPlus8);
	geVec3d_Set (&box[2], xMinus8, yMinus8, zPlus8);
	geVec3d_Set (&box[3], xPlus8,  yMinus8, zPlus8);
	geVec3d_Set (&box[4], xPlus8,  yPlus8,  zMinus8);
	geVec3d_Set (&box[5], xMinus8, yPlus8,  zMinus8);
	geVec3d_Set (&box[6], xMinus8, yMinus8, zMinus8);
	geVec3d_Set (&box[7], xPlus8,  yMinus8, zMinus8);
	for(i=0;i < 8;i++)
	{
		cpos	=Render_XFormVert(Cam, &box[i]);
		if(cpos.X < x0)	x0	=cpos.X;
		if(cpos.Y < y0)	y0	=cpos.Y;
		if(cpos.Z < z0)	z0	=cpos.Z;
		if(cpos.X > x1)	x1	=cpos.X;
		if(cpos.Y > y1)	y1	=cpos.Y;
		if(cpos.Z > z1)	z1	=cpos.Z;
	}
	cpos	=Render_XFormVert(Cam, pos);
	zval	=(uint32)(cpos.Z * FixedScale);
	
	sx	=(int)((x0 < 0.0f)? 0.0f : (x0 > Cam->Width)? Cam->Width : x0);
	ex	=(int)((x1 < 0.0f)? 0.0f : (x1 > Cam->Width)? Cam->Width : x1);
	sy	=(int)((y0 < 0.0f)? 0.0f : (y0 > Cam->Height)? Cam->Height : y0);
	ey	=(int)((y1 < 0.0f)? 0.0f : (y1 > Cam->Height)? Cam->Height : y1);

	if(!(ex - sx) || !(ey - sy))
	{
		return;
	}

	dxi	=Width / ((int)x1 - (int)x0);
	if(dxi)
	{
		dxf	=Width - ((int)x1 - (int)x0) * dxi;
	}
	else
	{
		dxf	=(int)((((float)Width) / ((int)x1 - (int)x0)) * ((float)Width));
	}
	dyi	=Height / ((int)y1 - (int)y0);
	if(dyi)
	{
		dyf	=Height - ((int)y1 - (int)y0) * dyi;
	}
	else
	{
		dyf	=(int)((((float)Height) / ((int)y1 - (int)y0)) * ((float)Height));
	}
	dyi	*=Width;


	geBitmap_LockForReadNative (bmap, &LockedBitmap, 0, 0);

	src		=((uint16 *)geBitmap_GetBits(LockedBitmap)) + (Height -1) * Width;
//	bpal = geBitmap_GetPalette ((geBitmap *)bmap);
//	geBitmap_Palette_GetData (bpal, WorkingPalette, GE_PIXELFORMAT_16BIT_555_BGR, 256);

	zbuf	=Cam->pZBuffer;
	if(*(zbuf + (((sy+((ey-sy)/2)))*Cam->Width + sx+((ex-sx)/2))) > zval)
	{
		for(yroll=0, y=sy;y < ey;y++)
		{
			dst		=((uint16 *)Cam->pBits) + (y * Cam->Width) + sx;
			zbuf	=(uint32 *)(Cam->pZBuffer + (y * Cam->Width) + sx);
			for(xroll=0, xacc=0, x=sx;x < ex;x++)
			{
				//uncomment to get per pixel z (and remove the if above)
				if(/*(*zbuf < zval) && */(src[xacc] != 0x7fff))
				{
					*dst	= src[xacc];
					*zbuf	=zval;
				}
				dst++;
				zbuf++;
				xacc	+=dxi;
				xroll	+=dxf;
				if(xroll > Width)
				{
					xroll	-=Width;
					xacc++;
				}
			}
			src		-=dyi;
			yroll	+=dyf;
			if(yroll > Height)
			{
				yroll	-=Height;
				src		-=Width;
			}
		}
	}
	geBitmap_UnLock (LockedBitmap);
}

void	Render_3DTextureZBufferOutline(ViewVars *Cam, const geVec3d *pos, const geBitmap *bmap, uint32 OutlineColor)
{
	int		x, y, i, Width, Height;
	int		xroll, yroll;
	int		sx, sy, ex, ey;
	geVec3d	cpos;
	uint16	*src;
	uint16	*dst;
	uint32	*zbuf, zval;
	geVec3d box[8];
	geFloat	x0, x1, y0, y1, z0, z1;
	int		dxi, dxf, dyi, dyf, xacc;

//	Bitmap_Palette	bpal;
//	geBoolean		Outlining	=GE_FALSE;

	//get the center position on the screen
	float xPlus8  = (float)(pos->X + 8.0);
	float xMinus8 = (float)(pos->X - 8.0);
	float yPlus8  = (float)(pos->Y + 8.0);
	float yMinus8 = (float)(pos->Y - 8.0);
	float zPlus8  = (float)(pos->Z + 8.0);
	float zMinus8 = (float)(pos->Z - 8.0);
	geBitmap *LockedBitmap;

	x0=y0=z0	=99999.0f;
	x1=y1=z1	=-99999.0f;

	{
		geBitmap_Info info, info2;

		geBitmap_GetInfo (bmap, &info, &info2);
		Width = info.Width;
		Height = info.Height;
	}
//	Width	=Bitmap_GetWidth(bmap);
//	Height	=Bitmap_GetHeight(bmap);

	geVec3d_Set (&box[0], xPlus8,  yPlus8,  zPlus8);
	geVec3d_Set (&box[1], xMinus8, yPlus8,  zPlus8);
	geVec3d_Set (&box[2], xMinus8, yMinus8, zPlus8);
	geVec3d_Set (&box[3], xPlus8,  yMinus8, zPlus8);
	geVec3d_Set (&box[4], xPlus8,  yPlus8,  zMinus8);
	geVec3d_Set (&box[5], xMinus8, yPlus8,  zMinus8);
	geVec3d_Set (&box[6], xMinus8, yMinus8, zMinus8);
	geVec3d_Set (&box[7], xPlus8,  yMinus8, zMinus8);
	for(i=0;i < 8;i++)
	{
		cpos	=Render_XFormVert(Cam, &box[i]);
		if(cpos.X < x0)	x0	=cpos.X;
		if(cpos.Y < y0)	y0	=cpos.Y;
		if(cpos.Z < z0)	z0	=cpos.Z;
		if(cpos.X > x1)	x1	=cpos.X;
		if(cpos.Y > y1)	y1	=cpos.Y;
		if(cpos.Z > z1)	z1	=cpos.Z;
	}
	cpos	=Render_XFormVert(Cam, pos);
	zval	=(uint32)(cpos.Z * FixedScale);
	
	sx	=(int)((x0 < 0.0f)? 0.0f : (x0 > Cam->Width)? Cam->Width : x0);
	ex	=(int)((x1 < 0.0f)? 0.0f : (x1 > Cam->Width)? Cam->Width : x1);
	sy	=(int)((y0 < 0.0f)? 0.0f : (y0 > Cam->Height)? Cam->Height : y0);
	ey	=(int)((y1 < 0.0f)? 0.0f : (y1 > Cam->Height)? Cam->Height : y1);

	if(!(ex - sx) || !(ey - sy))
	{
		return;
	}

	dxi	=Width / ((int)x1 - (int)x0);
	if(dxi)
	{
		dxf	=Width - ((int)x1 - (int)x0) * dxi;
	}
	else
	{
		dxf	=(int)((((float)Width) / ((int)x1 - (int)x0)) * ((float)Width));
	}
	dyi	=Height / ((int)y1 - (int)y0);
	if(dyi)
	{
		dyf	=Height - ((int)y1 - (int)y0) * dyi;
	}
	else
	{
		dyf	=(int)((((float)Height) / ((int)y1 - (int)y0)) * ((float)Height));
	}
	dyi	*=Width;

	geBitmap_LockForReadNative (bmap, &LockedBitmap, 0, 0);

	src		=((uint16 *)geBitmap_GetBits(LockedBitmap)) + (Height -1) * Width;
//	Bitmap_GetPalette(bmap,  &bpal);
	zbuf	=Cam->pZBuffer;
	if(*(zbuf + (((sy+((ey-sy)/2)))*Cam->Width + sx+((ex-sx)/2))) > zval)
	{
		for(yroll=0, y=sy;y < ey;y++)
		{
			dst		=((uint16 *)Cam->pBits) + (y * Cam->Width) + sx;
			zbuf	=(uint32 *)(Cam->pZBuffer + (y * Cam->Width) + sx);
			for(xroll=0, xacc=0, x=sx;x < ex;x++)
			{
				//uncomment to get per pixel z (and remove the if above)
				if(/*(*zbuf < zval) && */(src[xacc] != 0x7fff))
				{
/*					if(Outlining)
					{
						*dst	=((uint16)(((bpal.bpEntries[*(src + xacc)].bpeB)>>3)
									| ((bpal.bpEntries[*(src + xacc)].bpeG & 0xf8)<<2)
									| ((bpal.bpEntries[*(src + xacc)].bpeR & 0xf8)<<7)));
						*zbuf	=zval;
					}
					else
					{
						*(dst-1)=(((OutlineColor & 0x1f) + (*(dst-1) & 0x1f))>>1)
								|((((OutlineColor & 0x1f00)>>7) + (*(dst-1) & 0x3e0))>>1)
								|((((OutlineColor & 0x1f0000)>>14) + (*(dst-1) & 0x7c00))>>1);
						*dst	=(((OutlineColor & 0xff) + bpal.bpEntries[*(src + xacc)].bpeB)>>4)
								|(((((OutlineColor & 0xff00)>>8) + bpal.bpEntries[*(src + xacc)].bpeG)>>4)<<5)
								|(((((OutlineColor & 0xff0000)>>16) + bpal.bpEntries[*(src + xacc)].bpeR)>>4)<<10);
*/						*dst	=((uint16)(((OutlineColor & 0xf8)<<7) | ((OutlineColor & 0xf800)>>6) | ((OutlineColor & 0xf80000)>>19)));
						*zbuf	=zval;

//						Outlining	=GE_TRUE;
//					}
				}
/*				else if(Outlining)
				{
					Outlining	=GE_FALSE;
					*(dst-1)=(((OutlineColor & 0x1f) + (*(dst-1) & 0x1f))>>1)
							|((((OutlineColor & 0x1f00)>>7) + (*(dst-1) & 0x3e0))>>1)
							|((((OutlineColor & 0x1f0000)>>14) + (*(dst-1) & 0x7c00))>>1);
					*(dst)	=(((OutlineColor & 0x1f) + (*(dst) & 0x1f))>>1)
							|((((OutlineColor & 0x1f00)>>7) + (*(dst) & 0x3e0))>>1)
							|((((OutlineColor & 0x1f0000)>>14) + (*(dst) & 0x7c00))>>1);
				}
*/				dst++;
				zbuf++;
				xacc	+=dxi;
				xroll	+=dxf;
				if(xroll > Width)
				{
					xroll	-=Width;
					xacc++;
				}
			}
			src		-=dyi;
			yroll	+=dyf;
			if(yroll > Height)
			{
				yroll	-=Height;
				src		-=Width;
			}
		}
	}
	geBitmap_UnLock (LockedBitmap);
}

BOOL Render_PointInFrustum(ViewVars *Cam, geVec3d *v)
{
	int		i;

	for(i=0;i < 4;i++)
		if((geVec3d_DotProduct(v, &Cam->FrustPlanes[i].Normal)-Cam->FrustPlanes[i].Dist) < 0.0f)
			return	FALSE;

	return	TRUE;
}

geFloat Render_ViewDeltaToRadians
(
	const ViewVars *v, 
	const float dx
)
{
	return (dx)*(ONE_OVER_2PI/Render_GetXScreenScale (v));
}

void Render_ViewDeltaToRotation
	(
	  const ViewVars *v, 
	  const float dx, 
	  geVec3d *VecRotate
	)
{
	float RotationRads;

	assert (v != NULL);
	assert (VecRotate != NULL);

	RotationRads = (dx)*(ONE_OVER_2PI/Render_GetXScreenScale (v));
	switch (v->ViewType)
	{
		case VIEWTOP :	// +dx = negative rotation about Y
			geVec3d_Set (VecRotate, 0.0f, -RotationRads, 0.0f);
			break;
		case VIEWFRONT :  // +dx = negative rotation about Z
			//disable roll 
			geVec3d_Set (VecRotate, 0.0f, 0.0f, -RotationRads);
			break;
		case VIEWSIDE :	// +dx = positive rotation about X
			geVec3d_Set (VecRotate, RotationRads, 0.0f, 0.0f);
			break;
		default :
			assert (0);  // can't happen!
	}
}

// Return world position at center of view
geVec3d		Render_GetViewCenter (const ViewVars *v)
{
	geVec3d TopLeft;
	geVec3d BottomRight;
	geVec3d Center;

	Render_ViewToWorld (v, 0, 0, &TopLeft);
	Render_ViewToWorld (v, v->Width-1, v->Height-1, &BottomRight);
	geVec3d_Add (&TopLeft, &BottomRight, &Center);
	geVec3d_Scale (&Center, 0.5f, &Center);
	return Center;
}


void Render_ViewToWorld(const ViewVars *v, const int x, const int y, geVec3d *wp)
/*
  XY view coordinate transformed to world coordinate, depending on view.

    Mouse Coordinates
    -----------------

		   |
		   |
 		   |
	-------+-------> +X
		   |
		   |
		   |
		   +Y


        Top View				  Front View				   Side View
  	    --------				  ----------				   ---------
									   +Y						   +Y
		   |						   |						   |
		   |						   |						   |
		   |						   |						   |
	-------+-------> +X			-------+-------> +X			-------+-------> +Z
		   |						   |						   |
		   |						   |						   |
		   |						   |						   |
  		   +Z
*/
{
	geFloat	ZoomInv=1.0f / v->ZoomFactor;

	switch (v->ViewType)
	{
		case VIEWTOP :
		{
			geVec3d_Set (wp, (x - v->XCenter), 0.0f, (y - v->YCenter));
			geVec3d_Scale (wp, ZoomInv, wp);
			geVec3d_Add (wp, &v->CamPos, wp);
			break;
		}
		case VIEWFRONT :
		{

			geVec3d_Set (wp, (x - v->XCenter), -(y - v->YCenter), 0.0f);
			geVec3d_Scale (wp, ZoomInv, wp);
			geVec3d_Add (wp, &v->CamPos, wp);
			break;
		}
		case VIEWSIDE :
		{
			geVec3d_Set (wp, 0.0f, -(y - v->YCenter), (x - v->XCenter));
			geVec3d_Scale (wp, ZoomInv, wp);
			geVec3d_Add (wp, &v->CamPos, wp);
			break;
		}
		default :
		{
			geVec3d_Set 
			(
				wp,
				-(x -v->XCenter)*(v->MaxScreenScaleInv), 
				-(y -v->YCenter)*(v->MaxScreenScaleInv), 
				1.0f
			);
			geVec3d_Normalize(wp);
			break;
		}
	}
}

POINT Render_OrthoWorldToView(const ViewVars *v, geVec3d const *wp)
{
	POINT	sc = {0, 0};
	geVec3d ptView;
	switch (v->ViewType)
	{
		case VIEWTOP :
		{
			geVec3d_Subtract (wp, &v->CamPos, &ptView);
			geVec3d_Scale (&ptView, v->ZoomFactor, &ptView);
			sc.x = (int)(v->XCenter + ptView.X);
			sc.y = (int)(v->YCenter + ptView.Z);
			break;
		}
		case VIEWFRONT :
		{
			geVec3d_Subtract (wp, &v->CamPos, &ptView);
			geVec3d_Scale (&ptView, v->ZoomFactor, &ptView);
			sc.x = (int)(v->XCenter + ptView.X);
			sc.y = (int)(v->YCenter - ptView.Y);
			break;
		}
		case VIEWSIDE :
		{
			geVec3d_Subtract (wp, &v->CamPos, &ptView);
			geVec3d_Scale (&ptView, v->ZoomFactor, &ptView);
			sc.x = (int)(v->XCenter + ptView.Z);
			sc.y = (int)(v->YCenter - ptView.Y);
			break;
		}
		default :
//			assert (0);	// bad view type
			// I should be able to assert here, but I can't.
			// somebody is calling this function for the rendered
			// view.  Bad stuff, really, but I don't have time to change it.
			break;
	}
	return sc;
}

void Render_RenderOrthoGridFromSize(ViewVars *v, geFloat Interval, HDC ViewDC)
{
	geVec3d		ystep, xstep, Delt, Delt2;
	//, Test={1.0f, 1.0f, 1.0f};
	int			i, cnt, xaxis, yaxis, inidx;
	static int axidx[3][2]={ 2, 1, 0, 2, 0, 1 };
	geFloat	gsinv;//, sy;
	Box3d ViewBox;
	POINT		sp;

	inidx	=(v->ViewType>>3)&0x3;

	xaxis	=axidx[inidx][0];
	yaxis	=axidx[inidx][1];


	Render_ViewToWorld(v, Units_Round(-Interval), Units_Round(-Interval), &Delt);
	Render_ViewToWorld(v, Units_Round(v->Width+Interval), Units_Round(v->Height+Interval), &Delt2);

	Box3d_Set 
		(
		  &ViewBox,
		  Delt.X, Delt.Y, Delt.Z,
		  Delt2.X, Delt2.Y, Delt2.Z
		);
	VectorToSUB(ViewBox.Min, inidx)	=-FLT_MAX;
	VectorToSUB(ViewBox.Max, inidx)	=FLT_MAX;

	//snap viewmin and viewmax to grid
	gsinv	=1.0f/(geFloat)Interval;
	for(i=0;i<3;i++)
	{
		VectorToSUB(ViewBox.Min, i)	=(geFloat) ((int)(VectorToSUB(ViewBox.Min, i)*gsinv))*Interval;
		VectorToSUB(ViewBox.Max, i)	=(geFloat) ((int)(VectorToSUB(ViewBox.Max, i)*gsinv))*Interval;
	}

	geVec3d_Copy(&VecOrigin, &xstep);
	geVec3d_Copy(&VecOrigin, &ystep);
	VectorToSUB(ystep, yaxis)	=(geFloat)Interval;
	VectorToSUB(xstep, xaxis)	=(geFloat)Interval;

	// horizontal lines
	geVec3d_Copy(&ViewBox.Min, &Delt);
	geVec3d_Copy(&ViewBox.Min, &Delt2);
	VectorToSUB(Delt2, xaxis)	=VectorToSUB(ViewBox.Max, xaxis);
	cnt	=Units_Round((VectorToSUB(ViewBox.Max, yaxis) - VectorToSUB(ViewBox.Min, yaxis))*gsinv);
	for(i=0;i <= cnt;i++)
	{
		sp	=Render_OrthoWorldToView(v, &Delt);
		MoveToEx(ViewDC, 0, sp.y, NULL);
		sp	=Render_OrthoWorldToView(v, &Delt2);
		LineTo(ViewDC, v->Width, sp.y);
		geVec3d_Add(&Delt, &ystep, &Delt);
		geVec3d_Add(&Delt2, &ystep, &Delt2);
	}

	// vertical lines
	geVec3d_Copy(&ViewBox.Min, &Delt);
	geVec3d_Copy(&ViewBox.Min, &Delt2);
	VectorToSUB(Delt2, yaxis)	=VectorToSUB(ViewBox.Max, yaxis);
	cnt	=Units_Round((VectorToSUB(ViewBox.Max, xaxis) - VectorToSUB(ViewBox.Min, xaxis))*gsinv);
	for(i=0;i <= cnt;i++)
	{
		sp	=Render_OrthoWorldToView(v, &Delt);
		MoveToEx(ViewDC, sp.x, 0, NULL);
		sp	=Render_OrthoWorldToView(v, &Delt2);
		LineTo(ViewDC, sp.x, v->Height);
		geVec3d_Add(&Delt, &xstep, &Delt);
		geVec3d_Add(&Delt2, &xstep, &Delt2);
	}
}

void Render_RenderBrushSelFacesOrtho(ViewVars *Cam, Brush *b, HDC ViewDC)
{
	int	i, j;

	if(!b)
		return;

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		Face			*f		=Brush_GetFace(b, i);
		const geVec3d	*pnts	=Face_GetPoints(f);

		if(!Face_IsSelected(f))
			continue;

		for(j=0;j < Face_GetNumPoints(f);j++)
		{
			plist[j]	=Render_OrthoWorldToView(Cam, &pnts[j]);
		}
		plist[j]	=Render_OrthoWorldToView(Cam, &pnts[0]);
		Polyline(ViewDC, plist, j+1);
	}
}

void Render_RenderBrushSheetFacesOrtho(ViewVars *Cam, Brush *b, HDC ViewDC)
{
	int				j;
	Face			*f;
	const geVec3d	*pnts;

	if(!b)
	{
		return;
	}

	f		=Brush_GetFace(b, 0);
	pnts	=Face_GetPoints(f);

	for(j=0;j < Face_GetNumPoints(f);j++)
	{
		plist[j]	=Render_OrthoWorldToView(Cam, &pnts[j]);
	}
	plist[j]	=Render_OrthoWorldToView(Cam, &pnts[0]);
	Polyline(ViewDC, plist, j+1);
}
/*
void Render_RenderBrushHintFacesOrtho(ViewVars *Cam, Brush *b, HDC ViewDC)
{
	int	i, j;

	if(!b)
		return;

	for(i=0;i < b->bd.NumFaces;i++)
	{
		if(!(b->Faces[i].Flags & HINT))
			continue;

		for(j=0;j < b->Faces[i].NumPoints;j++)
		{
			plist[j]	=Render_OrthoWorldToView(Cam, &b->Faces[i].Points[j]);
		}
		plist[j]	=Render_OrthoWorldToView(Cam, &b->Faces[i].Points[0]);
		Polyline(ViewDC, plist, j+1);
	}
}
*/

//come back to this later
#pragma warning (disable:4100)
static void FillBackSpans(ViewVars *Cam)
{
/*
	long		i;
	Node		Dummy;
	RenderFace	Clipped, Unclipped, TempFace;
	Plane		dp;

	memset(&Dummy, 0, sizeof(Node));
//	Dummy.Faces=(Face *)geRam_Allocate(sizeof(Face));
//	memset(Dummy.Faces, 0, sizeof(Face));
	geVec3d_Subtract(&VecOrigin, &Cam->Vpn, &dp.Normal);
	dp.Dist	=-geVec3d_DotProduct(&Cam->Vpn, &Cam->CamPos)-5.0f;
	Node_CreateFromPlane(&Dummy, &dp);
	Dummy.Faces->Tex.Dib	=0xffff;

	if(ClipToFrustum(Dummy.Faces, &TempFace, Cam))
	{
		for(i=0;i < TempFace.NumPoints;i++)
			Clipped.Points[i]	=Render_XFormVert(Cam, &TempFace.Points[i]);
		for(i=0;i < Dummy.Faces->NumPoints;i++)
			Unclipped.Points[i]	=Render_XFormVert(Cam, &Dummy.Faces->Points[i]);

		Clipped.NumPoints	=TempFace.NumPoints;
		Unclipped.NumPoints	=Dummy.Faces->NumPoints;
		AddNodeEdges(&Dummy, 0, &Clipped, &Unclipped, 1, Cam, NORMAL);
	}
	geRam_Free (Dummy.Faces->Points);
	geRam_Free (Dummy.Faces);
	*/
}
#pragma warning (default:4100)

//standard error term line draw with zbuffering
static void	Render_LineZBuffer(int xa, int ya, geFloat za,
							   int xb, int yb, geFloat zb,
							   uint16 color, ViewVars *Cam)
{
	int		dx, dy;
	short	pos;
	int		g, r, c, inc1, inc2, f;
	uint32	zai, dz, *tz;
	uint16	*td;
	
	dy	=yb - ya;
	if(dy==0)
	{
		//Horizontal lines
		if(xb < xa)
		{
			dz	=(uint32)(((za - zb)/((geFloat)(xa-xb))) * FixedScale);
			zai	=(uint32)(zb * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (ya * Cam->Width) + xb);
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xb;
			for(;xb <= xa;xb++)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
				td++;
				tz++;
				zai	+=dz;
			}
		}
		else
		{
			dz	=(uint32)(((zb - za)/((geFloat)(xb-xa))) * FixedScale);
			zai	=(uint32)(za * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (ya * Cam->Width) + xa);
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xa;
			for(;xa <= xb;xa++)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
				td++;
				tz++;
				zai	+=dz;
			}
		}
		return;
	}

	dx	=xb - xa;
	if(dx==0)
	{
		//Vertical Lines
		if (yb < ya)
		{
			dz	=(uint32)(((za - zb)/((geFloat)(ya-yb))) * FixedScale);
			zai	=(uint32)(zb * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (yb * Cam->Width) + xb);
			td	=((uint16 *)Cam->pBits) + (yb * Cam->Width) + xb;
			for(;yb <= ya;yb++)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
				td	+=Cam->Width;
				tz	+=Cam->Width;
				zai	+=dz;
			}
		}
		else
		{
			dz	=(uint32)(((zb - za)/((geFloat)(yb-ya))) * FixedScale);
			zai	=(uint32)(za * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (ya * Cam->Width) + xb);
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xb;
			for(;ya <= yb;ya++)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
				td	+=Cam->Width;
				tz	+=Cam->Width;
				zai	+=dz;
			}
		}
		return;
	}
	if(dx > 0)
	{
		if(dy < 0)
		{
			pos	=0;
		}
		else
		{
			pos	=1;
		}
	}
	else
	{
		if(dy < 0)
		{
			pos	=1;
		}
		else
		{
			pos	=0;
		}
	}
	if(abs(dx) > abs(dy))
	{
		if(dx > 0)
		{
			c	=xa;
			r	=ya;
			f	=xb;
			dz	=(uint32)(((zb - za)/((geFloat)(xb-xa))) * FixedScale);
			zai	=(uint32)(za * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (ya * Cam->Width) + xa);
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xa;
		}
		else
		{
			c	=xb;
			r	=yb;
			f	=xa;
			dz	=(uint32)(((za - zb)/((geFloat)(xa-xb))) * FixedScale);
			zai	=(uint32)(zb * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (yb * Cam->Width) + xb);
			td	=((uint16 *)Cam->pBits) + (yb * Cam->Width) + xb;
		}
		inc1=abs(dy + dy);
		g	=abs(dy) * 2 - abs(dx);
		inc2=(abs(dy) - abs(dx)) * 2;
		if(pos)
		{
			while(c <= f)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
				td++;
				tz++;
				zai	+=dz;
				c++;

				if(g >= 0)
				{
					r++;
					g	+=inc2;
					td	+=Cam->Width;
					tz	+=Cam->Width;
				}
				else
				{
					g	+=inc1;
				}
			}
			return;
		}
		else
		{
			while(c <= f)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
				zai	+=dz;
				c++;
				td++;
				tz++;

				if(g > 0)
				{
					r--;
					g	+=inc2;
					td	-=Cam->Width;
					tz	-=Cam->Width;
				}
				else
				{
					g	+=inc1;
				}
			}
			return;
		}
	}
	else
	{
		if(dy > 0)
		{
			c	=ya;
			r	=xa;
			f	=yb;
			dz	=(uint32)(((zb - za)/((geFloat)(yb-ya))) * FixedScale);
			zai	=(uint32)(za * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (ya * Cam->Width) + xa);
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xa;
		}
		else
		{
			c	=yb;
			r	=xb;
			f	=ya;
			dz	=(uint32)(((za - zb)/((geFloat)(ya-yb))) * FixedScale);
			zai	=(uint32)(zb * FixedScale);
			tz	=(uint32 *)(Cam->pZBuffer + (yb * Cam->Width) + xb);
			td	=((uint16 *)Cam->pBits) + (yb * Cam->Width) + xb;
		}
		inc1	=abs(dx + dx);
		g		=abs(dx) * 2 - abs(dy);
		inc2	=(abs(dx) - abs(dy)) * 2;
		if(pos)
		{
			if(c <= f)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
LOOP3:
				td	+=Cam->Width;
				tz	+=Cam->Width;
				zai	+=dz;
				c++;
				if(g >= 0)
				{
					r++;
					g	+=inc2;
					td++;
					tz++;
					if(c > f)
					{
						goto	EXIT_LOOP3;
					}
					if(*tz > zai)
					{
						*td	=color;
						*tz	=zai;
					}
					zai	+=dz;
				}
				else
				{
					g	+=inc1;
					if(c > f)
					{
						goto	EXIT_LOOP3;
					}
					if(*tz > zai)
					{
						*td	=color;
						*tz	=zai;
					}
					zai	+=dz;
				}
				goto	LOOP3;
EXIT_LOOP3:
				;
			}
			return;
		}
		else
		{
			if(c <= f)
			{
				if(*tz > zai)
				{
					*td	=color;
					*tz	=zai;
				}
LOOP4:
				td	+=Cam->Width;
				tz	+=Cam->Width;
				zai	+=dz;
				c++;
				if(g >= 0)
				{
					r--;
					g	+=inc2;
					td--;
					tz--;
					if(c > f)
					{
						goto	EXIT_LOOP4;
					}
					if(*tz > zai)
					{
						*td	=color;
						*tz	=zai;
					}
					zai	+=dz;
				}
				else
				{
					g	+=inc1;
					if(c > f)
					{
						goto EXIT_LOOP4;
					}
					if(*tz > zai)
					{
						*td	=color;
						*tz	=zai;
					}
					zai	+=dz;
				}
				goto	LOOP4;
EXIT_LOOP4:
				;
			}
		}
	}
}

//standard error term line draw
static void	Render_Line(int xa, int ya,
					   int xb, int yb,
					   uint16 color, ViewVars *Cam)
{
	int		dx, dy;
	short	pos;
	int		g, r, c, inc1, inc2, f;
	uint16	*td;
	
	dy	=yb - ya;
	if(dy==0)
	{
		//Horizontal lines
		if(xb < xa)
		{
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xb;
			for(;xb <= xa;xb++)
			{
				*td	=color;
				td++;
			}
		}
		else
		{
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xa;
			for(;xa <= xb;xa++)
			{
				*td	=color;
				td++;
			}
		}
		return;
	}

	dx	=xb - xa;
	if(dx==0)
	{
		//Vertical Lines
		if (yb < ya)
		{
			td	=((uint16 *)Cam->pBits) + (yb * Cam->Width) + xb;
			for(;yb <= ya;yb++)
			{
				*td	=color;
				td	+=Cam->Width;
			}
		}
		else
		{
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xb;
			for(;ya <= yb;ya++)
			{
				*td	=color;
				td	+=Cam->Width;
			}
		}
		return;
	}
	if(dx > 0)
	{
		if(dy < 0)
		{
			pos	=0;
		}
		else
		{
			pos	=1;
		}
	}
	else
	{
		if(dy < 0)
		{
			pos	=1;
		}
		else
		{
			pos	=0;
		}
	}
	if(abs(dx) > abs(dy))
	{
		if(dx > 0)
		{
			c	=xa;
			r	=ya;
			f	=xb;
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xa;
		}
		else
		{
			c	=xb;
			r	=yb;
			f	=xa;
			td	=((uint16 *)Cam->pBits) + (yb * Cam->Width) + xb;
		}
		inc1=abs(dy + dy);
		g	=abs(dy) * 2 - abs(dx);
		inc2=(abs(dy) - abs(dx)) * 2;
		if(pos)
		{
			while(c <= f)
			{
				*td	=color;
				td++;
				c++;

				if(g >= 0)
				{
					r++;
					g	+=inc2;
					td	+=Cam->Width;
				}
				else
				{
					g	+=inc1;
				}
			}
			return;
		}
		else
		{
			while(c <= f)
			{
				*td	=color;
				c++;
				td++;

				if(g > 0)
				{
					r--;
					g	+=inc2;
					td	-=Cam->Width;
				}
				else
				{
					g	+=inc1;
				}
			}
			return;
		}
	}
	else
	{
		if(dy > 0)
		{
			c	=ya;
			r	=xa;
			f	=yb;
			td	=((uint16 *)Cam->pBits) + (ya * Cam->Width) + xa;
		}
		else
		{
			c	=yb;
			r	=xb;
			f	=ya;
			td	=((uint16 *)Cam->pBits) + (yb * Cam->Width) + xb;
		}
		inc1	=abs(dx + dx);
		g		=abs(dx) * 2 - abs(dy);
		inc2	=(abs(dx) - abs(dy)) * 2;
		if(pos)
		{
			if(c <= f)
			{
				*td	=color;
LOOP3:
				td	+=Cam->Width;
				c++;
				if(g >= 0)
				{
					r++;
					g	+=inc2;
					td++;
					if(c > f)
					{
						goto	EXIT_LOOP3;
					}
					*td	=color;
				}
				else
				{
					g	+=inc1;
					if(c > f)
					{
						goto	EXIT_LOOP3;
					}
					*td	=color;
				}
				goto	LOOP3;
EXIT_LOOP3:
				;
			}
			return;
		}
		else
		{
			if(c <= f)
			{
				*td	=color;
LOOP4:
				td	+=Cam->Width;
				c++;
				if(g >= 0)
				{
					r--;
					g	+=inc2;
					td--;
					if(c > f)
					{
						goto	EXIT_LOOP4;
					}
					*td	=color;
				}
				else
				{
					g	+=inc1;
					if(c > f)
					{
						goto EXIT_LOOP4;
					}
					*td	=color;
				}
				goto	LOOP4;
EXIT_LOOP4:
				;
			}
		}
	}
}

void	Render_BlitViewDib(ViewVars *v, HDC ViewDC)
{
	if(StretchDIBits(ViewDC, 0, 0, v->Width, v->Height,
					 0, 0, v->Width, v->Height, v->pBits, (BITMAPINFO *)&v->ViewBMI,
					 DIB_RGB_COLORS, SRCCOPY)==GDI_ERROR)
	{
		ConPrintf("Could not blit to screen!");
		assert(0);
	}
}

static void ClearZBuffer(ViewVars *Cam)
{
	memset(Cam->pZBuffer, 0, (sizeof(uint32) * (Cam->Width*Cam->Height)));
}

void	Render_RenderSpanFaceCB(Face *f, const Face *OGFace, int Key, geFloat pdist, int RFlags, void *pVoid)
{
	int				i;
	RenderFace		TempFace, Clipped, Unclipped;
	const geVec3d	*pnts;
	ViewVars		*Cam	=(ViewVars *)pVoid;

	if(Face_IsVisible(f))
	{
		if(pdist > ON_EPSILON)
		{
			if(ClipToFrustum(f, &TempFace, Cam))
			{
				pnts	=Face_GetPoints(f);
				for(i=0;i < TempFace.NumPoints;i++)
				{
					Clipped.Points[i]	=Render_XFormVert(Cam, &TempFace.Points[i]);
				}
				for(i=0;i < Face_GetNumPoints(f);i++)
				{
					Unclipped.Points[i]	=Render_XFormVert(Cam, &pnts[i]);
				}

				Clipped.NumPoints	=TempFace.NumPoints;
				Unclipped.NumPoints	=Face_GetNumPoints(f); //could use i here but unclear/risky

				AddNodeEdges(f, OGFace, &Clipped, &Unclipped, Key, Cam, RFlags);
			}
		}
		else if(Face_IsSheet(f))
		{
			if(ClipToFrustum(f, &TempFace, Cam))
			{
				pnts	=Face_GetPoints(f);
				for(i=0;i < TempFace.NumPoints;i++)
				{
					Clipped.Points[TempFace.NumPoints-(i+1)]	=Render_XFormVert(Cam, &TempFace.Points[i]);
				}

				Unclipped.NumPoints	=Face_GetNumPoints(f); //could use i here but unclear/risky
				Clipped.NumPoints	=TempFace.NumPoints;

				for(i=0;i < Face_GetNumPoints(f);i++)
				{
					Unclipped.Points[Unclipped.NumPoints-(i+1)]	=Render_XFormVert(Cam, &pnts[i]);
				}

				AddNodeEdges(f, OGFace, &Clipped, &Unclipped, Key, Cam, RFlags | FLIPPED);
			}
		}
	}
}

void Render_ClearViewDib (ViewVars *Cam)
{
	memset(Cam->pBits, 0, Cam->Width * Cam->Height *2);
	ClearZBuffer(Cam);  //TODO: vtune this, might march
}

void	Render_RenderTree(ViewVars *Cam, Node *n, HDC ViewDC, int RFlags)
{
    ClearEdgeLists(Cam);

    pAvailSurf	=surfz;
    pAvailEdge	=edgez;

	if(RFlags & ZFILL)
	{
		Render_ClearViewDib (Cam);
	}

	Node_EnumTreeFaces(n, &Cam->CamPos, RFlags, (void *)Cam, Render_RenderSpanFaceCB);

	if(RFlags & ZFILL)
	{
		FillBackSpans(Cam);
	}

	ScanEdges(Cam);
	DrawSpans(Cam, ViewDC);
}

static void AddNodeEdges(Face *NodeFace,	//node face
						 const Face *OGFace,//original brush face
						 RenderFace *sf,	//screenspace clipped
						 RenderFace *sfnc,	//screenspace unclipped
						 uint32 Key,		//bspsortkey
						 ViewVars *Cam,		//viewvars
						 int RFlags)		//render flags
{
	Plane		SPlane;
	geVec3d		tview, tworld;
	int			i, nextvert, topy, bottomy, height, temp, DibId = 0;
	int			NFaceNumPoints = 0;
	geFloat		deltax, deltay, slope, psfov, dinv, zinv;
	geFloat		aUOverZ, aVOverZ, dzinv, u, v;
	SpanSurface	*spsf;
    Edge		*pedge;
	geBoolean	GradNeeded, Sheet;

	TexInfo_Vectors const	*TVecs = NULL;
	const geVec3d			*NFacePnts = NULL;
	const Plane				*NPlane = NULL;

	spsf	=&SpanFaces[Cam->FacesDone];
	Sheet	=Face_IsSheet(NodeFace);

	if(OGFace)
	{
		DibId			=Face_GetTextureDibId(OGFace);
		TVecs			=Face_GetTextureVecs(OGFace);
		GradNeeded		=(DibId!=0xffff)&&(!Face_IsSky(OGFace));
		NPlane			=Face_GetPlane(OGFace);
		NFacePnts		=Face_GetPoints(NodeFace);
		NFaceNumPoints	=Face_GetNumPoints(NodeFace);
	}
	else
	{
		GradNeeded	=FALSE;
	}

	//TODO: clamp sf to window limits

	for(i=0;i < sf->NumPoints;i++)
	{
		assert(sf->Points);
		nextvert=(i+1)%sf->NumPoints;
		topy	=(int)ceil(sf->Points[i].Y);
		bottomy	=(int)ceil(sf->Points[nextvert].Y);
		height	=bottomy-topy;
		if(height==0)
			continue;	//doesn't cross any scan lines
		
		if(height < 0)	//leading edge
		{
			temp	=topy;
			topy	=bottomy;
			bottomy	=temp;

			pAvailEdge->leading=1;
			deltax	= sf->Points[i].X -sf->Points[nextvert].X;
			deltay	= sf->Points[i].Y -sf->Points[nextvert].Y;
			slope	= deltax/deltay;
			
			pAvailEdge->xstep=(int)(slope * (geFloat)0x10000);
			pAvailEdge->x=(int)((sf->Points[nextvert].X +
				((geFloat)topy - sf->Points[nextvert].Y) * slope) * (geFloat)0x10000);
		}
		else
		{
			pAvailEdge->leading=0;

			deltax	= sf->Points[nextvert].X -sf->Points[i].X;
			deltay	= sf->Points[nextvert].Y -sf->Points[i].Y;
			slope	= deltax/deltay;
			pAvailEdge->xstep = (int)(slope * (geFloat)0x10000);
			pAvailEdge->x=(int)((sf->Points[i].X +((geFloat)topy - sf->Points[i].Y)
				* slope) *(geFloat)0x10000);
		}
		pedge	=&Cam->NewEdges[topy];
		while(pedge->pnext->x < pAvailEdge->x)
			pedge	=pedge->pnext;

		pAvailEdge->pnext	=pedge->pnext;
		pedge->pnext		=pAvailEdge;

		pAvailEdge->pnextremove			=Cam->RemoveEdges[bottomy - 1];
		Cam->RemoveEdges[bottomy - 1]	=pAvailEdge;
		pAvailEdge->psurf				=pAvailSurf;

		if(pAvailEdge < &edgez[MAX_EDGES])
			pAvailEdge++;
	}

	spsf->sizes.ScreenWidth	=Cam->Width;
	spsf->sizes.ScreenHeight=Cam->Height;

	if(GradNeeded)	//texture gradient needed
	{
		spsf->sizes.TexWidth	=Cam->WadSizes[DibId].TexWidth;
		spsf->sizes.TexHeight	=Cam->WadSizes[DibId].TexHeight;
		spsf->sizes.TexData		=Cam->WadSizes[DibId].TexData;

		//rotate the faces plane into the view
		SPlane.Normal.X	=geVec3d_DotProduct(&NPlane->Normal, &Cam->Vright);
		SPlane.Normal.Y	=geVec3d_DotProduct(&NPlane->Normal, &Cam->Vup);
		SPlane.Normal.Z	=geVec3d_DotProduct(&NPlane->Normal, &Cam->Vpn);
		SPlane.Dist		=NPlane->Dist - geVec3d_DotProduct(&Cam->CamPos, &NPlane->Normal);

		//calc 1/z gradients
		psfov					=Cam->MaxScreenScaleInv*(Cam->FieldOfView/2.0f);
		dinv					=1.0f/SPlane.Dist;
		spsf->Grads.dOneOverZdX	=-SPlane.Normal.X*dinv*psfov;
		spsf->zinvstepy			=-SPlane.Normal.Y*dinv*psfov;
//		spsf->zinv00			=SPlane.Normal.Z*dinv-Cam->XCenter*spsf->Grads.dOneOverZdX
//									-Cam->YCenter*spsf->zinvstepy;

		spsf->zinv00			=SPlane.Normal.Z*dinv
									-Cam->XCenter*spsf->Grads.dOneOverZdX
									-Cam->YCenter*spsf->zinvstepy;

		//use the worldspace face to get a starting U and V
		//if faces are flipped, clipped index zero is world nv-1
		if(RFlags & FLIPPED)
		{
			u	=geVec3d_DotProduct(&NFacePnts[NFaceNumPoints-1], &TVecs->uVec) + TVecs->uOffset;
			v	=geVec3d_DotProduct(&NFacePnts[NFaceNumPoints-1], &TVecs->vVec) + TVecs->vOffset;
		}
		else
		{
			u	=geVec3d_DotProduct(&NFacePnts[0], &TVecs->uVec) + TVecs->uOffset;
			v	=geVec3d_DotProduct(&NFacePnts[0], &TVecs->vVec) + TVecs->vOffset;
		}

		//unclipped screen face will have the same zero index
		zinv	=1.0f / sfnc->Points[0].Z;
		aUOverZ =u *zinv;
		aVOverZ =v *zinv;

		//step 256 pixels in screen x to get u v z deltas
		dzinv=(zinv+spsf->Grads.dOneOverZdX*(256.0f));

		//step point zero 256 in screen x and unproject back to view
//		tview.X	=(sfnc->Points[0].X +256.0f -Cam->XCenter)/(Cam->MaxScale*dzinv);
		tview.X	=(sfnc->Points[0].X +256.0f -Cam->XCenter)/(Cam->MaxScale*-dzinv);
		tview.Y	=(sfnc->Points[0].Y -Cam->YCenter)/(Cam->MaxScale*-dzinv);
		tview.Z	=1.0f / dzinv;

		//rotate the point back to worldspace
		Render_BackRotateVector(Cam, &tview, &tworld);
		geVec3d_Add(&tworld, &Cam->CamPos, &tworld);

		//grab x deltas from the new point for u and v
		u	=geVec3d_DotProduct(&tworld, &TVecs->uVec) + TVecs->uOffset;
		v	=geVec3d_DotProduct(&tworld, &TVecs->vVec) + TVecs->vOffset;
		spsf->Grads.dUOverZdX=(dzinv*u -aUOverZ)/256.0f;
		spsf->Grads.dVOverZdX=(dzinv*v -aVOverZ)/256.0f;

		//step 256 pixels in screen y to get u v z deltas
		dzinv=(zinv+spsf->zinvstepy*(256.0f));

		//step point zero 256 in screen y and unproject back to view
//		tview.X	=(sfnc->Points[0].X -Cam->XCenter)/(Cam->MaxScale*dzinv);
		tview.X	=(sfnc->Points[0].X -Cam->XCenter)/(Cam->MaxScale*-dzinv);
		tview.Y	=(sfnc->Points[0].Y +256.0f -Cam->YCenter)/(Cam->MaxScale*-dzinv);
		tview.Z	=1.0f / dzinv;

		//rotate the point back to worldspace
		Render_BackRotateVector(Cam, &tview, &tworld);
		geVec3d_Add(&tworld, &Cam->CamPos, &tworld);

		//grab y deltas from the new point for u and v
		u	=geVec3d_DotProduct(&tworld, &TVecs->uVec) + TVecs->uOffset;
		v	=geVec3d_DotProduct(&tworld, &TVecs->vVec) + TVecs->vOffset;
		spsf->zinvustepy=(dzinv*u -aUOverZ)/256.0f;
		spsf->zinvvstepy=(dzinv*v -aVOverZ)/256.0f;

		//calculate u/z and v/z at screen 0,0
		spsf->zinvu00=aUOverZ -
				sfnc->Points[0].X * spsf->Grads.dUOverZdX -
				sfnc->Points[0].Y * spsf->zinvustepy;

		spsf->zinvv00=aVOverZ -
				sfnc->Points[0].X * spsf->Grads.dVOverZdX -
				sfnc->Points[0].Y * spsf->zinvvstepy;

		//these can be added to a span subdiv with no mul
		spsf->Grads.dOneOverZdX16	= spsf->Grads.dOneOverZdX * 16.0f;
		spsf->Grads.dUOverZdX16		= spsf->Grads.dUOverZdX * 16.0f;
		spsf->Grads.dVOverZdX16		= spsf->Grads.dVOverZdX * 16.0f;
	}
	else	//non textured
	{
		spsf->sizes.TexWidth	=0;
		spsf->sizes.TexHeight	=0;
		spsf->sizes.TexData		=NULL;
	}

	spsf->head			=NULL;
	spsf->cur			=NULL;
    pAvailSurf->state	=0;
	pAvailSurf->sfIdx	=Cam->FacesDone++;
	pAvailSurf->Key		=Key;
	pAvailSurf->RFlag	=RFlags;
	if(OGFace)
	{
		pAvailSurf->color	=(Face_IsSky(OGFace))? 0x2bf: (0x00b0b0b0-Key);
	}
	else
	{
		pAvailSurf->color	=(0x00b0b0b0-Key);
	}

    //make sure the surface array isn't full
    if(pAvailSurf < &surfz[MAX_SURFS])
		pAvailSurf++;
}

//this turns the edge list into spans sorted by texture
//the inner calculations of left and right u and v over z
//could really use some assembly.  I'll save that for later
//pollard has MUCH better span code, I need to implement it here
static void ScanEdges(ViewVars *Cam)
{
	int			x, y, zinv, zinv2;
	Edge		*pedge, *pedge2, *ptemp;
	Span		*pspan;
	Surf		*psurf, *psurf2;
	SpanSurface *sstemp;

	pspan = spanz;

	edgeHead.pnext	=&edgeTail;
	edgeHead.pprev	=NULL;
	edgeHead.x		=-0xFFFF;			// left edge of screen
	edgeHead.leading=1;
	edgeHead.psurf	=&SurfStack;

	edgeTail.pnext	=NULL;				// mark edge of list
	edgeTail.pprev	=&edgeHead;
	edgeTail.x		=Cam->Width << 16;	// right edge of screen
	edgeTail.leading=0;
	edgeTail.psurf	=&SurfStack;

	SurfStack.pnext=SurfStack.pprev=&SurfStack;
	SurfStack.color=0;
	SurfStack.Key=-999999;

	for(y=0;y < Cam->Height;y++)
	{
		pedge	=Cam->NewEdges[y].pnext;
		pedge2	=&edgeHead;
		while(pedge!=&MaxEdge)
		{
			while(pedge->x > pedge2->pnext->x)
			{
				pedge2=pedge2->pnext;
			}

			ptemp = pedge->pnext;
			pedge->pnext = pedge2->pnext;
			pedge->pprev = pedge2;
			pedge2->pnext->pprev = pedge;
			pedge2->pnext = pedge;

			pedge2 = pedge;
			pedge = ptemp;
		}
		SurfStack.state = 1;
		SurfStack.visxstart = 0;

		for(pedge=edgeHead.pnext;pedge;pedge=pedge->pnext)
		{
			psurf = pedge->psurf;
			if(pedge->leading)
			{
				if(++psurf->state==1)
				{
					zinv	=psurf->Key;
					psurf2	=SurfStack.pnext;
					zinv2	=psurf2->Key;
					if(zinv >= zinv2)
					{
						x=(pedge->x+0xFFFF)>>16;
						pspan->count=x-psurf2->visxstart;
						if(pspan->count > 0)
						{
							pspan->y		=y;
							pspan->x		=psurf2->visxstart;
							pspan->RFlag	=psurf2->RFlag;
							pspan->color	=psurf2->color;
							sstemp			=&SpanFaces[psurf2->sfIdx];
							if(!sstemp->head)
							{
								sstemp->head	=pspan;
							}
							else if(sstemp->cur)
							{
								sstemp->cur->next	=pspan;
							}
							pspan->next	=NULL;
							sstemp->cur	=pspan;
							if(pspan < &spanz[MAX_SPANS])
								pspan++;
						}
						psurf->visxstart	=x;
						psurf->pnext		=psurf2;
						psurf2->pprev		=psurf;
						SurfStack.pnext		=psurf;
						psurf->pprev		=&SurfStack;
					}
					else
					{
						do
						{
							psurf2	=psurf2->pnext;
							zinv2	=psurf2->Key;
						} while(zinv < zinv2);
						psurf->pnext		=psurf2;
						psurf->pprev		=psurf2->pprev;
						psurf2->pprev->pnext=psurf;
						psurf2->pprev		=psurf;
					}
				}
			}
			else
			{
				if(--psurf->state==0)
				{
					if(SurfStack.pnext==psurf)
					{
						x=((pedge->x+0xFFFF)>>16);
						pspan->count=x-psurf->visxstart;
						if(pspan->count > 0)
						{
							pspan->y		=y;
							pspan->x		=psurf->visxstart;
							pspan->color	=psurf->color;
							pspan->RFlag	=psurf->RFlag;
							sstemp			=&SpanFaces[psurf->sfIdx];
							if(!sstemp->head)
							{
								sstemp->head	=pspan;
							}
							else if(sstemp->cur)
							{
								sstemp->cur->next	=pspan;
							}
							pspan->next	=NULL;
							sstemp->cur	=pspan;
							if(pspan < &spanz[MAX_SPANS])
								pspan++;
						}
						psurf->pnext->visxstart=x;
					}
					psurf->pnext->pprev	=psurf->pprev;
					psurf->pprev->pnext	=psurf->pnext;
				}
			}
		}
		pedge = Cam->RemoveEdges[y];
		while(pedge)
		{
			pedge->pprev->pnext	=pedge->pnext;
			pedge->pnext->pprev	=pedge->pprev;
			pedge				=pedge->pnextremove;
		}
		for(pedge=edgeHead.pnext;pedge!=&edgeTail; )
		{
			ptemp	= pedge->pnext;
			pedge->x+=pedge->xstep;
			while(pedge->x < pedge->pprev->x)
			{
				pedge2				=pedge->pprev;
				pedge2->pnext		=pedge->pnext;
				pedge->pnext->pprev	=pedge2;
				pedge2->pprev->pnext=pedge;
				pedge->pprev		=pedge2->pprev;
				pedge->pnext		=pedge2;
				pedge2->pprev		=pedge;
			}
			pedge=ptemp;
		}
	}
	pspan->x=-1;	// mark the end of the list
}

uint16		FPUCW, OldFPUCW;

void SetFPU24(void)
{
	_asm
	{
		fstcw	[OldFPUCW]		; store copy of CW
		mov		ax, OldFPUCW	; get it in ax
		and		eax,0xFFFFFCFF
		mov		[FPUCW],ax		
		fldcw	[FPUCW]			; load the FPU
	}
}

void RestoreFPU(void)
{
	_asm	fldcw	[OldFPUCW]	; restore the FPU
}

#pragma warning (disable:4100)
static void DrawSpans(ViewVars *v, HDC ViewDC)
{
	long		i, w;
	EdgeAsm		left, right;
	SpanSurface	*sstemp;

	if(!v->FacesDone)
		return;

	SetFPU24();

	for(i=0;i < v->FacesDone;i++)
	{
		if(SpanFaces[i].head==NULL)
			continue;

		sstemp						=&SpanFaces[i];
		sstemp->cur					=sstemp->head;
		sstemp->sizes.ScreenData	=(unsigned char *)v->pBits;
		sstemp->sizes.ScreenWidth	=v->Width;
		sstemp->sizes.ZData			=v->pZBuffer;
		while(sstemp->cur)
		{
			if(sstemp->cur==NULL)
				continue;

			left.X			=sstemp->cur->x;
			left.XStep		=sstemp->cur->x;  //take this out
			left.Y			=sstemp->cur->y;

			right.X			=sstemp->cur->x+sstemp->cur->count;
			right.XStep		=sstemp->cur->x;  //take this out
			right.Y			=sstemp->cur->y;
			if(sstemp->sizes.TexData !=NULL)
			{
				left.OneOverZ	=sstemp->zinv00 +
					sstemp->Grads.dOneOverZdX * (geFloat)sstemp->cur->x+
					sstemp->zinvstepy * (geFloat)sstemp->cur->y;

				left.UOverZ		=sstemp->zinvu00 +
					sstemp->Grads.dUOverZdX * (geFloat)sstemp->cur->x+
					sstemp->zinvustepy * (geFloat)sstemp->cur->y;

				left.VOverZ		=sstemp->zinvv00 +
					sstemp->Grads.dVOverZdX * (geFloat)sstemp->cur->x+
					sstemp->zinvvstepy * (geFloat)sstemp->cur->y;
				
				right.OneOverZ	=sstemp->zinv00 +
					sstemp->Grads.dOneOverZdX * (geFloat)(sstemp->cur->x+sstemp->cur->count)+
					sstemp->zinvstepy * (geFloat)sstemp->cur->y;

				right.UOverZ	=sstemp->zinvu00 +
					sstemp->Grads.dUOverZdX * (geFloat)(sstemp->cur->x+sstemp->cur->count)+
					sstemp->zinvustepy * (geFloat)sstemp->cur->y;

				right.VOverZ	=sstemp->zinvv00 +
					sstemp->Grads.dVOverZdX * (geFloat)(sstemp->cur->x+sstemp->cur->count)+
					sstemp->zinvvstepy * (geFloat)sstemp->cur->y;

				if(sstemp->sizes.TexWidth > sstemp->sizes.TexHeight)
					w=sstemp->sizes.TexWidth;
				else
					w=sstemp->sizes.TexHeight;
			}
			else
				w=69; //force nontextured

			if(sstemp->cur->RFlag & NORMAL)
			{
				switch(w)
				{
				case 256:
					DrawScanLine256(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 128:
					DrawScanLine128(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 64:
					DrawScanLine64(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 32:
					DrawScanLine32(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 16:
					DrawScanLine16(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				default:
					memset((sstemp->sizes.ScreenData+((v->Width*left.Y)+left.X)*2),
						sstemp->cur->color, sstemp->cur->count*2);
					break;
				}
			}
			else if(sstemp->cur->RFlag & ZFILL)
			{
				switch(w)
				{
				case 256:
					DrawScanLine256_ZFill(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 128:
					DrawScanLine128_ZFill(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 64:
					DrawScanLine64_ZFill(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 32:
					DrawScanLine32_ZFill(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 16:
					DrawScanLine16_ZFill(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				default:
					memset((sstemp->sizes.ScreenData+((v->Width*left.Y)+left.X)*2),
						sstemp->cur->color, sstemp->cur->count*2);
					break;
				}
			}
			else if(sstemp->cur->RFlag & ZBUFFER)
			{
				switch(w)
				{
				case 256:
					DrawScanLine256_ZBuf(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 128:
					DrawScanLine128_ZBuf(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 64:
					DrawScanLine64_ZBuf(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 32:
					DrawScanLine32_ZBuf(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				case 16:
					DrawScanLine16_ZBuf(&sstemp->sizes, &sstemp->Grads, &left, &right);
					break;
				default:
					memset((sstemp->sizes.ScreenData+((v->Width*left.Y)+left.X)*2),
						sstemp->cur->color, sstemp->cur->count*2);
					break;
				}
			}
			sstemp->cur		=sstemp->cur->next;
		}
	}
	RestoreFPU();
}
#pragma warning (default:4100)

void DrawScanLine256(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return256				

		mov		eax,pSizeInfo
		mov		edi,[eax]SizeInfo.TexData
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi
		mov		edi,esi					
		mov		esi,[eax]SizeInfo.ScreenWidth
		mov		edx,[ebx]EdgeAsm.Y		
		shl		esi,1
		imul	edx,esi					
		mov		eax,[ebx]EdgeAsm.X
		shl		eax,1
		add		edi,edx					
		add		edi,eax

		
		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; short jump emit
		dec		ecx						
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi,pTex
		mov		eax,pSizeInfo

		mov		ebx,pLeft
		mov		edx,pGradients

										; st0  st1  st2  st3  st4  st5  st6  st7
		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 
		fld		st						; ZL   ZL   1/ZL U/ZL V/ZL 
		fmul	st,st(4)				; VL   ZL   1/ZL U/ZL V/ZL 
		fxch	st(1)					; ZL   VL   1/ZL U/ZL V/ZL 
		fmul	st,st(3)				; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)					; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)					; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)					; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)					; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jz		HandleLeftoverPixels256

SpanLoop256:
		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)					; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)					; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)					; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)					; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,8
		mov		esi,pTex

		shr		ebx,16
		and		eax,0000ff00h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,0ffh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		add		edx,ebp
		mov		ax,[2*esi]

		mov		esi,edx
		add		ebx,ecx

		shl		esi,8
		and		ebx,00ffffffh

		and		esi,0ff000000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,ecx

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+2],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+4],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+6],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+8],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+10],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+12],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+14],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+16],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+18],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+20],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+22],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+24],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+26],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+28],ax				

		shl		esi,8
		add		edx,ebp

		and		esi,0ff000000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		edi,32					
		dec		[NumASpans]			
		jnz		SpanLoop256				

HandleLeftoverPixels256:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn256			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan256			

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan256:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			


LeftoverLoop256:
		mov		eax,ecx					
		sar		eax,8					
		mov		edx,ebx
		and		eax,0ff00h
		sar		edx,16					
		and		edx,0ffh
		add		eax,edx					
		add		eax,esi
		mov		ax,[2*eax]				
		mov		[edi],ax				
		add		ebx,DeltaU				
		add		edi,2
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop256			

FPUReturn256:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return256:
	}		
}

void DrawScanLine128(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return128				

		mov		eax,pSizeInfo
		mov		edi,[eax]SizeInfo.TexData
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi
		mov		edi,esi					
		mov		esi,[eax]SizeInfo.ScreenWidth
		mov		edx,[ebx]EdgeAsm.Y		
		shl		esi,1
		imul	edx,esi					
		mov		eax,[ebx]EdgeAsm.X
		shl		eax,1
		add		edi,edx					
		add		edi,eax

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; short jump emit
		dec		ecx						
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi,pTex
		mov		eax,pSizeInfo

		mov		ebx,pLeft
		mov		edx,pGradients

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 
		fld		st						; ZL   ZL   1/ZL U/ZL V/ZL 
		fmul	st,st(4)				; VL   ZL   1/ZL U/ZL V/ZL 
		fxch	st(1)					; ZL   VL   1/ZL U/ZL V/ZL 
		fmul	st,st(3)				; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)					; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)					; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)					; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)					; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jz		HandleLeftoverPixels128

SpanLoop128:
		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)					; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)					; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)					; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)					; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,9
		mov		esi,pTex

		shr		ebx,16
		and		eax,00003f80h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,07fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		add		edx,ebp
		mov		ax,[2*esi]

		mov		esi,edx
		add		ebx,ecx

		shl		esi,7
		and		ebx,007fffffh

		and		esi,03f800000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,ecx

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+2],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+4],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+6],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+8],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+10],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+12],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+14],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+16],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+18],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+20],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+22],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+24],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+26],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+28],ax				

		shl		esi,7
		add		edx,ebp

		and		esi,03f800000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		edi,32					
		dec		[NumASpans]			
		jnz		SpanLoop128				

HandleLeftoverPixels128:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn128			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan128			

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan128:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop128:
		mov		eax,ecx					
		shr		eax,9					
		mov		edx,ebx
		and		eax,03f80h
		shr		edx,16					
		and		edx,07fh
		add		eax,edx					
		add		eax,esi
		mov		ax,[2*eax]				
		mov		[edi],ax				
		add		ebx,DeltaU				
		add		edi,2
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop128			

FPUReturn128:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return128:
	}		
}

void DrawScanLine64(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return64				

		mov		eax,pSizeInfo
		mov		edi,[eax]SizeInfo.TexData
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi
		mov		edi,esi					
		mov		esi,[eax]SizeInfo.ScreenWidth
		mov		edx,[ebx]EdgeAsm.Y		
		shl		esi,1
		imul	edx,esi					
		mov		eax,[ebx]EdgeAsm.X
		shl		eax,1
		add		edi,edx					
		add		edi,eax

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; short jump emit
		dec		ecx						
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi,pTex
		mov		eax,pSizeInfo

		mov		ebx,pLeft
		mov		edx,pGradients

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 
		fld		st						; ZL   ZL   1/ZL U/ZL V/ZL 
		fmul	st,st(4)				; VL   ZL   1/ZL U/ZL V/ZL 
		fxch	st(1)					; ZL   VL   1/ZL U/ZL V/ZL 
		fmul	st,st(3)				; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)					; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)					; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)					; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)					; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jz		HandleLeftoverPixels64

SpanLoop64:
		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)					; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)					; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)					; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)					; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,10
		mov		esi,pTex

		shr		ebx,16
		and		eax,00000fc0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,03fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		add		edx,ebp
		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,6
		and		ebx,003fffffh

		and		esi,00fc00000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,ecx

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+2],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+4],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+6],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+8],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+10],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+12],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+14],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+16],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+18],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+20],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+22],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+24],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+26],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+28],ax				

		shl		esi,6
		add		edx,ebp

		and		esi,00fc00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		edi,32					
		dec		[NumASpans]			
		jnz		SpanLoop64				

HandleLeftoverPixels64:

		mov		esi,pTex


		cmp		[RemainingCount],0		
		jz		FPUReturn64			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan64			

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan64:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop64:
		mov		eax,ecx					
		shr		eax,10					
		mov		edx,ebx
		and		eax,00fc0h
		shr		edx,16					
		and		edx,03fh
		add		eax,edx					
		add		eax,esi
		mov		ax,[2*eax]				
		mov		[edi],ax				
		add		ebx,DeltaU				
		add		edi,2
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop64			

FPUReturn64:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return64:
	}		
}

void DrawScanLine32(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return32				

		mov		eax,pSizeInfo
		mov		edi,[eax]SizeInfo.TexData
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi
		mov		edi,esi					
		mov		esi,[eax]SizeInfo.ScreenWidth
		mov		edx,[ebx]EdgeAsm.Y		
		shl		esi,1
		imul	edx,esi					
		mov		eax,[ebx]EdgeAsm.X
		shl		eax,1
		add		edi,edx					
		add		edi,eax

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; short jump emit
		dec		ecx						
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi,pTex
		mov		eax,pSizeInfo

		mov		ebx,pLeft
		mov		edx,pGradients

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 
		fld		st						; ZL   ZL   1/ZL U/ZL V/ZL 
		fmul	st,st(4)				; VL   ZL   1/ZL U/ZL V/ZL 
		fxch	st(1)					; ZL   VL   1/ZL U/ZL V/ZL 
		fmul	st,st(3)				; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)					; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)					; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)					; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)					; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jz		HandleLeftoverPixels32

SpanLoop32:
		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)					; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)					; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)					; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)					; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,11
		mov		esi,pTex

		shr		ebx,16
		and		eax,000003e0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,01fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		add		edx,ebp
		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,5
		and		ebx,001fffffh

		and		esi,003e00000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,ecx

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+2],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+4],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+6],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+8],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+10],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+12],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+14],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+16],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+18],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+20],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+22],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+24],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+26],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+28],ax				

		shl		esi,5
		add		edx,ebp

		and		esi,003e00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		edi,32					
		dec		[NumASpans]			
		jnz		SpanLoop32				

HandleLeftoverPixels32:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn32			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan32			

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan32:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop32:
		mov		eax,ecx					
		shr		eax,11					
		mov		edx,ebx
		and		eax,003e0h
		shr		edx,16					
		and		edx,01fh
		add		eax,edx					
		add		eax,esi
		mov		ax,[2*eax]				
		mov		[edi],ax				
		add		ebx,DeltaU				
		add		edi,2
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop32			

FPUReturn32:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return32:
	}		
}

void DrawScanLine16(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return16				

		mov		eax,pSizeInfo
		mov		edi,[eax]SizeInfo.TexData
		shr		edi, 1					; keep texture >>1
		mov		pTex,edi
		mov		edi,esi					
		mov		esi,[eax]SizeInfo.ScreenWidth
		mov		edx,[ebx]EdgeAsm.Y		
		shl		esi,1
		imul	edx,esi					
		mov		eax,[ebx]EdgeAsm.X
		shl		eax,1
		add		edi,edx					
		add		edi,eax

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; short jump emit
		dec		ecx						
		mov		eax,16

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi,pTex
		mov		eax,pSizeInfo

		mov		ebx,pLeft
		mov		edx,pGradients

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 
		fld		st						; ZL   ZL   1/ZL U/ZL V/ZL 
		fmul	st,st(4)				; VL   ZL   1/ZL U/ZL V/ZL 
		fxch	st(1)					; ZL   VL   1/ZL U/ZL V/ZL 
		fmul	st,st(3)				; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)					; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)					; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)					; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)					; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jz		HandleLeftoverPixels16

SpanLoop16:
		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16		; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)					; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16		; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)					; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16		; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)					; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)					; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,12
		mov		esi,pTex

		shr		ebx,16
		and		eax,000000f0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,0fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		add		edx,ebp
		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,4
		and		ebx,000fffffh

		and		esi,00f00000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		add		edx,ebp

		add		esi,pTex
		add		ebx,ecx

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+2],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+4],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+6],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+8],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+10],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+12],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+14],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+16],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+18],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+20],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+22],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+24],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+26],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+28],ax				

		shl		esi,4
		add		edx,ebp

		and		esi,00f00000h

		add		esi,ebx
		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		edi,32					
		dec		[NumASpans]			
		jnz		SpanLoop16				

HandleLeftoverPixels16:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn16			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan16			

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan16:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop16:
		mov		eax,ecx					
		shr		eax,12					
		mov		edx,ebx
		and		eax,0f0h
		shr		edx,16					
		and		edx,0fh
		add		eax,edx					
		add		eax,esi
		mov		ax,[2*eax]				
		mov		[edi],ax				
		add		ebx,DeltaU				
		add		edi,2
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop16			

FPUReturn16:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return16:
	}		
}

static void DrawScanLine256_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax, pSizeInfo
		mov		esi, [eax]SizeInfo.ScreenData	
		mov		ebx, pLeft						
		mov		eax, [ebx]EdgeAsm.X				
		mov		ecx, pRight						
		mov		ecx, [ecx]EdgeAsm.X				
		sub		ecx, eax						
		jle		Return256_ZFill						

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx


		mov		eax, ecx						
		shr		ecx, 4							
		and		eax, 15							
		_emit 75h								; short jump 6 bytes
		_emit 06h								; jnz any leftover?
		dec		ecx								
		mov		eax, 16

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
		mov		esi, pTex				; while fdiv is cooking
		mov		eax, pSizeInfo
		mov		edx, pGradients

//		fld		[edx]Gradients.dOneOverZdX	; dZ   ZL   1/ZL U/ZL V/ZL
//		fmul	[FixedScale16]				; dZ32 ZL   1/ZL U/ZL V/ZL
//		fistp	[DeltaW]					; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jnz		SpanLoop256_ZFill		; grab wleft for leftover loop
										

//		fld		st(3)					; 1/ZL UR   VR   V/ZR 1/ZR U/ZR UL   VL
//		fmul	[FixedScale16]			; W32  UR   VR   V/ZR 1/ZR U/ZR UL   VL
//		fistp	[WLeft]					; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		jmp		HandleLeftoverPixels256_ZFill

SpanLoop256_ZFill:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
//		fld		st							; 1/ZL 1/ZL V/ZR U/ZL UL   VL
//		fmul	[FixedScale16]				; W32  1/ZL V/ZR U/ZL UL   VL
//		fistp	[WLeft]						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1								; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)					; ZR   V/ZR 1/ZR U/ZR UL   VL


		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,8
		mov		esi,pTex

		shr		ebx,16
		and		eax,0000ff00h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,0ffh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,[pZBuf]

		mov		eax, [WLeft]
		add		edx,[DeltaV]

		mov		[ebp+0],eax

		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,8
		and		ebx,00ffffffh

		and		esi,0ff000000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax,[WLeft]

		add		esi,pTex
		mov		[ebp+4],eax

		add		ebx,ecx

		mov		[WLeft], eax

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+2],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+8],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+4],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+12],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+6],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+16],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+8],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+20],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+10],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+24],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+12],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+28],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+14],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+32],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+16],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+36],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+18],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+40],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+20],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+44],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+22],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+48],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+24],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+52],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+26],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+56],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,00ffffffh
		mov		[edi+28],ax				

		shl		esi,8
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,0ff000000h
		mov		[ebp+60],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		mov		ecx,pZBuf
		add		edi,32					

		add		ecx,64
		mov		pZBuf,ecx
		dec		[NumASpans]			
		jnz		SpanLoop256_ZFill		

HandleLeftoverPixels256_ZFill:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn256_ZFill			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan256_ZFill			

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan256_ZFill:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop256_ZFill:
		push	ebp
		mov		eax,ecx					

		mov		ebp,[DeltaW]

		shr		eax,8					
		add		ebp,[WLeft]

		mov		edx,ebx
		and		eax,0ff00h

		shr		edx,16					
		mov		[WLeft],ebp

		and		edx,0ffh
		add		eax,edx					

		add		eax,esi

		mov		edx,pZBuf
		mov		ax,[2*eax]			

		mov		[edx],ebp
		mov		[edi],ax				

		pop		ebp
		add		edx,4

		add		edi,2
		mov		pZBuf,edx

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop256_ZFill			

FPUReturn256_ZFill:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return256_ZFill:
	}		
}

static void DrawScanLine128_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return128_ZFill			

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; short jump emit
		dec		ecx						
		mov		eax,16

		mov		ebx, pLeft

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
		mov		esi, pTex				; while fdiv is cooking
		mov		eax, pSizeInfo
		mov		edx, pGradients

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jnz		SpanLoop128_ZFill		; grab wleft for leftover loop
										
		jmp		HandleLeftoverPixels128_ZFill

SpanLoop128_ZFill:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,9
		mov		esi,pTex

		shr		ebx,16
		and		eax,00003f80h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,07fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,[pZBuf]

		mov		eax, [WLeft]
		add		edx,[DeltaV]

		mov		[ebp+0], eax

		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,7
		and		ebx,007fffffh

		and		esi,03f800000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax,[WLeft]

		add		esi,pTex
		mov		[ebp+4], eax

		add		ebx,ecx

		mov		[WLeft], eax

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+2],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+8],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+4],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+12],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+6],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+16],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+8],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+20],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+10],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+24],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+12],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+28],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+14],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+32],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+16],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+36],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+18],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+40],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+20],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+44],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+22],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+48],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+24],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+52],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+26],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+56],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,007fffffh
		mov		[edi+28],ax				

		shl		esi,7
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,03f800000h
		mov		[ebp+60],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		mov		ecx,pZBuf
		add		edi,32					

		add		ecx,64
		mov		pZBuf,ecx
		dec		[NumASpans]			
		jnz		SpanLoop128_ZFill				

HandleLeftoverPixels128_ZFill:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn128_ZFill		

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan128_ZFill	

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan128_ZFill:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop128_ZFill:
		push	ebp
		mov		eax,ecx					

		mov		ebp,[DeltaW]

		shr		eax,9					
		add		ebp,[WLeft]

		mov		edx,ebx
		and		eax,03f80h

		shr		edx,16					
		mov		[WLeft],ebp

		and		edx,07fh
		add		eax,edx					

		add		eax,esi

		mov		edx,pZBuf
		mov		ax,[2*eax]			

		mov		[edx],ebp
		mov		[edi],ax				

		pop		ebp
		add		edx,4

		add		edi,2
		mov		pZBuf,edx

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop128_ZFill	

FPUReturn128_ZFill:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return128_ZFill:
	}		
}

static void DrawScanLine64_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return64_ZFill				

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; jnz @f any leftover?
		dec		ecx						
		mov		eax,16					

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
		mov		esi, pTex				; while fdiv is cooking
		mov		eax, pSizeInfo
		mov		edx, pGradients

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jnz		SpanLoop64_ZFill		; grab wleft for leftover loop
										
		jmp		HandleLeftoverPixels64_ZFill

SpanLoop64_ZFill:
		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,10
		mov		esi,pTex

		shr		ebx,16
		and		eax,00000fc0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,03fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,[pZBuf]

		mov		eax, [WLeft]
		add		edx,[DeltaV]

		mov		[ebp+0],eax

		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,6
		and		ebx,003fffffh

		and		esi,00fc00000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax,[WLeft]

		add		esi,pTex
		mov		[ebp+4],eax

		add		ebx,ecx

		mov		[WLeft], eax

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+2],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+8],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+4],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+12],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+6],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+16],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+8],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+20],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+10],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+24],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+12],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+28],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+14],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+32],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+16],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+36],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+18],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+40],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+20],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+44],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+22],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+48],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+24],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+52],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+26],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+56],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,003fffffh
		mov		[edi+28],ax				

		shl		esi,6
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00fc00000h
		mov		[ebp+60],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		mov		ecx,pZBuf
		add		edi,32					

		add		ecx,64
		mov		pZBuf,ecx
		dec		[NumASpans]			
		jnz		SpanLoop64_ZFill				

HandleLeftoverPixels64_ZFill:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn64_ZFill			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan64_ZFill			

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan64_ZFill:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop64_ZFill:
		push	ebp
		mov		eax,ecx					

		mov		ebp,[DeltaW]

		shr		eax,10					
		add		ebp,[WLeft]

		mov		edx,ebx
		and		eax,00fc0h

		shr		edx,16					
		mov		[WLeft],ebp

		and		edx,03fh
		add		eax,edx					

		add		eax,esi

		mov		edx,pZBuf
		mov		ax,[2*eax]			

		mov		[edx],ebp
		mov		[edi],ax				

		pop		ebp
		add		edx,4

		add		edi,2
		mov		pZBuf,edx

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop64_ZFill			

FPUReturn64_ZFill:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return64_ZFill:
	}		
}

static void DrawScanLine32_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return32_ZFill				

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; jnz @f any leftover?
		dec		ecx						
		mov		eax,16					

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
		mov		esi, pTex				; while fdiv is cooking
		mov		eax, pSizeInfo
		mov		edx, pGradients

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jnz		SpanLoop32_ZFill		; grab wleft for leftover loop
		jmp		HandleLeftoverPixels32_ZFill

SpanLoop32_ZFill:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,11
		mov		esi,pTex

		shr		ebx,16
		and		eax,000003e0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,01fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,[pZBuf]

		mov		eax, [WLeft]
		add		edx,[DeltaV]

		mov		[ebp+0],eax

		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,5
		and		ebx,001fffffh

		and		esi,003e00000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax,[WLeft]

		add		esi,pTex
		mov		[ebp+4],eax

		add		ebx,ecx

		mov		[WLeft], eax

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+2],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+8],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+4],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+12],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+6],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+16],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+8],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+20],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+10],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+24],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+12],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+28],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+14],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+32],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+16],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+36],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+18],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+40],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+20],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+44],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+22],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+48],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+24],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+52],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+26],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+56],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,001fffffh
		mov		[edi+28],ax				

		shl		esi,5
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,003e00000h
		mov		[ebp+60],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		mov		ecx,pZBuf
		add		edi,32					

		add		ecx,64
		mov		pZBuf,ecx
		dec		[NumASpans]			
		jnz		SpanLoop32_ZFill				

HandleLeftoverPixels32_ZFill:

		mov		esi,pTex

		
		cmp		[RemainingCount],0		
		jz		FPUReturn32_ZFill			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan32_ZFill			

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan32_ZFill:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop32_ZFill:
		push	ebp
		mov		eax,ecx					

		mov		ebp,[DeltaW]

		shr		eax,11					
		add		ebp,[WLeft]

		mov		edx,ebx
		and		eax,003e0h

		shr		edx,16					
		mov		[WLeft],ebp

		and		edx,01fh
		add		eax,edx					

		add		eax,esi

		mov		edx,pZBuf
		mov		ax,[2*eax]			

		mov		[edx],ebp
		mov		[edi],ax				

		pop		ebp
		add		edx,4

		add		edi,2
		mov		pZBuf,edx

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop32_ZFill			

FPUReturn32_ZFill:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return32_ZFill:
	}		
}

static void DrawScanLine16_ZFill(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	_asm
	{
		mov		eax,pSizeInfo
		mov		esi,[eax]SizeInfo.ScreenData	
		mov		ebx,pLeft				
		mov		eax,[ebx]EdgeAsm.X		
		mov		ecx,pRight				
		mov		ecx,[ecx]EdgeAsm.X		
		sub		ecx,eax					
		jle		Return16_ZFill				

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax,ecx					
		shr		ecx,4					
		and		eax,15					
		_emit 75h
		_emit 06h						; jnz @f any leftover?
		dec		ecx						
		mov		eax,16					

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax
		mov		esi, pTex				; while fdiv is cooking
		mov		eax, pSizeInfo
		mov		edx, pGradients

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		test	ecx,ecx					
		jnz		SpanLoop16_ZFill		; grab wleft for leftover loop
		jmp		HandleLeftoverPixels16_ZFill

SpanLoop16_ZFill:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,12
		mov		esi,pTex

		shr		ebx,16
		and		eax,000000f0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,0fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,[pZBuf]

		mov		eax, [WLeft]
		add		edx,[DeltaV]

		mov		[ebp+0],eax

		mov		ax,[2*esi]				

		mov		esi,edx
		add		ebx,ecx

		shl		esi,4
		and		ebx,000fffffh

		and		esi,00f00000h

		add		esi,ebx
		mov		[edi+0],ax				

		shr		esi,16
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax,[WLeft]

		add		esi,pTex
		mov		[ebp+4],eax

		add		ebx,ecx

		mov		[WLeft], eax

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+2],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+8],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+4],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+12],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+6],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+16],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+8],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+20],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+10],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+24],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+12],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+28],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+14],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+32],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+16],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+36],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+18],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+40],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+20],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+44],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+22],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+48],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+24],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+52],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+26],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+56],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		and		ebx,000fffffh
		mov		[edi+28],ax				

		shl		esi,4
		mov		eax, [DeltaW]

		add		edx,[DeltaV]
		add		eax, [WLeft]

		and		esi,00f00000h
		mov		[ebp+60],eax

		add		esi,ebx
		mov		[WLeft], eax

		shr		esi,16

		add		ebx,ecx
		add		esi,pTex

		mov		ax,[2*esi]
		mov		esi,edx

		pop     ebp						; restore access to stack frame
		mov		[edi+30],ax				

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		mov		ecx,pZBuf
		add		edi,32					

		add		ecx,64
		mov		pZBuf,ecx
		dec		[NumASpans]			
		jnz		SpanLoop16_ZFill				

HandleLeftoverPixels16_ZFill:

		mov		esi,pTex

		cmp		[RemainingCount],0		
		jz		FPUReturn16_ZFill			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan16_ZFill			

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX			; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX			; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ			; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX		; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]					; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st				; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st				; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st				; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)					; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st				; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)					; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]		; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]			; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]				; UR   inv. inv. inv. dU   VR
		fxch	st(4)					; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]		; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]			; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]				; inv. inv. inv. UR   VR

		fld		st(1)					; inv. inv. inv. inv. UR   VR
		fld		st(2)					; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan16_ZFill:
		mov		ebx,[UFixed]			
		mov		ecx,[VFixed]			

LeftoverLoop16_ZFill:
		push	ebp
		mov		eax,ecx					

		mov		ebp,[DeltaW]

		shr		eax,12					
		add		ebp,[WLeft]

		mov		edx,ebx
		and		eax,0f0h

		shr		edx,16					
		mov		[WLeft],ebp

		and		edx,0fh
		add		eax,edx					

		add		eax,esi

		mov		edx,pZBuf
		mov		ax,[2*eax]			

		mov		[edx],ebp
		mov		[edi],ax				

		pop		ebp
		add		edx,4

		add		edi,2
		mov		pZBuf,edx

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop16_ZFill			

FPUReturn16_ZFill:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return16_ZFill:
	}		
}

static void DrawScanLine256_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	SCanZ[0]=SCan[0]=(unsigned long)&can0;

	_asm
	{
		mov		eax, pSizeInfo
		mov		esi, [eax]SizeInfo.ScreenData	
		mov		ebx, pLeft						
		mov		eax, [ebx]EdgeAsm.X				
		mov		ecx, pRight						
		mov		ecx, [ecx]EdgeAsm.X				
		sub		ecx, eax						
		jle		Return256_ZBuf						

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax, ecx						
		shr		ecx, 4							
		and		eax, 15							
		_emit 75h								; short jump 6 bytes
		_emit 06h								; jnz any leftover?
		dec		ecx								
		mov		eax, 16							

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi, pTex				; while fdiv is cooking
		mov		eax,OFFSET SCanZ+4

		mov		edx,pZBuf
		mov		[eax],edx

		mov		edx, pGradients
		mov		eax, pSizeInfo

//		fld		[edx]Gradients.dOneOverZdX	; dZ   ZL   1/ZL U/ZL V/ZL
//		fmul	[FixedScale16]				; dZ32 ZL   1/ZL U/ZL V/ZL
//		fistp	[DeltaW]					; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL


		test	ecx,ecx					
		jnz		SpanLoop256_ZBuf		; grab wleft for leftover loop
										

//		fld		st(3)					; 1/ZL UR   VR   V/ZR 1/ZR U/ZR UL   VL
//		fmul	[FixedScale16]			; W32  UR   VR   V/ZR 1/ZR U/ZR UL   VL
//		fistp	[WLeft]					; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		jmp		HandleLeftoverPixels256_ZBuf

SpanLoop256_ZBuf:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
//		fld		st							; 1/ZL 1/ZL V/ZR U/ZL UL   VL
//		fmul	[FixedScale16]				; W32  1/ZL V/ZR U/ZL UL   VL
//		fistp	[WLeft]						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1								; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)					; ZR   V/ZR 1/ZR U/ZR UL   VL


		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,8
		mov		esi,pTex

		shr		ebx,16
		and		eax,0000ff00h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,0ffh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		[SCan+4],edi

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		mov		eax,[ecx+0]						;grab screen z

		add		edx,[DeltaV]					;step v
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		add		ebx,[DeltaU]					;step u

		mov		eax,[WLeft]						;re grab z
		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz

		mov		[ecx+0],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+0],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+4]						;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+4],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+2],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+8]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+8],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+4],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+12]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+12],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+6],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+16]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+16],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+8],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+20]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+20],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+10],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+24]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+24],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+12],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+28]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+28],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+14],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+32]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+32],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+16],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+36]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+36],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+18],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+40]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+40],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+20],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+44]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+44],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+22],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+48]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+48],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+24],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+52]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+52],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+26],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+56]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+56],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,8							;mul by y delta
		and		ebx,00ffffffh					;wrap u

		and		esi,0ff000000h					;wrap v
		mov		[ecx+28],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+60]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+60],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan
		pop		ebp

		mov		[ecx+30],ax						;write pixel

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		[SCan+4],32
		add		edi,32					
		add		[SCanZ+4],64
		dec		[NumASpans]			
		jnz		SpanLoop256_ZBuf		

HandleLeftoverPixels256_ZBuf:

		mov		esi,pTex
		mov		[SCan+4],edi

		cmp		[RemainingCount],0		
		jz		FPUReturn256_ZBuf			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan256_ZBuf			

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan256_ZBuf:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop256_ZBuf:
		push	ebp
		mov		eax,[DeltaW]					;grab deltaw

		mov		ebp,[SCanZ+4]					;grab zbuf ptr
		add		[WLeft],eax						;add left+delt

		mov		eax,[ebp]
		sub		eax,[WLeft]						;sub to compare

		sbb		edx,edx							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ebp,dword ptr[4*edx+SCanZ+4]	;grab zbuf or zcanz
		mov		[ebp],eax						;write to zbuf or zcan

		mov		eax,ecx					
		mov		ebp,dword ptr[4*edx+SCan+4]		;grab screen or zcan

		shr		eax,8					
		mov		edx,ebx

		and		eax,0ff00h

		shr		edx,16					
		and		edx,0ffh

		add		eax,edx					
		add		eax,esi

		mov		ax,[2*eax]				

		mov		[ebp],ax				;write pixel
		add		[SCan+4],2

		add		edi,2
		add		[SCanZ+4],4

		pop		ebp

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop256_ZBuf			

FPUReturn256_ZBuf:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return256_ZBuf:
	}		
}

static void DrawScanLine128_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	SCanZ[0]=SCan[0]=(unsigned long)&can0;

	_asm
	{
		mov		eax, pSizeInfo
		mov		esi, [eax]SizeInfo.ScreenData	
		mov		ebx, pLeft						
		mov		eax, [ebx]EdgeAsm.X				
		mov		ecx, pRight						
		mov		ecx, [ecx]EdgeAsm.X				
		sub		ecx, eax						
		jle		Return128_ZBuf					

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax, ecx						
		shr		ecx, 4							
		and		eax, 15							
		_emit 75h								; short jump 6 bytes
		_emit 06h								; jnz any leftover?
		dec		ecx								
		mov		eax, 16							

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi, pTex				; while fdiv is cooking
		mov		eax,OFFSET SCanZ+4

		mov		edx,pZBuf
		mov		[eax],edx

		mov		edx, pGradients
		mov		eax, pSizeInfo

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL


		test	ecx,ecx					
		jnz		SpanLoop128_ZBuf		; grab wleft for leftover loop
										
		jmp		HandleLeftoverPixels128_ZBuf

SpanLoop128_ZBuf:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1								; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)					; ZR   V/ZR 1/ZR U/ZR UL   VL


		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,9
		mov		esi,pTex

		shr		ebx,16
		and		eax,00003f80h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,07fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		[SCan+4],edi

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		mov		eax,[ecx+0]						;grab screen z

		add		edx,[DeltaV]					;step v
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		add		ebx,[DeltaU]					;step u

		mov		eax,[WLeft]						;re grab z
		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz

		mov		[ecx+0],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+0],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+4]						;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+4],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+2],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+8]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+8],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+4],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+12]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+12],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+6],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+16]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+16],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+8],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+20]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+20],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+10],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+24]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+24],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+12],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+28]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+28],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+14],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+32]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+32],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+16],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+36]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+36],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+18],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+40]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+40],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+20],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+44]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+44],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+22],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+48]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+48],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+24],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+52]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+52],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+26],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+56]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+56],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,7							;mul by y delta
		and		ebx,007fffffh					;wrap u

		and		esi,03f800000h					;wrap v
		mov		[ecx+28],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+60]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+60],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan
		pop		ebp

		mov		[ecx+30],ax						;write pixel

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		[SCan+4],32
		add		edi,32					
		add		[SCanZ+4],64
		dec		[NumASpans]			
		jnz		SpanLoop128_ZBuf		

HandleLeftoverPixels128_ZBuf:

		mov		esi,pTex
		mov		[SCan+4],edi

		cmp		[RemainingCount],0		
		jz		FPUReturn128_ZBuf			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan128_ZBuf	

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan128_ZBuf:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop128_ZBuf:
		push	ebp
		mov		eax,[DeltaW]					;grab deltaw

		mov		ebp,[SCanZ+4]					;grab zbuf ptr
		add		[WLeft],eax						;add left+delt

		mov		eax,[ebp]
		sub		eax,[WLeft]						;sub to compare

		sbb		edx,edx							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ebp,dword ptr[4*edx+SCanZ+4]	;grab zbuf or zcanz
		mov		[ebp],eax						;write to zbuf or zcan

		mov		eax,ecx					
		mov		ebp,dword ptr[4*edx+SCan+4]		;grab screen or zcan

		shr		eax,9					
		mov		edx,ebx

		and		eax,03f80h

		shr		edx,16					
		and		edx,07fh

		add		eax,edx					
		add		eax,esi

		mov		ax,[2*eax]				

		mov		[ebp],ax				;write pixel
		add		[SCan+4],2

		add		edi,2
		add		[SCanZ+4],4

		pop		ebp

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop128_ZBuf			

FPUReturn128_ZBuf:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return128_ZBuf:
	}		
}

static void DrawScanLine64_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	SCanZ[0]=SCan[0]=(unsigned long)&can0;

	_asm
	{
		mov		eax, pSizeInfo
		mov		esi, [eax]SizeInfo.ScreenData	
		mov		ebx, pLeft						
		mov		eax, [ebx]EdgeAsm.X				
		mov		ecx, pRight						
		mov		ecx, [ecx]EdgeAsm.X				
		sub		ecx, eax						
		jle		Return64_ZBuf					

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		mov		eax, ecx						
		shr		ecx, 4							
		and		eax, 15							
		_emit 75h								; short jump 6 bytes
		_emit 06h								; jnz any leftover?
		dec		ecx								
		mov		eax, 16							

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi, pTex				; while fdiv is cooking
		mov		eax,OFFSET SCanZ+4

		mov		edx,pZBuf
		mov		[eax],edx

		mov		edx, pGradients
		mov		eax, pSizeInfo

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL


		test	ecx,ecx					
		jnz		SpanLoop64_ZBuf			; grab wleft for leftover loop
										
		jmp		HandleLeftoverPixels64_ZBuf

SpanLoop64_ZBuf:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1								; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)					; ZR   V/ZR 1/ZR U/ZR UL   VL


		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,10
		mov		esi,pTex

		shr		ebx,16
		and		eax,00000fc0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,03fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		[SCan+4],edi

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		mov		eax,[ecx+0]						;grab screen z

		add		edx,[DeltaV]					;step v
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		add		ebx,[DeltaU]					;step u

		mov		eax,[WLeft]						;re grab z
		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz

		mov		[ecx+0],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+0],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+4]						;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+4],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+2],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+8]						;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+8],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+4],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+12]					;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+12],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+6],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+16]					;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+16],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+8],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+20]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+20],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+10],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+24]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+24],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+12],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+28]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+28],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+14],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+32]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+32],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+16],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+36]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+36],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+18],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+40]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+40],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+20],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+44]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+44],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+22],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+48]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+48],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+24],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+52]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+52],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+26],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+56]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+56],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,6							;mul by y delta
		and		ebx,003fffffh					;wrap u

		and		esi,00fc00000h					;wrap v
		mov		[ecx+28],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+60]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+60],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan
		pop		ebp

		mov		[ecx+30],ax						;write pixel

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		[SCan+4],32
		add		edi,32					
		add		[SCanZ+4],64
		dec		[NumASpans]			
		jnz		SpanLoop64_ZBuf		

HandleLeftoverPixels64_ZBuf:

		mov		esi,pTex
		mov		[SCan+4],edi

		cmp		[RemainingCount],0		
		jz		FPUReturn64_ZBuf			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan64_ZBuf	

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan64_ZBuf:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop64_ZBuf:
		push	ebp
		mov		eax,[DeltaW]					;grab deltaw

		mov		ebp,[SCanZ+4]					;grab zbuf ptr
		add		[WLeft],eax						;add left+delt

		mov		eax,[ebp]
		sub		eax,[WLeft]						;sub to compare

		sbb		edx,edx							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ebp,dword ptr[4*edx+SCanZ+4]	;grab zbuf or zcanz
		mov		[ebp],eax						;write to zbuf or zcan

		mov		eax,ecx					
		mov		ebp,dword ptr[4*edx+SCan+4]		;grab screen or zcan

		shr		eax,10					
		mov		edx,ebx

		and		eax,00fc0h

		shr		edx,16					
		and		edx,03fh

		add		eax,edx					
		add		eax,esi

		mov		ax,[2*eax]				

		mov		[ebp],ax				;write pixel
		add		[SCan+4],2

		add		edi,2
		add		[SCanZ+4],4

		pop		ebp

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop64_ZBuf			

FPUReturn64_ZBuf:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return64_ZBuf:
	}		
}

static void DrawScanLine32_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	SCanZ[0]=SCan[0]=(unsigned long)&can0;

	_asm
	{
		mov		eax, pSizeInfo
		mov		esi, [eax]SizeInfo.ScreenData	
		mov		ebx, pLeft						
		mov		eax, [ebx]EdgeAsm.X				
		mov		ecx, pRight						
		mov		ecx, [ecx]EdgeAsm.X				
		sub		ecx, eax						
		jle		Return32_ZBuf					

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		shl		edx, 1							; 32 bit zbuffer
		add		edi, ebx						; add in x start offset
		add		edx, eax
		mov		pZBuf, edx

		mov		eax, ecx						
		shr		ecx, 4							
		and		eax, 15							
		_emit 75h								; short jump 6 bytes
		_emit 06h								; jnz any leftover?
		dec		ecx								
		mov		eax, 16							

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi, pTex				; while fdiv is cooking
		mov		eax,OFFSET SCanZ+4

		mov		edx,pZBuf
		mov		[eax],edx

		mov		edx, pGradients
		mov		eax, pSizeInfo

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL


		test	ecx,ecx					
		jnz		SpanLoop32_ZBuf		; grab wleft for leftover loop
										
		jmp		HandleLeftoverPixels32_ZBuf

SpanLoop32_ZBuf:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1								; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)					; ZR   V/ZR 1/ZR U/ZR UL   VL


		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,11
		mov		esi,pTex

		shr		ebx,16
		and		eax,000003e0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,01fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		[SCan+4],edi

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		mov		eax,[ecx+0]						;grab screen z

		add		edx,[DeltaV]					;step v
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		add		ebx,[DeltaU]					;step u

		mov		eax,[WLeft]						;re grab z
		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz

		mov		[ecx+0],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+0],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+4]						;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+4],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+2],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+8]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+8],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+4],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+12]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+12],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+6],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+16]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+16],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+8],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+20]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+20],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+10],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+24]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+24],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+12],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+28]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+28],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+14],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+32]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+32],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+16],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+36]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+36],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+18],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+40]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+40],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+20],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+44]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+44],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+22],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+48]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+48],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+24],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+52]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+52],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+26],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+56]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+56],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,5							;mul by y delta
		and		ebx,001fffffh					;wrap u

		and		esi,003e00000h					;wrap v
		mov		[ecx+28],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+60]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+60],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan
		pop		ebp

		mov		[ecx+30],ax						;write pixel

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		[SCan+4],32
		add		edi,32					
		add		[SCanZ+4],64
		dec		[NumASpans]			
		jnz		SpanLoop32_ZBuf		

HandleLeftoverPixels32_ZBuf:

		mov		esi,pTex
		mov		[SCan+4],edi

		cmp		[RemainingCount],0		
		jz		FPUReturn32_ZBuf			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan32_ZBuf	

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan32_ZBuf:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop32_ZBuf:
		push	ebp
		mov		eax,[DeltaW]					;grab deltaw

		mov		ebp,[SCanZ+4]					;grab zbuf ptr
		add		[WLeft],eax						;add left+delt

		mov		eax,[ebp]
		sub		eax,[WLeft]						;sub to compare

		sbb		edx,edx							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ebp,dword ptr[4*edx+SCanZ+4]	;grab zbuf or zcanz
		mov		[ebp],eax						;write to zbuf or zcan

		mov		eax,ecx					
		mov		ebp,dword ptr[4*edx+SCan+4]		;grab screen or zcan

		shr		eax,11					
		mov		edx,ebx

		and		eax,003e0h

		shr		edx,16					
		and		edx,01fh

		add		eax,edx					
		add		eax,esi

		mov		ax,[2*eax]				

		mov		[ebp],ax				;write pixel
		add		[SCan+4],2

		add		edi,2
		add		[SCanZ+4],4

		pop		ebp

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop32_ZBuf			

FPUReturn32_ZBuf:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return32_ZBuf:
	}		
}

static void DrawScanLine16_ZBuf(SizeInfo *pSizeInfo,
				  Gradients const *pGradients,
				  EdgeAsm *pLeft,
				  EdgeAsm *pRight)
{
	SCanZ[0]=SCan[0]=(unsigned long)&can0;

	_asm
	{
		mov		eax, pSizeInfo
		mov		esi, [eax]SizeInfo.ScreenData	
		mov		ebx, pLeft						
		mov		eax, [ebx]EdgeAsm.X				
		mov		ecx, pRight						
		mov		ecx, [ecx]EdgeAsm.X				
		sub		ecx, eax						
		jle		Return16_ZBuf					

		mov		eax, pSizeInfo
		mov		edi, [eax]SizeInfo.TexData		; current texture
		shr		edi, 1
		mov		pTex, edi
		mov		edi, esi						
		mov		esi, [eax]SizeInfo.ScreenWidth
		mov		edx, [ebx]EdgeAsm.Y				
		shl		esi, 1
		mov		eax, [eax]SizeInfo.ZData		; zbuffer pointer
		imul	edx, esi						
		mov		ebx, [ebx]EdgeAsm.X
		add		edi, edx						
		shl		ebx, 1
		add		edx, ebx
		add		edi, ebx						; add in x start offset
		shl		edx, 1							; 32 bit zbuffer
		add		edx, eax
		mov		pZBuf, edx

		
		mov		eax, ecx						
		shr		ecx, 4							
		and		eax, 15							
		_emit 75h								; short jump 6 bytes
		_emit 06h								; jnz any leftover?
		dec		ecx								
		mov		eax, 16							

		mov		ebx, pLeft
												; start first fdiv cooking

		fld		[ebx]EdgeAsm.VOverZ		; V/ZL
		fld		[ebx]EdgeAsm.UOverZ		; U/ZL V/ZL 
		fld		[ebx]EdgeAsm.OneOverZ	; 1/ZL U/ZL V/ZL 
		fld1							; 1    1/ZL U/ZL V/ZL 
		fdiv	st,st(1)				; ZL   1/ZL U/ZL V/ZL 

		mov		[NumASpans],ecx
		mov		[RemainingCount],eax

		mov		esi, pTex				; while fdiv is cooking
		mov		eax,OFFSET SCanZ+4

		mov		edx,pZBuf
		mov		[eax],edx

		mov		edx, pGradients
		mov		eax, pSizeInfo

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	[FixedScale]				; ZL16 ZL   1/ZL U/ZL V/ZL
		fistp	[WLeft]						; ZL   1/ZL U/ZL V/ZL

		fld		st							; ZL   ZL   1/ZL U/ZL V/ZL
		fmul	st,st(4)					; VL   ZL   1/ZL U/ZL V/ZL
		fxch	st(1)						; ZL   VL   1/ZL U/ZL V/ZL
		fmul	st,st(3)					; UL   VL   1/ZL U/ZL V/ZL

		fstp	st(5)						; VL   1/ZL U/ZL V/ZL UL
		fstp	st(5)						; 1/ZL U/ZL V/ZL UL   VL

		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR U/ZL V/ZL UL   VL
		fxch	st(1)						; U/ZL 1/ZR V/ZL UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR 1/ZR V/ZL UL   VL
		fxch	st(2)						; V/ZL 1/ZR U/ZR UL   VL
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZR U/ZR UL   VL

		fld1							; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL
		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL


		test	ecx,ecx					
		jnz		SpanLoop16_ZBuf			; grab wleft for leftover loop
										
		jmp		HandleLeftoverPixels16_ZBuf

SpanLoop16_ZBuf:

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,4
		mov		[DeltaW],eax

		fld		st(5)					; UL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; UL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[UFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fld		st(6)					; VL   UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	[FixedScale]			; VL16 UR   VR   V/ZR 1/ZR U/ZR UL   VL
		fistp	[VFixed]				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		fsubr	st(5),st				; UR   VR   V/ZR 1/ZR U/ZR dU   VL
		fxch	st(1)					; VR   UR   V/ZR 1/ZR U/ZR dU   VL
		fsubr	st(6),st				; VR   UR   V/ZR 1/ZR U/ZR dU   dV
		fxch	st(6)					; dV   UR   V/ZR 1/ZR U/ZR dU   VR

		fmul	[FixedScale16]			; dV16  UR   V/ZR 1/ZR U/ZR dU   VR
		fistp	[DeltaV]				; UR   V/ZR 1/ZR U/ZR dU   VR
		fxch	st(4)					; dU   V/ZR 1/ZR U/ZR UR   VR
		fmul	[FixedScale16]			; duint16  V/ZR 1/ZR U/ZR UR   VR
		fistp	[DeltaU]				; V/ZR 1/ZR U/ZR UR   VR

		mov		edx,pGradients
		fadd	[edx]Gradients.dVOverZdX16	; V/ZR 1/ZL U/ZL UL   VL
		fxch	st(1)						; 1/ZL V/ZR U/ZL UL   VL
		fadd	[edx]Gradients.dOneOverZdX16; 1/ZR V/ZR U/ZL UL   VL
		fxch	st(2)						; U/ZL V/ZR 1/ZR UL   VL
		fadd	[edx]Gradients.dUOverZdX16	; U/ZR V/ZR 1/ZR UL   VL
		fxch	st(2)						; 1/ZR V/ZR U/ZR UL   VL
		fxch	st(1)						; V/ZR 1/ZR U/ZR UL   VL

		fld1								; 1    V/ZR 1/ZR U/ZR UL   VL
		fdiv	st,st(2)					; ZR   V/ZR 1/ZR U/ZR UL   VL


		mov		dl,[edi]	; preread the destination cache line
							; this moved up from the main loop to get
							; a non blocking fill on ppros and p2's
							; make sure dl doesn't prs!!!!

		mov		eax,[VFixed]
		mov		ebx,[UFixed]

		shr		eax,12
		mov		esi,pTex

		shr		ebx,16
		and		eax,000000f0h

		mov		ecx,[DeltaU]
		mov		edx,[VFixed]

		add		esi,eax
		and		ebx,0fh

		mov		eax,[DeltaV]
		add		esi,ebx

		mov		[SCan+4],edi

		mov		ebx,[UFixed]
		push	ebp

		mov		ebp,eax

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		mov		eax,[ecx+0]						;grab screen z

		add		edx,[DeltaV]					;step v
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		add		ebx,[DeltaU]					;step u

		mov		eax,[WLeft]						;re grab z
		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz

		mov		[ecx+0],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+0],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+4]						;update wleft
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+4],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+2],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+8]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+8],eax						;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+4],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+12]						;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+12],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+6],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+16]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+16],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+8],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+20]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+20],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+10],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+24]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+24],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+12],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+28]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+28],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+14],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+32]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+32],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+16],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+36]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+36],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+18],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+40]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+40],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+20],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+44]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+44],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+22],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+48]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+48],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+24],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+52]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+52],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+26],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+56]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+56],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		esi,edx							;add in v to texptr
		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan

		shl		esi,4							;mul by y delta
		and		ebx,000fffffh					;wrap u

		and		esi,00f00000h					;wrap v
		mov		[ecx+28],ax						;write pixel

		add		esi,ebx							;add in u to texptr
		mov		eax,[DeltaW]					;grab deltaw

		shr		esi,16							;int uv offset
		add		[WLeft],eax						;add left+delt

		mov		ecx,[SCanZ+4]					;grab zbuf ptr
		add		esi,pTex						;add texptr to u,v

		mov		eax,[ecx+60]					;grab screen z
		add		edx,[DeltaV]					;step v

		add		ebx,[DeltaU]					;step u
		sub		eax,[WLeft]						;sub to compare

		sbb		ebp,ebp							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ecx,dword ptr[4*ebp+SCanZ+4]	;grab zbuf or zcanz
		mov		[ecx+60],eax					;write to zbuf or zcan
		mov		ax,[2*esi]						;get texture pixel

		mov		ecx,dword ptr[4*ebp+SCan+4]		;grab screen or zcan
		pop		ebp

		mov		[ecx+30],ax						;write pixel

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		eax,[WRight]

		fmul	[FixedScale]			; ZR16 ZR   V/ZR 1/ZR U/ZR UL   VL

		mov		[WLeft],eax

		fistp	[WRight]				; ZR   V/ZR 1/ZR U/ZR UL   VL

		fld		st						; ZR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(2)				; VR   ZR   V/ZR 1/ZR U/ZR UL   VL
		fxch	st(1)					; ZR   VR   V/ZR 1/ZR U/ZR UL   VL
		fmul	st,st(4)				; UR   VR   V/ZR 1/ZR U/ZR UL   VL

		add		[SCan+4],32
		add		edi,32					
		add		[SCanZ+4],64
		dec		[NumASpans]			
		jnz		SpanLoop16_ZBuf		

HandleLeftoverPixels16_ZBuf:

		mov		esi,pTex
		mov		[SCan+4],edi

		cmp		[RemainingCount],0		
		jz		FPUReturn16_ZBuf			

		mov		ebx,pRight				
		mov		edx,pGradients			

		fld		st(5)					; UL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; UL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[UFixed]				; inv. inv. inv. inv. inv. UL   VL
		fld		st(6)					; VL   inv. inv. inv. inv. inv. UL   VL
		fmul	[FixedScale]			; VL16 inv. inv. inv. inv. inv. UL   VL
		fistp	[VFixed]				; inv. inv. inv. inv. inv. UL   VL

		dec		[RemainingCount]		
		jz		OnePixelSpan16_ZBuf	

		mov		eax,[WRight]
		sub		eax,[WLeft]
		sar		eax,3			//correct on average :)
		mov		[DeltaW],eax

		fstp	[geFloatTemp]				; inv. inv. inv. inv. UL   VL
		fstp	[geFloatTemp]				; inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.VOverZ			; V/Zr inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dVOverZdX	; V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.UOverZ			; U/Zr V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dUOverZdX	; U/ZR V/ZR inv. inv. inv. UL   VL
		fld		[ebx]EdgeAsm.OneOverZ		; 1/Zr U/ZR V/ZR inv. inv. inv. UL   VL
		fsub	[edx]Gradients.dOneOverZdX	; 1/ZR U/ZR V/ZR inv. inv. inv. UL   VL
		fdivr	[One]						; ZR   U/ZR V/ZR inv. inv. inv. UL   VL
		fmul	st(1),st					; ZR   UR   V/ZR inv. inv. inv. UL   VL
		fmulp	st(2),st					; UR   VR   inv. inv. inv. UL   VL

		fsubr	st(5),st					; UR   VR   inv. inv. inv. dU   VL
		fxch	st(1)						; VR   UR   inv. inv. inv. dU   VL
		fsubr	st(6),st					; VR   UR   inv. inv. inv. dU   dV
		fxch	st(6)						; dV   UR   inv. inv. inv. dU   VR
		fidiv   [RemainingCount]			; dv   UR   inv. inv. inv. dU   VR
		fmul	[FixedScale]				; dv16 UR   inv. inv. inv. dU   VR
		fistp	[DeltaV]					; UR   inv. inv. inv. dU   VR
		fxch	st(4)						; dU   inv. inv. inv. UR   VR
		fidiv	[RemainingCount]			; du   inv. inv. inv. UR   VR
		fmul	[FixedScale]				; duint16 inv. inv. inv. UR   VR
		fistp	[DeltaU]					; inv. inv. inv. UR   VR

		fld		st(1)						; inv. inv. inv. inv. UR   VR
		fld		st(2)						; inv. inv. inv. inv. inv. UR   VR

OnePixelSpan16_ZBuf:
		mov		ebx,[UFixed]				
		mov		ecx,[VFixed]				

LeftoverLoop16_ZBuf:
		push	ebp
		mov		eax,[DeltaW]					;grab deltaw

		mov		ebp,[SCanZ+4]					;grab zbuf ptr
		add		[WLeft],eax						;add left+delt

		mov		eax,[ebp]
		sub		eax,[WLeft]						;sub to compare

		sbb		edx,edx							;get -1 if carry
		mov		eax,[WLeft]						;re grab z

		mov		ebp,dword ptr[4*edx+SCanZ+4]	;grab zbuf or zcanz
		mov		[ebp],eax						;write to zbuf or zcan

		mov		eax,ecx					
		mov		ebp,dword ptr[4*edx+SCan+4]		;grab screen or zcan

		shr		eax,12					
		mov		edx,ebx

		and		eax,000f0h

		shr		edx,16					
		and		edx,0fh

		add		eax,edx					
		add		eax,esi

		mov		ax,[2*eax]				

		mov		[ebp],ax				;write pixel
		add		[SCan+4],2

		add		edi,2
		add		[SCanZ+4],4

		pop		ebp

		add		ebx,DeltaU				
		add		ecx,DeltaV				

		dec		[RemainingCount]		
		jge		LeftoverLoop16_ZBuf			

FPUReturn16_ZBuf:
		ffree	st(0)
		ffree	st(1)
		ffree	st(2)
		ffree	st(3)
		ffree	st(4)
		ffree	st(5)
		ffree	st(6)

Return16_ZBuf:
	}		
}
