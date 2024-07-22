/****************************************************************************************/
/*  Compiler.cpp                                                                        */
/*                                                                                      */
/*  Author:       Jim Mischel                                                           */
/*  Description:  Dialog and thread code for compiling maps with gbsplib                */
/*                                                                                      */
/*  The contents of this file are subject to the Genesis3D Public License               */
/*  Version 1.01 (the "License"); you may not use this file except in                   */
/*  compliance with the License. You may obtain a copy of the License at                */
/*  http://www.genesis3d.com                                                            */
/*                                                                                      */
/*  Software distributed under the License is distributed on an "AS IS"                 */
/*  basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See                */
/*  the License for the specific language governing rights and limitations              */
/*  under the License.                                                                  */
/*                                                                                      */
/*  The Original Code is Genesis3D, released March 25, 1999.                            */
/*Genesis3D Version 1.1 released November 15, 1999                            */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#include "stdafx.h"
#include "compiler.h"

#pragma warning(disable : 4201 4214 4115 4514)
#include <windows.h>
#include <shellapi.h>
#pragma warning(default : 4201 4214 4115)

#include <assert.h>

#include <io.h>
#include "filepath.h"
#include "consoletab.h"	// for ConPrintf (yes, it's ugly)
#include "util.h"

static CWinThread *CompilerThread = NULL;
static HWND hwndThreadParent = NULL;
static geBoolean ThreadActive = GE_FALSE;
static HINSTANCE GBSPHandle = NULL;
static GBSP_FuncHook *GlobalFHook;


CompilerErrorEnum Compiler_RunPreview
	(
	  char const *PreviewFilename,
	  char const *MotionFilename,
	  char const *GPreviewPath
	)
{
	assert (ThreadActive == GE_FALSE);

	char DestBspName[_MAX_PATH];

	{
		//	Verify the .BSP file exists...
		if (_access (PreviewFilename, 0) != 0)
		{
			return COMPILER_ERROR_NOBSP;
		}
	}


	// we're going to copy the preview file to the "levels" subdirectory
	// of the GPreviewPath directory...
	//
	// So, if the PreviewFilename is "C:\MyStuff\MyLevel.bsp",
	// and GPreviewPath is "C:\Editor\GPreview.exe", then the resulting
	// file name is "C:\Editor\levels\MyLevel.bsp"
	//
	{
		char PreviewDrive[_MAX_PATH];
		char PreviewDir[_MAX_PATH];
		char Name[_MAX_PATH];
		char Ext[_MAX_PATH];

		_splitpath (PreviewFilename, NULL, NULL, Name, Ext);
		_splitpath (GPreviewPath, PreviewDrive, PreviewDir, NULL, NULL);

		strcat (PreviewDir, "levels\\");
		_makepath (DestBspName, PreviewDrive, PreviewDir, Name, Ext);
	}


	// if source and destination are the same, then don't copy...
	if (stricmp (DestBspName, PreviewFilename) != 0)
	{
		if(!CopyFile (PreviewFilename, DestBspName, FALSE))
		{
			int LastError;

			LastError = GetLastError();
			if (LastError != ERROR_FILE_EXISTS)
			{
				ConPrintf("GPreviewPath: %s \n", GPreviewPath);
				ConPrintf("CopyFile (%s, %s)\nGetLastError()==%d\n", PreviewFilename, DestBspName, LastError);
				return COMPILER_ERROR_BSPCOPY;
			}
		}
	}

	{
		// copy the motion file if it exists
		char DestName[_MAX_PATH];

		// check to see if the file exists
		FilePath_SetExt (DestBspName, ".mot", DestName);
		if (_access (MotionFilename, 0) == 0)  // _access returns 0 on success...of course!
		{
			if (stricmp (MotionFilename, DestName) != 0)
			{
				if (!CopyFile (MotionFilename, DestName, FALSE))
				{
					if(GetLastError() != ERROR_FILE_EXISTS)
					{
						ConPrintf ("%s", "Unable to copy the motion file.\n");
						ConPrintf ("%s", "Continuing with preview.\n");
					}
				}
			}
		}
		else
		{
			_unlink (DestName);
		}
	}

	{
		SHELLEXECUTEINFO	shx;
		char Params[_MAX_PATH];
		char DefaultDir[_MAX_PATH];
		char NoExtName[_MAX_PATH];

		FilePath_GetDriveAndDir (GPreviewPath, DefaultDir);
		FilePath_GetName (DestBspName, NoExtName);
		sprintf (Params, "-map %s", NoExtName);

		shx.fMask		= 0;
		shx.hwnd		= NULL; //(HWND)AfxGetMainWnd();
		shx.lpVerb		= NULL;
		shx.lpFile		= GPreviewPath;
		shx.lpParameters= Params;
		shx.lpDirectory	= DefaultDir;
		shx.nShow		= SW_NORMAL;
		shx.hInstApp	= NULL;
		shx.cbSize		= sizeof(shx);

		ShellExecuteEx(&shx);
	}
	return COMPILER_ERROR_NONE;
}

geBoolean Compiler_CancelCompile
	(
	  void
	)
{
	if (!ThreadActive)
	{
		return GE_FALSE;
	}

	return GlobalFHook->GBSP_Cancel ();
}

static CompilerErrorEnum Compiler_LoadCompilerDLL
	(
	  GBSP_FuncHook **ppFHook,
	  HINSTANCE *pHandle,
	  ERROR_CB ErrorCallbackFcn,
	  PRINTF_CB PrintfCallbackFcn
	)
{
	GBSP_INIT *GBSP_Init;
	GBSP_Hook Hook;
	GBSP_FuncHook *FHook;

	assert (ppFHook != NULL);
	assert (pHandle != NULL);

	*ppFHook = NULL;
	Hook.Error	= ErrorCallbackFcn;
	Hook.Printf	= PrintfCallbackFcn;

	GBSPHandle = LoadLibrary("gbsplib.dll");

	if (GBSPHandle == NULL)
	{
		PrintfCallbackFcn ("Compile Failed: Unable to load gbsplib.dll!\nGetLastError returned %d.\n", GetLastError ());
		return COMPILER_ERROR_NODLL;
	}

	GBSP_Init	=(GBSP_INIT*)GetProcAddress(GBSPHandle, "GBSP_Init");

	if (GBSP_Init == NULL)
	{
		PrintfCallbackFcn ("%s", "Compile Failed: Couldn't initialize GBSP_Init, GBSPLib.Dll.\n.");
		FreeLibrary(GBSPHandle);
		return COMPILER_ERROR_MISSINGFUNC;
	}

	FHook = GBSP_Init (&Hook);
	if (FHook == NULL)
	{
		PrintfCallbackFcn ("%s", "Compile Failed: GBSP_Init returned NULL Hook!, GBSPLib.Dll.\n.");
		FreeLibrary(GBSPHandle);
		return COMPILER_ERROR_MISSINGFUNC;
	}

	if(FHook->VersionMajor > GBSP_VERSION_MAJOR)
	{
		PrintfCallbackFcn ("%s", "Compile Failed: GBSP Version Incompatible, GBSPLib.Dll.\n.");
		FreeLibrary (GBSPHandle);
		return COMPILER_ERROR_MISSINGFUNC;
	}

	*ppFHook = FHook;
	*pHandle = GBSPHandle;

	return COMPILER_ERROR_NONE;
}

CompilerErrorEnum Compiler_Compile
	(
	  CompileParamsType const *pParams,
	  ERROR_CB ErrorCallbackFcn,
	  PRINTF_CB PrintfCallbackFcn
	)
{
	HINSTANCE		GBSPHandle;
	CompilerErrorEnum CompileRslt;
	GBSP_RETVAL GbspRslt;
	char BspFilename[_MAX_PATH];
	char MotionFilename[_MAX_PATH];

	// load compiler DLL
	CompileRslt = Compiler_LoadCompilerDLL (&GlobalFHook, &GBSPHandle, ErrorCallbackFcn, PrintfCallbackFcn);

	if (CompileRslt == COMPILER_ERROR_NONE)
	{
		FilePath_SetExt (pParams->Filename, ".bsp", BspFilename);
		FilePath_SetExt (pParams->Filename, ".mot", MotionFilename);
		if (pParams->EntitiesOnly)
		{
			#if 1
				#pragma todo ("Should UpdateEntities return a GBSP_RETVAL?")
				if (GlobalFHook->GBSP_UpdateEntities ((char *)pParams->Filename, BspFilename))
				{
					GbspRslt = GBSP_OK;
				}
				else
				{
					GbspRslt = GBSP_ERROR;
				}
			#else
				GbspRslt = GlobalFHook->GBSP_UpdateEntities ((char *)pParams->Filename, BspFilename);
			#endif
			switch (GbspRslt)
			{
				case GBSP_ERROR:
					PrintfCallbackFcn ("%s", "Compile Failed:  GBSP_UpdateEntities returned an error.  GBSPLib.Dll.\n");
					CompileRslt = COMPILER_ERROR_BSPFAIL;
					break;
				case GBSP_OK :
					break;
				case GBSP_CANCEL :
					PrintfCallbackFcn ("%s", "UpdateEntities cancelled by user.\n");
					CompileRslt = COMPILER_ERROR_USERCANCEL;
					break;
				default :
					assert (0);
			}
		}
		else if (pParams->RunBsp)
		{
			_unlink (MotionFilename);  // gotta make sure this doesn't exist...
			GbspRslt = GlobalFHook->GBSP_CreateBSP ((char *)pParams->Filename, (BspParms *)&pParams->Bsp);
			switch (GbspRslt)
			{
				case GBSP_ERROR :
					PrintfCallbackFcn ("%s", "Compile Failed: GBSP_CreateBSP encountered an error, GBSPLib.Dll.\n.");
					CompileRslt = COMPILER_ERROR_BSPFAIL;
					break;
				case GBSP_OK :
					//save as a bsp
					GbspRslt = GlobalFHook->GBSP_SaveGBSPFile(BspFilename);
					switch (GbspRslt)
					{
						case GBSP_ERROR :
							PrintfCallbackFcn ("Compile Failed: GBSP_SaveGBSPFile for file: %s, GBSPLib.Dll.\n.", BspFilename);
							CompileRslt = COMPILER_ERROR_BSPSAVE;
							break;
						case GBSP_CANCEL :
							PrintfCallbackFcn ("%s", "Save cancelled by user\n");
							CompileRslt = COMPILER_ERROR_USERCANCEL;
							break;
						case GBSP_OK :
							break;
						default :
							assert (0);
					}
					break;
				case GBSP_CANCEL :
					PrintfCallbackFcn ("%s", "Compile cancelled by user\n");
					CompileRslt = COMPILER_ERROR_USERCANCEL;
					break;
				default :
					assert (0);  //??
			}
			GlobalFHook->GBSP_FreeBSP();
		}
	}

	if (CompileRslt == COMPILER_ERROR_NONE)
	{
		if(pParams->DoVis && !pParams->EntitiesOnly)
		{
			GbspRslt = GlobalFHook->GBSP_VisGBSPFile ((char *)BspFilename, (VisParms *)&pParams->Vis);
			switch (GbspRslt)
			{
				case GBSP_ERROR :
					PrintfCallbackFcn ("Warning: GBSP_VisGBSPFile failed for file: %s, GBSPLib.Dll.\n.", BspFilename);
					break;
				case GBSP_CANCEL :
					PrintfCallbackFcn ("%s", "GBSP_VisGBPSFile cancelled by user\n");
					CompileRslt = COMPILER_ERROR_USERCANCEL;
					break;
				case GBSP_OK :
					break;
				default :
					assert (0);
			}
		}
	}

	if (CompileRslt == COMPILER_ERROR_NONE)
	{
		if (pParams->DoLight)
		{
			/*
			  The compiler has no flag that tells it not to do min lighting.
			  So, if the min lighting flag is not on, we set the MinLight
			  vector to all 0s.
			*/
			LightParms rad;

			rad = pParams->Light;
			if (!pParams->UseMinLight)
			{
				geVec3d_Set (&rad.MinLight, 0.0f, 0.0f, 0.0f);
			}
			// If UseMinLight isn't on, then don't pass 
			GbspRslt = GlobalFHook->GBSP_LightGBSPFile(BspFilename, &rad);
			switch (GbspRslt)
			{
				case GBSP_ERROR :
					PrintfCallbackFcn ("Warning: GBSP_LightGBSPFile failed for file: %s, GBSPLib.Dll.\n", BspFilename);
					break;
				case GBSP_CANCEL :
					PrintfCallbackFcn ("%s", "GBSP_LightGBSPFile cancelled by user\n");
					CompileRslt = COMPILER_ERROR_USERCANCEL;
					break;
				case GBSP_OK :
					break;
				default :
					assert (0);
			}
		}
	}

	// and free the DLL...
	if (GBSPHandle != NULL)
	{
		FreeLibrary (GBSPHandle);
	}

	return CompileRslt;
}

static void Compiler_NotifyApp
	(
	  char const *buf,
	  int MsgId
	)
{
#if 1
	char *errmsg;

	errmsg = Util_Strdup (buf);
	if (errmsg != NULL)
	{
		::PostMessage (hwndThreadParent, MsgId, COMPILER_PROCESSID, (LPARAM)errmsg);
	}
#else
	ConPrintf ("%s", buf);
#endif
}

static void Compiler_PrintfCallback
	(
	  char *format,
	  ...
	)
{
	va_list argptr;
	char	buf[32768];		//this is ... cautious

	va_start (argptr, format);
	vsprintf (buf, format, argptr);
	va_end (argptr);

	Compiler_NotifyApp (buf, WM_USER_COMPILE_MSG);
}

static void Compiler_ErrorCallback
	(
	  char *format,
	  ...
	)
{
	va_list argptr;
	char	buf[32768];		//this is ... cautious

	va_start (argptr, format);
	vsprintf (buf, format, argptr);
	va_end (argptr);

	Compiler_NotifyApp (buf, WM_USER_COMPILE_ERR);
}


static UINT Compiler_ThreadProc 
	(
	  void *pParam
	)
{
	CompileParamsType CompileParams;
	CompilerErrorEnum CompileRslt;

	assert (pParam != NULL);
	assert (ThreadActive == GE_FALSE);

	ThreadActive = GE_TRUE;
	CompileParams = *((CompileParamsType *)pParam);

	CompileRslt = Compiler_Compile (&CompileParams, Compiler_ErrorCallback, Compiler_PrintfCallback);

	if (CompileRslt == COMPILER_ERROR_NONE)
	{
		Compiler_PrintfCallback ("%s", "------------------------------\n");
		Compiler_PrintfCallback ("%s", "Compile successfully completed\n");
	}

	// notify parent that the compile has completed
	::PostMessage (hwndThreadParent, WM_USER_COMPILE_DONE, COMPILER_PROCESSID, (LPARAM)CompileRslt);

	ThreadActive = GE_FALSE;
	return 0;
}


CompilerErrorEnum Compiler_StartThreadedCompile
	(
	  CompileParamsType const *pParams,
	  HWND hwndNotify
	)
{
	CWinThread *Thread;

	assert (ThreadActive == GE_FALSE);

	hwndThreadParent = hwndNotify;

	Thread = AfxBeginThread
		( 
		  Compiler_ThreadProc,		// AFX_THREADPROC pfnThreadProc, 
		  (LPVOID)pParams,			// LPVOID pParam, 
		  THREAD_PRIORITY_NORMAL,	// int nPriority = THREAD_PRIORITY_NORMAL, 
		  0,						// UINT nStackSize = 0, 
		  0,						// DWORD dwCreateFlags = 0, 
		  NULL						// LPSECURITY_ATTRIBUTES lpSecurityAttrs = NULL 
		);

	if (Thread != NULL)
	{
		CompilerThread = Thread;
		return COMPILER_ERROR_NONE;
	}
	else
	{
		return COMPILER_ERROR_THREAD;	// couldn't spawn the thread
	}
}

geBoolean Compiler_CompileInProgress
	(
	  void
	)
{
	return ThreadActive;
}
