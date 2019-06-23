/****************************************************************************************/
/*  ActivationWatch.cpp                                                                 */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird                                                */
/*  Description:  Checks for previous instances                                         */
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
#include "fusion.h"
#include "ActivationWatch.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CActivationWatch

IMPLEMENT_DYNCREATE(CActivationWatch, CWinThread)

CActivationWatch::CActivationWatch()
{
}

CActivationWatch::~CActivationWatch()
{
}

BOOL CActivationWatch::InitInstance()
{
	CFusionApp* pApp = ( CFusionApp* ) AfxGetApp();
	CSyncObject* pEvents[ 2 ];
	pEvents[ 1 ] = pApp->pNewInstanceEvent;
	pEvents[ 0 ] = pApp->pShutdownEvent;
	DWORD dwSignalled;
	CMultiLock Locker( pEvents, 2 );

	for (;;)
	{
		dwSignalled = Locker.Lock( INFINITE, FALSE );
		TRACE( _T( "dwSignalled is %d\n" ), dwSignalled );

		//	Have we shutdown?
		if( ( dwSignalled - WAIT_OBJECT_0 ) == 0 )
			break;

		//	Is this an instance?
		if( ( dwSignalled - WAIT_OBJECT_0 ) == 1 )
		{
#if 1
			CWnd *pWnd = pApp->m_pMainWnd->GetLastActivePopup ();
			pWnd->SetForegroundWindow();
			pWnd->ShowWindow( SW_RESTORE );
			OnIdle( 0 );
#else
			CWnd MainWnd;
			if( MainWnd.Attach( pApp->m_pMainWnd->m_hWnd ) )
			{
				CWnd* pWnd = MainWnd.GetLastActivePopup();
				pWnd->SetForegroundWindow();
				pWnd->ShowWindow( SW_RESTORE );
				MainWnd.Detach();
				OnIdle( 0 );
			}
			else
				::MessageBeep( 0 );
#endif
		}
	}

	return FALSE;
}

int CActivationWatch::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CActivationWatch, CWinThread)
	//{{AFX_MSG_MAP(CActivationWatch)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CActivationWatch message handlers
