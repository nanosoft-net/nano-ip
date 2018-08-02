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

#ifndef NANO_IP_TCP_H
#define NANO_IP_TCP_H

#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_TCP == 1 )

#include "nano_ip_types.h"
#include "nano_ip_ipv4.h"



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  
/** \brief TCP header */
typedef struct _tcp_header_t
{
    /** \brief IPv4 header */
    const ipv4_header_t* ipv4_header;
    /** \brief Source port */
    uint16_t src_port;
    /** \brief Destination port */
    uint16_t dest_port;
    /** \brief Sequence number */
    uint32_t seq_number;
    /** \brief Acknowledgement number */
    uint32_t ack_number;
    /** \brief Data offset */
    uint8_t data_offset;
    /** \brief Flags */
    uint8_t flags;
    /** \brief Window size */
    uint16_t window;
} tcp_header_t;

/** \brief TCP event */
typedef enum _nano_ip_tcp_event_t
{
    /** \brief Data received */
    TCP_EVENT_RX = 0u,
    /** \brief Data sent */
    TCP_EVENT_TX = 1u,
    /** \brief Data failed to sent */
    TCP_EVENT_TX_FAILED = 2u,
    /** \brief Connected */
    TCP_EVENT_CONNECTED = 3u,
    /** \brief Connection timedout */
    TCP_EVENT_CONNECT_TIMEOUT = 4u,
    /** \brief Connection closed */
    TCP_EVENT_CLOSED = 5u,
    /** \brief Accepting new connection */
    TCP_EVENT_ACCEPTING = 6u,
    /** \brief New connection accepted */
    TCP_EVENT_ACCEPTED = 7u,
    /** \brief Failed to accept new connection */
    TCP_EVENT_ACCEPT_FAILED = 8u,
    /** \brief Error */
    TCP_EVENT_ERROR = 9u
} nano_ip_tcp_event_t;

/** \brief TCP handle pre-declaration */
typedef struct _nano_ip_tcp_handle_t nano_ip_tcp_handle_t;

/** \brief TCP event data */
typedef struct _nano_ip_tcp_event_data_t
{
    /** \brief Error */
    nano_ip_error_t error;
    /** \brief Packet */
    nano_ip_net_packet_t* packet;
    /** \brief TCP accept handle */
    nano_ip_tcp_handle_t** accept_handle;
} nano_ip_tcp_event_data_t;

/** \brief TCP callback */
typedef bool (*fp_tcp_callback_t)(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data);

/** \brief TCP handle state */
typedef enum _nano_ip_tcp_handle_state_t
{
    /** \brief Closed */
    TCP_STATE_CLOSED = 0u,
    /** \brief Listenning */
    TCP_STATE_LISTEN = 1u,
    /** \brief SYN sent */
    TCP_STATE_SYN_SENT = 2u,
    /** \brief SYN received */
    TCP_STATE_SYN_RECEIVED = 3u,
    /** \brief Established */
    TCP_STATE_ESTABLISHED = 4u,
    /** \brief Fin wait 1 */
    TCP_STATE_FIN_WAIT_1 = 5u,
    /** \brief Fin wait 2 */
    TCP_STATE_FIN_WAIT_2 = 6u,
    /** \brief Close wait */
    TCP_STATE_CLOSE_WAIT = 7u,
    /** \brief Closing */
    TCP_STATE_CLOSING = 8u,
    /** \brief Last ack */
    TCP_STATE_LAST_ACK = 9u,
    /** \brief Time wait */
    TCP_STATE_TIME_WAIT = 10u,
    /** \brief Idle */
    TCP_STATE_IDLE = 255u
} nano_ip_tcp_handle_state_t;

/** \brief TCP handle */
typedef struct _nano_ip_tcp_handle_t
{
    /** \brief IPv4 address */
    ipv4_address_t ipv4_address;
    /** \brief IPv4 destination address */
    ipv4_address_t dest_ipv4_address;
    /** \brief Port */
    uint16_t port;
    /** \brief Destination port */
    uint16_t dest_port;
    /** \brief Callback */
    fp_tcp_callback_t callback;
    /** \brief IPv4 handle */
    nano_ip_ipv4_handle_t ipv4_handle;
    /** \brief State */
    nano_ip_tcp_handle_state_t state;
    /** \brief Sequence number */
    uint32_t seq_number;
    /** \brief Acknowledge number */
    uint32_t ack_number;
    /** \brief Last packet sent */
    nano_ip_net_packet_t* last_tx_packet;
    /** \brief Last packet sent position */
    uint8_t* last_tx_packet_pos;
    /** \brief Last packet sent count */
    uint16_t last_tx_packet_count;
    /** \brief Last packet IPv4 header */
    ipv4_header_t last_tx_packet_ipv4_header;
    /** \brief Retry count */
    uint8_t tx_retry_count;
    /** \brief State timeout */
    uint32_t state_timeout;
    /** \brief User data */
    void* user_data;
    /** \brief Next handle */
    struct _nano_ip_tcp_handle_t* next;
} nano_ip_tcp_handle_t;


/** \brief TCP module internal data */
typedef struct _nano_ip_tcp_module_data_t
{
    /** \brief IPv4 protocol description */
    nano_ip_ipv4_protocol_t ipv4_protocol;
    /** \brief IPv4 periodic callback */
    nano_ip_ipv4_periodic_callback_t ipv4_callback;
    /** \brief Next free local port */
    uint16_t next_free_local_port;
    /** \brief TCP handle list */
    nano_ip_tcp_handle_t* handles;
} nano_ip_tcp_module_data_t;



/** \brief Initialize the TCP module */
nano_ip_error_t NANO_IP_TCP_Init(void);

/** \brief Initialize a TCP handle */
nano_ip_error_t NANO_IP_TCP_InitializeHandle(nano_ip_tcp_handle_t* const handle, const fp_tcp_callback_t callback, void* const user_data);

/** \brief Release a TCP handle */
nano_ip_error_t NANO_IP_TCP_ReleaseHandle(nano_ip_tcp_handle_t* const handle);

/** \brief Open a TCP connection */
nano_ip_error_t NANO_IP_TCP_Open(nano_ip_tcp_handle_t* const handle, const uint16_t local_port);

/** \brief Bind a TCP handle to a specific address and port */
nano_ip_error_t NANO_IP_TCP_Bind(nano_ip_tcp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port);

/** \brief Establish a TCP connection */
nano_ip_error_t NANO_IP_TCP_Connect(nano_ip_tcp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port);

/** \brief Put a TCP handle into the listen state */
nano_ip_error_t NANO_IP_TCP_Listen(nano_ip_tcp_handle_t* const handle);

/** \brief Close a TCP connection */
nano_ip_error_t NANO_IP_TCP_Close(nano_ip_tcp_handle_t* const handle);

/** \brief Allocate a packet for a TCP frame */
nano_ip_error_t NANO_IP_TCP_AllocatePacket(nano_ip_net_packet_t** packet, const uint16_t packet_size);

/** \brief Send a TCP frame */
nano_ip_error_t NANO_IP_TCP_SendPacket(nano_ip_tcp_handle_t* const handle, nano_ip_net_packet_t* const packet);

/** \brief Indicate if a TCP handle is ready */
nano_ip_error_t NANO_IP_TCP_HandleIsReady(nano_ip_tcp_handle_t* const handle);

/** \brief Release a TCP frame */
nano_ip_error_t NANO_IP_TCP_ReleasePacket(nano_ip_net_packet_t* const packet);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_TCP */

#endif /* NANO_IP_TCP_H */
