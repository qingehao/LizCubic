/*
  LizCubic APP应用控制 主线程
*/
#include "app_controller.h"
#include "interface.h"
#include "lvgl.h"
#include "rtthread.h"
#include "liz_bsp.h"
#include "liz_config.h"
#define DBG_TAG    "liz.sys.appcontroller"
#if defined(LIZ_SYS_APPCONTROLLER_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

enum ENTER_TYPE {
  INTER_ENTER = 0,
  UP_ENTER,
  LEFT_ENTER,
  RIGHT_ENTER,
};

static struct APP_OBJ *appList[APP_MAX_NUM]; // 预留10个APP注册位

static int app_num = 0;
static int cur_app_index = 0;
static int pre_app_index = 0;
static int inter_app_index = 0;
static int app_change_flag = 0;
static lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;
static uint8_t need_del_inter = 0;

static void app_controller_init(void);
static void app_controller_process(struct _app_action_info *act_info);
static int  app_controller_req_event(const struct APP_OBJ *from, int type, int event_id);
static int  app_register(struct APP_OBJ *app);
static void app_controller_get_cur_app_parm(void *arg);
static void app_controller_load();

static struct APP_CONTROLLER app_controller={
  .init = app_controller_init,
  .load = app_controller_load,
  .main_process = app_controller_process,
  .req_event = app_controller_req_event,
  .app_register = app_register,
  .get_cur_app_parm = app_controller_get_cur_app_parm,
};
/**
 * @brief 初始化app控制器
 * @param None
 * @return None
 */
static void app_controller_init()
{
  cur_app_index = 0;
  pre_app_index = 0;
  app_change_flag = 0;
  appList[cur_app_index]->init(); // 初始化第一个app
}
/**
 * @brief 加载第一个应用
 * @param None
 * @return None
 */
static void app_controller_load()
{
  appList[cur_app_index]->load(LV_SCR_LOAD_ANIM_FADE_ON); //加载第一个app
}
/**
 * @brief 获得app当前的参数 会调用app向回调中注册的函数
 * @param void *arg 参数存放地址
 * @return None
 */
static void app_controller_get_cur_app_parm(void *arg)
{
  appList[cur_app_index]->get_parm(arg);
}
/**
 * @brief 判断app是否合法
 * @param APP_OBJ *app app结构体指针
 * @return 0,成功 1,app为空 2,app注册数量超过限制
 */
static int app_is_legal(const struct APP_OBJ *obj)
{
  if(obj == RT_NULL)  return 1;
  if(app_num >= APP_MAX_NUM)  return 2;
  return 0;
}
/**
 * @brief app注册 最后一个注册的为inter app
 * @param APP_OBJ *app app结构体指针
 * @return 0,成功 其他,错误代码
 */
static int app_register(struct APP_OBJ *app)
{
  int ret = app_is_legal(app);
  if(ret != 0)
  {
    return ret;
  }
  appList[app_num] = app;
  app_num++;
  inter_app_index = app_num-1;
  return 0;
}
/**
 * @brief 切换到下一个app
 * @param type 进入类型
 *             LEFT_ENTER    -- 左切换
 *             RIGHT_ENTER   -- 右切换
 *             INTER_ENTER   -- 中断切换
 *             UP_ENTER      -- 上切换
 * @return None
 */
static __inline void switch_to_next_app(enum ENTER_TYPE type)
{
    if(cur_app_index == inter_app_index) //当前app是heartbeat
    {
      anim_type = LV_SCR_LOAD_ANIM_MOVE_BOTTOM;
      cur_app_index = pre_app_index;
      pre_app_index = inter_app_index;
    }
    else
    {
      pre_app_index = cur_app_index;
      switch (type)
      {
        case LEFT_ENTER:anim_type = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
                        cur_app_index = (cur_app_index - 1 + (app_num-1)) % (app_num-1);
                        break;
        case RIGHT_ENTER:anim_type = LV_SCR_LOAD_ANIM_MOVE_LEFT;
                        cur_app_index = (cur_app_index + 1) % (app_num-1);
                        break;
        case INTER_ENTER:break;
        case UP_ENTER:break;
      }
    }
    app_change_flag = 1;
}
/**
 * @brief 切换到inter app
 * @param type 进入类型
 *             INTER_ENTER -- 再次中断进入
 *             UP_ENTER    -- 用户手动进入
 * @return None
 */
static __inline void switch_to_inter_app(enum ENTER_TYPE type)
{ 
    if(cur_app_index == inter_app_index) //如果当前app已经是heartbeat了
    { 
      if(type ==INTER_ENTER)  
      {
        appList[cur_app_index]->on_event(2); //处于中断app 又收到了中断
      }
      else
      {
         appList[cur_app_index]->on_event(1); //处于中断app 收到了人工切换
      }
    }
    else
    {
      anim_type = (type ==INTER_ENTER)? LV_SCR_LOAD_ANIM_MOVE_BOTTOM:LV_SCR_LOAD_ANIM_MOVE_TOP;
      pre_app_index = cur_app_index;
      cur_app_index = inter_app_index;
      app_change_flag = 1;
    }
}
/**
 * @brief app切换进程
 * @param *act_info app 动作
 * @return None
 */
static void app_controller_process(struct _app_action_info *act_info)
{
  if(act_info->inter) // 接收到了中断
  {
    act_info->inter = 0;
    switch_to_inter_app(INTER_ENTER);
  }
  else
  {
    if(act_info->is_valid)
    {
      if(act_info->act_type == MOVE_LEFT) //向左倾斜
      {
        act_info->is_valid = 0;
        act_info->act_type = UNKNOWN;
        switch_to_next_app(LEFT_ENTER);
      }
      else if(act_info->act_type == MOVE_RIGHT)
      {
        act_info->is_valid = 0;
        act_info->act_type = UNKNOWN;
        switch_to_next_app(RIGHT_ENTER);
      }
      else if(act_info->act_type == MOVE_UP) //向前倒
      {
        act_info->is_valid = 0;
        act_info->act_type = UNKNOWN;
        switch_to_inter_app(UP_ENTER);
      }
    }
  }
  if(app_change_flag) //当前APP发生了改变
  {
    app_change_flag = 0;
    LOG_D("unload %s,load %s",appList[pre_app_index]->name,appList[cur_app_index]->name);
    appList[pre_app_index]->exit_callback(); //之前的APP销毁掉
    appList[cur_app_index]->init(); //转入的app初始化
    appList[cur_app_index]->load(anim_type); //加载app界面 
  }
  else{
    appList[cur_app_index]->process(&app_controller,act_info); // 执行当前APP的process
  }
}
/**
 * @brief app向外请求的接口 暂未用到
 * @param APP_OBJ *from -- 来自哪个app
 *        int      type -- 请求类型
 *        int  event_id -- 事件id      
 * @return None
 */
static int app_controller_req_event(const struct APP_OBJ *from, int type, int event_id)
{
    return 0;
}
/**
 * @brief 获得app控制器地址
 * @param None
 * @return app控制器地址
 */
struct APP_CONTROLLER *app_controller_get()
{
    return &app_controller;
}
