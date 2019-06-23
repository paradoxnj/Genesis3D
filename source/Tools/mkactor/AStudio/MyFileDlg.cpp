/****************************************************************************************/
/*  MYFILEDLG.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Custom CFileDialog.													*/
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

#include "MyFileDlg.h"


// Create and return a CFileDialog that has the specified Title,
// default extension, and filter.
CFileDialog *MyFileDialog_Create
	(
	  BOOL bOpenFileDialog,
	  int nTitleId,
	  int nDefExtId,
	  int nFilterId,
	  const char *lpszFileName,
	  DWORD dwFlags,
	  CWnd *pParentWnd
	)
{
	CString DefExt, Filter;
	
	// This is a really ugly hack that prevents us from having more than one file dialog
	// open at any time.  Tough.  The CFileDialog interface is so screwed up that we
	// can't reasonably inherit from it without also writing a wrapper like this one.
	static CString Title;

	DefExt.LoadString (nDefExtId);
	Filter.LoadString (nFilterId);
	Title.LoadString (nTitleId);

	CFileDialog *FileDlg = new CFileDialog (bOpenFileDialog, DefExt, lpszFileName, dwFlags, Filter, pParentWnd);
	
	FileDlg->m_ofn.lpstrTitle = Title;
	return FileDlg;
}
