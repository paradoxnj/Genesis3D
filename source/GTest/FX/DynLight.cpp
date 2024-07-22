#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

#include	<math.h>
#include	<stdlib.h>
#include	<assert.h>

#define	BUILD_DYNLIGHT
#include	"DynLIGHT.H"
#include	"Errorlog.h"

static geBoolean DynLight_SetWorld(geWorld *World, geVFile *Context);

geBoolean DynLight_Init(geEngine *Engine, geWorld *World, geVFile *Context)
{
	
	if (!DynLight_SetWorld(World, Context))
		return GE_FALSE;

	return GE_TRUE;
}

geBoolean DynLight_Reset(geWorld *World)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;

	if	(!World)
		return GE_TRUE;
 
	Set = geWorld_GetEntitySet(World, "DynamicLight");
	if	(Set == NULL)		 
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		DynamicLight *		Light;

		Light = static_cast<DynamicLight*>(geEntity_GetUserData(Entity));
		Light->LastTime = 0.0f;

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}

geBoolean DynLight_Shutdown(void)
{
	return GE_TRUE;
}

geBoolean DynLight_Frame(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;
 
	if	(World == NULL)
		return GE_TRUE;

	Set = geWorld_GetEntitySet(World, "DynamicLight");
	if	(Set == NULL)
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		DynamicLight *	Light;
		geFloat			Radius;
		geFloat			Percentage;
		int				Index;
		geVec3d			Pos;

		Light = static_cast<DynamicLight*>(geEntity_GetUserData(Entity));
		assert(Light->DynLight);

//		if	(Light->Controller)
		if	(Light->Model)
		{
			geXForm3d	XForm;

//			geWorld_GetModelXForm(World, Light->Controller->Model, &XForm);
			geWorld_GetModelXForm(World, Light->Model, &XForm);

			Pos = Light->origin;
			if	(Light->AllowRotation)
			{
				geVec3d	Center;

//				geWorld_GetModelRotationalCenter(World, Light->Controller->Model, &Center);
				geWorld_GetModelRotationalCenter(World, Light->Model, &Center);
				geVec3d_Subtract(&Pos, &Center, &Pos);
				geXForm3d_Transform(&XForm, &Pos, &Pos);
				geVec3d_Add(&Pos, &Center, &Pos);
			}
			else
				geVec3d_Add(&Pos, &XForm.Translation, &Pos);
		}
		else
		{
			Pos = Light->origin;
		}

		assert(Light->RadiusSpeed > Light->LastTime);
		Percentage = Light->LastTime / Light->RadiusSpeed;

		Index = (int)(Percentage * Light->NumFunctionValues);
		assert(Index < Light->NumFunctionValues);
		assert(Light->RadiusFunction[Index] >= 'a' && Light->RadiusFunction[Index] <= 'z');

		if	(Light->InterpolateValues && Index < Light->NumFunctionValues - 1)
		{
			geFloat	Remainder;
			geFloat	InterpolationPercentage;
			int		DeltaValue;
			geFloat	Value;

			Remainder = (geFloat)fmod(Light->LastTime, Light->IntervalWidth);
			InterpolationPercentage = Remainder / Light->IntervalWidth;
//geEngine_Printf(TheEngine, 20, 120, "Interpolate = %4.2f", InterpolationPercentage);
			DeltaValue = Light->RadiusFunction[Index + 1] - Light->RadiusFunction[Index];
			Value = Light->RadiusFunction[Index] + DeltaValue * InterpolationPercentage;
			Percentage = ((geFloat)(Value - 'a')) / ((geFloat)('z' - 'a'));
		}
		else
		{
			Percentage = ((geFloat)(Light->RadiusFunction[Index] - 'a')) / ((geFloat)('z' - 'a'));
		}

		Radius = Percentage * (Light->MaxRadius - Light->MinRadius) + Light->MinRadius;

		geWorld_SetLightAttributes(World,
								   Light->DynLight,
								   &Pos,
								   &Light->Color,
								   Radius,
								   GE_TRUE);

		Light->LastTime = (geFloat)fmod(Light->LastTime + DeltaTime, Light->RadiusSpeed);

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}

static geBoolean DynLight_SetWorld(geWorld *World, geVFile *Context)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;

	Context;

	if	(World == NULL)
		return GE_TRUE;

	Set = geWorld_GetEntitySet(World, "DynamicLight");
	if	(Set == NULL)
		return GE_TRUE;

	Entity = geEntity_EntitySetGetNextEntity(Set, NULL);
	while	(Entity)
	{
		DynamicLight *	Light;
		#define MAX_NAME 200
		char	EntityName[MAX_NAME];

		geEntity_GetName(Entity, EntityName, MAX_NAME-1);
		EntityName[MAX_NAME-1]=0;

		Light = static_cast<DynamicLight*>(geEntity_GetUserData(Entity));
		Light->NumFunctionValues = strlen(Light->RadiusFunction);
		if (Light->NumFunctionValues == 0)
			{
				geErrorLog_AddString(0,"DynLight_SetWorld: DynamicLight has no RadiusFunction string:",EntityName);
				return GE_FALSE;
			}
		Light->IntervalWidth = Light->RadiusSpeed / (geFloat)Light->NumFunctionValues;
		Light->DynLight = geWorld_AddLight(World);
		if	(!Light->DynLight)
			{
				geErrorLog_AddString(0,"DynLight_SetWorld: DynamicLight failed to create light in world:",EntityName);
				return GE_FALSE;
			}

		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);
	}

	return GE_TRUE;
}
