#ifndef __JSON_MACRO_H__
#define __JSON_MACRO_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <libconfig.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#define JS_SEARCH_OBJ(json,key) search_json_object(json,key)
#define JS_SEARCH_STR(json,key) json_object_get_string(search_json_object(json,key))
#define JS_SEARCH_INT(json,key) json_object_get_int(search_json_object(json,key))
#define JS_PRINT_COMPACT(json) json_object_to_json_string_ext(json, JSON_C_TO_STRING_NOSLASHESCAPE)
#define JS_PRINT_PRETTY(json) json_object_to_json_string_ext(json, JSON_C_TO_STRING_PRETTY|JSON_C_TO_STRING_NOSLASHESCAPE)

#define MAX_JS_KEY_NUM	6
#define MAX_JS_KEY_LEN	64
typedef struct key_list_t {
    int depth;
    json_object *find_obj;

    int key_num;
    int key_depth;
    char key_val[MAX_JS_KEY_NUM][MAX_JS_KEY_LEN];
} key_list_t;

/* ------------------------- json_macro.c --------------------------- */
int     cnvt_cfg_to_json(json_object *obj, config_setting_t *setting, int callerType);
int     check_number(char *ptr);
json_object     *search_json_object(json_object *obj, char *key_string);
json_object     *search_json_object_ex(json_object *input_obj, const char *key_input, key_list_t *key_list);
#endif /* __JSON_MACRO_H__ */
