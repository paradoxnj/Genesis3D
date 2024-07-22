/****************************************************************************************/
/*  ModeList.c                                                                          */
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
#include <string.h>	// strupr, strncpy
#include <assert.h>

#include "modelist.h"
#include "RAM.H"
#include "Errorlog.h"

#define MODELIST_MAX_NAME (1000)
#define MODELIST_ALLOCATION_BLOCK_SIZE (10)

static char *ModeList_640WindowString="640x480 Window";
static char *ModeList_320WindowString="320x240 Window";
static char *ModeList_800WindowString="800x600 Window";


void	ModeList_Destroy(ModeList *List)
{
	assert( List );
	geRam_Free( List );
}


ModeList *ModeList_Create(geEngine *Engine,int *ListLength)
{
	geDriver_System	*DriverSystem;
	geDriver		*Driver;
	geDriver_Mode	*Mode;
	ModeList		*DriverList=NULL;
	ModeList		*NewDriverList;
	int				 Allocated = 0;
	int				 Needed    = 0;
	const char		*DriverNamePtr;
	const char		*ModeNamePtr;
	char			 DriverName[MODELIST_MAX_NAME];
	char			 ModeName[MODELIST_MAX_NAME];

	*ListLength = 0;
	DriverSystem = geEngine_GetDriverSystem(Engine);
	if	(DriverSystem == NULL)
		{
			geErrorLog_AddString(-1,"AutoSelect: Failed to get driver system",NULL);
			goto ModeList_Exit;
		}
		
	Driver = geDriver_SystemGetNextDriver(DriverSystem, NULL);
	while	(Driver != NULL)
	{
		ModeList *	dinfo;
		ModeList_DriverType DriverType;

		if (geDriver_GetName(Driver, &DriverNamePtr)==GE_FALSE)
			{
				geErrorLog_AddString(-1,"AutoSelect: Failed to get driver name",NULL);
				goto ModeList_Exit;
			}
		strncpy(DriverName,DriverNamePtr,MODELIST_MAX_NAME);
		DriverName[MODELIST_MAX_NAME-1]=0;		// just in case
		_strupr(DriverName);
		if (strstr(DriverName,"D3D")!=NULL)
			{
				/*
				if (  (strstr(DriverName,"3DFX")!=NULL) || (strstr(DriverName,"VOODOO")!=NULL) )
					{
						DriverType = MODELIST_TYPE_D3D_3DFX;
					}
				else
				*/
					{
						if (strstr(DriverName,"PRIMARY")!=NULL)
							DriverType = MODELIST_TYPE_D3D_PRIMARY;
						else
							DriverType = MODELIST_TYPE_D3D_SECONDARY;
					}
			}
		else if (strstr(DriverName,"GLIDE")!=NULL)
			{
				DriverType = MODELIST_TYPE_GLIDE;
			}
		else if (strstr(DriverName,"SOFT") != NULL)
			{
				DriverType = MODELIST_TYPE_SOFTWARE;
			}
		else 
			{
				DriverType = MODELIST_TYPE_UNKNOWN;
			}
		

		Mode = geDriver_GetNextMode(Driver, NULL);
		while	(Mode != NULL)
		{
			if (geDriver_ModeGetName(Mode, &ModeNamePtr)==GE_FALSE)
				{
					geErrorLog_AddString(-1,"AutoSelect: Failed to get mode name",NULL);
					goto ModeList_Exit;
				}

			Needed++;
			if (Allocated<Needed)
				{
					Allocated += MODELIST_ALLOCATION_BLOCK_SIZE;
					NewDriverList = static_cast<ModeList*>(geRam_Realloc(DriverList,sizeof(ModeList) * Allocated));
					if (NewDriverList == NULL)
						{
							geErrorLog_AddString(-1,"AutoSelect: Memory allocation failed",NULL);
							goto ModeList_Exit;
						}
					DriverList = NewDriverList;
				}
			dinfo = &(DriverList[Needed-1]);
			dinfo->DriverNamePtr = DriverNamePtr;
			dinfo->ModeNamePtr   = ModeNamePtr;
			dinfo->Driver = Driver;
			dinfo->Mode = Mode;
			dinfo->DriverType = DriverType;
			if (!geDriver_ModeGetWidthHeight(Mode, (int32*)&(dinfo->Width), (int32*)&(dinfo->Height)))
				{
					geErrorLog_AddString(-1,"AutoSelect: Failed to get mode width & height.  (recovering)",ModeNamePtr);
					dinfo->Evaluation = MODELIST_EVALUATED_TRIED_FAILED;
				}
			if ((dinfo->Width<=640) && (dinfo->Height<=480))
				{
					dinfo->Evaluation = MODELIST_EVALUATED_OK;
				}
			else
				{
					dinfo->Evaluation = MODELIST_EVALUATED_UNDESIRABLE;
				}
			if (dinfo->DriverType == MODELIST_TYPE_D3D_3DFX)
				{
					dinfo->Evaluation = MODELIST_EVALUATED_UNDESIRABLE;
				}

			if ((dinfo->Width == -1) && (dinfo->Height==-1))
				{
					// add some 'virtual modes'  for software window preselected resolutions
					
					dinfo->InAWindow = GE_TRUE;
					dinfo->ModeNamePtr   = ModeList_800WindowString;
					dinfo->Width = 800;
					dinfo->Height = 600;
					dinfo->Evaluation = MODELIST_EVALUATED_UNDESIRABLE;
			
					Needed++;
					if (Allocated<Needed)
						{
							Allocated += MODELIST_ALLOCATION_BLOCK_SIZE;
							NewDriverList = static_cast<ModeList*>(geRam_Realloc(DriverList,sizeof(ModeList) * Allocated));
							if (NewDriverList == NULL)
								{
									geErrorLog_AddString(-1,"AutoSelect: Memory allocation failed",NULL);
									goto ModeList_Exit;
								}
							DriverList = NewDriverList;
						}
					dinfo = &(DriverList[Needed-1]);
					dinfo->InAWindow = GE_TRUE;
					dinfo->DriverNamePtr = DriverNamePtr;
					dinfo->ModeNamePtr   = ModeList_640WindowString;
					dinfo->Driver = Driver;
					dinfo->Mode = Mode;
					dinfo->DriverType = DriverType;
					dinfo->Width = 640;
					dinfo->Height = 480;
					dinfo->Evaluation = MODELIST_EVALUATED_OK;

					Needed++;
					if (Allocated<Needed)
						{
							Allocated += MODELIST_ALLOCATION_BLOCK_SIZE;
							NewDriverList = static_cast<ModeList*>(geRam_Realloc(DriverList,sizeof(ModeList) * Allocated));
							if (NewDriverList == NULL)
								{
									geErrorLog_AddString(-1,"AutoSelect: Memory allocation failed",NULL);
									goto ModeList_Exit;
								}
							DriverList = NewDriverList;
						}
					dinfo = &(DriverList[Needed-1]);
					dinfo->InAWindow = GE_TRUE;
					dinfo->DriverNamePtr = DriverNamePtr;
					dinfo->ModeNamePtr   = ModeList_320WindowString;
					dinfo->Driver = Driver;
					dinfo->Mode = Mode;
					dinfo->DriverType = DriverType;
					dinfo->Width = 320;
					dinfo->Height = 240;
					dinfo->Evaluation = MODELIST_EVALUATED_OK;

				}
			else
				{
					strncpy(ModeName,ModeNamePtr,MODELIST_MAX_NAME);
					ModeName[MODELIST_MAX_NAME-1]=0;		// just in case
					_strupr(ModeName);
					if (strstr(ModeName,"WIN")!=NULL)
						{
							dinfo->InAWindow = GE_TRUE;
						}
					else
						{
							dinfo->InAWindow = GE_FALSE;
						}
				}

			
			Mode = geDriver_GetNextMode(Driver, Mode);
		}
		Driver = geDriver_SystemGetNextDriver(DriverSystem, Driver);
	}

	
	*ListLength = Needed;
	return DriverList;
	
	ModeList_Exit:
	
	if (DriverList!=NULL)
		{
			ModeList_Destroy(DriverList);
		}
			
	return NULL;
}

