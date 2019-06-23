/****************************************************************************************/
/*  GPREVIEW.C                                                                          */
/*                                                                                      */
/*  Description:  Simple Genesis level viewer.                                          */
/*                                                                                      */
/*  Copyright (c) 1999, WildTangent Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/


#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(disable : 4201 4214 4115)
#include <mmsystem.h>
#pragma warning(default : 4201 4214 4115)

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include "genesis.h"
#include "ram.h"
#include "errorlog.h"

#include "drvlist.h"
#include "function.h"
#include "resource.h"

#pragma warning (disable:4514)	// unreferenced inline function (caused by Windows)


#define	USER_ALL	0xffffffff

// Enumeration determines kind of world object.
// Used by WorldObject handler to process different object types.
typedef	enum
{
	WO_NONE	= 0,
	WO_MODEL,
}	WorldObjectKind;

typedef struct	WorldObject
{
	WorldObjectKind	woKind;
	geVec3d			woImpactPoint;
	union
	{
		geWorld_Model *	woModel;
	}	o;
}	WorldObject;

typedef	struct	Model_Animator
{
	Function_RTFunction *	Function;
	geWorld_Model *			Model;
	geWorld *				World;
	geFloat					TimeRange;
}	Model_Animator;

typedef	struct	DynamicLight
{
	geLight *	dlLight;
	geVec3d		dlPosition;
	geVec3d		dlDirection;
	float		dlSpeed;
	GE_RGBA		dlRGB;
	geFloat		dlIntensity;
	geWorld *	dlWorld;
}	DynamicLight;


typedef	struct	Cursor
{
	geEngine *	Engine;
	geBitmap *	Decal;
	int			ScreenX;
	int			ScreenY;
}	Cursor;

#define	TEXT_HEIGHT	13
#define	TEXTLINE(x) (2 + (x) * TEXT_HEIGHT)

// Misc global info
static	HWND			GDemohWnd;
static	HINSTANCE		GDemoInstance;
static	BOOL			Running			= TRUE;
static	char			*AppName		= "Genesis - Level Previewer";
static	char			LevelName[MAX_PATH];
static	int32			CWidth;
static	int32			CHeight;

// Current location and orientation
static	float			XRot = 0.0f;
static	float			YRot = 0.0f;
static	geVec3d			LocalPos;
static	geXForm3d		MyXForm;
static	geFloat			Velocity = 20.0f;

// Our size
static	const geVec3d	Maxs = { 5.0f,  5.0f,  5.0f};
static	const geVec3d	Mins = {-5.0f, -5.0f, -5.0f};


//	Genesis Objects
static	geEngine		*Engine;
static	geWorld			*World;
static	geCamera		*Camera;

// Status flags
static	geBoolean		ShowHelp = GE_FALSE;
static	geBoolean		ShowStats = GE_TRUE;
static	geBoolean		DoFrameRateCounter = GE_FALSE;
static	geBoolean		HeadLightOn = GE_FALSE;


// The headlight is just a dynamic light at our position.
static	geLight			*HeadLight;
static	GE_RGBA			HeadLightColor;
static	geFloat			HeadLightIntensity;

// The "spot" texture used for lights and active model identification
static	geBitmap		*SpotTexture;

static	Cursor			*CrossHairCursor;

// Dynamic light that moves when fired
static	DynamicLight	*Missile;

// World object for animating selected model
static	WorldObject		SelectedWorldObject;
static	Model_Animator	*Animator;



static	void MyError(const char *Msg, ...);

// Return GE_TRUE if we're currently upside-down
static	geBoolean	UpIsDown(void)
{
	return (MyXForm.BY < 0.0f) ? GE_TRUE : GE_FALSE;
}

// return TRUE if the specified key is down
static geBoolean IsKeyDown (int KeyCode, HWND hWnd)
{
	if (GetFocus() == hWnd)
	{
		if (GetAsyncKeyState(KeyCode) & 0x8000)
		{
			return GE_TRUE;
		}
	}

	return GE_FALSE;
}

// Return TRUE if currently animating (looking for model to animate)
static	geBoolean InAimMode(void)
{
	return IsKeyDown(VK_SPACE, GDemohWnd);
}

//
// Loads a bitmap from an RCDATA resource.
// This loads the resource into memory and then opens it as a memory file.
static geBitmap *gpreview_LoadBitmapFromResource (const int ResourceId)
{
	HRSRC hRes;
	HGLOBAL hGlbl;
	void *ResourceData = NULL;
	DWORD ResourceSize = 0;

	// Locate and load the named resource.
	hRes = FindResource (GDemoInstance, MAKEINTRESOURCE (ResourceId), RT_RCDATA);
	if (hRes != NULL)
	{
		hGlbl = LoadResource (GDemoInstance, hRes);
		if (hGlbl != NULL)
		{
			ResourceData = LockResource (hGlbl);
			if (ResourceData != NULL)
			{
				ResourceSize = SizeofResource (GDemoInstance, hRes);
			}
		}
	}
	if (ResourceSize == 0)
	{
		// something failed.  Too bad.
		return NULL;
	}

	// Open the memory pointed to by ResourceData as though it's a memory file.
	{
		geBitmap *bmp = NULL;
		geVFile_MemoryContext MemContext;
		geVFile *BmpFile;

		MemContext.Data = ResourceData;
		MemContext.DataLength = ResourceSize;
		BmpFile = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_MEMORY, NULL, &MemContext, GE_VFILE_OPEN_READONLY);
		if (BmpFile != NULL)
		{
			bmp = geBitmap_CreateFromFile (BmpFile);
			geVFile_Close (BmpFile);
		}
		return bmp;
	}
}


static	void	DrawPoint
	(
		geWorld *		World,
		geBitmap *	Texture,
		const geVec3d *	Pos,
		int				r,
		int				g,
		int				b
	)
{
	GE_LVertex	Vertex;

	Vertex.r = (geFloat)r;
	Vertex.g = (geFloat)g;
	Vertex.b = (geFloat)b;
	Vertex.a = 255.0f;

	Vertex.u = Vertex.v = 0.0f;

	geVec3d_Copy (Pos, (geVec3d *)&Vertex.X);

	geWorld_AddPolyOnce (World, &Vertex, 1, Texture, GE_TEXTURED_POINT, 0, 1.0f);
}


Model_Animator *	Model_AnimatorCreate (geWorld_Model *Model, geWorld *World)
{
	Model_Animator *	Animator;
	geFloat				StartTime;
	geFloat				EndTime;
	geMotion *			Motion;

	Animator = GE_RAM_ALLOCATE_STRUCT (Model_Animator);
	if	(Animator == NULL)
		return Animator;

	Animator->Model = Model;
	Animator->World = World;

	Motion = geWorld_ModelGetMotion(Model);
	assert(Motion != NULL);

	geMotion_GetTimeExtents(Motion, &StartTime, &EndTime);
	Animator->TimeRange = EndTime;

	Animator->Function = Function_RTFunctionCreateTriangleOscillator(2*EndTime, 0.0f, 1.0f);
	if	(!Animator->Function)
	{
		geRam_Free (Animator);
		return NULL;
	}

	return Animator;
}

void	Model_AnimatorDestroy(Model_Animator *Animator)
{
	assert(Animator != NULL);
	assert(Animator->Function != NULL);

	Function_RTFunctionDestroy(Animator->Function);
	geRam_Free (Animator);
}

void	Model_AnimatorFrame(Model_Animator *Animator)
{
	geXForm3d	XForm;
	geMotion *	Motion;

	Motion = geWorld_ModelGetMotion(Animator->Model);

	gePath_Sample(geMotion_GetPath(Motion, 0),
							  Function_RTFunctionValue(Animator->Function) * Animator->TimeRange,
							  &XForm);

	geWorld_SetModelXForm(Animator->World, Animator->Model, &XForm);
}

void	Cursor_Destroy(Cursor *curs)
{
	assert(curs != NULL);

	if (curs->Decal != NULL)
	{
		geBitmap_Destroy (&curs->Decal);
	}
	geRam_Free (curs);
}

Cursor *	Cursor_Create (geEngine *Engine, const int ResourceId)
{
	Cursor *curs;

	assert(Engine != NULL);
	
	curs = GE_RAM_ALLOCATE_STRUCT (Cursor);
	if (curs != NULL)
	{
		geBoolean NoErrors = GE_FALSE;

		memset(curs, 0, sizeof(*curs));

		curs->Engine = Engine;

		curs->Decal = gpreview_LoadBitmapFromResource (ResourceId);
		if	(curs->Decal != NULL)
		{
			if (geEngine_AddBitmap (Engine, curs->Decal) != GE_FALSE)
			{
				NoErrors = GE_TRUE;
			}
		}
		if (NoErrors == GE_FALSE)
		{
			// if any error occurred, then exit.
			Cursor_Destroy (curs);
			curs = NULL;
		}
	}
	return curs;	
}

void	Cursor_SetPosition(Cursor *curs, int X, int Y)
{
	curs->ScreenX = X;
	curs->ScreenY = Y;
}

void	Cursor_GetPosition(Cursor *curs, int *X, int *Y)
{
	*X = curs->ScreenX;
	*Y = curs->ScreenY;
}

void	Cursor_Render(Cursor *curs)
{
	GE_Rect	Rect;

	assert(curs);
	assert(curs->Engine);
	assert(curs->Decal);

	Rect.Left = 0;
	Rect.Top = 0;
	Rect.Right = 16;
	Rect.Bottom = 16;

	geEngine_DrawBitmap (curs->Engine, curs->Decal, &Rect, curs->ScreenX, curs->ScreenY);
}

//=====================================================================================
//	GetCmdLine
//=====================================================================================
static char *GetCmdLine(char *CmdLine, char *Data, BOOL flag)
{
	int32 dp = 0;
	char ch;

	for	(;;)
	{
		ch = *CmdLine++;
		if (ch == ' ')
			continue;
		if (ch == '-' && !flag) 
			break;
		if (flag && ch != ' ')
		{
			CmdLine--;
			break;
		}
		if (ch == 0 || ch == '\n')
			break;
	}

	if (ch == 0 || ch == '\n')
		return FALSE;

	while (ch != ' ' && ch != '\n' && ch != 0)
	{
		ch = *CmdLine++;
		Data[dp++] = ch;
	}

	Data[dp-1] = 0;

	return CmdLine;
		
}


static	void	DestroyDynamicLight(DynamicLight *dl)
{
	geWorld_RemoveLight(dl->dlWorld, dl->dlLight);
	geRam_Free (dl);
}

static	DynamicLight *	CreateDynamicLight(
	geWorld *	world,
	const geVec3d *position,
	const geVec3d *direction,
	GE_RGBA *	rgb,
	geFloat	intensity,
	geFloat	speed)
{
	DynamicLight *	dl;

	dl = GE_RAM_ALLOCATE_STRUCT (DynamicLight);
	if	(!dl)
		return dl;

	dl->dlPosition	= *position;
	dl->dlDirection	= *direction;
	dl->dlLight		= geWorld_AddLight(world);
	dl->dlSpeed		= speed;
	dl->dlRGB		= *rgb;
	dl->dlIntensity = intensity;
	dl->dlWorld		= world;

	geVec3d_Normalize(&dl->dlDirection);
	
	geWorld_SetLightAttributes(world, dl->dlLight, position, rgb, intensity, GE_FALSE);

	return dl;
}

static	void	Help (void)
{
	static	const char * HelpStrings[] =
	{
		" Help",
		" <esc>      Exit                              ",
		" r/R    -/+ Red light level                   ",
		" g/G    -/+ Green light level                 ",
		" b/B    -/+ Blue light level                  ",
		" i/I    -/+ Light intensity                   ",
		" v/V    -/+ Velocity                          ",
		" h          Toggle headlight                  ",
		" s          Screen shot (SS000.BMP)           ",
		" f          Toggle FPS counter                ",
		" t          Toggle Information                ",
		" F1         Toggle help                       ",
		" (shift arrows disable camera collisions)     ",
	};
	int	i;

	for	(i = 0; i < sizeof(HelpStrings) / sizeof(HelpStrings[0]); i++)
	{
		#pragma message ("Ugly cast 'cause geEngine_Printf is not const correct.")
		geEngine_Printf (Engine, 5, TEXTLINE(i), (char *)HelpStrings[i]);
	}
}

static	void	Stats(void)
{
	geEngine_Printf(Engine, 0, 480 - TEXTLINE(2), SelectedDriverString);
	geEngine_Printf(Engine, 0, 480 - TEXTLINE(1), "                                                                           ");
	geEngine_Printf(Engine, 5, 480 - TEXTLINE(1), "Speed: %5d        Headlight is %s   F1 for help", (int)Velocity,HeadLightOn ? "on" : "off");
	//geEngine_Printf(Engine, 200, 480 - TEXTLINE(1), "Headlight is %s", HeadLightOn ? "on" : "off");
}

static	int		UpdateDynamicLight (DynamicLight *dl)
{
	GE_LVertex		vert;
	geVec3d			newPos;
	GE_Collision	collision;
	geBoolean rslt;

	geVec3d_AddScaled (&dl->dlPosition, &dl->dlDirection, dl->dlSpeed, &newPos);
	
	rslt = geWorld_Collision 
		(
			dl->dlWorld, 
			NULL, 
			NULL, 
			&dl->dlPosition, 
			&newPos,
			GE_CONTENTS_SOLID_CLIP,
			GE_COLLIDE_ALL,
			USER_ALL,
			NULL,
			NULL,
			&collision
		);

	if	(rslt != GE_FALSE)
	{
		// light collided with something...must be done
		return 1;
	}

	// It didn't collide, so update its position and display it
	dl->dlPosition = newPos;

	geWorld_SetLightAttributes(dl->dlWorld, dl->dlLight, &dl->dlPosition, &dl->dlRGB, dl->dlIntensity, GE_FALSE);

	vert.X = dl->dlPosition.X;
	vert.Y = dl->dlPosition.Y;
	vert.Z = dl->dlPosition.Z;
	vert.u =
	vert.v = 0.0f;
	vert.r = dl->dlRGB.r;
	vert.g = dl->dlRGB.g;
	vert.b = dl->dlRGB.b;
	vert.a = 80.0f;
	geWorld_AddPolyOnce (dl->dlWorld, &vert, 1, SpotTexture, GE_TEXTURED_POINT, 0, 1.0f);

	return 0;
}

static	BOOL	ShiftKeyPressed(void)
{
	if	(GetKeyState(VK_SHIFT) & 0x8000)
	{
		return TRUE;
	}

	return FALSE;
}

static	void	Clamp(geFloat *num, geFloat low, geFloat high)
{
	if	(*num < low)
		*num = low;
	else if	(*num > high)
		*num = high;
}

// Display info about selected world object
static	void	Info(void)
{
	switch	(SelectedWorldObject.woKind)
	{
		geMotion *	Motion;

		case	WO_NONE:
			break;

		case	WO_MODEL:
			geEngine_Printf(Engine, 20, 480 - TEXTLINE(3), "Model: %p", SelectedWorldObject.o.woModel);
			Motion = geWorld_ModelGetMotion(SelectedWorldObject.o.woModel);
			DrawPoint (World, SpotTexture, &SelectedWorldObject.woImpactPoint, 255, 0, 0);
			if	(Motion)
			{
				geFloat	StartTime;
				geFloat	EndTime;

				geMotion_GetTimeExtents(Motion, &StartTime, &EndTime);
				geEngine_Printf(Engine, 20, 480 - TEXTLINE(2), "  Has motion. End time: %f", EndTime);
			}
			break;

		default:
			assert(!"What kind is this?");
			break;
	}
}


static	geBoolean	Collide(geWorld *World, const geVec3d *Front, const geVec3d *Back, GE_Collision *Collision)
{
	geVec3d		Direction;
	geVec3d		InBetween;
	geFloat	Length;
	geFloat	InBetweenLength;

	geVec3d_Subtract(Back, Front, &Direction);
	Length = geVec3d_Normalize(&Direction);
	geVec3d_Scale(&Direction, 5.0f, &Direction);
	geVec3d_Copy(Front, &InBetween);
	InBetweenLength = 5.0f;
	while	(InBetweenLength < Length)
	{
		if	(geWorld_Collision(World, NULL, NULL, Front, &InBetween, GE_CONTENTS_SOLID_CLIP, GE_COLLIDE_ALL, USER_ALL, NULL, NULL, Collision))
			return GE_TRUE;
		geVec3d_AddScaled(Front, &Direction, InBetweenLength, &InBetween);
		InBetweenLength += 5.0f;
	}

	return geWorld_Collision(World, NULL, NULL, Front, Back, GE_CONTENTS_SOLID_CLIP, GE_COLLIDE_ALL, USER_ALL, NULL, NULL, Collision);
}

static	void	SelectThing(int X, int Y)
{
	geVec3d			Direction;
	GE_Collision	Collision;

	geCamera_ScreenPointToWorld (Camera, X, Y, &Direction);
	geVec3d_AddScaled (&MyXForm.Translation, &Direction, 5000.0f, &Direction);

	SelectedWorldObject.woKind = WO_NONE;
	if	(Collide (World, &MyXForm.Translation, &Direction, &Collision))
	{
		SelectedWorldObject.woImpactPoint = Collision.Impact;
		if	(Collision.Model)
		{
			SelectedWorldObject.woKind = WO_MODEL;
			SelectedWorldObject.o.woModel = Collision.Model;

			if	(Animator != NULL)
			{
				Model_AnimatorDestroy(Animator);
				Animator = NULL;
			}
			if	(geWorld_ModelGetMotion(Collision.Model) != NULL)
         		Animator = Model_AnimatorCreate(Collision.Model, World);
		}
	}
}

//=====================================================================================
//	SetupCamera
//=====================================================================================
static void SetupCamera(geCamera *Camera, GE_Rect *Rect, geXForm3d *XForm)
{
	geCamera_SetWorldSpaceXForm(Camera, XForm);
//	geCamera_SetXForm (Camera, XForm);
	geCamera_SetAttributes(Camera, 2.0f, Rect);
}

//=====================================================================================
//	SetupXForm
//=====================================================================================
static void SetupXForm(float XRot, float YRot, geVec3d *Pos, geXForm3d *XForm)
{
	geXForm3d_SetIdentity(XForm);

	geXForm3d_RotateX(XForm, XRot);
	geXForm3d_RotateY(XForm, YRot);
	geXForm3d_Translate(XForm, Pos->X, Pos->Y, Pos->Z);
}

//=====================================================================================
//	WndProc
//=====================================================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	geVec3d	in;

	static int32 last_x = 0;
	static int32 last_y = 0;

	switch(iMessage)
	{
		case WM_LBUTTONDOWN:
			if (InAimMode())
			{
				SelectThing (CrossHairCursor->ScreenX, CrossHairCursor->ScreenY);
			}
			else
			{
				geXForm3d_GetIn (&MyXForm, &in);
				if (Missile != NULL)
				{
					// if there's currently a missile, then destroy it
					DestroyDynamicLight(Missile);
				}
				Missile = CreateDynamicLight (World, &LocalPos, &in, &HeadLightColor, HeadLightIntensity, 25.0f);
			}
			break;

		case WM_MOUSEMOVE:
		{
			int32 x = LOWORD(lParam);
			int32 y = HIWORD(lParam);

			if	(InAimMode())
			{
				Cursor_SetPosition (CrossHairCursor, x, y);
				last_x = x;
				last_y = y;
			}
			else
			{
				float dx = ((float) (last_x - x) / 150.0f);
				float dy = ((float) (last_y - y) / 150.0f);

				last_x = x;
				last_y = y;

				if ((x > CWidth - 80) || (x <= 80))
				{
					SetCursorPos(CWidth / 2, y);

					x = last_x = CWidth / 2;
					y = last_y = y;
				}

				if ((y > CHeight - 80) || (y <= 80))
				{
					SetCursorPos(x, CHeight / 2);

					x = last_x = x;
					y = last_y = CHeight / 2;
				}

				if	(UpIsDown())
					YRot -= dx;
				else
					YRot += dx;
				XRot += dy;
			}
			
			break;
		}

		case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_ESCAPE :
					Running = FALSE;
					break;

				case VK_SPACE :
					Cursor_SetPosition (CrossHairCursor, last_x, last_y);
					break;

				case VK_F1 :
					ShowHelp = !ShowHelp;
					break;

				case 'I' :
					HeadLightIntensity += ShiftKeyPressed() ? 10.0f : -10.0f;
					Clamp(&HeadLightIntensity, 0.0f, 100000.0f);
					break;

				case 'R' :
					HeadLightColor.r += ShiftKeyPressed() ? 5.0f : -5.0f;
					Clamp(&HeadLightColor.r, 0.0f, 255.0f);
					break;

				case 'G' :
					HeadLightColor.g += ShiftKeyPressed() ? 5.0f : -5.0f;
					Clamp(&HeadLightColor.g, 0.0f, 255.0f);
					break;

				case 'B' :
					HeadLightColor.b += ShiftKeyPressed() ? 5.0f : -5.0f;
					Clamp(&HeadLightColor.b, 0.0f, 255.0f);
					break;

				case 'H' :
					HeadLightOn = !HeadLightOn;
					break;

				case 'S' :
					geEngine_ScreenShot(Engine, "SS000.bmp");
					break;

				case 'F' :
					DoFrameRateCounter = !DoFrameRateCounter;
					geEngine_EnableFrameRateCounter(Engine, DoFrameRateCounter);
					break;

				case 'V' :
					Velocity += ShiftKeyPressed() ? 1.0f : -1.0f;
					Clamp(&Velocity, 1.0f, 50.0f);
					break;

				case 'T' :
					ShowStats = !ShowStats;
					break;
			}
			break;
		}

		default:
			return DefWindowProc (hWnd, iMessage, wParam, lParam);
	}
	return 0;
}

static HWND CreateMainWindow(HANDLE hInstance, char *AppName, int32 Width, int32 Height)
{
	WNDCLASS		wc;
	HWND			hWnd;
	RECT			WindowRect;
	int				Style;

	//
	// Set up and register application window class
	//

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = WndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(hInstance, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = (const char*)NULL;
	wc.lpszClassName = AppName;

	RegisterClass(&wc);

	Style = WS_OVERLAPPEDWINDOW;
	//
	// Create application's main window
	//

	hWnd = CreateWindowEx(
		0,
		AppName,
		AppName,
		Style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL);

	if (!hWnd)
	{
		MessageBox(0, "Could not create window.", "** ERROR **", MB_OK);
		_exit(1);
	}	


	UpdateWindow(hWnd);
	SetFocus(hWnd);

	// Adjust window size so that the client area is Width x Height
	WindowRect.left = 0;
	WindowRect.top = 0;
	WindowRect.right = Width - 1;
	WindowRect.bottom = Height - 1;
	AdjustWindowRect (&WindowRect, Style, FALSE);
	
	{
		int WindowWidth = WindowRect.right - WindowRect.left + 1;
		int WindowHeight = WindowRect.bottom - WindowRect.top + 1;
 
		SetWindowPos 
		(
			hWnd, 0,
			0, 0, 
			WindowWidth, WindowHeight,
			SWP_NOCOPYBITS | SWP_NOZORDER
		);
	}

	//
	// Make window visible
	//
	ShowWindow(hWnd, SW_SHOWNORMAL);

	return hWnd;

}

//=====================================================================================
//	ShutdownAll
//=====================================================================================
static void ShutdownAll(void)
{
	// Free each object (sub objects are freed by their parents...)
	if (SpotTexture != NULL)
	{
		geBitmap_Destroy (&SpotTexture);
	}

	if (CrossHairCursor != NULL)
	{
		Cursor_Destroy (CrossHairCursor);
	}

	if (Missile != NULL)
	{
		DestroyDynamicLight (Missile);
		Missile = NULL;
	}

	if (Camera)
	{
		geCamera_Destroy (&Camera);
	}

	if (World)
	{
		geWorld_Free (World);
	}

	if (Engine)
	{
		geEngine_Free (Engine);
	}


	Camera		= NULL;
	World		= NULL;
	Engine		= NULL;
}

//=====================================================================================
//	MovePos
//=====================================================================================
static void MovePos(geVec3d *Pos, geXForm3d *XForm)
{
	geVec3d	LVect, UVect, InVect;
	geVec3d	Pos2 = *Pos;

	geXForm3d_GetLeft(XForm, &LVect);
	geXForm3d_GetUp(XForm, &UVect);
	geXForm3d_GetIn(XForm, &InVect);

	if (IsKeyDown(VK_UP, GDemohWnd))	
		geVec3d_AddScaled(Pos, &InVect,  Velocity, &Pos2);
	
	if (IsKeyDown(VK_DOWN, GDemohWnd))	
		geVec3d_AddScaled(Pos, &InVect, -Velocity, &Pos2);

	
	// Right and left keys turn around
	// If shift key held, then they strafe
	// Note that because the Shift key is also used to turn off clipping,
	// no world clipping is performed when strafing.
	if (IsKeyDown(VK_LEFT, GDemohWnd))
	{
		if (IsKeyDown(VK_SHIFT, GDemohWnd))
			geVec3d_AddScaled(Pos, &LVect, Velocity, &Pos2);
		else if (UpIsDown ())
			YRot -= 0.1f;
		else
			YRot += 0.1f;
	}

	if (IsKeyDown(VK_RIGHT, GDemohWnd))
	{
		if (IsKeyDown(VK_SHIFT, GDemohWnd))
			geVec3d_AddScaled(Pos, &LVect, -Velocity, &Pos2);
		else if (UpIsDown ())
			YRot += 0.1f;
		else
			YRot -= 0.1f;
	}

	if (IsKeyDown('Q', GDemohWnd))
	{
		Pos2.Y += .001f;
	}

	// if the Shift key isn't down, then clip to the world
	// If holding the shift, then we can walk through walls.
	if (!IsKeyDown(VK_SHIFT, GDemohWnd))
	{
		GE_Collision Collision;
		if	(geWorld_Collision (World, &Mins, &Maxs, Pos, &Pos2, GE_CONTENTS_SOLID_CLIP, GE_COLLIDE_ALL, USER_ALL, NULL, NULL, &Collision) == GE_FALSE)
		{
			*Pos = Pos2;
		}
	}
	else
	{
		*Pos = Pos2;
	}

	// A and Z look up and down.
	if (IsKeyDown('A', GDemohWnd))
	{
		XRot += 0.1f;
	}

	if (IsKeyDown('Z', GDemohWnd))
	{
		XRot -= 0.1f;
	}
}

//=====================================================================================
//	WinMain
//=====================================================================================
#pragma warning (disable:4100)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpszCmdParam, int nCmdShow)
{
	MSG				Msg;
	char			*CmdLine = lpszCmdParam;
	char			Data[256];
	GE_Rect			Rect;
	geDriver *		Driver;
	geDriver_Mode *	Mode;
	int				KeepTryingToGetAGoodMode ;
	*LevelName = 0;

	GDemoInstance = hInstance;

	for	(;;)
	{	
		if ((CmdLine = GetCmdLine (CmdLine, Data, FALSE)) == NULL)
		{
			break;
		}

		if (!stricmp(Data, "Map"))
		{
			// Get the demo name
			if ((CmdLine = GetCmdLine (CmdLine, Data, TRUE)) == NULL)
			{
				MyError("No map name specified on command line.");
			}

			strcpy(LevelName, Data);
		}
		else
		{
			MyError("Unknown Option: %s.", Data);
		}
	}
	
	if (LevelName[0] == 0)
	{
		MyError("No map name specified on command line.\n");
	}
	
	//
	//	Create a window
	//
	CWidth = 640;
	CHeight = 480;

	geXForm3d_SetIdentity (&MyXForm);
	
	// Create the app's main window		
	GDemohWnd = CreateMainWindow(hInstance, AppName, CWidth, CHeight);
	
	KeepTryingToGetAGoodMode =1;
	do 
	{
	//
	// Start up the engine
	//
	Engine = geEngine_Create(GDemohWnd, AppName, ".");

	if (!Engine)
	{
		MyError("Could not initialize the Genesis engine.\n");
	}

	// 'cause we don't know the initial state...
	geEngine_EnableFrameRateCounter(Engine, DoFrameRateCounter);

	//
	//	Setup the viewing rect
	//
	Rect.Left = 0;
	Rect.Right = CWidth-1;
	Rect.Top = 0;
	Rect.Bottom = CHeight-1;	// Subtract more if you want a HUD
	//
	//	Create a camera
	//
	Camera = geCamera_Create(2.0f, &Rect);

	if (!Camera)
		MyError("Could not create the camera.\n");

	//
	//	Create a World
	//
	{
		// Open the file and create the world
		geVFile *WorldFile;

		WorldFile = geVFile_OpenNewSystem (NULL, GE_VFILE_TYPE_DOS, LevelName, NULL, GE_VFILE_OPEN_READONLY);
		if (WorldFile != NULL)
		{
			World = geWorld_Create (WorldFile);
			geVFile_Close (WorldFile);
		}
	}


	if (World == NULL)
	{
		MyError("Could not create the world.\n");
	}

	if (geEngine_AddWorld (Engine, World) == GE_FALSE)
	{
		MyError ("Could not add world to engine.\n");
	}

	// Set some light type defaults
	geWorld_SetLTypeTable(World, 0, "z");
	geWorld_SetLTypeTable(World, 1, "mmnmmommommnonmmonqnmmo");
	geWorld_SetLTypeTable(World, 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
	geWorld_SetLTypeTable(World, 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
	geWorld_SetLTypeTable(World, 4, "mamamamamama");
	geWorld_SetLTypeTable(World, 5, "jklmnopqrstuvwxyzyxwvutsrqponmlkj");
	geWorld_SetLTypeTable(World, 6, "nmonqnmomnmomomno");
	geWorld_SetLTypeTable(World, 7, "mmmaaaabcdefgmmmmaaaammmaamm");
	geWorld_SetLTypeTable(World, 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");
	geWorld_SetLTypeTable(World, 9, "aaaaaaaazzzzzzzz");
	geWorld_SetLTypeTable(World, 10,"mmamammmmammamamaaamammma");
	geWorld_SetLTypeTable(World, 11,"abcdefghijklmnopqrrqponmlkjihgfedcba");


	DrvList_PickDriver(hInstance, GDemohWnd, Engine, &Driver, &Mode);
	if	(!Driver || !Mode)
	{
		goto EXIT; //MyError("Could not find a driver");
	}

			if	(geEngine_SetDriverAndMode(Engine, Driver, Mode) == GE_FALSE)
			{
				char TempStr2[5000];
				ShutdownAll();
				sprintf(TempStr2,"Unable to use selected driver:\n '%s'",SelectedDriverString);
				MessageBox(0, TempStr2, "Error:", MB_OK);
			}
			else
			{
				 KeepTryingToGetAGoodMode =0;
			}
	}
	while ( KeepTryingToGetAGoodMode );

	//
	//	Clear the camera matrix and position
	//
	geVec3d_Set (&LocalPos, 0.0f, 0.0f, 0.0f);

	SpotTexture = gpreview_LoadBitmapFromResource (IDR_SPOT);
	if (SpotTexture != NULL)
	{
		geWorld_AddBitmap (World, SpotTexture);
	}
	//
	//  Create our headlight
	//
	HeadLight = geWorld_AddLight(World);
	HeadLightColor.r = 255.0f;
	HeadLightColor.g = 255.0f;
	HeadLightColor.b = 255.0f;
	HeadLightIntensity = 300.0f;
	HeadLightOn = GE_FALSE;

	CrossHairCursor = Cursor_Create (Engine, IDR_CURSOR);
	if (CrossHairCursor == NULL)
	{
		MyError ("Could not create cursor");
	}

	while (Running)
	{
		if	(Animator)
		{
			Model_AnimatorFrame(Animator);
		}

		if (!geEngine_BeginFrame(Engine, Camera, GE_FALSE))
		{
			MyError("EngineBeginFrame failed.\n");
		}
	
		SetupXForm (XRot, YRot, &LocalPos, &MyXForm);
		MovePos (&LocalPos, &MyXForm);

		geWorld_SetLightAttributes
			(
				World,
				HeadLight,
				&LocalPos,
				&HeadLightColor,
				HeadLightOn ? HeadLightIntensity : 0.0f,
				GE_FALSE
			);

		SetupCamera (Camera, &Rect, &MyXForm);

 		if (!geEngine_RenderWorld(Engine, World, Camera, 0.0f))
		{
			MyError("Could not render the world.\n");
		}

		if	(Missile)
		{
			// If a missile was created, then update it.
			// If it collides with something, then destroy it.
			if	(UpdateDynamicLight(Missile))
			{
				DestroyDynamicLight(Missile);
				Missile = NULL;
			}
		}

		if (ShowHelp != GE_FALSE)
		{
			Help();
		}

		if (ShowStats != GE_FALSE)
		{
			Stats();
		}

		Info();

		if	(InAimMode())
		{
			Cursor_Render(CrossHairCursor);
		}

		if (!geEngine_EndFrame(Engine))
		{
			MyError("EngineEndFrame failed.\n");
		}
		
		while (PeekMessage( &Msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&Msg, NULL, 0, 0 ))
			{
				PostQuitMessage(0);
				Running=0;
				break;
			}

			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
		
	}
EXIT:
	ShutdownAll();

	return 0;
}
#pragma warning (default:4100)

// MyError -- Fatal error.
// Shutdown engine, display error message box, and exit.
static	void MyError(const char *Msg, ...)
{
	#if 0
	va_list		ArgPtr;
    char		TempStr[500];

	va_start (ArgPtr, Msg);
    vsprintf (TempStr, Msg, ArgPtr);
	va_end (ArgPtr);

	ShutdownAll ();

	MessageBox (0, Msg, "** GPREVIEW ERROR **", MB_OK);

	_exit(1);
	#endif


	va_list		ArgPtr;
    char		TempStr[500];
    char		TempStr2[500];
	FILE		*f;

	va_start (ArgPtr, Msg);
    vsprintf (TempStr, Msg, ArgPtr);
	va_end (ArgPtr);

	ShutdownAll();

	f = fopen("GPreview.Log", "wt");

	if (f)
	{
		int32		i, NumErrors;

		NumErrors = geErrorLog_Count();

		fprintf(f, "Error#:%3i, Code#:%3i, Info: %s\n", NumErrors, 0, TempStr);

		for (i=0; i<NumErrors; i++)
		{
			geErrorLog_ErrorClassType	Error;
			char						*String;

			if (geErrorLog_Report(NumErrors-i-1, &Error, &String))
			{
				fprintf(f, "Error#:%3i, Code#:%3i, Info:%s\n", NumErrors-i-1, Error, String);
			}
		}

		fclose(f);
		
		sprintf(TempStr2, "%s\nPlease refer to GPreview.Log for more info.", TempStr);

		MessageBox(0, TempStr2, "** Genesis3D Virtual System Error **", MB_OK);
		WinExec( "Notepad GPreview.Log", SW_SHOW );
	}
	else
	{
		sprintf(TempStr2, "%s\nCould NOT output GPreview.log!!!", TempStr);

		MessageBox(0, TempStr2, "** Genesis3D Virtual System Error **", MB_OK);
	}

	_exit(1);

}
