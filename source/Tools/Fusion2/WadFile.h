/****************************************************************************************/
/*  WadFile.h                                                                           */
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
#ifndef WADFILE_H
#define WADFILE_H

#include "bitmap.h"

struct WadFileEntry
{
	geBitmap *bmp;
	char *Name;
	int Width, Height;
	geBitmap *LockedBitmap;
	void *BitsPtr;
};

class CWadFile 
{
public:
	int			mBitmapCount;

	WadFileEntry *mBitmaps;

	CWadFile();
	~CWadFile();
	geBoolean Setup(const char *Filename);
private:
	void DestroyBitmapArray ();
};
#endif
