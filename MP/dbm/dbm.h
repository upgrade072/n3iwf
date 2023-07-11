#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <event.h>
#include <event2/event.h>
#include <event2/util.h>

#include <commlib.h>
#include <mmclib.h>
#include <overloadlib.h>
#include <msgtrclib.h>
#include <stm_msgtypes.h>

/* ------------------------- dbm_mmc.c --------------------------- */
int     initMmc();
void    handleMMCReq (IxpcQMsgType *rxIxpcMsg);
int     mmcHdlrVector_qsortCmp (const void *a, const void *b);
int     mmcHdlrVector_bsrchCmp (const void *a, const void *b);
int     mmcReloadConfigData(IxpcQMsgType *rxIxpcMsg);
int     mmcDisConfigData(IxpcQMsgType *rxIxpcMsg);
int     printFailureHeader(char *buff, const char *reason);
int     printSuccessHeader(char *buff);
int     mmib_relay_mmc (IxpcQMsgType *rxIxpcMsg);
int     dis_config_data(IxpcQMsgType *rxIxpcMsg);
int     reload_config_data(IxpcQMsgType *rxIxpcMsg);

/* ------------------------- dbm_trace.c --------------------------- */
int     dis_msg_trc(IxpcQMsgType *rxIxpcMsg);
int     reg_msg_trc(IxpcQMsgType *rxIxpcMsg);
int     canc_msg_trc(IxpcQMsgType *rxIxpcMsg);

/* ------------------------- main.c --------------------------- */
int     main();
void    main_tick(evutil_socket_t fd, short events, void *data);
int     initDbm();
int     initConf();
void    printAppConfig(char* resBuf);
int     loadAppConfig(char* resBuf);
int     initMsgQ(void);
void    checkDbmMsgQueue(evutil_socket_t fd, short events, void *data);
