/****************************************************************************************/
/*  Portals.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Creates and manages portals (passages from leaf-to-leaf)               */
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
#ifndef PORTALS_H
#define PORTALS_H

#include <Windows.h>

#include "BSP.h"

extern GBSP_Node	*OutsideNode;				// Current outside node being used

geBoolean CreatePortals(GBSP_Node *RootNode, GBSP_Model *Model, geBoolean Vis);
geBoolean FreePortals(GBSP_Node *RootNode);

geBoolean GetLeafBBoxFromPortals(GBSP_Node *Node, geVec3d *Mins, geVec3d *Maxs);

geBoolean PartitionPortals_r(GBSP_Node *Node);

// This function actually lives in PortFile.cpp
geBoolean SavePortalFile(GBSP_Model *Model, char *FileName);

#endif
