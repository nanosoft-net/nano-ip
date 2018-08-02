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

#ifndef NANO_IP_NET_IFACES_H
#define NANO_IP_NET_IFACES_H

#include "nano_ip_cfg.h"
#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_oal.h"
#include "nano_ip_net_if.h"
#include "nano_ip_ipv4_def.h"
#include "nano_ip_net_driver.h"
#include "nano_ip_ethernet_def.h"

#if( NANO_IP_ENABLE_LOCALHOST == 1 )

/** \brief Localhost interface id */
#define LOCALHOST_INTERFACE_ID  0x00u

#endif /* NANO_IP_ENABLE_LOCALHOST */



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Network interfaces module internal data */
typedef struct _nano_ip_net_ifaces_module_data_t
{
    /** \brief List of network interfaces */
    nano_ip_net_if_t* net_ifaces;
    /** \brief Network interfaces count */
    uint8_t net_ifaces_count;
} nano_ip_net_ifaces_module_data_t;




/** \brief Initialize network interfaces module */
nano_ip_error_t NANO_IP_NET_IFACES_Init(void);

/** \brief Add a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_AddNetInterface(nano_ip_net_if_t* const net_iface, const char* const name, const uint32_t rx_packet_count, const uint32_t rx_packet_size);

/** \brief Bring up a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_Up(const uint8_t iface);

/** \brief Bring down a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_Down(const uint8_t iface);

/** \brief Set the MAC address of a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_SetMacAddress(const uint8_t iface, const uint8_t* const mac_address);

/** \brief Set the IPv4 address of a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_SetIpv4Address(const uint8_t iface, const ipv4_address_t address, const ipv4_address_t netmask, const ipv4_address_t gateway_address);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_NET_IF_H */
