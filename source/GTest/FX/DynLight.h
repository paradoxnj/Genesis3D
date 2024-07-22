#ifndef	DYNLIGHT_H
#define	DYNLIGHT_H

#include "GENESIS.H"

#pragma warning( disable : 4068 )

#pragma GE_Type("dynlight.bmp")

typedef struct	DynamicLight
{
#pragma	GE_Private
	geFloat			LastTime;
	geFloat			IntervalWidth;
	int				NumFunctionValues;
	geLight *		DynLight;
	
#pragma GE_Published
    geVec3d			origin;
    int				MinRadius;
    int				MaxRadius;
	int				InterpolateValues;
	int				AllowRotation;
    char *			RadiusFunction;
    geFloat			RadiusSpeed;
	geWorld_Model *	Model;
    GE_RGBA			Color;

#pragma GE_Origin(origin)
#pragma GE_DefaultValue(MinRadius, "50")
#pragma GE_DefaultValue(MaxRadius, "300")
#pragma GE_DefaultValue(Color, "40.0 40.0 153.0")
#pragma GE_DefaultValue(RadiusFunction, "aza")
#pragma GE_DefaultValue(RadiusSpeed, "2.0")
#pragma GE_DefaultValue(InterpolateValues, "1")
#pragma GE_DefaultValue(AllowRotation, "1")

#pragma GE_Documentation(MinRadius, "Minimum radius of the light (texels)")
#pragma GE_Documentation(MaxRadius, "Maximum radius of the light (texels)")
#pragma GE_Documentation(InterpolateValues, "0 or 1.  Whether to interpolate between the RadiusFunction values")
#pragma GE_Documentation(AllowRotation, "0 or 1.  Whether or not to follow the model rotation if the light is attached to a model")
#pragma GE_Documentation(RadiusFunction, "a-z, repeated (E.g. aabdfzddsfdz)  Specify light values over time.  a = Minimum z = maximum")
#pragma GE_Documentation(RadiusSpeed, "How fast to run through RadiusFunction values (seconds)")
#pragma GE_Documentation(Model, "Model that the light is attached to.  Optional")
#pragma GE_Documentation(Color, "Color of the light")

}	DynamicLight;

geBoolean	DynLight_Init(geEngine *Engine, geWorld *World, geVFile *Context);
geBoolean	DynLight_Reset(geWorld *World);
geBoolean	DynLight_Frame(geWorld *World, const geXForm3d *ViewPoint, geFloat Time);
geBoolean	DynLight_Shutdown(void);

#pragma warning( default : 4068 )

#endif

