/*****************************************************************************/
/* THIS SAMPLE PROGRAM IS PROVIDED AS IS. THE SAMPLE PROGRAM AND ANY RESULTS */
/* OBTAINED FROM IT ARE PROVIDED WITHOUT ANY WARRANTIES OR REPRESENTATIONS,  */
/* EXPRESS, IMPLIED OR STATUTORY.                                            */
/*****************************************************************************/

#include "NGAP-PDU-Descriptions-createvalue.h"

static const char _static_value0[] = "Test";

/* Creation of sample value for the type 'NGAP_PDU' */
NGAP_PDU * create_samplevalue_NGAP_PDU(OssGlobal * world)
{
    NGAP_PDU * _out_data = (NGAP_PDU *)ossGetMemory(world, sizeof(struct NGAP_PDU));

    _out_data->choice = initiatingMessage_chosen;
    _out_data->u.initiatingMessage.procedureCode = 0;
    _out_data->u.initiatingMessage.criticality = reject;
    _out_data->u.initiatingMessage.value.pduNum = PDU_NGAP_ELEMENTARY_PROCEDURES_InitiatingMessage_AMFConfigurationUpdate;
    _out_data->u.initiatingMessage.value.encoded.length = 0;
    _out_data->u.initiatingMessage.value.encoded.value = NULL;

    _out_data->u.initiatingMessage.value.decoded.pdu_AMFConfigurationUpdate = (AMFConfigurationUpdate *)ossGetMemory(world, sizeof(struct AMFConfigurationUpdate));
    {
	struct _seqof407 * _out_data4;
	struct _seqof407 * _cur4;

	/* Fill the element #0 */
	_cur4 = (struct _seqof407 *)ossGetMemory(world, sizeof(struct _seqof407));
	_out_data4 = _cur4;
	_cur4->value.id = 1;
	_cur4->value.criticality = reject;
	_cur4->value.value.pduNum = PDU_AMFConfigurationUpdateIEs_Value_AMFName;
	_cur4->value.value.encoded.length = 0;
	_cur4->value.value.encoded.value = NULL;

	_cur4->value.value.decoded.pdu_AMFName = (AMFName *)ossGetMemory(world, sizeof(struct AMFName));
	((AMFName *)_cur4->value.value.decoded.pdu_AMFName)->length = 4;
	((AMFName *)_cur4->value.value.decoded.pdu_AMFName)->value = (char *)ossGetMemory(world, 4);
	memcpy(((AMFName *)_cur4->value.value.decoded.pdu_AMFName)->value, _static_value0, 4);


	/* Fill the element #1 */
	_cur4->next = (struct _seqof407 *)ossGetMemory(world, sizeof(struct _seqof407));
	_cur4 = _cur4->next;
	_cur4->value.id = 1;
	_cur4->value.criticality = reject;
	_cur4->value.value.pduNum = PDU_AMFConfigurationUpdateIEs_Value_AMFName;
	_cur4->value.value.encoded.length = 0;
	_cur4->value.value.encoded.value = NULL;

	_cur4->value.value.decoded.pdu_AMFName = (AMFName *)ossGetMemory(world, sizeof(struct AMFName));
	((AMFName *)_cur4->value.value.decoded.pdu_AMFName)->length = 4;
	((AMFName *)_cur4->value.value.decoded.pdu_AMFName)->value = (char *)ossGetMemory(world, 4);
	memcpy(((AMFName *)_cur4->value.value.decoded.pdu_AMFName)->value, _static_value0, 4);


	/* Set the last element */
	_cur4->next = NULL;
	((AMFConfigurationUpdate *)_out_data->u.initiatingMessage.value.decoded.pdu_AMFConfigurationUpdate)->protocolIEs = _out_data4;
    }



    return _out_data;
}

