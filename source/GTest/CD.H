#ifndef CD_H
#define CD_H

#include <Windows.H>

#ifdef __cplusplus
extern "C" {
#endif

UINT	OpenCDPlayer(void);
void	StopCDPlayer(UINT id);
void	CloseCDPlayer(UINT id);
BOOL	GetCDPosition(UINT Id, INT *Track, INT *Minute, INT *Second);

DWORD	PlayCDTrack(UINT wDeviceID,
					HWND hWndNotify,
					BYTE bTrack,
					int  startMin,
					int  startSec,
					int	 stopMin,
					int	 stopSec);

#ifndef SINGLE_BUILD_FOR_TEST
void TestCd(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
