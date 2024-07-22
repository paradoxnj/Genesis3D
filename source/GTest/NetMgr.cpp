/****************************************************************************************/
/*  NetMgr.c                                                                            */
/*                                                                                      */
/*  Author: John Pollard                                                                */
/*  Description:                                                                        */
/*                                                                                      */
/*  Copyright (c) 1999 WildTangent, Inc.; All rights reserved.               */
/*                                                                                      */
/*  See the accompanying file LICENSE.TXT for terms on the use of this library.         */
/*  This library is distributed in the hope that it will be useful but WITHOUT          */
/*  ANY WARRANTY OF ANY KIND and without any implied warranty of MERCHANTABILITY        */
/*  or FITNESS FOR ANY PURPOSE.  Refer to LICENSE.TXT for more details.                 */
/*                                                                                      */
/****************************************************************************************/
#include <Windows.h>
#include <assert.h>
#include <stdio.h>

#include "GENESIS.H"
#include "Errorlog.h"
#include "RAM.H"

#include "NetMgr.h"
#include "Buffer.h"

//=====================================================================================
//=====================================================================================
typedef struct NetMgr
{
	NetMgr				*SelfCheck1;

	geCSNetMgr			*CSNetMgr;

	// Local loopback info
	geBoolean			UseLocalBuffers;

	uint8				ClientBuffer[NETMGR_LOCAL_MSG_BUFFER_SIZE];
	uint8				ServerBuffer[NETMGR_LOCAL_MSG_BUFFER_SIZE];

	Buffer_Data			ClientToServerBuffer;
	Buffer_Data			ServerToClientBuffer;

	NetMgr				*SelfCheck2;
} NetMgr;

//=====================================================================================
//	NetMgr_Create
//=====================================================================================
NetMgr *NetMgr_Create(geBoolean UseLocalBuffers)
{
	NetMgr	*NMgr;

	NMgr = GE_RAM_ALLOCATE_STRUCT(NetMgr);

	if (!NMgr)
	{
		geErrorLog_AddString(-1, "NetMgr_Create:  Could not create NetMgr object.", NULL);
		return NULL;
	}

	memset(NMgr, 0 , sizeof(NetMgr));

	// Setup local message buffers...
	Buffer_Set(&NMgr->ClientToServerBuffer, (char*)NMgr->ClientBuffer, NETMGR_LOCAL_MSG_BUFFER_SIZE);
	Buffer_Set(&NMgr->ServerToClientBuffer, (char*)NMgr->ServerBuffer, NETMGR_LOCAL_MSG_BUFFER_SIZE);

	NMgr->UseLocalBuffers = UseLocalBuffers;

	if (!UseLocalBuffers)
	{
		NMgr->CSNetMgr = geCSNetMgr_Create();

		if (!NMgr->CSNetMgr)
		{
			geErrorLog_AddString(-1,"NetMgr_Create Could not create geCSNeMgr...\n", NULL);
			NetMgr_FreeAllObjects(NMgr);
			return NULL;
		}
	}

	NMgr->SelfCheck1 = NMgr;
	NMgr->SelfCheck2 = NMgr;

	return NMgr;
}

//=====================================================================================
//	NetMgr_Destroy
//=====================================================================================
void NetMgr_Destroy(NetMgr *NMgr)
{
	assert(NetMgr_IsValid(NMgr));

	NetMgr_FreeAllObjects(NMgr);
}

//=====================================================================================
//	NetMgr_FreeAllObjects
//=====================================================================================
void NetMgr_FreeAllObjects(NetMgr *NMgr)
{
	assert(NMgr);

	if (NMgr->CSNetMgr)
	{
		assert(NMgr->UseLocalBuffers == GE_FALSE);
		geCSNetMgr_StopSession(NMgr->CSNetMgr);
		geCSNetMgr_Destroy(&NMgr->CSNetMgr);
	}
		
	NMgr->CSNetMgr = NULL;

	geRam_Free(NMgr);
}

//=====================================================================================
//	NetMgr_IsValid
//=====================================================================================
geBoolean NetMgr_IsValid(NetMgr *NMgr)
{
	if (!NMgr)
	{
		geErrorLog_AddString(-1, "NetMgr_IsValid:  NULL NetMgr object.", NULL);
		return GE_FALSE;
	}

	if (NMgr->SelfCheck1 != NMgr)
	{
		geErrorLog_AddString(-1, "NetMgr_IsValid:  NMgr->SeflfCheck1 != NMgr", NULL);
		return GE_FALSE;
	}

	if (NMgr->SelfCheck2 != NMgr)
	{
		geErrorLog_AddString(-1, "NetMgr_IsValid:  NMgr->SelfCheck2 != NMgr", NULL);
		return GE_FALSE;
	}

	return GE_TRUE;
}

//===========================================================================
//	NetMgr_StartSession
//===========================================================================
geBoolean NetMgr_StartSession(NetMgr *NMgr, const char *SessionName, const char *PlayerName)
{
	assert(NetMgr_IsValid(NMgr));

	assert(NMgr->UseLocalBuffers == GE_FALSE);

	if (!geCSNetMgr_StartSession(NMgr->CSNetMgr, SessionName, PlayerName))
		return GE_FALSE;

	return GE_TRUE;
}

//===========================================================================
//	NetMgr_JoinSession
//===========================================================================
geBoolean NetMgr_JoinSession(NetMgr *NMgr, const char *IPAddress, const char *PlayerName)
{
	geCSNetMgr_NetSession	*SessionList;
	int32					NumSessions;

	assert(NetMgr_IsValid(NMgr));
	assert(NMgr->UseLocalBuffers == GE_FALSE);

	if (!geCSNetMgr_FindSession(NMgr->CSNetMgr, IPAddress, &SessionList, &NumSessions))
	{
		geErrorLog_AddString(-1, "NetMgr_JoinSession:  geCSNetMgr_JoinSession failed...", NULL);
		return GE_FALSE;
	}

	if (!NumSessions)
	{
		geErrorLog_AddString(-1, "NetMgr_JoinSession:  Could not find a session at address:", IPAddress);
		return GE_FALSE;
	}

	if (!geCSNetMgr_JoinSession(NMgr->CSNetMgr, PlayerName, &SessionList[0]))
	{
		geErrorLog_AddString(-1, "NetMgr_JoinSession:  Could not join a session at address:", IPAddress);
		return GE_FALSE;
	}

	return GE_TRUE;
}

//===========================================================================
//	NetMgr_GetOurID
//===========================================================================
geCSNetMgr_NetID NetMgr_GetOurID(NetMgr *NMgr)
{
	assert(NetMgr_IsValid(NMgr));

	return geCSNetMgr_GetOurID(NMgr->CSNetMgr);
}

//===========================================================================
//	NetMgr_SendServerMessage
//	Send a message to the server
//===========================================================================
geBoolean NetMgr_SendServerMessage(NetMgr *NMgr, Buffer_Data *Buffer, geBoolean G)
{
	assert(NetMgr_IsValid(NMgr));

	if (NMgr->UseLocalBuffers)
	{
		if (!Buffer_FillBuffer(&NMgr->ClientToServerBuffer, Buffer))
			return GE_FALSE;
	}
	else
	{
		if (!geCSNetMgr_SendToServer(NMgr->CSNetMgr, G, Buffer->Data, Buffer->Pos))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//===========================================================================
//	NetMgr_SendClientMessage
//	Send a message to the client
//===========================================================================
geBoolean NetMgr_SendClientMessage(NetMgr *NMgr, geCSNetMgr_NetID NetID, Buffer_Data *Buffer, geBoolean G)
{
	assert(NetMgr_IsValid(NMgr));
    
	if (NetID == NETMGR_SPECIAL_BOT_NETID)
		return GE_TRUE;

	if (NMgr->UseLocalBuffers)
	{
		if (!Buffer_FillBuffer(&NMgr->ServerToClientBuffer, Buffer))
			return GE_FALSE;
	}
	else
	{
		if (!geCSNetMgr_SendToClient(NMgr->CSNetMgr, NetID, G, Buffer->Data, Buffer->Pos))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	NetMgr_ReceiveServerMessage
//=====================================================================================
geBoolean NetMgr_ReceiveServerMessage(NetMgr *NMgr, geCSNetMgr_NetMsgType *Type, Buffer_Data *Buffer)
{
	assert(NetMgr_IsValid(NMgr));

	if (NMgr->UseLocalBuffers)
	{
		*Type = NET_MSG_USER;

		// Use the short-curcuit  buffers for local messages 
		Buffer->Size = NMgr->ServerToClientBuffer.Pos;
		Buffer->Data = NMgr->ServerToClientBuffer.Data;
		
		NMgr->ServerToClientBuffer.Pos = 0;

		if (!Buffer->Size)
			*Type = NET_MSG_NONE;
	}
	else
	{
		// Use real message system if in real network mode
		if (!geCSNetMgr_ReceiveFromServer(NMgr->CSNetMgr, Type, &Buffer->Size, &Buffer->Data))
			return GE_FALSE;
	}

	return GE_TRUE;
}

//=====================================================================================
//	NetMgr_ReceiveClientMessage
//=====================================================================================
geBoolean NetMgr_ReceiveClientMessage(NetMgr *NMgr, geCSNetMgr_NetMsgType *MsgType, geCSNetMgr_NetID *ClientID, Buffer_Data *Buffer)
{
	if (NMgr->UseLocalBuffers)
	{
		*ClientID = NETMGR_SINGLE_PLAYER_NETID;
		*MsgType = NET_MSG_USER;
			
		Buffer->Size = NMgr->ClientToServerBuffer.Pos;
		Buffer->Data = NMgr->ClientToServerBuffer.Data;
		NMgr->ClientToServerBuffer.Pos = 0;

		if (!Buffer->Size)
			*MsgType = NET_MSG_NONE;
	}
	else
	{
		if (!geCSNetMgr_ReceiveFromClient(NMgr->CSNetMgr, MsgType, ClientID, &Buffer->Size, &Buffer->Data))
			return GE_FALSE;
	}

	return GE_TRUE;		// Got a message for'em
}

//=====================================================================================
//	NetMgr_ResetClientBuffer
//=====================================================================================
void NetMgr_ResetClientBuffer(NetMgr *NMgr)
{
	NMgr->ClientToServerBuffer.Pos = 0;
}

//=====================================================================================
//	NetMgr_ResetServerBuffer
//=====================================================================================
void NetMgr_ResetServerBuffer(NetMgr *NMgr)
{
	NMgr->ServerToClientBuffer.Pos = 0;
}
