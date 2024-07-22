#ifndef	PATHPT_H
#define	PATHPT_H

#include "GENESIS.H"

#pragma warning( disable : 4068 )

#pragma GE_Type("PathPt.bmp")
typedef struct PathPoint PathPoint;

#pragma GE_Type("PathPt.bmp")
typedef struct PathPoint {

	#pragma GE_Published

		geVec3d		origin;
		int         PathType;
		int			ActionType;
		float		Time;
		float		Dist;
		float		VelocityScale;
		PathPoint	*Next;
		geWorld_Model *MoveWithModel;
		int         Direction;
		int			ShootTimes;
		PathPoint	*WatchPoint;

	#pragma GE_Private
		geVec3d     Pos;

	#pragma GE_Origin(origin)

	#pragma GE_DefaultValue(PathType, "-1")
	#pragma GE_DefaultValue(ActionType, "-1")
	#pragma GE_DefaultValue(Direction, "1")

	#pragma GE_Documentation(Next, "Link to next point")
	#pragma GE_Documentation(PathType, "Path Type")
	#pragma GE_Documentation(ActionType, "ActionType")
	#pragma GE_Documentation(Time, "ActionType Modifier - Only applies to certain actions.  If left 0 a default value will be used.")
	#pragma GE_Documentation(Dist, "ActionType Modifier - Only applies to certain actions.  If left 0 a default value will be used.")
	#pragma GE_Documentation(VelocityScale, "ActionType Modifier - Only applies to certain actions.  If left 0 a default value will be used.")
	#pragma GE_Documentation(MoveWithModel,"Link to model. Entity will move with this model.")
	#pragma GE_Documentation(Direction, "Direction from which action is triggered: 1=Forward, -1=Reverse, 0=Both")
} PathPoint;

#pragma warning( default : 4068 )

geBoolean PathPt_Startup(geWorld *World, geVFile *Fs);
geBoolean	PathPt_Reset(geWorld *World);
geBoolean	PathPt_Frame(geWorld *World, const geXForm3d *ViewPoint, geFloat Time);
geBoolean	PathPt_Shutdown(void);

#endif
