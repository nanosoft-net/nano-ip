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

#include "nano_ip_ethernet.h"
#include "nano_ip_packet_funcs.h"
#include "nano_ip_tools.h"
#include "nano_ip_data.h"


/** \brief Minimum ethernet frame size in bytes (without CRC)*/
#define MIN_ETHERNET_FRAME_SIZE         60u

/** \brief Ethernet checksum size in bytes */
#define ETHERNET_CS_SIZE                4u

/** \brief Residue of the computation of the ethernet checksum on a frame */
#define ETHERNET_CS_RESIDUE             0xC704DD7Bu


/** \brief Ethernet broadcast MAC address */
const uint8_t ETHERNET_BROADCAST_MAC_ADDRESS[MAC_ADDRESS_SIZE] = {0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu};

/** \brief Ethernet NULL MAC address */
const uint8_t ETHERNET_NULL_MAC_ADDRESS[MAC_ADDRESS_SIZE] = { 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u };



/** \brief Compute the CRC of an ethernet frame */
static uint32_t NANO_IP_ETHERNET_ComputeCrc(nano_ip_net_packet_t* const packet, const bool compute_residue);






/** \brief Initialize the ethernet module */
nano_ip_error_t NANO_IP_ETHERNET_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    /* Nothing to do */

    return ret;
}

/** \brief Add an ethernet protocol */
nano_ip_error_t NANO_IP_ETHERNET_AddProtocol(nano_ip_ethernet_protocol_t* const protocol)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (protocol != NULL)
    {
        /* Add protocol to the list */
        protocol->next = g_nano_ip.eth_module.eth_protocols;
        g_nano_ip.eth_module.eth_protocols = protocol;

        ret = NIP_ERR_SUCCESS; 
    }

    return ret;
}


/** \brief Handle a received frame */
nano_ip_error_t NANO_IP_ETHERNET_RxFrame(nano_ip_net_if_t* const net_if, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((net_if != NULL) && (packet != NULL))
    {
        /* Check packet size */
        if (((net_if->driver->caps & NETDRV_CAPS_ETH_MIN_FRAME_SIZE) != 0u) || (packet->count >= MIN_ETHERNET_FRAME_SIZE))
        {
            /* Check packet CRC */
            if ((net_if->driver->caps & NETDRV_CAP_ETH_CS_CHECK) != 0u) 
            {
                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                /* Check against the ethernet residue of crc computation on the frame + FCS */
                if (NANO_IP_ETHERNET_ComputeCrc(packet, true) == ETHERNET_CS_RESIDUE)
                {
                    ret = NIP_ERR_SUCCESS;
                }
                else
                {
                    ret = NIP_ERR_INVALID_CRC;
                }
            }
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Decode frame */
                ethernet_header_t eth_header;
                nano_ip_ethernet_protocol_t* eth_protocol;

                /* Extract mac addresses */
                NANO_IP_PACKET_ReadBuffer(packet, eth_header.dest_address, MAC_ADDRESS_SIZE);
                NANO_IP_PACKET_ReadBuffer(packet, eth_header.src_address, MAC_ADDRESS_SIZE);

                /* Check if packet is for us */
                if ((net_if->driver->caps & NETDRV_CAP_DEST_MAC_ADDR_CHECK) != 0u)
                {
                    ret = NIP_ERR_SUCCESS;
                }
                else
                {
                    /* Compare to own address and broadcast address */
                    if ((MEMCMP(eth_header.dest_address, net_if->mac_address, MAC_ADDRESS_SIZE) != 0) &&
                        (MEMCMP(eth_header.dest_address, ETHERNET_BROADCAST_MAC_ADDRESS, MAC_ADDRESS_SIZE) != 0))
                    {
                        ret = NIP_ERR_IGNORE_PACKET;
                    }
                }
                if (ret == NIP_ERR_SUCCESS)
                {
                    /* Extract ethertype */
                    eth_header.ether_type = NANO_IP_PACKET_Read16bits(packet);

                    /* Look for the corresponding protocol handler */
                    eth_protocol = g_nano_ip.eth_module.eth_protocols;
                    while ((eth_protocol != NULL) && (eth_protocol->ether_type != eth_header.ether_type))
                    {
                        eth_protocol = eth_protocol->next;
                    }
                    if (eth_protocol != NULL)
                    {
                        /* Decode protocol */
                        ret = eth_protocol->rx_frame(eth_protocol->user_data, net_if, &eth_header, packet);
                    }
                    else
                    {
                        ret = NIP_ERR_PROTOCOL_NOT_FOUND;
                    }
                }
            }
        }
        else
        {
            ret = NIP_ERR_PACKET_TOO_SHORT;
        }
    }

    return ret;
}


/** \brief Allocate a packet for an ethernet frame */
nano_ip_error_t NANO_IP_ETHERNET_AllocatePacket(const uint16_t packet_size, nano_ip_net_packet_t** packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((packet != NULL) && (g_nano_ip.packet_allocator != NULL))
    {
        /* Compute total packet size needed */
        uint16_t total_packet_size = packet_size + ETHERNET_HEADER_SIZE + ETHERNET_CS_SIZE;
        if (total_packet_size < MIN_ETHERNET_FRAME_SIZE)
        {
            total_packet_size = MIN_ETHERNET_FRAME_SIZE;
        }

        /* Try to allocate a packet */
        ret = g_nano_ip.packet_allocator->allocate(g_nano_ip.packet_allocator->allocator_data, packet, total_packet_size);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Update packet flags */
            (*packet)->flags = NET_IF_PACKET_FLAG_TX;

            /* Skip header */
            NANO_IP_PACKET_WriteSkipBytes((*packet), ETHERNET_HEADER_SIZE);
        }
    }

    return ret;
}

/** \brief Send an ethernet packet on a specific network interface */
nano_ip_error_t NANO_IP_ETHERNET_SendPacket(nano_ip_net_if_t* const net_if, const ethernet_header_t* const eth_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((net_if != NULL) && (packet != NULL))
    {
        /* Pad frame to the minimum ethernet frame size */
        if ((packet->count < MIN_ETHERNET_FRAME_SIZE) && ((net_if->driver->caps & NETDRV_CAP_ETH_FRAME_PADDING) == 0u) )
        {
            MEMSET(packet->current, 0, MIN_ETHERNET_FRAME_SIZE - packet->count);
            packet->count = MIN_ETHERNET_FRAME_SIZE;
        }

        /* Fill ethernet header */
        packet->current = packet->data;
        NANO_IP_PACKET_WriteBufferNoCount(packet, eth_header->dest_address, MAC_ADDRESS_SIZE);
        NANO_IP_PACKET_WriteBufferNoCount(packet, eth_header->src_address, MAC_ADDRESS_SIZE);
        NANO_IP_PACKET_Write16bitsNoCount(packet, eth_header->ether_type);

        /* Compute packet CRC */
        if ((net_if->driver->caps & NETDRV_CAP_ETH_CS_COMPUTATION) == 0u)
        {
            /* Append CRC to the packet */
            uint8_t i;
            uint32_t fcs = 0u;
            const uint32_t computed_crc = NANO_IP_ETHERNET_ComputeCrc(packet, false);
            
            /* Invert bits order */
            for (i = 0; i < 32; i++)
            {
                if ((computed_crc & (1u << (31u - i))) != 0u)
                {
                    fcs += (1u << i);
                }
            }

            /* Invert bits value */
            fcs = ~fcs;

            /* Append FCS to the packet */
            packet->data[packet->count] = (fcs & 0xFFu);
            packet->data[packet->count + 1u] = ((fcs >> 8u) & 0xFFu);
            packet->data[packet->count + 2u] = ((fcs >> 16u) & 0xFFu);
            packet->data[packet->count + 3u] = ((fcs >> 24u) & 0xFFu);
            packet->count += sizeof(uint32_t);
        }

        /* Send the packet */
        ret = net_if->driver->send_packet(net_if->driver->user_data, packet);
    }

    return ret;
}

/** \brief Release an ethernet packet */
nano_ip_error_t NANO_IP_ETHERNET_ReleasePacket(nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (packet != NULL)
    {
        if ((packet->flags & NET_IF_PACKET_FLAG_TX) == 0u)
        {
            /* Requeue packet for reception */
            packet->flags = NET_IF_PACKET_FLAG_RX;
            packet->current = NANO_IP_CAST(uint8_t*, packet->data);
            ret = packet->net_if->driver->add_rx_packet(packet->net_if->driver->user_data, packet);
        }
        else
        {
            /* Release packet */
            ret = g_nano_ip.packet_allocator->release(g_nano_ip.packet_allocator->allocator_data, packet);
        }
    }

    return ret;
}

/** \brief Register a periodic callback */
nano_ip_error_t NANO_IP_ETHERNET_RegisterPeriodicCallback(nano_ip_ethernet_periodic_callback_t* const callback)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((callback != NULL) && (callback->callback != NULL))
    {
        /* Add callback to the list */
        callback->next = g_nano_ip.eth_module.callbacks;
        g_nano_ip.eth_module.callbacks = callback;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Ethernet periodic task */
nano_ip_error_t NANO_IP_ETHERNET_PeriodicTask(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    const uint32_t timestamp = NANO_IP_OAL_TIME_GetMsCounter();

    /* Call all the callbacks */
    nano_ip_ethernet_periodic_callback_t* callback = g_nano_ip.eth_module.callbacks;
    while (callback != NULL)
    {
        if (callback->callback != NULL)
        {
            callback->callback(timestamp, callback->user_data);
        }
        callback = callback->next;
    }

    return ret;
}


/** \brief Compute the CRC of an ethernet frame */
static uint32_t NANO_IP_ETHERNET_ComputeCrc(nano_ip_net_packet_t* const packet, const bool compute_residue)
{
    uint32_t crc32;
    uint32_t count;
    uint32_t i, j;
    uint8_t byte;
    uint8_t* pbyte;
    uint32_t q0, q1, q2, q3;

    pbyte = packet->data;
    count = packet->count;
    if (compute_residue)
    {
        count += sizeof(uint32_t);
    }
    crc32 = 0xFFFFFFFF;
    for (i = 0; i < count; i++, pbyte++) 
    {
        byte = (*pbyte);
        for (j = 0; j < 2; j++) 
        {
            if (((crc32 >> 28) ^ (byte >> 3)) & 0x00000001) 
            {
                q3 = 0x04C11DB7;
            } 
            else 
            {
                q3 = 0x00000000;
            }
            if (((crc32 >> 29) ^ (byte >> 2)) & 0x00000001) 
            {
                q2 = 0x09823B6E;
            } 
            else 
            {
                q2 = 0x00000000;
            }
            if (((crc32 >> 30) ^ (byte >> 1)) & 0x00000001) 
            {
                q1 = 0x130476DC;
            } 
            else 
            {
                q1 = 0x00000000;
            }
            if (((crc32 >> 31) ^ (byte >> 0)) & 0x00000001) 
            {
                q0 = 0x2608EDB8;
            } 
            else 
            {
                q0 = 0x00000000;
            }
            crc32 = (crc32 << 4) ^ q3 ^ q2 ^ q1 ^ q0;
            byte >>= 4;
        }
    }
    
    return crc32;
}
