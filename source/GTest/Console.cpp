/****************************************************************************************/
/*  Console.c                                                                           */
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
#include <Windows.h>
#include <Assert.h>
#include <Stdio.h>

#include "Console.h"

#include "Genesis.h"
#include "ErrorLog.h"
#include "Ram.h"

#include "Server.h"

extern Server_Server *GServer;

static geBoolean SetupConsole(Console_Console *Console);
static void PrintHeaderText(Console_Console *Console, float Time);

//================================================================================
//	Console_Create
//================================================================================
Console_Console *Console_Create(geEngine *Engine, VidMode VidMode)
{
	Console_Console	*NewConsole = nullptr;

	NewConsole = static_cast<Console_Console*>(geRam_Allocate(sizeof(Console_Console)));

	if (!NewConsole)
		return nullptr;

	memset(NewConsole, 0, sizeof(Console_Console));

	NewConsole->Engine = Engine;
	NewConsole->VidMode = VidMode;

	if (!SetupConsole(NewConsole))
		goto ExitWithError;

	NewConsole->DrawYPos = -400.0f;

	Console_Printf(NewConsole, " \n");
	Console_Printf(NewConsole, " \n");

	return NewConsole;

	// ERROR
	ExitWithError:
	{
		if (NewConsole)
			Console_Destroy(NewConsole);

		return nullptr;
	}
}

//================================================================================
//	Console_Destroy
//================================================================================
void Console_Destroy(Console_Console *Console)
{
	assert(Console);
	assert(Console->Engine);

	Console_FreeResources(Console);

	geRam_Free(Console);
}

//================================================================================
//	Console_FreeResources
//================================================================================
void Console_FreeResources(Console_Console *Console)
{
	geBoolean		Ret;

	// Free the background bitmap...
	if (Console->BGBitmap)
	{
		Ret = geEngine_RemoveBitmap(Console->Engine, Console->BGBitmap);
		assert(Ret == GE_TRUE);
		geBitmap_Destroy(&Console->BGBitmap);
		Console->BGBitmap = NULL;
	}

	if (Console->FontBitmap)
	{
		Ret = geEngine_RemoveBitmap(Console->Engine, Console->FontBitmap);
		assert(Ret == GE_TRUE);
		geBitmap_Destroy(&Console->FontBitmap);
		Console->FontBitmap = NULL;
	}
}

//================================================================================
//	Console_Printf
//================================================================================
geBoolean Console_Printf(Console_Console *Console, const char *Str, ...)
{
	va_list		ArgPtr;
    char		TempStr[500];
	int32		Length, i;

	assert(Console);
	assert(Str);

	va_start (ArgPtr, Str);
    vsprintf (TempStr, Str, ArgPtr);
	va_end (ArgPtr);

	Length = strlen(TempStr);

	// Insert the text a key at a time, through the normal KeyDown pipeline...
	for (i=0; i< Length; i++)
	{
		Console_KeyDown(Console, TempStr[i], GE_FALSE);
	}
	
	return GE_TRUE;
}

//================================================================================
//	Console_HeaderPrintf
//================================================================================
geBoolean Console_HeaderPrintf(Console_Console *Console, const char *Str, ...)
{
	va_list		ArgPtr;
    char		TempStr[256];

	assert(Console);
	assert(Str);

	assert(strlen(Str) < MAX_HEADER_TEXT_SIZE);

	va_start (ArgPtr, Str);
    vsprintf (TempStr, Str, ArgPtr);
	va_end (ArgPtr);

	assert(strlen(TempStr) < MAX_HEADER_TEXT_SIZE);

	strcpy(Console->HeaderText[Console->CurrentHeader], TempStr);
	Console->HeaderTime[Console->CurrentHeader] = HEADER_STAY_TIME;		

	Console->CurrentHeader++;
	Console->CurrentHeader%=MAX_HEADER_STRINGS;

	return GE_TRUE;
}

//================================================================================
//	Console_XYPrintf
//	Draw at the XY location, not neccesarally (did I spell that correctly?) on the console...
//================================================================================
geBoolean Console_XYPrintf(Console_Console *Console, int32 x, int32 y, uint32 Flags, const char *Str, ...)
{
	va_list		ArgPtr;
    char		TempStr[500];
	int32		Length, i;
	int32		StartX, StartY, SkipX;

	assert(Console);
	assert(Str);

	va_start (ArgPtr, Str);
    vsprintf (TempStr, Str, ArgPtr);
	va_end (ArgPtr);

	Length = strlen(TempStr);

	if (!Flags)
	{
		SkipX = Console->FontWidth+1;
		StartY = y*Console->FontHeight+1;
	}
	else
	{
		SkipX = 1;
		StartY = y;
	}

	StartX = x*SkipX;

	for (i=0; i< Length; i++)
	{
		GE_Rect	Rect;
		char	Val;

		Val = Val = TempStr[i];

		// Print the character
		Rect.Left = (Console->FontLUT1[Val]>>16);
		Rect.Right = Rect.Left+Console->FontWidth;
		Rect.Top = (Console->FontLUT1[Val]&0xffff);
		Rect.Bottom = Rect.Top+Console->FontHeight;

		geEngine_DrawBitmap(Console->Engine, Console->FontBitmap, &Rect, StartX, StartY);

		StartX+=(Console->FontWidth+1);	// Advance one cursor pos
	}
	
	return GE_TRUE;
}

//================================================================================
//	Console_ToggleActive
//================================================================================
geBoolean Console_ToggleActive(Console_Console *Console)
{
	assert(Console);

	Console->Active = !Console->Active;

	return GE_TRUE;
}

//================================================================================
//	Console_GetActive
//================================================================================
void Console_GetActive(Console_Console *Console, geBoolean *Active, geBoolean *Fast)
{
	*Active = Console->Active;
	*Fast = Console->Fast;
}

//================================================================================
//	Console_SetActive
//================================================================================
void Console_SetActive(Console_Console *Console, geBoolean Active, geBoolean Fast)
{
	assert(Console);

	Console->Active = Active;
	Console->Fast = Fast;
}

//================================================================================
//	Console_Scroll
//================================================================================
geBoolean Console_Scroll(Console_Console *Console, int32 Amount, geBoolean Cr)
{
	if (!Cr)
	{
		// Only let them scroll back to start of buffer
		if (Console->BackScroll+Amount < 0)		
			return GE_TRUE;

		// Don't let them scroll forward past original scroll pos
		if (Console->BackScroll+Amount > Console->Scroll)
			return GE_TRUE;
	}
	else
	{
		// Record the real scroll position on carriage return only
		Console->Scroll += Amount;
	}
	
	// Update how much they can backscroll
	Console->BackScroll += Amount;
	
	if (Amount > 0)
	{
		while (Console->TextBuffer[Console->PrintPos] != '\n')
		{
			Console->PrintPos++;
			Console->PrintPos%=TEXT_BUFFER_SIZE;
		}
		Console->PrintPos++;
	}
	else if (Amount < 0)
	{
		while (Console->TextBuffer[Console->PrintPos] != '\n')
		{
			Console->PrintPos--;
			Console->PrintPos%=TEXT_BUFFER_SIZE;
		}
		Console->PrintPos--;
	}

	return GE_TRUE;
}

//================================================================================
//	MoveCursorPos
//================================================================================
static void MoveCursorPos(Console_Console *Console, int32 Amount)
{
	if (Amount >= TEXT_BUFFER_SIZE)
		Amount = TEXT_BUFFER_SIZE-1;

	Console->CursorPos += Amount;

	if (Amount > 0)
	{
		if (Console->CursorPos >= TEXT_BUFFER_SIZE)
		{
			// Part of the amount they can back scroll has been erased, so make sure
			// they can't access that amount...
			Console->BackScroll -= (Console->CursorPos - TEXT_BUFFER_SIZE);
			Console->CursorPos -= TEXT_BUFFER_SIZE;
		}
	}
	else
	{
		if (Console->CursorPos < 0)
		{
			// Part of the amount they can back scroll has been erased, so make sure
			// they can't access that amount...
			Console->BackScroll += (Console->CursorPos);
			Console->CursorPos += TEXT_BUFFER_SIZE;
		}
	}
}

//================================================================================
//	Console_ParseTokens
//================================================================================
geBoolean Console_ParseTokens(Console_Console *Console)
{
	if (Console->CursorX <= 0)		// Nothing to parse
		return GE_TRUE;

	Console_Printf(Console, "*** Command not recognized ***\n");
	return GE_TRUE;

	//TokenFuncs[0].Func();
	
//	if (GServer)
//		Server_SetWorld(GServer, "Levels\\cool1.bsp");
//	else
//		Console_Printf(Console, "*** Cannot set world in client mode ***\n");
		
}

//================================================================================
//	Console_KeyDown
//================================================================================
geBoolean Console_KeyDown(Console_Console *Console, int32 Key, geBoolean Cmd)
{
	assert(Console);

	if (Key == VK_LEFT || Key == VK_RIGHT || Key == VK_UP || Key == VK_DOWN || Key == VK_ESCAPE)
		return GE_TRUE;

	if (Key == 33)		// Page up
	{
		Console_Scroll(Console, -1, GE_FALSE);

		return GE_TRUE;
	}
	else if (Key == 34)	// Page down
	{
		Console_Scroll(Console, 1, GE_FALSE);

		return GE_TRUE;
	}
	else if (Key == VK_RETURN || Key == '\n')
	{
		if (Console->CursorX <= 0)
			return GE_TRUE;

		if (Console->CursorY < Console->StopY-1)
			Console->CursorY++;
		else	// When it gets to the bottom of the screen, start scrolling...
			Console_Scroll(Console, 1, GE_TRUE);

		Console->TextBuffer[Console->CursorPos] = '\n';
		MoveCursorPos(Console, 1);

		if (Cmd)
			Console_ParseTokens(Console);

		// Must set to 0 after parse tokens, so it can use it to see where we are in the
		// buffer
		Console->CursorX = 0;

		return GE_TRUE;
	}
	else if (Key == VK_BACK)
	{
		if (Console->CursorX <= 0)
			return GE_TRUE;

		Console->CursorX--;
		MoveCursorPos(Console, -1);

		return GE_TRUE;
	}
	else if (Console->CursorX >= Console->StopX)		// Too much on one line
		return GE_TRUE;

	//Console->TokenBuffer[Console->CursorX] = Key;
	Console->TextBuffer[Console->CursorPos] = (char)Key;

	Console->CursorX++;

	MoveCursorPos(Console, 1);

	return GE_TRUE;
}

//================================================================================
//	Console_Draw
//================================================================================
geBoolean Console_Frame(Console_Console *Console, float Time)
{
	int32		w, h;
	int32		x, y, PrintPos, YPos;
	char		Val;

	assert(Console);

	PrintHeaderText(Console, Time);

	if (Console->Active && Console->DrawYPos < 0)
	{
		Console->DrawYPos += Time*700.0f;

		if (Console->DrawYPos > 0) 
			Console->DrawYPos = 0.0f;

		if (Console->Fast)
			Console->DrawYPos = 0;
	}
	else if (!Console->Active && Console->DrawYPos > -400)
	{
		Console->DrawYPos -= Time*700;

		if (Console->DrawYPos < -400.0f) 
			Console->DrawYPos = -400.0f;

		if (Console->Fast)
			Console->DrawYPos = -400;
	}

	if (Console->DrawYPos < -350)
		return GE_TRUE;

	YPos = (int32)Console->DrawYPos;

	for (h=0; h<3; h++)
	{
		for (w=0; w<5; w++)
		{
			geEngine_DrawBitmap(Console->Engine, Console->BGBitmap, NULL, w*128, h*128+YPos);
		}
	}

	// Draw the TextBuffer
	PrintPos = Console->PrintPos;
	x = 0;
	y = 0;
	while (1)	// Print till bottom of console, of till hit cursor pos
	{
		GE_Rect		Rect;
		int32		PrintX, PrintY;

		if (x >= 2)
		{
			if (PrintPos >= Console->CursorPos)		// At cursor, stop printing
				break;

			Val = Console->TextBuffer[PrintPos%TEXT_BUFFER_SIZE];
			PrintPos++;

			if (Val == '\n')		// Next line on enter
			{
				x = 0;
				y++;

				if (y >= Console->StopY)
					break;			// Bottom of console, so stop...

				continue;
			}
		}
		else
		{
			if (x == 0)
				Val = '(';		// Print (> for the first 2 characters
			else
				Val = '>';
		}


		PrintX = x*(Console->FontWidth+1);
		PrintY = y*(Console->FontHeight+1)+YPos;

		// Print the character
		Rect.Left = (Console->FontLUT1[Val]>>16);
		Rect.Right = Rect.Left+Console->FontWidth;
		Rect.Top = (Console->FontLUT1[Val]&0xffff);
		Rect.Bottom = Rect.Top+Console->FontHeight;
		geEngine_DrawBitmap(Console->Engine, Console->FontBitmap, &Rect, PrintX, PrintY);

		x++;	// Advance one cursor pos
	}

	return GE_TRUE;
}

extern geVFile *MainFS;

//================================================================================
//================================================================================
static geBoolean SetupConsole(Console_Console *Console)
{
	int32		PosX, PosY, Width;
	int32		i;
	int         VideoWidth, VideoHeight;
	char		CName[MAX_PATH];
	char		FName[MAX_PATH];

	assert(Console);

	VidMode_GetResolution(Console->VidMode,&VideoWidth,&VideoHeight);
	if (VideoWidth< SMALL_CONSOLE_CUTOFF_WIDTH )
		{
			strcpy(CName, "Bmp\\Console\\320x240\\Console.Bmp");
			strcpy(FName, "Bmp\\Console\\320x240\\Font.Bmp");

			Console->FontWidth = 5;
			Console->FontHeight = 6;

			Console->StopX = 60;
			Console->StopY = 29;		// last text line 
		}
	else
		{
			strcpy(CName, "Bmp\\Console\\640x480\\Console.Bmp");
			strcpy(FName, "Bmp\\Console\\640x480\\Font.Bmp");

			Console->FontWidth = 8;
			Console->FontHeight = 16;

			Console->StopX = 60;
			Console->StopY = 20;
		}
	
	// Create the console background bitmap
	Console->BGBitmap = geBitmap_CreateFromFileName(MainFS, CName);

	if (!Console->BGBitmap)
	{
		geErrorLog_AddString(-1, "Console_SetupConsole:  geBitmap_CreateFromFileName failed:", CName);
		goto ExitWithError;
	}

	if (!geBitmap_SetColorKey(Console->BGBitmap, GE_TRUE, 255, GE_FALSE))
	{
		geErrorLog_AddString(-1, "Console_SetupConsole:  geBitmap_SetColorKey failed.", NULL);
		goto ExitWithError;
	}

	// Add the console to the engine
	if (!geEngine_AddBitmap(Console->Engine, Console->BGBitmap))
	{
		geErrorLog_AddString(-1, "Console_SetupConsole:  geEngine_AddBitmap failed:", CName);
		goto ExitWithError;
	}

	// Create the font bitmap
	Console->FontBitmap = geBitmap_CreateFromFileName(MainFS, FName);
			
	if (!Console->FontBitmap)
	{
		geErrorLog_AddString(-1, "Console_SetupConsole:  geBitmap_CreateFromFileName failed:", FName);
		goto ExitWithError;
	}

	if (!geBitmap_SetColorKey(Console->FontBitmap, GE_TRUE, 255, GE_FALSE))
	{
		geErrorLog_AddString(-1, "Console_SetupConsole:  geBitmap_SetColorKey failed.", NULL);
		goto ExitWithError;
	}

	// Add the font bitmap to the engine
	if (!geEngine_AddBitmap(Console->Engine, Console->FontBitmap))
	{
		geErrorLog_AddString(-1, "Console_SetupConsole:  geEngine_AddBitmap failed:", FName);
		goto ExitWithError;
	}

	PosX = 0;
	PosY = 0;
	Width = Console->FontWidth*128;

	for (i=0; i< 128; i++)
	{
		Console->FontLUT1[i] = (PosX<<16) | PosY;
		PosX+=Console->FontWidth;

		if (PosX >= Width)
		{
			PosY += Console->FontHeight;
			PosX = 0;
		}
	}

	Console->CursorPos = 0;
	Console->PrintPos = 0;
	Console->Active = GE_FALSE;

	return GE_TRUE;

	ExitWithError:
	{
		Console_FreeResources(Console);

		return GE_FALSE;
	}
}


//================================================================================
//================================================================================
static void PrintHeaderText(Console_Console *Console, float Time)
{
	int32		i;

	for (i=0; i< MAX_HEADER_STRINGS; i++)
	{
		int32		HeaderNum;

		HeaderNum = (Console->CurrentHeader - (i+1));

		if (HeaderNum < 0)
			HeaderNum += MAX_HEADER_STRINGS;

		assert(HeaderNum >= 0 && HeaderNum < MAX_HEADER_STRINGS);

		if (Console->HeaderTime[HeaderNum] > 0.0f)		
		{
			Console_XYPrintf(Console, 15, MAX_HEADER_STRINGS-i, 0, Console->HeaderText[HeaderNum]);
			Console->HeaderTime[HeaderNum] -= Time;
		}
	}
}