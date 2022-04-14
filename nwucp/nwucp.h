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

/* for ASN.1 NGAP PDU */
#include <NGAP-PDU.h>
#include <ProcedureCode.h>
#include <ProtocolIE-ID.h>

typedef struct amf_ctx_t {
	char hostname[128];
	int ng_setup_status;
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

typedef struct main_ctx_t {
	config_t CFG;
	struct event_base *evbase_main;

	json_object *js_ng_setup_request;
	linked_list amf_list;
	ue_info_t ue_info;
} main_ctx_t;

/* ------------------------- ngap.c --------------------------- */
json_object     *create_ng_setup_request_json(const char *mnc_mcc, int n3iwf_id, const char *ran_node_name, json_object *js_support_ta_item);

/* ------------------------- main.c --------------------------- */
int     load_config(config_t *CFG);
int     create_n3iwf_profile(main_ctx_t *MAIN_CTX);
int     initialize(main_ctx_t *MAIN_CTX);
int     main();

/* ------------------------- amf.c --------------------------- */
int     create_amf_list(main_ctx_t *MAIN_CTX);

/* ------------------------- ue.c --------------------------- */
int     create_ue_list(main_ctx_t *MAIN_CTX);
