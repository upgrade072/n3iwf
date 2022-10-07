#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "NGAP-PDU-Descriptions.h"

struct ossGlobal w, *world = &w;

char *file_to_buffer(char *filename, const char *mode, size_t *handle_size)
{       
    FILE *fp = fopen(filename, mode);
    if (fp == NULL) {
        fprintf(stderr, "[%s] can't read file=[%s]!\n", __func__, filename);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size_t file_size = *handle_size = ftell(fp);
    rewind(fp);

    char *buffer = malloc(file_size);
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    return buffer; // Must be freed
}  

int buffer_to_file(char *filename, const char *mode, unsigned char *buffer, size_t buffer_size, int free_buffer) 
{   
    /* write pdu to file */
    FILE *fp = fopen(filename, mode);
    int ret = fwrite(buffer, buffer_size, 1, fp);
    fclose(fp);

    if (free_buffer)
        free(buffer);

    return ret;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s pdu_filename\n", argv[0]);
        exit(0);
    }

	/* oss init for NGAP_PDU */
	if (ossinit(world, NGAP_PDU_Descriptions)) {
		fprintf(stderr, "Error: ossinit()\n");
		exit(0);
	}

	/* apply some rules TODO: unset this */
	ossSetFlags(world, ossGetFlags(world) | AUTOMATIC_ENCDEC);
#if 1
	ossSetDebugFlags(world, ossGetDebugFlags(world));
#else
	ossSetDebugFlags(world, ossGetDebugFlags(world) | PRINT_HEX_WITH_ASCII | OSS_DEBUG_LEVEL_3 );
#endif
	int PDU_NUM = NGAP_PDU_PDU;

	/* read f */
    size_t read_size = 0;
    char *fname = argv[1];
    char *file_data = file_to_buffer(fname, "r", &read_size);

	/* decode pdu - (from APER) */
	OssBuf pdubuf = { .length = read_size, .value = (unsigned char *)file_data };
	NGAP_PDU *input_pdu = NULL;
	ossDecode(world, &PDU_NUM, &pdubuf, (void **)&input_pdu);

	ossPrintPDU(world, PDU_NUM, input_pdu);

	/* encode pdu - (to JER) */
	ossSetEncodingRules(world, OSS_JSON);
	ossSetJsonFlags(world, ossGetJsonFlags(world) | JSON_ENC_CONTAINED_AS_TEXT);
	OssBuf jsonbuf = { .length = 0, .value = NULL };
	ossEncode(world, PDU_NUM, input_pdu, &jsonbuf);
	fprintf(stderr, "%s\n[length=(%ld)]\n", jsonbuf.value, jsonbuf.length);

	/* encode pdu - (to compact JER) */
	ossSetEncodingRules(world, OSS_JSON);
	ossSetJsonFlags(world, ossGetJsonFlags(world) | JSON_ENC_CONTAINED_AS_TEXT|JSON_COMPACT_ENCODING);
	OssBuf jsonbuf_com = { .length = 0, .value = NULL };
	ossEncode(world, PDU_NUM, input_pdu, &jsonbuf_com);

	/* ---------------------------------------- */

	/* decode pdu - (from JER) */
	NGAP_PDU *output_pdu = NULL;
	ossDecode(world, &PDU_NUM, &jsonbuf, (void **)&output_pdu);

	/* encode pdu - (to APER) */
	ossSetEncodingRules(world, OSS_PER_ALIGNED);
	OssBuf testbuf = { .length = 0, .value = NULL };
	ossEncode(world, PDU_NUM, output_pdu, &testbuf);

	/* write f */
	buffer_to_file("./result.bin", "w", testbuf.value, testbuf.length, 0);

	/* format pdu */
	char *format = (char *)jsonbuf_com.value;
	for (int i = 0; i < strlen(format); i++) {
		if (format[i] == '"') {
			fprintf(stderr, "\\");
			fprintf(stderr, "%c", format[i]);
		} else {
			fprintf(stderr, "%c", format[i]);
		}
	}
	fprintf(stderr, "\n");

	/* release resources */
	ossFreePDU(world, PDU_NUM, input_pdu);
	ossFreeBuf(world, jsonbuf.value);
	ossFreeBuf(world, jsonbuf_com.value);

	ossFreePDU(world, PDU_NUM, output_pdu);
	ossFreeBuf(world, testbuf.value);

	free(file_data);

	fprintf(stderr, "program done\n");
}
	
