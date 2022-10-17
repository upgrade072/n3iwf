#ifndef __COMM_MSGID_H__
#define __COMM_MSGID_H__ 

/* mtype range : common : 1 ~99
				 OMP	: 100~199
				 NSCP	: 200~299	
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


#define MTYPE_RELOAD_EXTCONN_SHM                100
#define MTYPE_RECONF_SOC_NOTI					101
#define MTYPE_RECONF_TRACE_NOTI					102
#define MTYPE_RESERVED_END                      130

#define MTYPE_NSTEP_REQUEST						150	
#define MTYPE_NSTEP_RESPONSE					151

#define MTYPE_POOL_REQUEST						210
#define MTYPE_POOL_RESPONSE						211


#define MTYPE_RECONF_SOC_ROUTE_NOTI				230 
#define MTYPE_NSTEP_MSG_RELAY					231 

#define	MTYPE_ONBACK_REQUEST					240
#define	MTYPE_ONBACK_RESPONSE					241

#define	MTYPE_GTP_INFO							245

#define MTYPE_TRC_SYNC_NOTI                     246

/* NSCP */
#define MTYPE_NSCP_CHECKSTATUS_REQUEST          247
#define MTYPE_NSCP_SETUP_REQUEST                248

/* NSCPMON ==> NSCP */
#define MTYPE_NSCP_SETUP_REQUEST_RE             249


#define MTYPE_APP_OFCSIF_REQ					300
#define MTYPE_RCDR_OFCSIF_REQ					301

/* mtype */
#define SESS_DISCONNECT_REQ   9000 /* EMGC ==> EMGU */
#define SESS_DISCONNECT_RES   9001 /* EMGU ==> EMGC */
#define NAS_DISCONNECTED_NOTI 9002  /* EMGU ==> EMGC */


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

// SPR 통계 : Start ID 200  
#define MSGID_ROME_STATISTICS_REPORT            200
#define MSGID_NSTEP_STATISTICS_REPORT           201
#define MSGID_RADIUS_STATISTICS_REPORT          202
#define MSGID_GTPP_STATISTICS_REPORT            203
#define MSGID_MG_STATISTICS_REPORT            	204


#define MSGID_SYS_COMM_STATUS_REPORT			300
#define	MSGID_SYS_SPEC_CONN_STATUS_REPORT		301
#define MSGID_SYS_SPEC_HW_STATUS_REPORT    		302
#define MSGID_SYS_SPEC_SPDB_STATUS_REPORT  		303
#define MSGID_SYS_SPEC_TPS_STATUS_REPORT  		304
#define	MSGID_WATCHDOG_STATUS_REPORT			310

#define MSGID_SYS_SPEC_TCP_CONN_STATUS_REPORT	320

#define MSGID_SYS_SPEC_ENDP_STATUS_REPORT       306
#define MSGID_SYS_SPEC_PEER_STATUS_REPORT       307
#define MSGID_SYS_SPEC_PEER_ASSOC_REPORT        308

#define MSGID_SYS_SPEC_SESSION_INFO_REPORT      324
#define MSGID_SYS_SPEC_TPS_INFO_REPORT          325 // 2016.07.11
#define MSGID_SYS_SPEC_SUCC_RATE_REPORT         326 // 2016.07.11
#define MSGID_SYS_SPEC_ITEM_DATA                327

#define MSGID_PNR_MMC_JOB_REPORT				330
#define MSGID_PLISM_IPCHG_REQ_REPORT			331
#define MSGID_MZN_CDR_RESEND_REQ_REPORT			332

#define MSGID_SCTP_STATISTICS_REPORT            603
#define MSGID_DIAMETER_STATISTICS_REPORT        104
#define MSGID_DIAM_STATISTICS_REPORT            105 // 2016.07.14 for SHIF, actually is used the value in CONFIG/STMD.dat
#define MSGID_PCC_STATISTICS_REPORT				106

/* RADIUS ACESSS Statistics. add by hangkuk at 2020.05.08 */
#define MSGID_RADIUS_ACCESS_OP_STATISTICS_REPORT	210
#define MSGID_RADIUS_ACCESS_HOST_STATISTICS_REPORT	220

/* FTPIB Statistics. bhj add 2020.05.27 */
#define MSGID_FTPIB_STATISTICS_REPORT	221

/* RADIUS Statistics. schlee 20200527 */
#define MSGID_RADIUS_STATISTIC_REPORT		230
#define MSGID_NAS_START_STATISTIC_REPORT	231
#define MSGID_ACCESS_STATISTIC_REPORT		232		// for access request stat 20200706
    
/* rest */
#define MSGID_REST_NAS_START_STATISTIC_REPORT	240 /* 231 */
#define MSGID_REST_API_STATISTIC_REPORT	241

/* SUBSCRIBER  Statistics */
#define MSGID_SUBSCRIBER_STATISTICS_REPORT	242

/* for emgu management, process internal noti */
#define MSGID_NOTIFICATION_EMGU_MGNT		600
#define MSGID_EMGU_SDEVEND_MGNT				601
#define MSGID_EMGU_TBL_NOTI_MGNT			602
#define MSGID_EMGU_CHG_ENC_MGNT				603
#define MSGID_EMGU_PERMIT_SND_DEV_MGNT		611 // ?

/* BCSP send */
#define MSGID_SOC_POLICY_CREATE_MGNT        604
#define MSGID_SOC_POLICY_MODIFY_MGNT        605
#define MSGID_IPPOOL_CREATE_MGNT            606
#define MSGID_IPPOOL_DELETE_MGNT            607

/* 2-Factor */
#define MSGID_INITIAL_2FA					608 // Radiusd -> AUTHIF, Check 2FA
#define MSGID_REQUEST_2FA					609 // AUTHIF -> PROVIB, OTP request
#define MSGID_RESPONSE_2FA					610 // PROVIB -> AUTHIF, OTP response


#endif /* __COMM_MSGID_H__ */
