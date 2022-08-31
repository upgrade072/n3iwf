#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <libutil.h>

#include <eap.h>

/* ----------- EAP5G <--> NWUCP ------------------------------------------ */

typedef struct an_guami {
	uint8_t set;
	char mcc[4];
	char mnc[4];
	uint8_t amf_region_id;
	uint16_t amf_set_id;
	uint8_t amf_pointer;
	char plmn_id_bcd[7];
	char amf_id_str[7];
} an_guami_t;

typedef struct an_plmn_id {
	uint8_t set;
	char mcc[4];
	char mnc[4];
	char plmn_id_bcd[7];
} an_plmn_id_t;

typedef struct an_nssai {
	uint8_t set_num;
#define MAX_AN_NSSAI_NUM	8
	uint8_t sst[MAX_AN_NSSAI_NUM];
	char	ssd_str[MAX_AN_NSSAI_NUM][6+1];
} an_nssai_t;

typedef struct an_cause {
	uint8_t set;
	establishment_cause_t ec;
	char cause_str[24];
} an_cause_t;

typedef struct an_param {
	an_guami_t		guami;
	an_plmn_id_t	plmn_id;
	an_nssai_t		nssai;
	an_cause_t		cause;
	/* nid, ue_id ignored */
} an_param_t;

typedef struct eap_relay {
	eap_code_t		eap_code;
	uint8_t			eap_id;
	msgid_type_t	msg_id;
	an_param_t		an_param;
#define MAX_NAS_STR		512
	char			nas_str[MAX_NAS_STR];
} eap_relay_t;

/* ------------------------- eap5g.c --------------------------- */
const   char *establish_cause_str(establishment_cause_t cause);
size_t  encap_eap_req(eap_relay_t *eap_relay, unsigned char *buffer, size_t buffer_size);
size_t  encap_eap_result(eap_relay_t *eap_relay, unsigned char *buffer, size_t buffer_size);
void    parse_an_guami(unsigned char *params, size_t len, an_guami_t *guami);
void    parse_an_plmn_id(unsigned char *params, size_t len, an_plmn_id_t *plmn_id);
void    parse_an_aec(unsigned char *params, size_t len, an_cause_t *cause);
void    parse_an_nssai(unsigned char *params, size_t total_len, an_nssai_t *nssai);
void    parse_an_params(unsigned char *params, size_t total_len, an_param_t *an_param);
void    parse_nas_pdu(unsigned char *params, size_t total_len, char *nas_str);
void    decap_eap_res(eap_relay_t *eap_relay, unsigned char *buffer, size_t buffer_size);
