#include <stdio.h>
#include <string.h>

int main()
{
	char *ret_ptr, *next_ptr;
	char test[1024] = "/NGAP-PDU/*/value/*/ProtocolIEs/*/value/RAN-UE-NGAP-ID";

	ret_ptr = strtok_r(test, "/", &next_ptr);
	fprintf(stderr, "[%s] [%s]\n", ret_ptr, next_ptr);
	int i = 0;
	for ( i = 0; i < 7; i++) {
	ret_ptr = strtok_r(NULL, "/", &next_ptr);
	fprintf(stderr, "[%s] [%s]\n", ret_ptr, next_ptr);
	}
}
