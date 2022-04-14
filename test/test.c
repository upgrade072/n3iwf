#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *ipaddr_increaser(char *input_str)
{   
    char class_a[4] = {0,};
    char class_b[4] = {0,};
    char class_c[4] = {0,};
    char class_d[4] = {0,};
    sscanf(input_str, "%3[^.].%3[^.].%3[^.].%3s", class_a, class_b, class_c, class_d);
    
    // backward
    int ip_addr[4] = {0,};
    ip_addr[0] = atoi(class_d);
    ip_addr[1] = atoi(class_c);
    ip_addr[2] = atoi(class_b);
    ip_addr[3] = atoi(class_a);

    for (int i = 0; i < 4; i++) {
        int *a = &ip_addr[i];
        if (++(*a) > 255) {
            *a = 0;
            continue;
        }
        sprintf(input_str, "%d.%d.%d.%d", ip_addr[3], ip_addr[2], ip_addr[1], ip_addr[0]);
        return input_str;
    }
    return NULL;
}

int ipaddr_range_calc(char *start, char *end)
{
    char class_a[4] = {0,};
    char class_b[4] = {0,};
    char class_c[4] = {0,};
    char class_d[4] = {0,};

    sscanf(start, "%3[^.].%3[^.].%3[^.].%3s", class_a, class_b, class_c, class_d);
    int ip_addr_start = atoi(class_a) << 24 | atoi(class_b) << 16 | atoi(class_c) << 8 | atoi(class_d);

    sscanf(end, "%3[^.].%3[^.].%3[^.].%3s", class_a, class_b, class_c, class_d);
    int ip_addr_end = atoi(class_a) << 24 | atoi(class_b) << 16 | atoi(class_c) << 8 | atoi(class_d);

	int ip_count = ip_addr_end - ip_addr_start;
	fprintf(stderr, "%s(%d) - %s(%d) = range count(%d)\n", end, ip_addr_end, start, ip_addr_start, ip_count);

	return ip_count;
}

// range "10.0.0.0 ~ 10.0.0.100"
int ipaddr_range_scan(char *range, char *start, char *end)
{
	return sscanf(range, "%[^ ] ~ %s", start, end);
}

int main()
{
	char *ip_range = "10.0.0.0 ~ 10.0.0.100";
	char ip_start[256] = {0,}, ip_end[256] = {0,};
	int ret = ipaddr_range_scan(ip_range, ip_start, ip_end);
	fprintf(stderr, "ret=[%d] range=(%s) start=(%s) end=(%s)\n", ret, ip_range, ip_start, ip_end);

	int num_of_addr = ipaddr_range_calc(ip_start, ip_end);

	char *ip_list = strdup(ip_start);
	for (int i = 0; i < num_of_addr; i++) {
		fprintf(stderr, "%s\n", ip_list);
		ip_list = ipaddr_increaser(ip_list);
	}
	free(ip_list);

	fprintf(stderr, "num_of_addr=(%d)\n", num_of_addr);
}
