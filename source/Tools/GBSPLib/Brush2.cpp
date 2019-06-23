/****************************************************************************************/
/*  Brush2.Cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Code to load, csg, and split brushes, etc...                           */
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
#include <Windows.h>
#include <Math.h>

#include "Brush2.h"
#include "Mathlib.h"
#include "Poly.h"
#include "BSP.h"
#include "Texture.h"

#include "VFile.h"
#include "Vec3d.h"
#include "Ram.h"

#define BSP_BRUSH_SIZE(s) ((sizeof(GBSP_Brush)-sizeof(GBSP_Side[NUM_BRUSH_DEFAULT_SIDES]))+(sizeof(GBSP_Side)*(s)));
#define MAP_BRUSH_SIZE(s) ((sizeof(MAP_Brush)-sizeof(GBSP_Side[NUM_BRUSH_DEFAULT_SIDES]))+(sizeof(GBSP_Side)*(s)));

int32	gTotalBrushes;
int32	gPeekBrushes;

extern int32	NumSolidBrushes;
extern int32	NumCutBrushes;
extern int32	NumHollowCutBrushes;
extern int32	NumDetailBrushes;
extern int32	NumTotalBrushes;

//=======================================================================================
//	AllocMapBrush
//=======================================================================================
MAP_Brush *AllocMapBrush(int32 NumSides)
{
	MAP_Brush	*MapBrush;
	int32		Size;

	Size = MAP_BRUSH_SIZE(NumSides);

	MapBrush = (MAP_Brush*)geRam_Allocate(Size);

	memset(MapBrush, 0, Size);

	return MapBrush;
}

//=======================================================================================
//	FreeMapBrush
//=======================================================================================
void FreeMapBrush(MAP_Brush *Brush)
{
	int32		i;

	for (i=0 ; i<Brush->NumSides ; i++)
		if (Brush->OriginalSides[i].Poly)
			FreePoly(Brush->OriginalSides[i].Poly);

	geRam_Free(Brush);
}

//================================================================================
//	LoadMapBrush
//================================================================================
MAP_Brush *LoadMapBrush(geVFile *VFile)
{
	MAP_Brush		*Brush;
	MAP_FaceHeader	FaceHeader;
	MAP_BrushHeader	BrushHeader;
	GBSP_Side		*Side;
	int32			k, NumSides;
	int32			PlaneNum, PlaneSide, s;
	GFX_TexInfo		Tx;
	
	GBSP_Plane		Plane;
	float			FDist;
	geVec3d			VertF;
			
	// Get the number of faces
	if (!geVFile_Read(VFile, &BrushHeader, sizeof(MAP_BrushHeader)))
	{
		GHook.Error("LoadMapBrush:  There was an error reading the brush header.\n");
		return NULL;
	}

	Brush = AllocMapBrush(BrushHeader.NumFaces);
	Brush->NumSides = BrushHeader.NumFaces;

	NumSides = 0;

	for (k = 0; k< BrushHeader.NumFaces; k++)
	{
		memset (&Tx, 0, sizeof(GFX_TexInfo));

		// Get the face header
		if (!geVFile_Read(VFile, &FaceHeader, sizeof(MAP_FaceHeader)))
		{
			GHook.Error("LoadMapBrush:  There was an error reading the face header.\n");
			return NULL;
		}

		// Fill in the tex info
		Tx.Texture = FindTextureIndex(FaceHeader.TexName, 0);

		//if (fread(&VertF, sizeof(Vec3dF), 1, f) != 1)
		if (!geVFile_Read(VFile, &VertF, sizeof(geVec3d)))
		{
			FreeMapBrush(Brush);
			return NULL;
		}

		if (!geVFile_Read(VFile, &FDist, sizeof(float)))
		{
			GHook.Error("LoadMapBrush:  Could not read face plane dist.\n");
			FreeMapBrush(Brush);
			return NULL;
		}

		Plane.Normal.X = (geFloat)VertF.X;
		Plane.Normal.Y = (geFloat)VertF.Y;
		Plane.Normal.Z = (geFloat)VertF.Z;

		Plane.Dist = (geFloat)FDist;

		Plane.Type = PLANE_ANY;
		if (geVec3d_Compare(&Plane.Normal, &VecOrigin, VCOMPARE_EPSILON))
		{
			GHook.Error("LoadMapBrush: Invalid Normal, skipping!!!\n");
			GHook.Error("Normal - X: %2.2f, Y:%2.2f, Z:%2.2f\n", Plane.Normal.X, Plane.Normal.Y, Plane.Normal.Z);
			continue;
		}
		
		geVec3d_Normalize(&Plane.Normal);

		SnapPlane(&Plane.Normal, &Plane.Dist);

		PlaneNum = FindPlane(&Plane, &PlaneSide);

		Side = Brush->OriginalSides;
		for (s = 0; s<NumSides; s++, Side++)
		{
			if (Side->PlaneNum == PlaneNum && Side->PlaneSide == PlaneSide)
				break;
		}

		if (s != NumSides)
		{
			GHook.Printf("*WARNING* LoadMapBrush:  Brush with duplicate plane.\n");
			continue;
		}
		
		// Get the snapped plane...
		Plane = Planes[PlaneNum];

		if (PlaneSide)
			PlaneInverse(&Plane);

		Tx.Vecs[0].X = FaceHeader.uVecX;
		Tx.Vecs[0].Y = FaceHeader.uVecY;
		Tx.Vecs[0].Z = FaceHeader.uVecZ;

		Tx.Vecs[1].X = FaceHeader.vVecX;
		Tx.Vecs[1].Y = FaceHeader.vVecY;
		Tx.Vecs[1].Z = FaceHeader.vVecZ;

		Tx.Shift[0] = FaceHeader.OffsetX;
		Tx.Shift[1] = FaceHeader.OffsetY;

		Tx.DrawScale[0] = FaceHeader.DrawScaleX;
		Tx.DrawScale[1] = FaceHeader.DrawScaleY;

		Tx.FaceLight = (float)FaceHeader.FaceLight;
		Tx.Flags = FaceHeader.Flags;

		// Don't allow alpha values on faces that don't have the TRANS flag set...
		if (!(Tx.Flags & TEXINFO_TRANS))
			FaceHeader.Alpha = 255.0f;

		// Copy the brush's alpha directly over to the faces texinfo
		Tx.Alpha = (float)FaceHeader.Alpha;
		Tx.MipMapBias = FaceHeader.MipMapBias;
		Tx.ReflectiveScale = FaceHeader.ReflectiveScale;

		if (Tx.Flags & TEXINFO_MIRROR)
		{
			Tx.Flags |= TEXINFO_TRANS;
			Side->Flags |= SIDE_HINT;

			if (Tx.Alpha > 240.0f)
				GHook.Printf("*WARNING* HIGH Alpha value for mirror face: %f\n", Tx.Alpha);
		}
		
		// If any side has some transparancy, make is so that you can see through the brush, and make it detail
		// NOTE - We don't include mirror faces, since you can't actually see through those faces...
		if ((Tx.Flags & TEXINFO_TRANS) && !(Tx.Flags&TEXINFO_MIRROR))
		{
			BrushHeader.Flags |= BSP_CONTENTS_TRANSLUCENT2;
		}


		Side->PlaneNum = PlaneNum;
		Side->PlaneSide = (uint8)PlaneSide;
		Side->TexInfo = FindTexInfo (&Tx);		

		if (Side->TexInfo == -1)
		{
			GHook.Error("LoadMapBrush:  Could not find texinfo.\n");
			FreeMapBrush(Brush);
			return NULL;
		}

		NumSides++;
	}

	if (NumSides > Brush->NumSides)
	{
		GHook.Error("LoadMapBrush:  Num sides do not match.\n");
		return NULL;
	}
	
	Brush->Contents = BrushHeader.Flags;
	Brush->EntityNum = NumEntities-1;
	Brush->NumSides = NumSides;

	MakeMapBrushPolys(Brush);

	// If no contents, then make empty (so it will at least have a visibility bit set...)
	// Contents of 0 are invisible (air)
	// Only Contents with at least one bit matching BSP_VISIBLE_CONTENTS will be visible
	if (!Brush->Contents)
		Brush->Contents |= BSP_CONTENTS_EMPTY2;	

	// HACK!  Convert solid sheets to clip...
	if ((Brush->Contents & BSP_CONTENTS_SHEET) && (Brush->Contents & BSP_CONTENTS_SOLID2))
	{
		Brush->Contents &= ~BSP_CONTENTS_SOLID2;
		Brush->Contents |= BSP_CONTENTS_CLIP2;
	}

	// Force clip to solid/detail, and mark faces as not visible (they will get put last in the tree)
	if (Brush->Contents & BSP_CONTENTS_CLIP2)
	{
		for (k=0; k<NumSides; k++)
			Brush->OriginalSides[k].Flags &= ~SIDE_VISIBLE;	// Clips won't have faces

		Brush->Contents |= BSP_CONTENTS_DETAIL2;			// Clips are allways detail
	}
	
	if (Brush->Contents & BSP_CONTENTS_SHEET)
	{
		// Only the first side is visible for sheets
		Brush->OriginalSides[0].Flags |= SIDE_SHEET;
		// Sheets are allways detail!!!
		Brush->Contents |= BSP_CONTENTS_DETAIL2;		
	}

	// Force non-solid/non-hint to detail
	//if (!(Brush->Contents & BSP_CONTENTS_SOLID2))
	//	Brush->Contents |= BSP_CONTENTS_DETAIL2;

	// Convert all sides to hint if need so...
	if (Brush->Contents & BSP_CONTENTS_HINT2)
	{
		for (k=0; k<NumSides; k++)
		{
			Brush->OriginalSides[k].Flags |= SIDE_HINT;
			Brush->OriginalSides[k].Flags |= SIDE_VISIBLE;
		}

		if (Brush->Contents & BSP_CONTENTS_DETAIL2)
			Brush->Contents &= ~BSP_CONTENTS_DETAIL2;
	}

	if (Brush->Contents&BSP_CONTENTS_WINDOW2)
	{
		Brush->Contents |= BSP_CONTENTS_TRANSLUCENT2;
		Brush->Contents |= BSP_CONTENTS_DETAIL2;
	}

	// Count up what types of brushes we've loaded
	if (Brush->Contents & BSP_CONTENTS_SOLID2)
		NumSolidBrushes++;
	
	if (Brush->Contents & BSP_CONTENTS_DETAIL2)
		NumDetailBrushes++;

	NumTotalBrushes++;

	return Brush;
}

//=======================================================================================
//	FreeMapBrushList
//=======================================================================================
void FreeMapBrushList(MAP_Brush *Brushes)
{
	MAP_Brush	*Next;

	for ( ; Brushes ; Brushes = Next)
	{
		Next = Brushes->Next;

		FreeMapBrush(Brushes);
	}		
}

//=======================================================================================
//	MakeMapBrushPolys
//	Builds the faces for all the planes on a MAP_Brush
//	Or's all side flags with SIDE_VISIBLE
//=======================================================================================
geBoolean MakeMapBrushPolys(MAP_Brush *ob)
{
	int32		i, j;
	GBSP_Poly	*p;
	GBSP_Side	*Side;
	GBSP_Plane	Plane, *Plane2;

	ClearBounds(&ob->Mins, &ob->Maxs);

	for (i=0 ; i<ob->NumSides ; i++)
	{
		Plane = Planes[ob->OriginalSides[i].PlaneNum];

		if (ob->OriginalSides[i].PlaneSide)
			PlaneInverse(&Plane);

		p = CreatePolyFromPlane(&Plane);

		for (j=0 ; j<ob->NumSides && p; j++)
		{
			if (i == j)
				continue;
	
			Plane2 = &Planes[ob->OriginalSides[j].PlaneNum];

			ClipPolyEpsilon(p, 0.0f, Plane2, !ob->OriginalSides[j].PlaneSide, &p);
		}

		Side = &ob->OriginalSides[i];
		Side->Poly = p;

		if (p)
		{
			// All sides default to being visible...
			Side->Flags |= SIDE_VISIBLE;
			for (j=0 ; j<p->NumVerts ; j++)
				AddPointToBounds(&p->Verts[j], &ob->Mins, &ob->Maxs);
		}
	}

	for (i=0 ; i<3 ; i++)
	{
		if (VectorToSUB(ob->Mins,i) <= -MIN_MAX_BOUNDS || VectorToSUB(ob->Maxs, i) >= MIN_MAX_BOUNDS)
			GHook.Printf("Entity %i, Brush %i: Bounds out of range\n", ob->EntityNum, ob->BrushNum);
	}

	return GE_TRUE;
}

//=======================================================================================
//	CopyMapBrush
//=======================================================================================
MAP_Brush *CopyMapBrush(MAP_Brush *Brush)
{
	MAP_Brush *NewBrush;
	int32		Size;
	int32		i;
	
	Size = MAP_BRUSH_SIZE(Brush->NumSides);

	NewBrush = AllocMapBrush(Brush->NumSides);
	memcpy (NewBrush, Brush, Size);

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		if (Brush->OriginalSides[i].Poly)
		{
			if (!CopyPoly(Brush->OriginalSides[i].Poly, &NewBrush->OriginalSides[i].Poly))
			{
				GHook.Error("Error copying brush.\n");
				return NULL;
			}
		}
	}

	NewBrush->Contents = Brush->Contents;

	return NewBrush;
}

//=======================================================================================
//	CountMapBrushList
//=======================================================================================
int32 CountMapBrushList(MAP_Brush *Brushes)
{
	int32	Count;

	for (Count=0; Brushes ; Brushes = Brushes->Next)
		Count++;

	return Count;
}

//
//	BSP Brush code
//

//=======================================================================================
//	AllocBrush
//=======================================================================================
GBSP_Brush *AllocBrush(int32 NumSides)
{
	GBSP_Brush	*Brush;
	int32		Size;

	Size = BSP_BRUSH_SIZE(NumSides);

	Brush = (GBSP_Brush*)geRam_Allocate(Size);

	if (!Brush)
		return NULL;

	memset(Brush, 0, Size);

#ifdef SHOW_DEBUG_STATS
	gTotalBrushes++;

	if (gTotalBrushes > gPeekBrushes)
		gPeekBrushes = gTotalBrushes;
#endif

	return Brush;
}

//=======================================================================================
//	FreeBrush
//=======================================================================================
void FreeBrush(GBSP_Brush *Brush)
{
	int32			i;

	for (i=0 ; i<Brush->NumSides ; i++)
		if (Brush->Sides[i].Poly)
			FreePoly(Brush->Sides[i].Poly);

#ifdef SHOW_DEBUG_STATS
	gTotalBrushes--;
#endif

	geRam_Free(Brush);
}

//=======================================================================================
//	FreeBrushList
//=======================================================================================
void FreeBrushList(GBSP_Brush *Brushes)
{
	GBSP_Brush	*Next;

	for ( ; Brushes ; Brushes = Next)
	{
		Next = Brushes->Next;

		FreeBrush(Brushes);
	}		
}

void ShowBrushHeap(void)
{
	GHook.Printf("Active Brushes     : %5i \n", gTotalBrushes);
	GHook.Printf("Peek Brushes       : %5i \n", gPeekBrushes);
}

#define BOGUS_SIDES		256
//=======================================================================================
//	CopyBrush
//=======================================================================================
GBSP_Brush *CopyBrush(GBSP_Brush *Brush)
{
	GBSP_Brush *NewBrush;
	int32		Size;
	int32		i;
	
	if (!Brush)
	{
		GHook.Error("CopyBrush:  NULL brush!!!\n");
		return NULL;
	}
	
	if (Brush->NumSides <= 0 || Brush->NumSides >= BOGUS_SIDES)
	{
		GHook.Error("CopyBrush:  Bogus number of sides: %i\n", Brush->NumSides);
		return NULL;
	}

	Size = (int)&(((GBSP_Brush *)0)->Sides[Brush->NumSides]);

	NewBrush = AllocBrush(Brush->NumSides);

	if (!NewBrush)
	{
		GHook.Error("CopyBrush:  Out of memory for brush.\n");
		return NULL;
	}

	memcpy (NewBrush, Brush, Size);

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		if (!Brush->Sides[i].Poly)
			continue;

		if (Brush->Sides[i].Poly->NumVerts < 0 || Brush->Sides[i].Poly->NumVerts > 256)
		{
			GHook.Error("CopyBrush:  Bad number of verts in poly:  %i.\n", Brush->Sides[i].Poly->NumVerts);
			return NULL;
		}
		
		//if (!CopyPoly(Brush->Sides[i].Poly, &NewBrush->Sides[i].Poly))
		NewBrush->Sides[i].Poly = CopyPoly2(Brush->Sides[i].Poly);
		if (!NewBrush->Sides[i].Poly)
		{
			GHook.Error("Error copying brush.\n");
			return NULL;
		}
	}

	return NewBrush;
}

//=======================================================================================
//	BoundBrush
//=======================================================================================
void BoundBrush(GBSP_Brush *Brush)
{
	int32		i, j;
	GBSP_Poly	*p;

	ClearBounds (&Brush->Mins, &Brush->Maxs);

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		p = Brush->Sides[i].Poly;
		
		if (!p)
			continue;

		for (j=0 ; j<p->NumVerts ; j++)
			AddPointToBounds(&p->Verts[j], &Brush->Mins, &Brush->Maxs);
	}
}

//=======================================================================================
//	AddBrushListToTail
//=======================================================================================
GBSP_Brush *AddBrushListToTail(GBSP_Brush *List, GBSP_Brush *Tail)
{
	GBSP_Brush	*Walk, *Next;

	for (Walk=List ; Walk ; Walk=Next)
	{	// add to end of list
		Next = Walk->Next;
		Walk->Next = NULL;
		Tail->Next = Walk;
		Tail = Walk;
	}

	return Tail;
}

//=======================================================================================
//	CountBrushList
//=======================================================================================
int32 CountBrushList(GBSP_Brush *Brushes)
{
	int32	c;

	c = 0;

	for ( ; Brushes ; Brushes = Brushes->Next)
		c++;

	return c;
}

//=======================================================================================
//	RemoveBrushList
//=======================================================================================
GBSP_Brush *RemoveBrushList(GBSP_Brush *List, GBSP_Brush *Remove)
{
	GBSP_Brush	*NewList;
	GBSP_Brush	*Next;

	NewList = NULL;

	for ( ; List ; List = Next)
	{
		Next = List->Next;

		if (List == Remove)
		{
			FreeBrush(List);
			continue;
		}

		List->Next = NewList;
		NewList = List;
	}

	return NewList;
}

//=======================================================================================
//	BrushVolume
//=======================================================================================
geFloat BrushVolume(GBSP_Brush *Brush)
{
	int32		i;
	GBSP_Poly	*p;
	geVec3d		Corner;
	geFloat		d, Area, Volume;
	GBSP_Plane	Plane;

	if (!Brush)
		return 0.0f;

	p = NULL;
	for (i=0 ; i<Brush->NumSides ; i++)
	{
		p = Brush->Sides[i].Poly;
		if (p)
			break;
	}
	if (!p)
		return 0.0f;

	geVec3d_Copy(&p->Verts[0], &Corner);

	Volume = 0.0f;
	for ( ; i<Brush->NumSides ; i++)
	{
		p = Brush->Sides[i].Poly;
		if (!p)
			continue;

		Plane = Planes[Brush->Sides[i].PlaneNum];

		if (Brush->Sides[i].PlaneSide)
			PlaneInverse(&Plane);

		d = -(geVec3d_DotProduct(&Corner, &Plane.Normal) - Plane.Dist);
		Area = PolyArea(p);
		Volume += d*Area;
	}

	Volume /= 3.0f;
	return Volume;
}

//=======================================================================================
//	CreateBrushPolys
//=======================================================================================
void CreateBrushPolys(GBSP_Brush *Brush)
{
	int32		i, j;
	GBSP_Poly	*p;
	GBSP_Side	*Side;
	GBSP_Plane	Plane, *pPlane;

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		Side = &Brush->Sides[i];
		Plane = Planes[Side->PlaneNum];
		Plane.Type = PLANE_ANY;

		if (Side->PlaneSide)
			PlaneInverse(&Plane);

		p = CreatePolyFromPlane(&Plane);

		for (j=0 ; j<Brush->NumSides && p; j++)
		{
			if (i == j)
				continue;

			pPlane = &Planes[Brush->Sides[j].PlaneNum];

			ClipPolyEpsilon(p, 0.0f, pPlane, !Brush->Sides[j].PlaneSide, &p);
		}

		Side->Poly = p;
	}

	BoundBrush(Brush);
}

//=======================================================================================
//	BrushFromBounds
//=======================================================================================
GBSP_Brush *BrushFromBounds (geVec3d *Mins, geVec3d *Maxs)
{
	GBSP_Brush	*b;
	int32		i;
	GBSP_Plane	Plane;
	int32		Side;

	b = AllocBrush(6);
	b->NumSides = 6;

	for (i=0 ; i<3 ; i++)
	{
		geVec3d_Clear(&Plane.Normal);
		VectorToSUB(Plane.Normal, i) = 1.0f;
		Plane.Dist = VectorToSUB(*Maxs, i)+1;
		b->Sides[i].PlaneNum = FindPlane(&Plane, &Side);
		b->Sides[i].PlaneSide = (uint8)Side;

		VectorToSUB(Plane.Normal, i) = -1.0f;
		Plane.Dist = -(VectorToSUB(*Mins, i)-1);
		b->Sides[i+3].PlaneNum = FindPlane(&Plane, &Side);
		b->Sides[i+3].PlaneSide = (uint8)Side;
	}

	CreateBrushPolys(b);

	return b;
}

//=======================================================================================
//	BrushMostlyOnSide
//=======================================================================================
int32 BrushMostlyOnSide(GBSP_Brush *Brush, GBSP_Plane *Plane)
{
	int32		i, j;
	GBSP_Poly	*p;
	geFloat		d, Max;
	int32		Side;

	Max = 0.0f;
	Side = PSIDE_FRONT;

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		p = Brush->Sides[i].Poly;
		if (!p)
			continue;

		for (j=0 ; j<p->NumVerts ; j++)
		{
			d = geVec3d_DotProduct(&p->Verts[j], &Plane->Normal) - Plane->Dist;
			if (d > Max)
			{
				Max = d;
				Side = PSIDE_FRONT;
			}
			if (-d > Max)
			{
				Max = -d;
				Side = PSIDE_BACK;
			}
		}
	}

	return Side;
}

//=======================================================================================
//	CheckBrush
//=======================================================================================
geBoolean CheckBrush(GBSP_Brush *Brush)
{
	int32		j;

	if (Brush->NumSides < 3)
		return GE_FALSE;

	for (j=0 ; j<3 ; j++)
	{
		if (VectorToSUB(Brush->Mins, j) <= -MIN_MAX_BOUNDS || VectorToSUB(Brush->Maxs, j) >= MIN_MAX_BOUNDS)
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=======================================================================================
//	SplitBrush
//=======================================================================================
void SplitBrush(GBSP_Brush *Brush, int32 PNum, int32 PSide, uint8 MidFlags, geBoolean Visible, GBSP_Brush **Front, GBSP_Brush **Back)
{
	int32		i, j;
	GBSP_Side	*pSide;
	GBSP_Poly	*p, *MidPoly;
	GBSP_Plane	Plane, *Plane2;
	geFloat		d, FrontD, BackD;
	GBSP_Plane	*pPlane1;
	GBSP_Brush	*Brushes[2];

	pPlane1 = &Planes[PNum];

	Plane = *pPlane1;
	Plane.Type = PLANE_ANY;

	if (PSide)
		PlaneInverse(&Plane);

	*Front = *Back = NULL;

	// Check all points
	FrontD = BackD = 0.0f;

	for (i=0 ; i<Brush->NumSides ; i++)
	{
		geVec3d	*pVert;

		p = Brush->Sides[i].Poly;

		if (!p)
			continue;

		for (pVert = p->Verts, j=0 ; j<p->NumVerts ; j++, pVert++)
		{
		#if 1
			d = Plane_PointDistanceFast(pPlane1, pVert);

			if (PSide)
				d = -d;
		#else
			d = geVec3d_DotProduct(pVert, &Plane.Normal) - Plane.Dist;
		#endif

			if (d > FrontD)
				FrontD = d;
			else if (d < BackD)
				BackD = d;
		}
	}
	
	if (FrontD < 0.1f) 
	{	
		*Back = CopyBrush(Brush);
		return;
	}

	if (BackD > -0.1f)
	{
		*Front = CopyBrush(Brush);
		return;
	}

	// create a new poly from the split plane
	p = CreatePolyFromPlane(&Plane);

	if (!p)
	{
		GHook.Error("Could not create poly.\n");
	}
	
	// Clip the poly by all the planes of the brush being split
	for (i=0 ; i<Brush->NumSides && p ; i++)
	{
		Plane2 = &Planes[Brush->Sides[i].PlaneNum];
		
		ClipPolyEpsilon(p, 0.0f, Plane2, !(Brush->Sides[i].PlaneSide), &p);
	}

	if (!p || PolyIsTiny (p) )
	{	
		int32	Side;

		Side = BrushMostlyOnSide(Brush, &Plane);
		
		if (Side == PSIDE_FRONT)
			*Front = CopyBrush(Brush);
		if (Side == PSIDE_BACK)
			*Back = CopyBrush(Brush);
		return;
	}

	// Store the mid poly
	MidPoly = p;					

	// Create 2 brushes
	for (i=0 ; i<2 ; i++)
	{
		Brushes[i] = AllocBrush(Brush->NumSides+1);
		
		if (!Brushes[i])
		{
			GHook.Error("SplitBrush:  Out of memory for brush.\n");
			//return NULL;
		}
		
		Brushes[i]->Original = Brush->Original;
	}

	// Split all the current polys of the brush being split, and distribute it to the other 2 brushes
	pSide = Brush->Sides;
	for (i=0 ; i<Brush->NumSides ; i++, pSide++)
	{
		GBSP_Poly		*Poly[2];
		
		if (!pSide->Poly)
			continue;

		if (!CopyPoly (pSide->Poly, &p))
		{
			GHook.Error("Error Copying poly...\n");
		}

		if (!SplitPolyEpsilon(p, 0.0f, &Plane, &Poly[0], &Poly[1], GE_FALSE))
		{
			GHook.Error("Error Splitting poly...\n");
		}

		for (j=0 ; j<2 ; j++)
		{
			GBSP_Side	*pDestSide;

			if (!Poly[j])
				continue;
			
			#if 0
				if (PolyIsTiny(Poly[j]))
				{
					FreePoly(Poly[j]);
					continue;
				}
			#endif

			pDestSide = &Brushes[j]->Sides[Brushes[j]->NumSides];
			Brushes[j]->NumSides++;
			
			*pDestSide = *pSide;
			
			pDestSide->Poly = Poly[j];
			pDestSide->Flags &= ~SIDE_TESTED;
		}
	}

	for (i=0 ; i<2 ; i++)
	{
		BoundBrush(Brushes[i]);

		if (!CheckBrush(Brushes[i]))
		{
			FreeBrush(Brushes[i]);
			Brushes[i] = NULL;
		}
	
	}


	if (!(Brushes[0] && Brushes[1]) )
	{
		
		if (!Brushes[0] && !Brushes[1])
			GHook.Printf("Split removed brush\n");
		else
			GHook.Printf("Split not on both sides\n");
		
		if (Brushes[0])
		{
			FreeBrush(Brushes[0]);
			*Front = CopyBrush(Brush);
		}
		if (Brushes[1])
		{
			FreeBrush(Brushes[1]);
			*Back = CopyBrush(Brush);
		}
		return;
	}

	for (i=0 ; i<2 ; i++)
	{
		pSide = &Brushes[i]->Sides[Brushes[i]->NumSides];
		Brushes[i]->NumSides++;

		pSide->PlaneNum = PNum;
		pSide->PlaneSide = (uint8)PSide;

		if (Visible)
			pSide->Flags |= SIDE_VISIBLE;

		pSide->Flags &= ~SIDE_TESTED;

		pSide->Flags |= MidFlags;
	
		if (!i)
		{
			pSide->PlaneSide = !pSide->PlaneSide;

			if (!ReversePoly(MidPoly, &pSide->Poly))
				GHook.Error("Error copying poly.\n");
		}
		else
			pSide->Poly = MidPoly;
	}

	{
		geFloat	v1;
		int32	i;

		for (i=0 ; i<2 ; i++)
		{
			v1 = BrushVolume(Brushes[i]);
			if (v1 < 1.0f)
			{
				FreeBrush(Brushes[i]);
				Brushes[i] = NULL;
				//GHook.Printf("Tiny volume after clip\n");
			}
		}
	}

	if (!Brushes[0] && !Brushes[1])
	{
		GHook.Error("SplitBrush:  Brush was not split.\n");
	}
	
	*Front = Brushes[0];
	*Back = Brushes[1];
}

//=======================================================================================
//	SubtractBrush
//	Result = a - b.  a, and b are NOT freed.  Result could = instance of a (a is NOT ref'd)
//=======================================================================================
GBSP_Brush *SubtractBrush(GBSP_Brush *a, GBSP_Brush *b)
{	
	GBSP_Brush	*Outside, *Inside;
	GBSP_Brush	*Front, *Back;
	int32		i;

	Inside = a;			// Default a being inside b
	Outside = NULL;

	// Splitting the inside list against each plane of brush b, only keeping peices that fall on the
	// outside
	for (i=0 ; i<b->NumSides && Inside; i++)
	{
		SplitBrush(Inside, b->Sides[i].PlaneNum, b->Sides[i].PlaneSide, SIDE_NODE, GE_FALSE, &Front, &Back);

		if (Inside != a)			// Make sure we don't free a, but free all other fragments
			FreeBrush(Inside);

		if (Front)					// Kepp all front sides, and put them in the Outside list
		{	
			Front->Next = Outside;
			Outside = Front;
		}

		Inside = Back;
	}

	if (!Inside)
	{
		FreeBrushList(Outside);		
		return a;					// Nothing on inside list, so cancel all cuts, and return original
	}
	
	FreeBrush(Inside);				// Free all inside fragments

	return Outside;					// Return what was on the outside
}

//=======================================================================================
//	BrushCanBite
//=======================================================================================
geBoolean BrushCanBite(GBSP_Brush *Brush1, GBSP_Brush *Brush2)
{
	uint32		c1, c2;

	c1 = Brush1->Original->Contents;
	c2 = Brush2->Original->Contents;

	if ((c1 & BSP_CONTENTS_DETAIL2) && !(c2 & BSP_CONTENTS_DETAIL2) )
		return GE_FALSE;

	if ((c1|c2) & BSP_CONTENTS_FLOCKING)
		return GE_FALSE;

	if (c1 & BSP_CONTENTS_SOLID2)
		return GE_TRUE;

	return GE_FALSE;
}

//=======================================================================================
//	BrushesOverlap
//=======================================================================================
geBoolean BrushesOverlap(GBSP_Brush *Brush1, GBSP_Brush *Brush2)
{
	int32		i, j;

	// Check the boxs of the brushes first
	for (i=0 ; i<3 ; i++)
	{
		if (VectorToSUB(Brush1->Mins, i) >= VectorToSUB(Brush2->Maxs, i) || 
			VectorToSUB(Brush1->Maxs, i) <= VectorToSUB(Brush2->Mins, i))
			return GE_FALSE;	
	}
	
	for (i=0 ; i<Brush1->NumSides ; i++)
	{
		for (j=0 ; j<Brush2->NumSides ; j++)
		{
			if (Brush1->Sides[i].PlaneNum == Brush2->Sides[j].PlaneNum &&
				Brush1->Sides[i].PlaneSide != Brush2->Sides[j].PlaneSide)
				return GE_FALSE;	// Since brushes are convex, opposite planes result in no overlap
		}
	}
	
	return GE_TRUE;	
}

//=======================================================================================
//	CSGBrushes
//=======================================================================================
GBSP_Brush *CSGBrushes (GBSP_Brush *Head)
{
	GBSP_Brush	*b1, *b2, *Next;
	GBSP_Brush	*Tail;
	GBSP_Brush	*Keep;
	GBSP_Brush	*Sub, *Sub2;
	int32		c1, c2;

	if (Verbose)
	{
		GHook.Printf ("---- CSGBrushes ----\n");
		GHook.Printf ("Num brushes before CSG : %5i\n", CountBrushList (Head));
	}

	Keep = NULL;

NewList:

	if (!Head)
		return NULL;

	for (Tail=Head ; Tail->Next ; Tail=Tail->Next);

	for (b1=Head ; b1 ; b1=Next)
	{
		Next = b1->Next;
		
		if (CancelRequest)
			break;

		for (b2=b1->Next ; b2 ; b2 = b2->Next)
		{
			if (CancelRequest)
				continue;			// Don't stop, let it finish this round, so things get cleaned up...
			
			if (!BrushesOverlap(b1, b2))
				continue;

			Sub = NULL;
			Sub2 = NULL;
			c1 = 999999;
			c2 = 999999;

			if (BrushCanBite(b2, b1))
			{
				Sub = SubtractBrush(b1, b2);

				if (Sub == b1)
					continue;		

				if (!Sub)
				{
					Head = RemoveBrushList(b1, b1);
					goto NewList;
				}
				c1 = CountBrushList (Sub);
			}

			if (BrushCanBite(b1, b2))
			{
				Sub2 = SubtractBrush(b2, b1);

				if (Sub2 == b2)
					continue;		

				if (!Sub2)
				{	
					FreeBrushList(Sub);
					Head = RemoveBrushList(b1, b2);
					goto NewList;
				}
				c2 = CountBrushList(Sub2);
			}

			if (!Sub && !Sub2)
				continue;		

			if (c1 > 4 && c2 > 4)
			{
				if (Sub2)
					FreeBrushList(Sub2);
				if (Sub)
					FreeBrushList(Sub);
				continue;
			}
			

			if (c1 < c2)
			{
				if (Sub2)
					FreeBrushList(Sub2);
				Tail = AddBrushListToTail (Sub, Tail);
				Head = RemoveBrushList(b1, b1);
				goto NewList;
			}
			else
			{
				if (Sub)
					FreeBrushList(Sub);
				Tail = AddBrushListToTail (Sub2, Tail);
				Head = RemoveBrushList(b1, b2);
				goto NewList;
			}
			
			
		}

		if (!b2)
		{	
			b1->Next = Keep;
			Keep = b1;
		}
	}

	if (Verbose)
		GHook.Printf("Num brushes after CSG  : %5i\n", CountBrushList (Keep));

	return Keep;
}

//
//	3dt output code
//

//=====================================================================================
//	OutputBrushes
//=====================================================================================
geBoolean OutputBrushes(char *FileName, GBSP_Brush *Brushes)
{
	FILE		*f;
	int32		i, v;
	geVec3d		*Verts;
	GBSP_Brush	*Brush;
	GBSP_Side	*Side;

	f = fopen(FileName, "wb");

	if (!f)
		return GE_FALSE;

	// Write out the header
	fprintf(f, "3dtVersion 1.6\n");
	fprintf(f, "PalLoc gedit.pal\n");
	fprintf(f, "TexLoc gedit.txl\n");

	fprintf(f, "NumBrushes %i\n", CountBrushList(Brushes));
	fprintf(f, "NumEntities 0\n");
	fprintf(f, "NumModels 0\n");
	fprintf(f, "NumGroups 0\n");

	for (Brush = Brushes; Brush; Brush = Brush->Next)
	{
		fprintf(f, "Brush blue\n");
		fprintf(f, "    Flags 0\n");
		fprintf(f, "    ModelId 0\n");
		fprintf(f, "    GroupId 0\n");
		fprintf(f, "    HullSize 0.0\n");

		fprintf(f, "    BrushFaces %i\n", Brush->NumSides);

		for (i=0; i< Brush->NumSides; i++)
		{
			Side = &Brush->Sides[i];
			
			fprintf(f, "        NumPoints %i\n", Side->Poly->NumVerts);
			Verts = Side->Poly->Verts;
			for (v=0; v< Side->Poly->NumVerts; v++)
			{
				fprintf(f, "            Vec3d %f %f %f\n", Verts[v].X, Verts[v].Y, Verts[v].Z);
			}
			fprintf(f, "        TexInfo Rotate 0 Shift 0 0 Scale 1.000000 1.000000 Name blue\n");
		}
	}

	fprintf(f, "Class CEntList\n");
	fprintf(f, "EntCount 0\n");
	fprintf(f, "CurCount 0\n");

	fclose (f);

	return GE_TRUE;
}

//=====================================================================================
//	OutputMapBrushes
//=====================================================================================
geBoolean OutputMapBrushes(char *FileName, MAP_Brush *Brushes)
{
	FILE		*f;
	int32		i, v;
	geVec3d		*Verts;
	MAP_Brush	*Brush;
	GBSP_Side	*Side;

	f = fopen(FileName, "wb");

	if (!f)
		return GE_FALSE;

	// Write out the header
	fprintf(f, "3dtVersion 1.6\n");
	fprintf(f, "PalLoc gedit.pal\n");
	fprintf(f, "TexLoc gedit.txl\n");

	fprintf(f, "NumBrushes %i\n", CountMapBrushList(Brushes));
	fprintf(f, "NumEntities 0\n");
	fprintf(f, "NumModels 0\n");
	fprintf(f, "NumGroups 0\n");

	for (Brush = Brushes; Brush; Brush = Brush->Next)
	{
		fprintf(f, "Brush blue\n");
		fprintf(f, "    Flags 0\n");
		fprintf(f, "    ModelId 0\n");
		fprintf(f, "    GroupId 0\n");
		fprintf(f, "    HullSize 0.0\n");

		fprintf(f, "    BrushFaces %i\n", Brush->NumSides);

		for (i=0; i< Brush->NumSides; i++)
		{
			Side = &Brush->OriginalSides[i];

			fprintf(f, "        NumPoints %i\n", Side->Poly->NumVerts);
			Verts = Side->Poly->Verts;
			for (v=0; v< Side->Poly->NumVerts; v++)
			{
				fprintf(f, "            Vec3d %f %f %f\n", Verts[v].X, Verts[v].Y, Verts[v].Z);
			}
			fprintf(f, "        TexInfo Rotate 0 Shift 0 0 Scale 1.000000 1.000000 Name blue\n");
		}
	}

	fprintf(f, "Class CEntList\n");
	fprintf(f, "EntCount 0\n");
	fprintf(f, "CurCount 0\n");

	fclose (f);

	return GE_TRUE;
}
