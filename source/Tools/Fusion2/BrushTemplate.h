/****************************************************************************************/
/*  BrushTemplate.h                                                                     */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Genesis world editor header file                                      */
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
#ifndef BRUSHTEMPLATE_H
#define BRUSHTEMPLATE_H

#include "basetype.h"
#include "brush.h"

#ifdef __cplusplus
	extern "C" {
#endif


typedef struct
{
	int			NumSlits;
	geFloat		Thickness;
	geFloat		Width;
	geFloat		Radius;
	geFloat		WallSize;
	int			Style;
	geFloat		EndAngle;
	geFloat		StartAngle;
	geBoolean	TCut;
} BrushTemplate_Arch;


typedef struct
{
	int			Solid;
	geBoolean	TCut;
	geBoolean	TSheet;
	geFloat		Thickness;
	geFloat		XSizeTop;
	geFloat		XSizeBot;
	geFloat		YSize;
	geFloat		ZSizeTop;
	geFloat		ZSizeBot;
} BrushTemplate_Box;

typedef struct
{
	int			Style;
	geFloat		Width;
	geFloat		Height;
	int			VerticalStrips;
	geFloat		Thickness;
	geBoolean	TCut;
} BrushTemplate_Cone;

typedef struct
{
	geFloat		BotXOffset;
	geFloat		BotXSize;
	geFloat		BotZOffset;
	geFloat		BotZSize;
	int			Solid;
	geFloat		Thickness;
	geFloat		TopXOffset;
	geFloat		TopXSize;
	geFloat		TopZOffset;
	geFloat		TopZSize;
	int			VerticalStripes;
	geFloat		YSize;
	geFloat		RingLength;
	geBoolean	TCut;
} BrushTemplate_Cylinder;

typedef struct
{
	int			HorizontalBands;
	int			VerticalBands;
	geFloat		YSize;
	int			Solid;
	geFloat		Thickness;
	geBoolean	TCut;
} BrushTemplate_Spheroid;

typedef struct
{
	geFloat		Height;
	geFloat		Length;
	int			NumberOfStairs;
	geFloat		Width;
	geBoolean	MakeRamp;
	geBoolean	TCut;
} BrushTemplate_Staircase;


Brush *BrushTemplate_CreateArch      (const BrushTemplate_Arch *Template);
Brush *BrushTemplate_CreateBox		 (const BrushTemplate_Box *Template);
Brush *BrushTemplate_CreateCone		 (const BrushTemplate_Cone *Template);
Brush *BrushTemplate_CreateCylinder  (const BrushTemplate_Cylinder *Template);
Brush *BrushTemplate_CreateSpheroid  (const BrushTemplate_Spheroid *Template);
Brush *BrushTemplate_CreateStaircase (const BrushTemplate_Staircase *Template);

void BrushTemplate_ArchDefaults (BrushTemplate_Arch *pArchTemplate);
geBoolean BrushTemplate_WriteArch (const BrushTemplate_Arch *pArchTemplate, FILE *f);

geBoolean BrushTemplate_LoadArch 
	(
	  BrushTemplate_Arch *pArchTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	);

void BrushTemplate_BoxDefaults (BrushTemplate_Box *pBoxTemplate);
geBoolean BrushTemplate_WriteBox (const BrushTemplate_Box *pBoxTemplate, FILE *f);

geBoolean BrushTemplate_LoadBox
	(
	  BrushTemplate_Box *pBoxTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	);

void BrushTemplate_ConeDefaults (BrushTemplate_Cone *pConeTemplate);
geBoolean BrushTemplate_WriteCone (const BrushTemplate_Cone *pConeTemplate, FILE *f);

geBoolean BrushTemplate_LoadCone
	(
	  BrushTemplate_Cone *pConeTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	);

void BrushTemplate_CylinderDefaults (BrushTemplate_Cylinder *pCylinderTemplate);
geBoolean BrushTemplate_WriteCylinder (const BrushTemplate_Cylinder *pCylinderTemplate, FILE *f);

geBoolean BrushTemplate_LoadCylinder
	(
	  BrushTemplate_Cylinder *pCylinderTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	);

void BrushTemplate_SpheroidDefaults (BrushTemplate_Spheroid *pSpheroidTemplate);
geBoolean BrushTemplate_WriteSpheroid (const BrushTemplate_Spheroid *pSpheroidTemplate, FILE *f);

geBoolean BrushTemplate_LoadSpheroid
	(
	  BrushTemplate_Spheroid *pSpheroidTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	);

void BrushTemplate_StaircaseDefaults (BrushTemplate_Staircase *pStaircaseTemplate);
geBoolean BrushTemplate_WriteStaircase (const BrushTemplate_Staircase *pStaircaseTemplate, FILE *f);

geBoolean BrushTemplate_LoadStaircase
	(
	  BrushTemplate_Staircase *pStaircaseTemplate,
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	);


#ifdef __cplusplus
	}
#endif

#endif
