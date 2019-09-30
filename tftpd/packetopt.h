/**********************************************
*file name : packetopt.h
***********************************************/
#ifndef _PACKETOPT_H_
#define _PACKETOPT_H_

typedef unsigned int U16;

// error info
#define		UNKNOWNERROR	"Unknown error"
#define		FILENOTFIND		"Request file is not exist"
#define		OPTILLAGE		"File access is illage"
#define		SPACELOW		"Disk space is not enough"
#define		FILEEXIST		"File is already existed"
#define		INCORECTNUM		"Incorect Block Number"

//Error Codes
typedef enum 
{
	 EUNDEF = 0,	/* not defined */
	 ENOTFOUND = 1,	/* file not found */
	 EACCESS,		/* access violation */
	 ENOSPACE,		/* disk full or allocation exceeded */
	 EBADOP,		/* illegal TFTP operation */
	 EBADID,		/* unknown transfer ID */
	 EEXISTS		/* file already exists */
}ERR_TYPE ;

// optcodes
typedef enum
{
	 RRQ = 1,
	 WRQ ,
	 DATA,
	 ACK ,
     ERR 
}PACKET_OPT_TYPE;

PACKET_OPT_TYPE  getoptcode(char *buf)  ;
int getRWRQparm (char *pfilename,char *model,char *buf) ;
int getAckparm  (char *buf) ;
int getDataparm  (char *buf);
int getErrparm  (U16 errno , char *errmsg) ;

int packetack (U16 blocks,char *buf);
#ifdef _WIN32
int packetdata(U16 blocks, char *pdata, HANDLE hFile);
#else
int packetdata(U16 blocks, char *buf, int fd);
#endif // _WIN32
int packeterr (char *buf, ERR_TYPE errtype , const char *errmsg) ;


#endif 
