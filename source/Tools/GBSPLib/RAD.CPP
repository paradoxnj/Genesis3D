/****************************************************************************************/
/*  Rad.cpp                                                                             */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Cacluates radiosity for a BSP.                                         */
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

#include "DCommon.h"
#include "BSP.h"
#include "GBSPFile.h"
#include "Math.h"
#include "MathLib.h"
#include "Poly.h"
#include "Light.h"
#include "Texture.h"

#include "Ram.h"

pRAD_Patch	*FacePatches;
pRAD_Patch	*PatchList;
geFloat		*RecAmount;

int32		NumPatches;
int32		NumReceivers;

geBoolean BuildPatch(int32 Face);
geBoolean FinalizePatches(void);
RAD_Patch *AllocPatch(void);
void FreePatch(RAD_Patch *Patch);
void CalcPatchReflectivity(int32 Face, RAD_Patch *Patch);
void GetFaceMinsMaxs(int32 Face, geVec3d *Mins, geVec3d *Maxs);

geBoolean SaveReceiverFile(char *FileName);
geBoolean LoadReceiverFile(char *FileName);

//====================================================================================
//	BuildPatches
//====================================================================================
geBoolean BuildPatches(void)
{
	int32		i;

	GHook.Printf("--- Build Patches --- \n");

	NumPatches = NumGFXFaces;

	FacePatches = GE_RAM_ALLOCATE_ARRAY(pRAD_Patch,NumGFXFaces);

	if (!FacePatches)
	{
		GHook.Error("BuilkdPatches:  Not enough memory for patches.\n");
		return GE_FALSE;
	}

	for (i=0; i< NumGFXFaces; i++)
	{
		FacePatches[i] = NULL;

		//if (GFXTexInfo[GFXFaces[i].TexInfo].Flags & TEXINFO_NO_LIGHTMAP)
		//	continue;

		if (!BuildPatch(i))
			return GE_FALSE;
	}

	NumPatches = 0;
	if (!FinalizePatches())
	{
		GHook.Error("BuildPatches:  Could not finalize face patches.\n");
		return GE_FALSE;
	}

	if (LVerbose)
		GHook.Printf("Num Patches          : %5i\n", NumPatches);

	return GE_TRUE;
}

//====================================================================================
//	CalcPatchInfo
//====================================================================================
geBoolean CalcPatchInfo(RAD_Patch *Patch)
{
	int32		i, k;
	GBSP_Poly	*Poly;

	ClearBounds(&Patch->Mins, &Patch->Maxs);

	Poly = Patch->Poly;

	if (!Poly)
	{
		GHook.Error("CalcPatchInfo:  No Poly!\n");
		return GE_FALSE;
	}

	for (i=0; i< Poly->NumVerts; i++)
	{
		for (k=0; k< 3; k++)
		{
			if (VectorToSUB(Poly->Verts[i], k) < VectorToSUB(Patch->Mins, k))
				VectorToSUB(Patch->Mins, k) = VectorToSUB(Poly->Verts[i], k);
			if (VectorToSUB(Poly->Verts[i], k) > VectorToSUB(Patch->Maxs, k))
				VectorToSUB(Patch->Maxs, k) = VectorToSUB(Poly->Verts[i], k);
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	GBSPPolyFromGFXFace
//====================================================================================
GBSP_Poly *GBSPPolyFromGFXFace(GFX_Face *Face)
{
	GBSP_Poly	*NewPoly;
	int32		i, Index;

	NewPoly = AllocPoly(Face->NumVerts);

	for (i=0; i< Face->NumVerts; i++)
	{
		Index = GFXVertIndexList[i + Face->FirstVert];

		NewPoly->Verts[i].X = (geFloat)GFXVerts[Index].X;
		NewPoly->Verts[i].Y = (geFloat)GFXVerts[Index].Y;
		NewPoly->Verts[i].Z = (geFloat)GFXVerts[Index].Z;
	}

	RemoveDegenerateEdges(NewPoly);
	
	return NewPoly;
}

//====================================================================================
//	PatchNeedsSplit
//====================================================================================
geBoolean PatchNeedsSplit(RAD_Patch *Patch, GBSP_Plane *Plane)
{
	int32		i;

	
	if (NumPatches >= MAX_PATCHES-2)		// Do not cut any more if max patches exceeded!!!
		return GE_FALSE;

if (FastPatch)
{
	geFloat	Dist;

	for (i=0; i<3; i++)
	{
		Dist = VectorToSUB(Patch->Maxs, i) - VectorToSUB(Patch->Mins, i);

		if (Dist > PatchSize)
		{
			// Cut it right through the center...
			geVec3d_Clear(&Plane->Normal);
			VectorToSUB(Plane->Normal, i) = (geFloat)1;
			Plane->Dist = (VectorToSUB(Patch->Maxs, i) + VectorToSUB(Patch->Mins, i))/2.0f;
			Plane->Type = PLANE_ANY;
			return GE_TRUE;
		}
	}
}
else
{
	geFloat Min, Max;

	for (i=0 ; i<3 ; i++)
	{
		Min = VectorToSUB(Patch->Mins,i)+1.0f;
		Max = VectorToSUB(Patch->Maxs,i)-1.0f;
		if (floor(Min/PatchSize) < floor(Max/PatchSize))
		{
			geVec3d_Clear(&Plane->Normal);
			VectorToSUB(Plane->Normal, i) = (geFloat)1;
			Plane->Dist = PatchSize*(1.0f+(float)floor(Min/PatchSize));
			Plane->Type = PLANE_ANY;
			return GE_TRUE;		
		}
	}
}
	return GE_FALSE;
}

//====================================================================================
//	SubdivideFacePatches
//====================================================================================
RAD_Patch *SubdivideFacePatches(RAD_Patch *Patch)
{
	RAD_Patch	*CPatch, *NewPatch, *NextPatch;
	GBSP_Poly	*Poly, *FPoly, *BPoly;
	GBSP_Plane	Plane;

	for (CPatch = Patch; CPatch; CPatch = NextPatch)
	{
		NextPatch = CPatch->Next;

		if (PatchNeedsSplit(CPatch, &Plane))
		{
			NumPatches++;

			Poly = CPatch->Poly;
			if (!SplitPoly(Poly, &Plane, &FPoly, &BPoly, GE_FALSE))
				return GE_FALSE;
			
			if (!FPoly || !BPoly)
			{
				GHook.Error("SubdivideFacePatches:  Patch was not split.\n");
				return NULL;
			}
			
			NewPatch = AllocPatch();
			if (!NewPatch)
			{
				GHook.Error("SubdivideFacePatches:  Out of memory for new patch.\n");
				return NULL;
			}

			*NewPatch = *CPatch;			// Make it take on all the attributes of it's parent

			NewPatch->Next = NextPatch;
			NewPatch->Poly = FPoly;
			if (!CalcPatchInfo(NewPatch))
			{
				GHook.Error("SubdivideFacePatches:  Could not calculate patch info.\n");
				return NULL;
			}

			// Re-use the first patch
			CPatch->Next = NewPatch;
			CPatch->Poly = BPoly;

			if (!CalcPatchInfo(CPatch))
			{
				GHook.Error("SubdivideFacePatches:  Could not calculate patch info.\n");
				return NULL;
			}

			NextPatch = CPatch;	// Keep working from here till satisfied...
		}
	}

	return Patch;
}

//====================================================================================
//	FinalizePatchInfo
//====================================================================================
geBoolean FinalizePatchInfo(int32 Face, RAD_Patch *Patch)
{
	GBSP_Poly	*Poly;
	int32		i;

	Poly = Patch->Poly;

	if (!Poly)
	{
		GHook.Error("FinalizePatchInfo:  No Poly!\n");
		return GE_FALSE;
	}

	geVec3d_Clear(&Patch->Origin);

	for (i=0; i< Poly->NumVerts; i++)
	{
		geVec3d_Add(&Patch->Origin, &Poly->Verts[i], &Patch->Origin);
	}

	for (i=0; i<3; i++)
		VectorToSUB(Patch->Origin, i) /= Poly->NumVerts;

	Patch->Plane.Normal.X = (geFloat)GFXPlanes[GFXFaces[Face].PlaneNum].Normal.X;
	Patch->Plane.Normal.Y = (geFloat)GFXPlanes[GFXFaces[Face].PlaneNum].Normal.Y;
	Patch->Plane.Normal.Z = (geFloat)GFXPlanes[GFXFaces[Face].PlaneNum].Normal.Z;

	Patch->Plane.Dist = (geFloat)GFXPlanes[GFXFaces[Face].PlaneNum].Dist;
	Patch->Plane.Type = PLANE_ANY;

	if (GFXFaces[Face].PlaneSide)
	{
		geVec3d_Inverse(&Patch->Plane.Normal);
		Patch->Plane.Dist = -Patch->Plane.Dist;
	}

	geVec3d_AddScaled(&Patch->Origin, &Patch->Plane.Normal, 2.0f, &Patch->Origin);

	Patch->Leaf = FindGFXLeaf(0, &Patch->Origin);

	Patch->Area = PolyArea(Patch->Poly);
	if (Patch->Area < 1.0f)
		Patch->Area = 1.0f;

	FreePoly(Patch->Poly);		// Don't need this anymore
	Patch->Poly = NULL;

	return GE_TRUE;
}

//====================================================================================
//	FinalizePatches
//====================================================================================
geBoolean FinalizePatches(void)
{
	RAD_Patch	*Patch;
	int32		i, k;

	for (i=0; i< NumGFXFaces; i++)
	for (Patch = FacePatches[i]; Patch; Patch = Patch->Next)
	{
		FinalizePatchInfo(i, Patch);
		NumPatches++;
	}

	PatchList = GE_RAM_ALLOCATE_ARRAY(pRAD_Patch,NumPatches);

	if (!PatchList)
	{
		GHook.Error("FinalizePatches:  Out of memory for patch list.\n");
		return GE_FALSE;
	}
	
	// Build the patch list, so we can use indexing, instead of pointers (for receivers)...
	k = 0;
	for (i=0; i< NumGFXFaces; i++)
	for (Patch = FacePatches[i]; Patch; Patch = Patch->Next)
	{
		PatchList[k] = Patch;
		k++;
	}

	return GE_TRUE;
}

//====================================================================================
//	FreePatches
//====================================================================================
void FreePatches(void)
{
	int32		i;

	for (i=0; i< NumPatches; i++)
	{
		geRam_Free(PatchList[i]);
	}

	NumPatches = 0;

	if (PatchList)
		geRam_Free(PatchList);
	if (FacePatches)	
		geRam_Free(FacePatches);

	PatchList = NULL;
	FacePatches = NULL;
}


geFloat BestPatchSize(int32 Face)
{
	GBSP_Poly	*Poly;
	int32		i;
	int32		Axis;
	geFloat		Mins, Maxs, Size, BestSize, v;
	geVec3d		*Verts;
	GFX_TexInfo	*Tex;

	Poly = FacePatches[Face]->Poly;

	// Special (nonsurface cached) faces don't need subdivision
	Tex = &TexInfo[GFXFaces[Face].TexInfo];

	BestSize = -MIN_MAX_BOUNDS2;

	for (Axis = 0 ; Axis < 2 ; Axis++)
	{
		Mins =  MIN_MAX_BOUNDS;
		Maxs = -MIN_MAX_BOUNDS;
			
		Verts = Poly->Verts;
		for (i=0 ; i< Poly->NumVerts ; i++)
		{
			v = geVec3d_DotProduct(&Verts[i], &Tex->Vecs[Axis]);
			if (v < Mins)
				Mins = v;
			if (v > Maxs)
				Maxs = v;
		}
		
		Size = Maxs - Mins;
		if (Size > BestSize)
			BestSize = Size;
	}

	return (BestSize/4);

}
//====================================================================================
//	BuildPatch
//====================================================================================
void ApplyLightmapToPatches(int32 Face);

geBoolean BuildPatch(int32 Face)
{

	//int32		Texture;
	//Texture = GFXTexInfo[GFXFaces[Face].TexInfo].Texture;

	FacePatches[Face] = AllocPatch();
	if (!FacePatches[Face])
	{
		GHook.Error("BuildPatch:  Could not allocate patch.\n");
		return GE_FALSE;
	}

	CalcPatchReflectivity(Face, FacePatches[Face]);
	
	FacePatches[Face]->Poly = GBSPPolyFromGFXFace(&GFXFaces[Face]);

	if (!CalcPatchInfo(FacePatches[Face]))
	{
		GHook.Error("BuildPatch:  Could not calculate patch info.\n");
		return GE_FALSE;
	}

	FacePatches[Face] = SubdivideFacePatches(FacePatches[Face]);

	if (!FacePatches[Face])
	{
		GHook.Error("BuildPatch:  Could not subdivide patch.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//====================================================================================
//	AllocPatch
//====================================================================================
RAD_Patch *AllocPatch(void)
{
	RAD_Patch *Patch;

	Patch = GE_RAM_ALLOCATE_STRUCT(RAD_Patch);

	if (!Patch)
	{
		GHook.Error("AllocPatch:  Not enough memory.\n");
		return NULL;
	}

	memset(Patch, 0, sizeof(RAD_Patch));

	return Patch;
}

//====================================================================================
//	FreePatch
//====================================================================================
void FreePatch(RAD_Patch *Patch)
{
	if (!Patch)
	{
		GHook.Printf("*WARNING* FreePatch:  Nothing to free!\n");
		return;
	}

	if (Patch->Poly)
		FreePoly(Patch->Poly);

	Patch->Poly = NULL;

	geRam_Free(Patch);
}

//====================================================================================
//	AllocReceiver
//====================================================================================
RAD_Receiver *AllocReceiver(void)
{
	RAD_Receiver *Receiver;

	Receiver = GE_RAM_ALLOCATE_STRUCT(RAD_Receiver);

	if (!Receiver)
	{
		GHook.Error("AllocReceiver:  Not enough memory.\f");
		return NULL;
	}

	memset(Receiver, 0, sizeof(RAD_Receiver));

	return Receiver;
}

//====================================================================================
//	FreeReceiver
//====================================================================================
void FreeReceiver(RAD_Receiver *Rec)
{
	if (!Rec)
	{
		GHook.Printf("*WARNING* FreeReceiver:  Nothing to free!\n");
		return;
	}
	geRam_Free(Rec);
}

//====================================================================================
//	FindPatchReceivers
//	PreCalculate who can see who, and how much they emit
//====================================================================================
geBoolean FindPatchReceivers(RAD_Patch *Patch)
{
	RAD_Patch		*Patch2;
	uint8			*VisData;
	geBoolean			VisInfo;
	geFloat			Dist;
	geFloat			Amount;
	geFloat			Total, Scale;
	int32			i, Cluster;
	geVec3d			Vect, Normal;
	RAD_Receiver	*Receiver;
	GFX_Leaf		*pLeaf;
	int32			Area;

	pLeaf = &GFXLeafs[Patch->Leaf];
	Cluster = pLeaf->Cluster;
	Area = pLeaf->Area;

	if (Cluster >= 0 && GFXClusters[Cluster].VisOfs >= 0)
	{
		VisData = &GFXVisData[GFXClusters[Cluster].VisOfs];
		VisInfo = GE_TRUE;
	}
	else
		VisInfo = GE_FALSE;

	Total = 0.0f;

	Normal = Patch->Plane.Normal;

	// For each face, go through all it's patches
	for (i=0; i< NumPatches; i++)
	{
		if (CancelRequest)
		{
			GHook.Printf("Cancel requested...\n");
			return GE_FALSE;
		}

		Patch2 = PatchList[i];
		
		RecAmount[i] = 0.0f;

		if (Patch2 == Patch)
			continue;

		pLeaf = &GFXLeafs[Patch2->Leaf];

		if (pLeaf->Area != Area)			// Radiosity only bounces in it's original area
			continue;

		if (VisInfo)
		{
			Cluster = pLeaf->Cluster;
			if (Cluster >= 0 && !(VisData[Cluster>>3] &(1<<(Cluster&7))) )
				continue;

		}

		geVec3d_Subtract(&Patch2->Origin, &Patch->Origin, &Vect);
	
		Dist = geVec3d_Normalize(&Vect);

		//if (Dist > PatchSize)
		if (!Dist)
			continue;	// Error
		
		Scale =   geVec3d_DotProduct(&Vect, &Normal);
		Scale *= -geVec3d_DotProduct(&Vect, &Patch2->Plane.Normal);

		if (Scale <= 0)
			continue;

		if (RayCollision(&Patch->Origin, &Patch2->Origin, NULL))
			continue;		// Blocked by somthing in the world

		Amount = Scale * Patch2->Area / (Dist*Dist);

		if (Amount <= 0.0f)
			continue;

		RecAmount[i] = Amount;

		// Add the receiver
		Total += Amount;
		NumReceivers++;
		Patch->NumReceivers++;
	}

	Patch->Receivers = GE_RAM_ALLOCATE_ARRAY(RAD_Receiver,Patch->NumReceivers);

	if (!Patch->Receivers)
	{
		GHook.Error("CalcReceivers:  Out of memory for receiver.\n");
		return GE_FALSE;
	}

	Receiver = Patch->Receivers;

	for (i=0; i< NumPatches; i++)
	{
		if (!RecAmount[i])
			continue;

		Receiver->Patch = (uint16)i;
		Receiver->Amount = (uint16)(RecAmount[i]*0x10000 / Total);
		Receiver++;
	}

	return GE_TRUE;
}

//====================================================================================
//	CalcReceivers
//====================================================================================
geBoolean CalcReceivers(char *FileName)
{
	int32		i;
	RAD_Patch	*Patch;
	int32		Perc;
	geFloat		Megs;

	NumReceivers = 0;

	// Try to load the receiver file first!!!
	if (LoadReceiverFile(FileName))
	{
		GHook.Printf("--- Found receiver file ---\n");
		return GE_TRUE;
	}

	GHook.Printf(" --- Calculating receivers from scratch ---\n");

	RecAmount = GE_RAM_ALLOCATE_ARRAY(geFloat,NumPatches);

	if (!RecAmount)
	{
		GHook.Error("CalcReceivers:  Out of memory for RecAmount.\n");
		return GE_FALSE;
	}

	Perc = (NumPatches/20);
	for (i=0; i< NumPatches; i++)
	{
		if (Perc)
		{
			if (!(i%Perc) && (i/Perc)<=20)
				GHook.Printf(".%i", (i/Perc));
		}
		
		Patch = PatchList[i];

		if (!FindPatchReceivers(Patch))
		{
			GHook.Error("CalcReceivers:  There was an error calculating receivers.\n");
			return GE_FALSE;
		}
	}
	GHook.Printf("\n");


	geRam_Free(RecAmount);
	RecAmount = NULL;

	Megs = (float)NumReceivers * sizeof(RAD_Receiver) / (1024*1024);
	GHook.Printf("Num Receivers        : %5i, Megs %2.2f\n", NumReceivers, Megs);

	// Save receiver file for later retreival
	if (!SaveReceiverFile(FileName))
	{
		GHook.Error("CalcReceivers:  Failed to save receiver file...\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//====================================================================================
//	FreeReceivers
//====================================================================================
void FreeReceivers(void)
{
	int32			i;
	RAD_Patch		*Patch;

	NumReceivers = 0;

	for (i=0; i< NumPatches; i++)
	{
		Patch = PatchList[i];

		if (Patch->NumReceivers)
			geRam_Free(Patch->Receivers);

		Patch->Receivers = NULL;
	}
}

//====================================================================================
//	CheckPatch
//====================================================================================
geBoolean CheckPatch(RAD_Patch *Patch)
{
	int32	i;

	for (i=0; i<3; i++)
	{
		if (VectorToSUB(Patch->RadFinal, i) < 0.0f)
		{
			GHook.Error("CheckPatch:  Bad final radiosity Color in patch.\n");
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}

//====================================================================================
//	CollectPatchLight
//====================================================================================
geFloat CollectPatchLight(void)
{
	int			i, j;
	RAD_Patch	*Patch;
	geFloat		Total;

	Total = 0.0f;
	
	for (i=0; i< NumPatches; i++)
	{
		Patch = PatchList[i];
		
		for (j=0 ; j<3 ; j++)
		{
			// Add receive amount to Final amount
			VectorToSUB(Patch->RadFinal, j) +=
				VectorToSUB(Patch->RadReceive, j) / Patch->Area;

			VectorToSUB(Patch->RadSend, j) = 
				VectorToSUB(Patch->RadReceive, j) * VectorToSUB(Patch->Reflectivity, j);
			
			Total += VectorToSUB(Patch->RadSend, j);
		}

		geVec3d_Clear(&Patch->RadReceive);
	}

	return Total;
}


//====================================================================================
//	SendPatch
//====================================================================================
void SendPatch(RAD_Patch *Patch)
{
	geVec3d			Send;
	RAD_Patch		*RPatch;
	int32			k;
	RAD_Receiver	*Receiver;

	for (k=0; k<3; k++)
		VectorToSUB(Send, k) = VectorToSUB(Patch->RadSend, k) / (float)0x10000;
		//VectorToSUB(Send, k) = VectorToSUB(Patch->RadSend, k);

	// Send light out to each pre-computed receiver
	Receiver = Patch->Receivers;
	for (k=0; k< Patch->NumReceivers; k++, Receiver++)
	{
		RPatch = PatchList[Receiver->Patch];

		geVec3d_AddScaled(&RPatch->RadReceive, &Send, (float)Receiver->Amount, &RPatch->RadReceive);
		//geVec3d_AddScaled(&RPatch->RadReceive, &Send, .0005f, &RPatch->RadReceive);
	}
}

//====================================================================================
//	BouncePatches
//====================================================================================
geBoolean BouncePatches(void)
{
	int32		i, j;
	RAD_Patch	*Patch;
	geFloat		Total;

	GHook.Printf("--- Bounce Patches --- \n");
	
	for (i=0 ; i< NumPatches; i++)
	{
		// Set each patches first pass send amount with what was obtained
		// from their lightmaps... 
		Patch = PatchList[i];
		for (j=0 ; j<3 ; j++)
		{
			VectorToSUB(Patch->RadSend, j) = VectorToSUB(Patch->RadStart, j) *
				VectorToSUB(Patch->Reflectivity, j) * Patch->Area;
		}

		//geVec3d_Clear(&Patch->RadFinal);
	}

	for (i=0 ; i<NumBounce ; i++)
	{
		if (LVerbose)
			GHook.Printf("Bounce: %3i, ", i+1);
		
		if (CancelRequest)
		{
			GHook.Printf("Cancel requested...\n");
			return GE_FALSE;
		}

		// For each patch, send it's energy to each pre-computed receiver
		for (j=0 ; j< NumPatches; j++)
		{
			Patch = PatchList[j];
			SendPatch(Patch);
		}

		// For each patch, collect any light it might have received
		// and throw into patch RadFinal
		Total = CollectPatchLight();

		if (LVerbose)
			GHook.Printf("Energy: %2.2f\n", Total);
	}
	
	for (j=0 ; j< NumPatches; j++)
	{
		Patch = PatchList[j];
		if (!CheckPatch(Patch))
			return GE_FALSE;
	}

	return GE_TRUE;
}

typedef struct _PlaneFace
{
	_PlaneFace	*Next;
	int32		GFXFace;
} PlaneFace, *pPlaneFace;

pPlaneFace *PlaneFaces;

//====================================================================================
//	LinkPlaneFaces
//====================================================================================
void LinkPlaneFaces(void)
{
	PlaneFace	*PFace;
	int32		i, PlaneNum;

	PlaneFaces = GE_RAM_ALLOCATE_ARRAY(pPlaneFace,NumGFXPlanes);

	for (i=0; i< NumGFXPlanes; i++)
		PlaneFaces[i] = NULL;
		
	for (i=0; i< NumGFXFaces; i++)
	{
		PFace = GE_RAM_ALLOCATE_STRUCT(PlaneFace);
		memset(PFace, 0, sizeof(PlaneFace));

		PlaneNum = GFXFaces[i].PlaneNum;
		PFace->GFXFace = i;
		PFace->Next = PlaneFaces[PlaneNum];
		PlaneFaces[PlaneNum] = PFace;
	}
}

//====================================================================================
//	FreePlaneFaces
//====================================================================================
void FreePlaneFaces(void)
{
	int32		i;
	PlaneFace	*PFace, *Next;

	for (i=0; i< NumGFXPlanes; i++)
	{
		for (PFace = PlaneFaces[i]; PFace; PFace = Next)
		{
			Next = PFace->Next;
			geRam_Free(PFace);
		}
	}

	geRam_Free(PlaneFaces);
}

//====================================================================================
//	GetFaceMinsMaxs
//====================================================================================
void GetFaceMinsMaxs(int32 Face, geVec3d *Mins, geVec3d *Maxs)
{
	int32	i, k, Index;
	geFloat	t;

	ClearBounds(Mins, Maxs);

	for (i=0; i< GFXFaces[Face].NumVerts; i++)
	{
		Index = GFXVertIndexList[GFXFaces[Face].FirstVert+i];
		for (k=0; k<3; k++)
		{
			t = VectorToSUB(GFXVerts[Index], k);

			if (t < VectorToSUB(*Mins, k))
				VectorToSUB(*Mins, k) = t;
			if (t > VectorToSUB(*Maxs, k))
				VectorToSUB(*Maxs, k) = t;
		}
	}
}

//====================================================================================
// MakeCornerPatches
//====================================================================================
RAD_Patch *MakeCornerPatches(int32 Face, Tri_Patch *Tri)
{
	RAD_Patch	*PatchList, *NewPatch;
	int32		i, Width, Height, u, v;
	geFloat		StartU, StartV, CurU, CurV, EndU, EndV;
	FInfo		*Fi;
	LInfo		*Li;

	PatchList = NULL;

	Fi = &FaceInfo[Face];
	Li = &Lightmaps[Face];

	Width = Li->LSize[0]+1;
	Height = Li->LSize[1]+1;
	StartU = ((geFloat)Li->LMins[0]) * (geFloat)LGRID_SIZE;
	StartV = ((geFloat)Li->LMins[1]) * (geFloat)LGRID_SIZE;

	EndU = ((geFloat)Li->LMaxs[0]) * (geFloat)LGRID_SIZE;
	EndV = ((geFloat)Li->LMaxs[1]) * (geFloat)LGRID_SIZE;

	for (v=0; v < 2; v++)
	{
		for (u=0; u < 2; u++)
		{
			if (u == 0)
				CurU = StartU;
			else
				CurU = EndU;
			
			if (v == 0)
				CurV = StartV;
			else
				CurV = EndV;

			NewPatch = AllocPatch();
			if (!NewPatch)
			{
				GHook.Error("CreateCornerPatches:  Could not create patch.\n");
				return NULL;
			}

			for (i=0; i< 3; i++)
				VectorToSUB(NewPatch->Origin, i) = VectorToSUB(Fi->TexOrg,i) + 
					VectorToSUB(Fi->T2WVecs[0], i) * CurU + 
					VectorToSUB(Fi->T2WVecs[1], i) * CurV;
			
			if (!FindClosestTriPoint(&NewPatch->Origin, Tri, &NewPatch->RadFinal))
			{
				GHook.Error("MakeCornerPatches:  Could not find patch for Color.\n");
				return GE_FALSE;
			}
			
			//NewPatch->RadFinal.X = 0.0f;
			//NewPatch->RadFinal.Y = 255.0f;
			//NewPatch->RadFinal.Z = 0.0f;
			// Insert it into the beginning of the list
			NewPatch->Next = PatchList;
			PatchList = NewPatch;
		}
	}
	return PatchList;
}

//====================================================================================
// FreePatchList
//====================================================================================
void FreePatchList(RAD_Patch *Patches)
{
	RAD_Patch	*Patch, *Next;

	for (Patch = Patches; Patch; Patch = Next)
	{
		Next = Patch->Next;
		FreePatch(Patch);
	}
}

//====================================================================================
//	AbsorbPatches
//====================================================================================
geBoolean AbsorbPatches(void)
{
	Tri_Patch		*Tri;
	GBSP_Plane		Plane;
	geVec3d			Add, *pPoint, *pRGB;
	int32			i, k, PNum, FNum, PSide;
	RAD_Patch		*Patch, *OPatch, *TempPatches;
	PlaneFace		*PFace;
	geVec3d			FMins, FMaxs;

	LinkPlaneFaces();	// We need all the faces that belong to each Plane

	for (i=0; i< NumGFXFaces; i++)
	{
		int32		Flags;
		GFX_Face	*pGFXFace;

		pGFXFace = &GFXFaces[i];

		if (CancelRequest)
		{
			GHook.Printf("Cancel requested...\n");
			return GE_FALSE;
		}

		pPoint = FaceInfo[i].Points;
		pRGB = Lightmaps[i].RGBLData[0];

		Flags = GFXTexInfo[GFXFaces[i].TexInfo].Flags;

		if ((Flags & TEXINFO_NO_LIGHTMAP) && !(Flags & TEXINFO_GOURAUD))
			continue;

		//if (!pRGB)
		//	continue;

		Plane.Normal.X = (geFloat)GFXPlanes[GFXFaces[i].PlaneNum].Normal.X;
		Plane.Normal.Y = (geFloat)GFXPlanes[GFXFaces[i].PlaneNum].Normal.Y;
		Plane.Normal.Z = (geFloat)GFXPlanes[GFXFaces[i].PlaneNum].Normal.Z;
		Plane.Dist = (geFloat)GFXPlanes[GFXFaces[i].PlaneNum].Dist;
		Plane.Type = PLANE_ANY;
		/*
		if(GFXFaces[i].PlaneSide)
		{
			geVec3d_Inverse(&Plane.Normal);
			Plane.Dist = -Plane.Dist;
		}
		*/

		Tri = Tri_PatchCreate(&Plane);
		if (!Tri)
		{
			GHook.Error("AbsorbPatches:  Tri_PatchCreate failed.\n");
			return GE_FALSE;
		}
		
		PNum = GFXFaces[i].PlaneNum;
		PSide = GFXFaces[i].PlaneSide;
		
		OPatch = FacePatches[i];

		GetFaceMinsMaxs(i, &FMins, &FMaxs);
		
		for (PFace = PlaneFaces[PNum]; PFace; PFace = PFace->Next)
		{
			FNum = PFace->GFXFace;

			if (GFXFaces[FNum].PlaneSide != PSide)
				continue;

			for (Patch = FacePatches[FNum]; Patch; Patch = Patch->Next)
			{
				for (k=0 ; k < 3 ; k++)
				{
					if (VectorToSUB(Patch->Origin, k) < VectorToSUB(FMins,k) - (PatchSize*2))
						break;

					if (VectorToSUB(Patch->Origin, k) > VectorToSUB(FMaxs,k) + (PatchSize*2))
						break;
				}
				if (k != 3)
					continue;
				
				// Don't include patches that might cross through walls or floors, or
				// it will cause false light bleeds from patch to patch
				//if (OPatch != Patch)
				//if (RayCollision(&OPatch->Origin, &Patch->Origin, NULL))
				//	continue;
				
				if (!AddPointToTriangulation (Patch, Tri))
				{
					GHook.Error("AbsorbPatches:  Could not add patch to triangulation.\n");
					return GE_FALSE;
				}
				
			}
		}
		/*
		TempPatches = NULL;
		
		TempPatches = MakeCornerPatches(i, Tri);
		if (!TempPatches)
		{
			GHook.Error("AbsorbPatches:  Could not create corner temp patches.\n");
			return GE_FALSE;
		}

		for (Patch = TempPatches; Patch; Patch = Patch->Next)
		{
			if (!AddPointToTriangulation (Patch, Tri))
			{
				GHook.Error("AbsorbPatches:  Could not add temp patch to triangulation.\n");
				return GE_FALSE;
			}
		}
		*/
		if (!TriangulatePoints (Tri))
		{
			GHook.Error("AbsorbPatches:  Could not triangulate patches.\n");
			return GE_FALSE;
		}
		
		if (Flags & TEXINFO_GOURAUD)
		{
			for (k=0; k< pGFXFace->NumVerts; k++)
			{
				int32		vn;

				vn = pGFXFace->FirstVert+k;

				pPoint = &GFXVerts[GFXVertIndexList[vn]];

				SampleTriangulation (pPoint, Tri, &Add);

				geVec3d_Add(&GFXRGBVerts[vn], &Add, &GFXRGBVerts[vn]);
			}
		}
		else
		{

			geBoolean Created = (pRGB != NULL);
		
			for (k=0; k< FaceInfo[i].NumPoints; k++, pRGB++, pPoint++)
			{
				if (!SampleTriangulation (pPoint, Tri, &Add))
				{
					GHook.Error("AbsorbPatches:  Could not sample from patch triangles.\n");
					continue;
				}

				if (!Created)
				{
					if (Add.X > 0 || Add.Y > 0 || Add.Z > 0)
					{
						if (Lightmaps[i].NumLTypes > MAX_LTYPES)
						{
							GHook.Error("AbsorbPatches:  Too many Light Types on Face.\n");
							return GE_FALSE;
						}

						Lightmaps[i].RGBLData[0] = GE_RAM_ALLOCATE_ARRAY(geVec3d,FaceInfo[i].NumPoints);
						if (!Lightmaps[i].RGBLData[0])
						{
							GHook.Error("AbsorbPAtches:  Out of memory for lightmap.\n");
							return GE_FALSE;
						}
						Lightmaps[i].NumLTypes++;
						pRGB = Lightmaps[i].RGBLData[0];
						memset(pRGB, 0, FaceInfo[i].NumPoints*sizeof(geVec3d));
						pRGB = &pRGB[k];
						Created = GE_TRUE;
					}
				}
				if (Created)
					geVec3d_Add(pRGB, &Add, pRGB);

			}
		}

		Tri_PatchDestroy(Tri);
		//FreePatchList(TempPatches);
		TempPatches = NULL;
	}

	FreePlaneFaces();

	return GE_TRUE;
}

//====================================================================================
//	Tri_PatchCreate
//====================================================================================
Tri_Patch *Tri_PatchCreate(GBSP_Plane *Plane)
{
	Tri_Patch	*Patch;

	Patch = GE_RAM_ALLOCATE_STRUCT(Tri_Patch);
	if (!Patch)
	{
		GHook.Error("Tri_PatchCreate: Out of memory.\n");
		return NULL;
	}

	Patch->NumPoints = 0;
	Patch->NumEdges = 0;
	Patch->NumTris = 0;

	Patch->Plane = Plane;

	return Patch;
}

//====================================================================================
//	Tri_PatchDestroy
//====================================================================================
void Tri_PatchDestroy(Tri_Patch *tr)
{
	geRam_Free(tr);
}


//====================================================================================
//	FindEdge
//====================================================================================
Tri_Edge *FindEdge(Tri_Patch *TriPatch, int p0, int p1)
{
	Tri_Edge	*e, *be;
	geVec3d		v1;
	geVec3d		normal;
	geFloat		dist;

	if (TriPatch->EdgeMatrix[p0][p1])
		return TriPatch->EdgeMatrix[p0][p1];

	if (TriPatch->NumEdges > MAX_TRI_EDGES-2)
	{
		GHook.Error ("TriPatch->NumEdges > MAX_TRI_EDGES-2");
		return NULL;
	}

	geVec3d_Subtract (&TriPatch->Points[p1]->Origin, &TriPatch->Points[p0]->Origin, &v1);
	geVec3d_Normalize (&v1);
	geVec3d_CrossProduct (&v1, &TriPatch->Plane->Normal, &normal);
	dist = geVec3d_DotProduct (&TriPatch->Points[p0]->Origin, &normal);

	e = &TriPatch->Edges[TriPatch->NumEdges];
	e->p0 = p0;
	e->p1 = p1;
	e->tri = NULL;
	geVec3d_Copy(&normal, &e->normal);
	e->dist = dist;
	TriPatch->NumEdges++;
	TriPatch->EdgeMatrix[p0][p1] = e;

	// Go ahead and make the reverse edge ahead of time
	be = &TriPatch->Edges[TriPatch->NumEdges];
	be->p0 = p1;
	be->p1 = p0;
	be->tri = NULL;
	geVec3d_Copy(&normal, &be->normal);
	geVec3d_Inverse(&be->normal);
	be->dist = -dist;
	TriPatch->NumEdges++;
	TriPatch->EdgeMatrix[p1][p0] = be;

	return e;
}

//====================================================================================
//	AllocTriangle
//====================================================================================
Tri	*AllocTriangle(Tri_Patch *TriPatch)
{
	Tri	*t;

	if (TriPatch->NumTris >= MAX_TRI_TRIS)
	{
		GHook.Error ("TriPatch->NumTris >= MAX_TRI_TRIS");
		return NULL;
	}

	t = &TriPatch->TriList[TriPatch->NumTris];
	TriPatch->NumTris++;

	return t;
}

//====================================================================================
//	Tri_Edge_r
//====================================================================================
geBoolean Tri_Edge_r (Tri_Patch *TriPatch, Tri_Edge *e)
{
	int			i, bestp;
	geVec3d		v1, v2;
	geVec3d		*p0, *p1, *p;
	geFloat		best, ang;
	Tri			*nt;
	Tri_Edge	*e2;

	if (e->tri)
		return GE_TRUE;		

	p0 = &TriPatch->Points[e->p0]->Origin;
	p1 = &TriPatch->Points[e->p1]->Origin;
	best = 1.1f;
	for (i=0 ; i< TriPatch->NumPoints ; i++)
	{
		p = &TriPatch->Points[i]->Origin;

		if (geVec3d_DotProduct(p, &e->normal) - e->dist < 0)
			continue;	
		
		geVec3d_Subtract(p0, p, &v1);
		geVec3d_Subtract(p1, p, &v2);
		
		if (!geVec3d_Normalize(&v1))
			continue;
		if (!geVec3d_Normalize(&v2))
			continue;

		ang = geVec3d_DotProduct (&v1, &v2);
		if (ang < best)
		{
			best = ang;
			bestp = i;
		}
	}
	if (best >= 1)
		return GE_TRUE;		
	
	nt = AllocTriangle (TriPatch);
	if (!nt)
	{
		GHook.Error("Tri_Edge_r:  Could not allocate triangle.\n");
		return GE_FALSE;
	}
	nt->Edges[0] = e;
	if (!nt->Edges[0])
	{
		GHook.Error("Tri_Edge_r:  There was an error finding an edge.\n");
		return GE_FALSE;
	}
	nt->Edges[1] = FindEdge (TriPatch, e->p1, bestp);
	if (!nt->Edges[1])
	{
		GHook.Error("Tri_Edge_r:  There was an error finding an edge.\n");
		return GE_FALSE;
	}
	nt->Edges[2] = FindEdge (TriPatch, bestp, e->p0);
	if (!nt->Edges[2])
	{
		GHook.Error("Tri_Edge_r:  There was an error finding an edge.\n");
		return GE_FALSE;
	}
	for (i=0 ; i<3 ; i++)
		nt->Edges[i]->tri = nt;

	e2 = FindEdge (TriPatch, bestp, e->p1);
	if (!e2)
	{
		GHook.Error("Tri_Edge_r:  There was an error finding an edge.\n");
		return GE_FALSE;
	}
	if (!Tri_Edge_r (TriPatch, e2))
		return GE_FALSE;
	
	e2 = FindEdge (TriPatch, e->p0, bestp);
	if (!e2)
	{
		GHook.Error("Tri_Edge_r:  There was an error finding an edge.\n");
		return GE_FALSE;
	}
	if (!Tri_Edge_r (TriPatch, e2))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//	TriangulatePoints
//====================================================================================
geBoolean TriangulatePoints(Tri_Patch *TriPatch)
{
	geFloat		d, bestd;
	geVec3d		v1;
	int			bp1, bp2, i, j;
	geVec3d		*p1, *p2;
	Tri_Edge	*e, *e2;

	for (i=0; i<TriPatch->NumPoints; i++)
		memset(TriPatch->EdgeMatrix[i], 0, TriPatch->NumPoints*sizeof(TriPatch->EdgeMatrix[0][0]) );

	if (TriPatch->NumPoints < 2)
		return GE_TRUE;

	// Find the two closest Points
	bestd = MIN_MAX_BOUNDS2;
	for (i=0 ; i<TriPatch->NumPoints ; i++)
	{
		p1 = &TriPatch->Points[i]->Origin;
		for (j=i+1 ; j<TriPatch->NumPoints ; j++)
		{
			p2 = &TriPatch->Points[j]->Origin;
			geVec3d_Subtract(p2, p1, &v1);
			d = geVec3d_Length(&v1);
			if (d < bestd && d > .05f)
			{
				bestd = d;
				bp1 = i;
				bp2 = j;
			}
		}
	}

	e = FindEdge (TriPatch, bp1, bp2);
	if (!e)
	{
		GHook.Error("There was an error finding an edge.\n");
		return GE_FALSE;
	}
	e2 = FindEdge (TriPatch, bp2, bp1);
	if (!e2)
	{
		GHook.Error("There was an error finding an edge.\n");
		return GE_FALSE;
	}
	if (!Tri_Edge_r (TriPatch, e))
		return GE_FALSE;
	if (!Tri_Edge_r (TriPatch, e2))
		return GE_FALSE;

	return GE_TRUE;
}

//====================================================================================
//	AddPointToTriangulation
//====================================================================================
geBoolean AddPointToTriangulation(RAD_Patch *patch, Tri_Patch *TriPatch)
{
	int			pnum;

	pnum = TriPatch->NumPoints;
	if (pnum == MAX_TRI_POINTS)
	{
		GHook.Error ("TriPatch->NumPoints == MAX_TRI_POINTS");
		return GE_FALSE;
	}
	TriPatch->Points[pnum] = patch;
	TriPatch->NumPoints++;

	return GE_TRUE;
}

//====================================================================================
//	LerpTriangle
//====================================================================================
void LerpTriangle(Tri_Patch *TriPatch, Tri *t, geVec3d *Point, geVec3d *Color)
{
	RAD_Patch	*p1, *p2, *p3;
	geVec3d		base, d1, d2;
	geFloat		x, y, x1, y1, x2, y2;

	p1 = TriPatch->Points[t->Edges[0]->p0];
	p2 = TriPatch->Points[t->Edges[1]->p0];
	p3 = TriPatch->Points[t->Edges[2]->p0];

	geVec3d_Copy(&p1->RadFinal, &base);
	geVec3d_Subtract(&p2->RadFinal, &base, &d1);
	geVec3d_Subtract(&p3->RadFinal, &base, &d2);

	x = geVec3d_DotProduct(Point, &t->Edges[0]->normal) - t->Edges[0]->dist;
	y = geVec3d_DotProduct(Point, &t->Edges[2]->normal) - t->Edges[2]->dist;

	x1 = 0.0f;
	y1 = geVec3d_DotProduct(&p2->Origin, &t->Edges[2]->normal) - t->Edges[2]->dist;

	x2 = geVec3d_DotProduct(&p3->Origin, &t->Edges[0]->normal) - t->Edges[0]->dist;
	y2 = 0.0f;

	if (fabs(y1)<ON_EPSILON || fabs(x2)<ON_EPSILON)
	{
		geVec3d_Copy(&base, Color);
		return;
	}

	geVec3d_AddScaled(&base, &d2, x/x2, Color);
	geVec3d_AddScaled(Color, &d1, y/y1, Color);
}

//====================================================================================
//	Tri_PointInside
//====================================================================================
geBoolean Tri_PointInside(Tri *Tri, geVec3d *Point)
{
	int			i;

	for (i=0 ; i<3 ; i++)
	{
		geFloat		Dist;
		Tri_Edge	*pEdge;

		pEdge = Tri->Edges[i];

		Dist = geVec3d_DotProduct(&pEdge->normal, Point) - pEdge->dist;

		if (Dist < 0.0f)
			return GE_FALSE;	
	}

	return GE_TRUE;
}

//====================================================================================
//	SampleTriangulation
//====================================================================================
geBoolean SampleTriangulation(geVec3d *Point, Tri_Patch *TriPatch, geVec3d *Color)
{
	Tri			*t;
	Tri_Edge	*e;
	geFloat		d;
	RAD_Patch	*p0, *p1;
	geVec3d		v1, v2;
	int			i, j;

	if (TriPatch->NumPoints == 0)
	{
		geVec3d_Clear(Color);
		return GE_TRUE;
	}
	if (TriPatch->NumPoints == 1)
	{
		geVec3d_Copy(&TriPatch->Points[0]->RadFinal, Color);
		return GE_TRUE;
	}
	
	// See of the Point is inside a tri in the patch
	for (t = TriPatch->TriList, j=0 ; j < TriPatch->NumTris ; t++, j++)
	{
		if (!Tri_PointInside (t, Point))
			continue;

		LerpTriangle (TriPatch, t, Point, Color);

		return GE_TRUE;
	}
	
	for (e=TriPatch->Edges, j=0 ; j< TriPatch->NumEdges ; e++, j++)
	{
		if (e->tri)
			continue;		// not an exterior edge

		d = geVec3d_DotProduct(Point, &e->normal) - e->dist;
		if (d < 0)
			continue;	// not in front of edge

		p0 = TriPatch->Points[e->p0];
		p1 = TriPatch->Points[e->p1];
	
		geVec3d_Subtract(&p1->Origin, &p0->Origin, &v1);
		geVec3d_Normalize (&v1);
		
		geVec3d_Subtract(Point, &p0->Origin, &v2);
		d = geVec3d_DotProduct(&v2, &v1);

		if (d < 0)
			continue;
		if (d > 1)
			continue;
		
		for (i=0 ; i<3 ; i++)
			VectorToSUB(*Color,i) = VectorToSUB(p0->RadFinal,i) + d * (VectorToSUB(p1->RadFinal,i) - VectorToSUB(p0->RadFinal,i));
		return GE_TRUE;
	}
	
	if (!FindClosestTriPoint(Point, TriPatch, Color))
	{
		GHook.Error("SampleTriangulation:  Could not find closest Color.\n");
		return GE_FALSE;
	}

	return GE_TRUE;
}

//========================================================================================
//	FindClosestTriPoint
//========================================================================================
geBoolean FindClosestTriPoint(geVec3d *Point, Tri_Patch *Tri, geVec3d *Color)
{
	int32		i;
	RAD_Patch	*p0, *BestPatch;
	geFloat		BestDist, d;
	geVec3d		v1;

	// Search for nearest Point
	BestDist = MIN_MAX_BOUNDS2;
	BestPatch = NULL;

	for (i=0 ; i<Tri->NumPoints ; i++)
	{
		p0 = Tri->Points[i];
		geVec3d_Subtract(Point, &p0->Origin, &v1);
		d = geVec3d_Length(&v1);
		if (d < BestDist)
		{
			BestDist = d;
			BestPatch = p0;
		}
	}

	if (!BestPatch)
	{
		GHook.Error ("FindClosestTriPoint: No Points.\n");
		return GE_FALSE;
	}

	geVec3d_Copy(&BestPatch->RadFinal, Color);

	return GE_TRUE;
}

//========================================================================================
//	CalcPatchReflectivity
//========================================================================================
void CalcPatchReflectivity(int32 Face, RAD_Patch *Patch)
{
	GFX_Texture		*pTexture;
	geVec3d			Color;
	int32				i, Size;
	uint8			*pGFXTexData;
	DRV_Palette		*Palette;
	GFX_TexInfo		*pTexInfo;
	geFloat			Scale;
	
	pTexInfo = &GFXTexInfo[GFXFaces[Face].TexInfo];
	pTexture = &GFXTextures[pTexInfo->Texture];

	geVec3d_Clear(&Color);
	
	pGFXTexData = &GFXTexData[pTexture->Offset];
	Size = pTexture->Width*pTexture->Height;

	Palette = &GFXPalettes[pTexture->PaletteIndex];

	for (i=0; i< Size; i++, pGFXTexData++)
	{
		DRV_RGB *	RGB;

		RGB = &(*Palette)[*pGFXTexData];
		Color.X += (geFloat)RGB->r;
		Color.Y += (geFloat)RGB->g;
		Color.Z += (geFloat)RGB->b;
	}

	geVec3d_Scale(&Color, 1.0f/(geFloat)Size, &Color);
	geVec3d_Scale(&Color, 1.0f/255.0f, &Color);

	Scale = ColorNormalize(&Color, &Patch->Reflectivity);
	
	if (Scale < 0.5f)
	{
		Scale *= 2;
		geVec3d_Scale(&Patch->Reflectivity, Scale, &Patch->Reflectivity);
	}

	geVec3d_Scale(&Patch->Reflectivity, ReflectiveScale*pTexInfo->ReflectiveScale, &Patch->Reflectivity);
}

typedef struct
{
	int32		Version;
	SYSTEMTIME	BSPTime;
	int32		NumPatches;
} Rec_Header;

Rec_Header	RecHeader;	

//========================================================================================
//	SaveReceiverFile
//========================================================================================
geBoolean SaveReceiverFile(char *FileName)
{
	FILE		*f;
	int32		i;

	GHook.Printf("--- Save Receiver File --- \n");

	f = fopen(FileName, "wb");

	if (!f)
	{
		GHook.Error("SaveReceiverFile:  Could not open receiver file for writing...\n");
		return GE_FALSE;
	}

	RecHeader.Version = GBSPHeader.Version;
	RecHeader.BSPTime = GBSPHeader.BSPTime;
	RecHeader.NumPatches = NumPatches;

	// Save header
	if (fwrite(&RecHeader, sizeof(Rec_Header), 1, f) != 1)
	{
		GHook.Printf("*WARNING* SaveReceiverFile:  Could not save header info...\n");
		fclose(f);
		return GE_TRUE;
	}

	// Patches
	for (i=0; i< NumPatches; i++)
	{
		int32 NumReceivers = PatchList[i]->NumReceivers;

		// Write out the number of receivers for this patch
		if (fwrite(&NumReceivers, sizeof(int32), 1, f) != 1)
		{
			GHook.Printf("*WARNING* SaveReceiverFile:  Could not save num receivers...\n");
			fclose(f);
			return GE_TRUE;
		}
		
		// Write out the actual receivers
		if (fwrite(PatchList[i]->Receivers, sizeof(RAD_Receiver), NumReceivers, f) != (uint32)NumReceivers)
		{
			GHook.Printf("*WARNING* SaveReceiverFile:  Could not save receivers...\n");
			fclose(f);
			return GE_TRUE;
		}
	}

	return GE_TRUE;
}

//========================================================================================
//	LoadReceiverFile
//========================================================================================
geBoolean LoadReceiverFile(char *FileName)
{
	FILE		*f;
	int32		i;
	uint8		*pTime1, *pTime2;

	f = fopen(FileName, "rb");

	if (!f)		
		return GE_FALSE;

	// Load header
	if (fread(&RecHeader, sizeof(Rec_Header), 1, f) != 1)
	{
		GHook.Error("LoadReceiverFile:  Could not load header info...\n");
		fclose(f);
		return GE_FALSE;
	}

	pTime1 = (uint8*)&(RecHeader.BSPTime);
	pTime2 = (uint8*)&(GBSPHeader.BSPTime);

	if (RecHeader.Version != GBSPHeader.Version)
	{
		GHook.Printf("*WARNING*  LoadReceiverFile:  Versions do not match, skipping...\n");
		fclose(f);
		return GE_FALSE;
	}
	
	// Make sure internal time matches...
	for (i=0; i< sizeof(SYSTEMTIME); i++)
	{
		if (*pTime1 != *pTime2)
		{
			fclose(f);
			return GE_FALSE;
		}

		pTime1++;
		pTime2++;
	}

	// Make sure the number of patches in the receiver file, matches the number loaded
	// for this BSP
	if (RecHeader.NumPatches != NumPatches)
	{
		GHook.Printf("*WARNING*  LoadReceiverFile:  NumPatches do not match, skipping...\n");
		fclose(f);
		return GE_FALSE;
	}

	// Load Patche receivers
	for (i=0; i< NumPatches; i++)
	{
		int32 NumReceivers;

		// Load the number of receivers for this patch
		if (fread(&NumReceivers, sizeof(int32), 1, f) != 1)
		{
			GHook.Error("LoadReceiverFile:  Could not load num receivers...\n");
			fclose(f);
			return GE_FALSE;
		}
		
		PatchList[i]->Receivers = GE_RAM_ALLOCATE_ARRAY(RAD_Receiver,NumReceivers);

		if (!PatchList[i]->Receivers)
		{
			GHook.Error("LoadReceiverFile:  Out of memory for receivers.\n");
			FreeReceivers();
			return GE_FALSE;
		}

		PatchList[i]->NumReceivers = (uint16)NumReceivers;

		// Load the actual receivers
		if (fread(PatchList[i]->Receivers, sizeof(RAD_Receiver), NumReceivers, f) != (uint32)NumReceivers)
		{
			GHook.Error("SaveReceiverFile:  Could not save receivers...\n");
			fclose(f);
			return GE_FALSE;
		}
	}

	return GE_TRUE;
}
