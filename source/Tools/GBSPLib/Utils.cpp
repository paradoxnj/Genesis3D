/****************************************************************************************/
/*  Utils.cpp                                                                           */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description: Various handy functions                                                */
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

#include <Windows.h>
#include <StdIo.h>

#include "Utils.h"
#include "Basetype.h"

//====================================================================================
//	DefaultExtension
//====================================================================================
void DefaultExtension (char *Path, char *Ext)
{
	char    *Src;

	Src = Path + strlen(Path) - 1;

	while (*Src != PATH_SEPERATOR && Src != Path)
	{
		if (*Src == '.')
			return;                 
	
		Src--;
	}

	strcat (Path, Ext);
}

//====================================================================================
//	StripFilename
//====================================================================================
void StripFilename (char *Path)
{
	int32		Length;

	Length = strlen(Path)-1;
	while (Length > 0 && Path[Length] != PATH_SEPERATOR)
		Length--;
	Path[Length] = 0;
}

//====================================================================================
//	StripExtension
//====================================================================================
void StripExtension (char *Path)
{
	int32		Length;

	Length = strlen(Path)-1;
	while (Length > 0 && Path[Length] != '.')
	{
		Length--;
		if (Path[Length] == '/')
			return;		
	}
	if (Length)
		Path[Length] = 0;
}


//====================================================================================
//	ExtractFilePath
//====================================================================================
void ExtractFilePath (char *Path, char *Dest)
{
	char    *Src;

	Src = Path + strlen(Path) - 1;

	while (Src != Path && *(Src-1) != PATH_SEPERATOR)
		Src--;

	memcpy (Dest, Path, Src-Path);
	Dest[Src-Path] = 0;
}

//====================================================================================
//	ExtractFileBase
//====================================================================================
void ExtractFileBase (char *Path, char *Dest)
{
	char    *Src;

	Src = Path + strlen(Path) - 1;

	while (Src != Path && *(Src-1) != PATH_SEPERATOR)
		Src--;

	while (*Src && *Src != '.')
	{
		*Dest++ = *Src++;
	}
	*Dest = 0;
}

//====================================================================================
//	ExtractFileExtension
//====================================================================================
void ExtractFileExtension (char *Path, char *Dest)
{
	char    *Src;

	Src = Path + strlen(Path) - 1;

	while (Src != Path && *(Src-1) != '.')
		Src--;
	if (Src == Path)
	{
		*Dest = 0;	
		return;
	}

	strcpy (Dest, Src);
}

