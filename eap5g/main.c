#include <eap5g.h>

int main(int argc, char **argv)
{
	unsigned char buffer[10240] = {0,};

	fprintf(stderr, "\n1) Encap: eap_5g_start (id:159)\n");
	eap_relay_t eap_5g_start = { .eap_code = EAP_REQUEST, .eap_id = 159, .msg_id = NAS_5GS_START };
	buffer_to_file("./test.bin", "w", buffer, encap_eap_req(&eap_5g_start, buffer, 1024), 0);
	system("xxd ./test.bin");
	fprintf(stderr, "---------compare--------------------------\n");
	system("xxd ./sample_eap/'4. EAP-Req 5G-Start.bin'");
	fprintf(stderr, "\n");

	fprintf(stderr, "\n2) Decap: eap_5g_nas (id:159)\n");
	system("xxd './sample_eap/5. EAP-Res 5G-NAS (AN-Params, NAS-PDU [Regi Req]).bin'");
	size_t fsize = 0;
	unsigned char *file_data = file_to_buffer("./sample_eap/5. EAP-Res 5G-NAS (AN-Params, NAS-PDU [Regi Req]).bin", "r", &fsize);
	eap_relay_t eap_5g_nas = {0,};
	decap_eap_res(&eap_5g_nas, file_data, fsize);
	free(file_data);

	fprintf(stderr, "\n3) Encap: eap_5g_nas_req (id:66)\n");
	eap_relay_t eap_5g_nas_auth_req = { .eap_code = EAP_REQUEST, .eap_id = 66, .msg_id = NAS_5GS_NAS, .nas_str = "7e0056000200002142b257f5a147875a750c428ae763c43c2010fdb4f1fe9e89800033ccc420203dd5ee" };
	buffer_to_file("./test.bin", "w", buffer, encap_eap_req(&eap_5g_nas_auth_req, buffer, 1024), 0);
	system("xxd ./test.bin");
	fprintf(stderr, "---------compare--------------------------\n");
	system("xxd './sample_eap/8d. EAP-Req 5G-NAS (NAS-PDU [Auth Req [EAP_AKA-Challenge]]).bin'");
	fprintf(stderr, "\n");

	fprintf(stderr, "\n4) Decap: eap_5g_nas_res (id:66)\n");
	system("xxd './sample_eap/8e. EAP-Res 5G-NAS (NAS-PDU [Auth Res [EAP_AKA-Challenge]]).bin'");
	fsize = 0;
	file_data = file_to_buffer("./sample_eap/8e. EAP-Res 5G-NAS (NAS-PDU [Auth Res [EAP_AKA-Challenge]]).bin", "r", &fsize);
	eap_relay_t eap_5g_nas_auth_res = {0,};
	decap_eap_res(&eap_5g_nas_auth_res, file_data, fsize);
	free(file_data);

	fprintf(stderr, "\n5) Encap: eap_5g_nas_req (id:30)\n");
	eap_relay_t eap_5g_nas_security_command = { .eap_code = EAP_REQUEST, .eap_id = 30, .msg_id = NAS_5GS_NAS, .nas_str = "7e038b04bed8007e005d0200028020e1360100" };
	buffer_to_file("./test.bin", "w", buffer, encap_eap_req(&eap_5g_nas_security_command, buffer, 1024), 0);
	system("xxd ./test.bin");
	fprintf(stderr, "---------compare--------------------------\n");
	system("xxd './sample_eap/9b. EAP-Req 5G-NAS (NAS-PDU [Security Mode Command [EAP-Success]]).bin'");
	fprintf(stderr, "\n");

	fprintf(stderr, "\n6) Decap: eap_5g_nas_res (id:30)\n");
	system("xxd './sample_eap/9c. EAP-Res 5G-NAS (NAS-PDU [Security Mode Complete]).bin'");
	fsize = 0;
	file_data = file_to_buffer("./sample_eap/9c. EAP-Res 5G-NAS (NAS-PDU [Security Mode Complete]).bin", "r", &fsize);
	eap_relay_t eap_5g_nas_security_mode_complete = {0,};
	decap_eap_res(&eap_5g_nas_security_mode_complete, file_data, fsize);
	free(file_data);

	fprintf(stderr, "\n7) Encap: eap_success (id:142)\n");
	eap_relay_t eap_success = { .eap_code = EAP_SUCCESS, .eap_id = 142 };
	buffer_to_file("./test.bin", "w", buffer, encap_eap_result(&eap_success, buffer, 1024), 0);
	system("xxd ./test.bin");
	fprintf(stderr, "---------compare--------------------------\n");
	system("xxd './sample_eap/10b. EAP-Success.bin'");
	fprintf(stderr, "\n");
}
