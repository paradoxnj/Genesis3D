/****************************************************************************************/
/*  LOG.H                                                                               */
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
/*  Genesis3D Version 1.1 released November 15, 1999                                 */
/*  Copyright (C) 1999 WildTangent, Inc. All Rights Reserved           */
/*                                                                                      */
/****************************************************************************************/
#ifndef __LOG_H__
#define __LOG_H__
 
#ifdef __cplusplus
extern "C" {
#endif
 
typedef enum 
{ 
	LOG_NOWHERE, 
	LOG_TO_FILE, 
	LOG_TO_NOTEPAD, 
	LOG_TO_OPEN_NOTEPAD 
} Log_DestinationType;

typedef enum 
{ 
	LOG_RESET, 
	LOG_APPEND 
} Log_OpenType;


typedef struct _LogType LogType; 

//#ifndef NDEBUG
	#define DEBUG_LOGGING // comment out if dont want logging code in exe
//#endif

#ifdef DEBUG_LOGGING

LogType *Log_Open(Log_DestinationType DestinationType,
				char *Filename,Log_OpenType ResetOrAppend);
	// open a log (filename is nessary)
 
void Log_Close(LogType **log);
	// close a log	(not nessary except to free some memory)
 
void Log_Output(LogType *log, char *format, ... );
	// output to a log. use printf format.
 
void Log_Timestamp(LogType *log);
	// dumps a time/date stamp into log.

#else
	// Disable logging functions
#define Log_Open(dest,filename, mode)  0
#define Log_Close(log)
#define Log_Output	if (0)
#define Log_Timestamp(log)
#endif

#ifdef __cplusplus
}
#endif

 
#endif
 
 
