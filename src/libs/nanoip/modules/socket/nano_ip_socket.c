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

#include "nano_ip_socket.h"

#if( NANO_IP_ENABLE_SOCKET == 1 )

#include "nano_ip_tools.h"
#include "nano_ip_data.h"
#include "nano_ip_packet_funcs.h"



/** \brief Socket event flags */
typedef enum _nano_ip_socket_event_flags_t
{
    /** \brief Packet received */
    SOCKET_EVENT_RX = 1u,
    /** \brief Packet sent */
    SOCKET_EVENT_TX = 2u,
    /** \brief Packet error */
    SOCKET_EVENT_ERROR = 4u,

    /** \brief All flags */
    SOCKET_EVENT_ALL = (SOCKET_EVENT_RX | SOCKET_EVENT_TX | SOCKET_EVENT_ERROR)
} nano_ip_socket_event_flags_t;






/** \brief Look for an initialized socket in the socket array */
static nano_ip_socket_t* NANO_IP_SOCKET_Get(const uint32_t socket_id);

/** \brief Callback for UDP sockets */
static bool NANO_IP_SOCKET_UdpCallback(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data);

/** \brief Callback for TCP sockets */
static bool NANO_IP_SOCKET_TcpCallback(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data);



/** \brief Initialize the socket module */
nano_ip_error_t NANO_IP_SOCKET_Init(void)
{
    uint32_t i;
    nano_ip_error_t ret = NIP_ERR_SUCCESS;
    nano_ip_socket_module_data_t* const socket_module = &g_nano_ip.socket_module;

    /* Initialize the mutex */
    ret = NANO_IP_OAL_MUTEX_Create(&socket_module->mutex);

    /* Initialize the socket array */
    if (ret == NIP_ERR_SUCCESS)
    {
        for (i = 0; ((i < NANO_IP_SOCKET_MAX_COUNT) && (ret == NIP_ERR_SUCCESS)); i++)
        {
            #if (NANO_IP_ENABLE_TCP == 1u)
            socket_module->sockets[i].id = i;
            #endif /* NANO_IP_ENABLE_TCP */
            socket_module->sockets[i].is_free = true;
            ret = NANO_IP_OAL_FLAGS_Create(&socket_module->sockets[i].sync_flags);
        }
    }

    #if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
    /* Initialize the socket poll array */
    if (ret == NIP_ERR_SUCCESS)
    {
        for (i = 0; ((i < NANO_IP_SOCKET_MAX_POLL_COUNT) && (ret == NIP_ERR_SUCCESS)); i++)
        {
            socket_module->polls[i].is_free = true;
            ret = NANO_IP_OAL_FLAGS_Create(&socket_module->polls[i].sync_flags);
        }
    }
    #endif /* NANO_IP_ENABLE_SOCKET_POLL */

    return ret;
}


/** \brief Allocate a socket */
nano_ip_error_t NANO_IP_SOCKET_Allocate(uint32_t* const socket_id, const nano_ip_socket_type_t type)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    (*socket_id) = NANO_IP_INVALID_SOCKET_ID;
    if (socket_id != NULL)
    {
        uint32_t i;
        nano_ip_socket_t* socket = NULL;
        nano_ip_socket_module_data_t* const socket_module = &g_nano_ip.socket_module;

        /* Look for a free socket */
        i = 0;
        while ((i < NANO_IP_SOCKET_MAX_COUNT) && (socket == NULL))
        {
            if (socket_module->sockets[i].is_free)
            {
                socket = &socket_module->sockets[i];
            }
            i++;
        }
        if (socket == NULL)
        {
            ret = NIP_ERR_RESOURCE;
        }
        else
        {
            /* 0 init */
            socket->type = type;
            socket->options = 0u;
            NANO_IP_PACKET_ResetQueue(&socket->rx_packets);
            #if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
            socket->poll = NULL;
            #endif /* NANO_IP_ENABLE_SOCKET_POLL */
            
            /* Reset the synchronization object */
            ret = NANO_IP_OAL_FLAGS_Reset(&socket->sync_flags, NANO_IP_OAL_FLAGS_ALL);
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Connection handle init */
                switch (type)
                {
                    case NIPSOCK_UDP:
                    {
                        /* UDP */
                        ret = NANO_IP_UDP_InitializeHandle(&socket->connection_handle.udp, NANO_IP_SOCKET_UdpCallback, socket);
                        if (ret == NIP_ERR_SUCCESS)
                        {
                            /* Tx is allowed */
                            (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_TX, false);
                        }
                        break;
                    }

                    case NIPSOCK_TCP:
                    {
                        /* TCP */
                        socket->parent = NULL;
                        socket->child_count = 0u;
                        socket->max_child_count = 0u;
                        socket->accept_pending_sockets = NULL;
                        socket->accepted_sockets = NULL;
                        socket->next = NULL;

                        ret = NANO_IP_TCP_InitializeHandle(&socket->connection_handle.tcp, NANO_IP_SOCKET_TcpCallback, socket);
                        if (ret == NIP_ERR_SUCCESS)
                        {
                            ret = NANO_IP_TCP_Open(&socket->connection_handle.tcp, 0u);
                            if (ret != NIP_ERR_SUCCESS)
                            {
                                (void)NANO_IP_TCP_ReleaseHandle(&socket->connection_handle.tcp);
                            }
                        }
                        break;
                    }
                    
                    default:
                    {
                        /* Invalid socket type */
                        break;
                    }
                }
            }
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Socket is now allocated */
                socket->is_free = false;
                (*socket_id) = (i - 1u);
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Release a socket */
nano_ip_error_t NANO_IP_SOCKET_Release(const uint32_t socket_id)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if (socket != NULL)
    {
        /* Release connection handle */
        switch (socket->type)
        {
            case NIPSOCK_UDP:
            {
                /* UDP */
                ret = NANO_IP_UDP_ReleaseHandle(&socket->connection_handle.udp);
                break;
            }

            case NIPSOCK_TCP:
            {
                /* TCP */
                ret = NANO_IP_TCP_Close(&socket->connection_handle.tcp);
                break;
            }
            
            default:
            {
                /* Invalid socket type */
                break;
            }
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Release the synchronization object */
            (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, 0xFFFFFFFFu, false);

            /* Socket is now free */
            socket->is_free = true;
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Bind a socket to a specific address and port */
nano_ip_error_t NANO_IP_SOCKET_Bind(const uint32_t socket_id, const nano_ip_socket_endpoint_t* const end_point)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if ((socket != NULL) &&
        (end_point != NULL))
    {
        /* Bind connection handle */
        switch (socket->type)
        {
            case NIPSOCK_UDP:
            {
                /* UDP */
                ret = NANO_IP_UDP_Bind(&socket->connection_handle.udp, end_point->address, end_point->port);
                break;
            }

            case NIPSOCK_TCP:
            {
                /* TCP */
                ret = NANO_IP_TCP_Bind(&socket->connection_handle.tcp, end_point->address, end_point->port);
                break;
            }
            
            default:
            {
                /* Invalid socket type */
                break;
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Receive data from a socket */
nano_ip_error_t NANO_IP_SOCKET_ReceiveFrom(const uint32_t socket_id, void* const data, const size_t size, size_t* const received, nano_ip_socket_endpoint_t* const end_point)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if ((socket != NULL) &&
        (data != NULL) &&
        (received != NULL))
    {
        /* Check if socket is bound */
        (*received) = 0;
        switch (socket->type)
        {
            case NIPSOCK_UDP:
            {
                /* UDP socket */
                if (socket->connection_handle.udp.is_bound)
                {
                    ret = NIP_ERR_SUCCESS;
                }
                break;
            }

            case NIPSOCK_TCP:
            {
                /* TCP socket */
                if (socket->connection_handle.tcp.state == TCP_STATE_ESTABLISHED)
                {
                    ret = NIP_ERR_SUCCESS;
                }
                else
                {
                    ret = NIP_ERR_INVALID_TCP_STATE;
                }
                break;
            }

            default:
            {
                /* Invalid socket type */
                break;
            }
        }

        /* Wait for available data */    
        if (ret == NIP_ERR_SUCCESS)
        {
            bool wait_more = true;
            do
            {
                /* Packet available */
                if (!NANO_IP_PACKET_QueueIsEmpty(&socket->rx_packets))
                {
                    if (socket->type == NIPSOCK_UDP)
                    {
                        /* UDP socket => buffer size must be big enough to receive the whole packet at once */
                        if (size >= socket->rx_packets.head->count)
                        {
                            nano_ip_net_packet_t* packet = NANO_IP_PACKET_PopFromQueue(&socket->rx_packets);

                            /* Read header */
                            if (end_point != NULL)
                            {
                                (void)NANO_IP_UDP_ReadHeader(packet, &end_point->address, &end_point->port);
                            }

                            /* Copy received data */
                            (*received) = packet->count;
                            NANO_IP_PACKET_ReadBuffer(packet, data, packet->count);

                            /* Release packet */
                            (void)NANO_IP_UDP_ReleasePacket(packet);

                            wait_more = false;
                            ret = NIP_ERR_SUCCESS;
                        }
                        else
                        {
                            ret = NIP_ERR_BUFFER_TOO_SMALL;
                        }
                    }
                    else if (socket->type == NIPSOCK_TCP)
                    {
                        uint8_t* u8_data = NANO_IP_CAST(uint8_t*, data);

                        do
                        {
                            size_t read_size = 0u;
                            nano_ip_net_packet_t* packet;

                            /* Compute the maximum size which can be read */
                            packet = socket->rx_packets.head;
                            if (size <= packet->count)
                            {
                                read_size = size;
                            }
                            else
                            {
                                read_size = packet->count;
                            }

                            /* Read data */
                            NANO_IP_PACKET_ReadBuffer(packet, u8_data, read_size);

                            /* Update total received count */
                            (*received) += read_size;
                            u8_data += read_size;

                            /* Release packet if no more data available */
                            if (packet->count == 0u)
                            {
                                packet = NANO_IP_PACKET_PopFromQueue(&socket->rx_packets);
                                (void)NANO_IP_TCP_ReleasePacket(packet);
                            }
                        }
                        while (((*received) != size) && (!NANO_IP_PACKET_QueueIsEmpty(&socket->rx_packets)));

                        /* Fill endpoint */
                        if (end_point != NULL)
                        {
                            end_point->address = socket->connection_handle.tcp.dest_ipv4_address;
                            end_point->port = socket->connection_handle.tcp.dest_port;
                        }

                        wait_more = false;
                        ret = NIP_ERR_SUCCESS;
                    }
                    else
                    {
                        /* Invalid socket type */
                        ret = NIP_ERR_INVALID_ARG;
                    }
                }
                else
                {
                    /* Check for non-blocking socket */
                    if ((socket->options & NIPSOCK_OPT_NON_BLOCK) != 0)
                    {
                        ret = NIP_ERR_IN_PROGRESS;
                    }
                    else
                    {
                        ret = NIP_ERR_SUCCESS;
                    }
                    (*received) = 0u;
                }

                /* Wait for more data */
                if ((ret == NIP_ERR_SUCCESS) && wait_more)
                {
                    uint32_t mask = SOCKET_EVENT_RX | SOCKET_EVENT_ERROR;
                    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                    ret = NANO_IP_OAL_FLAGS_Wait(&socket->sync_flags, &mask, true, NANO_IP_MAX_TIMEOUT_VALUE);
                    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                    if (socket->is_free)
                    {
                        ret = NIP_ERR_FAILURE;
                    }
                    if (ret == NIP_ERR_SUCCESS)
                    {
                        /* Check event */
                        if ((mask & SOCKET_EVENT_RX) != 0)
                        {
                            /* Packet available */
                        }
                        if ((mask & SOCKET_EVENT_ERROR) != 0)
                        {
                            /* Error */
                            ret = NIP_ERR_FAILURE;
                        }
                    }
                }
            }
            while ((ret == NIP_ERR_SUCCESS) && wait_more);
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Send data to a socket */
nano_ip_error_t NANO_IP_SOCKET_SendTo(const uint32_t socket_id, const void* const data, const size_t size, size_t* const sent, const nano_ip_socket_endpoint_t* const end_point)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if ((socket != NULL) &&
        (data != NULL) &&
        (sent != NULL) && 
        ((socket->type == NIPSOCK_TCP) || ((socket->type == NIPSOCK_UDP) && (end_point != NULL))))
    {
        /* Bind connection handle */
        (*sent) = 0;
        switch (socket->type)
        {
            case NIPSOCK_UDP:
            {
                /* UDP */
                
                /* The whole data must fit into one datagram */
                bool retry = true;
                nano_ip_net_packet_t* packet = NULL;
                uint16_t packet_size = NANO_IP_CAST(uint16_t, size);
                do
                {
                    /* Check if handle is busy */
                    ret = NANO_IP_UDP_HandleIsReady(&socket->connection_handle.udp);
                    if (ret == NIP_ERR_SUCCESS)
                    {
                        /* Allocate packet */
                        ret = NANO_IP_UDP_AllocatePacket(&packet, packet_size);
                        if (ret == NIP_ERR_SUCCESS)
                        {
                            /* Copy data to send */
                            NANO_IP_PACKET_WriteBuffer(packet, data, packet_size);

                            /* Send packet */
                            ret = NANO_IP_UDP_SendPacket(&socket->connection_handle.udp, end_point->address, end_point->port, packet);
                            if (ret == NIP_ERR_SUCCESS)
                            {
                                /* Packet has been sent */
                                (*sent) = packet_size;
                            }
                            else if (ret == NIP_ERR_IN_PROGRESS)
                            {
                                /* Packet is going to be sent */
                                (*sent) = packet_size;
                                NANO_IP_OAL_FLAGS_Reset(&socket->sync_flags, SOCKET_EVENT_TX);
                            }
                            else
                            {
                                /* Critical error */
                                (void)NANO_IP_UDP_ReleasePacket(packet);
                            }
                        }
                        retry = false;
                    }
                    else
                    {
                        retry = true;
                    }
                    if (retry || (ret == NIP_ERR_IN_PROGRESS))
                    {
                        /* Check for non-blocking socket */
                        if ((socket->options & NIPSOCK_OPT_NON_BLOCK) == 0)
                        {
                            /* Wait ready */
                            uint32_t mask = SOCKET_EVENT_TX | SOCKET_EVENT_ERROR;
                            (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                            ret = NANO_IP_OAL_FLAGS_Wait(&socket->sync_flags, &mask, true, NANO_IP_MAX_TIMEOUT_VALUE);
                            (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                            if (socket->is_free)
                            {
                                ret = NIP_ERR_FAILURE;
                            }
                            if (ret == NIP_ERR_SUCCESS)
                            {
                                /* Check event */
                                if ((mask & SOCKET_EVENT_TX) != 0)
                                {
                                    /* UDP handle is ready for transmission */
                                    retry = false;
                                }
                                if ((mask & SOCKET_EVENT_ERROR) != 0)
                                {
                                    /* Error */
                                    ret = NIP_ERR_FAILURE;
                                }
                            }
                            if (ret == NIP_ERR_FAILURE)
                            {
                                (*sent) = 0u;
                            }
                        }
                    }
                }
                while (retry && (ret == NIP_ERR_SUCCESS));

                break;
            }

            case NIPSOCK_TCP:
            {
                /* TCP */

                /* The whole data must fit into one datagram */
                bool resend = false;
                nano_ip_net_packet_t* packet = NULL;
                uint16_t packet_size = NANO_IP_CAST(uint16_t, size);
                do
                {
                    ret = NANO_IP_TCP_AllocatePacket(&packet, packet_size);
                    if (ret == NIP_ERR_SUCCESS)
                    {
                        /* Copy data to send */
                        NANO_IP_PACKET_WriteBuffer(packet, data, packet_size);

                        /* Send packet */
                        ret = NANO_IP_TCP_SendPacket(&socket->connection_handle.tcp, packet);
                        if (ret == NIP_ERR_SUCCESS)
                        {
                            /* Packet has been sent */
                            (*sent) = packet_size;
                        }
                        else if ((ret == NIP_ERR_IN_PROGRESS) || (ret == NIP_ERR_BUSY))
                        {
                            /* If busy, the packet must be released manually and resent later */
                            if (ret == NIP_ERR_BUSY)
                            {
                                resend = true;
                                (void)NANO_IP_TCP_ReleasePacket(packet);
                            }

                            /* Check for non-blocking socket */
                            if ((socket->options & NIPSOCK_OPT_NON_BLOCK) == 0)
                            {
                                /* Wait ready */
                                uint32_t mask = SOCKET_EVENT_TX | SOCKET_EVENT_ERROR;
                                (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                                ret = NANO_IP_OAL_FLAGS_Wait(&socket->sync_flags, &mask, true, NANO_IP_MAX_TIMEOUT_VALUE);
                                (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                                if (socket->is_free)
                                {
                                    ret = NIP_ERR_FAILURE;
                                }
                                if (ret == NIP_ERR_SUCCESS)
                                {
                                    /* Check event */
                                    if ((mask & SOCKET_EVENT_TX) != 0)
                                    {
                                        /* TCP handle is ready for transmission */
                                    }
                                    if ((mask & SOCKET_EVENT_ERROR) != 0)
                                    {
                                        /* Error */
                                        ret = NIP_ERR_FAILURE;
                                    }
                                }
                            }
                        }
                        else
                        {
                            /* Error, release packet */
                            (void)NANO_IP_TCP_ReleasePacket(packet);
                        }
                    }
                }
                while ((ret == NIP_ERR_SUCCESS) && (resend));
                
                break;
            }
            
            default:
            {
                /* Invalid socket type */
                break;
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}


#if (NANO_IP_ENABLE_TCP == 1u)

/** \brief Receive data from a socket */
nano_ip_error_t NANO_IP_SOCKET_Receive(const uint32_t socket_id, void* const data, const size_t size, size_t* const received)
{
    return NANO_IP_SOCKET_ReceiveFrom(socket_id, data, size, received, NULL);
}

/** \brief Send data to a socket */
nano_ip_error_t NANO_IP_SOCKET_Send(const uint32_t socket_id, const void* const data, const size_t size, size_t* const sent)
{
    return NANO_IP_SOCKET_SendTo(socket_id, data, size, sent, NULL);
}

/** \brief Put a socket into listen state */
nano_ip_error_t NANO_IP_SOCKET_Listen(const uint32_t socket_id, const uint32_t max_incoming_connections)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if ((socket != NULL) && 
        (socket->type == NIPSOCK_TCP) &&
        (max_incoming_connections > 0) &&
        (max_incoming_connections <= NANO_IP_SOMAXCONN))
    {
        /* Put socket into listen state */
        socket->max_child_count = max_incoming_connections;
        ret = NANO_IP_TCP_Listen(&socket->connection_handle.tcp);
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Accept a connection on a socket */
nano_ip_error_t NANO_IP_SOCKET_Accept(const uint32_t socket_id, uint32_t* const client_socket_id, nano_ip_socket_endpoint_t* const end_point)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if ((socket != NULL) && 
        (client_socket_id != NULL) &&
        (socket->type == NIPSOCK_TCP) &&
        (socket->connection_handle.tcp.state == TCP_STATE_LISTEN))
    {
        bool wait_more;
        do
        {
            /* Check if a connection is pending */
            wait_more = false;
            if (socket->accepted_sockets != NULL)
            {
                /* Remove the socket from the pending list */
                nano_ip_socket_t* client_sock = socket->accepted_sockets;
                socket->accepted_sockets = client_sock->next;
                client_sock->next = NULL;

                /* Fill returned data */
                (*client_socket_id) = client_sock->id;
                if (end_point != NULL)
                {
                    end_point->address = client_sock->connection_handle.tcp.dest_ipv4_address;
                    end_point->port = client_sock->connection_handle.tcp.dest_port;
                }

                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                /* Check for non-blocking socket */
                if ((socket->options & NIPSOCK_OPT_NON_BLOCK) == 0)
                {
                    /* Wait ready */
                    uint32_t mask = SOCKET_EVENT_RX | SOCKET_EVENT_ERROR;
                    wait_more = true;
                    (void)NANO_IP_OAL_FLAGS_Reset(&socket->sync_flags, NANO_IP_OAL_FLAGS_ALL);
                    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                    ret = NANO_IP_OAL_FLAGS_Wait(&socket->sync_flags, &mask, true, NANO_IP_MAX_TIMEOUT_VALUE);
                    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                    if (socket->is_free)
                    {
                        ret = NIP_ERR_FAILURE;
                    }
                    if (ret == NIP_ERR_SUCCESS)
                    {
                        /* Check event */
                        if ((mask & SOCKET_EVENT_RX) != 0)
                        {
                            /* A connection is pending */
                        }
                        if ((mask & SOCKET_EVENT_ERROR) != 0)
                        {
                            /* Error */
                            ret = NIP_ERR_FAILURE;
                        }
                    }
                }
                else
                {
                    
                    ret = NIP_ERR_IN_PROGRESS;
                }
            }
        }
        while (wait_more);
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Connect a socket to a specific address and port */
nano_ip_error_t NANO_IP_SOCKET_Connect(const uint32_t socket_id, const nano_ip_socket_endpoint_t* const end_point)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if ((socket != NULL) && 
        (end_point != NULL) &&
        (socket->type == NIPSOCK_TCP))
    {
        /* Start connection process */
        ret = NANO_IP_TCP_Connect(&socket->connection_handle.tcp, end_point->address, end_point->port);
        if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
        {
            /* Check for non-blocking socket */
            if ((socket->options & NIPSOCK_OPT_NON_BLOCK) == 0)
            {
                /* Wait ready */
                uint32_t mask = SOCKET_EVENT_TX | SOCKET_EVENT_ERROR;
                (void)NANO_IP_OAL_FLAGS_Reset(&socket->sync_flags, NANO_IP_OAL_FLAGS_ALL);
                (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                ret = NANO_IP_OAL_FLAGS_Wait(&socket->sync_flags, &mask, true, NANO_IP_MAX_TIMEOUT_VALUE);
                (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                if (socket->is_free)
                {
                    ret = NIP_ERR_FAILURE;
                }
                if (ret == NIP_ERR_SUCCESS)
                {
                    /* Check event */
                    if ((mask & SOCKET_EVENT_TX) != 0)
                    {
                        /* Check if connection succeed */
                        if (socket->connection_handle.tcp.state != TCP_STATE_ESTABLISHED)
                        {
                            ret = NIP_ERR_FAILURE;    
                        }
                    }
                    if ((mask & SOCKET_EVENT_ERROR) != 0)
                    {
                        /* Error */
                        ret = NIP_ERR_FAILURE;
                    }
                }
            }
            else
            {
                
                ret = NIP_ERR_IN_PROGRESS;
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

#endif /* NANO_IP_ENABLE_TCP */


/** \brief Set/unset the non-blocking option to a socket */
nano_ip_error_t NANO_IP_SOCKET_SetNonBlocking(const uint32_t socket_id, const bool non_blocking)
{
    nano_ip_socket_t* socket;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    socket = NANO_IP_SOCKET_Get(socket_id);
    if (socket != NULL)
    {
        /* Apply option */
        if (non_blocking)
        {
            socket->options |= NIPSOCK_OPT_NON_BLOCK;
        }
        else
        {
            socket->options &= ~(NIPSOCK_OPT_NON_BLOCK);
        }

        ret = NIP_ERR_SUCCESS;
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}


#if (NANO_IP_ENABLE_SOCKET_POLL == 1u)

/** \brief Wait for events on an array of sockets */
nano_ip_error_t NANO_IP_SOCKET_Poll(nano_ip_socket_poll_data_t* const poll_datas, const uint32_t count, const uint32_t timeout, uint32_t* const poll_count)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if ((poll_datas != NULL) && 
        (count != 0u) &&
        (poll_count != NULL))
    {   
        uint32_t i;
        nano_ip_socket_poll_t* poll = NULL;
        nano_ip_socket_module_data_t* const socket_module = &g_nano_ip.socket_module;

        /* Look for a free poll handle */
        i = 0;
        while ((i < NANO_IP_SOCKET_MAX_POLL_COUNT) && (poll == NULL))
        {
            if (socket_module->polls[i].is_free)
            {
                poll = &socket_module->polls[i];
            }
            i++;
        }
        if (poll == NULL)
        {
            ret = NIP_ERR_RESOURCE;
        }
        else
        {
            (*poll_count) = 0u;
            ret = NIP_ERR_SUCCESS;
            do
            {
                /* Check for events in the poll array */
                for (i = 0; ((i < count) && (ret == NIP_ERR_SUCCESS)); i++)
                {
                    nano_ip_socket_poll_data_t* const poll_data = &poll_datas[i];
                    nano_ip_socket_t* const socket = NANO_IP_SOCKET_Get(poll_data->socket_id);
                    if (socket == NULL)
                    {
                        ret = NIP_ERR_INVALID_ARG;
                    }
                    else
                    {
                        /* Read socket synchronisation flags */
                        uint32_t flags = SOCKET_EVENT_ALL;
                        (void)NANO_IP_OAL_FLAGS_Wait(&socket->sync_flags, &flags, true, 0u);

                        /* Reset returned events */
                        poll_data->ret_events = 0u;
                        if ((poll_data->req_events & NIPSOCK_POLLIN) != 0u)
                        {
                            if (((flags & SOCKET_EVENT_RX) != 0u) ||
                                (!NANO_IP_PACKET_QueueIsEmpty(&socket->rx_packets)))
                            {
                                poll_data->ret_events |= NIPSOCK_POLLIN; 
                            }
                        }
                        if ((poll_data->req_events & NIPSOCK_POLLOUT) != 0u)
                        {
                            if (((flags & SOCKET_EVENT_TX) != 0u) ||
                                ((socket->type == NIPSOCK_UDP) && (NANO_IP_UDP_HandleIsReady(&socket->connection_handle.udp))))
                            {
                                poll_data->ret_events |= NIPSOCK_POLLOUT; 
                            }
                        }
                        if ((poll_data->req_events & NIPSOCK_POLLERR) != 0u)
                        {
                            if ((flags & SOCKET_EVENT_ERROR) != 0u)
                            {
                                poll_data->ret_events |= NIPSOCK_POLLERR; 
                            }
                        }

                        /* Update poll count */
                        if (poll_data->ret_events != 0u)
                        {
                            (*poll_count)++;
                        }

                        /* Update poll object */
                        socket->poll = poll;
                    }
                }

                /* Wait for a poll event */
                if ((ret == NIP_ERR_SUCCESS) && ((*poll_count) == 0u))
                {
                    uint32_t flags = NANO_IP_OAL_FLAGS_ALL;
                    (void)NANO_IP_OAL_FLAGS_Reset(&poll->sync_flags, NANO_IP_OAL_FLAGS_ALL);
                    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
                    ret = NANO_IP_OAL_FLAGS_Wait(&poll->sync_flags, &flags, true, timeout);
                    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);
                }
            }
            while ((ret == NIP_ERR_SUCCESS) && ((*poll_count) == 0u));

            /* Release poll handle */
            poll->is_free = true;
            for (i = 0; (i < count); i++)
            {
                nano_ip_socket_poll_data_t* const poll_data = &poll_datas[i];
                nano_ip_socket_t* const socket = NANO_IP_SOCKET_Get(poll_data->socket_id);
                if (socket != NULL)
                {
                    socket->poll = NULL;
                }
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

#endif /* NANO_IP_ENABLE_SOCKET_POLL */



/** \brief Look for an initialized socket in the socket array */
static nano_ip_socket_t* NANO_IP_SOCKET_Get(const uint32_t socket_id)
{
    nano_ip_socket_t* socket = NULL;

    /* Check id validity */
    if (socket_id < NANO_IP_SOCKET_MAX_COUNT)
    {
        /* Check if socket is free */
        if (!g_nano_ip.socket_module.sockets[socket_id].is_free)
        {
            socket = &g_nano_ip.socket_module.sockets[socket_id];
        }
    }

    return socket;
}

/** \brief Callback for UDP sockets */
static bool NANO_IP_SOCKET_UdpCallback(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data)
{
    const bool release_packet = false;
    nano_ip_socket_t* const socket = NANO_IP_CAST(nano_ip_socket_t*, user_data);

    /* Check parameterss */
    if ((socket != NULL) && (!socket->is_free))
    {
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        /* Check event */
        switch (event)
        {
            case UDP_EVENT_RX:
            {
                /* Queue received packet */
                NANO_IP_PACKET_AddToQueue(&socket->rx_packets, event_data->packet);

                /* Signal packet reception */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_RX, false);

                break;
            }

            case UDP_EVENT_TX:
            {
                /* Signal ready to transmit */
                if ((socket->type == NIPSOCK_UDP) ||
                    ((socket->type == NIPSOCK_TCP) && (socket->connection_handle.tcp.state == TCP_STATE_ESTABLISHED)))
                {
                    (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_TX, false);
                }
                break;
            }

            default:
            {
                /* Error */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_ERROR, false);
                break;
            }
        }

        #if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
        /* Notify poll function */
        if (socket->poll != NULL)
        {
            (void)NANO_IP_OAL_FLAGS_Set(&socket->poll->sync_flags, SOCKET_EVENT_ALL, false);
        }
        #endif /* NANO_IP_ENABLE_SOCKET_POLL */

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }
    
    return release_packet;
}

/** \brief Callback for TCP sockets */
static bool NANO_IP_SOCKET_TcpCallback(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data)
{
    const bool release_packet = false;
    nano_ip_socket_t* const socket = NANO_IP_CAST(nano_ip_socket_t*, user_data);

    /* Check parameterss */
    if ((socket != NULL) && (!socket->is_free))
    {
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        /* Check event */
        switch (event)
        {
            case TCP_EVENT_RX:
            {
                /* Queue received packet */
                NANO_IP_PACKET_AddToQueue(&socket->rx_packets, event_data->packet);

                /* Signal packet reception */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_RX, false);

                break;
            }

            case TCP_EVENT_TX:
            {
                /* Signal ready to transmit */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_TX, false);

                break;
            }

            case TCP_EVENT_CONNECTED:
            {
                /* Signal ready to transmit */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_TX, false);
                break;
            }

            case TCP_EVENT_ACCEPTING:
            {
                /* Accepting a new client */
                if (socket->child_count < socket->max_child_count)
                {
                    /* Look for a free socket */
                    uint32_t socket_id = 0u;
                    const nano_ip_error_t ret = NANO_IP_SOCKET_Allocate(&socket_id, NIPSOCK_TCP);
                    if (ret == NIP_ERR_SUCCESS)
                    {
                        /* Provide handle to the stack */
                        nano_ip_socket_module_data_t* const socket_module = &g_nano_ip.socket_module;
                        nano_ip_socket_t* const client_socket = &socket_module->sockets[socket_id];
                        (*event_data->accept_handle) = &client_socket->connection_handle.tcp;

                        /* Add socket to the pending list */
                        client_socket->next = socket->accept_pending_sockets;
                        socket->accept_pending_sockets = client_socket;

                        client_socket->parent = socket;
                        socket->child_count++;
                    }
                }
                break;
            }

            case TCP_EVENT_ACCEPTED:
            {
                /* Remove the socket from the pending list */
                nano_ip_socket_t* const parent_socket = socket->parent;
                nano_ip_socket_t* sock = parent_socket->accept_pending_sockets;
                nano_ip_socket_t* previous_sock = NULL;
                while ((sock != NULL) && (sock != socket))
                {
                    previous_sock = sock;
                    sock = sock->next;
                }
                if (sock != NULL)
                {
                    if (previous_sock != NULL)
                    {
                        previous_sock->next = sock->next;
                    }
                    else
                    {
                        parent_socket->accept_pending_sockets = sock->next;
                    }
                }

                /* Add socket to the accepted list */
                if (parent_socket->accepted_sockets == NULL)
                {
                    parent_socket->accepted_sockets = socket;
                }
                else
                {
                    previous_sock = parent_socket->accepted_sockets;
                    while (previous_sock->next != NULL)
                    {
                        previous_sock = previous_sock->next;
                    }
                    previous_sock->next = socket;
                }
                socket->next = NULL;

                /* Signal accept */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_RX, false);

                break;
            }

            case TCP_EVENT_ACCEPT_FAILED:
            {
                /* Remove the socket from the pending list */
                nano_ip_socket_t* const parent_socket = socket->parent;
                nano_ip_socket_t* sock = parent_socket->accept_pending_sockets;
                nano_ip_socket_t* previous_sock = NULL;
                while ((sock != NULL) && (sock != socket))
                {
                    previous_sock = sock;
                    sock = sock->next;
                }
                if (sock != NULL)
                {
                    if (previous_sock != NULL)
                    {
                        previous_sock->next = sock->next;
                    }
                    else
                    {
                        parent_socket->accept_pending_sockets = sock->next;
                    }
                }

                /* Socket is now free */
                (void)NANO_IP_SOCKET_Release(socket->id);
                socket->child_count--;
                break;
            }

            case TCP_EVENT_CLOSED:
            {
                /* Update parent socket */
                if (socket->parent != NULL)
                {
                    socket->parent->child_count--;
                }

                /* Notify error */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_ERROR, false);
                break;
            }

            case TCP_EVENT_TX_FAILED:
            /* Intended fallthrough */
            case TCP_EVENT_CONNECT_TIMEOUT:
            /* Intended fallthrough */
            default:
            {
                /* Error */
                (void)NANO_IP_OAL_FLAGS_Set(&socket->sync_flags, SOCKET_EVENT_ERROR, false);
                break;
            }
        }

        #if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
        /* Notify poll function */
        if ((!socket->is_free) && (socket->poll != NULL))
        {
            (void)NANO_IP_OAL_FLAGS_Set(&socket->poll->sync_flags, SOCKET_EVENT_ALL, false);
        }
        #endif /* NANO_IP_ENABLE_SOCKET_POLL */

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return release_packet;    
}


#endif /* NANO_IP_ENABLE_SOCKET */
