#pragma once
#define BUFLEN 1024

//定义数据类型, 存储接收到的数据以及远程客户端的信息
typedef struct  remote_information
{
	char buf[BUFLEN];
	struct sockaddr remoteaddr;
}Remote_information;

int myrecvdata(int sock, Remote_information * remote);
int mysenddata(int sock, Remote_information *remote, int len);