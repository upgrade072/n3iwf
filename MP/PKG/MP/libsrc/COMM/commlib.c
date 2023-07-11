#include <string.h>
#include <stdio.h>

#include "commlib.h"
#include "keepalivelib.h"
#include "sys_info.h"
#include "overloadlib.h"

extern int ixpc_que_init (char *app_name);
extern int conflib_initConfigData(void);

int
comlib_start (char *app_name, unsigned int options)
{
	unsigned int result = 0;

	if (options & LIB_INIT_SYSCONF)
	{
		if (conflib_initConfigData () < 0)
			return -1;

		result |= LIB_INIT_SYSCONF;
	}

	if (options & LIB_INIT_IXPC_QUEUE)
	{
		if (ixpc_que_init (app_name) < 0)
			return -1;

		result |= LIB_INIT_IXPC_QUEUE;
	}

	if (options & LIB_INIT_MSGQ)
	{
		if (msgq_init (app_name, TRUE) < 0)
			return -1;

		result |= LIB_INIT_MSGQ;
	}

#ifndef CONFIG_NO_SYS_HA
	if (options & LIB_INIT_SHM_SYS)
	{
		if (sys_init_shm (NULL) == NULL)
			return -1;

		result |= LIB_INIT_SHM_SYS;
	}
#endif /* CONFIG_NO_SYS_HA */

	if (options & LIB_INIT_KEEPALIVE)
	{
		if (keepalivelib_init (app_name) < 0)
			return -1;

		result |= LIB_INIT_KEEPALIVE;
	}

	return result;
}

