////////////////////////////////////////////////////////////////////////////////////////
//
//	Text.c
//
//	Copyright (c) 1999, WildTangent, Inc.; All rights reserved.
//
//	Text output API.
//
//	History
//	-------
//
//	Peter Siamidis	02/05/98	Created.
//
////////////////////////////////////////////////////////////////////////////////////////
#pragma warning ( disable : 4514 )
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Text.h"
#include "Errorlog.h"

#define	MAX_SUPPORTED_CHARS	128


////////////////////////////////////////////////////////////////////////////////////////
//	Structs
////////////////////////////////////////////////////////////////////////////////////////

// font struct
typedef struct
{
	geBitmap			*Bitmap;
	int32				FontWidth;
	int32				FontHeight;
	int32				AddWidth;
	int32				AddHeight;
} Font_Type;


////////////////////////////////////////////////////////////////////////////////////////
//	Globals
////////////////////////////////////////////////////////////////////////////////////////
static char			*Dir = "Bmp\\";						// where font decals are found
static char			*Ext = ".Bmp";						// exetension of decal files
static geEngine		*Engine = NULL;						// engine to use for output
Font_Type			Fonts[MAX_FONTS];					// font list
int32				FontLookup[MAX_SUPPORTED_CHARS];	// character lookup table
static char			FontNames[MAX_FONTS][100] =			// font names
{
	"SFont1",
	"SFont2",
	"Console\\640x480\\font"
};



////////////////////////////////////////////////////////////////////////////////////////
//
//	Text_Create()
//
//	Create everything required for text output. The passed engine pointer gets
//	saved for later use.
//
////////////////////////////////////////////////////////////////////////////////////////
extern geVFile *MainFS;
geBoolean Text_Create(
	geEngine	*SaveEngine )	// engine used for output
{

	// locals
	char		Filename[64];
	int32		i, PosX, PosY;
	int32		Width;
	geVFile *		File;

	// fail if we have an invalid engine pointer
	if ( SaveEngine == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// setup font character lookup table
	PosX = 0;
	PosY = 0;
	Width = 17 * 15;
	for ( i = 0; i < MAX_SUPPORTED_CHARS; i++ )
	{
		FontLookup[i] = ( PosX << 16 ) | PosY;
		PosX += 17;

		if ( PosX >= Width )
		{
			PosY += 17;
			PosX = 0;
		}
	}

	// setup all fonts (fail if we can't)
	for ( i = 0; i < MAX_FONTS; i++ )
	{
		//geBitmap	*Pal;

		// build font filename
		strcpy( Filename, Dir );
		strcat( Filename, FontNames[i] );
		strcat( Filename, Ext );

		// load the decal
		File = geVFile_Open(MainFS, Filename, GE_VFILE_OPEN_READONLY);
		if	(!File)
		{
			geErrorLog_AddString(-1, "Text_Create:  geVFile_Open failed:", Filename);
			return GE_FALSE;
		}
		Fonts[i].Bitmap = geBitmap_CreateFromFile(File);
		geVFile_Close(File);

		if (!Fonts[i].Bitmap)
		{
			geErrorLog_AddString(-1, "Text_Create:  geBitmap_CreateFromFile failed.", NULL);
			return GE_FALSE;
		}

		if (!geBitmap_SetColorKey(Fonts[i].Bitmap, GE_TRUE, 255, GE_FALSE))
		{
			geErrorLog_AddString(-1, "Text_Create:  geBitmap_SetColorKey failed.", NULL);
			return GE_FALSE;
			//goto ExitWithError;
		}

		if (!geEngine_AddBitmap(SaveEngine, Fonts[i].Bitmap))
		{
			geErrorLog_AddString(-1, "Text_Create:  geEngine_AddBitmap failed.", NULL);
			return GE_FALSE;
		}

		// init font dimensions
		if (i == Font_Small)	// special case. sorry. 
			{
				Fonts[i].FontWidth  = 8;
				Fonts[i].FontHeight = 16;
				Fonts[i].AddHeight  = 16  + 2;
				Fonts[i].AddWidth   = 8  - 1;
				continue;
			}
		else
			{
				Fonts[i].FontWidth = 16;
				Fonts[i].FontHeight = 16;
				Fonts[i].AddHeight = 16 + 5;
				Fonts[i].AddWidth = 13;
			}
	}

	// save the engine pointer
	Engine = SaveEngine;

	// all done
	return GE_TRUE;

} // Text_Create()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Text_Destroy()
//
//	Free everything.
//
//	Returns
//		GE_TRUE
//			Text system has been destroyed.
//		GE_FALSE
//			Text system has no been completely destroyed.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Text_Destroy(
	void )	// no parameters
{

	// locals
	int32		i;
	geBoolean	Ret;

	Ret = GE_TRUE;

	// fail if we have an invalid engine pointer
	if ( Engine == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// zap font lookup table
	memset( FontLookup, 0, sizeof( FontLookup ) );

	// zap all font decals
	for ( i = 0; i < MAX_FONTS; i++ )
	{
		if (!Fonts[i].Bitmap)
			continue;

		if (!geEngine_RemoveBitmap(Engine, Fonts[i].Bitmap))
			Ret = GE_FALSE;

		geBitmap_Destroy(&Fonts[i].Bitmap);
		Fonts[i].Bitmap = NULL;

	}

	// zap all font info
	memset( Fonts, 0, sizeof( Fonts ) );

	// zap saved engine pointer
	Engine = NULL;

	// all done
	return Ret;

} // Text_Create()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Text_Out()
//
//	Output a text string.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Text_Out(
	char		*Text,		// string to output
	FontType	FontNum,	// font to use
	int32		x,			// x location
	int32		y,			// y location
	FontStyle	Style )		// output style
{

	// locals
	GE_Rect	Rect;
	int32	Length;
	uint8	Val;

	// fail if we have invalid data
	if ( ( Engine == NULL ) || ( Text == NULL ) )
	{
		assert( 0 );
		return GE_FALSE;
	}
	if ( ( FontNum < 0 ) || ( FontNum >= MAX_FONTS ) )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// get length of string (fail if it's invalid)
	Length = strlen( Text );
	if ( Length <= 0 )
	{
		return GE_FALSE;
	}

	// always center characters on y axis
	y -= Fonts[FontNum].FontHeight / 2;

	// adjust x coordinate to reflect style
	switch ( Style )
	{

		case Style_Center:
		{
			x -= ( Length * Fonts[FontNum].AddWidth ) / 2;
			break;
		}

		case Style_RightJustify:
		{
			x -= ( Length * Fonts[FontNum].AddWidth );
			break;
		}
	}
	if (FontNum == Font_Small)
		{
			// process all characters
			while ( Length-- > 0 )
			{

				// get current character
				Val = *Text++;

				// only print characters in range
				if ( ( Val >= 0 ) && ( Val < MAX_SUPPORTED_CHARS ) )
				{
					// setup character rect
					Rect.Left = Val * Fonts[FontNum].FontWidth;
					Rect.Right = Rect.Left + Fonts[FontNum].FontWidth;
					Rect.Top = 0;
					Rect.Bottom = Rect.Top + Fonts[FontNum].FontHeight;

					// draw character
					geEngine_DrawBitmap( Engine, Fonts[FontNum].Bitmap, &Rect, x, y );
				}

				// shift to next position
				x += Fonts[FontNum].AddWidth;
			}
	
		}
	else
		{
			// process all characters
			while ( Length-- > 0 )
			{

				// get current character
				Val = *Text++;

				// only print characters in range
				if ( ( Val >= 0 ) && ( Val < MAX_SUPPORTED_CHARS ) )
				{
				
					// capitalize it if required
					if ( Val >= 'a' && Val <= 'z' )
					{
						Val += (uint8)( 'A' - 'a' );
					}

					// setup character rect
					Rect.Left = ( FontLookup[Val] >> 16 ) + 1;
					Rect.Right = Rect.Left + 16;
					Rect.Top = ( FontLookup[Val] & 0xffff ) + 1;
					Rect.Bottom = Rect.Top + 16;

					// draw character
					geEngine_DrawBitmap( Engine, Fonts[FontNum].Bitmap, &Rect, x, y );
				}

				// shift to next position
				x += Fonts[FontNum].AddWidth;
			}
		}

	// all done
	return GE_TRUE;

} // Text_Out()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Text_GetWidth()
//
//	Get the width of a font.
//
//	Returns
//		int32
//			Font spacing width.
//		-1
//			Could not get font spacing width.
//
////////////////////////////////////////////////////////////////////////////////////////
int32 Text_GetWidth(
	FontType	Font )	// font whose width we want
{

	// fail if we have invalid data
	if ( Fonts[Font].Bitmap == NULL )
	{
		assert( 0 );
		return -1;
	}

	// return font spacing width
	return Fonts[Font].AddWidth;
	
} // Text_GetWidth()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Text_GetHeight()
//
//	Get the height of a font.
//
//	Returns
//		int32
//			Font spacing height.
//		-1
//			Could not get font spacing height.
//
////////////////////////////////////////////////////////////////////////////////////////
int32 Text_GetHeight(
	FontType	Font )	// font whose height we want
{

	// fail if we have invalid data
	if ( Fonts[Font].Bitmap == NULL )
	{
		assert( 0 );
		return -1;
	}

	// return font spacing height
	return Fonts[Font].AddHeight;
	
} // Text_GetWidth()
