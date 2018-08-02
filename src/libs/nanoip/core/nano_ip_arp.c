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

#include "nano_ip_arp.h"
#include "nano_ip_packet_funcs.h"
#include "nano_ip_tools.h"
#include "nano_ip_data.h"


/** \brief ARP protocol identifier */
#define ARP_PROTOCOL    0x0806u

/** \brief ARP hardware type field */
#define ARP_HARDWARE_TYPE       0x01u

/** \brief ARP packet size in bytes on IPv4 */
#define ARP_PACKET_SIZE_IPV4    28u

/** \brief Hardware address length in bytes on Ethernet */
#define ARP_HW_ADDRESS_LENGTH_ETHERNET    MAC_ADDRESS_SIZE

/** \brief Protocol address length in bytes on IPv4 */
#define ARP_PROTO_ADDRESS_LENGTH_IPV4    IPV4_ADDRESS_SIZE


/** \brief ARP request success flag */
#define APR_REQ_SUCCESS_FLAG    0x01u

/** \brief ARP request cancel flag */
#define APR_REQ_CANCEL_FLAG     0x02u

/** \brief ARP request timeout flag */
#define APR_REQ_TIMEOUT_FLAG    0x04u



/** \brief ARP frame types */
typedef enum _nano_ip_arp_frame_type_t
{
    /** \brief Request */
    ARP_OP_REQUEST = 1u,
    /** \brief Response */
    ARP_OP_RESPONSE = 2u
} nano_ip_arp_frame_type_t;


/** \brief IPv4 ARP frame */
typedef struct _nano_ip_arp_ipv4_frame_t
{
    /** \brief Hardware type */
    uint16_t hardware_type;
    /** \brief Protocol type */
    uint16_t protocol_type;
    /** \brief Hardware address length */
    uint8_t hw_address_length;
    /** \brief Protocol address length */
    uint8_t proto_address_length;
    /** \brief Operation */
    uint16_t operation;
    /** \brief Sender hardware address */
    uint8_t sender_hw_address[MAC_ADDRESS_SIZE];
    /** \brief Sender protocol address */
    ipv4_address_t sender_proto_address;
    /** \brief Target hardware address */
    uint8_t target_hw_address[MAC_ADDRESS_SIZE];
    /** \brief Target protocol address */
    ipv4_address_t target_proto_address;
} nano_ip_arp_ipv4_frame_t;




/** \brief Handle a received ARP frame */
static nano_ip_error_t NANO_IP_ARP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet);

/** \brief Handle an ARP request frame */
static nano_ip_error_t NANO_IP_ARP_HandleRequest(nano_ip_net_if_t* const net_if, const nano_ip_arp_ipv4_frame_t* const request);

/** \brief Handle an ARP response frame */
static nano_ip_error_t NANO_IP_ARP_HandleResponse(nano_ip_net_if_t* const net_if, const nano_ip_arp_ipv4_frame_t* const response);

/** \brief ARP periodic task */
static void NANO_IP_ARP_PeriodicTask(const uint32_t timestamp, void* const user_data);




/** \brief Initialize the ARP module */
nano_ip_error_t NANO_IP_ARP_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_arp_module_data_t* const arp_module = &g_nano_ip.arp_module;

    /* Register protocol */
    arp_module->arp_protocol.ether_type = ARP_PROTOCOL;
    arp_module->arp_protocol.rx_frame = NANO_IP_ARP_RxFrame;
    arp_module->arp_protocol.user_data = arp_module;
    ret = NANO_IP_ETHERNET_AddProtocol(&arp_module->arp_protocol);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Register periodic callback */
        arp_module->eth_callback.callback = NANO_IP_ARP_PeriodicTask;
        arp_module->eth_callback.user_data = arp_module;
        ret = NANO_IP_ETHERNET_RegisterPeriodicCallback(&arp_module->eth_callback);
    }

    return ret;
}

/** \brief Add an entry in the ARP table */
nano_ip_error_t NANO_IP_ARP_AddEntry(const nano_ip_arp_entry_type_t type, const uint8_t* const mac_address, const ipv4_address_t ipv4_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((mac_address != NULL) && (ipv4_address != 0u))
    {   
        /* Look for a free entry and the corresponding entry if it exists */
        uint32_t i = 0;
        nano_ip_arp_table_entry_t* entry = NULL;
        nano_ip_arp_table_entry_t* free_entry = NULL;
        nano_ip_arp_table_entry_t* corresponding_entry = NULL;
        nano_ip_arp_table_entry_t* oldest_entry = NULL;
        nano_ip_arp_module_data_t* const arp_module = &g_nano_ip.arp_module;

        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        while ((i < NANO_IP_MAX_ARP_ENTRY_COUNT) && (corresponding_entry == NULL))
        {
            /* Check free entry */
            entry = &arp_module->entries[i];
            if (entry->entry_type != AET_UNUSED)
            {
                /* Check corresponding entry */
                if ( (entry->ipv4_address == ipv4_address) &&
                     ((entry->entry_type == type) || (entry->entry_type == AET_DYNAMIC)) )
                {
                    corresponding_entry = entry;
                }
                else
                {
                    /* Check oldest entry */
                    if ( (entry->entry_type == AET_DYNAMIC) && 
                         ((oldest_entry == NULL) || (oldest_entry->timestamp > entry->timestamp)) )
                    {
                        oldest_entry = entry;
                    }
                }
            }
            else
            {
                free_entry = entry;
            }

            /* Next entry */
            i++;
        }

        /* Determine which entry to use */
        if (corresponding_entry != NULL)
        {
            entry = corresponding_entry;
        }
        else if (free_entry != NULL)
        {
            entry = free_entry;
        }
        else if (oldest_entry != NULL)
        {
            entry = oldest_entry;
        }
        else
        {
            entry = NULL;
        }

        /* Store information */
        if (entry != NULL)
        {
            entry->entry_type = type;
            entry->ipv4_address = ipv4_address;
            MEMCPY(entry->mac_address, mac_address, MAC_ADDRESS_SIZE);
            entry->timestamp = NANO_IP_OAL_TIME_GetMsCounter();

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            /* No entry available */
            ret = NIP_ERR_RESOURCE;
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Remove a static entry from the ARP table */
nano_ip_error_t NANO_IP_ARP_RemoveEntry(const ipv4_address_t ipv4_address)
{
    uint32_t i = 0;
    nano_ip_arp_table_entry_t* entry = NULL;
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_arp_module_data_t* const arp_module = &g_nano_ip.arp_module;
    
    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Look for the corresponding entry if it exists */   
    while ((i < NANO_IP_MAX_ARP_ENTRY_COUNT) && (ret != NIP_ERR_SUCCESS))
    {
        /* Check entry type */
        entry = &arp_module->entries[i];
        if (entry->entry_type == AET_STATIC)
        {
            /* Check corresponding entry */
            if (entry->ipv4_address == ipv4_address)
            {
                /* Remove entry */
                MEMSET(entry, 0, sizeof(nano_ip_arp_table_entry_t));
                ret = NIP_ERR_SUCCESS;
            }
        }

        /* Next entry */
        i++;
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Request an ARP translation */
nano_ip_error_t NANO_IP_ARP_Request(nano_ip_net_if_t* const net_if, nano_ip_arp_request_t* const request, const ipv4_address_t ipv4_address, nano_ip_arp_resp_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((request != NULL) && (callback != NULL))
    {
        uint32_t i = NANO_IP_MAX_ARP_ENTRY_COUNT - 1u;
        nano_ip_arp_table_entry_t* entry = NULL;
        nano_ip_arp_module_data_t* const arp_module = &g_nano_ip.arp_module;

        /* Look for the IP address in the table */

        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        while ((entry == NULL) && (i < NANO_IP_MAX_ARP_ENTRY_COUNT))
        {
            /* Check if entry is valid */
            if ((arp_module->entries[i].entry_type != AET_UNUSED) && (arp_module->entries[i].ipv4_address == ipv4_address))
            {
                /* Check validity */
                const uint32_t timestamp = NANO_IP_OAL_TIME_GetMsCounter();
                entry = &arp_module->entries[i];
                if( (entry->entry_type == AET_STATIC) || ((timestamp - entry->timestamp) <= NANO_IP_ARP_ENTRY_VALIDITY_PERIOD) )
                {
                    /* Copy MAC address */
                    MEMCPY(request->mac_address, entry->mac_address, MAC_ADDRESS_SIZE);
                    ret = NIP_ERR_SUCCESS;
                }
                else
                {
                    /* Entry is not valid anymore */
                    entry->entry_type = AET_UNUSED;
                    entry = NULL;
                    i = NANO_IP_MAX_ARP_ENTRY_COUNT;
                }
            }
            else
            {
                /* Next entry */
                i--;
            }
        }

        if (entry == NULL)
        {
            /* Send ARP request if not found in the table */
            nano_ip_net_packet_t* packet;

            /* Allocate a response frame */
            ret = NANO_IP_ETHERNET_AllocatePacket(ARP_PACKET_SIZE_IPV4, &packet);
            if (ret == NIP_ERR_SUCCESS)
            {
                ethernet_header_t eth_header;

                /* Fill request */
                NANO_IP_PACKET_Write16bits(packet, ARP_HARDWARE_TYPE);
                NANO_IP_PACKET_Write16bits(packet, IP_PROTOCOL);
                NANO_IP_PACKET_Write8bits(packet, MAC_ADDRESS_SIZE);
                NANO_IP_PACKET_Write8bits(packet, IPV4_ADDRESS_SIZE);
                NANO_IP_PACKET_Write16bits(packet, ARP_OP_REQUEST);
                NANO_IP_PACKET_WriteBuffer(packet, net_if->mac_address, ARP_HW_ADDRESS_LENGTH_ETHERNET);
                NANO_IP_PACKET_Write32bits(packet, net_if->ipv4_address);
                NANO_IP_PACKET_WriteBuffer(packet, ETHERNET_NULL_MAC_ADDRESS, ARP_HW_ADDRESS_LENGTH_ETHERNET);
                NANO_IP_PACKET_Write32bits(packet, ipv4_address);

                /* Prepare ethernet header */
                MEMCPY(eth_header.dest_address, ETHERNET_BROADCAST_MAC_ADDRESS, MAC_ADDRESS_SIZE);
                MEMCPY(eth_header.src_address, net_if->mac_address, MAC_ADDRESS_SIZE);
                eth_header.ether_type = ARP_PROTOCOL;

                /* Send request */
                ret = NANO_IP_ETHERNET_SendPacket(net_if, &eth_header, packet);
                if (ret == NIP_ERR_SUCCESS)
                {
                    /* Save callback */
                    request->ipv4_address = ipv4_address;
                    request->response_callback = callback;
                    request->user_data = user_data;
                    request->timeout = NANO_IP_OAL_TIME_GetMsCounter() + NANO_IP_ARP_REQUEST_TIMEOUT;

                    /* Add request to the list */
                    request->next = arp_module->requests;
                    arp_module->requests = request;

                    ret = NIP_ERR_IN_PROGRESS;
                }
                else
                {
                    /* Error, release the packet */
                    (void)NANO_IP_ETHERNET_ReleasePacket(packet);
                }
            }
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Cancel an ARP request */
nano_ip_error_t NANO_IP_ARP_CancelRequest(nano_ip_arp_request_t* const request)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (request != NULL)
    {
        nano_ip_arp_request_t* req;
        nano_ip_arp_request_t* previous_req = NULL;
        nano_ip_arp_module_data_t* const arp_module = &g_nano_ip.arp_module;

        /* Remove request from the list */
        
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        req = arp_module->requests;
        while ((req != NULL) && (req != request))
        {
            previous_req = req;
            req = req->next;
        }
        if( req != NULL)
        {
            if (previous_req == NULL)
            {
                arp_module->requests = NULL;
            }
            else
            {
                previous_req->next = req->next;
            }
        }

        if (req != NULL)
        {
            /* Call the callback */
            if (request->response_callback != NULL)
            {
                request->response_callback(request->user_data, false);
            }
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}



/** \brief Handle a received ARP frame */
static nano_ip_error_t NANO_IP_ARP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_arp_module_data_t* const arp_module = NANO_IP_CAST(nano_ip_arp_module_data_t*, user_data);

    /* Check parameters */
    if ( (arp_module != NULL) && (net_if != NULL) && 
         (eth_header != NULL) && (packet != NULL) )

    {
        /* Check packet size */
        if (packet->count >= ARP_PACKET_SIZE_IPV4)
        {
            /* Decode packet */
            nano_ip_arp_ipv4_frame_t  arp_frame;
            arp_frame.hardware_type = NANO_IP_PACKET_Read16bits(packet);
            arp_frame.protocol_type = NANO_IP_PACKET_Read16bits(packet);
            arp_frame.hw_address_length = NANO_IP_PACKET_Read8bits(packet);
            arp_frame.proto_address_length = NANO_IP_PACKET_Read8bits(packet);
            arp_frame.operation = NANO_IP_PACKET_Read16bits(packet);
            NANO_IP_PACKET_ReadBuffer(packet, arp_frame.sender_hw_address, ARP_HW_ADDRESS_LENGTH_ETHERNET);
            arp_frame.sender_proto_address = NANO_IP_PACKET_Read32bits(packet);
            NANO_IP_PACKET_ReadBuffer(packet, arp_frame.target_hw_address, ARP_HW_ADDRESS_LENGTH_ETHERNET);
            arp_frame.target_proto_address = NANO_IP_PACKET_Read32bits(packet);

            /* Check validity */
            if ( (arp_frame.protocol_type == IP_PROTOCOL) && (arp_frame.hw_address_length == ARP_HW_ADDRESS_LENGTH_ETHERNET) &&
                 (arp_frame.proto_address_length == ARP_PROTO_ADDRESS_LENGTH_IPV4) )
            {
                /* Handle frame */
                if (arp_frame.operation == ARP_OP_REQUEST)
                {
                    ret = NANO_IP_ARP_HandleRequest(net_if, &arp_frame);
                }
                else if (arp_frame.operation == ARP_OP_RESPONSE)
                {
                    ret = NANO_IP_ARP_HandleResponse(net_if, &arp_frame);
                }
                else
                {
                    ret = NIP_ERR_INVALID_ARP_FRAME;
                }
            }
            else
            {
                ret = NIP_ERR_INVALID_ARP_FRAME;
            }
        }
        else
        {
            ret = NIPP_ERR_INVALID_PACKET_SIZE;
        }
    }

    return ret;
}


/** \brief Handle an ARP request frame */
static nano_ip_error_t NANO_IP_ARP_HandleRequest(nano_ip_net_if_t* const net_if, const nano_ip_arp_ipv4_frame_t* const request)
{
    nano_ip_error_t ret = NIP_ERR_IGNORE_PACKET;

    /* Check if we are the target for this request */
    if (net_if->ipv4_address == request->target_proto_address)
    {
        nano_ip_net_packet_t* packet;

        /* Allocate a response frame */
        ret = NANO_IP_ETHERNET_AllocatePacket(ARP_PACKET_SIZE_IPV4, &packet);
        if (ret == NIP_ERR_SUCCESS)
        {
            ethernet_header_t eth_header;

            /* Add entry in the ARP table */
            (void)NANO_IP_ARP_AddEntry(AET_DYNAMIC, request->sender_hw_address, request->sender_proto_address);

            /* Fill response */
            NANO_IP_PACKET_Write16bits(packet, request->hardware_type);
            NANO_IP_PACKET_Write16bits(packet, request->protocol_type);
            NANO_IP_PACKET_Write8bits(packet, request->hw_address_length);
            NANO_IP_PACKET_Write8bits(packet, request->proto_address_length);
            NANO_IP_PACKET_Write16bits(packet, ARP_OP_RESPONSE);
            NANO_IP_PACKET_WriteBuffer(packet, net_if->mac_address, ARP_HW_ADDRESS_LENGTH_ETHERNET);
            NANO_IP_PACKET_Write32bits(packet, net_if->ipv4_address);
            NANO_IP_PACKET_WriteBuffer(packet, request->sender_hw_address, ARP_HW_ADDRESS_LENGTH_ETHERNET);
            NANO_IP_PACKET_Write32bits(packet, request->sender_proto_address);

            /* Prepare ethernet header */
            MEMCPY(eth_header.dest_address, request->sender_hw_address, MAC_ADDRESS_SIZE);
            MEMCPY(eth_header.src_address, net_if->mac_address, MAC_ADDRESS_SIZE);
            eth_header.ether_type = ARP_PROTOCOL;

            /* Send response */
            ret = NANO_IP_ETHERNET_SendPacket(net_if, &eth_header, packet);
            if (ret != NIP_ERR_SUCCESS)
            {
                /* Error, release the packet */
                (void)NANO_IP_ETHERNET_ReleasePacket(packet);
            }
        }
    }

    return ret;
}


/** \brief Handle an ARP response frame */
static nano_ip_error_t NANO_IP_ARP_HandleResponse(nano_ip_net_if_t* const net_if, const nano_ip_arp_ipv4_frame_t* const response)
{
    nano_ip_error_t ret = NIP_ERR_IGNORE_PACKET;

    /* Check if we are the target for this response */
    if (net_if->ipv4_address == response->target_proto_address)
    {
        /* Look for the corresponding requests */
        nano_ip_arp_request_t* previous_request = NULL;
        nano_ip_arp_module_data_t* const arp_module = &g_nano_ip.arp_module;
        nano_ip_arp_request_t* request = arp_module->requests;

        while (request != NULL)
        {
            if (request->ipv4_address == response->sender_proto_address)
            {
                /* Add entry in the ARP table */
                (void)NANO_IP_ARP_AddEntry(AET_DYNAMIC, response->sender_hw_address, response->sender_proto_address);

                /* Save MAC address */
                MEMCPY(request->mac_address, response->sender_hw_address, MAC_ADDRESS_SIZE);

                /* Remove request from the list */
                if (previous_request != NULL)
                {
                    previous_request->next = request->next;
                }
                else
                {
                    arp_module->requests = request->next;
                }

                /* Call request callback */
                if (request->response_callback != NULL)
                {
                    request->response_callback(request->user_data, true);
                }
            }
            else
            {
                previous_request = request;
            }

            /* Next request */
            request = request->next;
        }
    }

    return ret;
}

/** \brief ARP periodic task */
static void NANO_IP_ARP_PeriodicTask(const uint32_t timestamp, void* const user_data)
{
    nano_ip_arp_module_data_t* const arp_module = NANO_IP_CAST(nano_ip_arp_module_data_t*, user_data);
    if (arp_module != NULL)
    {
        /* Check requests timeout */
        nano_ip_arp_request_t* previous_request = NULL;
        nano_ip_arp_request_t* request = arp_module->requests;

        while (request != NULL)
        {
            /* Check timeout */
            if (request->timeout < timestamp)
            {
                /* Remove request from list */
                if (previous_request == NULL)
                {
                    arp_module->requests = request->next;
                }
                else
                {
                    previous_request->next = request->next;
                }

                /* Call request callback */
                if (request->response_callback != NULL)
                {
                    request->response_callback(request->user_data, false);
                }
            }
            else
            {
                previous_request = request;
            }

            /* Next request */
            request = request->next;
        }
    }
}

