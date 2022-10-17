#ifndef __EXTCONN_H__
#define __EXTCONN_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>

#include "comm_msgq.h"
#include "extconn_type.h"


#define EXTCONN_EBUF_MAX		1024
#define EXTCONN_IS_SET(entry)	((entry)->ipAddr[0] ? 1 : 0)	

#ifndef SFM_MAX_TARGET_IP_LEN
#define	SFM_MAX_TARGET_IP_LEN			16
#endif

typedef struct sfm_extconn_info {
	/* Common Information */
	int				id;
    char            ipAddr[SFM_MAX_TARGET_IP_LEN];
    char            ipAddrSec[SFM_MAX_TARGET_IP_LEN];
	int				port;
    int				maxConn;
    int				curConn;

#define	STS_ABNORMAL	0
#define	STS_NORMAL		1
#define	STS_INACTIVE    2
#define	STS_STANDBY     3
    int			    status;						// 1: Normal, 0: Abnormal */
    char            desc[EXTCONN_NAME_LEN];	// 연동대상 시스템 이름

	/* Specific Information */
#define HA_INACTIVE		0
#define HA_ACTIVE		1
#define EXTCONN_IDX(x)	((x)->intReserved1)		/* 2015.07.01.ADDED */
	int				intReserved1;				/* INDEX. 2015.07.01.ADDED */		
	int				intReserved2;				// GTP', NodeAlive 체크 용
	int				intReserved3;   			// GTP' EchoRequest Connection Check
	int				intReserved4;   			// GTP' EchoRequest Connection Check
	int				intReserved5; 				// PGW Event 알람 Time  
#define EXT_NOT_CONNECTED	'0'
#define EXT_CONNECTED		'1'
#define EXT_NOT_USED		'X'
	char			strReserved1[EXTCONN_NAME_LEN];	
	char			strReserved2[EXTCONN_NAME_LEN];	
	char			strReserved3[EXTCONN_NAME_LEN];
	char			strReserved4[EXTCONN_NAME_LEN];	
	char			strReserved5[EXTCONN_NAME_LEN];   
} SFM_ExtConnInfo;


typedef struct sfm_extconn_list {
	SFM_ExtConnInfo		data[MAX_EXTCONN_TYPE][MAX_CONN_NUM];
} SFM_ExtConnInfoList;

extern char mySysName[];

struct sfm_extconn_info;
int extconn_attach_shm (void);
int extconn_init_nstep (int writer);
int extconn_init_rome (int writer);
int extconn_init_pgw (int writer);
int extconn_init_gtpp (int writer);
int extconn_init_radius (int writer);
int extconn_init_ntp (int writer);
int extconn_create_shm (void);
int extconn_check_exist (uint8_t type, uint32_t id);
struct sfm_extconn_info* extconn_lookup (uint8_t type, uint32_t id);
const struct sfm_extconn_info* extconn_add (uint8_t type, struct sfm_extconn_info *data, char *ebuf);
int extconn_delete (struct sfm_extconn_info *entry);
//int extconn_save_file (char *ebuf);
int extconn_save_each_file (uint8_t type, char *ebuf);
int extconn_load (uint8_t type, int reset);
const struct sfm_extconn_info* extconn_get (uint8_t type);
struct sfm_extconn_info* extconn_get_rome (void);;
struct sfm_extconn_info* extconn_get_nstep (void);;
struct sfm_extconn_info* extconn_get_pgw (void);;
struct sfm_extconn_info* extconn_get_gtpp (void);;
struct sfm_extconn_info* extconn_get_radius (void);;
struct sfm_extconn_info* extconn_get_ntp (void);;
const char * extconn_type2name (uint8_t type);

int extconn_nstep_set_cur_conn (uint8_t idx, int cur_con);
int extconn_rome_set_cur_conn (uint8_t idx, int cur_con);
int extconn_nstep_get_cur_conn (uint8_t idx);
int extconn_rome_get_cur_conn (uint8_t idx);

struct ixpc_qmsg;
int extif_show (struct ixpc_qmsg *ixpc, uint8_t type);
int extif_add_rome (struct ixpc_qmsg *ixpc);
const struct sfm_extconn_info* extif_add_nstep (struct ixpc_qmsg *ixpc, uint16_t bind_port);
int extif_delete_rome (struct ixpc_qmsg *ixpc);
int extif_delete_nstep (struct ixpc_qmsg *ixpc, struct sfm_extconn_info *backup);

int extif_add_pgw (struct ixpc_qmsg *ixpc);
int extif_delete_pgw (struct ixpc_qmsg *ixpc);
int extif_add_gtpp (struct ixpc_qmsg *ixpc);
int extif_delete_gtpp (struct ixpc_qmsg *ixpc);
int extif_add_radius (struct ixpc_qmsg *ixpc);
int extif_delete_radius (struct ixpc_qmsg *ixpc);
int extif_add_ntp (struct ixpc_qmsg *ixpc);
int extif_delete_ntp (struct ixpc_qmsg *ixpc);



#endif	/* __EXTCONN_H__ */
