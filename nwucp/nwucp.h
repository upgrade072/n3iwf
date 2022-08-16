#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libconfig.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <event.h>
#include <event2/event.h>

#include <libutil.h>
#include <libnode.h>
#include <json_macro.h>
#include <ngap_intf.h>

/* for ASN.1 NGAP PDU */
#include <NGAP-PDU.h>
#include <ProcedureCode.h>
#include <ProtocolIE-ID.h>

typedef struct amf_ctx_t {
	char hostname[128];
	int ng_setup_status;
	char amf_regi_tm[25];
	struct event *ev_amf_regi;
	json_object  *js_amf_data;
} amf_ctx_t;

typedef struct ue_ctx_t {
	int occupied;
	char ip_addr[128];
} ue_ctx_t;

#define MAX_NWUCP_UE_NUM	10000
typedef struct ue_info_t {
	int ue_num;
	int cur_index;
	ue_ctx_t *ue_ctx;
} ue_info_t;

typedef struct qid_info_t {
	int ngapp_nwucp_qid;
	int nwucp_ngapp_qid;
} qid_info_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	struct event_base *evbase_main;

	json_object *js_ng_setup_request;
	linked_list amf_list;
	ue_info_t ue_info;
} main_ctx_t;

/* ------------------------- main.c --------------------------- */
int     load_config(config_t *CFG);
int     create_n3iwf_profile(main_ctx_t *MAIN_CTX);
int     create_qid_info(main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
void    main_tick(evutil_socket_t fd, short events, void *data);
void    handle_ngap_msg(ngap_msg_t *ngap_msg);
void    main_msg_rcv(evutil_socket_t fd, short events, void *data);
int     main();

/* ------------------------- ue.c --------------------------- */
int     create_ue_list(main_ctx_t *MAIN_CTX);

/* ------------------------- amf.c --------------------------- */
int     create_amf_list(main_ctx_t *MAIN_CTX);
void    amf_regi(evutil_socket_t fd, short events, void *data);
void    amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx);
void    amf_regi_res_handle(sctp_tag_t *sctp_tag, bool success, json_object *js_ngap_pdu);

/* ------------------------- ngap.c --------------------------- */
json_object     *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item);
