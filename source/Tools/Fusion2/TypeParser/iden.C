/****************************************************************************************/
/*  IDEN.C                                                                              */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Text identifier hash table implementation                              */
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
#pragma hdrstop

#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>

#include	"hash.h"
#include	"iden.h"
#include	"ram.h"

#define	HASHBUCKETS	200

typedef struct	Iden_HashTable
{
	Iden *	ihtIdentifiers[HASHBUCKETS];
}	Iden_HashTable;

Iden_HashTable *	Iden_CreateHashTable(void)
{
	Iden_HashTable *	ht;

	ht = geRam_Allocate(sizeof(*ht));
	if	(!ht)
		return 0;

	memset(ht, 0, sizeof(*ht));

	Hash_Init();

	return ht;
}

Iden *	Iden_HashName(Iden_HashTable *ht, const char *s, int len)
{
	unsigned int	idx;
	Iden *	id;

	idx = hash(s, len) % HASHBUCKETS;

	id = ht->ihtIdentifiers[idx];
	while	(id)
	{
		if	(!memcmp(id->idenSpelling, s, len + 1))
			break;
		id = id->idenNext;
	}

	if	(!id)
	{
		id = geRam_Allocate(sizeof(*id));
		if	(!id)
			return 0;
		
		memset(id, 0, sizeof(*id));

		id->idenSpelling = geRam_Allocate(len + 1);
		if	(!id->idenSpelling)
		{
			geRam_Free (id);
			return 0;
		}

		memcpy(id->idenSpelling, s, len);
		id->idenSpelling[len] = 0;
		id->idenNext = ht->ihtIdentifiers[idx];
		ht->ihtIdentifiers[idx] = id;
	}

	return id;
}

void	Iden_DestroyHashTable(Iden_HashTable *ht)
{
	int	i;

	assert (ht != NULL);

	for	(i = 0; i < HASHBUCKETS; i++)
	{
		Iden *	id;

		id = ht->ihtIdentifiers[i];
		while	(id)
		{
			Iden *	tmp;

			tmp = id;
			id = id->idenNext;
			geRam_Free(tmp->idenSpelling);
			geRam_Free(tmp);
		}
	}
	geRam_Free (ht);
}

