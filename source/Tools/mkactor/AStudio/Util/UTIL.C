/****************************************************************************************/
/*  UTIL.C																				*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Various useful utility functions.										*/
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
#include "util.h"
#include "ram.h"
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <io.h>


char *Util_Strdup (const char *s)
{
    char *rslt;

	assert (s != NULL);

	rslt = geRam_Allocate (strlen (s) + 1);
	if (rslt != NULL)
	{
		strcpy (rslt, s);
	}
	return rslt;
}

geBoolean Util_SetString (char **ppString, const char *NewValue)
{
	char *pNewString;

	if (NewValue == NULL)
	{
		pNewString = NULL;
	}
	else 
	{
		pNewString = Util_Strdup (NewValue);
		if (pNewString == NULL)
		{
			return GE_FALSE;
		}
	}
	if (*ppString != NULL)
	{
		geRam_Free (*ppString);
	}
	*ppString = pNewString;

	return GE_TRUE;
}


geBoolean Util_IsValidInt
    (
	  char const *Text,
	  int *TheVal
	)
{
	// no overflow checking here...
	// also no hex numbers supported yet...
	char const *c;
    int sign;
	int val;

	sign = 1;
	c = Text;
	val = 0;

	if (*c == '-')
	{
	    sign = -1;
		++c;
	}
	else if (*c == '+')
	{
	    ++c;
	}

	while (isdigit (*c))
	{
		val *= 10;
		val += (*c - '0');
	    ++c;
	}

	if (*c == '\0')
	{
	    *TheVal = sign * val;
		return GE_TRUE;
	}
	else
	{
	    return GE_FALSE;
	}
}

geBoolean Util_IsValidFloat
    (
	  const char *Text,
	  float *TheFloat
	)
{
    char const *c;
	int sign;
	float num;

	num = 0;
	sign = 1;

	c = Text;
	if (*c == '-')
	{
	    sign = -1;
		++c;
	}
	else if (*c == '+')
	{
	    ++c;
	}

	while (isdigit (*c))
	{
		num = (num * 10) + (*c - '0');
		++c;
	}

	if (*c == '.')
	{
		float div;

		div = 10.0;
		++c;
		while (isdigit (*c))
		{
		    num += ((float)(*c - '0'))/div;
			div *= 10.0;
			++c;
		}
	}

	if (*c == '\0')
	{
		*TheFloat = num;
		return GE_TRUE;
	}
	else
	{
		return GE_FALSE;
	}
}

unsigned int	Util_htoi (const char *s)
{
    unsigned int hexValue;
    const char *c;

	hexValue = 0;
	if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')))
	{
		c = &(s[2]);
		while (isxdigit (*c))
		{
			// I know there's a faster way, I just don't remember
			// what it is....
			hexValue <<= 4;
			if ((*c >= '0') && (*c <= '9'))
			{
				hexValue += (*c - '0');
			}
			else if ((*c >= 'A') && (*c <= 'F'))
			{
				hexValue += (*c - 'A') + 10;
			}
			else // a..f
			{
				hexValue += (*c - 'a') + 10;
			}
			c++;
		}
	}
    return hexValue;
}

void Util_QuoteString (const char *s, char *d)
{
	unsigned int dest, src;
	
	assert (s != d);
	dest = 0;
	d[dest++] = '"';
	for (src = 0; src < strlen (s); ++src)
	{
		char c;

		c = s[src];
		// need to escape quotes and backslashes
		if ((c == '"') || (c == '\\'))
		{
			d[dest++] = '\\';
		}
		d[dest++] = c;
	}
	d[dest++] = '"';
	d[dest] = '\0';
}



geBoolean Util_GetEndStringValue
    ( 
      const char *psz, 
      int32 *pVal, 
      int32 *pLastChar 
    )
{
    int i;
    int Num;
    int Power;
    geBoolean bValueFound = GE_FALSE ;

    assert (psz != NULL);
    assert (pVal != NULL);
    assert (pLastChar != NULL);

    Num = 0;
    Power = 1;
    i = strlen( psz );
    while( (i > 0) && (isdigit (psz[i-1])) )
    {
		char c;

		c = psz[i-1];
		Num += ((c - '0') * Power);
		Power *= 10;
		--i;
		bValueFound = GE_TRUE ;
    }
    
    if( bValueFound )
    {
		*pVal = Num ;
		*pLastChar = i ;
    }

    return bValueFound ;
}

geBoolean Util_FileExists (const char *Filename)
{
	return (_access (Filename, 0) == 0) ? GE_TRUE : GE_FALSE;
}
