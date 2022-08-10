#include <json_macro.h>

int cnvt_cfg_to_json(json_object *obj, config_setting_t *setting, int callerType)
{   
    json_object *obj_group = NULL;
    json_object *obj_array = NULL;
    int count = 0;
    
    switch (setting->type) {
        // SIMPLE CASE
        case CONFIG_TYPE_INT:
            json_object_object_add(obj, setting->name, json_object_new_int(setting->value.ival));                                                                                  
            break;
        case CONFIG_TYPE_STRING:
            if (callerType == CONFIG_TYPE_ARRAY || callerType == CONFIG_TYPE_LIST) {
                json_object_array_add(obj, json_object_new_string(setting->value.sval));                                                                                           
            } else {
                json_object_object_add(obj, setting->name, json_object_new_string(setting->value.sval));                                                                           
            }
            break;
        case CONFIG_TYPE_BOOL:
            json_object_object_add(obj, setting->name, json_object_new_boolean(setting->value.ival));                                                                              
            break;
        // COMPLEX CASE
        case CONFIG_TYPE_GROUP:
            obj_group = json_object_new_object();    
            count = config_setting_length(setting);  
            for (int i = 0; i < count; i++) {            
                config_setting_t *elem = config_setting_get_elem(setting, i);
                cnvt_cfg_to_json(obj_group, elem, setting->type);                                                                                                                  
            }
            if (callerType == CONFIG_TYPE_ARRAY || callerType == CONFIG_TYPE_LIST) {
                json_object_array_add(obj, obj_group); 
            } else {
                json_object_object_add(obj, setting->name, obj_group);                                                                                                             
            }
            break;
        case CONFIG_TYPE_ARRAY:
        case CONFIG_TYPE_LIST:
            obj_array = json_object_new_array();
            json_object_object_add(obj, setting->name, obj_array);

            count = config_setting_length(setting);
            for (int i = 0; i < count; i++) {
                config_setting_t *elem = config_setting_get_elem(setting, i);
                cnvt_cfg_to_json(obj_array, elem, setting->type);
            }
            break;
        default:
            fprintf(stderr, "TODO| do something!\n");
            break;
    }

    return 0;
}

int check_number(char *ptr)
{
    for (int i = 0; i < strlen(ptr); i++) {
        if (isdigit(ptr[i]) == 0)
            return -1;
    }
    return atoi(ptr);
}

// json_object *js_value = search_json_object_ex(js_tok, "/NGAP-PDU/A/B/0{index}/C");
json_object *search_json_object(json_object *obj, char *key_string)
{
    char *key_parse = strdup(key_string);

    char *ptr = strtok(key_parse, "/");
    json_object *input = obj;
    json_object *output = NULL;

    while (ptr != NULL) {
        int cnvt_num = check_number(ptr);

        if (cnvt_num >= 0) {
            if (json_object_get_type(input) != json_type_array)
                goto SJO_ERR;
            if ((output = json_object_array_get_idx(input, cnvt_num)) == NULL)
                goto SJO_ERR;
        } else {
            if (json_object_object_get_ex(input, ptr, &output) == 0)
                goto SJO_ERR;
        }
        input = output;
        ptr = strtok(NULL, "/");
    }
    free(key_parse);
    return output;

SJO_ERR:
    free(key_parse);
    return NULL;
}

// json_object *js_value = search_json_object_ex(js_tok, "/NGAP-PDU/*/value/*/protocolIEs/*/value/AMF-UE-NGAP-ID", &key_list);
json_object *search_json_object_ex(json_object *input_obj, const char *key_input, key_list_t *key_list)
{   
    key_list->depth++;
    
    int is_leaf = 0, find = 0;
    char *key_parse = NULL, *key_tok = NULL, *key_rest = NULL;
    
    /* token drained out, don't go deeper */
    key_parse = strdup(key_input);
    if ((key_tok = strtok_r(key_parse, "/", &key_rest)) == NULL) {
        goto SJOE_RET;
    }
    /* check is_leaf */
    if (key_rest == NULL || strlen(key_rest) == 0) {
        is_leaf = 1;
    }
    
    json_object_object_foreach(input_obj, key, val) {
        if (!strcmp(key_tok, "*")) {
            if (key_list->key_depth < key_list->depth) {
                key_list->key_num++;
            } else if (key_list->key_depth > key_list->depth) {
                key_list->key_num--;
            }
			if ((key_list->key_num - 1) < MAX_JS_KEY_NUM) {
				snprintf(key_list->key_val[key_list->key_num -1], MAX_JS_KEY_LEN -1, "%s", key);
			}
            key_list->key_depth = key_list->depth;
        }
        if (!strcmp(key_tok, "*") || !strcmp(key_tok, key)) {
            find = 1;
        }
        
        json_object *obj = json_object_object_get(input_obj, key);
        if (find && is_leaf) { 
            key_list->find_obj = obj;
            goto SJOE_RET;
        }
        
        enum json_type o_type = json_object_get_type(obj);
		if (o_type == json_type_object) {
			if (search_json_object_ex(obj, key_rest, key_list) != NULL) {
				goto SJOE_RET;
			}
		} else if (o_type == json_type_array) {
            for (int i = 0; i < json_object_array_length(obj); i++) {
                json_object *elem = json_object_array_get_idx(obj, i);
                json_type elem_type = json_object_get_type(elem);
				if (elem_type == json_type_array || elem_type == json_type_object) {
					if (search_json_object_ex(elem, key_rest, key_list) != NULL) {
						goto SJOE_RET;
					}
				}
			}
		}
    }

SJOE_RET:
    key_list->depth--;
    if (key_parse) free(key_parse);
    return key_list->find_obj ? key_list->find_obj : NULL;
}
