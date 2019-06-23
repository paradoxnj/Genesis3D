/****************************************************************************************/
/*  Buffer.c                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description:                                                                        */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <Assert.h>

#include "Genesis.h"

#include "Buffer.h"

typedef struct Pos2
{
	int16	X, Y, Z;
} Pos2;

geBoolean Buffer_FillByte(Buffer_Data *Buffer, uint8 Byte)
{
	assert(Buffer->Pos + (int32)sizeof(uint8) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(uint8) >= Buffer->Size)
		return GE_FALSE;

	Buffer->Data[Buffer->Pos] = Byte;

	Buffer->Pos += sizeof(uint8);

	return GE_TRUE;
}

geBoolean Buffer_FillShort(Buffer_Data *Buffer, uint16 Short)
{
	uint16	*Data;

	assert(Buffer->Pos + (int32)sizeof(uint16) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(uint16) >= Buffer->Size)
		return GE_FALSE;

	Data = (uint16*)&Buffer->Data[Buffer->Pos];

	*Data = Short;

	Buffer->Pos += sizeof(uint16);

	return GE_TRUE;
}

geBoolean Buffer_FillLong(Buffer_Data *Buffer, uint32 Long)
{
	uint32	*Data;

	assert(Buffer->Pos + (int32)sizeof(uint32) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(uint32) >= Buffer->Size)
		return GE_FALSE;

	Data = (uint32*)&Buffer->Data[Buffer->Pos];

	*Data = Long;

	Buffer->Pos += sizeof(uint32);

	return GE_TRUE;
}

geBoolean Buffer_FillSLong(Buffer_Data *Buffer, int32 Long)
{
	int32	*Data;

	assert(Buffer->Pos + (int32)sizeof(int32) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(int32) >= Buffer->Size)
		return GE_FALSE;

	Data = (int32*)&Buffer->Data[Buffer->Pos];

	*Data = Long;

	Buffer->Pos += sizeof(int32);

	return GE_TRUE;
}

geBoolean Buffer_FillFloat(Buffer_Data *Buffer, float Float)
{
	float *Data;

	assert(Buffer->Pos + (int32)sizeof(float) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(float) >= Buffer->Size)
		return GE_FALSE;

	Data = (float*)&Buffer->Data[Buffer->Pos];

	*Data = Float;

	Buffer->Pos += sizeof(float);

	return GE_TRUE;
}

geBoolean Buffer_FillFloat2(Buffer_Data *Buffer, float Float, float Max)
{
	uint16 *Data;

	assert(Float <= Max);
	assert(Buffer->Pos + (int32)sizeof(uint16) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(uint16) >= Buffer->Size)
		return GE_FALSE;

	Data = (uint16*)&Buffer->Data[Buffer->Pos];

	*Data = (uint16)((Float/Max)*65535);

	Buffer->Pos += sizeof(uint16);

	return GE_TRUE;
}

geBoolean Buffer_FillAngle(Buffer_Data *Buffer, geVec3d Angle)
{
	geVec3d	*Data;

	assert(Buffer->Pos + (int32)sizeof(geVec3d) < Buffer->Size);
	
	if (Buffer->Pos + (int32)sizeof(geVec3d) >= Buffer->Size)
		return GE_FALSE;

	Data = (geVec3d*)&Buffer->Data[Buffer->Pos];

	*Data = Angle;

	Buffer->Pos += sizeof(geVec3d);

	return GE_TRUE;
}

geBoolean Buffer_FillPos(Buffer_Data *Buffer, geVec3d Pos)
{
	geVec3d		*Data;

	assert(Buffer->Pos + (int32)sizeof(geVec3d) < Buffer->Size);

	if (Buffer->Pos + (int32)sizeof(geVec3d) >= Buffer->Size)
		return GE_FALSE;

	Data = (geVec3d*)&Buffer->Data[Buffer->Pos];

	*Data = Pos;

	Buffer->Pos += sizeof(geVec3d);

	return GE_TRUE;
}

geBoolean Buffer_FillString(Buffer_Data *Buffer, uint8 *Str)
{
	int32		i;

	assert(Buffer->Pos + (int32)strlen((char*)Str)+1 < Buffer->Size);
	
	if (Buffer->Pos + (int32)strlen((char*)Str)+1 >= Buffer->Size)
		return GE_FALSE;

	for (i=0; i< (int32)strlen((char*)Str)+1; i++)
		Buffer->Data[Buffer->Pos++] = Str[i];

	return GE_TRUE;
}

geBoolean Buffer_FillBuffer(Buffer_Data *Buffer1, Buffer_Data *Buffer2)
{
	int32		i;

	assert(Buffer2->Pos + Buffer1->Pos < Buffer1->Size);

	if (Buffer2->Pos + Buffer1->Pos >= Buffer1->Size)
		return GE_FALSE;

	for (i=0; i< Buffer2->Pos; i++)
		Buffer1->Data[Buffer1->Pos++] = Buffer2->Data[i];

	return GE_TRUE;
}

geBoolean Buffer_FillData(Buffer_Data *Buffer1, uint8 *Data, int32 Size)
{
	int32		i;

	assert(Buffer1->Pos + Size < Buffer1->Size);

	if (Buffer1->Pos + Size >= Buffer1->Size)
		return GE_FALSE;

	for (i=0; i< Size; i++)
		Buffer1->Data[Buffer1->Pos++] = Data[i];

	return GE_TRUE;
}

geBoolean Buffer_GetByte(Buffer_Data *Buffer, uint8 *Byte)
{
	assert(Buffer->Pos + (int32)sizeof(uint8) <= Buffer->Size);

	*Byte = Buffer->Data[Buffer->Pos];

	Buffer->Pos += sizeof(uint8);

	return GE_TRUE;
}

geBoolean Buffer_GetShort(Buffer_Data *Buffer, uint16 *Short)
{
	assert(Buffer->Pos + (int32)sizeof(uint16) <= Buffer->Size);

	*Short = *((uint16*)&Buffer->Data[Buffer->Pos]);

	Buffer->Pos += sizeof(uint16);

	return GE_TRUE;
}

geBoolean Buffer_GetLong(Buffer_Data *Buffer, uint32 *Long)
{
	assert(Buffer->Pos + (int32)sizeof(uint32) <= Buffer->Size);

	*Long = *((uint32*)&Buffer->Data[Buffer->Pos]);

	Buffer->Pos += sizeof(uint32);
	
	return GE_TRUE;
}

geBoolean Buffer_GetSLong(Buffer_Data *Buffer, int32 *Long)
{
	assert(Buffer->Pos + (int32)sizeof(int32) <= Buffer->Size);

	*Long = *((int32*)&Buffer->Data[Buffer->Pos]);

	Buffer->Pos += sizeof(int32);
	
	return GE_TRUE;
}

geBoolean Buffer_GetFloat(Buffer_Data *Buffer, float *Float)
{
	assert(Buffer->Pos + (int32)sizeof(float) <= Buffer->Size);

	*Float = *((float*)&Buffer->Data[Buffer->Pos]);

	Buffer->Pos += sizeof(float);
	
	return GE_TRUE;
}

geBoolean Buffer_GetFloat2(Buffer_Data *Buffer, float *Float, float Max)
{
	uint16	Temp16;

	assert(Buffer->Pos + (int32)sizeof(uint16) <= Buffer->Size);

	Temp16 = *((uint16*)&Buffer->Data[Buffer->Pos]);

	*Float = ((float)Temp16 / 65535.0f) * Max;

	Buffer->Pos += sizeof(uint16);
	
	return GE_TRUE;
}

geBoolean Buffer_GetAngle(Buffer_Data *Buffer, geVec3d *Angle)
{
	assert(Buffer->Pos + (int32)sizeof(geVec3d) <= Buffer->Size);

	*Angle = *(geVec3d*)&Buffer->Data[Buffer->Pos];

	Buffer->Pos += sizeof(geVec3d);
	
	return GE_TRUE;
}

geBoolean Buffer_GetPos(Buffer_Data *Buffer, geVec3d *Pos)
{
	assert(Buffer->Pos + (int32)sizeof(geVec3d) <= Buffer->Size);

	*Pos = *(geVec3d*)&Buffer->Data[Buffer->Pos];
	
	Buffer->Pos += sizeof(geVec3d);
	
	return GE_TRUE;
}

geBoolean Buffer_GetString(Buffer_Data *Buffer, uint8 *Str)
{
	int32	i = 0;
	uint8	c = 0;

	do 
	{
		assert(Buffer->Pos+1 <= Buffer->Size);

		c = Buffer->Data[Buffer->Pos++];
		Str[i++] = c;
	} while (c != -1 && c != 0);

	return GE_TRUE;
}

geBoolean Buffer_GetData(Buffer_Data *Buffer1, uint8 *Data, int32 Size)
{
	int32		i;

	assert(Buffer1->Pos + Size <= Buffer1->Size);

	for (i=0; i< Size; i++)
		Data[i] = Buffer1->Data[Buffer1->Pos++];

	return GE_TRUE;
}

geBoolean Buffer_Set(Buffer_Data *Buffer, char *Data, int32 Size)
{
	assert(Buffer);

	Buffer->Data = (uint8*)Data;
	Buffer->Size = Size;
	Buffer->Pos = 0;

	return GE_TRUE;
}