#ifndef __COMM_MSGID_H__
#define __COMM_MSGID_H__ 

/* mtype range : common : 1 ~99
				 OMP	: 100~199
*/
#define MTYPE_SETPRINT							1
#define MTYPE_IXPC_CONNECTION_CHECK				2
#define MTYPE_MMC_REQUEST						3
#define MTYPE_MMC_RESPONSE						4
#define MTYPE_MMC_CANCEL                		5
#define MTYPE_STATISTICS_REQUEST				6
#define MTYPE_STATISTICS_REPORT					7
#define MTYPE_STATUS_REQUEST					8
#define MTYPE_STATUS_REPORT						9
#define MTYPE_ALARM_REQUEST             		10
#define MTYPE_ALARM_REPORT						11

/* 트래이스 창에 보여주기 위한 타입	*/
#define MTYPE_TRC_CONSOLE						50	// 17 --> 50. for EMG
#define MTYPE_NOTI_REPORT                       57  // 19 --> 57 /* Status Message */
#define MTYPE_TRC_DB_INSERT                     55  // 37 --> 55 for Common Trace Type /* INSERT DB TRACE  */

#define MTYPE_FAULT_MESSAGE_REPORT              24  // ePCC : nstep fault message 2016/12/12 JKLEE

#define MTYPE_RELOAD_CONFIG_DATA              	41
#define MTYPE_DIS_CONFIG_DATA                 	42
#define MTYPE_NFV_RELOAD_CONFIG_DATA            86

#define MTYPE_KILLPRC_NOTIFICATION              92


/* msg_id 선언 부분
 * 통계 관련 		100 ~ 300 
 * msg_id range : MTYPE_STATISTICS_REQUEST      : 1 ~ 99
 *                MTYPE_STATISTICS_REPORT       : 100 ~ 199
 *                MTYPE_STATUS_REQUEST          : 200 ~ 299
 *                MTYPE_STATUS_REPORT           : 300 ~ 399
 *                MTYPE_ALARM_REQUEST           : 400 ~ 499
 *                MTYPE_ALARM_REPORT            : 500 ~ 599
 *                MTYPE_MAINTENANCE_NOTIFICATION: 600 ~ 699
 */
// 공통 통계 
#define MSGID_LOAD_STATISTICS_REPORT       		100
#define MSGID_FAULT_STATISTICS_REPORT      		101
#define MSGID_PACKET_STATISTICS_REPORT          102
#define	MSGID_TPS_STATISTICS_REPORT				103

#define MSGID_SCTP_STATISTIC_REPORT				200

#define MTYPE_TRC_SYNC_NOTI                     246

#define MSGID_SYS_COMM_STATUS_REPORT			300
#define MSGID_SYS_SPEC_ITEM_DATA                327

#endif /* __COMM_MSGID_H__ */
