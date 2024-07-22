/****************************************************************************************/
/*  LOG.C                                                                               */
/*                                                                                      */
/*  Author: Mike Sandige	                                                            */
/*  Description:  Event logging API.	                                                */
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
#define xxLOG_TEST

#include <stdio.h>
#include <windows.h>
#include <dos.h>					   //getdate  gettime
#pragma warning (disable:4201)
#include <mmsystem.h>				   // timeGetTime
#pragma warning (default:4201)
#include <assert.h>				

#include "RAM.H"		// geRam_Allocate
#include "log.h"
 
 
#define LOG_MAX_FILENAME_LENGTH 300

typedef struct _LogType
{
	char filename[LOG_MAX_FILENAME_LENGTH];
	Log_DestinationType dest;
	Log_OpenType        mode;
} LogType;


#ifdef	DEBUG_LOGGING
 
static void Log_FileReset(LogType *log)
{
	FILE *f;
	if (log->mode==LOG_RESET)
		{
			f = fopen(log->filename,"w");
			assert(f);
			fclose(f);
		}
}
 
static void Log_FileWrite(LogType *log,char *buf)
{
	static InUse=FALSE;
	FILE *f;
 
   while( InUse )
	Sleep(0);
 
   InUse = TRUE;
	f = fopen(log->filename,"a");
	assert(f);
	fseek(f,0L,SEEK_END);
	fwrite(buf,lstrlen(buf),1,f);
	fflush(f);
	fclose(f);
   InUse = FALSE;
}
 
static int Log_NotepadSendMessage(
	char *filename, 	// filename of notepad file
	UINT  uMsg, // message to send
	WPARAM	wParam, // first message parameter
	LPARAM	lParam	// second message parameter
)
{
	HWND Notepad;
	char window_title[LOG_MAX_FILENAME_LENGTH + 50];

	assert( strlen(filename) < LOG_MAX_FILENAME_LENGTH );
	sprintf(window_title,"%s - Notepad",filename); 
	
	if( (Notepad = FindWindow( "Notepad",window_title )) != NULL )
		{
			HWND Edit;
			if( (Edit = GetWindow( Notepad, GW_CHILD )) != NULL )
				{
					SendMessage(Edit,uMsg,wParam,lParam);
					return 0;
				}
		}
	return 1;
}
 
 
static void Log_NotepadReset(LogType *log)
{
	if (log->dest==LOG_TO_NOTEPAD)
		{
			if (Log_NotepadSendMessage(log->filename,
				EM_REPLACESEL, 0, (LPARAM)"\n")!=0)
				{			// possibly not open, try to open.
					char cmd[LOG_MAX_FILENAME_LENGTH+50];
					FILE *f;
					f = fopen(log->filename,"a");
					fclose(f);
					assert(strlen(cmd)<LOG_MAX_FILENAME_LENGTH);
					sprintf(cmd,"Notepad %s",log->filename);
					WinExec(cmd,SW_SHOW);
					{	 // kill time for a bit to wait for file to open...
						long now,then;
						now = timeGetTime();
						then = now+500;
						do
							{
								now = timeGetTime();
							}
						while (now<then);
					}
				}
		}
	if (log->mode==LOG_RESET)
		{
			Log_NotepadSendMessage(log->filename,
				WM_SETTEXT, 0, (LPARAM)"");
		}
}
 
static void Log_NotepadWrite(LogType *log,char *buf)
{
	Log_NotepadSendMessage(log->filename,EM_REPLACESEL,0,(LPARAM)buf);
}
 
void Log_Close(LogType **log)
{
	assert(log);
	assert(*log);
	geRam_Free(*log);
	*log=NULL;
}
 
LogType *Log_Open(Log_DestinationType dest,char *filename,Log_OpenType mode)
{
	LogType *log;
	log = geRam_Allocate(sizeof(LogType));
	assert(log);
	assert(filename!=NULL);
	assert(strlen(filename)<LOG_MAX_FILENAME_LENGTH);
	assert(strlen(filename)>0);
	strcpy(log->filename,filename);
	log->dest=dest;
	log->mode=mode;
	switch (dest)
		{
			case (LOG_TO_FILE):
				{
					Log_FileReset(log);
					break;
				}
			case (LOG_TO_NOTEPAD):
			case (LOG_TO_OPEN_NOTEPAD):
				{
					Log_NotepadReset(log);
					break;
				}
		}
	if (log->dest==LOG_TO_OPEN_NOTEPAD)
		log->dest = LOG_TO_NOTEPAD;  //collapse to notepad destination
	return log;
}
 
void Log_Output(LogType *log, char *format, ... )
{
	char buf[ 1024 ];
	assert(log);
	assert(format);
 
	// I use vsprintf() from the C RTL instead of wvsprintf() from the
	// WIN API so that I can use floating point.
 
	vsprintf( buf, format, (char *)&format + sizeof( format ) );
	switch (log->dest)
		{
			case (LOG_TO_FILE):
				{
					lstrcat(buf,"\n");
					Log_FileWrite(log,buf);
					break;
				}
			case (LOG_TO_NOTEPAD):
				{
					lstrcat(buf,"\r\n");
					Log_NotepadWrite(log,buf);
					break;
				}
		}
}
 
void Log_Timestamp(LogType *log)
{
#ifndef BORLANDC	// use a time function that will compile
	SYSTEMTIME	st;

	GetLocalTime( &st );
	Log_Output(log,"Current time is: %2.2d:%2.2d:%2.2d",
					 (int)st.wHour,
					 (int)st.wMinute,
					 (int)st.wSecond );
#else
	struct date thedate;
	struct time thetime;
 
	assert(log);
 
	getdate(&thedate);
	gettime(&thetime);
 
	Log_Output(log,"%d/%d/%d - %2.2d:%2.2d:%2.2d : ",
					 (int)thedate.da_mon,
					 (int)thedate.da_day,
					 (int)thedate.da_year,
					 (int)thetime.ti_hour,
					 (int)thetime.ti_min,
					 (int)thetime.ti_sec);
#endif
}
 
 
#else
									   // not DEBUG_LOGGING
#endif
 
 
 
#ifdef LOG_TEST
#pragma argsused
int pascal WinMain
(
	HINSTANCE instance,
	HINSTANCE prev_instance,
	LPSTR cmd_line,
	int cmd_show
)
{
	LogType *log;
 
 
#if 0
	log = Log_Open(LOG_TO_FILE,"test.log",LOG_RESET);
	Log_Timestamp(log);
	Log_Output(log,"test %d",1);
	Log_Close(&log);
#endif
#if 0
	log = Log_Open(LOG_TO_FILE,"test.log",LOG_APPEND);
	Log_Timestamp(log);
	Log_Output(log,"test %d",3);
	Log_Close(&log);
#endif
 
	log = Log_Open(LOG_TO_OPEN_NOTEPAD,"testn.log",LOG_APPEND);
	Log_Timestamp(log);
	Log_Output(log,"test %d",5);
	Log_Close(&log);
 
#if 0
	log = Log_Open(LOG_TO_NOTEPAD,"testn.log",LOG_RESET);
	Log_Timestamp(log);
	Log_Output(log,"test %d",2);
	Log_Close(&log);
#endif
 
#if 0
	log = Log_Open(LOG_TO_NOTEPAD,"testn.log",LOG_APPEND);
	Log_Timestamp(log);
	Log_Output(log,"test %d",4);
	Log_Close(&log);
#endif
 
	MessageBeep(-1);
	return 0;
}
 
#endif
 
 
 
 
 
