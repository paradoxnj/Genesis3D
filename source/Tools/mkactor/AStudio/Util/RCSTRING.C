/****************************************************************************************/
/*  RCSTRING.C																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Resource string loading API.											*/
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
#include "rcstring.h"

#pragma warning (disable:4514)		// unreferenced local function

/*
  Gets a resource string and returns a pointer to it.
  This function maintains a static array of strings into which the resource strings
  are placed.  This function returns pointers to items in that static array.
  The array is maintained as a circular queue, so if you want to keep the contents 
  of a resource string around, you should make a copy of it yourself after loading 
  it from GetResourceString.
*/
const char *rcstring_Load
	(
	  HINSTANCE hinst,
	  int StringID
	)
{
	#define NUMSTRINGS 10
	#define STRINGSIZE 100
	static char Strings[NUMSTRINGS][STRINGSIZE];
	static int CurrentString = 0;
	const char *TheString;

	// load the string into the current spot in the array
	LoadString (hinst, StringID, Strings[CurrentString], STRINGSIZE);
	TheString = Strings[CurrentString];

	// Update current string pointer
	++CurrentString;
	if (CurrentString >= NUMSTRINGS)
	{
		CurrentString = 0;
	}

	return TheString;
}
