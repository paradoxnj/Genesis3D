////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu.c
//
//	Copyright (c) 1999, WildTangent, Inc.; All rights reserved.
//
//	API to manage menus.
//
//	History
//	-------
//
//	Peter Siamidis	07/08/98	Created.
//
////////////////////////////////////////////////////////////////////////////////////////
#define	WIN32_LEAN_AND_MEAN
#pragma warning ( disable : 4201 4214 )
#include <windows.h>
#pragma warning ( default : 4201 4214 )
#include <string.h>
#include <assert.h>
#include "Genesis.h"
#include "Ram.h"
#include "Text.h"
#include "MenuItem.h"


#define MENU_ITEMCHECKED_ACTIVE_STRING ("x ")
#define MENU_SELECTED_BORDER (24)		// selected menu item will stay in a box that the menu height, trimmed 
										// by this many pixels on the top and bottom
////////////////////////////////////////////////////////////////////////////////////////
//	Structs
////////////////////////////////////////////////////////////////////////////////////////

// generic menu item
typedef struct tag_MenuItem_T
{

	// adjacent menu items
	struct tag_MenuItem_T	*Next;
	struct tag_MenuItem_T	*Prev;

	// data
	int32			Identifier;	// unique menu item identifier
	Menu_ItemType	ItemType;	// what type of item it is
	int32			MaxWidth;	// max item width
	int32			MaxHeight;	// max item height
	void			*Data;		// item data

} MenuItem_T;

// menu
typedef struct
{
	geEngine	*Engine;				// engine in which menu will be drawn
	int32		Width;					// menu width
	int32		Top,Bottom;				// menu top and bottom (bottom-top = height)
	int32		ItemWidth, ItemHeight;	// dimensions of all menu items
	int32		x, y;					// menu location
	int32		ItemCount;				// how many items are in the menu
	int32		Identifier;				// numeric menu identifier
	MenuItem_T	*Head;					// head of the menu
	MenuItem_T	*Current;				// current active menu item
	int32		MinimumSelectedY;		// minimum allowed y value for top of selected menu item
	int32		MaximumSelectedY;		// maximum allowed y value for bottom of selected menu item
} Menu_T;



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_ToggleItem()
//
//	Toggle a menu item.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_ToggleItem(
	Menu_T		*Menu,			// menu to search through
	int32		Identifier )	// toggle to be toggled
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemToggle_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemToggle );
			Data = (MenuItemToggle_T *)CurItem->Data;
			assert( Data != NULL );

			// toggle the item
			Data->ActiveItem = !Data->ActiveItem;

			// return slider percentage
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the string in question was not found
	return GE_FALSE;

} // Menu_ToggleItem()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_FlagString()
//
//	Marks a string item for change or no change.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_FlagString(
	Menu_T		*Menu,		// menu to search through
	int32		Identifier,	// string to be marked for change
	geBoolean	Flag )		// new setting
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemString_T	*Data;
			int32				Size;

			// get item data
			assert( CurItem->ItemType == Menu_ItemString );
			Data = (MenuItemString_T *)CurItem->Data;
			assert( Data != NULL );

			// mark for change
			Data->AwaitingInput = Flag;

			Size = strlen(Data->StringData);

			if (Flag)
			{
				if (Size + 2 < MENU_MAXSTRINGSIZE)
				{
					Data->StringData[Size] = 0x40;
					Data->StringData[Size+1] = 0;
				}
			}
			// return slider percentage
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the string in question was not found
	return GE_FALSE;

} // Menu_FlagString()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_FlagField()
//
//	Marks a field for change or no change.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_FlagField(
	Menu_T		*Menu,		// menu to search through
	int32		Identifier,	// field to be marked for change
	geBoolean	Flag )		// new setting
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemField_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemField );
			Data = (MenuItemField_T *)CurItem->Data;
			assert( Data != NULL );

			// mark for change
			Data->AwaitingChange = Flag;

			// return slider percentage
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the field in question was not found
	return GE_FALSE;

} // Menu_FlagField()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_SetFieldText()
//
//	Sets a fields text data.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_SetFieldText(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier,		// field whose text we want to set
	char	*NewText )		// text to set in field text data
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( ( Menu == NULL ) || ( NewText == NULL ) )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemField_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemField );
			Data = (MenuItemField_T *)CurItem->Data;
			assert( Data != NULL );

			// save new field text
			strcpy( Data->FieldTextData, NewText );
			Data->FieldTextDataLength = strlen( Data->FieldTextData );

			// return slider percentage
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the field in question was not found
	return GE_FALSE;

} // Menu_SetFieldText()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_SetStringText()
//
//	Sets a string items data.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_SetStringText(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier,		// field whose text we want to set
	char	*NewText )		// text to set in string data
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( ( Menu == NULL ) || ( NewText == NULL ) )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemString_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemString );
			Data = (MenuItemString_T *)CurItem->Data;
			assert( Data != NULL );

			// save new string text
			strcpy( Data->StringData, NewText );
			Data->StringDataLength = strlen( Data->StringData );

			// return slider percentage
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the string in question was not found
	return GE_FALSE;

} // Menu_SetStringText()


////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_GetStringText()
//
//	Gets a string items data.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_GetStringText(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier,		// field whose text we want to set
	char	*NewText )		// text to set in string data
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( ( Menu == NULL ) || ( NewText == NULL ) )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemString_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemString );
			Data = (MenuItemString_T *)CurItem->Data;
			assert( Data != NULL );

			// save new string text
			strcpy(NewText, Data->StringData);
			Data->StringDataLength = strlen( Data->StringData );

			// return slider percentage
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the string in question was not found
	return GE_FALSE;

} // Menu_GetStringText()


////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_GetIdentifier()
//
//	Gets a menus identifier.
//
////////////////////////////////////////////////////////////////////////////////////////
int32 Menu_GetIdentifier(
	Menu_T	*Menu )		// menu whose identifier we want
{

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return -1;
	}

	// return identifier
	return Menu->Identifier;

} // Menu_GetIdentifier()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_GetSliderPercent()
//
//	Get what percentage a slider is set at.
//
////////////////////////////////////////////////////////////////////////////////////////
float Menu_GetSliderPercent(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier )	// slider whose percentage we want
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return -1.0f;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemSlider_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemSlider );
			Data = (MenuItemSlider_T *)CurItem->Data;
			assert( Data != NULL );

			// return slider percentage
			return Data->Percent;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the slider in question was not found
	return -1.0f;

} // Menu_GetSliderPercent()


////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_SetSliderText()
//
////////////////////////////////////////////////////////////////////////////////////////
void Menu_SetSliderText(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier, 
    char * NewText)
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return;// -1.0f;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{
			// locals
			MenuItemSlider_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemSlider );
			Data = (MenuItemSlider_T *)CurItem->Data;
			assert( Data != NULL );

            strcpy(Data->Text, NewText);
            Data->TextLength = strlen(NewText);

			return;
		}

		// go to next item
		CurItem = CurItem->Next;
	}
} // Menu_GetSliderPercent()


////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_SetSliderPercent()
//
//	Set what percentage a slider is set at.
//
////////////////////////////////////////////////////////////////////////////////////////
void Menu_SetSliderPercent(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier, 
    float Percent)
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return;// -1.0f;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its percentage
		if ( CurItem->Identifier == Identifier )
		{
			// locals
			MenuItemSlider_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemSlider );
			Data = (MenuItemSlider_T *)CurItem->Data;
			assert( Data != NULL );

            Data->Percent = Percent;

			return;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

} // Menu_SetSliderPercent()


////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_GetItemSize()
//
//	Gets how many bytes an item requires.
//
////////////////////////////////////////////////////////////////////////////////////////
static int32 Menu_GetItemSize(
	Menu_ItemType	ItemType )		// item whose size we want
{

	// return item size
	switch ( ItemType )
	{
		case Menu_ItemText:
		{
			return sizeof( MenuItemText_T );
		}

		case Menu_ItemSlider:
		{
			return sizeof( MenuItemSlider_T );
		}

		case Menu_ItemField:
		{
			return sizeof( MenuItemField_T );
		}

		case Menu_ItemString:
		{
			return sizeof( MenuItemString_T );
		}

		case Menu_ItemGraphic:
		{
			return sizeof( MenuItemGraphic_T );
		}

		case Menu_ItemToggle:
		{
			return sizeof( MenuItemToggle_T );
		}

		case Menu_ItemChecked:
		{
			return sizeof( MenuItemChecked_T );
		}
	}

	// if we got to here then a size was not found
	assert( 0 );
	return 0;

} //Menu_GetItemSize()


////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_GetCheckedData()
//
//	Get extra data associated with menu item 
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_GetCheckedData(
	Menu_T	*Menu,			// menu to search through
	int32	Identifier,
	int32  *UserData )	// store the data here
{

	// locals	
	MenuItem_T	*CurItem;

	// fail if we have bad data
	if ( Menu == NULL || UserData == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// search all items
	CurItem = Menu->Head;
	while ( CurItem != NULL )
	{
		
		// if its a match then get its data
		if ( CurItem->Identifier == Identifier )
		{

			// locals
			MenuItemChecked_T	*Data;

			// get item data
			assert( CurItem->ItemType == Menu_ItemChecked );
			Data = (MenuItemChecked_T *)CurItem->Data;
			assert( Data != NULL );

			*UserData = Data->UserData;
			return GE_TRUE;
		}

		// go to next item
		CurItem = CurItem->Next;
	}

	// if we got to here then the checked item in question was not found
	return GE_FALSE;

} // Menu_GetSliderPercent()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_Create()
//
//	Create a new menu.
//
////////////////////////////////////////////////////////////////////////////////////////
Menu_T * Menu_Create(
	geEngine	*Engine,		// engine in which menu exists
	int32		Width,			// menu width
	int32		Top,			// top of menu
	int32		Bottom,			// bottom of menu  (bottom-top = menu height)
	int32		x,				// x position
	int32		y,				// y position
	int32		Identifier )	// menu identifier
{

	// locals
	Menu_T	*Menu;

	// fail if we have bad data
	if (	( Engine == NULL ) ||
			( Width <= 0 ) ||
			( Top < 0 ) ||
			( Bottom < Top ) )
	{
		assert( 0 );
		return NULL;
	}

	// allocate new menu struct
	Menu = static_cast<Menu_T*>(geRam_Allocate( sizeof( *Menu ) ));
	if ( Menu == NULL )
	{
		return NULL;
	}
	memset( Menu, 0, sizeof( *Menu ) );

	// save passed data
	Menu->Engine = Engine;
	Menu->Width = Width;
	//Menu->Height = Height;
	Menu->Top = Top;
	Menu->Bottom = Bottom;
	Menu->x = x;
	Menu->y = y;
	Menu->Identifier = Identifier;

	// return new menu pointer
	return Menu;

} // Menu_Create()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_Destroy()
//
//	Destroy a menu.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_Destroy(
	Menu_T	**Menu )		// menu to destroy
{

	// locals
	MenuItem_T	*CurMenuItem;
	MenuItem_T	*NextMenuItem;

	// fail if we have bad data
	if ( *Menu == NULL )
	{
		return GE_FALSE;
	}

	// free all items
	CurMenuItem = ( *Menu )->Head;
	while ( CurMenuItem != NULL )
	{
		NextMenuItem = CurMenuItem->Next;
		geRam_Free( CurMenuItem->Data );
		geRam_Free( CurMenuItem );
		CurMenuItem = NextMenuItem;
	}

	// destroy the menu itself
	geRam_Free( *Menu );
	*Menu = NULL;

	// all done
	return GE_TRUE;

} // Menu_Destroy()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_AddItem()
//
//	Add an item to a menu.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_AddItem(
	Menu_T				*Menu,		// menu to add item to
	Menu_ItemType		ItemType,	// what type of item to add
	int32				Identifier,	// unique item identifier
	void				*Data )		// item data
{

	// locals
	MenuItem_T	*Item;
	int32		ItemSize;

	// fail if we have bad data
	if (	( Menu == NULL ) ||
			( Identifier <= 0 ) ||
			( Data == NULL ) )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// determine item size
	ItemSize = Menu_GetItemSize( ItemType );
	if ( ItemSize <= 0 )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// allocate generic menu item data
	Item = (MenuItem_T *)geRam_Allocate( sizeof( *Item ) );
	if ( Item == NULL )
	{
		return GE_FALSE;
	}
	memset( Item, 0, sizeof( *Item ) );

	// allocate specific menu item data
	Item->Data = geRam_Allocate( ItemSize );
	if ( Item->Data == NULL )
	{
		geRam_Free( Item );
		return GE_FALSE;
	}
	memset( Item->Data, 0, ItemSize );

	// save passed data
	Item->Identifier = Identifier;
	Item->ItemType = ItemType;
	memcpy( Item->Data, Data, ItemSize );

	// save items max dimensions
	switch ( ItemType )
	{

		case Menu_ItemText:
		{
			
			// locals
			MenuItemText_T	*TextData;
			int32			Size;

			// get item data
			TextData = (MenuItemText_T *)Data;

			// set max item width
			Size = Text_GetWidth( TextData->NormalFont );
			if ( Text_GetWidth( TextData->SelectFont ) > Size )
			{
				Size = Text_GetWidth( TextData->SelectFont );
			}
			Item->MaxWidth = Size * TextData->TextLength;

			// set max item height
			Size = Text_GetHeight( TextData->NormalFont );
			if ( Text_GetHeight( TextData->SelectFont ) > Size )
			{
				Size = Text_GetHeight( TextData->SelectFont );
			}
			Item->MaxHeight = Size;
			break;
		}

		case Menu_ItemSlider:
		{
			
			// locals
			MenuItemSlider_T	*SliderData;
			int32				TextWidth;
			int32				TextHeight;
			int32				SliderWidth;
			int32				SliderHeight;
			int32				Size;

			// get item data
			SliderData = (MenuItemSlider_T *)Data;

			// get text width
			Size = Text_GetWidth( SliderData->NormalFont );
			if ( Text_GetWidth( SliderData->SelectFont ) > Size )
			{
				Size = Text_GetWidth( SliderData->SelectFont );
			}
			TextWidth = Size * SliderData->TextLength;

			// get text height
			Size = Text_GetHeight( SliderData->NormalFont );
			if ( Text_GetHeight( SliderData->SelectFont ) > Size )
			{
				Size = Text_GetHeight( SliderData->SelectFont );
			}
			TextHeight = Size;

			// get slider width and height //undone, hardcoded
			SliderWidth = 128;// geGetTextureWidth( SliderData->Range );
			SliderHeight = 14;// geGetTextureHeight( SliderData->Bar );

			// set max width and height
			Item->MaxWidth = TextWidth + SliderWidth;
			Item->MaxHeight = ( TextHeight > SliderHeight ) ? TextHeight : SliderHeight;
			break;
		}

		case Menu_ItemField:
		{

			// locals
			MenuItemField_T	*FieldData;
			int32			Size;

			// get item data
			FieldData = (MenuItemField_T *)Data;

			// get text width
			Size = Text_GetWidth( FieldData->NormalFont );
			if ( Text_GetWidth( FieldData->SelectFont ) > Size )
			{
				Size = Text_GetWidth( FieldData->SelectFont );
			}
			Item->MaxWidth = Size * ( FieldData->FieldNameLength + FieldData->FieldTextDataLength );

			// get text height
			Size = Text_GetHeight( FieldData->NormalFont );
			if ( Text_GetHeight( FieldData->SelectFont ) > Size )
			{
				Size = Text_GetHeight( FieldData->SelectFont );
			}
			Item->MaxHeight = Size;
			break;
		}

		case Menu_ItemString:
		{
			
			// locals
			MenuItemString_T	*StringData;
			int32				Size;

			// get item data
			StringData = (MenuItemString_T *)Data;
			assert( StringData != NULL );

			// set max item width
			Size = Text_GetWidth( StringData->NormalFont );
			if ( Text_GetWidth( StringData->SelectFont ) > Size )
			{
				Size = Text_GetWidth( StringData->SelectFont );
			}
			Item->MaxWidth = Size * ( StringData->StringLabelLength + StringData->StringDataLength );

			// set max item height
			Size = Text_GetHeight( StringData->NormalFont );
			if ( Text_GetHeight( StringData->SelectFont ) > Size )
			{
				Size = Text_GetHeight( StringData->SelectFont );
			}
			Item->MaxHeight = Size;
			break;
		}

		case Menu_ItemGraphic:
		{
			// not supported untill we get a function that gives us the
			// dimensions of a texture
			Item->MaxWidth = 1;
			Item->MaxHeight = 1;
			break;
		}

		case Menu_ItemToggle:
		{
			
			// locals
			MenuItemToggle_T	*ToggleData;
			int32				Size;

			// get item data
			ToggleData = (MenuItemToggle_T *)Data;
			assert( ToggleData != NULL );

			// set max item width
			Size = Text_GetWidth( ToggleData->NormalFont );
			if ( Text_GetWidth( ToggleData->SelectFont ) > Size )
			{
				Size = Text_GetWidth( ToggleData->SelectFont );
			}
			Item->MaxWidth = Size * ( ToggleData->ToggleLabelLength + ToggleData->ToggleData1Length );

			// set max item height
			Size = Text_GetHeight( ToggleData->NormalFont );
			if ( Text_GetHeight( ToggleData->SelectFont ) > Size )
			{
				Size = Text_GetHeight( ToggleData->SelectFont );
			}
			Item->MaxHeight = Size;
			break;
		}

		case Menu_ItemChecked:
		{
			
			// locals
			MenuItemChecked_T	*CheckedData;
			int32				Size;

			// get item data
			CheckedData = (MenuItemChecked_T *)Data;
			assert( CheckedData != NULL );

			// set max item width
			Size = Text_GetWidth( CheckedData->NormalFont );
			if ( Text_GetWidth( CheckedData->SelectFont ) > Size )
			{
				Size = Text_GetWidth( CheckedData->SelectFont );
			}
			Item->MaxWidth = Size * ( CheckedData->LabelLength );

			// set max item height
			Size = Text_GetHeight( CheckedData->NormalFont );
			if ( Text_GetHeight( CheckedData->SelectFont ) > Size )
			{
				Size = Text_GetHeight( CheckedData->SelectFont );
			}
			Item->MaxHeight = Size;
			break;
		}

		default:
		{
			assert( 0 );
			geRam_Free( Item );
			return GE_FALSE;
		}
	}

	// bail if the item sizes are bad
	if ( ( Item->MaxWidth <= 0 ) || ( Item->MaxHeight <= 0 ) )
	{
		assert( 0 );
		geRam_Free( Item );
		return GE_FALSE;
	}

	// make menu head...
	if ( Menu->Current == NULL )
	{
		Menu->Head = Menu->Current = Item;
		Menu->ItemCount = 1;
	}
	// ...or just append item
	else
	{

		// find the end of the items
		Menu->Current = Menu->Head;
		while ( Menu->Current->Next != NULL )
		{
			Menu->Current = Menu->Current->Next;
		}

		// append this item
		Menu->Current->Next = Item;
		Item->Prev = Menu->Current;
		Menu->ItemCount++;
		Menu->Current = Menu->Head;
	}

	// adjust size taken up by all menu items
	Menu->ItemHeight += Item->MaxHeight;
	Menu->ItemWidth = ( Menu->ItemWidth < Item->MaxWidth ) ? Item->MaxWidth : Menu->ItemWidth;

	Menu->MaximumSelectedY = Menu->Bottom- MENU_SELECTED_BORDER;
	Menu->MinimumSelectedY = Menu->Top + MENU_SELECTED_BORDER;
	
	// all done
	return GE_TRUE;

} // Menu_AddItem()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_Draw()
//
//	Draw a new menu.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean Menu_Draw(
	Menu_T	*Menu )		// menu to draw
{

	// locals
	MenuItem_T	*Item;
	int32		x, y;
	int32		TestY;

	// fail if we have bad data
	if ( Menu == NULL )
	{
		assert( 0 );
		return GE_FALSE;
	}

	// get head of menu
	Item = Menu->Head;
	if ( Item == NULL )
	{
		return GE_FALSE;
	}

	// set starting coordinates
	x = ( Menu->Width / 2 );
	y = ( (Menu->Bottom+Menu->Top) / 2 ) - ( Menu->ItemHeight / 2 );


	// assumes only one item is selected at a time.
	TestY = y;
	while ( Item != NULL )
	{
		if (Menu->Current == Item)
			break;
		TestY += Item->MaxHeight;
		Item=Item->Next;
	}

	if (TestY < Menu->MinimumSelectedY)
		{
			y += Menu->MinimumSelectedY-TestY;
		}
	if (TestY + Item->MaxHeight > Menu->MaximumSelectedY)
		{
			y -= TestY + Item->MaxHeight - Menu->MaximumSelectedY;
		}
			

	Item = Menu->Head;
	// draw all items
	while ( Item != NULL )
	{
		if ( y + Item->MaxHeight > Menu->Bottom)
			break;

		// draw item based on its type
		switch ( Item->ItemType )
		{

			case Menu_ItemText:
			{

				// locals
				MenuItemText_T	*Data;
				FontType		Font;

				// get item data
				Data = (MenuItemText_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}

				// choose font
				if ( Menu->Current != Item )
				{
					Font = Data->NormalFont;
				}
				else
				{
					Font = Data->SelectFont;
				}

				// draw text
				Text_Out( Data->Text, Font, x, y, Style_Center );

				y += Item->MaxHeight;
				break;
			}

			case Menu_ItemSlider:
			{

				// locals
				MenuItemSlider_T	*Data;
				FontType			Font;
				int32				RangeWidth, RangeHeight;
				int32				BarWidth, BarHeight;
				int32				SliderX, SliderY;

				// get item data
				Data = (MenuItemSlider_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}

				// choose font
				if ( Menu->Current != Item )
				{
					Font = Data->NormalFont;
				}
				else
				{
					Font = Data->SelectFont;
				}

				// draw text
				Text_Out( Data->Text, Font, x, y, Style_RightJustify );

				// get slider dimensions //undone hardcoded
				RangeHeight = 14;// geGetTextureHeight( Data->Range );
				RangeWidth = 128 - Data->Slack;// geGetTextureWidth( Data->Range );
				BarWidth = 16;// geGetTextureWidth( Data->Bar );
				BarHeight = 14;// geGetTextureHeight( Data->Bar );

				// draw slider
				geEngine_DrawBitmap(Menu->Engine, Data->Range, NULL, x, y - ( RangeHeight / 2 ) );

				// draw slider bar
				SliderX = x + ( Data->Slack / 2 ) + (int32)( Data->Percent * (float)RangeWidth ) - ( BarWidth / 2 );
				SliderY = y - ( BarHeight / 2 );
				geEngine_DrawBitmap( Menu->Engine, Data->Bar, NULL, SliderX, SliderY );
				y += Item->MaxHeight;
				break;
			}

			case Menu_ItemField:
			{

				// locals
				MenuItemField_T	*Data;
				FontType		Font;

				// get item data
				Data = (MenuItemField_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}

				// choose font
				if ( Menu->Current != Item )
				{
					Font = Data->NormalFont;
				}
				else
				{
					Font = Data->SelectFont;
				}

				// output data fields
				Text_Out( Data->FieldName, Font, x, y, Style_RightJustify );
				if ( Data->AwaitingChange == GE_FALSE )
				{
					Text_Out( Data->FieldTextData, Font, x, y, Style_LeftJustify );
				}
				else
				{
					Text_Out( "PRESS NEW KEY", Font, x, y, Style_LeftJustify );
				}
				y += Item->MaxHeight;
				break;
			}

			case Menu_ItemString:
			{

				// locals
				MenuItemString_T	*Data;
				FontType			Font;

				// get item data
				Data = (MenuItemString_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}

				// choose font
				if ( Menu->Current != Item )
				{
					Font = Data->NormalFont;
				}
				else
				{
					Font = Data->SelectFont;
				}

				// output data fields
				Text_Out( Data->StringLabel, Font, x, y, Style_RightJustify );
				Text_Out( Data->StringData, Font, x, y, Style_LeftJustify );
				y += Item->MaxHeight;
				break;
			}

			case Menu_ItemGraphic:
			{

				// locals
				MenuItemGraphic_T	*Data;

				// get item data
				Data = (MenuItemGraphic_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}
				
				// draw graphic
				geEngine_DrawBitmap( Menu->Engine, Data->Art, NULL, Data->x, Data->y );
				break;
			}

			case Menu_ItemToggle:
			{

				// locals
				MenuItemToggle_T	*Data;
				FontType			Font;

				// get item data
				Data = (MenuItemToggle_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}

				// choose font
				if ( Menu->Current != Item )
				{
					Font = Data->NormalFont;
				}
				else
				{
					Font = Data->SelectFont;
				}

				// output data fields
				Text_Out( Data->ToggleLabel, Font, x, y, Style_RightJustify );
				if ( Data->ActiveItem == 0 )
				{
					Text_Out( Data->ToggleData1, Font, x, y, Style_LeftJustify );
				}
				else
				{
					Text_Out( Data->ToggleData2, Font, x, y, Style_LeftJustify );
				}
				y += Item->MaxHeight;
				break;
			}
		case Menu_ItemChecked:
			{

				// locals
				MenuItemChecked_T	*Data;
				FontType			Font;
				
				// get item data
				Data = (MenuItemChecked_T *)Item->Data;
				if ( Data == NULL )
				{
					assert( 0 );
					break;
				}

				// choose font
				if ( Menu->Current != Item )
				{
					Font = Data->NormalFont;
				}
				else
				{
					Font = Data->SelectFont;
				}

				// output data fields
				Text_Out( Data->Label, Font, Data->LabelPosition, y, Style_LeftJustify );
				if ( Data->ActiveItem == 1 )
				{
					Text_Out( MENU_ITEMCHECKED_ACTIVE_STRING, Font, Data->LabelPosition, y, Style_RightJustify );
				}
				y += Item->MaxHeight;
				break;
			}

		}

		// get next item
		Item = Item->Next;
	}

	// all done
	return GE_TRUE;

} // Menu_Draw()



////////////////////////////////////////////////////////////////////////////////////////
//
//	Menu_Key()
//
//	Pass a keystroke to a menu.
//
////////////////////////////////////////////////////////////////////////////////////////
int32 Menu_Key(
	Menu_T	*Menu,	// menu to receive the keystroke
	int32	Key )	// input keystroke
{

	// fail if we have invalid data
	if ( Menu == NULL )
	{
		assert( 0 );
		return -1;
	}

	// decide what to do based on keystroke type
	switch( Key )
	{

		// up arrow
		case VK_UP:
		{

			// pick item above as curently selected one...
			if ( Menu->Current->Prev != NULL )
			{
				Menu->Current = Menu->Current->Prev;
			}
			// ...or wrap around to the bottom of the list
			else
			{
				while ( Menu->Current->Next != NULL )
				{
					Menu->Current = Menu->Current->Next;
				}
			}
		
			while (Menu->Current->Identifier == 0xffff)
			{
				// pick item above as curently selected one...
				if ( Menu->Current->Prev != NULL )
				{
					Menu->Current = Menu->Current->Prev;
				}
				// ...or wrap around to the bottom of the list
				else
				{
					while ( Menu->Current->Next != NULL )
					{
						Menu->Current = Menu->Current->Next;
					}
				}
			}
			break;
		}

		// down arrow
		case VK_DOWN:
		{

			// pick item below as curently selected one...
			if ( Menu->Current->Next != NULL )
			{
				Menu->Current = Menu->Current->Next;
			}
			// ... or wrap around to the top of the list
			else
			{
				Menu->Current = Menu->Head;
			}
	
			while (Menu->Current->Identifier == 0xffff)
			{
				if ( Menu->Current->Next != NULL )
				{
					Menu->Current = Menu->Current->Next;
				}
				else
				{
					Menu->Current = Menu->Head;
				}
			}

			break;
		}

		// left arrow
		case VK_LEFT:
		{

			// adjust a slider...
			if ( Menu->Current->ItemType == Menu_ItemSlider )
			{

				// locals
				MenuItemSlider_T	*Data;

				// get item data
				Data = (MenuItemSlider_T *)Menu->Current->Data;
				assert( Data != NULL );

				//Data->Percent -= 0.05f;
				Data->Percent -= Data->Increment;
				if ( Data->Percent < 0.01f )
				{
					Data->Percent = 0.0f;
				}
			}
			break;
		}

		// right arrow
		case VK_RIGHT:
		{

			// adjust a slider...
			if ( Menu->Current->ItemType == Menu_ItemSlider )
			{

				// locals
				MenuItemSlider_T	*Data;

				// get item data
				Data = (MenuItemSlider_T *)Menu->Current->Data;
				assert( Data != NULL );

				//Data->Percent += 0.05f;
                Data->Percent += Data->Increment;
				if ( Data->Percent > 0.99f )
				{
					Data->Percent = 1.0f;
				}
			}
			break;
		}
	}

	// return id of currently selected item
	return Menu->Current->Identifier;

} // Menu_Key()
