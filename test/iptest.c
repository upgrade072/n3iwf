#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	struct in_addr ip_a = {0,};
	inet_aton("100.100.100.0", &ip_a);
	int a_ip = htonl(ip_a.s_addr);

	struct in_addr ip_b = {0,};
	inet_aton("100.100.101.0", &ip_b);
	int b_ip = htonl(ip_b.s_addr);

	fprintf(stderr,"%d\n", b_ip - a_ip);
}
