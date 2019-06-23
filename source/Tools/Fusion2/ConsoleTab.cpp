/****************************************************************************************/
/*  ConsoleTab.cpp                                                                      */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Cheezy text console                                                   */
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
#include "stdafx.h"
#include "ConsoleTab.h"

#include "FusionTabControls.h"  // this is probably bad
#include "FusionDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static HWND ConHwnd;

/////////////////////////////////////////////////////////////////////////////
// CConsoleTab dialog

CConsoleTab::CConsoleTab(CFusionTabControls* pParent /*=NULL*/, CFusionDoc* pDoc)
	: CDialog(CConsoleTab::IDD, pParent),
	  Font(NULL), bNewFont (FALSE)
{
	//{{AFX_DATA_INIT(CConsoleTab)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pFusionDoc = pDoc;
	m_pParentCtrl = pParent;

	CDialog::Create( IDD, (CWnd*) pParent );
}

void CConsoleTab::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConsoleTab)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

void CConsoleTab::SetConHwnd(void)
{
	GetDlgItem(IDC_CONEDIT, &ConHwnd);
}

BEGIN_MESSAGE_MAP(CConsoleTab, CDialog)
	//{{AFX_MSG_MAP(CConsoleTab)
	ON_WM_DESTROY()
	ON_EN_MAXTEXT(IDC_CONEDIT, OnMaxtextConedit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConsoleTab message handlers

BOOL CConsoleTab::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	//	Position the new dialog on the tab...
	RECT rect;
	RECT rect2;

	m_pParentCtrl->GetClientRect( &rect );
	m_pParentCtrl->GetItemRect( 0, &rect2 );
	rect.top = rect2.bottom + FTC_BORDER_SIZE_TOP;
	rect.left = rect.left + FTC_BORDER_SIZE_LEFT;
	rect.right = rect.right - FTC_BORDER_SIZE_RIGHT ;
	rect.bottom = rect.bottom - FTC_BORDER_SIZE_BOTTOM ;

	SetWindowPos( NULL, rect.left,
			rect.top, rect.right, rect.bottom, SWP_NOZORDER );

	GetDlgItem(IDC_CONEDIT, &ConHwnd);
	GetDlgItem(IDC_CONEDIT)->SetWindowPos
	(
		NULL, 
		FTC_BORDER_SIZE_LEFT,	FTC_BORDER_SIZE_TOP, 
		rect.right-4,			rect.bottom-28, 
		SWP_NOZORDER 
	);


#if 1
	Font = ::CreateFont
		(
			11,						// logical height of font
			0,						// logical average character width
			0,						// angle of escapement
			0,						// base-line orientation angle
			FW_NORMAL,				// font weight
			0,						// italic attribute flag
			0,						// underline attribute flag
			0,						// strikeout attribute flag
			ANSI_CHARSET,			// character set identifier
			OUT_DEFAULT_PRECIS,		// output precision
			CLIP_DEFAULT_PRECIS,	// clipping precision
			DRAFT_QUALITY,			// output quality
			DEFAULT_PITCH,			// pitch and family
			"MS Sans Serif"			// pointer to typeface name string
		);
#endif
	if (Font == NULL)
	{
		Font = (HFONT)(::GetStockObject (SYSTEM_FIXED_FONT));
	}
	else
	{	
		bNewFont = TRUE;
	}

	if (ConHwnd != NULL)
	{
		::SendMessage (ConHwnd, WM_SETFONT, (WPARAM)Font, FALSE);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CConsoleTab::OnDestroy() 
{
	CDialog::OnDestroy();

	//Set the hwnd for ConPrintf to null
	if (bNewFont != FALSE)
	{
		::DeleteObject (Font);
		Font = NULL;
	}
	ConHwnd=NULL;
}

char *TranslateString(char *buf)
{
	static	char	buf2[32768];
	char			*out;
	unsigned int	i;

	for(i=0, out=buf2;i < strlen(buf);i++)
	{
		if(buf[i]=='\n')
		{
			*out++	='\r';
			*out++	='\n';
		}
		else
		{
			*out++	=buf[i];
		}
	}
	*out++	=0;

	return	buf2;
}

extern "C" void ConClear(void)
{
	char	text[4];

	text[0]	=0;

	if(ConHwnd)
	{
		SendMessage(ConHwnd, WM_SETTEXT, 0, (LPARAM)text);
	}
}

extern "C" void ConPrintf(char *text, ...)
{
	char	buf[32768];		//this is ... cautious
	char	*out;
	va_list argptr;

	va_start(argptr, text);
	vsprintf(buf, text, argptr);
	va_end(argptr);

	out	=TranslateString(buf);

	if(ConHwnd)
	{
		SendMessage(ConHwnd, EM_REPLACESEL, 0, (LPARAM)out);
	}
}

extern "C" void ConError(char *text, ...)
{
	char	buf[32768];		//this is ... cautious
	char	*out;
	va_list argptr;

	va_start(argptr, text);
	vsprintf(buf, text, argptr);
	va_end(argptr);

	out	=TranslateString(buf);

	if(ConHwnd)
	{
		SendMessage(ConHwnd, EM_REPLACESEL, 0, (LPARAM)out);
	}
}

void CConsoleTab::OnMaxtextConedit(void) 
{
	ConClear();
}
