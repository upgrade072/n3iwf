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

#include <glib-2.0/glib.h>

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
	DownlinkNASTransport	= 4,
	NGSetupResponse			= 21,
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
	int  amf_ue_id;
	int  ran_ue_id;
} amf_tag_t;

typedef struct sock_ctx_t {
	int    ue_index;
	int	   sock_fd;
	int	   connected;
	char   client_ipaddr[INET_ADDRSTRLEN];
	int    client_port;
	time_t create_time;
	struct bufferevent *bev;
	struct event *ev_flush_cb;
} sock_ctx_t;

typedef struct ue_ctx_t {
	int 		 index;
	int 		 occupied;
	char 		 ip_addr[INET_ADDRSTRLEN];

	/* TODO reset under info when ctx assign */
	ike_tag_t	 ike_tag;
	amf_tag_t	 amf_tag;
	ctx_info_t	 ctx_info;
	eap_relay_t	 eap_5g;

	const char	 *state;
	struct event *ev_timer;
	int           ipsec_sa_created;

	GSList		 *temp_cached_nas_message;
	sock_ctx_t	 *sock_ctx;
} ue_ctx_t;

#define MAX_NWUCP_UE_NUM	10000
typedef struct ue_info_t {
	int		  ue_num;
	char	  ue_start_ip[INET_ADDRSTRLEN];
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
	int					   listen_port;
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

/* ------------------------- ue.c --------------------------- */
int     create_ue_list(main_ctx_t *MAIN_CTX);
ue_ctx_t        *ue_ctx_assign(main_ctx_t *MAIN_CTX);
void    ue_ctx_unset(ue_ctx_t *ue_ctx);
ue_ctx_t        *ue_ctx_get_by_index(int index, worker_ctx_t *worker_ctx);
void    ue_ctx_stop_timer(ue_ctx_t *ue_ctx);
void    ue_ctx_release(int conn_fd, short events, void *data);
void    ue_assign_by_ike_auth(ike_msg_t *ike_msg);
int     ue_compare_guami(an_param_t *an_param, json_object *js_guami);
int     ue_compare_nssai(const char *sstsd_str, json_object *js_slice_list);
int     ue_compare_plmn_nssai(an_param_t *an_param, json_object *js_plmn_id);
int     ue_check_an_param_with_amf(eap_relay_t *eap_5g, json_object *js_guami, json_object *js_plmn_id);
void    ue_set_amf_by_an_param(ike_msg_t *ike_msg);

/* ------------------------- tcp.c --------------------------- */
sock_ctx_t      *create_sock_ctx(int fd, struct sockaddr *sa, int ue_index);
void    release_sock_ctx(sock_ctx_t *sock_ctx);
void    listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *data);
void    accept_sock_cb(int conn_fd, short events, void *data);
void    accept_new_client(ue_ctx_t *ue_ctx, sock_ctx_t *sock_ctx);
void    sock_event_cb(struct bufferevent *bev, short events, void *data);
void    sock_read_cb(struct bufferevent *bev, void *data);

/* ------------------------- ngap.c --------------------------- */
void    ngap_send_json(char *hostname, const char *body);
json_object     *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item);
json_object     *create_initial_ue_message_json(int ue_id, char *nas_pdu, char *cause);
json_object     *create_uplink_nas_transport_json(int amf_ue_id, int ran_ue_id, const char *nas_pdu);

/* ------------------------- main.c --------------------------- */
int     load_config(config_t *CFG);
int     create_n3iwf_profile(main_ctx_t *MAIN_CTX);
int     create_qid_info(main_ctx_t *MAIN_CTX);
int     create_tcp_server(main_ctx_t *MAIN_CTX);
int     create_worker_thread(worker_thread_t *WORKER, const char *prefix, main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
void    main_tick(int conn_fd, short events, void *data);
int     main();

/* ------------------------- test.c --------------------------- */
int     ipaddr_range_calc(char *start, char *end);
int     main();

/* ------------------------- io_worker.c --------------------------- */
worker_ctx_t    *worker_ctx_get_by_index(int index);
uint8_t create_eap_id();
void    eap_req_snd_msg(ue_ctx_t *ue_ctx, int msg_id, const char *nas_pdu);
void    tcp_req_snd_msg(ue_ctx_t *ue_ctx, int msg_id, const char *nas_pdu);
void    eap_req_5g_start(int conn_fd, short events, void *data);
void    eap_req_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu);
void    tcp_req_5g_nas(ue_ctx_t *ue_ctx, const char *nas_pdu);
void    ngap_proc_initial_ue_message(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    ngap_send_uplink_nas(ue_ctx_t *ue_ctx, char *nas_str);
void    ngap_proc_uplink_nas_transport(ue_ctx_t *ue_ctx, ike_msg_t *ike_msg);
void    nas_relay_to_amf(ike_msg_t *ike_msg);
void    nas_relay_to_ue(ngap_msg_t *ngap_msg, json_object *js_ngap_pdu);
void    handle_ngap_msg(ngap_msg_t *ngap_msg, event_caller_t caller);
void    handle_ike_msg(ike_msg_t *ike_msg, event_caller_t caller);
void    msg_rcv_from_ngapp(int conn_fd, short events, void *data);
void    msg_rcv_from_eap5g(int conn_fd, short events, void *data);
void    io_thrd_tick(int conn_fd, short events, void *data);
void    *io_worker_thread(void *arg);

/* ------------------------- amf.c --------------------------- */
int     create_amf_list(main_ctx_t *MAIN_CTX);
void    amf_regi(int conn_fd, short events, void *data);
void    amf_regi_start(main_ctx_t *MAIN_CTX, amf_ctx_t *amf_ctx);
void    amf_ctx_unset(amf_ctx_t *amf_ctx);
void    amf_regi_res_handle(sctp_tag_t *sctp_tag, bool success, json_object *js_ngap_pdu);
