/****************************************************************************/
/*    FILE: _bot.c	- Misc bot utilities & support							*/
/****************************************************************************/
#include <Windows.h>
#include <assert.h>
#include <Math.h>
#include <Time.h>

#include "GENESIS.H"
#include "RAM.H"

#include "GMain.h"

#include "_Bot.h"
#include "Track.h"

int32 randomseed=17;

void StackInit(Stack *s)
	{
	s->TOS = -1;
	s->Data = GE_RAM_ALLOCATE_ARRAY(int32, MAX_TRACKS);
    assert(s->Data);
    s->Size = MAX_TRACKS;
	}

void StackReset(Stack *s)
	{
	s->TOS = -1;
	}

void StackPush(Stack *s, int32 data)
	{
	s->TOS++;
    assert(s->TOS < s->Size);
	s->Data[s->TOS] = data;
	}

int32 StackPop(Stack *s)
	{
    int32 value;

	if (s->TOS <= -1)
		return (s->TOS = -1);

	value = s->Data[s->TOS];
	s->TOS--;

	return (value);
	}

int32 StackTop(Stack *s)
	{
	if (s->TOS <= -1)
		return (-1);

	return (s->Data[s->TOS]);
	}

geBoolean StackIsEmpty(Stack *s)
	{
	return (s->TOS <= -1);
	}

int32 krand()
    {
    randomseed = ((randomseed * 21 + 1) & 65535);
    return (randomseed);
    }

void ksrand(int32 seed)
    {
	
    randomseed = (unsigned)time( NULL );
    }

int32 RandomRange(int32 range)
    {
    int32 rand_num;
    int32 value;
    
    if (range <= 0)
        return(0);
    
    rand_num = krand();
    
    if (rand_num == 65535U)
        rand_num--;
    
    // shift values to give more precision
    value = (rand_num << 14) / ((65535UL << 14) / range);
    
    if (value >= range)
        value = range - 1;
        
    return(value);
    }

float DistWeightedY(const geVec3d *Pos1, const geVec3d *Pos2, const float Scale)
{
	geVec3d LPos1, LPos2;

	LPos1 = *Pos1;
	LPos2 = *Pos2;

	LPos1.Y *= Scale;
	LPos2.Y *= Scale;

	return(geVec3d_DistanceBetween(&LPos1, &LPos2));
}

void Ang2Vec(float ang, geVec3d *vec)
{
vec->X = (float)cos(ang);
vec->Z = (float)sin(ang);
vec->Y = 0.0f;
SqueezeVector(vec, 0.0001f);

geVec3d_Normalize(vec);
}

void VectorRotateY(geVec3d *vec, float delta_ang, geVec3d *result)
    {
    geXForm3d XForm;

    geXForm3d_SetIdentity(&XForm);
    geXForm3d_RotateY(&XForm, delta_ang);
    geXForm3d_Rotate(&XForm, vec, result);
    }

#if 0 // never ended up needing these
float NormAng(float ang)
{
if (ang > (PI_2))
	ang = (ang - PI_2);

if (ang < 0.0f)
	ang = PI_2 + ang;

return(ang);
}

float Vec2Ang(geVec3d *vec)
{
	float ang;

	if (geVec3d_Length(vec) == 0.0f)
		return(0.0f);

	ang = (float)atan2(vec->Z,vec->X);
	if (ang < 0)
		ang += M_PI2;

	return ang;
}


#endif