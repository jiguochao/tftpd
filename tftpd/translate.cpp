#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif // _WIN32
#include <cstring>

#include "translate.h"

#ifdef _WIN32
#define socklen_t int
#endif // _WIN32
	
int myrecvdata(int sock, Remote_information * remote)
{
	int ret ;
	int len ;

	len = sizeof(remote->remoteaddr);
	memset(remote, 0, sizeof(Remote_information));
	ret = recvfrom(sock, remote->buf, BUFLEN, 0, &(remote->remoteaddr), (socklen_t*)&len);
	return ret;
}

int mysenddata(int sock ,Remote_information* remote,int buflen)
{
	return sendto(sock, remote->buf, buflen, 0, &(remote->remoteaddr), sizeof(remote->remoteaddr));
}
