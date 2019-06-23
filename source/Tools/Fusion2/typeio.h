/****************************************************************************************/
/*  typeio.h                                                                            */
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
#ifndef __TypeIO_H
#define __TypeIO_H

#ifdef __cplusplus
	extern "C" {
#endif

#include <stdio.h>
#include "basetype.h"
#include "XForm3d.h"

// writing

/*
  Write an untyped block.  Why?  Because we may want to compress
  blocks at some point.
*/
geBoolean TypeIO_WriteBlock
	(
	  FILE *f,
	  void const *b,
	  int cb
	);

geBoolean TypeIO_WriteInt
	(
	  FILE *f,
	  int i
	);

geBoolean TypeIO_WriteIntText
	(
	  FILE *f,
	  int i
	);

geBoolean TypeIO_WriteUint
    (
	  FILE *f,
	  unsigned int i
	);

geBoolean TypeIO_WriteUintText
    (
	  FILE *f,
	  unsigned int i
	);

geBoolean TypeIO_WriteFloat
	(
	  FILE *f,
	  float n
	);

geBoolean TypeIO_WriteFloatText
	(
	  FILE *f,
	  float n
	);

geBoolean TypeIO_WriteString
    (
	  FILE *f,
	  char const *s
	);

geBoolean TypeIO_WriteStringText
    (
	  FILE *f,
	  char const *s
	);

geBoolean TypeIO_WriteVec3d
	(
	  FILE *f,
	  geVec3d const *v
	);

geBoolean TypeIO_WriteVec3dText
	(
	  FILE *f,
	  geVec3d const *v
	);

geBoolean TypeIO_WriteXFormm3d
	(
	  FILE *f,
	  geXForm3d const *pXfm
	);

geBoolean TypeIO_WriteXForm3dText
	(
	  FILE *f,
	  geXForm3d const *pXfm
	);

// reading
geBoolean TypeIO_ReadBlock
	(
	  FILE *f,
	  void *b,
	  int BytesToRead
	);

geBoolean TypeIO_ReadInt
	(
	  FILE *f,
	  int * pi
	);

geBoolean TypeIO_ReadIntText
	(
	  FILE *f,
	  int * pi
	);

geBoolean TypeIO_ReadUint
    (
	  FILE *f,
	  unsigned int * pu
	);

geBoolean TypeIO_ReadUintText
    (
	  FILE *f,
	  unsigned int * pu
	);

geBoolean TypeIO_ReadFloat
	(
	  FILE *f,
	  float * pf
	);

geBoolean TypeIO_ReadFloatText
	(
	  FILE *f,
	  float * pf
	);

/*
  Reads a string into the passed buffer.
  Buffer must be large enough to hold the string.
  cb must be > 0.

  On success, the buffer will be filled with the nul-terminated string,
  and *pRequired will contain the length of the string, including the
  nul terminator (i.e. strlen (s) + 1).

  If a read error occurs, the routine will return GE_FALSE 
  and *pRequired will be set to 0.

  If the buffer is too small (i.e. cb <= length) then the function
  returns GE_FALSE and *pRequired contains the required buffer size.
  In this case the file position is not moved.

  If the length is negative, then the function returns GE_FALSE and
  *pRequired contains the length.
*/
geBoolean TypeIO_ReadString
    (
	  FILE *f,
	  char *s,
	  int cb,
	  int * pRequired
	);
/*
  Reads a string into the passed buffer.
  Buffer must be large enough to hold the string.
  cb must be > 0.

  On success, the buffer will be filled with the nul-terminated string,
  and *pRequired will contain the length of the string.

  If a read error occurs, the routine will return GE_FALSE 
  and *pRequired will be set to 0.

  If the buffer is too small (i.e. cb <= length) then the function
  returns GE_FALSE, the buffer is filled to capacity (cb-1 characters),
  *pRequired contains the amount of text remaining.
*/
geBoolean TypeIO_ReadStringText
    (
	  FILE *f,
	  char *s,
	  int cb,
	  int *pRequired
	);

geBoolean TypeIO_ReadVec3d
	(
	  FILE *f,
	  geVec3d *pv
	);

geBoolean TypeIO_ReadVec3dText
	(
	  FILE *f,
	  geVec3d *pv
	);

geBoolean TypeIO_ReadXFormm3d
	(
	  FILE *f,
	  geXForm3d *pXfm
	);

geBoolean TypeIO_ReadXForm3dText
	(
	  FILE *f,
	  geXForm3d *pXfm
	);

#ifdef __cplusplus
	}
#endif

#endif
