/****************************************************************************************/
/*  WINUTIL.C                                                                           */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Windows helper functions.												*/
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
#include "WinUtil.h"
#include <stdio.h>

#pragma warning (disable:4514)

// Set an edit box with floating point value
void WinUtil_SetDlgItemFloat
	(
	  HWND hwnd,
	  int ControlId,
	  geFloat fVal
	)
{
	char sVal[100];

	sprintf (sVal, "%.2f", fVal);
	SetDlgItemText (hwnd, ControlId, sVal);
}

// Get float from a dialog box.
BOOL WinUtil_GetDlgItemFloat 
	(
	  HWND hwnd, 
	  int ControlId, 
	  geFloat *pVal
	)
{
	char sVal[100];
	geFloat fVal;

	GetDlgItemText (hwnd, ControlId, sVal, sizeof (sVal));
	if (sscanf (sVal, "%f", &fVal) != 1)
	{
		return FALSE;
	}
	*pVal = fVal;
	return TRUE;
}

// Convert a screen-relative rectangle to client coordinates.
BOOL WinUtil_ScreenRectToClient
	(
	  HWND hwnd,
	  RECT *pRect
	)
{
	POINT ptLT, ptRB;
	
	ptLT.x = pRect->left;
	ptLT.y = pRect->top;
	if (ScreenToClient (hwnd, &ptLT))
	{
		ptRB.x = pRect->right;
		ptRB.y = pRect->bottom;
		if (ScreenToClient (hwnd, &ptRB))
		{
			pRect->left = ptLT.x;
			pRect->top = ptLT.y;
			pRect->right = ptRB.x;
			pRect->bottom = ptRB.y;
			return TRUE;
		}
	}
	return FALSE;
}

// Enable a dialog box item.
void WinUtil_EnableDlgItem
	(
	  HWND hwnd,
	  int ControlId,
	  BOOL Enable
	)
{
	EnableWindow (GetDlgItem (hwnd, ControlId), Enable);
}
