/*
Copyright(c) 2017 Cedric Jimenez

This file is part of Nano-IP.

Nano-IP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Nano-IP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Nano-IP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nano_ip.h"
#include "nano_ip_data.h"


#if( NANO_IP_ENABLE_LOCALHOST == 1 )
#include "nano_ip_localhost.h"
#endif /* NANO_IP_ENABLE_LOCALHOST */


/** \brief Nano IP handle */
nano_ip_t g_nano_ip;


/** \brief Initialize Nano IP stack */
nano_ip_error_t NANO_IP_Init(const nano_ip_net_packet_allocator_t* const packet_allocator)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((packet_allocator != NULL))
    {
        /* 0 init */
        NANO_IP_MEMSET(&g_nano_ip, 0, sizeof(nano_ip_t));

        /* Save packet allocator */
        g_nano_ip.packet_allocator = packet_allocator;

        /* Initialize the OAL */
        ret = NANO_IP_OAL_Init();

        /* Create the stack's mutex */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_MUTEX_Create(&g_nano_ip.mutex);
        }

        /* Initialize modules */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_ETHERNET_Init();
        }

        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_ARP_Init();
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_IPV4_Init();
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_ROUTE_Init();
        }
        #if( NANO_IP_ENABLE_ICMP == 1 )
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_ICMP_Init();
        }
        #endif /* NANO_IP_ENABLE_ICMP */
        #if( NANO_IP_ENABLE_UDP == 1 )
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_UDP_Init();
        }
        #endif /* NANO_IP_ENABLE_UDP */
        #if( NANO_IP_ENABLE_TCP == 1 )
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_TCP_Init();
        }
        #endif /* NANO_IP_ENABLE_TCP */
        #if( NANO_IP_ENABLE_SOCKET == 1 )
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_SOCKET_Init();
        }
        #endif /* NANO_IP_ENABLE_SOCKET */

        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_NET_IFACES_Init();
        }

        #if( NANO_IP_ENABLE_LOCALHOST == 1 )
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_LOCALHOST_Init();
            if (ret == NIP_ERR_SUCCESS)
            {
                ipv4_address_t localhost_addr = NANO_IP_inet_ntoa(IPV4_LOCALHOST_ADDR);
                ipv4_address_t localhost_netmask = NANO_IP_inet_ntoa(IPV4_LOCALHOST_NETMASK);
                ret = NANO_IP_NET_IFACES_SetIpv4Address(LOCALHOST_INTERFACE_ID, localhost_addr, localhost_netmask, 0u);
            }
        }
        #endif /* NANO_IP_ENABLE_LOCALHOST */
    }

    return ret;
}

/** \brief Start Nano IP stack */
nano_ip_error_t NANO_IP_Start(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    #if( NANO_IP_ENABLE_LOCALHOST == 1 )
    /* Bring up localhost interface */
    ret = NANO_IP_NET_IFACES_Up(LOCALHOST_INTERFACE_ID);
    #endif /* NANO_IP_ENABLE_LOCALHOST */

    return ret;
}
