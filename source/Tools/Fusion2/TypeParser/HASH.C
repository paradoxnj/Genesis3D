/****************************************************************************************/
/*  HASH.C                                                                              */
/*                                                                                      */
/*  Author: Eli Boling                                                                  */
/*  Description: Simple character hashing function                                      */
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
#include	<limits.h>

#include	"hash.h"

#ifndef __BORLANDC__
// 02/12/98 - jim - MSVC doesn't have a random() function
#define random(num)(int)(((long)rand()*(num))/(RAND_MAX+1))
#endif

static	int	hashValues[256];
static	int	Initialized = 0;

void			Hash_Init(void)
{
	int	i;

	if	(Initialized)
		return;

	Initialized = 1;

	for	(i = 0; i < 256; i++)
		hashValues[i] = random(INT_MAX);
}

unsigned int	hash(const char *s, int count)
{
	int	value;

	value = 0;
	while	(count--)
	{
		value += hashValues[*s];
		s++;
	}

	return (unsigned int)value;
}

