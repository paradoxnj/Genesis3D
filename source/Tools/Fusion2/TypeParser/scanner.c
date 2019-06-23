/****************************************************************************************/
/*  SCANNER.C                                                                           */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Generic token scanner implementation                                   */
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
#include "scanner.h"
#include <assert.h>
#include "ram.h"
#include <memory.h>
#include <stdio.h>
#include "util.h"

#if _MSC_VER
	// disable "unknown pragma" warning for VC
	#pragma warning (disable:4068)
#endif

enum
{
	SCANNER_TYPE_NONE,
	SCANNER_TYPE_MEMORY,
	SCANNER_TYPE_FILE
};


#define	SCANNER_EOF_CHAR	256

typedef	struct	Scanner
{
	FILE *				scnFile;
	const char *		scnData;
	int					scnDataLen;
	int					scnIndex;
	int					scnCommentLevel;
	Iden_HashTable *	scnHashTable;
	int					scnLineNumber;
	char *				scnFileName;
	const Scanner_StateInitializer *scnStateInitializers;
	int					scnNumInitializers;
	const Scanner_CharMap *scnNullState;
	int					scnType;
}	Scanner;



Scanner *		Scanner_Create 
	(
	  const Scanner_StateInitializer StateInitializers[], 
	  int nInitializers,
	  const Scanner_CharMap *NullState
	)
{
	Scanner * res;

	assert (StateInitializers != NULL);
	assert (nInitializers > 0);
	assert (NullState != NULL);

	res = geRam_Allocate(sizeof(*res));
	if	(!res)
		return res;

	memset(res, 0, sizeof(*res));

	res->scnStateInitializers = StateInitializers;
	res->scnNumInitializers = nInitializers;
	res->scnNullState = NullState;

	return res;
}

geBoolean Scanner_InitMemory
	(
	  Scanner *pScanner,
	  const char * DataBlock,
	  int length,
	  const char *Name,
	  Iden_HashTable *ht
	)
{
	assert (pScanner != NULL);
	assert (DataBlock != NULL);
	assert (Name != NULL);
	assert (ht != NULL);
	assert (pScanner->scnType == SCANNER_TYPE_NONE);

	pScanner->scnLineNumber = 1;

	pScanner->scnData = DataBlock;
	pScanner->scnDataLen = length;

	pScanner->scnIndex = 0;
	pScanner->scnHashTable = ht;

	pScanner->scnFileName = Util_Strdup(Name);
	pScanner->scnType = SCANNER_TYPE_MEMORY;
	
	return GE_TRUE;
}

geBoolean Scanner_InitFile 
	(
	  Scanner *pScanner,
	  const char *Filename, 
	  int FileType,
	  Iden_HashTable *ht
	)
{
	const char *OpenMode;

	assert (pScanner != NULL);
	assert (Filename != NULL);
	assert (ht != NULL);
	assert (pScanner->scnType == SCANNER_TYPE_NONE);
	assert ((FileType == SCANNER_FILE_TEXT) || (FileType == SCANNER_FILE_BINARY));

	OpenMode = (FileType == SCANNER_FILE_TEXT) ? "rt" : "rb";
	pScanner->scnFile = fopen (Filename, OpenMode);
	if (pScanner->scnFile != NULL)
	{
		pScanner->scnLineNumber = 1;
		pScanner->scnHashTable = ht;
		pScanner->scnFileName = Util_Strdup (Filename);
		pScanner->scnType = SCANNER_TYPE_FILE;
		return GE_TRUE;
	}
	return GE_FALSE;
}

geBoolean		Scanner_Uninit (Scanner *pScanner)
{
	assert (pScanner != NULL);

	switch (pScanner->scnType)
	{
		case SCANNER_TYPE_MEMORY :
			pScanner->scnData = NULL;
			pScanner->scnDataLen = 0;
			pScanner->scnIndex = 0;
			pScanner->scnHashTable = NULL;
			break;
		case SCANNER_TYPE_FILE :
			if (pScanner->scnFile != NULL)
			{
				fclose (pScanner->scnFile);
				pScanner->scnFile = NULL;
			}
			break;
		case SCANNER_TYPE_NONE :
			break;
		default :
			assert (0);
			break;
	}
	pScanner->scnCommentLevel = 0;
	if (pScanner->scnFileName != NULL)
	{
		geRam_Free (pScanner->scnFileName);
		pScanner->scnFileName = NULL;
	}
	pScanner->scnType = SCANNER_TYPE_NONE;
	return GE_TRUE;
}


void	Scanner_Destroy(Scanner *s)
{
	assert(s);

	if (s->scnType != SCANNER_TYPE_NONE)
	{
		Scanner_Uninit (s);
	}
	geRam_Free(s);
}

static	int	Scanner_GetChar(Scanner *s)
{
	int c;

	switch (s->scnType)
	{
		case SCANNER_TYPE_MEMORY :
			if (s->scnIndex >= s->scnDataLen)
			{
				c = EOF;
			}
			else
			{
				c = s->scnData[s->scnIndex];
				++(s->scnIndex);
			}
			break;

		case SCANNER_TYPE_FILE :
			c = fgetc (s->scnFile);
			break;

		default :
			assert (0);
			c = EOF;
			break;
	}
	return c;
}

#pragma argsused
#pragma warning (disable:4100)
static	void	Scanner_PushBackChar(Scanner *s, int c)
{
	switch (s->scnType)
	{
		case SCANNER_TYPE_MEMORY :
			assert (s->scnIndex > 0);
			--(s->scnIndex);
			break;

		case SCANNER_TYPE_FILE :
			if (ungetc (c, s->scnFile) == EOF)
			{
				// ungetc returned an error...
				assert (0);
			}
			break;

		default :
			assert (0);
			break;
	}
}
#pragma warning (default:4100)

void	Scanner_Scan(Scanner *s, Scanner_Token *t)
{
	const Scanner_CharMap *	state;

	t->tTokenDataPtr = t->tTokenData;

	state = s->scnNullState;
	for (;;)	// while (1) causes a warning
	{
		const Scanner_CharMap *	cm;
		int			c;

#ifdef	DEBUG
{
		int		i;
		for	(i = 0; i < sizeof(StateNameAssociations) / sizeof(StateNameAssociations[0]); i++)
		{
			if	(state == StateNameAssociations[i].snaMap)
			{
				StateName = StateNameAssociations[i].snaName;
				break;
			}
		}
}
#endif

		c = Scanner_GetChar(s);

		if	(c == EOF)
			c = SCANNER_EOF_CHAR;

		cm = &state[c];
		state = cm->cmNextState;
		if	(cm->cmFlags & F_CM_BUMPLINENUMBER)
			s->scnLineNumber++;
		if	(cm->cmFlags & F_CM_PUSHBACKCHAR)
			Scanner_PushBackChar(s, c);
		if	(cm->cmAction)
		{
			if	(cm->cmAction(s, t, c))
			{
				assert (state == s->scnNullState);
				return;
			}
		}
	}
}

Iden *Scanner_GenerateIdentifier (Scanner *s, const char *str, int len)
{
	return Iden_HashName (s->scnHashTable, str, len);
}

const char * Scanner_GetFileName (const Scanner *s)
{
	return s->scnFileName;
}

int	Scanner_GetLineNo(const Scanner *s)
{
	return s->scnLineNumber;
}

FILE *Scanner_GetFile (Scanner *s)
{
	assert (s != NULL);
	assert (s->scnType == SCANNER_TYPE_FILE);
	assert (s->scnFile != NULL);

	return s->scnFile;
}

int	Scanner_GetOffset (Scanner *s)
{
	assert (s != NULL);
	
	switch (s->scnType)
	{
		case SCANNER_TYPE_FILE :
			assert (s->scnFile != NULL);
			return ftell (s->scnFile);

		case SCANNER_TYPE_MEMORY :
			return s->scnIndex;

		default :
			assert (0);
			return -1;
	}
}

