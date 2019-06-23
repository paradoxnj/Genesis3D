/****************************************************************************************/
/*  List.c                                                                              */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  This module implements a fairly simple linked-list API that provides  */
/*                the most common list operations.  See list.h for a function listing.  */
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
#include "list.h"
#include <malloc.h>
#include <assert.h>
#include <string.h>


typedef struct tag_ListIterator ListNode;
struct tag_ListIterator
{
	ListNode *Next;		// next node in list
	ListNode *Prev;		// previous node in list
	void *Data;			// data being held by the node
};

struct tag_List
{
	ListNode *Head;		// pointer to first node in list
	ListNode *Tail;		// pointer to last node in list (makes appending much faster)
	int nItems;			// number of items in the list
};


List *List_Create
	(
	  void
	)
{
	List *pList;

	pList = (List *)malloc (sizeof (List));
	if (pList != NULL)
	{
		pList->nItems = 0;
		pList->Head = NULL;
		pList->Tail = NULL;
	}
	return pList;
}


// Destroy a list object.
// Deallocates all memory allocated to the list, and sets *ppList to NULL.
// If DestroyFcn is not NULL, the function is called for each item in the list.
void List_Destroy
	(
	  List **ppList,
	  List_DestroyCallback DestroyFcn
	)
{
	List *pList;
	ListNode *p;

	assert (ppList != NULL);
	assert (*ppList != NULL);

	pList = *ppList;
	p = pList->Head;
	while (p != NULL)
	{
		ListNode *pNext;

		pNext = p->Next;
		if (DestroyFcn != NULL)
		{
			DestroyFcn (p->Data);
		}
		free (p);
		--(pList->nItems);
		p = pNext;
	}
	assert (pList->nItems == 0);

	free (*ppList);
	*ppList = NULL;
}


void *List_GetData
	(
	  ListIterator pli
	)
{
	assert (pli != LIST_INVALID_NODE);

	return pli->Data;
}

// List_Insert is a local function that inserts a node between two
// existing nodes.  It handles the head and tail cases specially.
// All insertion functions call this one to do the actual insertion.
// On success, returns an iterator that references the new node.
// Returns NULL on failure
static ListIterator List_Insert
	(
	  List *pList,
	  ListNode *Prev,
	  ListNode *Next,
	  void *pData
	)
{
	ListNode *NewNode;

	#ifndef NDEBUG
	{
		// Verify the links so we know we're not cross-linking the list.
		if (Prev == LIST_INVALID_NODE)
		{
			assert (Next == pList->Head);
		}
		else
		{
			assert (Next == Prev->Next);
		}
		if (Next == LIST_INVALID_NODE)
		{
			assert (Prev == pList->Tail);
		}
		else
		{
			assert (Prev == Next->Prev);
		}
	}
	#endif


	// set up the new node...
	NewNode = malloc (sizeof (ListNode));
	if (NewNode != LIST_INVALID_NODE)
	{
		NewNode->Data = pData;
		NewNode->Prev = Prev;
		NewNode->Next = Next;

		// and connect everything else accordingly

		if (Next == LIST_INVALID_NODE)
		{
			// appending
			assert (Prev == pList->Tail);
			pList->Tail = NewNode;
		}

		if (Prev == LIST_INVALID_NODE)
		{
			// prepending
			assert (Next == pList->Head);
			pList->Head = NewNode;
		}


		if (Prev != LIST_INVALID_NODE)
		{
			Prev->Next = NewNode;
		}
		if (Next != LIST_INVALID_NODE)
		{
			Next->Prev = NewNode;
		}

		// maintain list node count
		++(pList->nItems);
	}
	return NewNode;
}


// Append an item to the list (add to the end)
ListIterator List_Append
	(
	  List *pList,
	  void *pData
	)
{
	assert (pList != NULL);

	return List_Insert (pList, pList->Tail, LIST_INVALID_NODE, pData);
}

// Prepend (add to the front) an item to the list
ListIterator List_Prepend
	(
	  List *pList,
	  void *pData
	)
{
	assert (pList != NULL);

	return List_Insert (pList, LIST_INVALID_NODE, pList->Head, pData);
}


// Insert an item after an existing item.
// *pli references the node after which the data should be inserted,
// and must have been returned by one of the Get functions,
ListIterator List_InsertAfter
	(
	  List *pList,
	  ListIterator pli,
	  void *pData
	)
{
	assert (pList != NULL);
	assert (pli != LIST_INVALID_NODE);

	return List_Insert (pList, pli, pli->Next, pData);
}


// Insert an item before an existing item.
// *pli references the node before which the data should be inserted,
// and must have been returned by one of the Get functions,
ListIterator List_InsertBefore
	(
	  List *pList,
	  ListIterator pli,
	  void *pData
	)
{
	// make sure that *pli is referencing a valid node
	assert (pList != NULL);
	assert (pli != LIST_INVALID_NODE);

	return List_Insert (pList, pli->Prev, pli, pData);
}

// Remove the item referenced by pli.
// If DestroyFcn is not NULL, it will be called with the address of the
// item's data.
geBoolean List_Remove
	(
	  List *pList,
	  ListIterator pli,
	  List_DestroyCallback DestroyFcn
	)
{
	// make sure we're referencing a valid node
	assert (pList != NULL);
	assert (pli != LIST_INVALID_NODE);

	// get rid of the data
	if (DestroyFcn != NULL)
	{
		DestroyFcn (pli->Data);
	}

	// Now unlink it

	if (pList->Head == pli)
	{
		// then this better be the first node
		assert (pli->Prev == LIST_INVALID_NODE);
		pList->Head = pli->Next;
	}
	else
	{
		assert (pli->Prev != LIST_INVALID_NODE);
		pli->Prev->Next = pli->Next;
	}

	if (pList->Tail == pli)
	{
		// then this better be the last node
		assert (pli->Next == LIST_INVALID_NODE);
		pList->Tail = pli->Prev;
	}
	else
	{
		assert (pli->Next != LIST_INVALID_NODE);

		pli->Next->Prev = pli->Prev;
	}

	// and deallocate the node
	free (pli);

	--(pList->nItems);

	return GE_TRUE;
}

// Return a pointer to the first node's data.
// *pli is initialized here to reference the first node.
void *List_GetFirst
	(
	  List *pList,
	  ListIterator *pli
	)
{
    assert (pList != NULL);
    assert (pli != NULL);

    *pli = pList->Head;
    if (*pli != LIST_INVALID_NODE)
    {
        return (*pli)->Data;
    }
    else
    {
        return NULL;
    }
}

// Return a pointer to the item following the node referenced by pli.
// *pli is updated to reference the new node.
#pragma warning (disable: 4100)
void *List_GetNext
	(
	  List *pList,
	  ListIterator *pli
	)
{
    assert (pList != NULL);
	// make sure that pli is referencing a valid node.
    assert (pli != NULL);
    assert (*pli != LIST_INVALID_NODE);

	*pli = (*pli)->Next;
    if (*pli == LIST_INVALID_NODE)
    {
        return LIST_INVALID_NODE;
    }
    else
    {
        return (*pli)->Data;
    }
}
#pragma warning (default: 4100)

// Return a pointer to the last node's data.
// *pli is initialized here to reference the last node.
void *List_GetLast
	(
	  List *pList,
	  ListIterator *pli
	)
{
    *pli = pList->Tail;
    if (*pli != LIST_INVALID_NODE)
    {
        return (*pli)->Data;
    }
    else
    {
        return NULL;
    }
}

// Return a pointer to the item preceding the node referenced by pli.
// *pli is updated to reference the new node.
#pragma warning (disable: 4100)
void *List_GetPrev
	(
	  List *pList,
	  ListIterator *pli
	)
{
    assert (pList != NULL);

	// make sure that pli is referencing a valid node
    assert (pli != NULL);
    assert (*pli != LIST_INVALID_NODE);

    *pli = (*pli)->Prev;
    if (*pli == LIST_INVALID_NODE)
    {
        return LIST_INVALID_NODE;
    }
    else
    {
        return (*pli)->Data;
    }
}
#pragma warning (default: 4100)

// return number of items in list.
int List_GetNumItems
	(
	  const List *pList
	)
{
    assert (pList != NULL);
	assert (pList->nItems >= 0);	// if it's < 0, then something really bad happened
    
	#ifndef NDEBUG
    {
		ListNode *pNode;
		int Count;

		Count = 0;
		pNode = pList->Head;
		while (pNode != LIST_INVALID_NODE)
		{
			++Count;
			pNode = pNode->Next;
		}
        assert (Count == pList->nItems);
    }
    #endif

    return pList->nItems;
}


// Call the CallbackFcn for each item in the list.
// CallbackFcn will be called with each list item's data, and lParam.
// lParam is a pointer to a user-defined data block.
int List_ForEach
	(
	  List *pList,
	  List_ForEachCallback CallbackFcn,
	  void *lParam
	)
{
    ListIterator li;
    void *pData;

    assert (pList != NULL);
    assert (CallbackFcn != NULL);

    pData = List_GetFirst (pList, &li);
    while (pData != NULL)
    {
        CallbackFcn (pData, lParam);
        pData = List_GetNext (pList, &li);
    }
    return 0;
}

// Search for an item in the list.
// For each item in the list, the SearchFcn is called with the item's data,
// and lParam.
// lParam is a pointer to a user-defined data block.
// Returns GE_TRUE if the search is successful.  If GE_TRUE is returned,
// a pointer to the found item's data is returned in *ppData, and
// *pli is initialized to reference the found item.
// Returns GE_FALSE if the search is not successful.
geBoolean List_Search
	(
	  List *pList,
	  List_SearchCallback SearchFcn,
	  void *lParam,
	  void **ppData,
	  ListIterator *pli
	)
{
    void *pData;

    assert (pList != NULL);
    assert (SearchFcn != NULL);

    pData = List_GetFirst (pList, pli);
    while (pData != NULL)
    {
        if (SearchFcn (pData, lParam) != GE_FALSE)
        {
			*ppData = pData;
			return GE_TRUE;
        }
        pData = List_GetNext (pList, pli);
    }
    return GE_FALSE;
}



#ifdef LIST_TEST_CODE
#include <stdio.h>

#pragma warning (disable: 4100)
static void StringPrint (void *item, void *lParam)
{
	char *s;

	s = (char *)item;

	printf ("%s\n", s);
}
#pragma warning (default: 4100)

static geBoolean StringSearchFcn (void *p1, void *lParam)
{
	char *i1 = (char *)p1;
	char *i2 = (char *)lParam;

	return (strcmp (i1, i2) == 0) ? GE_TRUE : GE_FALSE;
}

static void StringDestroyFcn (void *p1)
{
	assert (p1 != NULL);
	free (p1);
}

/*
  The test code reads list commands from an input file and applies those
  commands to lists.  The commands are in a sort of mini scripting language.

  The commands take the form <command> <argument>, where <command> is
  a 1- or 2-letter command (see below), and the argument is a string.
  Commands are stored in the file one per line, with <command> being
  the first two characters, followed by a space, and then the argument
  which takes up the rest of the line.  Blank lines and lines that start 
  with a semicolon are considered comments and are ignored by the
  interpreter.

  For example:

  ;create a new list
  n
  ;append an item
  a Hello, world
  ;print the list from front to back
  pf
  ;destroy the list
  d

  The full list of commands is:

  c		Create a new list	no argument
  d		Destroy a list		no argument
  a		Append an item		argument is the item to append
  p		Prepend an item		argument is the item to prepend
  s		Search for an item	argument is the item to search for
  ib	Insert before		argument is the item to insert
  ia	Insert after		argument is the item to insert
  gf	Get First			no argument
  gn	Get Next			no argument
  gl	Get Last			no argument
  gp	Get Previous		no argument
  r		Remove				no argument
  pc	Print current		no argument
  pn	Print # items		no argument
  pl	Print list			no argument
  pb	Print list reversed no argument
  pr	Print				string to output

  The script interpreter maintains the concept of a "current item,"
  which is initialized by Search, GetFirst, or GetLast, and is
  updated by GetNext and GetPrevious.  This current item is an
  implied argument in Insert before, Insert after, GetNext, GetPrevious,
  and Remove.
*/

static geBoolean ParseCommand
	(
	  char const *InputLine,
	  int LineNo,
	  List **ppList,
	  ListIterator *pCurItem,
	  char **ppCurData
	)
{
	short intcmd;
	char *sarg;
	geBoolean rslt;

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
			if (*ppList != NULL)
			{
				puts ("Warning:  Deleting old list.");
				List_Destroy (ppList, StringDestroyFcn);
			}
			*ppList = List_Create ();
			if (*ppList == NULL)
			{
				puts ("Error creating list");
				rslt = GE_FALSE;
			}
			else
			{
				puts ("Created new list");
			}
			break;

		case 'd' :
			if (*ppList == NULL)
			{
				puts ("Warning:  Trying to destroy NULL list.");
			}
			else
			{
				puts ("Destroy list");
				List_Destroy (ppList, StringDestroyFcn);
			}
			break;

		case 'a' :
			sarg = strdup (&InputLine[2]);
			if (sarg == NULL)
			{
				puts ("Error: no argument for append");
				rslt = GE_FALSE;
			}
			else
			{
				if (List_Append (*ppList, sarg))
				{
					printf ("Append (%s)\n", sarg);
				}
				else
				{
					puts ("Error in append");
					rslt = GE_FALSE;
				}
			}
			break;

		case 'p' :
			sarg = strdup (&InputLine[2]);
			if (sarg == NULL)
			{
				puts ("Error: no argument for prepend");
				rslt = GE_FALSE;
			}
			else
			{
				if (List_Prepend (*ppList, sarg))
				{
					printf ("Prepend (%s)\n", sarg);
				}
				else
				{
					puts ("Error in Prepend");
					rslt = GE_FALSE;
				}
			}
			break;

		case 's' :
			sarg = strdup (&InputLine[2]);
			if (sarg == NULL)
			{
				puts ("Error:  no argument for search");
				rslt = GE_FALSE;
			}
			else
			{
				geBoolean SrchRslt;
				char *pData;

				SrchRslt = List_Search (*ppList, StringSearchFcn, sarg, (void **)&pData, pCurItem);
				if (SrchRslt)
				{
					printf ("Search:  Found (%s).  Data = (%s)\n", sarg, pData);
				}
				else
				{
					printf ("Search:  Unable to find (%s)\n", sarg);
				}
			}
			break;

		case 'ib' :
			sarg = strdup (&InputLine[3]);
			if (sarg == NULL)
			{
				puts ("Error:  no argument for insert before.");
				rslt = GE_FALSE;
			}
			else
			{
				if (List_InsertBefore (*ppList, *pCurItem, sarg) != LIST_INVALID_NODE)
				{
					printf ("Insert (%s) before (%s).\n", sarg, (*pCurItem)->Data);
				}
				else
				{
					printf ("InsertBefore:  error inserting (%s)\n", sarg);
					rslt = GE_FALSE;
				}
			}
			break;

		case 'ia' :
			sarg = strdup (&InputLine[3]);
			if (sarg == NULL)
			{
				puts ("Error:  no argument for insert after.");
				rslt = GE_FALSE;
			}
			else
			{
				if (List_InsertAfter (*ppList, *pCurItem, sarg))
				{
					printf ("Insert (%s) after (%s).\n", sarg, (*pCurItem)->Data);
				}
				else
				{
					printf ("InsertAfter:  error inserting (%s)\n", sarg);
					rslt = GE_FALSE;
				}
			}
			break;

		case 'gf' :
			puts ("Get First");
			*ppCurData = List_GetFirst (*ppList, pCurItem);
			break;

		case 'gn' :
			puts ("Get Next");
			*ppCurData = List_GetNext (*ppList, pCurItem);
			break;

		case 'gl' :
			puts ("Get Last");
			*ppCurData = List_GetLast (*ppList, pCurItem);
			break;

		case 'gp' :
			puts ("Get Previous");
			*ppCurData = List_GetPrev (*ppList, pCurItem);
			break;

		case 'r' :
			puts ("Remove");
			List_Remove (*ppList, *pCurItem, StringDestroyFcn);
			break;

		case 'pc' :
			if (*ppCurData == NULL)
			{
				puts ("No current item to print\n");
			}
			else
			{
				printf ("Print Current = (%s)\n", *ppCurData);
			}
			break;

		case 'pn' :
			printf ("NumItems = %d\n", List_GetNumItems (*ppList));
			break;

		case 'pl' :
			puts ("Print list");
			printf ("NumItems = %d\n", List_GetNumItems (*ppList));
			puts ("----------");
			List_ForEach (*ppList, StringPrint, NULL);
			puts ("----------");
			break;

		case 'pb' :
		{
			char *MyData;
			ListIterator li;

			puts ("Print list backwards");
			printf ("NumItems = %d\n", List_GetNumItems (*ppList));
			puts ("--------------------");
			MyData = List_GetLast (*ppList, &li);
			while (MyData != NULL)
			{
				printf ("%s\n", MyData);
				MyData = List_GetPrev (*ppList, &li);
			}
			puts ("--------------------");
			break;
		}
		case 'pr' :
			puts (&InputLine[3]);
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
	List *pList;
	ListIterator CurItem;
	char *pCurrentData;
	char *InputFilename;
	FILE *infile;
	int LineNo;
	geBoolean rslt;

	puts ("List test version 1.0");
	puts ("---------------------");
	if (argc != 2)
	{
		puts ("Usage is List <scriptname>");
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
	pList = NULL;
	CurItem = LIST_INVALID_NODE;
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
		rslt = ParseCommand (InputLine, LineNo, &pList, &CurItem, &pCurrentData);
	}

	if (pList != NULL)
	{
		List_Destroy (&pList, StringDestroyFcn);
	}

	fclose (infile);
	return 0;
}


#endif
