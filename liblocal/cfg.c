#include "libcfg.h"

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
