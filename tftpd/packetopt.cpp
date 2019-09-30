/**********************************************************
filename : packetopt.c
**********************************************************/
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif // _WIN32
#include <string.h>
#include <stdio.h>
#include "packetopt.h"

PACKET_OPT_TYPE  getoptcode(char *buf)
{
	U16  optcode;
	memcpy(&optcode, buf, 2);
	optcode = ntohs(optcode);
	return (PACKET_OPT_TYPE)optcode;
}

int getRWRQparm(char *pfilename, char *model, char *buf)
{
	int len;
	strcpy(pfilename, buf + 2);
	len = strlen(buf + 2);
	strcpy(model, buf + 3 + len);
	return 0;
}

int getAckparm(char * buf)
{
	U16 blocks;
	memcpy(&blocks, buf + 2, 2);
	blocks = ntohs(blocks);
	return blocks;
}

int getErrparm(U16 errno, char *errmsg)
{
	return 0;
}

int getDataparm(char * buf)
{
	U16  blocks;
	memcpy(&blocks, buf + 2, 2);
	blocks = ntohs(blocks);
	return  blocks;
}
int packetack(U16 blocks, char *buf)
{
	U16   optcode = ACK;
	optcode = htons(optcode);
	blocks = htons(blocks);
	memcpy(buf, &optcode, 2);
	memcpy(buf + 2, &blocks, 2);
	return 4;
}

#ifdef _WIN32
int packetdata(U16 blocks, char *buf, HANDLE hFile)
{
	int ret;
	char data[1024] = { 0 };
	U16 optcode = DATA;
	optcode = htons(optcode);
	blocks = htons(blocks);
	if (FALSE == ReadFile(hFile, data, 512, (LPDWORD)&ret, NULL))
	{
		printf("read the file failed!, errno: %d, info: %s\n", GetLastError(), strerror(GetLastError()));
		ret = 0;
	}

	memcpy(buf, &optcode, 2);
	memcpy(buf + 2, &blocks, 2);
	memcpy(buf + 4, data, ret);
	return (ret + 4);
}
#else
int packetdata(U16 blocks, char *buf, int fd)
{
	int ret;
	char data[1024] = { 0 };
	U16  optcode = DATA;
	optcode = htons(optcode);
	blocks = htons(blocks);
	ret = read(fd, data, 512);
	memcpy(buf, &optcode, 2);
	memcpy(buf + 2, &blocks, 2);
	memcpy(buf + 4, data, ret);
	return ret + 4;
}
#endif // _WIN32

int packeterr(char *buf, ERR_TYPE errtype, const char *errmsg)
{
	U16 optcode = ERR;
	U16 errnum = (U16)errtype;
	optcode = htons(optcode);
	errnum = htons(errnum);
	memcpy(buf, &optcode, 2);
	memcpy(buf + 2, &errnum, 2);
	strcpy(buf + 4, errmsg);
	return strlen(errmsg) + 4;
}
