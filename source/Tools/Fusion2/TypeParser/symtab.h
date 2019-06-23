/****************************************************************************************/
/*  SYMTAB.H                                                                            */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Symbol table for C Language parser support                             */
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
#ifndef	SYMTAB_H
#define	SYMTAB_H

#ifdef __cplusplus
    extern "C" {
#endif

typedef struct	SymTab_Symbol
{
	Iden *					symName;
	Type *					symType;
	struct SymTab_Symbol *	symNext;
	struct SymTab_Symbol *	symScopeNext;
}	SymTab_Symbol;

typedef	struct	SymTab_Scope
{
	struct SymTab_Scope *	scpOuter;
	struct SymTab_Scope *	scpInner;
	struct SymTab_Scope *	scpNext;
	unsigned short			scpFlags;
	SymTab_Symbol *			scpSymbols;
}	SymTab_Scope;

#define	F_SCP_TAG	0x0001
#define	F_SCP_FILE	0x0002

typedef	struct	SymTab
{
	SymTab_Symbol *			stSymbols;
}	SymTab;

SymTab *	SymTab_Create(void);

void		SymTab_Destroy(SymTab *st);

SymTab_Symbol * SymTab_CreateSymbol(
	SymTab *		st,
	SymTab_Scope *	scp,
	Iden *			name,
	Type *			tp);

SymTab_Scope *	SymTab_CreateScope(
	SymTab *		st,
	SymTab_Scope *	parent,
	unsigned short	scpFlags);

SymTab_Symbol *	SymTab_FindSymbolInScope(SymTab *st, SymTab_Scope *scp, Iden *name);

void SymTab_DestroyScope(SymTab_Scope *scp);

#ifdef __cplusplus
    }
#endif


#endif

