/****************************************************************************************/
/*  ABOUT.C		                                                                        */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Actor Viewer's About dialog.											*/
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
#include "about.h"
#include "resource.h"

#pragma warning (disable:4514)	// unreferenced inline function

// AboutDlgProc -- Dialog procedure for simple about dialog box.
#pragma warning (disable:4100)
static BOOL CALLBACK About_DlgProc
	(
	  HWND hwnd,
	  UINT msg,
	  WPARAM wParam,
	  LPARAM lParam
	)
{
	switch (msg)
	{
		case WM_INITDIALOG :
			return TRUE;
		case WM_COMMAND :
		{
			int wID = LOWORD (wParam);
			
			if ((wID == IDOK) || (wID == IDCANCEL))
			{
				EndDialog (hwnd, wID);
			}
		}
	}
	return FALSE;
}
#pragma warning (default:4100)


// Just display the dialog box...
void About_DoDialog
	(
	  HWND hwndParent,
	  HINSTANCE hinst
	)
{
	DialogBox 
	(
	  hinst,
	  MAKEINTRESOURCE (IDD_ABOUT),
	  hwndParent,
	  About_DlgProc
	);
}
