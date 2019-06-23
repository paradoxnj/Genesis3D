/****************************************************************************************/
/*  Leaf.h                                                                              */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates leaf collision hulls, areas, etc...                            */
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
#ifndef LEAF_H
#define LEAF_H

#include <Windows.h>

#include "BSP.h"

#define MAX_LEAF_SIDES	64000*2


extern int32				NumLeafClusters;
extern int32				NumLeafSides;
extern int32				NumLeafBevels;

extern GBSP_LeafSide	LeafSides[MAX_LEAF_SIDES];

geBoolean CreateLeafClusters(GBSP_Node *RootNode);
geBoolean CreateLeafSides(GBSP_Node *RootNode);
geBoolean CreateAreas(GBSP_Node *RootNode);
GBSP_Node *FindLeaf(GBSP_Node *Node, geVec3d *Origin);

#endif