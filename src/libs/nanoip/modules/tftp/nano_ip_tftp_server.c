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

#include "nano_ip_tftp_server.h"

#if( (NANO_IP_ENABLE_TFTP == 1) && (NANO_IP_ENABLE_TFTP_SERVER == 1) )

#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"


/** \brief TFTP server start flag */
#define TFTP_SERVER_START_FLAG      0x01u

/** \brief TFTP server stop flag */
#define TFTP_SERVER_STOP_FLAG       0x02u

/** \brief TFTP server frame received flag */
#define TFTP_SERVER_RX_FLAG         0x04u


/** \brief UDP event callback */
static bool NANO_IP_TFTP_SERVER_UdpEvent(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data);

/** \brief Process a received TFTP packet */
static void NANO_IP_TFTP_SERVER_ProcessPacket(nano_ip_tftp_server_t* const tftp_server, nano_ip_net_packet_t* const packet);


#if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
/** \brief TFTP server task */
static void NANO_IP_TFTP_SERVER_Task(void* const param);
#endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */




/** \brief Initialize a TFTP server instance */
nano_ip_error_t NANO_IP_TFTP_SERVER_Init(nano_ip_tftp_server_t* const tftp_server, const uint32_t listen_address, const uint16_t listen_port, const nano_ip_tftp_callbacks_t* const callbacks, void* const user_data, const uint32_t timeout)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((tftp_server != NULL) &&
        (callbacks != NULL))
    {
        /* 0 init */
        MEMSET(tftp_server, 0, sizeof(nano_ip_tftp_server_t));

        /* Initialize instance */
        ret = NANO_IP_TFTP_Init(&tftp_server->tftp_module, listen_address, listen_port, callbacks, user_data, timeout);
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_UDP_InitializeHandle(&tftp_server->tftp_module.udp_handle, NANO_IP_TFTP_SERVER_UdpEvent, tftp_server);
        }

        #if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_MUTEX_Create(&tftp_server->mutex);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_FLAGS_Create(&tftp_server->sync_flags);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_TASK_Create(&tftp_server->task, "TFTP Server", NANO_IP_TFTP_SERVER_Task, tftp_server);
        }
        #endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */
    }

    return ret;
}


/** \brief Start a TFTP server instance */
nano_ip_error_t NANO_IP_TFTP_SERVER_Start(nano_ip_tftp_server_t* const tftp_server)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (tftp_server != NULL)
    {
        /* Start TFTP */
        ret = NANO_IP_TFTP_Start(&tftp_server->tftp_module);

        #if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
        /* Start processing */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_FLAGS_Set(&tftp_server->sync_flags, TFTP_SERVER_START_FLAG, false);
        }
        #endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */
    }

    return ret;
}

/** \brief Stop a TFTP server instance */
nano_ip_error_t NANO_IP_TFTP_SERVER_Stop(nano_ip_tftp_server_t* const tftp_server)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (tftp_server != NULL)
    {
        /* Stop TFTP */
        ret = NANO_IP_TFTP_Stop(&tftp_server->tftp_module);

        #if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
        /* Stop processing */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_FLAGS_Set(&tftp_server->sync_flags, TFTP_SERVER_STOP_FLAG, false);
        }
        #endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */
    }

    return ret;
}


/** \brief UDP event callback */
static bool NANO_IP_TFTP_SERVER_UdpEvent(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data)
{
    bool release_packet = true;
    nano_ip_tftp_server_t* const tftp_server = NANO_IP_CAST(nano_ip_tftp_server_t*, user_data);

    /* Check parameters */
    if (tftp_server != NULL)
    {
        /* Check event */
        switch (event)
        {
            case UDP_EVENT_RX:
            {
                udp_header_t* const udp_header = event_data->udp_header;
                nano_ip_net_packet_t* const packet = event_data->packet;

                /* Save packet reference */
                tftp_server->tftp_module.dest_address = udp_header->ipv4_header->src_address;
                tftp_server->tftp_module.dest_port = udp_header->src_port;

                #if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
                
                /* Add packet to the list */
                (void)NANO_IP_OAL_MUTEX_Lock(&tftp_server->mutex);
                if (tftp_server->rx_packets == NULL)
                {
                    tftp_server->rx_packets = packet;
                }
                else
                {
                    nano_ip_net_packet_t* pkt = tftp_server->rx_packets;
                    while (pkt->next != NULL)
                    {
                        pkt = pkt->next;
                    }
                    pkt->next = packet;
                }
                packet->next = NULL;
                (void)NANO_IP_OAL_MUTEX_Unlock(&tftp_server->mutex);

                /* Signal TFTP server task */
                (void)NANO_IP_OAL_FLAGS_Set(&tftp_server->sync_flags, TFTP_SERVER_RX_FLAG, false);

                /* Do not automatically release packet,
                    packet will be released by the task */
                release_packet = false;
                
                #else
                
                /* Process packet */
                NANO_IP_TFTP_SERVER_ProcessPacket(tftp_server, packet);

                #endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */

                break;        
            }

            default:
            {
                /* Ignore */
                break;
            }
        }
    }
    
    return release_packet;
}


/** \brief Process a received TFTP packet */
static void NANO_IP_TFTP_SERVER_ProcessPacket(nano_ip_tftp_server_t* const tftp_server, nano_ip_net_packet_t* const packet)
{
    /* Check minimum length */
    if (packet->count >= TFTP_DATA_PACKET_HEADER_SIZE)
    {
        /* Decode header */
        uint16_t opcode = NANO_IP_PACKET_Read16bits(packet);
        switch (opcode)
        {
            case TFTP_REQ_READ:
                /* Read request */
                /* intended fallthrough */
            case TFTP_REQ_WRITE:
                /* Write request */
                NANO_IP_TFTP_ProcessReadWriteRequest(&tftp_server->tftp_module, packet, opcode);
                break;

            case TFTP_REQ_DATA:
                /* Data packet */
                NANO_IP_TFTP_ProcessDataPacket(&tftp_server->tftp_module, packet, TFTP_REQ_WRITE);
                break;

            case TFTP_REQ_ACK:
                /* Ack packet */
                NANO_IP_TFTP_ProcessAckPacket(&tftp_server->tftp_module, packet, TFTP_REQ_READ);
                break;

            case TFTP_REQ_ERROR:
                /* Error packet */
                NANO_IP_TFTP_ProcessErrorPacket(&tftp_server->tftp_module, packet);
                break;

            default:
                /* Ignore packet */
                break;
        }
    }
}


#if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
/** \brief TFTP server task */
static void NANO_IP_TFTP_SERVER_Task(void* const param)
{
    uint32_t flags;
    nano_ip_error_t err;
    nano_ip_tftp_server_t* const tftp_server = NANO_IP_CAST(nano_ip_tftp_server_t*, param);

    /* Task loop */
    #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
    while (true)
    #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
    {
        /* Wait start flag */
        flags = TFTP_SERVER_START_FLAG;
        err = NANO_IP_OAL_FLAGS_Wait(&tftp_server->sync_flags, &flags, true, NANO_IP_MAX_TIMEOUT_VALUE);
        if ((err == NIP_ERR_SUCCESS) && ((flags & TFTP_SERVER_START_FLAG) != 0u))
        {
            /* Started loop */
            while ((flags & TFTP_SERVER_STOP_FLAG) == 0u)
            {
                /* Wait end flag or receive flag */
                flags = TFTP_SERVER_RX_FLAG | TFTP_SERVER_STOP_FLAG;
                err = NANO_IP_OAL_FLAGS_Wait(&tftp_server->sync_flags, &flags, true, NANO_IP_MAX_TIMEOUT_VALUE);
                if ((err == NIP_ERR_SUCCESS) && ((flags & TFTP_SERVER_RX_FLAG) != 0u))
                {
                    nano_ip_net_packet_t* packet = NULL;
                    do
                    {
                        /* Get packet from the list */
                        (void)NANO_IP_OAL_MUTEX_Lock(&tftp_server->mutex);
                        packet = tftp_server->rx_packets;
                        if (packet != NULL)
                        {
                            tftp_server->rx_packets = packet->next;
                        }
                        (void)NANO_IP_OAL_MUTEX_Unlock(&tftp_server->mutex);
                        if (packet != NULL)
                        {
                            /* Process packet */
                            NANO_IP_TFTP_SERVER_ProcessPacket(tftp_server, packet);

                            /* Release packet */
                            (void)NANO_IP_UDP_ReleasePacket(packet);
                        }
                    }
                    while(packet != NULL);
                }
            }
        }
    }
}
#endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */

#endif /* NANO_IP_ENABLE_TFTP && NANO_IP_ENABLE_TFTP_SERVER */
