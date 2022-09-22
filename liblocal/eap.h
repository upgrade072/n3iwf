#include <stdint.h>

#pragma pack(push, 1)

/*
==== EAP-5G ====
Bits  ----------------
7 6 5 4 3 2 1 0 Octets 
Code            1      (eap_packet)
Identifier      2 
Length          3 - 4 
Type            5      (data[0] & expanded type)
Vendor-Id       6 - 8 
Vendor-Type     9 - 12 
Message-Id      13 
Spare           14 
Extensions      15 - m (optional)
----------------------
*/

typedef struct eap_packet_raw {
	uint8_t     code;
	uint8_t     id;
	uint8_t     length[2];
	uint8_t     data[1];
} eap_packet_raw_t;
#define EAP_HEADER_LEN      4		/* Code (1) + Identifier (1) + Length (2) */
    
// eap->code
typedef enum eap_code {
	EAP_UNSET = 0,
    EAP_REQUEST,
    EAP_RESPONSE,
    EAP_SUCCESS,
    EAP_FAILURE,
    EAP_MAX_CODES
} eap_code_t;

// eap->data[0]
typedef enum eap_type {
    EAP_EXPANDED_TYPE = 254,
} eap_type_t;

// eap->data[8]
typedef enum msgid_type {
	NAS_5GS_UNSET = 0,
	NAS_5GS_START,
	NAS_5GS_NAS,
	NAS_5GS_NOTI,
	NAS_5GS_STOP,
	NAS_5GS_MAX_ID
} msgid_type_t;

/*
==== AN-PARAM ====
Bits  ----------------
7 6 5 4 3 2 1 0 Octets 
type            1
length          2
value           3 ~ length
----------------------
*/

typedef struct an_param_raw {
	uint8_t     type;
	uint8_t     length;
	uint8_t     value[1];
} an_param_raw_t;

typedef enum an_type {
	AN_GUAMI = 1,
	AN_PLMN_ID,
	AN_NSSAI,
	AN_CAUSE,
	AN_NID,
	AN_UE_ID,
	AN_MAX_TYPE
} an_type_t;

typedef enum establishment_cause {
	AEC_EMERENGY		= 0x00,
	AEC_HIGH_PRIORITY	= 0x01,
	AEC_MO_SIGNALLING	= 0x03,
	AEC_MO_DATA			= 0x04,
	AEC_MPS_PRIORITY	= 0x08,
	AEC_MCS_PRIORITY	= 0x09
} establishment_cause_t;

#pragma pack(pop)
