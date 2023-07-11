#include "sctpc.h"

extern main_ctx_t *MAIN_CTX;

int numMmcHdlr;

MmcHdlrVector   mmcHdlrVector[4096] =
{
    { "dis-sctpc-status",     dis_sctpc_status},

    {"", NULL}
};

int initMmc()
{
    int i;
    for(i = 0; i < 4096; i++) {
        if(mmcHdlrVector[i].func == NULL)
            break;
    }
    numMmcHdlr = i;

    qsort ( (void*)mmcHdlrVector,
                   numMmcHdlr,
                   sizeof(MmcHdlrVector),
                   mmcHdlrVector_qsortCmp );
    return 1;
}

void handleMMCReq (IxpcQMsgType *rxIxpcMsg)
{
    MMLReqMsgType       *mmlReqMsg;
    MmcHdlrVector       *mmcHdlr;

    mmlReqMsg = (MMLReqMsgType*)rxIxpcMsg->body;

    if ((mmcHdlr = (MmcHdlrVector*) bsearch (
                    mmlReqMsg->head.cmdName,
                    mmcHdlrVector,
                    numMmcHdlr,
                    sizeof(MmcHdlrVector),
                    mmcHdlrVector_bsrchCmp)) == NULL) {
        logPrint(ELI, FL, "handleMMCReq() received unknown mml_cmd(%s)\n", mmlReqMsg->head.cmdName);
        return ;
    }
    logPrint(LL3, FL, "handleMMCReq() received mml_cmd(%s)\n", mmlReqMsg->head.cmdName);

    (int)(*(mmcHdlr->func)) (rxIxpcMsg);

    return;
}

int mmcHdlrVector_qsortCmp (const void *a, const void *b)
{
    return (strcasecmp (((MmcHdlrVector*)a)->cmdName, ((MmcHdlrVector*)b)->cmdName));
}

int mmcHdlrVector_bsrchCmp (const void *a, const void *b)
{
    return (strcasecmp ((char*)a, ((MmcHdlrVector*)b)->cmdName));
}

int dis_sctpc_status(IxpcQMsgType *rxIxpcMsg)
{
    char    mmlBuf[8109], *p = mmlBuf;

    p += mmclib_printSuccessHeader(p);

	disp_conn_list(MAIN_CTX, p);

    mmclib_sendMmcResult(rxIxpcMsg, mmlBuf, RES_SUCCESS, FLAG_COMPLETE, 0, 0, 0);

    return 0;
}
