////////////////////////////////////////////////////////////////////////////////////////
//
//	MenuItem.h
//
//	Supported menu items.
//
////////////////////////////////////////////////////////////////////////////////////////
#ifndef MENUITEM_H
#define MENUITEM_H

#include "GENESIS.H"
#include "Text.h"

#define MENU_MAXSTRINGSIZE	64



////////////////////////////////////////////////////////////////////////////////////////
//	Supported menu items.
////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	Menu_ItemText = 0,
	Menu_ItemGraphic,
	Menu_ItemSlider,
	Menu_ItemField,
	Menu_ItemString,
	Menu_ItemToggle,
	Menu_ItemChecked
} Menu_ItemType;


////////////////////////////////////////////////////////////////////////////////////////
//	Text menu item.
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	char				*Text;		// text
	int32				TextLength;	// length of text
	FontType			NormalFont;	// regular font
	FontType			SelectFont;	// selected font

} MenuItemText_T;


////////////////////////////////////////////////////////////////////////////////////////
//	Slider menu item.
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	char				*Text;			// text
	int32				TextLength;		// length of text
	FontType			NormalFont;		// regular font
	FontType			SelectFont;		// selected font
	geBitmap			*Range;			// slider range art
	geBitmap			*Bar;			// slider bar art
	int32				Slack;			// amount of dead pixels in slider range art
	float				Percent;		// what percentage slider is at
    float               Increment;      // increment amount

} MenuItemSlider_T;


////////////////////////////////////////////////////////////////////////////////////////
//	Field menu item.
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	char				*FieldName;							// field name text
	int32				FieldNameLength;					// length of field name text
	char				FieldTextData[MENU_MAXSTRINGSIZE];	// data text
	int32				FieldTextDataLength;				// length of data text
	FontType			NormalFont;							// regular font
	FontType			SelectFont;							// selected font
	geBoolean			AwaitingChange;						// field is awaiting input flag

} MenuItemField_T;


////////////////////////////////////////////////////////////////////////////////////////
//	String menu item.
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	char				*StringLabel;					// string label text
	int32				StringLabelLength;				// length of string label text
	char				StringData[MENU_MAXSTRINGSIZE];	// string data text
	int32				StringDataLength;				// length of string data text
	FontType			NormalFont;						// regular font
	FontType			SelectFont;						// selected font
	geBoolean			AwaitingInput;					// string is awaiting input flag

} MenuItemString_T;


////////////////////////////////////////////////////////////////////////////////////////
//	Graphic menu item.
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	geBitmap			*Art;	// graphic art
	int32				x, y;	// graphic location

} MenuItemGraphic_T;


////////////////////////////////////////////////////////////////////////////////////////
//	Toggle menu item.
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	char				*ToggleLabel;						// toggle label text
	int32				ToggleLabelLength;					// length of toggle label text
	char				ToggleData1[MENU_MAXSTRINGSIZE];	// toggle data text
	int32				ToggleData1Length;					// length of toggle data text
	char				ToggleData2[MENU_MAXSTRINGSIZE];	// toggle data text
	int32				ToggleData2Length;					// length of toggle data text
	FontType			NormalFont;							// regular font
	FontType			SelectFont;							// selected font
	int32				ActiveItem;							// currently active item

} MenuItemToggle_T;

////////////////////////////////////////////////////////////////////////////////////////
//	Checked menu item.
////////////////////////////////////////////////////////////////////////////////////////
#define MENUITEM_MAX_CHECKED_LABEL (100)
typedef struct
{
	char				Label[MENUITEM_MAX_CHECKED_LABEL];	// label text
	int32				LabelLength;						// length of label text
	FontType			NormalFont;							// regular font
	FontType			SelectFont;							// selected font
	int32				ActiveItem;							// currently active item
	int32				LabelPosition;						// left edge of label text  ('check' is to the left)
	int32				UserData;							// misc data.
} MenuItemChecked_T;


#endif
