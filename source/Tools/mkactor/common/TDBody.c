/****************************************************************************************/
/*  TDBODY.C																			*/
/*                                                                                      */
/*  Author: Stephen Balkum	                                                            */
/*  Description: Build-time internal body hierarchy format.								*/
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
#include <assert.h>

#include "tdbody.h"

#include "ram.h"

static void TopDownBody_InitNode(TopDownBody* pTDNode, int ThisBone)
{
	assert(pTDNode != NULL);

	pTDNode->BoneIndex = ThisBone;
	pTDNode->NumChildren = 0;
	pTDNode->pChildren = NULL;
}

static geBoolean TopDownBody_AddChildren(TopDownBody* pTDNode, int ThisBone, geBody* pBody)
{
	int BoneCount;
	int i;
	const char* pName;
	geXForm3d dummyMatrix;
	int ParentIndex;

	assert(pTDNode != NULL);
	assert(pBody != NULL);
	assert(geBody_IsValid(pBody) != GE_FALSE);
	assert(pTDNode->NumChildren == 0);
	assert(pTDNode->pChildren == NULL);

	// Body's bone hierarchy links each bone to its parent.  This guarantees
	// all children to have a higher bone index or they would not have been
	// to be added.
	BoneCount = geBody_GetBoneCount(pBody);
	for(i=ThisBone+1;i<BoneCount;i++)
	{
		geBody_GetBone(pBody, i, &pName, &dummyMatrix, &ParentIndex);
		if(ParentIndex == ThisBone)
		{
			// found a child
			TopDownBody* pNewNode;

#ifdef _DEBUG
			if(pTDNode->NumChildren == 0)
			{
				assert(pTDNode->pChildren == NULL);
			}
#endif

			pNewNode = geRam_Realloc(	pTDNode->pChildren, 
										sizeof(TopDownBody) * (pTDNode->NumChildren + 1) );
			if(pNewNode == NULL)
			{
				return(GE_FALSE);
			}

			// Do not adjust the number of children until the child node
			// actually exists.
			pTDNode->pChildren = pNewNode;
			TopDownBody_InitNode(&pTDNode->pChildren[pTDNode->NumChildren], i);
			pTDNode->NumChildren++;

			if(TopDownBody_AddChildren(&pTDNode->pChildren[pTDNode->NumChildren - 1], i, pBody) == GE_FALSE)
			{
				// something failed
				return(GE_FALSE);
			}
		}
	}

	return(GE_TRUE);
}

static void TopDownBody_DestroyChildren(TopDownBody* pTDNode)
{
	int i;
	assert(pTDNode != NULL);

	if(pTDNode->NumChildren > 0)
	{
		for(i=0;i<pTDNode->NumChildren;i++)
		{
			TopDownBody_DestroyChildren(&pTDNode->pChildren[i]);
		}

		geRam_Free(pTDNode->pChildren);
		pTDNode->NumChildren = 0;
	}
}

TopDownBody* TopDownBody_CreateFromBody(geBody* pBody)
{
	TopDownBody* pTDBody = NULL;

	assert(pBody != NULL);
	assert(geBody_IsValid(pBody) != GE_FALSE);

	pTDBody = GE_RAM_ALLOCATE_STRUCT(TopDownBody);
	if(pTDBody == NULL)
	{
		return(NULL);
	}

	// Initialize for root which is guaranteed to be the first bone (0).
	TopDownBody_InitNode(pTDBody, 0);

	if(TopDownBody_AddChildren(pTDBody, 0, pBody) == GE_FALSE)
	{
		// something failed
		TopDownBody_Destroy(&pTDBody);
	}

	return(pTDBody);
}

void TopDownBody_Destroy(TopDownBody** ppTDBody)
{
	TopDownBody_DestroyChildren(*ppTDBody);

	geRam_Free(*ppTDBody);
}

const TopDownBody* TopDownBody_FindBoneIndex(const TopDownBody* pTDNode, int Index)
{
	const TopDownBody* pFoundNode = NULL;
	int i;

	if(pTDNode->BoneIndex == Index)
		return(pTDNode);

	for(i=0;i<pTDNode->NumChildren;i++)
	{
		pFoundNode = TopDownBody_FindBoneIndex(pTDNode->pChildren + i, Index);
		if(pFoundNode != NULL)
			break;
	}

	return(pFoundNode);
}

TDBodyHeritage TopDownBody_IsDescendentOf(const TopDownBody* pTDBody, int ParentIndex, int Index)
{
	const TopDownBody* pParentNode;
	const TopDownBody* pChildNode;

	pParentNode = TopDownBody_FindBoneIndex(pTDBody, ParentIndex);
	if(pParentNode == NULL)
	{
		assert(0);
	}
	else
	{
		pChildNode = TopDownBody_FindBoneIndex(pParentNode, Index);
		if(pChildNode != NULL)
			return(TDBODY_IS_DESCENDENT);
	}

	return(TDBODY_IS_NOT_DESCENDENT);
}

