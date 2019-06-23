/****************************************************************************/
/*    FILE: PathPt.c														*/
/****************************************************************************/
#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>

// PathPt kind of does a naughty thing.  It digs directly into the world, to get the entities needed
// to make a path.  This is ok, UNTIL the game stuff gets put into a dll.  Then we'll need to make
// an interface to all the genesis API's needed to access the world.  ALSO, NOTE that this stuff will
// still only happen on the server side...

#include	<math.h>
#include	<stdlib.h>
#include	<assert.h>

#include	"PathPt.h"
#include    "_Bot.h"
#include	"Track.h"

static	geBoolean PathPoint_Frame2(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime);
static	geBoolean PathPoint_Frame3(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime);

static	geBitmap	*PathPointTexture;

#define	SetRGB(t,rv,gv,bv) ((t).r = (float)(rv), (t).g = (float)(gv), (t).b = (float)(bv))

static	void	DrawLine3d(
	geWorld *			World,
	const geXForm3d *	M, 
	const geVec3d *		p1,
	const geVec3d *		p2,
	int r, int g, int b,
	int r1, int g1, int b1
	)
{
	GE_LVertex	v[4];
	int			i;
	geVec3d		perp;
	geVec3d		in;

	v[0].u = 0.0f;
	v[0].v = 0.0f;
	v[1].u = 1.0f;
	v[1].v = 0.0f;
	v[2].u = 1.0f;
	v[2].v = 1.0f;
	v[3].u = 0.0f;
	v[3].v = 1.0f;

	for	(i = 0; i < 4; i++)
	{
		v[i].r = (geFloat)r;
		v[i].g = (geFloat)g;
		v[i].b = (geFloat)b;
		v[i].a = 255.0f;
	}

	geVec3d_Subtract(p1, p2, &perp);
	geXForm3d_GetIn(M, &in);
	geVec3d_CrossProduct(&perp, &in, &perp);
	geVec3d_Normalize(&perp);
//	geVec3d_Scale(&perp, 4.0f / 2.0f, &perp);
	geVec3d_Scale(&perp, 2.0f / 2.0f, &perp);

	v[0].X = p2->X - perp.X;
	v[0].Y = p2->Y - perp.Y;
	v[0].Z = p2->Z - perp.Z;
	v[1].X = p2->X + perp.X;
	v[1].Y = p2->Y + perp.Y;
	v[1].Z = p2->Z + perp.Z;
	v[2].X = p1->X + perp.X;
	v[2].Y = p1->Y + perp.Y;
	v[2].Z = p1->Z + perp.Z;
	v[3].X = p1->X - perp.X;
	v[3].Y = p1->Y - perp.Y;
	v[3].Z = p1->Z - perp.Z;

	SetRGB(v[0], r1, g1, b1);
	SetRGB(v[1], r1, g1, b1);
	SetRGB(v[2], r, g, b);
	SetRGB(v[3], r, g, b);

	geWorld_AddPolyOnce(World,
						v,
						4,
						NULL,
						GE_GOURAUD_POLY,
						GE_RENDER_DO_NOT_OCCLUDE_OTHERS,
						1.0f);
}

geBoolean PathPt_Reset(geWorld *World)
{
	if	(!World)
		return GE_TRUE;

	memset(TrackList,0,sizeof(TrackList[0])*MAX_TRACKS);
	TrackCount = 0;

	return GE_TRUE;
}

static geBoolean PathPoint_SetTexture(geWorld *World, geVFile *MainFS)
{
	geEntity_EntitySet	*Set;
	geBitmap			*ABitmap;

	ABitmap = NULL;

	if	(World == NULL)
		return GE_TRUE;

	Set = geWorld_GetEntitySet(World, "PathPoint");
	if	(Set == NULL)
		return GE_TRUE;

	PathPointTexture = geBitmap_CreateFromFileName(MainFS, "Bmp\\Corona.Bmp");

	if (!PathPointTexture)
	{
		goto ExitWithError;
	}

	ABitmap = geBitmap_CreateFromFileName(MainFS, "Bmp\\Corona.Bmp");

	if (!ABitmap)
	{
		goto ExitWithError;
	}

	if (!geBitmap_SetAlpha(PathPointTexture, ABitmap))
	{
		goto ExitWithError;
	}

	geBitmap_Destroy(&ABitmap);
	ABitmap = NULL;

	if (!geWorld_AddBitmap(World, PathPointTexture))
	{
		goto ExitWithError;
	}

	return GE_TRUE;

	// Error clean up
	ExitWithError:
	{
		if (PathPointTexture)
		{
			geBitmap_Destroy(&PathPointTexture);
			PathPointTexture = NULL;
		}

		if (ABitmap)
		{
			geBitmap_Destroy(&ABitmap);
			ABitmap = NULL;
		}

		return GE_FALSE;
	}
}


geBoolean PathPt_Startup(geWorld *World, geVFile *Fs)
{
	void				*ClassData;
	geEntity_EntitySet	*ClassSet;
	geEntity			*Entity;
	PathPoint			*Point;
	
	int pointnum=0;
	TrackPt *track_pt;

	if	(!World)
		return GE_TRUE;

	PathPt_Reset(World);

	// Look for the class name in the world
	ClassSet = geWorld_GetEntitySet(World, "PathPoint");

    if (!ClassSet)
        return GE_TRUE;

	Entity = NULL;

	while (1)
	{
		Entity = geEntity_EntitySetGetNextEntity(ClassSet, Entity);

		if (!Entity)
			break;

		ClassData = geEntity_GetUserData(Entity);
		Point = (PathPoint*)ClassData;

		assert(Point);

		// initialize translated position
		Point->Pos = Point->origin;

		if (Point->PathType <= 0)
			continue;

		//save off header info
		TrackList[TrackCount].Type = Point->PathType;
        pointnum = 0;

		//move through points setting up the list
		do
		{
			//save off point info
			track_pt = &TrackList[TrackCount].PointList[pointnum];
			// initialize translated position
			Point->Pos = Point->origin;
			track_pt->Pos = &Point->Pos;
			if (Point->WatchPoint)
				track_pt->WatchPos = &Point->WatchPoint->origin;

			track_pt->Action = Point->ActionType;
			track_pt->Time = Point->Time;
            track_pt->ActionDir = Point->Direction;
			track_pt->Dist = Point->Dist;
			track_pt->VelocityScale = Point->VelocityScale;

			// automatic setup for doors
			if (TrackList[TrackCount].Type == TRACK_TYPE_TRAVERSE_DOOR)
				{
				if (pointnum == 0)
					{
					track_pt->ActionDir = 1;
					track_pt->Action = POINT_TYPE_WAIT_POINT_VISIBLE;
					track_pt->WatchPos = &Point->Next->origin;
					}
				else
				if (pointnum == 1)
					{
					track_pt->ActionDir = -1;
					track_pt->Action = POINT_TYPE_WAIT_POINT_VISIBLE;
					track_pt->WatchPos = (track_pt-1)->Pos;
					}
				}

			pointnum++;
			TrackList[TrackCount].PointCount = pointnum;

            assert(pointnum < MAX_TRACK_POINTS);

			Point = Point->Next;

			// Found a type
			if (Point && Point->PathType >= 0)
				{
				track_pt = &TrackList[TrackCount].PointList[pointnum];
				SET(track_pt->Flags, BIT(0));
				}

		}while(Point && Point != (PathPoint*)ClassData);

		TrackCount++;
		assert(TrackCount < MAX_TRACKS);
	}

	Entity = NULL;

	{
		void Track_LinkTracks(geWorld *World);
		Track_LinkTracks(World);
	}

	if (Fs)
	{
		if (!PathPoint_SetTexture(World, Fs))
			return GE_FALSE;
	}

	return GE_TRUE;
}

geBoolean PathPt_Shutdown(void)
{
	if (PathPointTexture)
	{
		// NOTE - We really should remove it from the world, but when the world is destroyed
		// it removes all leftover bitmaps, so we are free to just let that happen...
		geBitmap_Destroy(&PathPointTexture);
		PathPointTexture = NULL;
	}

	return GE_TRUE;
}

geBoolean PathPt_Frame(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime)
{
	geEntity_EntitySet *	Set;
	geEntity *				Entity;

	PathPoint *	P;
	geVec3d		Pos;
	geVec3d	Center;
	extern geBoolean PathLight;
	extern geBoolean MultiPathLight;

	if	(!World)
		return GE_TRUE;
 
	Set = geWorld_GetEntitySet(World, "PathPoint");

	if	(Set == NULL)
		return GE_TRUE;

	Entity = NULL;

	while	(1)
	{
		Entity = geEntity_EntitySetGetNextEntity(Set, Entity);

		if (!Entity)
			break;

		P = static_cast<PathPoint*>(geEntity_GetUserData(Entity));

		if	(P->MoveWithModel)
		{
			geXForm3d	XForm;

			geWorld_GetModelXForm(World, P->MoveWithModel, &XForm);

			Pos = P->origin;

			geWorld_GetModelRotationalCenter(World, P->MoveWithModel, &Center);
			geVec3d_Subtract(&Pos, &Center, &Pos);
			geXForm3d_Transform(&XForm, &Pos, &Pos);
			geVec3d_Add(&Pos, &Center, &P->Pos);
		}
	}

	if (XForm)
	{
		if (PathLight)
			PathPoint_Frame2(World, XForm, DeltaTime);
		if (MultiPathLight)
			PathPoint_Frame3(World, XForm, DeltaTime);
	}


	return GE_TRUE;
}

static	geBoolean PathPoint_Frame2(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime)
{
	GE_Collision			Collision;
	int						i;
	geBoolean	Visible;
	geFloat		DistanceToCorona;
	geVec3d		Delta;

	if	(!World)
		return GE_TRUE;
 
	// Now walk the track list, drawing lines to connect them all.  Draw red
	// lines to things that are being watched.

	for	(i = 0; i < TrackCount; i++)
	{
		int		j;
		Track *	T;

		T = &TrackList[i];
		assert(T->PointCount > 0);

		for	(j = 0; j < T->PointCount; j++)
		{
			GE_LVertex	Vert;
			float Radius;

			DistanceToCorona = geVec3d_Length(&Delta);

			if	(!geWorld_Collision(World,
									NULL,
									NULL,
									T->PointList[j].Pos,
									&XForm->Translation,
									GE_CONTENTS_CANNOT_OCCUPY,
									GE_COLLIDE_MODELS,
									0xffffffff, NULL, NULL, 
									&Collision) &&
				 (DistanceToCorona < 2000.0f))//C->MaxVisibleDistance))
				{
				Visible = GE_TRUE;
				}
			else
				{
				Visible = GE_FALSE;
				}

			
			if (Visible)
				{
				Vert.X = T->PointList[j].Pos->X;
				Vert.Y = T->PointList[j].Pos->Y;
				Vert.Z = T->PointList[j].Pos->Z;

				if (j != 0 && TEST(T->PointList[j].Flags, BIT(0)))
					{
					Vert.g = 0.0f;
					Vert.b = 0.0f;
					Radius = 5.0f;
					}
				else
					{
					Vert.g = 255.0f;
					Vert.b = 255.0f;
					Radius = 1.0f;
					}

				Vert.r = 255.0f;
				Vert.a = 255.0f;
				Vert.u = Vert.v = 0.0f;

				geWorld_AddPolyOnce(World,
									&Vert,
									1,
									PathPointTexture,
									GE_TEXTURED_POINT,
									GE_RENDER_DO_NOT_OCCLUDE_OTHERS | GE_RENDER_DO_NOT_OCCLUDE_SELF,
									Radius);
				}


			if (j < T->PointCount - 1)
				{
				DrawLine3d(World,
						   XForm, 
						   T->PointList[j].Pos,
						   T->PointList[j + 1].Pos,
						   255, 255, 255,
						   0, 255, 0);
				}

			if	(T->PointList[j].WatchPos)
				{
				DrawLine3d(World,
						   XForm, 
						   T->PointList[j].Pos,
						   T->PointList[j].WatchPos,
						   255, 255, 255,
						   255, 0, 0);
				}
		}
	}

	return GE_TRUE;
}


static	geBoolean PathPoint_Frame3(geWorld *World, const geXForm3d *XForm, geFloat DeltaTime)
{
	int	i,v;

	geVec3d *sp, *ep, *ep2, *sp2;

	if	(!World)
		return GE_TRUE;
 
	// Now walk the track list, drawing lines to connect them all.  Draw red
	// lines to things that are being watched.

	for	(i = 0; i < TrackCount; i++)
	    {
		Track *	T;

		T = &TrackList[i];
		assert(T->PointCount > 0);

		sp = T->PointList[0].Pos;
		ep = T->PointList[T->PointCount - 1].Pos;

        for (v = 0; T->Vis[v]; v++)
            {
			sp2 = T->Vis[v]->PointList[0].Pos;
			ep2 = T->Vis[v]->PointList[T->Vis[v]->PointCount - 1].Pos;

			switch(T->VisFlag[v])
				{
				case 1:
					DrawLine3d(World,
							   XForm, 
							   sp,
							   sp2,
							   255, 255, 255,
							   0, 0, 255);
					break;
				case 2:
					DrawLine3d(World,
							   XForm, 
							   sp,
							   ep2,
							   255, 255, 255,
							   0, 0, 255);
					break;
				case 3:
					DrawLine3d(World,
							   XForm, 
							   ep,
							   ep2,
							   255, 255, 255,
							   0, 0, 255);
					break;
				case 4:
					DrawLine3d(World,
							   XForm, 
							   ep,
							   sp2,
							   255, 255, 255,
							   0, 0, 255);
					break;
				}
            }
    	}

	return GE_TRUE;
}

