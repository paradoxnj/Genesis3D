/****************************************************************************************/
/*  FILEPATH.H																			*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Useful file and pathname functions.									*/
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
#ifndef FILEPATH_H

#define FILEPATH_H

#include "basetype.h"

#ifdef __cplusplus
	extern "C" {
#endif

// Extract drive (d:\) from pPath and place in pDrive
geBoolean FilePath_GetDrive (char const *pPath, char *pDrive);

// Extract directory from pPath and place in pDir
geBoolean FilePath_GetDir (char const *pPath, char *pDir);

// Extract Name from pPath and place in pName
geBoolean FilePath_GetName (char const *pPath, char *pName);

// Extract Extension from pPath and place in pExt
geBoolean FilePath_GetExt (char const *pPath, char *pExt);

// Extract drive and directory from pPath and place in pDriveDir
// pDriveDir may be the same as pPath
geBoolean FilePath_GetDriveAndDir (char const *pPath, char *pDriveDir);

// Extract Name and extension from pPath and place in pName
// pName may be the same as pPath
geBoolean FilePath_GetNameAndExt (char const *pPath, char *pName);

// set extension of pSourceFile to pExt and place result in pDestFile
// pDestFile may be the same as pSourceFile
geBoolean FilePath_SetExt (char const *pSourceFile, char const *pExt, char *pDestFile);

// Terminate pPath with a slash (by appending if necessary), and return result in pDest.
// pPath and pDest may be the same.
geBoolean FilePath_SlashTerminate (const char *pPath, char *pDest);

// Append pName to pPath and return result in pDest.
// pDest may be the same as pPath or pName.
geBoolean FilePath_AppendName (char const *pPath, char const *pName, char *pDest);

// Search for a Filename in the semicolon-separated paths specified in SearchPath.
// If found, returns GE_TRUE and the full path name of the file in FoundPath.
// Returns GE_FALSE if unsuccessful.
geBoolean FilePath_SearchForFile (const char *Filename, const char *SearchPath, char *FoundPath);


geBoolean FilePath_AppendSearchDir (char *SearchList, const char *NewDir);
geBoolean FilePath_ResolveRelativePath (const char *Relative, char *Resolved);
geBoolean FilePath_ResolveRelativePathList (const char *RelativeList, char *ResolvedList);

#ifdef __cplusplus
	}
#endif


#endif
