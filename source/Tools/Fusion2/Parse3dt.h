/****************************************************************************************/
/*  Parse3dt.h                                                                          */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Genesis world editor header file                                      */
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
#ifndef PARSE3DT_H
#define PARSE3DT_H

#include "scanner.h"
#include "VEC3D.H"
#include "XFORM3D.H"

#ifdef __cplusplus
	extern "C" {
#endif

typedef struct tag_Parse3dt Parse3dt;

typedef enum
{
	P3TK_ERROR = 0,
	P3TK_IDEN,
	P3TK_LITERALSTRING,
	P3TK_ICON,
	P3TK_FCON,
	P3TK_PATH,
	P3TK_EOF
} Parse3dt_TokenKind;


struct tag_Parse3dt
{
	Scanner *Scanner;
	Iden_HashTable *HashTable;
};

Parse3dt *Parse3dt_Create (const char *Filename);
void Parse3dt_Destroy (Parse3dt **pParser);
char const *Parse3dt_Error (Parse3dt *Parser, char *fmt, ...);

geBoolean Parse3dt_ScanExpecting (Parse3dt *Parser, Scanner_Token *t, Parse3dt_TokenKind kind);
geBoolean Parse3dt_ScanExpectingText (Parse3dt *Parser, char const *idText);

geBoolean Parse3dt_GetIdentifier (Parse3dt *Parser, char const *Tag, char *idText);
geBoolean Parse3dt_GetPath (Parse3dt *Parser, char const *Tag, char *Path);
geBoolean Parse3dt_GetInt (Parse3dt *Parser, char const *Tag, int *Value);
geBoolean Parse3dt_GetFloat (Parse3dt *Parser, char const *Tag, float *Value);
geBoolean Parse3dt_GetLiteral (Parse3dt *Parser, char const *Tag, char *idText);
geBoolean Parse3dt_GetVec3d (Parse3dt *Parser, char const *Tag, geVec3d *pVec);
geBoolean Parse3dt_GetXForm3d (Parse3dt *Parser, char const *Tag, geXForm3d *pXfm);

geBoolean Parse3dt_GetVersion (Parse3dt *Parser, int *VersionMajor, int *VersionMinor);

#ifdef __cplusplus
	}
#endif


#endif
