/****************************************************************************************/
/*  Console.h                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description:                                                                        */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "GENESIS.H"

#define SMALL_CONSOLE_CUTOFF_WIDTH (640)

#include "VidMode.h"

	//================================================================================
	//================================================================================
#define TEXT_BUFFER_SIZE			4096			// 4k text buffer should do it...
#define TOKEN_BUFFER_SIZE			1024	

#define MAX_HEADER_TEXT_SIZE		128
#define MAX_HEADER_STRINGS			4
#define HEADER_STAY_TIME			5.0f

	typedef struct
	{
		geBoolean		Active;							// GE_TRUE is console is down..
		geBoolean		Fast;							// Fast movement

		geEngine		*Engine;

		// Screen info
		VidMode			VidMode;

		// Info about how to display console
		float			DrawYPos;

		// Text buffer info
		char			TokenBuffer[TOKEN_BUFFER_SIZE];
		char			TextBuffer[TEXT_BUFFER_SIZE];
		int32			PrintPos;						// Current print pos
		int32			CursorPos;						// Where text is current being inserted
		int32			CursorX;						// Where X and y is on the screen
		int32			CursorY;

		int32			BackScroll;
		int32			Scroll;

		int32			StopX;
		int32			StopY;

		geBitmap		*BGBitmap;

		// Font
		int32			FontWidth;
		int32			FontHeight;
		int32			FontLUT1[128];
		geBitmap		*FontBitmap;

		char			HeaderText[MAX_HEADER_STRINGS][MAX_HEADER_TEXT_SIZE];
		float			HeaderTime[MAX_HEADER_STRINGS];
		int32			CurrentHeader;

		// User data
		void			*UserData;

	} Console_Console;

	//================================================================================
	//================================================================================
	Console_Console *Console_Create(geEngine *Engine, VidMode VidMode);
	void			Console_Destroy(Console_Console *Console);
	void			Console_FreeResources(Console_Console *Console);
	geBoolean		Console_Printf(Console_Console *Console, const char *Str, ...);
	geBoolean		Console_HeaderPrintf(Console_Console *Console, const char *Str, ...);
	geBoolean		Console_XYPrintf(Console_Console *Console, int32 x, int32 y, uint32 Flags, const char *Str, ...);
	geBoolean		Console_ToggleActive(Console_Console *Console);
	void			Console_GetActive(Console_Console *Console, geBoolean *Active, geBoolean *Fast);
	void			Console_SetActive(Console_Console *Console, geBoolean Active, geBoolean Fast);
	geBoolean		Console_KeyDown(Console_Console *Console, int32 Key, geBoolean Cmd);
	geBoolean		Console_Frame(Console_Console *Console, float Time);

#endif
