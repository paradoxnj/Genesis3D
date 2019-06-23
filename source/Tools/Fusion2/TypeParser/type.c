/****************************************************************************************/
/*  TYPE.C                                                                              */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: C Language typedef parsing support                                     */
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

#include	"iden.h"
#include	"type.h"
#include	"symtab.h"
#include	"ram.h"

Type *	Type_CreatePtrType(Type **tl, Type *tp)
{
	Type *	ptrTp;

	ptrTp = geRam_Allocate(sizeof(*ptrTp));

	if	(!ptrTp)
		return ptrTp;

	memset(ptrTp, 0, sizeof(*ptrTp));

	ptrTp->tpTopType	 = T_PTR;
	ptrTp->t.p.tpPtrType = tp;
	ptrTp->tpNext		 = *tl;
						 *tl = ptrTp;

	return ptrTp;
}

Type *	Type_CreateType(Type **tl, Iden *name, unsigned short flags)
{
	Type *	tp;

	tp = geRam_Allocate(sizeof(*tp));

	memset(tp, 0, sizeof(*tp));

	if	(!tp)
		return tp;

	tp->tpName = name;
	
	tp->tpTopType = T_STRUCT;
	tp->tpFlags   = flags;

	tp->tpNext = *tl;
				 *tl = tp;

	return tp;
}

static	void	Type_DestroyType(Type *tp)
{
	TypeField *	tf;

	switch	(tp->tpTopType)
	{
	case	T_STRUCT:

		tf = tp->t.s.tpFields;
		while	(tf)
		{
			TypeField *	tmp;
	
			tmp = tf->tfNext;
			if (tf->tfDefaultValue != NULL)
			{
				geRam_Free (tf->tfDefaultValue);
			}
			if (tf->tfDocumentation != NULL)
			{
				geRam_Free (tf->tfDocumentation);
			}
			geRam_Free (tf);
			tf = tmp;
		}
		if	(tp->t.s.tpIcon)
			geRam_Free(tp->t.s.tpIcon);
		break;

	case	T_PTR:
		break;

	default:
//		assert(!"Shouldn't be destroying this type");
		break;
	}

	geRam_Free(tp);
}

void	Type_DestroyTypeList(Type *tp)
{
	while	(tp)
	{
		Type *	tmp;

		tmp = tp;
		tp = tp->tpNext;
		Type_DestroyType(tmp);
	}
}

Type *	Type_FindTypeByName(Type *tp, const Iden *name)
{
	while	(tp)
	{
		if	(tp->tpName == name)
			return tp;
		tp = tp->tpNext;
	}

	return NULL;
}

Type *	Type_InitTypeList(Iden_HashTable *ht)
{
	Type *	tp;
	Type *	tl;

	tl = NULL;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "int", 3), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_INT;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "geVec3d", 7), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_POINT;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "float", 5), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_FLOAT;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "geFloat", 7), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_FLOAT;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "GE_RGBA", 7), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_COLOR;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "<string>", 8), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_STRING;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "char", 4), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_POINT;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "<model>", 7), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_MODEL;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "void", 4), F_TYPE_READONLY);
	if	(!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_VOID;

	tp = Type_CreateType(&tl, Iden_HashName(ht, "geBoolean", 9), F_TYPE_READONLY);
	if (!tp)
	{
		Type_DestroyTypeList(tl);
		return NULL;
	}
	tp->tpTopType = T_BOOLEAN;

	return tl;
}

void		Type_AddTypeField(Type *tp, SymTab_Symbol *fieldName, unsigned short flags)
{
	TypeField *		tf;
	TypeField *		newField;
	SymTab_Symbol *	before;

	before = 0;

	assert(tp->tpTopType == T_STRUCT);

	newField = (TypeField *)geRam_Allocate(sizeof(*newField));
	if	(!newField)
	{
#pragma message("Type_AddTypeField can't propagate memory allocation errors")
		assert(0);
		return;
	}
	memset(newField, 0, sizeof(*newField));
	newField->tfSymbol = fieldName;
	newField->tfFlags  = flags;

	tf = tp->t.s.tpFields;
	while	(tf)
	{
		if	(tf->tfSymbol == before)
		{
			if	(tf->tfPrev)
				tf->tfPrev->tfNext = newField;
			newField->tfPrev = tf->tfPrev;
							   tf->tfPrev = newField;
			newField->tfNext = tf;
			if	(tf == tp->t.s.tpFields)
				tp->t.s.tpFields = newField;
			return;
		}

		if	(!tf->tfNext)
		{
			tf->tfNext = newField;
			newField->tfPrev = tf;
			return;
		}

		tf = tf->tfNext;
	}

	assert(!tp->t.s.tpFields);
	tp->t.s.tpFields = newField;
	newField->tfNext =
	newField->tfPrev = NULL;
}

TypeField *	Type_GetTypeField(Type *tp, const Iden *fieldName)
{
	TypeField *	tf;

	tf = tp->t.s.tpFields;

	while	(tf)
	{
		if	(tf->tfSymbol->symName == fieldName)
			return tf;
		tf = tf->tfNext;
	}

	return tf;
}

#ifdef	ELIDEBUG
static	void	dumpStruct(const Type *tp, int descend)
{
	TypeField *	tf;

	assert(tp->tpTopType == T_STRUCT);

	printf("%s ", tp->tpName->idenSpelling);

	tf = tp->t.s.tpFields;
	printf(" { ");
	while	(tf)
	{
		Type_DumpType(tf->tfSymbol->symType, descend);
		printf(" %s", tf->tfSymbol->symName->idenSpelling);
		if	(tf->tfFlags & F_TF_PUBLISHED)
			printf("(p)");
		tf = tf->tfNext;
		if	(tf)
			printf(", ");
	}
	printf(" } ");
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
	"void",
};

void	Type_DumpType(const Type *tp, int descend)
{
	switch	(tp->tpTopType)
	{
	case	T_STRUCT:
		if	(descend)
			dumpStruct(tp, descend);
		else
			printf("%s", tp->tpName->idenSpelling);
		break;
	case	T_PTR:
		printf("ptr to ");
		Type_DumpType(tp->t.p.tpPtrType, 0);
		break;
	default:
		printf("%s", topTypeNames[tp->tpTopType]);
		break;
	}
}
#endif

const char *Type_GetName(Type *tp)
{
	assert(tp->tpTopType == T_STRUCT);
	return tp->tpName->idenSpelling;
}

const char *Type_GetFieldDocumentation(Type *tp, const Iden *FieldName)
{
	TypeField *	tf;

	tf = Type_GetTypeField(tp, FieldName);
	if	(tf != NULL)
		return tf->tfDocumentation;

	return NULL;
}

