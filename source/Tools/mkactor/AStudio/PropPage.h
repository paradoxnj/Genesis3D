/****************************************************************************************/
/*  PROPPAGE.H																			*/
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#if !defined(PROPPAGE_H)
#define PROPPAGE_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CAStudioPropPage
#include "AProject.h"
#include "AOptions.h"

class CAStudioPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CAStudioPropPage)

// Construction
public:
	~CAStudioPropPage();
	CAStudioPropPage (UINT nIDTemplate, UINT nIDCaption = 0);
	CAStudioPropPage (LPCSTR lpszTemplateName, UINT nIDCaption = 0);

	virtual bool WasModified () { return m_Modified; }
	void SetModifiedFlag (bool Flag);
	virtual void SetProjectPointer (AProject *Project) {m_Project = Project;}
	void SetOptionsPointer (const AOptions *Options) {m_Options = Options;}
	virtual void GetDialogData () {};
	virtual void SetCompileStatus (bool Status) { m_Compiling = Status; }
// Implementation
protected:
	// available only to inherited classes and friends
	bool		m_Modified;
	bool		m_Compiling;
	AProject	*m_Project;
	const AOptions *m_Options;
	// Generated message map functions
	//{{AFX_MSG(CAStudioPropPage)
	virtual void OnMessages ();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CAStudioPropPage();		// declared private so it can't be used!
	void CommonInit (void);
};

#endif // !defined(PROPPAGE_H)
