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

#ifndef NANO_IP_ROUTE_H
#define NANO_IP_ROUTE_H

#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_cfg.h"
#include "nano_ip_net_if.h"
#include "nano_ip_ipv4_def.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Network route */
typedef struct _nano_ip_net_route_t
{
    /** \brief Destination address */
    ipv4_address_t dest_addr;
    /** \brief Network mask */
    ipv4_address_t netmask;
    /** \brief Gateway address */
    ipv4_address_t gateway_addr;
    /** \brief Network interface */
    nano_ip_net_if_t* net_if;
    /** \brief Free entry flag */
    bool used;
} nano_ip_net_route_t;



/** \brief Route module internal data */
typedef struct _nano_ip_route_module_data_t
{
    /** \brief Route table */
    nano_ip_net_route_t route_table[NANO_IP_MAX_NET_ROUTE_COUNT];
    /** \brief Route table used entries count */
    uint32_t route_used_entries_count;
} nano_ip_route_module_data_t;



/** \brief Initialize the route module */
nano_ip_error_t NANO_IP_ROUTE_Init(void);

/** \brief Add a network route */
nano_ip_error_t NANO_IP_ROUTE_Add(const ipv4_address_t dest_addr, const ipv4_address_t netmask, const ipv4_address_t gateway_addr, nano_ip_net_if_t* const net_if);

/** \brief Remove a network route */
nano_ip_error_t NANO_IP_ROUTE_Delete(const ipv4_address_t dest_addr, const ipv4_address_t netmask);

/** \brief Search for a network route */
nano_ip_error_t NANO_IP_ROUTE_Search(const ipv4_address_t dest_addr, ipv4_address_t* const gateway_addr, nano_ip_net_if_t** net_if);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ROUTE_H */
