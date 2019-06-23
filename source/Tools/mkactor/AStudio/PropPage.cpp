/****************************************************************************************/
/*  PROPPAGE.CPP																		*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Actor Studio property page parent class.								*/
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
#include "PropPage.h"
#include <assert.h>
#include "resource.h"

/////////////////////////////////////////////////////////////////////////////
// CAStudioPropPage property page

IMPLEMENT_DYNCREATE(CAStudioPropPage, CPropertyPage)

BEGIN_MESSAGE_MAP(CAStudioPropPage, CPropertyPage)
	//{{AFX_MSG_MAP(CAStudioPropPage)
//	ON_BN_CLICKED(IDC_MESSAGES, OnMessages)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// Common initialization called by the 2 different constructors because MFC
// won't let me use a common constructor.  Grrrrr.....
void CAStudioPropPage::CommonInit (void)
{
	m_Modified = false;
	m_Compiling = false;
	m_Project = NULL;
}

CAStudioPropPage::CAStudioPropPage (LPCSTR lpszTemplateName, UINT nIDCaption) :
	CPropertyPage (lpszTemplateName, nIDCaption)
{
	CommonInit ();
}

CAStudioPropPage::CAStudioPropPage (UINT nIDTemplate, UINT nIDCaption) :
	CPropertyPage (nIDTemplate, nIDCaption)
{
	CommonInit ();
}

CAStudioPropPage::CAStudioPropPage ()
{
	assert (0);	// Default constructor not allowed
}

CAStudioPropPage::~CAStudioPropPage()
{
}

/////////////////////////////////////////////////////////////////////////////
// CAStudioPropPage message handlers


void CAStudioPropPage::SetModifiedFlag (bool Flag)
{
	m_Modified = Flag;
}

void CAStudioPropPage::OnMessages ()
{
//	::SendMessage (GetParent ()->m_hWnd, WM_COMMAND, IDC_MESSAGES, 0);
}
