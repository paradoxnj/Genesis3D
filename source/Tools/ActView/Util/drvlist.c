/****************************************************************************************/
/*  DRVLIST.C	                                                                        */
/*                                                                                      */
/*  Author: Modified by Jim Mischel														*/
/*  Description:  Actor Viewer's driver picker.											*/
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


/*
  NOTE:  This version has been modified to list only those drivers that
  have a windowed mode.
*/
#include <windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<assert.h>

#include	"genesis.h"

#include	"resource.h"
#include	"drvlist.h"

#pragma warning(disable : 4514)

typedef	struct	DriverInfo
{
	geDriver * 				diDriver;
	geDriver_Mode *			diMode;
	struct	DriverInfo *	diNext;
}	DriverInfo;

static	geDriver *		PickedDriver;
static	geDriver_Mode *	PickedMode;

static	void	DestroyDriverList(DriverInfo *dlist)
{
	DriverInfo *	temp;

	while	(dlist)
	{
		temp = dlist->diNext;
		free(dlist);
		dlist = temp;
	}
}

static DriverInfo *BuildDriverList(geEngine *Engine)
{
	geDriver_System *	DriverSystem;
	geDriver *			Driver;
	geDriver_Mode *		Mode;
	DriverInfo *		DriverList;

	DriverSystem = geEngine_GetDriverSystem(Engine);
	if	(DriverSystem == NULL)
		return NULL;

	DriverList = NULL;

	Driver = geDriver_SystemGetNextDriver(DriverSystem, NULL);
	while	(Driver != NULL)
	{
		DriverInfo *	dinfo;
		const char *	DriverName;

		geDriver_GetName(Driver, &DriverName);

		Mode = geDriver_GetNextMode(Driver, NULL);
		while	(Mode != NULL)
		{
			const char *	ModeName;

			geDriver_ModeGetName(Mode, &ModeName);

			OutputDebugString (DriverName);
			OutputDebugString (" : ");
			OutputDebugString(ModeName);
			OutputDebugString("\r\n");

			// looking for all drivers that have window mode...
			if(!strnicmp(ModeName, "Window", 6))
			{
				dinfo = malloc(sizeof(*dinfo));
				if	(!dinfo)
				{
					DestroyDriverList(DriverList);
					return NULL;
				}
				dinfo->diNext = DriverList;
								DriverList = dinfo;
				dinfo->diDriver = Driver;
				dinfo->diMode = Mode;
			}
			Mode = geDriver_GetNextMode(Driver, Mode);
		}
		Driver = geDriver_SystemGetNextDriver(DriverSystem, Driver);
	}

	return DriverList;
}

static void SetSelectedDriver (HWND hwndDlg, DriverInfo * DriverList)
{
	int		DriverIdx;

	if	(DriverList)
	{
		DriverInfo *	Temp;
		DriverIdx = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
		Temp = DriverList;
		while	(DriverIdx--)
		{
			Temp = Temp->diNext;
			assert(Temp != NULL);
		}
		PickedDriver = Temp->diDriver;
		PickedMode = Temp->diMode;
	}
}

static	BOOL	CALLBACK	DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static	DriverInfo *	DriverList;

	switch	(uMsg)
	{
	case	WM_INITDIALOG:
		DriverList = (DriverInfo *)lParam;
		if	(DriverList)
		{
			DriverInfo *	temp;
			HWND			DriverListBox;
			HDC				hDC;
			int				MaxCX;

			DriverListBox = GetDlgItem(hwndDlg, IDC_DRIVERLIST);
			hDC = GetDC(DriverListBox);

			MaxCX = 0;

			temp = DriverList;
			while	(temp)
			{
				char			buff[256];
				SIZE			extents;
				const char *	DriverName;
				const char *	ModeName;
				
				geDriver_GetName(temp->diDriver, &DriverName);
				geDriver_ModeGetName(temp->diMode, &ModeName);
				sprintf(buff, "%s %s", DriverName, ModeName);
				SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_ADDSTRING, 0, (LPARAM)buff);
				GetTextExtentPoint32(hDC, buff, strlen(buff), &extents);
				if	(extents.cx > MaxCX)
					MaxCX = extents.cx;
				temp = temp->diNext;
			}

			SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_SETCURSEL, 0, 0);
			SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_SETHORIZONTALEXTENT, MaxCX, 0);

			ReleaseDC(DriverListBox, hDC);

			return TRUE;
		}
		break;


	case	WM_COMMAND :
		switch (LOWORD (wParam))
		{
			case IDC_DRIVERLIST :
				if (HIWORD (wParam) != LBN_DBLCLK)
				{
					break;
				}
				// double click, so fall through to OK
			case IDOK :
			{
				SetSelectedDriver (hwndDlg, DriverList);
				EndDialog(hwndDlg, 1);
				break;
			}
	
			case IDCANCEL :
				EndDialog (hwndDlg, 0);
				break;

			default :
				break;
		}
	}

	return 0;
}

void	DrvList_PickDriver(HANDLE hInstance, HWND hwndParent, geEngine *Engine, geDriver **Driver, geDriver_Mode **Mode)
{
	DriverInfo *	DriverList;
	int				res;

	PickedDriver = NULL;
	PickedMode = NULL;

	DriverList = BuildDriverList(Engine);
	res = DialogBoxParam(hInstance,
						 MAKEINTRESOURCE(IDD_DRIVERDIALOG),
						 hwndParent,
						 DlgProc,
						 (LPARAM)DriverList);

	DestroyDriverList(DriverList);
	DriverList = NULL;

	if	(res == 1)
	{
		*Driver = PickedDriver;
		*Mode = PickedMode;
	}
	else
	{
		*Driver = NULL;
		*Mode = NULL;
	}
}
