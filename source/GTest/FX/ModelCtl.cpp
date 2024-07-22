#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

#include	<math.h>
#include	<stdlib.h>
#include	<assert.h>

#define	BUILD_MODELCTL
#include	"modelctl.h"
#include	"Errorlog.h"

geBoolean ModelCtl_Init(void)
{
	return GE_TRUE;
}

geBoolean ModelCtl_Reset(geWorld *World)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;
 
	if	(World == NULL)
		return GE_TRUE;

	Set = geWorld_GetEntitySet(World, "ModelController");
	if	(Set == NULL)
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		ModelController *		Controller;

		Controller = static_cast<ModelController*>(geEntity_GetUserData(Entity));
		Controller->LastTime = 0.0f;

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}

geBoolean ModelCtl_Shutdown(void)
{
	return GE_TRUE;
}

geBoolean ModelCtl_Frame(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;
 
	if	(World == NULL)
		return GE_TRUE;

	Set = geWorld_GetEntitySet(World, "ModelController");
	if	(Set == NULL)
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		ModelController *	Controller;
		geMotion *			Motion;
		gePath *			Path;
		geFloat				StartTime;
		geFloat				EndTime;
		geXForm3d			XForm;

		#define MAX_NAME 200
		char	EntityName[MAX_NAME];

		geEntity_GetName(Entity, EntityName, MAX_NAME-1);
		EntityName[MAX_NAME-1]=0;

		Controller = static_cast<ModelController*>(geEntity_GetUserData(Entity));
		if (Controller->Model==NULL)
			{
				geErrorLog_AddString(0,"ModelCtl_Frame: Model controller has no model:",EntityName);
				return GE_FALSE;
			}
		Motion = geWorld_ModelGetMotion(Controller->Model);
		if (Motion == NULL)
			{
				geErrorLog_AddString(0,"ModelCtl_Frame: Model controller has no motion:",EntityName);
				return GE_FALSE;
			}
		Path = geMotion_GetPath(Motion, 0);
		if (Path == NULL)
			{
				geErrorLog_AddString(0,"ModelCtl_Frame: Model controller has no path in its motion:",EntityName);
				return GE_FALSE;
			}
		gePath_GetTimeExtents(Path, &StartTime, &EndTime);
		gePath_Sample(Path, StartTime + Controller->LastTime, &XForm);

		geWorld_SetModelXForm(World, Controller->Model, &XForm);
		
		Controller->LastTime = (geFloat)fmod(Controller->LastTime + DeltaTime, EndTime - StartTime);

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}
