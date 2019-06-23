/****************************************************************************************/
/*  ColorBtn.cpp                                                                        */
/*                                                                                      */
/*  Author:       Jim Mischel, Jeff Lomax                                               */
/*  Description:  Dialog code for brush flags                                           */
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
#include "Colorbtn.h"

// CColorButton

CColorButton::CColorButton()
{
	mCurrentColor = RGB(255,0,255);
}

CColorButton::~CColorButton()
{
}


BEGIN_MESSAGE_MAP(CColorButton, CButton)
	//{{AFX_MSG_MAP(CColorButton)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColorButton message handlers

void CColorButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
	RECT ClientRect;

	// get a cdc
	CDC* pDC = CDC::FromHandle( lpDrawItemStruct->hDC );

	// now draw a solid rectangle
	pDC->FillSolidRect( &lpDrawItemStruct->rcItem, mCurrentColor);

	{
		// draw the button's text...
		CString Text;

		this->GetClientRect (&ClientRect);

		this->GetWindowText (Text);
		pDC->DrawText( Text, &ClientRect, DT_CENTER |DT_VCENTER|DT_SINGLELINE ) ;
	}
	// if we have the focus

	if( lpDrawItemStruct->itemState & ODS_FOCUS ) {
		// get a null brush
		CBrush *NullBrush = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH)), *OldBrush;

		// select the brush
		OldBrush = pDC->SelectObject( NullBrush );

		// draw a cute rectangle around it
		pDC->Rectangle(&lpDrawItemStruct->rcItem);

		// get old
		pDC->SelectObject( OldBrush );
	}
	pDC->DrawEdge( &ClientRect, EDGE_RAISED, BF_RECT ) ;

}

// this function returns our current Color
COLORREF CColorButton::GetColor()
{
	return mCurrentColor;
}

void CColorButton::SetColor( COLORREF Color)
{
	mCurrentColor = Color;
	// update ourselves
	RedrawWindow();
}

void CColorButton::OnClicked() 
{
	COLORREF Color ;
	CColorDialog dlg;

	// get a Color
	if (dlg.DoModal() == IDOK)
	{
		// assign what it was to the current Color
		Color = dlg.GetColor() ;
		if( mCurrentColor != Color )
		{
			CWnd * pParent ;

			mCurrentColor = Color ;

			pParent = GetParent( ) ;
			pParent->SendMessage( WM_UPDATEGROUPCOLOR );	// Tell them we changed
		
			// update ourselves
			RedrawWindow();
		}
	}
}/* CColorButton::OnClicked */
