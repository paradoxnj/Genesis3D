////////////////////////////////////////////////////////////////////////////////////////
//
//	GMenu.h
//
//	Include for GMenu.c
//
////////////////////////////////////////////////////////////////////////////////////////
#ifndef GMENU_H
#define GMENU_H

#include "Genesis.h"

#include "Menu.h"

#include "ModeList.h"

#define BOT_UPDATE 


////////////////////////////////////////////////////////////////////////////////////////
//	Menu ID's
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
//	Menu item ID's in hierarchy order
////////////////////////////////////////////////////////////////////////////////////////
enum
{
	GMenu_NoMenu = 0,
	GMenu_DoNothing,
	GMenu_MainMenu,

	GMenu_SingleMapMenu,
		GMenu_UserSinglePlayerGame,
		GMenu_SinglePlayerGame,
		GMenu_SinglePlayerGame1,
		GMenu_SinglePlayerGame2,
		GMenu_SinglePlayerGame3,
		#ifdef BOT_UPDATE
		GMenu_Bots,
			GMenu_BotsMenu,
		#endif

	GMenu_CreateMultiPlayerGame,
		GMenu_CreateMenu,
			GMenu_IPAddress,
			GMenu_StartGame,
			GMenu_StartGame1,
			GMenu_StartGame2,

	GMenu_JoinMultiPlayerGame,
		GMenu_JoinMenu,
			GMenu_IPEntry,
			GMenu_Connect,


	GMenu_Options,
		GMenu_OptionsMenu,
			GMenu_DisplayMode,
			GMenu_Volume,
			GMenu_Brightness,
			GMenu_NameEntry,
			GMenu_Control,
				GMenu_ControlMenu,
					GMenu_KeyShoot,
					GMenu_KeyJump,
					GMenu_KeyStrafeLeft,
					GMenu_KeyStrafeRight,
					GMenu_KeyForward,
					GMenu_KeyBackward,
					GMenu_KeyNextWeapon,
					GMenu_MouseInvert,
            GMenu_BotSlider,
            GMenu_ShowStatistics,
	GMenu_Credits,
		GMenu_CreditsMenu,

	GMenu_DriverMenu,
	GMenu_QuitGame,

	
	GMenu_Driver=1000,		
		// id space between these identifiers reserved 
	GMenu_Mode=2000,
		// id space between these identifiers reserved 
	GMenu_ModeMenu=3000,
		// id space between these identifiers reserved 
	GMenu_NoIdRequired=4000,

};


////////////////////////////////////////////////////////////////////////////////////////
//	Prototypes
////////////////////////////////////////////////////////////////////////////////////////

Menu_T *GMenu_GetMenu(int32 Id);

//	Determines if any menu is currently active.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean GMenu_IsAMenuActive(
	void );	// no parameters

//	Create all required game menus.
//
////////////////////////////////////////////////////////////////////////////////////////
void GMenu_SetActive(geBoolean Active);

geBoolean GMenu_Create(
	geEngine	  *Engine,
	ModeList	  *List,
	int			   ListLength,
	int			   ListSelection);

void GMenu_DestroyAllData(void);

//	Destroy all game menus.
//
////////////////////////////////////////////////////////////////////////////////////////
void GMenu_Destroy(
	void );	// no parameters

//	Draw any active game menus.
//
////////////////////////////////////////////////////////////////////////////////////////
geBoolean GMenu_Draw(
	void );	// no parameters

//	Send a keystroke to a menu.
//
////////////////////////////////////////////////////////////////////////////////////////
int32 GMenu_Key(
	int32	Key,		// keystroke input
	int32	lParam );	// lParam

// FIXME:  Do somthing with these...
extern float			UserGamma;

#endif
