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

#ifndef NANO_IP_ICMP_H
#define NANO_IP_ICMP_H

#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_ICMP == 1 )

#include "nano_ip_types.h"
#include "nano_ip_ipv4.h"



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \brief ICMP message types */
typedef enum _nano_ip_icmp_msg_type_t
{
    /** \brief Echo reply */
    ICMP_ECHO_REPLY = 0u,
    /** \brief Echo request */
    ICMP_ECHO_REQUEST = 8u
} nano_ip_icmp_msg_type_t;

#if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

/** \brief ICMP request handle */
typedef struct _nano_ip_icmp_request_t
{
    /** \brief Requested IPv4 address */
    ipv4_address_t ipv4_address;
    /** \brief Timeout */
    uint32_t timeout;
    /** \brief Response time */
    uint32_t response_time;
    /** \brief Identifier */
    uint32_t identifier;
    /** \brief Synchronization object */
    oal_flags_t sync_obj;
    /** \brief IPv4 handle */
    nano_ip_ipv4_handle_t ipv4_handle;
    /** \brief Next request */
    struct _nano_ip_icmp_request_t* next;
} nano_ip_icmp_request_t;

#endif /* NANO_IP_ENABLE_ICMP_PING_REQ */

/** \brief ICMP module internal data */
typedef struct _nano_ip_icmp_module_data_t
{
    /** \brief IPv4 protocol description */
    nano_ip_ipv4_protocol_t ipv4_protocol;
    /** \brief IPv4 handle */
    nano_ip_ipv4_handle_t ipv4_handle;

    #if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

    /** \brief ICMP requests */
    nano_ip_icmp_request_t* requests;
    /** \brief IPv4 periodic callback */
    nano_ip_ipv4_periodic_callback_t ipv4_callback;

    #endif /* NANO_IP_ENABLE_ICMP_PING_REQ */

} nano_ip_icmp_module_data_t;



/** \brief Initialize the ICMP module */
nano_ip_error_t NANO_IP_ICMP_Init(void);

#if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

/** \brief Initialize an ICMP request handle */
nano_ip_error_t NANO_IP_ICMP_InitRequest(nano_ip_icmp_request_t* const request);

/** \brief Initiate an ICMP ping request */
nano_ip_error_t NANO_IP_ICMP_PingRequest(nano_ip_icmp_request_t* const request, const uint32_t ipv4_address, const uint32_t timeout, const uint8_t data_size);

/** \brief Wait for an ICMP request */
nano_ip_error_t NANO_IP_ICMP_WaitRequest(nano_ip_icmp_request_t* const request, const uint32_t timeout);

/** \brief Cancel an ICMP request */
nano_ip_error_t NANO_IP_ICMP_CancelRequest(nano_ip_icmp_request_t* const request);

#endif /* NANO_IP_ENABLE_ICMP_PING_REQ */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_ICMP */

#endif /* NANO_IP_ICMP_H */
