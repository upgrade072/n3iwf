#ifndef __LIBUTIL_H__
#define __LIBUTIL_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <conflib.h>
#include <typedef.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>


extern int errno;

extern void commlib_convByteOrd (char*p, int);
extern void commlib_microSleep (int);
extern void commlib_setupSignals (int*);
extern void commlib_quitSignal (int);
extern void commlib_ignoreSignal (int);
extern char *commlib_printTStamp (void);
extern	void commlib_printTStamp2 (char *buf);
extern	char *commlib_printUsec (void);
extern char *commlib_printMicrosec(void);
extern char *commlib_printTStampUsec (void);
extern char *commlib_printDateTime (time_t);
extern	char *commlib_printDateHM (time_t when);
extern	int commlib_dateTimeStr (time_t when, char *tbuf);
extern	int commlib_dateTimeStr_z (time_t when, char *tbuf);
extern char *commlib_timeStr (time_t);
extern char *commlib_printTime (void);
extern	int commlib_dateStr2 (time_t when, char *buf);
extern	int commlib_nullcmp (unsigned char *src, int len);
extern	double commlib_getCurrTime_double (void);
extern	int commlib_getMachineId_fromString (char *name);
extern	int commlib_getOtherSideMachineName (char *own, char *other);
extern	int commlib_getLastTokenInString (char *src, char *delimiter, char *lastToken, char *prefixString);
extern	int commlib_tokenizeQuoteMark (char *src, char tokens[16][CONFLIB_MAX_TOKEN_LEN]);
extern	char *commlib_getNthTokenQuoteMark (char *src, int n);
extern	int getTime_tFromDateStr(time_t *destTime, char *srcStr);
extern	int commlib_dateTimeStr2 (time_t when, char *tbuf);
extern	int getTime_charFromTimeT(char *destStr, time_t *srcTime );
extern  int commlib_mktime(const char *strTime);
extern  void Hexa2char(unsigned char *h, char *c, int len);
extern  int Char2Hexa(char *c, unsigned char *h, int len);
extern  void Hexa2char2(unsigned char *h, char *c, int len);
extern  int Char2Hexa2(char *c, unsigned char *h, int len);
extern	double convDegree(double dbCoord);
extern	double convCoord(double degree);
extern void Octet2String(int numdigit, unsigned char *tmp, char *term_num);
extern int String2Octet(char *in_string, unsigned char *out_bcd);
extern char *commlib_strIp(int af, unsigned char *src);
extern void commlib_strhexa(char *str, unsigned char *src, int size);
extern int commlib_getTimeStr(time_t when, char* format, char* buf, size_t buf_size);
extern bool commlib_cvtStr2Time_14(char *str, time_t *tt);

extern	int	ipinfo_str(ip_info_t *ipinfop, int size, char *str);
extern	int	str_to_ipinfo(char *str, int af, ip_info_t *ipInfop);
extern	char *commlib_printDateHM2 ();

extern int str2tbcd(uint8_t *dst, const char *c);
extern int tbcd2str(char *dst, uint8_t *h, int len);
extern int util_get_progname (char *argv, char *dest);

extern int Remove_special_char(char* source);

extern uint64_t get_db_crc64_key(char *pro, int len);
extern int unidirection_char2hexa(char *c, unsigned char *h, int len);
extern int unidirection_hexa2char(unsigned char *h, int hlen, char *c, int clen);


#endif /*__COMM_UTIL_H__*/
