/****************************************************************************************/
/*  TDBODY.H																			*/
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef TDBODY_H
#define TDBODY_H

#include "body.h"

typedef struct tag_TopDownBody
{
	int BoneIndex;
	int NumChildren;
	struct tag_TopDownBody* pChildren;
} TopDownBody;

typedef enum 
{
	TDBODY_IS_NOT_DESCENDENT,
	TDBODY_IS_DESCENDENT,
} TDBodyHeritage;

TopDownBody* TopDownBody_CreateFromBody(geBody* pBody);

void TopDownBody_Destroy(TopDownBody** ppTDBody);

TDBodyHeritage TopDownBody_IsDescendentOf(const TopDownBody* pTDBody, int ParentIndex, int Index);

#endif // TDBODY_H