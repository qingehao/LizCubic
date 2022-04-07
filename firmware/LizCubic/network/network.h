#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "rtthread.h"

#if defined(RT_USING_SAL)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#if defined(RT_USING_NETDEV)
#include <netdev.h>
#elif defined(RT_USING_LWIP)
#include <lwip/netif.h>
#endif /* RT_USING_NETDEV */

#include <webclient.h>

enum NETWORK_INIT_STATUS
{
    NETWORK_INIT_STATUS_INITING = 0,
    NETWORK_INIT_STATUS_SIM_FAIL,
    NETWORK_INIT_STATUS_CSQ_FAIL,
    NETWORK_INIT_STATUS_NET_FAIL,
    NETWORK_INIT_STATUS_SUCCESS,
    NETWORK_INIT_STATUS_DONE,
};

void liz_network_init();
rt_bool_t check_network();

#endif
