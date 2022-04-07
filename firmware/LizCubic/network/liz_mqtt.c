#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <rtthread.h>
#include "paho_mqtt.h"
#include "liz_bsp.h"
#include "liz_config.h"
#define DBG_TAG    "liz.net.mqtt"
#if defined(LIZ_NET_MQTT_DEBUG)
#define DBG_LVL    DBG_LOG
#else 
#define DBG_LVL    DBG_INFO
#endif
#include <rtdbg.h>

#define MQTT_WILLMSG                 "Goodbye!"

void (*mqtt_sub_cb)(char *recv_buf,uint8_t len) = RT_NULL;
/* define MQTT client context */
static MQTTClient client;
static uint8_t is_started = 0;
static uint8_t is_online = 0;

static char liz_mqtt_subtopic[10]={0};
static char liz_mqtt_pubtopic[10]={0};

static void mqtt_sub_callback(MQTTClient *c, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    if(mqtt_sub_cb)
    {
        mqtt_sub_cb((char *)msg_data->message->payload,msg_data->message->payloadlen);
    }
    LOG_D("mqtt sub callback: %.*s %.*s",
          msg_data->topicName->lenstring.len,
          msg_data->topicName->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}

static void mqtt_sub_default_callback(MQTTClient *c, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt sub default callback: %.*s %.*s",
          msg_data->topicName->lenstring.len,
          msg_data->topicName->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}

static void mqtt_connect_callback(MQTTClient *c)
{
    while(!check_network())
    {
        rt_thread_mdelay(100);
    }
    LOG_D("inter mqtt_connect_callback!");
}

static void mqtt_online_callback(MQTTClient *c)
{
    is_started = 1;
    is_online = 1;
    LOG_D("inter mqtt_online_callback!");
}

static void mqtt_offline_callback(MQTTClient *c)
{
	is_started = 0;
    is_online = 0;
    LOG_D("inter mqtt_offline_callback!");
}

uint8_t liz_mqtt_get_status()
{
    return is_online;
}

int liz_mqtt_start(void (*sub_cb)(char *recv_buf,uint8_t len))
{
    MQTTPacket_connectData condata = MQTTPacket_connectData_initializer;
    static char cid[30] = { 0 };
    char str_role[10] = {0};
    
    if (is_started)
    {
        LOG_E("mqtt client is already connected.");
        return -1;
    }
    liz_bsp.db_get_data("liz_role",str_role); // 获得角色
    LOG_D("liz role:%s",str_role);
    if(strcmp(str_role,"heart") == 0)
    {
        strcpy(liz_mqtt_subtopic,"/beat");
        strcpy(liz_mqtt_pubtopic,"/heart");
    }
    else 
    {
        strcpy(liz_mqtt_subtopic,"/heart");
        strcpy(liz_mqtt_pubtopic,"/beat");
    }
    mqtt_sub_cb = sub_cb;
    /* config MQTT context param */
    {
        client.isconnected = 0;
        client.uri = LIZ_MQTT_URI;
        /* generate the random client ID */
        rt_snprintf(cid, sizeof(cid), "liz_%s", str_role);
        LOG_D("client id:%s",cid);
        /* config connect param */
        memcpy(&client.condata, &condata, sizeof(condata));
        client.condata.clientID.cstring = cid;
        client.condata.keepAliveInterval = 30;
        client.condata.cleansession = 1;
        /* config MQTT will param. */
        client.condata.willFlag = 1;
        client.condata.will.qos = 1;
        client.condata.will.retained = 0;
        client.condata.will.topicName.cstring = "will";
        client.condata.will.message.cstring = MQTT_WILLMSG;
        /* malloc buffer. */
		client.buf_size = 512;
        client.readbuf_size = 512;
        client.buf = rt_calloc(1, client.buf_size);
        client.readbuf = rt_calloc(1, client.readbuf_size);
        if (!(client.buf && client.readbuf))
        {
            LOG_E("no memory for MQTT client buffer!");
            return -1;
        }
        /* set event callback function */
        client.connect_callback = mqtt_connect_callback;
        client.online_callback = mqtt_online_callback;
        client.offline_callback = mqtt_offline_callback;

        /* set subscribe table and event callback */
        client.messageHandlers[0].topicFilter = rt_strdup(liz_mqtt_subtopic);
        client.messageHandlers[0].callback = mqtt_sub_callback;
        client.messageHandlers[0].qos = QOS1;

        /* set default subscribe event callback */
        client.defaultMessageHandler = mqtt_sub_default_callback;
    }
    /* run mqtt client */
    paho_mqtt_start(&client);
    is_started = 1;
    return 0;
}

int LIZ_mqtt_stop()
{
    is_started = 0;
    is_online = 0;
    return paho_mqtt_stop(&client);
}

/**
  * @brief mqtt发布函数
  * @param None
  * @retval None
  */
int liz_mqtt_publish(uint8_t *msg_str, int len)
{
    if(is_online == 0)
	{
		return RT_ERROR;
	}
    MQTTMessage message;
    message.qos = QOS1;
    message.retained = 0;
    message.payload = (void *)msg_str;
    message.payloadlen = len;
    return MQTTPublish(&client, liz_mqtt_pubtopic, &message);
}

static void mqtt_new_sub_callback(MQTTClient *client, MessageData *msg_data)
{
    *((char *)msg_data->message->payload + msg_data->message->payloadlen) = '\0';
    LOG_D("mqtt new subscribe callback: %.*s %.*s",
          msg_data->topicName->lenstring.len,
          msg_data->topicName->lenstring.data,
          msg_data->message->payloadlen,
          (char *)msg_data->message->payload);
}

static int mqtt_subscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_subscribe [topic]  --send an mqtt subscribe packet and wait for suback before returning.\n");
        return -1;
    }
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }
    return paho_mqtt_subscribe(&client, QOS1, argv[1], mqtt_new_sub_callback);
}

static int mqtt_unsubscribe(int argc, char **argv)
{
    if (argc != 2)
    {
        rt_kprintf("mqtt_unsubscribe [topic]  --send an mqtt unsubscribe packet and wait for suback before returning.\n");
        return -1;
    }
    if (is_started == 0)
    {
        LOG_E("mqtt client is not connected.");
        return -1;
    }
    return paho_mqtt_unsubscribe(&client, argv[1]);
}

