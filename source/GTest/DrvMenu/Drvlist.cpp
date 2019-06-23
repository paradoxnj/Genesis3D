/****************************************************************************************/
/*  DrvList.c                                                                           */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:  Dialog control logic for driver/mode selection dialog                 */
/*                 (rewritten from previous version)                                    */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<assert.h>

#include	"genesis.h"

#include	"resource.h"
#include	"drvlist.h"


#define DRVLIST_OK		1
#define DRVLIST_CANCEL	2

#define DRVLIST_MAX_DISPLAY_MODES (40)
typedef struct 
{
	ModeList *ModeList;
	int		  ModeListLength;
	int		  ModeListDriverCount;
	geDriver *IndexTable[DRVLIST_MAX_DISPLAY_MODES];
	int		  Selection;
}	DrvList_LocalStruct;


static DrvList_LocalStruct DrvList_Locals;

static geBoolean DrvList_FillModeList(HWND hwndDlg,int DriverNumber,DrvList_LocalStruct *DL)
{
	int		i;
	int		MaxCX = 0;
	SIZE	extents;
	HWND	ListBox;
	HDC		hDC;
	
	ListBox = GetDlgItem(hwndDlg, IDC_DRIVERLIST);
	if (ListBox ==NULL)
		return GE_FALSE;
		
	hDC = GetDC(ListBox);
	if (hDC == NULL)
		return GE_FALSE;
	
	SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_RESETCONTENT, (WPARAM)0, (LPARAM)0);

	for (i=0; i<DL->ModeListLength; i++)
		{
			if (DL->IndexTable[DriverNumber] == DL->ModeList[i].Driver)
				{
					int index;
					index = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_ADDSTRING, (WPARAM)0, 
												(LPARAM)(DL->ModeList[i].ModeNamePtr));
					SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_SETITEMDATA, (WPARAM)index,(LPARAM)i);
					GetTextExtentPoint32(hDC, DL->ModeList[i].ModeNamePtr, strlen(DL->ModeList[i].ModeNamePtr), &extents);
					if	(extents.cx > MaxCX)
						MaxCX = extents.cx;
				}
		}	
	SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_SETCURSEL, (WPARAM)0, (LPARAM)0);
	SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_SETHORIZONTALEXTENT, (WPARAM)MaxCX, (LPARAM)0);
	ReleaseDC(ListBox, hDC);
	return GE_TRUE;
}



static	BOOL	CALLBACK	DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DrvList_LocalStruct *DL = &DrvList_Locals;
			
	switch	(uMsg)
	{
	case	WM_INITDIALOG:
		{
			HWND			DriverListBox;
			HDC				hDC;
			int				MaxCX;
			SIZE			extents;
			int				DriverNumber;
			int				i,j;

			DriverListBox = GetDlgItem(hwndDlg, IDC_DRIVERLIST);
			hDC = GetDC(DriverListBox);

			MaxCX = 0;

			DriverNumber = 0;
			for (i=0; i<DL->ModeListLength; i++)
				{
					int AlreadyAdded=0;
					
					for (j=0; j<i; j++)
						{
							if (DL->ModeList[j].Driver == DL->ModeList[i].Driver)	// only add one entry for each driver.
								AlreadyAdded = 1;
						}
					
					if (!AlreadyAdded)
						{
							int index;
							index = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_ADDSTRING, (WPARAM)0, (LPARAM)(DL->ModeList[i].DriverNamePtr));
							SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_SETITEMDATA, (WPARAM)index,(LPARAM)DriverNumber);
							GetTextExtentPoint32(hDC, DL->ModeList[i].DriverNamePtr, strlen(DL->ModeList[i].DriverNamePtr), &extents);
							if	(extents.cx > MaxCX)
								MaxCX = extents.cx;
							DL->IndexTable[DriverNumber] = DL->ModeList[i].Driver;
							DriverNumber ++;
						}
					if (DriverNumber >= DRVLIST_MAX_DISPLAY_MODES)
						break;
				}
			DL->ModeListDriverCount = DriverNumber;
		
			SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_SETCURSEL, (WPARAM)0, (LPARAM)0);
			SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_SETHORIZONTALEXTENT, (WPARAM)MaxCX, (LPARAM)0);

			ReleaseDC(DriverListBox, hDC);

			DrvList_FillModeList(hwndDlg,0,DL);

			return TRUE;
		}
		break;

	case	WM_COMMAND:
		switch (LOWORD(wParam))
			{
				case IDC_DRIVERLIST:
					if (HIWORD(wParam) == LBN_SELCHANGE)
						{
							int Driver;
							Driver = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
							if (Driver>=0)
								DrvList_FillModeList(hwndDlg,Driver,DL);
						}
					break;
				case IDC_DRIVERLIST2:
					if (HIWORD(wParam) == LBN_DBLCLK)
						{
							int		index;
							index = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
							if (index>=0)
								{
									DL->Selection = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_GETITEMDATA, index, 0);
									EndDialog(hwndDlg, DRVLIST_OK);
								}
						}
					break;
				case IDOK:
					{
						int		index;
						index = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_GETCURSEL, (WPARAM)0, (LPARAM)0);
						if (index>=0)
							{
								DL->Selection = SendDlgItemMessage(hwndDlg, IDC_DRIVERLIST2, LB_GETITEMDATA, index, 0);
								EndDialog(hwndDlg, DRVLIST_OK);
							}
					}
					break;
				case IDCANCEL:
					EndDialog(hwndDlg, DRVLIST_CANCEL);
					break;
			}
		break;
	}

	return 0;
}

geBoolean DrvList_PickDriver(HINSTANCE hInstance, HWND hwndParent, 
		ModeList *List, int ListLength, int *ListSelection)
{
	int				res;
	DrvList_LocalStruct *DL = &DrvList_Locals;

	assert( hInstance != 0 );
	assert( List  != NULL );
	assert( ListLength >=0 );
	assert( ListSelection != NULL );

	
	DL->ModeList			= List;
	DL->ModeListLength		= ListLength;
	DL->ModeListDriverCount = 0;
	DL->Selection			= -1;

	res = DialogBoxParam(hInstance,
						 MAKEINTRESOURCE(IDD_DRIVERDIALOG),
						 hwndParent,
						 DlgProc,
						 (LPARAM)0);

	*ListSelection = DL->Selection;

	if	(res == DRVLIST_OK || res == DRVLIST_CANCEL)
		{
			return GE_TRUE;
		}

	return GE_FALSE;
}
