/****************************************************************************************/
/*  Poly.Cpp                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Various Poly routines (clipping, splitting, etc)                       */
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
#include <math.h>

#include "BSP.h"
#include "Mathlib.h"
#include "Map.h"
#include "Poly.h"
#include "Texture.h"
#include "GBSPFile.h"
#include "Light.h"

#include "Ram.h"

int32		NumSubdivides;
float		SubdivideSize = 235.0f;
//float		SubdivideSize = ((float)((16*MAX_LMAP_SIZE)-20));
//float		SubdivideSize = ((float)((16*MAX_LMAP_SIZE)-32));

int32		NumSubdivided;
int32		NumMerged;

//#define FREAKED_OUT

geBoolean	gCountVerts;
int32		gTotalVerts;
int32		gPeekVerts;	

//#define DEGENERATE_EPSILON		0.05f
#define DEGENERATE_EPSILON			0.001f

//
//	Polys
//

//====================================================================================
//	AllocPoly
//====================================================================================
GBSP_Poly *AllocPoly(int32 NumVerts)
{
	GBSP_Poly	*NewPoly;

	if (NumVerts > 1000)
	{
		GHook.Error("Bogus verts: %i\n", NumVerts);
		return NULL;
	}
	
	NewPoly = GE_RAM_ALLOCATE_STRUCT(GBSP_Poly);

	if (!NewPoly)
	{
		GHook.Error("AllocPoly:  Not enough memory.\n");
		return NULL;
	}
	
	NewPoly->Verts = GE_RAM_ALLOCATE_ARRAY(geVec3d,NumVerts);
	
	if (!NewPoly->Verts)
	{
		GHook.Error("AllocPoly:  Not enough memory for verts: %i\n", NumVerts);
		geRam_Free(NewPoly);
		return NULL;
	}

	NewPoly->NumVerts = NumVerts;

#ifdef SHOW_DEBUG_STATS
	if (gCountVerts)
	{
		gTotalVerts += NumVerts;
		if (gTotalVerts > gPeekVerts)
			gPeekVerts = gTotalVerts;
	}
#endif

	return NewPoly;
}

//====================================================================================
//	FreePoly
//====================================================================================
void FreePoly(GBSP_Poly *Poly)
{
	if (!Poly)
	{
		GHook.Printf("*WARNING* FreePoly: NULL Poly.\n");
		return;
	}

	if (Poly->Verts)
	{
		geRam_Free(Poly->Verts);

	#ifdef SHOW_DEBUG_STATS
		if (gCountVerts)
		{
			gTotalVerts -= Poly->NumVerts;
		}
	#endif
	}

	geRam_Free(Poly);
}

//=====================================================================================
//	TextureAxisFromPlane
//=====================================================================================
geBoolean TextureAxisFromPlane(GBSP_Plane *Pln, geVec3d *Xv, geVec3d *Yv)
{
	int32	BestAxis;
	geFloat	Dot,Best;
	int32		i;
	
	Best = 0.0f;
	BestAxis = -1;
	
	for (i=0 ; i<3 ; i++)
	{
		Dot = (float)fabs(VectorToSUB(Pln->Normal, i));
		if (Dot > Best)
		{
			Best = Dot;
			BestAxis = i;
		}
	}

	switch(BestAxis)
	{
		case 0:						// X
			Xv->X = (geFloat)0;
			Xv->Y = (geFloat)0;
			Xv->Z = (geFloat)1;

			Yv->X = (geFloat)0;
			Yv->Y = (geFloat)-1;
			Yv->Z = (geFloat)0;
			break;
		case 1:						// Y
			Xv->X = (geFloat)1;
			Xv->Y = (geFloat)0;
			Xv->Z = (geFloat)0;

			Yv->X = (geFloat)0;
			Yv->Y = (geFloat)0;
			Yv->Z = (geFloat)1;
			break;
		case 2:						// Z
			Xv->X = (geFloat)1;
			Xv->Y = (geFloat)0;
			Xv->Z = (geFloat)0;

			Yv->X = (geFloat)0;
			Yv->Y = (geFloat)-1;
			Yv->Z = (geFloat)0;
			break;
		default:
			GHook.Error("TextureAxisFromPlane: No Axis found.\n");
			return GE_FALSE;
			break;
	}

	return GE_TRUE;
}

//====================================================================================
//	CreatePolyFromPlane	
//	Create a huge poly with normal pointing in same direction as Plane
//====================================================================================
GBSP_Poly *CreatePolyFromPlane(GBSP_Plane *Plane)
{
	geVec3d		Normal = Plane->Normal;
	geVec3d		UpVect = {0.0f, 0.0f, 0.0f};
	geVec3d		RightVect, Org;
	GBSP_Poly	*Poly;
	
	if (!TextureAxisFromPlane(Plane, &RightVect, &UpVect))
		return NULL;

	// Produce some walk vectors
	geVec3d_CrossProduct(&Normal, &UpVect, &RightVect);		// Get right vector
	geVec3d_CrossProduct(&Normal, &RightVect, &UpVect);		// Get new Up vector from correct Right vector

	geVec3d_Normalize(&UpVect);
	geVec3d_Normalize(&RightVect);

	// Create the poly with 4 verts
	Poly = AllocPoly(4);

	if (!Poly)
	{
		GHook.Error("CreatePolyFromPlane:  Not enough memory for new poly!\n");
		return NULL;
	}

	geVec3d_Scale(&Normal, Plane->Dist, &Org);						// Get the Org
	geVec3d_Scale(&UpVect, (geFloat)MIN_MAX_BOUNDS, &UpVect);			// Scale walk vectors
	geVec3d_Scale(&RightVect, (geFloat)MIN_MAX_BOUNDS, &RightVect);

	geVec3d_Subtract(&Org, &RightVect, &Poly->Verts[0]);
	geVec3d_Add(&Poly->Verts[0], &UpVect, &Poly->Verts[0]);

	geVec3d_Add(&Org, &RightVect, &Poly->Verts[1]);
	geVec3d_Add(&Poly->Verts[1], &UpVect, &Poly->Verts[1]);

	geVec3d_Add(&Org, &RightVect, &Poly->Verts[2]);
	geVec3d_Subtract(&Poly->Verts[2], &UpVect, &Poly->Verts[2]);

	geVec3d_Subtract(&Org, &RightVect, &Poly->Verts[3]);
	geVec3d_Subtract(&Poly->Verts[3], &UpVect, &Poly->Verts[3]);

#ifdef FREAKED_OUT	
	geVec3d	Normal2, V1, V2;
	geVec3d_Subtract(&Poly->Verts[0], &Poly->Verts[1], &V1);
	geVec3d_Subtract(&Poly->Verts[2], &Poly->Verts[1], &V2);
	geVec3d_CrossProduct(&V1, &V2, &Normal2);
	geVec3d_Normalize(&Normal2);
	
	Normal = Plane->Normal;
	
	if (!geVec3d_Compare(&Normal, &Normal2, VCOMPARE_EPSILON))
	{
		GHook.Error("Normal1 X:%2.2f, Y:%2.2f, Z:%2.2f\n", Normal.X, Normal.Y, Normal.Z);
		GHook.Error("Normal2 X:%2.2f, Y:%2.2f, Z:%2.2f\n", Normal2.X, Normal2.Y, Normal2.Z);
		GHook.Error("CreatePolyFromPlane:  Normals do not match.\n");
		return NULL;
	}
#endif	

	return Poly;

}

#define MAX_TEMP_VERTS	200

geVec3d TempVerts[MAX_TEMP_VERTS];
geVec3d TempVerts2[MAX_TEMP_VERTS];

#define CLIP_EPSILON	(geFloat)0.001

//====================================================================================
//	ClipPoly
//====================================================================================
geBoolean ClipPoly(GBSP_Poly *InPoly, GBSP_Plane *Plane, geBoolean FlipTest, GBSP_Poly **OutPoly)
{
	GBSP_Poly	*NewPoly = NULL;
	geVec3d		*pInVert, Vert1, Vert2;
	geVec3d		*pFrontVert, *Verts;
	int32		i, NextVert, NewNumVerts = 0;
	geFloat		Scale;

	geVec3d		Normal = Plane->Normal;
	geFloat		Dist = Plane->Dist;
	int32		NumVerts = InPoly->NumVerts;
	int32		VSides[100];
	geFloat		VDist[100];
	int32		CountSides[3];

	*OutPoly = NULL;

	if (NumVerts >= 100)
	{
		GHook.Error("ClipPoly:  Too many verts.\n");
		return GE_FALSE;
	}
	
	memset(CountSides, 0, sizeof(CountSides));

	Verts = InPoly->Verts;
	pInVert = Verts;
	pFrontVert = TempVerts;
	
	if (FlipTest)		// Flip the normal and dist
	{
		Normal.X = -Normal.X;
		Normal.Y = -Normal.Y;
		Normal.Z = -Normal.Z;
		Dist = -Dist;
	}
	
	// See what side of plane each vert is on
	for (i=0; i< NumVerts; i++)
	{
		VDist[i] = geVec3d_DotProduct(&Verts[i], &Normal) - Dist;
		if (VDist[i] > ON_EPSILON)
			VSides[i] = 0;
		else if (VDist[i] < -ON_EPSILON)
			VSides[i] = 1;
		else
			VSides[i] = 2;

		CountSides[VSides[i]]++;
	}

	if (!CountSides[0])
	{
		FreePoly(InPoly);
		*OutPoly = NULL;
		return GE_TRUE;
	}

	if (!CountSides[1])
	{
		*OutPoly = InPoly;
		return GE_TRUE;
	}

	// Else we have to split this sucker
	for (i=0; i< NumVerts; i++)
	{
		Vert1 = Verts[i];

		if (VSides[i] == 2)				// On plane, put on both sides
		{
            *pFrontVert++ = Vert1;
			continue;
		}
		
		if (VSides[i] == 0)				// Front side, put on front list
            *pFrontVert++ = Vert1;

		NextVert = (i+1) % NumVerts;
	 	
		if (VSides[NextVert] == 2 || VSides[NextVert] == VSides[i])
			continue;

		Vert2 = Verts[NextVert];
		Scale = VDist[i] / (VDist[i] - VDist[NextVert]);

		pFrontVert->X = Vert1.X + (Vert2.X - Vert1.X) * Scale;
		pFrontVert->Y = Vert1.Y + (Vert2.Y - Vert1.Y) * Scale;
		pFrontVert->Z = Vert1.Z + (Vert2.Z - Vert1.Z) * Scale;

		pFrontVert++;
	}

	FreePoly(InPoly);		// Don't need this anymore

    NewNumVerts = pFrontVert - TempVerts;

	if (NewNumVerts < 3)
	{
		*OutPoly = NULL;
		return GE_TRUE;
	}
	
	NewPoly = AllocPoly(NewNumVerts);

	if (!NewPoly)
	{
		GHook.Error("ClipPoly:  Not enough mem for new poly.\n");
		return GE_FALSE;
	}

	for (i = 0; i< NewNumVerts; i++)
		NewPoly->Verts[i] =	TempVerts[i];

	*OutPoly = NewPoly;
	return GE_TRUE;
}

//====================================================================================
//	ClipPolyEpsilon
//====================================================================================
geBoolean ClipPolyEpsilon(GBSP_Poly *InPoly, geFloat Epsilon, GBSP_Plane *Plane, geBoolean FlipTest, GBSP_Poly **OutPoly)
{
	GBSP_Poly	*NewPoly = NULL;
	geVec3d		*pInVert, Vert1, Vert2;
	geVec3d		*pFrontVert, *Verts;
	int32		i, NextVert, NewNumVerts = 0;
	geFloat		Scale;

	geVec3d		Normal = Plane->Normal;
	geFloat		Dist = Plane->Dist;
	int32		NumVerts = InPoly->NumVerts;
	int32		VSides[100];
	geFloat		VDist[100];
	int32		CountSides[3];

	*OutPoly = NULL;

	if (NumVerts >= 100)
	{
		GHook.Error("ClipPoly:  Too many verts.\n");
		return GE_FALSE;
	}
	
	memset(CountSides, 0, sizeof(CountSides));

	Verts = InPoly->Verts;
	pInVert = Verts;
	pFrontVert = TempVerts;
	
	if (FlipTest)		// Flip the normal and dist
	{
		Normal.X = -Normal.X;
		Normal.Y = -Normal.Y;
		Normal.Z = -Normal.Z;
		Dist = -Dist;
	}
	
	// See what side of plane each vert is on
	for (i=0; i< NumVerts; i++)
	{
		VDist[i] = geVec3d_DotProduct(&Verts[i], &Normal) - Dist;
		if (VDist[i] > Epsilon)
			VSides[i] = 0;
		else if (VDist[i] < -Epsilon)
			VSides[i] = 1;
		else
			VSides[i] = 2;

		CountSides[VSides[i]]++;
	}

	if (!CountSides[0])
	{
		FreePoly(InPoly);
		*OutPoly = NULL;
		return GE_TRUE;
	}

	if (!CountSides[1])
	{
		*OutPoly = InPoly;
		return GE_TRUE;
	}

	// Else we have to split this sucker
	for (i=0; i< NumVerts; i++)
	{
		Vert1 = Verts[i];

		if (VSides[i] == 2)				// On plane, put on both sides
		{
            *pFrontVert++ = Vert1;
			continue;
		}
		
		if (VSides[i] == 0)				// Front side, put on front list
            *pFrontVert++ = Vert1;

		NextVert = (i+1) % NumVerts;
	 	
		if (VSides[NextVert] == 2 || VSides[NextVert] == VSides[i])
			continue;

		Vert2 = Verts[NextVert];
		Scale = VDist[i] / (VDist[i] - VDist[NextVert]);

		pFrontVert->X = Vert1.X + (Vert2.X - Vert1.X) * Scale;
		pFrontVert->Y = Vert1.Y + (Vert2.Y - Vert1.Y) * Scale;
		pFrontVert->Z = Vert1.Z + (Vert2.Z - Vert1.Z) * Scale;

		pFrontVert++;
	}

	FreePoly(InPoly);		// Don't need this anymore

    NewNumVerts = pFrontVert - TempVerts;

	if (NewNumVerts < 3)
	{
		*OutPoly = NULL;
		return GE_TRUE;
	}
	
	NewPoly = AllocPoly(NewNumVerts);

	if (!NewPoly)
	{
		GHook.Error("ClipPoly:  Not enough mem for new poly.\n");
		return GE_FALSE;
	}

	for (i = 0; i< NewNumVerts; i++)
		NewPoly->Verts[i] =	TempVerts[i];

	*OutPoly = NewPoly;
	return GE_TRUE;
}

//====================================================================================
//	SplitPoly
//====================================================================================
geBoolean SplitPoly(GBSP_Poly *InPoly, GBSP_Plane *Plane, GBSP_Poly **Front, GBSP_Poly **Back, 
			   geBoolean FlipTest)
{
	GBSP_Poly	*NewPoly = NULL;
	geVec3d		*pInVert, Vert1, Vert2;
	geVec3d		*pFrontVert, *pBackVert, *Verts;
	int32		i, NextVert, NewNumVerts = 0;
	geFloat		Scale;

	geVec3d		Normal = Plane->Normal;
	geFloat		Dist = Plane->Dist, *pDist;
	int32		NumVerts = InPoly->NumVerts;
	int32		VSides[100];
	geFloat		VDist[100];
	int32		CountSides[3];

	if (NumVerts >= 100)
	{
		GHook.Error("SplitPoly:  Too many verts.\n");
		return GE_FALSE;
	}
	
	memset(CountSides, 0, sizeof(CountSides));

	Verts = InPoly->Verts;
	pInVert = Verts;
	pFrontVert = TempVerts;
	pBackVert = TempVerts2;
	
	if (FlipTest)		// Flip the normal and dist
	{
		Normal.X = -Normal.X;
		Normal.Y = -Normal.Y;
		Normal.Z = -Normal.Z;
		Dist = -Dist;
	}
	
	// See what side of plane each vert is on
	if (Plane->Type < PLANE_ANYX)
	{
		pDist = &VectorToSUB(Verts[0], Plane->Type);

		for (i=0; i< NumVerts ; i++)
		{

			//VDist[i] = VectorToSUB(Verts[i], Plane->Type) - Plane->Dist;
			VDist[i] = *pDist - Plane->Dist;
			if (FlipTest)
				VDist[i] = -VDist[i];

			if (VDist[i] > ON_EPSILON)
				VSides[i] = 0;
			else if (VDist[i] < -ON_EPSILON)
				VSides[i] = 1;
			else
				VSides[i] = 2;

			CountSides[VSides[i]]++;

			pDist += 3;
		}
	}
	else
	{
		for (i=0; i< NumVerts; i++)
		{
			VDist[i] = geVec3d_DotProduct(&Verts[i], &Normal) - Dist;
			if (VDist[i] > ON_EPSILON)
				VSides[i] = 0;
			else if (VDist[i] < -ON_EPSILON)
				VSides[i] = 1;
			else
				VSides[i] = 2;

			CountSides[VSides[i]]++;
		}
	}

	if (!CountSides[0] && !CountSides[1])
	{
		*Back = NULL;
		*Front = InPoly;
		return GE_TRUE;
	}

	// Get out quick, if no splitting is neccesary
	if (!CountSides[0])
	{
		*Front = NULL;
		*Back = InPoly;
		return GE_TRUE;
	}
	if (!CountSides[1])
	{
		*Back = NULL;
		*Front = InPoly;
		return GE_TRUE;
	}

	// Else we have to split this sucker
	for (i=0; i< NumVerts; i++)
	{
		Vert1 = Verts[i];

		if (VSides[i] == 2)				// On plane, put on both sides
		{
            *pFrontVert++ = Vert1;
            *pBackVert++ = Vert1;
			continue;
		}
		
		if (VSides[i] == 0)				// Front side, put on front list
            *pFrontVert++ = Vert1;
		else if (VSides[i] == 1)		// Back side, put on back list
            *pBackVert++ = Vert1;


		NextVert = (i+1) % NumVerts;
	 	
		if (VSides[NextVert] == 2 || VSides[NextVert] == VSides[i])
			continue;

		Vert2 = Verts[NextVert];
		Scale = VDist[i] / (VDist[i] - VDist[NextVert]);

		pFrontVert->X = Vert1.X + (Vert2.X - Vert1.X) * Scale;
		pFrontVert->Y = Vert1.Y + (Vert2.Y - Vert1.Y) * Scale;
		pFrontVert->Z = Vert1.Z + (Vert2.Z - Vert1.Z) * Scale;

		*pBackVert = *pFrontVert;
		pFrontVert++;
		pBackVert++;
	}

	FreePoly(InPoly);		// Don't need this anymore

    NewNumVerts = pFrontVert - TempVerts;

	if (NewNumVerts < 3)
		*Front = NULL;
	else
	{
		NewPoly = AllocPoly(NewNumVerts);

		if (!NewPoly)
		{
			GHook.Error("SplitPoly:  Not enough mem for new poly.\n");
			return GE_FALSE;
		}

		for (i = 0; i< NewNumVerts; i++)
			NewPoly->Verts[i] =	TempVerts[i];

		*Front = NewPoly;
	}
    
	NewNumVerts = pBackVert - TempVerts2;
	
	if (NewNumVerts < 3)
		*Back = NULL;
	else
	{
		NewPoly = AllocPoly(NewNumVerts);

		if (!NewPoly)
		{
			GHook.Error("SplitPoly:  Not enough mem for new poly.\n");
			return GE_FALSE;
		}

		for (i = 0; i< NewNumVerts; i++)
			NewPoly->Verts[i] = TempVerts2[i];

		*Back = NewPoly;
	}

	return GE_TRUE;
}

//====================================================================================
//	SplitPoly
//====================================================================================
geBoolean SplitPolyEpsilon(GBSP_Poly *InPoly, geFloat Epsilon, GBSP_Plane *Plane, GBSP_Poly **Front, GBSP_Poly **Back, 
			   geBoolean FlipTest)
{
	GBSP_Poly	*NewPoly = NULL;
	geVec3d		*pInVert, Vert1, Vert2;
	geVec3d		*pFrontVert, *pBackVert, *Verts;
	int32		i, NextVert, NewNumVerts = 0;
	geFloat		Scale;

	geVec3d		Normal = Plane->Normal;
	geFloat		Dist = Plane->Dist, *pDist;
	int32		NumVerts = InPoly->NumVerts;
	int32		VSides[100];
	geFloat		VDist[100];
	int32		CountSides[3];

	if (NumVerts >= 100)
	{
		GHook.Error("SplitPoly:  Too many verts.\n");
		return GE_FALSE;
	}
	
	memset(CountSides, 0, sizeof(CountSides));

	Verts = InPoly->Verts;
	pInVert = Verts;
	pFrontVert = TempVerts;
	pBackVert = TempVerts2;
	
	if (FlipTest)		// Flip the normal and dist
	{
		Normal.X = -Normal.X;
		Normal.Y = -Normal.Y;
		Normal.Z = -Normal.Z;
		Dist = -Dist;
	}
	
	// See what side of plane each vert is on
	if (Plane->Type < PLANE_ANYX)
	{
		pDist = &VectorToSUB(Verts[0], Plane->Type);

		for (i=0; i< NumVerts ; i++)
		{

			//VDist[i] = VectorToSUB(Verts[i], Plane->Type) - Plane->Dist;
			VDist[i] = *pDist - Plane->Dist;
			if (FlipTest)
				VDist[i] = -VDist[i];

			if (VDist[i] > Epsilon)
				VSides[i] = 0;
			else if (VDist[i] < -Epsilon)
				VSides[i] = 1;
			else
				VSides[i] = 2;

			CountSides[VSides[i]]++;

			pDist += 3;
		}
	}
	else
	{
		for (i=0; i< NumVerts; i++)
		{
			VDist[i] = geVec3d_DotProduct(&Verts[i], &Normal) - Dist;
			if (VDist[i] > Epsilon)
				VSides[i] = 0;
			else if (VDist[i] < -Epsilon)
				VSides[i] = 1;
			else
				VSides[i] = 2;

			CountSides[VSides[i]]++;
		}
	}

	// Get out quick, if no splitting is neccesary
	if (!CountSides[0])
	{
		*Front = NULL;
		*Back = InPoly;
		return GE_TRUE;
	}
	if (!CountSides[1])
	{
		*Back = NULL;
		*Front = InPoly;
		return GE_TRUE;
	}

	// Else we have to split this sucker
	for (i=0; i< NumVerts; i++)
	{
		Vert1 = Verts[i];

		if (VSides[i] == 2)				// On plane, put on both sides
		{
            *pFrontVert++ = Vert1;
            *pBackVert++ = Vert1;
			continue;
		}
		
		if (VSides[i] == 0)				// Front side, put on front list
            *pFrontVert++ = Vert1;
		else if (VSides[i] == 1)		// Back side, put on back list
            *pBackVert++ = Vert1;


		NextVert = (i+1) % NumVerts;
	 	
		if (VSides[NextVert] == 2 || VSides[NextVert] == VSides[i])
			continue;

		Vert2 = Verts[NextVert];
		Scale = VDist[i] / (VDist[i] - VDist[NextVert]);

		pFrontVert->X = Vert1.X + (Vert2.X - Vert1.X) * Scale;
		pFrontVert->Y = Vert1.Y + (Vert2.Y - Vert1.Y) * Scale;
		pFrontVert->Z = Vert1.Z + (Vert2.Z - Vert1.Z) * Scale;

		*pBackVert = *pFrontVert;
		pFrontVert++;
		pBackVert++;
	}

	FreePoly(InPoly);		// Don't need this anymore

    NewNumVerts = pFrontVert - TempVerts;

	if (NewNumVerts < 3)
		*Front = NULL;
	else
	{
		NewPoly = AllocPoly(NewNumVerts);

		if (!NewPoly)
		{
			GHook.Error("SplitPoly:  Not enough mem for new poly.\n");
			return GE_FALSE;
		}

		for (i = 0; i< NewNumVerts; i++)
			NewPoly->Verts[i] =	TempVerts[i];

		*Front = NewPoly;
	}
    
	NewNumVerts = pBackVert - TempVerts2;
	
	if (NewNumVerts < 3)
		*Back = NULL;
	else
	{
		NewPoly = AllocPoly(NewNumVerts);

		if (!NewPoly)
		{
			GHook.Error("SplitPoly:  Not enough mem for new poly.\n");
			return GE_FALSE;
		}

		for (i = 0; i< NewNumVerts; i++)
			NewPoly->Verts[i] = TempVerts2[i];

		*Back = NewPoly;
	}

	return GE_TRUE;
}

//====================================================================================
//	RemoveDegenerateEdges
//====================================================================================
void RemoveDegenerateEdges(GBSP_Poly *Poly)
{
	int32		i, NumVerts;
	geVec3d		*Verts, V1, V2, NVerts[1000], Vec;
	geBoolean	Bad = GE_FALSE;
	int32		NumNVerts = 0;

	Verts = Poly->Verts;
	NumVerts = Poly->NumVerts;

	for (i=0; i< NumVerts; i++)
	{
		V1 = Verts[i];
		V2 = Verts[(i+1)%NumVerts];

		geVec3d_Subtract(&V1, &V2, &Vec);

		if (geVec3d_Length(&Vec) > DEGENERATE_EPSILON)
		{
			NVerts[NumNVerts++] = V1;	
		}
		else
			Bad = GE_TRUE;
	}

	if (Bad)
	{
		//Hook.Printf("Removing degenerate edge...\n");

		Poly->NumVerts = NumNVerts;
		for (i=0; i< NumNVerts; i++)
		{
			Verts[i] = NVerts[i];
		}
	}
}

//====================================================================================
//	RemoveDegenerateEdges2
//====================================================================================
geBoolean RemoveDegenerateEdges2(GBSP_Poly *Poly)
{
	int32		i, NumVerts;
	geVec3d		*Verts, V1, V2, NVerts[1000], Vec;
	geBoolean	Bad = GE_FALSE;
	int32		NumNVerts = 0;

	Verts = Poly->Verts;
	NumVerts = Poly->NumVerts;

	for (i=0; i< NumVerts; i++)
	{
		V1 = Verts[i];
		V2 = Verts[(i+1)%NumVerts];

		geVec3d_Subtract(&V1, &V2, &Vec);

		if (geVec3d_Length(&Vec) > DEGENERATE_EPSILON)
		{
			NVerts[NumNVerts++] = V1;	
		}
		else
			Bad = GE_TRUE;
	}

	if (NumNVerts < 3)
		return GE_FALSE;

	if (Bad)
	{
		Poly->NumVerts = NumNVerts;
		for (i=0; i< NumNVerts; i++)
		{
			Verts[i] = NVerts[i];
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	PolyArea
//====================================================================================
float PolyArea(GBSP_Poly *Poly)
{
	int32	i;
	geVec3d	Vect1, Vect2, Cross;
	geFloat	Total;

	Total = 0.0f;

	for (i=2 ; i<Poly->NumVerts ; i++)
	{
		geVec3d_Subtract(&Poly->Verts[i-1], &Poly->Verts[0], &Vect1);
		geVec3d_Subtract(&Poly->Verts[i]  , &Poly->Verts[0], &Vect2);
		geVec3d_CrossProduct (&Vect1, &Vect2, &Cross);
		Total += (geFloat)0.5 * geVec3d_Length(&Cross);
	}

	return (float)Total;
}

#define BOGUS_VERTS		256
//====================================================================================
//	CopyPoly
//====================================================================================
geBoolean CopyPoly(GBSP_Poly *In, GBSP_Poly **Out)
{
	GBSP_Poly	*NewPoly;
	int32		i;

	if (!In)
	{
		GHook.Error("CopyPoly:  NULL Poly!!!\n");
		return GE_FALSE;
	}

	if (In->NumVerts <= 0 || In->NumVerts >= BOGUS_VERTS)
	{
		GHook.Error("CopyPoly:  Bogus number of verts:  %i\n", In->NumVerts);
		return GE_FALSE;
	}
	
	NewPoly = AllocPoly(In->NumVerts);
	if (!NewPoly)
	{
		return GE_FALSE;
	}

	for (i=0; i<In->NumVerts; i++)
		NewPoly->Verts[i] = In->Verts[i];
	
	*Out = NewPoly;

	return GE_TRUE;
}

//====================================================================================
//	CopyPoly2
//====================================================================================
GBSP_Poly *CopyPoly2(GBSP_Poly *In)
{
	GBSP_Poly	*NewPoly;
	int32		i;

	if (!In)
	{
		GHook.Error("CopyPoly:  NULL Poly!!!\n");
		return NULL;
	}

	if (In->NumVerts <= 0 || In->NumVerts >= BOGUS_VERTS)
	{
		GHook.Error("CopyPoly:  Bogus number of verts:  %i\n", In->NumVerts);
		return NULL;
	}
	
	NewPoly = AllocPoly(In->NumVerts);

	if (!NewPoly)
		return NULL;

	for (i=0; i<In->NumVerts; i++)
		NewPoly->Verts[i] = In->Verts[i];
	
	return NewPoly;
}

//====================================================================================
//	ReversePoly
//====================================================================================
geBoolean ReversePoly(GBSP_Poly *In, GBSP_Poly **Out)
{
	GBSP_Poly	*NewPoly;
	int32		i;

	NewPoly = AllocPoly(In->NumVerts);
	if (!NewPoly)
	{
		return GE_FALSE;
	}

	for (i=0; i<In->NumVerts; i++)
		NewPoly->Verts[i] = In->Verts[In->NumVerts-i-1];
	
	*Out = NewPoly;

	return GE_TRUE;
}

//====================================================================================
//	PolyCenter
//====================================================================================
void PolyCenter(GBSP_Poly *Poly, geVec3d *Center)
{
	int32	i;

	geVec3d_Clear(Center);

	for (i=0; i< Poly->NumVerts; i++)
		geVec3d_Add(Center, &Poly->Verts[i], Center);

	geVec3d_Scale(Center, (geFloat)1.0/(geFloat)Poly->NumVerts, Center);
}

#define EDGE_LENGTH			(geFloat)0.1

//====================================================================================
//	PolyIsTiny
//====================================================================================
geBoolean PolyIsTiny (GBSP_Poly *Poly)
{
	int32	i, j;
	geFloat	Len;
	geVec3d	Delta;
	int32	Edges;

	Edges = 0;

	//return GE_FALSE;

	for (i=0 ; i<Poly->NumVerts ; i++)
	{
		j = i == Poly->NumVerts - 1 ? 0 : i+1;

		geVec3d_Subtract(&Poly->Verts[j], &Poly->Verts[i], &Delta);

		Len = geVec3d_Length(&Delta);

		if (Len > EDGE_LENGTH)
		{
			if (++Edges == 3)
				return GE_FALSE;
		}
	}

	//GHook.Printf("Poly is tiny...\n");

	return GE_TRUE;
}

//
//	Faces
//

//====================================================================================
//	AllocFace
//====================================================================================
GBSP_Face *AllocFace(int32 NumVerts)
{
	GBSP_Face *Face;

	Face = GE_RAM_ALLOCATE_STRUCT(GBSP_Face);

	if (!Face)
		return NULL;

	memset(Face, 0, sizeof(GBSP_Face));

	if (NumVerts)
	{
		Face->Poly = AllocPoly(NumVerts);
		if (!Face->Poly)
		{
			FreeFace(Face);
			return NULL;
		}
	}
	else
		Face->Poly = NULL;

	return Face;
}

//====================================================================================
//	FreeFace
//====================================================================================
void FreeFace(GBSP_Face *Face)
{
	if (!Face)
	{
		GHook.Printf("*WARNING* FreeFace: NULL Face.\n");
		return;
	}

	//if (!Face->Poly)
	//	Hook.Error("FreeFace: NULL Poly.\n");
	if (Face->Poly)
		FreePoly(Face->Poly);
	Face->Poly = NULL;

	if (Face->IndexVerts)
		geRam_Free(Face->IndexVerts);
	Face->IndexVerts = NULL;
	
	geRam_Free(Face);
}

//====================================================================================
//	void SplitFace
//====================================================================================
geBoolean SplitFace(GBSP_Face *In, GBSP_Plane *Split, GBSP_Face **Front, GBSP_Face **Back, geBoolean FlipTest)
{
	GBSP_Poly	*F, *B;
	GBSP_Face	*NewFace;

	if (!SplitPoly(In->Poly, Split, &F, &B, FlipTest))
		return GE_FALSE;

	if (!F && !B)
	{
		GHook.Error("SplitFace:  Poly was clipped away.\n");
		return GE_FALSE;
	}

	if (!F)
	{
		In->Poly = B;
		*Back = In;
		*Front = NULL;
		return GE_TRUE;
	}
	if (!B)
	{
		In->Poly = F;
		*Front = In;
		*Back = NULL;
		return GE_TRUE;
	}

	NewFace = GE_RAM_ALLOCATE_STRUCT(GBSP_Face);
	if (!NewFace)
	{
		GHook.Error("SplitFace:  Out of memory for new face.\n");
		return GE_FALSE;
	}

	*NewFace = *In;

	In->Poly = F;
	NewFace->Poly = B;

	*Front = In;
	*Back = NewFace;

	return GE_TRUE;
}

//====================================================================================
//	CopyFace
//====================================================================================
geBoolean CopyFace(GBSP_Face *In, GBSP_Face **Out)
{
	GBSP_Face	*NewFace;
	GBSP_Poly	*NewPoly;
	int32		i;

	NewFace = AllocFace(0);

	if (!NewFace)
	{
		GHook.Error("CopyFaceList:  Out of memory for new face.\n");
		return GE_FALSE;
	}

	*NewFace = *In;
	NewPoly = AllocPoly(In->Poly->NumVerts);

	if (!NewPoly)
	{
		GHook.Error("CopyFaceList:  Out of memory for newpoly.\n");
		return GE_FALSE;
	}
		
	NewFace->Poly = NewPoly;

	for (i=0; i< In->Poly->NumVerts; i++)
		NewFace->Poly->Verts[i] = In->Poly->Verts[i];

	return GE_TRUE;
}

//====================================================================================
//	MergeFaceList2
//====================================================================================
geBoolean MergeFaceList2(GBSP_Face *Faces)
{
	GBSP_Face	*Face1, *Face2, *End, *Merged;

	for (Face1 = Faces ; Face1 ; Face1 = Face1->Next)
	{
		if (Face1->Poly->NumVerts == -1)
			continue;

		if (Face1->Merged || Face1->Split[0] || Face1->Split[1])
			continue;

		for (Face2 = Faces ; Face2 != Face1 ; Face2 = Face2->Next)
		{
			if (Face2->Poly->NumVerts == -1)
				continue;

			if (Face2->Merged || Face2->Split[0] || Face2->Split[1])
				continue;
			
			Merged = MergeFace(Face1, Face2);

			if (!Merged)
				continue;

			RemoveDegenerateEdges(Merged->Poly);
			
			if (!CheckFace(Merged, GE_FALSE))
			{
				FreeFace(Merged);
				Face1->Merged = NULL;
				Face2->Merged = NULL;
				continue;
			}

			NumMerged++;

			// Add the Merged to the end of the face list 
			// so it will be checked against all the faces again
			for (End = Faces ; End->Next ; End = End->Next);
				
			Merged->Next = NULL;
			End->Next = Merged;
			break;
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	FreeFaceList
//====================================================================================
void FreeFaceList(GBSP_Face *List)
{			
	GBSP_Face	*Next;

	while(List)
	{
		Next = List->Next;
		FreeFace(List);
		List = Next;
	}
}

//====================================================================================
//	EdgeExist
//====================================================================================
int32 EdgeExist(geVec3d *Edge1, GBSP_Poly *Poly, int32 *EdgeIndexOut)
{
	int32		i;
	geVec3d		Edge2[2], *Verts;
	int32		NumVerts;

	Verts = Poly->Verts;
	NumVerts = Poly->NumVerts;

	for (i=0; i< NumVerts; i++)
	{
		Edge2[0] = Verts[i];
		Edge2[1] = Verts[(i+1)%NumVerts];

		if (geVec3d_Compare(Edge1, Edge2, VCOMPARE_EPSILON))
		if (geVec3d_Compare(Edge1+1, Edge2+1, VCOMPARE_EPSILON))
		{
			EdgeIndexOut[0] = i;
			EdgeIndexOut[1] = (i+1)%NumVerts;
			return GE_TRUE;
		}
	}

	return GE_FALSE;
}

#define COLINEAR_EPSILON		0.0001f

//====================================================================================
//	MergeFace
//====================================================================================
GBSP_Face *MergeFace(GBSP_Face *Face1, GBSP_Face *Face2)
{
	geVec3d		Edge1[2], *Verts1, *Verts2;
	int32		i, k, NumVerts, NumVerts2, EdgeIndex[2], NumNewVerts;
	GBSP_Poly	*NewPoly = NULL, *Poly1, *Poly2;
	geVec3d		Normal1, Normal2, Vec1, Vec2;
	GBSP_Face	*NewFace = NULL;
	geFloat		Dot;
	//int32		Start, End;
	geBoolean	Keep1 = GE_TRUE, Keep2 = GE_TRUE;

	//
	// Planes and Sides MUST match before even trying to merge
	//
	if (Face1->PlaneNum != Face2->PlaneNum)
		return NULL;
	if (Face1->PlaneSide != Face2->PlaneSide)
		return NULL;

#if 1
	if ((Face1->Contents[0]&BSP_MERGE_SEP_CONTENTS) != (Face2->Contents[0]&BSP_MERGE_SEP_CONTENTS))
		return NULL;
	if ((Face1->Contents[1]&BSP_MERGE_SEP_CONTENTS) != (Face2->Contents[1]&BSP_MERGE_SEP_CONTENTS))
		return NULL;
#endif

	if (Face1->TexInfo != Face2->TexInfo)
		return NULL;
	
	Poly1 = Face1->Poly;
	Poly2 = Face2->Poly;

	if (Poly1->NumVerts == -1 || Poly2->NumVerts == -1)
		return NULL;

	Verts1 = Poly1->Verts;
	NumVerts = Poly1->NumVerts;

	//
	// Go through each edge of Poly1, and see if the reverse of it exist in Poly2
	//
	for (i=0; i< NumVerts; i++)		
	{
		Edge1[1] = Verts1[i];
		Edge1[0] = Verts1[(i+1)%NumVerts];

		if (EdgeExist(Edge1, Poly2, EdgeIndex))	// Found one, break out
			break;
	}

	if (i >= NumVerts)							// Did'nt find an edge, return nothing
		return NULL;

	Verts2 = Poly2->Verts;
	NumVerts2 = Poly2->NumVerts;

	//
	//	See if the 2 joined make a convex poly, connect them, and return new one
	//
	Normal1 = Planes[Face1->PlaneNum].Normal;	// Get the normal
	if (Face1->PlaneSide)
		geVec3d_Subtract(&VecOrigin, &Normal1, &Normal1);

	Vec1 = Verts1[(i+NumVerts-1)%NumVerts];		// Get the normal of the edge just behind edge1
	geVec3d_Subtract(&Vec1, Edge1+1, &Vec1);
	geVec3d_CrossProduct(&Normal1, &Vec1, &Normal2);
	geVec3d_Normalize(&Normal2);
	geVec3d_Subtract(&Verts2[(EdgeIndex[1]+1)%NumVerts2], &Verts2[EdgeIndex[1]], &Vec2);
	Dot = geVec3d_DotProduct(&Vec2, &Normal2);
	if (Dot > COLINEAR_EPSILON)
		return NULL;							// Edge makes a non-convex poly
	if (Dot >=-COLINEAR_EPSILON)				// Drop point, on colinear edge
		Keep1 = GE_FALSE;

	Vec1 = Verts1[(i+2)%NumVerts];				// Get the normal of the edge just behind edge1
	geVec3d_Subtract(&Vec1, Edge1, &Vec1);
	geVec3d_CrossProduct(&Normal1, &Vec1, &Normal2);
	geVec3d_Normalize(&Normal2);
	geVec3d_Subtract(&Verts2[(EdgeIndex[0]+NumVerts2-1)%NumVerts2], &Verts2[EdgeIndex[0]], &Vec2);
	Dot = geVec3d_DotProduct(&Vec2, &Normal2);
	if (Dot > COLINEAR_EPSILON)
		return NULL;							// Edge makes a non-convex poly
	if (Dot >=-COLINEAR_EPSILON)				// Drop point, on colinear edge
		Keep2 = GE_FALSE;

	//if (NumVerts+NumVerts2 > 30)
	//	return NULL;
	
	NewFace = AllocFace(NumVerts+NumVerts2);
	NewPoly = NewFace->Poly;
	if (!NewFace)
	{
		GHook.Error("*WARNING* MergeFace:  Out of memory for new face!\n");
		return NULL;
	}

	//
	// Make a new poly, free the old ones...
	//
	NumNewVerts = 0;

	for (k = (i+1)%NumVerts; k != i ; k = (k+1)%NumVerts)
	{
		if (k == (i+1)%NumVerts && ! Keep2)
			continue;
		NewPoly->Verts[NumNewVerts++] = Verts1[k];
	}

	i = EdgeIndex[0];

	for (k = (i+1)%NumVerts2; k != i ; k = (k+1)%NumVerts2)
	{
		if (k == (i+1)%NumVerts2 && ! Keep1)
			continue;
		NewPoly->Verts[NumNewVerts++] = Verts2[k];
	}

	*NewFace = *Face2;

	NewPoly->NumVerts = NumNewVerts;
	NewFace->Poly = NewPoly;

	//Hook.Printf("Merged face: %i\n", NumNewVerts);

	Face1->Merged = NewFace;
	Face2->Merged = NewFace;

	return NewFace;
}

//====================================================================================
//	NewFaceFromFace
//====================================================================================
GBSP_Face *NewFaceFromFace (GBSP_Face *f)
{
	GBSP_Face	*Newf;

	Newf = AllocFace (0);
	*Newf = *f;
	//Newf->merged = NULL;
	Newf->Split[0] = Newf->Split[1] = NULL;
	Newf->Poly = NULL;
	
	return Newf;
}

//====================================================================================
//	FanFace_r
//====================================================================================
void FanFace_r(GBSP_Node *Node, GBSP_Face *Face)
{
	GBSP_Poly	*Poly, *NewPoly;
	geVec3d		*pVert;
	int32		i;

	Poly = Face->Poly;

	if (Poly->NumVerts == 3)		// Don't need to fan...
		return;			

	// Split this poly at the firsdt fan point into 2 more polys, and fan those...
	NewPoly = AllocPoly(3);

	NewPoly->Verts[0] = Poly->Verts[0];
	NewPoly->Verts[1] = Poly->Verts[1];
	NewPoly->Verts[2] = Poly->Verts[2];

	Face->Split[0] = NewFaceFromFace(Face);	
	Face->Split[0]->Poly = NewPoly;
	Face->Split[0]->Next = Node->Faces;
	Node->Faces = Face->Split[0];
		
	// Now make the rest of the polys
	NewPoly = AllocPoly(Poly->NumVerts-1);

	pVert = NewPoly->Verts;

	for (i=0; i<Poly->NumVerts; i++)
	{
		if (i==1)
			continue;

		*pVert++ = Poly->Verts[i];
	}
	
	Face->Split[1] = NewFaceFromFace(Face);	
	Face->Split[1]->Poly = NewPoly;
	Face->Split[1]->Next = Node->Faces;
	Node->Faces = Face->Split[1];
	
	FanFace_r(Node, Face->Split[1]);
}

//====================================================================================
//	GetLog
//====================================================================================
uint32 GetLog(uint32 P2)
{
	U32		p = 0;
	S32		i = 0;

	if (!P2)
	{
		GHook.Printf("*WARNING* GetLog:  Bad input.\n");
		return 0;
	}
	
	for (i = P2; i > 0; i>>=1)
		p++;

	return (p-1);
}


//====================================================================================
//	Snaps a number to a power of 2
//====================================================================================
int32 SnapToPower2(int32 Width)
{
	int RetWidth;

	for ( RetWidth = 1; RetWidth < Width; RetWidth <<= 1 ) ;
	
	//assert( RetWidth <= 256 );

	return RetWidth;
}

//====================================================================================
//	SubdivideFace
//====================================================================================
void SubdivideFace(GBSP_Node *Node, GBSP_Face *Face)
{
	geFloat		Mins, Maxs;
	geFloat		v;
	int32		Axis, i;
	GFX_TexInfo	*Tex;
	geVec3d		Temp;
	geFloat		Dist;
	GBSP_Poly	*p, *Frontp, *Backp;
	GBSP_Plane	Plane;

	if (Face->Merged)
		return;

	// Special (non-surface cached) faces don't need subdivision
	Tex = &TexInfo[Face->TexInfo];

	//if ( Tex->Flags & TEXINFO_GOURAUD)
	//	FanFace_r(Node, Face);

	if ( Tex->Flags & TEXINFO_NO_LIGHTMAP)
		return;

	for (Axis = 0 ; Axis < 2 ; Axis++)
	{
		while (1)
		{
			float		TexSize, SplitDist;
			float		Mins16, Maxs16;

			Mins = 999999.0f;
			Maxs = -999999.0f;
			
			geVec3d_Copy(&Tex->Vecs[Axis], &Temp);

			for (i=0 ; i<Face->Poly->NumVerts ; i++)
			{
				v = geVec3d_DotProduct (&Face->Poly->Verts[i], &Temp);
				if (v < Mins)
					Mins = v;
				if (v > Maxs)
					Maxs = v;
			}
			
			Mins16 = (float)floor(Mins/16.0f)*16.0f;
			Maxs16 = (float)ceil(Maxs/16.0f)*16.0f;

			TexSize = Maxs - Mins;
			//TexSize = Maxs16 - Mins16;

			// Default to the Subdivide size
			SplitDist = SubdivideSize;

		#if 0
			if (TexSize <= SubdivideSize)
			{
				int32		Log, LogSize;
				float		Ratio;

				LogSize = SnapToPower2((int32)TexSize);
				Log = GetLog(LogSize);

				if (LogSize <= 16)
					break;						// Don't bother with small lmaps

				Ratio = TexSize / (float)LogSize;	// Get the percentage of the space being taken advantage of if it were to be snapped to power of 2 texture in drivers

				if (Ratio >= 0.75f)
					break;			// No need to split, if going to use at least 50% of the space

				// The lmap is not going to use a big enough percentage of the size it's currently going to be snapped to
				// So, cut it along the next size down...
				Log--;				// Split along next log down

				// Overide the subdivide size with the next size down
				SplitDist = (float)(1<<Log);
			}
		#else
			if (TexSize <= SubdivideSize)
				break;
		#endif
			
			// Make a copy
			CopyPoly(Face->Poly, &p);

			// Split it
			NumSubdivided++;
			
			v = geVec3d_Normalize (&Temp);	

			Dist = (Mins + SplitDist - 16)/v;
			//Dist = (Mins16 + SplitDist)/v;

			Plane.Normal = Temp;
			Plane.Dist = Dist;
			Plane.Type = PLANE_ANY;
			
			SplitPoly(p, &Plane, &Frontp, &Backp, GE_FALSE);

			if (!Frontp || !Backp)
			{
				GHook.Printf("*WARNING* SubdivideFace: didn't split the polygon: %f, %f, %f\n", Mins, Maxs, SplitDist);
				break;
			}

			Face->Split[0] = NewFaceFromFace (Face);
			Face->Split[0]->Poly = Frontp;
			Face->Split[0]->Next = Node->Faces;
			Node->Faces = Face->Split[0];

			Face->Split[1] = NewFaceFromFace (Face);
			Face->Split[1]->Poly = Backp;
			Face->Split[1]->Next = Node->Faces;
			Node->Faces = Face->Split[1];

			SubdivideFace (Node, Face->Split[0]);
			SubdivideFace (Node, Face->Split[1]);
			return;
		}
	}
}

//====================================================================================
//	SubdivideNodeFaces
//====================================================================================
void SubdivideNodeFaces (GBSP_Node *Node)
{
	GBSP_Face	*f;

	for (f = Node->Faces ; f ; f=f->Next)
	{
		SubdivideFace (Node, f);
	}
}

//====================================================================================
//	CheckFace
//====================================================================================
geBoolean CheckFace(GBSP_Face *Face, geBoolean Verb)
{
	int32		i, j, NumVerts;
	geVec3d		Vect1, *Verts, Normal, V1, V2, EdgeNormal;
	GBSP_Poly	*Poly;
	geFloat		Dist, PDist, EdgeDist;
	
	Poly = Face->Poly;
	Verts = Poly->Verts;
	NumVerts = Poly->NumVerts;

	if (NumVerts < 3)
	{
		if (Verb)
			GHook.Printf("CheckFace:  NumVerts < 3.\n");
		return GE_FALSE;
	}
	
	Normal = Planes[Face->PlaneNum].Normal;
	PDist = Planes[Face->PlaneNum].Dist;
	if (Face->PlaneSide)
	{
		geVec3d_Subtract(&VecOrigin, &Normal, &Normal);
		PDist = -PDist;
	}

	//
	//	Check for degenerate edges, convexity, and make sure it's planar
	//
	for (i=0; i< NumVerts; i++)
	{
		V1 = Verts[i];
		V2 = Verts[(i+1)%NumVerts];

		//	Check for degenreate edge
		geVec3d_Subtract(&V2, &V1, &Vect1);
		Dist = geVec3d_Length(&Vect1);
		if (fabs(Dist) < DEGENERATE_EPSILON)
		{
			if (Verb)
				GHook.Printf("WARNING CheckFace:  Degenerate Edge.\n");
			return GE_FALSE;
		}

		// Check for planar
		Dist = geVec3d_DotProduct(&V1, &Normal) - PDist;
		if (Dist > ON_EPSILON || Dist <-ON_EPSILON)
		{
			if (Verb)
				GHook.Printf("WARNING CheckFace:  Non planar: %f.\n", Dist);
			return GE_FALSE;
		}

		geVec3d_CrossProduct(&Normal, &Vect1, &EdgeNormal);
		geVec3d_Normalize(&EdgeNormal);
		EdgeDist = geVec3d_DotProduct(&V1, &EdgeNormal);
		
		// Check for convexity
		for (j=0; j< NumVerts; j++)
		{
			Dist = geVec3d_DotProduct(&Verts[j], &EdgeNormal) - EdgeDist;
			if (Dist > ON_EPSILON)
			{
				if (Verb)
					GHook.Printf("CheckFace:  Face not convex.\n");
				return GE_FALSE;
			}
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	GetFaceListBOX
//====================================================================================
void GetFaceListBOX(GBSP_Face *Faces, geVec3d *Mins, geVec3d *Maxs)
{
	GBSP_Face	*Face;
	GBSP_Poly	*Poly;
	geVec3d		Vert;
	int32		i;

	ClearBounds(Mins, Maxs);

	// Go through each face in this brush...
	for (Face = Faces; Face; Face = Face->Next)
	{
		Poly = Face->Poly;
		for (i=0; i< Poly->NumVerts; i++)
		{
			Vert = Poly->Verts[i];

			// First get maxs
			if (Vert.X > Maxs->X)
				Maxs->X = Vert.X;
			if (Vert.Y > Maxs->Y)
				Maxs->Y = Vert.Y;
			if (Vert.Z > Maxs->Z)
				Maxs->Z = Vert.Z;

			// Then check mins
			if (Vert.X < Mins->X)
				Mins->X = Vert.X;
			if (Vert.Y < Mins->Y)
				Mins->Y = Vert.Y;
			if (Vert.Z < Mins->Z)
				Mins->Z = Vert.Z;
		}
	}
}

