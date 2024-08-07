/****************************************************************************************/
/*  PtrTypes.c                                                                          */
/*                                                                                      */
/*  Description: File to resolve interdependency problems                               */
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

#ifndef GE_PTRTYPES_H
#define GE_PTRTYPES_H

#include "BASETYPE.H"

// System.h
typedef struct	geEngine			geEngine;

// LIGHT.H
typedef struct	Light_LightInfo		Light_LightInfo;

//	SURFACE.H
typedef	struct	Surf_SurfInfo		Surf_SurfInfo;
typedef	struct	Surf_TexVert		Surf_TexVert;

// WORLD.H
typedef	struct	geWorld				geWorld;

// FRUSTUM.H
typedef	struct	Frustum_Info		Frustum_Info;

// WORLD.H
typedef struct	World_BSP			World_BSP;
typedef struct	geWorld_Leaf		geWorld_Leaf;

				
// Mesh.h
typedef struct	Mesh_MeshInfo		Mesh_MeshInfo;
typedef struct	Mesh_MeshDef		Mesh_MeshDef;
typedef struct	Mesh_RenderQ		Mesh_RenderQ;

// Entities.h
typedef struct	geEntity_EntitySet geEntity_EntitySet;

// USER.H
typedef struct	User_Info		User_Info;
typedef struct  gePoly			gePoly;


#endif
