/****************************************************************************************/
/*  Vis.h                                                                               */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef VIS_H
#define VIS_H

#include <Windows.h>
#include <Stdio.h>

#include "BSP.h"
#include "GBSPFile.h"


typedef struct _VIS_Portal
{
	_VIS_Portal	*Next;					// Next portal in list

	GBSP_Poly	*Poly;					// Convex poly shape of portal
	GBSP_Plane	Plane;					// Plane of portal
	geVec3d		Center;					// Center of portal
	float		Radius;					// Radius of portal (Fromt Center to a vert)

	uint8		*VisBits;				// What leafs portal might can see (byte compressed)
	uint8		*FinalVisBits;			// What leafs portal really can see (byte compressed)
	int32		Leaf;					// What leaf portal is looking directly into
	int32		MightSee;
	int32		CanSee;
	geBoolean	Done;

} VIS_Portal, *pVIS_Portal;

typedef struct
{
	VIS_Portal	*Portals;				// List of portals for this leaf
	int32		MightSee;
	int32		CanSee;
} VIS_Leaf;


#define MAX_TEMP_PORTALS		25000

typedef struct
{
	uint8		VisBits[MAX_TEMP_PORTALS/8];
	GBSP_Poly	*Source;
	GBSP_Poly	*Pass;
} VIS_PStack;

geBoolean VisGBSPFile(char *FileName, VisParms *Parms);

void CleanupVis(void);

geBoolean VisAllLeafs(void);
geBoolean CollectLeafVisBits(int32 LeafNum);
void FloodPortalsFast_r(VIS_Portal *SrcPortal, VIS_Portal *DestPortal);
void FloodLeafPortalsFast(int32 LeafNum);
geBoolean PortalCanSeePortal(VIS_Portal *Portal1, VIS_Portal *Portal2);
geBoolean LoadPortalFile(char *FileName);

geBoolean FloodLeafPortalsSlow(int32 LeafNum);
geBoolean FloodPortalsSlow(void);


#endif
