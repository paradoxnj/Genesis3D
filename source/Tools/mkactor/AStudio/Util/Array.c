/****************************************************************************************/
/*  ARRAY.C																				*/
/*                                                                                      */
/*  Author: Jim Mischel		                                                            */
/*  Description: Lightweight dynamic array.												*/
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
#include "array.h"
#include <assert.h>
#include "RAM.H"

geBoolean Array_Init (Array *pArray, int InitialSize, int ItemSize)
{
	assert (pArray != NULL);
	assert (InitialSize >= 0);
	assert (ItemSize > 0);

	pArray->ItemSize = ItemSize;
	pArray->ItemsAllocated = 0;

	// if user requested initial size, 
	// then allocate memory for the items.
	if (InitialSize == 0)
	{
		pArray->Items = NULL;
	}
	else
	{
		pArray->Items = geRam_Allocate (InitialSize * ItemSize);
		if (pArray->Items == NULL)
		{
			return GE_FALSE;
		}
		pArray->ItemsAllocated = InitialSize;
	}
	return GE_TRUE;
}

void Array_Uninit (Array *pArray)
{
	assert (pArray != NULL);

	if (pArray->Items != NULL)
	{
		geRam_Free (pArray->Items);
		pArray->Items = NULL;
	}
	pArray->ItemsAllocated = 0;
}


// Create an array object with given initial size (possibly 0).
Array *Array_Create (int InitialSize, int ItemSize)
{
	Array *pArray;

	assert (InitialSize >= 0);
	assert (ItemSize > 0);

	pArray = geRam_Allocate (sizeof (Array));
	if (pArray != NULL)
	{
		if (Array_Init (pArray, InitialSize, ItemSize) == GE_FALSE)
		{
			Array_Destroy (&pArray);
		}
	}

	return pArray;
}

// Destroy an array object
void Array_Destroy (Array **ppArray)
{
	assert (ppArray != NULL);
	assert (*ppArray != NULL);

	Array_Uninit (*ppArray);
	geRam_Free (*ppArray);
	*ppArray = NULL;
}



// Resizes the array to contain NewSize elements.
// Returns new size.
int Array_Resize (Array *pArray, int NewSize)
{
	void *NewItems;

	assert (pArray != NULL);
	assert (NewSize >= 0);

	NewItems = geRam_Realloc (pArray->Items, (NewSize * pArray->ItemSize));

	// realloc returns NULL in two cases:
	//   1) It was unable to allocate memory
	//   2) The size parameter was 0.
	if ((NewItems != NULL) || (NewSize == 0))
	{
		pArray->Items = NewItems;
		pArray->ItemsAllocated = NewSize;
	}
	return pArray->ItemsAllocated;
}


//
// Following code active only in debug mode.
// Non-debug mode has all this stuff inline in the header.
#ifdef _DEBUG
	// Returns the array's allocated size.
	int Array_GetSize (const Array *pArray)
	{
		assert (pArray != NULL);

		return pArray->ItemsAllocated;
	}

	int Array_GetItemSize (const Array *pArray)
	{
		assert (pArray != NULL);

		return pArray->ItemSize;
	}


	// Returns a pointer to the item's data.
	void *Array_ItemPtr (Array *pArray, int Index)
	{
		assert (pArray != NULL);
		assert (Index >= 0);
		assert (Index < pArray->ItemsAllocated);

		return (void *)(((long)(pArray->Items)) + (Index * pArray->ItemSize));
	}

	// Copies DataSize bytes from pData to array[Index]
	void Array_PutAt (Array *pArray, int Index, void *pData, int DataSize)
	{
		void *pDest;

		assert (pArray != NULL);
		assert (pData != NULL);
		assert (Index < pArray->ItemsAllocated);
		assert (DataSize <= pArray->ItemSize);
		assert (DataSize >= 0);

		pDest = Array_ItemPtr (pArray, Index);
		memcpy (pDest, pData, DataSize);
	}

	// Inserts an item at the given position, moving all other items
	// down by 1.  This causes the last item in the array to be lost.
	void Array_InsertAt (Array *pArray, int Index, void *pData, int DataSize)
	{
		int MoveSize;
		void *pSrc;
		void *pDest;

		assert (pArray != NULL);
		assert (pData != NULL);
		assert (Index < pArray->ItemsAllocated);
		assert (DataSize <= pArray->ItemSize);
		assert (DataSize >= 0);

		// move everything down by 1
		MoveSize = (pArray->ItemsAllocated - Index - 1) * pArray->ItemSize;
		pSrc = Array_ItemPtr (pArray, Index);
		pDest = Array_ItemPtr (pArray, Index+1);
		memmove (pDest, pSrc, MoveSize);

		// and copy the new item into the spot
		memcpy (pSrc, pData, DataSize);
	}

	// Deletes the item at and moves all following items in the array up
	// to fill in the empty spot.
	void Array_DeleteAt (Array *pArray, int Index)
	{
		int MoveSize;
		void *pSrc;
		void *pDest;

		assert (pArray != NULL);
		assert (Index >= 0);
		assert (Index < pArray->ItemsAllocated);

		MoveSize = (pArray->ItemsAllocated - Index - 1) * pArray->ItemSize;
		if (MoveSize > 0)
		{
			// In debug mode, Array_ItemPtr will assert if you try to get a pointer
			// to item that's bigger than the number of items allocated.
			// This isn't a problem unless you're deleting the last allocated item
			// in the array.  Oops...
			// Anyway, if MoveSize is 0, then we're deleting the last item, so the
			// conditional handles the ugly case.
			pSrc = Array_ItemPtr (pArray, Index+1);
			pDest = Array_ItemPtr (pArray, Index);

			memcpy (pDest, pSrc, MoveSize);
		}
	}
#endif

#ifdef ARRAY_TEST_CODE

#include <stdio.h>
#include <stdlib.h>		// atoi
#include <string.h>

/*
  The test code reads array commands from an input file and applies those
  commands to arrays.  The commands are in a sort of mini scripting language.

  The commands take the form <command> <argument>, where <command> is
  a 1- or 2-letter command (see below), and the argument is a string.
  Commands are stored in the file one per line, with <command> being
  the first two characters, followed by a space, and then the argument
  which takes up the rest of the line.  Blank lines and lines that start 
  with a semicolon are considered comments and are ignored by the
  interpreter.

  For example:

  ;create a new array
  n
  ;append an item
  a Hello, world
  ;print the array from front to back
  pf
  ;destroy the array
  d

  The full list of commands is:

  c		Create a new array	<Initial size> <Item size>
  d		Destroy an array	no argument
  i		Insert an item		<item number> <data>
  p		Put an item			<item number> <data>
  r		Remove an item		<item number>
  g		Resize (grow)array	<new size>
  pn	Print #items		no argument
  pi	Print item			<item number>
  ps	Print item size		no argument
  pr	Print string		<string to print>
*/

static geBoolean ParseCommand
	(
	  char *InputLine,
	  int LineNo,
	  Array **ppArray
	)
{
	short intcmd;
	char *sarg;
	geBoolean rslt;
	char *c;
	int Index;
	int nItems, ItemSize;

	if ((InputLine[0] == '\0') || (InputLine[0] == ';'))
	{
		// it's a comment line
		return GE_TRUE;
	}
	if ((InputLine[1] == ' ') || (InputLine[1] == '\0') || (InputLine[1] == '\n'))
	{
		intcmd = InputLine[0];
	}
	else if ((InputLine[2] == ' ') || (InputLine[2] == '\0') || (InputLine[2] == '\n'))
	{
		short temp;

		intcmd = *((short *)&InputLine[0]);
		// swap the bytes
		temp = (short)(intcmd & 0xff);
		intcmd = (short)((intcmd >> 8) | (temp << 8));
	}
	else
	{
		printf ("Parse error on line %d\n", LineNo);
		return GE_FALSE;
	}

	rslt = GE_TRUE;
	switch (intcmd)
	{
		case 'c' :
		{
			// need to get size and item size from sarg
			sarg = &InputLine[2];
			c = strchr (sarg, ' ');
			if (c == NULL)
			{
				puts ("Expected <nItems> <ItemSize>");
				rslt = GE_FALSE;
				break;
			}
			*c = '\0';
			++c;
			nItems = atoi (sarg);
			ItemSize = atoi (c);
						
			if (*ppArray != NULL)
			{
				puts ("Warning:  Deleting old array.");
				Array_Destroy (ppArray);
			}
			printf ("Creating array with %d items of size %d\n", nItems, ItemSize);
			*ppArray = Array_Create (nItems, ItemSize);
			if (*ppArray == NULL)
			{
				puts ("Error creating array");
				rslt = GE_FALSE;
			}
			else
			{
				puts ("Created array");
			}
			break;
		}

		case 'd' :
			if (*ppArray == NULL)
			{
				puts ("Warning:  Trying to destroy NULL array.");
			}
			else
			{
				puts ("Destroy array");
				Array_Destroy (ppArray);
			}
			break;

		case 'i' :
			sarg = &InputLine[2];
			c = strchr (sarg, ' ');
			if (c == NULL)
			{
				puts ("Expected <ItemNo> <String>");
				rslt = GE_FALSE;
				break;
			}
			*c = '\0';
			++c;
			Index = atoi (sarg);
			printf ("Insert (%s) at item %d\n", c, Index);
			Array_InsertAt (*ppArray, Index, c, strlen (c)+1);
			break;

		case 'p' :
			sarg = &InputLine[2];
			c = strchr (sarg, ' ');
			if (c == NULL)
			{
				puts ("Expected <ItemNo> <String>");
				rslt = GE_FALSE;
				break;
			}
			*c = '\0';
			++c;
			Index = atoi (sarg);
			printf ("Put (%s) at item %d\n", c, Index);
			Array_PutAt (*ppArray, Index, c, strlen (c)+1);
			break;

		case 'r' :
			sarg = &InputLine[2];
			Index = atoi (sarg);
			c = Array_ItemPtr (*ppArray, Index);
			printf ("Remove item %d (%s)\n", Index, c);
			Array_DeleteAt (*ppArray, Index);
			break;

		case 'g' :
			sarg = &InputLine[2];
			nItems = atoi (sarg);
			printf ("Resize to %d items\n", nItems);
			Array_Resize (*ppArray, nItems);
			break;

		case 'pn' :
			nItems = Array_GetSize (*ppArray);
			printf ("Number of items = %d\n", nItems);
			break;

		case 'ps' :
			ItemSize = Array_GetSize (*ppArray);
			printf ("Item size = %d\n", ItemSize);
			break;

		case 'pr' :
			puts (&InputLine[3]);
			break;
		
		case 'pi' :
			sarg = &InputLine[2];
			Index = atoi (sarg);
			c = Array_ItemPtr (*ppArray, Index);
			printf ("Item #%d = (%s)\n", Index, c);
			break;

		default :
			printf ("Unknown command on line %d\n", LineNo);
			rslt = GE_FALSE;
			break;
	}
	if (!rslt)
	{
		printf ("Error on line %d\n", LineNo);
		printf ("Line = (%s)\n", InputLine);
	}
	return rslt;
}

int main 
	(
	  int argc,
	  char *argv[]
	)
{
	Array *pArray;
	char *InputFilename;
	FILE *infile;
	int LineNo;
	geBoolean rslt;

	puts ("Array test version 1.0");
	puts ("---------------------");
	if (argc != 2)
	{
		puts ("Usage is Array <scriptname>");
		puts ("See source for scripting information.");
		return 0;
	}


	InputFilename = argv[1];
	infile = fopen (InputFilename, "rt");
	if (infile == NULL)
	{
		printf ("Can't open input file: '%s'\n", InputFilename);
		return 0;
	}

	LineNo = 0;
	pArray = NULL;
	rslt = GE_TRUE;
	while (rslt && !feof (infile))
	{
		char InputLine[256];
		char *c;

		if (fgets (InputLine, sizeof (InputLine), infile) == NULL)
		{
			// Why is this test necessary?
			// Why doesn't feof return TRUE above?
			if (!feof (infile))
			{
				puts ("Input error");
			}
			break;
		}
		// strip newline
		c = strchr (InputLine, '\n');
		if (c != NULL)
		{
			*c = '\0';
		}
		++LineNo;
		rslt = ParseCommand (InputLine, LineNo, &pArray);
	}

	if (pArray != NULL)
	{
		Array_Destroy (&pArray);
	}

	fclose (infile);
	return 0;
}


#endif
