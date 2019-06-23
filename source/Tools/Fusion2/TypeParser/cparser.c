/****************************************************************************************/
/*  CPARSER.C                                                                           */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Partial parser for C code.                                             */
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
#ifdef	__BORLANDC__
#include	<dir.h>
#else
#define	MAXPATH	_MAX_PATH
#endif
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include	<setjmp.h>
#include	<stdarg.h>

#include	"iden.h"
#include	"cscanner.h"
#include	"cparser.h"
#include	"type.h"
#include	"symtab.h"
#include	"ram.h"
#include	"util.h"
#include	"filepath.h"

#define	CONTENTS_USER_MASK	(0xFFFF0000)

static	char *	KeywordNames[] =
{
	"typedef",
	"struct",
	"int",
	"char",
	"float",
	"geFloat",
	"GE_RGBA",
	"geWorld_Model",
	"geVec3d",
	"pragma",
	"void",
	"enum",
	"geBoolean"
};

static	jmp_buf	parserError;

#define	NUMKEYWORDS	(sizeof(KeywordNames) / sizeof(KeywordNames[0]))

#define	KW_NOTAKEYWORD	0
#define	KW_TYPEDEF		1
#define KW_STRUCT		2
#define	KW_INT			3
#define	KW_CHAR			4
#define	KW_FLOAT		5
#define	KW_GE_FLOAT		6
#define	KW_GE_RGB		7
#define	KW_GE_MODEL		8
#define	KW_VEC3D		9
#define	KW_PRAGMA		10
#define	KW_VOID			11
#define	KW_ENUM			12
#define KW_BOOLEAN		13

typedef	struct	CParser_BrushEnumValue
{
	Iden *			Name;
	unsigned long	Value;
}	CParser_BrushEnumValue;

typedef	struct	CParser
{
	Scanner *					cpScanner;
	Iden_HashTable *			cpIdenHashTable;
	int							cpNestingLevel;
	Type *						cpTypes;
	Iden *						cpNoName;
	SymTab *					cpSymbolTable;
	SymTab_Scope *				cpGlobalScope;
	Type *						cpVoidType;
	CParser_ErrFunc				cpErrorFunc;
	int							cpBrushContentsCount;
	CParser_BrushEnumValue *	cpBrushContents;
}	CParser;

#define ERR_EXPECTEDPOINTER			0
#define ERR_EXPECTEDIDEN			1
#define ERR_EXPECTEDSEMI			2
#define ERR_SYNTAXERROR				3
#define ERR_EXPECTEDFIELDNAME		4
#define ERR_LIMITEDSTRUCTSUPPORT	5
#define ERR_ILLEGALTYPEREDEFINITION	6
#define	ERR_BADFORWARDTYPEDEF		7
#define ERR_ONLYPRAGMA				8
#define	ERR_EXPECTEDTYPENAME		9
#define	ERR_UNDEFINEDTYPE			10
#define	ERR_OUTOFMEMORY				11
#define	ERR_EXPECTEDENUM			12
#define	ERR_EXPECTEDRBRACE			13
#define	ERR_VALUENOTINRANGE			14
#define ERR_BADREDEFINITION			15
#define ERR_VALUEUSEDTWICE			16

static	char *	ErrStrings[] =
{
	"Expected a pointer declaration",
	"Expected an identifier",
	"Expected a semicolon",
	"Syntax error",
	"Expected a field name",
	"Structs must be of the form 'typedef struct tagfoo { } foo' or 'typedef struct tagfoo foo'",
	"Illegal type definition",
	"Bad forward type declaration",
	"Only pragma directives allowed inside structure definitions",
	"Expected a type name",
	"Undefined type '%s'",
	"Out of memory",
	"Expected enum keyword",
	"Expected a '}'",
	"A numeric value was not in the expected range",
	"Redefinition of a symbol",
	"Value already defined",
};

static	const char *	LoadErrorString(int errCode)
{
	return ErrStrings[errCode];
}

CParser *	CParser_Init(CParser_ErrFunc errFunc)
{
	CParser *	p;
	int			i;

	p = geRam_Allocate(sizeof(*p));
	if	(!p)
		return p;

	memset(p, 0, sizeof(*p));

	p->cpErrorFunc = errFunc;

	p->cpSymbolTable = SymTab_Create();
	if	(!p->cpSymbolTable)
	{
		CParser_Destroy(p);
		return NULL;
	}

	p->cpGlobalScope = SymTab_CreateScope(p->cpSymbolTable, NULL, F_SCP_FILE);
	if	(!p->cpGlobalScope)
	{
		CParser_Destroy(p);
		return NULL;
	}

	p->cpIdenHashTable = Iden_CreateHashTable();
	if	(!p->cpIdenHashTable)
	{
		CParser_Destroy(p);
		return NULL;
	}

	p->cpNoName = Iden_HashName(p->cpIdenHashTable, "<no name>", 9);
	if	(!p->cpNoName)
	{
		CParser_Destroy(p);
		return NULL;
	}

	p->cpScanner = CScanner_Create ();
	if (p->cpScanner == NULL)
	{
		CParser_Destroy (p);
		return NULL;
	}

	for	(i = 0; i < NUMKEYWORDS; i++)
	{
		Iden *	id;
		id = Iden_HashName(p->cpIdenHashTable, KeywordNames[i], strlen(KeywordNames[i]));
		id->idenKeyword = i + 1;
	}

	p->cpTypes = Type_InitTypeList(p->cpIdenHashTable);

	if	(p->cpTypes == NULL)
	{
		CParser_Destroy (p);
		return NULL;
	}

	p->cpVoidType = Type_FindTypeByName(p->cpTypes, Iden_HashName(p->cpIdenHashTable, "void", 4));
	if	(!p->cpVoidType)
	{
		CParser_Destroy (p);
		return NULL;
	}

	return p;
}

void	CParser_Destroy(CParser *p)
{
	assert(p);

	if (p->cpScanner != NULL)
	{
		Scanner_Destroy(p->cpScanner);
	}

	if	(p->cpBrushContents != NULL)
	{
		assert(p->cpBrushContentsCount > 0);
		geRam_Free(p->cpBrushContents);
	}

	if (p->cpGlobalScope != NULL)
	{
		SymTab_DestroyScope(p->cpGlobalScope);
	}

	if (p->cpSymbolTable != NULL)
	{
		SymTab_Destroy(p->cpSymbolTable);
	}

	if (p->cpTypes != NULL)
	{
		Type_DestroyTypeList(p->cpTypes);
	}

	if (p->cpIdenHashTable != NULL)
	{
		Iden_DestroyHashTable(p->cpIdenHashTable);
	}

	geRam_Free (p);
}

#ifdef	ELIDEBUG
static	void	printToken(const Scanner_Token *t)
{
//static	count = 0;
//	printf("% 3d: ", count++);
	switch	(t->tKind)
	{
	case	TK_IDEN:
//		printf("<TK_IDEN: '%s'>\n", t->tTokenData);
		printf("<TK_IDEN: '%s'>\n", t->tIden->idenSpelling);
		break;
	case	TK_ICON:
		printf("<TK_ICON: '%d'>\n", t->tICONValue);
		break;
	case	TK_FCON:
		printf("<TK_FCON: '%f'>\n", t->tFCONValue);
		break;
	case	TK_LPAREN:
		printf("<TK_LPAREN>\n");
		break;
	case	TK_RPAREN:
		printf("<TK_RPAREN>\n");
		break;
	case	TK_LBRACE:
		printf("<TK_LBRACE>\n");
		break;
	case	TK_RBRACE:
		printf("<TK_RBRACE>\n");
		break;
	case	TK_SEMI:
		printf("<TK_SEMI>\n");
		break;
	case	TK_EQUAL:
		printf("<TK_EQUAL>\n");
		break;
	case	TK_PLUS:
		printf("<TK_PLUS>\n");
		break;
	case	TK_SHARP:
		printf("<TK_SHARP>\n");
		break;
	case	TK_STAR:
		printf("<TK_STAR>\n");
		break;
	case	TK_EOF:
		printf("<TK_EOF>\n");
		break;
	default:
		printf("<Illegal token>\n");
		break;
	}
}
#endif

static	void	CParser_Error(CParser *p, int errCode, ...)
{
	Scanner_Token	t;
	va_list			ap;
	char			buff[1024];
	char			buff2[1024];

	va_start(ap, errCode);
	
#ifdef	DEBUG
	memset(buff, 0, sizeof(buff);
#endif
	vsprintf(buff, LoadErrorString(errCode), ap);
#ifdef	DEBUG
	assert(buff[sizeof(buff) - 1] == 0);
#endif
	va_end(ap);

	sprintf(buff2,
			"Error %s %d: %s\n",
			Scanner_GetFileName(p->cpScanner),
			Scanner_GetLineNo(p->cpScanner),
			buff);
 	
 	if	(p->cpErrorFunc)
 	{
		p->cpErrorFunc(Scanner_GetFileName(p->cpScanner),
					   Scanner_GetLineNo(p->cpScanner),
					   buff2);
	}
	else
	{
		printf("%s", buff2);
	}

	for (;;)
	{
		Scanner_Scan(p->cpScanner, &t);

		if	(t.tKind == TK_LBRACE)
			p->cpNestingLevel++;
		if	(t.tKind == TK_RBRACE)
			p->cpNestingLevel++;
		if	(p->cpNestingLevel == 0)
			break;
		if	(t.tKind == TK_EOF)
			break;
	}
	longjmp(parserError, 1);
}

static	void	CParser_ParseUserDecl(CParser *p, Type *tp, Iden *fieldTypeName, int publishing)
{
	Scanner_Token	t;
	SymTab_Symbol *	fieldSym;
	Type *			ptrTp;
	Type *			fieldTp;
	
	Scanner_Scan(p->cpScanner, &t);
	if	(t.tKind != TK_STAR)
		CParser_Error(p, ERR_EXPECTEDPOINTER);

	fieldTp = Type_FindTypeByName(p->cpTypes, fieldTypeName);
	if	(!fieldTp)
	{
		if	(publishing)
		{
			CParser_Error(p, ERR_UNDEFINEDTYPE, fieldTypeName->idenSpelling);
		}
		else
		{
			fieldTp = Type_FindTypeByName(p->cpTypes, p->cpVoidType->tpName);
			// Could be multiple pointers here.  Burn them off.
			assert(t.tKind == TK_STAR);
			while	(t.tKind == TK_STAR)
				Scanner_Scan(p->cpScanner, &t);

			if	(t.tKind != TK_IDEN)
				CParser_Error(p, ERR_EXPECTEDIDEN);
		}
	}
	else
	{
		Scanner_Scan(p->cpScanner, &t);
		if	(t.tKind != TK_IDEN)
			CParser_Error(p, ERR_EXPECTEDIDEN);
	}
	ptrTp = Type_CreatePtrType(&p->cpTypes, fieldTp);

	fieldSym = SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t.tIden, ptrTp);
	Type_AddTypeField(tp, fieldSym, (unsigned short)(publishing ? F_TF_PUBLISHED : 0));

	Scanner_Scan(p->cpScanner, &t);
	if	(t.tKind != TK_SEMI)
		CParser_Error(p, ERR_EXPECTEDSEMI);
}

static	void	ScanExpecting(CParser *p, Scanner_Token *t, CScanner_TokenKind kind)
{
	Scanner_Scan(p->cpScanner, t);
	if	(t->tKind != kind)
		CParser_Error(p, ERR_SYNTAXERROR);
}

static	void	ParseDefaultValue(CParser *p, Type *tp)
{
	Scanner_Token	t;
	TypeField *		tf;

	ScanExpecting(p, &t, TK_LPAREN);
	ScanExpecting(p, &t, TK_IDEN);
	tf = Type_GetTypeField(tp, t.tIden);
	if	(!tf)
		CParser_Error(p, ERR_EXPECTEDFIELDNAME);
	ScanExpecting(p, &t, TK_COMMA);
	ScanExpecting(p, &t, TK_LITERALSTRING);
	tf->tfDefaultValue = Util_Strdup (t.tTokenData);
	if	(!tf->tfDefaultValue)
		CParser_Error(p, ERR_OUTOFMEMORY);
	ScanExpecting(p, &t, TK_RPAREN);
}

static	Type *	FindType(CParser *p, Iden *name)
{
	SymTab_Symbol *	sym;

	sym = SymTab_FindSymbolInScope(p->cpSymbolTable, p->cpGlobalScope, name);
	if	(!sym)
		return NULL;

	return sym->symType;
}

static void CParser_ParseSpecialField (CParser *p, Type *tp, TypeField **ptfRslt)
{
	TypeField *		tf;
	Scanner_Token	t;


	ScanExpecting(p, &t, TK_LPAREN);
	ScanExpecting(p, &t, TK_IDEN);
	tf = Type_GetTypeField(tp, t.tIden);
	if	(!tf)
		CParser_Error(p, ERR_EXPECTEDFIELDNAME);
	ScanExpecting(p, &t, TK_RPAREN);
	*ptfRslt = tf;
}

static	void	CParser_ParsePrivatePtrDecl(CParser *p, Type *tp, Scanner_Token *t)
{
	SymTab_Symbol *	fieldSym;
	Type *			ptrTp;
	Type *			fieldTp;
	
	fieldTp = Type_FindTypeByName(p->cpTypes, p->cpVoidType->tpName);
	// Could be multiple pointers here.  Burn them off.
	assert(t->tKind == TK_STAR);
	while	(t->tKind == TK_STAR)
		Scanner_Scan(p->cpScanner, t);

	if	(t->tKind != TK_IDEN)
		CParser_Error(p, ERR_EXPECTEDIDEN);

	ptrTp = Type_CreatePtrType(&p->cpTypes, fieldTp);

	fieldSym = SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t->tIden, ptrTp);
	Type_AddTypeField(tp, fieldSym, (unsigned short)0);

	Scanner_Scan(p->cpScanner, t);
	if	(t->tKind != TK_SEMI)
		CParser_Error(p, ERR_EXPECTEDSEMI);
}

static	void	CParser_ParseStruct(CParser *p, const char *typeIcon)
{
	Scanner_Token	t;
	Type *			tp;
	SymTab_Symbol *	fieldSym;
	Type *			fieldTp;
	int				publishing;

	publishing = 0;

	/* Expect: struct <iden> { */
	Scanner_Scan(p->cpScanner, &t);
	if	(t.tKind != TK_IDEN || t.tIden->idenKeyword != KW_NOTAKEYWORD)
		CParser_Error(p, ERR_LIMITEDSTRUCTSUPPORT);

	tp = FindType(p, t.tIden);
	if	(!tp)
	{
		tp = Type_CreateType(&p->cpTypes, t.tIden, 0);
		tp->tpScope = p->cpGlobalScope;
		SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t.tIden, tp);
	}
	else
	{
		if	(tp->tpTopType != T_STRUCT || tp->t.s.tpFields)
			CParser_Error(p, ERR_ILLEGALTYPEREDEFINITION);
	}
	if (tp->t.s.tpIcon != NULL)
	{
		geRam_Free (tp->t.s.tpIcon);
	}
	{
		// prepend the current file's path to the icon file name
		const char *scnFilename;
		char WorkFilename[300];

		scnFilename = Scanner_GetFileName (p->cpScanner);
		FilePath_GetDriveAndDir (scnFilename, WorkFilename);
		FilePath_AppendName (WorkFilename, typeIcon, WorkFilename);
		tp->t.s.tpIcon = Util_Strdup (WorkFilename);
		if	(!tp->t.s.tpIcon)
			CParser_Error(p, ERR_OUTOFMEMORY);
	}

	Scanner_Scan(p->cpScanner, &t);
	if	(t.tKind != TK_LBRACE)
	{
		/* This must be something of the form 'typedef struct tagfoo foo' */
		if	(t.tKind != TK_IDEN)
			CParser_Error(p, ERR_BADFORWARDTYPEDEF);
				
		ScanExpecting(p, &t, TK_SEMI);
		return;
	}

	p->cpNestingLevel++;

	for (;;)
	{
		Scanner_Scan(p->cpScanner, &t);

		while	(t.tKind == TK_SHARP)
		{
			ScanExpecting(p, &t, TK_IDEN);
			if	(t.tIden->idenKeyword != KW_PRAGMA)
				CParser_Error(p, ERR_ONLYPRAGMA);
			ScanExpecting(p, &t, TK_IDEN);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Published"))
				publishing = 1;
			if	(!strcmp(t.tIden->idenSpelling, "GE_Private"))
				publishing = 0;
			if	(!strcmp(t.tIden->idenSpelling, "GE_DefaultValue"))
				ParseDefaultValue(p, tp);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Origin"))
				CParser_ParseSpecialField (p, tp, &tp->tpOriginField);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Arc"))
				CParser_ParseSpecialField (p, tp, &tp->tpArcField);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Angles"))
				CParser_ParseSpecialField (p, tp, &tp->tpAnglesField);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Radius"))
				CParser_ParseSpecialField (p, tp, &tp->tpRadiusField);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Documentation"))
			{
				TypeField *	tf;

				ScanExpecting(p, &t, TK_LPAREN);
				ScanExpecting(p, &t, TK_IDEN);
				tf = Type_GetTypeField(tp, t.tIden);
				if	(!tf)
					CParser_Error(p, ERR_EXPECTEDFIELDNAME);
				ScanExpecting(p, &t, TK_COMMA);
				ScanExpecting(p, &t, TK_LITERALSTRING);
				tf->tfDocumentation = Util_Strdup(t.tTokenData);
				if	(!tf->tfDocumentation)
					CParser_Error(p, ERR_OUTOFMEMORY);
				ScanExpecting(p, &t, TK_RPAREN);
			}

			Scanner_Scan(p->cpScanner, &t);
		}

		if	(t.tKind == TK_RBRACE)
		{
			ScanExpecting(p, &t, TK_IDEN);
			SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t.tIden, tp);
			tp->tpName = t.tIden;
			ScanExpecting(p, &t, TK_SEMI);
			return;
		}

		if	(t.tKind != TK_IDEN)
			CParser_Error(p, ERR_EXPECTEDTYPENAME);
		switch	(t.tIden->idenKeyword)
		{
		case	KW_NOTAKEYWORD:
			CParser_ParseUserDecl(p, tp, t.tIden, publishing);
			break;

		case	KW_VOID:
			if	(publishing)
				CParser_Error(p, ERR_SYNTAXERROR);
			ScanExpecting(p, &t, TK_STAR);
			CParser_ParsePrivatePtrDecl(p, tp, &t);
			break;

		case	KW_INT:
		case	KW_FLOAT:
		case	KW_GE_FLOAT:
		case	KW_GE_RGB:
		case	KW_VEC3D:
		case	KW_BOOLEAN:
			fieldTp = Type_FindTypeByName(p->cpTypes, t.tIden);
			if	(!fieldTp)
				CParser_Error(p, ERR_UNDEFINEDTYPE, t.tIden->idenSpelling);
			Scanner_Scan(p->cpScanner, &t);
			if	(t.tKind == TK_STAR)
			{
				if	(publishing)
					CParser_Error(p, ERR_SYNTAXERROR, t.tIden->idenSpelling);
				else
					CParser_ParsePrivatePtrDecl(p, tp, &t);
			}
			else if	(t.tKind == TK_IDEN)
			{
				fieldSym = SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t.tIden, fieldTp);
				Type_AddTypeField(tp, fieldSym, (unsigned short)(publishing ? F_TF_PUBLISHED : 0));
				ScanExpecting(p, &t, TK_SEMI);
			}
			else
			{
				CParser_Error(p, ERR_SYNTAXERROR, t.tIden->idenSpelling);
			}
			break;

		case	KW_GE_MODEL:
			ScanExpecting(p, &t, TK_STAR);
			ScanExpecting(p, &t, TK_IDEN);
			fieldTp = Type_FindTypeByName(p->cpTypes, Iden_HashName(p->cpIdenHashTable, "<model>", 7));
			if	(!fieldTp)
				CParser_Error(p, ERR_UNDEFINEDTYPE, "<model>");
			fieldSym = SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t.tIden, fieldTp);
			Type_AddTypeField(tp, fieldSym, (unsigned short)(publishing ? F_TF_PUBLISHED : 0));
			ScanExpecting(p, &t, TK_SEMI);
			break;

		case	KW_CHAR:
			ScanExpecting(p, &t, TK_STAR);
			ScanExpecting(p, &t, TK_IDEN);
			fieldTp = Type_FindTypeByName(p->cpTypes, Iden_HashName(p->cpIdenHashTable, "<string>", 8));
			if	(!fieldTp)
				CParser_Error(p, ERR_UNDEFINEDTYPE, "<string>");
			fieldSym = SymTab_CreateSymbol(p->cpSymbolTable, tp->tpScope, t.tIden, fieldTp);
			Type_AddTypeField(tp, fieldSym, (unsigned short)(publishing ? F_TF_PUBLISHED : 0));
			ScanExpecting(p, &t, TK_SEMI);
			break;

		default:
			CParser_Error(p, ERR_SYNTAXERROR);
			break;
		}
	}
}

static	void	CParser_ParseType(CParser *p, const char *typeIcon)
{
	Scanner_Token	t;

	Scanner_Scan(p->cpScanner, &t);
	if	(t.tIden->idenKeyword == KW_STRUCT)
		CParser_ParseStruct(p, typeIcon);
}

int	CParser_GetContentsCount(const CParser *p)
{
	assert(p);
	return p->cpBrushContentsCount;
}

void	CParser_GetContentsNameAndValue(const CParser *p, int idx, const char **Name, unsigned long *Value)
{
	assert(p);
	assert(p->cpBrushContents);
	assert(idx < p->cpBrushContentsCount);

	*Name = p->cpBrushContents[idx].Name->idenSpelling;
	*Value = p->cpBrushContents[idx].Value;
}

static	int	CheckBitMask(unsigned long Value, unsigned long Mask)
{
	int	i;
	int	BitCount;

	if	(!(Value & Mask))
		return 0;

	BitCount = 0;
	for	(i = 0; i < sizeof(Value) * 8; i++)
	{
		if	(Value & 1)
			BitCount++;
		Value = Value >> 1;
	}

	if	(BitCount > 1)
		return 0;

	return 1;
}

static	void	CParser_ParseContentsEnum(CParser *p)
{
	Scanner_Token	t;

	Scanner_Scan(p->cpScanner, &t);
	if	(t.tIden->idenKeyword != KW_ENUM)
		CParser_Error(p, ERR_EXPECTEDENUM);

	ScanExpecting(p, &t, TK_LBRACE);
	while	(t.tKind != TK_EOF && t.tKind != TK_ERROR && t.tKind != TK_RBRACE)
	{
		CParser_BrushEnumValue *	NewValues;
		int							idx;
		Iden *						Name;
		int							i;
		unsigned long				Value;

		Scanner_Scan(p->cpScanner, &t);
		if	(t.tKind == TK_RBRACE)
			break;

		if	(t.tKind != TK_IDEN)
			CParser_Error(p, ERR_SYNTAXERROR);
		Name = t.tIden;

		ScanExpecting(p, &t, TK_EQUAL);

		ScanExpecting(p, &t, TK_ICON);
		Value = (unsigned long)t.tICONValue;

		Scanner_Scan(p->cpScanner, &t);
		if	(t.tKind != TK_COMMA && t.tKind != TK_RBRACE)
			CParser_Error(p, ERR_SYNTAXERROR);

		/*
			Make sure that we're in the user mask range, and that there's
			only one bit set.
		*/
		if	(!CheckBitMask(t.tICONValue, CONTENTS_USER_MASK))
			CParser_Error(p, ERR_VALUENOTINRANGE);

		/*
			Check to see if we already have a name that's the same or a value
			that's the same.
		*/
		for	(i = 0; i < p->cpBrushContentsCount; i++)
		{
			if	(Name == p->cpBrushContents[i].Name)
				CParser_Error(p, ERR_BADREDEFINITION);

			if	((unsigned long)t.tICONValue == p->cpBrushContents[i].Value)
				CParser_Error(p, ERR_VALUEUSEDTWICE);
		}

		NewValues = geRam_Realloc(p->cpBrushContents,
								  sizeof(*p->cpBrushContents) * (p->cpBrushContentsCount + 1));
		if	(!NewValues)
			CParser_Error(p, ERR_OUTOFMEMORY);

		p->cpBrushContents = NewValues;
		idx = p->cpBrushContentsCount;
		p->cpBrushContentsCount++;

		p->cpBrushContents[idx].Name	= Name;
		p->cpBrushContents[idx].Value	= t.tICONValue;
	}

	if	(t.tKind != TK_RBRACE)
		CParser_Error(p, ERR_EXPECTEDRBRACE);

	ScanExpecting(p, &t, TK_IDEN);
	ScanExpecting(p, &t, TK_SEMI);
}

static	void	CParser_Parse(CParser *p)
{
	Scanner_Token	t;
	int				parseType;
	int				parseContents;
	char 			typeIcon[MAXPATH];

	if	(setjmp(parserError) != 0)
		return;

	parseType = 0;
	parseContents = 0;
	do
	{
		Scanner_Scan(p->cpScanner, &t);
//		printf("%03d", CScanner_CurrentLine(p->cpScanner));
//		printToken(&t);
		if	(t.tKind == TK_SHARP)
		{
			ScanExpecting(p, &t, TK_IDEN);
			if	(t.tIden->idenKeyword != KW_PRAGMA)
				continue;
			ScanExpecting(p, &t, TK_IDEN);
			if	(!strcmp(t.tIden->idenSpelling, "GE_Type"))
			{
				ScanExpecting(p, &t, TK_LPAREN);
				ScanExpecting(p, &t, TK_LITERALSTRING);
				strcpy(typeIcon, t.tTokenData);
				ScanExpecting(p, &t, TK_RPAREN);
				parseType = 1;
			}

			if	(!strcmp(t.tIden->idenSpelling, "GE_BrushContents"))
			{
				assert(!parseType);
				parseContents = 1;
			}
			Scanner_Scan(p->cpScanner, &t);
		}
		if	(t.tKind != TK_IDEN)
		{
			parseType = 0;
			continue;
		}
		switch	(t.tIden->idenKeyword)
		{
		case	KW_NOTAKEYWORD:
			break;

		case	KW_TYPEDEF:
			if	(parseType)
			{
				parseType = 0;
				CParser_ParseType(p, typeIcon);
			}
			if	(parseContents)
			{
				parseContents = 0;
				CParser_ParseContentsEnum(p);
			}
			break;

		default:
			break;
		}
	}	while	(t.tKind != TK_EOF);
}

void		CParser_ParseFile(CParser *p, const char *file)
{
	assert (p != NULL);
	assert (p->cpScanner != NULL);
	assert (file != NULL);

	if (Scanner_InitFile (p->cpScanner, file, SCANNER_FILE_TEXT, p->cpIdenHashTable))
	{
		CParser_Parse(p);
		Scanner_Uninit (p->cpScanner);
	}
}

void		CParser_ParseMemoryBlock (CParser *p, const char *Block, int length, const char *Name)
{
	assert (p != NULL);
	assert (Block != NULL);
	assert (length >= 0);
	assert (p->cpScanner != NULL);

	if (Scanner_InitMemory (p->cpScanner, Block, length, Name, p->cpIdenHashTable))
	{
		CParser_Parse (p);
		Scanner_Uninit (p->cpScanner);
	}
}

#ifdef	ELIDEBUG
static	void	DumpTypes(Type *tp)
{
	while	(tp)
	{
		Type_DumpType(tp, 1);
		printf("\n");
		tp = tp->tpNext;
	}
}
#endif

Type *		CParser_GetFirstStruct(StructIter *iter, CParser *p)
{
	Type *	tp;

	memset(iter, 0, sizeof(*iter));

	tp = p->cpTypes;
	while	(tp)
	{
		if	(tp->tpTopType == T_STRUCT)
		{
			iter->siType = tp;
			return tp;
		}
		tp = tp->tpNext;
	}
	return NULL;
}

Type *		CParser_GetNextStruct(StructIter *iter)
{
	Type *	tp;

	tp = iter->siType->tpNext;
	while	(tp)
	{
		if	(tp->tpTopType == T_STRUCT)
		{
			iter->siType = tp;
			return tp;
		}
		tp = tp->tpNext;
	}
	iter->siType = tp;
	return NULL;
}

int			CParser_GetFirstField(FieldIter *fi, Type *tp, TopType *tt, const char **tpName, const char **fName, int *published, const char **defaultValue)
{
	assert(tp->tpTopType == T_STRUCT);
	fi->fiCurrentField = tp->t.s.tpFields;
	if	(!fi->fiCurrentField)
		return 0;

	tp = fi->fiCurrentField->tfSymbol->symType;

	if	(tp->tpTopType == T_PTR && tp->t.p.tpPtrType->tpTopType == T_STRUCT)
	{
		tp = tp->t.p.tpPtrType;
		assert(tp->tpTopType == T_STRUCT);
		*tpName = tp->tpName->idenSpelling;
	}
	else
		*tpName = NULL;

	*tt = tp->tpTopType;

	*fName = fi->fiCurrentField->tfSymbol->symName->idenSpelling;

	*published = fi->fiCurrentField->tfFlags & F_TF_PUBLISHED;

	*defaultValue = fi->fiCurrentField->tfDefaultValue;

	return 1;
}

int			CParser_GetNextField(FieldIter *fi, TopType *tt, const char **tpName, const char **fName, int *published, const char **defaultValue)
{
	Type *	tp;

	fi->fiCurrentField = fi->fiCurrentField->tfNext;
	if	(!fi->fiCurrentField)
		return 0;


	tp = fi->fiCurrentField->tfSymbol->symType;

	if	(tp->tpTopType == T_PTR && tp->t.p.tpPtrType->tpTopType == T_STRUCT)
	{
		tp = tp->t.p.tpPtrType;
		assert(tp->tpTopType == T_STRUCT);
		*tpName = tp->tpName->idenSpelling;
	}
	else
		*tpName = NULL;

	*tt = tp->tpTopType;

	*fName = fi->fiCurrentField->tfSymbol->symName->idenSpelling;

	*published = fi->fiCurrentField->tfFlags & F_TF_PUBLISHED;

	*defaultValue = fi->fiCurrentField->tfDefaultValue;

	return 1;
}

const char *CParser_GetTypeName(const Type *tp)
{
	assert(tp->tpTopType == T_STRUCT);
	return tp->tpName->idenSpelling;
}

static	char *	topTypeNames[] =
{
	"int",
	"float",
	"color",
	"point",
	"string",
	"model",
	"ptr",
	"struct",
};

static	void	WriteInt(FILE *fp, int i)
{
	fwrite(&i, 4, 1, fp);
}

static	void	WriteString(FILE *fp, const char *s)
{
	int	len;

	len = strlen(s);

	WriteInt(fp, len);
	fwrite(s, len, 1, fp);
}

int	CParser_WriteTypesToMap(CParser *p, FILE *fp)
{
	StructIter	si;
	FieldIter	fi;
	Type *		tp;

	tp = CParser_GetFirstStruct(&si, p);
	while	(tp)
	{
		int				res;
		int				published;
		TopType			tt;
		const char *	typeName;
		const char *	fieldName;
		const char *	defaultValue;
		int				fieldCount;

		res = CParser_GetFirstField(&fi, tp, &tt, &typeName, &fieldName, &published, &defaultValue);

		fieldCount = 0;
		while	(res)
		{
			fieldCount++;
			res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName, &published, &defaultValue);
		}

		assert(fieldCount != 0);

		/* Adjust for default value pairs and 2 special fields */
		fieldCount = fieldCount * 2 + 2;

		/* Write out num brushes */
		WriteInt(fp, 0);
		/* Write out model motion flag */
		WriteInt(fp, 0);
		/* Write out num fields */
		WriteInt(fp, fieldCount);
		
		/* Write special classname and %typename% fields */
		WriteString(fp, "classname");
		WriteString(fp, "%typedef%");
		WriteString(fp, "%typename%");
		WriteString(fp, CParser_GetTypeName(tp));

		res = CParser_GetFirstField(&fi, tp, &tt, &typeName, &fieldName, &published, &defaultValue);
		while	(res)
		{
			switch (tt)
			{
				case T_STRUCT :
					WriteString(fp, fieldName);
					WriteString(fp, typeName);
					WriteString(fp, "%defaultvalue%");
					WriteString(fp, "");
					break;

				// we treat booleans as if they are ints...
				case T_BOOLEAN :
					WriteString(fp, fieldName);
					WriteString(fp, "int");
					WriteString(fp, "%defaultvalue%");
					WriteString(fp, "");
					break;

				default :
					WriteString(fp, fieldName);
					WriteString(fp, topTypeNames[tt]);
					WriteString(fp, "%defaultvalue%");
					WriteString(fp, "");
					break;
			}
			res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName, &published, &defaultValue);
		}
		tp = CParser_GetNextStruct(&si);
	}

	return 1;
}

const char *	CParser_GetOriginFieldName(Type *tp)
{
	if	(tp->tpOriginField)
		return tp->tpOriginField->tfSymbol->symName->idenSpelling;

	return NULL;
}

const char *	CParser_GetRadiusFieldName(Type *tp)
{
	if (tp->tpRadiusField)
		return tp->tpRadiusField->tfSymbol->symName->idenSpelling;
	return NULL;
}

const char *	CParser_GetAnglesFieldName(Type *tp)
{
	if (tp->tpAnglesField)
		return tp->tpAnglesField->tfSymbol->symName->idenSpelling;
	return NULL;
}

const char *	CParser_GetArcFieldName(Type *tp)
{
	if (tp->tpArcField)
		return tp->tpArcField->tfSymbol->symName->idenSpelling;
	return NULL;
}

const char *	CParser_GetTypeFieldDocumentation(CParser *p, Type *tp, const char *FieldName)
{
	Iden *	id;

	assert(p != NULL);
	assert(tp != NULL);
	assert(FieldName != NULL);

	id = Iden_HashName(p->cpIdenHashTable, FieldName, strlen(FieldName));
	return Type_GetFieldDocumentation(tp, id);
}

const char *	CParser_GetIconName (Type *pType)
{
	return pType->t.s.tpIcon;
}

#if 0
// Testing code
static	char *	topTypeNames[] =
{
	"int",
	"float",
	"color",
	"point",
	"string",
	"model",
	"ptr",
	"struct",
};

void	main(void)
{
	CParser *	p;
	StructIter	si;
	FieldIter	fi;
	Type *		tp;

	p = CParser_Init();

	CParser_ParseFile(p, "foo.h");
//	DumpTypes(p->cpTypes);

	tp = CParser_GetFirstStruct(&si, p);
	while	(tp)
	{
		int				res;
		TopType			tt;
		const char *	typeName;
		const char *	fieldName;

		printf("%s\n{\n", CParser_GetTypeName(tp));

		res = CParser_GetFirstField(&fi, tp, &tt, &typeName, &fieldName);
		while	(res)
		{
			if	(tt == T_STRUCT)
				printf("\t%s\t%s\n", typeName, fieldName);
			else
				printf("\t%s\t%s\n", topTypeNames[tt], fieldName);
			res = CParser_GetNextField(&fi, &tt, &typeName, &fieldName);
		}
		printf("}\n\n");
		tp = CParser_GetNextStruct(&si);
	}
}
#endif

