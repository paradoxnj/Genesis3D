/****************************************************************************************/
/*  ABOUT.H		                                                                        */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Actor Viewer's About dialog.											*/
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
#ifndef ABOUT_H
#define ABOUT_H

#pragma warning(disable : 4201 4214 4115)
#include <windows.h>
#pragma warning(default : 4201 4214 4115)

void About_DoDialog
	(
	  HWND hwndParent,
	  HINSTANCE hinst
	);

#endif
