/****************************************************************************************/
/*  face.c                                                                              */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird                                                */
/*  Description:  Face list management, io, etc...                                      */
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
#include "facelist.h"
#include <assert.h>
#include "typeio.h"
#include "ram.h"
#include <memory.h>

struct tag_FaceList
{
	int NumFaces;
	int Limit;
	Face **Faces;
	geBoolean Dirty;
	Box3d Bounds;
};

FaceList	*FaceList_Create(int NumFaces)
{
	FaceList *pList;

	assert (NumFaces > 0);

	// allocate the structure
	pList = geRam_Allocate(sizeof (FaceList));
	if (pList != NULL)
	{
		pList->NumFaces = 0;
		pList->Limit = NumFaces;
		// allocate space for NumFaces pointers
		pList->Faces = geRam_Allocate(NumFaces * sizeof (Face *));
		if (pList->Faces != NULL)
		{
			int i;
			// set the pointers to NULL
			for (i = 0; i < pList->Limit; i++)
			{
				pList->Faces[i] = NULL;
			}
		}
		pList->Dirty = GE_TRUE;
	}

	return pList;
}

void	FaceList_Destroy(FaceList **ppList)
{
	int i;
	FaceList *pList;

	assert (ppList != NULL);
	assert (*ppList != NULL);

	pList = *ppList;
	for (i = 0; i < pList->NumFaces; i++)
	{
		Face_Destroy (&pList->Faces[i]);
	}
	geRam_Free (pList->Faces);
	geRam_Free (*ppList);
	*ppList = NULL;
}

// Grows the face list by doubling its size.
geBoolean FaceList_Grow (FaceList **ppList)
{
	FaceList *pList;
	FaceList *NewList;

	assert (ppList != NULL);
	assert (*ppList != NULL);

	pList = *ppList;
	// create the new list
	NewList = FaceList_Create (2 * pList->Limit);
	if (NewList == NULL)
	{
		return GE_FALSE;
	}
	
	// copy old list into new list
	NewList->NumFaces = pList->NumFaces;
	memcpy (NewList->Faces, pList->Faces, (pList->NumFaces * sizeof (Face *)));
	NewList->Dirty = GE_TRUE;

	// get rid of the old list, but not the pointers...
	geRam_Free ((*ppList)->Faces);
	geRam_Free (*ppList);

	// and set the new list pointer.
	*ppList= NewList;

	return GE_TRUE;
}

FaceList	*FaceList_Clone(const FaceList *pList)
{
	FaceList	*cpList;
	Face		*cpf;
	int			i;
	const Face	*f;

	assert (pList != NULL);
	assert (pList->NumFaces <= pList->Limit);

	// allocate the structure
	cpList	=FaceList_Create(pList->NumFaces);

	if(cpList)
	{
		for(i=0;i < pList->NumFaces;i++)
		{
			f	=FaceList_GetFace(pList, i);
			cpf	=Face_Clone(f);
			FaceList_AddFace(cpList, cpf);
		}
	}
	return	cpList;
}

void	FaceList_AddFace(FaceList *pList, Face *pFace)
{
	assert (pList != NULL);
	assert (pFace != NULL);
	assert (pList->NumFaces < pList->Limit);

	// here we could grow the list if we want...

	pList->Faces[pList->NumFaces] = pFace;
	++(pList->NumFaces);
	pList->Dirty = GE_TRUE;
}

void	FaceList_SetDirty(FaceList *pList)
{
	assert (pList != NULL);
	assert (pList->NumFaces <= pList->Limit);

	pList->Dirty = GE_TRUE;
}

//frees the affected face
void	FaceList_RemoveFace(FaceList *pList, int WhichFace)
{
	int	i;

	assert (pList != NULL);
	assert (pList->NumFaces <= pList->Limit);
	assert (pList->Faces[WhichFace] !=NULL);

	Face_Destroy(&pList->Faces[WhichFace]);

	for(i=WhichFace;i < pList->NumFaces-1;i++)
	{
		pList->Faces[i]	=pList->Faces[i+1];
	}
	pList->Faces[--pList->NumFaces]= NULL;

	pList->Dirty = GE_TRUE;
}

int	FaceList_GetNumFaces(const FaceList *pList)
{
	assert (pList != NULL);
	return pList->NumFaces;
}

int	FaceList_GetFaceLimit (const FaceList *pList)
{
	assert (pList != NULL);

	return pList->Limit;
}


Face	*FaceList_GetFace(const FaceList *pList, int WhichFace)
{
	assert (pList != NULL);
	assert (WhichFace < pList->NumFaces);

	return	pList->Faces[WhichFace];
}


void	FaceList_Rotate(FaceList *pList, const geXForm3d *pXfm, const geVec3d *pCenter)
{
	int i;

	assert (pList != NULL);
	assert (pXfm != NULL);
	assert (pCenter != NULL);

	for (i = 0; i < pList->NumFaces; i++)
	{
		Face_Rotate(pList->Faces[i], pXfm, pCenter);
	}
	pList->Dirty	=GE_TRUE;
}

void	FaceList_Move(FaceList *pList, const geVec3d *trans)
{
	int i;

	assert (pList != NULL);
	assert (trans != NULL);

	for (i = 0; i < pList->NumFaces; i++)
	{
		Face_Move(pList->Faces[i], trans);
	}
	pList->Dirty	=GE_TRUE;
}

void	FaceList_Transform(FaceList *pList, const geXForm3d *pXfm)
{
	int i;

	assert (pList != NULL);
	assert (pXfm != NULL);

	for (i = 0; i < pList->NumFaces; i++)
	{
		Face_Transform(pList->Faces[i], pXfm);
	}
	pList->Dirty	=GE_TRUE;
}

void	FaceList_Scale(FaceList *pList, const geVec3d *ScaleVec)
{
	int i;

	assert(pList != NULL);
	assert(ScaleVec != NULL);

	for(i=0;i < pList->NumFaces;i++)
	{
		Face_Scale(pList->Faces[i], ScaleVec);
	}
	pList->Dirty	=GE_TRUE;
}

void	FaceList_Shear(FaceList *pList, const geVec3d *ShearVec, const geVec3d *ShearAxis)
{
	int i;

	assert(pList != NULL);
	assert(ShearVec != NULL);
	assert(ShearAxis != NULL);

	for(i=0;i < pList->NumFaces;i++)
	{
		Face_Shear(pList->Faces[i], ShearVec, ShearAxis);
	}
	pList->Dirty	=GE_TRUE;
}

void	FaceList_GetCenter(const FaceList *pList, geVec3d *pCenter)
{
	Box3d	Bounds;

	assert (pList != NULL);
	assert (pCenter != NULL);

	FaceList_GetBounds(pList, &Bounds);
	Box3d_GetCenter(&Bounds, pCenter);
}

void	FaceList_GetBounds(const FaceList *pList, Box3d *pBounds)
{
	int i;
	
	assert (pList != NULL);
	assert (pBounds != NULL);
	assert (pList->NumFaces > 0);

	if(pList->Dirty)
	{
		Box3d	Bounds;

		Face_GetBounds(pList->Faces[0], &Bounds);
		for (i = 1; i < pList->NumFaces; i++)
		{
			Box3d FaceBounds;

			Face_GetBounds(pList->Faces[i], &FaceBounds);
			Box3d_Union(&Bounds, &FaceBounds, &Bounds);
		}
		// The casts are necessary because pList is const
		// The idea here is that Bounds and Dirty are implementation
		// details rather than interface data.  So it's OK to change
		// them here.  Like C++'s mutable keyword.
		((FaceList *)pList)->Bounds = Bounds;
		((FaceList *)pList)->Dirty = GE_FALSE;
	}
	*pBounds = pList->Bounds;
}

geBoolean	FaceList_Write(const FaceList *pList, FILE *f)
{
	int i;

	assert (pList != NULL);
	assert (f != NULL);

	if (fprintf(f, "\tBrushFaces %d\n",pList->NumFaces) < 0) return GE_FALSE;
	for(i=0;i < pList->NumFaces; i++)
	{
		if (!Face_Write(pList->Faces[i], f)) return GE_FALSE;
	}
	return GE_TRUE;
}

FaceList	*FaceList_CreateFromFile
	(
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor,
	  const char **Expected
	)
{
	int NumFaces;
	FaceList *pList;

	assert (Parser != NULL);

	if (!Parse3dt_GetInt (Parser, (*Expected = "BrushFaces"), &NumFaces)) return NULL;

	pList = FaceList_Create (NumFaces);
	if (pList != NULL)
	{
		int i;
		for (i = 0; i < NumFaces; i++)
		{
			Face *pFace;

			pFace = Face_CreateFromFile (Parser, VersionMajor, VersionMinor, Expected);
			if (pFace != NULL)
			{
				FaceList_AddFace (pList, pFace);
			}
		}
		if(pList->NumFaces < 4)
		{
			FaceList_Destroy (&pList);
			return NULL;
		}
		pList->Dirty = GE_TRUE;
	}
	return pList;
}

geBoolean	FaceList_WriteToMap(const FaceList *pList, FILE *f)
{
	int	i;

	assert (pList != NULL);
	assert (f != NULL);

	TypeIO_WriteInt(f, pList->NumFaces);
	for (i = 0; i < pList->NumFaces; i++)
	{
		Face_WriteToMap(pList->Faces[i], f);
	}
	return	GE_TRUE;
}

geBoolean	FaceList_WriteToQuakeMap(const FaceList *pList, FILE *f)
{
	int	i;

	assert (pList != NULL);
	assert (f != NULL);

	fprintf(f, "{\n ");

	for (i = 0; i < pList->NumFaces; i++)
	{
		Face_WriteToQuakeMap(pList->Faces[i], f);
	}

	fprintf(f, "}\n");
	return	GE_TRUE;
}

static	geFloat		dists[256];
static	uint8		sides[256];

enum SideFlags
{
	SIDE_FRONT	=0,
	SIDE_BACK	=1,
	SIDE_ON		=2,
	SIDE_SPLIT	=3
};

//clips f behind every face in fl
void	FaceList_ClipFaceToList(const FaceList *fl, Face **f)
{
	int			i;
	const Plane	*p;
	uint8		cnt[3];

	assert(fl != NULL);
	assert(f != NULL);
	assert(*f != NULL);

	for(i=0;i < fl->NumFaces;i++)
	{
		p	=Face_GetPlane(fl->Faces[i]);

		Face_GetSplitInfo(*f, p, dists, sides, cnt);
		if(!cnt[SIDE_FRONT] && !cnt[SIDE_BACK])	//coplanar
		{
			Face_Destroy(f);
			return;
		}
		else if(!cnt[SIDE_FRONT])	//back
		{
			continue;
		}
		else if(!cnt[SIDE_BACK])	//front
		{
			Face_Destroy(f);
			return;
		}
		else	//split
		{
			Face_Clip(*f, p, dists, sides);
		}
	}
}

//if it's selected, chances are it will need modification
Face	*FaceList_GetSelectedFace(const FaceList *fl)
{
	int		i;

	assert(fl);

	for(i=0;i < fl->NumFaces;i++)
	{
		if(Face_IsSelected(fl->Faces[i]))
		{
			break;
		}
	}

	if(i < fl->NumFaces)
	{
		return	fl->Faces[i];
	}
	else
	{
		return	NULL;
	}
}

geBoolean	FaceList_SetNextSelectedFace(FaceList *fl)
{
	int		i;

	assert(fl);

	for(i=0;i < fl->NumFaces;i++)
	{
		if(Face_IsSelected(fl->Faces[i]))
		{
			break;
		}
	}

	if(i < fl->NumFaces-1)
	{
		Face_SetSelected(fl->Faces[i], GE_FALSE);
		i++;
		i	%=fl->NumFaces;
		Face_SetSelected(fl->Faces[i], GE_TRUE);
		return	GE_TRUE;
	}
	else if(i < fl->NumFaces)
	{
		Face_SetSelected(fl->Faces[i], GE_FALSE);
		return	GE_FALSE;	//skip to next brush or select first 
	}
	else	//if it didn't overflow... there simply wasn't
	{		//anything selected, select the first face
		Face_SetSelected(fl->Faces[0], GE_TRUE);
		return	GE_TRUE;
	}
}

geBoolean	FaceList_SetPrevSelectedFace(FaceList *fl)
{
	int		i;

	assert(fl);

	for(i=0;i < fl->NumFaces;i++)
	{
		if(Face_IsSelected(fl->Faces[i]))
		{
			break;
		}
	}

	if(i >= fl->NumFaces)	//if it didn't underflow... there simply wasn't
	{						//anything selected, select the last face
		Face_SetSelected(fl->Faces[fl->NumFaces-1], GE_TRUE);
		return	GE_TRUE;
	}
	else if(i==0)
	{
		Face_SetSelected(fl->Faces[i], GE_FALSE);
		return	GE_FALSE;	//skip to prev brush or select last
	}
	else
	{
		Face_SetSelected(fl->Faces[i], GE_FALSE);
		i--;
		Face_SetSelected(fl->Faces[i], GE_TRUE);
		return	GE_TRUE;
	}
}

void	FaceList_CopyFaceInfo(const FaceList *src, FaceList *dst)
{
	int i;

	assert(src);
	assert(dst);

	if(src->NumFaces != dst->NumFaces)
	{
		return;
	}

	for(i=0;i < src->NumFaces;i++)
	{
		Face_CopyFaceInfo(src->Faces[i], dst->Faces[i]);
	}
	dst->Dirty	=GE_TRUE;
}

//Drops trans values into every face
void	FaceList_SetTranslucency(const FaceList *fl, geFloat trans)
{
	int			i;

	assert(fl != NULL);

	for(i=0;i < fl->NumFaces;i++)
	{
		Face_SetTranslucency(fl->Faces[i], trans);
	}
}

// sets every face to translucent
void		FaceList_SetTransparent (const FaceList *fl, geBoolean trans)
{
	int			i;

	assert(fl != NULL);

	for(i=0;i < fl->NumFaces;i++)
	{
		Face_SetTransparent (fl->Faces[i], trans);
	}
}

