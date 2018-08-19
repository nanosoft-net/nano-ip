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

#ifndef NANO_IP_NET_IF_H
#define NANO_IP_NET_IF_H

#include "nano_ip_types.h"
#include "nano_ip_oal.h"
#include "nano_ip_ipv4_def.h"
#include "nano_ip_net_driver.h"
#include "nano_ip_ethernet_def.h"



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Network interface */
typedef struct _nano_ip_net_if_t
{
    /** \brief Id */
    uint8_t id;
    /** \brief Name */
    const char* name;
    /** \brief MAC address */
    uint8_t mac_address[MAC_ADDRESS_SIZE];
    /** \brief IPv4 address */
    ipv4_address_t ipv4_address;
    /** \brief IPv4 netmask */
    ipv4_address_t ipv4_netmask;

    /** \brief Network driver */
    const nano_ip_net_driver_t* driver;

    /** \brief Task handle */
    oal_task_t task;
    /** \brief Synchronization flags */
    oal_flags_t sync_flags;
    /** \brief Periodic timer */
    oal_timer_t timer;
    /** \brief Link state */
    net_link_state_t link_state;

    /** \brief Next interface */
    struct _nano_ip_net_if_t* next;
} nano_ip_net_if_t;





/** \brief Initialize a network interface */
nano_ip_error_t NANO_IP_NET_IF_Init(nano_ip_net_if_t* const net_if, const char* const name, 
                                    const uint32_t rx_packet_count, const uint32_t rx_packet_size,
                                    const uint8_t task_priority, const uint32_t task_stack_size);

/** \brief Bring up a network interface */
nano_ip_error_t NANO_IP_NET_IF_Up(nano_ip_net_if_t* const net_if);

/** \brief Bring down a network interface */
nano_ip_error_t NANO_IP_NET_IF_Down(nano_ip_net_if_t* const net_if);

/** \brief Set the MAC address of a network interface */
nano_ip_error_t NANO_IP_NET_IF_SetMacAddress(nano_ip_net_if_t* const net_if, const uint8_t* const mac_address);

/** \brief Set the IPv4 address of a network interface */
nano_ip_error_t NANO_IP_NET_IF_SetIpv4Address(nano_ip_net_if_t* const net_if, const ipv4_address_t address, const ipv4_address_t netmask);




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_NET_IF_H */
