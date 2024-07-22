/****************************************************************************************/
/*  node.c                                                                              */
/*                                                                                      */
/*  Author:       Ken Baird                                                             */
/*  Description:  Bsp code for rendered view                                            */
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
#include "node.h"
#include "BASETYPE.H"
#include "VEC3D.H"
#include <assert.h>
#include "brush.h"
#include "facelist.h"
#include "face.h"
#include "RAM.H"


//#pragma todo:  uncut feature for case

//temp buffers for splitting faces
//use caution in recursive code that uses these...
static	geVec3d		spf[256], spb[256];
static	geFloat		dists[256];
static	uint8		sides[256];

enum NodeFlags
{
	NEWNODE		=1
};

typedef struct NodeTag
{
	struct NodeTag	*Front, *Back;
	Face			*Face;
	const Face		*OGFace;
	int				Flags;
} Node;


//clones the passed in face
static Node	*Node_Create(const Face *f)
{
	Node	*n;

	assert(f);

	n	=geRam_Allocate(sizeof(Node));
	if(n)
	{
		n->Face	=Face_Clone(f);
		if(n->Face)
		{
			n->Front	=NULL;
			n->Back		=NULL;
			n->OGFace	=f;
			n->Flags	=NEWNODE;
		}
		else
		{
			geRam_Free (n);
			n	=NULL;
		}
	}

	return	n;
}

static Node	*Node_Clone(Node *from)
{
	Node	*to;

	assert(from);
	assert(from->Face);

	to	=Node_Create(from->Face);

	if(to)
	{
		to->OGFace	=from->OGFace;
		to->Flags	=from->Flags;
	}
	return to;
}

static void	Node_Destroy(Node **n)
{
	assert(n);
	assert(*n);

	if((*n)->Face)
	{
		Face_Destroy(&((*n)->Face));
	}

	geRam_Free (*n);
	*n	=NULL;
}

void	Node_ClearBsp(Node *n)
{
	if(!n)
	{
		return;
	}

	Node_ClearBsp(n->Back);
	Node_ClearBsp(n->Front);

	Node_Destroy(&n);
}

void	Node_InvalidateTreeOGFaces(Node *n)
{
	if(!n)
	{
		return;
	}

	Node_InvalidateTreeOGFaces(n->Back);
	Node_InvalidateTreeOGFaces(n->Front);

	n->OGFace	=NULL;
}

static void	Node_ClearNewFlags(Node *n)
{
	if(!n)
	{
		return;
	}

	Node_ClearNewFlags(n->Back);
	Node_ClearNewFlags(n->Front);

	n->Flags	&=~NEWNODE;
}

static void	Node_AddToTree(Node *tree, Node *n)
{
	uint8		cnt[3];
	const Plane	*tp, *np;
	Face		*ff, *bf;
	Node		*bnode;

	assert(tree);
	assert(n->Face);
	assert(!n->Front && !n->Back);

	tp	=Face_GetPlane(tree->Face);
	Face_GetSplitInfo(n->Face, tp, dists, sides, cnt);

	if(!cnt[0] && !cnt[1])	//coplanar
	{
		np	=Face_GetPlane(n->Face);
		if(geVec3d_DotProduct(&tp->Normal, &np->Normal) > 0)
		{
			if(tree->Back)
			{
				Node_AddToTree(tree->Back, n);
			}
			else
			{
				tree->Back	=Node_Clone(n);
			}
		}
		else
		{
			if(tree->Front)
			{
				Node_AddToTree(tree->Front, n);
			}
			else
			{
				tree->Front	=Node_Clone(n);
			}
		}
	}
	else if(!cnt[0])	//back
	{
		if(tree->Back)
		{
			Node_AddToTree(tree->Back, n);
		}
		else
		{
			tree->Back	=Node_Clone(n);
		}
	}
	else if(!cnt[1])	//front
	{
		if(tree->Front)
		{
			Node_AddToTree(tree->Front, n);
		}
		else
		{
			tree->Front	=Node_Clone(n);
		}
	}
	else	//split
	{
		ff=bf=NULL;
		Face_Split(n->Face, tp, &ff, &bf, dists, sides);

		Face_Destroy(&n->Face);
		if(ff)
		{
			n->Face	=ff;

			if(tree->Front)
			{
				Node_AddToTree(tree->Front, n);
			}
			else
			{
				tree->Front	=Node_Clone(n);
			}
		}

		if(bf)
		{
			if(tree->Back)
			{
				bnode			=Node_Create(bf);
				bnode->Flags	=n->Flags;
				bnode->OGFace	=n->OGFace;
				Node_AddToTree(tree->Back, bnode);
				Node_Destroy(&bnode);
			}
			else
			{
				tree->Back			=Node_Create(bf);
				tree->Back->Flags	=n->Flags;
				tree->Back->OGFace	=n->OGFace;
			}
			//bf must be cleaned unlike the ff handoff
			Face_Destroy(&bf);
		}
	}
}

Node	*Node_AddBrushToTree(Node *tree, Brush *b)
{
	int		i;
	Node	*n;

	assert(b);
	assert(!Brush_IsSubtract(b));
	assert(!Brush_IsHollow(b));
	assert(!Brush_IsHollowCut(b));

	if(tree)
	{
		Node_ClearNewFlags(tree);
	}

	for(i=0;i < Brush_GetNumFaces(b);i++)
	{
		n	=Node_Create(Brush_GetFace(b, i));

		if(tree)
		{
			Node_AddToTree(tree, n);
		}
		else
		{
			tree	=Node_Clone(n);
		}

		Node_Destroy(&n);
	}
	return	tree;
}

static	Node	*pendingnodes[16000];

void	Node_EnumTreeFaces(Node *tree, geVec3d *pov, int RFlags, void *pVoid, Node_CB NodeCallBack)
{
	int		Key;
	Node	*pFarChildren, *pNearChildren, *pwall;
	Node	**pendingstackptr;
	geFloat	sgn;

	pendingnodes[0]=(Node *)NULL;
	pendingstackptr=pendingnodes+1;

	for(Key=50000, pwall=tree;;)
	{
		for(;;)
		{
			if(Face_PlaneDistance(pwall->Face, pov) > 0.0f)
			{
				pFarChildren=pwall->Front;
			}
			else
			{
				pFarChildren=pwall->Back;
			}

			if(pFarChildren==NULL)
			{
				break;
			}
			*pendingstackptr	=pwall;
			pwall				=pFarChildren;

			pendingstackptr++;
		}
		for(;;)
		{
			sgn	=Face_PlaneDistance(pwall->Face, pov);

			NodeCallBack(pwall->Face, pwall->OGFace, Key--, sgn, RFlags, pVoid);

			if(sgn > 0.0)
			{
				pNearChildren=pwall->Back;
			}
			else
			{
				pNearChildren=pwall->Front;
			}

			if(pNearChildren!=NULL)
			{
				goto WalkNearTree;
			}
			pendingstackptr--;

			pwall	=*pendingstackptr;
			if(pwall==NULL)
			{
				goto NodesDone;
			}
		}
WalkNearTree:
		pwall	=pNearChildren;
	}
NodesDone:
	pFarChildren	=pNearChildren=pwall=NULL;
	pendingnodes[0]	=(Node *)NULL;
	pendingstackptr	=pendingnodes+1;
}
