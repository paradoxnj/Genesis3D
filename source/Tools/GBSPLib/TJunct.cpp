/****************************************************************************************/
/*  TJunct.cpp                                                                          */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Removes T-Juncts                                                       */
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
#include <Stdio.h>
#include <Math.h>

#include "BSP.h"
#include "Poly.h"
#include "Texture.h"

#include "Ram.h"

#define OFF_EPSILON					0.05f

geBoolean	FixTJuncts = GE_TRUE;

#define MAX_TEMP_INDEX_VERTS		1024

int32	NumTempIndexVerts;
int32	TempIndexVerts[MAX_TEMP_INDEX_VERTS];

int32	NumWeldedVerts;
int32	TotalIndexVerts;
geVec3d	WeldedVerts[MAX_WELDED_VERTS];

int32	EdgeVerts[MAX_WELDED_VERTS];
int32	NumEdgeVerts;

int32	NumFixedFaces;
int32	NumTJunctions;

#define USE_HASHING

#define	HASH_SIZE	128							// Must be power of 2
#define	HASH_SIZE2	HASH_SIZE*HASH_SIZE			// Squared(HASH_SIZE)
#define HASH_SHIFT	8							// Log2(HASH_SIZE)+1

int32	VertexChain[MAX_WELDED_VERTS];			// the next vertex in a hash chain
int32	HashVerts[HASH_SIZE*HASH_SIZE];			// a vertex number, or 0 for no verts

//=====================================================================================
//	FinalizeFace
//=====================================================================================
geBoolean FinalizeFace(GBSP_Face *Face, int32 Base)
{
	int32	i;

	TotalIndexVerts += NumTempIndexVerts;

	if (NumTempIndexVerts == Face->NumIndexVerts)
		return GE_TRUE;

	if (TexInfo[Face->TexInfo].Flags & TEXINFO_MIRROR)
		return GE_TRUE;

	if (TexInfo[Face->TexInfo].Flags & TEXINFO_SKY)
		return GE_TRUE;

	if (Face->IndexVerts)
		geRam_Free(Face->IndexVerts);

	Face->IndexVerts = GE_RAM_ALLOCATE_ARRAY(int32,NumTempIndexVerts);
		
	for (i=0; i< NumTempIndexVerts; i++)
		Face->IndexVerts[i] = TempIndexVerts[(i+Base)%NumTempIndexVerts];

	Face->NumIndexVerts = NumTempIndexVerts;

	NumFixedFaces++;

	return GE_TRUE;
}

geVec3d	EdgeStart;
geVec3d	EdgeDir;

geBoolean TestEdge_r(geFloat Start, geFloat End, int32 p1, int32 p2, int32 StartVert)
{
	int32	j, k;
	geFloat	Dist;
	geVec3d	Delta;
	geVec3d	Exact;
	geVec3d	Off;
	geFloat	Error;
	geVec3d	p;

	if (p1 == p2)
	{
		//GHook.Printf("TestEdge_r:  Degenerate Edge.\n");
		return GE_TRUE;		// degenerate edge
	}

	for (k=StartVert ; k<NumEdgeVerts ; k++)
	{
		j = EdgeVerts[k];

		if (j==p1 || j == p2)
			continue;

		p = WeldedVerts[j];

		geVec3d_Subtract(&p, &EdgeStart, &Delta);
		Dist = geVec3d_DotProduct(&Delta, &EdgeDir);
		
		if (Dist <= Start || Dist >= End)
			continue;	
		
		geVec3d_AddScaled(&EdgeStart, &EdgeDir, Dist, &Exact);
		geVec3d_Subtract(&p, &Exact, &Off);
		Error = geVec3d_Length(&Off);

		if (fabs(Error) > OFF_EPSILON)
			continue;		

		// break the edge
		NumTJunctions++;

		TestEdge_r (Start, Dist, p1, j, k+1);
		TestEdge_r (Dist, End, j, p2, k+1);

		return GE_TRUE;
	}

	if (NumTempIndexVerts >= MAX_TEMP_INDEX_VERTS)
	{
		GHook.Error("Max Temp Index Verts.\n");
		return GE_FALSE;
	}

	TempIndexVerts[NumTempIndexVerts] = p1;
	NumTempIndexVerts++;

	return GE_TRUE;
}

void FindEdgeVerts(geVec3d *V1, geVec3d *V2)
{
#ifdef USE_HASHING
	int32	x1, y1, x2, y2;
	int32	t, x, y, Index;

	x1 = (HASH_SIZE2 + (int32)(V1->X+0.5)) >> HASH_SHIFT;
	y1 = (HASH_SIZE2 + (int32)(V1->Y+0.5)) >> HASH_SHIFT;
	x2 = (HASH_SIZE2 + (int32)(V2->X+0.5)) >> HASH_SHIFT;
	y2 = (HASH_SIZE2 + (int32)(V2->Y+0.5)) >> HASH_SHIFT;

	if (x1 > x2)
	{
		t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y1 > y2)
	{
		t = y1;
		y1 = y2;
		y2 = t;
	}

	NumEdgeVerts = 0;
	for (x=x1 ; x <= x2 ; x++)
	{
		for (y=y1 ; y <= y2 ; y++)
		{
			for (Index = HashVerts[y*HASH_SIZE+x] ; Index ; Index = VertexChain[Index])
			{
				EdgeVerts[NumEdgeVerts++] = Index;
			}
		}
	}

#else
	int32		i;

	NumEdgeVerts = NumWeldedVerts-1;

	for (i=0; i< NumEdgeVerts; i++)
	{
		EdgeVerts[i] = i+1;
	}
#endif

}

//=====================================================================================
//	FixFaceTJunctions
//=====================================================================================
geBoolean FixFaceTJunctions(GBSP_Node *Node, GBSP_Face *Face)
{
	int32	i, P1, P2;
	int32	Start[MAX_TEMP_INDEX_VERTS];
	int32	Count[MAX_TEMP_INDEX_VERTS];
	geVec3d	Edge2;
	geFloat	Len;
	int32	Base;

	NumTempIndexVerts = 0;
	
	for (i=0; i< Face->NumIndexVerts; i++)
	{
		P1 = Face->IndexVerts[i];
		P2 = Face->IndexVerts[(i+1)%Face->NumIndexVerts];

		EdgeStart = WeldedVerts[P1];
		Edge2 = WeldedVerts[P2];

		FindEdgeVerts(&EdgeStart, &Edge2);

		geVec3d_Subtract(&Edge2, &EdgeStart, &EdgeDir);
		Len = geVec3d_Normalize(&EdgeDir);

		Start[i] = NumTempIndexVerts;

		TestEdge_r(0.0f, Len, P1, P2, 0);

		Count[i] = NumTempIndexVerts - Start[i];
	}

	if (NumTempIndexVerts < 3)
	{
		Face->NumIndexVerts = 0;

		//GHook.Printf("FixFaceTJunctions:  Face collapsed.\n");
		return GE_TRUE;
	}

	for (i=0 ; i<Face->NumIndexVerts; i++)
	{
		if (Count[i] == 1 && Count[(i+Face->NumIndexVerts-1)%Face->NumIndexVerts] == 1)
			break;
	}

	if (i == Face->NumIndexVerts)
	{
		Base = 0;
	}
	else
	{	// rotate the vertex order
		Base = Start[i];
	}

	if (!FinalizeFace(Face, Base))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	FixTJunctions_r
//=====================================================================================
geBoolean FixTJunctions_r(GBSP_Node *Node)
{
	GBSP_Face	*Face, *Next;

	if (Node->PlaneNum == PLANENUM_LEAF)
		return GE_TRUE;

	for (Face = Node->Faces; Face; Face = Next)
	{
		Next = Face->Next;

		if (Face->Merged || Face->Split[0] || Face->Split[1])
			continue;

		FixFaceTJunctions(Node, Face);
	}
	
	if (!FixTJunctions_r(Node->Children[0]))
		return GE_FALSE;

	if (!FixTJunctions_r(Node->Children[1]))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	HashVert
//=====================================================================================
int32 HashVert(geVec3d *Vert)
{
	int32	x, y;

	x = (HASH_SIZE2 + (int32)(Vert->X + 0.5)) >> HASH_SHIFT;
	y = (HASH_SIZE2 + (int32)(Vert->Y + 0.5)) >> HASH_SHIFT;

	if ( x < 0 || x >= HASH_SIZE || y < 0 || y >= HASH_SIZE )
	{
		GHook.Error ("HashVert: Vert outside valid range");
		return -1;
	}
	
	return y*HASH_SIZE + x;
}

#define INTEGRAL_EPSILON	0.01f

//=====================================================================================
//	WeldVert
//=====================================================================================
#ifdef USE_HASHING
int32 WeldVert(geVec3d *Vert)
{
	int32		i, h;

	for (i=0; i<3; i++)
		if (fabs(VectorToSUB(*Vert, i) - (int32)(VectorToSUB(*Vert, i)+0.5)) < INTEGRAL_EPSILON )
			VectorToSUB(*Vert, i) = (geFloat)((int32)(VectorToSUB(*Vert, i)+0.5));

	h = HashVert(Vert);

	if (h == -1)
		return -1;

	for (i=HashVerts[h]; i; i = VertexChain[i])
	{
		if (i >= MAX_WELDED_VERTS)
		{
			GHook.Error("WeldVert:  Invalid hash vert.\n");
			return -1;
		}

		if (geVec3d_Compare(Vert, &WeldedVerts[i], VCOMPARE_EPSILON))
			return i;
	}

	if (NumWeldedVerts >= MAX_WELDED_VERTS)
	{
		GHook.Error("WeldVert:  Max welded verts.\n");
		return -1;
	}

	WeldedVerts[NumWeldedVerts] = *Vert;

	VertexChain[NumWeldedVerts] = HashVerts[h];
	HashVerts[h] = NumWeldedVerts;

	NumWeldedVerts++;

	return NumWeldedVerts-1;
}
#else
int32 WeldVert(geVec3d *Vert)
{
	int32	i;
	geVec3d	*pWeldedVerts;

	pWeldedVerts = WeldedVerts;

	for (i=0; i< NumWeldedVerts; i++)
	{
		if (geVec3d_Compare(Vert, pWeldedVerts, VCOMPARE_EPSILON))
			return i;

		pWeldedVerts++;
	}

	if (i >= MAX_WELDED_VERTS)
	{
		GHook.Error("WeldVert:  Max welded verts.\n");
		return -1;
	}

	WeldedVerts[i] = *Vert;

	NumWeldedVerts++;

	return i;
}
#endif
//=====================================================================================
//	GetFaceVertIndexNumbers
//=====================================================================================
geBoolean GetFaceVertIndexNumbers(GBSP_Face *Face)
{
	int32	i, Index;
	geVec3d	*Verts;

	NumTempIndexVerts = 0;
	
	Verts = Face->Poly->Verts;

	for (i=0; i< Face->Poly->NumVerts; i++)
	{
		if (NumTempIndexVerts >= MAX_TEMP_INDEX_VERTS)
		{
			GHook.Error("GetFaceVertIndexNumbers:  Max Temp Index Verts.\n");
			return GE_FALSE;
		}

		Index = WeldVert(&Verts[i]);

		if (Index == -1)
		{
			GHook.Error("GetFaceVertIndexNumbers:  Could not FindVert.\n");
			return GE_FALSE;
		}

		TempIndexVerts[NumTempIndexVerts] = Index;
		NumTempIndexVerts++;

		TotalIndexVerts++;
	}

	Face->NumIndexVerts = NumTempIndexVerts;
	Face->IndexVerts = GE_RAM_ALLOCATE_ARRAY(int32,NumTempIndexVerts);

	if (!Face->IndexVerts)
	{
		GHook.Error("GetFaceVertIndexNumbers:  Out of memory for index list.\n");
		return GE_FALSE;
	}

	for (i=0; i < NumTempIndexVerts; i++)
		Face->IndexVerts[i] = TempIndexVerts[i];

	return GE_TRUE;
}

//=====================================================================================
//	GetFaceVertIndexNumbers_r
//=====================================================================================
geBoolean GetFaceVertIndexNumbers_r(GBSP_Node *Node)
{
	GBSP_Face	*Face, *Next;

	if (Node->PlaneNum == PLANENUM_LEAF)
		return GE_TRUE;

	for (Face = Node->Faces; Face; Face = Next)
	{
		Next = Face->Next;

		if (Face->Merged || Face->Split[0] || Face->Split[1])
			continue;

		if (!GetFaceVertIndexNumbers(Face))
			return GE_FALSE;
	}
	
	if (!GetFaceVertIndexNumbers_r(Node->Children[0]))
		return GE_FALSE;

	if (!GetFaceVertIndexNumbers_r(Node->Children[1]))
		return GE_FALSE;

	return GE_TRUE;
}

//=====================================================================================
//	FixModelTJunctions
//=====================================================================================
geBoolean FixModelTJunctions(void)
{
	int32		i;

	GHook.Printf(" --- Weld Model Verts --- \n");

	NumWeldedVerts = 0;
	TotalIndexVerts = 0;
	
	for (i=0; i< NumBSPModels; i++)
	{

		if (!GetFaceVertIndexNumbers_r(BSPModels[i].RootNode[0]))
			return GE_FALSE;
	}

	if (!FixTJuncts)		// Skip if asked to do so...
		return GE_TRUE;

	GHook.Printf(" --- Fix Model TJunctions --- \n");

	TotalIndexVerts = 0;

	NumTJunctions = 0;
	NumFixedFaces = 0;

	for (i=0; i< NumBSPModels; i++)
	{

		if (!FixTJunctions_r(BSPModels[i].RootNode[0]))
			return GE_FALSE;
	}

	if (Verbose)
	{
		GHook.Printf(" Num TJunctions        : %5i\n", NumTJunctions);
		GHook.Printf(" Num Fixed Faces       : %5i\n", NumFixedFaces);
	}

	return GE_TRUE;
}