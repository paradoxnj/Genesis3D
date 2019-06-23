/****************************************************************************************/
/*  Texture.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: This code keeps a list of shared textures.                             */
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
#ifndef TEXTURE_H
#define TEXTURE_H

//#include <Windows.h>

#include <Stdio.h>
#include "Map.h"
#include "DCommon.h"
#include "GBSPFile.h"
#include "VFile.h"

#define TEXTURE_SKYBOX		(1<<0)

// Don't let this naming convention full you.  TEXINFO is just everything about a face, that could be
// shared with another face or more...
#define TEXINFO_MIRROR		(1<<0)					// This face is a mirror face
#define TEXINFO_FULLBRIGHT	(1<<1)					// This face is fully bright
#define TEXINFO_SKY			(1<<2)					// This face is a sky portal
#define TEXINFO_LIGHT		(1<<3)					// This face emits light
#define TEXINFO_TRANS		(1<<4)					// A hint to the engine, that this face is not solid 
#define TEXINFO_GOURAUD		(1<<5)					// This face is gouraud shaded
#define TEXINFO_FLAT		(1<<6)					// This face is flat shaded
#define TEXINFO_NO_LIGHTMAP	(1<<15)					// This face does not have a lightmap

geBoolean InitTextureLib(char *File);
void ShutdownTextureLib(void);
geBoolean GetTexture(char *Name, uint8 *Data, int32 *Size, int32 *Width, int32 *Height, geVFile *f);
geBoolean GetTexturePalette(const char *Name, DRV_Palette Palette);

extern int32		NumTextures;
extern GFX_Texture	Textures[MAX_MAP_TEXTURES];
extern int32		NumTexInfo;
extern GFX_TexInfo	TexInfo[MAX_MAP_TEXINFO];

int32 FindTextureIndex(char *Name, uint32 Flags);
int32 FindTexInfo(GFX_TexInfo *Tex2);

#endif
