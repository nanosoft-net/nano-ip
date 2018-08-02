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

#include "nano_ip_localhost.h"

#if( NANO_IP_ENABLE_LOCALHOST == 1 )

#include "nano_ip_tools.h"
#include "nano_ip_oal.h"
#include "nano_ip_data.h"
#include "nano_ip_packet_funcs.h"


/** \brief Init the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks);
/** \brief Start the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvStart(void* const user_data);
/** \brief Stop the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvStop(void* const user_data);
/** \brief Send a packet on the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Add a packet for reception for the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Get the next received packet on the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_GetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the next sent packet on the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_GetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the link state of the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvGetLinkState(void* const user_data, net_link_state_t* const state);


/** \brief Localhost interface driver */
static const nano_ip_net_driver_t localhost_driver = {
                                                    0xFFFFFFFFu, /* Capabilities => all */
                                                    &g_nano_ip.localhost_module.driver, /* User defined data */

                                                    NANO_IP_LOCALHOST_DrvInit,
                                                    NANO_IP_LOCALHOST_DrvStart,
                                                    NANO_IP_LOCALHOST_DrvStop,
                                                    NULL, /* set_mac_address() */
                                                    NULL, /* set_ipv4_address() */
                                                    NANO_IP_LOCALHOST_DrvSendPacket,
                                                    NANO_IP_LOCALHOST_DrvAddRxPacket,
                                                    NANO_IP_LOCALHOST_GetNextRxPacket,
                                                    NANO_IP_LOCALHOST_GetNextTxPacket,
                                                    NANO_IP_LOCALHOST_DrvGetLinkState
                                             };



/** \brief Initialize localhost interface */
nano_ip_error_t NANO_IP_LOCALHOST_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_FAILURE;
    
    /* Add interface to the stack */
    g_nano_ip.localhost_module.net_if.driver = &localhost_driver;
    ret = NANO_IP_NET_IFACES_AddNetInterface(&g_nano_ip.localhost_module.net_if, "localhost", 0, 0);

    return ret;
}


/** \brief Init the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;
    localhost_drv_t* localhost_drv_inst = NANO_IP_CAST(localhost_drv_t*, user_data);

    /* Check parameters */
    if ((localhost_drv_inst != NULL) && (callbacks != NULL))
    {
        /* Create the interface's mutex */
        ret = NANO_IP_OAL_MUTEX_Create(&localhost_drv_inst->mutex);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Save the callbacks */
            (void)MEMCPY(&localhost_drv_inst->callbacks, callbacks, sizeof(net_driver_callbacks_t));
        }
    }

    return ret;
}

/** \brief Start the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvStart(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;
    localhost_drv_t* localhost_drv_inst = NANO_IP_CAST(localhost_drv_t*, user_data);
    (void)localhost_drv_inst;
    return ret;
}

/** \brief Stop the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvStop(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;
    localhost_drv_t* localhost_drv_inst = NANO_IP_CAST(localhost_drv_t*, user_data);
    (void)localhost_drv_inst;
    return ret;
}

/** \brief Send a packet on the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    localhost_drv_t* localhost_drv_inst = NANO_IP_CAST(localhost_drv_t*, user_data);

    /* Check parameters */
    if ((localhost_drv_inst != NULL) && (packet != NULL))
    {
        /* Lock interface */
        (void)NANO_IP_OAL_MUTEX_Lock(&localhost_drv_inst->mutex);

        /* Add packet to the received packet list */
        NANO_IP_PACKET_AddToQueue(localhost_drv_inst->received_packets, packet);

        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&localhost_drv_inst->mutex);

        /* Notify packet reception */
        localhost_drv_inst->callbacks.packet_received(localhost_drv_inst->callbacks.stack_data, false);

        /* Notify packet transmission */
        localhost_drv_inst->callbacks.packet_sent(localhost_drv_inst->callbacks.stack_data, false);
        
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Add a packet for reception for the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    const nano_ip_error_t ret = NIP_ERR_FAILURE;
    (void)user_data;
    (void)packet;
    return ret;
}

/** \brief Get the last received packet on the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_GetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    localhost_drv_t* localhost_drv_inst = NANO_IP_CAST(localhost_drv_t*, user_data);

    /* Check parameters */
    if ((localhost_drv_inst != NULL) && (packet != NULL))
    {
        /* Lock interface */
        (void)NANO_IP_OAL_MUTEX_Lock(&localhost_drv_inst->mutex);

        /* Remove last received packet from the list */
        (*packet) = NANO_IP_PACKET_PopFromQueue(localhost_drv_inst->received_packets);
        if ((*packet) != NULL)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }

        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&localhost_drv_inst->mutex);
    }

    return ret;
}

/** \brief Get the last sent packet on the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_GetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    const nano_ip_error_t ret = NIP_ERR_PACKET_NOT_FOUND;
    (void)user_data;
    (void)packet;
    return ret;
}


/** \brief Get the link state of the localhost interface driver */
static nano_ip_error_t NANO_IP_LOCALHOST_DrvGetLinkState(void* const user_data, net_link_state_t* const state)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    localhost_drv_t* localhost_drv_inst = NANO_IP_CAST(localhost_drv_t*, user_data);

    /* Check parameters */
    if ((localhost_drv_inst != NULL) && (state != NULL))
    {
        (*state) = NLS_UP_1000_FD;
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

#endif /* NANO_IP_ENABLE_LOCALHOST */
