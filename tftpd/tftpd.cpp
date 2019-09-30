
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#endif // _WIN32
#include <stdio.h>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <vector>
#include <thread>

#include "translate.h"
#include "packetopt.h"
#include "handlecmd.h"

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR (-1)
#endif

/*定义处理客户端发来的申请线程处理函数*/
void Handle_Apply(void *arg)
{
	Remote_information remote = *(Remote_information *)arg;
	PACKET_OPT_TYPE opttypes = getoptcode(remote.buf);
	handlecmd(opttypes, &remote);
}

/*生成一个与本地69端口绑定的socket*/
static int  getlistensock()
{
	int ret;
	int sock;

	struct sockaddr_in lisaddr;
	memset(&lisaddr, 0, sizeof(lisaddr));
	lisaddr.sin_family = AF_INET;
	lisaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	lisaddr.sin_port = htons(69);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == sock)
	{
	#ifdef _WIN32
		printf("socket() error, errno[%d], info: %s\n", WSAGetLastError(), strerror(WSAGetLastError()));
	#else
		printf("socket() error, errno[%d], info: %s\n", errno, strerror(errno));
	#endif //_WIN32
		return -1;
	}

	// resolve the WAIT_TIME problem after close, which causes bind to fail.
	int val = 1;
	ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(int));
	if (SOCKET_ERROR == ret)
	{
	#ifdef _WIN32
		printf("setsockopt() error, errno[%d], info: %s\n", WSAGetLastError(), strerror(WSAGetLastError()));
	#else
		printf("setsockopt() error, errno[%d], info: %s\n", errno, strerror(errno));
	#endif //_WIN32
	}

	ret = bind(sock, (struct sockaddr *)&lisaddr, sizeof(lisaddr));
	if (SOCKET_ERROR == ret)
	{
	#ifdef _WIN32
		printf("bind() error, errno[%d], info: %s\n", WSAGetLastError(), strerror(WSAGetLastError()));
	#else
		printf("bind() error, errno[%d], info: %s\n", errno, strerror(errno));
	#endif //_WIN32
		return -2;
	}

	return sock;
}

int main()
{
#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif // _WIN32

	int sock = 0;
	fd_set fdset;

	Remote_information remote;
	memset(&remote, 0, sizeof(remote));

	sock = getlistensock(); /*get a UDP type sock decriptor*/
	if (sock < 0)
	{
		printf("getlistensock error: %d\n", sock);
		return 1;
	}

	std::vector<std::thread> threads;

	do
	{
		FD_ZERO(&fdset);
		FD_SET(sock, &fdset);
		int ret = select(sock + 1, &fdset, NULL, NULL, NULL);
		if (ret > 0)
		{
			ret = myrecvdata(sock, &remote);
			if (ret < 0)
			{
				printf("error3\n");
			}
			else
			{
				threads.push_back(std::thread([=]()
				{
					Handle_Apply((void*)&remote);
				}));
			}
		}
		else
		{
			printf("server died , restart...\n");
		}
	} while (1);

	for (auto& thread : threads)
	{
		thread.join();
	}

#ifdef _WIN32
	closesocket(sock);
	WSACleanup();
#else
	close(sock);
#endif // _WIN32

}
