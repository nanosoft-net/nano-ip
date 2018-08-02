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

#ifndef NANO_IP_NET_DRIVER_H
#define NANO_IP_NET_DRIVER_H

#include "nano_ip_error.h"
#include "nano_ip_packet_allocator.h"
#include "nano_ip_ipv4_def.h"

/*    Network drivers capabilities */


/** \brief Ethernet minimum frame size check */
#define NETDRV_CAPS_ETH_MIN_FRAME_SIZE          1u

/** \brief Ethernet checksum computation */
#define NETDRV_CAP_ETH_CS_COMPUTATION           2u

/** \brief Ethernet checksum verification */
#define NETDRV_CAP_ETH_CS_CHECK                 4u

/** \brief Ethernet destination MAC address verification */
#define NETDRV_CAP_DEST_MAC_ADDR_CHECK          8u

/** \brief Ethernet frame padding */
#define NETDRV_CAP_ETH_FRAME_PADDING            16u

/** \brief IPv4 checksum computation */
#define NETDRV_CAP_IPV4_CS_COMPUTATION          32u

/** \brief IPv4 checksum verification */
#define NETDRV_CAP_IPV4_CS_CHECK                64u

/** \brief IPv4 address verification */
#define NETDRV_CAP_IPV4_ADDRESS_CHECK           128u

/** \brief TCP/IPv4 checksum computation */
#define NETDRV_CAP_TCPIPV4_CS_COMPUTATION       256u

/** \brief TCP/IPv4 checksum verification */
#define NETDRV_CAP_TCPIPV4_CS_CHECK             512u

/** \brief UDP/IPv4 checksum computation */
#define NETDRV_CAP_UDPIPV4_CS_COMPUTATION       1024u

/** \brief UDP/IPv4 checksum verification */
#define NETDRV_CAP_UDPIPV4_CS_CHECK             2048u


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Network driver speeds */
typedef enum _net_driver_speed_t
{
    /** \brief 10Mbits */
    SPEED_10    = 0u,
    /** \brief 100Mbits */
    SPEED_100   = 1u,
    /** \brief 1000Mbits */
    SPEED_1000  = 2u
} net_driver_speed_t;

/** \brief Network driver duplexes */
typedef enum _net_driver_duplex_t
{
    /** \brief Half duplex */
    DUP_HALF    = 0u,
    /** \brief Full duplex */
    DUP_FULL    = 1u,
    /** \brief Auto-negociation */
    DUP_AUTO    = 2u
} net_driver_duplex_t;


/** \brief Network link state */
typedef enum _net_link_state_t
{
    /** \brief Link down */
    NLS_DOWN =          0u,
    /** \brief Link up 10Mbits Half Duplex */
    NLS_UP_10_HD =      1u,
    /** \brief Link up 10Mbits Full Duplex */
    NLS_UP_10_FD =      2u,
    /** \brief Link up 100Mbits Half Duplex */
    NLS_UP_100_HD =     3u,
    /** \brief Link up 100Mbits Full Duplex */
    NLS_UP_100_FD =     4u,
    /** \brief Link up 1000Mbits Half Duplex */
    NLS_UP_1000_HD =    5u,
    /** \brief Link up 1000Mbits Full Duplex */
    NLS_UP_1000_FD =    6u,
    /** \brief Auto-negociation in progress */
    NLS_AUTO_NEGO =     7u,
    /** \brief Link up, speed and duplex unknown */
    NLS_UP =            8u
} net_link_state_t;


/** \brief Network driver callbacks */
typedef struct _net_driver_callbacks_t
{
    /** \brief Called when a packet has been received */
    void (*packet_received)(void* const stack_data, const bool from_isr);
    /** \brief Called when a packet has been sent */
    void (*packet_sent)(void* const stack_data, const bool from_isr);
    /** \brief Called when a network driver error occured */
    void (*net_drv_error)(void* const stack_data, const bool from_isr);
    /** \brief Called when the link state has changed */
    void (*link_state_changed)(void* const stack_data, const bool from_isr);
    /** \brief Stack data */
    void* stack_data;
} net_driver_callbacks_t;


/** \brief Network driver interface */
typedef struct _nano_ip_net_driver_t
{
    /** \brief Capabilities */
    uint32_t caps;
    /** \brief User defined data */
    void* user_data;

    /** \brief Init the driver */
    nano_ip_error_t (*init)(void* const user_data, net_driver_callbacks_t* const callbacks);
    /** \brief Start the driver */
    nano_ip_error_t (*start)(void* const user_data);
    /** \brief Stop the driver */
    nano_ip_error_t (*stop)(void* const user_data);
    /** \brief Set the MAC address */
	nano_ip_error_t (*set_mac_address)(void* const user_data, const uint8_t* const mac_address);
	/** \brief Set the IPv4 address */
	nano_ip_error_t (*set_ipv4_address)(void* const user_data, const ipv4_address_t ipv4_address, const ipv4_address_t ipv4_netmask);
    /** \brief Send a packet */
    nano_ip_error_t (*send_packet)(void* const user_data, nano_ip_net_packet_t* const packet);
    /** \brief Add a packet for reception */
    nano_ip_error_t (*add_rx_packet)(void* const user_data, nano_ip_net_packet_t* const packet);
    /** \brief Get the next received packet */
    nano_ip_error_t(*get_next_rx_packet)(void* const user_data, nano_ip_net_packet_t** const packet);
    /** \brief Get the next sent packet */
    nano_ip_error_t(*get_next_tx_packet)(void* const user_data, nano_ip_net_packet_t** const packet);
    /** \brief Get the link state*/
    nano_ip_error_t (*get_link_state)(void* const user_data, net_link_state_t* const state);
} nano_ip_net_driver_t;




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_NET_DRIVER_H */
