/****************************************************************************************/
/*  EntityVisDlg.cpp                                                                    */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Entity visibility dialog                                              */
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
#include "resource.h"
#include "EntityVisDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEntityVisDlg dialog


CEntityVisDlg::CEntityVisDlg(CWnd* pParent /*=NULL*/)
	: pViewList (NULL),
	  CDialog(CEntityVisDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CEntityVisDlg)
	//}}AFX_DATA_INIT
}


void CEntityVisDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEntityVisDlg)
	DDX_Control(pDX, IDC_LIST1, m_EntsList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CEntityVisDlg, CDialog)
	//{{AFX_MSG_MAP(CEntityVisDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEntityVisDlg message handlers

int CEntityVisDlg::DoModal(EntityViewList *pList) 
{
	pViewList = pList;
	
	return CDialog::DoModal();
}

BOOL CEntityVisDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if (pViewList != NULL)
	{
		for (int i = 0; i < pViewList->nEntries; i++)
		{
			int Index;
			EntityViewEntry *pEntry;

			pEntry = &(pViewList->pEntries[i]);
			
			Index = m_EntsList.AddString (pEntry->pName);
			if (Index != LB_ERR)
			{
				m_EntsList.SetItemData (Index, i);
				m_EntsList.SetCheck (Index, pEntry->IsVisible ? 1 : 0);
			}
		}
		m_EntsList.SetCurSel (0);
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEntityVisDlg::OnOK() 
{
	// set visibility state from list box
	int Item;

	for (Item = 0; Item < m_EntsList.GetCount (); ++Item)
	{
		int EntryIndex;
		EntityViewEntry *pEntry;

		EntryIndex = m_EntsList.GetItemData (Item);
		pEntry = &(pViewList->pEntries[EntryIndex]);
		pEntry->IsVisible = (m_EntsList.GetCheck (Item) == 1);
	}

	CDialog::OnOK();
}
