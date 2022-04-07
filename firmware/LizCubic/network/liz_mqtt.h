#ifndef _LIZ_MQTT_H_
#define _LIZ_MQTT_H_
#include "stdint.h"

int liz_mqtt_start(void (*sub_cb)(char *recv_buf,uint8_t len));
uint8_t liz_mqtt_get_status();
int liz_mqtt_publish(uint8_t *msg_str, int len);

#endif
