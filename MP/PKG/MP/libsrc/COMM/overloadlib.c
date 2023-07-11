#include <stdio.h>
#include "overloadlib.h"

T_OverloadInfo      *ovldInfo;
int	ovld_myAppId;


int ovldlib_init (char *myAppName)
{
	FILE	*fp;
	char	fname[256], getBuf[256], token[32];

    if (ovldlib_attach_shm () < 0)
        return -1;

    sprintf (fname, "%s/%s", getenv(IV_HOME), SYSCONF_FILE);
    fp = fopen(fname,"r");
    conflib_seekSection (fp, "APPLICATIONS");
    while (fgets(getBuf,sizeof(getBuf),fp) != NULL) {
        if (getBuf[0] == '[') break;
        if (getBuf[0]=='#' || getBuf[0]=='\n') continue;
        sscanf (getBuf,"%s",token);
        if (!strcasecmp(token,myAppName))
            break;
        ovld_myAppId++;
    }
    fclose(fp);

	return 1;
}

int ovldlib_attach_shm (void)
{
    char    *env, fname[256], tmp[32];
    int     key, id;

    if ((env = getenv(IV_HOME)) == NULL) {
        fprintf(stderr, "[%s] not found %s environment name\n", __func__, IV_HOME);
        return -1;
    }
    sprintf (fname, "%s/%s", env, SYSCONF_FILE);

    if (conflib_getNthTokenInFileSection (fname, "SHARED_MEMORY_KEY", "SHM_OVERLOAD_INFO", 1, tmp) < 0) {
        fprintf (stderr,"[%s] not found SHM_OVERLOAD_INFO in %s\n",__func__, fname);
        exit(-1);
    }
    key = (int)strtol(tmp,0,0);
    if ((id = (int)shmget (key, sizeof(T_OverloadInfo), 0644|IPC_CREAT)) < 0) {
        if (errno != ENOENT) {
            fprintf (stderr,"[%s] shmget fail; key=0x%x, err=%d(%s)\n",__func__, key, errno, strerror(errno));
            exit(-1);
        }
    }
    ovldInfo = (T_OverloadInfo *) shmat(id,0,0);

    return 1;
}


// 호 처리 프로세스가 메시지를 받은 직후 overload control 여부를 판단하기 위해 호출한다.
// 호출될때마다 tps 1씩 증가시키고, 같은 그룹으로 묶인 tps 합산과 비교하여 호 제어 여부 판단한다.
// OVLD_CTRL_SET   : 임계치 초과된 상태. 해당 메시지를 discard하거나 적절한 에러를 보내야한다.
// OVLD_CTRL_CLEAR : 허용 범위이내. 정상 처리해야한다.
int ovldlib_isOvldCtrl (time_t now, int mode, int svcId, char *operName)
{
    int     i, svc_tot_tps, limit_tps, found;
	T_OverloadCurrTPS	*currTPS;

    if ( svcId >= NUM_OVLD_CTRL_SVC || ovld_myAppId >= NUM_OVLD_CTRL_APPL )
        return OVLD_CTRL_CLEAR;

    // 과부하 감시 프로세스가 죽어 있는 경우 curr_svc_tps가 reset 되지 않아서 오동작하게 되므로
    // 호 제어 여부 판단 시 현재시간과 2초 이상 차이 나면 무조건 정상 처리한다.
    if ( (now - ovldInfo->sts.last_monitor_time) > 2 )
        return OVLD_CTRL_CLEAR;

	// 2019-1108 : 초 단위시간을 홀,짝수로 나누어 짝수 초에는 curr0에, 홀수 초에는 curr1에 기록한다.
	// 호처리 블록에서 count하고 olcd 블록에서 수집할때 read/write 분리하기 위해
	if (now%2==0) currTPS = &ovldInfo->curr0;
	else          currTPS = &ovldInfo->curr1;

    // 자신의 index 영역에 카운트한다.
    currTPS->svc_tps[svcId][ovld_myAppId]++;

    // 단순히 tps만 누적하고, 호 제어가 필요 없는 경우
    if ( (mode & OVLDLIB_MODE_CALL_CTRL) == 0 ) {
        return OVLD_CTRL_CLEAR;
    }

	// 호 제어 대상 operation list에 있는지 확인한다.
	//
	if (operName != NULL) {
		for (i=0, found=0; i<NUM_OVLD_CTRL_OPER; i++) {
			if ( !strcasecmp(ovldInfo->cfg.ctrl_oper_list[svcId][i], "") )
				break;

			if ( !strcasecmp(ovldInfo->cfg.ctrl_oper_list[svcId][i], operName) ) {
				found = 1;
				break;
			}
		}
		if (found == 0) {
			return OVLD_CTRL_CLEAR;
		}
	}

    //
    // 자신의 서비스 그룹의 합산 tps를 계산해서 임계치 초과 여부를 판단한다.
    //
    for (i=svc_tot_tps=0; i<NUM_OVLD_CTRL_APPL; i++) {
        svc_tot_tps += currTPS->svc_tps[svcId][i];
    }

    // normal or overload 상태에 따라 기준값이 다르다.
    //if (ovldInfo->sts.curr_level == OVLD_CTRL_CLEAR) {
	if ((ovldInfo->sts.curr_level == OVLD_CTRL_CLEAR) && (ovldInfo->sts.mem_curr_level == OVLD_CTRL_CLEAR)) {
        limit_tps = ovldInfo->cfg.tps_limit_normal[svcId];
    } else {
        limit_tps = ovldInfo->cfg.tps_limit_ovld[svcId];
    }

    if (svc_tot_tps > limit_tps) {
		//fprintf(stderr, "[%s DBUG] svcId=%d, svc_tot_tps = %d , limit_tps = %d\n", __func__, svcId, svc_tot_tps, limit_tps);
        currTPS->ctrl_tps[svcId][ovld_myAppId]++; // 호 제한 개수 카운트
        return OVLD_CTRL_SET;
    } else {
        return OVLD_CTRL_CLEAR;
    }
}

int ovldlib_increaseFailCnt (int svcId)
{
	T_OverloadCurrTPS	*currTPS;
	time_t	now=time(0);

    if ( svcId >= NUM_OVLD_CTRL_SVC || ovld_myAppId >= NUM_OVLD_CTRL_APPL )
        return -1;

	if (now%2==0) currTPS = &ovldInfo->curr0;
	else          currTPS = &ovldInfo->curr1;

    // 자신의 index 영역에 카운트한다.
    currTPS->fail_tps[svcId][ovld_myAppId]++;
	return 0;
}
