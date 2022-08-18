#include "libutil.h"

void util_shell_cmd_apply(char *command, char *res, size_t res_size)
{
    FILE *p = popen(command, "r");
    if (p != NULL) {
        if (fgets(res, res_size, p) == NULL)
            res = NULL;
        pclose(p);
    }
}

void util_print_msgq_info(int key, int msqid)
{
	struct msqid_ds m_stat = {{0,}};

	fprintf(stderr, "---------- messege queue info -------------\n");
	if (msgctl(msqid, IPC_STAT, &m_stat)== -1) {
		fprintf(stderr, "msgctl failed (key=0x%x:msgqid=%d) [%d:%s]\n", key, msqid, errno, strerror(errno));
		return;
	}
	fprintf(stderr, " queue key : 0x%x\n", key);
	fprintf(stderr, " msg_lspid : %d\n",  m_stat.msg_lspid);
	fprintf(stderr, " msg_qnum  : %lu\n", m_stat.msg_qnum);
	fprintf(stderr, " msg_stime : %lu\n", m_stat.msg_stime);
	fprintf(stderr, "-------------------------------------------\n");
}

int util_get_queue_info(int key, const char *prefix)
{
	int msgq_id = 0;

	if (key <= 0 || ((msgq_id = msgget(key, IPC_CREAT | 0666)) < 0)) {
		fprintf(stderr, "%s() fail to create msgq [%s:key=(%d)]\n", __func__, prefix, key);
		return -1;
	}

	fprintf(stderr, "msgq_id=(%d)\n", msgq_id);
	util_print_msgq_info(key, msgq_id);
	return msgq_id;
}

void *util_get_shm(int key, size_t size, const char *prefix)
{
	int shm_id = 0;

	if (key <= 0 || ((shm_id = shmget(key, size, IPC_CREAT|0666)) < 0)) {
		fprintf(stderr, "%s() fail to create shm [%s:key=(%d)]\n", __func__, prefix, key);
		return NULL;
	}

	return shmat(shm_id, NULL, 0);
}

// return ip address 
char *util_get_ip_from_sa(struct sockaddr *sa)
{
    struct sockaddr_in *peer_addr = (struct sockaddr_in *)sa;
    return inet_ntoa(peer_addr->sin_addr);
}

// return port no
int util_get_port_from_sa(struct sockaddr *sa)
{
    in_port_t port = {0,};

    if (sa->sa_family == AF_INET) {
        port = (((struct sockaddr_in*)sa)->sin_port);
    } else {
        port = (((struct sockaddr_in6*)sa)->sin6_port);
    }

    return ntohs(port);
}

// set so_linger (abort) option to sock
int util_set_linger(int fd, int onoff, int linger)
{
    struct linger l = { .l_linger = linger, .l_onoff = onoff};
    int res = setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));

    return res;
}

int util_set_rcvbuffsize(int fd, int byte)
{
    int opt = byte;
    int res = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &opt, sizeof(opt));

    return res;
}

int util_set_sndbuffsize(int fd, int byte)
{
    int opt = byte;
    int res = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &opt, sizeof(opt));

    return res;
}

// set keepalived option to sock
int util_set_keepalive(int fd, int keepalive, int cnt, int idle, int intvl)
{
    int res = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
    res = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &idle, sizeof(idle));
    res = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    res = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));

    return res;
}

void print_byte_bin(unsigned char value, char *ptr, size_t size)
{
    if (size % 8 != 1) {
        fprintf(stderr, "size wrong [need %%8 + 1]\n");
        return;
    }
    int digit_cnt = 0;
    for (int i = sizeof(char) * 7; i >= 0; i--) {
        digit_cnt++;
    }
    int dummy_cnt = (size - 1) - digit_cnt;
    for (int i = 0; dummy_cnt > 0 && i < dummy_cnt; i++) {
        sprintf(ptr + strlen(ptr), "%c", '0');
    }
    for (int i = sizeof(char) * 7; i >= 0; i--) {
        sprintf(ptr + strlen(ptr), "%d", (value & (1 << i)) >> i );
    }
}

void print_bcd_str(const char *input, char *output, size_t size)
{
    int len = strlen(input);
    if (size < (len + 1)) {
        fprintf(stderr, "size wrong [need %d + 1]\n", len);
        return;
    }
    for (int i = 0; i < len; i ++) {
        int pos = i % 2 == 0 ? i + 1 : i - 1;
        output[pos] = isdigit(input[i]) ? input[i] : toupper(input[i]);
    }
    output[len] = '\0';
}

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

int buffer_to_file(char *filename, const char *mode, char *buffer, size_t buffer_size, int free_buffer)
{
    /* write pdu to file */
    FILE *fp = fopen(filename, mode);
    int ret = fwrite(buffer, buffer_size, 1, fp);
    fclose(fp);

    if (free_buffer)
        free(buffer);

    return ret;
}

void bin_to_hex(char *input, char *output)
{
	int len = strlen(input);
	int mod = (4 - (len % 4)) % 4;
	char *correct = NULL;

	if (mod) {
		asprintf(&correct, "%0*d%s", mod, 0, input);
	} else {
		asprintf(&correct, "%s", input);
	}

	for (int i = 0; i < len / 4; i++) {
		char bin_str[4+1] = {0,};
		sprintf(bin_str, "%.4s", correct + i*4);
		int val = strtol(bin_str, NULL, 2);
		sprintf(output + strlen(output), "%X", val);
	}

	free(correct);
}

void hex_to_bin(char *input, char *output)
{
	char bin_str[][5] = 
	{"0000", "0001", "0010", "0011", "0100", "0101", "0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110", "1111" };

	int len = strlen(input);
	char *bin_val = NULL;

	for (int i = 0; i < len; i++) {
		char hex_str = input[i];
		switch (hex_str) {
			case '0': bin_val = bin_str[0x0]; break;
			case '1': bin_val = bin_str[0x1]; break;
			case '2': bin_val = bin_str[0x2]; break;
			case '3': bin_val = bin_str[0x3]; break;
			case '4': bin_val = bin_str[0x4]; break;
			case '5': bin_val = bin_str[0x5]; break;
			case '6': bin_val = bin_str[0x6]; break;
			case '7': bin_val = bin_str[0x7]; break;
			case '8': bin_val = bin_str[0x8]; break;
			case '9': bin_val = bin_str[0x9]; break;
			case 'A': case 'a':bin_val = bin_str[0xa]; break;
			case 'B': case 'b':bin_val = bin_str[0xb]; break;
			case 'C': case 'c':bin_val = bin_str[0xc]; break;
			case 'D': case 'd':bin_val = bin_str[0xd]; break;
			case 'E': case 'e':bin_val = bin_str[0xe]; break;
			case 'F': case 'f':bin_val = bin_str[0xf]; break;
			default: fprintf(stderr, "err) unknown hex=(%c)\n", hex_str); break;
		}
		sprintf(output + strlen(output), "%s", bin_val);
	}
}

void mem_to_hex(char *input, size_t input_size, char *output)
{
	for (int i = 0; i < input_size; i++) {
		sprintf(output + strlen(output), "%02X", ((unsigned char *)input)[i]);
	}
}

void hex_to_mem(char *input, char *output, size_t *output_size)
{
	*output_size = 0;
	char *pos = input;
	for (int i = 0; i < strlen(input)/2; i++) {
		unsigned int num = 0;
		unsigned char val = 0;
		sscanf(pos, "%02X", &num);
		val = (char)num;
		output[(*output_size)++] = val;
		pos += 2;
	}
}

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
	struct in_addr ip_a = {0,}, ip_b = {0,};
	inet_aton(start, &ip_a);
	inet_aton(end, &ip_b);
	int ip_addr_start = htonl(ip_a.s_addr);
	int ip_addr_end = htonl(ip_b.s_addr);

	int ip_count = ip_addr_end - ip_addr_start;
	fprintf(stderr, "%s(%d) - %s(%d) = range count(%d)\n", end, ip_addr_end, start, ip_addr_start, ip_count);

	return ip_count;
}

// range "10.0.0.0 ~ 10.0.0.100"
int ipaddr_range_scan(const char *range, char *start, char *end)
{
	return sscanf(range, "%[^ ] ~ %s", start, end);
}

int ipaddr_sample()
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

	return 0;
}
