////////////////////////////////////////////////////////////////////////////////////////
//
//	Text.h
//
//	Include for Text.c
//
////////////////////////////////////////////////////////////////////////////////////////
#ifndef TEXT_H
#define TEXT_H

#include "Genesis.h"

#ifdef __cplusplus
	extern "C" {
#endif


////////////////////////////////////////////////////////////////////////////////////////
//	Structs
////////////////////////////////////////////////////////////////////////////////////////

// available fonts
typedef enum
{
	Font_Default = 0,
	Font_DefaultSelect,
	Font_Small,
	MAX_FONTS
} FontType;

// output styles
typedef enum
{
	Style_Center = 0,
	Style_LeftJustify,
	Style_RightJustify,
} FontStyle;


////////////////////////////////////////////////////////////////////////////////////////
//	Prototypes
////////////////////////////////////////////////////////////////////////////////////////

// create everything required for text output
extern geBoolean Text_Create(
	geEngine	*SaveEngine );	// engine used for output, which gets saved

// free everything
extern geBoolean Text_Destroy(
	void );	// no parameters

// output a text string
extern geBoolean Text_Out(
	char		*Text,		// string to output
	FontType	Font,		// font to use
	int32		x,			// x location
	int32		y,			// y location
	FontStyle	Style );	// output stylea

// get the width of a font
extern int32 Text_GetWidth(
	FontType	Font );	// font whose width we want

// get the height of a font
extern int32 Text_GetHeight(
	FontType	Font );	// font whose height we want


#ifdef __cplusplus
	}
#endif

#endif
