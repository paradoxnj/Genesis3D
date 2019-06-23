////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu.c
//
//	Copyright (c) 1999, WildTangent, Inc.; All rights reserved.
//
//	Game menu management layer.
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
#include <assert.h>
#include <string.h>
#include "GMenu.h"
//#include "Menu.h"
#include "ipaddr.h"
#include "AutoSelect.h"

#include "Errorlog.h"

#include "Host.h"

// this is ugly
extern Host_Host		*Host;
extern geSound_System	*SoundSys;
extern geEngine			*Engine;	
extern geBoolean         ShowStats;
extern int			ChangeDisplaySelection;
extern geBoolean		 ChangingDisplayMode;

#define GMENU_VERSION		0x100

#define NUM_MAPPED_KEYS		7

#define GAMMA_MAXIMUM		(4.0f)

#define SMALL_MENU_CUTOFF_WIDTH (512)
#define SMALL_CREDITS_CUTOFF_WIDTH (640)

#define GMENU_DRIVER_LEFT_EDGE (50)

////////////////////////////////////////////////////////////////////////////////////////
//	Keyboard mappings ( action, wParam, lParam )
////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	int32 Action;		// game action identifier
	int32 wParam;		// wParam from WM_KEYDOWN message
	int32 lParam;		// lParam from WM_KEYDOWN message
} GMenu_SingleKeyMapping;

static GMenu_SingleKeyMapping OriginalKeyMapping[NUM_MAPPED_KEYS] =
{
	{ GMenu_KeyShoot, VK_LBUTTON, 1 },
	{ GMenu_KeyJump, VK_RBUTTON, 2 },
	{ GMenu_KeyStrafeLeft, VK_LEFT, 21692417 },
	{ GMenu_KeyStrafeRight, VK_RIGHT, 21823489 },
	{ GMenu_KeyForward, VK_UP, 21495809 },
	{ GMenu_KeyBackward, VK_DOWN, 22020097 },
	{ GMenu_KeyNextWeapon, VK_CONTROL, 18677761 }
};


static GMenu_SingleKeyMapping KeyMapping[NUM_MAPPED_KEYS] =
{
	{ GMenu_KeyShoot, VK_LBUTTON, 1 },
	{ GMenu_KeyJump, VK_RBUTTON, 2 },
	{ GMenu_KeyStrafeLeft, VK_LEFT, 21692417 },
	{ GMenu_KeyStrafeRight, VK_RIGHT, 21823489 },
	{ GMenu_KeyForward, VK_UP, 21495809 },
	{ GMenu_KeyBackward, VK_DOWN, 22020097 },
	{ GMenu_KeyNextWeapon, VK_CONTROL, 18677761 }
};

//undone
int32		KeyLut[256];
geBoolean	MouseInvert = GE_FALSE;
int32		MenuBotCount = 1;

#define GMENU_MAX_DISPLAY_MODES 20
////////////////////////////////////////////////////////////////////////////////////////
//	Globals
////////////////////////////////////////////////////////////////////////////////////////
static Menu_T				*MainMenu = NULL;
static Menu_T				*SingleMapMenu = NULL;
static Menu_T				*OptionsMenu = NULL;
static Menu_T				*BotsMenu = NULL;
static Menu_T				*JoinMenu = NULL;
static Menu_T				*CreateNetMenu = NULL;
static Menu_T				*ControlMenu = NULL;
static Menu_T				*ActiveMenu = NULL;
static Menu_T				*CreditsMenu = NULL;
static Menu_T				*DriverMenu = NULL;
static Menu_T				*ModeMenuList[GMENU_MAX_DISPLAY_MODES];
static geBitmap				*SliderBar = NULL;
static geBitmap				*SliderRange = NULL;
static geBitmap				*CreditsArt = NULL;
static geEngine				*SaveEngine = NULL;
static float				VolumePercent = 0.7f;
static float				BotPercent = 0.25f;

float						UserGamma = 1.0f;



Menu_T *GMenu_GetMenu(int32 Id)
{
	switch(Id)
	{
		case GMenu_NameEntry:
			return OptionsMenu;

		case GMenu_IPEntry:
			return JoinMenu;
	}

	return NULL;	// Not found
}


int MyGetKeyNameText( int lParam, char *KeyText, int Size)
{
	// get name of key that was pressed
	if ( lParam == 1 )
	{
		strcpy( KeyText, "Left Mouse Button" );
	}
	else if ( lParam == 2 )
	{
		strcpy( KeyText, "Right Mouse Button" );
	}
	else
	{
		return (GetKeyNameText( lParam, KeyText, Size));
	}

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_SetKeyMapping()
//
//	Set a new keyboard mapping.
//
////////////////////////////////////////////////////////////////////////////////////////
static geBoolean GMenu_SetKeyMapping(
	int32	Identifier,	// control type whose mapping will be changed
	int32	wParam,		// wParam
	int32	lParam )	// lParam
{
	int32	i;

	// set new mapping
	for ( i = 0; i < NUM_MAPPED_KEYS; i++ )
	{
		if ( KeyMapping[i].Action == Identifier )
		{
			KeyMapping[i].wParam = wParam;
			KeyMapping[i].lParam = lParam;
			return GE_TRUE;
		}
	}

	// if we got to here then no mapping was set
	assert( 0 );
	return GE_FALSE;

} // GMenu_SetKeyMapping()



////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_GetKeyLParam()
//
//	Gets a keys lParam mapping.
//
////////////////////////////////////////////////////////////////////////////////////////
static int32 GMenu_GetKeyLParam(
	int32	Identifier )	// whose lParam we want
{
	int32	i;

	// set new mapping
	for ( i = 0; i < NUM_MAPPED_KEYS; i++ )
	{
		if ( KeyMapping[i].Action == Identifier )
		{
			return KeyMapping[i].lParam;
		}
	}
	return -1;
} // GMenu_GetKeyLParam()

static int32 GMenu_GetOriginalKeyMapping(
	int32	Identifier )	// whose lParam we want
{
	int32	i;

	for ( i = 0; i < NUM_MAPPED_KEYS; i++ )
	{
		if ( OriginalKeyMapping[i].Action == Identifier )
		{
			return OriginalKeyMapping[i].wParam;
		}
	}
	return -1;
} // GMenu_GetOriginalKeyMapping




////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_IsAMenuActive()
//
//	Determines if any menu is currently active.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean GMenu_IsAMenuActive(
	void )	// no parameters
{
	if ( ActiveMenu == NULL )
	{
		return GE_FALSE;
	}
	else
	{
		return GE_TRUE;
	}

} // GMenu_IsAMenuActive()
	
////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_SetActive(geBoolean Active)
//
////////////////////////////////////////////////////////////////////////////////////////
void GMenu_SetActive(geBoolean Active)
{
	if (!Active)
		ActiveMenu = NULL;
	else
		ActiveMenu = MainMenu;
}



static geBoolean GMenu_BuildDisplayModeMenus(geEngine *Engine, 
								ModeList *List, int ListLength, int ListSelection, 
								int Width, int Top, int Bottom)
{
	MenuItemChecked_T	 CheckedItem;
	int					 DriverNumber	= 0;
	int					 i,j;
	int					 DriverCount;
	geDriver			*IndexTable[GMENU_MAX_DISPLAY_MODES];
	int					 CountTable[GMENU_MAX_DISPLAY_MODES];

	assert( List != NULL );
	assert( ListLength >=0 );
	assert( ListSelection >=0 );
	assert( ListSelection < ListLength );

	DriverMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_DriverMenu );
	if ( MainMenu == NULL )
		{
			geErrorLog_AddString(-1,"GMenu_BuildDisplayModeMenus: Failed to create driver menu",NULL);
			return GE_FALSE;
		}

	if (ListLength==0)
		return GE_TRUE;

	DriverNumber = 0;
	for (i=0; i<ListLength; i++)
		{
			int AlreadyAdded=0;
			
			for (j=0; j<i; j++)
				{
					if (List[j].Driver == List[i].Driver)	// only add one entry for each driver.
						AlreadyAdded = 1;
				}
			
			if (!AlreadyAdded)
				{
					ModeMenuList[DriverNumber] = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_ModeMenu+DriverNumber );
					if ( ModeMenuList[DriverNumber] == NULL )
						{
							geErrorLog_AddString(-1,"GMenu_BuildDisplayModeMenus: Failed to create driver menu",NULL);
							return GE_FALSE;
						}
					CheckedItem.NormalFont  = Font_Default;
					CheckedItem.SelectFont  = (Width<SMALL_MENU_CUTOFF_WIDTH)?Font_Small:Font_DefaultSelect;
					strncpy(CheckedItem.Label,(char *)List[i].DriverNamePtr,MENUITEM_MAX_CHECKED_LABEL-1);
					CheckedItem.LabelLength = strlen( CheckedItem.Label );
					CheckedItem.ActiveItem  = List[i].Driver == List[ListSelection].Driver;
					CheckedItem.LabelPosition = GMENU_DRIVER_LEFT_EDGE;
					Menu_AddItem( DriverMenu, Menu_ItemChecked, GMenu_Driver + DriverNumber, &CheckedItem );
					
					IndexTable[DriverNumber] = List[i].Driver;
					CountTable[DriverNumber] = 0;
					DriverNumber ++;
				}
			if (DriverNumber >= GMENU_MAX_DISPLAY_MODES)
				break;
		}

	DriverCount = DriverNumber;
	for (i=0; i<ListLength; i++)
		{
			DriverNumber = -1;
			for (j=0; j<DriverCount; j++)
				{
					if (IndexTable[j] == List[i].Driver)
						{
							DriverNumber = j; 
							break;
						}
				}
			assert( DriverNumber >= 0 );

			CheckedItem.NormalFont  = Font_Default;
			CheckedItem.SelectFont  = Font_DefaultSelect;
			strncpy(CheckedItem.Label,(char *)List[i].ModeNamePtr,MENUITEM_MAX_CHECKED_LABEL-1);
			CheckedItem.LabelLength = strlen( CheckedItem.Label );
			CheckedItem.ActiveItem  = List[i].Mode == List[ListSelection].Mode;
			CheckedItem.LabelPosition = Width/4;
			CheckedItem.UserData    = (int32) i;
			Menu_AddItem( ModeMenuList[DriverNumber], Menu_ItemChecked, GMenu_Mode + CountTable[DriverNumber], &CheckedItem );
			(CountTable[DriverNumber])++;
		}	
	
	return GE_TRUE;
}



////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_Create()
//
//	Create all required game menus.
//
////////////////////////////////////////////////////////////////////////////////////////
extern geVFile * MainFS;
geBoolean GMenu_Create(
	geEngine	  *Engine,
	ModeList	  *List,
	int			   ListLength,
	int			   ListSelection)
{

	// locals
	MenuItemText_T		TextItem;
	MenuItemSlider_T	SliderItem;
	MenuItemSlider_T	SaveSliderItem;
	MenuItemField_T		FieldItem;
	MenuItemString_T	StringItem;
	MenuItemGraphic_T	GraphicItem;
	MenuItemToggle_T	ToggleItem;
	char				KeyText[64];
	int32				i;
	int32				Width, Top, Bottom;
	extern Host_Init		HostInit;
	geBoolean			success;

	assert( List != NULL );
	assert( ListLength >=0 );
	assert( ListSelection >=0 );
	assert( ListSelection < ListLength );

	// fail if we have bad data
	if ( Engine == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	SaveEngine = Engine;

	if (List[ListSelection].Width<SMALL_CREDITS_CUTOFF_WIDTH)
		{
				//load decal art
			CreditsArt = geBitmap_CreateFromFileName(MainFS, "Bmp\\Menu\\Credits0.Bmp");
			
			if (!CreditsArt)
			{
				geErrorLog_AddString(-1, "GMenu_Create:  geBitmap_CreateFromFile failed:", "Bmp\\Menu\\Credits0.Bmp");
				goto ExitWithError;
			}

			Width = List[ListSelection].Width;
			Top = 0; 
			Bottom = List[ListSelection].Height - 1 - 30;
		}
	else
		{
			//load decal art
			CreditsArt = geBitmap_CreateFromFileName(MainFS, "Bmp\\Menu\\Credits1.Bmp");

			if (!CreditsArt)
			{
				geErrorLog_AddString(-1, "GMenu_Create:  geBitmap_CreateFromFile failed:","Bmp\\Menu\\Credits1.Bmp");
				goto ExitWithError;
			}

			Width = List[ListSelection].Width;
			Top = 0;
			Bottom = List[ListSelection].Height - 1 - 60;
		}

	// Add the bitmap to the world
	if (!geEngine_AddBitmap(Engine, CreditsArt))
	{
		geErrorLog_AddString(-1, "GMenu_Create:  geEngine_AddBitmap failed for CreditsArt.", NULL);
		goto ExitWithError;
	}

	// init keyboard lookup table
	for ( i = 0; i < 256; i++ )
	{
		KeyLut[i] = i;
	}

	{
		FILE	*f;

		f = fopen("GMenu.Dat", "rb");

		if (f)
		{
			int	Version;

			fread(&Version, sizeof(Version), 1, f);

			if (Version == GMENU_VERSION)
			{
				// MLS: no error handling!
				fread(&(KeyMapping[0].Action), sizeof(KeyMapping), 1, f);
				fread(KeyLut, sizeof(KeyLut), 1, f);
				fread(&MouseInvert, sizeof(MouseInvert), 1, f);
                fread(&MenuBotCount, sizeof(MenuBotCount), 1, f);
				if (MenuBotCount > 4)
					MenuBotCount = 4;
			}

			fclose(f);
		}
	}

	// load slider art
	SliderBar = geBitmap_CreateFromFileName(MainFS, "Bmp\\Menu\\SliderB.Bmp");
	success = geBitmap_SetColorKey(SliderBar,GE_TRUE,255,GE_FALSE);
	assert(success);

	if (!SliderBar)
	{
		geErrorLog_AddString(-1, "GMenu_Create:  geBitmap_CreateFromFile failed:","Bmp\\Menu\\SliderB.Bmp");
		goto ExitWithError;
	}

	if (!geEngine_AddBitmap(Engine, SliderBar))
	{
		geErrorLog_AddString(-1, "GMenu_Create:  geEngine_AddBitmap failed:", "Bmp\\Menu\\SliderB.Bmp");
		goto ExitWithError;
	}

	SliderRange = geBitmap_CreateFromFileName(MainFS, "Bmp\\Menu\\SliderR.Bmp");
	success = geBitmap_SetColorKey(SliderRange,GE_TRUE,255,GE_FALSE);
	assert(success);

	if (!SliderRange)
	{
		geErrorLog_AddString(-1, "GMenu_Create:  geBitmap_CreateFromFile failed:", "Bmp\\Menu\\SliderR.Bmp");
		goto ExitWithError;
	}

	if (!geEngine_AddBitmap(Engine, SliderRange))
	{
		geErrorLog_AddString(-1, "GMenu_Create:  geEngine_AddBitmap failed:", "Bmp\\Menu\\SliderR.Bmp");
		goto ExitWithError;
	}

	// init common item data
	TextItem.NormalFont = Font_Default;
	TextItem.SelectFont = Font_DefaultSelect;
	SliderItem.NormalFont = Font_Default;
	SliderItem.SelectFont = Font_DefaultSelect;
	SliderItem.Range = SliderRange;
	SliderItem.Bar = SliderBar;
	SliderItem.Slack = 14;
	SliderItem.Percent = 0.7f;
    SliderItem.Increment = 0.05f;
	FieldItem.NormalFont = Font_Default;
	FieldItem.SelectFont = Font_DefaultSelect;
	FieldItem.AwaitingChange = GE_FALSE;
	StringItem.NormalFont = Font_Default;
	StringItem.SelectFont = Font_DefaultSelect;
	StringItem.AwaitingInput = GE_FALSE;

	//
	// create main menu
	//


	MainMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_MainMenu );
	if ( MainMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}


	// add items to the menu
	TextItem.Text = "Single Player Game";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( MainMenu, Menu_ItemText, GMenu_SinglePlayerGame, &TextItem );

	TextItem.Text = "Create Multi Player Game";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( MainMenu, Menu_ItemText, GMenu_CreateMultiPlayerGame, &TextItem );

	TextItem.Text = "Join Multi Player Game";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( MainMenu, Menu_ItemText, GMenu_JoinMultiPlayerGame, &TextItem );

	TextItem.Text = "Options";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( MainMenu, Menu_ItemText, GMenu_Options, &TextItem );

	TextItem.Text = "Credits";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( MainMenu, Menu_ItemText, GMenu_Credits, &TextItem );

	TextItem.Text = "Quit";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( MainMenu, Menu_ItemText, GMenu_QuitGame, &TextItem );


	//
	// create main menu
	//
	SingleMapMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_SingleMapMenu );
	if ( SingleMapMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

#ifdef BOT_UPDATE
	if (HostInit.UserLevel[0])
		{
		// add items to the menu
		static char Name[256];

		if (!strstr(HostInit.UserLevel,"Levels\\"))
			assert(0); // didn't find prefix

		sprintf(Name, "User Level %s", &HostInit.UserLevel[strlen("Levels\\")]);
		TextItem.Text = Name;
		TextItem.TextLength = strlen( TextItem.Text );
		Menu_AddItem( SingleMapMenu, Menu_ItemText, GMenu_UserSinglePlayerGame, &TextItem );
		}

	// add items to the menu
	TextItem.Text = "The Mine";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( SingleMapMenu, Menu_ItemText, GMenu_SinglePlayerGame1, &TextItem );

	// add items to the menu
	TextItem.Text = "Physics Mania";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( SingleMapMenu, Menu_ItemText, GMenu_SinglePlayerGame2, &TextItem );

	/*
	// add items to the menu
	TextItem.Text = "The Gallery";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( SingleMapMenu, Menu_ItemText, GMenu_SinglePlayerGame3, &TextItem );
	*/

	//
	// create bots menu
	//
	BotsMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_BotsMenu );
	if ( BotsMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	
	
	// add items to the menu
	TextItem.Text = "Start Game";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( BotsMenu, Menu_ItemText, GMenu_Bots, &TextItem );

	// add items to the menu
    SaveSliderItem = SliderItem;
    SliderItem.Increment = 1.0f/4;
	{ //{}
	static char TargetText[20];
	SliderItem.Text = TargetText;
	}
	assert( MenuBotCount <= 99 && MenuBotCount >= 0);
	sprintf(SliderItem.Text, "Bots %2d \0",MenuBotCount);
	SliderItem.Percent = BotPercent = (MenuBotCount * SliderItem.Increment);
	SliderItem.TextLength = strlen( SliderItem.Text );
	Menu_AddItem( BotsMenu, Menu_ItemSlider, GMenu_BotSlider, &SliderItem );
    SliderItem = SaveSliderItem;

#else
	// add items to the menu
	TextItem.Text = "The Mine";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( SingleMapMenu, Menu_ItemText, GMenu_SinglePlayerGame1, &TextItem );

	// add items to the menu
	TextItem.Text = "Physics Mania";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( SingleMapMenu, Menu_ItemText, GMenu_SinglePlayerGame2, &TextItem );
#endif



	//
	// create options menu
	//
	OptionsMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_OptionsMenu );
	if ( OptionsMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	// add items to the menu

	TextItem.Text = "Display Mode ";
	TextItem.TextLength = strlen(TextItem.Text);
	Menu_AddItem( OptionsMenu, Menu_ItemText, GMenu_DisplayMode, &TextItem );

	if (GMenu_BuildDisplayModeMenus(Engine,List,ListLength,ListSelection,Width,Top,Bottom)==GE_FALSE)
		{
			geErrorLog_AddString(-1,"Failed to build display mode menus",TextItem.Text);
			goto ExitWithError;
		}

	SliderItem.Text = "Volume ";
	SliderItem.TextLength = strlen( SliderItem.Text );
	Menu_AddItem( OptionsMenu, Menu_ItemSlider, GMenu_Volume, &SliderItem );

	StringItem.StringLabel = "Player Name: ";
	StringItem.StringLabelLength = strlen( StringItem.StringLabel );
	//strcpy( StringItem.StringData, PlayerName);
	strcpy( StringItem.StringData, "Unknown");
	StringItem.StringDataLength = strlen( StringItem.StringData );
	Menu_AddItem( OptionsMenu, Menu_ItemString, GMenu_NameEntry, &StringItem );

	SliderItem.Text = "Brightness ";
	SliderItem.TextLength = strlen( SliderItem.Text );
#if 1 // <>
	SliderItem.Percent = UserGamma / GAMMA_MAXIMUM;
#endif
	Menu_AddItem( OptionsMenu, Menu_ItemSlider, GMenu_Brightness, &SliderItem );

	TextItem.Text = "Customize Controls";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( OptionsMenu, Menu_ItemText, GMenu_Control, &TextItem );

//#ifndef BOT_UPDATE
    SaveSliderItem = SliderItem;
    SliderItem.Increment = 1.0f/4;
	{ //{}
	static char TargetText2[20];
	SliderItem.Text = TargetText2;
	}
	assert( MenuBotCount <= 99 && MenuBotCount >= 0);
	sprintf(SliderItem.Text, "Bots %2d \0",MenuBotCount);
	SliderItem.Percent = BotPercent = (MenuBotCount * SliderItem.Increment);
	SliderItem.TextLength = strlen( SliderItem.Text );
	Menu_AddItem( OptionsMenu, Menu_ItemSlider, GMenu_BotSlider, &SliderItem );
    SliderItem = SaveSliderItem;
//#endif

	ToggleItem.NormalFont = Font_Default;
	ToggleItem.SelectFont = Font_DefaultSelect;
	ToggleItem.ToggleLabel = "Frame Statistics  ";
	ToggleItem.ToggleLabelLength = strlen( ToggleItem.ToggleLabel );
	strcpy( ToggleItem.ToggleData1, "Off" );
	ToggleItem.ToggleData1Length = strlen( ToggleItem.ToggleData1 );
	strcpy( ToggleItem.ToggleData2, "On" );
	ToggleItem.ToggleData2Length = strlen( ToggleItem.ToggleData2 );
	ToggleItem.ActiveItem = ShowStats;
	Menu_AddItem( OptionsMenu, Menu_ItemToggle, GMenu_ShowStatistics, &ToggleItem );

	//
	// create join menu
	//
	JoinMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_JoinMenu );
	if ( JoinMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	StringItem.StringLabel = "IP Address: ";
	StringItem.StringLabelLength = strlen( StringItem.StringLabel );
	//strcpy( StringItem.StringData, IPAddress);
	strcpy( StringItem.StringData, "");
	StringItem.StringDataLength = strlen( StringItem.StringData );
	Menu_AddItem( JoinMenu, Menu_ItemString, GMenu_IPEntry, &StringItem );
	
	TextItem.Text = "Connect";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( JoinMenu, Menu_ItemText, GMenu_Connect, &TextItem );

	TextItem.Text = "   ";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( JoinMenu, Menu_ItemText, 0xffff, &TextItem );

	TextItem.Text = "Enter an IP address";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( JoinMenu, Menu_ItemText, 0xffff, &TextItem );

	TextItem.Text = "Leave blank for a LAN game";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( JoinMenu, Menu_ItemText, 0xffff, &TextItem );

	//
	// create create menu
	//
	CreateNetMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_CreateMenu );
	if ( CreateNetMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	StringItem.StringLabel = "IP Address: ";
	StringItem.StringLabelLength = strlen( StringItem.StringLabel );

	strcpy(StringItem.StringData, "Not available");
	
	StringItem.StringDataLength = strlen( StringItem.StringData );

	Menu_AddItem( CreateNetMenu, Menu_ItemString, GMenu_IPAddress, &StringItem );
	/*
	TextItem.Text = "Start Game";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( CreateNetMenu, Menu_ItemText, GMenu_StartGame, &TextItem );
	*/
	TextItem.Text = "The Mine";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( CreateNetMenu, Menu_ItemText, GMenu_StartGame1, &TextItem );

	TextItem.Text = "Physics Mania";
	TextItem.TextLength = strlen( TextItem.Text );
	Menu_AddItem( CreateNetMenu, Menu_ItemText, GMenu_StartGame2, &TextItem );

	//
	// create customize control menu
	//
	ControlMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_ControlMenu );
	if ( ControlMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	// add items to the menu
	FieldItem.FieldName = "Shoot  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyShoot ), KeyText, sizeof( KeyText ) );
  	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyShoot, &FieldItem );

	FieldItem.FieldName = "Jump  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyJump ), KeyText, sizeof( KeyText ) );
	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyJump, &FieldItem );

	FieldItem.FieldName = "Strafe Left  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyStrafeLeft ), KeyText, sizeof( KeyText ) );
	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyStrafeLeft, &FieldItem );

	FieldItem.FieldName = "Strafe Right  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyStrafeRight ), KeyText, sizeof( KeyText ) );
	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyStrafeRight, &FieldItem );

	FieldItem.FieldName = "Forward  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyForward ), KeyText, sizeof( KeyText ) );
	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyForward, &FieldItem );

	FieldItem.FieldName = "Backward  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyBackward ), KeyText, sizeof( KeyText ) );
	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyBackward, &FieldItem );

	FieldItem.FieldName = "Next Weapon  ";
	FieldItem.FieldNameLength = strlen( FieldItem.FieldName );
	MyGetKeyNameText( GMenu_GetKeyLParam( GMenu_KeyNextWeapon ), KeyText, sizeof( KeyText ) );
	strcpy( FieldItem.FieldTextData, KeyText );
	FieldItem.FieldTextDataLength = strlen( FieldItem.FieldTextData );
	Menu_AddItem( ControlMenu, Menu_ItemField, GMenu_KeyNextWeapon, &FieldItem );

	ToggleItem.NormalFont = Font_Default;
	ToggleItem.SelectFont = Font_DefaultSelect;
	ToggleItem.ToggleLabel = "Mouse Invert  ";
	ToggleItem.ToggleLabelLength = strlen( ToggleItem.ToggleLabel );
	strcpy( ToggleItem.ToggleData1, "No" );
	ToggleItem.ToggleData1Length = strlen( ToggleItem.ToggleData1 );
	strcpy( ToggleItem.ToggleData2, "Yes" );
	ToggleItem.ToggleData2Length = strlen( ToggleItem.ToggleData2 );
	ToggleItem.ActiveItem = MouseInvert;
	Menu_AddItem( ControlMenu, Menu_ItemToggle, GMenu_MouseInvert, &ToggleItem );



	//
	// create credits menu
	//
	CreditsMenu = Menu_Create( Engine, Width, Top, Bottom, 0, 0, GMenu_CreditsMenu );
	if ( CreditsMenu == NULL )
	{
		assert( 0 );
		goto ExitWithError;
	}

	// add items to the menu
	GraphicItem.Art = CreditsArt;
	GraphicItem.x = 0;
	GraphicItem.y = 0;
	Menu_AddItem( CreditsMenu, Menu_ItemGraphic, GMenu_NoIdRequired, &GraphicItem );



	// all done
	ActiveMenu = MainMenu;
	return GE_TRUE;

	ExitWithError:
	{
		GMenu_DestroyAllData();
		return GE_FALSE;
	}

} // GMenu_Create()



////////////////////////////////////////////////////////////////////////////////////////
//	GMenu_DestroyAllData
////////////////////////////////////////////////////////////////////////////////////////
void GMenu_DestroyAllData(void)
{
	// free all decals
	if ( SliderBar != NULL )
	{
		assert(SaveEngine);
		geEngine_RemoveBitmap( SaveEngine, SliderBar );
		geBitmap_Destroy(&SliderBar);
		SliderBar = NULL;
	}
	if ( SliderRange != NULL )
	{
		assert(SaveEngine);
		geEngine_RemoveBitmap( SaveEngine, SliderRange );
		geBitmap_Destroy(&SliderRange);
		SliderRange = NULL;
	}
	if ( CreditsArt != NULL )
	{
		assert(SaveEngine);
		geEngine_RemoveBitmap( SaveEngine, CreditsArt );
		geBitmap_Destroy(&CreditsArt);
		CreditsArt = NULL;
	}

	// Destroy all menus
	if (MainMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &MainMenu );
		MainMenu = NULL;
	}

	if (SingleMapMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &SingleMapMenu );
		SingleMapMenu = NULL;
	}

	if (BotsMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &BotsMenu );
		BotsMenu = NULL;
	}

	if (OptionsMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &OptionsMenu );
		OptionsMenu = NULL;
	}

	if (JoinMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &JoinMenu );
		JoinMenu = NULL;
	}

	if (CreateNetMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &CreateNetMenu );
		CreateNetMenu = NULL;
	}

	if (ControlMenu)
	{
		assert(SaveEngine);
		Menu_Destroy( &ControlMenu );
		ControlMenu = NULL;
	}

	SaveEngine = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_Destroy()
//
//	Destroy all game menus.
//
////////////////////////////////////////////////////////////////////////////////////////
void GMenu_Destroy(
	void )	// no parameters
{

	{
		FILE	*f;

		f = fopen("GMenu.Dat", "wb");

		if (f)
		{
			int	Version;

			Version = GMENU_VERSION;

			fwrite(&Version, sizeof(Version), 1, f);
			fwrite(&KeyMapping[0].Action, sizeof(KeyMapping), 1, f);
			fwrite(KeyLut, sizeof(KeyLut), 1, f);
			fwrite(&MouseInvert, sizeof(MouseInvert), 1, f);
            fwrite(&MenuBotCount, sizeof(MenuBotCount), 1, f);
            fclose(f);
		}
	}

	GMenu_DestroyAllData();

} // GMenu_Destroy()



////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_Draw()
//
//	Draw any active game menus.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean GMenu_Draw(
	void )	// no parameters
{

	// draw menu if required
	if ( ActiveMenu == NULL )
	{
		return GE_FALSE;
	}
	Menu_Draw( ActiveMenu );

	// all done
	return GE_TRUE;

} // GMenu_Draw()

////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu_Key()
//
//	Send a keystroke to a menu.
//
////////////////////////////////////////////////////////////////////////////////////////
int32 GMenu_Key(
	int32	wParam,		// wParam
	int32	lParam )	// lParam
{

	// locals
	static int32	ControlChangeIdentifier = -1;
	static int32	NameChangeIdentifier = -1;
	static int32	Identifier = -1;
	static int32	OldIdentifier = -1;
	static int32	LastReturnIdentifier = -1;
	int32			Result = GMenu_DoNothing;

	// if a string is waiting for input then process it
	if ( NameChangeIdentifier != -1 )
	{

		// locals
		geBoolean	Change;
		char		TempString[MENU_MAXSTRINGSIZE];

		// skip ESC
		//if ( wParam == VK_ESCAPE )
		//{
		//	return Result;
		//}

		Change = Menu_GetStringText( ActiveMenu, NameChangeIdentifier, TempString );
		assert( Change == GE_TRUE );

		// stop name entry if enter is pressed
		if ( wParam == VK_RETURN  || wParam == VK_ESCAPE )
		{
			// locals
			int32	Size;

			Size = strlen( TempString );

			TempString[Size-1] = '\0';

			Change = Menu_SetStringText( ActiveMenu, NameChangeIdentifier, TempString );
			assert( Change == GE_TRUE );

			Change = Menu_FlagString( ActiveMenu, NameChangeIdentifier, GE_FALSE );
			assert( Change == GE_TRUE );
			NameChangeIdentifier = -1;
			return Result;
		}

		// delete a character...
		if ( wParam == VK_BACK )
		{

			// locals
			int32	Size;

			// do nothing if there are no more characters left
			Size = strlen( TempString );
			if ( Size < 2 )
			{
				return Result;
			}

			// delete as character
			TempString[Size-2] = 0x40;
			TempString[Size-1] = '\0';
		}
		// ...or add one
		else
		{

			// locals
			int32	Size;

			// do nothing is there is no more room to add characters
			Size = strlen( TempString );
			if ( Size+2 >= sizeof( TempString ) )
			{
				return Result;
			}
			TempString[Size-1] = (char)MapVirtualKey(wParam, 2);
			TempString[Size] = 0x40;
			TempString[Size+1] = '\0';
		}

		// set new string text
		Change = Menu_SetStringText( ActiveMenu, NameChangeIdentifier, TempString );
		assert( Change == GE_TRUE );

		// all done
		return Result;
	}
	// if a field is waiting for input then process it
	else if ( ControlChangeIdentifier != -1 )
	{

		// locals
		geBoolean	Change;
		char		KeyText[64];

		// skip ESC
		if ( wParam == VK_ESCAPE )
		{
			return Result;
		}

		MyGetKeyNameText( lParam, KeyText, sizeof( KeyText ) );

		// set new field text
		Change = Menu_SetFieldText( ActiveMenu, ControlChangeIdentifier, KeyText );
		assert( Change == GE_TRUE );

		// set new key mapping
		KeyLut[GMenu_GetOriginalKeyMapping(ControlChangeIdentifier)] = wParam;
		GMenu_SetKeyMapping( ControlChangeIdentifier, wParam, lParam );

		// all done
		Change = Menu_FlagField( ActiveMenu, ControlChangeIdentifier, GE_FALSE );
		assert( Change == GE_TRUE );
		ControlChangeIdentifier = -1;
		return Result;
	}

	// if ESC was hit then back out of the menus as required
	if ( wParam == VK_ESCAPE )
	{

		// locals
		int32	MenuIdentifier;

		// get menu identifier
		if ( ActiveMenu == NULL )
		{
			MenuIdentifier = GMenu_NoMenu;
		}
		else
		{
			MenuIdentifier = Menu_GetIdentifier( ActiveMenu );
		}

		// decide what to do based on currently active menu
		switch ( MenuIdentifier )
		{

			case GMenu_MainMenu:
			{
				ActiveMenu = NULL;
				break;
			}

			case GMenu_ControlMenu:
			case GMenu_DriverMenu:
			{
				ActiveMenu = OptionsMenu;
				break;
			}

			case GMenu_CreditsMenu:
			{
				Host_ClientRefreshStatusBar(0);
				ActiveMenu = MainMenu;
				break;
			}

			case GMenu_OptionsMenu:
			case GMenu_NoMenu:
			case GMenu_CreateMenu:
			case GMenu_SingleMapMenu:
			case GMenu_JoinMenu:
			{
				ActiveMenu = MainMenu;
				break;
			}

			default:
			{
				if (MenuIdentifier>=GMenu_ModeMenu && MenuIdentifier<GMenu_ModeMenu+1000)
					ActiveMenu = DriverMenu;
				else
					ActiveMenu = MainMenu;
				break;
			}

		}
		return Result;
	}

	// do nothing if no menu is currently active
	if ( ActiveMenu == NULL )
	{
		return Result;
	}

	// send keystroke to currently active menu
	Identifier = Menu_Key( ActiveMenu, wParam );

	// process keystroke
	switch ( wParam )
	{

		// process sliders
		case VK_LEFT:
		case VK_RIGHT:
		{
			switch ( Identifier )
			{

				// volume slider
				case GMenu_Volume:
				{

					// locals
					float	Percent;

					// get volume percent
					Percent = Menu_GetSliderPercent( ActiveMenu, Identifier );
					assert( Percent >= 0.0f );
					assert( Percent <= 1.0f );

					// only change volume if percentage has changed
					if ( Percent != VolumePercent )
					{
						VolumePercent = Percent;
						VolumePercent *= 0.4f;
						VolumePercent += 0.6f;
						//undone, set the volume here
						if (SoundSys)
							geSound_SetMasterVolume(SoundSys, VolumePercent);
					}
					break;
				}

				case GMenu_BotSlider:
				{
					// locals
					float	Percent;
                    char NewString[32];
					Menu_T *PiggyBackMenu = NULL;

					if (ActiveMenu == OptionsMenu)
						PiggyBackMenu = BotsMenu; //also set slider in bots menu
					else
					if (ActiveMenu == BotsMenu)
						PiggyBackMenu = OptionsMenu; //also set slider in options menu

					// get volume percent
					Percent = Menu_GetSliderPercent( ActiveMenu, Identifier );
					assert( Percent >= 0.0f );
					assert( Percent <= 1.0f );


					// only change Bot if percentage has changed
					if ( Percent != BotPercent )
					{
						BotPercent = Percent;
						MenuBotCount = (int32)(BotPercent * 4.0f);
					}

                    sprintf(NewString, "Bots %2d ", MenuBotCount);

                    Menu_SetSliderText(ActiveMenu, Identifier, (char*)NewString);

					if (PiggyBackMenu)
						{
						// Piggy back the results
	                    Menu_SetSliderText(PiggyBackMenu, Identifier, (char*)NewString);
	                    Menu_SetSliderPercent(PiggyBackMenu, Identifier, Percent);
						}
					break;
				}

				// gamma slider
				case GMenu_Brightness:
				{
					float	Percent;

					// get gamma percent
					Percent = Menu_GetSliderPercent( ActiveMenu, Identifier );
					assert( Percent >= 0.0f );
					assert( Percent <= 1.0f );

					UserGamma = Percent * GAMMA_MAXIMUM;

					// engine won't do anything unless gamma is a change
					if (SaveEngine)
						geEngine_SetGamma(SaveEngine, UserGamma );
					break;
				}
			}
			break;
		}

		// if enter was hit then check if we need to do anything
		case VK_RETURN:
		{
		switch ( Identifier )
			{
				case GMenu_SinglePlayerGame:
				{
					ActiveMenu = SingleMapMenu;
					break;
				}
				
				case GMenu_QuitGame:
				{
					Result = Identifier;
					ActiveMenu = NULL;
					break;
				}

				case GMenu_CreateMultiPlayerGame:
				{
					geBoolean	Change;
					char		TempString[MENU_MAXSTRINGSIZE];

					ActiveMenu = CreateNetMenu;

					if (IPAddr_GetHostID(TempString))
					{
						Change = Menu_SetStringText( ActiveMenu, GMenu_IPAddress, TempString );
					}
					else
					{
						Change = Menu_SetStringText( ActiveMenu, GMenu_IPAddress, "Not Available");
					}

					assert(Change == GE_TRUE);

					break;
				}

				case GMenu_JoinMultiPlayerGame:
				{
					ActiveMenu = JoinMenu;
					break;
				}

				case GMenu_Connect:
				{
					Result = Identifier;
					ActiveMenu = NULL;
					break;
				}

#ifdef BOT_UPDATE
				case GMenu_Bots:
				{
					switch (LastReturnIdentifier)
					{
						case GMenu_UserSinglePlayerGame:
						case GMenu_SinglePlayerGame1:
						case GMenu_SinglePlayerGame2:
						case GMenu_SinglePlayerGame3:
							Result = LastReturnIdentifier;
							ActiveMenu = NULL;
							break;
						case -1:
							Result = GMenu_UserSinglePlayerGame;
							ActiveMenu = NULL;
							break;
					}
						break;
				}

				case GMenu_UserSinglePlayerGame:
				case GMenu_SinglePlayerGame1:
				case GMenu_SinglePlayerGame2:
				case GMenu_SinglePlayerGame3:
				{
					LastReturnIdentifier = Identifier;
					Result = GMenu_DoNothing;//Identifier;
					ActiveMenu = BotsMenu;
					break;
				}

				case GMenu_StartGame:
				case GMenu_StartGame1:
				case GMenu_StartGame2:
				{
					Result = Identifier;
					ActiveMenu = NULL;
					break;
				}
#else
				case GMenu_StartGame:
				case GMenu_StartGame1:
				case GMenu_StartGame2:
				case GMenu_UserSinglePlayerGame:
				case GMenu_SinglePlayerGame1:
				case GMenu_SinglePlayerGame2:
				{
					Result = Identifier;
					ActiveMenu = NULL;
					break;
				}
#endif

				case GMenu_Options:
				{
					ActiveMenu = OptionsMenu;
					break;
				}


				case GMenu_Control:
				{
					ActiveMenu = ControlMenu;
					break;
				}

				case GMenu_Credits:
				{
					ActiveMenu = CreditsMenu;
					break;
				}

				case GMenu_KeyShoot:
				case GMenu_KeyJump:
				case GMenu_KeyStrafeLeft:
				case GMenu_KeyStrafeRight:
				case GMenu_KeyForward:
				case GMenu_KeyBackward:
				case GMenu_KeyNextWeapon:
				{
					Menu_FlagField( ActiveMenu, Identifier, GE_TRUE );
					ControlChangeIdentifier = Identifier;
					break;
				}

				case GMenu_NameEntry:
				{
					Menu_FlagString( ActiveMenu, Identifier, GE_TRUE );
					NameChangeIdentifier = Identifier;
					break;
				}

				case GMenu_IPEntry:
				{
					//char TempString[MENU_MAXSTRINGSIZE];
					
					Menu_FlagString( ActiveMenu, Identifier, GE_TRUE );
					NameChangeIdentifier = Identifier;
					
					/*
					assert(Menu_GetStringText( ActiveMenu, NameChangeIdentifier, TempString ));
					if (strlen(TempString) <= 0)
					{
						strcpy(TempString, "Search Local LAN");
						assert(Menu_SetStringText( ActiveMenu, NameChangeIdentifier, TempString ));
					}
					*/
					break;
				}

				case GMenu_MouseInvert:
				{
					Menu_ToggleItem( ActiveMenu, Identifier );
					MouseInvert = !MouseInvert;
					break;
				}
				case GMenu_ShowStatistics:
				{
					Menu_ToggleItem( ActiveMenu, Identifier );
					if ( ShowStats ) ShowStats = GE_FALSE;
					else ShowStats = GE_TRUE;
					geEngine_EnableFrameRateCounter(Engine, ShowStats);		
					break;
				}
				case GMenu_DisplayMode:
				{
					ActiveMenu = DriverMenu;
					break;
				}
				default:
					if (Identifier>=GMenu_Driver && Identifier<GMenu_Mode)
						{
							ActiveMenu = ModeMenuList[Identifier - GMenu_Driver];
							break;
						}
					else if (Identifier>=GMenu_Mode && Identifier<GMenu_ModeMenu)
						{
							if (Menu_GetCheckedData(ActiveMenu,Identifier,(int32 *)(&ChangeDisplaySelection))==GE_FALSE)
								{
									// error
									break;
								}
							else
								{
									ChangingDisplayMode = GE_TRUE;
									ActiveMenu = NULL;
								}
							break;
						}
			}
			// save the current id
			break;
		}
	}

	OldIdentifier = Identifier;

	// all done
	return Result;

} // GMenu_Key()
