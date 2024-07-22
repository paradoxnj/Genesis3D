/****************************************************************************************/
/*  Parse3dt.c                                                                          */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Definitions for parsing an editor .3dt file                           */
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
#include "Parse3dt.h"
#include <assert.h>
#include "util.h"
#include <stdlib.h>
#include "RAM.H"
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include "units.h"

#if _MSC_VER
	// disable "unknown pragma" warning for VC
	#pragma warning (disable:4068)
#endif

#define			SkipChar	0
static	int		IllegalChar(Scanner *s, Scanner_Token *t, int c);
static	int		CopyChar(Scanner *s, Scanner_Token *t, int c);

static	int		genIDEN(Scanner *s, Scanner_Token *t, int c);
static	int		genLITERALSTRING(Scanner *s, Scanner_Token *t, int c);
static	int		genICON(Scanner *s, Scanner_Token *t, int c);
static	int		genFCON(Scanner *s, Scanner_Token *t, int c);
static	int		genEOF(Scanner *s, Scanner_Token *t, int c);
static  int		genPATH(Scanner *s, Scanner_Token *t, int c);


static Scanner_CharMap SNull[257];
static Scanner_CharMap SLitString[257];
static Scanner_CharMap SLitChar[257];
static Scanner_CharMap SIden[257];
static Scanner_CharMap SNumber[257];
static Scanner_CharMap SFloat[257];
static Scanner_CharMap SPath[257];


static	Scanner_CharMapInitializer	SNullInitializer[] =
{
	{   0,   9, 	SNull, IllegalChar , 0},
	{   9,  10, 	SNull,    SkipChar , 0},
	{  10,  11, 	SNull,    SkipChar , F_CM_BUMPLINENUMBER},
	{  11,  13, 	SNull, IllegalChar , 0},
	{  13,  14, 	SNull,    SkipChar , 0},
	{  14,  32, 	SNull, IllegalChar , 0},
	{  32,  33, 	SNull,    SkipChar , 0},
	{ '_', '_' + 1, SIden,    CopyChar , 0},
	{ 'A', 'Z' + 1, SIden,    CopyChar , 0},
	{ 'a', 'z' + 1, SIden,    CopyChar , 0},
	{ '0', '9' + 1, SNumber,  CopyChar , 0},
	{ '-', '-' + 1, SNumber,  CopyChar , 0},
	{ '"', '"' + 1, SLitString,  SkipChar , 0},
	{ ':', ':' + 1, SPath,	  CopyChar , 0},	// need to allow path characters
	{ '.', '.' + 1, SPath,	  CopyChar , 0},
	{ '\\', '\\'+1, SPath,	  CopyChar , 0},
	{ '$', '$' + 1, SPath,	  CopyChar , 0},
	{ '%', '%' + 1, SIden,	  CopyChar , 0},
	{ '\'', '\''+1, SPath,	  CopyChar , 0},
	{ '@', '@' + 1, SPath,	  CopyChar , 0},
	{ '~', '~' + 1, SPath,	  CopyChar , 0},
	{ '`', '`' + 1, SPath,	  CopyChar , 0},
	{ '!', '!' + 1, SPath,    CopyChar , 0},
	{ '(', '(' + 1, SPath,	  CopyChar , 0},
	{ ')', ')' + 1, SPath,	  CopyChar , 0},
	{ '{', '{' + 1, SPath,	  CopyChar , 0},
	{ '}', '}' + 1, SPath,	  CopyChar , 0},
	{ '^', '^' + 1, SPath,	  CopyChar , 0},
	{ '#', '#' + 1, SPath,	  CopyChar , 0},
	{ '&', '&' + 1, SPath,	  CopyChar , 0},
	{ '*', '*' + 1, SIden,	  CopyChar , 0},
	{ 127, 256, 	SPath,	  CopyChar , 0},
	{ 256, 257, 	SNull,      genEOF , 0}
};

static	Scanner_CharMapInitializer	SLitStringInitializer[] =
{
	
	{   0, 256,		SLitString, CopyChar , 0},
	{  13,	14,		SLitString, SkipChar,  0},
	{ '\\', '\\'+1, SLitChar,   SkipChar,  0},
	{ '"', '"' + 1,	SNull,		genLITERALSTRING, 0 },
	{ 256, 257, 	SNull,		IllegalChar, 0}
};

// Table branched to by SLitString when a backslash found...
static Scanner_CharMapInitializer SLitCharInitializer[] =
{
	{0,   256, SLitString, CopyChar,    0},
	{256, 257, SNull,	   IllegalChar, 0}
};

static	Scanner_CharMapInitializer	SIdenInitializer[] =
{
	{   0,  32,     SNull, IllegalChar , 0 },
	{  33, 256,     SPath,    SkipChar , F_CM_PUSHBACKCHAR},
	{   9,  10, 	SNull,	   genIDEN , 0 },
	{  10,  11, 	SNull,	   genIDEN , F_CM_BUMPLINENUMBER},
	{  13,  14, 	SNull,     genIDEN , 0 },
	{  32,  33, 	SNull,     genIDEN , 0 },
	{ '_', '_' + 1, SIden,    CopyChar , 0 },
	{ 'A', 'Z' + 1, SIden,    CopyChar , 0 },
	{ 'a', 'z' + 1, SIden,    CopyChar , 0 },
	{ '0', '9' + 1, SIden,    CopyChar , 0 },
	{ '%', '%' + 1, SIden,	  CopyChar , 0 },
	{ 256, 257, 	SNull,     genIDEN , 0 }
};

static Scanner_CharMapInitializer SPathInitializer[] =
{
	{   0, 255,     SNull,    IllegalChar , 0},
	{   9,  10,		SNull,		  genPATH , 0},
	{  10,  11,		SNull,		  genPATH , F_CM_BUMPLINENUMBER},
	{  13,  14,		SNull,		  genPATH , 0},
	{  32,  33,		SNull,		  genPATH , 0},
	{ '0', '9' + 1, SPath,	     CopyChar , 0},
	{ 'A', 'Z' + 1, SPath,		 CopyChar , 0},
	{ 'a', 'z' + 1, SPath,		 CopyChar , 0},
	{ '_', '_' + 1, SPath,		 CopyChar , 0},
	{ ':', ':' + 1, SPath,	     CopyChar , 0},
	{ '.', '.' + 1, SPath,		 CopyChar , 0},
	{ '\\', '\\'+1, SPath,		 CopyChar , 0},
	{ '$', '$' + 1, SPath,	     CopyChar , 0},
	{ '%', '%' + 1, SPath,	     CopyChar , 0},
	{ '\'', '\''+1, SPath,	     CopyChar , 0},
	{ '-', '-' + 1, SPath,	     CopyChar , 0},
	{ '@', '@' + 1, SPath,	     CopyChar , 0},
	{ '~', '~' + 1, SPath,	     CopyChar , 0},
	{ '`', '`' + 1, SPath,	     CopyChar , 0},
	{ '!', '!' + 1, SPath,       CopyChar , 0},
	{ '(', '(' + 1, SPath,	     CopyChar , 0},
	{ ')', ')' + 1, SPath,	     CopyChar , 0},
	{ '{', '{' + 1, SPath,	     CopyChar , 0},
	{ '}', '}' + 1, SPath,	     CopyChar , 0},
	{ '^', '^' + 1, SPath,	     CopyChar , 0},
	{ '#', '&' + 1, SPath,	     CopyChar , 0},
	{ '&', '&' + 1, SPath,	     CopyChar , 0},
	{ 127, 256, 	SPath,	     CopyChar , 0},
};

static	Scanner_CharMapInitializer	SNumberInitializer[] =
{
	{   0,  32,     SNull,  IllegalChar, 0 },
	{  33, 256,     SPath,  SkipChar,	 F_CM_PUSHBACKCHAR},
	{   9,  10, 	SNull,	genICON,     0 },
	{  10,  11, 	SNull,	genICON,     F_CM_BUMPLINENUMBER},
	{  13,  14, 	SNull, 	genICON,	 0 },
	{  32,  33,		SNull,  genICON,	 0 },
	{ '0', '9' + 1, SNumber,CopyChar,    0 },
	{ '.', '.' + 1, SFloat,	CopyChar,    0 },
	{ '_', '_' + 1, SIden,  CopyChar ,   0 },
	{ 'A', 'Z' + 1, SIden,  CopyChar ,   0 },
	{ 'a', 'z' + 1, SIden,  CopyChar ,   0 },
	{ 256, 257, 	SNull,	genICON ,	 0 }
};

static	Scanner_CharMapInitializer	SFloatInitializer[] =
{
	{   0,  32,     SNull,  IllegalChar, 0},
	{  33, 256,     SPath,  SkipChar, F_CM_PUSHBACKCHAR},
	{   9,  10, 	SNull,  genFCON,  0},
	{  10,  11, 	SNull,  genFCON,  F_CM_BUMPLINENUMBER},
	{  13,  14,		SNull,	genFCON,  0},
	{  32,  33,		SNull,  genFCON,  0},
	{ '0', '9' + 1, SFloat, CopyChar, 0},
	{ 256, 257, 	SNull,	genFCON,  0}
};

static Scanner_StateInitializer StateInitializers[] = 
{
	STATEINITIALIZER(SNull, 	 	SNullInitializer),
	STATEINITIALIZER(SLitString, 	SLitStringInitializer),
	STATEINITIALIZER(SLitChar,		SLitCharInitializer),
	STATEINITIALIZER(SIden, 		SIdenInitializer),
	STATEINITIALIZER(SNumber, 		SNumberInitializer),
	STATEINITIALIZER(SFloat,		SFloatInitializer),
	STATEINITIALIZER(SPath,			SPathInitializer)
};

static const int NumInitializers = (sizeof (StateInitializers) / sizeof (Scanner_StateInitializer));


/*
  Disable Unused parameter warnings for these functions...
*/
#pragma warning (disable:4100)

#pragma argsused
static	int	IllegalChar(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = P3TK_ERROR;
	return 1;
}

#pragma argsused
static	int	CopyChar(Scanner *s, Scanner_Token *t, int c)
{
	assert(c != EOF);
	*t->tTokenDataPtr++ = (char)c;
	return 0;
}


#pragma argsused
static	int	genIDEN(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = P3TK_IDEN;
	*t->tTokenDataPtr = 0;
	// don't really  need identifiers here....
	t->tIden = NULL;
//	t->tIden = Scanner_GenerateIdentifier (s, t->tTokenData, (t->tTokenDataPtr - t->tTokenData));
	return 1;
}

#pragma argsused
static	int	genLITERALSTRING(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = P3TK_LITERALSTRING;
	*t->tTokenDataPtr = 0;
	return 1;
}

#pragma argsused
static int genPATH(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = P3TK_PATH;
	*t->tTokenDataPtr = 0;
	return 1;
}

#pragma argsused
static	int	genICON(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = P3TK_ICON;
	*t->tTokenDataPtr = 0;
	if	(t->tTokenData[1] == 'x' || t->tTokenData[1] == 'X')
		t->tICONValue = Util_htoi(t->tTokenData);
	else
		t->tICONValue = atoi(t->tTokenData);
	return 1;
}

#pragma argsused
static	int	genFCON(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = P3TK_FCON;
	*t->tTokenDataPtr = 0;
	t->tFCONValue = (float)atof(t->tTokenData);
	return 1;
}


#define	ONESHOT(func, token) \
	static	int	func(Scanner *s, Scanner_Token *t, int c)	\
	{														\
		t->tKind = token;									\
		return 1;											\
	}

ONESHOT(genEOF, P3TK_EOF)

// ok, reenable...
#pragma warning (default:4100)


Parse3dt *Parse3dt_Create (const char *Filename)
{
	int	i;
	int	j;
	Parse3dt *Parser;

	assert (Filename != NULL);
	// initialize the tables
	for	(i = 0; i < NumInitializers; i++)
	{
		Scanner_CharMap *	cm;

		cm = StateInitializers[i].siMap;
		for	(j = 0; j < 256; j++)
		{
			cm[j].cmNextState = SNull;
			cm[j].cmAction	  = IllegalChar;
		}

		for	(j = 0; j < StateInitializers[i].siInitializerCount; j++)
		{
			Scanner_CharMapInitializer *	cmi;
			int						k;

			cmi = &(StateInitializers[i].siMapInitializer[j]);
			for	(k = cmi->cmiStartChar; k < cmi->cmiEndChar; k++)
			{
				cm[k].cmNextState = cmi->cmiNextState;
				cm[k].cmAction	  = cmi->cmiAction;
				cm[k].cmFlags	  = cmi->cmiFlags;
			}
		}
	}

	Parser = geRam_Allocate (sizeof (Parse3dt));
	if (Parser != NULL)
	{
		geBoolean NoErrors;

		Parser->HashTable = NULL;
		Parser->Scanner = Scanner_Create (StateInitializers, NumInitializers, SNull);

		// Create the hash table
		Parser->HashTable = Iden_CreateHashTable();

		NoErrors = ((Parser->Scanner != NULL) && (Parser->HashTable != NULL));

		if (NoErrors)
		{
			NoErrors = Scanner_InitFile (Parser->Scanner, Filename, SCANNER_FILE_TEXT, Parser->HashTable);
		}
		if (!NoErrors)
		{
			Parse3dt_Destroy (&Parser);
		}
	}
	return Parser;
}


void Parse3dt_Destroy (Parse3dt **pParser)
{
	Parse3dt *Parser;

	assert (pParser != NULL);
	assert (*pParser != NULL);

	Parser = *pParser;
	if (Parser->Scanner != NULL)
	{
		Scanner_Destroy (Parser->Scanner);
	}
	if (Parser->HashTable != NULL)
	{
		Iden_DestroyHashTable (Parser->HashTable);
	}
	geRam_Free (*pParser);
	*pParser = NULL;
}


char const *Parse3dt_Error (Parse3dt *Parser, char *fmt, ...)
{
	va_list			ap;
	static char		buff[1024];
	char			tmpBuff[1024];

	va_start(ap, fmt);

	// format the caller's string into the temporary buffer
	vsprintf (tmpBuff, fmt, ap);

	va_end(ap);

	// and then wrap the whole thing up prettily into buff
	sprintf (buff, "Error '%s' in file <%s> at offset 0x%08X",
		tmpBuff, Scanner_GetFileName (Parser->Scanner),
		Scanner_GetOffset (Parser->Scanner));

	return buff;
}

geBoolean Parse3dt_ScanExpecting
	(
	  Parse3dt *Parser,
	  Scanner_Token *t, 
	  Parse3dt_TokenKind kind
	)
{
	Scanner_Scan (Parser->Scanner, t);
	return (t->tKind == kind);
}

geBoolean Parse3dt_ScanExpectingText
	(
	  Parse3dt *Parser,
	  const char *idText
	)
{
	Scanner_Token t;

	assert (idText != NULL);

	Scanner_Scan (Parser->Scanner, &t);
	if (t.tKind != P3TK_ERROR)
	{
		*(t.tTokenDataPtr) = '\0';
		return (strcmp (idText, t.tTokenData) == 0) ? GE_TRUE : GE_FALSE;
	}
	return GE_FALSE;
}

geBoolean Parse3dt_GetIdentifier 
	(
	  Parse3dt *Parser,
	  const char *Tag, 
	  char *idText
	)
{
	Scanner_Token t;

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}

	if (Parse3dt_ScanExpecting (Parser, &t, P3TK_IDEN))
	{
		// identifiers are nul-terminated...
		strcpy (idText, t.tTokenData);
		return GE_TRUE;
	}
	return GE_FALSE;
}

geBoolean Parse3dt_GetPath
	(
	  Parse3dt *Parser,
	  char const *Tag,
	  char *Path
	)
{
	Scanner_Token t;

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}
	Scanner_Scan (Parser->Scanner, &t);
	if ((t.tKind == P3TK_IDEN) || (t.tKind == P3TK_PATH))
	{
		strcpy (Path, t.tTokenData);
		return GE_TRUE;
	}
	return GE_FALSE;
}

geBoolean Parse3dt_GetInt (Parse3dt *Parser, char const *Tag, int *Value)
{
	Scanner_Token t;

	assert (Parser != NULL);
	assert (Value != NULL);

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}
	if (Parse3dt_ScanExpecting (Parser, &t, P3TK_ICON))
	{
		*Value = t.tICONValue;
		return GE_TRUE;
	}
	return GE_FALSE;
}

geBoolean Parse3dt_GetFloat (Parse3dt *Parser, char const *Tag, float *Value)
{
	Scanner_Token t;

	assert (Parser != NULL);
	assert (Value != NULL);

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}
	Scanner_Scan (Parser->Scanner, &t);
	switch (t.tKind)
	{
		case P3TK_FCON :
			*Value = t.tFCONValue;
			return GE_TRUE;
		case P3TK_ICON :
			*Value = (float)t.tICONValue;
			return GE_TRUE;
		default :
			break;
	}
	return GE_FALSE;
}

geBoolean Parse3dt_GetLiteral (Parse3dt *Parser, char const *Tag, char *strtext)
{
	Scanner_Token t;

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}
	Scanner_Scan (Parser->Scanner, &t);
	if ((t.tKind == P3TK_IDEN) || (t.tKind == P3TK_LITERALSTRING))
	{
		strcpy (strtext, t.tTokenData);
		return GE_TRUE;
	}
	return GE_FALSE;
}

geBoolean Parse3dt_GetVec3d (Parse3dt *Parser, char const *Tag, geVec3d *pVec)
{
	assert (Parser != NULL);
	assert (pVec != NULL);

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}
	
	return (Parse3dt_GetFloat (Parser, NULL, &pVec->X) &&
			Parse3dt_GetFloat (Parser, NULL, &pVec->Y) &&
			Parse3dt_GetFloat (Parser, NULL, &pVec->Z));
}

geBoolean Parse3dt_GetXForm3d (Parse3dt *Parser, char const *Tag, geXForm3d *pXfm)
{
	assert (Parser != NULL);
	assert (pXfm != NULL);

	if (Tag != NULL)
	{
		if (!Parse3dt_ScanExpectingText (Parser, Tag))
		{
			return GE_FALSE;
		}
	}

	return
	(
		Parse3dt_GetFloat (Parser, NULL, &pXfm->AX) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->AY) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->AZ) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->BX) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->BY) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->BZ) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->CX) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->CY) &&
		Parse3dt_GetFloat (Parser, NULL, &pXfm->CZ) &&
		Parse3dt_GetVec3d (Parser, NULL, &pXfm->Translation)
	);
}


geBoolean Parse3dt_GetVersion (Parse3dt *Parser, int *VersionMajor, int *VersionMinor)
{
	Scanner_Token t;

	if (!Parse3dt_ScanExpectingText (Parser, "3dtVersion"))
	{
		return GE_FALSE;
	}
	Scanner_Scan (Parser->Scanner, &t);

	switch (t.tKind)
	{
		case P3TK_FCON :
		{
			// If it's a float, then parse the token data to get
			// major and minor version numbers.
			char *c;

			*(t.tTokenDataPtr) = '\0';
			c = strchr (t.tTokenData, '.');
			if (c != NULL)
			{
				*c = '\0';
				++c;
				*VersionMinor = atoi (c);
			}
			else
			{
				*VersionMinor = 0;
			}

			*VersionMajor = atoi (t.tTokenData);
			return GE_TRUE;
		}
		case P3TK_ICON :
			// if it's an integer, then the major version is the number
			// and the minor version number is 0.
			*VersionMajor = t.tICONValue;
			*VersionMinor = 0;
			return GE_TRUE;
		default :
			break;
	}
	return GE_FALSE;
}

