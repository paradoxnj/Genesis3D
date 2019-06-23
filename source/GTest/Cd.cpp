#define	WIN32_LEAN_AND_MEAN
#include	<windows.h>
#include	<mmsystem.h>
#pragma hdrstop

#include	<stdio.h>
#include	<stdlib.h>

#include "cd.h"

UINT	OpenCDPlayer(void)
{
	MCI_SET_PARMS	mciSetParms;
	MCI_OPEN_PARMS	mciOpenParms;
	DWORD			dwReturn;
	
	// Open the CD audio device by specifying the device name.
	mciOpenParms.lpstrDeviceType = "cdaudio";
	dwReturn = mciSendCommand(0, MCI_OPEN, MCI_OPEN_TYPE, (DWORD)&mciOpenParms);
	if	(dwReturn)
	{
		// Failed to open device. Don't close it; just return error.
		return 0;
	}

	// Set the time format to track/minute/second/frame (TMSF).
	mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	dwReturn = mciSendCommand(mciOpenParms.wDeviceID, MCI_SET, MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms);
	if	(dwReturn)
	{
		mciSendCommand(mciOpenParms.wDeviceID, MCI_CLOSE, 0, 0);
		return 0;
	}

	return mciOpenParms.wDeviceID;
}

BOOL GetCDPosition(UINT Id, INT *Track, INT *Minute, INT *Second)
{
	MCI_STATUS_PARMS	Parms;
	DWORD				dwReturn;

	Parms.dwItem = MCI_STATUS_POSITION;
	Parms.dwCallback = 0;
	dwReturn = mciSendCommand(Id, MCI_STATUS, MCI_STATUS_ITEM, (DWORD)&Parms);

	if (dwReturn)
		return FALSE;

	if (Track)
		*Track = (INT)MCI_TMSF_TRACK(Parms.dwReturn);

	if (Minute)
		*Minute = (INT)MCI_TMSF_MINUTE(Parms.dwReturn);

	if (Second)
		*Second = (INT)MCI_TMSF_SECOND(Parms.dwReturn);

	return(TRUE);
}

void	StopCDPlayer(UINT id)
{
	mciSendCommand(id, MCI_STOP, 0, 0);
}

void	CloseCDPlayer(UINT id)
{
	mciSendCommand(id, MCI_CLOSE, 0, 0);
}

DWORD	PlayCDTrack(UINT wDeviceID,
					HWND hWndNotify,
					BYTE bTrack,
					int  startMin,
					int  startSec,
					int	 stopMin,
					int	 stopSec)
{
	DWORD 			dwReturn;
	MCI_PLAY_PARMS	mciPlayParms;

	// Begin playback from the given track and play until the beginning 
	// of the next track. The window procedure function for the parent 
	// window will be notified with an MM_MCINOTIFY message when 
	// playback is complete. Unless the play command fails, the window 
	// procedure closes the device.
	mciPlayParms.dwFrom = 0L;
	mciPlayParms.dwTo   = 0L;
	mciPlayParms.dwFrom = MCI_MAKE_TMSF(bTrack, startMin, startSec, 0);
	mciPlayParms.dwTo   = MCI_MAKE_TMSF(bTrack, stopMin, stopSec, 0);
	mciPlayParms.dwCallback = (DWORD) hWndNotify;
	dwReturn = mciSendCommand(wDeviceID, MCI_PLAY, MCI_FROM | MCI_TO | MCI_NOTIFY, (DWORD)(LPVOID) &mciPlayParms);
	if (dwReturn)
	{
		mciSendCommand(wDeviceID, MCI_CLOSE, 0, 0);
		return 0;
	}

	return 0;
}

#ifdef SINGLE_BUILD_FOR_TEST
void	main(void)
#else
void	TestCd(void)
#endif
{
	UINT	id;

	id = OpenCDPlayer();
	PlayCDTrack(id, 0, 1, 1, 0, 5, 0);
	Sleep(2000);
	PlayCDTrack(id, 0, 1, 1, 0, 5, 30);
	Sleep(2000);
	PlayCDTrack(id, 0, 1, 1, 0, 5, 30);
	Sleep(2000);
	StopCDPlayer(id);
	CloseCDPlayer(id);
}
