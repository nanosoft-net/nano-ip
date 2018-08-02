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

#ifndef NANO_IP_ETHERNET_H
#define NANO_IP_ETHERNET_H


#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_net_if.h"
#include "nano_ip_ethernet_def.h"




#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Ethernet header */
typedef struct _ethernet_header_t
{
    /** \brief Destination address */
    uint8_t dest_address[MAC_ADDRESS_SIZE];
    /** \brief Source address */
    uint8_t src_address[MAC_ADDRESS_SIZE];
    /** \brief Ether type */
    uint16_t ether_type;
} ethernet_header_t;


/** \brief Ethernet protocol */
typedef struct _nano_ip_ethernet_protocol_t
{
    /** \brief Ether type */
    uint16_t ether_type;
    /** \brief Frame handling function */
    nano_ip_error_t (*rx_frame)(void* user_data, nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet);
    /** \brief User data */
    void* user_data;
    /* Next protocol */
    struct _nano_ip_ethernet_protocol_t* next;
} nano_ip_ethernet_protocol_t;

/** \brief Ethernet periodic callback */
typedef struct _nano_ip_ethernet_periodic_callback_t
{
    /** \brief Callback */
    void (*callback)(const uint32_t timestamp, void* const user_data);
    /** \brief User data */
    void* user_data;
    /** \brief Next callback */
    struct _nano_ip_ethernet_periodic_callback_t* next;
} nano_ip_ethernet_periodic_callback_t;



/** \brief Ethernet module internal data */
typedef struct _nano_ip_ethernet_module_data_t
{
    /** \brief Ethernet protocols */
    nano_ip_ethernet_protocol_t* eth_protocols;
    /** \brief Ethernet callbacks */
    nano_ip_ethernet_periodic_callback_t* callbacks;
} nano_ip_ethernet_module_data_t;




/** \brief Initialize the ethernet module */
nano_ip_error_t NANO_IP_ETHERNET_Init(void);

/** \brief Add an ethernet protocol */
nano_ip_error_t NANO_IP_ETHERNET_AddProtocol(nano_ip_ethernet_protocol_t* const protocol);

/** \brief Handle a received frame */
nano_ip_error_t NANO_IP_ETHERNET_RxFrame(nano_ip_net_if_t* const net_if, nano_ip_net_packet_t* const packet);

/** \brief Allocate a packet for an ethernet frame */
nano_ip_error_t NANO_IP_ETHERNET_AllocatePacket(const uint16_t packet_size, nano_ip_net_packet_t** packet);

/** \brief Send an ethernet packet on a specific network interface */
nano_ip_error_t NANO_IP_ETHERNET_SendPacket(nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet);

/** \brief Release an ethernet packet */
nano_ip_error_t NANO_IP_ETHERNET_ReleasePacket(nano_ip_net_packet_t* const packet);

/** \brief Register a periodic callback */
nano_ip_error_t NANO_IP_ETHERNET_RegisterPeriodicCallback(nano_ip_ethernet_periodic_callback_t* const callback);

/** \brief Ethernet periodic task */
nano_ip_error_t NANO_IP_ETHERNET_PeriodicTask(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_ETHERNET_H */
