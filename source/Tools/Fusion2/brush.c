/****************************************************************************************/
/*  brush.c                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax, John Pollard                      */
/*  Description:  Brush management, io, csg, list management, and transform operations  */
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
#include "brush.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "VEC3D.H"
#include "BASETYPE.H"
#include "typeio.h"
#include "facelist.h"
#include "units.h"
#include "RAM.H"
#include "util.h"
#include "ConsoleTab.h"	//for conprintf

/*
	Our brushes work as three different objects.  The basic square box is a leaf brush
	that contains the original verts and info from the template dialog.  Leaf brushes
	will have a facelist, and possibly a brushlist containing pieces of the original
	brush that have been cut up by others.  Leaf is a bit confusing, since these
	brushes aren't actually leaf regions in a tree or anything.
	
	Multi brushes such as hollows and arches and other nonconvex brushes will contain
	a series of leaf brushes.  Multi brushes should contain no faces themselves.

	CSG brushes are the end result of cuts.  They are stored beneath the parent leaf.

	Brushes are not exclusive to volume in our system.  We don't csg multiple solid
	areas away.  Only drastic content changes will cause this. (cuts)

	There's alot of work done to keep the data structures hidden to other files.  This
	is a standard practice here at WildTangent and brush, face, facelist are all good
	examples of how "opaque" objects work.
*/


static const int		axidx[3][2]	={ 2, 1, 0, 2, 0, 1 };
static const geVec3d	VecOrigin	={ 0.0f, 0.0f, 0.0f };

struct tag_BrushList
{
	Brush *First;
	Brush *Last;
};

#define	VectorToSUB(a, b)			(*((((geFloat *)(&a))) + (b)))
#define	VCOMPARE_EPSILON			0.001f
#define BOGUS_RANGE					32000.0f

enum BrushFlags
{
	BRUSH_SOLID			=0x0001,
	BRUSH_WINDOW		=0x0002, 
	BRUSH_WAVY			=0x0004,
	BRUSH_DETAIL		=0x0008,	//not included in vis calculations		
	BRUSH_HOLLOWCUT		=0x0010,
	BRUSH_TRANSLUCENT	=0x0020,
	BRUSH_EMPTY			=0x0040,
	BRUSH_SUBTRACT		=0x0080,
	BRUSH_CLIP			=0x0100,
	BRUSH_FLOCKING		=0x0200,	
	BRUSH_HOLLOW		=0x0400,
	BRUSH_SHEET			=0x0800,
	BRUSH_HIDDEN		=0x1000,
	BRUSH_LOCKED		=0x2000,
	BRUSH_HINT			=0x4000,
	BRUSH_AREA			=0x8000
	// All flags larger than 0x8000 (i.e. 0x00010000 through 0x80000000)
	// are reserved for user contents.
};

// Brush flags output to GBSPLIB
enum
{
	bfSolid			= (1<<0),
	bfWindow		= (1<<1),
	bfEmpty			= (1<<2),
	bfTranslucent	= (1<<3),
	bfWavy			= (1<<4),
	bfDetail		= (1<<5),
	bfClip			= (1<<6),
	bfHint			= (1<<7),
	bfArea			= (1<<8),
	bfFlocking		= (1<<9),
	bfSheet			= (1<<10)
	// flags 11 through 15 are reserved for future expansion.
	// flags 16 through 31 are user flags.
};

#define USER_FLAGS_MASK		(0xffff0000ul)
#define SYSTEM_FLAGS_MASK	(0x0000fffful)

// brush edit flags (not currently used, but please don't remove)
enum
{
	befHidden		= (1<<0),
	befLocked		= (1<<1),
	befSelected		= (1<<2),
	befEntity		= (1<<3),
//	befTexInvalid	= (1<<4),
	befHollow		= (1<<5),
	befRing			= (1<<6)
};

typedef struct BrushTag
{
	struct BrushTag	*Prev, *Next;
	FaceList		*Faces;			//null if multibrush
	BrushList		*BList;			//null if csgbrush
	unsigned long	Flags;
	int				Type;
	int				ModelId;
	int				GroupId;
	geFloat			HullSize;		//for hollows
	uint32			Color;
	char			*Name;
	Box3d			BoundingBox;
} Brush;

typedef void (*BrushList_FlagCB)(Brush *pBrush, const geBoolean bState);
typedef void (*BrushList_IntCB)(Brush *pBrush, const int iVal);
typedef void (*BrushList_FloatCB)(Brush *pBrush, const float fVal);
typedef void (*BrushList_Uint32CB)(Brush *pBrush, const uint32 uVal);

static void	BrushList_SetFlag(BrushList *bl, const geBoolean bState, BrushList_FlagCB cbSetFlag)
{
	Brush	*b;

	assert(bl != NULL);

	for(b=bl->First;b;b=b->Next)
	{
		cbSetFlag(b, bState);
	}
}

static void	BrushList_SetInt(BrushList *bl, const int iVal, BrushList_IntCB cbSetInt)
{
	Brush	*b;

	assert(bl != NULL);

	for(b=bl->First;b;b=b->Next)
	{
		cbSetInt(b, iVal);
	}
}

static void	BrushList_SetUint32(BrushList *bl, const uint32 uVal, BrushList_Uint32CB cbSetUint)
{
	Brush	*b;

	assert(bl != NULL);

	for(b=bl->First;b;b=b->Next)
	{
		cbSetUint(b, uVal);
	}
}

static void	BrushList_SetFloat(BrushList *bl, const geFloat fVal, BrushList_FloatCB cbSetFloat)
{
	Brush	*b;

	assert(bl != NULL);

	for(b=bl->First;b;b=b->Next)
	{
		cbSetFloat(b, fVal);
	}
}

Brush	*Brush_Create(int Type, const FaceList *fl, const BrushList *BList)
{
	Brush	*pBrush;

	pBrush	=geRam_Allocate(sizeof (Brush));
	if(pBrush != NULL)
	{
		pBrush->Prev	=NULL;
		pBrush->Next	=NULL;
		pBrush->ModelId	=0;
		pBrush->GroupId	=0;
		pBrush->HullSize=1.0f;
		pBrush->Color	=0;
		pBrush->Name	=Util_Strdup("NoName");
		pBrush->Type	=Type;
		switch(Type)
		{
		case	BRUSH_MULTI:
			assert(fl==NULL);
			assert(BList);

			pBrush->Faces	=NULL;
			pBrush->BList	=(BrushList *)BList;
			pBrush->Flags	=BRUSH_SOLID;
			BrushList_GetBounds(BList, &pBrush->BoundingBox);
			break;

		case	BRUSH_LEAF:
		case	BRUSH_CSG:
			assert(fl);
			assert(BList==NULL);	//shouldn't create leaf from multiple

			pBrush->Faces	=(FaceList *)fl;
			pBrush->BList	=NULL;
			pBrush->Flags	=BRUSH_SOLID;
			FaceList_GetBounds(fl, &pBrush->BoundingBox);
			break;

		default:
			assert(0);
		}
	}
	return pBrush;
}

//creates a csg brush from a leaf or csg brush
static Brush	*Brush_CreateFromParent(const Brush *ParentBrush, const FaceList *fl)
{
	Brush	*pBrush;

	assert(ParentBrush);
	assert(fl != NULL);
	assert(ParentBrush->Type!=BRUSH_MULTI);

	pBrush	=geRam_Allocate(sizeof (Brush));
	if(pBrush != NULL)
	{
		pBrush->Prev	=NULL;
		pBrush->Next	=NULL;
		pBrush->Faces	=(FaceList *)fl;
		pBrush->BList	=NULL;
		pBrush->Flags	=ParentBrush->Flags;
		pBrush->ModelId	=ParentBrush->ModelId;
		pBrush->GroupId	=ParentBrush->GroupId;
		pBrush->HullSize=ParentBrush->HullSize;
		pBrush->Color	=ParentBrush->Color;
		pBrush->Name	=Util_Strdup(ParentBrush->Name);
		pBrush->Type	=BRUSH_CSG;
		FaceList_GetBounds(fl, &pBrush->BoundingBox);
	}
	return pBrush;
}

void	Brush_Destroy(Brush **b)
{
	assert(b != NULL);
	assert(*b != NULL);
	assert((*b)->Prev == NULL);
	assert((*b)->Next == NULL);

	if((*b)->Type!=BRUSH_CSG)
	{
		if((*b)->BList)
		{
			BrushList_Destroy(&((*b)->BList));
		}
	}
	if((*b)->Type!=BRUSH_MULTI)
	{
		if((*b)->Faces)
		{
			FaceList_Destroy(&((*b)->Faces));
		}
	}

	if((*b)->Name)
	{
		geRam_Free ((*b)->Name);
	}
	geRam_Free (*b);
	*b	=NULL;
}

Brush	*Brush_Clone(Brush const *from)
{
	Brush		*to = NULL;
	FaceList	*NewFaces;
	BrushList	*MBList;

	assert(from != NULL);

	switch(from->Type)
	{
	case	BRUSH_MULTI:
		assert(from->Faces==NULL);
		assert(from->BList);

		MBList	=BrushList_Clone(from->BList);
		if(!MBList)
		{
			break;
		}
		to		=Brush_Create(from->Type, NULL, MBList);	
		break;

	case	BRUSH_LEAF:
	case	BRUSH_CSG:
		assert(from->Faces != NULL);

		NewFaces	=FaceList_Clone(from->Faces);
		if(NewFaces != NULL)
		{
			to	=Brush_Create(from->Type, NewFaces, NULL);
		}
		if(to==NULL)
		{
			geRam_Free (NewFaces);
		}
		break;

	default:
		assert(0);
		break;
	}

	if(to != NULL)
	{
		to->Flags		=from->Flags;
		to->Type		=from->Type;
		to->ModelId		=from->ModelId;
		to->GroupId		=from->GroupId;
		to->HullSize	=from->HullSize;
		to->Color		=from->Color;
		Brush_SetName (to, from->Name);
		to->BoundingBox	=from->BoundingBox;
	}

	return	to;
}

static void	Brush_CopyFaceInfo(Brush const *src, Brush *dst)
{
	assert(src);
	assert(dst);
	assert(src->Type!=BRUSH_MULTI);
	assert(dst->Type!=BRUSH_MULTI);
	assert(src->Faces);
	assert(dst->Faces);

	FaceList_CopyFaceInfo(src->Faces, dst->Faces);
}

int	Brush_GetNumFaces(const Brush *b)
{
	assert(b != NULL);
	assert(b->Faces != NULL);

	return	FaceList_GetNumFaces(b->Faces);
}

Face *Brush_GetFace(const Brush *b, int i)
{
	assert(b != NULL);
	assert(b->Faces != NULL);

	return	FaceList_GetFace(b->Faces, i);
}

int	Brush_GetModelId(const Brush *b)
{
	assert(b != NULL);

	return	b->ModelId;
}

int	Brush_GetGroupId(const Brush *b)
{
	assert(b != NULL);
	
	return	b->GroupId;
}

geFloat	Brush_GetHullSize(const Brush *b)
{
	assert(b != NULL);

	return	b->HullSize;
}

uint32	Brush_GetColor(const Brush *b)
{
	assert(b != NULL);
	
	return	b->Color;
}

int			Brush_GetType (const Brush *b)
{
	return b->Type;
}

const char	*Brush_GetName(const Brush *b)
{
	assert(b != NULL);
	
	//are empty names ok?
	return	b->Name;
}

const BrushList	*Brush_GetBrushList(const Brush *b)
{
	assert(b != NULL);
	
	//are empty names ok?
	return	b->BList;
}


void	Brush_SetModelId(Brush *b, const int mid)
{
	assert(b != NULL);
	
	if (b->Type == BRUSH_MULTI)
	{
		BrushList_SetInt (b->BList, mid, Brush_SetModelId);
	}
	b->ModelId	=mid;
}

//should this be set in child brushes too?
void	Brush_SetGroupId(Brush *b, const int gid)
{
	assert(b != NULL);
	
	if (b->Type == BRUSH_MULTI)
	{
		BrushList_SetInt (b->BList, gid, Brush_SetGroupId);
	}
	b->GroupId	=gid;
}

void	Brush_SetHullSize(Brush *b, const geFloat hsize)
{
	geFloat RealHullSize = hsize;

	assert(b != NULL);

	if (hsize < 1.0f)
	{
		RealHullSize = 1.0f;
	}
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFloat(b->BList, RealHullSize, Brush_SetHullSize);
	}

	b->HullSize	=RealHullSize;
}

void	Brush_SetColor(Brush *b, const uint32 newcolor)
{
	assert(b != NULL);
	
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetUint32(b->BList, newcolor, Brush_SetColor);
	}
	b->Color	=newcolor;
}

//should these go to child brushes?
void	Brush_SetName(Brush *b, const char *newname)
{
	assert(b != NULL);
	assert (newname != NULL);

	//empty names ok?  should I check?  Should I assert?
	if (b->Name != NULL)
	{
		geRam_Free (b->Name);
	}

	b->Name	=Util_Strdup(newname);
}

geBoolean	Brush_IsSolid(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_SOLID)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsWindow(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_WINDOW)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsWavy(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_WAVY)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsDetail(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_DETAIL)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsSubtract(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_SUBTRACT)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsClip(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_CLIP)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsHollow(const Brush *b)
{
	assert(b != NULL);

	return	(b->Flags & BRUSH_HOLLOW)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsHollowCut(const Brush *b)
{
	assert(b != NULL);

	return	(b->Flags & BRUSH_HOLLOWCUT)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsEmpty(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_EMPTY)?	GE_TRUE : GE_FALSE;
}

//this one is ! value of flag
geBoolean	Brush_IsVisible(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_HIDDEN)?	GE_FALSE : GE_TRUE;
}

geBoolean	Brush_IsLocked(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_LOCKED)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsHint(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_HINT)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsArea(const Brush *b)
{
	assert(b != NULL);
	
	return	(b->Flags & BRUSH_AREA)?	GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsTranslucent(const Brush *b)
{

	assert (b != NULL);

	return (b->Flags & BRUSH_TRANSLUCENT) ? GE_TRUE : GE_FALSE;
}

geBoolean	Brush_IsMulti(const Brush *b)
{
	assert (b != NULL);

	return (b->Type == BRUSH_MULTI) ? GE_TRUE : GE_FALSE;
}

geBoolean Brush_IsFlocking (const Brush *b)
{
	assert (b != NULL);

	return (b->Flags & BRUSH_FLOCKING) ? GE_TRUE : GE_FALSE;
}

geBoolean Brush_IsSheet (const Brush *b)
{
	assert (b != NULL);

	return (b->Flags & BRUSH_SHEET) ? GE_TRUE : GE_FALSE;
}

static void Brush_SetExclusiveState (Brush *b, int Mask, geBoolean bState)
{
	if (bState)
	{
		b->Flags &= ~(BRUSH_SOLID | BRUSH_CLIP | BRUSH_WINDOW | BRUSH_HINT | BRUSH_SUBTRACT | BRUSH_EMPTY);
		b->Flags |= Mask;
	}
	else
	{
		b->Flags &= ~Mask;
	}
}

void	Brush_SetSolid(Brush *b, const geBoolean bState)
{
	assert(b != NULL);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetSolid);
	}

	Brush_SetExclusiveState (b, BRUSH_SOLID, bState);
}

void	Brush_SetWindow(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetWindow);
	}

	Brush_SetExclusiveState (b, BRUSH_WINDOW, bState);
}

void	Brush_SetWavy(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetWavy);
	}

	b->Flags	=(bState)? b->Flags|BRUSH_WAVY : b->Flags&~BRUSH_WAVY;
}

void	Brush_SetDetail(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetDetail);
	}

	b->Flags	=(bState)? b->Flags|BRUSH_DETAIL : b->Flags&~BRUSH_DETAIL;
}

void	Brush_SetSubtract(Brush *b, const geBoolean bState)
{
	assert(b != NULL);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetSubtract);
	}

	if(b->Flags & BRUSH_HOLLOWCUT)
	{
		return;	//can't reset these
	}
	
	Brush_SetExclusiveState (b, BRUSH_SUBTRACT, bState);
}

void	Brush_SetClip(Brush *b, const geBoolean bState)
{
	assert(b != NULL);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetClip);
	}
		
	Brush_SetExclusiveState (b, BRUSH_CLIP, bState);
}

void	Brush_SetHollow(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetHollow);
	}
	if(b->Flags & BRUSH_HOLLOWCUT)
	{
		return;	//can't reset these
	}

	b->Flags	=(bState)? b->Flags|BRUSH_HOLLOW : b->Flags&~BRUSH_HOLLOW;
}

void	Brush_SetHollowCut(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
	assert(b->Type != BRUSH_MULTI);

	//should set both here... HOLLOWCUT is just a dialog helper		
	b->Flags	=(bState)? b->Flags|BRUSH_HOLLOWCUT : b->Flags&~BRUSH_HOLLOWCUT;
	b->Flags	=(bState)? b->Flags|BRUSH_SUBTRACT : b->Flags&~BRUSH_SUBTRACT;
}

void	Brush_SetFlocking (Brush *b, const geBoolean bState)
{
	assert (b != NULL);

	b->Flags = (bState) ? b->Flags | BRUSH_FLOCKING : b->Flags & ~BRUSH_FLOCKING;
}

void	Brush_SetSheet (Brush *b, const geBoolean bState)
{
	int		i;
	Face	*f;

	assert (b != NULL);
	if(b->Type != BRUSH_MULTI)
	{
		b->Flags = (bState) ? b->Flags | BRUSH_SHEET : b->Flags & ~BRUSH_SHEET;

		f	=FaceList_GetFace(b->Faces, 0);
		Face_SetSheet(f, bState);

		for(i=1;i < FaceList_GetNumFaces(b->Faces);i++)
		{
			f	=FaceList_GetFace(b->Faces, i);
			Face_SetVisible(f, !bState);
		}
	}
}

void		Brush_SetUserFlags (Brush *b, unsigned long UserFlags)
{
	assert (b != NULL);

	// mask off the lower bits so they don't get modified
	b->Flags = (b->Flags & SYSTEM_FLAGS_MASK) | (UserFlags & USER_FLAGS_MASK);
}

unsigned long Brush_GetUserFlags (const Brush *b)
{
	assert (b != NULL);

	return (b->Flags & USER_FLAGS_MASK);
}

void	Brush_SetEmpty(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetEmpty);
	}

	Brush_SetExclusiveState (b, BRUSH_EMPTY, bState);
}

void	Brush_SetHint(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetHint);
	}

	Brush_SetExclusiveState (b, BRUSH_HINT, bState);
}

//opposite flag value here... should this affect children?
void	Brush_SetVisible(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	b->Flags	=(bState)? b->Flags&~BRUSH_HIDDEN : b->Flags|BRUSH_HIDDEN;
}

//should this affect child brushes?
void	Brush_SetLocked(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	b->Flags	=(bState)? b->Flags|BRUSH_LOCKED : b->Flags&~BRUSH_LOCKED;
}

void	Brush_SetArea(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
		
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetArea);
	}

	b->Flags	=(bState)? b->Flags|BRUSH_AREA : b->Flags&~BRUSH_AREA;
}

void	Brush_SetTranslucent(Brush *b, const geBoolean bState)
{
	assert(b != NULL);
	
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_SetFlag(b->BList, bState, Brush_SetTranslucent);
	}

	b->Flags	=(bState)? b->Flags|BRUSH_TRANSLUCENT : b->Flags&~BRUSH_TRANSLUCENT;
}

//brushes written to gbsplib should be convex
void Brush_WriteToMap(Brush const *b, FILE *ofile, geBoolean VisDetail)
{
	uint32 OutputFlags;

	assert(b != NULL);
	assert(b->Faces != NULL);
	assert(ofile);
	assert(!(b->Flags & BRUSH_HOLLOW));	//only send convex
	assert(!(b->Flags & BRUSH_SUBTRACT));	//only send convex

	// write flags

	// convert flags to output flags expected by GBSPLIB
	OutputFlags = 0;
	if (b->Flags & BRUSH_SOLID)			OutputFlags |= bfSolid;
	if (b->Flags & BRUSH_WINDOW)		OutputFlags |= bfWindow;
	if (b->Flags & BRUSH_WAVY)			OutputFlags |= bfWavy;
	if (b->Flags & BRUSH_DETAIL)		OutputFlags |= bfDetail;
	if (b->Flags & BRUSH_CLIP)			OutputFlags |= bfClip;
	if (b->Flags & BRUSH_HINT)			OutputFlags |= bfHint;
	if (b->Flags & BRUSH_AREA)			OutputFlags |= bfArea;
	if (b->Flags & BRUSH_FLOCKING)		OutputFlags |= bfFlocking;
//	if (b->Flags & BRUSH_TRANSLUCENT)	OutputFlags |= bfTranslucent;
	if (b->Flags & BRUSH_EMPTY)			OutputFlags |= bfEmpty;
	if (b->Flags & BRUSH_SHEET)			OutputFlags |= bfSheet;

	// and do the user flags
	OutputFlags |= (b->Flags & USER_FLAGS_MASK);

	if (VisDetail)
	{
		OutputFlags &= ~bfDetail;
	}

	TypeIO_WriteInt(ofile, OutputFlags);
	FaceList_WriteToMap(b->Faces, ofile);
}

geBoolean Brush_Write(const Brush *b, FILE *ofile)
{
	assert(ofile);
	assert(b);

	if (b->Type == BRUSH_CSG)
	{
		// CSG brushes aren't saved
		return GE_TRUE;
	}

	{
		// make sure we don't output a blank name
		char *Name;
		char QuotedValue[SCANNER_MAXDATA];

		Name = ((b->Name == NULL) || (*b->Name == '\0')) ? "NoName" : b->Name;
		// quote the brush name string...
		Util_QuoteString (Name, QuotedValue);
		if (fprintf (ofile, "Brush %s\n", QuotedValue) < 0) return GE_FALSE;
	}

	if (fprintf(ofile, "\tFlags %d\n",	b->Flags) < 0) return GE_FALSE;
	if (fprintf(ofile, "\tModelId %d\n",b->ModelId) < 0) return GE_FALSE;
	if (fprintf(ofile, "\tGroupId %d\n", b->GroupId) < 0) return GE_FALSE;
	{
		if (b->HullSize < 1.0f)
		{
			((Brush *)b)->HullSize = 1.0f;
		}
		if (fprintf(ofile, "\tHullSize %f\n", b->HullSize) < 0) return GE_FALSE;
	}
	if (fprintf(ofile, "\tType %d\n", b->Type) < 0) return GE_FALSE;

	switch (b->Type)
	{
		case	BRUSH_MULTI:
			return BrushList_Write (b->BList, ofile);

		case	BRUSH_LEAF:
			return FaceList_Write (b->Faces, ofile);

		default :
			assert (0);		// invalid brush type
			break;
	}
	return GE_TRUE;
}

Brush	*Brush_CreateFromFile
	(
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	FaceList	*fl;
	Brush		*b;
	int			tmpFlags, tmpModelId, tmpGroupId, tmpType, tmpTranslucency;
	geFloat		tmpHullSize;
	BrushList	*blist;
	char		szTemp[_MAX_PATH];

	assert (Parser != NULL);

	b	=NULL;

	if((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 24)))
	{
		// version 1.24 and later we quoted the texture names...
		if (!Parse3dt_GetLiteral (Parser, (*Expected = "Brush"), szTemp)) return NULL;
	}
	else
	{
		if (!Parse3dt_GetIdentifier (Parser, (*Expected = "Brush"), szTemp)) return NULL;
	}

	if (!Parse3dt_GetInt (Parser, (*Expected = "Flags"), &tmpFlags)) return NULL;

	if ((VersionMajor == 1) && (VersionMinor <= 10))
	{
		// Clear unused flags from older file versions
		// All brushes are solid by default.
		tmpFlags &= (BRUSH_DETAIL | BRUSH_HOLLOW | BRUSH_SUBTRACT | BRUSH_HOLLOWCUT | BRUSH_HIDDEN | BRUSH_LOCKED);
		tmpFlags |= BRUSH_SOLID;
		tmpFlags &= (~(BRUSH_HINT | BRUSH_CLIP | BRUSH_AREA | BRUSH_TRANSLUCENT | BRUSH_EMPTY));
	}
	if ((VersionMajor == 1) && (VersionMinor < 25))
	{
		// clear flocking flag on old file versions.
		tmpFlags &= ~BRUSH_FLOCKING;
	}
	if ((VersionMajor == 1) && (VersionMinor < 29))
	{
		// clear sheet flag on older file versions
		tmpFlags &= ~BRUSH_SHEET;
	}

	if((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 3)))
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "ModelId"), &tmpModelId)) return NULL;
		if (!Parse3dt_GetInt (Parser, (*Expected = "GroupId"), &tmpGroupId)) return NULL;
	}
	else
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "EntityId"), &tmpModelId)) return NULL;
		tmpGroupId	=0;
	}
	
	if (!Parse3dt_GetFloat (Parser, (*Expected = "HullSize"), &tmpHullSize)) return NULL;
	if (tmpHullSize < 1.0f)
	{
		tmpHullSize = 1.0f;
	}

	if ((VersionMajor == 1) && (VersionMinor <= 16))
	{
		tmpTranslucency = 0;
	}
	else if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor < 27)))
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "Translucency"), &tmpTranslucency)) return NULL;
		if (tmpTranslucency > 255) tmpTranslucency = 255;
		if (tmpTranslucency < 0) tmpTranslucency = 0;
	}
	else
	{
		tmpTranslucency = 0;
	}

	tmpType = BRUSH_LEAF;	// default is leaf brush
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor >= 15)))
	{
		if (!Parse3dt_GetInt (Parser, (*Expected = "Type"), &tmpType)) return NULL;
	}

	fl = NULL;
	blist = NULL;
	switch (tmpType)
	{
		case BRUSH_LEAF :
		{
			fl = FaceList_CreateFromFile (Parser, VersionMajor, VersionMinor, Expected);
			if (fl == NULL)
			{
				goto DoneLoad;
			}
			break;
		}
		case BRUSH_MULTI :
			blist = BrushList_CreateFromFile (Parser, VersionMajor, VersionMinor, Expected);
			if (blist == NULL)
			{
				goto DoneLoad;
			}
			break;
		default :
			assert (0);		//bad stuff here
			return NULL;
	}

	//drop trans into faces from old maps
	if ((VersionMajor > 1) || ((VersionMajor == 1) && (VersionMinor < 27)))
	{
		if(fl)
		{
			FaceList_SetTranslucency(fl, (float)tmpTranslucency);
		}
	}

	if (tmpFlags & BRUSH_TRANSLUCENT)
	{
		// set faces as translucent
		if (fl != NULL)
		{
			FaceList_SetTransparent (fl, GE_TRUE);
		}
		tmpFlags &= ~BRUSH_TRANSLUCENT;
	}

	b = Brush_Create (tmpType, fl, blist);
	if (b == NULL)
	{
		if (fl != NULL)
		{
			FaceList_Destroy (&fl);
		}
		if (blist != NULL)
		{
			BrushList_Destroy (&blist);
		}
	}
	else
	{
		b->Flags	=tmpFlags;
		b->HullSize	=tmpHullSize;
		b->ModelId	=tmpModelId;
		b->GroupId	=tmpGroupId;
		Brush_SetName (b, szTemp);
	}

DoneLoad:
	return	b;
}

void	Brush_Resize(Brush *b, float dx, float dy, int sides, int inidx, geVec3d *fnscale, int *ScaleNum)
{
	int		i;
	geVec3d	FixOrg, BrushOrg, ScaleVec;

	assert(b);
	assert(fnscale);
	assert(ScaleNum);

	geVec3d_Add(&b->BoundingBox.Min, &b->BoundingBox.Max, &BrushOrg);
	geVec3d_Scale(&BrushOrg, 0.5f, &BrushOrg);

	//find the corner of the bounds to keep fixed
	VectorToSUB(FixOrg, inidx)			=0.0f;
	if((sides&3)==0)	//center x
	{
		dx=-dx;
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(BrushOrg, axidx[inidx][0]);
	}
	else if((sides&3)==2)	//less x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==1)	//greater x
	{
		dx=-dx;
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}

	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(BrushOrg, axidx[inidx][1]);
	}
	else if((sides&0x0c)==4)	//less y
	{
		dy=-dy;
		if(inidx!=1)
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		else
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
	}
	else if((sides&0x0c)==8)	//greater y
	{
		if(inidx!=1)
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		else
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
	}

	if((sides&3)==0)	//center x
		dx	=0;
	if((sides&0x0c)==0)	//center x
		dy	=0;

	//translate to fixed origin
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	dx	*=0.005f;
	dy	*=0.005f;

	dx	=1-dx;
	dy	=1-dy;

	VectorToSUB(ScaleVec, inidx)			=1.0f;
	VectorToSUB(ScaleVec, axidx[inidx][0])	=dx;
	VectorToSUB(ScaleVec, axidx[inidx][1])	=dy;

	for(i=0;i<3;i++)
		VectorToSUB(*fnscale, i)	*=VectorToSUB(ScaleVec, i);

	(*ScaleNum)++;

	Brush_Scale3d(b, &ScaleVec);

	//translate back
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	Brush_Bound(b);
}

void Brush_Bound(Brush *b)
{
	assert(b);
	
	Box3d_SetBogusBounds(&b->BoundingBox);
	if(b->Type==BRUSH_MULTI)
	{
		BrushList_GetBounds(b->BList, &b->BoundingBox);
	}
	else
	{
		FaceList_GetBounds(b->Faces, &b->BoundingBox);
	}
}

geBoolean	Brush_TestBoundsIntersect(const Brush *b, const Box3d *pBox)
{
	assert(b);
	assert(pBox);

	return Box3d_Intersection (&b->BoundingBox, pBox, NULL);
}

/*
void Brush_SnapNearest(Brush *b, geFloat gsize, int sides, int inidx)
{
	int				i;
	geVec3d			dmin, vsnap, sbound;
	geFloat const	gsizeinv	=1.0f/(geFloat)gsize;

	//find the corner of the bounds to snap
	VectorToSUB(dmin, inidx)	=0.0f;
	if((sides&3)==0)	//center x
	{
		VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==1)	//less x
	{
		VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==2)	//greater x
	{
		VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}

	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
	}
	else if((sides&0x0c)==4)	//less y
	{
		if(inidx != 1)	//check for top view (which has backwards y axis relation)
		{
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
	}
	else if((sides&0x0c)==8)	//greater y
	{
		if(inidx != 1)
		{
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
	}

	geVec3d_Scale(&dmin, gsizeinv, &sbound);	//get ratio to grid
	for(i=0;i<3;i++)
	{
		//get amount off in grid space
		VectorToSUB(vsnap, i)	=(geFloat)Units_Trunc(VectorToSUB(sbound, i));
	}
	geVec3d_Subtract(&sbound, &vsnap, &sbound);	//delta gridspace error
	geVec3d_Scale(&vsnap, gsize, &vsnap);		//return to worldspace
	geVec3d_Subtract(&dmin, &vsnap, &dmin);		//get worldspace delta

	//move to the nearest corner
	for(i=0;i<3;i++)
	{
		if(VectorToSUB(sbound, i) > 0.5f)
		{
			VectorToSUB(dmin, i)	-=gsize;
		}
		else if(VectorToSUB(sbound, i) < -0.5f)
		{
			VectorToSUB(dmin, i)	+=gsize;
		}
	}

	geVec3d_Inverse(&dmin);
	Brush_Move(b, &dmin);

	Brush_Bound(b);
}
*/

void Brush_SnapScaleNearest(Brush *b, geFloat gsize, int sides, int inidx, geVec3d *fnscale, int *ScaleNum)
{
	int		i;
	geVec3d	FixOrg, BrushOrg;
	geVec3d	dmin, vsnap, sbound;
	geFloat	const	gsizeinv	=1.0f/(geFloat)gsize;

	geVec3d_Add(&b->BoundingBox.Min, &b->BoundingBox.Max, &BrushOrg);
	geVec3d_Scale(&BrushOrg, 0.5f, &BrushOrg);

	//find the corner of the bounds to keep fixed
	VectorToSUB(FixOrg, inidx)			=0.0f;
	VectorToSUB(dmin, inidx)			=0.0f;
	if((sides&3)==0)
	{
		VectorToSUB(FixOrg, axidx[inidx][0])=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
		VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}
	else if((sides&3)==2)
	{
		VectorToSUB(FixOrg, axidx[inidx][0])=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
		VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==1)
	{
		VectorToSUB(FixOrg, axidx[inidx][0])=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
		VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}

	if((sides&0x0c)==0)
	{
		VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
	}
	else if((sides&0x0c)==8)
	{
		if(inidx!=1)	//check for top view (which has backwards y axis relation)
		{
			VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
	}
	else if((sides&0x0c)==4)
	{
		if(inidx!=1)
		{
			VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
			VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
	}

	geVec3d_Scale(&FixOrg, gsizeinv, &sbound);

	for(i=0;i<3;i++)
	{
		VectorToSUB(vsnap, i)	=(geFloat)Units_Trunc(VectorToSUB(sbound, i));
	}

	geVec3d_Subtract(&sbound, &vsnap, &sbound);
	geVec3d_Scale(&vsnap, gsize, &vsnap);
	geVec3d_Subtract(&FixOrg, &dmin, &FixOrg);
	geVec3d_Subtract(&vsnap, &dmin, &vsnap);

	for(i=0;i<3;i++)
	{
		if(VectorToSUB(sbound, i) > 0.5f)
		{
			VectorToSUB(vsnap, i)	+=gsize;
		}
		else if(VectorToSUB(sbound, i) < -0.5f)
		{
			VectorToSUB(vsnap, i)	-=gsize;
		}
	}

	//find the magnitude to expand onto the boundary
	for(i=0;i<3;i++)
	{
		if(VectorToSUB(FixOrg, i))
		{
			VectorToSUB(sbound, i)	=(1.0f - (VectorToSUB(vsnap, i) / VectorToSUB(FixOrg, i)))* 200.0f;
		}
	}
	if((sides&3)==1)
	{
		VectorToSUB(sbound, axidx[inidx][0])=-VectorToSUB(sbound, axidx[inidx][0]);
	}
	if((sides&0x0c)==4)
	{
		VectorToSUB(sbound, axidx[inidx][1])=-VectorToSUB(sbound, axidx[inidx][1]);
	}

	Brush_Resize(b, VectorToSUB(sbound, axidx[inidx][0]), VectorToSUB(sbound, axidx[inidx][1]), sides, inidx, fnscale, ScaleNum);

	Brush_Bound(b);
}

void Brush_ResizeFinal(Brush *b, int sides, int inidx, geVec3d *fnscale)
{
	geVec3d	FixOrg, BrushOrg;

	geVec3d_Add(&b->BoundingBox.Min, &b->BoundingBox.Max, &BrushOrg);
	geVec3d_Scale(&BrushOrg, 0.5f, &BrushOrg);

	//find the corner of the bounds to keep fixed
	VectorToSUB(FixOrg, inidx)			=0.0f;
	if((sides&3)==0)	//center x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(BrushOrg, axidx[inidx][0]);
	}
	else if((sides&3)==2)	//less x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==1)	//greater x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}

	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(BrushOrg, axidx[inidx][1]);
	}
	else if((sides&0x0c)==4)	//less y
	{
		if(inidx!=1)
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
	}
	else if((sides&0x0c)==8)	//greater y
	{
		if(inidx!=1)
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
	}

	if((sides&3)==0)	//center x
	{
		VectorToSUB(*fnscale, axidx[inidx][0])	=1.0f;
	}

	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(*fnscale, axidx[inidx][1])	=1.0f;
	}

	//translate to fixed origin
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	VectorToSUB(*fnscale, inidx)	=1.0f;

	Brush_Scale3d(b, fnscale);

	//translate back
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	Brush_Bound(b);
}

void	Brush_ShearFinal(Brush *b, int sides, int inidx, geVec3d *fnscale)
{
	geVec3d	FixOrg, BrushOrg, ShearAxis;

	assert(b);
	assert(fnscale);

	if(!(((sides&3)==0) ^ ((sides&0x0c)==0)))
	{
		return;
	}

	geVec3d_Add(&b->BoundingBox.Min, &b->BoundingBox.Max, &BrushOrg);
	geVec3d_Scale(&BrushOrg, 0.5f, &BrushOrg);

	//find the corner of the bounds to keep fixed
	VectorToSUB(FixOrg, inidx)			=0.0f;
	if((sides&3)==0)	//center x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(BrushOrg, axidx[inidx][0]);
	}
	else if((sides&3)==2)	//less x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==1)	//greater x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}

	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(BrushOrg, axidx[inidx][1]);
	}
	else if((sides&0x0c)==4)	//less y
	{
		if(inidx!=1)
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		else
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
	}
	else if((sides&0x0c)==8)	//greater y
	{
		if(inidx!=1)
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		else
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
	}

	geVec3d_Clear(&ShearAxis);

	//translate to fixed origin
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	if((sides&3)==0)	//center x
	{
		VectorToSUB(ShearAxis, axidx[inidx][0])	=
			1.0f / VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}
	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(ShearAxis, axidx[inidx][1])	=
			1.0f / VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
	}

	Brush_Shear(b, fnscale, &ShearAxis);

	//translate back
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	Brush_Bound(b);
}

// moves the passed in brush by the vector specified in trans
void	Brush_Move(Brush *b,  const geVec3d *trans)
{
	assert(b && trans);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_Move(b->BList, trans);
	}
	else
	{
		FaceList_Move(b->Faces, trans);
	}

	Brush_Bound(b);
}

void	Brush_Rotate
	(
	  Brush *b,
	  const geXForm3d *pXfmRotate,
	  const geVec3d *pCenter
	)
{
	assert (b != NULL);
	assert (pXfmRotate != NULL);
	assert (pCenter != NULL);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_Rotate(b->BList, pXfmRotate, pCenter);
	}
	else
	{
		FaceList_Rotate(b->Faces, pXfmRotate, pCenter);
	}

	Brush_Bound (b);
}

void	Brush_Transform 
	(
	  Brush *b, 
	  const geXForm3d *pXfm
	)
// Apply the transform to all points in all brush faces
{
	assert (b != NULL);
	assert (pXfm != NULL);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_Transform(b->BList, pXfm);
	}
	else
	{
		FaceList_Transform(b->Faces, pXfm);
	}

	Brush_Bound(b);
}

// calculates relative "center" of brush by finding midpoint between min and max
void	Brush_Center(const Brush *b, geVec3d *center)
{
	assert(b && center);

	Box3d_GetCenter (&b->BoundingBox, center);
}

void	Brush_Scale (Brush *b, float ScaleFactor)
{

	assert (b != NULL);

	b->HullSize	*= ScaleFactor;
	if (b->Type == BRUSH_MULTI)
	{
		BrushList_Scale (b->BList, ScaleFactor);
	}
	else
	{
		geVec3d vecScale;

		geVec3d_Set (&vecScale, ScaleFactor, ScaleFactor, ScaleFactor);
		FaceList_Scale (b->Faces, &vecScale);
	}
	Brush_Bound (b);
}

void	Brush_Scale3d(Brush *b, const geVec3d *mag)
{
	assert(b);
	assert(mag);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_Scale3d(b->BList, mag);
	}
	else
	{
		FaceList_Scale(b->Faces, mag);
	}

	Brush_Bound(b);
}

void	Brush_Shear(Brush *b, const geVec3d *ShearVec, const geVec3d *ShearAxis)
{
	assert(b);
	assert(ShearVec);
	assert(ShearAxis);

	if(b->Type==BRUSH_MULTI)
	{
		BrushList_Shear(b->BList, ShearVec, ShearAxis);
	}
	else
	{
		FaceList_Shear(b->Faces, ShearVec, ShearAxis);
	}
	Brush_Bound(b);
}

Face	*Brush_RayCast(const Brush *b, geVec3d *CamOrg, geVec3d *dir, geFloat *dist)
{
	Face		*f, *firstface;
	geVec3d		p1, p2;
	geFloat		mid, d1, d2;
	int32		i;
	const Plane	*p;
	geFloat		minBDist;
	Face		*curFace	=NULL;
	const Brush	*b2;

	minBDist	=999999.0f;

	//this multi section isn't used currently
	//but might be needed later
	if(b->Type & BRUSH_MULTI)
	{
		for(b2=b->BList->First;b2;b2=b2->Next)
		{
			if(!(b2->Flags & BRUSH_SUBTRACT))
			{
				f	=Brush_RayCast(b2, CamOrg, dir, dist);
				if(f)
				{
					if((*dist >= 8.0f) && (*dist < minBDist))
					{ 
						minBDist	=*dist;
						curFace		=f;
					}
				}
			}
		}
		if(curFace)
		{
			*dist	=minBDist;
			return	curFace;
		}
		else
		{
			*dist	=0;
			return	NULL;
		}
	}

	geVec3d_Copy(CamOrg, &p1);
	geVec3d_AddScaled (&p1, dir, 2*BOGUS_RANGE, &p2);

	firstface	=NULL;

	assert(!(b->Flags & BRUSH_SUBTRACT));

	for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
	{
		geVec3d tempVec;

		f	=FaceList_GetFace(b->Faces, i);
		p	=Face_GetPlane(f);

		d1	=geVec3d_DotProduct(&p1, &p->Normal)	-p->Dist;
		d2	=geVec3d_DotProduct(&p2, &p->Normal)	-p->Dist;
		if(d1 >= 0 && d2 >= 0)
		{
			*dist	=0;
			return	NULL;
		}
		if(d1 <=0 && d2 <= 0)
			continue;

		mid	=d1 / (d1 - d2);

		geVec3d_Subtract (&p2, &p1, &tempVec);
		geVec3d_AddScaled (&p1, &tempVec, mid, &tempVec);
		if (d1 > 0)
		{
			firstface = f;
			p1 = tempVec;
		}
		else
		{
			p2 = tempVec;
		}
	}
	geVec3d_Subtract(&p1, CamOrg, &p1);

	d1		=geVec3d_DotProduct(&p1, dir);
	*dist	=d1;

	return	firstface;
}



/*******************************************************************/
/**************  BRUSH LIST HANDLERS *******************************/
/*******************************************************************/

BrushList *BrushList_Create 
	(
	  void
	)
{
	BrushList *pList;

	pList = geRam_Allocate (sizeof (BrushList));
	if (pList != NULL)
	{
		pList->First = NULL;
		pList->Last = NULL;
	}
	return pList;
}

BrushList *BrushList_CreateFromFile 
	(
	  Parse3dt *Parser, 
	  int VersionMajor, 
	  int VersionMinor, 
	  const char **Expected
	)
{
	int NumBrushes;
	BrushList *blist;

	if (!Parse3dt_GetInt (Parser, (*Expected = "Brushlist"), &NumBrushes)) return NULL;
	blist = BrushList_Create ();
	if (blist != NULL)
	{
		int i;

		for (i = 0; i < NumBrushes; ++i)
		{
			Brush *pBrush;

			pBrush = Brush_CreateFromFile (Parser, VersionMajor, VersionMinor, Expected);
			if (pBrush == NULL)
			{
				BrushList_Destroy (&blist);
				break;
			}
			else
			{
				BrushList_Append (blist, pBrush);
			}
		}
	}
	return blist;
}

void BrushList_Destroy 
	(
	  BrushList **ppList
	)
{
	BrushList *pList;

	assert (ppList != NULL);
	assert (*ppList != NULL);

	pList = *ppList;
	BrushList_DeleteAll (pList);

	geRam_Free (*ppList);
	*ppList = NULL;
}

void BrushList_Append 
	(
	  BrushList *pList, 
	  Brush *pBrush
	)
{
	assert (pList != NULL);
	assert (pBrush != NULL);

	if (pList->First == NULL)
	{
		// the list is empty
		assert (pList->Last == NULL);
		pList->First = pBrush;
		pList->Last = pBrush;
		pBrush->Next = NULL;
		pBrush->Prev = NULL;
	}
	else
	{
		// add it to the end of the list
		assert (pList->Last != NULL);
		assert (pList->Last->Next == NULL);

		pBrush->Next = NULL;			// it's the end of the list
		pBrush->Prev = pList->Last;		// point back to previous end
		pList->Last->Next = pBrush;		// previous end points to me
		pList->Last = pBrush;			// and we're now the list end
	}
}

//insert pbrush into the list after pBMarker
//went nuts on asserts here, but maybe they will save us someday
void BrushList_InsertAfter(BrushList *pList, Brush *pBMarker, Brush *pBrush)
{
	assert (pList != NULL);
	assert (pBMarker != NULL);
	assert (pBrush != NULL);
	assert (pList->First != NULL); //must be at least one
	assert (pList->Last != NULL);
	assert (pList->Last->Next == NULL);

	pBrush->Next	=pBMarker->Next;
	pBrush->Prev	=pBMarker;

	if(pBrush->Next==NULL)	//last in list?
	{
		pList->Last	=pBrush;
	}
	else
	{
		pBrush->Next->Prev	=pBrush;
	}
	pBMarker->Next	=pBrush;
}

//insert pbrush into the list before pBMarker
void BrushList_InsertBefore(BrushList *pList, Brush *pBMarker, Brush *pBrush)
{
	assert (pList != NULL);
	assert (pBMarker != NULL);
	assert (pBrush != NULL);
	assert (pList->First != NULL); //must be at least one
	assert (pList->Last != NULL);
	assert (pList->Last->Next == NULL);

	pBrush->Prev	=pBMarker->Prev;
	pBrush->Next	=pBMarker;

	if(pBrush->Prev==NULL)	//first in list?
	{
		pList->First	=pBrush;
	}
	else
	{
		pBrush->Prev->Next	=pBrush;
	}
	pBMarker->Prev	=pBrush;
}

void BrushList_Prepend 
	(
	  BrushList *pList, 
	  Brush *pBrush
	)
{
	assert (pList != NULL);
	assert (pBrush != NULL);

	if (pList->First == NULL)
	{
		// it's the first brush in the list
		assert (pList->Last == NULL);
		pList->First = pBrush;
		pList->Last = pBrush;
		pBrush->Next = NULL;
		pBrush->Prev = NULL;
	}
	else
	{
		// put it at the head of the list
		assert (pList->Last != NULL);
		assert (pList->First->Prev == NULL);

		pBrush->Prev = NULL;			// nothing before me
		pBrush->Next = pList->First;	// point to previous head
		pList->First->Prev = pBrush;	// previous head points to me
		pList->First = pBrush;			// and now we're the head of the list
	}
}

void BrushList_Remove 
	(
	  BrushList *pList, 
	  Brush *pBrush
	)
{
	Brush *pPrev;
	Brush *pNext;

	assert (pList != NULL);
	assert (pBrush != NULL);
	assert (pBrush->Prev != pBrush);

	pPrev = pBrush->Prev;
	pNext = pBrush->Next;

	// unlink the brush from the list
	pBrush->Prev = NULL;
	pBrush->Next = NULL;

	// update neighbors' previous/next links
	if (pPrev != NULL)
	{
		pPrev->Next = pNext;
	}
	if (pNext != NULL)
	{
		pNext->Prev = pPrev;
	}

	// if this is the first brush in the list,
	// then update the First pointer
	if (pList->First == pBrush)
	{
		assert (pPrev == NULL);
		pList->First = pNext;
	}
	// if it's the last brush in the list,
	// then update the Last pointer
	if (pList->Last == pBrush)
	{
		assert (pNext == NULL);
		pList->Last = pPrev;
	}
}

void BrushList_DeleteAll
	(
	  BrushList *pList
	)
{
	Brush *pBrush;
	BrushIterator bi;

	assert (pList != NULL);

	pBrush = BrushList_GetFirst (pList, &bi);
	while (pBrush != NULL)
	{
		BrushList_Remove (pList, pBrush);
		Brush_Destroy(&pBrush);
		pBrush = BrushList_GetNext (&bi);
	}
}

Brush *BrushList_GetFirst 
	(
	  BrushList *pList, 
	  BrushIterator *bi
	 )
{
	assert (pList != NULL);
	assert (bi != NULL);

	if (pList->First == NULL)
	{
		*bi = NULL;
	}
	else
	{
		*bi = pList->First->Next;
	}
	return pList->First;
}

Brush *BrushList_GetNext 
	(
	  BrushIterator *bi
	)
{
	assert (bi != NULL);

	if (*bi == NULL)
	{
		return NULL;
	}
	else
	{
		Brush *b;

		b = *bi;
		*bi = (*bi)->Next;

		return b;
	}
}

Brush *BrushList_GetLast
	(
	  BrushList *pList, 
	  BrushIterator *bi
	 )
{
	assert (pList != NULL);
	assert (bi != NULL);

	if (pList->Last == NULL)
	{
		*bi = NULL;
	}
	else
	{
		*bi = pList->Last->Prev;
	}
	return pList->Last;
}

Brush *BrushList_GetPrev
	(
	  BrushIterator *bi
	)
{
	assert (bi != NULL);

	if (*bi == NULL)
	{
		return NULL;
	}
	else
	{
		Brush *b;

		b = *bi;
		*bi = (*bi)->Prev;
		return b;
	}
}

int BrushList_Count
	(
	  BrushList const *pList,
	  int CountFlags
	)
{
	int Count;
	Brush *b;
//	geBoolean bResult = GE_TRUE;

	assert (pList != NULL);

	Count = 0;

	b = pList->First;
	while (b != NULL)
	{
		geBoolean CountIt;
		switch (b->Type)
		{
			case BRUSH_MULTI :
				CountIt = (CountFlags & BRUSH_COUNT_MULTI);
				break;

			case BRUSH_LEAF :
				CountIt = (CountFlags & BRUSH_COUNT_LEAF);
				break;

			case BRUSH_CSG :
				CountIt = (CountFlags & BRUSH_COUNT_CSG);
				break;

			default :
				assert (0);
				CountIt = GE_FALSE;
				break;
		}
		if (CountIt)
		{
			++Count;
		}

		if ((b->Type == BRUSH_MULTI) && (!(CountFlags & BRUSH_COUNT_NORECURSE)))
		{
			Count += BrushList_Count (b->BList, CountFlags);
		}
		b = b->Next;
	}

	return Count;
}

// call CallBack for top level brushes in the list...
geBoolean BrushList_Enum
	(
		BrushList const *pList,
		void *			lParam,
		BrushList_CB	CallBack
	)
{
	geBoolean bResult = GE_TRUE ;	// TRUE means entire list was processed
	Brush * b;

	assert (pList != NULL);

	b = pList->First;
	while (b != NULL)
	{
		if( (bResult = CallBack( b, lParam )) == GE_FALSE )
			break ;		
		b = b->Next;
	}
	return bResult ;
}

// call CallBack for all brushes in the list...
geBoolean BrushList_EnumAll
	(
		BrushList const *pList,
		void *			lParam,
		BrushList_CB	CallBack
	)
{
	geBoolean bResult = GE_TRUE ;	// TRUE means entire list was processed
	Brush * b;

	assert (pList != NULL);

	b = pList->First;
	while (b != NULL)
	{
		if( (bResult = CallBack( b, lParam )) == GE_FALSE )
			break ;		
		if (b->Type == BRUSH_MULTI)
		{
			bResult = BrushList_EnumAll (b->BList, lParam, CallBack);
			if (!bResult)
			{
				break;
			}
		}
		b = b->Next;
	}
	return bResult ;
}

//traverses inorder 
int	BrushList_EnumLeafBrushes(const BrushList	*pList,
							  void *			pVoid,
							  BrushList_CB		CallBack)
{
	geBoolean	bResult	=GE_TRUE;	// TRUE means entire list was processed
	Brush		*b;

	assert(pList != NULL);

	for(b=pList->First;b;b=b->Next)
	{
		assert(b->Type!=BRUSH_CSG);

		if(b->Type==BRUSH_MULTI)
		{
			if(!BrushList_EnumLeafBrushes(b->BList, pVoid, CallBack))
			{
				break;
			}
		}
		else if( (bResult = CallBack( b, pVoid )) == GE_FALSE )
		{
			break;
		}
	}
	return bResult ;
}

//grabs csg brushes and leafs with no children
//the exception being hollows which are all based
//on a parent multi and might contain a hollowcut
int	BrushList_EnumCSGBrushes(const BrushList	*pList,
							  void *			pVoid,
							  BrushList_CB		CallBack)
{
	geBoolean	bResult	=GE_TRUE;	// TRUE means entire list was processed
	Brush		*b;

	assert(pList != NULL);

	for(b=pList->First;b && bResult;b=b->Next)
	{
		switch (b->Type)
		{
			case BRUSH_MULTI :
				bResult = BrushList_EnumCSGBrushes (b->BList, pVoid, CallBack);
				break;
			case BRUSH_LEAF :
				if (b->BList)
				{
					bResult = BrushList_EnumCSGBrushes (b->BList, pVoid, CallBack);
				}
				else
				{
					if(!(b->Flags & (BRUSH_HOLLOW | BRUSH_HOLLOWCUT)))
					{
						bResult = CallBack (b, pVoid);
					}
				}
				break;
			case BRUSH_CSG :
				bResult = CallBack (b, pVoid);
				break;
			default :
				assert (0);		// bad brush type
				bResult = GE_FALSE;
				break;
		}
	}
	return bResult ;
}

geBoolean	Brush_GetParent(const BrushList	*pList,		//list to search
							const Brush		*b,			//brush to find
							Brush			**bParent)	//parent returned
{
	Brush	*b2;

	assert(b);
	assert(pList);
	assert(bParent);

	for(b2=pList->First;b2;b2=b2->Next)

	{
		if(b2==b)
		{
			*bParent	=(Brush *)b;	//const override!
			return		GE_TRUE;
		}

		if(b2->Type==BRUSH_LEAF)
		{
			if(b2->BList)
			{
				if(Brush_GetParent(b2->BList, b, bParent))
				{
					*bParent	=b2;
					return		GE_TRUE;
				}
			}
		}
		else if(b2->Type==BRUSH_MULTI)
		{
			if(Brush_GetParent(b2->BList, b, bParent))
			{
				*bParent	=b2;
				return		GE_TRUE;
			}
		}
	}
	return	GE_FALSE;

}


Brush *	Brush_GetTopLevelParent 
	(
	  const BrushList	*pList,		//list to search
	  const Brush		*b			//brush to find
	)
{
	Brush const *bWork;
	Brush *pImmediateParent;

	bWork = b;

	while (Brush_GetParent (pList, bWork, &pImmediateParent) == GE_TRUE)
	{
		if (bWork == pImmediateParent)
		{
			break;
		}
		bWork = pImmediateParent;
	}
	return (Brush *)bWork;
}


static geBoolean	Brush_SelectMatchingFace(Brush *b, Face *f, Face **pMatchingFace)
{
	Face		*f2;
	const Plane	*p, *p2;
	int			i;

	assert(b);

	p	=Face_GetPlane(f);
	for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
	{
		f2	=FaceList_GetFace(b->Faces, i);
		p2	=Face_GetPlane(f2);

		if(b->Flags & BRUSH_SUBTRACT)
		{
			geVec3d	InvNorm	=p2->Normal;
			geVec3d_Inverse(&InvNorm);

			if(geVec3d_Compare(&p->Normal, &InvNorm, 0.01f))
			{
				if(((p->Dist + p2->Dist) > -0.01f)&&((p->Dist + p2->Dist) < 0.01f))
				{
					Face_SetSelected(f2, GE_TRUE);
					*pMatchingFace = f2;
					return	GE_TRUE;
				}
			}
		}
		else
		{
			if(geVec3d_Compare(&p->Normal, &p2->Normal, 0.01f))
			{
				if(((p->Dist - p2->Dist) > -0.01f)&&((p->Dist - p2->Dist) < 0.01f))
				{
					Face_SetSelected(f2, GE_TRUE);
					*pMatchingFace = f2;
					return	GE_TRUE;
				}
			}
		}
	}
	return	GE_FALSE;
}


static geBoolean	BrushList_SelectMatchingCutFace
	(
		const BrushList	*pList,
		const Brush		*b,
		Face			*f,
		Brush			**CutBrush,
		Face			**pMatchingFace
	)
{
	Brush	*cb;

	assert(b);
	assert(CutBrush);
	assert(pList);
	assert(f);

	if(b->Type==BRUSH_LEAF)
	{
		for(cb=pList->Last;cb;cb=cb->Prev)
		{
			if(cb==b)
			{
				break;
			}
			if(Brush_TestBoundsIntersect(b, &cb->BoundingBox))
			{
				if(cb->Type==BRUSH_MULTI)
				{
					if(BrushList_SelectMatchingCutFace(cb->BList, b, f, CutBrush, pMatchingFace))
					{
						return	GE_TRUE;
					}
				}
				else if(cb->Flags & BRUSH_SUBTRACT)
				{
					if(Brush_SelectMatchingFace(cb, f, pMatchingFace))
					{
						*CutBrush	=cb;
						return		GE_TRUE;
					}
				}
			}
		}
	}
	return	GE_FALSE;
}

Brush * BrushList_FindFaceParent (const BrushList *pList, const Face *pFace)
{
	Brush *pBrush;

	for (pBrush = pList->First; pBrush != NULL; pBrush = pBrush->Next)
	{
		switch (pBrush->Type)

		{
			case BRUSH_MULTI :
			{
				Brush *pFound;

				pFound = BrushList_FindFaceParent (pBrush->BList, pFace);
				if (pFound != NULL)
				{
					return pFound;
				}
				break;
			}
			case BRUSH_LEAF :
			case BRUSH_CSG :




			{
				int i;
				for(i=0;i < Brush_GetNumFaces(pBrush);i++)


				{
					Face	*pCheckFace;

					pCheckFace = Brush_GetFace (pBrush, i);
					if (pFace == pCheckFace)
					{
						return pBrush;
					}
				}
				break;;
			}
			default :
				assert (0);
				break;
		}
	}
	return NULL;
}

Brush * BrushList_FindTopLevelFaceParent (const BrushList *pList, const Face *pFace)
{
	Brush *bFound;

	bFound = BrushList_FindFaceParent (pList, pFace);
	if (bFound != NULL)
	{
		bFound = Brush_GetTopLevelParent (pList, bFound);
	}
	return bFound;
}

static	geFloat		dists[256];
static	uint8		sides[256];
static	uint8		fsides[256];

enum SideFlags
{
	SIDE_FRONT	=0,
	SIDE_BACK	=1,
	SIDE_ON		=2,
	SIDE_SPLIT	=3
};


//handle cases where two brushes share a coplanar face
static int	Brush_MostlyOnSide(const Brush *b, const Plane *p)
{
	int		i, side;
	geFloat	max;

	max		=0;
	side	=SIDE_FRONT;
	for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
	{
		Face_MostlyOnSide(FaceList_GetFace(b->Faces, i), p, &max, &side);
	}
	return	side;
}

//Split the original brush by the face passed in returning
//the brush in front of and the brush behind the 
//face passed in.  
//front and back brush pointers should be null on entry
void	Brush_SplitByFace(Brush	*ogb,	//original brush
						  Face	*sf,	//split face
						  Brush	**fb,	//front brush
						  Brush	**bb)	//back brush
{
	const Plane	*p;
	int			i;
	uint8		cnt[3], fcnt[4];
	FaceList	*fl, *bl;
	const Face	*f;
	Face		*cpf, *ff, *bf, *midf;
	geBoolean	WasSplit	=GE_FALSE;

	assert(ogb);
	assert(sf);
	assert(fb);
	assert(bb);
	assert(*fb==NULL);
	assert(*bb==NULL);

	p	=Face_GetPlane(sf);

	fcnt[0]=fcnt[1]=fcnt[2]=fcnt[3]=0;

	for(i=0;i < FaceList_GetNumFaces(ogb->Faces);i++)
	{
		f	=FaceList_GetFace(ogb->Faces, i);
		Face_GetSplitInfo(f, p, dists, sides, cnt);
		if(!cnt[SIDE_FRONT] && !cnt[SIDE_BACK])	//coplanar
		{
			fsides[i]	=SIDE_ON;
		}
		else if(!cnt[SIDE_FRONT])	//back
		{
			fsides[i]	=SIDE_BACK;
		}
		else if(!cnt[SIDE_BACK])	//front
		{
			fsides[i]	=SIDE_FRONT;
		}
		else	//split
		{
			fsides[i]	=SIDE_SPLIT;
		}
		fcnt[fsides[i]]++;
	}
	fsides[i]	=fsides[0];

	if(fcnt[SIDE_SPLIT])	//at least one face split
	{
		//clip the split face
		midf	=Face_Clone(sf);
		FaceList_ClipFaceToList(ogb->Faces, &midf);
		if(!midf)
		{
			if(Brush_MostlyOnSide(ogb, Face_GetPlane(sf))==SIDE_FRONT)
			{
				*fb	=Brush_Clone(ogb);
			}
			else
			{
				*bb	=Brush_Clone(ogb);
			}
			return;
		}
		//alloc face lists for front and back
		fl	=FaceList_Create(fcnt[SIDE_FRONT] + fcnt[SIDE_SPLIT] + 1);
		bl	=FaceList_Create(fcnt[SIDE_BACK] + fcnt[SIDE_SPLIT] + 1);

		WasSplit	=GE_FALSE;
		for(i=0;i < FaceList_GetNumFaces(ogb->Faces);i++)
		{
			f	=FaceList_GetFace(ogb->Faces, i);

			switch(fsides[i])
			{
			case	SIDE_FRONT:
				cpf	=Face_Clone(f);
				if(cpf)
				{
					FaceList_AddFace(fl, cpf);
				}
				break;
			case	SIDE_BACK:
				cpf	=Face_Clone(f);
				if(cpf)
				{
					FaceList_AddFace(bl, cpf);
				}
				break;
			case	SIDE_SPLIT:	//this info should be reused from above!!!
				ff=bf=NULL;
				Face_GetSplitInfo(f, p, dists, sides, cnt);
				if(!cnt[SIDE_FRONT] && !cnt[SIDE_BACK])	//coplanar
				{
					assert(0);	//shouldn't happen
				}
				else if(!cnt[SIDE_FRONT])	//back
				{
					assert(0);	//shouldn't happen
//					bf	=Face_Clone(f);
				}
				else if(!cnt[SIDE_BACK])	//front
				{
					assert(0);	//shouldn't happen
//					ff	=Face_Clone(f);
				}
				else	//split
				{
					WasSplit	=GE_TRUE;
					Face_Split(f, p, &ff, &bf, dists, sides);
				}

				if(ff)
				{
					FaceList_AddFace(fl, ff);
				}
				if(bf)
				{
					FaceList_AddFace(bl, bf);
				}
			}
		}
		if(WasSplit)
		{
			//add and clip the split face
//			FaceList_ClipFaceToList(bl, &cpf);
			FaceList_AddFace(bl, midf);

			//flip for front side brush
			cpf	=Face_CloneReverse(midf);
			FaceList_AddFace(fl, cpf);
		}
		*fb	=Brush_CreateFromParent(ogb, fl);
		*bb	=Brush_CreateFromParent(ogb, bl);

		Brush_SealFaces(fb);
		Brush_SealFaces(bb);
	}
	else if(fcnt[SIDE_FRONT] && fcnt[SIDE_BACK])
	{
		if(fcnt[SIDE_FRONT] > fcnt[SIDE_BACK])	//mostly in front
		{
			*fb	=Brush_Clone(ogb);
		}
		else									//mostly behind
		{
			*bb	=Brush_Clone(ogb);
		}
	}
	else if(!fcnt[SIDE_FRONT])		//all faces behind
	{
		*bb	=Brush_Clone(ogb);
	}
	else if(!fcnt[SIDE_BACK])		//all faces front
	{
		*fb	=Brush_Clone(ogb);
	}
}

void BrushList_ClearAllCSG (BrushList *pList)
{
	Brush *b;

	for (b = pList->First; b != NULL; b = b->Next)
	{
		if (b->Type == BRUSH_MULTI)
		{
			BrushList_ClearAllCSG (b->BList);
		}
		else if (b->BList != NULL)
		{
			BrushList_Destroy (&b->BList);
		}
	}
}


//clears anything in the leaf's brush list
void	BrushList_ClearCSGAndHollows(BrushList *inList, int mid)
{
	Brush	*b;

	assert(inList != NULL);

	for(b=inList->First;b;b=b->Next)
	{
		if (b->ModelId == mid)
		{
			if (b->Type == BRUSH_MULTI)
			{
				BrushList_ClearCSGAndHollows (b->BList, mid);
			}
			else if (b->BList != NULL)
			{
				BrushList_Destroy (&b->BList);
			}
		}
	}
}

//regenerates interior cut brushes
void	BrushList_RebuildHollowFaces(BrushList *inList, int mid, Brush_CSGCallback Callback, void *lParam)
{
	Brush			*b;

	assert(inList != NULL);

	for(b=inList->First;b;b=b->Next)
	{
		if (!Callback (b, lParam))
		{
			continue;
		}
		if(b->Type==BRUSH_MULTI)	//recurse multibrushes
		{
			assert(b->BList);

			BrushList_RebuildHollowFaces(b->BList, mid, Callback, lParam);
			continue;
		}
		else if(!(b->Flags & BRUSH_HOLLOW))
		{
			Brush		*bh;

			if(mid==b->ModelId && b->BList)
			{
				BrushList_Destroy(&b->BList);
			}
			if(b->Flags & BRUSH_HOLLOWCUT)
			{
				//hollows are contained in a multi with the brush
				//that generates them
				bh	=Brush_CreateHollowFromBrush(inList->First);
				if(bh)
				{
					BrushList_Append(inList, bh);
					bh->Flags	=b->Flags | BRUSH_SUBTRACT; //make sure this is set 
					bh->Type	=b->Type;
					bh->ModelId	=b->ModelId;
					bh->GroupId	=b->GroupId;
					bh->HullSize=b->HullSize;
					bh->Color	=b->Color;
					if(bh->Name)
					{
						geRam_Free(bh->Name);
					}
					bh->Name	=Util_Strdup(b->Name);
					Brush_CopyFaceInfo(b, bh);
					BrushList_Remove(inList, b);
					Brush_Destroy(&b);
					b	=bh;
				}
				else	//destroy old core, probably was clipped away
				{
					bh	=b->Prev;
					BrushList_Remove(inList, b);
					Brush_Destroy(&b);
					b	=bh;
				}
			}
			continue;
		}
		else if(mid!=b->ModelId)
		{
			continue;
		}
		assert(!b->BList);	//these should be nuked
		assert(b->Type==BRUSH_LEAF);

		if(b->Next)
		{
			if(b->Next->Flags & BRUSH_HOLLOWCUT)
			{
				continue;
			}
		}

		//generate from scratch if theres nothing
		{
			Brush		*bh;

			bh	=Brush_CreateHollowFromBrush(b);
			if(bh)
			{
				Brush_SetHollowCut(bh, GE_TRUE);
				BrushList_Append(inList, bh);
				bh->Flags	=b->Flags&(~BRUSH_HOLLOW);	//clear hollow
				bh->Type	=b->Type;
				bh->ModelId	=b->ModelId;
				bh->GroupId	=b->GroupId;
				bh->HullSize=b->HullSize;
				bh->Color	=b->Color;
				Brush_SetHollowCut(bh, GE_TRUE);
				if(bh->Name)
				{
					geRam_Free(bh->Name);
				}
				bh->Name	=Util_Strdup(b->Name);
			}
		}
	}
}

//cuts b2 by b (b should be a cut brush)
static void	Brush_CutBrush(Brush *b, Brush *b2)
{
	Brush		*cb, *fb, *bb = NULL, *cb2;
	Face		*f, *sf;
	const Plane	*p;
	int			i;

	assert(b);
	assert(b2);
	assert(b2->Type!=BRUSH_CSG);
	assert(b->Flags & BRUSH_SUBTRACT);

	if(b2->Type==BRUSH_LEAF)
	{
		if(b2->BList)
		{
			for(cb=b2->BList->First;cb;)
			{
				if(!Brush_TestBoundsIntersect(b, &cb->BoundingBox))
				{
					cb=cb->Next;
					continue;
				}
				cb2	=cb->Next;
				for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
				{
					f	=FaceList_GetFace(b->Faces, i);
					p	=Face_GetPlane(f);

					//create a new face from the split plane
					sf	=Face_CreateFromPlane(p, BOGUS_RANGE, 0);
					Face_CopyFaceInfo(f, sf);

					fb=bb=NULL;

					//split b by sf, adding the front brush to the brushlist
					Brush_SplitByFace(cb, sf, &fb, &bb);
					if(fb)
					{	//clear hollow for csg
						fb->Flags	&=~(BRUSH_HOLLOW | BRUSH_HOLLOWCUT);
						BrushList_Prepend(b2->BList, fb);
					}
					Face_Destroy(&sf);
					if(!i)
					{
						BrushList_Remove(b2->BList, cb);
					}
					Brush_Destroy(&cb);

					//make the back side brush current
					cb	=bb;
					if(!cb)
						break;
				}
				if(bb)
				{
					Brush_Destroy(&bb);
				}
				cb	=cb2;
			}
		}
		else
		{
			b2->BList	=BrushList_Create();
			cb			=b2;
			for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
			{
				f	=FaceList_GetFace(b->Faces, i);
				p	=Face_GetPlane(f);

				//create a new face from the split plane
				sf	=Face_CreateFromPlane(p, BOGUS_RANGE, 0);
				Face_CopyFaceInfo(f, sf);

				fb=bb=NULL;

				//split b by sf, adding the front brush to the brushlist
				Brush_SplitByFace(cb, sf, &fb, &bb);
				if(fb)
				{	//clear hollow for csg
					fb->Flags	&=~(BRUSH_HOLLOW | BRUSH_HOLLOWCUT);
					BrushList_Append(b2->BList, fb);
				}
				Face_Destroy(&sf);
				if(i)
				{
					Brush_Destroy(&cb);
				}

				//make the back side brush current
				cb	=bb;
				if(!cb)
					break;
			}
			if(bb)
			{
				Brush_Destroy(&bb);
			}
		}
	}
	else
	{
		for(cb=b2->BList->First;cb;cb=cb->Next)
		{
			if(Brush_TestBoundsIntersect(b, &cb->BoundingBox))
			{
				if(!(cb->Flags & BRUSH_SUBTRACT))
				{
					Brush_CutBrush(b, cb);
				}
			}
		}
	}
}

static	Brush	*bstack[8192];	//8192 levels of recursion
static	Brush	**bsp;

static void	BrushList_DoHollowCuts(BrushList *pList, int mid, Brush_CSGCallback Callback, void *lParam)
{
	Brush		*b, *b2, *cb;

	assert(pList != NULL);

	//iterate cuts in reverse list order cutting solids
	for(b=pList->Last;b;b=b->Prev)
	{
		if ((b->ModelId != mid) || (!Callback (b, lParam)))
		{
			continue;
		}

		//recurse to cut with private multicuts
		if(b->BList && b->Type==BRUSH_MULTI)
		{
			BrushList_DoHollowCuts(b->BList, mid, Callback, lParam);
		}

		if(b->Flags & BRUSH_HOLLOWCUT)
		{
			if(b->BList)
			{
				for(cb=b->BList->First;;)	//leaf hollows or multis
				{
					if(!cb)
					{
						if(--bsp >= bstack)
						{
							cb	=(*bsp)->Next;
							continue;
						}
						else
						{
							bsp++;
							break;
						}
					}
					//true subtract flags are always passed on now
					assert(cb->Flags & BRUSH_SUBTRACT);

					if(cb->BList)
					{
						*bsp++	=cb;
						cb		=cb->BList->First;
						continue;
					}

					for(b2=b->Prev;b2;b2=b2->Prev)
					{
						if(b2->ModelId!=mid)
						{
							continue;
						}

						if(Brush_TestBoundsIntersect(cb, &b2->BoundingBox))
						{
							assert(b2->Type!=BRUSH_CSG);
							Brush_CutBrush(cb, b2);
						}
					}
					cb	=cb->Next;
				}
			}
			else
			{
				for(b2=b->Prev;b2;b2=b2->Prev)
				{
					if(b2->ModelId!=mid)
					{
						continue;
					}

					if(Brush_TestBoundsIntersect(b, &b2->BoundingBox))
					{
						assert(b2->Type!=BRUSH_CSG);

						Brush_CutBrush(b, b2);
					}
				}
			}
		}
	}
}

static void	BrushList_DoCuts(BrushList *pList, int mid, Brush_CSGCallback Callback, void *lParam)
{
	Brush		*b, *b2, *cb;

	assert(pList != NULL);

	//iterate cuts in reverse list order cutting solids
	for(b=pList->Last;b;b=b->Prev)
	{
		if ((b->ModelId != mid) || !Callback (b, lParam))
		{
			continue;
		}

		if(b->Flags & BRUSH_SUBTRACT)
		{
			if(b->BList)
			{
				for(cb=b->BList->First;;)	//leaf hollows or multis
				{
					if(!cb)
					{
						if(--bsp >= bstack)
						{
							cb	=(*bsp)->Next;
							continue;
						}
						else
						{
							bsp++;
							break;
						}
					}
					//true subtract flags are always passed on now
					assert(cb->Flags & BRUSH_SUBTRACT);

					if(cb->BList)
					{
						*bsp++	=cb;
						cb		=cb->BList->First;
						continue;
					}

					for(b2=b->Prev;b2;b2=b2->Prev)
					{
						if((b2->Flags & BRUSH_SUBTRACT) ||(b2->ModelId!=mid))
						{
							continue;
						}

						if(Brush_TestBoundsIntersect(cb, &b2->BoundingBox))
						{
							assert(b2->Type!=BRUSH_CSG);
							if(!(cb->Flags & BRUSH_HOLLOWCUT))
							{
								Brush_CutBrush(cb, b2);
							}
						}
					}
					cb	=cb->Next;
				}
			}
			else
			{
				for(b2=b->Prev;b2;b2=b2->Prev)
				{
					if((b2->Flags & BRUSH_SUBTRACT) ||(b2->ModelId!=mid))
					{
						continue;
					}

					if(Brush_TestBoundsIntersect(b, &b2->BoundingBox))
					{
						assert(b2->Type!=BRUSH_CSG);

						Brush_CutBrush(b, b2);
					}
				}
			}
		}
	}
}

//might move this to facelist...
//could get rid of setdirty
void	Brush_SealFaces(Brush **b)
{
	const Plane	*p;
	Face		*f2;
	Face		*f;
	FaceList	*fl;
	int			i, j;
	uint8		cnt[3];

	assert(b != NULL);
	assert(*b != NULL);
	assert((*b)->Type!=BRUSH_MULTI);

	if(FaceList_GetNumFaces((*b)->Faces) < 4)
	{
		Brush_Destroy(b);
		return;
	}
	fl	=FaceList_Create(FaceList_GetNumFaces((*b)->Faces));

	//expand all faces out to seal cracks
	for(i=0;i < FaceList_GetNumFaces((*b)->Faces);i++)
	{
		f	=(Face *)FaceList_GetFace((*b)->Faces, i);
		p	=Face_GetPlane(f);
		f2	=Face_CreateFromPlane(p, BOGUS_RANGE, 0);
		Face_CopyFaceInfo(f, f2);
		FaceList_AddFace(fl, f2);
	}
	for(i=0;i < FaceList_GetNumFaces(fl);i++)
	{
		//const override here
		f	=(Face *)FaceList_GetFace(fl, i);

		for(j=0;j < FaceList_GetNumFaces(fl);j++)
		{
			if(j==i)
			{
				continue;
			}
			f2	=FaceList_GetFace(fl, j);
			p	=Face_GetPlane(f2);

			Face_GetSplitInfo(f, p, dists, sides, cnt);

			if(!cnt[SIDE_FRONT] && !cnt[SIDE_BACK])	//coplanar
			{
				FaceList_RemoveFace(fl, i);
				j--;	//to ensure we restart if this is the last face
				break;
			}
			else if(!cnt[SIDE_FRONT])	//back
			{
				continue;
			}
			else if(!cnt[SIDE_BACK])	//front
			{
				FaceList_RemoveFace(fl, i);
				j--;	//to ensure we restart if this is the last face
				break;
			}
			else	//split
			{
				Face_Clip(f, p, dists, sides);
			}
		}
		if(j < FaceList_GetNumFaces(fl))
		{
			i=-1;	//restart!
		}
	}
	if(FaceList_GetNumFaces(fl) < 4)
	{
		ConPrintf("Overconstrained brush clipped away...\n");
		Brush_Destroy(b);
		FaceList_Destroy(&fl);
	}
	else
	{
		FaceList_Destroy(&(*b)->Faces);
		(*b)->Faces	=fl;
		Brush_Bound((*b));
	}
}

static geBoolean	BrushList_SetNextSelectedFace(BrushList *pList)
{
	Brush	*b;

	assert(pList);

	for(b=pList->First;b;b=b->Next)
	{
		if(Brush_GetSelectedFace(b))
		{
			break;
		}
	}
	if(!b)	//no faces found selected
	{
		Brush_SelectFirstFace(pList->First);	//in case it's also a multi
		return	GE_TRUE;
	}
	for(;b;b=b->Next)
	{
		if(Brush_SetNextSelectedFace(b))
		{
			return	GE_TRUE;
		}
	}
	return	GE_FALSE;	//wrapped around the end... handle outside
}

//finds the selected face and deselects it selecting the next
geBoolean	Brush_SetNextSelectedFace(Brush *b)
{
	assert(b);

	if(Brush_IsMulti(b))
	{
		return	BrushList_SetNextSelectedFace(b->BList);
	}
	else
	{
		return	FaceList_SetNextSelectedFace(b->Faces);
	}
}

static geBoolean	BrushList_SetPrevSelectedFace(BrushList *pList)
{
	Brush	*b;

	assert(pList);

	for(b=pList->Last;b;b=b->Prev)
	{
		if(Brush_GetSelectedFace(b))
		{
			break;
		}
	}
	if(!b)	//no faces found selected
	{
		Brush_SelectLastFace(pList->Last);	//in case it's also a multi
		return	GE_TRUE;
	}
	for(;b;b=b->Prev)
	{
		if(Brush_SetPrevSelectedFace(b))
		{
			return	GE_TRUE;
		}
	}
	return	GE_FALSE;	//wrapped around the end... handle outside
}

geBoolean	Brush_SetPrevSelectedFace(Brush *b)
{
	assert(b);

	if(Brush_IsMulti(b))
	{
		return	BrushList_SetPrevSelectedFace(b->BList);
	}
	else
	{
		return	FaceList_SetPrevSelectedFace(b->Faces);
	}
}

Face	*Brush_SelectFirstFace(Brush *b)
{
	Face *pFace;

	assert(b);

	if(Brush_IsMulti(b))
	{
		return Brush_SelectFirstFace(b->BList->First);
	}
	else
	{
		pFace = FaceList_GetFace (b->Faces, 0);

		Face_SetSelected(pFace, GE_TRUE);
		return pFace;
	}
}

Face	*Brush_SelectLastFace(Brush *b)
{
	assert(b);

	if(Brush_IsMulti(b))
	{
		return Brush_SelectLastFace(b->BList->Last);
	}
	else
	{
		Face *pFace;

		pFace = FaceList_GetFace (b->Faces, FaceList_GetNumFaces (b->Faces)-1);
		Face_SetSelected (pFace, GE_TRUE);
		return pFace;
	}
}

static Face	*BrushList_GetSelectedFace(const BrushList *pList)
{
	Face	*f;
	Brush	*b;

	assert(pList);

	for(f=NULL,b=pList->First;(b && !f);b=b->Next)
	{
		f	=Brush_GetSelectedFace(b);
	}
	return	f;
}

Face	*Brush_GetSelectedFace(const Brush *b)
{
	assert(b);

	if(Brush_IsMulti(b))
	{
		return BrushList_GetSelectedFace(b->BList);
	}
	else
	{
		return	FaceList_GetSelectedFace(b->Faces);
	}
}

const Box3d		*Brush_GetBoundingBox (const Brush *b)
{
	assert (b != NULL);

	return &b->BoundingBox;
}

void	BrushList_DoCSG(BrushList *inList, int mid, Brush_CSGCallback Callback, void *lParam)
{
	assert(inList != NULL);

	bsp	=bstack;

	BrushList_ClearCSGAndHollows(inList, mid);
	BrushList_RebuildHollowFaces(inList, mid, Callback, lParam);
	BrushList_DoHollowCuts(inList, mid, Callback, lParam);

	bsp	=bstack;	//reset this just incase...

	BrushList_DoCuts(inList, mid, Callback, lParam);
}

void	BrushList_GetBounds(const BrushList *BList, Box3d *pBounds)
{
	Box3d	Bounds;
	Box3d	BrushBounds;
	Brush	*b;	

	assert(BList != NULL);
	assert(pBounds != NULL);

	b		=BList->First;
	Brush_Bound(b);
	Bounds	=b->BoundingBox;

	for(;b;b=b->Next)
	{
		Brush_Bound(b);
		BrushBounds	=b->BoundingBox;

		Box3d_Union(&Bounds, &BrushBounds, &Bounds);
	}
	*pBounds	=Bounds;
}

BrushList	*BrushList_Clone(BrushList *inList)
{
	BrushList	*outList;
	Brush		*b, *b2;

	assert(inList != NULL);

	outList	=BrushList_Create();

	for(b=inList->First;b;b=b->Next)
	{
		b2	=Brush_Clone(b);
		BrushList_Append(outList, b2);
	}
	return	outList;
}

void	BrushList_Move(BrushList *pList, const geVec3d *trans)
{
	Brush	*b;

	assert(pList);
	assert(trans);

	for(b=pList->First;b;b=b->Next)
	{
		Brush_Move(b, trans);
	}
}

void	BrushList_Rotate(BrushList *pList, const geXForm3d *pXfmRotate, const geVec3d *pCenter)
{
	Brush	*b;

	assert (pList != NULL);
	assert (pXfmRotate != NULL);
	assert (pCenter != NULL);

	for(b=pList->First;b;b=b->Next)
	{
		Brush_Rotate(b, pXfmRotate, pCenter);
	}
}

void	BrushList_Transform(BrushList *pList, const geXForm3d *pXfm)
{
	Brush	*b;

	assert (pList != NULL);
	assert (pXfm != NULL);

	for(b=pList->First;b;b=b->Next)
	{
		Brush_Transform(b, pXfm);
	}
}

void	BrushList_Scale (BrushList *pList, float ScaleFactor)
{
	Brush *b;
	for (b = pList->First; b != NULL; b=b->Next)
	{
		Brush_Scale (b, ScaleFactor);
	}
}
		
void	BrushList_Scale3d(BrushList *pList, const geVec3d *trans)
{
	Brush	*b;

	assert(pList);
	assert(trans);

	for(b=pList->First;b;b=b->Next)
	{
		Brush_Scale3d(b, trans);
	}
}

void	BrushList_Shear(BrushList *pList, const geVec3d *ShearVec, const geVec3d *ShearAxis)
{
	Brush	*b;

	assert(pList);
	assert(ShearVec);
	assert(ShearAxis);

	for(b=pList->First;b;b=b->Next)
	{
		Brush_Shear(b, ShearVec, ShearAxis);
	}
}

geBoolean BrushList_Write (BrushList *BList, FILE *ofile)
{
	Brush *pBrush;
	BrushIterator bi;
	int Count;

	Count = BrushList_Count (BList, (BRUSH_COUNT_MULTI | BRUSH_COUNT_LEAF | BRUSH_COUNT_NORECURSE));
	if (fprintf (ofile, "Brushlist %d\n", Count) < 0) return GE_FALSE;

	pBrush = BrushList_GetFirst (BList, &bi);
	while (pBrush != NULL)
	{
		if (!Brush_Write(pBrush, ofile)) return GE_FALSE;
		pBrush = BrushList_GetNext (&bi);
	}
	return GE_TRUE;
}

//updates face flags and texinfos in children brushes
static void	Brush_UpdateChildFacesRecurse(Brush *b, Brush *bp)
{
	Brush		*cb;
	Face		*f, *f2;
	const Plane	*p, *p2;
	int			i, j;
	geBoolean	Update;

	assert(b);

	if(b->Type==BRUSH_LEAF)
	{
		if(b->Flags & BRUSH_SUBTRACT)	//find cuts
		{
			if(!bp)
			{
				bp	=b;
			}
			for(cb=bp->Prev;;)
			{
				if(!cb)
				{
					if(--bsp >= bstack)
					{
						cb	=(*bsp)->Prev;
						continue;
					}
					else
					{
						bsp++;
						break;
					}
				}
				if(!(cb->Flags & BRUSH_SUBTRACT))
				{
					if(Brush_TestBoundsIntersect(b, &cb->BoundingBox))
					{
						if(cb->BList)
						{
							*bsp++	=cb;
							cb		=cb->BList->Last;
							continue;
						}

						Update	=GE_FALSE;
						for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
						{
							f		=FaceList_GetFace(b->Faces, i);
							p		=Face_GetPlane(f);
							for(j=0;j < FaceList_GetNumFaces(cb->Faces);j++)
							{
								geVec3d	v;
								f2	=FaceList_GetFace(cb->Faces, j);
								p2	=Face_GetPlane(f2);
								v	=p->Normal;

								geVec3d_Inverse(&v);

								if(geVec3d_Compare(&v, &p2->Normal, 0.01f))
								{
									if(fabs (-p->Dist - p2->Dist) < 0.01f)
									{
										Face_CopyFaceInfo(f, f2);
										Update	=GE_TRUE;
									}
								}
							}
						}
					}
				}
				cb	=cb->Prev;
			}
		}
		else if(b->BList)
		{
			for(cb=b->BList->First;cb;cb=cb->Next)
			{
				for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
				{
					f	=FaceList_GetFace(b->Faces, i);
					p	=Face_GetPlane(f);
					for(j=0;j < FaceList_GetNumFaces(cb->Faces);j++)
					{
						f2	=FaceList_GetFace(cb->Faces, j);
						p2	=Face_GetPlane(f2);

						if(geVec3d_Compare(&p->Normal, &p2->Normal, 0.01f))
						{
							if(fabs (p->Dist - p2->Dist) < 0.01f)
							{
								Face_CopyFaceInfo(f, f2);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		if(bp)
		{
			for(cb=b->BList->First;cb;cb=cb->Next)
			{
				Brush_UpdateChildFacesRecurse(cb, bp);
			}
		}
		else
		{
			if(b->Flags & BRUSH_SUBTRACT)
			{
				//only pass down parents that need to affect
				//the list above... like in the case of a multicut
				for(cb=b->BList->First;cb;cb=cb->Next)
				{
					Brush_UpdateChildFacesRecurse(cb, b);
				}
			}
			else
			{
				//cuts should be localized if the entire
				//parent brush isn't a cut
				for(cb=b->BList->First;cb;cb=cb->Next)
				{
					Brush_UpdateChildFacesRecurse(cb, NULL);
				}
			}
		}
	}
}

//sets up stack recurse
void	Brush_UpdateChildFaces(Brush *b)
{
	assert(b);

	bsp	=bstack;

	Brush_UpdateChildFacesRecurse(b, NULL);
}

void	Brush_SetFaceListDirty(Brush *b)
{
	assert(b != NULL);
	FaceList_SetDirty(b->Faces);
}

Brush	*Brush_GetNextBrush(Brush *b, BrushList *pList)
{
	assert(b);
	assert(pList);

	if(b->Next)
	{
		return	b->Next;
	}
	else
	{
		return	pList->First;
	}
}

Brush	*Brush_GetPrevBrush(Brush *b, BrushList *pList)
{
	assert(b);
	assert(pList);

	if(b->Prev)
	{
		return	b->Prev;
	}
	else
	{
		return	pList->Last;
	}
}


static void BrushList_SetTextureScale (BrushList *pList, geFloat ScaleVal)
{
	BrushIterator bi;
	Brush *pBrush;

	pBrush = BrushList_GetFirst (pList, &bi);
	while (pBrush != NULL)
	{
		Brush_SetTextureScale (pBrush, ScaleVal);
		pBrush = BrushList_GetNext (&bi);
	}
}

// Sets texture scale on all brush faces
void	Brush_SetTextureScale (Brush *b, geFloat ScaleVal)
{
	if (Brush_IsMulti (b))
	{
		BrushList_SetTextureScale (b->BList, ScaleVal);
	}
	else
	{
		int iFace, NumFaces;

		NumFaces = Brush_GetNumFaces (b);
		for (iFace = 0; iFace < NumFaces; ++iFace)
		{
			Face	*f	= Brush_GetFace (b, iFace);
			Face_SetTextureScale (f, ScaleVal, ScaleVal);
		}
	}
}


//keeps opposite corner fixed
void	Brush_ShearFixed(Brush *b, float dx, float dy, int sides, int inidx, geVec3d *fnscale, int *ScaleNum)
{
	geVec3d	FixOrg, BrushOrg, ScaleVec, ShearAxis;

	assert(b);
	assert(fnscale);
	assert(ScaleNum);

	if(!(((sides&3)==0) ^ ((sides&0x0c)==0)))
	{
		return;
	}
	if(inidx==1)
	{
		dx	=-dx;
		dy	=-dy;
	}

	geVec3d_Add(&b->BoundingBox.Min, &b->BoundingBox.Max, &BrushOrg);
	geVec3d_Scale(&BrushOrg, 0.5f, &BrushOrg);

	//find the corner of the bounds to keep fixed
	VectorToSUB(FixOrg, inidx)			=0.0f;
	if((sides&3)==0)	//center x
	{
		dx=-dx;
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(BrushOrg, axidx[inidx][0]);
	}
	else if((sides&3)==2)	//less x
	{
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][0]);
	}
	else if((sides&3)==1)	//greater x
	{
		dx=-dx;
		VectorToSUB(FixOrg, axidx[inidx][0])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}

	if((sides&0x0c)==0)	//center y
	{
		VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(BrushOrg, axidx[inidx][1]);
	}
	else if((sides&0x0c)==4)	//less y
	{
		dy=-dy;
		if(inidx!=1)
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
	}
	else if((sides&0x0c)==8)	//greater y
	{
		if(inidx!=1)
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
		}
		else
		{
			VectorToSUB(FixOrg, axidx[inidx][1])	=VectorToSUB(b->BoundingBox.Min, axidx[inidx][1]);
		}
	}

	geVec3d_Clear(&ShearAxis);

	//translate to fixed origin
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	if((sides&3)==0)	//center x
	{
		dx	=0;
		VectorToSUB(ShearAxis, axidx[inidx][0])	=
			1.0f / VectorToSUB(b->BoundingBox.Max, axidx[inidx][0]);
	}
	if((sides&0x0c)==0)	//center y
	{
		dy	=0;
		VectorToSUB(ShearAxis, axidx[inidx][1])	=
			1.0f / VectorToSUB(b->BoundingBox.Max, axidx[inidx][1]);
	}

	VectorToSUB(ScaleVec, inidx)			=0.0f;
	VectorToSUB(ScaleVec, axidx[inidx][0])	=dx;
	VectorToSUB(ScaleVec, axidx[inidx][1])	=dy;

	geVec3d_Add(fnscale, &ScaleVec, fnscale);

	(*ScaleNum)++;

	Brush_Shear(b, fnscale, &ShearAxis);

	//translate back
	geVec3d_Inverse(&FixOrg);
	Brush_Move(b, &FixOrg);

	Brush_Bound(b);
}

void	Brush_SnapShearNearest(Brush *b, geFloat gsize, int sides, int inidx, int snapside)
{
	int				i, incr;
	geVec3d			dmin, vsnap, sbound, SnapEdge, FixOrg, ShearAxis;
	const geFloat	gsizeinv	=1.0f/(geFloat)gsize;
	const Box3d		*bbox, *bbox2;
	Box3d			OGBox;

	assert(b);

	if(!(((sides&3)==0) ^ ((sides&0x0c)==0)))
	{
		return;
	}

	for(incr=0;;incr++)	//accuracy loop with a safety counter
	{
		bbox	=Brush_GetBoundingBox(b);
		OGBox	=*bbox;	//grab a copy for checking to result

		FixOrg.X=(snapside & 2)?	bbox->Min.X : (snapside & 1)?	bbox->Max.X : 0;
		FixOrg.Y=(snapside & 8)?	bbox->Min.Y : (snapside & 4)?	bbox->Max.Y : 0;
		FixOrg.Z=(snapside & 32)?	bbox->Min.Z : (snapside & 16)?	bbox->Max.Z : 0;
		dmin.X	=(snapside & 1)?	bbox->Min.X : (snapside & 2)?	bbox->Max.X : 0;
		dmin.Y	=(snapside & 4)?	bbox->Min.Y : (snapside & 8)?	bbox->Max.Y : 0;
		dmin.Z	=(snapside & 16)?	bbox->Min.Z : (snapside & 32)?	bbox->Max.Z : 0;

		if(!(sides & 12))
		{
			if(sides & 1)
			{
				VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(bbox->Min, axidx[inidx][0]);
				VectorToSUB(FixOrg, axidx[inidx][0])=VectorToSUB(bbox->Max, axidx[inidx][0]);
			}
			else
			{
				VectorToSUB(dmin, axidx[inidx][0])	=VectorToSUB(bbox->Max, axidx[inidx][0]);
				VectorToSUB(FixOrg, axidx[inidx][0])=VectorToSUB(bbox->Min, axidx[inidx][0]);
			}
		}
		else if(!(sides & 3))	//check for top view (inidx==1)
		{
			if(((sides & 4) && inidx!=1) || (!(sides & 4) && inidx==1))
			{
				VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(bbox->Max, axidx[inidx][1]);
				VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(bbox->Min, axidx[inidx][1]);
			}
			else
			{
				VectorToSUB(dmin, axidx[inidx][1])	=VectorToSUB(bbox->Min, axidx[inidx][1]);
				VectorToSUB(FixOrg, axidx[inidx][1])=VectorToSUB(bbox->Max, axidx[inidx][1]);
			}
		}

		geVec3d_Scale(&dmin, gsizeinv, &sbound);	//get ratio to grid
		for(i=0;i<3;i++)
		{
			//get amount off in grid space
			VectorToSUB(vsnap, i)	=(geFloat)Units_Trunc(VectorToSUB(sbound, i));
		}
		geVec3d_Subtract(&sbound, &vsnap, &sbound);	//delta gridspace error
		geVec3d_Scale(&vsnap, gsize, &vsnap);		//return to worldspace

		//move to the nearest corner
		for(i=0;i<3;i++)
		{
			if(VectorToSUB(sbound, i) > 0.5f)
			{
				VectorToSUB(vsnap, i)	+=gsize;
			}
			else if(VectorToSUB(sbound, i) < -0.5f)
			{
				VectorToSUB(vsnap, i)	-=gsize;
			}
		}

		geVec3d_Clear(&ShearAxis);
		geVec3d_Clear(&SnapEdge);

		//translate to fixed origin
		geVec3d_Inverse(&FixOrg);
		Brush_Move(b, &FixOrg);

		geVec3d_Add(&dmin, &FixOrg, &dmin);
		geVec3d_Add(&vsnap, &FixOrg, &vsnap);

		//look for an axis crossover
		for(i=0;i<2;i++)
		{
			if(!VectorToSUB(dmin, axidx[inidx][i]))
			{
				int	sidemask	=((1 + axidx[inidx][i])|((1 + axidx[inidx][i])<<1)<<axidx[inidx][i]);

#ifdef _DEBUG
				ConPrintf("Axis crossover detected...\n");
#endif

				//make sure this is a snapping axis
				if(snapside & sidemask)
				{
					//check for both sides set
					if((snapside & sidemask)==sidemask)
					{
						snapside	&=~((1 + axidx[inidx][i])<<axidx[inidx][i]);
						break;
					}
					else
					{
						//flip this axis's snapside and restart
						snapside	=(snapside&sidemask)^sidemask;
						break;
					}
				}
			}
		}
		if(i!=2)
		{
			//translate back
			geVec3d_Inverse(&FixOrg);
			Brush_Move(b, &FixOrg);
			if(incr > 22)
			{
				ConPrintf("Please inform Kenneth you got this message!\n");
				return;
			}
			continue;
		}

		if((sides&3)==0)	//center x
		{
			VectorToSUB(ShearAxis, axidx[inidx][0])	=1.0f/VectorToSUB(dmin, axidx[inidx][1]);
			VectorToSUB(SnapEdge, axidx[inidx][1])	=1.0f;
			VectorToSUB(dmin, axidx[inidx][1])	=0.0f;
		}
		if((sides&0x0c)==0)	//center y
		{
			VectorToSUB(ShearAxis, axidx[inidx][1])	=1.0f/VectorToSUB(dmin, axidx[inidx][0]);
			VectorToSUB(SnapEdge, axidx[inidx][0])	=1.0f;
			VectorToSUB(dmin, axidx[inidx][0])	=0.0f;
		}
		for(i=0;i<3;i++)
		{
			if(VectorToSUB(dmin, i))
			{
				geVec3d_Scale(&SnapEdge, (VectorToSUB(vsnap, i) - VectorToSUB(dmin, i)), &SnapEdge);
			}
		}

		//loop breaker
		if(geVec3d_Compare(&SnapEdge, &VecOrigin, 0.0001f) || incr > 20)
		{
			//translate back
			geVec3d_Inverse(&FixOrg);
			Brush_Move(b, &FixOrg);
			if(incr > 20)
			{
				ConPrintf("WARNING:  Shear snap inaccurate!\n");
			}
			break;
		}

		Brush_Shear(b, &SnapEdge, &ShearAxis);

		//translate back
		geVec3d_Inverse(&FixOrg);
		Brush_Move(b, &FixOrg);

		//test if the snap did anything to the bounds
		//on the correct side... If a crossover happens
		//less than the size of the grid, it can't be detected above
		bbox2	=Brush_GetBoundingBox(b);
		if(snapside & 1)
		{
			if(OGBox.Min.X == bbox2->Min.X)
			{
				snapside	^=3;
			}
		}
		else if(snapside & 2)
		{
			if(OGBox.Max.X == bbox2->Max.X)
			{
				snapside	^=3;
			}
		}
		if(snapside & 4)
		{
			if(OGBox.Min.Y == bbox2->Min.Y)
			{
				snapside	^=12;
			}
		}
		else if(snapside & 8)
		{
			if(OGBox.Max.Y == bbox2->Max.Y)
			{
				snapside	^=12;
			}
		}
		if(snapside & 16)
		{
			if(OGBox.Min.Z == bbox2->Min.Z)
			{
				snapside	^=48;
			}
		}
		else if(snapside & 32)
		{
			if(OGBox.Max.Z == bbox2->Max.Z)
			{
				snapside	^=48;
			}
		}
	}
}

Brush	*Brush_CreateHollowFromBrush(const Brush *b)
{
	Face		*f, *sf;
	Plane		ExpandPlane;
	const Plane	*p;
	geVec3d		pnt;
	FaceList	*fl;
	int			i;
	Brush		*b2;

	assert(b);

	fl	=FaceList_Create(FaceList_GetNumFaces(b->Faces));

	for(i=0;i < FaceList_GetNumFaces(b->Faces);i++)
	{
		f	=FaceList_GetFace(b->Faces, i);
		p	=Face_GetPlane(f);

		//reverse plane and move inward by hullsize 
		//newdist =dot(((normal * -hull) + (normal * dist)), normal)
		if(Face_IsFixedHull(f))
		{
			ExpandPlane.Normal	=p->Normal;
			ExpandPlane.Dist	=p->Dist;
		}
		else
		{
			geVec3d_Scale(&p->Normal, -b->HullSize, &pnt);
			geVec3d_MA(&pnt, p->Dist, &p->Normal, &pnt);

			ExpandPlane.Normal	=p->Normal;
			ExpandPlane.Dist	=geVec3d_DotProduct(&ExpandPlane.Normal, &pnt);
		}
		//create a face from the inner plane
		sf	=Face_CreateFromPlane(&ExpandPlane, BOGUS_RANGE, 0);
		Face_CopyFaceInfo(f, sf);

		FaceList_AddFace(fl, sf);
	}
	b2	=Brush_Create(BRUSH_LEAF, fl, NULL);
	if(b2)
	{
		Brush_SealFaces(&b2);
	}

	return	b2;
}

//a necessary fixup after a load or convert to hollow...
//converts leaf hollows into multis
void	BrushList_MakeHollowsMulti(BrushList *inList)
{
	Brush		*b, *cb, *b2;

	assert(inList != NULL);

	for(b=inList->First;b;b=b->Next)
	{
		if(b->Type==BRUSH_MULTI)	//recurse multibrushes
		{
			assert(b->BList);

			BrushList_MakeHollowsMulti(b->BList);
			continue;
		}
		if(b->Flags & BRUSH_HOLLOW)
		{
			BrushList	*bl;

			if(b->Next)	//check for additional brushes, or hollowcuts
			{
				if(b->Next->Flags & BRUSH_HOLLOWCUT)
				{
					continue;	//valid
				}
			}
			//make this a multi
			bl	=BrushList_Create();
			b2	=b->Prev;

			BrushList_Remove(inList, b);
			BrushList_Append(bl, b);

			cb	=Brush_Create(BRUSH_MULTI, NULL, bl);
			if(b2)
			{
				BrushList_InsertAfter(inList, b2, cb);
			}
			else
			{
				BrushList_Prepend(inList, cb);
			}
			b	=cb;
		}	
	}
}

void Brush_WriteToQuakeMap(Brush const *b, FILE *ofile)
{
	assert(b != NULL);
	assert(b->Faces != NULL);
	assert(ofile);
	assert(!(b->Flags & BRUSH_HOLLOW));	//only send convex
	assert(!(b->Flags & BRUSH_SUBTRACT));	//only send convex

	// write flags
	FaceList_WriteToQuakeMap(b->Faces, ofile);
}


void	Brush_EnumFaces (Brush *b, void *lParam, Brush_FaceCallback Callback)
{
	switch (b->Type)
	{
		case BRUSH_MULTI :
		{
			Brush *pBrush;

			pBrush = b->BList->First;
			while (pBrush != NULL)
			{
				Brush_EnumFaces (pBrush, lParam, Callback);
				pBrush = pBrush->Next;
			}
			break;
		}

		case BRUSH_LEAF :
		{
			int NumFaces, iFace;

			NumFaces = Brush_GetNumFaces (b);
			for (iFace = 0; iFace < NumFaces; ++iFace)
			{
				Face *pFace = Brush_GetFace (b, iFace);
 				Callback (pFace, lParam);
			}
			Brush_UpdateChildFaces (b);
			break;
		}

		default :
			break;
	}
}

