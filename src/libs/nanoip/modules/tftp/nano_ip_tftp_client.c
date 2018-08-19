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

#include "nano_ip_tftp_client.h"

#if( (NANO_IP_ENABLE_TFTP == 1) && (NANO_IP_ENABLE_TFTP_CLIENT == 1) )

#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"


/** \brief TFTP client start flag */
#define TFTP_CLIENT_START_FLAG      0x01u

/** \brief TFTP client stop flag */
#define TFTP_CLIENT_STOP_FLAG       0x02u

/** \brief TFTP client frame received flag */
#define TFTP_CLIENT_RX_FLAG         0x04u



/** \brief UDP event callback */
static bool NANO_IP_TFTP_CLIENT_UdpEvent(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data);

/** \brief Process a received TFTP packet */
static void NANO_IP_TFTP_CLIENT_ProcessPacket(nano_ip_tftp_client_t* const tftp_client, nano_ip_net_packet_t* const packet);


#if (NANO_IP_ENABLE_TFTP_CLIENT_TASK == 1u)
/** \brief TFTP client task */
static void NANO_IP_TFTP_CLIENT_Task(void* const param);
#endif /* NANO_IP_ENABLE_TFTP_CLIENT_TASK */




/** \brief Initialize a TFTP client instance */
nano_ip_error_t NANO_IP_TFTP_CLIENT_Init(nano_ip_tftp_client_t* const tftp_client, const uint32_t listen_address, const uint16_t listen_port, const nano_ip_tftp_callbacks_t* const callbacks, void* const user_data, const uint32_t timeout)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((tftp_client != NULL) &&
        (callbacks != NULL))
    {
        /* 0 init */
        NANO_IP_MEMSET(tftp_client, 0, sizeof(nano_ip_tftp_client_t));

        /* Initialize instance */
        ret = NANO_IP_TFTP_Init(&tftp_client->tftp_module, listen_address, listen_port, callbacks, user_data, timeout);
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_UDP_InitializeHandle(&tftp_client->tftp_module.udp_handle, NANO_IP_TFTP_CLIENT_UdpEvent, tftp_client);
        }

        #if (NANO_IP_ENABLE_TFTP_CLIENT_TASK == 1u)
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_MUTEX_Create(&tftp_client->mutex);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_FLAGS_Create(&tftp_client->sync_flags);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_TASK_Create(&tftp_client->task, "TFTP Client", NANO_IP_TFTP_CLIENT_Task, tftp_client, NANO_IP_TFTP_CLIENT_TASK_PRIORITY, NANO_IP_TFTP_CLIENT_TASK_STACK_SIZE);
        }
        #endif /* NANO_IP_ENABLE_TFTP_CLIENT_TASK */
    }

    return ret;
}


/** \brief Start a TFTP client instance */
nano_ip_error_t NANO_IP_TFTP_CLIENT_Start(nano_ip_tftp_client_t* const tftp_client)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (tftp_client != NULL)
    {
        /* Start TFTP */
        ret = NANO_IP_TFTP_Start(&tftp_client->tftp_module);

        #if (NANO_IP_ENABLE_TFTP_CLIENT_TASK == 1u)
        /* Start processing */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_FLAGS_Set(&tftp_client->sync_flags, TFTP_CLIENT_START_FLAG, false);
        }
        #endif /* NANO_IP_ENABLE_TFTP_CLIENT_TASK */
    }

    return ret;
}

/** \brief Stop a TFTP client instance */
nano_ip_error_t NANO_IP_TFTP_CLIENT_Stop(nano_ip_tftp_client_t* const tftp_client)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (tftp_client != NULL)
    {
        /* Stop TFTP */
        ret = NANO_IP_TFTP_Stop(&tftp_client->tftp_module);

        #if (NANO_IP_ENABLE_TFTP_CLIENT_TASK == 1u)
        /* Stop processing */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_FLAGS_Set(&tftp_client->sync_flags, TFTP_CLIENT_STOP_FLAG, false);
        }
        #endif /* NANO_IP_ENABLE_TFTP_CLIENT_TASK */
    }

    return ret;
}

/** \brief Send a TFTP read request */
nano_ip_error_t NANO_IP_TFTP_CLIENT_Read(nano_ip_tftp_client_t* const tftp_client, const uint32_t server_address, const uint16_t server_port, const char* const filename)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((tftp_client != NULL) && (tftp_client->tftp_module.req_type == TFTP_REQ_IDLE))
    {
        /* Send request */
        ret = NANO_IP_TFTP_SendReadWriteRequest(&tftp_client->tftp_module, server_address, server_port, filename, TFTP_REQ_READ);
    }

    return ret;
}

/** \brief Send a TFTP write request */
nano_ip_error_t NANO_IP_TFTP_CLIENT_Write(nano_ip_tftp_client_t* const tftp_client, const uint32_t server_address, const uint16_t server_port, const char* const filename)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((tftp_client != NULL) && (tftp_client->tftp_module.req_type == TFTP_REQ_IDLE))
    {
        /* Send request */
        ret = NANO_IP_TFTP_SendReadWriteRequest(&tftp_client->tftp_module, server_address, server_port, filename, TFTP_REQ_WRITE);
    }

    return ret;
}

/** \brief UDP event callback */
static bool NANO_IP_TFTP_CLIENT_UdpEvent(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data)
{
    bool release_packet = true;
    nano_ip_tftp_client_t* const tftp_client = NANO_IP_CAST(nano_ip_tftp_client_t*, user_data);

    /* Check parameters */
    if (tftp_client != NULL)
    {
        /* Check event */
        switch (event)
        {
            case UDP_EVENT_RX:
            {
                udp_header_t* const udp_header = event_data->udp_header;
                nano_ip_net_packet_t* const packet = event_data->packet;

                /* Save packet reference */
                tftp_client->tftp_module.dest_address = udp_header->ipv4_header->src_address;
                tftp_client->tftp_module.dest_port = udp_header->src_port;

                #if (NANO_IP_ENABLE_TFTP_CLIENT_TASK == 1u)

                /* Add packet to the list */
                (void)NANO_IP_OAL_MUTEX_Lock(&tftp_client->mutex);
                if (tftp_client->rx_packets == NULL)
                {
                    tftp_client->rx_packets = packet;
                }
                else
                {
                    nano_ip_net_packet_t* pkt = tftp_client->rx_packets;
                    while (pkt->next != NULL)
                    {
                        pkt = pkt->next;
                    }
                    pkt->next = packet;
                }
                packet->next = NULL;
                (void)NANO_IP_OAL_MUTEX_Unlock(&tftp_client->mutex);

                /* Signal TFTP client task */
                (void)NANO_IP_OAL_FLAGS_Set(&tftp_client->sync_flags, TFTP_CLIENT_RX_FLAG, false);

                /* Do not automatically release packet,
                packet will be released by the task */
                release_packet = false;

                #else

                /* Process packet */
                NANO_IP_TFTP_CLIENT_ProcessPacket(tftp_client, packet);

                #endif /* NANO_IP_ENABLE_TFTP_CLIENT_TASK */

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
static void NANO_IP_TFTP_CLIENT_ProcessPacket(nano_ip_tftp_client_t* const tftp_client, nano_ip_net_packet_t* const packet)
{
    /* Check minimum length */
    if (packet->count >= TFTP_DATA_PACKET_HEADER_SIZE)
    {
        /* Decode header */
        uint16_t opcode = NANO_IP_PACKET_Read16bits(packet);
        switch (opcode)
        {
            case TFTP_REQ_DATA:
                /* Data packet */
                NANO_IP_TFTP_ProcessDataPacket(&tftp_client->tftp_module, packet, TFTP_REQ_READ);
                break;

            case TFTP_REQ_ACK:
                /* Ack packet */
                NANO_IP_TFTP_ProcessAckPacket(&tftp_client->tftp_module, packet, TFTP_REQ_WRITE);
                break;

            case TFTP_REQ_ERROR:
                /* Error packet */
                NANO_IP_TFTP_ProcessErrorPacket(&tftp_client->tftp_module, packet);
                break;

            default:
                /* Ignore packet */
                break;
        }
    }
}


#if (NANO_IP_ENABLE_TFTP_CLIENT_TASK == 1u)
/** \brief TFTP client task */
static void NANO_IP_TFTP_CLIENT_Task(void* const param)
{
    uint32_t flags;
    nano_ip_error_t err;
    nano_ip_tftp_client_t* const tftp_client = NANO_IP_CAST(nano_ip_tftp_client_t*, param);

    /* Task loop */
    #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
    while (true)
    #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
    {
        /* Wait start flag */
        flags = TFTP_CLIENT_START_FLAG;
        err = NANO_IP_OAL_FLAGS_Wait(&tftp_client->sync_flags, &flags, true, NANO_IP_MAX_TIMEOUT_VALUE);
        if ((err == NIP_ERR_SUCCESS) && ((flags & TFTP_CLIENT_START_FLAG) != 0u))
        {
            /* Started loop */
            while ((flags & TFTP_CLIENT_STOP_FLAG) == 0u)
            {
                /* Wait end flag or receive flag */
                flags = TFTP_CLIENT_RX_FLAG | TFTP_CLIENT_STOP_FLAG;
                err = NANO_IP_OAL_FLAGS_Wait(&tftp_client->sync_flags, &flags, true, NANO_IP_MAX_TIMEOUT_VALUE);
                if ((err == NIP_ERR_SUCCESS) && ((flags & TFTP_CLIENT_RX_FLAG) != 0u))
                {
                    nano_ip_net_packet_t* packet = NULL;
                    do
                    {
                        /* Get packet from the list */
                        (void)NANO_IP_OAL_MUTEX_Lock(&tftp_client->mutex);
                        packet = tftp_client->rx_packets;
                        if (packet != NULL)
                        {
                            tftp_client->rx_packets = packet->next;
                        }
                        (void)NANO_IP_OAL_MUTEX_Unlock(&tftp_client->mutex);
                        if (packet != NULL)
                        {
                            /* Process packet */
                            NANO_IP_TFTP_CLIENT_ProcessPacket(tftp_client, packet);

                            /* Release packet */
                            (void)NANO_IP_UDP_ReleasePacket(packet);
                        }
                    } 
                    while (packet != NULL);
                }
            }
        }
    }
}
#endif /* NANO_IP_ENABLE_TFTP_CLIENT_TASK */

#endif /* NANO_IP_ENABLE_TFTP && NANO_IP_ENABLE_TFTP_CLIENT */
