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

#ifndef NANO_IP_ARP_H
#define NANO_IP_ARP_H


#include "nano_ip_types.h"
#include "nano_ip_cfg.h"
#include "nano_ip_ethernet.h"
#include "nano_ip_ipv4_def.h"


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \brief ARP entry types */
typedef enum _nano_ip_arp_entry_type_t
{
    /** \brief Unused entry */
    AET_UNUSED = 0u,
    /** \brief Static entry */
    AET_STATIC = 1u,
    /** \brief Dynamic entry */
    AET_DYNAMIC = 2u
} nano_ip_arp_entry_type_t;


/** \brief ARP table entry */
typedef struct _nano_ip_arp_table_entry_t
{
    /** \brief Entry type */
    uint8_t entry_type;
    /** \brief Unused - needed for memory alignment */
    uint8_t unused;
    /** \brief Mac address */
    uint8_t mac_address[MAC_ADDRESS_SIZE];
    /** \brief IPv4 address */
    ipv4_address_t ipv4_address;
    /** \brief Timestamp */
    uint32_t timestamp;
} nano_ip_arp_table_entry_t;

/** \brief ARP response callback */
typedef void(*nano_ip_arp_resp_callback_t)(void* const user_data, const bool success);

/** \brief ARP request handle */
typedef struct _nano_ip_arp_request_t
{
    /** \brief Requested IPv4 address */
    uint32_t ipv4_address;
    /** \brief Corresponding MAC address */
    uint8_t mac_address[MAC_ADDRESS_SIZE];
    /** \brief Timeout */
    uint32_t timeout;
    /** \brief Callback */
    nano_ip_arp_resp_callback_t response_callback;
    /** \brief User data */
    void* user_data;
    /** \brief Next request */
    struct _nano_ip_arp_request_t* next;
} nano_ip_arp_request_t;

/** \brief ARP module internal data */
typedef struct _nano_ip_arp_module_data_t
{
    /** \brief Ethernet periodic callback */
    nano_ip_ethernet_periodic_callback_t eth_callback;
    /** \brief ARP protocol description */
    nano_ip_ethernet_protocol_t arp_protocol;
    /** \brief ARP table */
    nano_ip_arp_table_entry_t entries[NANO_IP_MAX_ARP_ENTRY_COUNT];
    /** \brief ARP requests */
    nano_ip_arp_request_t* requests;
} nano_ip_arp_module_data_t;



/** \brief Initialize the ARP module */
nano_ip_error_t NANO_IP_ARP_Init(void);

/** \brief Add an entry in the ARP table */
nano_ip_error_t NANO_IP_ARP_AddEntry(const nano_ip_arp_entry_type_t type, const uint8_t* const mac_address, const ipv4_address_t ipv4_address);

/** \brief Remove a static entry from the ARP table */
nano_ip_error_t NANO_IP_ARP_RemoveEntry(const ipv4_address_t ipv4_address);

/** \brief Request an ARP translation */
nano_ip_error_t NANO_IP_ARP_Request(nano_ip_net_if_t* const net_if, nano_ip_arp_request_t* const request, const ipv4_address_t ipv4_address, nano_ip_arp_resp_callback_t callback, void* const user_data);

/** \brief Cancel an ARP request */
nano_ip_error_t NANO_IP_ARP_CancelRequest(nano_ip_arp_request_t* const request);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_ARP_H */
