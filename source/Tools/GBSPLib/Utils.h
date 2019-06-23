/****************************************************************************************/
/*  Utils.h                                                                             */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef UTILS_H
#define UTILS_H

#include <StdIo.h>

#define PATH_SEPERATOR   '/'

//
//	File name manipulation
//
void DefaultExtension (char *Path, char *Ext);
void StripFilename (char *Path);
void StripExtension (char *Path);
void ExtractFilePath (char *Path, char *Dest);
void ExtractFileBase (char *Path, char *Dest);
void ExtractFileExtension (char *Path, char *Dest);


#endif