#ifndef	MODELCTL_H
#define	MODELCTL_H

#include	"GENESIS.H"

#pragma warning( disable : 4068 )

#pragma GE_Type("modelctl.bmp")
typedef struct	ModelController
{
#pragma	GE_Private
	geFloat			LastTime;
	
#pragma GE_Published
	geWorld_Model *	Model;

}	ModelController;

//void	ModelCtl_GetCurrentPosition(ModelController *Controller, geVec3d *Pos);

#pragma warning( default : 4068 )

geBoolean	ModelCtl_Init(void);
geBoolean	ModelCtl_Reset(geWorld *World);
geBoolean	ModelCtl_Frame(geWorld *World, const geXForm3d *ViewPoint, geFloat Time);
geBoolean	ModelCtl_Shutdown(void);

#endif

