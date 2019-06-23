#ifndef	ELECTRIC_H
#define ELECTRIC_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "Genesis.h"

#define	ELECTRIC_BOLT_REDDOMINANT	0
#define	ELECTRIC_BOLT_GREENDOMINANT	1
#define	ELECTRIC_BOLT_BLUEDOMINANT	2

#pragma warning( disable : 4068 )

#pragma GE_Type("terminus.bmp")
typedef struct	ElectricBoltTerminus
{
#pragma GE_Published
    geVec3d			origin;

#pragma GE_Origin(origin)
}	ElectricBoltTerminus;

typedef struct Electric_BoltEffect Electric_BoltEffect;

#pragma GE_Type("bolt.bmp")
typedef struct	ElectricBolt
{
#pragma	GE_Private
	geFloat					LastTime;
	geFloat					LastBoltTime;
	Electric_BoltEffect		*Bolt;
	geSound *				LoopingSound;
	
#pragma GE_Published
    geVec3d					origin;
    int						Width;
    int						NumPoints;
    int						Intermittent;
    geFloat					MinFrequency;
    geFloat					MaxFrequency;
    geFloat					Wildness;
    ElectricBoltTerminus *	Terminus;
    int						DominantColor;
    GE_RGBA					Color;

#pragma GE_Origin(origin)
#pragma GE_DefaultValue(Width, "8")
#pragma GE_DefaultValue(NumPoints, "64")
#pragma GE_DefaultValue(Intermittent, "1")
#pragma GE_DefaultValue(MinFrequency, "4.0")
#pragma GE_DefaultValue(MaxFrequency, "1.0")
#pragma GE_DefaultValue(Wildness, "0.5")
#pragma GE_DefaultValue(DominantColor, "2")
#pragma GE_DefaultValue(Color, "160.0 160.0 255.0")

#pragma GE_Documentation(Width, "Width in texels of the bolt")
#pragma GE_Documentation(NumPoints, "Power of 2.  Number of control points.  Stick to 32, 64, 128.")
#pragma GE_Documentation(Intermittent, "0 or 1.  Whether or not the bolt is continuous, or random")
#pragma GE_Documentation(MinFrequency, "If the bolt is intermittent, the minimum time in seconds between zaps")
#pragma GE_Documentation(MaxFrequency, "If the bolt is intermittent, the maximum time in seconds between zaps")
#pragma GE_Documentation(Wildness, "Degree of 'freneticity' of the bolt (0 to 1)")
#pragma GE_Documentation(Terminus, "Where the other end of the bolt is")
#pragma GE_Documentation(DominantColor, "Specifies the dominant color of the bolt. 0 = Red, 1 = Green, 2 = Blue")
#pragma GE_Documentation(Color, "Base color of the bolt.  The two non-dominant color values MUST be the same!")

}	ElectricBolt;
#pragma warning( default : 4068 )

geBoolean	Electric_Init(geEngine *Engine, geWorld *World, geVFile *MainFS, geSound_System *SoundSystem);
geBoolean	Electric_Reset(geWorld *World);
geBoolean	Electric_Frame(geWorld *World, const geXForm3d *ViewPoint, geFloat Time);
geBoolean	Electric_Shutdown(void);

#ifdef	__cplusplus
}
#endif

#endif

