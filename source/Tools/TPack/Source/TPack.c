

/****************************************************************************************/
/*  TPACK.C                                                                             */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Texture packer UI implementation                                       */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                    */
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

/*--------------------------------------------------------------------------------
  REVISION LOG ENTRY
  Revision By: Dennis Tierney (DJT) - dtierney@oneoverz.com
  Revised on 4/18/99 6:51:32 AM
  Comments: Added "Extract all" and "Extract selected" textures to TPack.
            Most of the new code is at the bottom of the file.

  1) Added "Extract all" and "Extract selected" menu selections (Modified TPack.rc)
  2) Added WriteBMP8() routine to write a geBitmap object to a 8-bit BMP file.
  3) Added void TPack_ExtractAll() and void TPack_ExtractSelected().
  4) Added HandleInitMenu called with WM_INITMENU message to properly set
     the pulldown menu state.

  TODO: 1) Getting the palette entries is pretty slow. I can't seem to get 
           geBitmap_Palette_GetData() to work propoerly.
        2) Progress bar for extract all would be nice.
        3) In 8-bit color video mode, TPack doesn't display the textures
           properly. That's because the texture palette is not being 
           "Realized".
---------------------------------------------------------------------------------*/

#include <windows.h>
#pragma warning(disable : 4201 4214 4115)
#include <commctrl.h>
#include <commdlg.h>
#include <mmsystem.h>
#include <shellapi.h>
#pragma warning(default : 4201 4214 4115)
#include <stdlib.h>

#include <stdio.h>
#include <assert.h>

#include "resource.h"
#include "genesis.h"
#include "bitmap.h"
#include "ram.h"

#pragma warning (disable:4514)		// unreferenced inline function

#define	MSG_SELECT_BITMAP	(WM_USER + 1)
#define	MSG_DELETE_BITMAP	(WM_USER + 2)

#define	MAX_TEXTURE_NAME_LENGTH	_MAX_FNAME

#define	ENTRY_DELETED	0x00000001

typedef	struct	BitmapEntry
{
	char *		Name;
	geBitmap *	Bitmap;
	HBITMAP		WinBitmap;
	unsigned	Flags;
}	BitmapEntry;

typedef struct	TPack_WindowData
{
	HINSTANCE		Instance;
	HWND			hwnd;
	int				BitmapCount;
	BitmapEntry	*	Bitmaps;
	BitmapEntry *	SelectedEntry;
	BOOL			FileNameIsValid;
	char			TXLFileName[_MAX_PATH];
	BOOL			Dirty;
}	TPack_WindowData;

static HWND TPack_DlgHandle = NULL;

// Prototypes - DJT (see revision note - 4/18/99)
void TPack_ExtractAll(TPack_WindowData * pData);
void TPack_ExtractSelected(TPack_WindowData * pData);
void HandleInitMenu(TPack_WindowData * pData, HMENU hMenu);

static	void	NonFatalError(const char *Msg, ...)
{
	char Buffer[1024];
	va_list argptr;

	va_start (argptr, Msg);
	vsprintf (Buffer, Msg, argptr);
	va_end (argptr);

	MessageBox (NULL, Buffer, "Error", MB_ICONEXCLAMATION | MB_OK);
}

static TPack_WindowData *TPack_GetWindowData (HWND hwnd)
{
	return (TPack_WindowData *)GetWindowLong (hwnd, GWL_USERDATA);
}

static	void	Save(TPack_WindowData *pData, const char *Path)
{
	char		FileName[_MAX_PATH];
	geVFile *	VFS;
	int			i;

	if	(!Path)
	{
		OPENFILENAME ofn;	// Windows open filename structure...
		char Filter[_MAX_PATH];
		char	Dir[_MAX_PATH];
	
		FileName[0] = '\0';

		GetCurrentDirectory(sizeof(Dir), Dir);
	
		ofn.lStructSize = sizeof (OPENFILENAME);
		ofn.hwndOwner = pData->hwnd;
		ofn.hInstance = pData->Instance;
		{
			char *c;
	
			// build actor file filter string
			strcpy (Filter, "Texture Libraries (*.txl)");
			c = &Filter[strlen (Filter)] + 1;
			// c points one beyond end of string
			strcpy (c, "*.txl");
			c = &c[strlen (c)] + 1;
			*c = '\0';	// 2nd terminating nul character
		}
		ofn.lpstrFilter = Filter;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = FileName;
		ofn.nMaxFile = sizeof(FileName);
		ofn.lpstrFileTitle = FileName;
		ofn.nMaxFileTitle = sizeof(FileName);
		ofn.lpstrInitialDir = Dir;
		ofn.lpstrTitle = NULL;
		ofn.Flags = OFN_HIDEREADONLY;
		ofn.nFileOffset = 0;
		ofn.nFileExtension = 0;
		ofn.lpstrDefExt = "txl";
		ofn.lCustData = 0;
		ofn.lpfnHook = NULL;
		ofn.lpTemplateName = NULL;

		if	(!GetSaveFileName (&ofn))
			return;

		Path = FileName;
	}
	
	unlink(Path);
	VFS = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_VIRTUAL, Path, NULL, GE_VFILE_OPEN_CREATE | GE_VFILE_OPEN_DIRECTORY);
	if	(!VFS)
	{
		NonFatalError("Could not open file %s", Path);
		return;
	}

	for	(i = 0; i < pData->BitmapCount; i++)
	{
		geVFile *	File;
		geBoolean	WriteResult;

		if	(pData->Bitmaps[i].Flags & ENTRY_DELETED)
			continue;

		File = geVFile_Open(VFS, pData->Bitmaps[i].Name, GE_VFILE_OPEN_CREATE);
		if	(!File)
		{
			NonFatalError("Could not save bitmap %s", pData->Bitmaps[i].Name);
			geVFile_Close(VFS);
			return;
		}
		WriteResult = geBitmap_WriteToFile(pData->Bitmaps[i].Bitmap, File);
		geVFile_Close(File);
		if	(WriteResult == GE_FALSE)
		{
			NonFatalError("Could not save bitmap %s", pData->Bitmaps[i].Name);
			geVFile_Close(VFS);
			return;
		}
	}

	strcpy(pData->TXLFileName, Path);
	pData->FileNameIsValid = TRUE;
	
	if	(geVFile_Close(VFS) == GE_FALSE)
		NonFatalError("I/O error writing %s", Path);
	else
		pData->Dirty = FALSE;
}

static	BOOL	AddTexture(TPack_WindowData *pData, geVFile *BaseFile, const char *Path)
{
	geBitmap_Info	PInfo;
	geBitmap_Info	SInfo;
	geBitmap *		Bitmap;
	BitmapEntry *	NewBitmapList;
	geVFile *		File;
	char			FileName[_MAX_FNAME];
	char *			Name;

	Bitmap = NULL;
	File = NULL;

	_splitpath(Path, NULL, NULL, FileName, NULL);
	Name = strdup(FileName);
	if	(!Name)
	{
		NonFatalError("Memory allocation error processing %s", Path);
		return FALSE;
	}

	if	(BaseFile)
		File = geVFile_Open(BaseFile, Path, GE_VFILE_OPEN_READONLY);
	else
		File = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_DOS, Path, NULL, GE_VFILE_OPEN_READONLY);

	if	(!File)
	{
		NonFatalError("Could not open %s", Path);
		goto fail;
	}

	Bitmap = geBitmap_CreateFromFile(File);
	geVFile_Close(File);
	if	(!Bitmap)
	{
		NonFatalError("%s is not a valid bitmap", Path);
		goto fail;
	}
	geBitmap_GetInfo(Bitmap, &PInfo, &SInfo);
	if	(PInfo.Format != GE_PIXELFORMAT_8BIT)
	{
		NonFatalError("%s is not an 8bit bitmap", Path);
		goto fail;
	}
	NewBitmapList = geRam_Realloc(pData->Bitmaps, sizeof(*NewBitmapList) * (pData->BitmapCount + 1));
	if	(!NewBitmapList)
	{
		NonFatalError("Memory allocation error processing %s", Path);
		goto fail;
	}

	NewBitmapList[pData->BitmapCount].Name		= Name;
	NewBitmapList[pData->BitmapCount].Bitmap	= Bitmap;
	NewBitmapList[pData->BitmapCount].WinBitmap	= NULL;
	NewBitmapList[pData->BitmapCount].Flags		= 0;
	pData->BitmapCount++;
	pData->Bitmaps = NewBitmapList;

	SendDlgItemMessage(pData->hwnd, IDC_TEXTURELIST, LB_ADDSTRING, (WPARAM)0, (LPARAM)Name);
	return TRUE;

fail:
	if	(Name)
		free(Name);
	if	(Bitmap)
		geBitmap_Destroy(&Bitmap);
	return FALSE;
}

static	void	Load(TPack_WindowData *pData)
{
	char				FileName[_MAX_PATH];
	geVFile *			VFS;
	int					i;
	geVFile_Finder *	Finder;

	if	(pData->Dirty)
	{
		int	Result;

		Result = MessageBox(NULL,
							"Do you want to save changes to the current file?",
							"Texture Packer",
							MB_YESNOCANCEL);
		
		if	(Result == IDCANCEL)
			return;

		if	(Result == IDYES)
		{
#pragma message ("We don't respect CANCEL of saving dirty data in Load")
			if	(pData->FileNameIsValid)
				Save(pData, pData->TXLFileName);
			else
				Save(pData, NULL);
		}
	}

	for	(i = 0; i < pData->BitmapCount; i++)
	{
		if	(!(pData->Bitmaps[i].Flags & ENTRY_DELETED))
		{
			SendDlgItemMessage(pData->hwnd, IDC_TEXTURELIST, LB_DELETESTRING, (WPARAM)0, (LPARAM)0);
			free(pData->Bitmaps[i].Name);
			geBitmap_Destroy(&pData->Bitmaps[i].Bitmap);
			if	(pData->Bitmaps[i].WinBitmap)
				DeleteObject(pData->Bitmaps[i].WinBitmap);
		}
	}
	if	(pData->Bitmaps)
		geRam_Free(pData->Bitmaps);
	pData->Bitmaps = NULL;
	pData->BitmapCount = 0;
	pData->SelectedEntry = NULL;

	{
		OPENFILENAME ofn;	// Windows open filename structure...
		char Filter[_MAX_PATH];
		char	Dir[_MAX_PATH];
	
		FileName[0] = '\0';

		GetCurrentDirectory(sizeof(Dir), Dir);
	
		ofn.lStructSize = sizeof (OPENFILENAME);
		ofn.hwndOwner = pData->hwnd;
		ofn.hInstance = pData->Instance;
		{
			char *c;
	
			// build actor file filter string
			strcpy (Filter, "Texture Libraries (*.txl)");
			c = &Filter[strlen (Filter)] + 1;
			// c points one beyond end of string
			strcpy (c, "*.txl");
			c = &c[strlen (c)] + 1;
			*c = '\0';	// 2nd terminating nul character
		}
		ofn.lpstrFilter = Filter;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;
		ofn.nFilterIndex = 1;
		ofn.lpstrFile = FileName;
		ofn.nMaxFile = sizeof(FileName);
		ofn.lpstrFileTitle = FileName;
		ofn.nMaxFileTitle = sizeof(FileName);
		ofn.lpstrInitialDir = Dir;
		ofn.lpstrTitle = NULL;
		ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
		ofn.nFileOffset = 0;
		ofn.nFileExtension = 0;
		ofn.lpstrDefExt = "txl";
		ofn.lCustData = 0;
		ofn.lpfnHook = NULL;
		ofn.lpTemplateName = NULL;

		if	(!GetOpenFileName (&ofn))
			return;
	}
	
	VFS = geVFile_OpenNewSystem(NULL, GE_VFILE_TYPE_VIRTUAL, FileName, NULL, GE_VFILE_OPEN_READONLY | GE_VFILE_OPEN_DIRECTORY);
	if	(!VFS)
	{
		NonFatalError("Could not open file %s", FileName);
		return;
	}

	Finder = geVFile_CreateFinder(VFS, "*.*");
	if	(!Finder)
	{
		NonFatalError("Could not load textures from %s", FileName);
		geVFile_Close(VFS);
		return;
	}
	
	while	(geVFile_FinderGetNextFile(Finder) != GE_FALSE)
	{
		geVFile_Properties	Properties;

		geVFile_FinderGetProperties(Finder, &Properties);
		if	(!AddTexture(pData, VFS, Properties.Name))
		{
			geVFile_Close(VFS);
			return;
		}
	}

	strcpy(pData->TXLFileName, FileName);
	pData->FileNameIsValid = TRUE;
	pData->Dirty = FALSE;
	
	geVFile_Close(VFS);
}

// This will create a 24BIT bgr...
static HBITMAP CreateHBitmapFromgeBitmap (geBitmap *Bitmap, HDC hdc)
{
	geBitmap * Lock;
	gePixelFormat Format;
	geBitmap_Info info;
	HBITMAP hbm = NULL;

	// <> choose format to be 8,16,or 24, whichever is closest to Bitmap
	Format = GE_PIXELFORMAT_24BIT_BGR;

	if ( geBitmap_GetBits(Bitmap) )
	{
		Lock = Bitmap;
	}
	else
	{
		if ( ! geBitmap_LockForRead(Bitmap, &Lock, 0, 0, Format,	GE_FALSE,0) )
		{
			return NULL;
		}
	}

	geBitmap_GetInfo(Lock,&info,NULL);

	if ( info.Format != Format )
		return NULL;

	{
		void * bits;
		BITMAPINFOHEADER bmih;
		int pelbytes;

		pelbytes = gePixelFormat_BytesPerPel(Format);
		bits = geBitmap_GetBits(Lock);

		bmih.biSize = sizeof(bmih);
		bmih.biHeight = - info.Height;
		bmih.biPlanes = 1;
		bmih.biBitCount = 24;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = bmih.biYPelsPerMeter = 10000;
		bmih.biClrUsed = bmih.biClrImportant = 0;

		if ( (info.Stride*pelbytes) == (((info.Stride*pelbytes)+3)&(~3)) )
		{
			bmih.biWidth = info.Stride;
			hbm = CreateDIBitmap( hdc, &bmih , CBM_INIT , bits, (BITMAPINFO *)&bmih , DIB_RGB_COLORS );
		}
		else
		{
			void * newbits;
			int Stride;

			bmih.biWidth = info.Width;
			Stride = (((info.Width*pelbytes)+3)&(~3));
			newbits = geRam_Allocate(Stride * info.Height);
			if ( newbits )
			{
				char *newptr,*oldptr;
				int y;

				newptr = (char *)newbits;
				oldptr = (char *)bits;
				for(y=0; y<info.Height; y++)
				{
					memcpy(newptr,oldptr,(info.Width)*pelbytes);
					oldptr += info.Stride*pelbytes;
					newptr += Stride;
				}
				hbm = CreateDIBitmap( hdc, &bmih , CBM_INIT , newbits, (BITMAPINFO *)&bmih , DIB_RGB_COLORS );
				geRam_Free(newbits);
			}
		}
	}

	if ( Lock != Bitmap )
	{
		geBitmap_UnLock (Lock);
	}

	return hbm;
}

static	BitmapEntry *FindBitmap(TPack_WindowData *pData, const char *Name)
{
	int	i;

	for	(i = 0; i < pData->BitmapCount; i++)
	{
		if	(pData->Bitmaps[i].Flags & ENTRY_DELETED)
			continue;

		if	(!strcmp(Name, pData->Bitmaps[i].Name))
			return &pData->Bitmaps[i];
	}

	return NULL;
}

static	BOOL Render2d_Blit(HDC hDC, HBITMAP Bmp, const RECT *SourceRect, const RECT *DestRect)
{
	HDC		MemDC;
    int		SourceWidth;
    int		SourceHeight;
    int		DestWidth;
    int		DestHeight;

	MemDC = CreateCompatibleDC(hDC);
	if	(MemDC == NULL)
		return FALSE;

	SelectObject(MemDC, Bmp);

	SourceWidth = SourceRect->right - SourceRect->left;
   	SourceHeight = SourceRect->bottom - SourceRect->top;
	DestWidth = DestRect->right - DestRect->left;
   	DestHeight = DestRect->bottom - DestRect->top;
    if	(SourceWidth == DestWidth && SourceHeight == DestHeight)
    {
		BitBlt(hDC,
        	   DestRect->left,
               DestRect->top,
               DestRect->right - DestRect->left,
               DestRect->bottom - DestRect->top,
               MemDC,
               SourceRect->left,
               SourceRect->top,
               SRCCOPY);
    }
    else
    {
    	StretchBlt(hDC,
        		   DestRect->left,
        		   DestRect->top,
                   DestWidth,
                   DestHeight,
                   MemDC,
        		   SourceRect->left,
        		   SourceRect->top,
                   SourceWidth,
                   SourceHeight,
                   SRCCOPY);
    }

	DeleteDC(MemDC);

	return TRUE;
}

#pragma warning (disable:4100)
static LRESULT wm_Command
	(
	  HWND hwnd,
	  TPack_WindowData *pData,
	  WORD wNotifyCode,
	  WORD wID,
	  HWND hwndCtl
	)
{
	switch (wID)
	{
	case	ID_FILE_OPEN:
		Load(pData);
		return 0;

	case	ID_FILE_SAVE:
		if	(pData->Dirty)
		{
			if	(pData->FileNameIsValid)
				Save(pData, pData->TXLFileName);
			else
				Save(pData, NULL);
		}
		return 0;

	case	ID_FILE_SAVEAS:
		Save(pData, NULL);
		return 0;

	case ID_FILE_EXIT :
	case IDCANCEL :
		if	(pData->Dirty)
		{
			int	Result;

			Result = MessageBox(NULL,
								"Do you want to save changes before quitting?",
								"Texture Packer",
								MB_YESNOCANCEL);
			
			if	(Result == IDCANCEL)
				return 0;

			if	(Result == IDYES)
			{
#pragma message("If we do a cancel in here, then we still shut down the app, and lose changes.")
				if	(pData->FileNameIsValid)
					Save(pData, pData->TXLFileName);
				else
					Save(pData, NULL);
			}
		}
		DestroyWindow (hwnd);
		return 0;

	case	IDC_TEXTURELIST:
		if	(wNotifyCode == LBN_SELCHANGE)
		{
			SendMessage(pData->hwnd, MSG_SELECT_BITMAP, (WPARAM)0, (LPARAM)0);
		}
		return 0;

	// Extract all resource entries to individual BMP files.
	// Added DJT (see revision note - 4/18/99)
	case ID_OPTIONS_EXTRACTALL: 
		{
			TPack_ExtractAll(pData);
		}
		return 0;

	// Extract the selected resource entry to BMP file.
	// Added DJT (see revision note - 4/18/99)
	case ID_OPTIONS_EXTRACTSELECTED: 
		{
			TPack_ExtractSelected(pData);
		}
		return 0;


	}
	return 0;
}
#pragma warning (default:4100)


#pragma warning (disable:4100)
static BOOL CALLBACK TPack_PreviewWndProc
	(
	  HWND hwnd,
	  UINT msg,
	  WPARAM wParam,
	  LPARAM lParam
	)
{
	BitmapEntry *	Entry;
	TPack_WindowData *pData = TPack_GetWindowData (hwnd);

	if	(msg == WM_PAINT)
	{
		PAINTSTRUCT	ps;
		HDC			hDC;
		RECT		Rect;

		hDC = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &Rect);
		Rect.left--;
		Rect.bottom--;
		FillRect(hDC, &Rect, GetStockObject(WHITE_BRUSH));

		Entry = pData->SelectedEntry;
		if	(Entry)
		{
			RECT	Source;
			RECT	Dest;
			HDC		hDC;

			Source.left = 0;
			Source.top = 0;
			Source.bottom = geBitmap_Height(Entry->Bitmap);
			Source.right = geBitmap_Width(Entry->Bitmap);
			Dest = Rect;

			hDC = GetDC(hwnd);
			SetStretchBltMode(hDC, HALFTONE);
			Render2d_Blit(hDC,
						  Entry->WinBitmap,
						  &Source,
						  &Dest);
			ReleaseDC(hwnd, hDC);
		}
		EndPaint(hwnd, &ps);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

#pragma warning (disable:4100)
static BOOL CALLBACK TPack_ListBoxWndProc
	(
	  HWND hwnd,
	  UINT msg,
	  WPARAM wParam,
	  LPARAM lParam
	)
{
	if	(msg == WM_KEYDOWN)
	{
		if	(wParam == VK_DELETE)
		{
			int					Index;
			char				TextureName[MAX_TEXTURE_NAME_LENGTH];

			Index = SendMessage(hwnd, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
			if	(Index != LB_ERR)
			{
				int		Count;
				HWND	Parent;

				Parent = GetParent(hwnd);

				SendMessage(hwnd, LB_GETTEXT, (WPARAM)Index, (LPARAM)&TextureName[0]);
				SendMessage(Parent, MSG_DELETE_BITMAP, (WPARAM)0, (LPARAM)&TextureName[0]);
				SendMessage(hwnd, LB_DELETESTRING, (WPARAM)Index, (LPARAM)0);
				Count = SendMessage(hwnd, LB_GETCOUNT, (WPARAM)0, (LPARAM)0);
				if	(Count != LB_ERR)
				{
					if	(Count <= Index)
						SendMessage(hwnd, LB_SETCURSEL, (WPARAM)(Count - 1), (LPARAM)0);
					else
						SendMessage(hwnd, LB_SETCURSEL, (WPARAM)Index, (LPARAM)0);
				}

				SendMessage(Parent, MSG_SELECT_BITMAP, (WPARAM)0, (LPARAM)0);
			}
			return 0;
		}
	}

	return CallWindowProc((WNDPROC)GetWindowLong(hwnd, GWL_USERDATA), hwnd, msg, wParam, lParam);
}

static BOOL TPack_InitializeDialog (HWND hwnd)
{
	TPack_WindowData *	pData;
	HFONT				Font;
	HWND				ListWnd;

	// allocate window local data structure
	pData = GE_RAM_ALLOCATE_STRUCT (TPack_WindowData);
	if (pData == NULL)
	{
		DestroyWindow (hwnd);
		return TRUE;
	}

	// and initialize it
	pData->Instance		 = (HINSTANCE)GetWindowLong (hwnd, GWL_HINSTANCE);
	pData->hwnd			 = hwnd;
	pData->BitmapCount	 = 0;
	pData->Bitmaps		 = NULL;
	pData->SelectedEntry = NULL;
	pData->FileNameIsValid = FALSE;
	pData->Dirty		 = FALSE;
	// Set the window data pointer in the GWL_USERDATA field
	SetWindowLong(hwnd, GWL_USERDATA, (LONG)pData);
	SetWindowLong(GetDlgItem(hwnd, IDC_PREVIEW), GWL_USERDATA, (LONG)pData);
	SetWindowLong(GetDlgItem(hwnd, IDC_PREVIEW), GWL_WNDPROC, (LONG)TPack_PreviewWndProc);

	ListWnd = GetDlgItem(hwnd, IDC_TEXTURELIST);
	SetWindowLong(ListWnd, GWL_USERDATA, (LONG)GetWindowLong(ListWnd, GWL_WNDPROC));
	SetWindowLong(ListWnd, GWL_WNDPROC, (LONG)TPack_ListBoxWndProc);
	
	Font = CreateFont( -24,
    					    0,0,0, 0,
    					    0,0,0,0,OUT_TT_ONLY_PRECIS ,0,0,0, "Arial Black");
    SendDlgItemMessage(hwnd, IDC_TEXTURESIZE, WM_SETFONT, (WPARAM)Font, MAKELPARAM(TRUE, 0));

	// set the program icon on the dialog window
	SendMessage (hwnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon (pData->Instance, MAKEINTRESOURCE (IDI_MAINICON)));

	DragAcceptFiles (hwnd, TRUE);

	return TRUE;
}

static void TPack_ShutdownAll
	(
	  HWND hwnd,
	  TPack_WindowData *pData
	)
{
	DragAcceptFiles (hwnd, FALSE);

	if (pData != NULL)
	{
		if	(pData->Bitmaps)
			geRam_Free(pData->Bitmaps);

		geRam_Free (pData);
	}

	SetWindowLong (hwnd, GWL_USERDATA, (LPARAM)NULL);
}

#pragma warning (disable:4100)
static BOOL CALLBACK TPack_DlgProc
	(
	  HWND hwnd,
	  UINT msg,
	  WPARAM wParam,
	  LPARAM lParam
	)
{
	TPack_WindowData *pData = TPack_GetWindowData (hwnd);

	switch (msg)
	{
	case WM_INITDIALOG :
		return TPack_InitializeDialog (hwnd);

	case WM_DESTROY :
		TPack_ShutdownAll (hwnd, pData);
		PostQuitMessage (0);
		break;

	case WM_COMMAND :
	{
		WORD wNotifyCode = HIWORD (wParam);
		WORD wID = LOWORD (wParam);
		HWND hwndCtl = (HWND)lParam;

		return wm_Command (hwnd, pData, wNotifyCode, wID, hwndCtl);
	}


	case	MSG_DELETE_BITMAP:
	{
		BitmapEntry *	Entry;

		Entry = FindBitmap(pData, (const char *)lParam);
		assert(Entry);
		if	(Entry->Name)
		{
			free(Entry->Name);
			Entry->Name = NULL;
		}
		if	(Entry->Bitmap)
			geBitmap_Destroy(&Entry->Bitmap);
		if	(Entry->WinBitmap)
			DeleteObject(Entry->WinBitmap);
		Entry->Flags |= ENTRY_DELETED;
		pData->Dirty = TRUE;
		return 0;
	}
		
	case	MSG_SELECT_BITMAP:
	{
		char	Buff[128];
		int		Index;
		char	TextureName[MAX_TEXTURE_NAME_LENGTH];
		BitmapEntry *	Entry;

		Index = SendDlgItemMessage(pData->hwnd, IDC_TEXTURELIST, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		if	(Index == LB_ERR)
		{
			Entry = NULL;
		}
		else
		{
			SendDlgItemMessage(pData->hwnd, IDC_TEXTURELIST, LB_GETTEXT, (WPARAM)Index, (LPARAM)&TextureName[0]);
			Entry = FindBitmap(pData, TextureName);
			assert(Entry);
			if	(!Entry->WinBitmap)
			{
				HWND	PreviewWnd;
				HBITMAP	hbm;
				HDC		hDC;
	
				PreviewWnd = GetDlgItem(pData->hwnd, IDC_PREVIEW);
				hDC = GetDC(PreviewWnd);
				hbm = CreateHBitmapFromgeBitmap(Entry->Bitmap, hDC);
				Entry->WinBitmap = hbm;
				ReleaseDC(PreviewWnd, hDC);
			}
	
			if	(!Entry->WinBitmap)
			{
				NonFatalError("Memory allocation error creating bitmap");
				return 0;
			}
		}

		InvalidateRect(GetDlgItem(hwnd, IDC_PREVIEW), NULL, TRUE);
		pData->SelectedEntry = Entry;
		
		if	(Entry)
			sprintf(Buff, "%dx%d", geBitmap_Width(Entry->Bitmap), geBitmap_Height(Entry->Bitmap));
		else
			Buff[0] = '\0';
		SetWindowText(GetDlgItem(hwnd, IDC_TEXTURESIZE), Buff);
		return 0;
	}

	case WM_DROPFILES :
	{
		// Files dropped.
		HDROP hDrop;
		UINT FileCount;
		char Buff[MAX_PATH];

		hDrop = (HDROP)wParam;
		FileCount = DragQueryFile (hDrop, 0xffffffff, Buff, sizeof (Buff));
		while	(FileCount--)
		{
			DragQueryFile (hDrop, FileCount, Buff, sizeof (Buff));
			AddTexture(pData, NULL, Buff);
		}
		pData->Dirty = TRUE;
		DragFinish (hDrop);
		return 0;
	}

	// Initialize the pulldown
	// Added DJT (see revision note - 4/18/99)
	case WM_INITMENU:

		if (GetMenu(hwnd) == (HMENU)wParam)
			HandleInitMenu(pData, (HMENU)wParam);
		break;


	default :
		break;

	}
	return FALSE;
}
#pragma warning (default:4100)


#pragma warning (disable:4100)
int CALLBACK WinMain
      (
        HINSTANCE instance,
        HINSTANCE prev_instance,
        LPSTR cmd_line,
        int cmd_show
      )
{
	TPack_WindowData *pData;

	InitCommonControls ();

    // create main window.
	// This is the controlling dialog.
    TPack_DlgHandle = CreateDialog 
	(
		instance, 
		MAKEINTRESOURCE (IDD_TEXTUREPACKER),
		NULL, 
		TPack_DlgProc
	);

    if (TPack_DlgHandle == NULL)
    {
        return 0;
    }

	pData = TPack_GetWindowData (TPack_DlgHandle);
	
	ShowWindow (TPack_DlgHandle, SW_SHOWNORMAL);
	UpdateWindow (TPack_DlgHandle);

	// clear render window's text
	// (otherwise it might show through at times...trust me)
	SetDlgItemText (TPack_DlgHandle, IDC_RENDERWIN, "");

	{
		// the application's message loop
		MSG Msg;
		HACCEL accel = LoadAccelerators (instance, MAKEINTRESOURCE (IDR_ACCELERATOR1));

		// do the message loop thing...
		while (GetMessage( &Msg, NULL, 0, 0))
		{
			/*
			  This is kind of weird.  
			  The main window is a dialog that has a menu.
			  In order for accelerators to work, we have to check the accelerators
			  first, then IsDialogMessage, then standard Windows message processing.
			  Any other order causes some weird behavior.
			*/
			if ((accel == NULL) || !TranslateAccelerator (TPack_DlgHandle, accel, &Msg))
			{
				if (!IsDialogMessage (TPack_DlgHandle, &Msg))
				{
					TranslateMessage(&Msg);
					DispatchMessage(&Msg);
				}
			}
		}
	}	

	return 0;
}



// Extract all resource entries to individual BMP files.
// Added DJT (see revision note - 4/18/99)

#include <windowsx.h>      // Because I like those crackers.
static int WriteBMP8(const char * pszFile, geBitmap *Bitmap);

// Error codes
#define TPACKERROR_OK                0    
#define TPACKERROR_UNKNOWN           (TPACKERROR_OK - 1)    
#define TPACKERROR_WRITE             (TPACKERROR_OK - 2)
#define TPACKERROR_MEMORYALLOCATION  (TPACKERROR_OK - 3)
#define TPACKERROR_CREATEFILE        (TPACKERROR_OK - 4)

// Initialize the pulldown menu.
// 1) Make sure the extract options are not available,
// when there are no file to extract.
void HandleInitMenu(TPack_WindowData * pData, HMENU hMenu)
{
	HWND  hLB = GetDlgItem(pData->hwnd, IDC_TEXTURELIST);

	if (ListBox_GetCount(hLB) > 0)
	{
		EnableMenuItem(hMenu, ID_OPTIONS_EXTRACTALL, MF_ENABLED);
		if (ListBox_GetCurSel(hLB) != LB_ERR)
			EnableMenuItem(hMenu, ID_OPTIONS_EXTRACTSELECTED, MF_ENABLED);
		else
			EnableMenuItem(hMenu, ID_OPTIONS_EXTRACTSELECTED, MF_GRAYED);
	}
	else
	{
		EnableMenuItem(hMenu, ID_OPTIONS_EXTRACTALL, MF_GRAYED);
		EnableMenuItem(hMenu, ID_OPTIONS_EXTRACTSELECTED, MF_GRAYED);
	}
}


void TPack_ExtractAll(TPack_WindowData * pData)
{
	HWND            hLB = GetDlgItem(pData->hwnd, IDC_TEXTURELIST);
	int             nCount;
	int             i;
	BitmapEntry *	pEntry;
	char            szName[MAX_TEXTURE_NAME_LENGTH];
	char            szFile[MAX_PATH];
	char            szPath[MAX_PATH];
	int             nErrorCode;
	HCURSOR         hCursor;

	//----------------------------------------------
	// Travese list box.
	// For each list box entry.
	//    Get the  geBitmap
	//    Write 8-bit BMP file.
	//----------------------------------------------

	// This may take a while, let them know something is happening
	// NOTE: Ideally a progress bar is needed.
	hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	ShowCursor(FALSE);
	ShowCursor(TRUE);


	// Ouput to the current directory
	GetCurrentDirectory(MAX_PATH, szPath);

	nCount = ListBox_GetCount(hLB);

	for (i = 0; i < nCount; i++)
	{
		if (ListBox_GetText(hLB, i, szName) != LB_ERR)
		{
			pEntry = FindBitmap(pData, szName);
			if (pEntry)
			{
				strcpy(szFile, szPath);
				strcat(szFile, "\\");
				strcat(szFile, pEntry->Name);
				strcat(szFile, ".bmp");

				nErrorCode = WriteBMP8(szFile, pEntry->Bitmap);

				if (nErrorCode != TPACKERROR_OK)
				{
					// Error writing this bitmap
					switch (nErrorCode)
					{
						case TPACKERROR_CREATEFILE:
							NonFatalError("Unable to create output file %s", szFile);
							break;
						case TPACKERROR_WRITE:
							NonFatalError("I/O error writing %s", szFile);
							break;
						case TPACKERROR_MEMORYALLOCATION:
							NonFatalError("Memory allocation error writing %s", szFile);
							break;
						case TPACKERROR_UNKNOWN:
						default:
							NonFatalError("UInknown error writing %s", szFile);
					}

					// Exit extract all loop on error
					break;
				}
			}
		}
	}

	// Restore the cursor
	SetCursor(hCursor);

}


void TPack_ExtractSelected(TPack_WindowData * pData)
{
	HWND            hLB = GetDlgItem(pData->hwnd, IDC_TEXTURELIST);
	int             nSel;
	BitmapEntry *	pEntry;
	char            szName[MAX_TEXTURE_NAME_LENGTH];
	char            szFile[MAX_PATH];
	char            szPath[MAX_PATH];
	int             nErrorCode;

	//----------------------------------------------
	// Get current selected list box entry.
	// Get the  geBitmap.
	// Write 8-bit BMP file.
	//----------------------------------------------

	// Ouput to the current directory
	GetCurrentDirectory(MAX_PATH, szPath);

	nSel = ListBox_GetCurSel(hLB);
	if (nSel != LB_ERR)

	if (ListBox_GetText(hLB, nSel, szName) != LB_ERR)
	{
		pEntry = FindBitmap(pData, szName);
		if (pEntry)
		{
			// Create an output file name
			strcpy(szFile, szPath);
			strcat(szFile, "\\");
			strcat(szFile, pEntry->Name);
			strcat(szFile, ".bmp");

			nErrorCode = WriteBMP8(szFile, pEntry->Bitmap);

			if (nErrorCode != TPACKERROR_OK)
			{
				// Error writing this bitmap
				switch (nErrorCode)
				{
					case TPACKERROR_CREATEFILE:
						NonFatalError("Unable to create output file %s", szFile);
						break;
					case TPACKERROR_WRITE:
						NonFatalError("I/O error writing %s", szFile);
						break;
					case TPACKERROR_MEMORYALLOCATION:
						NonFatalError("Memory allocation error writing %s", szFile);
						break;
					case TPACKERROR_UNKNOWN:
					default:
						NonFatalError("UInknown error writing %s", szFile);
				}
			}
		}
	}
}



// 8-bit bitmap info
typedef struct tagMY_BITMAPINFO
{
	BITMAPINFOHEADER bmiHeader; 
	RGBQUAD          bmiColors[256]; 
} MY_BITMAPINFO, *pMY_BITMAPINFO, **ppMY_BITMAPINFO; 

#define PAD32(i)     (((i)+31)/32*4)

static int WriteBMP8(const char * pszFile, geBitmap *pBitmap)
{
	geBitmap *       pLock = NULL;
	gePixelFormat    Format;
	geBitmap_Info    BitmapInfo;
	int              nErrorCode = TPACKERROR_UNKNOWN;      // Return code
	BITMAPFILEHEADER BmpHeader;                            // bitmap file-header 
	MY_BITMAPINFO    BmInfo;
	uint32           nBytesPerPixel;
	void *           pPixelData;
	uint8 *          pOut = NULL;
	int              nNewStride = 0;
	int              nOldStride = 0;
	int              i;
	HANDLE           hFile = NULL;
	DWORD            nBytesWritten;
//	uint8            PaletteData[768];        // palette data (see note below)

	// Create the .BMP file.
	hFile = CreateFile(pszFile, 
                       GENERIC_READ | GENERIC_WRITE,
				       (DWORD) 0, 
                       NULL,
				       CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL,
				       (HANDLE) NULL);

	if (hFile == INVALID_HANDLE_VALUE)

		return TPACKERROR_CREATEFILE;

	// get 8-bit palettized bitmap
	Format = GE_PIXELFORMAT_8BIT;

	if ( geBitmap_GetBits(pBitmap))
	{
		pLock = pBitmap;
	}
	else
	{
		if (! geBitmap_LockForRead(pBitmap, &pLock, 0, 0, Format,	GE_FALSE,0) )
		{
			return FALSE;
		}
	}

	geBitmap_GetInfo(pLock, &BitmapInfo, NULL);
	if ( BitmapInfo.Format != Format )
	{
		nErrorCode = TPACKERROR_UNKNOWN;
		goto ExitWriteBitmap;
	}


//	if (!geBitmap_Palette_Lock(BitmapInfo.Palette, (void **)&PaletteData, NULL ,NULL))
//		goto ExitWriteBitmap;

	// NOTE: For some reason the code below for getting the palette data doesn't work.
	//       Incorrect palette data is returned.
	//       Because of that I'm using the much slower geBitmap_Palette_GetEntryColor()
	//       method (below). This seems to produce the correct results.

//	// Get the palette array
//	if (!geBitmap_Palette_GetData(BitmapInfo.Palette, (void *)PaletteData, GE_PIXELFORMAT_8BIT, 255))
//		goto ExitWriteBitmap;

//	// Save the palette
//	for (i = 0; i < 256; i++)
//	{
//		BmInfo.bmiColors[i].rgbRed      = PaletteData[i];
//		BmInfo.bmiColors[i].rgbGreen    = PaletteData[i+1];
//		BmInfo.bmiColors[i].rgbBlue     = PaletteData[i+2];
//		BmInfo.bmiColors[i].rgbReserved = 0;
//	}

	for (i = 0; i < 256; i++)
	{
		int r, g, b, a;
		geBitmap_Palette_GetEntryColor(BitmapInfo.Palette, i, &r, &g, &b, &a);

		BmInfo.bmiColors[i].rgbRed      = (uint8)r;
		BmInfo.bmiColors[i].rgbGreen    = (uint8)g;
		BmInfo.bmiColors[i].rgbBlue     = (uint8)b;
		BmInfo.bmiColors[i].rgbReserved = (uint8)0;
	}


//	geBitmap_Palette_UnLock(BitmapInfo.Palette);


	nBytesPerPixel = gePixelFormat_BytesPerPel(Format);
	pPixelData     = geBitmap_GetBits(pLock);

	// Build bitmap info
	BmInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
	BmInfo.bmiHeader.biWidth         = BitmapInfo.Width;
	BmInfo.bmiHeader.biHeight        = BitmapInfo.Height;    // Bitmap are bottom-up
	BmInfo.bmiHeader.biPlanes        = 1;
	BmInfo.bmiHeader.biBitCount      = (WORD)8;
	BmInfo.bmiHeader.biCompression   = BI_RGB;
	BmInfo.bmiHeader.biSizeImage     = 0;
	BmInfo.bmiHeader.biXPelsPerMeter = BmInfo.bmiHeader.biYPelsPerMeter = 0;   // 10000;

	if (BmInfo.bmiHeader.biBitCount< 24) 
		BmInfo.bmiHeader.biClrUsed = (1 << BmInfo.bmiHeader.biBitCount);
	else
		BmInfo.bmiHeader.biClrUsed = 0;

	BmInfo.bmiHeader.biClrImportant  = 0;

	nNewStride   = PAD32(BitmapInfo.Width * BmInfo.bmiHeader.biBitCount);
	nOldStride   = BitmapInfo.Width * nBytesPerPixel;   

	BmInfo.bmiHeader.biSizeImage     = nNewStride * BitmapInfo.Height;

	// Bitmap scanlines are padded to the nearest dword. If the pixel data from pBitmap
	// is not a the correct width, we need to fix it.
	// NOTE: The best solution is to write each scanline, That way we don't
	//       have to allocate a new pixel buffer.
	if (nNewStride == nOldStride)
	{
		pOut = (uint8 *)pPixelData;
	}

	// Allocate new pixel buffer.
	else
	{
		uint8 *pNew;
		uint8 *pOld;
		int    y;

		pOut = (uint8 *)geRam_Allocate(nNewStride * BitmapInfo.Height);
		if (pOut == (uint8 *)0)
		{
			// Memory allocation error
			nErrorCode = TPACKERROR_MEMORYALLOCATION;
			goto ExitWriteBitmap;
		}


		pNew = (uint8 *)pOut;
		pOld = (uint8 *)pPixelData;

		// Copy old to new
		for (y = 0; y < BitmapInfo.Height; y++)
		{
			memcpy(pNew, pOld, nOldStride);

			// Next row
			pOld += nOldStride;
			pNew += nNewStride;
		}
	}

	// Build the file header
    BmpHeader.bfType = 0x4d42;        // 0x42 = "B" 0x4d = "M" 

    // Compute the size of the entire file. 
    BmpHeader.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) +  
	                          BmInfo.bmiHeader.biSize + 
	                          (BmInfo.bmiHeader.biClrUsed  * sizeof(RGBQUAD)) + 
	                          (nNewStride * BitmapInfo.Height)); 
    BmpHeader.bfReserved1 = 0;
	BmpHeader.bfReserved2 = 0; 

    // Compute the offset to the array of color indices. 
    BmpHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + 
	                             BmInfo.bmiHeader.biSize + 
	                             (BmInfo.bmiHeader.biClrUsed * sizeof(RGBQUAD)); 

	// Write the BMP file header
    if (!WriteFile(hFile, (LPVOID)&BmpHeader, sizeof(BITMAPFILEHEADER), (LPDWORD)&nBytesWritten, (NULL)))
	{
		nErrorCode = TPACKERROR_WRITE;
		goto ExitWriteBitmap;
	}

	// Write the Bitmap infor header and palette
	if (!WriteFile(hFile, (LPVOID)&BmInfo, sizeof(MY_BITMAPINFO), (LPDWORD)&nBytesWritten, (NULL)))
	{
		nErrorCode = TPACKERROR_WRITE;
		goto ExitWriteBitmap;
	}

	// Write the pixel data
    if (!WriteFile(hFile, (LPVOID)pOut, nNewStride * BitmapInfo.Height, (LPDWORD)&nBytesWritten, (NULL)))
	{
		nErrorCode = TPACKERROR_WRITE;
		goto ExitWriteBitmap;
	}


	CloseHandle(hFile);
	hFile = NULL;

	// Success!
	nErrorCode = TPACKERROR_OK;

ExitWriteBitmap:

	// Clean-up
	//------------------------------------
	// Make sure the file gets closed
	if (hFile)
		CloseHandle(hFile);

	// If the temp pixel buffer was allocated, then free it
	if (pOut && nNewStride != nOldStride)
		geRam_Free(pOut);

	// Unlock the geBitmap
	if ( pLock != pBitmap )
	{
		geBitmap_UnLock (pLock);
	}

	return nErrorCode;
}


#pragma warning (default:4100)

