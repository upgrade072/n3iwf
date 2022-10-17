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
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/thread.h>
#include <sys/inotify.h>
#include <sys/uio.h>

#include <glib-2.0/glib.h>

#include <libutil.h>
#include <libnode.h>
#include <json_macro.h>
#include <ngap_intf.h>

#include <eap_intf.h>

#include <commlib.h>
#include <mmclib.h>
#include <overloadlib.h>
#include <msgtrclib.h>
#include <stm_msgtypes.h>

#define MAX_WORKER_NUM      12
#define UE_TAG(a)			(a->ctx_info.ue_id)

// for tcp nas relay
typedef struct nas_envelop_t {
	uint16_t			length;
	char				message[1];
} nas_envelop_t;

/*--------------------------------------------------------------*/

typedef enum event_caller_t {
	EC_MAIN = 0,
	EC_WORKER
} event_caller_t;

typedef enum regi_step_t {
	RS_AN_PARAM_NONE	= 0,
	RS_AN_PARAM_SET,
	RS_AN_PARAM_PROCESS,
} regi_step_t;

typedef enum ngap_msg_enum_t {
	AMFStatusIndication					= 1,
	DownlinkNASTransport				= 4,
	InitialContextSetupRequest			= 14,
	NGSetupResponse						= 21,
	PDUSessionResourceReleaseRequest	= 28,
	PDUSessionResourceSetupRequest		= 29,
	UEContextReleaseCommand				= 41,
} ngap_msg_enum_t;

typedef struct amf_ctx_t {
	char		  hostname[128];
	int			  ng_setup_status;
	char		  amf_regi_tm[25];
	struct event *ev_amf_regi;
	json_object  *js_amf_data;
} amf_ctx_t;

typedef struct amf_tag_t {
	char amf_host[128];
	uint64_t  amf_ue_id;
	uint64_t  ran_ue_id;
} amf_tag_t;

typedef struct sock_ctx_t {
	int    ue_index;
	int	   sock_fd;
	int	   connected;
	char   client_ipaddr[INET_ADDRSTRLEN];
	int    client_port;
	time_t create_time;
	struct bufferevent *bev;
	size_t remain_size;
#define SOCK_BUFF_MAX_SIZE	10240
	char   sock_buffer[SOCK_BUFF_MAX_SIZE];
} sock_ctx_t;

typedef enum pdu_state_t {
	PS_UNSET	= 0,
	PS_PREPARE,
	PS_CREATED,
} pdu_state_t;

typedef struct pdu_ctx_t {
	int		pdu_sess_id;
	int		pdu_state;
	char   *pdu_nas_pdu;
} pdu_ctx_t;

typedef struct ue_ctx_t {
	int 		 index;
	int 		 occupied;
	char 		 ip_addr[INET_ADDRSTRLEN];

	/* TODO reset under info when ctx assign */
	ike_tag_t	 ike_tag;
	amf_tag_t	 amf_tag;
	ctx_info_t	 ctx_info;
	eap_relay_t	 eap_5g;

	const char	 *state;					/* static assign, don't free */
	struct event *ev_timer;
	int           ipsec_sa_created;

	json_object	 *js_ue_regi_data;
	char		 *security_key;
	GSList		 *temp_cached_nas_message;	/* char *nas_str */
	GSList		 *pdu_ctx_list;				/* pdu_ctx_t */
	sock_ctx_t	 *sock_ctx;
} ue_ctx_t;

#define MAX_NWUCP_UE_NUM	10000
typedef struct ue_info_t {
	int		  ue_num;
	char	  ue_start_ip[INET_ADDRSTRLEN];
	int		  netmask;
	int		  cur_index;
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
    pthread_t		   pthread_id;
    int				   thread_index;
    char			   thread_name[16];
    struct event_base *evbase_thrd;
} worker_ctx_t;

typedef struct worker_thread_t {
    int			  worker_num;
    worker_ctx_t *workers;
} worker_thread_t;

typedef struct tcp_ctx_t {
	const char			  *tcp_listen_addr;
	int					   tcp_listen_port;
	struct sockaddr_in	   listen_addr;
	struct evconnlistener *listener;
} tcp_ctx_t;

typedef struct main_ctx_t {
	config_t			CFG;
	struct event_base	*evbase_main;

	qid_info_t			QID_INFO;
	worker_thread_t		IO_WORKERS;
	tcp_ctx_t			tcp_server;

	json_object			*js_ng_setup_request;

	linked_list			amf_list;
	ue_info_t			ue_info;
} main_ctx_t;

/* ------------------------- handler.c --------------------------- */
void    eap_proc_5g_start(int conn_fd, short events, void *data);
void    eap_proc_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu);
void    eap_proc_final(ue_ctx_t *ue_ctx, bool success, const char *security_key);
void    tcp_proc_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu);
void    ike_proc_pdu_release(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info);
void    ike_proc_pdu_request(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info);
void    ike_proc_ue_release(ue_ctx_t *ue_ctx);
void    ngap_proc_initial_ue_message(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    ngap_proc_uplink_nas_transport(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    ngap_proc_initial_context_setup_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    ngap_proc_pdu_session_resource_release_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    ngap_proc_pdu_session_resource_setup_response(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    ngap_proc_ue_context_release_request(ue_ctx_t *ue_ctx);
void    ngap_proc_ue_context_release_complete(ue_ctx_t *ue_ctx);
void    nas_relay_to_amf(ike_msg_t *ike_msg);
void    nas_regi_to_amf(ike_msg_t *ike_msg);
void    nas_relay_to_ue(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu);
void    ue_context_release(ike_msg_t *ike_msg);
void    ue_regi_res_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu);
void    ue_pdu_release_req_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu);
void    ue_pdu_setup_req_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu);
void    ue_pdu_setup_res_handle(ike_msg_t *ike_msg);
void    ue_ctx_release_handle(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu);
void    ue_inform_res_handle(ike_msg_t *ike_msg);

/* ------------------------- amf.c --------------------------- */
int     create_amf_list(main_ctx_t *MAIN_CTX);
void    remove_amf_list(main_ctx_t *MAIN_CTX);
void    amf_regi(int conn_fd, short events, void *data);
void    amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx);
void    amf_ctx_unset(amf_ctx_t *amf_ctx);
void    amf_regi_res_handle(sctp_tag_t *sctp_tag, bool success, json_object *js_ngap_pdu);
void    amf_status_ind_handle(sctp_tag_t *sctp_tag, json_object *js_ngap_pdu);

/* ------------------------- tcp.c --------------------------- */
sock_ctx_t      *create_sock_ctx(int fd, struct sockaddr *sa, int ue_index);
void    release_sock_ctx(sock_ctx_t *sock_ctx);
void    listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *data);
void    accept_sock_cb(int conn_fd, short events, void *data);
void    accept_new_client(ue_ctx_t *ue_ctx, sock_ctx_t *sock_ctx);
void    sock_event_cb(struct bufferevent *bev, short events, void *data);
void    sock_read_cb(struct bufferevent *bev, void *data);
void    sock_flush_cb(ue_ctx_t *ue_ctx);
void    sock_flush_temp_nas_pdu(ue_ctx_t *ue_ctx);

/* ------------------------- pdu.c --------------------------- */
pdu_ctx_t       *create_pdu_ctx(int pdu_sess_id, int state, const char *pdu_nas_pdu);
int     pdu_proc_fill_qos_flow_ids(n3_pdu_sess_t *pdu_sess, json_object *js_pdu_qos_flow_ids /* x MAX_N3_QOS_FLOW_IDS */);
int     pdu_proc_fill_pdu_sess_release_list(ue_ctx_t *ue_ctx, json_object *js_pdu_sess_list, n3_pdu_sess_t *pdu_sess_list /* x MAX_N3_PDU_NUM */);
int     pdu_proc_fill_pdu_sess_setup_list(ue_ctx_t *ue_ctx, json_object *js_pdu_sess_list, n3_pdu_sess_t *pdu_sess_list /* x MAX_N3_PDU_NUM */);
void    pdu_proc_sess_establish_accept(pdu_ctx_t *pdu_ctx, void *arg);
void    pdu_proc_json_id_attach(pdu_ctx_t *pdu_ctx, void *arg);
gint    pdu_proc_find_ctx(gpointer data, gpointer arg);
void    pdu_proc_remove_ctx(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info);
void    pdu_proc_flush_ctx(ue_ctx_t *ue_ctx);

/* ------------------------- ue.c --------------------------- */
int     create_ue_list(main_ctx_t *MAIN_CTX);
ue_ctx_t        *ue_ctx_assign(main_ctx_t *MAIN_CTX);
void    ue_assign_by_ike_auth(ike_msg_t *ike_msg);
ue_ctx_t        *ue_ctx_get_by_index(int index, worker_ctx_t *worker_ctx);
void    ue_ctx_transit_state(ue_ctx_t *ue_ctx, const char *new_state);
void    ue_ctx_stop_timer(ue_ctx_t *ue_ctx);
void    ue_ctx_release(int conn_fd, short events, void *data);
void    ue_ctx_unset(ue_ctx_t *ue_ctx);
int     ue_compare_guami(an_param_t *an_param, json_object *js_guami);
int     ue_compare_nssai(const char *sstsd_str, json_object *js_slice_list);
int     ue_compare_plmn_nssai(an_param_t *an_param, json_object *js_plmn_id);
int     ue_check_an_param_with_amf(eap_relay_t *eap_5g, json_object *js_guami, json_object *js_plmn_id);
void    ue_set_amf_by_an_param(ike_msg_t *ike_msg);
int     ue_check_ngap_id(ue_ctx_t *ue_ctx, json_object *js_ngap_pdu);

/* ------------------------- io_worker.c --------------------------- */
void    handle_ngap_log(const char *prefix, sctp_tag_t *sctp_tag, event_caller_t caller);
void    handle_ngap_msg(ngap_msg_t *ngap_msg, event_caller_t caller);
void    handle_ike_log(const char *prefix, ike_msg_t *ike_msg, event_caller_t caller);
void    handle_ike_msg(ike_msg_t *ike_msg, event_caller_t caller);
void    msg_rcv_from_ngapp(int conn_fd, short events, void *data);
void    msg_rcv_from_eap5g(int conn_fd, short events, void *data);
worker_ctx_t    *worker_ctx_get_by_index(int index);
void    io_thrd_tick(int conn_fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- ngap.c --------------------------- */
void    ngap_send_json(char *hostname, json_object *js_ngap);
json_object     *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item);
json_object     *create_initial_ue_message_json(uint32_t ue_id, char *nas_pdu, char *ip_str, int port, char *cause);
json_object     *create_uplink_nas_transport_json(uint32_t amf_ue_id, uint32_t ran_ue_id, const char *nas_pdu, char *ip_str, int port);
json_object     *create_initial_context_setup_response_json(const char *result, uint32_t amf_ue_id, uint32_t ran_ue_id);
json_object     *create_pdu_session_resource_release_response_json(const char *result, uint32_t amf_ue_id, uint32_t ran_ue_id, n3_pdu_info_t *pdu_info);
json_object     *create_pdu_session_resource_setup_response_json(const char *result, uint32_t amf_ue_id, uint32_t ran_ue_id, n3_pdu_info_t *pdu_info);
json_object     *create_ue_context_release_request_json(uint32_t amf_ue_id, uint32_t ran_ue_id, ue_ctx_t *ue_ctx);
json_object     *create_ue_context_release_complete_json(uint32_t amf_ue_id, uint32_t ran_ue_id, char *ip_str, int port, ue_ctx_t *ue_ctx);

/* ------------------------- parse.c --------------------------- */
int64_t ngap_get_amf_ue_ngap_id(json_object *js_ngap_pdu);
int64_t ngap_get_ran_ue_ngap_id(json_object *js_ngap_pdu);
const   char *ngap_get_nas_pdu(json_object *js_ngap_pdu);
const   char *ngap_get_security_key(json_object *js_ngap_pdu);
int     ngap_get_pdu_sess_id(json_object *js_pdu_sess_elem);
const   char *ngap_get_pdu_sess_nas_pdu(json_object *js_pdu_sess_elem);
const   char *ngap_get_pdu_sess_rel_cause(json_object *js_pdu_sess_elem);
int64_t ngap_get_pdu_sess_ambr_dl(json_object *js_pdu_sess_elem);
int64_t ngap_get_pdu_sess_ambr_ul(json_object *js_pdu_sess_elem);
const   char *ngap_get_pdu_gtp_te_addr(json_object *js_pdu_sess_elem);
const   char *ngap_get_pdu_gtp_te_id(json_object *js_pdu_sess_elem);
const   char *ngap_get_pdu_sess_type(json_object *js_pdu_sess_elem);
json_object     *ngap_get_pdu_qos_flow_ids(json_object *js_pdu_sess_elem);
int64_t ngap_get_ctx_rel_amf_ue_ngap_id(json_object *js_ngap_pdu);
int64_t ngap_get_ctx_rel_ran_ue_ngap_id(json_object *js_ngap_pdu);
const   char *ngap_get_ctx_rel_cause(json_object *js_ngap_pdu);
json_object     *ngap_get_unavailable_guami_list(json_object *js_ngap_pdu);

/* ------------------------- watch.c --------------------------- */
int     watch_directory_init(struct event_base *evbase, const char *path_name, void (*callback_function)(const char *arg_is_path));
void    start_watching_dir(main_ctx_t *MAIN_CTX);
void    watch_sctpc_restart(const char *file_name);

/* ------------------------- main.c --------------------------- */
int     load_config(config_t *CFG);
int     create_n3iwf_profile(main_ctx_t *MAIN_CTX);
int     create_qid_info(main_ctx_t *MAIN_CTX);
int     create_tcp_server(main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
void    main_tick(int conn_fd, short events, void *data);
int     main();

/* ------------------------- send.c --------------------------- */
uint8_t create_eap_id();
void    eap_send_eap_request(ue_ctx_t *ue_ctx, int msg_id, const char *nas_pdu);
void    eap_send_final_eap(ue_ctx_t *ue_ctx, bool success, const char *security_key);
void    ike_send_pdu_release(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info);
void    ike_send_pdu_request(ue_ctx_t *ue_ctx, n3_pdu_info_t *pdu_info);
void    ike_send_ue_release(ue_ctx_t *ue_ctx);
void    ngap_send_uplink_nas(ue_ctx_t *ue_ctx, char *nas_str);
void    tcp_save_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu);
void    tcp_send_downlink_nas(ue_ctx_t *ue_ctx, const char *nas_pdu);
