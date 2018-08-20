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

#include "nano_ip_net_ifaces.h"
#include "nano_ip_tools.h"
#include "nano_ip_ethernet.h"
#include "nano_ip_data.h"


/** \brief Look for a network interface */
static nano_ip_net_if_t* NANO_IP_NET_IF_LookForNetIf(const uint8_t iface);


/** \brief Initialize network interfaces module */
nano_ip_error_t NANO_IP_NET_IFACES_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    return ret;
}

/** \brief Add a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_AddNetInterface(nano_ip_net_if_t* const net_iface, const char* const name, 
                                                   const uint32_t rx_packet_count, const uint32_t rx_packet_size,
                                                   const uint8_t task_priority, const uint32_t task_stack_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((net_iface != NULL) &&
        (name != NULL))
    {
        /* Initialize the network interface */
        ret = NANO_IP_NET_IF_Init(net_iface, name, rx_packet_count, rx_packet_size, task_priority, task_stack_size);
        if (ret == NIP_ERR_SUCCESS)
        {
            nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

            /* Add interface to the interface list */
            (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

            net_iface->next = net_ifaces_module->net_ifaces;
            net_ifaces_module->net_ifaces = net_iface;
            net_iface->id = net_ifaces_module->net_ifaces_count;
            net_ifaces_module->net_ifaces_count++;

            (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
        }
    }

    return ret;
}

/** \brief Bring up a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_Up(const uint8_t iface)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (iface < net_ifaces_module->net_ifaces_count)
    {
        /* Look for the network interface */
        nano_ip_net_if_t* net_if = NANO_IP_NET_IF_LookForNetIf(iface);
        if (net_if != NULL)
        {
            /* Bring up the network interface */
            ret = NANO_IP_NET_IF_Up(net_if);
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Bring down a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_Down(const uint8_t iface)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (iface < net_ifaces_module->net_ifaces_count)
    {
        /* Look for the network interface */
        nano_ip_net_if_t* net_if = NANO_IP_NET_IF_LookForNetIf(iface);
        if (net_if != NULL)
        {
            /* Bring down the network interface */
            ret = NANO_IP_NET_IF_Down(net_if);
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Set the MAC address of a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_SetMacAddress(const uint8_t iface, const uint8_t* const mac_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (iface < net_ifaces_module->net_ifaces_count)
    {
        /* Look for the network interface */
        nano_ip_net_if_t* net_if = NANO_IP_NET_IF_LookForNetIf(iface);
        if (net_if != NULL)
        {
            #if( NANO_IP_ENABLE_LOCALHOST == 1 )

            /* Remove previous localhost ARP */
            (void)NANO_IP_ARP_RemoveEntry(net_if->ipv4_address);
            
            #endif /* NANO_IP_ENABLE_LOCALHOST */

            /* Set the MAC address the network interface */
            ret = NANO_IP_NET_IF_SetMacAddress(net_if, mac_address);

            #if( NANO_IP_ENABLE_LOCALHOST == 1 )

            /* Add localhost ARP */
            (void)NANO_IP_ARP_AddEntry(AET_STATIC, net_if->mac_address, net_if->ipv4_address);

            #endif /* NANO_IP_ENABLE_LOCALHOST */
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Set the IPv4 address of a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_SetIpv4Address(const uint8_t iface, const ipv4_address_t address, const ipv4_address_t netmask, const ipv4_address_t gateway_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (iface < net_ifaces_module->net_ifaces_count)
    {
        nano_ip_net_if_t* net_if;

        /* Look for interface */
        net_if = NANO_IP_NET_IF_LookForNetIf(iface);
        if (net_if != NULL)
        {

            #if( NANO_IP_ENABLE_LOCALHOST == 1 )
            nano_ip_net_if_t* localhost_net_if = NULL;
            /* Remove previous localhost route */
            (void)NANO_IP_ROUTE_Delete(net_if->ipv4_address, 0xFFFFFFFFu);

            /* Remove previous localhost ARP */
            (void)NANO_IP_ARP_RemoveEntry(address);
            #endif /* NANO_IP_ENABLE_LOCALHOST */

            /* Remove previous routes */
            if (net_if->ipv4_address != 0u)
            {
                (void)NANO_IP_ROUTE_Delete(net_if->ipv4_address, net_if->ipv4_netmask);
            }
            if (gateway_address != 0u)
            {
                (void)NANO_IP_ROUTE_Delete(0u, 0u);
            }

            /* Update IP address and netmask */
            ret = NANO_IP_NET_IF_SetIpv4Address(net_if, address, netmask);

            #if( NANO_IP_ENABLE_LOCALHOST == 1 )
            /* Add localhost route */
            localhost_net_if = NANO_IP_NET_IF_LookForNetIf(LOCALHOST_INTERFACE_ID);
            (void)NANO_IP_ROUTE_Add(net_if->ipv4_address, 0xFFFFFFFFu, 0u, localhost_net_if);

            /* Add localhost ARP */
            (void)NANO_IP_ARP_AddEntry(AET_STATIC, net_if->mac_address, net_if->ipv4_address);
            #endif /* NANO_IP_ENABLE_LOCALHOST */

            /* Add new routes */
            ret = NANO_IP_ROUTE_Add(net_if->ipv4_address, net_if->ipv4_netmask, 0u, net_if);
            if ((ret == NIP_ERR_SUCCESS) && (gateway_address != 0u))
            {
                ret = NANO_IP_ROUTE_Add(0u, 0u, gateway_address, net_if);
            }
        }
        else
        {
            ret = NIP_ERR_NETIF_NOT_FOUND;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Get informations about a network interface */
nano_ip_error_t NANO_IP_NET_IFACES_GetInfo(const uint8_t iface, const char** const name, ipv4_address_t* const address, ipv4_address_t* const netmask, ipv4_address_t* const gateway_address, uint8_t* const mac_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if ((iface < net_ifaces_module->net_ifaces_count) &&
        (name != NULL) &&
        (address != NULL) && 
        (netmask != NULL) &&
        (gateway_address != NULL) &&
        (mac_address != NULL))
    {
        nano_ip_net_if_t* net_if;

        /* Look for interface */
        net_if = NANO_IP_NET_IF_LookForNetIf(iface);
        if (net_if != NULL)
        {
            /* Copy information */
            (*name) = net_if->name;
            (*address) = net_if->ipv4_address;
            (*netmask) = net_if->ipv4_netmask;
            (*gateway_address) = 0u;
            (void)MEMCPY(mac_address, net_if->mac_address, MAC_ADDRESS_SIZE); 

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_NETIF_NOT_FOUND;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Look for a network interface */
static nano_ip_net_if_t* NANO_IP_NET_IF_LookForNetIf(const uint8_t iface)
{
    nano_ip_net_if_t* net_if = NULL;
    nano_ip_net_ifaces_module_data_t* const net_ifaces_module = &g_nano_ip.net_ifaces_module;

    net_if = net_ifaces_module->net_ifaces;
    while ((net_if != NULL) && (net_if->id != iface))
    {
        /* Next interface */
        net_if = net_if->next;
    }

    return net_if;
}
