/****************************************************************************************/
/*  VisFlood.cpp                                                                        */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Vises a BSP                                                            */
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

#include "Utils.h"
#include "Vis.h"
#include "GBSPFile.h"
#include "Poly.h"
#include "Bsp.h"

#include "Ram.h"

extern int32		NumVisPortals;				// Total portals
extern int32		NumVisPortalBytes;			// Total portals / 8
extern int32		NumVisPortalLongs;			// Total portalbytes / 4
extern VIS_Portal	*VisPortals;				// NumVisPortals
extern pVIS_Portal	*VisSortedPortals;			// Pointers to portals sorted by MightSee
extern uint8		*PortalSeen;				// Temp vis array

extern int32		NumVisLeafs;				// Total VisLeafs
extern int32		NumVisLeafBytes;			// NumVisLeaf / 8
extern int32		NumVisLeafLongs;			// NumVisBytes / sizeof(uint32)
extern uint8		*LeafVisBits;				// Should be NumVisLeafs * (NumVisLeafs / 8)
extern VIS_Leaf		*VisLeafs;					// NumVisLeafs

int32	CanSee;
int32	SrcLeaf;
int32	MightSee;

extern geBoolean	VisVerbose;
extern geBoolean	NoSort;
extern geBoolean	FullVis;

//=======================================================================================
//	FloodPortalsFast_r
//=======================================================================================
void FloodPortalsFast_r(VIS_Portal *SrcPortal, VIS_Portal *DestPortal)
{
	VIS_Leaf	*Leaf;
	VIS_Portal	*Portal;
	int32		LeafNum;
	int32		PNum;

	PNum = (int32)(DestPortal - VisPortals);
	
	if (CancelRequest)
		return;

	if (PortalSeen[PNum])
		return;

	PortalSeen[PNum] = 1;

	// Add the portal that we are Flooding into, to the original portals visbits
	LeafNum = DestPortal->Leaf;
	
	int32	Bit = 1<<(PNum&7);
	if (!(SrcPortal->VisBits[PNum>>3] & Bit))
	{
		SrcPortal->VisBits[PNum>>3] |= Bit;
		SrcPortal->MightSee++;
		VisLeafs[SrcLeaf].MightSee++;
		MightSee++;
	}

	Leaf = &VisLeafs[LeafNum];
	// Now, try and Flood into the leafs that this portal touches
	for (Portal = Leaf->Portals; Portal; Portal = Portal->Next)
	{
		// If SrcPortal can see this Portal, flood into it...
		if (PortalCanSeePortal(SrcPortal, Portal))
			FloodPortalsFast_r(SrcPortal, Portal);
	}
}

//=======================================================================================
// FloodLeafPortalsFast
//=======================================================================================
void FloodLeafPortalsFast(int32 LeafNum)
{
	VIS_Leaf	*Leaf;
	VIS_Portal	*Portal;
	int32		PNum;

	Leaf = &VisLeafs[LeafNum];

	if (!Leaf->Portals)
	{
		//GHook.Printf("*WARNING* FloodLeafPortalsFast:  Leaf with no portals.\n");
		return;
	}
	
	SrcLeaf = LeafNum;

	for (Portal = Leaf->Portals; Portal; Portal = Portal->Next)
	{
		Portal->VisBits = GE_RAM_ALLOCATE_ARRAY(uint8,NumVisPortalBytes);

		// This portal can't see anyone yet...
		memset(Portal->VisBits, 0, NumVisPortalBytes);
		memset(PortalSeen, 0, NumVisPortals);

		MightSee = 0;
		
		FloodPortalsFast_r(Portal, Portal);

		PNum = (int32)(Portal - VisPortals);
		
		//Hook.Printf("Portal: %5i - MightSee: %5i\n", PNum, MightSee);
		if (CancelRequest)
			return;
	}
}

geFloat PlaneDistanceFastP(geVec3d *Point, GBSP_Plane *Plane)
{
   geFloat	Dist,Dist2;
   Dist2 = Plane->Dist;

   switch (Plane->Type)
   {
	   case PLANE_X:
           //Dist = (Point->X - Dist2);
           //break;
	   case PLANE_Y:
           //Dist = (Point->Y - Dist2);
           //break;
	   case PLANE_Z:
           //Dist = (Point->Z - Dist2);
           //break;
       default:
           Dist = geVec3d_DotProduct(Point, &Plane->Normal) - Dist2;
           break;
    }

    return Dist;
}

//=======================================================================================
//	PortalCanSeePortal
//	See if Portal1 can see into Portal2 
//	(Quick, does not take into acount of other leafs blocking path)
//=======================================================================================
geBoolean PortalCanSeePortal(VIS_Portal *Portal1, VIS_Portal *Portal2)
{
	int32		i;
	GBSP_Poly	*Poly;
	GBSP_Plane	*pPlane;
	geVec3d		*pVert;
	geFloat		Dist;

	// Check first portal
	Poly = Portal1->Poly;
	pVert = Poly->Verts;
	pPlane = &Portal2->Plane;

	for (i=0; i< Poly->NumVerts; i++)
	{
		Dist = PlaneDistanceFastP(pVert, pPlane);

		if (Dist < -ON_EPSILON)
			break;
	
		pVert++;
	}

	if (i == Poly->NumVerts)
		return GE_FALSE;				// No points of Portal1 behind Portal2, can't possibly see

	// Check second portal
	Poly = Portal2->Poly;
	pVert = Poly->Verts;

	pPlane = &Portal1->Plane;

	for (i=0; i< Poly->NumVerts; i++)
	{
		Dist = PlaneDistanceFastP(pVert, pPlane);

		if (Dist > ON_EPSILON)
			break;

		pVert++;
	}

	if (i == Poly->NumVerts)
		return GE_FALSE;				// No points of Portal2 in front of Portal1, can't possibly see

	return GE_TRUE;						// It can see through it!!!
}


//=======================================================================================
//	ClipToSeperators
//=======================================================================================
geBoolean ClipToSeperators (GBSP_Poly *Source, GBSP_Poly *Pass, GBSP_Poly *Target, geBoolean FlipClip, GBSP_Poly **Dest)
{
	int32		i, j, k, l;
	GBSP_Plane	Plane;
	geVec3d		v1, v2;
	geFloat		d;
	geFloat		Length;
	int32		Counts[3];
	geBoolean	FlipTest;

	for (i=0 ; i<Source->NumVerts ; i++)
	{
		l = (i+1)%Source->NumVerts;
		geVec3d_Subtract(&Source->Verts[l] , &Source->Verts[i], &v1);

		for (j=0 ; j<Pass->NumVerts ; j++)
		{
			geVec3d_Subtract(&Pass->Verts[j], &Source->Verts[i], &v2);

			geVec3d_CrossProduct(&v1, &v2, &Plane.Normal);
			
			Length = geVec3d_Normalize(&Plane.Normal);

			if (Length < ON_EPSILON)
				continue;
			
			Plane.Dist = geVec3d_DotProduct(&Pass->Verts[j], &Plane.Normal);

		#if 1
			FlipTest = GE_FALSE;
			for (k=0 ; k<Source->NumVerts ; k++)
			{
				if (k == i || k == l)
					continue;

				d = geVec3d_DotProduct(&Source->Verts[k], &Plane.Normal) - Plane.Dist;
				if (d < -ON_EPSILON)
				{
					FlipTest = GE_FALSE;
					break;
				}
				else if (d > ON_EPSILON)
				{	
					FlipTest = GE_TRUE;
					break;
				}
			}
			if (k == Source->NumVerts)
				continue;		
#else
			FlipTest = FlipClip;
#endif
			if (FlipTest)
			{
				geVec3d_Inverse(&Plane.Normal);
				Plane.Dist = -Plane.Dist;
			}
#if 1
			Counts[0] = Counts[1] = Counts[2] = 0;

			for (k=0 ; k<Pass->NumVerts ; k++)
			{
				if (k==j)
					continue;
				d = geVec3d_DotProduct(&Pass->Verts[k], &Plane.Normal) - Plane.Dist;
				if (d < -ON_EPSILON)
					break;
				else if (d > ON_EPSILON)
					Counts[0]++;
				else
					Counts[2]++;
			}
			if (k != Pass->NumVerts)
				continue;	
				
			if (!Counts[0])
				continue;
#else
			k = (j+1)%Pass->NumVerts;
			d = geVec3d_DotProduct(&Pass->Verts[k], &Plane.Normal) - Plane.Dist;
			if (d < -ON_EPSILON)
				continue;
			k = (j+Pass->NumVerts-1)%Pass->NumVerts;
			d = geVec3d_DotProduct(&Pass->Verts[k], &Plane.Normal) - Plane.Dist;
			if (d < -ON_EPSILON)
				continue;			
#endif
			if (!ClipPoly(Target, &Plane, FlipClip, &Target))
			{
				GHook.Error("ClipToPortals:  Error clipping portal.\n");
				return GE_FALSE;
			}

			if (!Target)
			{
				*Dest = NULL;
				return GE_TRUE;	
			}
		}
	}
	
	*Dest = Target;
	return GE_TRUE;
}

//=======================================================================================
//	FloodPortalsSlow_r
//=======================================================================================
geBoolean FloodPortalsSlow_r(VIS_Portal *SrcPortal, VIS_Portal *DestPortal, VIS_PStack *PrevStack)
{
	VIS_Leaf	*Leaf;
	VIS_Portal	*Portal;
	int32		LeafNum, j;
	int32		PNum;
	uint32		*Test, *Might, *Vis, More;
	VIS_PStack	Stack;

	PNum = (int32)(DestPortal - VisPortals);
	
	if (CancelRequest)
	{
		GHook.Printf("Cancel requested...\n");
		return GE_FALSE;
	}

	//if (PortalSeen[PNum])
	//	return GE_TRUE;
	//PortalSeen[PNum] = 1;

	// Add the portal that we are Flooding into, to the original portals visbits
	int32	Bit = 1<<(PNum&7);
	if (!(SrcPortal->FinalVisBits[PNum>>3] & Bit))
	{
		SrcPortal->FinalVisBits[PNum>>3] |= Bit;
		SrcPortal->CanSee++;
		VisLeafs[SrcLeaf].CanSee++;
		CanSee++;
	}

	// Get the leaf that this portal looks into, and flood from there
	LeafNum = DestPortal->Leaf;
	Leaf = &VisLeafs[LeafNum];

	Might = (uint32*)Stack.VisBits;
	Vis = (uint32*)SrcPortal->FinalVisBits;

	// Now, try and Flood into the leafs that this portal touches
	for (Portal = Leaf->Portals; Portal; Portal = Portal->Next)
	{
		PNum = (int32)(Portal - VisPortals);
		Bit = 1<<(PNum&7);

		//GHook.Printf("PrevStack VisBits:  %i\n", PrevStack->VisBits[PNum>>3]);

		// If might see could'nt see it, then don't worry about it
		if (!(SrcPortal->VisBits[PNum>>3] & Bit))
			continue;

		if (!(PrevStack->VisBits[PNum>>3] & Bit))
			continue;	// Can't possibly see it

		// If the portal can't see anything we haven't allready seen, skip it
		if (Portal->Done)
			Test = (uint32*)Portal->FinalVisBits;
		else
			Test = (uint32*)Portal->VisBits;

		More = 0;
		for (j=0 ; j<NumVisPortalLongs ; j++)
		{
			Might[j] = ((uint32*)PrevStack->VisBits)[j] & Test[j];
			More |= (Might[j] & ~Vis[j]);
		}
		
		if (!More && (SrcPortal->FinalVisBits[PNum>>3] & Bit) )	// Can't see anything new
			continue;
		
		// Setup Source/Pass
		if (!CopyPoly(Portal->Poly, &Stack.Pass))
			return GE_FALSE;

	#if 0
		geFloat Dist;

		Dist = geVec3d_DotProduct(&Portal->Center, &SrcPortal->Plane.Normal) - SrcPortal->Plane.Dist;

		if (Dist < -Portal->Radius)			// Totally behind
			continue;
		else if (Dist < Portal->Radius)	
		{
			if (!ClipPoly(Stack.Pass, &SrcPortal->Plane, GE_FALSE, &Stack.Pass))
				return GE_FALSE;
			if (!Stack.Pass)
				continue;
		}
	#else
		// Cut away portion of pass portal we can't see through
		if (!ClipPoly(Stack.Pass, &SrcPortal->Plane, GE_FALSE, &Stack.Pass))
			return GE_FALSE;
		if (!Stack.Pass)
			continue;
	#endif

		if (!CopyPoly(PrevStack->Source, &Stack.Source))
			return GE_FALSE;

	#if 0
		Dist = geVec3d_DotProduct(&SrcPortal->Center, &Portal->Plane.Normal) - Portal->Plane.Dist;

		if (Dist > Portal->Radius)			// Totally behind
			continue;
		else if (Dist > -Portal->Radius)	
		{
			if (!ClipPoly(Stack.Source, &Portal->Plane, GE_TRUE, &Stack.Source))
				return GE_FALSE;
			if (!Stack.Source)
			{
				FreePoly(Stack.Pass);
				continue;
			}
		}
	#else
		// Also, Cut away portion of source portal that can't be seen through pass
		if (!ClipPoly(Stack.Source, &Portal->Plane, GE_TRUE, &Stack.Source))
			return GE_FALSE;
		if (!Stack.Source)
		{
			FreePoly(Stack.Pass);
			continue;
		}
	#endif

		// If we don't have a PrevStack->Pass, then we don't have enough to look through.
		// This portal can only be blocked by VisBits (Above test)...
		if (!PrevStack->Pass)
		{
			if (!FloodPortalsSlow_r(SrcPortal, Portal, &Stack))
				return GE_FALSE;

			FreePoly(Stack.Source);
			FreePoly(Stack.Pass);
			continue;
		}

		if (!ClipToSeperators(Stack.Source, PrevStack->Pass, Stack.Pass, GE_FALSE, &Stack.Pass))
			return GE_FALSE;

		if (!Stack.Pass)
		{
			FreePoly(Stack.Source);
			continue;
		}
		
	#if 1
		if (!ClipToSeperators(PrevStack->Pass, Stack.Source, Stack.Pass, GE_TRUE, &Stack.Pass))
			return GE_FALSE;
		if (!Stack.Pass)
		{
			FreePoly(Stack.Source);
			continue;
		}
	#else
	/*
		if (!ClipToSeperators(Stack.Source, PrevStack->Pass, Stack.Source, GE_TRUE, &Stack.Source))
			return GE_FALSE;

		if (!Stack.Source)
		{
			FreePoly(Stack.Pass);
			continue;
		}
	*/
	#endif		
		// Flood into it...
		if (!FloodPortalsSlow_r(SrcPortal, Portal, &Stack))
			return GE_FALSE;

		FreePoly(Stack.Source);
		FreePoly(Stack.Pass);
	}

	return GE_TRUE;
}

//=======================================================================================
// FloodLeafPortalsSlow
//=======================================================================================
geBoolean FloodLeafPortalsSlow(int32 LeafNum)
{
	VIS_Leaf	*Leaf;
	VIS_Portal	*Portal;
	int32		PNum;
	VIS_PStack	PStack;
	int32		i;

	Leaf = &VisLeafs[LeafNum];

	if (!Leaf->Portals)
	{
		//GHook.Printf("*WARNING* FloodLeafPortalsFast:  Leaf with no portals.\n");
		return GE_TRUE;
	}
	
	SrcLeaf = LeafNum;
	for (Portal = Leaf->Portals; Portal; Portal = Portal->Next)
	{
		Portal->FinalVisBits = GE_RAM_ALLOCATE_ARRAY(uint8,NumVisPortalBytes);

		// This portal can't see anyone yet...
		memset(Portal->FinalVisBits, 0, NumVisPortalBytes);
		memset(PortalSeen, 0, NumVisPortals);

		CanSee = 0;
		
		for (i=0; i< NumVisPortalBytes; i++)
			PStack.VisBits[i] = Portal->VisBits[i];

		// Setup Source/Pass
		if (!CopyPoly(Portal->Poly, &PStack.Source))
			return GE_FALSE;
		PStack.Pass = NULL;

		if (!FloodPortalsSlow_r(Portal, Portal, &PStack))
			return GE_FALSE;

		Portal->Done = GE_TRUE;

		PNum = (int32)(Portal - VisPortals);
		//Hook.Printf("Portal: %5i - MightSee: %5i\n", PNum, MightSee);
	}

	return GE_TRUE;
}

//=======================================================================================
//	FloodPortalsSlow
//=======================================================================================
geBoolean FloodPortalsSlow(void)
{
	VIS_Portal	*Portal;
	int32		PNum;
	VIS_PStack	PStack;
	int32		i, k;

	for (k=0; k< NumVisPortals; k++)
		VisPortals[k].Done = GE_FALSE;

	for (k=0; k< NumVisPortals; k++)
	{
		Portal = VisSortedPortals[k];
		
		Portal->FinalVisBits = GE_RAM_ALLOCATE_ARRAY(uint8,NumVisPortalBytes);

		// This portal can't see anyone yet...
		memset(Portal->FinalVisBits, 0, NumVisPortalBytes);
		memset(PortalSeen, 0, NumVisPortals);

		CanSee = 0;
		
		for (i=0; i< NumVisPortalBytes; i++)
			PStack.VisBits[i] = Portal->VisBits[i];

		// Setup Source/Pass
		if (!CopyPoly(Portal->Poly, &PStack.Source))
			return GE_FALSE;
		PStack.Pass = NULL;

		if (!FloodPortalsSlow_r(Portal, Portal, &PStack))
			return GE_FALSE;

		FreePoly(PStack.Source);

		Portal->Done = GE_TRUE;

		PNum = (int32)(Portal - VisPortals);
		if (VisVerbose)
			GHook.Printf("Portal: %4i - Fast Vis: %4i, Full Vis: %4i\n", k+1, Portal->MightSee, Portal->CanSee);
	}

	return GE_TRUE;
}
