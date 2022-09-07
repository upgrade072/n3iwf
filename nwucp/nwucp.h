#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <libconfig.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <event.h>
#include <event2/event.h>

#include <libutil.h>
#include <libnode.h>
#include <json_macro.h>
#include <ngap_intf.h>

#include <eap_intf.h>

#define MAX_WORKER_NUM      12

typedef enum event_caller_t {
	EC_MAIN = 0,
	EC_WORKER
} event_caller_t;

typedef enum ngap_msg_enum_t {
	NGSetupResponse = 21,
} ngap_msg_enum_t;

typedef struct amf_ctx_t {
	char hostname[128];
	int ng_setup_status;
	char amf_regi_tm[25];
	struct event *ev_amf_regi;
	json_object  *js_amf_data;
} amf_ctx_t;

typedef struct ue_ctx_t {
	int index;
	int occupied;
	char ip_addr[INET_ADDRSTRLEN];

	ike_tag_t	ike_tag;
	ctx_info_t	ctx_info;
	eap_relay_t	eap_5g;

	const char *state;
	struct event *ev_timer;
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
	int eap5g_nwucp_qid;
	int nwucp_eap5g_qid;
	int ngapp_nwucp_worker_qid;
	int eap5g_nwucp_worker_qid;
} qid_info_t;

typedef struct worker_ctx_t {
    pthread_t pthread_id;
    int thread_index;
    char thread_name[16];
    struct event_base *evbase_thrd;
} worker_ctx_t;

typedef struct worker_thread_t {
    int worker_num;
    worker_ctx_t *workers;
} worker_thread_t;

typedef struct main_ctx_t {
	config_t CFG;
	qid_info_t QID_INFO;
	worker_thread_t IO_WORKERS;

	struct event_base *evbase_main;
	json_object *js_ng_setup_request;
	linked_list amf_list;
	ue_info_t ue_info;
} main_ctx_t;

/* ------------------------- ngap.c --------------------------- */
json_object     *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item);

/* ------------------------- amf.c --------------------------- */
int     create_amf_list(main_ctx_t *MAIN_CTX);
void    amf_regi(evutil_socket_t fd, short events, void *data);
void    amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx);
void    amf_ctx_unset(amf_ctx_t *amf_ctx);
void    amf_regi_res_handle(sctp_tag_t *sctp_tag, bool success, json_object *js_ngap_pdu);

/* ------------------------- io_worker.c --------------------------- */
void    handle_ngap_msg(ngap_msg_t *ngap_msg, event_caller_t caller);
void    handle_ike_msg(ike_msg_t *ike_msg, event_caller_t caller);
void    msg_rcv_from_ngapp(evutil_socket_t fd, short events, void *data);
void    msg_rcv_from_eap5g(evutil_socket_t fd, short events, void *data);
void    io_thrd_tick(evutil_socket_t fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- ue.c --------------------------- */
int     create_ue_list(main_ctx_t *MAIN_CTX);
ue_ctx_t        *get_ue_ctx(main_ctx_t *MAIN_CTX);
void    ike_auth_assign_ue(ike_msg_t *ike_msg);
void    eap_req_5g_start(int conn_fd, short events, void *data);

/* ------------------------- main.c --------------------------- */
int     load_config(config_t *CFG);
int     create_n3iwf_profile(main_ctx_t *MAIN_CTX);
int     create_qid_info(main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
void    main_tick(evutil_socket_t fd, short events, void *data);
int     main();
