#include <windows.h>
#include <winsock.h>
#include	<stdio.h>

#include <string.h>
#include "ipaddr.h"

#define	MAXHOSTNAME	256

static	void	IPAddr_AddrToString(unsigned long Addr, char *AddrBuff)
{
	AddrBuff += sprintf(AddrBuff, "%d.", Addr & 0xff);
	Addr = Addr >> 8;

	AddrBuff += sprintf(AddrBuff, "%d.", Addr & 0xff);
	Addr = Addr >> 8;

	AddrBuff += sprintf(AddrBuff, "%d.", Addr & 0xff);
	Addr = Addr >> 8;

				sprintf(AddrBuff, "%d",  Addr & 0xff);
}

#define	WS_VERSION_REQD    	0x0101
#define	WS_VERSION_MAJOR    HIBYTE(WS_VERSION_REQD)
#define	WS_VERSION_MINOR    LOBYTE(WS_VERSION_REQD)

int IPAddr_GetHostID (char *AddrBuff)
{
	WSADATA			wsaData;
    char			HostName[MAXHOSTNAME];
    LPHOSTENT		HostEnt;
    SOCKADDR_IN 	LocalAddress;
    SOCKADDR_IN 	RemoteAddress;
    SOCKET			Socket;
    int				Res;
	int				i;

	Res = WSAStartup(WS_VERSION_REQD,&wsaData);
	if	(Res != 0)
		return 0;

	if	(( LOBYTE (wsaData.wVersion) < WS_VERSION_MAJOR) || 
		( LOBYTE (wsaData.wVersion) == WS_VERSION_MAJOR &&
		HIBYTE (wsaData.wVersion) < WS_VERSION_MINOR))
	{
		return 0;
	}

    LocalAddress.sin_addr.s_addr = INADDR_ANY;
    
    Res = gethostname(HostName, sizeof(HostName));
    if	(Res != SOCKET_ERROR)
    {
		HostEnt = gethostbyname(HostName);
		if	(HostEnt)
		{
			IPAddr_AddrToString(*(unsigned long *)HostEnt->h_addr_list[0], AddrBuff);
			i = 0;
			while	(HostEnt->h_addr_list[i])
				i++;
			return i;
		}
    } 
    
    Socket = socket(AF_INET, SOCK_DGRAM, 0);
	if	(Socket != INVALID_SOCKET)
	{
		RemoteAddress.sin_family		= AF_INET;
		RemoteAddress.sin_port			= htons(IPPORT_ECHO);
		RemoteAddress.sin_addr.s_addr	= inet_addr("199.1.90.2");
		Res = connect(Socket, (SOCKADDR *)&RemoteAddress, sizeof(SOCKADDR));
		if	(Res != SOCKET_ERROR)
		{
			int	AddrSize = sizeof(SOCKADDR);

			getsockname(Socket, (LPSOCKADDR)&LocalAddress, (int *)&AddrSize);

			IPAddr_AddrToString(LocalAddress.sin_addr.s_addr, AddrBuff);

			return 1;
		}

		closesocket(Socket);
	}

	return 0;
}

#if 0
void	main(void)
{
	char	Buff[16];
	int		Count;

	Count = IPAddr_GetHostID(Buff);
	printf("%d addrs\n", Count);
	printf("%s\n", Buff);
}
#endif

