/****************************************************************************************/
/*  Model.c                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Fusion editor model management module                                 */
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
#include "model.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include "TypeIO.h"
#include <float.h>
#include "units.h"
#include "RAM.H"
#include "util.h"
#include "node.h"
#include "brush.h"
#include <vfile.h>

// Major/minor versions at which motions were introduced
#define MODEL_MOTION_VERSION_MAJOR 1
#define MODEL_MOTION_VERSION_MINOR 5

typedef struct tag_BrushList BrushList;

static const char MODEL_PATH_NAME[] = "PathInfo";

struct tag_Model
{
	int Id;
	char *Name;
	geMotion *pMotion;
	geXForm3d XfmObjectToWorld;
	geBoolean Locked;
	geFloat CurrentKeyTime;

	// information above here is persistent (i.e. stored in file)
	Node *ModelTree;	// mini bsp tree for the model
	geBoolean Animating;
	geBoolean RotationLocked;
};


static Model *Model_Create
	(
	  int id,
	  char const *Name
	)
{
	Model *pModel;

	pModel = (Model *)geRam_Allocate (sizeof (Model));
	if (pModel != NULL)
	{
		pModel->Id = id;
		pModel->Name = Util_Strdup (Name);
		pModel->pMotion = NULL;
		pModel->ModelTree = NULL;
		geXForm3d_SetIdentity (&(pModel->XfmObjectToWorld));
		pModel->Locked = GE_TRUE;
		pModel->CurrentKeyTime = 0.0f;
		pModel->Animating = GE_FALSE;
		pModel->RotationLocked = GE_FALSE;
	}
	return pModel;
}

void Model_CopyStuff
	(
	  Model *pDest,
	  Model const *pSrc
	)
{
	gePath *pSrcPath;
	gePath *pDestPath;
	int iKey;

	assert (pDest != NULL);
	assert (pSrc != NULL);

	pDest->XfmObjectToWorld = pSrc->XfmObjectToWorld;
	pDest->CurrentKeyTime = pSrc->CurrentKeyTime;
		
	// this will force creation of a path
	pDestPath = Model_GetPath (pDest);
	pSrcPath = Model_GetPath ((Model *)pSrc);

	// now get keys for source model and create in destination
	for (iKey = 0; iKey < gePath_GetKeyframeCount (pSrcPath, GE_PATH_TRANSLATION_CHANNEL); ++iKey)
	{
		geXForm3d XfmXlate;
		geXForm3d XfmRotate;
		geFloat Time;

		// get the rotation element
		gePath_GetKeyframe (pSrcPath, iKey, GE_PATH_ROTATION_CHANNEL, &Time, &XfmRotate);

		// get the translation element
		gePath_GetKeyframe (pSrcPath, iKey, GE_PATH_TRANSLATION_CHANNEL, &Time, &XfmXlate);

		XfmRotate.Translation = XfmXlate.Translation;

		gePath_InsertKeyframe (pDestPath, GE_PATH_ALL_CHANNELS, Time, &XfmRotate);
	}

	pDest->RotationLocked = (Model_GetNumKeys (pDest) > 1);
}

Model *Model_Clone 
	(
	  Model *OldModel
	)
{
	Model *NewModel;

	assert (OldModel != NULL);

	NewModel = Model_Create (OldModel->Id, OldModel->Name);
	if (NewModel != NULL)
	{
		Model_CopyStuff (NewModel, OldModel);
	}

	return NewModel;
}

static void Model_Destroy
	(
	  Model **ppModel
	)
{
	Model *pModel;

	assert (ppModel != NULL);
	assert (*ppModel != NULL);

	pModel = *ppModel;
	if (pModel->Name != NULL)
	{
		geRam_Free (pModel->Name);
	}
	if (pModel->pMotion != NULL)
	{
		geMotion_Destroy (&pModel->pMotion);
	}
	if (pModel->ModelTree != NULL)
	{
		Node_ClearBsp(pModel->ModelTree);
	}

	geRam_Free (pModel);
	*ppModel = NULL;
}

geMotion *Model_GetMotion
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return pModel->pMotion;
}

gePath *Model_GetPath
	(
	  Model *pModel
	)
/*
  Get the path associated with this model.
  Currently, models can have only one path.
  If there's no motion, create it.  If no path, create it.
*/
{
	gePath *pPath;

	assert (pModel != NULL);

	pPath = NULL;

	if (pModel->pMotion == NULL)
	{
		pModel->pMotion = geMotion_Create (GE_TRUE);
	}
	
	if (pModel->pMotion != NULL)
	{
		pPath = geMotion_GetPathNamed (pModel->pMotion, MODEL_PATH_NAME);	// single path, for now
		if (pPath == NULL)
		{
			pPath = gePath_Create (
				GE_PATH_INTERPOLATE_HERMITE, // translational
				GE_PATH_INTERPOLATE_SLERP,	 // rotational
				GE_FALSE);					 // looped flag

			if (pPath != NULL)
			{
				int Index;
				geBoolean rslt;

				rslt = geMotion_AddPath (pModel->pMotion, pPath, MODEL_PATH_NAME, &Index);
				// Path is reference counted, so we have to destroy it.
				gePath_Destroy (&pPath);

				if (rslt != GE_FALSE)
				{
					pPath = geMotion_GetPath (pModel->pMotion, Index);
				}
			}
		}
	}
	return pPath;
}

void Model_AddKeyframe
	(
	  Model *pModel,
	  geFloat Time,
	  geXForm3d const *pXfm
	)
{
	gePath *pPath;

	assert (pModel != NULL);
	assert (pXfm != NULL);

	pPath = Model_GetPath (pModel);
	gePath_InsertKeyframe (pPath, GE_PATH_ALL_CHANNELS, Time, pXfm);
}

void Model_DeleteKeyframe
	(
	  Model *pModel,
	  geFloat Time
	)
{
	gePath *pPath;
	int iKey;
	int KeyframeCount;

	assert (pModel != NULL);

	pPath = Model_GetPath (pModel);


	// Since there's no Path function to get the keyframe for a particular time,
	// we'll iterate the path's keyframes looking for it.  If it doesn't exist,
	// then we'll return GE_FALSE.
	// 
	// Probably should make the search into a utility function at some point.
	KeyframeCount = gePath_GetKeyframeCount (pPath, GE_PATH_ROTATION_CHANNEL);
	for (iKey = 0; iKey < KeyframeCount; ++iKey)
	{
		geFloat kTime;
		geXForm3d XfmRotate;

		gePath_GetKeyframe (pPath, iKey, GE_PATH_ROTATION_CHANNEL, &kTime, &XfmRotate);
		if (fabs (kTime - Time) < 0.0001f)
		{
			gePath_DeleteKeyframe (pPath, iKey, GE_PATH_ALL_CHANNELS);
			return;
		}
	}
}

void Model_AddEvent
	(
	  Model *pModel,
	  geFloat Time,
	  char const *EventString
	)
{
	geMotion *pMotion;

	assert (pModel != NULL);
	assert (EventString != NULL);

	pMotion = Model_GetMotion (pModel);
	assert (pMotion != NULL);		// This has to exist...

	geMotion_InsertEvent (pMotion, Time, EventString);
}

void Model_DeleteEvent
	(
	  Model *pModel,
	  geFloat Time
	)
{
	geMotion *pMotion;

	assert (pModel != NULL);
	
	pMotion = Model_GetMotion (pModel);
	assert (pMotion != NULL);

	geMotion_DeleteEvent (pMotion, Time);
}

int Model_GetId
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return pModel->Id;
}


const char *Model_GetName
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return pModel->Name;
}

void Model_SetName
	(
	  Model *pModel,
	  const char *pName
	)
{
	assert (pModel != NULL);
	assert (pName != NULL);

	if (pModel->Name != NULL)
	{
		geRam_Free (pModel->Name);
	}
	pModel->Name = Util_Strdup (pName);
}


// Returns a pointer to the model's mini bsp tree
Node *Model_GetModelTree
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return pModel->ModelTree;
}


// sets the model's mini bsp tree pointer
void Model_SetModelTree
	(
	  Model *pModel,
	  Node *n
	)
{
	assert (pModel != NULL);

	pModel->ModelTree	=n;
}

// determine if all of the model's brushes are selected
geBoolean Model_IsSelected
	(
	  Model const *pModel,
	  SelBrushList *pSelList,
	  BrushList *pList
	)
{
	int i;
	int SelCount;
	int SelBrushCount;
	Brush *pBrush;

	assert (pModel != NULL);
	assert (pList != NULL);
	
	SelBrushCount = SelBrushList_GetSize (pSelList);
	SelCount = 0;
	// count the number of selected brushes that have this model id
	for (i = 0; i < SelBrushCount; i++)
	{
		pBrush = SelBrushList_GetBrush (pSelList, i);
		if (Brush_GetModelId (pBrush) == pModel->Id)
		{
			++SelCount;
		}
	}

	
	if (SelCount > 0)
	{
		// now count number of model brushes in the entire list...
		int ModelBrushCount;
		Brush *pBrush;
		BrushIterator bi;

		ModelBrushCount = 0;
		pBrush = BrushList_GetFirst (pList, &bi);
		while (pBrush != NULL)
		{
			if (Brush_GetModelId (pBrush) == pModel->Id)
			{
				++ModelBrushCount;
			}
			pBrush = BrushList_GetNext (&bi);
		}
		if (ModelBrushCount == SelCount)
		{
			return GE_TRUE;
		}		
	}
	return GE_FALSE;
}


geBoolean Model_IsLocked
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return pModel->Locked;
}


void Model_SetLock
	(
	  Model *pModel,
	  geBoolean Locked
	)
{
	assert (pModel != NULL);

	pModel->Locked = Locked;
}

int Model_GetNumKeys
	(
	  Model const *pModel
	)
{
	gePath *pPath;
	int nKeys;

	assert (pModel != NULL);

	nKeys = 0;
	pPath = Model_GetPath ((Model *)pModel);
	if (pPath != NULL)
	{
		nKeys = gePath_GetKeyframeCount (pPath, GE_PATH_ROTATION_CHANNEL);
	}
	return nKeys;
}

geBoolean Model_IsRotationLocked
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return (pModel->RotationLocked && (Model_GetNumKeys (pModel) > 1));
}

void Model_SetRotationLock
	(
	  Model *pModel,
	  geBoolean Locked
	)
{
	assert (pModel != NULL);

	pModel->RotationLocked = Locked;
}


geBoolean Model_IsAnimating
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return pModel->Animating;
}

void Model_SetAnimating
	(
	  Model *pModel, 
	  geBoolean Anim
	)
{
	assert (pModel != NULL);

	pModel->Animating = Anim;
}

/*
static void OrientationFromPoints
	(
	  const geVec3d *p0,
	  const geVec3d *p1,
	  const geVec3d *p2,
	  const geVec3d *p3,
	  geXForm3d *XForm
	)
{
	geVec3d	V1, V2, V3;
	geVec3d	Left, Up, In;

	geVec3d_Subtract(p1, p0, &V1);
	geVec3d_Subtract(p2, p0, &V2);
	geVec3d_Subtract(p3, p0, &V3);

	geVec3d_Normalize(&V1);
	geVec3d_Normalize(&V2);
	geVec3d_Normalize(&V3);
	geVec3d_CrossProduct(&V2, &V1, &Up);
	// make sure it's a right-handed coordinate system...
	if	(geVec3d_DotProduct(&Up, &V3) >= 0.0f)
	{
		geVec3d_CrossProduct(&Up, &V1, &Left);
	}
	else
	{
		geVec3d_CrossProduct(&V1, &V2, &Up);
		geVec3d_CrossProduct(&Up, &V2, &Left);
	}

	geVec3d_CrossProduct(&Left, &Up, &In);

	geVec3d_Normalize (&Left);
	geVec3d_Normalize (&Up);
	geVec3d_Normalize (&In);

	geXForm3d_SetFromLeftUpIn(XForm, &Left, &Up, &In);
}
*/

void Model_GetCenter
	(
	  Model const *pModel,
	  BrushList *pList,
	  geVec3d *pVecCenter
	)
{
	// compute geometric center of model brushes
	Box3d Box;
	BrushIterator bi;
	Brush *pBrush;
	geBoolean First;

	assert (pModel != NULL);
	assert (pVecCenter != NULL);

	First = GE_FALSE;
	pBrush = BrushList_GetFirst (pList, &bi);

	while (pBrush != NULL)
	{
		if (Brush_GetModelId (pBrush) == pModel->Id)
		{
			Box3d const *pBrushBox;

			pBrushBox = Brush_GetBoundingBox (pBrush);
			if (!First)
			{
				Box = *pBrushBox;
				First = GE_TRUE;
			}
			else
			{
				// Update bounding box from brush's bounding box
				Box3d_Union (&Box, pBrushBox, &Box);
			}
		}
		pBrush = BrushList_GetNext (&bi);
	}

	Box3d_GetCenter (&Box, pVecCenter);
}

static void Model_ComputeObjectToWorldXfm
	(
	  Model const *pModel,
	  BrushList *pList,
	  geXForm3d *pXfm
	)
/*
  Computes the model's position and orientation relative
  to the world origin.

  This occurs in two parts.  First we compute the geometric
  "center" of the model by averaging the centers of all
  the brushes.

  Then, we pick the first face of the first brush in the model
  compute its orientation, and set the model's orientation from
  that.  The final step is to apply the translation computed
  in step 1 above.
*/
{
	geVec3d Pos;

	assert (pModel != NULL);
	assert (pList != NULL);
	assert (pXfm != NULL);

	Model_GetCenter (pModel, pList, &Pos);

	geXForm3d_SetTranslation (pXfm, Pos.X, Pos.Y, Pos.Z);
}

void Model_Transform
	(
	  Model const *pModel,
	  const geXForm3d *pXfm,
	  BrushList *pList
	)
// apply the given transformation to all brushes in the model
{
	Brush *pBrush;
	BrushIterator bi;

	assert (pModel != NULL);
	assert (pXfm != NULL);

	pBrush = BrushList_GetFirst (pList, &bi);
	while (pBrush != NULL)
	{
		if (Brush_GetModelId (pBrush) == pModel->Id)
		{
			Brush_Transform (pBrush, pXfm);
		}
		pBrush = BrushList_GetNext (&bi);
	}
}

void Model_TransformFromTo
	(
	  Model const *pModel, 
	  geXForm3d const *pXfmFrom,
	  geXForm3d const *pXfmTo,
	  BrushList *pList
	)
{
	geXForm3d XfmDoIt;

	assert (pModel != NULL);
	assert (pXfmFrom != NULL);
	assert (pXfmTo != NULL);
	assert (pList != NULL);

	// back out old transform	
	geXForm3d_Multiply (&(pModel->XfmObjectToWorld), pXfmFrom, &XfmDoIt);
	geXForm3d_GetTranspose (&XfmDoIt, &XfmDoIt);

	// add the new TO matrix
	geXForm3d_Multiply (pXfmTo, &XfmDoIt, &XfmDoIt);

	// ...and world-relative positioning
	geXForm3d_Multiply (&(pModel->XfmObjectToWorld), &XfmDoIt, &XfmDoIt);

	Model_Transform (pModel, &XfmDoIt, pList);
}

void Model_GetCurrentPos
	(
	  Model const *pModel,
	  geVec3d *pVecPos
	)
{
	geXForm3d XfmWork, XfmCurrent;
	geVec3d VecWork;

	assert (pModel != NULL);
	assert (pVecPos != NULL);

	VecWork = pModel->XfmObjectToWorld.Translation;

	// get current position
	Model_GetKeyframe (pModel, pModel->CurrentKeyTime, &XfmCurrent);

	// and build world-relative transform
	geXForm3d_GetTranspose (&(pModel->XfmObjectToWorld), &XfmWork);
	geXForm3d_Multiply (&XfmCurrent, &XfmWork, &XfmWork);
	geXForm3d_Multiply (&(pModel->XfmObjectToWorld), &XfmWork, &XfmWork);

	geXForm3d_Transform (&XfmWork, &VecWork, pVecPos);
}


geXForm3d const * Model_GetObjectToWorldXfm
	(
	  Model const *pModel
	)
{
	assert (pModel != NULL);

	return &(pModel->XfmObjectToWorld);
}


void Model_UpdateOrigin 
	(
	  Model *pModel, 
	  int MoveRotate, 
	  geVec3d const *pVecDelta
	)
{
	geXForm3d *pXfm;

	assert (pModel != NULL);
	assert (pVecDelta != NULL);

	pXfm = &(pModel->XfmObjectToWorld);

	switch (MoveRotate)
	{
		case BRUSH_MOVE :
		{
			// simple translation
			geXForm3d_Translate (pXfm, pVecDelta->X, pVecDelta->Y, pVecDelta->Z);
			break;
		}
		case BRUSH_ROTATE :
		{
			if (Model_IsRotationLocked (pModel))
			{
				geXForm3d XfmRotate;
				geVec3d VecXlate;

				// build rotation transform from angles
				geXForm3d_SetEulerAngles (&XfmRotate, pVecDelta);

				// and apply it
				/*
				  The problem here is that this new rotation has to be
				  applied AFTER the existing rotations.  So I need to
				  back out the current translations, apply this rotation,
				  and then re-apply the translations.
				*/
				VecXlate = pXfm->Translation;
				geVec3d_Clear (&(pXfm->Translation));

				geXForm3d_Multiply (&XfmRotate, pXfm, pXfm);
				pXfm->Translation = VecXlate;
			}
			break;
		}
		default:
			// what did you want to do?
			assert (0);
			break;
	}
}


void Model_AddBrushes
	(
	  Model *pModel,
	  SelBrushList *pSelList
	)
{
	int i;
	int NumBrushes;

	assert (pModel != NULL);

	NumBrushes = SelBrushList_GetSize (pSelList);
	for (i = 0; i < NumBrushes; ++i)
	{
		Brush *pBrush = SelBrushList_GetBrush (pSelList, i);

		Brush_SetModelId (pBrush, pModel->Id);
	}		
}

void Model_RemoveBrushes
	(
	  Model *pModel,
	  SelBrushList *pSelList
	)
{
	int i;
	int NumBrushes;

	assert (pModel != NULL);

	NumBrushes = SelBrushList_GetSize (pSelList);
	for (i = 0; i < NumBrushes; ++i)
	{
		Brush *pBrush = SelBrushList_GetBrush (pSelList, i);
		if (Brush_GetModelId (pBrush) == pModel->Id)
		{
			Brush_SetModelId (pBrush, 0);
		}
	}
}


geFloat Model_GetCurrentKeyTime
	(
	  Model *pModel
	)
{
	assert (pModel != NULL);

	return pModel->CurrentKeyTime;
}

geBoolean Model_GetKeyframe
	(
	  Model const *pModel,
	  geFloat Time,
	  geXForm3d *pXfm
	)
{
	gePath *pPath;
	int iKey;
	int KeyframeCount;

	assert (pModel != NULL);
	assert (pXfm != NULL);

	pPath = Model_GetPath ((Model *)pModel);
	if (pPath == NULL)
	{
		return GE_FALSE;
	}

	// Since there's no Path function to get the keyframe for a particular time,
	// we'll iterate the path's keyframes looking for it.  If it doesn't exist,
	// then we'll return GE_FALSE.

	KeyframeCount = gePath_GetKeyframeCount (pPath, GE_PATH_ROTATION_CHANNEL);
	for (iKey = 0; iKey < KeyframeCount; ++iKey)
	{
		geFloat kTime;
		geXForm3d XfmRotate;

		gePath_GetKeyframe (pPath, iKey, GE_PATH_ROTATION_CHANNEL, &kTime, &XfmRotate);
		if (fabs (kTime - Time) < 0.0001f)
		{
			geXForm3d XfmXlate;

			gePath_GetKeyframe (pPath, iKey, GE_PATH_TRANSLATION_CHANNEL, &kTime, &XfmXlate);
			geXForm3d_Translate (&XfmRotate, XfmXlate.Translation.X, XfmXlate.Translation.Y, XfmXlate.Translation.Z);
			*pXfm = XfmRotate;
			return GE_TRUE;
		}
	}
	return GE_FALSE;
}


void Model_SetCurrentKeyTime
	(
	  Model *pModel,
	  geFloat KeyTime
	)
{
	assert (pModel != NULL);

	pModel->CurrentKeyTime = KeyTime;
}

/*
  The old version of the Genesis libraries did all their I/O through FILE * pointers.
  Since the new version works with geVFile, any engine I/O done by the editor must
  be done through geVFile.  But since we don't have time to rewrite all editor I/O,
  we redirect the FILE * I/O to geVFile.  It's not pretty, but it works.
*/
static geMotion *Model_CreateMotionFromFile (FILE *f, const char *Filename)
{
	geMotion *m = NULL;
	geVFile *vfile;

	vfile = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_DOS, Filename, NULL, GE_VFILE_OPEN_READONLY);
	if (vfile != NULL)
	{
		// get current file position
		const size_t CurrentFilePos = ftell (f);

		// position vfile to that position
		if (geVFile_Seek (vfile, CurrentFilePos, GE_VFILE_SEEKSET) != GE_FALSE)
		{
			long NewFilePos;
			
			// read the motion from that position
			m = geMotion_CreateFromFile (vfile);

			// get new file position and set the FILE * to that position
			geVFile_Tell (vfile, &NewFilePos);

			fseek (f, NewFilePos, SEEK_SET);
		}
		geVFile_Close (vfile);
	}
	return m;
}

// Write motion to memory VFile, then to the FILE *
static geBoolean Model_WriteMotionToFile (geMotion *pMotion, FILE *f)
{
	geVFile_MemoryContext MemContext;
	geVFile *MemSys;
//	geVFile *MemFile;
	geBoolean rslt = GE_FALSE;

	// context is initially NULL for create
	MemContext.Data = NULL;
	MemContext.DataLength = 0;

	MemSys = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_MEMORY, NULL, &MemContext, GE_VFILE_OPEN_CREATE);
	if (MemSys != NULL)
	{
		// write to the memory file
		rslt = geMotion_WriteToFile (pMotion, MemSys);
		if (rslt != GE_FALSE)
		{
			// UpdateContext returns the memory pointer and size
			geVFile_UpdateContext (MemSys, &MemContext, sizeof (MemContext));

			// now write this to the FILE *
			rslt = (fwrite (MemContext.Data, MemContext.DataLength, 1, f) == 1);
		}
		geVFile_Close (MemSys);
	}
	return rslt;
}


static geBoolean Model_Write
	(
	  Model const *m,
	  FILE *f
	)
{
	int IsMotion;
	geBoolean rslt;

	assert (m != NULL);
	assert (f != NULL);

	rslt = GE_TRUE;

	{
		char Name[300];

		Util_QuoteString (m->Name, Name);
		if (fprintf (f, "Model %s\n", Name) < 0) return GE_FALSE;
	}
	if (fprintf (f, "\tModelId %d\n", m->Id) < 0) return GE_FALSE;

	if (fprintf (f, "\tCurrentKeyTime %f\n", m->CurrentKeyTime) < 0) return GE_FALSE;
	if (fprintf (f, "%s", "\tTransform\n") < 0) return GE_FALSE;
	
	if (!TypeIO_WriteXForm3dText (f, &(m->XfmObjectToWorld))) return GE_FALSE;

	/*
	  Determine if we should write motion information.
	  Write motion information only if:
		The motion is non-null
	*/
	if (m->pMotion != NULL)
	{
		IsMotion = 1;
	}
	else
	{
		IsMotion = 0;
	}

	if (fprintf (f, "\tMotion %d\n", IsMotion) < 0) return GE_FALSE;
	if (IsMotion != 0)
	{
		rslt = Model_WriteMotionToFile (m->pMotion, f);
//		rslt = geMotion_WriteToFile (m->pMotion, f);
	}
	return rslt;
}

static Model *Model_CreateFromFile
	(
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	)
{
	char ModelName[500];
	int ModelId;
	Model *m;

	if ((VersionMajor == 1) && (VersionMinor < 19))
	{
		if (!Parse3dt_GetIdentifier (Parser, (*Expected = "Model"), ModelName)) return NULL;
	}
	else
	{
		if (!Parse3dt_GetLiteral (Parser, (*Expected = "Model"), ModelName)) return NULL;
	}
	if (!Parse3dt_GetInt (Parser, (*Expected = "ModelId"), &ModelId)) return NULL;


	// create the model structure
	m = Model_Create (ModelId, ModelName);
	if (m != NULL)
	{
		FILE *f;
		f = Scanner_GetFile (Parser->Scanner);

		if ((VersionMajor > 1) ||
			(VersionMajor == 1) && (VersionMinor >= 7))
		{
			if (!Parse3dt_GetFloat (Parser, (*Expected = "CurrentKeyTime"), &m->CurrentKeyTime)) goto ReadError;
			if (!Parse3dt_GetXForm3d (Parser, (*Expected = "Transform"), &m->XfmObjectToWorld)) goto ReadError;
		}

		if ((VersionMajor > MODEL_MOTION_VERSION_MAJOR) ||
			((VersionMajor == MODEL_MOTION_VERSION_MAJOR) &&
			 (VersionMinor >= MODEL_MOTION_VERSION_MINOR)))
		{
			int IsMotion;
			if (!Parse3dt_GetInt (Parser, (*Expected = "Motion"), &IsMotion)) goto ReadError;
			if (IsMotion != 0)
			{
				m->pMotion = Model_CreateMotionFromFile (f, Scanner_GetFileName (Parser->Scanner));
			}
		}
		if ((VersionMajor == 1) && (VersionMinor < 10))
		{
			Model_Scale (m, Units_CentimetersToEngine (1.0f));
		}

		// calling GetPath will force a path to be created if one doesn't exist.
		Model_GetPath (m);
		{
			int NumKeys;

			NumKeys = Model_GetNumKeys (m);
			if (NumKeys == 0)
			{
				// no keys, add one...
				geXForm3d XfmIdentity;

				geXForm3d_SetIdentity (&XfmIdentity);
				Model_AddKeyframe (m, 0.0f, &XfmIdentity);
			}
			Model_SetRotationLock (m, (NumKeys > 1));
		}
	}
	return m;
ReadError :
	Model_Destroy (&m);
	return NULL;
}


static geMotion *Model_CreateScaledMotion
	(
	  Model const *pModel,
	  geFloat ScaleFactor
	)
{
	geMotion *pFixedMotion;
	geMotion *pMotion;
	geXForm3d XfmObjWorld;

	assert (pModel != NULL);

	pMotion = Model_GetMotion (pModel);

	// get object-to-world transform that's used in transforming keyframes
	// from object space to world space.
	XfmObjWorld = *Model_GetObjectToWorldXfm (pModel);
	geVec3d_Clear (&XfmObjWorld.Translation);

	pFixedMotion = geMotion_Create (GE_TRUE);
	if (pFixedMotion != NULL)
	{
		int Index;
		gePath *pPath;
		gePath *pNewPath;
		int iKey;
		pPath = geMotion_GetPath (pMotion, 0);

		pNewPath = gePath_Create (
			GE_PATH_INTERPOLATE_HERMITE, // translational
			GE_PATH_INTERPOLATE_SLERP,	 // rotational
			GE_FALSE);					 // looped flag

		// add the path to the new motion.
		geMotion_AddPath (pFixedMotion, pNewPath, MODEL_PATH_NAME, &Index);

		// Get the transforms from the original path, scale them,
		// and add them to the new path.
		for (iKey = 0; iKey < gePath_GetKeyframeCount (pPath, GE_PATH_TRANSLATION_CHANNEL); ++iKey)
		{
			geXForm3d XfmXlate;
			geXForm3d XfmRotate;
			geFloat Time;

			// get the rotation element
			gePath_GetKeyframe (pPath, iKey, GE_PATH_ROTATION_CHANNEL, &Time, &XfmRotate);

			// get the translation element
			gePath_GetKeyframe (pPath, iKey, GE_PATH_TRANSLATION_CHANNEL, &Time, &XfmXlate);

			geVec3d_Scale (&XfmXlate.Translation, ScaleFactor, &XfmRotate.Translation);

			// Make the transform relative to the model's current orientation
			{
				geXForm3d XfmWork;

				geXForm3d_GetTranspose (&XfmObjWorld, &XfmWork);
				geXForm3d_Multiply (&XfmRotate, &XfmWork, &XfmRotate);
				geXForm3d_Multiply (&XfmObjWorld, &XfmRotate, &XfmRotate);
			}
	
			gePath_InsertKeyframe (pNewPath, GE_PATH_ALL_CHANNELS, Time, &XfmRotate);
		}
	}
	return pFixedMotion;
}

void Model_Scale
	(
	  Model *pModel,
	  geFloat ScaleFactor
	)
{
	geMotion *pNewMotion;
	
	assert (pModel != NULL);
	assert (ScaleFactor > 0);

	if (pModel->pMotion != NULL)
	{
		pNewMotion = Model_CreateScaledMotion (pModel, ScaleFactor);
		if (pNewMotion != NULL)
		{
			geMotion_Destroy (&pModel->pMotion);
			pModel->pMotion = pNewMotion;
		}
	}
	geVec3d_Scale (&pModel->XfmObjectToWorld.Translation, ScaleFactor, &pModel->XfmObjectToWorld.Translation);
}

typedef struct
{
	geBoolean SuppressHidden;
	geBoolean VisDetail;
	int ModelId;
	int BrushCount;
	BrushList *pList;
} Model_BrushEnumData;


static geBoolean Model_BrushCountCallback
	(
	  Brush *b,
	  void *pVoid
	)
{
	Model_BrushEnumData *pData;

	pData = (Model_BrushEnumData *)pVoid;

	if ((Brush_GetModelId (b) == pData->ModelId) && (!Brush_IsSubtract(b)) &&
		((pData->SuppressHidden == GE_FALSE) || (Brush_IsVisible (b))))
	{
		Brush *pBrush;

		// add it to the list
		pBrush = Brush_Clone (b);
		if (pBrush != NULL)
		{
			BrushList_Append (pData->pList, pBrush);
			++(pData->BrushCount);
		}
	}
	return GE_TRUE;
}

static geBoolean Model_WriteToMap
	(
	  Model const *m,
	  FILE *f,
	  BrushList const *pBList,
	  geBoolean SuppressHidden,
	  geBoolean VisDetail
	)
{
	Brush const *b;
	BrushIterator bi;
	geXForm3d XfmCurrentSave;	
	geXForm3d XfmZero;
	Model_BrushEnumData EnumData;

	assert (m != NULL);
	assert (f != NULL);
	assert (pBList != NULL);

	// get transforms from current to key 0
	Model_GetKeyframe (m, m->CurrentKeyTime, &XfmCurrentSave);
	Model_GetKeyframe (m, 0.0f, &XfmZero);

	// count number of brushes in this model

	EnumData.SuppressHidden = SuppressHidden;
	EnumData.VisDetail = VisDetail;
	EnumData.ModelId = m->Id;
	EnumData.BrushCount = 0;
	EnumData.pList = BrushList_Create ();
	if (EnumData.pList == NULL)
	{
		return GE_FALSE;
	}

	// Build a list of brushes that belong to this model.
	// Count them at the same time.
	BrushList_EnumCSGBrushes (pBList, &EnumData, Model_BrushCountCallback);

	// Transform the brushes to the model's origin
	Model_TransformFromTo (m, &XfmCurrentSave, &XfmZero, EnumData.pList);
	
	// write brush count
	TypeIO_WriteInt (f, EnumData.BrushCount);

	// write the individual brushes from the temporary list
	b = BrushList_GetFirst (EnumData.pList, &bi);
	while (b != NULL)
	{
		Brush_WriteToMap (b, f, VisDetail);
		b = BrushList_GetNext (&bi);
	}

	// destroy model brush list...
	BrushList_Destroy (&EnumData.pList);

	// If the model has an animation path, write that to the file.
	{
		gePath *pPath;
		geBoolean HasMotion;

		pPath = Model_GetPath ((Model *)m);
		HasMotion = ((pPath != NULL) && (gePath_GetKeyframeCount (pPath, GE_PATH_TRANSLATION_CHANNEL) > 1));

		// write motion flag
		TypeIO_WriteInt (f, HasMotion);

		if (HasMotion)
		{
			geMotion *NewMotion;

			/*
			  The editor stores the motion's keys relative to the model's current orientation.
			  The utilities expect the keys to be relative to the world's origin with no rotation.
			  So I need to rotate the keys to match the model's default orientation.
			*/
			NewMotion = geMotion_Create (GE_TRUE);
			if (NewMotion != NULL)
			{
				int Index;
				gePath *pPath;
				gePath *NewPath;
				int iKey;

				NewPath = gePath_Create (
					GE_PATH_INTERPOLATE_HERMITE, // translational
					GE_PATH_INTERPOLATE_SLERP,	 // rotational
					GE_FALSE);					 // looped flag
				geMotion_AddPath (NewMotion, NewPath, "FixedPath", &Index);
				
				// paths are reference counted, so need to destroy it...
				pPath = NewPath;
				gePath_Destroy (&pPath);


				pPath = geMotion_GetPath (m->pMotion, 0);
				// Get the keys from the original path, transform them, and add to the new path
				for (iKey = 0; iKey < gePath_GetKeyframeCount (pPath, GE_PATH_TRANSLATION_CHANNEL); ++iKey)
				{
					geXForm3d XfmXlate;
					geXForm3d XfmRotate;
					geFloat Time;

					// get rotation element
					gePath_GetKeyframe (pPath, iKey, GE_PATH_ROTATION_CHANNEL, &Time, &XfmRotate);
					// get translation element
					gePath_GetKeyframe (pPath, iKey, GE_PATH_TRANSLATION_CHANNEL, &Time, &XfmXlate);

					// Make the transform relative to the model's current orientation
					{
/*
						geXForm3d XfmWork;
						geXForm3d XfmObjWldRot;
						geVec3d *VecX = &(m->XfmObjectToWorld.Translation);

						XfmObjWldRot = m->XfmObjectToWorld;
						geVec3d_Clear (&XfmObjWldRot.Translation);
#if 1
						geXForm3d_Translate (&XfmRotate, -VecX->X, -VecX->Y, -VecX->Z);
						geXForm3d_Multiply (&XfmObjWldRot, &XfmRotate, &XfmRotate);

						geXForm3d_Multiply (&XfmObjWldRot, &XfmXlate, &XfmXlate);
						geXForm3d_Multiply (&XfmXlate, &XfmRotate, &XfmRotate);
#else
						geXForm3d_GetTranspose (&m->XfmObjectToWorld, &XfmWork);
						geXForm3d_Multiply (&XfmRotate, &XfmWork, &XfmRotate);
						geXForm3d_Multiply (&XfmXlate, &XfmRotate, &XfmRotate);
						geXForm3d_Multiply (&m->XfmObjectToWorld, &XfmRotate, &XfmRotate);
#endif
*/
						geXForm3d_Multiply (&XfmXlate, &XfmRotate, &XfmRotate);
						gePath_InsertKeyframe (NewPath, GE_PATH_ALL_CHANNELS, Time, &XfmRotate);
					}
				}

				// Add events from original motion to the new motion
				{
					geFloat StartTime = -100000.0f;
					geFloat EndTime = 100000.0f;
					geFloat EventTime;
					char const *EventString;

					geMotion_SetupEventIterator (m->pMotion, StartTime, EndTime);
					while (geMotion_GetNextEvent (m->pMotion, &EventTime, &EventString) != GE_FALSE)
					{
						geMotion_InsertEvent (NewMotion, EventTime, EventString);
					}
				}
				// write newly-created motion.
				Model_WriteMotionToFile (NewMotion, f);
//				geMotion_WriteToFile (NewMotion, f);
				// Seems like a lot of work just to destroy it here, huh?
				geMotion_Destroy (&NewMotion);
			}
		}
	}

	// Write key/value pairs
	TypeIO_WriteInt (f, 3);	// model
	TypeIO_WriteString (f, "classname");
	TypeIO_WriteString (f, "%Model%");
	TypeIO_WriteString (f, "%name%");
	TypeIO_WriteString (f, m->Name);
	{
		char ValueString[100];
		geVec3d const *pVec;

		// rotation origin
		pVec = &(m->XfmObjectToWorld.Translation);
		sprintf (ValueString, "%f %f %f", pVec->X, pVec->Y, pVec->Z);

		TypeIO_WriteString (f, "origin");
		TypeIO_WriteString (f, ValueString);
	}
	return 1;
}


struct tag_ModelList
{
	Model *First;
};


/////////////// List operations

ModelList *ModelList_Create
	(
	  void
	)
// Create and initialize a model list
{
	return List_Create ();
}

#ifndef NDEBUG
	static geBoolean ModelList_IsInOrder
		(
		  const ModelList *pList
		)
	{
		int LastId;
		Model *pModel;
		ListIterator li;

		LastId = -1;
		pModel = (Model *)List_GetFirst ((ModelList *)pList, &li);
		while (pModel != NULL)
		{
			assert (pModel->Id > 0);
			if (LastId >= pModel->Id)
			{
				return GE_FALSE;
			}
			LastId = pModel->Id;
			pModel = List_GetNext ((ModelList *)pList, &li);
		}
		return GE_TRUE;
	}
#endif

static void ModelList_DestroyCallback (void *pData)
{
	Model_Destroy ((Model **)&pData);
}

void ModelList_Destroy
	(
	  ModelList **ppList
	)
{
	List_Destroy (ppList, ModelList_DestroyCallback);
}

static geBoolean FindById_Callback (void *pData, void *lParam)
{
	Model *m = (Model *)pData;
	int *pId = (int *)lParam;

	return (m->Id == *pId);
}

Model *ModelList_FindById
	(
	  ModelList const *pList,
	  int ModelId
	)
{
    Model *m;
	ListIterator li;

	assert (pList != NULL);

	if (List_Search ((List *)pList, FindById_Callback, &ModelId, (void **)&m, &li))
	{
		return m;
	}
	return NULL;
}

static geBoolean FindByName_Callback (void *pData, void *lParam)
{
	Model *m = (Model *)pData;
	char *Name = (char *)lParam;

	return (stricmp (m->Name, Name) == 0);
}

Model *ModelList_FindByName
	(
	  ModelList const *pList,
	  char const *Name
	)
{
	Model *m;
	ListIterator li;

	assert (pList != NULL);
	assert (Name != NULL);

	if (List_Search ((List *)pList, FindByName_Callback, (void *)Name, (void **)&m, &li))
	{
		return m;
	}
	return NULL;
}


#pragma warning (disable:4100)
static geBoolean IsAnimating_Callback (void *pData, void *lParam)
{
	Model *m = (Model *)pData;

	return (m->Animating);
}
#pragma warning (default:4100)

Model *ModelList_GetAnimatingModel
	(
	  ModelList const *pList
	)
{
	Model *pModel;
	ListIterator li;

	assert (pList != NULL);

	if (List_Search ((List *)pList, IsAnimating_Callback, NULL, (void **)&pModel, &li))
	{
		return pModel;
	}
	return NULL;
}

static geBoolean ClearBrushModelId (Brush *pBrush, void *lParam)
{
	int *pId = (int *)lParam;

	if (Brush_GetModelId (pBrush) == *pId)
	{
		Brush_SetModelId (pBrush, 0);
	}
	return GE_TRUE;
}

void ModelList_Remove
	(
	  ModelList *pList,
	  int ModelId,
	  BrushList *ppBList
	)
{
	Model *m;
	ListIterator li;

	assert (pList != NULL);

	if (List_Search (pList, FindById_Callback, &ModelId, (void **)&m, &li))
	{
		BrushList_Enum (ppBList, &ModelId, ClearBrushModelId);
		List_Remove (pList, li, NULL);
		Model_Destroy (&m);
		assert (ModelList_IsInOrder (pList));
	}
}

static geBoolean FindNextId_Callback (void *pData, void *lParam)
{
	Model *m = (Model *)pData;
	int *pId = (int *)lParam;

	if (m->Id > *pId)
	{
		return GE_TRUE;
	}

	++(*pId);
	return GE_FALSE;
}

static Model *ModelList_AddIt
	(
	  ModelList *pList,
	  char const *Name
	)
{
	Model *pParent;
	Model *NewModel;
	ListIterator li;

	int ModelId;

	assert (pList != NULL);
	assert (Name != NULL);

	ModelId = 1;
	if (!List_Search (pList, FindNextId_Callback, &ModelId, (void **)&pParent, &li))
	{
		pParent = NULL;
	}
	NewModel = Model_Create (ModelId, Name);
	if (NewModel != NULL)
	{
		if (pParent == NULL)
		{
			List_Append (pList, NewModel);
		}
		else
		{
			List_InsertBefore (pList, li, NewModel);
		}
	}
	return NewModel;
}

// Creates a model from all of the selected brushes
geBoolean ModelList_Add
	(
	  ModelList *pList,
	  char const *Name,
	  SelBrushList *pSelList
	)
{
	Model *NewModel;
	BrushList *pBrushList;

	assert (pList != NULL);
	assert (ModelList_IsInOrder (pList));

	NewModel = ModelList_AddIt (pList, Name);
	assert (ModelList_IsInOrder (pList));
	if (NewModel != NULL)
	{
		int ModelId;

		ModelId = Model_GetId (NewModel);
		// build a brush list from the selected list
		// tag all selected brushes with this model id...
		pBrushList = BrushList_Create ();

		{
			int i;
			int NumBrushes;

			NumBrushes = SelBrushList_GetSize (pSelList);
			for (i = 0; i < NumBrushes; i++)
			{
				Brush *pBrush, *pNewBrush;

				pBrush = SelBrushList_GetBrush (pSelList, i);
				Brush_SetModelId (pBrush, ModelId);
				pNewBrush = Brush_Clone (pBrush);
				BrushList_Append (pBrushList, pNewBrush);
			}
		}

		// compute object-to-world transform and save it
		Model_ComputeObjectToWorldXfm (NewModel, pBrushList, &NewModel->XfmObjectToWorld);

		BrushList_Destroy (&pBrushList);

		return GE_TRUE;
	}
	return GE_FALSE;
}

Model *ModelList_AddFromBrushList
	(
	  ModelList *pList,
	  char const *Name,
	  BrushList *pBrushList
	)
{
	Model *NewModel;

	assert (pList != NULL);
	assert (Name != NULL);
	assert (pBrushList != NULL);

	assert (ModelList_IsInOrder (pList));
	NewModel = ModelList_AddIt (pList, Name);
	assert (ModelList_IsInOrder (pList));
	if (NewModel != NULL)
	{
		int ModelId;
		BrushIterator bi;
		Brush *pBrush;

		ModelId = Model_GetId (NewModel);
		pBrush = BrushList_GetFirst (pBrushList, &bi);
		while (pBrush != NULL)
		{
			Brush_SetModelId (pBrush, ModelId);
			pBrush = BrushList_GetNext (&bi);
		}
		Model_ComputeObjectToWorldXfm (NewModel, pBrushList, &NewModel->XfmObjectToWorld);
	}

	return NewModel;
}


int ModelList_GetCount
	(
	  ModelList const *pList
	)
// I guess I could maintain a count, but it seems easier
// to just go through and count 'em here...
{
	Model *pModel;
	ModelIterator mi;
	int Count;

	assert (pList != NULL);

	Count = 0;
	pModel = ModelList_GetFirst (pList, &mi);
	while (pModel != NULL)
	{
		++Count;
		pModel = ModelList_GetNext (pList, &mi);
	}
	return Count;
}

geBoolean ModelList_Write
	(
	  ModelList const *pList,
	  FILE *f
	)
{
	Model const *m;
	ListIterator li;

	assert (pList != NULL);
	assert (f != NULL);

	m = (const Model *)List_GetFirst ((List *)pList, &li);
	while (m != NULL)
	{
		if (!Model_Write (m, f)) return GE_FALSE;

		m = (const Model *)List_GetNext ((List *)pList, &li);
	}
	return GE_TRUE;
}

static geBoolean FindInsertionSpot_Callback (void *pData, void *lParam)
{
	Model *pInList = (Model *)pData;
	Model *pNew = (Model *)lParam;

	return (pInList->Id >= pNew->Id);
}

// Insert a model into the list in ascending order.
static void ModelList_Insert
	(
	  ModelList *pList,
	  Model *pModel
	)
{
	Model *p;	// parent
	ListIterator li;

	if (List_Search (pList, FindInsertionSpot_Callback, pModel, (void **)&p, &li))
	{
		List_InsertBefore (pList, li, pModel);
	}
	else
	{
		List_Append (pList, pModel);
	}
}

void ModelList_AddModel
	(
	  ModelList *pList,
	  Model *pModel
	)
{
	assert (pList != NULL);
	assert (pModel != NULL);

	ModelList_Insert (pList, pModel);
}

// Must ensure that models are inserted into the list in ascending order!
geBoolean ModelList_Read
	(
	  ModelList *pList,
	  int nModels,		// so we know how many to read
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	)
{
	int Count;

	assert (pList != NULL);

	// list end pointer for fast append
	for (Count = 0; Count < nModels; Count++)
	{
		Model *m;

		m = Model_CreateFromFile (Parser, VersionMajor, VersionMinor, Expected);

		if (m == NULL)
		{
			return GE_FALSE;
		}

		ModelList_Insert (pList, m);
	}

	assert (ModelList_IsInOrder (pList));
	return GE_TRUE;
}

geBoolean ModelList_WriteToMap
	(
	  ModelList const *pList,
	  FILE *f,
	  BrushList const * ppBList,
	  geBoolean SuppressHidden,
	  geBoolean VisDetail
	)
{
	Model *m;
	ListIterator li;

	m = List_GetFirst ((List *)pList, &li);
	while (m != NULL)
	{
		Model_WriteToMap (m, f, ppBList, SuppressHidden, VisDetail);
		m = List_GetNext ((List *)pList, &li);
	}
	return GE_TRUE;;
}

void ModelList_ScaleAll
	(
	  ModelList *pList,
	  geFloat ScaleFactor
	)
{
	Model *m;
	ListIterator li;

	assert (pList != NULL);
	assert (ScaleFactor > 0.0f);

	m = List_GetFirst (pList, &li);
	while (m != NULL)
	{
		Model_Scale (m, ScaleFactor);
		m = List_GetNext (pList, &li);
	}
}

Model *ModelList_GetFirst
	(
	  ModelList const *pList,
	  ModelIterator *mi
	)
{
	assert (pList != NULL);
	assert (mi != NULL);

	return (Model *)List_GetFirst ((List *)pList, mi);
}

Model *ModelList_GetNext
	(
	  ModelList const *pList,
	  ModelIterator *mi
	)
{
	assert (pList != NULL);
	assert (mi != NULL);

	return (Model *)List_GetNext ((List *)pList, mi);
}


typedef struct
{
	int OldId;
	int NewId;
} Model_MapEntry;

static geBoolean Model_RenumBrush (Brush *pBrush, void *lParam)
{
	Model_MapEntry *Mappings;
	int i, ModelId;

	Mappings = (Model_MapEntry *)lParam;
	ModelId = Brush_GetModelId (pBrush);

	for (i = 0; Mappings[i].OldId != 0; ++i)
	{
		if (Mappings[i].OldId == ModelId)
		{
			Brush_SetModelId (pBrush, Mappings[i].NewId);
			break;
		}
	}
	return GE_TRUE;
}

void ModelList_Collapse 
	(
	  ModelList *pList,
	  int StartingModel,
	  BrushList *Brushes
	)
{
	int nEntries;

	assert (pList != NULL);
	assert (StartingModel > 0);
	assert (Brushes != NULL);

	nEntries = ModelList_GetCount (pList);
	if (nEntries > 0)
	{
		Model_MapEntry *Mappings;

		Mappings = (Model_MapEntry *)geRam_Allocate ((nEntries + 1) * sizeof (Model_MapEntry));
		if (Mappings != NULL)
		{
			int i;
			Model *m;
			ListIterator li;

			m = (Model *)List_GetFirst (pList, &li);
			i = 0;
			while (m != NULL)
			{
				assert (m->Id != 0);
				Mappings[i].OldId = m->Id;
				Mappings[i].NewId = i+StartingModel;
				m->Id = i+StartingModel;
				++i;
				m = (Model *)List_GetNext (pList, &li);
			}
			Mappings[i].OldId = 0;	// stop sentinel
			Mappings[i].NewId = 0;
			BrushList_Enum (Brushes, Mappings, Model_RenumBrush);

			geRam_Free (Mappings);
		}
	}
}
