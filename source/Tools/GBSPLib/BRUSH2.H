/****************************************************************************************/
/*  Brush2.h                                                                            */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef BRUSH2_H
#define BRUSH2_H

#include <Windows.h>

#include "Mathlib.h"
#include "Map.h"

#include "VFile.h"
#include "Vec3d.h"

#define	NUM_BRUSH_DEFAULT_SIDES		6

typedef struct MAP_Brush
{
	MAP_Brush	*Next;

	int32		EntityNum;
	int32		BrushNum;

	int32		Contents;

	geVec3d		Mins, Maxs;

	int32		OrderID;

	int32		NumSides;
	GBSP_Side	OriginalSides[NUM_BRUSH_DEFAULT_SIDES];	
} MAP_Brush;

typedef struct GBSP_Brush
{
	GBSP_Brush	*Next;
	geVec3d		Mins, Maxs;
	int32		Side, TestSide;	
	MAP_Brush	*Original;
	int32		NumSides;
	GBSP_Side	Sides[NUM_BRUSH_DEFAULT_SIDES];

} GBSP_Brush;

extern int32	gTotalBrushes;
extern int32	gPeekBrushes;

MAP_Brush	*AllocMapBrush(int32 NumSides);
void		FreeMapBrush(MAP_Brush *Brush);
MAP_Brush	*LoadMapBrush(geVFile *VFile);
void		FreeMapBrushList(MAP_Brush *Brushes);
geBoolean	MakeMapBrushPolys(MAP_Brush *ob);
MAP_Brush	*CopyMapBrush(MAP_Brush *Brush);
int32		CountMapBrushList(MAP_Brush *Brushes);

GBSP_Brush	*AllocBrush(int32 NumSides);
void		FreeBrush(GBSP_Brush *Brush);
void		FreeBrushList(GBSP_Brush *Brushes);
void		ShowBrushHeap(void);
GBSP_Brush	*CopyBrush(GBSP_Brush *Brush);
void		BoundBrush(GBSP_Brush *Brush);
GBSP_Brush	*AddBrushListToTail(GBSP_Brush *List, GBSP_Brush *Tail);
int32		CountBrushList(GBSP_Brush *Brushes);
GBSP_Brush	*RemoveBrushList(GBSP_Brush *List, GBSP_Brush *Remove);
geFloat		BrushVolume (GBSP_Brush *Brush);
void		CreateBrushPolys(GBSP_Brush *Brush);
GBSP_Brush	*BrushFromBounds (geVec3d *Mins, geVec3d *Maxs);
geBoolean	CheckBrush(GBSP_Brush *Brush);
void		SplitBrush(GBSP_Brush *Brush, int32 PNum, int32 PSide, uint8 MidFlags, geBoolean Visible, GBSP_Brush **Front, GBSP_Brush **Back);
GBSP_Brush	*SubtractBrush(GBSP_Brush *a, GBSP_Brush *b);
GBSP_Brush	*CSGBrushes(GBSP_Brush *Head);

geBoolean	OutputBrushes(char *FileName, GBSP_Brush *Brushes);
geBoolean	OutputMapBrushes(char *FileName, MAP_Brush *Brushes);

#endif