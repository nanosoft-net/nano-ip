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

#include "nano_ip_net_if.h"
#include "nano_ip_tools.h"
#include "nano_ip_ethernet.h"
#include "nano_ip_data.h"


/** \brief Period in milliseconds of the network interface periodic tasks */
#define NET_IF_PERIODIC_TASK_PERIOD     250u



/** \brief Network interface event flags */
typedef enum _nano_ip_net_if_event_flags_t
{
    /** \brief Packet received */
    NETIF_PACKET_RECEIVED = 1u,
    /** \brief Packet sent */
    NETIF_PACKET_SENT = 2u,
    /** \brief Driver error */
    NETIF_DRV_ERROR = 4u,
    /** \brief Link state changed */
    NETIF_LINK_STATE_CHANGED = 8u,
    /** \brief Periodic timer */
    NETIF_PERIODIC_TIMER = 16u
} nano_ip_net_if_event_flags_t;



/** \brief Rx task */
static void NANO_IP_NET_IF_RxTask(void* param);

/** \brief Called when a packet has been received */
static void NANO_IP_NET_IF_PacketReceivedCallback(void* const stack_data, const bool from_isr);
/** \brief Called when a packet has been sent */
static void NANO_IP_NET_IF_PacketSentCallback(void* const stack_data, const bool from_isr);
/** \brief Called when a network driver error occured */
static void NANO_IP_NET_IF_NetDrvErrorCallback(void* const stack_data, const bool from_isr);
/** \brief Called when the link state has changed */
static void NANO_IP_NET_IF_LinkStateChangedCallback(void* const stack_data, const bool from_isr);
/** \brief Called when the periodic timer has elapsed */
static void NANO_IP_NET_IF_PeriodicTimerCallback(oal_timer_t* const timer, void* const user_data);


/** \brief Initialize a network interface */
nano_ip_error_t NANO_IP_NET_IF_Init(nano_ip_net_if_t* const net_if, const char* const name, 
                                    const uint32_t rx_packet_count, const uint32_t rx_packet_size,
                                    const uint8_t task_priority, const uint32_t task_stack_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((net_if != NULL) && 
        (net_if->driver != NULL) &&
        (name != NULL) )
    {
        const nano_ip_net_driver_t* const net_driver = net_if->driver;

        /* 0 init */
        NANO_IP_MEMSET(net_if, 0, sizeof(nano_ip_net_if_t));
        net_if->driver = net_driver;

        /* Save name */
        net_if->name = name;
        
        /* Create network interface synchronization flags */
        ret = NANO_IP_OAL_FLAGS_Create(&net_if->sync_flags);

        /* Create network interface timer */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_TIMER_Create(&net_if->timer, NANO_IP_NET_IF_PeriodicTimerCallback, net_if);
        }

        /* Initialize network interface driver */
        if (ret == NIP_ERR_SUCCESS)
        {
            net_driver_callbacks_t callbacks;
            callbacks.packet_received = NANO_IP_NET_IF_PacketReceivedCallback;
            callbacks.packet_sent = NANO_IP_NET_IF_PacketSentCallback;
            callbacks.net_drv_error = NANO_IP_NET_IF_NetDrvErrorCallback;
            callbacks.link_state_changed = NANO_IP_NET_IF_LinkStateChangedCallback;
            callbacks.stack_data = net_if;
            ret = net_if->driver->init(net_if->driver->user_data, &callbacks);
        }

        /* Allocate Rx packets */
        if (ret == NIP_ERR_SUCCESS)
        {
            uint32_t i;
            nano_ip_net_packet_t* packet;
            for (i = 0; (i < rx_packet_count) && (ret == NIP_ERR_SUCCESS); i++)
            {
                ret = g_nano_ip.packet_allocator->allocate(g_nano_ip.packet_allocator->allocator_data, &packet, rx_packet_size);
                if (ret == NIP_ERR_SUCCESS)
                {
                    packet->flags = NET_IF_PACKET_FLAG_RX;
                    ret = net_if->driver->add_rx_packet(net_if->driver->user_data, packet);
                }
            }
        }

        /* Create the Rx task */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_TASK_Create(&net_if->task, "NanoIP NANO_IP_NET_IF_RxTask()", NANO_IP_NET_IF_RxTask, net_if, task_priority, task_stack_size);
        }

    }

    return ret;
}

/** \brief Bring up a network interface */
nano_ip_error_t NANO_IP_NET_IF_Up(nano_ip_net_if_t* const net_if)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (net_if != NULL)
    {
        /* Start the network interface */
        ret = net_if->driver->start(net_if->driver->user_data);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Start the timer */
            ret = NANO_IP_OAL_TIMER_Start(&net_if->timer, NET_IF_PERIODIC_TASK_PERIOD);
        }
    }

    return ret;
}

/** \brief Bring down a network interface */
nano_ip_error_t NANO_IP_NET_IF_Down(nano_ip_net_if_t* const net_if)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (net_if != NULL)
    {
        /* Stop the network interface */
        ret = net_if->driver->stop(net_if->driver->user_data);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Stop the timer */
            ret = NANO_IP_OAL_TIMER_Stop(&net_if->timer);
        }
    }

    return ret;
}

/** \brief Set the MAC address of a network interface */
nano_ip_error_t NANO_IP_NET_IF_SetMacAddress(nano_ip_net_if_t* const net_if, const uint8_t* const mac_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (net_if != NULL)
    {
        /* Update MAC address */
        (void)NANO_IP_MEMCPY(net_if->mac_address, mac_address, MAC_ADDRESS_SIZE);

        /* Set the MAC addresss in the driver if the implementation allows it */
        if (net_if->driver->set_mac_address != NULL)
        {
            ret = net_if->driver->set_mac_address(net_if->driver->user_data, mac_address);
        }
    }

    return ret;
}

/** \brief Set the IPv4 address of a network interface */
nano_ip_error_t NANO_IP_NET_IF_SetIpv4Address(nano_ip_net_if_t* const net_if, const ipv4_address_t address, const ipv4_address_t netmask)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (net_if != NULL)
    {
        /* Update IP address and netmask */
        net_if->ipv4_address = address;
        net_if->ipv4_netmask = netmask;

        /* Set the IP addresss in the driver if the implementation allows it */
        if (net_if->driver->set_ipv4_address != NULL)
        {
            ret = net_if->driver->set_ipv4_address(net_if->driver->user_data, address, netmask);
        }
    }

    return ret;
}


/** \brief Rx task */
static void NANO_IP_NET_IF_RxTask(void* param)
{
    nano_ip_net_if_t* const net_if = NANO_IP_CAST(nano_ip_net_if_t*, param);

    /* Task loop */
    if (net_if != NULL)
    {
        #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
        while (true)
        #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
        {
            uint32_t flags = NANO_IP_OAL_FLAGS_ALL;
            bool check_link_state = false;

            /* Wait for a network interface event */
            nano_ip_error_t ret = NANO_IP_OAL_FLAGS_Wait(&net_if->sync_flags, &flags, true, NANO_IP_MAX_TIMEOUT_VALUE);
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Check flags */
                if ((flags & NETIF_PACKET_RECEIVED) != 0u)
                {
                    /* Packet received */
                    nano_ip_net_packet_t* packet = NULL;
                    do
                    {
                        /* Get received packet */
                        ret = net_if->driver->get_next_rx_packet(net_if->driver->user_data, &packet);
                        if (ret == NIP_ERR_SUCCESS)
                        {
                            /* Decode packet */
                            packet->net_if = net_if;
                            if ((packet->flags & NET_IF_PACKET_FLAG_ERROR) == 0u)
                            {
                                (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                                (void)NANO_IP_ETHERNET_RxFrame(net_if, packet);
                                (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                            }

                            /* Check if packet is still in use */
                            if ((packet->flags & NET_IF_PACKET_FLAG_KEEP_PACKET) == 0u)
                            {
                                if ((packet->flags & NET_IF_PACKET_FLAG_TX) == 0u)
                                {
                                    /* Requeue packet for reception */
                                    packet->flags = NET_IF_PACKET_FLAG_RX;
                                    packet->current = NANO_IP_CAST(uint8_t*, packet->data);
                                    (void)net_if->driver->add_rx_packet(net_if->driver->user_data, packet);
                                }
                                else
                                {
                                    /* Release packet */
                                    (void)g_nano_ip.packet_allocator->release(g_nano_ip.packet_allocator->allocator_data, packet);
                                }
                            }
                        }
                    } while (ret == NIP_ERR_SUCCESS);
                }
                if ((flags & NETIF_PACKET_SENT) != 0u)
                {
                    /* Packet sent */
                    nano_ip_net_packet_t* packet = NULL;
                    do
                    {
                        /* Get sent packet */
                        ret = net_if->driver->get_next_tx_packet(net_if->driver->user_data, &packet);
                        if (ret == NIP_ERR_SUCCESS)
                        {
                            /* Release packet */
                            if ((packet->flags & NET_IF_PACKET_FLAG_KEEP_PACKET) == 0u)
                            {
                                ret = g_nano_ip.packet_allocator->release(g_nano_ip.packet_allocator->allocator_data, packet);
                            }
                        }
                    } 
                    while (ret == NIP_ERR_SUCCESS);
                }
                if ((flags & NETIF_DRV_ERROR) != 0u)
                {
                    /* Driver error */
                }
                if ((flags & NETIF_LINK_STATE_CHANGED) != 0u)
                {
                    /* Link state event */
                    check_link_state = true;
                }
                if ((flags & NETIF_PERIODIC_TIMER) != 0u)
                {
                    /* Periodic timer */
                    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                    ret = NANO_IP_ETHERNET_PeriodicTask();
                    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                    check_link_state = true;
                }
                
                /* Check link state */
                if (check_link_state)
                {
                    net_link_state_t link_state = NLS_DOWN;
                    ret = net_if->driver->get_link_state(net_if->driver->user_data, &link_state);
                    if (ret == NIP_ERR_SUCCESS)
                    {
                        if (link_state != net_if->link_state)
                        {
                            NANO_IP_LOG_INFO("[%s] : link state => %d", net_if->name, link_state);
                            net_if->link_state = link_state;
                        }
                    }
                }
            }
        }
    }
}


/** \brief Called when a packet has been received */
static void NANO_IP_NET_IF_PacketReceivedCallback(void* const stack_data, const bool from_isr)
{
    nano_ip_net_if_t* const net_if = NANO_IP_CAST(nano_ip_net_if_t*, stack_data);

    /* Check parameters */
    if (net_if != NULL)
    {
        (void)NANO_IP_OAL_FLAGS_Set(&net_if->sync_flags, NETIF_PACKET_RECEIVED, from_isr);
    }
}

/** \brief Called when a packet has been sent */
static void NANO_IP_NET_IF_PacketSentCallback(void* const stack_data, const bool from_isr)
{
    nano_ip_net_if_t* const net_if = NANO_IP_CAST(nano_ip_net_if_t*, stack_data);

    /* Check parameters */
    if (net_if != NULL)
    {
        (void)NANO_IP_OAL_FLAGS_Set(&net_if->sync_flags, NETIF_PACKET_SENT, from_isr);
    }
}

/** \brief Called when a network driver error occured */
static void NANO_IP_NET_IF_NetDrvErrorCallback(void* const stack_data, const bool from_isr)
{
    nano_ip_net_if_t* const net_if = NANO_IP_CAST(nano_ip_net_if_t*, stack_data);

    /* Check parameters */
    if (net_if != NULL)
    {
        (void)NANO_IP_OAL_FLAGS_Set(&net_if->sync_flags, NETIF_DRV_ERROR, from_isr);
    }
}

/** \brief Called when the link state has changed */
static void NANO_IP_NET_IF_LinkStateChangedCallback(void* const stack_data, const bool from_isr)
{
    nano_ip_net_if_t* const net_if = NANO_IP_CAST(nano_ip_net_if_t*, stack_data);

    /* Check parameters */
    if (net_if != NULL)
    {
        (void)NANO_IP_OAL_FLAGS_Set(&net_if->sync_flags, NETIF_LINK_STATE_CHANGED, from_isr);
    }
}

/** \brief Called when the periodic timer has elapsed */
static void NANO_IP_NET_IF_PeriodicTimerCallback(oal_timer_t* const timer, void* const user_data)
{
    nano_ip_net_if_t* const net_if = NANO_IP_CAST(nano_ip_net_if_t*, user_data);
    (void)timer;

    /* Check parameters */
    if (net_if != NULL)
    {
        (void)NANO_IP_OAL_FLAGS_Set(&net_if->sync_flags, NETIF_PERIODIC_TIMER, false);
    }
}
