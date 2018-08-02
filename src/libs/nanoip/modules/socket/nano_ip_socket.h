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

#ifndef NANO_IP_SOCKET_H
#define NANO_IP_SOCKET_H

#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_SOCKET == 1 )

#include "nano_ip_udp.h"
#include "nano_ip_tcp.h"



/** \brief Invalid socket id */
#define NANO_IP_INVALID_SOCKET_ID       0xFFFFFFFFu

/** \brief Maximum number of incoming connections on a socket */
#define NANO_IP_SOMAXCONN               (NANO_IP_SOCKET_MAX_COUNT - 1u)


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Socket poll data */
typedef struct _nano_ip_socket_poll_data_t
{
    /** \brief Socket id */
    uint32_t socket_id;
    /** \brief Requested events */
    uint16_t req_events;
    /** \brief Returned events */
    uint16_t ret_events;
} nano_ip_socket_poll_data_t;

/** \brief Socket poll handle */
typedef struct _nano_ip_socket_poll_t
{
    /** \brief Indicate if the poll handle is free */
    bool is_free;
    /** \brief Synchronisation object */
    oal_flags_t sync_flags;
} nano_ip_socket_poll_t;

/** \brief Socket polling events */
typedef enum _nano_ip_socket_poll_event_t
{
    /** \brief Data received */
    NIPSOCK_POLLIN = 1u,
    /** \brief Ready to transmit */
    NIPSOCK_POLLOUT = 2u,
    /** \brief Error */
    NIPSOCK_POLLERR = 4u
} nano_ip_socket_poll_event_t;


 
/** \brief Socket types */
typedef enum _nano_ip_socket_type_t
{
    /** \brief UDP */
    NIPSOCK_UDP = 0u,
    /** \brief TCP */
    NIPSOCK_TCP = 1u
} nano_ip_socket_type_t;



/** \brief Socket options */
typedef enum _nano_ip_socket_option_t
{
    /** \brief Non blocking socket */
    NIPSOCK_OPT_NON_BLOCK = 1u
} nano_ip_socket_option_t;






/** \brief Socket data */
typedef struct _nano_ip_socket_t
{
    /** \brief Indicate if the socket is free */
    bool is_free;
    /** \brief Socket type */
    nano_ip_socket_type_t type;
    /** \brief Socket options */
    uint32_t options;
    /** \brief Received packets */
    nano_ip_packet_queue_t rx_packets;
    /** \brief Synchronization object */
    oal_flags_t sync_flags;

    /** \brief Underlying connection handle */
    union
    {
        #if (NANO_IP_ENABLE_UDP == 1u)
        /** \brief UDP handle */
        nano_ip_udp_handle_t udp;
        #endif /* NANO_IP_ENABLE_UDP */

        #if (NANO_IP_ENABLE_TCP == 1u)
        /** \brief TCP handle */
        nano_ip_tcp_handle_t tcp;
        #endif /* NANO_IP_ENABLE_TCP */
    } connection_handle;

    #if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
    /** \brief Poll object */
    nano_ip_socket_poll_t* poll;
    #endif /* NANO_IP_ENABLE_SOCKET_POLL */

    #if (NANO_IP_ENABLE_TCP == 1u)
    /** \brief Socket id */
    uint32_t id;
    /** \brief Parent socket */
    struct _nano_ip_socket_t* parent;
    /** \brief Child sockets count */
    uint32_t child_count;
    /** \brief Maximum child sockets count */
    uint32_t max_child_count;
    /** \brief Accept pending sockets */
    struct _nano_ip_socket_t* accept_pending_sockets;
    /** \brief Accepted sockets */
    struct _nano_ip_socket_t* accepted_sockets;
    /** \brief Next socket */
    struct _nano_ip_socket_t* next;
    #endif /* NANO_IP_ENABLE_TCP */

} nano_ip_socket_t;

/** \brief Socket endpoint */
typedef struct _nano_ip_socket_endpoint_t
{
    /** \brief IPv4 address */
    ipv4_address_t address;
    /** \brief Port */
    uint16_t port;
} nano_ip_socket_endpoint_t;


/** \brief Socket module internal data */
typedef struct _nano_ip_socket_module_data_t
{
    /** \brief Module mutex */
    oal_mutex_t mutex;
    /** \brief Socket array */
    nano_ip_socket_t sockets[NANO_IP_SOCKET_MAX_COUNT];

    #if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
    /** \brief Socket poll array */
    nano_ip_socket_poll_t polls[NANO_IP_SOCKET_MAX_POLL_COUNT];
    #endif /* NANO_IP_ENABLE_SOCKET_POLL */

} nano_ip_socket_module_data_t;




/** \brief Initialize the socket module */
nano_ip_error_t NANO_IP_SOCKET_Init(void);

/** \brief Allocate a socket */
nano_ip_error_t NANO_IP_SOCKET_Allocate(uint32_t* const socket_id, const nano_ip_socket_type_t type);

/** \brief Release a socket */
nano_ip_error_t NANO_IP_SOCKET_Release(const uint32_t socket_id);

/** \brief Bind a socket to a specific address and port */
nano_ip_error_t NANO_IP_SOCKET_Bind(const uint32_t socket_id, const nano_ip_socket_endpoint_t* const end_point);

/** \brief Receive data from a socket */
nano_ip_error_t NANO_IP_SOCKET_ReceiveFrom(const uint32_t socket_id, void* const data, const size_t size, size_t* const received, nano_ip_socket_endpoint_t* const end_point);

/** \brief Send data to a socket */
nano_ip_error_t NANO_IP_SOCKET_SendTo(const uint32_t socket_id, const void* const data, const size_t size, size_t* const sent, const nano_ip_socket_endpoint_t* const end_point);

#if (NANO_IP_ENABLE_TCP == 1u)

/** \brief Receive data from a socket */
nano_ip_error_t NANO_IP_SOCKET_Receive(const uint32_t socket_id, void* const data, const size_t size, size_t* const received);

/** \brief Send data to a socket */
nano_ip_error_t NANO_IP_SOCKET_Send(const uint32_t socket_id, const void* const data, const size_t size, size_t* const sent);

/** \brief Put a socket into listen state */
nano_ip_error_t NANO_IP_SOCKET_Listen(const uint32_t socket_id, const uint32_t max_incoming_connections);

/** \brief Accept a connection on a socket */
nano_ip_error_t NANO_IP_SOCKET_Accept(const uint32_t socket_id, uint32_t* const client_socket_id, nano_ip_socket_endpoint_t* const end_point);

/** \brief Connect a socket to a specific address and port */
nano_ip_error_t NANO_IP_SOCKET_Connect(const uint32_t socket_id, const nano_ip_socket_endpoint_t* const end_point);

#endif /* NANO_IP_ENABLE_TCP */

/** \brief Set/unset the non-blocking option to a socket */
nano_ip_error_t NANO_IP_SOCKET_SetNonBlocking(const uint32_t socket_id, const bool non_blocking);

#if (NANO_IP_ENABLE_SOCKET_POLL == 1u)
/** \brief Wait for events on an array of sockets */
nano_ip_error_t NANO_IP_SOCKET_Poll(nano_ip_socket_poll_data_t* const poll_datas, const uint32_t count, const uint32_t timeout, uint32_t* const poll_count);
#endif /* NANO_IP_ENABLE_SOCKET_POLL */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_SOCKET */

#endif /* NANO_IP_SOCKET_H */
