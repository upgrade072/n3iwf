#include <nwucp.h>

extern main_ctx_t *MAIN_CTX;

int NWUCP_OVLD_CHECK(int svc_id, char *oper_str)
{
	int ret = 0;

	struct timeval curr_time; 
	gettimeofday(&curr_time, NULL);

	pthread_mutex_lock(&MAIN_CTX->mutex_for_ovldctrl);

	ret = ovldlib_isOvldCtrl(curr_time.tv_sec, OVLDLIB_MODE_CALL_CTRL, svc_id, oper_str);

	pthread_mutex_unlock(&MAIN_CTX->mutex_for_ovldctrl);

	return ret == OVLD_CTRL_SET ? -1 : 0;
}

int NWUCP_OVLD_CHK_NGAP(int proc_code)
{
	switch (proc_code)
	{
		case NGAP_DownlinkNASTransport:
			return NWUCP_OVLD_CHECK(OVLD_CTRL_SVC_NGAP, "DOWNLINK_NAS_TRANS");
		case NGAP_InitialContextSetup:
			return NWUCP_OVLD_CHECK(OVLD_CTRL_SVC_NGAP, "INITIAL_CTX_SETUP");
		case NGAP_PDUSessionResourceSetup:
			return NWUCP_OVLD_CHECK(OVLD_CTRL_SVC_NGAP, "PDU_SESS_SETUP");
		default:
			return 0; /* ok */
	}
}

void NWUCP_OVLD_CHK_FAIL_NGAP(int proc_code, int res)
{
	if (res < 0) {
		switch (proc_code)
		{
			case NGAP_DownlinkNASTransport:
			case NGAP_InitialContextSetup:
			case NGAP_PDUSessionResourceSetup:
				ovldlib_increaseFailCnt(OVLD_CTRL_SVC_NGAP);
				break;
			default:
				break;
		}
	}
}

int NWUCP_OVLD_CHK_EAP(n3iwf_msg_t *n3iwf_msg)
{
	if (n3iwf_msg->msg_code == N3_IKE_AUTH_REQ &&
			n3iwf_msg->res_code == N3_EAP_INIT) {
		return NWUCP_OVLD_CHECK(OVLD_CTRL_SVC_EAP, "EAP_INIT");
	} else {
		return 0; /* ok */
	}
}

void NWUCP_OVLD_CHK_FAIL_EAP(n3iwf_msg_t *n3iwf_msg, int res)
{
	if (res < 0) {
		if (n3iwf_msg->msg_code == N3_IKE_AUTH_REQ &&
				n3iwf_msg->res_code == N3_EAP_INIT) {
			ovldlib_increaseFailCnt(OVLD_CTRL_SVC_EAP);
		}
	}
}

int NWUCP_OVLD_CHK_TCP()
{
	return NWUCP_OVLD_CHECK(OVLD_CTRL_SVC_TCP, "UPLINK_NAS_TRANS");
}

void NWUCP_OVLD_CHK_FAIL_TCP()
{
	ovldlib_increaseFailCnt(OVLD_CTRL_SVC_TCP);
}
