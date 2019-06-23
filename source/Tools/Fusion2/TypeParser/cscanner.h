/****************************************************************************************/
/*  CSCANNER.C                                                                          */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: C Language specific scanner tokens                                     */
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
#ifndef	CSCANNER_H
#define	CSCANNER_H

#include "scanner.h"

typedef	enum
{
	TK_ERROR = 0,		/* Must be zero */

	TK_EOF,
	TK_IDEN,
	TK_ICON,
	TK_FCON,

	TK_SEMI,

	TK_LPAREN,
	TK_RPAREN,
	TK_LBRACE,
	TK_RBRACE,

	TK_SHARP,

	TK_PLUS,
	TK_HYPHEN,
	TK_STAR,
	TK_SLASH,
	TK_BACKSLASH,

	TK_TILDE,
	TK_BANG,
	TK_MOD,
	TK_HAT,
	TK_AMPER,
	TK_EQUAL,
	TK_DOT,
	TK_COMMA,

	TK_LITERALSTRING,

}	CScanner_TokenKind;

Scanner *CScanner_Create (void);


#endif

