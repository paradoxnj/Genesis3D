/****************************************************************************************/
/*  Buffer.h                                                                            */
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
#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <Windows.h>

#include "Genesis.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int32	Size;			// Size of buffer in bytes
	int32	Pos;			// Current size/pos
	uint8	*Data;			// Pointer to data
} Buffer_Data;

geBoolean Buffer_FillByte(Buffer_Data *Buffer, uint8 Byte);
geBoolean Buffer_FillShort(Buffer_Data *Buffer, uint16 Short);
geBoolean Buffer_FillLong(Buffer_Data *Buffer, uint32 Long);
geBoolean Buffer_FillSLong(Buffer_Data *Buffer, int32 Long);
geBoolean Buffer_FillFloat(Buffer_Data *Buffer, float Float);
geBoolean Buffer_FillFloat2(Buffer_Data *Buffer, float Float, float Max);
geBoolean Buffer_FillAngle(Buffer_Data *Buffer, geVec3d Angle);
geBoolean Buffer_FillPos(Buffer_Data *Buffer, geVec3d Pos);
geBoolean Buffer_FillString(Buffer_Data *Buffer, uint8 *Str);
geBoolean Buffer_FillBuffer(Buffer_Data *Buffer1, Buffer_Data *Buffer2);
geBoolean Buffer_FillData(Buffer_Data *Buffer1, uint8 *Data, int32 Size);

geBoolean Buffer_GetByte(Buffer_Data *Buffer, uint8 *Byte);
geBoolean Buffer_GetShort(Buffer_Data *Buffer, uint16 *Short);
geBoolean Buffer_GetLong(Buffer_Data *Buffer, uint32 *Long);
geBoolean Buffer_GetSLong(Buffer_Data *Buffer, int32 *Long);
geBoolean Buffer_GetFloat(Buffer_Data *Buffer, float *Float);
geBoolean Buffer_GetFloat2(Buffer_Data *Buffer, float *Float, float Max);
geBoolean Buffer_GetAngle(Buffer_Data *Buffer, geVec3d *Angle);
geBoolean Buffer_GetPos(Buffer_Data *Buffer, geVec3d *Pos);
geBoolean Buffer_GetString(Buffer_Data *Buffer, uint8 *Str);
geBoolean Buffer_GetData(Buffer_Data *Buffer1, uint8 *Data, int32 Size);
geBoolean Buffer_Set(Buffer_Data *Buffer, char *Data, int32 Size);

#ifdef __cplusplus
}
#endif

#endif