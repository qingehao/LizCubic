#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H
#include "interface.h"

#define APP_MAX_NUM 4           // 最大的可运行的APP数量

struct APP_CONTROLLER
{
    void (*init)(void);
    void (*load)(void);
    void (*main_process)(struct _app_action_info *act_info);
    int  (*req_event)(const struct APP_OBJ *from, int type, int event_id);   
    int  (*app_register)(struct APP_OBJ *app);
    void (*get_cur_app_parm)(void *arg);
};
struct APP_CONTROLLER *app_controller_get();

#endif
