/****************************************************************************************/
/*  SYMTAB.C                                                                            */
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
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	"iden.h"
#include	"type.h"
#include	"symtab.h"
#include	"RAM.H"

SymTab *	SymTab_Create(void)
{
	SymTab *	st;

	st = geRam_Allocate(sizeof(*st));
	memset(st, 0, sizeof(*st));
	return st;
}

static	void	DestroySymList(SymTab_Symbol *sym)
{
	while	(sym)
	{
		SymTab_Symbol *tmp;

		tmp = sym;
		sym = sym->symScopeNext;
		geRam_Free(tmp);
	}
}

void	SymTab_Destroy(SymTab *st)
{
#ifdef	DEBUG
	memset(st, 0xef, sizeof(*st));
#endif
	geRam_Free(st);
}

SymTab_Symbol * SymTab_CreateSymbol(
	SymTab *		st,
	SymTab_Scope *	scp,
	Iden *			name,
	Type *			tp)
{
	SymTab_Symbol *sym;

	sym = geRam_Allocate(sizeof(*sym));
	if	(!sym)
		return sym;

	memset(sym, 0, sizeof(*sym));

	if	(scp->scpFlags & F_SCP_FILE)
	{
		sym->symNext = st->stSymbols;
					   st->stSymbols = sym;
	}

	sym->symName = name;
	sym->symType = tp;
	sym->symScopeNext = scp->scpSymbols;
						scp->scpSymbols = sym;

	return sym;
}

#if _MSC_VER
	#pragma warning (disable:4100)
#endif
SymTab_Scope *	SymTab_CreateScope(SymTab *st, SymTab_Scope *parent, unsigned short scpFlags)
{
	SymTab_Scope *	scp;

	scp = geRam_Allocate(sizeof(*scp));
	assert(scp);
	memset(scp, 0, sizeof(*scp));

	scp->scpFlags = scpFlags;
	scp->scpOuter = parent;
	if	(parent)
	{
		SymTab_Scope *	child;

		child = parent->scpInner;
		if	(child)
			scp->scpNext = child->scpInner;
						   child->scpInner = scp;
		parent->scpInner = scp;
	}

	return scp;
}
#if _MSC_VER
	#pragma warning (default:4100)
#endif

SymTab_Symbol *	SymTab_FindSymbolInScope(SymTab *st, SymTab_Scope *scp, Iden *name)
{
	SymTab_Symbol *	sym;

	while	(scp)
	{
		sym = scp->scpSymbols;
		while	(sym)
		{
			if	(sym->symName == name)
				return sym;
			sym = sym->symScopeNext;
		}
		scp = scp->scpOuter;
	}

	sym = st->stSymbols;
	while	(sym)
	{
		if	(sym->symName == name)
			return sym;
		sym = sym->symScopeNext;
	}

	return NULL;
}

void		SymTab_DestroyScope(SymTab_Scope *scp)
{
	assert(!scp->scpOuter);
	assert(!scp->scpInner);
	assert(!scp->scpNext);
	DestroySymList(scp->scpSymbols);
#ifdef	DEBUG
	memset(scp, 0xea, sizeof(*scp));
#endif
	geRam_Free(scp);
}

