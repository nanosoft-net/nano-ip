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

#include "nano_ip_tcp.h"

#if( NANO_IP_ENABLE_TCP == 1 )

#include "nano_ip_data.h"
#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"


/** \brief TCP protocol id */
#define TCP_PROTOCOL                        0x06u


/** \brief TCP flag ECN */
#define TCP_FLAG_ECN                        (3u << 6u)

/** \brief TCP flag URG */
#define TCP_FLAG_URG                        (1u << 5u)

/** \brief TCP flag ACK */
#define TCP_FLAG_ACK                        (1u << 4u)

/** \brief TCP flag PSH */
#define TCP_FLAG_PSH                        (1u << 3u)

/** \brief TCP flag RST */
#define TCP_FLAG_RST                        (1u << 2u)

/** \brief TCP flag SYN */
#define TCP_FLAG_SYN                        (1u << 1u)

/** \brief TCP flag FIN */
#define TCP_FLAG_FIN                        (1u << 0u)



/** \brief TCP header size in bytes */
#define TCP_HEADER_SIZE                     0x14u

/** \brief TCP header size in 32bits words (data offset) */
#define TCP_HEADER_DATA_OFFSET              0x50u

/** \brief TCP pseudo header size in bytes */
#define TCP_PSEUDO_HEADER_SIZE              0x0Cu

/** \brief TCP window size */
#define TCP_WINDOW_SIZE                     1024u

/** \brief Max retry count for transmitted packets */
#define TCP_MAX_RETRY_COUNT                 5u

/** \brief Timeout in milliseconds for a TCP state */
#define TCP_STATE_TIMEOUT                   500u

/** \brief TCP port pool start  */
#define TCP_PORT_POOL_START                 10000u




/** \brief Handle an IPv4 error */
static void NANO_IP_TCP_Ipv4ErrorCallback(void* const user_data, const nano_ip_error_t error);

/** \brief Remove a handle from the handle list */
static void NANO_IP_TCP_RemoveHandle(nano_ip_tcp_handle_t* const handle);

/** \brief Handle a received TCP frame */
static nano_ip_error_t NANO_IP_TCP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet);

/** \brief TCP periodic task */
static void NANO_IP_TCP_PeriodicTask(const uint32_t timestamp, void* const user_data);

/** \brief Compute the TCP checksum of a buffer */
static uint16_t NANO_IP_TCP_ComputeCS(const ipv4_header_t* const ipv4_header, uint8_t* const buffer, const uint16_t size);

/** \brief Send a TCP control frame */
static nano_ip_error_t NANO_IP_TCP_SendControlFrame(nano_ip_tcp_handle_t* const handle, const uint8_t flags);

/** \brief Finalize and send a TCP packet */
static nano_ip_error_t NANO_IP_TCP_FinalizeAndSendPacket(nano_ip_tcp_handle_t* const handle, const uint8_t flags, nano_ip_net_packet_t* const packet);




/** \brief Initialize the TCP module */
nano_ip_error_t NANO_IP_TCP_Init(void)
{
    nano_ip_error_t ret;
    nano_ip_tcp_module_data_t* const tcp_module = &g_nano_ip.tcp_module; 

    /* Initialize port pool */
    tcp_module->next_free_local_port = TCP_PORT_POOL_START + NANO_IP_CAST(uint16_t, (NANO_IP_OAL_TIME_GetMsCounter() & 0xFFFFu));

    /* Register protocol */
    tcp_module->ipv4_protocol.protocol = TCP_PROTOCOL;
    tcp_module->ipv4_protocol.rx_frame = NANO_IP_TCP_RxFrame;
    tcp_module->ipv4_protocol.user_data = tcp_module;
    ret = NANO_IP_IPV4_AddProtocol(&tcp_module->ipv4_protocol);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Register periodic callback */
        tcp_module->ipv4_callback.callback = NANO_IP_TCP_PeriodicTask;
        tcp_module->ipv4_callback.user_data = tcp_module;
        ret = NANO_IP_IPV4_RegisterPeriodicCallback(&tcp_module->ipv4_callback);
    }

    return ret;
}


/** \brief Initialize a TCP handle */
nano_ip_error_t NANO_IP_TCP_InitializeHandle(nano_ip_tcp_handle_t* const handle, const fp_tcp_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((handle != NULL) && (callback != NULL))
    {
        /* 0 init */
        MEMSET(handle, 0, sizeof(nano_ip_tcp_handle_t));

        /* Initialize IPv4 handle */
        ret = NANO_IP_IPV4_InitializeHandle(&handle->ipv4_handle, handle, NANO_IP_TCP_Ipv4ErrorCallback);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Initialize handle */
            handle->callback = callback;
            handle->user_data = user_data;
        }
    }
    
    return ret;
}

/** \brief Release a TCP handle */
nano_ip_error_t NANO_IP_TCP_ReleaseHandle(nano_ip_tcp_handle_t* const handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (handle != NULL)
    {
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        /* Check handle state */
        if (handle->state == TCP_STATE_CLOSED)
        {
            /* Release IPv4 handle */
            ret = NANO_IP_IPV4_ReleaseHandle(&handle->ipv4_handle);
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Open a TCP connection */
nano_ip_error_t NANO_IP_TCP_Open(nano_ip_tcp_handle_t* const handle, const uint16_t local_port)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (handle != NULL)
    {
        /* Check handle state */
        if (handle->state == TCP_STATE_CLOSED)
        {
            nano_ip_tcp_module_data_t* const tcp_module = &g_nano_ip.tcp_module;

            /* Update handle state */
            handle->state = TCP_STATE_IDLE;

            /* Bind the handle */
            handle->port = 0u;
            do
            {
                if (local_port == 0u)
                {
                    ret = NANO_IP_TCP_Bind(handle, IPV4_ANY_ADDRESS, tcp_module->next_free_local_port);
                    
                    /* Next free port */
                    tcp_module->next_free_local_port += NANO_IP_CAST(uint16_t, (NANO_IP_OAL_TIME_GetMsCounter() & 0xFFFFu));
                    if (tcp_module->next_free_local_port == 0u)
                    {
                        tcp_module->next_free_local_port = TCP_PORT_POOL_START;
                    }
                }
                else
                {
                    ret = NANO_IP_TCP_Bind(handle, IPV4_ANY_ADDRESS, local_port);
                }
            }
            while ((local_port == 0u) && (handle->port == 0u));
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Update handle state */
                handle->state = TCP_STATE_IDLE;

                /* Add the handle to the list */
                handle->next = tcp_module->handles;
                tcp_module->handles = handle;
            }
            else
            {
                /* Update handle state */
                handle->state = TCP_STATE_CLOSED;
            }
        }
        else
        {
            ret = NIP_ERR_INVALID_TCP_STATE;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Bind a TCP handle to a specific address and port */
nano_ip_error_t NANO_IP_TCP_Bind(nano_ip_tcp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((handle != NULL) && (port != 0u))
    {
        nano_ip_tcp_handle_t* hdl;
        nano_ip_tcp_module_data_t* const tcp_module = &g_nano_ip.tcp_module;

        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        /* Check handle state */
        if (handle->state == TCP_STATE_IDLE)
        {
            /* Check if the pair (address, port) is already in use */
            hdl = tcp_module->handles;
            while ((hdl != NULL) && ((hdl->port != port) || (hdl->ipv4_address != ipv4_address)))
            {
                hdl = hdl->next;
            }
            if (hdl == NULL)
            {
                /* Bind the handle */
                handle->ipv4_address = ipv4_address;
                handle->port = port;

                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                ret = NIP_ERR_ADDRESS_IN_USE;
            }
        }
        else
        {
            ret = NIP_ERR_INVALID_TCP_STATE;
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Establish a TCP connection */
nano_ip_error_t NANO_IP_TCP_Connect(nano_ip_tcp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (handle != NULL)
    {
        /* Check handle state */
        if (handle->state == TCP_STATE_IDLE)
        {
            /* Save connexion endpoint */
            handle->dest_ipv4_address = ipv4_address;
            handle->dest_port = port;

            /* Initialize sequence number */
            handle->seq_number = NANO_IP_OAL_TIME_GetMsCounter();

            /* Send SYN frame */
            ret = NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_SYN);
            if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
            {
                /* Update handle state */
                handle->state = TCP_STATE_SYN_SENT;

                /* Update sequence number */
                handle->seq_number++;

                /* Initiate timeout */
                handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
            }
        }
        else
        {
            ret = NIP_ERR_INVALID_TCP_STATE;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Put a TCP handle into the listen state */
nano_ip_error_t NANO_IP_TCP_Listen(nano_ip_tcp_handle_t* const handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (handle != NULL)
    {
        /* Check handle state */
        if (handle->state == TCP_STATE_IDLE)
        {
            /* Update handle state */
            handle->state = TCP_STATE_LISTEN;

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_INVALID_TCP_STATE;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Close a TCP connection */
nano_ip_error_t NANO_IP_TCP_Close(nano_ip_tcp_handle_t* const handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (handle != NULL)
    {
        /* Check handle state */
        if (handle->state == TCP_STATE_ESTABLISHED)
        {
            /* Send FIN frame */
            ret = NANO_IP_TCP_SendControlFrame(handle, (TCP_FLAG_FIN | TCP_FLAG_ACK));
            if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
            {
                /* Update handle state */
                handle->state = TCP_STATE_FIN_WAIT_1;

                /* Update sequence number */
                handle->seq_number++;

                /* Initiate timeout */
                handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
            }
        }
        if (handle->state != TCP_STATE_IDLE)
        {
            /* Force connection close */
            handle->state = TCP_STATE_CLOSED;
            NANO_IP_TCP_RemoveHandle(handle);
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_INVALID_TCP_STATE;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Allocate a packet for a TCP frame */
nano_ip_error_t NANO_IP_TCP_AllocatePacket(nano_ip_net_packet_t** packet, const uint16_t packet_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        /* Compute total packet size needed */
        const uint16_t total_packet_size = packet_size + TCP_HEADER_SIZE;

        /* Try to allocate an IPv4 packet */
        ret = NANO_IP_IPV4_AllocatePacket(total_packet_size, packet);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Skip header */
            NANO_IP_PACKET_WriteSkipBytes((*packet), TCP_HEADER_SIZE);

            /* Reset count */
            (*packet)->count = 0;
        }
    }

    return ret;
}

/** \brief Send a TCP frame */
nano_ip_error_t NANO_IP_TCP_SendPacket(nano_ip_tcp_handle_t* const handle, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if ((handle != NULL) && (packet != NULL))
    {
        /* Check handle state */
        if (handle->last_tx_packet != NULL)
        {
            ret = NIP_ERR_BUSY;
        }
        else if (handle->state == TCP_STATE_ESTABLISHED)
        {
            /* Save packet informations */
            handle->last_tx_packet = packet;
            handle->last_tx_packet_pos = packet->current;
            handle->last_tx_packet_count = packet->count;
            handle->tx_retry_count = 0u;

            /* Keep packet for retransmission */
            handle->last_tx_packet->flags |= NET_IF_PACKET_FLAG_KEEP_PACKET;

            /* Send frame */
            ret = NANO_IP_TCP_FinalizeAndSendPacket(handle, (TCP_FLAG_PSH | TCP_FLAG_ACK), packet);
            if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
            {
                /* Update sequence number */
                handle->seq_number += handle->last_tx_packet_count;
            }
        }
        else
        {
            ret = NIP_ERR_INVALID_TCP_STATE;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Indicate if a TCP handle is ready */
nano_ip_error_t NANO_IP_TCP_HandleIsReady(nano_ip_tcp_handle_t* const handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (handle != NULL)
    {
        if (handle->last_tx_packet == NULL)
        {
            ret = NANO_IP_IPV4_HandleIsReady(&handle->ipv4_handle);
        }
        else
        {
            ret = NIP_ERR_BUSY;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Release a TCP frame */
nano_ip_error_t NANO_IP_TCP_ReleasePacket(nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        /* Release packet */
        ret = NANO_IP_IPV4_ReleasePacket(packet);
    }

    return ret;
}



/** \brief Handle an IPv4 error */
static void NANO_IP_TCP_Ipv4ErrorCallback(void* const user_data, const nano_ip_error_t error)
{
    nano_ip_tcp_handle_t* const handle = NANO_IP_CAST(nano_ip_tcp_handle_t*, user_data);

    /* Call the registered callback */
    if (handle != NULL)
    {
        nano_ip_tcp_event_data_t event_data;
        (void)MEMSET(&event_data, 0, sizeof(event_data));
        event_data.error = error;
        if (error == NIP_ERR_SUCCESS)
        {
            if (handle->state == TCP_STATE_ESTABLISHED)
            {
                (void)handle->callback(handle->user_data, TCP_EVENT_TX, &event_data);
            }
        }
        else
        {
            (void)handle->callback(handle->user_data, TCP_EVENT_TX_FAILED, &event_data);
        }
    }
}

/** \brief Remove a handle from the handle list */
static void NANO_IP_TCP_RemoveHandle(nano_ip_tcp_handle_t* const handle)
{
    /* Remove handle from list */
    nano_ip_tcp_handle_t* hdl;
    nano_ip_tcp_handle_t* previous_hdl = NULL;
    nano_ip_tcp_module_data_t* const tcp_module = &g_nano_ip.tcp_module;

    hdl = tcp_module->handles;
    while ((hdl != NULL) && (hdl != handle))
    {
        previous_hdl = hdl;
        hdl = hdl->next;
    }
    if (hdl != NULL)
    {
        if (previous_hdl == NULL)
        {
            tcp_module->handles = hdl->next;
        }
        else
        {
            previous_hdl->next = hdl->next;
        }
    }
}

/** \brief Handle a received TCP frame */
static nano_ip_error_t NANO_IP_TCP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_tcp_module_data_t* const tcp_module = NANO_IP_CAST(nano_ip_tcp_module_data_t*, user_data);

    /* Check parameters */
    if ((tcp_module != NULL) && (net_if != NULL) &&
        (ipv4_header != NULL) && (packet != NULL))
    {
        /* Check packet size */
        if (packet->count >= TCP_HEADER_SIZE)
        {
            /* Decode packet */
            tcp_header_t tcp_header;
            uint16_t null_checksum;
            uint16_t options_length;
            uint8_t* const header_start = packet->current;
            tcp_header.ipv4_header = ipv4_header;
            tcp_header.src_port = NANO_IP_PACKET_Read16bits(packet);
            tcp_header.dest_port = NANO_IP_PACKET_Read16bits(packet);
            tcp_header.seq_number = NANO_IP_PACKET_Read32bits(packet);
            tcp_header.ack_number = NANO_IP_PACKET_Read32bits(packet);
            tcp_header.data_offset = (NANO_IP_PACKET_Read8bits(packet) >> 4u);
            tcp_header.flags = NANO_IP_PACKET_Read8bits(packet);
            tcp_header.window = NANO_IP_PACKET_Read16bits(packet);
            options_length = tcp_header.data_offset * sizeof(uint32_t) - TCP_HEADER_SIZE;
            NANO_IP_PACKET_ReadSkipBytes(packet, sizeof(uint16_t)); /* Checksum */
            NANO_IP_PACKET_ReadSkipBytes(packet, sizeof(uint16_t)); /* Urgent pointer */
            NANO_IP_PACKET_ReadSkipBytes(packet, options_length); /* Options */

            /* Compute checkum */
            if ((net_if->driver->caps & NETDRV_CAP_TCPIPV4_CS_CHECK) == 0u)
            {
                null_checksum = NANO_IP_TCP_ComputeCS(ipv4_header, header_start, ipv4_header->data_length);
                if (null_checksum != 0u)
                {
                    ret = NIP_ERR_INVALID_CS;
                }
            }
            if (ret != NIP_ERR_INVALID_CS)
            {
                /* Check length */
                const uint16_t length = ipv4_header->data_length - (TCP_HEADER_SIZE + options_length); 
                if (length <= packet->count)
                {
                    nano_ip_tcp_handle_t* handle = NULL;
                    nano_ip_tcp_handle_t* could_match_handle = NULL;
                    nano_ip_tcp_handle_t* current_handle = NULL;
                                
                    /* Adjust packet length */
                    packet->count = length;

                    /* Look for a corresponding tcp handle */
                    current_handle = tcp_module->handles;
                    while ((current_handle != NULL) && (handle == NULL))
                    {
                        /* Check for a match on the destination port */
                        if (current_handle->port == tcp_header.dest_port)
                        {
                            /* Check for a perfect match on the source port */
                            if (current_handle->dest_port == tcp_header.src_port)
                            {
                                handle = current_handle;
                            }
                            else
                            {
                                could_match_handle = current_handle;
                            }
                        }

                        /* Next handle */
                        current_handle = current_handle->next;
                    }
                    if (handle == NULL)
                    {
                        handle = could_match_handle;
                    }
                    if (handle != NULL)
                    {   
                        bool release_packet = true;
                        nano_ip_tcp_event_data_t event_data;

                        /* Check connection reset */
                        if ((tcp_header.flags & TCP_FLAG_RST) != 0u)
                        {
                            if ((handle->state != TCP_STATE_LISTEN) &&
                                (handle->state != TCP_STATE_IDLE))
                            {
                                /* Reset connection */
                                handle->state = TCP_STATE_CLOSED;
                                NANO_IP_TCP_RemoveHandle(handle);

                                /* Call the registered callback */
                                (void)MEMSET(&event_data, 0, sizeof(event_data));
                                event_data.error = NIP_ERR_CONN_RESET;
                                (void)handle->callback(handle->user_data, TCP_EVENT_CLOSED, &event_data);
                            }
                        }

                        /* Check acknowledge number */
                        if ((handle->state == TCP_STATE_LISTEN) || 
                            (tcp_header.ack_number == handle->seq_number))
                        {
                            /* TCP state machine */
                            switch (handle->state)
                            {
                                case TCP_STATE_LISTEN:
                                {
                                    /* Check SYN flag */
                                    if (tcp_header.flags == TCP_FLAG_SYN)
                                    {
                                        nano_ip_tcp_handle_t* accept_handle = NULL;

                                        /* Call the registered callback */
                                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                                        event_data.error = NIP_ERR_SUCCESS;
                                        event_data.accept_handle = &accept_handle;
                                        (void)handle->callback(handle->user_data, TCP_EVENT_ACCEPTING, &event_data);

                                        /* Check if client must be accepted */
                                        if (accept_handle != NULL)
                                        {
                                            /* Check accept handle state */
                                            if (accept_handle->state == TCP_STATE_IDLE)
                                            {
                                                /* Update connexion endpoint */
                                                accept_handle->ipv4_address = ipv4_header->dest_address;
                                                accept_handle->port = tcp_header.dest_port;
                                                accept_handle->dest_ipv4_address = ipv4_header->src_address;
                                                accept_handle->dest_port = tcp_header.src_port;

                                                /* Initialize sequence number */
                                                handle->seq_number = NANO_IP_OAL_TIME_GetMsCounter();

                                                /* Update ack number */
                                                accept_handle->ack_number = tcp_header.seq_number + 1u;

                                                /* Send acknowledge */
                                                ret = NANO_IP_TCP_SendControlFrame(accept_handle, TCP_FLAG_SYN | TCP_FLAG_ACK);
                                                if (ret == NIP_ERR_SUCCESS)
                                                {
                                                    /* Update sequence number */
                                                    accept_handle->seq_number++;

                                                    /* Wait final ack */
                                                    accept_handle->state = TCP_STATE_SYN_RECEIVED;

                                                    /* Initiate timeout */
                                                    accept_handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
                                                }
                                            }
                                            else
                                            {
                                                /* Call the registered callback */
                                                (void)MEMSET(&event_data, 0, sizeof(event_data));
                                                event_data.error = NIP_ERR_INVALID_TCP_STATE;
                                                (void)handle->callback(accept_handle->user_data, TCP_EVENT_ACCEPT_FAILED, &event_data);
                                            }
                                        }
                                    }
                                    break;
                                }

                                case TCP_STATE_SYN_RECEIVED:
                                {
                                    /* Check acknowledge */
                                    if (tcp_header.flags == TCP_FLAG_ACK)
                                    {
                                        /* Connection established */
                                        handle->state = TCP_STATE_ESTABLISHED;

                                        /* Call the registered callback */
                                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                                        event_data.error = NIP_ERR_SUCCESS;
                                        (void)handle->callback(handle->user_data, TCP_EVENT_ACCEPTED, &event_data);
                                    }
                                    break;
                                }

                                case TCP_STATE_SYN_SENT:
                                {
                                    /* Check SYN flag acknowledge */
                                    if (tcp_header.flags == (TCP_FLAG_SYN | TCP_FLAG_ACK))
                                    {
                                        /* Update ack number */
                                        handle->ack_number = tcp_header.seq_number + 1u;

                                        /* Send acknowledge */
                                        ret = NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_ACK);
                                        if (ret == NIP_ERR_SUCCESS)
                                        {
                                            /* Connection established */
                                            handle->state = TCP_STATE_ESTABLISHED;

                                            /* Call the registered callback */
                                            (void)MEMSET(&event_data, 0, sizeof(event_data));
                                            event_data.error = NIP_ERR_SUCCESS;
                                            (void)handle->callback(handle->user_data, TCP_EVENT_CONNECTED, &event_data);
                                        }
                                    }
                                    break;
                                }

                                case TCP_STATE_ESTABLISHED:
                                {
                                    /* Check PUSH flag */
                                    if ((packet->count != 0u) && (tcp_header.flags == (TCP_FLAG_PSH | TCP_FLAG_ACK)))
                                    {
                                        /* Check duplicate sequence */
                                        if (tcp_header.seq_number == handle->ack_number)
                                        {
                                            /* Update ack number */
                                            handle->ack_number = tcp_header.seq_number + packet->count;

                                            /* Send acknowledge */
                                            ret = NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_ACK);
                                            if (ret == NIP_ERR_SUCCESS)
                                            {
                                                /* Data received, call the registered callback */
                                                (void)MEMSET(&event_data, 0, sizeof(event_data));
                                                event_data.error = NIP_ERR_SUCCESS;
                                                event_data.packet = packet;
                                                release_packet = handle->callback(handle->user_data, TCP_EVENT_RX, &event_data);
                                            }
                                        }
                                    }
                                    else if ((handle->last_tx_packet != NULL) && (tcp_header.flags == TCP_FLAG_ACK))
                                    {
                                        /* Packet has been acknowledged */
                                        (void)NANO_IP_TCP_ReleasePacket(handle->last_tx_packet);
                                        handle->last_tx_packet = NULL;

                                        /* Call the registered callback */
                                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                                        event_data.error = NIP_ERR_SUCCESS;
                                        (void)handle->callback(handle->user_data, TCP_EVENT_TX, &event_data);
                                    }
                                    else if (tcp_header.flags == (TCP_FLAG_FIN | TCP_FLAG_ACK))
                                    {
                                        /* Update ack number */
                                        handle->ack_number = tcp_header.seq_number + 1u;

                                        /* Send acknowledge */
                                        ret = NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_FIN | TCP_FLAG_ACK);
                                        if (ret == NIP_ERR_SUCCESS)
                                        {
                                            /* Connection closing */
                                            handle->state = TCP_STATE_CLOSE_WAIT;

                                            /* Update sequence number */
                                            handle->seq_number++;

                                            /* Initiate timeout */
                                            handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
                                        }
                                    }

                                    break;
                                }

                                case TCP_STATE_CLOSE_WAIT:
                                {
                                    /* Check acknowledge */
                                    if (tcp_header.flags == TCP_FLAG_ACK)
                                    {
                                        /* Connection closed */
                                        handle->state = TCP_STATE_CLOSED;
                                        NANO_IP_TCP_RemoveHandle(handle);

                                        /* Call the registered callback */
                                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                                        event_data.error = NIP_ERR_SUCCESS;
                                        (void)handle->callback(handle->user_data, TCP_EVENT_CLOSED, &event_data);
                                    }
                                    break;
                                }

                                case TCP_STATE_FIN_WAIT_1:
                                {
                                    /* Check acknowledge */
                                    if (tcp_header.flags == TCP_FLAG_ACK)
                                    {
                                        /* 4 way handshake */
                                        handle->state = TCP_STATE_FIN_WAIT_2;

                                        /* Initiate timeout */
                                        handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
                                    }
                                    else if (tcp_header.flags == (TCP_FLAG_FIN | TCP_FLAG_ACK))
                                    {
                                        /* 3 way handshake */
                                        handle->ack_number = tcp_header.seq_number + 1u;

                                        /* Send acknowledge */
                                        ret = NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_ACK);
                                        if (ret == NIP_ERR_SUCCESS)
                                        {
                                            /* Connection closing */
                                            handle->state = TCP_STATE_TIME_WAIT;

                                            /* Initiate timeout */
                                            handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
                                        }
                                    }
                                    else
                                    {
                                        /* Do nothing... */
                                    }
                                    break;
                                }

                                case TCP_STATE_FIN_WAIT_2:
                                {
                                    /* Check acknowledge */
                                    if (tcp_header.flags == (TCP_FLAG_FIN | TCP_FLAG_ACK))
                                    {
                                        /* Update ack number */
                                        handle->ack_number = tcp_header.seq_number + 1u;

                                        /* Send acknowledge */
                                        ret = NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_ACK);
                                        if (ret == NIP_ERR_SUCCESS)
                                        {
                                            /* Connection closing */
                                            handle->state = TCP_STATE_TIME_WAIT;

                                            /* Initiate timeout */
                                            handle->state_timeout = NANO_IP_OAL_TIME_GetMsCounter() + TCP_STATE_TIMEOUT;
                                        }
                                    }
                                    break;
                                }

                                case TCP_STATE_IDLE:
                                /* Intended fallthrough */
                                case TCP_STATE_CLOSED:
                                {
                                    /* Do nothing ... */
                                    break;
                                }

                                default:
                                {
                                    /* Reset connection */
                                    handle->state = TCP_STATE_CLOSED;
                                    (void)NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_RST);
                                    NANO_IP_TCP_RemoveHandle(handle);

                                    /* Call the registered callback */
                                    (void)MEMSET(&event_data, 0, sizeof(event_data));
                                    event_data.error = NIP_ERR_FAILURE;
                                    (void)handle->callback(handle->user_data, TCP_EVENT_CLOSED, &event_data);
                                    break;
                                }
                            }
                        }
                        else
                        {
                            /* Ignore duplicates */
                            if (tcp_header.ack_number > handle->seq_number)
                            {
                                /* Reset connection */
                                handle->state = TCP_STATE_CLOSED;
                                (void)NANO_IP_TCP_SendControlFrame(handle, TCP_FLAG_RST);
                                NANO_IP_TCP_RemoveHandle(handle);

                                /* Call the registered callback */
                                (void)MEMSET(&event_data, 0, sizeof(event_data));
                                event_data.error = NIP_ERR_FAILURE;
                                (void)handle->callback(handle->user_data, TCP_EVENT_CLOSED, &event_data);
                            }
                        }

                        /* Release received packet */
                        if (!release_packet)
                        {
                            packet->flags |= NET_IF_PACKET_FLAG_KEEP_PACKET;
                        }
                        ret = NIP_ERR_SUCCESS;
                    }
                    else
                    {
                        ret = NIP_ERR_IGNORE_PACKET;
                    }
                }
                else
                {
                    ret = NIPP_ERR_INVALID_PACKET_SIZE;
                }
            } 
        }
        else
        {
            ret = NIPP_ERR_INVALID_PACKET_SIZE;
        }
    }

    return ret;
}

/** \brief TCP periodic task */
static void NANO_IP_TCP_PeriodicTask(const uint32_t timestamp, void* const user_data)
{
    nano_ip_tcp_module_data_t* const tcp_module = NANO_IP_CAST(nano_ip_tcp_module_data_t*, user_data);

    /* Check parameters */
    if (tcp_module != NULL)
    {
        /* Go through the opened TCP handles */
        nano_ip_tcp_event_data_t event_data;
        nano_ip_tcp_handle_t* tcp_handle = tcp_module->handles;
        while (tcp_handle != NULL)
        {
            /* TCP state machine */
            switch (tcp_handle->state)
            {
                case TCP_STATE_SYN_SENT:
                {
                    /* Check timeout */
                    if (tcp_handle->state_timeout <= timestamp)
                    {
                        /* Abort connection */
                        tcp_handle->state = TCP_STATE_CLOSED;
                        NANO_IP_TCP_RemoveHandle(tcp_handle);

                        /* Call the registered callback */
                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                        event_data.error = NIP_ERR_TIMEOUT;
                        (void)tcp_handle->callback(tcp_handle->user_data, TCP_EVENT_CONNECT_TIMEOUT, &event_data);
                    }
                    break;
                }

                case TCP_STATE_SYN_RECEIVED:
                {
                    /* Check timeout */
                    if (tcp_handle->state_timeout <= timestamp)
                    {
                        /* Abort connection */
                        tcp_handle->state = TCP_STATE_CLOSED;
                        NANO_IP_TCP_RemoveHandle(tcp_handle);

                        /* Call the registered callback */
                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                        event_data.error = NIP_ERR_TIMEOUT;
                        (void)tcp_handle->callback(tcp_handle->user_data, TCP_EVENT_ACCEPT_FAILED, &event_data);
                    }
                    break;
                }

                case TCP_STATE_ESTABLISHED:
                {
                    /* Check timeout */
                    if ((tcp_handle->last_tx_packet != NULL) && (tcp_handle->state_timeout <= timestamp))
                    {
                        /* Update retry count */
                        tcp_handle->tx_retry_count++;
                        if (tcp_handle->tx_retry_count == TCP_MAX_RETRY_COUNT)
                        {
                            /* Release packet */
                            (void)NANO_IP_TCP_ReleasePacket(tcp_handle->last_tx_packet);
                            tcp_handle->last_tx_packet = NULL;

                            /* Abort connection */
                            tcp_handle->state = TCP_STATE_CLOSED;
                            NANO_IP_TCP_RemoveHandle(tcp_handle);

                            /* Call the registered callback */
                            (void)MEMSET(&event_data, 0, sizeof(event_data));
                            event_data.error = NIP_ERR_TIMEOUT;
                            (void)tcp_handle->callback(tcp_handle->user_data, TCP_EVENT_TX_FAILED, &event_data);
                            (void)tcp_handle->callback(tcp_handle->user_data, TCP_EVENT_CLOSED, &event_data);
                        }
                        else
                        {
                            /* Resend packet */
                            tcp_handle->last_tx_packet->current = tcp_handle->last_tx_packet_pos;
                            tcp_handle->last_tx_packet->count = tcp_handle->last_tx_packet_count;
                            (void)NANO_IP_IPV4_SendPacket(&tcp_handle->ipv4_handle, &tcp_handle->last_tx_packet_ipv4_header, tcp_handle->last_tx_packet);
                            tcp_handle->state_timeout += TCP_STATE_TIMEOUT;
                        }
                        
                    }

                    break;
                }

                case TCP_STATE_CLOSE_WAIT:
                /* Intended fallthrough */
                case TCP_STATE_FIN_WAIT_1:
                /* Intended fallthrough */
                case TCP_STATE_FIN_WAIT_2:
                /* Intended fallthrough */
                case TCP_STATE_TIME_WAIT:
                {
                    /* Check timeout */
                    if (tcp_handle->state_timeout <= timestamp)
                    {
                        /* Abort connection */
                        tcp_handle->state = TCP_STATE_CLOSED;
                        NANO_IP_TCP_RemoveHandle(tcp_handle);

                        /* Call the registered callback */
                        (void)MEMSET(&event_data, 0, sizeof(event_data));
                        if (tcp_handle->state == TCP_STATE_CLOSE_WAIT)
                        {
                            event_data.error = NIP_ERR_SUCCESS;
                        }
                        else
                        {
                            event_data.error = NIP_ERR_TIMEOUT;
                        }
                        (void)tcp_handle->callback(tcp_handle->user_data, TCP_EVENT_CLOSED, &event_data);
                    }
                    break;
                }

                default:
                {
                    /* Do nothing... */
                    break;
                }
            }

            /* Next handle */
            tcp_handle = tcp_handle->next;
        }
    }
}

/** \brief Compute the TCP checksum of a buffer */
static uint16_t NANO_IP_TCP_ComputeCS(const ipv4_header_t* const ipv4_header, uint8_t* const buffer, const uint16_t size)
{
    uint8_t pseudo_header[TCP_PSEUDO_HEADER_SIZE];

    /* Fill pseudo header */
    pseudo_header[0u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address >> 24u));
    pseudo_header[1u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address >> 16u));
    pseudo_header[2u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address >> 8u));
    pseudo_header[3u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address & 0xFFu));
    pseudo_header[4u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address >> 24u));
    pseudo_header[5u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address >> 16u));
    pseudo_header[6u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address >> 8u));
    pseudo_header[7u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address & 0xFFu));
    pseudo_header[8u] = 0;
    pseudo_header[9u] = TCP_PROTOCOL;
    pseudo_header[10u] = NANO_IP_CAST(uint8_t, (size >> 8u));
    pseudo_header[11u] = NANO_IP_CAST(uint8_t, (size & 0x00FFu));

    /* Compute checksum with the pseudo header */
    return NANO_IP_ComputeInternetCS(pseudo_header, sizeof(pseudo_header), buffer, size);
}

/** \brief Send a TCP control frame */
static nano_ip_error_t NANO_IP_TCP_SendControlFrame(nano_ip_tcp_handle_t* const handle, const uint8_t flags)
{
    nano_ip_error_t ret;
    nano_ip_net_packet_t* packet;

    /* Allocate a packet */
    ret = NANO_IP_TCP_AllocatePacket(&packet, 0u);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Send frame */
        ret = NANO_IP_TCP_FinalizeAndSendPacket(handle, flags, packet);
        if ((ret != NIP_ERR_SUCCESS) && (ret != NIP_ERR_IN_PROGRESS))
        {
            /* Release packet */
            (void)NANO_IP_TCP_ReleasePacket(packet);
        }
    }

    return ret;
}

/** \brief Finalize and send a TCP packet */
static nano_ip_error_t NANO_IP_TCP_FinalizeAndSendPacket(nano_ip_tcp_handle_t* const handle, const uint8_t flags, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    uint16_t checksum;
    uint8_t* checksum_pos;
    uint8_t* header_start;
    ipv4_header_t ipv4_header;
    uint8_t* const current_pos = packet->current;
    uint16_t tcp_length = packet->count + TCP_HEADER_SIZE;
    uint16_t packet_size = NANO_IP_CAST(uint16_t, packet->current - packet->data);

    /* Write header */
    header_start = packet->current - tcp_length;
    packet->current = header_start;
    NANO_IP_PACKET_Write16bits(packet, handle->port);
    NANO_IP_PACKET_Write16bits(packet, handle->dest_port);
    NANO_IP_PACKET_Write32bits(packet, handle->seq_number);
    if ((flags & TCP_FLAG_ACK) != 0u)
    {
        NANO_IP_PACKET_Write32bits(packet, handle->ack_number);
    }
    else
    {
        NANO_IP_PACKET_Write32bits(packet, 0u);
    }
    NANO_IP_PACKET_Write8bits(packet, TCP_HEADER_DATA_OFFSET);
    NANO_IP_PACKET_Write8bits(packet, flags);
    NANO_IP_PACKET_Write16bits(packet, TCP_WINDOW_SIZE);

    checksum_pos = packet->current;
    NANO_IP_PACKET_Write32bits(packet, 0u);

    /* Prepare IPv4 header */
    ipv4_header.dest_address = handle->dest_ipv4_address;
    if (handle->ipv4_address == 0u)
    {
        /* Get the route to the destination */

        uint32_t gateway_addr = 0u;
        nano_ip_net_if_t* net_if = packet->net_if;
        if (net_if == NULL)
        {
            (void)NANO_IP_ROUTE_Search(handle->dest_ipv4_address, &gateway_addr, &net_if);
        }
        if (net_if != NULL)
        {
            ipv4_header.src_address = net_if->ipv4_address;
        }
        else
        {
            ipv4_header.src_address = 0u;
        }
    }
    else
    {
        ipv4_header.src_address = handle->ipv4_address;
    }
    ipv4_header.protocol = TCP_PROTOCOL;

    /* Save IPv4 header */
    if (handle->last_tx_packet != NULL)
    {
        (void)MEMCPY(&handle->last_tx_packet_ipv4_header, &ipv4_header, sizeof(ipv4_header_t));
    }

    /* Write checksum */
    checksum = NANO_IP_TCP_ComputeCS(&ipv4_header, header_start, tcp_length);
    checksum_pos[0] = NANO_IP_CAST(uint8_t, (checksum & 0xFFu));
    checksum_pos[1] = NANO_IP_CAST(uint8_t, (checksum >> 8u));

    /* Restore packet pointers */
    packet->current = current_pos;
    packet->count = packet_size;

    /* Send frame */
    ret = NANO_IP_IPV4_SendPacket(&handle->ipv4_handle, &ipv4_header, packet);

    return ret;
}

#endif /* NANO_IP_ENABLE_TCP */
