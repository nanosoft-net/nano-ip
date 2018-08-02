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

#include "nano_ip_ipv4.h"
#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"
#include "nano_ip_data.h"


/** \brief Minimum packet size in bytes for an IPv4 header = header without options */
#define IPV4_MIN_HEADER_SIZE    20u

/** \brief Maximum IPv4 header size in bytes */
#define IPV4_MAX_HEADER_SIZE    60u

/** \brief Minimum size for an IPv4 packet */
#define IPV4_MIN_PACKET_SIZE    (ETHERNET_HEADER_SIZE + IPV4_MIN_HEADER_SIZE)

/** \brief Default value for the Version and IHL fields */
#define IPV4_VERSION_IHL_FIELD  0x45u

/** \brief Default value for the TTL field */
#define IPV4_DEFAULT_TTL_FIELD  0x80u



/** \brief IPV4 handle ready flag */
#define IPV4_HANDLE_READY_FLAG    0x01u




/** \brief ARP response callback */
static void NANO_IP_IPV4_ARPResponseCallback(void* const user_data, const bool success);

/** \brief Fill the header of an IPv4 frame */
static nano_ip_error_t NANO_IP_IPV4_FillHeader(const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet);

/** \brief Finalize the transmission of an IPv4 packet */
static nano_ip_error_t NANO_IP_IPV4_FinalizeSendPacket(nano_ip_ipv4_handle_t* const ipv4_handle);

/** \brief Handle a received IPv4 frame */
static nano_ip_error_t NANO_IP_IPV4_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet);

/** \brief IPv4 periodic task */
static void NANO_IP_IPV4_PeriodicTask(const uint32_t timestamp, void* const user_data);



/** \brief Initialize the IPv4 module */
nano_ip_error_t NANO_IP_IPV4_Init(void)
{
    nano_ip_error_t ret;
    nano_ip_ipv4_module_data_t* const ipv4_module = &g_nano_ip.ipv4_module;

    /* Register protocol */
    ipv4_module->ipv4_protocol.ether_type = IP_PROTOCOL;
    ipv4_module->ipv4_protocol.rx_frame = NANO_IP_IPV4_RxFrame;
    ipv4_module->ipv4_protocol.user_data = ipv4_module;
    ret = NANO_IP_ETHERNET_AddProtocol(&ipv4_module->ipv4_protocol);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Register periodic callback */
        ipv4_module->eth_callback.callback = NANO_IP_IPV4_PeriodicTask;
        ipv4_module->eth_callback.user_data = ipv4_module;
        ret = NANO_IP_ETHERNET_RegisterPeriodicCallback(&ipv4_module->eth_callback);

        /* Add IPv4 broadcast address in the ARP table */
        (void)NANO_IP_ARP_AddEntry(AET_STATIC, ETHERNET_BROADCAST_MAC_ADDRESS, IPV4_BROADCAST_ADDRESS);
    }

    return ret;
}


/** \brief Add an IPv4 protocol */
nano_ip_error_t NANO_IP_IPV4_AddProtocol(nano_ip_ipv4_protocol_t* const protocol)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (protocol != NULL)
    {
        nano_ip_ipv4_module_data_t* const ipv4_module = &g_nano_ip.ipv4_module;

        /* Add protocol to the list */
        protocol->next = ipv4_module->protocols;
        ipv4_module->protocols = protocol;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Initialize an IPv4 handle */
nano_ip_error_t NANO_IP_IPV4_InitializeHandle(nano_ip_ipv4_handle_t* const ipv4_handle, void* const user_data, const fp_ipv4_error_callback_t error_callback)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((ipv4_handle != NULL) && (error_callback != NULL))
    {
        /* 0 init */
        MEMSET(ipv4_handle, 0, sizeof(nano_ip_ipv4_handle_t));

        /* Save callback */
        ipv4_handle->user_data = user_data;
        ipv4_handle->error_callback = error_callback;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Release an IPv4 handle */
nano_ip_error_t NANO_IP_IPV4_ReleaseHandle(nano_ip_ipv4_handle_t* const ipv4_handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (ipv4_handle != NULL)
    {
        /* Cancel current ARP request */
        if (ipv4_handle->busy)
        {
            (void)NANO_IP_ARP_CancelRequest(&ipv4_handle->arp_request);
        }
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Allocate a packet for an IPv4 frame */
nano_ip_error_t NANO_IP_IPV4_AllocatePacket(const uint16_t packet_size, nano_ip_net_packet_t** packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        /* Compute total packet size needed */
        const uint16_t total_packet_size = packet_size + IPV4_MIN_HEADER_SIZE;

        /* Try to allocate an ethernet packet */
        ret = NANO_IP_ETHERNET_AllocatePacket(total_packet_size, packet);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Skip header */
            NANO_IP_PACKET_WriteSkipBytes((*packet), IPV4_MIN_HEADER_SIZE);
        }
    }

    return ret;
}

/** \brief Send an IPv4 frame */
nano_ip_error_t NANO_IP_IPV4_SendPacket(nano_ip_ipv4_handle_t* const ipv4_handle, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((ipv4_handle != NULL) && (ipv4_header != NULL) && (packet != NULL))
    {
        /* Check if the handle is ready */
        if (!ipv4_handle->busy)
        {
            /* Get the route to the destination */
            ipv4_address_t gateway_addr = 0u;
            if (packet->net_if != NULL)
            {
                ipv4_handle->net_if = packet->net_if;
                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                ret = NANO_IP_ROUTE_Search(ipv4_header->dest_address, &gateway_addr, &ipv4_handle->net_if);
            }
            if (ret == NIP_ERR_SUCCESS)
            {
                ipv4_address_t dest_mac_address;

                /* Initialize handle */
                ipv4_handle->packet = packet;
                if (ipv4_header->src_address == 0)
                {
                    ipv4_handle->header.src_address = ipv4_handle->net_if->ipv4_address;
                }
                else
                {
                    ipv4_handle->header.src_address = ipv4_header->src_address;
                }
                ipv4_handle->header.dest_address = ipv4_header->dest_address;
                ipv4_handle->header.protocol = ipv4_header->protocol;

                /* Get the MAC address of the destination */
                dest_mac_address = ipv4_handle->header.dest_address;
                if (gateway_addr != 0u)
                {
                    dest_mac_address = gateway_addr;
                }
                ret = NANO_IP_ARP_Request(ipv4_handle->net_if, &ipv4_handle->arp_request, dest_mac_address, NANO_IP_IPV4_ARPResponseCallback, ipv4_handle);
                if (ret == NIP_ERR_SUCCESS)
                {
                    /* Send frame */
                    ret = NANO_IP_IPV4_FinalizeSendPacket(ipv4_handle);
                }
                else if (ret == NIP_ERR_IN_PROGRESS)
                {
                    /* Busy until ARP request response */
                    ipv4_handle->busy = true;
                }
                else
                {
                    /* Error */
                }
            }
        }
        else
        {
            /* Handle is busy */
            ret = NIP_ERR_BUSY;
        }
    }

    return ret;
}

/** \brief Indicate if an IPv4 handle is ready */
nano_ip_error_t NANO_IP_IPV4_HandleIsReady(nano_ip_ipv4_handle_t* const ipv4_handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (ipv4_handle != NULL)
    {
        if (ipv4_handle->busy)
        {
            ret = NIP_ERR_BUSY;
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}

/** \brief Release an IPv4 frame */
nano_ip_error_t NANO_IP_IPV4_ReleasePacket(nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        /* Release packet */
        ret = NANO_IP_ETHERNET_ReleasePacket(packet);
    }

    return ret;
}

/** \brief Register a periodic callback */
nano_ip_error_t NANO_IP_IPV4_RegisterPeriodicCallback(nano_ip_ipv4_periodic_callback_t* const callback)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((callback != NULL) && (callback->callback != NULL))
    {
        nano_ip_ipv4_module_data_t* const ipv4_module = &g_nano_ip.ipv4_module;

        /* Add callback to the list */
        callback->next = ipv4_module->callbacks;
        ipv4_module->callbacks = callback;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief ARP response callback */
static void NANO_IP_IPV4_ARPResponseCallback(void* const user_data, const bool success)
{
    nano_ip_ipv4_handle_t* const ipv4_handle = NANO_IP_CAST(nano_ip_ipv4_handle_t*, user_data);

    /* Check parameters */
    if (ipv4_handle != NULL)
    {
        /* If arp request did succeed, finish frame transmission */
        nano_ip_error_t err;
        if (success)
        {
            err = NANO_IP_IPV4_FinalizeSendPacket(ipv4_handle);
        }
        else
        {
            /* Release packet */
            (void)NANO_IP_ETHERNET_ReleasePacket(ipv4_handle->packet);
            err = NIP_ERR_ARP_FAILURE;
        }

        /* Handle is now ready */
        ipv4_handle->busy = false;

        /* Notify error */
        ipv4_handle->error_callback(ipv4_handle->user_data, err);
    }
}

/** \brief Fill the header of an IPv4 frame */
static nano_ip_error_t NANO_IP_IPV4_FillHeader(const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIPP_ERR_INVALID_PACKET_SIZE;

    /* Check packet size */
    if (packet->count >= IPV4_MIN_PACKET_SIZE)
    {
        /* Fill IPv4 header */
        uint16_t checksum;
        uint8_t* checksum_pos;
        uint8_t* const header_start = &packet->data[ETHERNET_HEADER_SIZE];
        const uint16_t total_packet_size = packet->count - ETHERNET_HEADER_SIZE;
        uint8_t* const current_pos = packet->current;
        packet->current = header_start;
        NANO_IP_PACKET_Write8bitsNoCount(packet, IPV4_VERSION_IHL_FIELD);
        NANO_IP_PACKET_Write8bitsNoCount(packet, 0x00u);
        NANO_IP_PACKET_Write16bitsNoCount(packet, total_packet_size);
        NANO_IP_PACKET_Write32bitsNoCount(packet, 0x00000000u);
        NANO_IP_PACKET_Write8bitsNoCount(packet, IPV4_DEFAULT_TTL_FIELD);
        NANO_IP_PACKET_Write8bitsNoCount(packet, ipv4_header->protocol);
        checksum_pos = packet->current;
        NANO_IP_PACKET_Write16bitsNoCount(packet, 0x0000u);
        NANO_IP_PACKET_Write32bitsNoCount(packet, ipv4_header->src_address);
        NANO_IP_PACKET_Write32bitsNoCount(packet, ipv4_header->dest_address);

        /* Compute checksum */
        checksum = NANO_IP_ComputeInternetCS(NULL, 0u, header_start, IPV4_MIN_HEADER_SIZE);
        checksum_pos[0] = NANO_IP_CAST(uint8_t, (checksum & 0xFFu));
        checksum_pos[1] = NANO_IP_CAST(uint8_t, (checksum >> 8u));

        /* Fix packet current position */
        packet->current = current_pos;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Finalize the transmission of an IPv4 packet */
static nano_ip_error_t NANO_IP_IPV4_FinalizeSendPacket(nano_ip_ipv4_handle_t* const ipv4_handle)
{
    nano_ip_error_t ret;

    /* Fill IPv4 header */
    ret = NANO_IP_IPV4_FillHeader(&ipv4_handle->header, ipv4_handle->packet);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Fill ethernet header */
        ethernet_header_t eth_header;
        MEMCPY(eth_header.src_address, ipv4_handle->net_if->mac_address, MAC_ADDRESS_SIZE);
        MEMCPY(eth_header.dest_address, ipv4_handle->arp_request.mac_address, MAC_ADDRESS_SIZE);
        eth_header.ether_type = IP_PROTOCOL;

        /* Send packet */
        ret = NANO_IP_ETHERNET_SendPacket(ipv4_handle->net_if, &eth_header, ipv4_handle->packet);
    }

    return ret;
}


/** \brief Handle a received IPv4 frame */
static nano_ip_error_t NANO_IP_IPV4_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_ipv4_module_data_t* const ipv4_module = NANO_IP_CAST(nano_ip_ipv4_module_data_t*, user_data);

    /* Check parameters */
    if ((ipv4_module != NULL) && (net_if != NULL) &&
        (eth_header != NULL) && (packet != NULL))

    {
        /* Check packet size */
        if (packet->count >= IPV4_MIN_HEADER_SIZE)
        {
            /* Decode header */
            ipv4_header_t header;
            uint8_t ip_version;
            uint8_t header_length;
            uint16_t fragment;
            uint8_t* const header_start = packet->current;
            header.eth_header = eth_header;
            ip_version = NANO_IP_PACKET_Read8bits(packet);
            header_length = (ip_version & 0x0Fu) * sizeof(uint32_t);
            ip_version = (ip_version >> 4u);
            NANO_IP_PACKET_ReadSkipBytes(packet, 1u); /* tos */
            header.data_length = NANO_IP_PACKET_Read16bits(packet) - header_length;
            (void)NANO_IP_PACKET_Read16bits(packet); /* Ignore identification field => no management of fragmented packets */
            fragment = NANO_IP_PACKET_Read16bits(packet);

            /* Check fragmented frame */
            fragment = (fragment & 0x07u);
            if ((fragment & 0x04u) == 0)
            {
                /* Decode end of header */
                NANO_IP_PACKET_ReadSkipBytes(packet, 1u); /* ttl */
                header.protocol = NANO_IP_PACKET_Read8bits(packet);
                NANO_IP_PACKET_ReadSkipBytes(packet, 2u); /* checksum */
                header.src_address = NANO_IP_PACKET_Read32bits(packet);
                header.dest_address = NANO_IP_PACKET_Read32bits(packet);

                /* Check packet CRC */
                if ((net_if->driver->caps & NETDRV_CAP_IPV4_CS_CHECK) != 0u)
                {
                    ret = NIP_ERR_SUCCESS;
                }
                else
                {
                    const uint16_t null_checksum = NANO_IP_ComputeInternetCS(NULL, 0u, header_start, header_length);
                    if (null_checksum == 0u)
                    {
                        ret = NIP_ERR_SUCCESS;
                    }
                    else
                    {
                        ret = NIP_ERR_INVALID_CS;
                    }
                }
                if (ret == NIP_ERR_SUCCESS)
                {
                    /* Check if packet is for us */
                    if ( ((net_if->driver->caps & NETDRV_CAP_IPV4_ADDRESS_CHECK) != 0) ||
                         ((net_if->ipv4_address & header.dest_address) == net_if->ipv4_address) )
                    {
                        /* Look for the corresponding protocol handler */
                        nano_ip_ipv4_protocol_t* ipv4_protocol = ipv4_module->protocols;
                        while ((ipv4_protocol != NULL) && (ipv4_protocol->protocol != header.protocol))
                        {
                            ipv4_protocol = ipv4_protocol->next;
                        }
                        if (ipv4_protocol != NULL)
                        {
                            /* Skip header options */
                            const uint16_t options_size = header_length - IPV4_MIN_HEADER_SIZE;
                            NANO_IP_PACKET_ReadSkipBytes(packet, options_size);

                            /* Decode protocol */
                            ret = ipv4_protocol->rx_frame(ipv4_protocol->user_data, net_if, &header, packet);
                        }
                        else
                        {
                            ret = NIP_ERR_PROTOCOL_NOT_FOUND;
                        }
                    }
                    else
                    {
                        ret = NIP_ERR_IGNORE_PACKET;
                    }
                }
            }
            else
            {
                ret = NIP_ERR_IGNORE_PACKET;
            }
        }
        else
        {
            ret = NIPP_ERR_INVALID_PACKET_SIZE;
        }
    }

    return ret;
}


/** \brief IPv4 periodic task */
static void NANO_IP_IPV4_PeriodicTask(const uint32_t timestamp, void* const user_data)
{
    nano_ip_ipv4_module_data_t* const ipv4_module = NANO_IP_CAST(nano_ip_ipv4_module_data_t*, user_data);
    if (ipv4_module != NULL)
    {
        /* Call all the callbacks */
        nano_ip_ipv4_periodic_callback_t* callback = ipv4_module->callbacks;
        while (callback != NULL)
        {
            if (callback->callback != NULL)
            {
                callback->callback(timestamp, callback->user_data);
            }
            callback = callback->next;
        }
    }
}
