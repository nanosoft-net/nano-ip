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

#ifndef NANO_IP_UDP_H
#define NANO_IP_UDP_H

#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_UDP == 1 )

#include "nano_ip_types.h"
#include "nano_ip_ipv4.h"



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  
/** \brief UDP header */
typedef struct _udp_header_t
{
    /** \brief IPv4 header */
    const ipv4_header_t* ipv4_header;
    /** \brief Source port */
    uint16_t src_port;
    /** \brief Destination port */
    uint16_t dest_port;
} udp_header_t;


/** \brief UDP event */
typedef enum _nano_ip_udp_event_t
{
    /** \brief Datagram received */
    UDP_EVENT_RX = 0u,
    /** \brief Datagram sent */
    UDP_EVENT_TX = 1u,
    /** \brief Datagram failed to sent */
    UDP_EVENT_TX_FAILED = 2u,
    /** \brief ERROR */
    UDP_EVENT_ERROR = 3u
} nano_ip_udp_event_t;

/** \brief UDP event data */
typedef struct _nano_ip_udp_event_data_t
{
    /** \brief UDP header */
    udp_header_t* udp_header;
    /** \brief Error */
    nano_ip_error_t error;
    /** \brief Packet */
    nano_ip_net_packet_t* packet;
} nano_ip_udp_event_data_t;

/** \brief UDP callback */
typedef bool (*fp_udp_callback_t)(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data);

/** \brief UDP handle */
typedef struct _nano_ip_udp_handle_t
{
    /** \brief IPv4 address */
    ipv4_address_t ipv4_address;
    /** \brief Port */
    uint16_t port;
    /** \brief Callback */
    fp_udp_callback_t callback;
    /** \brief IPv4 handle */
    nano_ip_ipv4_handle_t ipv4_handle;
    /** \brief Indicate if the handle is bound  to an address */
    bool is_bound;
    /** \brief User data */
    void* user_data;
    /** \brief Next handle */
    struct _nano_ip_udp_handle_t* next;
} nano_ip_udp_handle_t;


/** \brief UDP module internal data */
typedef struct _nano_ip_udp_module_data_t
{
    /** \brief IPv4 protocol description */
    nano_ip_ipv4_protocol_t ipv4_protocol;
    /** \brief UDP handle list */
    nano_ip_udp_handle_t* handles;
} nano_ip_udp_module_data_t;



/** \brief Initialize the UDP module */
nano_ip_error_t NANO_IP_UDP_Init(void);

/** \brief Initialize an UDP handle */
nano_ip_error_t NANO_IP_UDP_InitializeHandle(nano_ip_udp_handle_t* const handle, const fp_udp_callback_t callback, void* const user_data);

/** \brief Release an UDP handle */
nano_ip_error_t NANO_IP_UDP_ReleaseHandle(nano_ip_udp_handle_t* const handle);

/** \brief Bind an UDP handle to a specific address and port */
nano_ip_error_t NANO_IP_UDP_Bind(nano_ip_udp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port);

/** \brief Unbind an UDP handle from a specific address and port */
nano_ip_error_t NANO_IP_UDP_Unbind(nano_ip_udp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port);

/** \brief Allocate a packet for an UDP frame */
nano_ip_error_t NANO_IP_UDP_AllocatePacket(nano_ip_net_packet_t** packet, const uint16_t packet_size);

/** \brief Send an UDP frame */
nano_ip_error_t NANO_IP_UDP_SendPacket(nano_ip_udp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port, nano_ip_net_packet_t* const packet);

/** \brief Indicate if an UDP handle is ready */
nano_ip_error_t NANO_IP_UDP_HandleIsReady(nano_ip_udp_handle_t* const handle);

/** \brief Release an UDP frame */
nano_ip_error_t NANO_IP_UDP_ReleasePacket(nano_ip_net_packet_t* const packet);

/** \brief Reads the UDP header of a packet */
nano_ip_error_t NANO_IP_UDP_ReadHeader(nano_ip_net_packet_t* const packet, ipv4_address_t* const src_address, uint16_t* const src_port);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_UDP */

#endif /* NANO_IP_UDP_H */
