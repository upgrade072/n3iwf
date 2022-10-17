#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <sys_info.h>
#include <comm_shm.h>
#include <sysconf.h>

#ifndef CONFIG_NO_SYS_HA

struct sys_info *shm_sys = NULL;

struct sys_info*
sys_init_shm (int *created)
{
	if (created)
		*created = 0;

	if (shm_sys)
		return shm_sys;

	struct sys_info *shm = NULL;
	size_t shm_size = sizeof (struct sys_info);

	int ret = commlib_shm_get (SHM_SYS_INFO, shm_size, (void **)&shm);
	if (ret < 0)
	{
		fprintf (stderr, "[%s.%s] ATTACH SHM_SYS(%s) FAIL. S:%lu\n", __FILE__, __func__, 
				SHM_SYS_INFO, (unsigned long)shm_size);
		return NULL;
	}

	if (ret == SHM_CREATE)
	{
		memset (shm, 0x00, shm_size);
		if (created)
			*created = 1;
	}

	shm_sys = shm;

	return shm_sys;
}


static inline struct sys_info*
sys_get_shm (void)
{
	if (shm_sys)
		return shm_sys;

	return sys_init_shm (NULL);
}


int
sys_ha_get_reserve (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		return sys->ha_reserve;
	return -1;
}


void
sys_ha_set_reserve (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		sys->ha_reserve = 1;

	return;
}

void
sys_ha_unset_reserve (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		sys->ha_reserve = 0;

	return;
}

int
sys_ha_get_enable (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		return sys->ha_enable;
	return -1;
}


void
sys_ha_set_enable (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		sys->ha_enable = 1;

	return;
}

void
sys_ha_unset_enable (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		sys->ha_enable = 0;

	return;
}

int
sys_ha_get_mode (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		return sys->ha_active;
	return -1;
}

void
sys_ha_set_active (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		sys->ha_active = MODE_ACTIVE;

	return;
}

void
sys_ha_unset_active (void)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys)
		sys->ha_active = MODE_STANDBY;

	return;
}

void
sys_ha_set_mode (uint8_t mode)
{
	struct sys_info *sys = sys_get_shm ();
	if (sys) {
		sys->ha_active = mode;
	}

	return;
}

int 
sys_ha_check_active (void)
{
	struct sys_info *sys = sys_get_shm ();

	if (sys && 
		sys->ha_enable &&
		sys->ha_active == MODE_ACTIVE)
		return 1;

	return 0;
}

#endif	/* CONFIG_NO_SYS_HA */
