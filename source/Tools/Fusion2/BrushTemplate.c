/****************************************************************************************/
/*  BrushTemplate.c                                                                     */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Pollard, John Moore          */
/*  Description:  Geometric generation of template brushes                              */
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
#include "BrushTemplate.h"
#include "units.h"
#include "facelist.h"
#include "ram.h"

Brush *BrushTemplate_CreateArch (const BrushTemplate_Arch *pTemplate)
{
	Brush		*b, *b2;
	BrushList	*MBList	=BrushList_Create();
	FaceList	*fl;
	Face		*f;
	geVec3d		FaceVerts[4];

	int		i, NumSlits			=pTemplate->NumSlits;
	int		NumCrossSections	=NumSlits + 2;
	geFloat	Thickness			=pTemplate->Thickness;
	geFloat	Width				=pTemplate->Width;
	geFloat	InnerRadius			=pTemplate->Radius;
//	geFloat	WallSize			=pTemplate->WallSize;
	double	StartAngleDegrees	=pTemplate->StartAngle;
	double	EndAngleDegrees		=pTemplate->EndAngle;
	double	AngleDelta			=0;
	double	CurAngle			=0;
	double	StartAngle			=Units_DegreesToRadians (StartAngleDegrees);
	double	EndAngle			=Units_DegreesToRadians (EndAngleDegrees);
	double	Temp;
	geVec3d	TopInnerPoint;
	geVec3d	TopOuterPoint;
	geVec3d	FinalTopInnerPoint;
	geVec3d	FinalTopOuterPoint;
	geVec3d	FinalBottomInnerPoint;
	geVec3d	FinalBottomOuterPoint;
	geVec3d	OldTopInner;
	geVec3d	OldTopOuter;
	geVec3d	OldBottomInner;
	geVec3d	OldBottomOuter;

	//If angles are equal, we have an empty shape...
	if(StartAngle==EndAngle)
	{
		return	NULL;
	}

	//	Put the angles in order...
	if(StartAngle > EndAngle)
	{
		Temp		=StartAngle;
		StartAngle	=EndAngle;
		EndAngle	=Temp;
	}

	geVec3d_Set(&TopInnerPoint, (float)InnerRadius, 0.0, (float)(Width / 2));
	geVec3d_Set(&TopOuterPoint, (float)(InnerRadius + Thickness), 0.0, (float)(Width / 2));

	AngleDelta	=(EndAngle - StartAngle)/(NumCrossSections - 1);
	CurAngle	=StartAngle + AngleDelta;

	//	Create first cross section of 4 vertices ( outer face @ start angle)...
	geVec3d_Set
	(
		&FinalTopInnerPoint,
		(float)(( TopInnerPoint.X * cos( StartAngle ) ) - ( TopInnerPoint.Y * sin( StartAngle ) )),
		(float)(( TopInnerPoint.X * sin( StartAngle ) ) + ( TopInnerPoint.Y * cos( StartAngle ) )),
		TopInnerPoint.Z
	);
	geVec3d_Set
	(
		&FinalTopOuterPoint,
		(float)(( TopOuterPoint.X * cos( StartAngle ) ) - ( TopInnerPoint.Y * sin( StartAngle ) )),
		(float)(( TopOuterPoint.X * sin( StartAngle ) ) + ( TopInnerPoint.Y * cos( StartAngle ) )),
		TopOuterPoint.Z
	);
	FinalBottomInnerPoint	=FinalTopInnerPoint;
	FinalBottomInnerPoint.Z	=-FinalTopInnerPoint.Z;
	FinalBottomOuterPoint	=FinalTopOuterPoint;
	FinalBottomOuterPoint.Z	=-FinalTopOuterPoint.Z;
	OldTopInner				=FinalTopInnerPoint;
	OldTopOuter				=FinalTopOuterPoint;
	OldBottomInner			=FinalBottomInnerPoint;
	OldBottomOuter			=FinalBottomOuterPoint;

	//Create the other cross sections and assign verts to polys after each...
	for(i=0;i < (NumCrossSections-1);i++)
	{
		geVec3d_Set
		(
			&FinalTopInnerPoint,
			(float)(( TopInnerPoint.X * cos( CurAngle ) ) - ( TopInnerPoint.Y * sin( CurAngle ) )),
			(float)(( TopInnerPoint.X * sin( CurAngle ) ) + ( TopInnerPoint.Y * cos( CurAngle ) )),
			TopInnerPoint.Z
		);
		geVec3d_Set
		(
			&FinalTopOuterPoint,
			(float)(( TopOuterPoint.X * cos( CurAngle ) ) - ( TopInnerPoint.Y * sin( CurAngle ) )),
			(float)(( TopOuterPoint.X * sin( CurAngle ) ) + ( TopInnerPoint.Y * cos( CurAngle ) )),
			TopOuterPoint.Z
		);
		FinalBottomInnerPoint = FinalTopInnerPoint;
		FinalBottomInnerPoint.Z = -FinalTopInnerPoint.Z;

		FinalBottomOuterPoint = FinalTopOuterPoint;
		FinalBottomOuterPoint.Z = -FinalTopOuterPoint.Z;

		CurAngle += AngleDelta;

		fl	=FaceList_Create(6);

		//Assign points to the 4 outer poly faces...

		//Top face...
		FaceVerts[0]	=FinalTopInnerPoint;
		FaceVerts[1]	=FinalTopOuterPoint;
		FaceVerts[2]	=OldTopOuter;
		FaceVerts[3]	=OldTopInner;
		f				=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		//	Bottom face...
		FaceVerts[3]	=FinalBottomInnerPoint;
		FaceVerts[2]	=FinalBottomOuterPoint;
		FaceVerts[1]	=OldBottomOuter;
		FaceVerts[0]	=OldBottomInner;
		f				=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		//	Inner side face...
		FaceVerts[0]	=FinalTopInnerPoint;
		FaceVerts[1]	=OldTopInner;
		FaceVerts[2]	=OldBottomInner;
		FaceVerts[3]	=FinalBottomInnerPoint;
		f				=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		//	Outer side face...
		FaceVerts[3]	=FinalTopOuterPoint;
		FaceVerts[2]	=OldTopOuter;
		FaceVerts[1]	=OldBottomOuter;
		FaceVerts[0]	=FinalBottomOuterPoint;
		f				=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		//make the end faces
		FaceVerts[0]	=OldTopOuter;
		FaceVerts[1]	=OldBottomOuter;
		FaceVerts[2]	=OldBottomInner;
		FaceVerts[3]	=OldTopInner;
		f				=Face_Create(4, FaceVerts, 0);

		if(f)
		{
			if(pTemplate->Style < 2)	//default to hollow (if they make hollow later)
			{
				if(i)
				{
					Face_SetFixedHull(f, GE_TRUE);
				}
			}
			else
			{
				Face_SetFixedHull(f, GE_TRUE);
			}
			FaceList_AddFace(fl, f);
		}

		FaceVerts[3]	=FinalTopOuterPoint;
		FaceVerts[2]	=FinalBottomOuterPoint;
		FaceVerts[1]	=FinalBottomInnerPoint;
		FaceVerts[0]	=FinalTopInnerPoint;
		f				=Face_Create(4, FaceVerts, 0);

		if(f)
		{
			if(pTemplate->Style < 2)	//default to hollow (if they make hollow later)
			{
				if(i < (NumCrossSections-2))
				{
					Face_SetFixedHull(f, GE_TRUE);
				}
			}
			else
			{
				Face_SetFixedHull(f, GE_TRUE);
			}
			FaceList_AddFace(fl, f);
		}

		if(!pTemplate->Style)
		{
			b2	=Brush_Create(BRUSH_LEAF, fl, NULL);
			if(b2)
			{
				Brush_SetSubtract(b2, pTemplate->TCut);
			}
			BrushList_Append(MBList, b2);
		}
		else
		{
			BrushList	*bl	=BrushList_Create();
			Brush		*bh, *bm;

			b2	=Brush_Create(BRUSH_LEAF, fl, NULL);
			if(b2)
			{
				Brush_SetHollow(b2, GE_TRUE);
				Brush_SetHullSize(b2, pTemplate->WallSize);
				bh	=Brush_CreateHollowFromBrush(b2);
				if(bh)
				{
					Brush_SetHollowCut(bh, GE_TRUE);
					BrushList_Append(bl, b2);
					BrushList_Append(bl, bh);

					bm	=Brush_Create(BRUSH_MULTI, NULL, bl);
					if(bm)
					{
						Brush_SetHollow(bm, GE_TRUE);
						Brush_SetSubtract(bm, pTemplate->TCut);
						Brush_SetHullSize(bm, pTemplate->WallSize);

						BrushList_Append(MBList, bm);
					}
				}
				else
				{
					Brush_Destroy(&b2);
					BrushList_Destroy(&bl);
				}
			}
			else
			{
				BrushList_Destroy(&bl);
			}
		}

		//	Set old points...
		OldTopInner		=FinalTopInnerPoint;
		OldTopOuter		=FinalTopOuterPoint;
		OldBottomInner	=FinalBottomInnerPoint;
		OldBottomOuter	=FinalBottomOuterPoint;

	}
	b	=Brush_Create(BRUSH_MULTI, NULL, MBList);

	if(b)
	{
		Brush_SetSubtract(b, pTemplate->TCut);
	}

	return	b;
}


Brush *BrushTemplate_CreateBox (const BrushTemplate_Box *pTemplate)
{
	//revisit for error handling when merged
	geVec3d		Verts[8];
	geVec3d		FaceVerts[4];
	FaceList	*fl;
	Face		*f;
	Brush		*b;

	fl	=FaceList_Create(6);

	// Vertices 0 to 3 are the 4 corners of the top face
	geVec3d_Set (&Verts[0], (float)-(pTemplate->XSizeTop/2), (float)(pTemplate->YSize/2), (float)-(pTemplate->ZSizeTop/2));
	geVec3d_Set (&Verts[1], (float)-(pTemplate->XSizeTop/2), (float)(pTemplate->YSize/2), (float)(pTemplate->ZSizeTop/2));
	geVec3d_Set (&Verts[2], (float)(pTemplate->XSizeTop/2), (float)(pTemplate->YSize/2), (float)(pTemplate->ZSizeTop/2));
	geVec3d_Set (&Verts[3], (float)(pTemplate->XSizeTop/2), (float)(pTemplate->YSize/2), (float)-(pTemplate->ZSizeTop/2));

	// Vertices 4 to 7 are the 4 corners of the bottom face
	geVec3d_Set (&Verts[4], (float)-(pTemplate->XSizeBot/2), (float)-(pTemplate->YSize/2), (float)-(pTemplate->ZSizeBot/2));
	geVec3d_Set (&Verts[5], (float)(pTemplate->XSizeBot/2), (float)-(pTemplate->YSize/2), (float)-(pTemplate->ZSizeBot/2));
	geVec3d_Set (&Verts[6], (float)(pTemplate->XSizeBot/2), (float)-(pTemplate->YSize/2), (float)(pTemplate->ZSizeBot/2));
	geVec3d_Set (&Verts[7], (float)-(pTemplate->XSizeBot/2), (float)-(pTemplate->YSize/2), (float)(pTemplate->ZSizeBot/2));

	FaceVerts[3]	=Verts[0];
	FaceVerts[2]	=Verts[1];
	FaceVerts[1]	=Verts[2];
	FaceVerts[0]	=Verts[3];

	f	=Face_Create(4, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	FaceVerts[3]	=Verts[4];
	FaceVerts[2]	=Verts[5];
	FaceVerts[1]	=Verts[6];
	FaceVerts[0]	=Verts[7];

	f	=Face_Create(4, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	FaceVerts[3]	=Verts[1];
	FaceVerts[2]	=Verts[7];
	FaceVerts[1]	=Verts[6];
	FaceVerts[0]	=Verts[2];

	f	=Face_Create(4, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	FaceVerts[3]	=Verts[0];
	FaceVerts[2]	=Verts[3];
	FaceVerts[1]	=Verts[5];
	FaceVerts[0]	=Verts[4];

	f	=Face_Create(4, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	FaceVerts[3]	=Verts[0];
	FaceVerts[2]	=Verts[4];
	FaceVerts[1]	=Verts[7];
	FaceVerts[0]	=Verts[1];

	f	=Face_Create(4, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	FaceVerts[3]	=Verts[3];
	FaceVerts[2]	=Verts[2];
	FaceVerts[1]	=Verts[6];
	FaceVerts[0]	=Verts[5];

	f	=Face_Create(4, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	if(!pTemplate->Solid)
	{
		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetSubtract(b, pTemplate->TCut);
			Brush_SetSheet (b, pTemplate->TSheet);
		}
		return	b;
	}
	else
	{
		// hollow brush
		BrushList	*bl	=BrushList_Create();
		Brush		*bh, *bm;

		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetHollow(b, GE_TRUE);
			Brush_SetHullSize(b, (float)pTemplate->Thickness);
			bh	=Brush_CreateHollowFromBrush(b);
			if(bh)
			{
				Brush_SetHollowCut(bh, GE_TRUE);
				BrushList_Append(bl, b);
				BrushList_Append(bl, bh);

				bm	=Brush_Create(BRUSH_MULTI, NULL, bl);
				if(bm)
				{
					Brush_SetHollow(bm, GE_TRUE);
					Brush_SetSubtract(bm, pTemplate->TCut);
					Brush_SetHullSize(bm, (float)pTemplate->Thickness);
					return	bm;
				}
			}
			else
			{
				Brush_Destroy(&b);
				BrushList_Destroy(&bl);
			}
		}
		else
		{
			BrushList_Destroy(&bl);
		}
	}

	return	NULL;
}


Brush *BrushTemplate_CreateCone (const BrushTemplate_Cone *pTemplate)
{
	int			index, BottomCount;
	geVec3d		StartPoint, CurPoint, OldPoint, OuterFocus;
	geVec3d		FaceVerts[3], *BottomVerts;
	FaceList	*fl;
	Face		*f;
	Brush		*b;

	double CurAngle;
	double AngleDeltaDegrees = 360.0f/(float)pTemplate->VerticalStrips;
	double AngleDelta = UNITS_DEGREES_TO_RADIANS (AngleDeltaDegrees);


	fl	=FaceList_Create(pTemplate->VerticalStrips + 1);
	
	geVec3d_Set (&OuterFocus, 0, (float)(pTemplate->Height/2), 0);
	geVec3d_Set (&StartPoint, (float)(pTemplate->Width/2), (float)-(pTemplate->Height/2), 0);

	CurPoint		=OldPoint	=StartPoint;
	BottomVerts		=(geVec3d *)geRam_Allocate (sizeof(geVec3d) * pTemplate->VerticalStrips);
	BottomVerts[0]	=CurPoint;

	CurAngle	=BottomCount	=0;
	for( index = 1; index < pTemplate->VerticalStrips; index++ )
	{
		//	Rotate around to create our successive points...
		CurAngle += AngleDelta;

		geVec3d_Set
		(
			&CurPoint,
			(float)(( StartPoint.X * cos( CurAngle ) ) +( StartPoint.Z * sin( CurAngle ) )),
			StartPoint.Y,
			(float)(( StartPoint.Z * cos( CurAngle ) ) -( StartPoint.X * sin( CurAngle ) ))
		);

		FaceVerts[2]	=OuterFocus;
		FaceVerts[1]	=OldPoint;
		FaceVerts[0]	=CurPoint;

		f	=Face_Create(3, FaceVerts, 0);
		FaceList_AddFace(fl, f);

		OldPoint		=CurPoint;

		//	Assign the current point to bottom plane...
		BottomVerts[++BottomCount]	=CurPoint;
	}

	//	Create the final polygon...
	CurAngle += AngleDelta;

	geVec3d_Set
	(
		&CurPoint,
		(float)(( StartPoint.X * cos( CurAngle ) ) + ( StartPoint.Z * sin( CurAngle ) )),
		StartPoint.Y,
		(float)(( StartPoint.Z * cos( CurAngle ) ) - ( StartPoint.X * sin( CurAngle ) ))
	);

	FaceVerts[2]	=OuterFocus;
	FaceVerts[1]	=OldPoint;
	FaceVerts[0]	=CurPoint;

	f	=Face_Create(3, FaceVerts, 0);
	if(f)
	{
		FaceList_AddFace(fl, f);
	}

	f	=Face_Create(pTemplate->VerticalStrips, BottomVerts, 0);

	if(f)
	{
		if(pTemplate->Style > 1)	//default to hollow (if they make hollow later)
		{
			Face_SetFixedHull(f, GE_TRUE);
		}
		FaceList_AddFace(fl, f);
	}
	geRam_Free(BottomVerts);

	if(!pTemplate->Style)
	{
		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetSubtract(b, pTemplate->TCut);
		}
		return	b;
	}
	else
	{
		BrushList	*bl	=BrushList_Create();
		Brush		*bh, *bm;

		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetHollow(b, GE_TRUE);
			Brush_SetHullSize(b, (float)pTemplate->Thickness);
			bh	=Brush_CreateHollowFromBrush(b);
			if(bh)
			{
				Brush_SetHollowCut(bh, GE_TRUE);
				BrushList_Append(bl, b);
				BrushList_Append(bl, bh);

				bm	=Brush_Create(BRUSH_MULTI, NULL, bl);
				if(bm)
				{
					Brush_SetHollow(bm, GE_TRUE);
					Brush_SetSubtract(bm, pTemplate->TCut);
					Brush_SetHullSize(bm, (float)pTemplate->Thickness);
					return	bm;
				}
			}
			else
			{
				Brush_Destroy(&b);
				BrushList_Destroy(&bl);
			}
		}
		else
		{
			BrushList_Destroy(&bl);
		}
	}
	return	NULL;
}

Brush *BrushTemplate_CreateCylinder (const BrushTemplate_Cylinder *pTemplate)
{
	double		CurrentXDiameter, CurrentZDiameter;
	double		DeltaXDiameter, DeltaZDiameter;
	double		CurrentXOffset, CurrentZOffset;
	double		DeltaXOffset, DeltaZOffset, sqrcheck;
	double		EllipseZ;
	int			NumVerticalBands, HBand, VBand;
	int			VertexCount=0;
	geVec3d		*Verts, *TopPoints;
	geVec3d		Current, Final, Delta;
	geXForm3d	YRotation;
	FaceList	*fl;
	Face		*f;
	Brush		*b;

	NumVerticalBands	= (int)(pTemplate->VerticalStripes);

	if (NumVerticalBands < 3)
	{
		return	NULL;
	}


	Verts		=(geVec3d *)geRam_Allocate(sizeof(geVec3d)*NumVerticalBands * 2);
	TopPoints	=(geVec3d *)geRam_Allocate(sizeof(geVec3d)*NumVerticalBands);
	fl			=FaceList_Create(NumVerticalBands + 2);

	if(!Verts || !TopPoints ||!fl)
	{
		return	NULL;
	}

	geXForm3d_SetIdentity(&YRotation);
	geXForm3d_SetYRotation(&YRotation, (M_PI * 2.0f)/(geFloat)NumVerticalBands);

	// Start with the top of cylinder
	CurrentXDiameter	=pTemplate->TopXSize;
	CurrentZDiameter	=pTemplate->TopZSize;
	DeltaXDiameter		=(pTemplate->BotXSize - pTemplate->TopXSize);
	DeltaZDiameter		=(pTemplate->BotZSize - pTemplate->TopZSize);
	
	// Get the offset amounts
	CurrentXOffset	=pTemplate->TopXOffset;
	CurrentZOffset	=pTemplate->TopZOffset;
	DeltaXOffset	=(pTemplate->BotXOffset - pTemplate->TopXOffset);
	DeltaZOffset	=(pTemplate->BotZOffset - pTemplate->TopZOffset);

	// Get the band positions and deltas
	geVec3d_Set(&Current, (float)(pTemplate->TopXSize / 2), (float)(pTemplate->YSize / 2), 0.0);
	geVec3d_Set(&Delta, (float)((pTemplate->BotXSize / 2) - Current.X), (float)(-(pTemplate->YSize/2) - Current.Y), 0.0);

	for(HBand = 0;HBand <= 1;HBand++)
	{
		Final = Current;
		for(VBand = 0;VBand < NumVerticalBands;VBand++)
		{
			// Get the elliptical Z value
			// (x^2/a^2) + (z^2/b^2) = 1
			// z = sqrt(b^2(1 - x^2/a^2))
			sqrcheck=( ((CurrentZDiameter/2)*(CurrentZDiameter/2))
				* (1.0 - (Final.X*Final.X)
				/ ( (CurrentXDiameter/2)*(CurrentXDiameter/2) )) );
			if(sqrcheck <0.0)
				sqrcheck=0.0;
			EllipseZ = sqrt(sqrcheck);

			// Check if we need to negate this thing
			if(VBand > (NumVerticalBands/2))
				EllipseZ = -EllipseZ;

			geVec3d_Set
			(
				&Verts[VertexCount],
				(float)(Final.X + CurrentXOffset),
				Final.Y,
				(float)(EllipseZ + CurrentZOffset)
			);
			VertexCount++;

			// Rotate the point around the Y to get the next vertical band
			geXForm3d_Rotate(&YRotation, &Final, &Final);
		}
		CurrentXDiameter	+=DeltaXDiameter;
		CurrentZDiameter	+=DeltaZDiameter;
		CurrentXOffset		+=DeltaXOffset;
		CurrentZOffset		+=DeltaZOffset;

		geVec3d_Add(&Current, &Delta, &Current);
	}

	for(VBand=0;VBand < NumVerticalBands;VBand++)
	{
		TopPoints[VBand]	=Verts[VBand];
	}
	f	=Face_Create(NumVerticalBands, TopPoints, 0);

	if(f)
	{
		if(pTemplate->Solid > 1)
		{
			Face_SetFixedHull(f, GE_TRUE);
		}
		FaceList_AddFace(fl, f);
	}

	for(VBand=NumVerticalBands-1, HBand=0;VBand >=0;VBand--, HBand++)
	{
		TopPoints[HBand]	=Verts[VBand + NumVerticalBands];
	}
	f	=Face_Create(NumVerticalBands, TopPoints, 0);

	if(f)
	{
		if(pTemplate->Solid > 1)
		{
			Face_SetFixedHull(f, GE_TRUE);
		}
		FaceList_AddFace(fl, f);
	}

	// Generate the polygons
	for(HBand = 0;HBand < 1;HBand++)
	{
		for(VBand = 0;VBand < NumVerticalBands;VBand++)
		{
			TopPoints[3]	=Verts[(HBand * NumVerticalBands) + VBand];
			TopPoints[2]	=Verts[(HBand * NumVerticalBands) + ((VBand + 1) % NumVerticalBands)];
			TopPoints[1]	=Verts[((HBand + 1) * NumVerticalBands) + ((VBand + 1) % NumVerticalBands)];
			TopPoints[0]	=Verts[((HBand + 1) * NumVerticalBands) + VBand];
			f				=Face_Create(4, TopPoints, 0);
			if(f)
			{
				FaceList_AddFace(fl, f);
			}
		}
	}
	geRam_Free(Verts);
	geRam_Free(TopPoints);

	if(!pTemplate->Solid)
	{
		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetSubtract(b, pTemplate->TCut);
		}
		return	b;
	}
	else
	{
		BrushList	*bl	=BrushList_Create();
		Brush		*bh, *bm;

		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetHollow(b, GE_TRUE);
			Brush_SetHullSize(b, (float)pTemplate->Thickness);
			bh	=Brush_CreateHollowFromBrush(b);
			if(bh)
			{
				Brush_SetHollowCut(bh, GE_TRUE);
				BrushList_Append(bl, b);
				BrushList_Append(bl, bh);

				bm	=Brush_Create(BRUSH_MULTI, NULL, bl);
				if(bm)
				{
					Brush_SetHollow(bm, GE_TRUE);
					Brush_SetSubtract(bm, pTemplate->TCut);
					Brush_SetHullSize(bm, (float)pTemplate->Thickness);
					return	bm;
				}
			}
			else
			{
				Brush_Destroy(&b);
				BrushList_Destroy(&bl);
			}
		}
		else
		{
			BrushList_Destroy(&bl);
		}
	}

	return	NULL;
}



Brush	*BrushTemplate_CreateSpheroid (const BrushTemplate_Spheroid *pTemplate)
{
	double		z, ring_radius, r, dz, t, dt;
	int			vcnt, HBand, VBand;
	geVec3d		*sv, FaceVerts[4];
	FaceList	*fl;
	Face		*f;
	Brush		*b;

	if((pTemplate->HorizontalBands < 2) || (pTemplate->VerticalBands < 3))
	{
		return GE_FALSE ;
	}
	
	fl			=FaceList_Create((pTemplate->HorizontalBands)* pTemplate->VerticalBands);
	sv			=(geVec3d *)geRam_Allocate(sizeof(geVec3d) * (((pTemplate->HorizontalBands-1) * pTemplate->VerticalBands)+2));
	r			=pTemplate->YSize;
	vcnt		=0;
	geVec3d_Set (&sv[vcnt], 0.0f, pTemplate->YSize, 0.0f);
	vcnt++;
	dz			=2.0*r/(double)(pTemplate->HorizontalBands-1);
	for(z=(-r)+dz/2.0; z<(r-dz/2.0+dz/4.0); z+=dz)
	{
		ring_radius	=sqrt(r*r - z*z);
		dt			=PI2 /(double)(pTemplate->VerticalBands);
		for(t=0.0;t < PI2-(dt*0.5);t+=dt)
		{
			sv[vcnt].X	=(float)(sin(t) * ring_radius);
			sv[vcnt].Z	=(float)(cos(t) * ring_radius);
			sv[vcnt++].Y=(float)(-z);
		}
	}
	sv[vcnt].X	=0.0f;
	sv[vcnt].Y	=(float)(-pTemplate->YSize);
	sv[vcnt++].Z=0.0f;

	for(VBand=0;VBand < pTemplate->VerticalBands;VBand++)
	{
		FaceVerts[0]	=sv[0];
		FaceVerts[1]	=sv[(((1 + VBand) % pTemplate->VerticalBands) + 1)];
		FaceVerts[2]	=sv[(VBand % pTemplate->VerticalBands)+1];

		f	=Face_Create(3, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}
	}

	for(HBand=1;HBand < (pTemplate->HorizontalBands-1);HBand++)
	{
		for(VBand=0;VBand < pTemplate->VerticalBands;VBand++)
		{
			FaceVerts[0]	=sv[(((HBand-1)*pTemplate->VerticalBands)+1)+VBand];
			FaceVerts[1]	=sv[(((HBand-1)*pTemplate->VerticalBands)+1)+((VBand+1)%pTemplate->VerticalBands)];
			FaceVerts[2]	=sv[((HBand*pTemplate->VerticalBands)+1)+((VBand+1)%pTemplate->VerticalBands)];
			FaceVerts[3]	=sv[((HBand*pTemplate->VerticalBands)+1)+VBand];

			f	=Face_Create(4, FaceVerts, 0);
			if(f)
			{
				FaceList_AddFace(fl, f);
			}
		}
	}

	for(VBand=0;VBand < pTemplate->VerticalBands;VBand++)
	{
		FaceVerts[0]	=sv[1+((pTemplate->HorizontalBands-2)*pTemplate->VerticalBands)+VBand];
		FaceVerts[1]	=sv[1+((pTemplate->HorizontalBands-2)*pTemplate->VerticalBands)+((VBand+1)%pTemplate->VerticalBands)];
		FaceVerts[2]	=sv[((pTemplate->HorizontalBands-1)*pTemplate->VerticalBands)+1];

		f	=Face_Create(3, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}
	}
	geRam_Free(sv);

	if(!pTemplate->Solid)
	{
		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetSubtract(b, pTemplate->TCut);
		}
		return	b;
	}
	else
	{
		BrushList	*bl	=BrushList_Create();
		Brush		*bh, *bm;

		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
		if(b)
		{
			Brush_SetHollow(b, GE_TRUE);
			Brush_SetHullSize(b, (float)pTemplate->Thickness);
			bh	=Brush_CreateHollowFromBrush(b);
			if(bh)
			{
				Brush_SetHollowCut(bh, GE_TRUE);
				BrushList_Append(bl, b);
				BrushList_Append(bl, bh);

				bm	=Brush_Create(BRUSH_MULTI, NULL, bl);
				if(bm)
				{
					Brush_SetHollow(bm, GE_TRUE);
					Brush_SetSubtract(bm, pTemplate->TCut);
					Brush_SetHullSize(bm, (float)pTemplate->Thickness);
					return	bm;
				}
			}
			else
			{
				Brush_Destroy(&b);
				BrushList_Destroy(&bl);
			}
		}
		else
		{
			BrushList_Destroy(&bl);
		}
	}
	return	NULL;
}




Brush	*BrushTemplate_CreateStaircase (const BrushTemplate_Staircase *pTemplate)
{
	int			i;
	geFloat		HalfWidth = (geFloat)(pTemplate->Width/2);
	geFloat		HalfHeight = (geFloat)(pTemplate->Height/2);
	geFloat		HalfLength = (geFloat)(pTemplate->Length/2);
	Brush		*b, *b2;
	BrushList	*MBList	=BrushList_Create();
	FaceList	*fl;
	Face		*f;
	geVec3d		v, FaceVerts[4];


	if( pTemplate->MakeRamp )
	{
		fl	=FaceList_Create(5);

		geVec3d_Set (&(FaceVerts[3]), -HalfWidth, -HalfHeight, HalfLength);
		geVec3d_Set (&(FaceVerts[2]), HalfWidth,  -HalfHeight, HalfLength);
		geVec3d_Set (&(FaceVerts[1]), HalfWidth,	 HalfHeight,  HalfLength);
		geVec3d_Set (&(FaceVerts[0]), -HalfWidth, HalfHeight,  HalfLength);
		f	=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		geVec3d_Set (&(FaceVerts[3]), HalfWidth,  -HalfHeight,  HalfLength);
		geVec3d_Set (&(FaceVerts[2]), -HalfWidth, -HalfHeight,  HalfLength);
		geVec3d_Set (&(FaceVerts[1]), -HalfWidth, -HalfHeight,  -HalfLength);
		geVec3d_Set (&(FaceVerts[0]), HalfWidth,  -HalfHeight,  -HalfLength);
		f	=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		geVec3d_Set (&(FaceVerts[3]), -HalfWidth, HalfHeight,   HalfLength);
		geVec3d_Set (&(FaceVerts[2]), HalfWidth,  HalfHeight,   HalfLength);
		geVec3d_Set (&(FaceVerts[1]), HalfWidth,  -HalfHeight,  -HalfLength);
		geVec3d_Set (&(FaceVerts[0]), -HalfWidth, -HalfHeight,  -HalfLength);
		f	=Face_Create(4, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		geVec3d_Set (&(FaceVerts[2]), HalfWidth,  HalfHeight,   HalfLength);
		geVec3d_Set (&(FaceVerts[1]), HalfWidth,  -HalfHeight,  HalfLength);
		geVec3d_Set (&(FaceVerts[0]), HalfWidth,  -HalfHeight,  -HalfLength);
		f	=Face_Create(3, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		geVec3d_Set (&(FaceVerts[0]), -HalfWidth, HalfHeight,   HalfLength);
		geVec3d_Set (&(FaceVerts[1]), -HalfWidth, -HalfHeight,  HalfLength);
		geVec3d_Set (&(FaceVerts[2]), -HalfWidth, -HalfHeight,  -HalfLength);
		f	=Face_Create(3, FaceVerts, 0);
		if(f)
		{
			FaceList_AddFace(fl, f);
		}

		b	=Brush_Create(BRUSH_LEAF, fl, NULL);
	}
	else
	{
		float	StairYSize	=(geFloat)pTemplate->Height/(geFloat)pTemplate->NumberOfStairs;
		float	DZ			=(geFloat)pTemplate->Length/(geFloat)pTemplate->NumberOfStairs;
		float	ZSize		=(geFloat)pTemplate->Length;
		BrushTemplate_Box BoxTemplate;

		BoxTemplate.Solid = 0;
		BoxTemplate.TCut = pTemplate->TCut;
		BoxTemplate.Thickness = 0.0f;
		BoxTemplate.XSizeTop = pTemplate->Width;
		BoxTemplate.XSizeBot = pTemplate->Width;
		BoxTemplate.YSize = StairYSize;

		for(i=0;i < pTemplate->NumberOfStairs;i++)
		{
			BoxTemplate.ZSizeTop = ZSize;
			BoxTemplate.ZSizeBot = ZSize;
			BoxTemplate.TSheet	 = GE_FALSE;	// nasty, nasty, nasty
			b2 = BrushTemplate_CreateBox (&BoxTemplate);
			ZSize	-=DZ;
			geVec3d_Set(&v, 0.0f, i*StairYSize, (i*DZ)/2);
			Brush_Move(b2, &v);
			BrushList_Append(MBList, b2);
		}
		b	=Brush_Create(BRUSH_MULTI, NULL, MBList);
	}

	if(b)
	{
		Brush_SetSubtract(b, pTemplate->TCut);
	}

	return	b;
}


void BrushTemplate_ArchDefaults (BrushTemplate_Arch *pArchTemplate)
{
	pArchTemplate->NumSlits		= 3;
	pArchTemplate->Thickness	= 150;
	pArchTemplate->Width		= 100;
	pArchTemplate->Radius		= 200;
	pArchTemplate->WallSize		= 16;
	pArchTemplate->Style		= 0;
	pArchTemplate->EndAngle		= 180.0f;
	pArchTemplate->StartAngle	= 0.0f;
	pArchTemplate->TCut			= GE_FALSE;
}

geBoolean BrushTemplate_WriteArch (const BrushTemplate_Arch *pArchTemplate, FILE *f)
{
	if (fprintf (f, "%s\n", "ArchTemplate") < 0) return GE_FALSE;
	if (fprintf (f, "NumSlits %d\n", pArchTemplate->NumSlits) < 0) return GE_FALSE;
	if (fprintf (f, "Thickness %f\n", pArchTemplate->Thickness) < 0) return GE_FALSE;
	if (fprintf (f, "Width %f\n", pArchTemplate->Width) < 0) return GE_FALSE;
	if (fprintf (f, "Radius %f\n", pArchTemplate->Radius) < 0) return GE_FALSE;
	if (fprintf (f, "WallSize %f\n", pArchTemplate->WallSize) < 0) return GE_FALSE;
	if (fprintf (f, "Style %d\n", pArchTemplate->Style) < 0) return GE_FALSE;
	if (fprintf (f, "EndAngle %f\n", pArchTemplate->EndAngle) < 0) return GE_FALSE;
	if (fprintf (f, "StartAngle %f\n", pArchTemplate->StartAngle) < 0) return GE_FALSE;
	if (fprintf (f, "TCut %d\n", pArchTemplate->TCut) < 0) return GE_FALSE;

	return GE_TRUE;
}

#pragma warning (disable:4100)
geBoolean BrushTemplate_LoadArch 
	(
	  BrushTemplate_Arch *pArchTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "ArchTemplate"))) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "NumSlits"), &pArchTemplate->NumSlits)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Thickness"), &pArchTemplate->Thickness)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Width"), &pArchTemplate->Width)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Radius"), &pArchTemplate->Radius)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "WallSize"), &pArchTemplate->WallSize)) return GE_FALSE;
	if (pArchTemplate->WallSize < 1.0f)
	{
		pArchTemplate->WallSize = 1.0f;
	}
	if (!Parse3dt_GetInt (Parser, (*Expected = "Style"), &pArchTemplate->Style)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "EndAngle"), &pArchTemplate->EndAngle)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "StartAngle"), &pArchTemplate->StartAngle)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "TCut"), &pArchTemplate->TCut)) return GE_FALSE;

	return GE_TRUE;
}
#pragma warning (default:4100)


void BrushTemplate_BoxDefaults (BrushTemplate_Box *pBoxTemplate)
{
	pBoxTemplate->Solid		= 1;		// hollow
	pBoxTemplate->TCut		= GE_FALSE;
	// emperically derived default sizes
	pBoxTemplate->Thickness = 16.0f;
	pBoxTemplate->XSizeBot	= 680.0f;
	pBoxTemplate->XSizeTop	= 680.0f;
	pBoxTemplate->YSize		= 360.0f;
	pBoxTemplate->ZSizeBot	= 560.0f;
	pBoxTemplate->ZSizeTop	= 560.0f;
}

geBoolean BrushTemplate_WriteBox (const BrushTemplate_Box *pBoxTemplate, FILE *f)
{
	if (fprintf (f, "%s\n", "BoxTemplate") < 0) return GE_FALSE;
	if (fprintf (f, "Solid %d\n", pBoxTemplate->Solid) < 0) return GE_FALSE;
	if (fprintf (f, "TCut %d\n", pBoxTemplate->TCut) < 0) return GE_FALSE;
	if (fprintf (f, "TSheet %d\n", pBoxTemplate->TSheet) < 0) return GE_FALSE;
	if (fprintf (f, "Thickness %f\n", pBoxTemplate->Thickness) < 0) return GE_FALSE;
	if (fprintf (f, "XSizeBot %f\n", pBoxTemplate->XSizeBot) < 0) return GE_FALSE;
	if (fprintf (f, "XSizeTop %f\n", pBoxTemplate->XSizeTop) < 0) return GE_FALSE;
	if (fprintf (f, "YSize %f\n", pBoxTemplate->YSize) < 0) return GE_FALSE;
	if (fprintf (f, "ZSizeBot %f\n", pBoxTemplate->ZSizeBot) < 0) return GE_FALSE;
	if (fprintf (f, "ZSizeTop %f\n", pBoxTemplate->ZSizeTop) < 0) return GE_FALSE;

	return GE_TRUE;
}


geBoolean BrushTemplate_LoadBox
	(
	  BrushTemplate_Box *pBoxTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "BoxTemplate"))) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Solid"), &pBoxTemplate->Solid)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "TCut"), &pBoxTemplate->TCut)) return GE_FALSE;
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 29)))
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "TSheet"), &pBoxTemplate->TSheet)) return GE_FALSE;
	}
	else
	{
		pBoxTemplate->TSheet = 0;
	}
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Thickness"), &pBoxTemplate->Thickness)) return GE_FALSE;
	if (pBoxTemplate->Thickness < 1.0f)
	{
		pBoxTemplate->Thickness = 1.0f;
	}
	if (!Parse3dt_GetFloat (Parser, (*Expected = "XSizeBot"), &pBoxTemplate->XSizeBot)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "XSizeTop"), &pBoxTemplate->XSizeTop)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "YSize"), &pBoxTemplate->YSize)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "ZSizeBot"), &pBoxTemplate->ZSizeBot)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "ZSizeTop"), &pBoxTemplate->ZSizeTop)) return GE_FALSE;

	return GE_TRUE;
}


void BrushTemplate_ConeDefaults (BrushTemplate_Cone *pConeTemplate)
{
	pConeTemplate->Style	= 0;
	pConeTemplate->Width	= 200;
	pConeTemplate->Height	= 300;
	pConeTemplate->VerticalStrips = 4;
	pConeTemplate->Thickness = 16;
	pConeTemplate->TCut		= GE_FALSE;
}

geBoolean BrushTemplate_WriteCone (const BrushTemplate_Cone *pConeTemplate, FILE *f)
{
	if (fprintf (f, "%s\n", "ConeTemplate") < 0) return GE_FALSE;
	if (fprintf (f, "Style %d\n", pConeTemplate->Style) < 0) return GE_FALSE;
	if (fprintf (f, "Width %f\n", pConeTemplate->Width) < 0) return GE_FALSE;
	if (fprintf (f, "Height %f\n", pConeTemplate->Height) < 0) return GE_FALSE;
	if (fprintf (f, "VerticalStrips %d\n", pConeTemplate->VerticalStrips) < 0) return GE_FALSE;
	if (fprintf (f, "Thickness %f\n", pConeTemplate->Thickness) < 0) return GE_FALSE;
	if (fprintf (f, "TCut %d\n", pConeTemplate->TCut) < 0) return GE_FALSE;

	return GE_TRUE;
}


#pragma warning (disable:4100)
geBoolean BrushTemplate_LoadCone
	(
	  BrushTemplate_Cone *pConeTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "ConeTemplate"))) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Style"), &pConeTemplate->Style)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Width"), &pConeTemplate->Width)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Height"), &pConeTemplate->Height)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "VerticalStrips"), &pConeTemplate->VerticalStrips)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Thickness"), &pConeTemplate->Thickness)) return GE_FALSE;
	if (pConeTemplate->Thickness < 1.0f)
	{
		pConeTemplate->Thickness = 1.0f;
	}
	if (!Parse3dt_GetInt (Parser, (*Expected = "TCut"), &pConeTemplate->TCut)) return GE_FALSE;
	return GE_TRUE;
}
#pragma warning (default:4100)

void BrushTemplate_CylinderDefaults (BrushTemplate_Cylinder *pCylinderTemplate)
{
	pCylinderTemplate->BotXOffset	= 0.0;
	pCylinderTemplate->BotXSize		= 128.0;
	pCylinderTemplate->BotZOffset	= 0.0;
	pCylinderTemplate->BotZSize		= 128.0;
	pCylinderTemplate->Solid		= 0;
	pCylinderTemplate->Thickness	= 16.0;
	pCylinderTemplate->TopXOffset	= 0.0;
	pCylinderTemplate->TopXSize		= 128.0;
	pCylinderTemplate->TopZOffset	= 0.0;
	pCylinderTemplate->TopZSize		= 128.0;
	pCylinderTemplate->VerticalStripes = 6;
	pCylinderTemplate->YSize		= 512.0;
	pCylinderTemplate->RingLength	= 0.0;
	pCylinderTemplate->TCut			= GE_FALSE;
}

geBoolean BrushTemplate_WriteCylinder (const BrushTemplate_Cylinder *pCylinderTemplate, FILE *f)
{
	if (fprintf (f, "%s\n", "CylinderTemplate") < 0) return GE_FALSE;
	if (fprintf (f, "BotXOffset %f\n", pCylinderTemplate->BotXOffset) < 0) return GE_FALSE;
	if (fprintf (f, "BotXSize %f\n", pCylinderTemplate->BotXSize) < 0) return GE_FALSE;
	if (fprintf (f, "BotZOffset %f\n", pCylinderTemplate->BotZOffset) < 0) return GE_FALSE;
	if (fprintf (f, "BotZSize %f\n", pCylinderTemplate->BotZSize) < 0) return GE_FALSE;
	if (fprintf (f, "Solid %d\n", pCylinderTemplate->Solid) < 0) return GE_FALSE;
	if (fprintf (f, "Thickness %f\n", pCylinderTemplate->Thickness) < 0) return GE_FALSE;
	if (fprintf (f, "TopXOffset %f\n", pCylinderTemplate->TopXOffset) < 0) return GE_FALSE;
	if (fprintf (f, "TopXSize %f\n", pCylinderTemplate->TopXSize) < 0) return GE_FALSE;
	if (fprintf (f, "TopZOffset %f\n", pCylinderTemplate->TopZOffset) < 0) return GE_FALSE;
	if (fprintf (f, "TopZSize %f\n", pCylinderTemplate->TopZSize) < 0) return GE_FALSE;
	if (fprintf (f, "VerticalStripes %d\n", pCylinderTemplate->VerticalStripes) < 0) return GE_FALSE;
	if (fprintf (f, "YSize %f\n", pCylinderTemplate->YSize) < 0) return GE_FALSE;
	if (fprintf (f, "RingLength %f\n", pCylinderTemplate->RingLength) < 0) return GE_FALSE;
	if (fprintf (f, "TCut %d\n", pCylinderTemplate->TCut) < 0) return GE_FALSE;
	return GE_TRUE;
}


#pragma warning (disable:4100)
geBoolean BrushTemplate_LoadCylinder
	(
	  BrushTemplate_Cylinder *pCylinderTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "CylinderTemplate"))) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "BotXOffset"), &pCylinderTemplate->BotXOffset)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "BotXSize"), &pCylinderTemplate->BotXSize)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "BotZOffset"), &pCylinderTemplate->BotZOffset)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "BotZSize"), &pCylinderTemplate->BotZSize)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Solid"), &pCylinderTemplate->Solid)) return GE_FALSE;
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor > 22)))
	{
		if (!Parse3dt_GetFloat (Parser, (*Expected = "Thickness"), &pCylinderTemplate->Thickness)) return GE_FALSE;
		if (pCylinderTemplate->Thickness < 1.0f)
		{
			pCylinderTemplate->Thickness = 1.0f;
		}
	}
	if (!Parse3dt_GetFloat (Parser, (*Expected = "TopXOffset"), &pCylinderTemplate->TopXOffset)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "TopXSize"), &pCylinderTemplate->TopXSize)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "TopZOffset"), &pCylinderTemplate->TopZOffset)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "TopZSize"), &pCylinderTemplate->TopZSize)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "VerticalStripes"), &pCylinderTemplate->VerticalStripes)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "YSize"), &pCylinderTemplate->YSize)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "RingLength"), &pCylinderTemplate->RingLength)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "TCut"), &pCylinderTemplate->TCut)) return GE_FALSE;

	return GE_TRUE;
}

#pragma warning (default:4100)

void BrushTemplate_SpheroidDefaults (BrushTemplate_Spheroid *pSpheroidTemplate)
{
	pSpheroidTemplate->HorizontalBands	= 4;
	pSpheroidTemplate->VerticalBands	= 8;
	pSpheroidTemplate->YSize			= 256.0;
	pSpheroidTemplate->Solid			= 0;
	pSpheroidTemplate->Thickness		= 16;
	pSpheroidTemplate->TCut				= GE_FALSE;
}

geBoolean BrushTemplate_WriteSpheroid (const BrushTemplate_Spheroid *pSpheroidTemplate, FILE *f)
{
	if (fprintf (f, "%s\n", "SpheroidTemplate") < 0) return GE_FALSE;
	if (fprintf (f, "HorizontalBands %d\n", pSpheroidTemplate->HorizontalBands) < 0) return GE_FALSE;
	if (fprintf (f, "VerticalBands %d\n", pSpheroidTemplate->VerticalBands) < 0) return GE_FALSE;
	if (fprintf (f, "YSize %f\n", pSpheroidTemplate->YSize) < 0) return GE_FALSE;
	if (fprintf (f, "Solid %d\n", pSpheroidTemplate->Solid) < 0) return GE_FALSE;
	if (fprintf (f, "Thickness %f\n", pSpheroidTemplate->Thickness) < 0) return GE_FALSE;
	if (fprintf (f, "TCut %d\n", pSpheroidTemplate->TCut) < 0) return GE_FALSE;

	return GE_TRUE;
}


#pragma warning (disable:4100)
geBoolean BrushTemplate_LoadSpheroid
	(
	  BrushTemplate_Spheroid *pSpheroidTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "SpheroidTemplate"))) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "HorizontalBands"), &pSpheroidTemplate->HorizontalBands)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "VerticalBands"), &pSpheroidTemplate->VerticalBands)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "YSize"), &pSpheroidTemplate->YSize)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "Solid"), &pSpheroidTemplate->Solid)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Thickness"), &pSpheroidTemplate->Thickness)) return GE_FALSE;
	if (pSpheroidTemplate->Thickness < 1.0f)
	{
		pSpheroidTemplate->Thickness = 1.0f;
	}
	if (!Parse3dt_GetInt (Parser, (*Expected = "TCut"), &pSpheroidTemplate->TCut)) return GE_FALSE;

	return GE_TRUE;
}
#pragma warning (default:4100)

void BrushTemplate_StaircaseDefaults (BrushTemplate_Staircase *pStaircaseTemplate)
{
	pStaircaseTemplate->Height		= 128.0;
	pStaircaseTemplate->Length		= 128.0;
	pStaircaseTemplate->NumberOfStairs = 8;
	pStaircaseTemplate->Width		= 64.0;
	pStaircaseTemplate->MakeRamp	= GE_FALSE;
	pStaircaseTemplate->TCut		= GE_FALSE;
}

geBoolean BrushTemplate_WriteStaircase (const BrushTemplate_Staircase *pStaircaseTemplate, FILE *f)
{
	if (fprintf (f, "%s\n", "StaircaseTemplate") < 0) return GE_FALSE;
	if (fprintf (f, "Height %f\n", pStaircaseTemplate->Height) < 0) return GE_FALSE;
	if (fprintf (f, "Length %f\n", pStaircaseTemplate->Length) < 0) return GE_FALSE;
	if (fprintf (f, "NumberOfStairs %d\n", pStaircaseTemplate->NumberOfStairs) < 0) return GE_FALSE;
	if (fprintf (f, "Width %f\n", pStaircaseTemplate->Width) < 0) return GE_FALSE;
	if (fprintf (f, "MakeRamp %d\n", pStaircaseTemplate->MakeRamp) < 0) return GE_FALSE;
	if (fprintf (f, "TCut %d\n", pStaircaseTemplate->TCut) < 0) return GE_FALSE;

	return GE_TRUE;
}

#pragma warning (disable:4100)
geBoolean BrushTemplate_LoadStaircase
	(
	  BrushTemplate_Staircase *pStaircaseTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	if (!Parse3dt_ScanExpectingText (Parser, (*Expected = "StaircaseTemplate"))) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Height"), &pStaircaseTemplate->Height)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Length"), &pStaircaseTemplate->Length)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "NumberOfStairs"), &pStaircaseTemplate->NumberOfStairs)) return GE_FALSE;
	if (!Parse3dt_GetFloat (Parser, (*Expected = "Width"), &pStaircaseTemplate->Width)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "MakeRamp"), &pStaircaseTemplate->MakeRamp)) return GE_FALSE;
	if (!Parse3dt_GetInt (Parser, (*Expected = "TCut"), &pStaircaseTemplate->TCut)) return GE_FALSE;
	return GE_TRUE;
}
#pragma warning (default:4100)
