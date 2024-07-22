/****************************************************************************/
/*    FILE: ProcEng.c														*/
/*                                                                          */
/*    Copyright (c) 1999, Wild Tangent, Inc.; All rights reserved.       */
/*                                                                          */
/****************************************************************************/
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>	// for dll stuff

#include "GENESIS.H"
#include "RAM.H"
#include "Errorlog.h"
#include "Stdio.h"
#include "String.h"

#include "ProcEng.h"
#include "Procedural.h"

// at the end of this file :
typedef Procedural_Table * (*GetProceduralFunc)(void);

extern GetProceduralFunc GetProceduralFunctions[];
extern int NumGetProceduralFunctions;

#define	PROCENG_MAX_PTABLES		32			// Pre-Loaded tables that procs attach to
#define	PROCENG_MAX_PROCS		256			// Actual procs (created from table data)

//====================================================================================
//====================================================================================
typedef struct ProcEng_PTable
{
	char				DllName[_MAX_PATH];
	HINSTANCE			DllHandle;
	Procedural_Table	*Table;
	int					Uses;
} ProcEng_PTable;

typedef struct ProcEng_Proc
{
	Procedural_Table	*Table;				// Table used to create this proc (So we can destroy it)
	Procedural			*Proc;
	geBitmap			*Bitmap;
} ProcEng_Proc;

typedef struct ProcEng
{
	geWorld				*World;

	int32				NumPTables;
	ProcEng_PTable		PTables[PROCENG_MAX_PTABLES];

	int32				NumProcs;
	ProcEng_Proc		Procs[PROCENG_MAX_PROCS];
} ProcEng;

geBoolean VFile_ReadBetween(geVFile * File,char *Into,char Start,char Stop);
geBoolean VFile_SeekPastString(geVFile * File,const char *str);
geBoolean VFile_SeekPastSpaces(geVFile * File);
int VFile_GetC(geVFile * File);
void VFile_UnGetC(geVFile * File);
static char * stristr(const char *StrBase,const char *SubBase);

#define SkipWhite(p)		while (   isspace(*p) ) p++
#define SkipNonWhite(p)		while ( ! isspace(*p) && *p ) p++

//====================================================================================
//	ProcEng_Create
//====================================================================================
ProcEng *ProcEng_Create(geVFile *CfgFile, geWorld *World)
{
	ProcEng		*PEng;
	int32		i;

	// For now, hard code in the procedures (but later, load *.dll)...
	PEng = static_cast<ProcEng*>(geRam_Allocate(sizeof(*PEng)));

	if (!PEng)
		return GE_FALSE;

	memset(PEng, 0, sizeof(*PEng));

	// init all compiled-in procedurals:

	for(i=0;i<NumGetProceduralFunctions;i++)
	{	
		if ( GetProceduralFunctions[i] )
		{
		Procedural_Table * pTable;
			pTable = (*GetProceduralFunctions[i]) ();
			if ( pTable->Tag == Procedurals_Tag && 
					pTable->Version == Procedurals_Version )
			{
				PEng->PTables[PEng->NumPTables].DllHandle = NULL;
				PEng->PTables[PEng->NumPTables].Table = pTable;
				PEng->PTables[PEng->NumPTables].Uses  = 0;
				PEng->NumPTables ++;
			}
			else if ( pTable->Tag == Procedurals_Tag )
			{
			//char ErrStr[1024];
			//	sprintf(ErrStr,"ProcEng_Create : found procedural : %s : but ignored because of version mismatch",pTable->Name);
			//	geErrorLog_AddString(-1,ErrStr);
			}
		}
	}

	do
	{
	geVFile * FileBase;
	geVFile_Finder * Finder;
	char BasePath[1024];

		//harcoded path!
		//<>
		strcpy(BasePath,"c:\\genesis\\procedurals");

		FileBase = geVFile_OpenNewSystem(NULL,GE_VFILE_TYPE_DOS,BasePath,NULL,GE_VFILE_OPEN_READONLY|GE_VFILE_OPEN_DIRECTORY);
		if ( ! FileBase )
		{
			// We really should not use ErrorLog for normal logging info...
			//geErrorLog_AddString(-1,"ProcEng_Create : Couldn't open procedural DLL directory : non_fatal");
			break;
		}

		Finder = geVFile_CreateFinder(FileBase,"*.dll");
		if ( ! Finder )
		{
			//geErrorLog_AddString(-1,"ProcEng_Create : Couldn't open procedural Finder : non_fatal");
			geVFile_Close(FileBase);
			break;
		}

		while( geVFile_FinderGetNextFile(Finder) )
		{
		GetProceduralFunc GetProcFunc;
		HMODULE TheDll;
		geVFile_Properties Properties;
		char DLLName[_MAX_PATH];
		Procedural_Table * pTable;
		
			geVFile_FinderGetProperties(Finder,&Properties);

			strcpy(DLLName,BasePath);
			strcat(DLLName,"\\");
			strcat(DLLName,Properties.Name);

			TheDll = LoadLibrary(DLLName);
			if ( ! TheDll )
			{
				#if 0
			//char ErrStr[_MAX_PATH+1024];

				//sprintf(ErrStr,"ProcEng_Create : LoadLibrary failed on DLL : %s : non-fatal",DLLName);
				//geErrorLog_AddString(-1,ErrStr);

				{
				char string[1024];
				int plen;
				DWORD err;
	
					err = GetLastError();
					sprintf(string,"Windows Error : %d = %08X : \0",err,err);
					plen = strlen(string);
					FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM , NULL, err, 0, string+plen, sizeof(string)-plen, NULL);
					geErrorLog_AddString(-1,string);
				}
				#endif

				continue;
			}

			GetProcFunc = (GetProceduralFunc) GetProcAddress(TheDll,"GetProceduralTable");
			
			if ( ! GetProcFunc )
			{
			//char ErrStr[_MAX_PATH+1024];
			//	sprintf(ErrStr,"ProcEng_Create : no GetProceduralTable in DLL : %s : non-fatal",DLLName);
			//	geErrorLog_AddString(-1,ErrStr);
				FreeLibrary(TheDll);
				continue;
			}

			pTable = GetProcFunc();
			if ( !pTable || pTable->Tag != Procedurals_Tag || pTable->Version != Procedurals_Version )
			{
			//char ErrStr[1024];
			//	sprintf(ErrStr,"ProcEng_Create : found procedural : %s : but ignored because of version mismatch",pTable == NULL ? "null!" : pTable->Name);
			//	geErrorLog_AddString(-1,ErrStr);
				FreeLibrary(TheDll);
				continue;
			}

			strcpy(PEng->PTables[PEng->NumPTables].DllName,DLLName);
			PEng->PTables[PEng->NumPTables].DllHandle = TheDll;
			PEng->PTables[PEng->NumPTables].Table = pTable;
			PEng->PTables[PEng->NumPTables].Uses  = 0;
			PEng->NumPTables ++;
		}

		geVFile_DestroyFinder(Finder);
		geVFile_Close(FileBase);
	}
	while(0);

	if ( CfgFile && VFile_SeekPastString(CfgFile,"procedurals") )
	{

		if ( ! VFile_SeekPastString(CfgFile,"{") )
			goto ExitWithError;
				
		for(;;)
		{
			char		*TargetName,*ProcName;
			geBitmap	*Bitmap;
			char		InputStr[1024];
			char		*p;
			int			ReadLen;

			if ( ! VFile_SeekPastSpaces(CfgFile) )
				goto ExitWithError;

			if ( ! geVFile_GetS(CfgFile,InputStr,sizeof(InputStr)) )
				goto ExitWithError;

			ReadLen = strlen(InputStr) + 1;

			if ( p = strchr(InputStr,'}') )
			{
				p ++;
				// un-read data we haven't looked at
				geVFile_Seek(CfgFile, (int)p - ((int)InputStr + ReadLen), GE_VFILE_SEEKCUR);
				break;
			}

			p = InputStr;
			SkipWhite(p);
			if ( _strnicmp(p,"wbm:",4) == 0 )
			{
			char ParamStr[4096];
			int SeekLen;


				p += 4;
				SkipWhite(p);
				TargetName = p;
				SkipNonWhite(p);
				if ( *p ) *p++ = 0;
				SkipWhite(p);
				ProcName = p;
				SkipNonWhite(p);
				
				if ( strchr(ProcName,'(') < p && strchr(ProcName,'(') )
				{
					p = strchr(ProcName,'(');
					SeekLen = (int)p - ((int)InputStr + ReadLen);
					// SeekLen --; // ?
					*p = 0;
				}
				else
				{
					SeekLen = (int)p - ((int)InputStr + ReadLen);
					*p = 0;
				}
		
				// un-read data we haven't looked at
				if ( ! geVFile_Seek(CfgFile, SeekLen, GE_VFILE_SEEKCUR) )
					goto ExitWithError;

				VFile_ReadBetween(CfgFile,ParamStr,'(',')');

				// find name
				Bitmap = geWorld_GetBitmapByName(World, TargetName);

				if (!Bitmap)
				{
				//char ErrStr[1024];
				//	sprintf(ErrStr, "ProcEng_Create : couldn't find world bitmap : %s\n",TargetName);
				//	geErrorLog_AddString(-1,ErrStr);
				}

				// Should we really stop when a proc cannot be found for a bitmap?
				if ( ! ProcEng_AddProcedural(PEng, ProcName, &Bitmap, ParamStr) )
				{
				//char ErrStr[1024];
				//	sprintf(ErrStr, "ProcEng_Create : couldn't find procedural : %s\n",ProcName);
				//	geErrorLog_AddString(-1,ErrStr);
				}
			}
		}
	}

	PEng->World = World;
	//geWorld_CreateRef(World);

	return PEng;												 

	ExitWithError:

		geErrorLog_AddString(-1,"ProcEng_Create : failure!", NULL);

		if (PEng)
		{
			ProcEng_Destroy(&PEng);
		}
			
		return NULL;
}

//====================================================================================
//	ProcEng_Destroy
//====================================================================================
void ProcEng_Destroy(ProcEng **pPEng)
{
	int32			i;
	ProcEng_Proc	*pProc;
	ProcEng_PTable	*pTable;
	ProcEng			*PEng;

	assert(pPEng);

	PEng = *pPEng;
	if ( ! PEng )
		return;
	
	// Free all the allocated procs
	pProc = PEng->Procs;
	for (i=0; i< PEng->NumProcs; i++, pProc++)
	{
		assert(pProc->Table);
		assert(pProc->Proc);

		pProc->Table->Destroy(pProc->Proc);

		geBitmap_Destroy(&(pProc->Bitmap)); // we did creatref

		memset(pProc, 0, sizeof(*pProc));	
	}
	PEng->NumProcs = 0;

	// Free all the table dlls...
	pTable = PEng->PTables;
	for (i=0; i< PEng->NumPTables; i++, pTable++)
	{
		if ( pTable->DllHandle )
		{
			FreeLibrary(pTable->DllHandle);
		}
		memset(pTable, 0, sizeof(*pTable));
	}
	PEng->NumPTables = 0;

	//if (PEng)
	//	geWorld_Free(PEng->World);
	PEng->World = NULL;

	geRam_Free(PEng);

	*pPEng = NULL;
}

//====================================================================================
//	ProcEng_Destroy
//====================================================================================
geBoolean ProcEng_Minimize(ProcEng *PEng)
{
ProcEng_PTable	*pTable;
ProcEng_PTable	*pTableLast;

	assert(PEng);

	// Free all the table dlls...
	pTable = PEng->PTables;
	pTableLast = pTable + (PEng->NumPTables - 1);
	while( pTable <= pTableLast )
	{
		if ( pTable->Uses < 1 )
		{
			if ( pTable->DllHandle )
			{
				FreeLibrary(pTable->DllHandle);
			}
			*pTable = *pTableLast;
			pTableLast--;
			PEng->NumPTables --;
		}
		else
		{
			pTable ++;
		}
	}
	return GE_TRUE;
}

//====================================================================================
//	ProcEng_AddProcedural
//	NOTE - The only way to remove procs, is to destroy the ProcEng object, and start over for now
//====================================================================================
geBoolean ProcEng_AddProcedural(ProcEng *PEng, const char *ProcName, geBitmap **pBitmap, const char * Params)
{
	int32			i;
	ProcEng_PTable	*pTable;

	assert(PEng);
	assert(ProcName);
	assert(pBitmap);

	if (PEng->NumProcs+1 >= PROCENG_MAX_PROCS)
	{
		geErrorLog_AddString(-1, "ProcEng_AddProcedural:  Max procs.", NULL);
		return GE_FALSE;
	}

	// Find Procedural_Table by Name
	pTable = PEng->PTables;
	for (i=0; i< PEng->NumPTables; i++, pTable++)
	{
		// Should we ignore case? ! YES!
		if ( _stricmp(pTable->Table->Name, ProcName) == 0)
			break;
	}

	if (i == PEng->NumPTables)		// Didn't find it
	{
		geErrorLog_AddString(-1, "ProcEng_AddProcedural:  Table not found for procedural.", NULL);
		return GE_FALSE;
	}

	assert(pTable->Table);

	// Create the proc (each bitmap should have it's own)
	PEng->Procs[PEng->NumProcs].Proc = pTable->Table->Create(pBitmap, Params);

	if (!PEng->Procs[PEng->NumProcs].Proc)
	{
		geErrorLog_AddString(-1," ProcEng_AddProcedural:  (%s)->Create failed", pTable->Table->Name);
		return GE_FALSE;
	}
	if (! *pBitmap)
	{
		geErrorLog_AddString(-1,"ProcEng_AddProcedural: No Bitmap:", pTable->Table->Name);
		return GE_FALSE;
	}

	PEng->Procs[PEng->NumProcs].Bitmap = *pBitmap;
	PEng->Procs[PEng->NumProcs].Table = pTable->Table;

	geBitmap_CreateRef(*pBitmap);
	// make sure the bitmap isn't destroyed before our procedural

	PEng->NumProcs++;

	pTable->Uses ++;

	return GE_TRUE;
}

//====================================================================================
//	ProcEng_Animate
//====================================================================================
geBoolean ProcEng_Animate(ProcEng *PEng, float ElapsedTime)
{
	int32			i;
	ProcEng_Proc	*pProc;

	pProc = PEng->Procs;
	for (i=0; i< PEng->NumProcs; i++, pProc++)
	{
		assert(pProc->Table);
		assert(pProc->Proc);

		if (!geWorld_BitmapIsVisible(PEng->World, pProc->Bitmap))
			continue;

		if (!pProc->Table->Animate(pProc->Proc, ElapsedTime))
		{
			geErrorLog_AddString(-1,"ProcEng_Animate: pProc->Table->Animate failed", NULL);
			return GE_FALSE;
		}
	}
	return GE_TRUE;
}


//====================================================================================
// VFile Seek utils
//====================================================================================

geBoolean VFile_SeekPastString(geVFile * File,const char *str)
{
long LastPosition;
char Buf[1024];
int len;

	if ( strlen(str) == 0 )
		return GE_FALSE;

	if ( ! geVFile_Tell(File,&LastPosition) )
		return GE_FALSE;

	for(;;)
	{
		if ( ! VFile_SeekPastSpaces(File) )
			goto fail;

		len = strlen(str);
		if ( ! geVFile_Read(File,Buf,len) )
			goto fail;
		Buf[len] = 0;

		if ( _stricmp(Buf,str) == 0 )
			return GE_TRUE;
		
		len = 1 - len;
		if ( ! geVFile_Seek(File,len,GE_VFILE_SEEKCUR) )
			goto fail;
	}
	
	fail:

	geVFile_Seek(File,LastPosition,GE_VFILE_SEEKSET);
	return GE_FALSE;
}

int VFile_GetC(geVFile * File)
{
uint8 Char;
	if ( ! geVFile_Read(File,&Char,1) )
		return -1;
return (int)Char;
}

void VFile_UnGetC(geVFile * File)
{
	geVFile_Seek(File,-1,GE_VFILE_SEEKCUR);
}

geBoolean VFile_SeekPastSpaces(geVFile * File)
{
long LastPosition;
int Char;

	if ( ! geVFile_Tell(File,&LastPosition) )
		return GE_FALSE;

	while( (Char = VFile_GetC(File)) != -1 )
	{
		if ( ! isspace(Char) )
		{
			VFile_UnGetC(File);
			return GE_TRUE;
		}
	}
	geVFile_Seek(File,LastPosition,GE_VFILE_SEEKSET);
	return GE_FALSE;
}

static char * stristr(const char *StrBase,const char *SubBase)
{

while ( *StrBase )
  {
  if ( toupper(*StrBase) == toupper(*SubBase) )
	 {
	 const char * Str,* Sub;
	 Str = StrBase + 1;
	 Sub = SubBase + 1;
	 while ( *Sub && toupper(*Sub) == toupper(*Str) )
		{
		Sub++; Str++;
		}
	 if ( ! *Sub) return((char *)StrBase);
	 }
  StrBase++;
  }

return(NULL);
}

geBoolean VFile_ReadBetween(geVFile * File,char *Into,char Start,char Stop)
{
int c;
int ins;

	Into[0] = 0;

	if ( ! VFile_SeekPastSpaces(File) )
		return GE_FALSE;

	if ( (c = VFile_GetC(File)) == Start )
	{
	char *p;
		ins = 1;
		p = Into;
		for(;;)
		{
			c = VFile_GetC(File);
			if ( c == -1 )
			{
				*p = 0;
				return GE_FALSE;
			}
			*p = c;
			if ( c == Start ) ins ++;
			else if ( c == Stop ) ins --;
			if ( ins == 0 ) break;
			p++;
		}
		*p = 0;
	}
	else
	{
		if ( c == -1 )
			return GE_FALSE;
		else
			VFile_UnGetC(File);
	}

return GE_TRUE;
}
