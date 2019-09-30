/*************************************************
*filename : handlecmd.c
*function : cmd handle
**************************************************/
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <sys/socket.h>
#include <unistd.h>
#endif // _WIN32

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "packetopt.h"
#include "translate.h"
#define  MAXTIME  5 
static  void handle_rrq(Remote_information * premote);
static  void handle_wrq(Remote_information * premote);
static  int  selectsock(int sock, int timeout);

void  handlecmd(PACKET_OPT_TYPE cmdtype, Remote_information *premote)
{
	int ret;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	switch (cmdtype)
	{
	case RRQ:
		handle_rrq(premote);
		break;
	case WRQ:
		handle_wrq(premote);
		break;
	default:
		ret = packeterr(premote->buf, EUNDEF, UNKNOWNERROR);
		ret = mysenddata(sock, premote, ret);
		printf("UNKNOWN CMD\n");
		break;
	}
}

static int selectsock(int sock, int timeout)
{
	int ret;
	fd_set fds;
	struct timeval  tm;
	tm.tv_sec = timeout;
	tm.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	ret = select(sock + 1, &fds, NULL, NULL, &tm);
	//printf("ret: %d\n", ret);
	return ret;
}

#ifdef _WIN32
static void handle_rrq(Remote_information * premote)
{
	printf("retset\n");
	int retranstime = 0;
	int flag = 0;
	unsigned short int  sendblock = 1;
	unsigned short int  ackblock = 1;
	int sock;
	int len;
	int ret;

	char filename[128] = { 0 };
	char model[10] = { 0 };
	PACKET_OPT_TYPE opttype;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	getRWRQparm(filename, model, premote->buf);

	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("file not exit\n");
		memset(premote->buf, 0, sizeof(premote->buf));
		len = packeterr(premote->buf, ENOTFOUND, FILENOTFIND);
		ret = mysenddata(sock, premote, len);

		return;
	}

	DWORD dwSize = GetFileSize(hFile, NULL); //读取文件大小
	DWORD sendSize = 0;

	while (1)
	{
		memset(premote->buf, 0, BUFLEN);
		len = packetdata(sendblock, premote->buf, hFile);
		ret = mysenddata(sock, premote, len);
		if (0 < ret && ret < 516)
		{
			flag = 1;
			printf("the last packet\n");
		}

		sendSize += (ret-4);
		int progress = ((float)sendSize / dwSize) * 100;
		printf("%d, %d, translate data %d%%\n", sendSize, dwSize, progress);

		// timeout
		ret = selectsock(sock, 3);
		if (ret <= 0)
		{
			if (retranstime == MAXTIME)
			{
				printf("unreached, Retrans max\n");
				return;
			}
			printf("retrans\n");
			retranstime++;
			continue;
		}

		// ack
		ret = myrecvdata(sock, premote);
		if (ret > 0)
		{
			opttype = getoptcode(premote->buf);
			switch (opttype)
			{
			case ACK:
				if (ackblock == getAckparm(premote->buf))
				{
					ackblock++;
				}
				else if(ackblock != 1 && (unsigned short)getAckparm(premote->buf) == (ackblock - 1))
				{
					printf("block is not equal: block: %d, recv: %d\n", ackblock, getAckparm(premote->buf));
				}
				else
				{
					len = packeterr(premote->buf, EBADID, INCORECTNUM);
					ret = mysenddata(sock, premote, len);
					return;
				}
				break;
			case ERR:
				printf("we really can got error packet\n");
				break;
			default:
				printf("unknow ack optcode: %d\n", opttype);
				break;
			}

			sendblock++;
		}

		// trans over
		if (flag)
		{
			CloseHandle(hFile);
			closesocket(sock);
			printf("translate complete...\n");
			break;
		}
	}

	printf("RRQ\n");
}

static void handle_wrq(Remote_information * premote)
{
	unsigned short int  block = 0;//记录数据包的编号
	int sock;
	int len;
	int ret;
	int fd;
	int flag = 0;//定义一个标志，判断是否是最后一个数据包
	char filename[30] = { 0 };//暂存文件名
	char model[10] = { 0 };//暂存模式octet 和 netascii模式

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	getRWRQparm(filename, model, premote->buf);

	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		printf("create file failed\n");
		memset(premote->buf, 0, sizeof(premote->buf));
		len = packeterr(premote->buf, EACCESS, OPTILLAGE);
		ret = mysenddata(sock, premote, len);

		return;
	}

	while (1)
	{
		/*判断收到的包的序号是否正确 */
		if (0 != block)
		{
			len = myrecvdata(sock, premote);
			ret = getDataparm(premote->buf);

			if (ret != block)
			{
				printf("this is a erro packet, abandon it\n");
				memset(premote->buf, 0, BUFLEN);
				continue;
			}

			WriteFile(hFile, premote->buf + 4, len - 4, (LPDWORD)(&ret), NULL);

			if (ret > 0 && ret < 512)
			{
				memset(premote->buf, 0, BUFLEN);
				flag = 1;
			}
		}

		memset(premote->buf, 0, BUFLEN);
		len = packetack(block, premote->buf);
		ret = mysenddata(sock, premote, len);
		block++;

		if (flag)
		{
			CloseHandle(hFile);
			closesocket(sock);
			break;
		}
	}

	printf("WRQ\n");
	return;
}
#else
static  void handle_rrq(Remote_information * premote)
{
	printf("retset\n");
	int retranstime = 0;
	int flag = 0;
	unsigned short int  sendblock = 1;
	unsigned short int  ackblock = 1;
	int sock;
	int len;
	int ret;
	int fd;
	char filename[128] = { 0 };
	char model[10] = { 0 };
	PACKET_OPT_TYPE opttype;

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	getRWRQparm(filename, model, premote->buf);
	fd = open(filename, O_RDONLY, 0666);
	if (fd < 0)
	{
		if (errno == ENOENT)
		{
			printf("file not exit\n");
			memset(premote->buf, 0, sizeof(premote->buf));
			len = packeterr(premote->buf, ENOTFOUND, FILENOTFIND);
			ret = mysenddata(sock, premote, len);
		}
		else if (errno == EEXIST)
		{
			memset(premote->buf, 0, sizeof(premote->buf));
			len = packeterr(premote->buf, EEXISTS, FILEEXIST);
			ret = mysenddata(sock, premote, len);
		}
		else if (errno == EDQUOT)
		{
			printf("file not eixist\n");
			memset(premote->buf, 0, sizeof(premote->buf));
			len = packeterr(premote->buf, ENOTFOUND, FILENOTFIND);
			ret = mysenddata(sock, premote, len);
		}
		printf("open file erro %s\n", strerror(errno));
		return;
	}

	while (1)
	{
		memset(premote->buf, 0, BUFLEN);
		len = packetdata(sendblock, premote->buf, fd);
		ret = mysenddata(sock, premote, len);
		if (0 < ret && ret < 516)
		{
			flag = 1;
			printf("the last packet\n");
		}

		ret = selectsock(sock, 3);
		if (ret <= 0)
		{
			if (retranstime == MAXTIME)
			{
				printf("unreached, Retrans max\n");
				return;
			}
			printf("retrans\n");
			retranstime++;
			continue;
		}

		ret = myrecvdata(sock, premote);

		if (ret > 0)
		{
			opttype = getoptcode(premote->buf);
			switch (opttype)
			{
			case ACK:
				if (ackblock == getAckparm(premote->buf))
				{
					ackblock++;
				}
				else if (ackblock != 1 && (unsigned short)getAckparm(premote->buf) == (ackblock - 1))
				{
					printf("block is not equal: block: %d, recv: %d\n", ackblock, getAckparm(premote->buf));
				}
				else
				{
					len = packeterr(premote->buf, EBADID, INCORECTNUM);
					ret = mysenddata(sock, premote, len);
					return;
				}
				break;
			case ERR:
				printf("we really can got error packet\n");
				break;
			default:
				printf("unknow ack optcode: %d\n", opttype);
				break;
			}

			sendblock++;
		}

		// trans over
		if (flag)
		{
			close(fd);
			close(sock);
			printf("translate complete...\n");
			break;
		}
	}


	printf("RRQ\n");
}

static  void handle_wrq(Remote_information * premote)
{
	unsigned short int  block = 0;//记录数据包的编号
	int sock;
	int len;
	int ret;
	int fd;
	int flag = 0;//定义一个标志，判断是否是最后一个数据包
	char filename[30] = { 0 };//暂存文件名
	char model[10] = { 0 };//暂存模式octet 和 netascii模式

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	getRWRQparm(filename, model, premote->buf);
	fd = open(filename, O_WRONLY | O_CREAT, 0666);

	if (fd < 0)
	{
		memset(premote->buf, 0, sizeof(premote->buf));
		len = packeterr(premote->buf, EUNDEF, UNKNOWNERROR);
		ret = mysenddata(sock, premote, len);
		close(fd);
		close(sock);
		printf("open file erro %d\n", ret);
		return;
	}

	while (1)
	{
		/*判断收到的包的序号是否正确 */
		if (0 != block)
		{
			len = myrecvdata(sock, premote);
			ret = getDataparm(premote->buf);

			if (ret != block)
			{
				printf("this is a erro packet, abandon it\n");
				memset(premote->buf, 0, BUFLEN);
				continue;
			}

			ret = write(fd, premote->buf + 4, len - 4);

			if (ret > 0 && ret < 512)
			{
				memset(premote->buf, 0, BUFLEN);
				flag = 1;
			}
		}

		memset(premote->buf, 0, BUFLEN);
		len = packetack(block, premote->buf);
		ret = mysenddata(sock, premote, len);
		block++;

		if (flag)
		{
			close(fd);
			close(sock);
			break;
		}
	}

	printf("WRQ\n");
}
#endif // _WIN32
