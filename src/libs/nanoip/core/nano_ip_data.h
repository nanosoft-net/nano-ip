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

#ifndef NANO_IP_DATA_H
#define NANO_IP_DATA_H

#include "nano_ip.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Nano IP handle */
typedef struct _nano_ip_t
{
    /** \brief Stack mutex */
    oal_mutex_t mutex;
    /** \brief Network interfaces module internal data */
    nano_ip_net_ifaces_module_data_t net_ifaces_module;
    #if (NANO_IP_ENABLE_LOCALHOST == 1)
    /** \brief Localhost module internal data */
    nano_ip_localhost_module_data_t localhost_module;
    #endif /* (NANO_IP_ENABLE_LOCALHOST == 1) */
    /** \brief Ethernet module internal data */
    nano_ip_ethernet_module_data_t eth_module;
    /** \brief ARP module internal data */
    nano_ip_arp_module_data_t arp_module;
    /** \brief Route module internal data */
    nano_ip_route_module_data_t route_module;
    /** \brief IPv4 module internal data */
    nano_ip_ipv4_module_data_t ipv4_module;
    #if (NANO_IP_ENABLE_ICMP == 1)
    /** \brief ICMP module internal data */
    nano_ip_icmp_module_data_t icmp_module;
    #endif /* (NANO_IP_ENABLE_ICMP == 1) */
    #if (NANO_IP_ENABLE_UDP == 1)
    nano_ip_udp_module_data_t udp_module;
    #endif /* (NANO_IP_ENABLE_UDP == 1) */
    #if (NANO_IP_ENABLE_TCP == 1)
    nano_ip_tcp_module_data_t tcp_module;
    #endif /* (NANO_IP_ENABLE_TCP == 1) */
    #if (NANO_IP_ENABLE_SOCKET == 1)
    nano_ip_socket_module_data_t socket_module;
    #endif /* (NANO_IP_ENABLE_SOCKET == 1) */
    /** \brief Packet allocator */
    const nano_ip_net_packet_allocator_t* packet_allocator;
} nano_ip_t;



/** \brief Nano IP handle */
extern nano_ip_t g_nano_ip;


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_DATA_H */
