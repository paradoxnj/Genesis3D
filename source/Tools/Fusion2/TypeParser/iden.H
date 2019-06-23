/****************************************************************************************/
/*  IDEN.H                                                                              */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Text identifier hash table interface                                   */
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
#ifndef IDEN_H
#define	IDEN_H

#ifdef __cplusplus
    extern "C" {
#endif


typedef struct	Iden_HashTable Iden_HashTable;

typedef struct	Iden
{
	char *			idenSpelling;
	int				idenKeyword;
	struct	Iden *	idenNext;
}	Iden;

Iden_HashTable *	Iden_CreateHashTable(void);
Iden *		Iden_HashName(Iden_HashTable *ht, const char *s, int len);
void	Iden_DestroyHashTable(Iden_HashTable *ht);

#ifdef __cplusplus
    }
#endif

#endif

