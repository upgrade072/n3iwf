#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/queue.h>
#include <errno.h>

/* ------------------------- util.c --------------------------- */
void    util_shell_cmd_apply(char *command, char *res, size_t res_size);
void    util_print_msgq_info(int key, int msqid);
int     util_get_queue_info(int key, const char *prefix);
void    *util_get_shm(int key, size_t size, const char *prefix);
