/****************************************************************************************/
/*  RCSTRING.H                                                                          */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Resource string wrapper function.										*/
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
#ifndef RCSTRING_H
#define RCSTRING_H

#include <windows.h>

#ifdef __cplusplus
	extern "C" {
#endif

/*
  Gets a resource string and returns a pointer to it.
  This function maintains a static array of strings into which the resource strings
  are placed.  This function returns pointers to items in that static array.
  The array is maintained as a circular queue, so if you want to keep the contents 
  of a resource string around, you should make a copy of it yourself after loading 
  it.
*/
const char *rcstring_Load
	(
	  HINSTANCE hinst,
	  int StringID
	);


#ifdef __cplusplus
	}
#endif

#endif
