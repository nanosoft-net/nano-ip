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

#ifndef NANO_IP_DHCP_CLIENT_H
#define NANO_IP_DHCP_CLIENT_H

#include "nano_ip_cfg.h"

#if( (NANO_IP_ENABLE_DHCP == 1) && (NANO_IP_ENABLE_DHCP_CLIENT == 1) )

#include "nano_ip_types.h"
#include "nano_ip_oal.h"
#include "nano_ip_udp.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Standard port used by DHCP server */
#define DHCP_SERVER_STANDARD_PORT              67u

/** \brief Standard port used by DHCP client */
#define DHCP_CLIENT_STANDARD_PORT              68u

/** \brief Minimum pooling period in milliseconds for the DHCP operations */
#define DHCP_MIN_POOLING_PERIOD         1000u


/** \brief DHCP magic cookie */
#define DHCP_MAGIC_COOKIE               0x63825363u

/** \brief BOOTP padding size in bytes */
#define DHCP_BOOTP_PADDING_SIZE         192u

/** \brief DHCP boot request */
#define DHCP_BOOT_REQUEST               0x01u

/** \brief DHCP boot reply */
#define DHCP_BOOT_REPLY                 0x02u

/** \brief DHCP discover message type */
#define DHCP_DISCOVER_MSG_TYPE          0x01u

/** \brief DHCP offer message type */
#define DHCP_OFFER_MSG_TYPE             0x02u

/** \brief DHCP request message type */
#define DHCP_REQUEST_MSG_TYPE           0x03u

/** \brief DHCP ack message type */
#define DHCP_ACK_MSG_TYPE               0x05u

/** \brief DHCP inform message type */
#define DHCP_INFORM_MSG_TYPE            0x01u


/** \brief DHCP HTYPE field */
#define DHCP_HTYPE_FIELD                0x01u

/** \brief DHCP flags broadcast */
#define DHCP_FLAGS_BROADCAST            0x8000u


/** \brief DHCP requested address option */
#define DHCP_REQUESTED_ADDRESS_OPTION           0x32u

/** \brief DHCP lease time option size in bytes */
#define DHCP_REQUESTED_ADDRESS_OPTION_SIZE      0x04u


/** \brief DHCP lease time option */
#define DHCP_LEASE_TIME_OPTION          0x33u

/** \brief DHCP lease time option size in bytes */
#define DHCP_LEASE_TIME_OPTION_SIZE     0x04u


/** \brief DHCP request type option */
#define DHCP_REQUEST_TYPE_OPTION        0x35u

/** \brief DHCP request type option size in bytes */
#define DHCP_REQUEST_TYPE_OPTION_SIZE   0x01u


/** \brief DHCP server identifier option */
#define DHCP_SERVER_ID_OPTION           0x36u

/** \brief DHCP server identifier option size in bytes */
#define DHCP_SERVER_ID_OPTION_SIZE      0x04u


/** \brief DHCP parameters request list option */
#define DHCP_PARAMETER_REQUEST_LIST_OPTION                      0x37u

/** \brief DHCP parameters request list option size in bytes */
#define DHCP_PARAMETER_REQUEST_LIST_OPTION_SIZE(nb_params)      (nb_params)


/** \brief DHCP subnet mask option */
#define DHCP_SUBNET_MASK_OPTION         0x01u

/** \brief DHCP subnet mask option size in bytes */
#define DHCP_SUBNET_MASK_OPTION_SIZE    0x04u


/** \brief DHCP router option */
#define DHCP_ROUTER_OPTION              0x03u

/** \brief DHCP router option size in bytes */
#define DHCP_ROUTER_OPTION_SIZE         0x04u


/** \brief DHCP renewal time option */
#define DHCP_RENEWAL_TIME_OPTION        0x3Au

/** \brief DHCP renewal time option size in bytes */
#define DHCP_RENEWAL_TIME_OPTION_SIZE   0x04u


/** \brief DHCP rebinding time option */
#define DHCP_REBINDING_TIME_OPTION      0x3Bu

/** \brief DHCP rebinding time option size in bytes */
#define DHCP_REBINDING_TIME_OPTION_SIZE   0x04u


/** \brief DHCP end flag option */
#define DHCP_END_FLAG_OPTION            0xFFu




/** \brief Minimum size in bytes of a DHCP message 
           header = 4 bytes
           transaction id = 4 bytes
           flags = 4 bytes
           CIADDR = 4 bytes
           YIADDR = 4 bytes
           SIADDR = 4 bytes
           GIADDR = 4 bytes
           CHADDR = 16 bytes
           BOOTP padding = 192 bytes
           DHCP magic cookie = 4 bytes
*/
#define DHCP_MIN_MSG_SIZE               240u

/** \brief Size in bytes of a DHCP discover message 
           option DHCP discover : 3 bytes
           end flag : 1 byte
*/
#define DHCP_DISCOVER_MSG_SIZE          (DHCP_MIN_MSG_SIZE + 4u)

/** \brief Size in bytes of a DHCP request message
           option DHCP discover : 3 bytes
           option server id : 6 bytes
           option requested address : 6 bytes
           end flag : 1 byte
*/
#define DHCP_REQUEST_MSG_SIZE           (DHCP_MIN_MSG_SIZE + 10u)




/** \brief DHCP message */
typedef struct _nano_ip_dhcp_msg_t
{
    /** \brief OP */
    uint8_t op;
    /** \brief HTYPE */
    uint8_t htype;
    /** \brief HLEN */
    uint8_t hlen;
    /** \brief HOPS */
    uint8_t hops;
    /** \brief Transaction id */
    uint32_t transaction_id;
    /** \brief Flags */
    uint32_t flags;
    /** \brief CIADDR */
    uint32_t ciaddr;
    /** \brief YIADDR */
    uint32_t yiaddr;
    /** \brief SIADDR */
    uint32_t siaddr;
    /** \brief GIADDR */
    uint32_t giaddr;
    /** \brief CHADDR */
    uint8_t chaddr[16u];
} nano_ip_dhcp_msg_t;


/** \brief DHCP client state */
typedef enum _nano_ip_dhcp_client_state_t
{
    /** \brief Stopped */
    DCS_STOPPED = 0u,
    /** \brief Init */
    DCS_INIT = 1u,
    /** \brief Selecting */
    DCS_SELECTING = 2u,
    /** \brief Requesting */
    DCS_REQUESTING = 3u,
    /** \brief Bound */
    DCS_BOUND = 4u,
    /** \brief Renewing */
    DCS_RENEWING = 5u,
    /** \brief Rebinding */
    DCS_REBINDING = 6u
} nano_ip_dhcp_client_state_t;



/** \brief DHCP client module internal data */
typedef struct _nano_ip_dhcp_client_t
{
    /** \brief UDP handle */
    nano_ip_udp_handle_t udp_handle;
    /** \brief State */
    nano_ip_dhcp_client_state_t state;
    /** \brief Server address */
    uint32_t server_address;
    /** \brief Server port */
    uint16_t server_port;
    /** \brief Client port */
    uint16_t client_port;
    /** \brief Timeout */
    uint32_t timeout;
    /** \brief Current timeout */
    uint32_t current_timeout;
    /** \brief Timer */
    oal_timer_t timer;
    /** \brief Network interface */
    nano_ip_net_if_t* net_if;
    /** \brief Transaction id */
    uint32_t transaction_id;
    /** \brief Lease address */
    ipv4_address_t lease_address;
    /** \brief Netmask */
    ipv4_address_t netmask;
    /** \brief Gateway */
    ipv4_address_t gateway;
    /** \brief Lease time */
    uint32_t lease_time;
    /** \brief Lease time 1 */
    uint32_t lease_time_1;
    /** \brief Lease time 2 */
    uint32_t lease_time_2;
} nano_ip_dhcp_client_t;



/** \brief Initialize a DHCP client instance */
nano_ip_error_t NANO_IP_DHCP_CLIENT_Init(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_if_t* const net_if, const uint16_t server_port, const uint16_t client_port, const uint32_t timeout);

/** \brief Start a DHCP client instance */
nano_ip_error_t NANO_IP_DHCP_CLIENT_Start(nano_ip_dhcp_client_t* const dhcp_client);

/** \brief Stop a DHCP client instance */
nano_ip_error_t NANO_IP_DHCP_CLIENT_Stop(nano_ip_dhcp_client_t* const dhcp_client);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_DHCP && NANO_IP_ENABLE_DHCP_CLIENT */

#endif /* NANO_IP_DHCP_CLIENT_H */
