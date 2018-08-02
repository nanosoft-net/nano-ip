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

#ifndef NANO_IP_IPV4_H
#define NANO_IP_IPV4_H


#include "nano_ip_types.h"
#include "nano_ip_oal.h"
#include "nano_ip_ethernet.h"
#include "nano_ip_arp.h"



/** \brief Offset of the source IP adress in a IPv4 frame header */
#define IPV4_SOURCE_ADDRESS_OFFSET      12u



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief IPv4 header */
typedef struct _ipv4_header_t
{
    /** \brief Ethernet header */
    const ethernet_header_t* eth_header;
    /** \brief Source address */
    ipv4_address_t src_address;
    /** \brief Destination address */
    ipv4_address_t dest_address;
    /** \brief Data length */
    uint16_t data_length;
    /** \brief Protocol */
    uint8_t protocol;
} ipv4_header_t;


/** \brief IPv4 error callback */
typedef void (*fp_ipv4_error_callback_t)(void* const user_data, const nano_ip_error_t error);


/** \brief IPv4 handle */
typedef struct _nano_ip_ipv4_handle_t
{
    /** \brief Header */
    ipv4_header_t header;
    /** \brief ARP request handle */
    nano_ip_arp_request_t arp_request;
    /** \brief Network interface */
    nano_ip_net_if_t* net_if;
    /** \brief Packet to send */
    nano_ip_net_packet_t* packet;
    /** \brief Error callback */
    fp_ipv4_error_callback_t error_callback;
    /** \brief User data */
    void* user_data;
    /** \brief Busy flag */
    bool busy;
} nano_ip_ipv4_handle_t;


/** \brief IPv4 protocol */
typedef struct _nano_ip_ipv4_protocol_t
{
    /** \brief Protocol */
    uint8_t protocol;
    /** \brief Frame handling function */
    nano_ip_error_t(*rx_frame)(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ip_header, nano_ip_net_packet_t* const packet);
    /** \brief User data */
    void* user_data;
    /* Next protocol */
    struct _nano_ip_ipv4_protocol_t* next;
} nano_ip_ipv4_protocol_t;


/** \brief IPv4 periodic callback */
typedef struct _nano_ip_ipv4_periodic_callback_t
{
    /** \brief Callback */
    void(*callback)(const uint32_t timestamp, void* const user_data);
    /** \brief User data */
    void* user_data;
    /** \brief Next callback */
    struct _nano_ip_ipv4_periodic_callback_t* next;
} nano_ip_ipv4_periodic_callback_t;



/** \brief IPv4 module internal data */
typedef struct _nano_ip_ipv4_module_data_t
{
    /** \brief IPv4 protocol description */
    nano_ip_ethernet_protocol_t ipv4_protocol;
    /** \brief Ethernet periodic callback */
    nano_ip_ethernet_periodic_callback_t eth_callback;
    /** \brief IPv4 protocols */
    nano_ip_ipv4_protocol_t* protocols;
    /** \brief IPv4 callbacks */
    nano_ip_ipv4_periodic_callback_t* callbacks;
} nano_ip_ipv4_module_data_t;




/** \brief Initialize the IPv4 module */
nano_ip_error_t NANO_IP_IPV4_Init(void);

/** \brief Add an IPv4 protocol */
nano_ip_error_t NANO_IP_IPV4_AddProtocol(nano_ip_ipv4_protocol_t* const protocol);

/** \brief Initialize an IPv4 handle */
nano_ip_error_t NANO_IP_IPV4_InitializeHandle(nano_ip_ipv4_handle_t* const ipv4_handle, void* const user_data, const fp_ipv4_error_callback_t error_callback);

/** \brief Release an IPv4 handle */
nano_ip_error_t NANO_IP_IPV4_ReleaseHandle(nano_ip_ipv4_handle_t* const ipv4_handle);

/** \brief Allocate a packet for an IPv4 frame */
nano_ip_error_t NANO_IP_IPV4_AllocatePacket(const uint16_t packet_size, nano_ip_net_packet_t** packet);

/** \brief Send an IPv4 frame */
nano_ip_error_t NANO_IP_IPV4_SendPacket(nano_ip_ipv4_handle_t* const ipv4_handle, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet);

/** \brief Indicate if an IPv4 handle is ready */
nano_ip_error_t NANO_IP_IPV4_HandleIsReady(nano_ip_ipv4_handle_t* const ipv4_handle);

/** \brief Release an IPv4 frame */
nano_ip_error_t NANO_IP_IPV4_ReleasePacket(nano_ip_net_packet_t* const packet);

/** \brief Register a periodic callback */
nano_ip_error_t NANO_IP_IPV4_RegisterPeriodicCallback(nano_ip_ipv4_periodic_callback_t* const callback);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_IPV4_H */
