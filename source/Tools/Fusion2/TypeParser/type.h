/****************************************************************************************/
/*  TYPE.H                                                                              */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef	TYPE_H
#define	TYPE_H

typedef	enum
{
	T_INT,
	T_FLOAT,
	T_COLOR,
	T_POINT,
	T_STRING,
	T_MODEL,
	T_PTR,
	T_STRUCT,
	T_VOID,
	T_BOOLEAN,
}	TopType;

typedef	struct	Type			Type;
typedef struct	SymTab_Symbol	SymTab_Symbol;
typedef struct	SymTab_Scope	SymTab_Scope;

typedef	struct	TypeField
{
	SymTab_Symbol *		tfSymbol;
	unsigned short		tfFlags;
	char *				tfDefaultValue;
	char *				tfDocumentation;
	struct TypeField *	tfNext;
	struct TypeField *	tfPrev;
}	TypeField;

#define	F_TF_PUBLISHED	0x0001

typedef	struct	Type
{
	TopType			tpTopType;
	union
	{
		struct
		{
			TypeField *		tpFields;
			char *			tpIcon;
		}	s;
		struct
		{
			Type *			tpPtrType;
		}	p;
	}	t;
	Iden *			tpName;
	TypeField *		tpOriginField;
	TypeField *		tpAnglesField;
	TypeField *		tpArcField;
	TypeField *		tpRadiusField;
	SymTab_Scope *	tpScope;
	unsigned short	tpFlags;
	struct Type *	tpNext;
	struct Type *	tpPrev;
}	Type;

#define	F_TYPE_READONLY		0x0001

Type *		Type_InitTypeList(Iden_HashTable *ht);
Type *		Type_CreateType(Type **tl, Iden *name, unsigned short flags);
Type *		Type_CreatePtrType(Type **tl, Type *tp);
Type *		Type_FindTypeByName(Type *tl, const Iden *name);
void		Type_DestroyTypeList(Type *tp);
void		Type_AddTypeField(Type *tp, SymTab_Symbol *fieldName, unsigned short flags);
TypeField *	Type_GetTypeField(Type *tp, const Iden *name);

void		Type_DumpType(const Type *tp, int descend);

const char *Type_GetName(Type *tp);
const char *Type_GetFieldDocumentation(Type *tp, const Iden *FieldName);

#endif

