#ifndef __LIBCFG_H__
#define __LIBCFG_H__

#include <libconfig.h>
#include <json-c/json.h>
#include <json-c/json_object.h>

/* ------------------------- cfg.c --------------------------- */
int     cnvt_cfg_to_json(json_object *obj, config_setting_t *setting, int callerType);

#endif /* __LIBCFG_H__ */
