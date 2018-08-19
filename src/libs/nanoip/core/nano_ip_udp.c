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

#include "nano_ip_udp.h"

#if( NANO_IP_ENABLE_UDP == 1 )

#include "nano_ip_data.h"
#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"


/** \brief UDP protocol id */
#define UDP_PROTOCOL                0x11u


/** \brief UDP header size in bytes */
#define UDP_HEADER_SIZE             0x08u

/** \brief UDP pseudo header size in bytes */
#define UDP_PSEUDO_HEADER_SIZE      0x0Cu




/** \brief Handle an IPv4 error */
static void NANO_IP_UDP_Ipv4ErrorCallback(void* const user_data, const nano_ip_error_t error);

/** \brief Handle a received UDP frame */
static nano_ip_error_t NANO_IP_UDP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet);


#if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)

/** \brief Compute the UDP checksum of a buffer */
static uint16_t NANO_IP_UDP_ComputeCS(const ipv4_header_t* const ipv4_header, uint8_t* const buffer, const uint16_t size);

#endif /* NANO_IP_ENABLE_UDP_CHECKSUM */



/** \brief Initialize the UDP module */
nano_ip_error_t NANO_IP_UDP_Init(void)
{
    nano_ip_error_t ret;
    nano_ip_udp_module_data_t* const udp_module = &g_nano_ip.udp_module; 

    /* Register protocol */
    udp_module->ipv4_protocol.protocol = UDP_PROTOCOL;
    udp_module->ipv4_protocol.rx_frame = NANO_IP_UDP_RxFrame;
    udp_module->ipv4_protocol.user_data = udp_module;
    ret = NANO_IP_IPV4_AddProtocol(&udp_module->ipv4_protocol);

    return ret;
}


/** \brief Initialize an UDP handle */
nano_ip_error_t NANO_IP_UDP_InitializeHandle(nano_ip_udp_handle_t* const handle, const fp_udp_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((handle != NULL) && (callback != NULL))
    {
        /* 0 init */
        NANO_IP_MEMSET(handle, 0, sizeof(nano_ip_udp_handle_t));

        /* Initialize IPv4 handle */
        ret = NANO_IP_IPV4_InitializeHandle(&handle->ipv4_handle, handle, NANO_IP_UDP_Ipv4ErrorCallback);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Initialize handle */
            handle->callback = callback;
            handle->user_data = user_data;
        }
    }
    
    return ret;
}

/** \brief Release an UDP handle */
nano_ip_error_t NANO_IP_UDP_ReleaseHandle(nano_ip_udp_handle_t* const handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (handle != NULL)
    {
        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        /* Remove handle from list */
        if (handle->is_bound)
        {
            nano_ip_udp_handle_t* hdl;
            nano_ip_udp_handle_t* previous_hdl = NULL;
            nano_ip_udp_module_data_t* const udp_module = &g_nano_ip.udp_module;

            hdl = udp_module->handles;
            while ((hdl != NULL) && (hdl != handle))
            {
                previous_hdl = hdl;
                hdl = hdl->next;
            }
            if (hdl != NULL)
            {
                if (previous_hdl == NULL)
                {
                    udp_module->handles = hdl->next;
                }
                else
                {
                    previous_hdl->next = hdl->next;
                }
            }
        }

        /* Release IPv4 handle */
        ret = NANO_IP_IPV4_ReleaseHandle(&handle->ipv4_handle);

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Bind an UDP handle to a specific address and port */
nano_ip_error_t NANO_IP_UDP_Bind(nano_ip_udp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (handle != NULL)
    {
        nano_ip_udp_handle_t* hdl;
        nano_ip_udp_module_data_t* const udp_module = &g_nano_ip.udp_module;

        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        /* Check if the pair (address, port) is already in use */
        hdl = udp_module->handles;
        while ((hdl != NULL) && ((hdl->port != port) || (hdl->ipv4_address != ipv4_address)))
        {
            hdl = hdl->next;
        }
        if (hdl == NULL)
        {
            /* Bind the handle */
            handle->ipv4_address = ipv4_address;
            handle->port = port;
            if (!handle->is_bound)
            {
                /* Add the handle to the list */
                handle->next = udp_module->handles;
                udp_module->handles = handle;
                handle->is_bound = true;
            }

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_ADDRESS_IN_USE;
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Unbind an UDP handle from a specific address and port */
nano_ip_error_t NANO_IP_UDP_Unbind(nano_ip_udp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (handle != NULL)
    {
        /* Look for the handle */
        nano_ip_udp_handle_t* hdl;
        nano_ip_udp_handle_t* previous_hdl = NULL;
        nano_ip_udp_module_data_t* const udp_module = &g_nano_ip.udp_module;

        (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

        hdl = udp_module->handles;
        while ((hdl != NULL) && ((hdl->port != port) || (hdl->ipv4_address != ipv4_address)))
        {
            previous_hdl = hdl;
            hdl = hdl->next;
        }
        if ((hdl != NULL) && (hdl == handle))
        {
            /* Unbind the handle */            
            handle->is_bound = false;
            if (previous_hdl == NULL)
            {
                udp_module->handles = NULL;
            }
            else
            {
                previous_hdl->next = handle->next;
            }

            ret = NIP_ERR_SUCCESS;
        }

        (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);
    }

    return ret;
}

/** \brief Allocate a packet for an UDP frame */
nano_ip_error_t NANO_IP_UDP_AllocatePacket(nano_ip_net_packet_t** packet, const uint16_t packet_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        /* Compute total packet size needed */
        const uint16_t total_packet_size = packet_size + UDP_HEADER_SIZE;

        /* Try to allocate an IPv4 packet */
        ret = NANO_IP_IPV4_AllocatePacket(total_packet_size, packet);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Skip header */
            NANO_IP_PACKET_WriteSkipBytes((*packet), UDP_HEADER_SIZE);

            /* Reset count */
            (*packet)->count = 0;
        }
    }

    return ret;
}

/** \brief Send an UDP frame */
nano_ip_error_t NANO_IP_UDP_SendPacket(nano_ip_udp_handle_t* const handle, const ipv4_address_t ipv4_address, const uint16_t port, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if ((handle != NULL) && (packet != NULL))
    {
        ipv4_header_t ipv4_header;
        uint16_t udp_length = packet->count + UDP_HEADER_SIZE;
        uint16_t packet_size = NANO_IP_CAST(uint16_t, packet->current - packet->data);
        uint8_t* const current_pos = packet->current;
        uint8_t* checksum_pos;
        uint16_t checksum;
        #if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)
        uint8_t* header_start;
        #endif /* NANO_IP_ENABLE_UDP_CHECKSUM */

        /* Set current position to UDP header */
        packet->current -= udp_length;
        #if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)
        header_start = packet->current;
        #endif /* NANO_IP_ENABLE_UDP_CHECKSUM */

        /* Fill UDP header */
        NANO_IP_PACKET_Write16bitsNoCount(packet, handle->port);
        NANO_IP_PACKET_Write16bitsNoCount(packet, port);
        NANO_IP_PACKET_Write16bitsNoCount(packet, udp_length);
        checksum_pos = packet->current;
        NANO_IP_PACKET_Write16bits(packet, 0x0000u);

        /* Prepare IPv4 header */
        ipv4_header.dest_address = ipv4_address;
        if (handle->ipv4_address == 0u)
        {
            /* Get the route to the destination */
            uint32_t gateway_addr = 0u;
            nano_ip_net_if_t* net_if = packet->net_if;
            if (net_if == NULL)
            {
                (void)NANO_IP_ROUTE_Search(ipv4_address, &gateway_addr, &net_if);
            }
            if (net_if != NULL)
            {
                ipv4_header.src_address = net_if->ipv4_address;
            }
            else
            {
                ipv4_header.src_address = 0u;
            }
        }
        else
        {
            ipv4_header.src_address = handle->ipv4_address;
        }
        ipv4_header.protocol = UDP_PROTOCOL;

        /* Write checksum */
        #if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)
        checksum = NANO_IP_UDP_ComputeCS(&ipv4_header, header_start, udp_length);
        #else
        checksum = 0u;
        #endif /* NANO_IP_ENABLE_UDP_CHECKSUM */
        checksum_pos[0] = NANO_IP_CAST(uint8_t, (checksum & 0xFFu));
        checksum_pos[1] = NANO_IP_CAST(uint8_t, (checksum >> 8u));

        /* Update packet size and pointer position */
        packet->count = packet_size;
        packet->current = current_pos;
        
        /* Send frame */
        ret = NANO_IP_IPV4_SendPacket(&handle->ipv4_handle, &ipv4_header, packet);
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Indicate if an UDP handle is ready */
nano_ip_error_t NANO_IP_UDP_HandleIsReady(nano_ip_udp_handle_t* const handle)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (handle != NULL)
    {
        ret = NANO_IP_IPV4_HandleIsReady(&handle->ipv4_handle);
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Release an UDP frame */
nano_ip_error_t NANO_IP_UDP_ReleasePacket(nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        /* Release packet */
        ret = NANO_IP_IPV4_ReleasePacket(packet);
    }

    return ret;
}

/** \brief Reads the UDP header of a packet */
nano_ip_error_t NANO_IP_UDP_ReadHeader(nano_ip_net_packet_t* const packet, ipv4_address_t* const src_address, uint16_t* const src_port)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((packet != NULL) && (src_address != NULL) && (src_port != NULL))
    {
        uint8_t* udp_header_start = packet->current - UDP_HEADER_SIZE;
        uint8_t* ipv4_header_start = packet->data + ETHERNET_HEADER_SIZE + IPV4_SOURCE_ADDRESS_OFFSET;

        (*src_address) = NET_READ_32(ipv4_header_start);
        (*src_port) = NET_READ_16(udp_header_start);

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}




/** \brief Handle an IPv4 error */
static void NANO_IP_UDP_Ipv4ErrorCallback(void* const user_data, const nano_ip_error_t error)
{
    nano_ip_udp_handle_t* const handle = NANO_IP_CAST(nano_ip_udp_handle_t*, user_data);

    /* Call the registered callback */
    if (handle != NULL)
    {
        nano_ip_udp_event_data_t event_data;
        (void)NANO_IP_MEMSET(&event_data, 0, sizeof(event_data));
        event_data.error = error;
        if (error == NIP_ERR_SUCCESS)
        {
            (void)handle->callback(handle->user_data, UDP_EVENT_TX, &event_data);
        }
        else
        {
            (void)handle->callback(handle->user_data, UDP_EVENT_TX_FAILED, &event_data);
        }
    }
}

/** \brief Handle a received UDP frame */
static nano_ip_error_t NANO_IP_UDP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_udp_module_data_t* const udp_module = NANO_IP_CAST(nano_ip_udp_module_data_t*, user_data);

    /* Check parameters */
    if ((udp_module != NULL) && (net_if != NULL) &&
        (ipv4_header != NULL) && (packet != NULL))
    {
        /* Check packet size */
        if (packet->count >= UDP_HEADER_SIZE)
        {
            /* Decode packet */
            udp_header_t udp_header;
            uint16_t length;
            #if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)
            uint16_t checksum;
            uint16_t null_checksum;
            uint8_t* const header_start = packet->current;
            #endif /* NANO_IP_ENABLED_UDP_CHECKSUM */

            /* Fill UDP header */
            udp_header.ipv4_header = ipv4_header;
            udp_header.src_port = NANO_IP_PACKET_Read16bits(packet);
            udp_header.dest_port = NANO_IP_PACKET_Read16bits(packet);
            length = NANO_IP_PACKET_Read16bits(packet) - UDP_HEADER_SIZE;

            /* Compute checkum */
            #if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)
            checksum = NANO_IP_PACKET_Read16bits(packet);
            if ((checksum != 0u) && ((net_if->driver->caps & NETDRV_CAP_UDPIPV4_CS_CHECK) == 0u))
            {
                null_checksum = NANO_IP_UDP_ComputeCS(ipv4_header, header_start, length + UDP_HEADER_SIZE);
                if (null_checksum != 0u)
                {
                    ret = NIP_ERR_INVALID_CS;
                }
            }
            #else
            /* Skip checksum */
            (void)NANO_IP_PACKET_Read16bits(packet);
            #endif /* NANO_IP_ENABLE_UDP_CHECKSUM */
            if (ret != NIP_ERR_INVALID_CS)
            {
                /* Check length */
                if (length <= packet->count)
                {
                    nano_ip_udp_handle_t* handle;
                                
                    /* Adjust packet length */
                    packet->count = length;

                    /* Look for a corresponding udp handle */
                    handle = udp_module->handles;
                    while ((handle != NULL) && 
                        ((handle->port != udp_header.dest_port) || ((handle->ipv4_address & ipv4_header->dest_address) != handle->ipv4_address)) )
                    {
                        handle = handle->next;
                    }
                    if (handle != NULL)
                    {
                        /* Call the registered callback */
                        bool release_packet;
                        nano_ip_udp_event_data_t event_data;
                        (void)NANO_IP_MEMSET(&event_data, 0, sizeof(event_data));
                        event_data.error = NIP_ERR_SUCCESS;
                        event_data.udp_header = &udp_header;
                        event_data.packet = packet;
                        release_packet = handle->callback(handle->user_data, UDP_EVENT_RX, &event_data);
                        if (!release_packet)
                        {
                            packet->flags |= NET_IF_PACKET_FLAG_KEEP_PACKET;
                        }
                        ret = NIP_ERR_SUCCESS;
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
        }
        else
        {
            ret = NIPP_ERR_INVALID_PACKET_SIZE;
        }
    }

    return ret;
}


#if (NANO_IP_ENABLE_UDP_CHECKSUM == 1u)

/** \brief Compute the UDP checksum of a buffer */
static uint16_t NANO_IP_UDP_ComputeCS(const ipv4_header_t* const ipv4_header, uint8_t* const buffer, const uint16_t size)
{
    uint8_t pseudo_header[UDP_PSEUDO_HEADER_SIZE];

    /* Fill pseudo header */
    pseudo_header[0u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address >> 24u));
    pseudo_header[1u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address >> 16u));
    pseudo_header[2u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address >> 8u));
    pseudo_header[3u] = NANO_IP_CAST(uint8_t, (ipv4_header->src_address & 0xFFu));
    pseudo_header[4u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address >> 24u));
    pseudo_header[5u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address >> 16u));
    pseudo_header[6u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address >> 8u));
    pseudo_header[7u] = NANO_IP_CAST(uint8_t, (ipv4_header->dest_address & 0xFFu));
    pseudo_header[8u] = 0;
    pseudo_header[9u] = UDP_PROTOCOL;
    pseudo_header[10u] = NANO_IP_CAST(uint8_t, (size >> 8u));
    pseudo_header[11u] = NANO_IP_CAST(uint8_t, (size & 0x00FFu));

    /* Compute checksum with the pseudo header */
    return NANO_IP_ComputeInternetCS(pseudo_header, sizeof(pseudo_header), buffer, size);
}

#endif /* NANO_IP_ENABLE_UDP_CHECKSUM */

#endif /* NANO_IP_ENABLE_UDP */
