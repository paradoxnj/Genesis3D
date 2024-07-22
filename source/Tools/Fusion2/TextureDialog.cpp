/****************************************************************************************/
/*  TextureDialog.cpp                                                                   */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Moore                        */
/*  Description:  Texture tab code                                                      */
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
#include "TextureDialog.h"
#include "FusionTabControls.h"
#include "FUSIONDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LB_BORDER_SIZE_TOP	5
#define LB_BORDER_SIZE_LEFT	5
#define LB_WIDTH	150
#define LB_HEIGHT	175

#define BM_BORDER_SIZE_TOP	185
#define BM_BORDER_SIZE_LEFT 60
#define BM_WIDTH	256
#define BM_HEIGHT	256

#define SIZETEXT_BORDER_SIZE_TOP	452
#define SIZETEXT_BORDER_SIZE_LEFT	60

//extern	TexInfo	TexInf[MAX_EDGES];

CTextureDialog::CTextureDialog(CFusionTabControls* pParent /*=NULL*/, CFusionDoc* pDoc)
	: CDialog(CTextureDialog::IDD, (CWnd*) pParent)
{
	//{{AFX_DATA_INIT(CTextureDialog)
	//}}AFX_DATA_INIT

	//	Let's save our pointers...
	m_pDoc = pDoc;
	m_pParentCtrl = pParent;

	CDialog::Create( IDD, (CWnd*) pParent );
	//SetupDialog();
}

CTextureDialog::~CTextureDialog()
{
	m_TxlibChanged = false;
}

void CTextureDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextureDialog)
	DDX_Control(pDX, IDC_SIZETEXT, m_SizeText);
	DDX_Control(pDX, IDC_APPLYTEXTURE, m_ApplyButton);
	DDX_Control(pDX, IDC_TEXTUREIMAGE, m_TextureImage);
	DDX_Control(pDX, IDC_TEXTURELIST, m_TextureList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextureDialog, CDialog)
	//{{AFX_MSG_MAP(CTextureDialog)
	ON_BN_CLICKED(IDC_APPLYTEXTURE, OnApply)
	ON_CBN_SELCHANGE(IDC_TEXTURELIST, OnSelchangetexturelist)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextureDialog message handlers

void CTextureDialog::FillTheListbox ()
{
	CWadFile *pWad;

	m_TextureList.ResetContent();
	pWad = Level_GetWadFile (m_pDoc->pLevel);

	// fill list box with texture names...
	for (int index = 0; index < pWad->mBitmapCount; index++)
	{
		CString Name = pWad->mBitmaps[index].Name;

		// Listbox is sorted, so we need to put the wad file index into the item data.
		int lbIndex = m_TextureList.AddString( (LPCTSTR) Name );
		if (lbIndex != LB_ERR)
		{
			m_TextureList.SetItemData (lbIndex, index);
		}
	}
}

BOOL CTextureDialog::OnInitDialog()
{
	//
	// At this point, all of the controls are created, but
	// they're in the wrong places.
	// We position and size them here...
	//
	CDialog::OnInitDialog ();

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

	rect.left = LB_BORDER_SIZE_LEFT;
	rect.top = LB_BORDER_SIZE_TOP;
	rect.right = rect.left + LB_WIDTH;
	rect.bottom = rect.top + LB_HEIGHT;

	m_TextureList.SetWindowPos 
		(
			NULL, 
			LB_BORDER_SIZE_LEFT, LB_BORDER_SIZE_TOP,  // position
			LB_WIDTH, LB_HEIGHT, // size
			(SWP_NOACTIVATE | SWP_NOZORDER)
		);

//	pWad = Level_GetWadFile (m_pDoc->pLevel);
	FillTheListbox ();

	// move the button...
	int left = rect.right + 10;
	int top = rect.top;

	// put the button to the right of the listbox
	m_ApplyButton.SetWindowPos (NULL, left, top, 0, 0, (SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));

	// and the size text at the bottom of the listbox
	m_SizeText.GetClientRect (&rect2);
	top = rect.bottom - rect2.bottom;
	m_SizeText.SetWindowPos (NULL, left, top, 0, 0, (SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER));

	//	Create the bitmap image...
	m_TextureRect.top = rect.bottom + 10;  //BM_BORDER_SIZE_TOP;
	m_TextureRect.left = rect.left;  //BM_BORDER_SIZE_LEFT;
	m_TextureRect.bottom = m_TextureRect.top + BM_HEIGHT;
	m_TextureRect.right = m_TextureRect.left + BM_WIDTH;

	m_TextureImage.SetWindowPos 
		(
			NULL, 
			m_TextureRect.left, m_TextureRect.top,
			BM_HEIGHT, BM_WIDTH,
			(SWP_NOACTIVATE | SWP_NOZORDER)
		);
	
	//	Set the current selection to the first...
	int index=0;

	CString FirstName;

	m_TextureList.SetCurSel(0);
	index = m_TextureList.GetCurSel ();
	if (index == LB_ERR)
	{
		m_CurrentTexture = "Invalid";
		return TRUE;
	}
	index = 0;
	m_TextureList.GetText(0, FirstName);
	while(!isalpha(FirstName[0]))
		m_TextureList.GetText(++index, FirstName);

	m_TextureList.GetText( m_TextureList.GetCurSel(), FirstName );
	m_CurrentTexture = FirstName;

	// Update bitmap image and size
	UpdateBitmap();
	UpdateSize();

	return TRUE;
}

static void TextureFace
	(
	  Face *pFace,
	  int SelId,
	  char const *Name
	)
{
	Face_SetTextureDibId (pFace, SelId);
	Face_SetTextureName (pFace, Name);
}

static void TextureBrushList(BrushList *pList, int SelId, char const *Name);

static void TextureBrush
	(
	  Brush *pBrush,
	  int SelId,
	  char const *Name
	)
{
	int j;

	assert(pBrush);
	
	if(Brush_IsMulti(pBrush))
	{
		TextureBrushList((BrushList *)Brush_GetBrushList(pBrush), SelId, Name);
	}
	else
	{
		for (j = 0; j < Brush_GetNumFaces (pBrush); ++j)
		{
			Face *pFace;

			pFace = Brush_GetFace (pBrush, j);
			TextureFace (pFace, SelId, Name);
		}
	}
}

static void TextureBrushList
	(
	  BrushList *pList,
	  int SelId,
	  char const *Name
	)
{
	Brush *b;
	BrushIterator bi;

	assert(pList);
	assert(Name);

#pragma todo ("Change this and the function above to use BrushList_EnumAll")
	for(b=BrushList_GetFirst(pList, &bi);b;b=BrushList_GetNext(&bi))
	{
		TextureBrush(b, SelId, Name);
	}	
}


//	We need to set the current texture to all applicable faces
//	in the document's active brush list...
void CTextureDialog::OnApply() 
{
	int SelectedItem;
	int SelId;
	int		i;
	
	SelectedItem	=m_TextureList.GetCurSel();
	SelId = m_TextureList.GetItemData (SelectedItem);

	if(m_pDoc->mModeTool==ID_TOOLS_TEMPLATE)
		return;
	
	switch (m_pDoc->mAdjustMode)
	{
		case ADJUST_MODE_FACE :
		{
			int Size;

			Size = SelFaceList_GetSize (m_pDoc->pSelFaces);
			for (i = 0; i < Size; ++i)
			{
				Face *pFace;
				pFace = SelFaceList_GetFace (m_pDoc->pSelFaces, i);

				::TextureFace (pFace, SelId, (LPCSTR)m_CurrentTexture);
			}
			// have to go through the selected brushes and update their child faces
			int NumSelBrushes = SelBrushList_GetSize (m_pDoc->pSelBrushes);
			for (i = 0; i < NumSelBrushes; ++i)
			{
				Brush *pBrush;

				pBrush = SelBrushList_GetBrush (m_pDoc->pSelBrushes, i);
				Brush_UpdateChildFaces (pBrush);
			}
			break;
		}

		case ADJUST_MODE_BRUSH :
		{
			if(m_pDoc->GetSelState() & MULTIBRUSH)
			{
				int NumSelBrushes = SelBrushList_GetSize (m_pDoc->pSelBrushes);
				for (i = 0; i < NumSelBrushes; ++i)
				{
					Brush *pBrush = SelBrushList_GetBrush (m_pDoc->pSelBrushes, i);
					::TextureBrush (pBrush, SelId, (LPCSTR)m_CurrentTexture);
					Brush_UpdateChildFaces (pBrush);
				}
			}
			else
			{
				::TextureBrush (m_pDoc->CurBrush, SelId, (LPCSTR)m_CurrentTexture);
				Brush_UpdateChildFaces (m_pDoc->CurBrush);
			}
			break;
		}

		default :
			return;
	}

	if(m_pParentCtrl->LastView)
		m_pParentCtrl->LastView->SetFocus();

	m_pDoc->UpdateAllViews(UAV_ALL3DVIEWS, NULL);
}


void CTextureDialog::OnSelchangetexturelist()
{
	//	Let's set the current texture to this one...
	int SelNum = m_TextureList.GetCurSel();
	
	m_TextureList.GetText( SelNum, m_CurrentTexture );

	UpdateBitmap();
	UpdateSize();

//	ConPrintf("%s:  SelNum %d: Dib %d\n", (LPCSTR)m_CurrentTexture, SelNum, CurTexInfo.Dib);

	//	Also set the current selection value in the document...
	m_pDoc->mCurTextureSelection = SelNum;
}

void CTextureDialog::SelectTexture(int SelNum)
{
	for (int i = 0; i < m_TextureList.GetCount(); ++i)
	{
		int SelId = m_TextureList.GetItemData (i);
		if (SelNum == SelId)
		{
			m_TextureList.SetCurSel(i);
			OnSelchangetexturelist();
			return;
		}
	}
}

void CTextureDialog::UpdateBitmap()
{
	//	Draw the texture image bitmap...
	WadFileEntry* BitmapPtr = m_pDoc->GetDibBitmap( m_CurrentTexture );

	if( BitmapPtr == NULL )
		return;

	if( m_TextureImage.GetSafeHwnd() == NULL )
		return;

	CDC* pDC = m_TextureImage.GetDC();

	ClearTextureImage( pDC );	

	// Create a BITMAPINFOHEADER structure for blitting...
	BITMAPINFOHEADER bmih;
	bmih.biSize = sizeof (BITMAPINFOHEADER);
	bmih.biWidth = BitmapPtr->Width;
	bmih.biHeight = -BitmapPtr->Height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 16;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = 0;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
 
	if( StretchDIBits(
		(HDC)*pDC,
		0,
		0,
		BitmapPtr->Width,
		BitmapPtr->Height,
	   	0,
		0,
		BitmapPtr->Width,
		BitmapPtr->Height,
		BitmapPtr->BitsPtr,
		(BITMAPINFO *) &bmih,
		DIB_RGB_COLORS,
		SRCCOPY ) == GDI_ERROR )
	{
		TRACE("Could not blit to screen!");
		_ASSERT(0);
	}

	m_TextureImage.ReleaseDC( pDC );
}

void CTextureDialog::ClearTextureImage(CDC* pDC)
{
	/*	This is a little hack.  We're grabbing the color of
		the pixel in the main dialog and using that color to
		clear the texture image space.

		For the moment, I can't think of an easier way to just
		grab the dialog's color.

		- John Moore
	*/

	CBrush* pBrush;
	CDC* pDCDialog = GetDC();
	COLORREF BkColor = pDCDialog->GetPixel( 0, 0 );
	ReleaseDC( pDCDialog );

	pBrush = new CBrush( BkColor );
	pDC->SelectObject( pBrush );

	CRgn rgn;

	rgn.CreateRectRgn( 0, 0, BM_WIDTH, BM_HEIGHT );

	pDC->PaintRgn( &rgn );

	delete pBrush;
}

void CTextureDialog::UpdateSize()
{
	CString size;
	int width, height;
	WadFileEntry* BitmapPtr = m_pDoc->GetDibBitmap( m_CurrentTexture );

	if( BitmapPtr == NULL )
		return;

	width = BitmapPtr->Width;
	height = BitmapPtr->Height;

	size.Format( "%d X %d", width, height );
	m_SizeText.SetWindowText (size);

}
/*
void CTextureDialog::ClearSizeImage( CDC* pDC )
{
	//	Same as clearing a bitmap area, only for the size string...
	CBrush* pBrush;
	CDC* pDCDialog = GetDC();
	COLORREF BkColor = pDCDialog->GetPixel( 0, 0 );
	ReleaseDC( pDCDialog );

	pBrush = new CBrush( BkColor );
	pDC->SelectObject( pBrush );
	pDC->SetBkColor( BkColor );

	CRgn rgn;

	RECT rect;
	rect.left = SIZETEXT_BORDER_SIZE_LEFT;
	rect.top = SIZETEXT_BORDER_SIZE_TOP;
	rect.right = rect.left + 70;
	rect.bottom = rect.top + 16;

	rgn.CreateRectRgn(
		rect.left,
		rect.top,
		rect.right,
		rect.bottom );

	pDC->PaintRgn( &rgn );

	delete pBrush;
}
*/


BOOL CTextureDialog::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo) 
{
	switch( nID )
	{
	case IDC_TEXTURELIST:
		return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

	case IDC_APPLYTEXTURE:
		return CDialog::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

	default:
		//	Let's update stuff first...
		//UpdateBitmap();
		//UpdateSize();
		return TRUE;
	}
}

void CTextureDialog::Update( CFusionDoc* pDoc )
{
//	int index;
//	int PolyNum;
	CString Name;
	CString FirstName;

	if ((m_pDoc == pDoc) && (m_TxlibChanged == false))
	{
		//	Document is the same, so just redraw stuff...
		UpdateBitmap();
		UpdateSize();
	}
	else if (pDoc)
	{
		int CurSel;
//		CWadFile *pWad;

		m_pDoc = pDoc;
		//	Document changed, so we need to change our list values...
		FillTheListbox ();
/*
		m_TextureList.ResetContent();
		pWad = Level_GetWadFile (m_pDoc->pLevel);
		for (index = 0; index < pWad->mBitmapCount; index++)
		{
			//	Insert the names, one by one...
			Name = pWad->mBitmaps[index].Name;

			int lbIndex = m_TextureList.AddString( (LPCTSTR) Name );
			if (lbIndex != LB_ERR)
			{
				m_TextureList.SetItemData (lbIndex, index);
			}

		}
*/
		//	Set the current selection to whatever is pointed to in the document...
		m_TextureList.SetCurSel( m_pDoc->mCurTextureSelection );
		CurSel = m_TextureList.GetCurSel ();
		if (CurSel != LB_ERR)
		{
			m_TextureList.GetText( CurSel, FirstName );
			m_CurrentTexture = FirstName;

			//	update the bitmap image...
			UpdateBitmap();

			//	Finally, update the size...
			UpdateSize();
		}
	}
	else
	{
		//	Documents all closed.  Update any values that need to be...
		m_pDoc = NULL;
		m_TextureList.ResetContent();
	}
	m_TxlibChanged = false;
}

void CTextureDialog::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	//	Let's redraw our bitmap and size...
	UpdateBitmap();
	UpdateSize();
}

const char *CTextureDialog::GetCurrentTexture()
{
	return	(LPCSTR)m_CurrentTexture;
}
