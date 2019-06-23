/****************************************************************************************/
/*  Sound.c                                                                             */
/*                                                                                      */
/*  Author: Brian Adelberg                                                              */
/*  Description: DirectSound wrapper                                                    */
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
#include	<windows.h>
#include	<dsound.h>
#include	<stdio.h>
#include	<assert.h>

#include	"BaseType.h"
#include	"ErrorLog.h"
#include	"VFile.h"
#include	"Sound.h"
#include	"Ram.h"

typedef struct	SoundManager	SoundManager;
typedef struct  Channel			Channel;


typedef struct geSound_System
{
	geBoolean		Active;
	SoundManager	*SoundM;
	geFloat			GlobalVolume;
} geSound_System;

typedef struct geSound_Cfg
{
	geFloat			Volume;
	geFloat			Pan;
	geFloat			Frequency;
} geSound_Cfg;


/*
	The interfaces here allow an application to write sound data to
	abstract channels which are then to be mixed.  The interfaces here
	require two things.  First, that the application create only one
	sound manager per instance, and second that the type of sound data
	being passed into the sound channels remains constant.  That is,
	the format of the binary information is all one format from
	one sound to another; the application cannot combine RIFF and WAV
	formats in a single channel.
*/
/*
	Call these ones only once per application:
*/

static SoundManager *	CreateSoundManager(HWND hWnd);
static void		DestroySoundManager(SoundManager *sm);

//static BOOL		FillSoundChannel(SoundManager *sm, char* Dir, char *Name, unsigned int* Handle );
static BOOL		FillSoundChannel(SoundManager *sm, geVFile *File, unsigned int* Handle );
//static BOOL		FillSoundChannelMemory(SoundManager *sm, const void *Buffer, unsigned int* Handle );
static BOOL		StartSoundChannel( SoundManager *sm, unsigned int Handle, geSound_Cfg *cfg, int loop, unsigned int* sfx);
static BOOL		StopSoundChannel(Channel *channel);
static BOOL		FreeAllChannels(SoundManager *sm);
static BOOL		FreeChannel(SoundManager *sm, Channel *channel);
static BOOL		ModifyChannel( Channel *channel, geSound_Cfg *cfg );
static int		ChannelPlaying( Channel *channel );
static Channel*	GetChannel( SoundManager *sm, unsigned int ID );


typedef struct Channel
{
//	char*				name;
	LPDIRECTSOUNDBUFFER8	buffer;
	unsigned int		ID;
	int					BaseFreq;
	geSound_Cfg			cfg;
	void *				Data;
	struct Channel		*next;
	struct Channel		*nextDup;
} Channel;

typedef struct	SoundManager
{
	int						smChannelCount;
	unsigned int			smNextChannelID;

	LPDIRECTSOUNDBUFFER 	smPrimaryChannel;
	Channel*				smChannels;
    //LPDIRECTSOUNDNOTIFY *   smNotify;
}   SoundManager;

static	LPDIRECTSOUND8			lpDirectSound;
// This isn't really safe as a global.  But it's consistent with the global lpDirectSound.
//static  HMODULE					hmodDirectSound = NULL;

//=====================================================================================
//	geSound_SystemCreate
//=====================================================================================
GENESISAPI	geSound_System *geSound_CreateSoundSystem(HWND hWnd)
{
	geSound_System		*SoundSystem;

	SoundSystem = GE_RAM_ALLOCATE_STRUCT(geSound_System);

	if (!SoundSystem)
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return NULL;
	}

	memset(SoundSystem, 0, sizeof(geSound_System));
	
	// Initialize the sound system
	SoundSystem->SoundM = CreateSoundManager(hWnd);

	if (!SoundSystem->SoundM)
	{
		geRam_Free(SoundSystem);
		geErrorLog_Add(GE_ERR_CREATE_SOUND_MANAGER_FAILED, NULL);
		return NULL;
	}
	SoundSystem->GlobalVolume = 1.0f;

	return SoundSystem;
}

//=====================================================================================
//	geSound_SystemFree
//=====================================================================================
GENESISAPI	void geSound_DestroySoundSystem(geSound_System *Sound)
{
	assert(Sound != NULL);

	// Shutdown the sound system
	DestroySoundManager(Sound->SoundM);

	Sound->SoundM = NULL;

	geRam_Free(Sound);
}

//=====================================================================================
//	Sound_LoadSound
//=====================================================================================
//GENESISAPI	geSound_Def *geSound_LoadSoundDef(geSound_System *SoundS, const char *Path, const char *FileName)
GENESISAPI	geSound_Def *geSound_LoadSoundDef(geSound_System *SoundS, geVFile *File)
{
	unsigned int SoundDef = 0;

	assert(SoundS != NULL);

//	if (!FillSoundChannel(SoundS->SoundM, (char*)Path, (char*)FileName, &SoundDef))
	if (!FillSoundChannel(SoundS->SoundM, File, &SoundDef))
		return 0;
	
	return (geSound_Def *)SoundDef;
}

#if 0
//=====================================================================================
//	Sound_LoadSound
//=====================================================================================
GENESISAPI	geSound_Def *geSound_LoadSoundDefFromMemory(
	geSound_System *SoundS,
	const void *Buffer)
{
	unsigned int SoundDef = 0;

	assert(SoundS != NULL);
	assert(Buffer != NULL);

	if (!FillSoundChannelMemory(SoundS->SoundM, Buffer, &SoundDef))
		return 0;
	
	return (geSound_Def *)SoundDef;
}
#endif

//=====================================================================================
//	Sound_FreeSound
//=====================================================================================
GENESISAPI	void geSound_FreeSoundDef(geSound_System *SoundS, geSound_Def *SoundDef)
{
	Channel*	Channel;

	assert(SoundS != NULL);
	assert(SoundDef != 0);

	Channel = GetChannel(SoundS->SoundM, (unsigned int)SoundDef);

	if (!Channel)
		return;

	FreeChannel(SoundS->SoundM, Channel);
}

//=====================================================================================
//	Sound_SetGlobalVolume
//=====================================================================================
GENESISAPI	geBoolean geSound_SetMasterVolume( geSound_System *SoundS, geFloat Volume )
{
	if( !SoundS )
		return( GE_FALSE );
	SoundS->GlobalVolume = Volume;
	return( GE_TRUE );
}
	
//=====================================================================================
//	Sound_PlaySound
//=====================================================================================
GENESISAPI	geSound *geSound_PlaySoundDef(geSound_System *SoundS, 
							geSound_Def *SoundDef, 
							geFloat Volume, 
							geFloat Pan, 
							geFloat Frequency, 
							geBoolean Loop)
{
	unsigned int Sound;
	geSound_Cfg LocalCfg;

	LocalCfg.Volume		= Volume;
	LocalCfg.Pan		= Pan;
	LocalCfg.Frequency  = Frequency;

	LocalCfg.Volume *= SoundS->GlobalVolume;
	if (!StartSoundChannel(SoundS->SoundM, (unsigned int)SoundDef, &LocalCfg, (BOOL)Loop, &Sound))
	{
		return 0;
	}

	return (geSound *)Sound;
}
	
//=====================================================================================
//	Sound_StopSound
//=====================================================================================
GENESISAPI	geBoolean geSound_StopSound(geSound_System *SoundS, geSound *Sound)
{
	Channel*	Channel;

	assert(SoundS != NULL);
	assert(Sound  != NULL);	

	Channel = GetChannel(SoundS->SoundM, (unsigned int)Sound);

	if (!Channel)
		return GE_FALSE;

	return StopSoundChannel(Channel);	
}

//=====================================================================================
//	Sound_ModifySound
//=====================================================================================
GENESISAPI	geBoolean geSound_ModifySound(geSound_System *SoundS, 
								geSound *Sound,geFloat Volume, 
								geFloat Pan, 
								geFloat Frequency)
{
	Channel*	Channel;
	geSound_Cfg	LocalCfg;

	assert(SoundS != NULL);
	assert(Sound  != NULL);	

	Channel = GetChannel(SoundS->SoundM, (unsigned int)Sound);

	if (!Channel)
		return GE_FALSE;
	LocalCfg.Volume    = Volume;
	LocalCfg.Pan       = Pan;
	LocalCfg.Frequency = Frequency;
	LocalCfg.Volume *= SoundS->GlobalVolume;
	return ModifyChannel(Channel, &LocalCfg);
}

//=====================================================================================
//	Sound_SoundIsPlaying
//=====================================================================================
GENESISAPI	geBoolean geSound_SoundIsPlaying(geSound_System *SoundS, geSound *Sound)
{
	Channel*	Channel;

	assert(SoundS != NULL);
	assert(Sound  != NULL);	

	Channel = GetChannel(SoundS->SoundM, (unsigned int)Sound);

	if (!Channel)
		return GE_FALSE;

	return ChannelPlaying(Channel);
}


//=====================================================================================
//=====================================================================================

static	BOOL DSParseWaveResource(const void *pvRes, WAVEFORMATEX **ppWaveHeader,
                         BYTE **ppbWaveData,DWORD *pcbWaveSize)
{
    DWORD *pdw;
    DWORD *pdwEnd;
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;

    if (ppWaveHeader)
        *ppWaveHeader = NULL;

    if (ppbWaveData)
        *ppbWaveData = NULL;

    if (pcbWaveSize)
        *pcbWaveSize = 0;

    pdw = (DWORD *)pvRes;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        goto exit;      // not even RIFF

    if (dwType != mmioFOURCC('W', 'A', 'V', 'E'))
        goto exit;      // not a WAV

    pdwEnd = (DWORD *)((BYTE *)pdw + dwLength-4);

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
        case mmioFOURCC('f', 'm', 't', ' '):
            if (ppWaveHeader && !*ppWaveHeader)
            {
                if (dwLength < sizeof(WAVEFORMAT))
                    goto exit;      // not a WAV

                *ppWaveHeader = (WAVEFORMATEX *)pdw;

                if ((!ppbWaveData || *ppbWaveData) &&
                    (!pcbWaveSize || *pcbWaveSize))
                {
                    return TRUE;
                }
            }
            break;

        case mmioFOURCC('d', 'a', 't', 'a'):
            if ((ppbWaveData && !*ppbWaveData) ||
                (pcbWaveSize && !*pcbWaveSize))
            {
                if (ppbWaveData)
                    *ppbWaveData = (LPBYTE)pdw;

                if (pcbWaveSize)
                    *pcbWaveSize = dwLength;

                if (!ppWaveHeader || *ppWaveHeader)
                    return TRUE;
            }
            break;
        }

        pdw = (DWORD *)((BYTE *)pdw + ((dwLength+1)&~1));
    }

exit:
    return FALSE;
}

static	BOOL DSFillSoundBuffer(IDirectSoundBuffer *pDSB, BYTE *pbWaveData, DWORD cbWaveSize)
{

    if (pDSB && pbWaveData && cbWaveSize)
    {
        LPVOID pMem1, pMem2;
        DWORD dwSize1, dwSize2;

        if (SUCCEEDED(pDSB->Lock(0, cbWaveSize,
            &pMem1, &dwSize1, &pMem2, &dwSize2, 0)))
        {
            ZeroMemory(pMem1, dwSize1);
            CopyMemory(pMem1, pbWaveData, dwSize1);

            if ( 0 != dwSize2 )
                CopyMemory(pMem2, pbWaveData+dwSize1, dwSize2);

            pDSB->Unlock(pMem1, dwSize1, pMem2, dwSize2);
            return TRUE;
        }
    }
    return FALSE;
}


DSCAPS			dsCaps;
static	SoundManager *	CreateSoundManager(HWND hWnd )
{
	typedef HRESULT (WINAPI *DS_CREATE_FUNC)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);
	PCMWAVEFORMAT	pcmwf;
	DSBUFFERDESC	dsbdesc;
	HRESULT			hres;
	SoundManager *	sm;
//	DS_CREATE_FUNC pDirectSoundCreate;

	CoInitialize(NULL);

	// load the DirectSound DLL
	/*hmodDirectSound = LoadLibrary ("DSOUND.DLL");
	if (hmodDirectSound == NULL)
	{
		// Couldn't load DSOUND.DLL
		return NULL;
	}

	pDirectSoundCreate = (DS_CREATE_FUNC)GetProcAddress (hmodDirectSound, "DirectSoundCreate");
	if (pDirectSoundCreate == NULL)
	{
		// couldn't find the DirectSoundCreate function
		FreeLibrary (hmodDirectSound);
		return NULL;
	}*/

	hres = CoCreateInstance(CLSID_DirectSound8, nullptr, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (LPVOID*)&lpDirectSound);
	if (hres != S_OK)
	{
		return nullptr;
	}

	hres = lpDirectSound->Initialize(nullptr);
	if (hres != S_OK)
	{
		lpDirectSound->Release();
		lpDirectSound = nullptr;
		return nullptr;
	}

	/*hres = pDirectSoundCreate(NULL, &lpDirectSound, NULL);
	if	(hres != DS_OK)
	{
		// failed somehow
		FreeLibrary (hmodDirectSound);
		return NULL;
	}*/

//	sm = malloc(sizeof(*sm));
	sm = static_cast<SoundManager*>(geRam_Allocate(sizeof(*sm)));
	if	(!sm)
	{
		lpDirectSound->Release();
		lpDirectSound = nullptr;
		//FreeLibrary (hmodDirectSound);
		return NULL;
	}
	sm->smChannelCount = 0;
	sm->smNextChannelID = 1;
	sm->smChannels = NULL;

	memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;

	//pcmwf.wf.nChannels = 1;
	//pcmwf.wf.nSamplesPerSec = 44050;
	//pcmwf.wf.nBlockAlign = 2;
#if 1	
	pcmwf.wf.nChannels = 2;
	pcmwf.wf.nSamplesPerSec = 44100;
	pcmwf.wf.nBlockAlign = 4;
	pcmwf.wBitsPerSample = 16;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
#else
	pcmwf.wf.nChannels = 1;
	pcmwf.wf.nSamplesPerSec = 22050;
	pcmwf.wf.nBlockAlign = 1;
	pcmwf.wBitsPerSample = 8;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * 2;
#endif

	memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
	dsbdesc.dwSize = sizeof(DSBUFFERDESC);
	dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;// | DSBCAPS_CTRLDEFAULT;// | DSBCAPS_CTRL3D;
	dsbdesc.dwBufferBytes = 0; //dwBufferBytes and lpwfxFormat must be set this way.
	dsbdesc.lpwfxFormat = NULL;

#if 1
	if (DS_OK== lpDirectSound->SetCooperativeLevel(hWnd,DSSCL_NORMAL))
#else
	if (DS_OK== lpDirectSound->SetCooperativeLevel(hWnd,DSSCL_EXCLUSIVE))
#endif
	{
		if (DS_OK== lpDirectSound->CreateSoundBuffer(&dsbdesc, &sm->smPrimaryChannel, NULL))
		{
			return sm;
		}
		
		lpDirectSound->Release();
		lpDirectSound = nullptr;

		//FreeLibrary (hmodDirectSound);
	}
//	free( sm );
	geRam_Free(sm);
	return nullptr;
}

//static	BOOL CreateChannel( char* Name, DSBUFFERDESC*	dsBD, Channel** chanelPtr)
static	BOOL CreateChannel(DSBUFFERDESC*	dsBD, Channel** chanelPtr)
{
	Channel* channel;
	LPDIRECTSOUNDBUFFER lpBuf = nullptr;

//	channel = malloc( sizeof( Channel ) );
	channel = static_cast<Channel*>(geRam_Allocate( sizeof( Channel ) ));
	if	( channel == NULL )
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return( FALSE );
	}
	if(DS_OK != lpDirectSound->CreateSoundBuffer(dsBD, &lpBuf, NULL))
	{
		geErrorLog_Add(GE_ERR_CREATE_SOUND_BUFFER_FAILED, NULL);
		return FALSE;
	}

	if (DS_OK != lpBuf->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&channel->buffer))
	{
		lpBuf->Release();
		lpBuf = nullptr;

		geErrorLog_Add(GE_ERR_CREATE_SOUND_BUFFER_FAILED, NULL);
		return FALSE;
	}

	lpBuf->Release();
	lpBuf = nullptr;

	if(DS_OK != channel->buffer->GetFrequency((LPDWORD)&channel->BaseFreq) )
	{
		geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
		return FALSE;
	}
	channel->next = NULL;
	channel->nextDup = NULL;
	channel->ID = 0;
	channel->cfg.Volume = 1.0f;
	channel->cfg.Pan = 0.0f;
	channel->cfg.Frequency = 0.0f;
//	channel->name = Name;

	*chanelPtr = channel;
	return( TRUE );
}

//static	BOOL GetSoundData( char* Name, unsigned char** dataPtr)
static	BOOL GetSoundData( geVFile *File, unsigned char** dataPtr)
{
//	FILE * f;
	int32 Size;
	uint8 *data;
//	int32		CurPos;

#if 0
	f = fopen(Name, "rb");
	
	if (!f)
	{
		geErrorLog_Add(GE_ERR_FILE_OPEN_ERROR, NULL);
		return FALSE;
	}
#endif

#if 0
	CurPos = ftell (f);				// Save the startinf pos into this function
	fseek (f, 0, SEEK_END);			// Seek to end
	Size = ftell (f);				// Get End (this will be the size)
	fseek (f, CurPos, SEEK_SET);	// Restore file position
#endif

	if	(geVFile_Size(File, &Size) == GE_FALSE)
		return FALSE;

	data = static_cast<uint8*>(geRam_Allocate(Size));

	if (!data) 
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL);
		return FALSE;
	}
	
	if	(geVFile_Read(File, data, Size) == GE_FALSE)
	{
		geRam_Free(data);
		return FALSE;
	}

//	fread(data, Size, 1, f);

//	fclose(f);
	*dataPtr = data;
	return( TRUE );
}

static	BOOL ParseData( const uint8* data, DSBUFFERDESC* dsBD, BYTE ** pbWaveData )
{

	//Parse the Data
	memset(dsBD, 0, sizeof(DSBUFFERDESC));

	dsBD->dwSize = sizeof(DSBUFFERDESC);
	dsBD->dwFlags = DSBCAPS_STATIC | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN;
	if	(!DSParseWaveResource(data, &dsBD->lpwfxFormat, pbWaveData, &dsBD->dwBufferBytes))
	{
		geErrorLog_Add(GE_ERR_INVALID_WAV, NULL);
		return FALSE;
	}
	return( TRUE );

}

//static	BOOL FillSoundChannel(SoundManager *sm, char* Dir, char *Name, unsigned int* Handle )
static	BOOL FillSoundChannel(SoundManager *sm, geVFile *File, unsigned int* Handle )
{
	DSBUFFERDESC	dsBD;
	INT NumBytes;
	uint8		*data = nullptr;
	BYTE *			pbWaveData = nullptr;
//	char*	Name2;
	Channel* channel = nullptr;

	*Handle = 0;
	if (!sm)
		return TRUE;

#if 0
	//Open the file
	if (Dir)
	{
		Name2 = malloc( strlen( Name ) + strlen( Dir ) + 3);  // 2 for the "//" and 1 for terminator
		if( !Name2 )
			return( 0 );
		sprintf(Name2, "%s\\%s", Dir, Name);
	}
	else
	{
		Name2 = malloc( strlen( Name ) + 3);  // 2 for the "//" and 1 for terminator
		if( !Name2 )
			return( 0 );

		sprintf(Name2, "%s", Name);
	}
#endif
	if(!GetSoundData( File, &data ))
		return( FALSE );

	if( !ParseData( data, &dsBD, &pbWaveData ) )
	{
		geRam_Free(data);
		return( FALSE );
	}

	NumBytes = dsBD.dwBufferBytes;
	
	//Create the channel
//	if( !CreateChannel( Name2, &dsBD, &channel ) )
	if	(!CreateChannel(&dsBD, &channel))
	{
		geRam_Free(data);
		return FALSE;
	}
	channel->next = sm->smChannels;
	channel->ID = sm->smNextChannelID++;
	channel->Data = data;

	sm->smChannels = channel;
	sm->smChannelCount++;

	//Fill the channel
	if (!DSFillSoundBuffer(channel->buffer, pbWaveData, NumBytes))
		return FALSE;
	
//	free( data );
//	geRam_Free(data);

	*Handle = channel->ID;
	return TRUE;
}

#if 0
static	BOOL FillSoundChannelMemory(SoundManager *sm, const void *Buffer, unsigned int* Handle )
{
	DSBUFFERDESC	dsBD;
	INT 			NumBytes;
	BYTE *			pbWaveData;
	char *			Name;
	Channel * 		channel;

	*Handle = 0;
	if	(!sm)
		return TRUE;

	if	(!ParseData(Buffer, &dsBD, &pbWaveData))
		return FALSE;

	NumBytes = dsBD.dwBufferBytes;

	Name = malloc(11);
	if	(Name == NULL)
		return FALSE;
	sprintf(Name, "0x%8x", Buffer);
	
	//Create the channel
//	if	(!CreateChannel(Name, &dsBD, &channel))
	if	(!CreateChannel(&dsBD, &channel))
		return FALSE;

	channel->next = sm->smChannels;
	channel->ID   = sm->smNextChannelID++;

	sm->smChannels = channel;
	sm->smChannelCount++;

	//Fill the channel
	if	(!DSFillSoundBuffer(channel->buffer, pbWaveData, NumBytes))
		return FALSE;
	
	*Handle = channel->ID;
	return TRUE;
}
#endif

static	void StopDupBuffers( Channel* channel )
{
	Channel* dupChannel, *prevChannel;

	assert( channel );

	dupChannel = channel->nextDup;
	prevChannel = channel;
	while( dupChannel )
	{
		dupChannel->buffer->Stop();
		dupChannel = dupChannel->nextDup;
	}
}

static	void ClearDupBuffers( Channel* channel )
{
	Channel* dupChannel, *prevChannel;

	if( channel == NULL)
		return;

	dupChannel = channel->nextDup;
	prevChannel = channel;
	while( dupChannel )
	{
		if( !ChannelPlaying( dupChannel ) )
		{
			prevChannel->nextDup = dupChannel->nextDup;
			dupChannel->buffer->Release();
			dupChannel->buffer = nullptr;
//			free( dupChannel );
			geRam_Free(dupChannel);
			dupChannel = prevChannel->nextDup;
		}
		else
		{
			prevChannel = dupChannel;
			dupChannel = dupChannel->nextDup;
		}
	}
}

static	BOOL FreeAllChannels(SoundManager *sm)
{
	int Error;
	
	Channel* channel, *nextChannel;

	channel = sm->smChannels;
	while( channel )
	{
		nextChannel = channel->next;
		StopDupBuffers( channel );
		ClearDupBuffers( channel );
		Error = channel->buffer->Stop();
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
		Error = channel->buffer->Release();
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
		
//		if( channel->name )
//			geRam_Free(channel->name);
//			free( channel->name );
		if	(channel->Data)
			geRam_Free(channel->Data);
		geRam_Free(channel);
//		free( channel );
		channel = nextChannel;
	}
	sm->smChannels = NULL;
	sm->smChannelCount = 0;

	return TRUE;
}


static	BOOL FreeChannel(SoundManager *sm, Channel* channel)
{
	int Error;
	Channel*prevChannel = NULL, *curChannel;
	if	( channel )
	{
		StopDupBuffers( channel );
		ClearDupBuffers( channel );
		Error = channel->buffer->Stop();
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
		Error = channel->buffer->Release();
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
//		if( channel->name )
//			geRam_Free(channel->name);
//			free( channel->name );

		if( channel->Data )
			geRam_Free(channel->Data);

		curChannel = sm->smChannels;
		while( curChannel && curChannel != channel )
		{
			prevChannel = curChannel;
			curChannel = curChannel->next;
		}
		if( curChannel )
		{
			if( prevChannel )
				prevChannel->next = curChannel->next;
			else
				sm->smChannels = curChannel->next;
			geRam_Free(curChannel);
//			free( curChannel );
		}
	}

	return TRUE;
}

static	Channel* ReloadData(void *Data)
{
	DSBUFFERDESC	dsBD;
	BYTE *			pbWaveData;
	INT NumBytes;
//	uint8		*data = NULL;
	Channel* channel;

//	if( !Name )
//		return( NULL );
//	if( !GetSoundData( Data, &data ) )
//		return( NULL );

	if( !ParseData( (const uint8*)Data, &dsBD, &pbWaveData ) )
		return( NULL );

	NumBytes = dsBD.dwBufferBytes;
	
	//Create the channel
//	if( !CreateChannel( Name, &dsBD, &channel ) )
	if( !CreateChannel(&dsBD, &channel ) )
		return NULL;

	//Fill the channel
	if ( !DSFillSoundBuffer(channel->buffer, pbWaveData, NumBytes))
		return NULL;
	
//	geRam_Free(data);
//	free( data );
	return( channel );
}

static	BOOL DupChannel( SoundManager *sm, Channel* channel, Channel** dupChannelPtr )
{
	Channel* dupChannel;
	HRESULT Error;
	LPDIRECTSOUNDBUFFER lpBuf = nullptr;

	*dupChannelPtr = NULL;
//	dupChannel =  malloc( sizeof(Channel ) );
	dupChannel =  static_cast<Channel*>(geRam_Allocate( sizeof(Channel ) ));
	if( dupChannel == NULL )
	{
		geErrorLog_Add(GE_ERR_OUT_OF_MEMORY, NULL );
		return FALSE;
	}
	Error = lpDirectSound->DuplicateSoundBuffer(channel->buffer, &lpBuf );
	if( Error != DS_OK )
	{
		geRam_Free(dupChannel);
//		free( dupChannel );
		dupChannel = ReloadData( channel->Data );
		if( dupChannel == NULL )
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
	}

	lpBuf->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&dupChannel->buffer);

	dupChannel->ID =  sm->smNextChannelID++;
	dupChannel->next = NULL;
	dupChannel->nextDup = channel->nextDup;
	dupChannel->cfg = channel->cfg;
//	dupChannel->name = NULL;
	dupChannel->Data = channel->Data;
	channel->nextDup = dupChannel;
	*dupChannelPtr = dupChannel;
	return( TRUE );
}

static	BOOL	StartSoundChannel( SoundManager *sm, unsigned int Handle, geSound_Cfg *cfg, int loop, unsigned int* sfx)
{
	HRESULT	hres;
	Channel* channel, *dupChannel;
	
	if( Handle == 0 )
		return( FALSE );
	channel = GetChannel( sm, Handle );
	//Clear all non-playing duplicate buffers.
	ClearDupBuffers(channel);
	//If the main buffer is playing and all non-playing dups have been cleared
	//we need a new duplicate.
	if( ChannelPlaying( channel ) )
	{
		if(!DupChannel( sm,channel, &dupChannel ) )
			return( FALSE );
		channel = dupChannel;
	}
	if( !ModifyChannel( channel, cfg ) )
		return( FALSE );
	channel->buffer->SetCurrentPosition(0);
	hres = channel->buffer->Play(  0,
				  				   0,
				  				   loop ? DSBPLAY_LOOPING : 0);

	if	(hres == DS_OK)
	{
		if( sfx )
			*sfx = channel->ID;
		return TRUE;
	}
	
	geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
	return FALSE;
}

static	BOOL StopSoundChannel(Channel* channel)
{
	HRESULT	hres;

	assert(channel);

	hres = channel->buffer->Stop();

	if	(hres == DS_OK)
		return TRUE;

	geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
	return FALSE;
}

static	void DestroySoundManager(SoundManager *sm)
{
	if (!sm) return;

	FreeAllChannels( sm );
	if	(sm->smPrimaryChannel != NULL)
		sm->smPrimaryChannel->Release();
	if (lpDirectSound != NULL)
		lpDirectSound->Release();
	//if  (hmodDirectSound != NULL)
	//	FreeLibrary (hmodDirectSound);
	geRam_Free(sm);
//	free(sm);
}

static	BOOL	ModifyChannel( Channel *channel, geSound_Cfg *cfg )
{
	int Error, Vol, Pan, Freq;
	assert(channel);
	
	if( !cfg )
		return( TRUE );
	ClearDupBuffers(channel);
	if( cfg->Volume != channel->cfg.Volume )
	{
		Vol = (DWORD)((1.0 - cfg->Volume  ) * DSBVOLUME_MIN);
		Error = channel->buffer->SetVolume(Vol);
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
		channel->cfg.Volume = cfg->Volume;
	}

	if( cfg->Pan != channel->cfg.Pan )
	{
		Pan = (int)(cfg->Pan  * DSBPAN_RIGHT);
		Error = channel->buffer->SetPan(Pan);
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
		channel->cfg.Pan = cfg->Pan;
	}


	if( cfg->Frequency != channel->cfg.Frequency )
	{

		Freq = (DWORD)(channel->BaseFreq * cfg->Frequency);
		Error = channel->buffer->SetFrequency(Freq);
		if (Error != DS_OK)
		{
			geErrorLog_Add(GE_ERR_DS_ERROR, NULL);
			return FALSE;
		}
		channel->cfg.Frequency = cfg->Frequency;
	}

	return TRUE;
}

static	int	ChannelPlaying( Channel *channel )
{
	DWORD status, Error;

	if(!channel)
		return( 0 );

	Error = channel->buffer->GetStatus(&status);
	if( Error != DS_OK)
		return 0;
	return( status & DSBSTATUS_PLAYING  );
}

static	Channel* GetChannel( SoundManager *sm, unsigned int ID )
{
	Channel* dupChannel;
	Channel* channel = sm->smChannels;

	while( channel )
	{
		if( channel->ID == ID )
			break;
		dupChannel = channel->nextDup;
		while( dupChannel )
		{
			if( dupChannel->ID == ID )
				break;
			dupChannel = dupChannel->nextDup;
		}
		if( dupChannel )
			return( dupChannel );
		channel = channel->next;
	}
	return( channel );
}
