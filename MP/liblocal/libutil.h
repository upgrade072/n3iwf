#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/queue.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <loglib.h>

/* ------------------------- util.c --------------------------- */
void    util_shell_cmd_apply(char *command, char *res, size_t res_size);
void    util_print_msgq_info(int key, int msqid);
int     util_get_queue_info(int key, const char *prefix);
void    *util_get_shm(int key, size_t size, const char *prefix);
char    *util_get_ip_from_sa(struct sockaddr *sa);
int     util_get_port_from_sa(struct sockaddr *sa);
int     util_set_linger(int fd, int onoff, int linger);
int     util_set_rcvbuffsize(int fd, int byte);
int     util_set_sndbuffsize(int fd, int byte);
int     util_set_keepalive(int fd, int keepalive, int cnt, int idle, int intvl);
void    print_byte_bin(unsigned char value, char *ptr, size_t size);
void    print_bcd_str(const char *input, char *output, size_t size);
unsigned        char *file_to_buffer(char *filename, const char *mode, size_t *handle_size);
int     buffer_to_file(char *filename, const char *mode, unsigned char *buffer, size_t buffer_size, int free_buffer);
void    bin_to_hex(char *input, char *output);
void    hex_to_bin(char *input, char *output);
void    mem_to_hex(unsigned char *input, size_t input_size, char *output);
void    hex_to_mem(char *input, char *output, size_t *output_size);
void    ipaddr_to_hex(const char *ip_str, char *hex_str);
int     ip_num_to_subnet(int ip_num);
void    ipaddr_hex_to_inet(const char *hex_ip_str, char *inet_ip_str);
char    *ipaddr_increaser(char *input_str);
int     ipaddr_range_calc(char *start, char *end);
int     ipaddr_range_scan(const char *range, char *start, char *end);
int     ipaddr_sample();
