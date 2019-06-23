/****************************************************************************************/
/*  model.h                                                                             */
/*                                                                                      */
/*  Author:       Jim Mischel, Ken Baird, Jeff Lomax                                    */
/*  Description:  Genesis world editor header file                                      */
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
#ifndef MODEL_H
#define MODEL_H

#include <stdio.h>
#include "brush.h"
#include "motion.h"
#include "parse3dt.h"
#include "list.h"
#include "selbrushlist.h"

#ifdef __cplusplus
	extern "C" {
#endif

typedef List ModelList;
typedef struct tag_Model Model;
typedef ListIterator ModelIterator;

struct ModelInfo_Type
{
	int CurrentModel;
	ModelList *Models;
};

// individual model operations
void Model_CopyStuff
	(
	  Model *pDest,
	  Model const *pSrc
	);

Model *Model_Clone 
	(
	  Model *OldModel
	);

geMotion *Model_GetMotion
	(
	  Model const *pModel
	);

gePath *Model_GetPath
	(
	  Model *pModel
	);

void Model_AddKeyframe
	(
	  Model *pModel,
	  geFloat Time,
	  geXForm3d const *pXfm
	);

void Model_DeleteKeyframe
	(
	  Model *pModel,
	  geFloat Time
	);

void Model_AddEvent
	(
	  Model *pModel,
	  geFloat Time,
	  char const *EventString
	);

void Model_DeleteEvent
	(
	  Model *pModel,
	  geFloat Time
	);

geXForm3d const * Model_GetObjectToWorldXfm
	(
	  Model const *pModel
	);

void Model_UpdateOrigin 
	(
	  Model *pModel, 
	  int MoveRotate, 
	  geVec3d const *pVecDelta
	);

void Model_Transform
	(
	  Model const *pModel,
	  const geXForm3d *pXfm,
	  BrushList *pList
	);

void Model_TransformFromTo
	(
	  Model const *pModel, 
	  geXForm3d const *pXfmFrom,
	  geXForm3d const *pXfmTo,
	  BrushList *pList
	);

void Model_GetCurrentPos
	(
	  Model const *pModel,
	  geVec3d *pVecPos
	);

int Model_GetId
	(
	  Model const *pModel
	);

const char *Model_GetName
	(
	  Model const *pModel
	);

void Model_SetName
	(
	  Model *pModel,
	  const char *pName
	);

void Model_GetCenter
	(
	  Model const *pModel,
	  BrushList *pList,
	  geVec3d *pVecCenter
	);

void Model_AddBrushes
	(
	  Model *pModel,
	  SelBrushList *pSelList
	);

void Model_RemoveBrushes
	(
	  Model *pModel,
	  SelBrushList *pSelList
	);

// gets and sets for the model draw tree
Node *Model_GetModelTree
	(
	  Model const *pModel
	);

void Model_SetModelTree
	(
	  Model *pModel,
	  Node *n
	);

// determine if all of the model's brushes are selected
geBoolean Model_IsSelected
	(
	  Model const *pModel,
	  SelBrushList *pSelList,
	  BrushList *pList
	);

geBoolean Model_IsLocked
	(
	  Model const *pModel
	);

void Model_SetLock
	(
	  Model *pModel,
	  geBoolean Locked
	);

geBoolean Model_IsRotationLocked
	(
	  Model const *pModel
	);

void Model_SetRotationLock
	(
	  Model *pModel,
	  geBoolean Locked
	);

geBoolean Model_IsAnimating
	(
	  Model const *pModel
	);

void Model_SetAnimating
	(
	  Model *pModel, 
	  geBoolean Anim
	);

geFloat Model_GetCurrentKeyTime
	(
	  Model *pModel
	);

geBoolean Model_GetKeyframe
	(
	  Model const *pModel,
	  geFloat Time,
	  geXForm3d *pXfm
	);


int Model_GetNumKeys
	(
	  Model const *pModel
	);


void Model_SetCurrentKeyTime
	(
	  Model *pModel,
	  geFloat Key
	);

void Model_Scale
	(
	  Model *pModel,
	  geFloat ScaleFactor
	);

// List operations
ModelList *ModelList_Create
	(
	  void
	);

void ModelList_Destroy
	(
	  ModelList **ppModel
	);

// Creates a model from all of the selected brushes
geBoolean ModelList_Add
	(
	  ModelList *pList,
	  char const *Name,
	  SelBrushList *pSelList
	);

Model *ModelList_AddFromBrushList
	(
	  ModelList *pList,
	  char const *Name,
	  BrushList *pBrushList
	);

void ModelList_AddModel
	(
	  ModelList *pList,
	  Model *pModel
	);

void ModelList_Remove
	(
	  ModelList *pList,
	  int ModelId,
	  BrushList *BrushList
	);

Model *ModelList_FindById
	(
	  ModelList const *pList,
	  int ModelId
	);

Model *ModelList_FindByName
	(
	  ModelList const *pList,
	  char const *Name
	);

int ModelList_GetCount
	(
	  ModelList const *pList
	);

Model *ModelList_GetAnimatingModel
	(
	  ModelList const *pList
	);

geBoolean ModelList_Write
	(
	  ModelList const *pList,
	  FILE *f
	);

geBoolean ModelList_Read
	(
	  ModelList *pList,
	  int nModels,		// so we know how many to read
	  Parse3dt *Parser,
	  int VersionMajor,
	  int VersionMinor,
	  const char **Expected
	);

geBoolean ModelList_WriteToMap
	(
	  ModelList const *pList,
	  FILE *f,
	  BrushList const *BrushList,
	  geBoolean SuppressHidden,
	  geBoolean VisDetail
	);

void ModelList_ScaleAll
	(
	  ModelList *pList,
	  geFloat ScaleFactor
	);

Model *ModelList_GetFirst
	(
	  ModelList const *pList,
	  ModelIterator *mi
	);

Model *ModelList_GetNext
	(
	  ModelList const *pList,
	  ModelIterator *mi
	);

void ModelList_Collapse 
	(
	  ModelList *pList,
	  int StartingModel,
	  BrushList *Brushes
	);

#ifdef __cplusplus
	}
#endif

#endif  // __MODEL_H
