/****************************************************************************************/
/*  CPARSER.H                                                                           */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Interface for partial parser for C code.                               */
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
#ifndef	CPARSER_H
#define	CPARSER_H

#ifdef	__cplusplus
extern "C" {
#endif

#include	"iden.h"
#include	"type.h"

typedef	struct	CParser	CParser;

typedef	struct	StructIter
{
	Type *	siType;
}	StructIter;

typedef struct	FieldIter
{
	TypeField *	fiCurrentField;
}	FieldIter;

/*
	Error function gets a filename, line number and error string.
*/
typedef	void	(*CParser_ErrFunc)(const char *, int, const char *);

CParser *	CParser_Init(CParser_ErrFunc errFunc);
void		CParser_Destroy (CParser *p);
void		CParser_ParseFile(CParser *p, const char *file);
void		CParser_ParseMemoryBlock (CParser *p, const char *Block, int length, const char *Name);

const char *CParser_GetTypeName(const Type *tp);
Type *		CParser_GetFirstStruct(StructIter *iter, CParser *p);
Type *		CParser_GetNextStruct(StructIter *iter);
int			CParser_GetFirstField(FieldIter *fi,
								  Type *tp,
								  TopType *tt,
								  const char **tpName,
								  const char **fieldName,
								  int *published,
								  const char **defaultValue);
int			CParser_GetNextField(FieldIter *fi,
								 TopType *tt,
								 const char **tpName,
								 const char **fieldName,
								 int *published,
								 const char **defaultValue);

int	CParser_WriteTypesToMap(CParser *p, FILE *fp);
const char *	CParser_GetOriginFieldName(Type *tp);
const char *	CParser_GetRadiusFieldName(Type *tp);
const char *	CParser_GetAnglesFieldName(Type *tp);
const char *	CParser_GetArcFieldName(Type *tp);
const char *	CParser_GetTypeFieldDocumentation(CParser *p, Type *tp, const char *FieldName);
int				CParser_GetContentsCount(const CParser *p);
void			CParser_GetContentsNameAndValue(const CParser *p, int idx, const char **Name, unsigned long *Value);
const char *	CParser_GetIconName (Type *pType);

#ifdef	__cplusplus
}
#endif
#endif

