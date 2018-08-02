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

#include "nano_ip_route.h"
#include "nano_ip_tools.h"
#include "nano_ip_data.h"

/** \brief Initialize the route module */
nano_ip_error_t NANO_IP_ROUTE_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    return ret;
}

/** \brief Add a network route */
nano_ip_error_t NANO_IP_ROUTE_Add(const ipv4_address_t dest_addr, const ipv4_address_t netmask, const ipv4_address_t gateway_addr, nano_ip_net_if_t* const net_if)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (net_if != NULL)
    {
        uint32_t i;
        nano_ip_route_module_data_t* const route_module = &g_nano_ip.route_module;

        /* Look for a free entry */
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        ret = NIP_ERR_RESOURCE;
        if (route_module->route_used_entries_count < NANO_IP_MAX_NET_ROUTE_COUNT)
        {
            for (i = 0; (i < NANO_IP_MAX_NET_ROUTE_COUNT) && (ret == NIP_ERR_RESOURCE); i++)
            {
                nano_ip_net_route_t* route_entry = &route_module->route_table[i];
                if (!route_entry->used)
                {
                    /* Fill route entry */
                    route_entry->dest_addr = (dest_addr & netmask);
                    route_entry->netmask = netmask;
                    route_entry->gateway_addr = gateway_addr;
                    route_entry->net_if = net_if;
                    route_entry->used = true;
                    route_module->route_used_entries_count++;
                    ret = NIP_ERR_SUCCESS;
                }
            }
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Remove a network route */
nano_ip_error_t NANO_IP_ROUTE_Delete(const ipv4_address_t dest_addr, const ipv4_address_t netmask)
{
    uint32_t i;
    nano_ip_error_t ret = NIP_ERR_ROUTE_NOT_FOUND;
    nano_ip_route_module_data_t* const route_module = &g_nano_ip.route_module;
    const ipv4_address_t netaddr = dest_addr & netmask;
    
    /* Look for the corresponding entry */
    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    for (i = 0; (i < NANO_IP_MAX_NET_ROUTE_COUNT) && (ret == NIP_ERR_ROUTE_NOT_FOUND); i++)
    {
        nano_ip_net_route_t* route_entry = &route_module->route_table[i];
        if ( (route_entry->used) && 
                (route_entry->dest_addr == netaddr) &&
                (route_entry->netmask == netmask) )
        {
            /* Free entry */
            MEMSET(route_entry, 0, sizeof(nano_ip_net_route_t));
            route_module->route_used_entries_count--;
            ret = NIP_ERR_SUCCESS;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Search for a network route */
nano_ip_error_t NANO_IP_ROUTE_Search(const ipv4_address_t dest_addr, ipv4_address_t* const gateway_addr, nano_ip_net_if_t** net_if)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((gateway_addr != NULL) && (net_if != NULL))
    {
        uint32_t i;
        nano_ip_route_module_data_t* const route_module = &g_nano_ip.route_module;

        /* Look for a the corresponding entry */
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        ret = NIP_ERR_ROUTE_NOT_FOUND;
        for (i = 0; (i < NANO_IP_MAX_NET_ROUTE_COUNT) && (ret == NIP_ERR_ROUTE_NOT_FOUND); i++)
        {
            nano_ip_net_route_t* route_entry = &route_module->route_table[i];
            if ((route_entry->used) &&
                (route_entry->dest_addr == (dest_addr & route_entry->netmask)) )
            {
                /* Retrieve route data */
                (*gateway_addr) = route_entry->gateway_addr;
                (*net_if) = route_entry->net_if;
                ret = NIP_ERR_SUCCESS;
            }
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

