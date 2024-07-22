/****************************************************************************************/
/*  CSCANNER.C                                                                          */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: C Language specific scanner actions                                    */
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
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include    <ctype.h>

#include	"CScanner.h"
#include	"util.h"
#include	"RAM.H"

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
static	int		genLBRACE(Scanner *s, Scanner_Token *t, int c);
static	int		genRBRACE(Scanner *s, Scanner_Token *t, int c);
static	int		genLPAREN(Scanner *s, Scanner_Token *t, int c);
static	int		genRPAREN(Scanner *s, Scanner_Token *t, int c);
static	int		genSEMI(Scanner *s, Scanner_Token *t, int c);
static	int		genDOT(Scanner *s, Scanner_Token *t, int c);
static	int		genCOMMA(Scanner *s, Scanner_Token *t, int c);
static	int		genSLASH(Scanner *s, Scanner_Token *t, int c);
static	int		genHYPHEN(Scanner *s, Scanner_Token *t, int c);
static	int		genPLUS(Scanner *s, Scanner_Token *t, int c);
static	int		genBANG(Scanner *s, Scanner_Token *t, int c);
static	int		genTILDE(Scanner *s, Scanner_Token *t, int c);
static	int		genSHARP(Scanner *s, Scanner_Token *t, int c);
static	int		genMOD(Scanner *s, Scanner_Token *t, int c);
static	int		genHAT(Scanner *s, Scanner_Token *t, int c);
static	int		genAMPER(Scanner *s, Scanner_Token *t, int c);
static	int		genSTAR(Scanner *s, Scanner_Token *t, int c);
static	int		genEQUAL(Scanner *s, Scanner_Token *t, int c);


static	Scanner_CharMap	SNull[257];
static	Scanner_CharMap	SLitString[257];
static	Scanner_CharMap	SIden[257];
static	Scanner_CharMap	SNumber[257];
static	Scanner_CharMap	SMaybeHexNumber[257];
static	Scanner_CharMap SHexNumber[257];
static	Scanner_CharMap	SFloat[257];
static	Scanner_CharMap	SMaybeComment[257];
static	Scanner_CharMap	SMaybeEndComment[257];
static	Scanner_CharMap	SOneLineComment[257];
static	Scanner_CharMap	SComment[257];

#define	TERMINATING_CHARS(action) \
	{ ',', ',' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '.', '.' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '{', '{' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '}', '}' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '(', '(' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ ')', ')' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ ';', ';' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '_', '_' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '+', '+' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '-', '-' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '/', '/' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '^', '^' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '|', '|' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '=', '=' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '&', '&' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '#', '#' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '!', '!' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ '~', '~' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ 'A', 'Z' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\
	{ 'a', 'z' + 1, SNull, action   , F_CM_PUSHBACKCHAR },	\

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
	{ '0', '0' + 1, SMaybeHexNumber, CopyChar, 0},
	{ '{', '{' + 1, SNull,    genLBRACE , 0},
	{ '}', '}' + 1, SNull,    genRBRACE , 0},
	{ '(', '(' + 1, SNull,    genLPAREN , 0},
	{ ')', ')' + 1, SNull,    genRPAREN , 0},
	{ ';', ';' + 1, SNull,    genSEMI , 0},
	{ '.', '.' + 1, SNull,    genDOT , 0},
	{ ',', ',' + 1, SNull,    genCOMMA , 0},
	{ '-', '-' + 1, SNull,    genHYPHEN , 0},
	{ '+', '+' + 1, SNull,    genPLUS , 0},
	{ '!', '!' + 1, SNull,    genBANG , 0},
	{ '~', '~' + 1, SNull,    genTILDE , 0},
	{ '#', '#' + 1, SNull,    genSHARP , 0},
	{ '%', '%' + 1, SNull,    genMOD , 0},
	{ '^', '^' + 1, SNull,    genHAT , 0},
	{ '&', '&' + 1, SNull,    genAMPER , 0},
	{ '*', '*' + 1, SNull,    genSTAR , 0},
	{ '=', '=' + 1, SNull,    genEQUAL , 0},
	{ '/', '/' + 1, SMaybeComment, SkipChar, 0},
	{ '"', '"' + 1, SLitString,  SkipChar , 0},
	{ 127, 256, 	SNull, IllegalChar , 0},
	{ 256, 257, 	SNull,    genEOF , 0}
};

static	Scanner_CharMapInitializer	SLitStringInitializer[] =
{
	
	{   0, 256,		SLitString, CopyChar , 0},
	{  13,	14,		SLitString, SkipChar,  0},
	{ '"', '"' + 1,	SNull,		genLITERALSTRING, 0 },
	{ 256, 257, 	SNull,		IllegalChar, 0}
};

static	Scanner_CharMapInitializer	SMaybeCommentInitializer[] =
{
	
	{   0,   256,	SNull, 				genSLASH, 	F_CM_PUSHBACKCHAR},
	{ '*', '*' + 1, SComment,			SkipChar, 	0 },
	{ '/', '/' + 1,	SOneLineComment,	SkipChar, 	0 },
	{ 256, 257, 	SNull, 				IllegalChar,0}
};

static	Scanner_CharMapInitializer	SOneLineCommentInitializer[] =
{
	{   0,   256,	SOneLineComment, 	SkipChar, 	0 },
	{  10,    11,	SNull,				SkipChar, 	F_CM_BUMPLINENUMBER },
	{ 256, 257, 	SNull, 				IllegalChar,0}
};

static	Scanner_CharMapInitializer	SCommentInitializer[] =
{
	{   0,   256,	SComment, 			SkipChar, 	0},
	{  10,    11,	SComment,			SkipChar, 	F_CM_BUMPLINENUMBER },
	{ '*', '*' + 1, SMaybeEndComment,	SkipChar, 	0 },
	{ 256, 257, 	SNull, 				IllegalChar,0}
};

static	Scanner_CharMapInitializer	SMaybeEndCommentInitializer[] =
{
	{   0,   256,	SComment, 			SkipChar, 	0 },
	{  10,    11,	SComment,			SkipChar, 	F_CM_BUMPLINENUMBER },
	{ '/', '/' + 1, SNull,				SkipChar, 	0 },
	{ 256, 257, 	SNull, 				IllegalChar,0}
};

static	Scanner_CharMapInitializer	SIdenInitializer[] =
{
	TERMINATING_CHARS(genIDEN)

	{   0,   9,		SNull, IllegalChar , 0},
	{   9,  10, 	SNull, genIDEN , 0},
	{  10,  11, 	SNull, genIDEN , F_CM_BUMPLINENUMBER},
	{  11,  13, 	SNull, IllegalChar , 0},
	{  13,  14, 	SNull,     genIDEN , 0},
	{  14,  32, 	SNull, IllegalChar , 0},
	{  32,  33, 	SNull,     genIDEN , 0},
	{ ';', ';' + 1, SNull,     genIDEN , F_CM_PUSHBACKCHAR},
	{ '(', '(' + 1, SNull,     genIDEN , F_CM_PUSHBACKCHAR},
	{ ')', ')' + 1, SNull,     genIDEN , F_CM_PUSHBACKCHAR},
	{ '{', '{' + 1, SNull,     genIDEN , F_CM_PUSHBACKCHAR},
	{ '}', '}' + 1, SNull,     genIDEN , F_CM_PUSHBACKCHAR},
	{ '_', '_' + 1, SIden,    CopyChar , 0},
	{ 'A', 'Z' + 1, SIden,    CopyChar , 0},
	{ 'a', 'z' + 1, SIden,    CopyChar , 0},
	{ '0', '9' + 1, SIden,    CopyChar , 0},
	{ 127, 256, 	SNull, IllegalChar , 0},
	{ 256, 257, 	SNull, genIDEN	   , 0}
};

static	Scanner_CharMapInitializer	SNumberInitializer[] =
{
	TERMINATING_CHARS(genICON)

	{   0,   9,		SNull, 	IllegalChar , 0},
	{   9,  10, 	SNull,  genICON , 0},
	{  10,  11, 	SNull,  genICON , F_CM_BUMPLINENUMBER},
	{  11,  13, 	SNull, 	IllegalChar , 0},
	{  13,  14, 	SNull, 	genICON , 0},
	{  14,  32, 	SNull, 	IllegalChar , 0},
	{ '0', '9' + 1, SNumber,   CopyChar , 0},
	{ '.', '.' + 1, SFloat,    CopyChar , 0},
	{ 127, 256, 	SNull, 	IllegalChar , 0},
	{ 256, 257, 	SNull, genICON      , 0}
};

static	Scanner_CharMapInitializer	SMaybeHexNumberInitializer[] =
{
	TERMINATING_CHARS(genICON)
	{   9,  10, 	SNull,  genICON , 0},
	{  10,  11, 	SNull,  genICON , F_CM_BUMPLINENUMBER},
	{  13,  14,		SNull,	genICON , 0},
	{ 'x', 'x' + 1, SHexNumber, CopyChar, 0},
	{ 'X', 'X' + 1, SHexNumber, CopyChar, 0},
	{ '0', '9' + 1, SNumber, CopyChar , 0},
// if you want to do Octal, then 0-7 here would go to octal, rather than decimal
	{ '.', '.' + 1, SFloat, CopyChar, 0},
	{ 256, 257, 	SNull, genICON      , 0}
};

static Scanner_CharMapInitializer SHexNumberInitializer[] =
{
	TERMINATING_CHARS(genICON)
	{   9,  10, 	SNull,  genICON , 0},
	{  10,  11, 	SNull,  genICON , F_CM_BUMPLINENUMBER},
	{  13,  14,		SNull,	genICON , 0},
	{ '0', '9' + 1, SHexNumber, CopyChar, 0},
	{ 'A', 'F' + 1, SHexNumber, CopyChar, 0},
	{ 'a', 'f' + 1, SHexNumber, CopyChar, 0},
	{ 256, 257, 	SNull, genICON      , 0}
};

static	Scanner_CharMapInitializer	SFloatInitializer[] =
{
	TERMINATING_CHARS(genFCON)

	{   9,  10, 	SNull, genICON , 0},
	{  10,  11, 	SNull,    genFCON , F_CM_BUMPLINENUMBER},
	{  13,  14,		SNull, genFCON , 0},
	{ '0', '9' + 1, SFloat,   CopyChar , 0},
	{ 'f', 'f' + 1, SNull, genFCON      , 0},
	{ 256, 257, 	SNull, genFCON      , 0}
};

static Scanner_StateInitializer StateInitializers[] = 
{
	STATEINITIALIZER(SNull, 	 		SNullInitializer),
	STATEINITIALIZER(SLitString, 		SLitStringInitializer),
	STATEINITIALIZER(SIden, 			SIdenInitializer),
	STATEINITIALIZER(SNumber, 			SNumberInitializer),
	STATEINITIALIZER(SMaybeHexNumber,	SMaybeHexNumberInitializer),
	STATEINITIALIZER(SHexNumber,		SHexNumberInitializer),
	STATEINITIALIZER(SFloat,			SFloatInitializer),
	STATEINITIALIZER(SMaybeComment, 	SMaybeCommentInitializer),
	STATEINITIALIZER(SMaybeEndComment, 	SMaybeEndCommentInitializer),
	STATEINITIALIZER(SOneLineComment,	SOneLineCommentInitializer),
	STATEINITIALIZER(SComment, 			SCommentInitializer),
};

static const int NumInitializers = (sizeof (StateInitializers) / sizeof (Scanner_StateInitializer));


Scanner *CScanner_Create (void)
{
	int	i;
	int	j;

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

	return Scanner_Create (StateInitializers, NumInitializers, SNull);
}


#pragma argsused
#pragma warning (disable:4100)
static	int	IllegalChar(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = TK_ERROR;
	return 1;
}
#pragma warning (default:4100)

#pragma argsused
#pragma warning (disable:4100)
static	int	CopyChar(Scanner *s, Scanner_Token *t, int c)
{
	assert(c != EOF);
	*t->tTokenDataPtr++ = (char)c;
	return 0;
}
#pragma warning (default:4100)

#pragma argsused
#pragma warning (disable:4100)
static	int	genIDEN(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = TK_IDEN;
	*t->tTokenDataPtr = 0;
	t->tIden = Scanner_GenerateIdentifier (s, t->tTokenData, (t->tTokenDataPtr - t->tTokenData));
	return 1;
}
#pragma warning (default:4100)

#pragma argsused
#pragma warning (disable:4100)
static	int	genLITERALSTRING(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = TK_LITERALSTRING;
	*t->tTokenDataPtr = 0;
	return 1;
}
#pragma warning (default:4100)

#pragma argsused
#pragma warning (disable:4100)
static	int	genICON(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = TK_ICON;
	*t->tTokenDataPtr = 0;
	if	(t->tTokenData[1] == 'x' || t->tTokenData[1] == 'X')
		t->tICONValue = Util_htoi(t->tTokenData);
	else
		t->tICONValue = atoi(t->tTokenData);
	return 1;
}
#pragma warning (default:4100)

#pragma argsused
#pragma warning (disable:4100)
static	int	genFCON(Scanner *s, Scanner_Token *t, int c)
{
	t->tKind = TK_FCON;
	*t->tTokenDataPtr = 0;
	t->tFCONValue = (float)atof(t->tTokenData);
	return 1;
}
#pragma warning (default:4100)

#ifdef __BORLANDC__
	// turn off warnings
	#pragma option -w-
#endif

#if _MSC_VER
	#pragma warning (disable: 4100)
#endif

#define	ONESHOT(func, token) \
	static	int	func(Scanner *s, Scanner_Token *t, int c)	\
	{														\
		t->tKind = token;									\
		return 1;											\
	}

ONESHOT(genEOF, TK_EOF)
ONESHOT(genLBRACE, TK_LBRACE)
ONESHOT(genRBRACE, TK_RBRACE)
ONESHOT(genLPAREN, TK_LPAREN)
ONESHOT(genRPAREN, TK_RPAREN)
ONESHOT(genSEMI, TK_SEMI)
ONESHOT(genDOT, TK_DOT)
ONESHOT(genCOMMA, TK_COMMA)
ONESHOT(genSLASH, TK_SLASH)
ONESHOT(genHYPHEN, TK_HYPHEN)
ONESHOT(genPLUS, TK_PLUS)
ONESHOT(genTILDE, TK_TILDE)
ONESHOT(genSHARP, TK_SHARP)
ONESHOT(genMOD, TK_MOD)
ONESHOT(genHAT, TK_HAT)
ONESHOT(genAMPER, TK_AMPER)
ONESHOT(genSTAR, TK_STAR)
ONESHOT(genEQUAL, TK_EQUAL)
ONESHOT(genBANG, TK_BANG)

