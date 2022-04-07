#include "network.h"
#include "liz_bsp.h"
#include <at_device_sim76xx.h>
#include "WS2812B/ws2812b.h"

enum NETWORK_INIT_STATUS network_status = 0;

extern struct _app_action_info act_info;
uint8_t network_link_status = 0;

static void mqtt_sub_cb(char *recv_buf, uint8_t len)
{
    if (strcmp(recv_buf, "HEARTBEAT") == 0)
    {
        act_info.inter = 1;
    }

}

void network_notification(enum AT_NETDEV_LTE_NOTIF_EVENT event);


void network_notification(enum AT_NETDEV_LTE_NOTIF_EVENT event)
{
    switch (event)
    {
    case LTE_SIM_FAIL: 
    {
        network_status = NETWORK_INIT_STATUS_SIM_FAIL;
        network_link_status = 0;
    }
    break;

    case LTE_CSQ_FAIL: 
    {
        network_status = NETWORK_INIT_STATUS_CSQ_FAIL;
        network_link_status = 0;
        // liz_bsp.rgbled->set_color(0, RGB_LED_COLOR_RED);
        // liz_bsp.rgbled->set_blink_period(120);
    }
    break;

    case LTE_NET_FAIL:  
    {
        network_status = NETWORK_INIT_STATUS_NET_FAIL;
        network_link_status = 0;
        // liz_bsp.rgbled->set_mode(RGB_LED_MODE_HOLDLIGHT);
    }
    break;

    case LTE_NET_SUCCESS:  
    {
        network_status = NETWORK_INIT_STATUS_SUCCESS;
        network_link_status = 1;
        liz_mqtt_start(mqtt_sub_cb);
    }
    break;
    case LTE_INIT_DONE:   
    {
        network_status = NETWORK_INIT_STATUS_DONE;
    }
    break;
    default:
        break;
    }
}


static struct at_device_sim76xx sim1 =
{
    "sim1",
    SIM76XX_SAMPLE_CLIENT_NAME,

    26, //on /off
    28, // status
    SIM76XX_SAMPLE_RECV_BUFF_LEN,
    network_notification,
};

static int sim76xx_device_register(void)
{
    struct at_device_sim76xx *sim76xx = &sim1;

    return at_device_register(&(sim76xx->device),
                              sim76xx->device_name,
                              sim76xx->client_name,
                              AT_DEVICE_CLASS_SIM76XX,
                              (void *) sim76xx);
}

#include "board.h"
#define POWER_EN_PIN GET_PIN(B,12)
#define POWER_ON_OFF_PIN GET_PIN(B,10)
void liz_network_init()
{
    struct at_device_sim76xx *sim76xx = &sim1;

    rt_pin_mode(28, PIN_MODE_OUTPUT);
    rt_pin_mode(26, PIN_MODE_OUTPUT);
    rt_pin_write(28, PIN_HIGH);
    rt_pin_write(26, PIN_HIGH);

    at_device_register(&(sim76xx->device),
                       sim76xx->device_name,
                       sim76xx->client_name,
                       AT_DEVICE_CLASS_SIM76XX,
                       (void *) sim76xx);

}
rt_bool_t check_network()
{
#ifdef RT_USING_NETDEV
    struct netdev *netdev = netdev_get_by_family(AF_INET);
    return (netdev && netdev_is_link_up(netdev));
#else
    return RT_TRUE;
#endif
}
