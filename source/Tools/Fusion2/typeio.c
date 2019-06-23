/****************************************************************************************/
/*  typeio.c                                                                            */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  reading and writing standard types                                    */
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
#include "typeio.h"
#include <string.h>
#include <assert.h>

geBoolean TypeIO_WriteBlock
	(
	  FILE *f,
	  void const *b,
	  int cb
	)
{
	int rslt;

	assert (f != NULL);
	assert (b != NULL);
	assert (cb > 0);

	rslt = fwrite (b, 1, cb, f);
	return (rslt == cb);
}

geBoolean TypeIO_WriteInt
	(
	  FILE *f,
	  int i
	)
{
	return TypeIO_WriteBlock (f, &i, sizeof (i));
}

geBoolean TypeIO_WriteIntText
	(
	  FILE *f,
	  int i
	)
{
	assert (f != NULL);
	return (fprintf (f, "%i ", i) != 0);
}

geBoolean TypeIO_WriteUint
    (
	  FILE *f,
	  unsigned int i
	)
{
	return TypeIO_WriteBlock (f, &i, sizeof (i));
}

geBoolean TypeIO_WriteUintText
    (
	  FILE *f,
	  unsigned int i
	)
{
	assert (f != NULL);

	return (fprintf (f, "%u ", i) != 0);
}


geBoolean TypeIO_WriteFloat
	(
	  FILE *f,
	  float n
	)
{
	return TypeIO_WriteBlock (f, &n, sizeof (n));
}

geBoolean TypeIO_WriteFloatText
	(
	  FILE *f,
	  float n
	)
{
	assert (f != NULL);

	return (fprintf (f, "%f ", n) != 0);
}

geBoolean TypeIO_WriteString
    (
	  FILE *f,
	  char const *s
	)
// strings are written in two parts:  a length and then the data.
{
	int length;

	// a NULL will result in a 0-length string
	if (s == NULL)
	{
		return TypeIO_WriteInt (f, 0);
	}

	length = strlen (s);

	// write the length first
	if (TypeIO_WriteInt (f, length))
	{
		if (length > 0)
		{
			// write string data
			return TypeIO_WriteBlock (f, s, length);
		}
		return GE_TRUE;  // 0-length string
	}
	return GE_FALSE;  // couldn't write length
}


geBoolean TypeIO_WriteStringText
    (
	  FILE *f,
	  char const *s
	)
{
	assert (f != NULL);
	assert (s != NULL);
	assert (*s != '\0');	// 'cause I don't know how to handle this yet...

	return (fprintf (f, "%s ", s) != 0);
}

geBoolean TypeIO_WriteVec3d
	(
	  FILE *f,
	  geVec3d const *v
	)
{
	assert (f != NULL);
	assert (v != NULL);

	return (TypeIO_WriteFloat (f, v->X) &&
			TypeIO_WriteFloat (f, v->Y) &&
			TypeIO_WriteFloat (f, v->Z));
}

geBoolean TypeIO_WriteVec3dText
	(
	  FILE *f,
	  geVec3d const *v
	)
{
	geBoolean rslt;

	assert (f != NULL);
	assert (v != NULL);

	rslt = (TypeIO_WriteFloatText (f, v->X) &&
			TypeIO_WriteFloatText (f, v->Y) &&
			TypeIO_WriteFloatText (f, v->Z));
	if (rslt)
	{
		rslt = (fprintf (f, "\n") != 0);
	}
	return rslt;
}

geBoolean TypeIO_WriteXForm3d
	(
	  FILE *f,
	  geXForm3d const *pXfm
	)
{
	assert (f != NULL);
	assert (pXfm != NULL);

	return
	(
		TypeIO_WriteFloat (f, pXfm->AX) &&
		TypeIO_WriteFloat (f, pXfm->AY) &&
		TypeIO_WriteFloat (f, pXfm->AZ) &&
		TypeIO_WriteFloat (f, pXfm->BX) &&
		TypeIO_WriteFloat (f, pXfm->BY) &&
		TypeIO_WriteFloat (f, pXfm->BZ) &&
		TypeIO_WriteFloat (f, pXfm->CX) &&
		TypeIO_WriteFloat (f, pXfm->CY) &&
		TypeIO_WriteFloat (f, pXfm->CZ) &&
		TypeIO_WriteVec3d (f, &pXfm->Translation)
	);

}

geBoolean TypeIO_WriteXForm3dText
	(
	  FILE *f,
	  geXForm3d const *pXfm
	)
{
	assert (f != NULL);
	assert (pXfm != NULL);

	return
	(
		TypeIO_WriteFloatText (f, pXfm->AX) &&
		TypeIO_WriteFloatText (f, pXfm->AY) &&
		TypeIO_WriteFloatText (f, pXfm->AZ) &&
		TypeIO_WriteFloatText (f, pXfm->BX) &&
		TypeIO_WriteFloatText (f, pXfm->BY) &&
		TypeIO_WriteFloatText (f, pXfm->BZ) &&
		TypeIO_WriteFloatText (f, pXfm->CX) &&
		TypeIO_WriteFloatText (f, pXfm->CY) &&
		TypeIO_WriteFloatText (f, pXfm->CZ) &&
		TypeIO_WriteVec3dText (f, &pXfm->Translation)
	);
}

// *********************************************************
// ********************* INPUT ROUTINES ********************
// *********************************************************

geBoolean TypeIO_ReadBlock
	(
	  FILE *f,
	  void *b,
	  int BytesToRead
	)
{
	int rslt;

	assert (f != NULL);
	assert (b != NULL);
	assert (BytesToRead > 0);

	rslt = fread (b, 1, BytesToRead, f);
	return (rslt == BytesToRead);
}

geBoolean TypeIO_ReadInt
	(
	  FILE *f,
	  int * pi
	)
{
	assert (f != NULL);
	assert (pi != NULL);

	return TypeIO_ReadBlock (f, pi, sizeof (*pi));
}

geBoolean TypeIO_ReadIntText
	(
	  FILE *f,
	  int * pi
	)
{
	assert (f != NULL);
	assert (pi != NULL);

	return (fscanf (f, "%i", pi) == 1);
}

geBoolean TypeIO_ReadUint
    (
	  FILE *f,
	  unsigned int * pu
	)
{
	assert (f != NULL);
	assert (pu != NULL);

	return TypeIO_ReadBlock (f, pu, sizeof (*pu));
}

geBoolean TypeIO_ReadUintText
    (
	  FILE *f,
	  unsigned int * pu
	)
{
	assert (f != NULL);
	assert (pu != NULL);

	return (fscanf (f, "%u", pu) == 1);
}


geBoolean TypeIO_ReadFloat
	(
	  FILE *f,
	  float * pf
	)
{
	assert (f != NULL);
	assert (pf != NULL);

	return TypeIO_ReadBlock (f, pf, sizeof (*pf));
}

geBoolean TypeIO_ReadFloatText
	(
	  FILE *f,
	  float * pf
	)
{
	assert (f != NULL);
	assert (pf != NULL);

	return (fscanf (f, "%f", pf) == 1);
}


geBoolean TypeIO_ReadString
    (
	  FILE *f,
	  char *s,
	  int cb,
	  int * pRequired
	)
{
	int length;
	int CurPos;

	assert (f != NULL);
	assert (s != NULL);
	assert (cb > 0);
	assert (pRequired != NULL);

	// save position 'cause we may need to move back...
	CurPos = ftell (f);
	// get string length (doesn't include nul terminator)
	if (!TypeIO_ReadInt (f, &length))
	{
		*pRequired = 0;
		return GE_FALSE;
	}
	
	if (length < 0)
	{
		// negative length is an error
		*pRequired = length;
		return GE_FALSE;
	}

	*pRequired = length + 1;
	if (length >= cb)
	{
		// passed buffer isn't long enough
		// position back to length word...
		fseek (f, CurPos, SEEK_SET);
		return GE_FALSE;
	}

	// buffer's big enough, read data into it.
	if (length > 0)
	{
		if (!TypeIO_ReadBlock (f, s, length))
		{
			*pRequired = 0;				// read error...
			return GE_FALSE;
		}
	}
	// add nul terminator
	s[length] = '\0';
	return GE_TRUE;
}

geBoolean TypeIO_ReadStringText
    (
	  FILE *f,
	  char *s,
	  int cb,
	  int *pRequired
	)
{
	geBoolean rslt;
	char BigBuffer[32768];  // too cautious here??

	assert (f != NULL);
	assert (s != NULL);
	assert (pRequired != NULL);
	assert (cb > 0);

	rslt = GE_FALSE;

	if (fscanf (f, "%s", BigBuffer) == 1)
	{
		strncpy (s, BigBuffer, cb-1);
		s[cb-1] = '\0';

		if (strlen (BigBuffer) < (unsigned)cb)
		{
			rslt = GE_TRUE;
			*pRequired = strlen (BigBuffer);
		}
		else
		{
			*pRequired = 1 + strlen (BigBuffer) - cb;
		}
	}
	else
	{
		*pRequired = 0;
	}
	return rslt;
}


geBoolean TypeIO_ReadVec3d
	(
	  FILE *f,
	  geVec3d *pv
	)
{
	assert (f != NULL);
	assert (pv != NULL);

	return (TypeIO_ReadFloat (f, &(pv->X)) &&
			TypeIO_ReadFloat (f, &(pv->Y)) &&
			TypeIO_ReadFloat (f, &(pv->Z)));
}

geBoolean TypeIO_ReadVec3dText
	(
	  FILE *f,
	  geVec3d *pv
	)
{
	geBoolean rslt;

	assert (f != NULL);
	assert (pv != NULL);

	rslt = (TypeIO_ReadFloatText (f, &(pv->X)) &&
			TypeIO_ReadFloatText (f, &(pv->Y)) &&
			TypeIO_ReadFloatText (f, &(pv->Z)));
	if (rslt)
	{
		fscanf (f, "\n");
	}
	return rslt;
}


geBoolean TypeIO_ReadXForm3d
	(
	  FILE *f,
	  geXForm3d *pXfm
	)
{
	assert (f != NULL);
	assert (pXfm != NULL);

	return
	(
		TypeIO_ReadFloat (f, &pXfm->AX) &&
		TypeIO_ReadFloat (f, &pXfm->AY) &&
		TypeIO_ReadFloat (f, &pXfm->AZ) &&
		TypeIO_ReadFloat (f, &pXfm->BX) &&
		TypeIO_ReadFloat (f, &pXfm->BY) &&
		TypeIO_ReadFloat (f, &pXfm->BZ) &&
		TypeIO_ReadFloat (f, &pXfm->CX) &&
		TypeIO_ReadFloat (f, &pXfm->CY) &&
		TypeIO_ReadFloat (f, &pXfm->CZ) &&
		TypeIO_ReadVec3d (f, &pXfm->Translation)
	);
}


geBoolean TypeIO_ReadXForm3dText
	(
	  FILE *f,
	  geXForm3d *pXfm
	)
{
	assert (f != NULL);
	assert (pXfm != NULL);

	return
	(
		TypeIO_ReadFloatText (f, &pXfm->AX) &&
		TypeIO_ReadFloatText (f, &pXfm->AY) &&
		TypeIO_ReadFloatText (f, &pXfm->AZ) &&
		TypeIO_ReadFloatText (f, &pXfm->BX) &&
		TypeIO_ReadFloatText (f, &pXfm->BY) &&
		TypeIO_ReadFloatText (f, &pXfm->BZ) &&
		TypeIO_ReadFloatText (f, &pXfm->CX) &&
		TypeIO_ReadFloatText (f, &pXfm->CY) &&
		TypeIO_ReadFloatText (f, &pXfm->CZ) &&
		TypeIO_ReadVec3dText (f, &pXfm->Translation)
	);
}
