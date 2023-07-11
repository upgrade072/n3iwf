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


// ȣ ó�� ���μ����� �޽����� ���� ���� overload control ���θ� �Ǵ��ϱ� ���� ȣ���Ѵ�.
// ȣ��ɶ����� tps 1�� ������Ű��, ���� �׷����� ���� tps �ջ�� ���Ͽ� ȣ ���� ���� �Ǵ��Ѵ�.
// OVLD_CTRL_SET   : �Ӱ�ġ �ʰ��� ����. �ش� �޽����� discard�ϰų� ������ ������ �������Ѵ�.
// OVLD_CTRL_CLEAR : ��� �����̳�. ���� ó���ؾ��Ѵ�.
int ovldlib_isOvldCtrl (time_t now, int mode, int svcId, char *operName)
{
    int     i, svc_tot_tps, limit_tps, found;
	T_OverloadCurrTPS	*currTPS;

    if ( svcId >= NUM_OVLD_CTRL_SVC || ovld_myAppId >= NUM_OVLD_CTRL_APPL )
        return OVLD_CTRL_CLEAR;

    // ������ ���� ���μ����� �׾� �ִ� ��� curr_svc_tps�� reset ���� �ʾƼ� �������ϰ� �ǹǷ�
    // ȣ ���� ���� �Ǵ� �� ����ð��� 2�� �̻� ���� ���� ������ ���� ó���Ѵ�.
    if ( (now - ovldInfo->sts.last_monitor_time) > 2 )
        return OVLD_CTRL_CLEAR;

	// 2019-1108 : �� �����ð��� Ȧ,¦���� ������ ¦�� �ʿ��� curr0��, Ȧ�� �ʿ��� curr1�� ����Ѵ�.
	// ȣó�� ��Ͽ��� count�ϰ� olcd ��Ͽ��� �����Ҷ� read/write �и��ϱ� ����
	if (now%2==0) currTPS = &ovldInfo->curr0;
	else          currTPS = &ovldInfo->curr1;

    // �ڽ��� index ������ ī��Ʈ�Ѵ�.
    currTPS->svc_tps[svcId][ovld_myAppId]++;

    // �ܼ��� tps�� �����ϰ�, ȣ ��� �ʿ� ���� ���
    if ( (mode & OVLDLIB_MODE_CALL_CTRL) == 0 ) {
        return OVLD_CTRL_CLEAR;
    }

	// ȣ ���� ��� operation list�� �ִ��� Ȯ���Ѵ�.
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
    // �ڽ��� ���� �׷��� �ջ� tps�� ����ؼ� �Ӱ�ġ �ʰ� ���θ� �Ǵ��Ѵ�.
    //
    for (i=svc_tot_tps=0; i<NUM_OVLD_CTRL_APPL; i++) {
        svc_tot_tps += currTPS->svc_tps[svcId][i];
    }

    // normal or overload ���¿� ���� ���ذ��� �ٸ���.
    //if (ovldInfo->sts.curr_level == OVLD_CTRL_CLEAR) {
	if ((ovldInfo->sts.curr_level == OVLD_CTRL_CLEAR) && (ovldInfo->sts.mem_curr_level == OVLD_CTRL_CLEAR)) {
        limit_tps = ovldInfo->cfg.tps_limit_normal[svcId];
    } else {
        limit_tps = ovldInfo->cfg.tps_limit_ovld[svcId];
    }

    if (svc_tot_tps > limit_tps) {
		//fprintf(stderr, "[%s DBUG] svcId=%d, svc_tot_tps = %d , limit_tps = %d\n", __func__, svcId, svc_tot_tps, limit_tps);
        currTPS->ctrl_tps[svcId][ovld_myAppId]++; // ȣ ���� ���� ī��Ʈ
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

    // �ڽ��� index ������ ī��Ʈ�Ѵ�.
    currTPS->fail_tps[svcId][ovld_myAppId]++;
	return 0;
}
