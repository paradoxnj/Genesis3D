#ifndef	CORONA_H
#define	CORONA_H

#include	"GENESIS.H"

#pragma warning( disable : 4068 )

#ifdef __cplusplus
extern "C" {
#endif

#pragma GE_Type("corona.bmp")
typedef struct	Corona
{
	#pragma	GE_Private
		geFloat			LastTime;
		geFloat			LastVisibleTime;
		geFloat			LastVisibleRadius;
	
	#pragma GE_Published
	    geVec3d			origin;
		int				FadeOut;
		geFloat			FadeTime;
	    int				MinRadius;
	    int				MaxRadius;
		int				MaxVisibleDistance;
		int				MaxRadiusDistance;
		int				MinRadiusDistance;
		int				AllowRotation;
		geWorld_Model *	Model;
	    GE_RGBA			Color;

	#pragma GE_Origin(origin)
	#pragma GE_DefaultValue(FadeOut, "1")
	#pragma GE_DefaultValue(FadeTime, "0.15")
	#pragma GE_DefaultValue(MinRadius, "1")
	#pragma GE_DefaultValue(MaxRadius, "20")
	#pragma GE_DefaultValue(MaxVisibleDistance, "2000")
	#pragma GE_DefaultValue(MaxRadiusDistance, "1000")
	#pragma GE_DefaultValue(MinRadiusDistance, "100")
	#pragma GE_DefaultValue(AllowRotation, "1")
	#pragma GE_DefaultValue(Color, "255.0 255.0 255.0")

	#pragma GE_Documentation(FadeOut, "Whether to fade out coronas when they pass out of visibility (0 or 1)")
	#pragma GE_Documentation(FadeTime, "How long the fade takes to drop to zero visibility (seconds)")
	#pragma GE_Documentation(MinRadius, "Minimum radius corona will ever drop to (texels)")
	#pragma GE_Documentation(MaxRadius, "Maximum radius corona will ever increase to (texels)")
	#pragma GE_Documentation(MaxVisibleDistance, "Maximum distance corona is visible at (texels)")
	#pragma GE_Documentation(MaxRadiusDistance, "Above this distance, corona is capped at MaxRadius (texels)")
	#pragma GE_Documentation(MinRadiusDistance, "Below this distance, corona is capped at MinRadius (texels)")
	#pragma GE_Documentation(AllowRotation, "Permit rotation about model, if one is present (0 or 1)")
	#pragma GE_Documentation(Model, "World model to slave motion of corona to")
}	Corona;

geBoolean Corona_Init(geEngine *Engine, geWorld *World, geVFile *MainFS);
geBoolean Corona_Shutdown(void);
geBoolean Corona_Frame(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime);

#ifdef __cplusplus
}
#endif

#pragma warning( default : 4068 )

#endif
