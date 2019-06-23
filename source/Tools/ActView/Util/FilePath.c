/****************************************************************************************/
/*  FILEPATH.C                                                                          */
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description:  Commonly used file and path name functions.	                        */
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
#include "FilePath.h"

#include <windows.h>

#include <assert.h>
#include <stdlib.h>
#include <io.h>

geBoolean FilePath_GetDrive
	(
	  char const *pPath,
	  char *pDrive
	)
{
	assert (pPath != NULL);
	assert (pDrive != NULL);

	_splitpath (pPath, pDrive, NULL, NULL, NULL);
	return (*pDrive != '\0');
}

geBoolean FilePath_GetDir
	(
	  char const *pPath,
	  char *pDir
	)
{
	assert (pPath != NULL);
	assert (pDir != NULL);

	_splitpath (pPath, NULL, pDir, NULL, NULL);
	return (*pDir != '\0');

}

geBoolean FilePath_GetName
	(
	  char const *pPath,
	  char *pName
	)
{
	assert (pPath != NULL);
	assert (pName != NULL);

	_splitpath (pPath, NULL, NULL, pName, NULL);
	return (*pName != '\0'); 
}

geBoolean FilePath_GetExt
	(
	  char const *pPath,
	  char *pExt
	)
{
	assert (pPath != NULL);
	assert (pExt != NULL);

	_splitpath (pPath, NULL, NULL, NULL, pExt);
	return (*pExt != '\0');
}


geBoolean FilePath_GetDriveAndDir
	(
	  char const *pPath,
	  char *pDriveDir
	)
{
	char Drive[_MAX_PATH];
	char Dir[_MAX_PATH];

	assert (pPath != NULL);
	assert (pDriveDir != NULL);

	_splitpath (pPath, Drive, Dir, NULL, NULL);
	_makepath (pDriveDir, Drive, Dir, NULL, NULL);
	return (*pDriveDir != '\0');
}

geBoolean FilePath_GetNameAndExt
	(
	  char const *pPath,
	  char *pNameExt
	)
{
	char Name[_MAX_PATH];
	char Ext[_MAX_PATH];

	assert (pPath != NULL);
	assert (pNameExt != NULL);

	_splitpath (pPath, NULL, NULL, Name, Ext);
	_makepath (pNameExt, NULL, NULL, Name, Ext);
	return (*pNameExt != '\0');
}

geBoolean FilePath_SetExt
	(
	  char const *pSrcFile,
	  char const *pExt,
	  char *pDestFile
	)
{

	char Drive[_MAX_PATH];
	char Dir[_MAX_PATH];
	char Name[_MAX_PATH];

	assert (pSrcFile != NULL);
	assert (pExt != NULL);
	assert (pDestFile != NULL);

	_splitpath (pSrcFile, Drive, Dir, Name, NULL);
	_makepath (pDestFile, Drive, Dir, Name, pExt);
	return GE_TRUE;  // what's reasonable here???

}

geBoolean FilePath_SlashTerminate
	(
	  char const *pPath,
	  char *pDest
	)
{
	char *c;

	assert (pPath != NULL);
	assert (pDest != NULL);

	if (pDest != pPath)
	{
		strcpy (pDest, pPath);
	}

	c = &(pDest[strlen (pDest)]);
	if ((c ==  pDest) || (*(c-1) != '\\'))
	{
		*c = '\\';
		*(c+1) = '\0';
	}
	return GE_TRUE;
}


geBoolean FilePath_AppendName
	(
	  char const *pPath,
	  char const *pName,
	  char *pDest
	)
{
	assert (pPath != NULL);
	assert (pName != NULL);
	assert (pDest != NULL);

	if (*pPath == '\0')
	{
		strcpy (pDest, pName);
	}
	else
	{
		FilePath_SlashTerminate (pPath, pDest);
		strcat (pDest, pName);
	}

	return GE_TRUE;
}

// Search for a Filename in the semicolon-separated paths specified in SearchPath.
// If found, returns GE_TRUE and the full path name of the file in FoundPath.
// Returns GE_FALSE if unsuccessful.
geBoolean FilePath_SearchForFile (const char *Filename, const char *SearchPath, char *FoundPath)
{
	const char *c, *pPath;
	char WorkPath[MAX_PATH];
	
	c = SearchPath;

	do
	{
		pPath = c;
		c = strchr (pPath, ';');
		if (c == NULL)
		{
			strcpy (WorkPath, pPath);
		}
		else
		{
			strncpy (WorkPath, pPath, (c-pPath));
			WorkPath[c-pPath] = '\0';
			++c;
		}

		FilePath_AppendName (WorkPath, Filename, WorkPath);

		if (_access (WorkPath, 0) == 0)
		{
			strcpy (FoundPath, WorkPath);
			return GE_TRUE;
		}
	} while ((c != NULL) && (*c != '\0'));

	return GE_FALSE;
}

geBoolean FilePath_AppendSearchDir (char *SearchList, const char *NewDir)
{
	if (*NewDir != '\0')
	{
		if (*SearchList != '\0')
		{
			strcat (SearchList, ";");
		}
		strcat (SearchList, NewDir);
	}
	return GE_TRUE;
}

geBoolean FilePath_ResolveRelativePath (const char *Relative, char *Resolved)
{
	GetFullPathName (Relative, MAX_PATH, Resolved, NULL);
	return GE_TRUE;
}
/*
geBoolean FilePath_ResolveRelativePathList (const char *RelativeList, char *ResolvedList)
{
	char *PathString = Util_Strdup (RelativeList);
	char *c;

	*ResolvedList = '\0';

	if (PathString == NULL)
	{
		return GE_FALSE;
	}

	c = strtok (PathString, ";");
	while (c != NULL)
	{
		char WorkPath[MAX_PATH];

		GetFullPathName (c, sizeof (WorkPath), WorkPath, NULL);
		FilePath_AppendSearchDir (ResolvedList, WorkPath);
		c = strtok (NULL, ";");
	}

	geRam_Free (PathString);

	return GE_TRUE;
}
*/