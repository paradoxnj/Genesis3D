/****************************************************************************************/
/*  SCANNER.H                                                                           */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Generic token scanner interface                                        */
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
#ifndef SCANNER_H
#define SCANNER_H

#include "BASETYPE.H"
#include "iden.h"
#include <stdio.h>

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct	Scanner	Scanner;

#define SCANNER_MAXDATA	1024

typedef struct	Scanner_Token
{
	int					tKind;
	char				tTokenData[SCANNER_MAXDATA];
	char *				tTokenDataPtr;
	int					tICONValue;
	float				tFCONValue;
	Iden *				tIden;
}	Scanner_Token;


typedef	int	(*ScannerAction)(Scanner *s, Scanner_Token *t, int c);

typedef struct Scanner_CharMap
{
	struct Scanner_CharMap *	cmNextState;
	ScannerAction		cmAction;
	unsigned short		cmFlags;
}	Scanner_CharMap;

#define	F_CM_PUSHBACKCHAR	0x0001
#define	F_CM_BUMPLINENUMBER	0x0002

typedef struct
{
	int					cmiStartChar;
	int					cmiEndChar;
	struct Scanner_CharMap *	cmiNextState;
	ScannerAction		cmiAction;
	unsigned short		cmiFlags;
}	Scanner_CharMapInitializer;


enum
{
	SCANNER_FILE_TEXT,
	SCANNER_FILE_BINARY
};

#define	STATEINITIALIZER(s,i) { (s), (i), sizeof(i) / sizeof((i)[0]) }

typedef struct
{
	Scanner_CharMap *				siMap;
	Scanner_CharMapInitializer *	siMapInitializer;
	int								siInitializerCount;
} Scanner_StateInitializer;

Scanner *		Scanner_Create 
	(
	  const Scanner_StateInitializer StateInitializers[], 
	  int nInitializers,
	  const Scanner_CharMap *NullState
	);

geBoolean		Scanner_InitMemory (Scanner *pScanner, const char * DataBlock, int length, const char * Name, Iden_HashTable *ht);
geBoolean		Scanner_InitFile (Scanner *pScanner, const char *file, int FileType, Iden_HashTable *ht);
geBoolean		Scanner_Uninit (Scanner *pScanner);
void			Scanner_Destroy(Scanner *s);
void			Scanner_Scan(Scanner *s, Scanner_Token *t);
Iden *			Scanner_GenerateIdentifier (Scanner *s, const char *str, int len);

int				Scanner_GetLineNo(const Scanner *s);
const char *	Scanner_GetFileName(const Scanner *s);
FILE *			Scanner_GetFile (Scanner *s);
int				Scanner_GetOffset (Scanner *s);

#ifdef __cplusplus
	}
#endif


#endif
