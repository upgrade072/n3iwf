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
