/****************************************************************************************/
/*  POP.C																				*/
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description: Path optimizer.														*/
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
#include <math.h>

#include "pop.h"
#include "path.h"

#include "MkUtil.h"   // ONLY for Interrupt!


#define xxSUPERSAMPLE

geBoolean Pop_RotationCompare( const gePath *PLong, const gePath *PShort, geFloat Tolerance)
{
	geXForm3d M1;
	geFloat T;
	#ifdef SUPERSAMPLE
		geFloat StartTime,EndTime;
		geFloat LastT=0.0f;
		geFloat DT=0.0f;
	#endif
	geQuaternion Q1,Q2;
	geVec3d V1,V2;

	int Count;
	int i;

	assert( PLong != NULL );
	assert( PShort != NULL );
	
	Count = gePath_GetKeyframeCount(PLong,GE_PATH_ROTATION_CHANNEL);
	if (Count == 0)
		{
			return GE_TRUE;
		}
	assert( gePath_GetKeyframeCount(PShort,GE_PATH_ROTATION_CHANNEL) <= Count );

	for (i=0; i<Count; i++)
		{
			gePath_GetKeyframe(PLong,i,GE_PATH_ROTATION_CHANNEL, &T, &M1);
			gePath_SampleChannels(PShort, T,&Q1,&V1);
			gePath_SampleChannels(PLong , T,&Q2,&V2);
			if (geQuaternion_Compare(&Q1,&Q2,Tolerance) == GE_FALSE)
				return GE_FALSE; 
			#ifdef SUPERSAMPLE
				if (i>0)
					{
						if (i>1)
							{
								if (T-LastT < DT)
									{
										DT = T-LastT;
									}
							}
						else
							{
								DT = T-LastT;
							}
					}
				LastT = T;
			#endif
		}

	#ifdef SUPERSAMPLE
		DT = DT / 3.0f;   // 'oversample' at 3 times

		if (Count>0)
			{
				gePath_GetKeyframe(PLong,0,GE_PATH_ROTATION_CHANNEL, &StartTime, &M1);
			}
		if (Count>1)
			{
				gePath_GetKeyframe(PLong,Count-1,GE_PATH_ROTATION_CHANNEL, &EndTime, &M1);
			}
		else
			{
				EndTime = StartTime;
			}

		for (T=StartTime; T<=EndTime; T+=DT)
			{
				gePath_SampleChannels(PLong,  T, &Q1, &V1);
				gePath_SampleChannels(PShort, T, &Q2, &V2);
				if (geQuaternion_Compare(&Q1,&Q2,Tolerance) == GE_FALSE)
					{
						return GE_FALSE; 
					}
			}
	#endif

	return GE_TRUE;
}


geBoolean Pop_ZapRotationsIfAllKeysEqual( gePath *P, geFloat Tolerance )
{
	geXForm3d M1;
	geFloat T;
	geQuaternion Q1,Q2;
	geVec3d V1,V2;

	int Count;
	int i;

	assert( P != NULL );
	Count = gePath_GetKeyframeCount(P,GE_PATH_ROTATION_CHANNEL);
	if (Count == 0)
		{
			return GE_TRUE;
		}
	gePath_GetKeyframe(P,0,GE_PATH_ROTATION_CHANNEL, &T, &M1);
	gePath_SampleChannels(P, T,&Q1,&V1);

	for (i=1; i<Count; i++)
		{
			gePath_GetKeyframe(P,i,GE_PATH_ROTATION_CHANNEL, &T, &M1);
			gePath_SampleChannels(P , T,&Q2,&V2);
			if (geQuaternion_Compare(&Q1,&Q2,Tolerance) == GE_FALSE)
				return GE_TRUE; 
		}

	// if we get to here, all keys are equal.  (delete all but first and last)
	for (i=Count-2; i>0; i--)
		{
			//gePath_GetKeyframe(P,i,GE_PATH_ROTATION_CHANNEL, &T, &M1);
			if (gePath_DeleteKeyframe(P,i,GE_PATH_ROTATION_CHANNEL) == GE_FALSE)
				{
					return GE_FALSE;
				}
		}

	return GE_TRUE;
}


geBoolean Pop_TranslationCompare( const gePath *PLong, const gePath *PShort, geFloat Tolerance)
{
	geXForm3d M1;
	geFloat T;
	#ifdef SUPERSAMPLE
		geFloat StartTime,EndTime;
		geFloat LastT=0.0f;
		geFloat DT=0.0f;
	#endif
	geQuaternion Q1;
	//geQuaternion Q2;
	geVec3d V1;
	//veVec3d V2;

	int Count;
	int i;

	assert( PLong != NULL );
	assert( PShort != NULL );
	
	Count = gePath_GetKeyframeCount(PLong,GE_PATH_TRANSLATION_CHANNEL);
	if (Count == 0)
		{
			return GE_TRUE;
		}
	assert( gePath_GetKeyframeCount(PShort,GE_PATH_TRANSLATION_CHANNEL) <= Count );

	for (i=0; i<Count; i++)
		{
			gePath_GetKeyframe(PLong,i,GE_PATH_TRANSLATION_CHANNEL, &T, &M1);
			gePath_SampleChannels(PShort, T,&Q1,&V1);
			if (geVec3d_Compare(&V1, &(M1.Translation),Tolerance) == GE_FALSE)
				{
					return GE_FALSE; 
				}
			#ifdef SUPERSAMPLE
				if (i>0)
					{
						if (i>1)
							{
								if (T-LastT < DT)
									{
										DT = T-LastT;
									}
							}
						else
							{
								DT = T-LastT;
							}
					}
				LastT = T;
			#endif
		}

	#ifdef SUPERSAMPLE

		DT = DT / 3.0f;   // 'oversample' at 3 times

		if (Count>0)
			{
				gePath_GetKeyframe(PLong,0,GE_PATH_TRANSLATION_CHANNEL, &StartTime, &M1);
			}
		if (Count>1)
			{
				gePath_GetKeyframe(PLong,Count-1,GE_PATH_TRANSLATION_CHANNEL, &EndTime, &M1);
			}
		else
			{
				EndTime = StartTime;
			}

		for (T=StartTime; T<=EndTime; T+=DT)
			{
				gePath_SampleChannels(PLong,  T, &Q1, &V1);
				gePath_SampleChannels(PShort, T, &Q2, &V2);
				if (geVec3d_Compare(&V1, &V2,Tolerance) == GE_FALSE)
					{
						return GE_FALSE; 
					}
			}

	#endif
	return GE_TRUE;
}

geBoolean Pop_ZapTranslationsIfAllKeysEqual( gePath *P, geFloat Tolerance )
{
	geXForm3d M1;
	geFloat T;
	geQuaternion Q1,Q2;
	geVec3d V1,V2;

	int Count;
	int i;

	assert( P != NULL );
	
	Count = gePath_GetKeyframeCount(P,GE_PATH_TRANSLATION_CHANNEL);
	if (Count == 0)
		{
			return GE_TRUE;
		}
	gePath_GetKeyframe(P,0,GE_PATH_TRANSLATION_CHANNEL, &T, &M1);
	gePath_SampleChannels(P, T,&Q1,&V1);

	for (i=1; i<Count; i++)
		{
			gePath_GetKeyframe(P,i,GE_PATH_TRANSLATION_CHANNEL, &T, &M1);
			gePath_SampleChannels(P , T,&Q2,&V2);
			if (geVec3d_Compare(&V1, &V2,Tolerance) == GE_FALSE)
				return GE_TRUE; 
		}

	// if we get to here, all keys are equal.  (delete all but first and last)
	for (i=Count-2; i>0; i--)
		{
			if (gePath_DeleteKeyframe(P,i,GE_PATH_TRANSLATION_CHANNEL) == GE_FALSE)
				{
					return GE_FALSE;
				}
		}

	return GE_TRUE;
}


geBoolean pop_ZapLastKey(const gePath *P, gePath *Popt, int Channel1, int Channel2, geFloat Tolerance)
{
	assert( Popt != NULL );
	assert( Channel1 != Channel2 );
	assert( Channel1 == GE_PATH_ROTATION_CHANNEL || Channel1 == GE_PATH_TRANSLATION_CHANNEL );
	assert( Channel2 == GE_PATH_ROTATION_CHANNEL || Channel2 == GE_PATH_TRANSLATION_CHANNEL );
	
	if (gePath_GetKeyframeCount(Popt, Channel1)==2)
		{
			if (gePath_GetKeyframeCount(Popt, Channel2)>=2)
				{
					geFloat T1,T2,StartTime1,EndTime1,StartTime2,EndTime2;
					geXForm3d M;
					geQuaternion Q1,Q2;
					geVec3d V1,V2;

					if (gePath_GetTimeExtents(Popt, &StartTime1, &EndTime1)==GE_FALSE)
						{
							return GE_FALSE;
						}
			
					gePath_GetKeyframe(Popt,0,Channel1, &T1, &M);
					gePath_GetKeyframe(Popt,1,Channel1, &T2, &M);
					
					gePath_SampleChannels(Popt, T1, &Q1, &V1);
					gePath_SampleChannels(Popt, T2, &Q2, &V2);
					
					// first and last key have to be equal!
					if (Channel1 == GE_PATH_ROTATION_CHANNEL)
						{
							if (geQuaternion_Compare(&Q1,&Q2,Tolerance) == GE_FALSE)
								{
									return GE_TRUE;
								}
						}
					else
						{
							if (geVec3d_Compare(&V1, &V2,Tolerance) == GE_FALSE)
								{
									return GE_TRUE; 
								}
						}

					if (gePath_DeleteKeyframe(Popt,1,Channel1) == GE_FALSE)
						{
							return GE_FALSE;
						}
					if (gePath_GetTimeExtents(Popt,&StartTime2,&EndTime2)==GE_FALSE)
						{	// cant get extents: try to reverse change and bail out
							if (gePath_InsertKeyframe(Popt,Channel1,T2,&M) == GE_FALSE)
								{
									return GE_FALSE;
								}
							return GE_FALSE;
						}
					if (      (fabs(StartTime1-StartTime2) > Tolerance)
						  ||  (fabs(EndTime1-EndTime2) > Tolerance)     )
						{	// new extents are bad: reverse change and bail out
							if (gePath_InsertKeyframe(Popt,Channel1,T2,&M) == GE_FALSE)
								{
									return GE_FALSE;
								}
							return GE_FALSE;
						}

						
					if (Channel1 == GE_PATH_ROTATION_CHANNEL)
						{
							if ( Pop_RotationCompare   (P,Popt,Tolerance)==GE_FALSE )
								{	// new path has too much error: reverse change and bail out
									if (gePath_InsertKeyframe(Popt,Channel1,T2,&M) == GE_FALSE)
										{
											return GE_FALSE;
										}
									return GE_FALSE;
								}
						}
					else
						{
							if (Pop_TranslationCompare(P,Popt,Tolerance)==GE_FALSE )
								{
									if (gePath_InsertKeyframe(Popt,Channel1,T2,&M) == GE_FALSE)
										{
											return GE_FALSE;
										}
									return GE_FALSE;
								}
						}
					return GE_TRUE;
				}	
		}
	return GE_TRUE;
}

geBoolean pop_ZapIdentityKey(gePath *P, gePath *Popt,int Channel,geFloat Tolerance)
{
	geFloat T;
	geXForm3d M;
	geQuaternion Q;
	geVec3d V;

	assert( Popt != NULL );
	assert( Channel == GE_PATH_ROTATION_CHANNEL || Channel == GE_PATH_TRANSLATION_CHANNEL );
	
	if (gePath_GetKeyframeCount(Popt, Channel)==1)
		{
			gePath_GetKeyframe(Popt,0,Channel, &T, &M);
			gePath_SampleChannels(Popt, T,&Q,&V);

			switch (Channel)
				{
					case (GE_PATH_ROTATION_CHANNEL):
						{
							geQuaternion QI;
							geQuaternion_SetNoRotation(&QI);
							if (geQuaternion_Compare(&Q,&QI, Tolerance) == GE_TRUE)
								{
									if (gePath_DeleteKeyframe(Popt,0,Channel) == GE_FALSE)
										{
											return GE_FALSE;
										}
									if ( Pop_RotationCompare   (P,Popt,Tolerance)==GE_FALSE )
										{
											if (gePath_InsertKeyframe(Popt,GE_PATH_ROTATION_CHANNEL,T,&M) == GE_FALSE)
												{
													return GE_FALSE;
												}
										}
									assert( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE );
								}
						}
						break;
					case (GE_PATH_TRANSLATION_CHANNEL):
						{
							geVec3d VI;
							geVec3d_Clear(&VI);
							if (geVec3d_Compare(&V, &VI ,Tolerance) == GE_TRUE)
								{
									if (gePath_DeleteKeyframe(Popt,0,Channel) == GE_FALSE)
										{
											return GE_FALSE;
										}
									if ( Pop_TranslationCompare(P,Popt,Tolerance)==GE_FALSE )
										{
											if (gePath_InsertKeyframe(Popt,GE_PATH_TRANSLATION_CHANNEL,T,&M) == GE_FALSE)
												{
													return GE_FALSE;
												}
										}
									assert( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE );
								}
						}
						break;
					default:
						assert(0);
				}
		}

	return GE_TRUE;
}


gePath *Pop_PathOptimize( gePath *P, geFloat Tolerance)
{
	int Count;
	gePath *Popt;
	int i,pass;
	geBoolean AnyRemoved;
	geFloat T;
	geXForm3d M;
	

	assert( P != NULL );

	Popt = gePath_CreateCopy(P);
	if (Popt==NULL)
		{
			return NULL;
		}

	AnyRemoved = GE_TRUE;
	pass = 0;

	Count = gePath_GetKeyframeCount(Popt,GE_PATH_ROTATION_CHANNEL);

	//for (i=1; i< Count-1; i++)
	for (i=Count-2; i>=1; i--)
		{
			gePath_GetKeyframe(P,i,GE_PATH_ROTATION_CHANNEL, &T, &M);
			if (MkUtil_Interrupt())
				{
					gePath_Destroy(&Popt);
					return NULL;
				}
		
			if (gePath_DeleteKeyframe(Popt,i,GE_PATH_ROTATION_CHANNEL) == GE_FALSE)
				{
					gePath_Destroy(&Popt);
					return NULL;
				}
			if ( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE )
				{
					AnyRemoved = GE_TRUE;
				}
			else
				{
					if (gePath_InsertKeyframe(Popt,GE_PATH_ROTATION_CHANNEL,T,&M) == GE_FALSE)
						{
							gePath_Destroy(&Popt);
							return NULL;
						}
				}
			assert( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE );
		}

	assert( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE );
	assert( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE );
	

	Count = gePath_GetKeyframeCount(Popt,GE_PATH_TRANSLATION_CHANNEL);
	for (i=Count-2; i>=1; i--)
		{
			gePath_GetKeyframe(P,i,GE_PATH_TRANSLATION_CHANNEL, &T, &M);
			if (MkUtil_Interrupt())
				{
					gePath_Destroy(&Popt);
					return NULL;
				}
			if (gePath_DeleteKeyframe(Popt,i,GE_PATH_TRANSLATION_CHANNEL) == GE_FALSE)
				{
					gePath_Destroy(&Popt);
					return NULL;
				}
			if ( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE )
				{
					AnyRemoved = GE_TRUE;
				}
			else
				{
					if (gePath_InsertKeyframe(Popt,GE_PATH_TRANSLATION_CHANNEL,T,&M) == GE_FALSE)
						{
							gePath_Destroy(&Popt);
							return NULL;
						}
				}
			assert( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE );
		}

	assert( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE );
	assert( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE );

	// Try to kill last 2 keys, if the extent of the path is defined by the other channel...
	if (pop_ZapLastKey(P,Popt,GE_PATH_TRANSLATION_CHANNEL,GE_PATH_ROTATION_CHANNEL,Tolerance)==GE_FALSE)
		{
			gePath_Destroy(&Popt);
			return NULL;
		}
	if (pop_ZapLastKey(P,Popt,GE_PATH_ROTATION_CHANNEL,GE_PATH_TRANSLATION_CHANNEL,Tolerance)==GE_FALSE)
		{
			gePath_Destroy(&Popt);
			return NULL;
		}

	assert( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE );
	assert( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE );

	if (pop_ZapIdentityKey(P,Popt,GE_PATH_ROTATION_CHANNEL,Tolerance)==GE_FALSE)
		{
			gePath_Destroy(&Popt);
			return NULL;
		}
	if (pop_ZapIdentityKey(P,Popt,GE_PATH_TRANSLATION_CHANNEL,Tolerance)==GE_FALSE)
		{
			gePath_Destroy(&Popt);
			return NULL;
		}

	assert( Pop_TranslationCompare(P,Popt,Tolerance)!=GE_FALSE );
	assert( Pop_RotationCompare   (P,Popt,Tolerance)!=GE_FALSE );
			
	return Popt;
}
