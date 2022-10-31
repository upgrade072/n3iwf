#include "olcd_proto.h"

extern int	ixpcQid;
extern char	mySysName[COMM_MAX_NAME_LEN], myAppName[COMM_MAX_NAME_LEN];
extern T_OverloadInfo	*ovldInfo;
extern char    standbySysName[COMM_MAX_NAME_LEN];

/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int initConf()
{
	char    confBuf[4096];

	memset(confBuf, 0, sizeof(confBuf));

	if (loadAppConf(confBuf) < 0) {
		fprintf(stderr, "[%s.%d] loadAppConf[%s]\n", FL, confBuf);
		return -1;
	}

	printAppConf(confBuf);

	ERRLOG(LLE, FL, "\n%s\n", confBuf);

	return 0;
}

int loadAppConf(char *resBuf)
{
	int     level;
	char    token[64];
	char    fname[128];

	memset(fname,  0, sizeof(fname));

	sprintf(fname, "%s/%s/%s.dat", homeEnv, APPCONF_DIR, myAppName);

	if(conflib_getNthTokenInFileSection(fname, "[GENERAL]", "ERR_LOG_LEVEL", 1, token) < 0) {
		sprintf(resBuf, "ERR_LOG_LEVEL FIELD NOT FOUND IN %s", fname);
		return -1;
	}

	level = atoi(token);
	loglib_setLogLevel(ELI, level);

	/* OVERLOAD LOG */
	loglib_setLogLevel(ovldMonLog, LLE);

	if(conflib_getNthTokenInFileSection(fname, "[GENERAL]", "STANDBY_SYS_NAME", 1, token) < 0) {
		sprintf(resBuf, "ERR_LOG_LEVEL FIELD NOT FOUND IN %s", fname);
		return -1;
	}
	strcpy(standbySysName, token);


	return 0;
}

void printAppConf(char *resBuf)
{
	char  *p=resBuf;

	/* log level */
	p += sprintf(p, "  LOG_LEVEL        :\n");
	p += sprintf(p, "    %-20s = [%d]\n", "ERR_LOG_LEVEL", loglib_getLogLevel(ELI));
	p += sprintf(p, "\n");

	return;
}




int olcd_loadOlvdCfg (void)
{
    char    fname[256], getBuf[1024], token[16][CONFLIB_MAX_TOKEN_LEN], *pList, *ptr, *next;
    int     lNum, svcCnt, svcIdx, operIdx;
	FILE	*fp;

#if 0 /* edited by hangkuk at 2020.05.12 */
    if ((env = getenv(IV_HOME)) == NULL) {
        fprintf(stderr, "[%s] not found %s environment name\n", __func__, IV_HOME);
        return -1;
    }
    sprintf (fname, "%s/%s", env, OVLD_CONF_FILE);
#else
    sprintf (fname, "%s/%s", homeEnv, OVLD_CONF_FILE);
#endif 

    if (conflib_getNTokenInFileSection (fname, "GENERAL", "MON_PERIOD", 1, token) < 0) {
        fprintf(stderr,"[%s] not found MON_PERIOD in file[%s]\n", __func__, fname);
        return -1;
	}
    ovldInfo->cfg.mon_period = strtol (token[0],0,0);

    if (conflib_getNTokenInFileSection (fname, "GENERAL", "CPU_DURATION", 1, token) < 0) {
        fprintf(stderr,"[%s] not found CPU_DURATION in file[%s]\n", __func__, fname);
        return -1;
	}
    ovldInfo->cfg.cpu_duration = strtol (token[0],0,0);

    if (conflib_getNTokenInFileSection (fname, "GENERAL", "CPU_LIMIT", 2, token) < 0) {
        fprintf(stderr,"[%s] not found CPU_LIMIT in file[%s]\n", __func__, fname);
        return -1;
	}
    ovldInfo->cfg.cpu_limit_occur = strtol (token[0],0,0);
    ovldInfo->cfg.cpu_limit_clear = strtol (token[1],0,0);

	if (conflib_getNTokenInFileSection (fname, "GENERAL", "MEM_DURATION", 1, token) < 0) {
		fprintf(stderr,"[%s] not found MEM_DURATION in file[%s]\n", __func__, fname);
		return -1;
	}
	ovldInfo->cfg.mem_duration = strtol (token[0],0,0);

	if (conflib_getNTokenInFileSection (fname, "GENERAL", "MEM_LIMIT", 2, token) < 0) {
		fprintf(stderr,"[%s] not found MEM_LIMIT in file[%s]\n", __func__, fname);
		return -1;
	}
	ovldInfo->cfg.mem_limit_occur = strtol (token[0],0,0);
	ovldInfo->cfg.mem_limit_clear = strtol (token[1],0,0);

	//--------------------------------------------------------------------

    if ((fp = fopen(fname,"r")) == NULL) {
        fprintf(stderr,"[%s] fopen fail[%s]; err=%d(%s)\n", __func__, fname, errno, strerror(errno));
        return -1;
    }
	svcIdx = 0;
    if ((lNum = conflib_seekSection (fp, "TPS_LIMIT")) < 0) {
        fprintf(stderr,"[%s] not found TPS_LIMIT section in file[%s]\n", __func__, fname);
        return -1;
    }
    while ( (fgets(getBuf, sizeof(getBuf), fp) != NULL) && (svcIdx < NUM_OVLD_CTRL_SVC) ) {
        lNum++;
        if (getBuf[0] == '[') break;
        if (getBuf[0]=='#' || getBuf[0]=='\n') continue;
        if (sscanf(getBuf,"%s%s%s%s",token[0],token[1],token[2],token[3]) < 4) {
            fprintf(stderr,"[%s] syntax error; file=%s, lNum=%d\n", __func__, fname, lNum);
            return -1;
        }
        strcpy (ovldInfo->cfg.svc_name[svcIdx], token[0]);
        ovldInfo->cfg.tps_limit_normal[svcIdx] = strtol (token[2],0,0);
		ovldInfo->cfg.tps_limit_ovld[svcIdx]   = strtol (token[3],0,0);
		svcIdx++;
	}

	fclose(fp);
	svcCnt = svcIdx;
	if ( svcIdx < NUM_OVLD_CTRL_SVC ) {
		strcpy (ovldInfo->cfg.svc_name[svcIdx], "");
	}
	//--------------------------------------------------------------------

    for (svcIdx=0; svcIdx < svcCnt; svcIdx++) {
        if (conflib_getStringInFileSection (fname, "CTRL_OPER_LIST", ovldInfo->cfg.svc_name[svcIdx], getBuf) < 0) {
			fprintf(stderr,"[%s] not found CTRL_OPER_LIST (%s) in file[%s]\n", __func__, ovldInfo->cfg.svc_name[svcIdx], fname);
			return -1;
		}
		pList = getBuf;
		ptr = strtok_r (pList, " ", &next);
		operIdx = 0;
		while (ptr != NULL) {
			if (ptr[0] == '#') break;
			strcpy (ovldInfo->cfg.ctrl_oper_list[svcIdx][operIdx], ptr);
			operIdx++;
			pList = next;
			ptr = strtok_r (pList, " ", &next);
		}
		strcpy (ovldInfo->cfg.ctrl_oper_list[svcIdx][operIdx], ""); // 맨뒤에 NULL 표시
    }

    return 1;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_bkupOlvdCfg (void)
{
    char    fname[256];
    int     i, j;
    FILE    *fp;

    sprintf (fname, "%s/%s", homeEnv, OVLD_CONF_FILE);
    if ((fp = fopen (fname, "w")) == NULL) {
        fprintf(stderr,"[%s] fopen fail[%s]; errno=%d(%s)\n", __func__, fname, errno, strerror(errno));
        return -1;
    }

    fprintf (fp, "[GENERAL]\n");
    fprintf (fp, "%-12s = %-7d # seconds. Avarage TPS 산출 및 Alarm,Status 메시지 출력 주기.\n", 
                            "MON_PERIOD", ovldInfo->cfg.mon_period);
    fprintf (fp, "%-12s = %-7d # seconds. CPU 과부하 발생/해제 지속 시간 조건 값.\n", 
                            "CPU_DURATION", ovldInfo->cfg.cpu_duration);
    fprintf (fp, "%-12s = %-3d %-3d # percentage. CPU 과부하 발생 조건 값. 과부하 발생 후 clear될 조건 값.\n", 
                            "CPU_LIMIT", ovldInfo->cfg.cpu_limit_occur, ovldInfo->cfg.cpu_limit_clear);
	fprintf (fp, "%-12s = %-7d # seconds. MEM 과부하 발생/해제 지속 시간 조건 값.\n",
			"MEM_DURATION", ovldInfo->cfg.mem_duration);
	fprintf (fp, "%-12s = %-3d %-3d # percentage. MEM 과부하 발생 조건 값. 과부하 발생 후 clear될 조건 값.\n",
			"MEM_LIMIT", ovldInfo->cfg.mem_limit_occur, ovldInfo->cfg.mem_limit_clear);

    fprintf (fp, "\n[TPS_LIMIT]\n");
    fprintf (fp, "# normal 상태에서의 허용 TPS 설정 값. overload 상태에서의 허용 TPS 설정 값.\n");
    for (i=0; i < NUM_OVLD_CTRL_SVC; i++) {
		if ( !strcmp (ovldInfo->cfg.svc_name[i], "") )
			break;
        fprintf (fp, "%-8s = %5d %5d\n", ovldInfo->cfg.svc_name[i], ovldInfo->cfg.tps_limit_normal[i], ovldInfo->cfg.tps_limit_ovld[i]);
    }
    fprintf (fp, "\n[CTRL_OPER_LIST]\n");
    fprintf (fp, "# Operation별 호 제한 대상 여부 설정 : 최대 %d개까지 등록할 수 있다.\n", NUM_OVLD_CTRL_OPER);
    for (i=0; i < NUM_OVLD_CTRL_SVC; i++) {
		if ( !strcmp (ovldInfo->cfg.svc_name[i], "") )
			break;
        fprintf (fp, "%-8s =", ovldInfo->cfg.svc_name[i]);
        for (j=0; j < NUM_OVLD_CTRL_OPER; j++) {
            if ( !strcmp (ovldInfo->cfg.ctrl_oper_list[i][j], "") )
				break;
            fprintf (fp, " %s", ovldInfo->cfg.ctrl_oper_list[i][j]);
        }
        fprintf (fp, "  #\n");
    }

    fclose (fp);
    return 1;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_hdlMMCReqMsg (GeneralQMsgType *rxQMsg)
{
	IxpcQMsgType    *rxIxpcMsg = (IxpcQMsgType *)rxQMsg->body;
	MMLReqMsgType   *mmlReqMsg = (MMLReqMsgType *)rxIxpcMsg->body;

	ERRLOG(LL3, FL, "[%s] recv MMC REQUEST; cmdName=(%s)", __func__, mmlReqMsg->head.cmdName);

	if (!strcasecmp (mmlReqMsg->head.cmdName, "dis-ovld-info")) {
		olcd_dis_ovld_info (rxIxpcMsg);
	}
	else if (!strcasecmp (mmlReqMsg->head.cmdName, "chg-ovld-cfg")) {
		olcd_chg_ovld_cfg (rxIxpcMsg);
	}
	else if (!strcasecmp (mmlReqMsg->head.cmdName, "reload-ovld-cfg")) {
		olcd_reload_ovld_cfg (rxIxpcMsg);
	}
	else {
		ERRLOG(LLE, FL, "[%s] >>> recv unknown MMC REQUEST; cmdName=(%s)", __func__, mmlReqMsg->head.cmdName);
	}

	return 1;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_dis_ovld_info (IxpcQMsgType *rxIxpcMsg)
{
	MMLReqMsgHeadType   *mmlReqMsg = (MMLReqMsgHeadType *)rxIxpcMsg->body;
    int     len=0, i,j, total_tps=0;
	char	resBuf[8192], arg_type[32];

    strcpy (arg_type, "ALL"); // default
    for (i=0; i < mmlReqMsg->paraCnt; i++) {
        if ( !strcasecmp (mmlReqMsg->para[i].paraName, "TYPE") )
            strcpy (arg_type, mmlReqMsg->para[i].paraVal);
    }

	len = sprintf (resBuf, "\n  RESULT = SUCCESS  \n  SYSTEM = %s\n\n", mySysName);

    if ( !strcasecmp (arg_type, "ALL") || !strcasecmp (arg_type, "LIMIT") ) {
        len += sprintf (&resBuf[len], "  %s = [%s/%s]\n", "Configuration File", getenv(IV_HOME), OVLD_CONF_FILE);
        len += sprintf (&resBuf[len], "  -----------------------------------------------------------------------\n");
        len += sprintf (&resBuf[len], "  %-12s = %d seconds  [%s]\n", "MON_PERIOD", ovldInfo->cfg.mon_period,
                "Avarage TPS 산출 및 Alarm,Status 메시지 출력 주기");
        len += sprintf (&resBuf[len], "  %-12s = %d seconds  [%s]\n", "CPU_DURATION", ovldInfo->cfg.cpu_duration,
                "CPU 과부하 발생/해제 지속시간 조건");
		len += sprintf (&resBuf[len], "  %-12s = %d%% %d%%  [%s]\n", "CPU_LIMIT", ovldInfo->cfg.cpu_limit_occur, ovldInfo->cfg.cpu_limit_clear,
				"CPU 과부하 발생/해제 임계치");
		len += sprintf (&resBuf[len], "  %-12s   %d%% 이상 %d초 이상 지속되면 과부하 발생하고, 과부하 상태에서 %d%% 이하로 %d초 지속되어야 해제된다.\n",
				"", ovldInfo->cfg.cpu_limit_occur, ovldInfo->cfg.cpu_duration, ovldInfo->cfg.cpu_limit_clear, ovldInfo->cfg.cpu_duration);

		len += sprintf (&resBuf[len], "  %-12s = %d seconds  [%s]\n", "MEM_DURATION", ovldInfo->cfg.mem_duration,
				"MEM 과부하 발생/해제 지속시간 조건");
		len += sprintf (&resBuf[len], "  %-12s = %d%% %d%%  [%s]\n", "MEM_LIMIT", ovldInfo->cfg.mem_limit_occur, ovldInfo->cfg.mem_limit_clear,
				"MEM 과부하 발생/해제 임계치");
		len += sprintf (&resBuf[len], "  %-12s   %d%% 이상 %d초 이상 지속되면 과부하 발생하고, 과부하 상태에서 %d%% 이하로 %d초 지속되어야 해제된다.\n",
				"", ovldInfo->cfg.mem_limit_occur, ovldInfo->cfg.mem_duration, ovldInfo->cfg.mem_limit_clear, ovldInfo->cfg.mem_duration);

        len += sprintf (&resBuf[len], "  %-12s : %8s %8s  [%s]\n", "TPS_LIMIT", "Normal", "Overload",
                "normal 상태에서의 허용 TPS 설정 값. overload 상태에서의 허용 TPS 설정 값.");
        for (i=0; i<NUM_OVLD_CTRL_SVC; i++) {
            if (!strcmp (ovldInfo->cfg.svc_name[i], ""))
				break;
            len += sprintf (&resBuf[len], "    %-15s = %8d %8d\n", ovldInfo->cfg.svc_name[i], ovldInfo->cfg.tps_limit_normal[i], ovldInfo->cfg.tps_limit_ovld[i]);
        }
        len += sprintf (&resBuf[len], "  -----------------------------------------------------------------------\n");
    }

	if(!strcasecmp (arg_type, "ALL")){
		len += sprintf (&resBuf[len], "  CTRL_OPER_LIST : Operation별 호 제한 대상 여부 설정 : 최대 %d개까지 등록할 수 있다.\n", NUM_OVLD_CTRL_OPER);
		for (i=0; i < NUM_OVLD_CTRL_SVC; i++) {
			if ( !strcmp (ovldInfo->cfg.svc_name[i], "") )
				break;
			len += sprintf (&resBuf[len], "    %-15s =", ovldInfo->cfg.svc_name[i]);
			for (j=0; j < NUM_OVLD_CTRL_OPER; j++) {
				if ( !strcmp (ovldInfo->cfg.ctrl_oper_list[i][j], "") ){
					break;
				}
				len += sprintf (&resBuf[len], " %s", ovldInfo->cfg.ctrl_oper_list[i][j]);
			}
		}
	}

    if ( !strcasecmp (arg_type, "ALL") || !strcasecmp (arg_type, "CURR_TPS") ) {
    	len += sprintf (&resBuf[len], "  -----------------------------------------------------------------------\n");
	    // CPU
        len += sprintf (&resBuf[len], "  %-12s = %s\n", "CPU_Status",  ovldInfo->sts.curr_level==OVLD_CTRL_CLEAR ? "Normal" : "Overload");
        if (ovldInfo->sts.last_occur_time)
            len += sprintf (&resBuf[len], "  %-12s = %s\n", "Last_Occur",  commlib_printDateTime(ovldInfo->sts.last_occur_time));
        if (ovldInfo->sts.last_clear_time)
            len += sprintf (&resBuf[len], "  %-12s = %s\n", "Last_Clear",  commlib_printDateTime(ovldInfo->sts.last_clear_time));

		//MEM
		len += sprintf (&resBuf[len], "  %-12s = %s\n", "MEM_Status",  ovldInfo->sts.mem_curr_level==OVLD_CTRL_CLEAR ? "Normal" : "Overload");
		if (ovldInfo->sts.mem_last_occur_time)
			len += sprintf (&resBuf[len], "  %-12s = %s\n", "Last_Occur",  commlib_printDateTime(ovldInfo->sts.mem_last_occur_time));
		if (ovldInfo->sts.mem_last_clear_time)
			len += sprintf (&resBuf[len], "  %-12s = %s\n", "Last_Clear",  commlib_printDateTime(ovldInfo->sts.mem_last_clear_time));

        len += sprintf (&resBuf[len], "  %-12s :\n", "Current_TPS");
        for (i=total_tps=0; i<NUM_OVLD_CTRL_SVC; i++) {
            if (!strcmp (ovldInfo->cfg.svc_name[i], ""))
				break;
            len += sprintf (&resBuf[len], "    %-15s = %5d\n", ovldInfo->cfg.svc_name[i], ovldInfo->sts.last_svc_tps[i]);
            total_tps += ovldInfo->sts.last_svc_tps[i];
        }
        len += sprintf (&resBuf[len], "    %-15s = %5d\n", "Total",  total_tps);
        len += sprintf (&resBuf[len], "  -----------------------------------------------------------------------\n");
    }

    return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_SUCCESS);
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_chg_ovld_cfg (IxpcQMsgType *rxIxpcMsg)
{
#if 1
    int     len, i, j, old_val, new_val, svc_id;
    char    resBuf[8192], arg_type[32];
    MMLReqMsgHeadType   *mmlReqMsg = (MMLReqMsgHeadType *)rxIxpcMsg->body;

	len = sprintf (resBuf, "\n  RESULT = FAIL  \n  SYSTEM = %s\n\n", mySysName);

    for (i=0; i < mmlReqMsg->paraCnt; i++) {
        for (j=0; j < strlen(mmlReqMsg->para[i].paraVal); j++)
            mmlReqMsg->para[i].paraVal[j] = toupper (mmlReqMsg->para[i].paraVal[j]);
        if ( !strcasecmp (mmlReqMsg->para[i].paraName, "TYPE") )
            strcpy (arg_type, mmlReqMsg->para[i].paraVal);
        else if ( !strcasecmp (mmlReqMsg->para[i].paraName, "VAL") )
            new_val = strtol (mmlReqMsg->para[i].paraVal,0,0);
    }

    if (!strncasecmp (arg_type, "TPS_", 4))
    {
        if (strstr (arg_type, "NGAP"))					svc_id = OVLD_CTRL_SVC_NGAP;
        else if (strstr (arg_type, "TCP"))				svc_id = OVLD_CTRL_SVC_TCP;
        else if (strstr (arg_type, "EAP"))				svc_id = OVLD_CTRL_SVC_EAP;
        else {
            len += sprintf (&resBuf[len], "  REASON = UNKNOWN TYPE\n");
            return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
        }
    }

	if ( !strcasecmp (arg_type, "MON_PERIOD") ) {
        old_val = ovldInfo->cfg.mon_period;
		ovldInfo->cfg.mon_period = new_val;
	} else if ( !strcasecmp (arg_type, "CPU_DURATION") ) {
        old_val = ovldInfo->cfg.cpu_duration;
        ovldInfo->cfg.cpu_duration = new_val;
    }
    else if ( !strcasecmp (arg_type, "CPU_OCCUR") ) {
        if ( new_val < ovldInfo->cfg.cpu_limit_clear ) {
            len += sprintf (&resBuf[len], "  REASON = CPU_OCCUR must be bigger than CPU_CLEAR\n");
    		return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
        }
        if ( new_val > 100 ) {
            len += sprintf (&resBuf[len], "  REASON = CPU_LIMIT must be less than 100%%\n");
    		return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
        }
        old_val = ovldInfo->cfg.cpu_limit_occur;
        ovldInfo->cfg.cpu_limit_occur = new_val;
    }
    else if ( !strcasecmp (arg_type, "CPU_CLEAR") ) {
        if ( new_val > ovldInfo->cfg.cpu_limit_occur ) {
            len += sprintf (&resBuf[len], "  REASON = CPU_CLEAR must be less than CPU_OCCUR\n");
    		return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
        }
        if ( new_val > 100 ) {
            len += sprintf (&resBuf[len], "  REASON = CPU_LIMIT must be less than 100%%\n");
    		return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
        }
        old_val = ovldInfo->cfg.cpu_limit_clear;
        ovldInfo->cfg.cpu_limit_clear = new_val;
	} else if ( !strcasecmp (arg_type, "MEM_DURATION") ) {
		old_val = ovldInfo->cfg.mem_duration;
		ovldInfo->cfg.mem_duration = new_val;
	}
	else if ( !strcasecmp (arg_type, "MEM_OCCUR") ) {
		if ( new_val < ovldInfo->cfg.mem_limit_clear ) {
			len += sprintf (&resBuf[len], "  REASON = MEM_OCCUR must be bigger than MEM_CLEAR\n");
			return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
		}
		if ( new_val > 100 ) {
			len += sprintf (&resBuf[len], "  REASON = MEM_LIMIT must be less than 100%%\n");
			return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
		}
		old_val = ovldInfo->cfg.mem_limit_occur;
		ovldInfo->cfg.mem_limit_occur = new_val;
	}
	else if ( !strcasecmp (arg_type, "MEM_CLEAR") ) {
		if ( new_val > ovldInfo->cfg.mem_limit_occur ) {
			len += sprintf (&resBuf[len], "  REASON = MEM must be less than MEM_OCCUR\n");
			return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
		}
		if ( new_val > 100 ) {
			len += sprintf (&resBuf[len], "  REASON = MEM_LIMIT must be less than 100%%\n");
			return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
		}
		old_val = ovldInfo->cfg.mem_limit_clear;
		ovldInfo->cfg.mem_limit_clear = new_val;
	}
	else {
        if ( strstr (arg_type, "TPS_NORM_") ) {
            if ( new_val < ovldInfo->cfg.tps_limit_ovld[svc_id] ) {
                len += sprintf (&resBuf[len], "  REASON = TPS_NORM must be bigger than TPS_OVLD\n");
    			return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
            }
            old_val = ovldInfo->cfg.tps_limit_normal[svc_id];
            ovldInfo->cfg.tps_limit_normal[svc_id] = new_val;
        } else if ( strstr (arg_type, "TPS_OVLD_") ) {
            if ( new_val > ovldInfo->cfg.tps_limit_normal[svc_id] ) {
                len += sprintf (&resBuf[len], "  REASON = TPS_OVLD must be less than TPS_NORM\n");
    			return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_FAIL);
            }
            old_val = ovldInfo->cfg.tps_limit_ovld[svc_id];
            ovldInfo->cfg.tps_limit_ovld[svc_id] = new_val;
        }
    }

	len = sprintf (resBuf, "\n  RESULT = SUCCESS  \n  SYSTEM = %s\n\n", mySysName);

    len += sprintf (&resBuf[len], "  %-6s = %s\n", "TYPE", arg_type);
    len += sprintf (&resBuf[len], "  %-6s = %d\n", "PREV", old_val);
    len += sprintf (&resBuf[len], "  %-6s = %d\n", "NEW",  new_val);

	olcd_bkupOlvdCfg ();

    return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_SUCCESS);
#endif
	return 1;
}


/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_reload_ovld_cfg (IxpcQMsgType *rxIxpcMsg)
{
	char	resBuf[8192];

    olcd_loadOlvdCfg ();

	sprintf (resBuf, "\n  RESULT = SUCCESS  \n  SYSTEM = %s\n\n", mySysName);

    return olcd_sendMMCResponse (rxIxpcMsg, resBuf, RES_SUCCESS);
}



/*------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
int olcd_sendMMCResponse (IxpcQMsgType *rxIxpcMsg, char *resBuf, int resCode)
{
	GeneralQMsgType  txGenQMsg;
	IxpcQMsgType     *txIxpcMsg;
	MMLResMsgType    *txResMsg;
	MMLReqMsgType    *mmlReqMsg;
	int              txLen;

	txIxpcMsg = (IxpcQMsgType *)txGenQMsg.body;
	txResMsg  = (MMLResMsgType *)txIxpcMsg->body;
	mmlReqMsg = (MMLReqMsgType *)rxIxpcMsg->body;

	memset (txIxpcMsg, 0, IXPC_HEAD_LEN);
	memset (txResMsg, 0x00, sizeof(MMLResMsgType));

	txGenQMsg.mtype = MTYPE_MMC_RESPONSE; 

	txIxpcMsg->head.msgId = rxIxpcMsg->head.msgId;
	strcpy (txIxpcMsg->head.srcSysName, mySysName);
	strcpy (txIxpcMsg->head.srcAppName, myAppName);
	strcpy (txIxpcMsg->head.dstSysName, rxIxpcMsg->head.srcSysName);
	strcpy (txIxpcMsg->head.dstAppName, rxIxpcMsg->head.srcAppName);

    txResMsg->head.mmcdJobNo    = mmlReqMsg->head.mmcdJobNo;
    txResMsg->head.resCode      = resCode;
    strcpy(txResMsg->head.cmdName, mmlReqMsg->head.cmdName);
    strcpy(txResMsg->body, resBuf);

    txIxpcMsg->head.bodyLen = sizeof(txResMsg->head) + strlen(txResMsg->body);
    txLen = sizeof(txIxpcMsg->head) + txIxpcMsg->head.bodyLen;

    if (msgsnd (ixpcQid, (void *)&txGenQMsg, txLen, IPC_NOWAIT) < 0) {
		ERRLOG(LLE, FL, "[%s] >>> msgsnd() fail; cmdName=(%s)", __func__, mmlReqMsg->head.cmdName);
        return -1;
    }
    return 1;
}
