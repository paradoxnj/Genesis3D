/****************************************************************************************/
/*  ModeList.h                                                                          */
/*                                                                                      */
/*  Author: Mike Sandige                                                                */
/*  Description:    Builds handy list of available modes                                */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#ifndef MODELIST_H
#define MODELIST_H

#include	"GENESIS.H"

typedef enum ModeList_DriverType 
{
	MODELIST_TYPE_GLIDE,
	MODELIST_TYPE_D3D_PRIMARY,
	MODELIST_TYPE_D3D_SECONDARY,
	MODELIST_TYPE_UNKNOWN,
	MODELIST_TYPE_D3D_3DFX,
	MODELIST_TYPE_SOFTWARE,
} ModeList_DriverType;

typedef enum ModeList_Evaluation
{
	MODELIST_EVALUATED_OK,
	MODELIST_EVALUATED_UNDESIRABLE,
	MODELIST_EVALUATED_TRIED_FAILED,
} ModeList_Evaluation;

typedef	struct ModeList
{
	geDriver * 				Driver;
	geDriver_Mode *			Mode;
	const char *			DriverNamePtr;
	const char *			ModeNamePtr;
	ModeList_DriverType	DriverType;
	int						Width;
	int						Height;
	ModeList_Evaluation	Evaluation;
	geBoolean				InAWindow;
} ModeList;


void      ModeList_Destroy(ModeList *List);
ModeList *ModeList_Create(geEngine *Engine,int *ListLength);

#endif

