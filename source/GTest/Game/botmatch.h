#ifndef	BOTMATCH_H
#define	BOTMATCH_H

#include	"GENESIS.H"

#pragma warning( disable : 4068 )

#pragma GE_Type("BotMatch.bmp")
typedef struct BotMatchStart BotMatchStart;

#pragma GE_Type("BotMatch.bmp")
typedef struct BotMatchStart {
#pragma GE_Published

geVec3d		origin;

#pragma GE_Origin(origin)
} BotMatchStart;

#pragma warning( default : 4068 )

#endif
