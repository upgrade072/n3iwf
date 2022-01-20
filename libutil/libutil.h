#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

/* ------------------------- util.c --------------------------- */
void    util_shell_cmd_apply(char *command, char *res, size_t res_size);
void    util_print_msgq_info(int key, int msqid);
