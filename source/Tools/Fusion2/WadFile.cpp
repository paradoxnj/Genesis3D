/****************************************************************************************/
/*  WadFile.cpp                                                                         */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Eli Boling                                    */
/*  Description:  bitmap related stuff                                                  */
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
#include "stdafx.h"
#include "WadFile.h"
#include "RAM.H"
#include "vfile.h"
#include "util.h"
#include <assert.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static void WadFileEntry_Free (WadFileEntry *pe)
{
	if (pe->LockedBitmap != NULL)
	{
		geBitmap_UnLock (pe->LockedBitmap);
	}
	if (pe->bmp != NULL)
	{
		geBitmap_Destroy (&pe->bmp);
	}
	if (pe->Name != NULL)
	{
		geRam_Free (pe->Name);
	}
}

CWadFile::CWadFile()
{
	mBitmapCount = 0;
	mBitmaps = NULL;
}


void CWadFile::DestroyBitmapArray ()
{
	if( mBitmaps != NULL )
	{
		for (; mBitmapCount > 0; --mBitmapCount)
		{
			WadFileEntry_Free (&mBitmaps[mBitmapCount-1]);
		}
		geRam_Free (mBitmaps);
	}
}

CWadFile::~CWadFile()
{
	DestroyBitmapArray ();
}

static int wadCountFiles (geVFile *vfs, const char *fspec)
{
	int nFiles = 0;
	geVFile_Finder *Finder;

	// count the number of files
	Finder = geVFile_CreateFinder (vfs, fspec);
	if (Finder != NULL)
	{
		while (geVFile_FinderGetNextFile (Finder) != GE_FALSE)
		{
			++nFiles;
		}
		geVFile_DestroyFinder (Finder);
	}
	return nFiles;
}

geBoolean CWadFile::Setup(const char *Filename)
{
	geVFile *			Library;

	geBoolean			NoErrors = GE_FALSE;

	Library = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_VIRTUAL, Filename, NULL, GE_VFILE_OPEN_READONLY | GE_VFILE_OPEN_DIRECTORY);
	if (Library != NULL)
	{
		NoErrors = GE_TRUE;

		DestroyBitmapArray ();

		int nFiles = wadCountFiles (Library, "*.*");

		if (nFiles > 0)
		{
			// make new array and fill it with loaded bitmaps
			mBitmaps = (WadFileEntry *)geRam_Allocate (nFiles * sizeof (WadFileEntry));

			// and fill array with filenames
			NoErrors = GE_FALSE;
			geVFile_Finder *Finder = geVFile_CreateFinder (Library, "*.*");
			if (Finder != NULL)
			{
				NoErrors = GE_TRUE;
				geVFile_Properties Props;

				while (geVFile_FinderGetNextFile (Finder) != GE_FALSE)
				{
					geVFile_FinderGetProperties (Finder, &Props);

					// load the file and create a DibBitmap from it
					geVFile *BmpFile = geVFile_Open (Library, Props.Name, GE_VFILE_OPEN_READONLY);
					geBitmap *TheBitmap;

					if (BmpFile == NULL)
					{
						NoErrors = GE_FALSE;
					}
					else
					{
						TheBitmap = geBitmap_CreateFromFile (BmpFile);
						geVFile_Close (BmpFile);
						if (TheBitmap == NULL)
						{
							NoErrors = GE_FALSE;
						}
						else
						{
							if (geBitmap_SetFormat (TheBitmap, GE_PIXELFORMAT_16BIT_555_RGB, GE_FALSE, 0, NULL) != GE_FALSE)
							{
								WadFileEntry *pe;
								geBitmap_Info info, info2;

								geBitmap_GetInfo (TheBitmap, &info, &info2);
								pe = &mBitmaps[mBitmapCount];
								pe->bmp = TheBitmap;
								pe->Height = info.Height;
								pe->Width = info.Width;
								pe->Name = Util_Strdup (Props.Name);
								geBitmap_LockForReadNative (pe->bmp, &pe->LockedBitmap, 0, 0);
								pe->BitsPtr = geBitmap_GetBits (pe->LockedBitmap);

								++mBitmapCount;
							}
							else
							{
								geBitmap_Destroy (&TheBitmap);
								NoErrors = GE_FALSE;
							}
						}
					}
				}

				geVFile_DestroyFinder (Finder);
			}
		}
		geVFile_Close (Library);
	}

	return GE_TRUE;
}
