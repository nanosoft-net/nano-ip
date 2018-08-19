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

#ifndef NANO_IP_PACKET_FUNCS_H
#define NANO_IP_PACKET_FUNCS_H


#include "nano_ip_packet.h"
#include "nano_ip_tools.h"


/** \brief Read a 16 bits value in network order from a buffer */
#define NET_READ_16(buffer) (((buffer)[0u] << 8u) + (buffer)[1u])

/** \brief Read a 32 bits value in network order from a buffer */
#define NET_READ_32(buffer) (((buffer)[0u] << 24u) + ((buffer)[1u] << 16u) + ((buffer)[2u] << 8u) + (buffer)[3u])


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Reset a packet queue */
static inline void NANO_IP_PACKET_ResetQueue(nano_ip_packet_queue_t* const packet_queue)
{
    packet_queue->head = NULL;
    packet_queue->tail = NULL;
}

/** \brief Indicate if a queue is empty */
static inline bool NANO_IP_PACKET_QueueIsEmpty(const nano_ip_packet_queue_t* const packet_queue)
{
    return (packet_queue->head == NULL);
}

/** \brief Add a packet to a packet queue */
static inline void NANO_IP_PACKET_AddToQueue(nano_ip_packet_queue_t* const packet_queue, nano_ip_net_packet_t* const packet)
{
    if (packet_queue->tail == NULL)
    {
        packet_queue->head = packet;
    }
    else
    {
        packet_queue->tail->next = packet;
    }
    packet_queue->tail = packet;
    packet->next = NULL;
}

/** \brief Pop a packet from a packet queue */
static inline nano_ip_net_packet_t* NANO_IP_PACKET_PopFromQueue(nano_ip_packet_queue_t* const packet_queue)
{
    nano_ip_net_packet_t* const packet = packet_queue->head; 
    if (packet_queue->head != NULL)
    {
        packet_queue->head = packet_queue->head->next;
        if (packet_queue->head == NULL)
        {
            packet_queue->tail = NULL;
        }
        packet->next = NULL;
    }

    return  packet;
}


/** \brief Read an 8bits integer from a packet */
static inline uint8_t NANO_IP_PACKET_Read8bits(nano_ip_net_packet_t* const packet)
{
    const uint8_t ret = *packet->current;
    packet->current++;
    packet->count--;
    return ret;
}

/** \brief Read a 16bits integer from a packet */
static inline uint16_t NANO_IP_PACKET_Read16bits(nano_ip_net_packet_t* const packet)
{
    const uint16_t ret = NET_READ_16(packet->current);
    packet->current += sizeof(uint16_t);
    packet->count -= sizeof(uint16_t);
    return ret;
}

/** \brief Read a 32bits integer from a packet */
static inline uint32_t NANO_IP_PACKET_Read32bits(nano_ip_net_packet_t* const packet)
{
    const uint32_t ret = NET_READ_32(packet->current);
    packet->current += sizeof(uint32_t);
    packet->count -= sizeof(uint32_t);
    return ret;
}

/** \brief Read buffer from a packet */
static inline void NANO_IP_PACKET_ReadBuffer(nano_ip_net_packet_t* const packet, void* const buffer, const uint16_t size)
{
    (void)NANO_IP_MEMCPY(buffer, packet->current, size);
    packet->current += size;
    packet->count -= size;
}

/** \brief Skip some bytes from a packet */
static inline void NANO_IP_PACKET_ReadSkipBytes(nano_ip_net_packet_t* const packet, const uint16_t size)
{
    packet->current += size;
    packet->count -= size;
}




/** \brief Write an 8bits integer to a packet */
static inline void NANO_IP_PACKET_Write8bitsNoCount(nano_ip_net_packet_t* const packet, const uint8_t val)
{
    *packet->current = val;
    packet->current++;
}

/** \brief Write an 8bits integer to a packet */
static inline void NANO_IP_PACKET_Write8bits(nano_ip_net_packet_t* const packet, const uint8_t val)
{
    NANO_IP_PACKET_Write8bitsNoCount(packet, val);
    packet->count++;
}

/** \brief Write a 16bits integer to a packet */
static inline void NANO_IP_PACKET_Write16bitsNoCount(nano_ip_net_packet_t* const packet, const uint16_t val)
{
    packet->current[0] = NANO_IP_CAST(uint8_t, (val >> 8u));
    packet->current[1] = NANO_IP_CAST(uint8_t, (val & 0xFFu));
    packet->current += sizeof(uint16_t);
}

/** \brief Write a 16bits integer to a packet */
static inline void NANO_IP_PACKET_Write16bits(nano_ip_net_packet_t* const packet, const uint16_t val)
{
    NANO_IP_PACKET_Write16bitsNoCount(packet, val);
    packet->count += sizeof(uint16_t);
}

/** \brief Write a 32bits integer to a packet */
static inline void NANO_IP_PACKET_Write32bitsNoCount(nano_ip_net_packet_t* const packet, const uint32_t val)
{
    packet->current[0] = NANO_IP_CAST(uint8_t, (val >> 24u));
    packet->current[1] = NANO_IP_CAST(uint8_t, (val >> 16u));
    packet->current[2] = NANO_IP_CAST(uint8_t, (val >> 8u));
    packet->current[3] = NANO_IP_CAST(uint8_t, (val & 0xFFu));
    packet->current += sizeof(uint32_t);
}

/** \brief Write a 32bits integer to a packet */
static inline void NANO_IP_PACKET_Write32bits(nano_ip_net_packet_t* const packet, const uint32_t val)
{
    NANO_IP_PACKET_Write32bitsNoCount(packet, val);
    packet->count += sizeof(uint32_t);
}

/** \brief Write buffer to a packet */
static inline void NANO_IP_PACKET_WriteBufferNoCount(nano_ip_net_packet_t* const packet, const void* const buffer, const uint16_t size)
{
    (void)NANO_IP_MEMCPY(packet->current, buffer, size);
    packet->current += size;
}

/** \brief Write buffer to a packet */
static inline void NANO_IP_PACKET_WriteBuffer(nano_ip_net_packet_t* const packet, const void* const buffer, const uint16_t size)
{
    NANO_IP_PACKET_WriteBufferNoCount(packet, buffer, size);
    packet->count += size;
}

/** \brief Skip some bytes from a packet */
static inline void NANO_IP_PACKET_WriteSkipBytes(nano_ip_net_packet_t* const packet, const uint16_t size)
{
    packet->current += size;
    packet->count += size;
}

/** \brief Write 0s to a packet */
static inline void NANO_IP_PACKET_WriteZeros(nano_ip_net_packet_t* const packet, const uint16_t size)
{
    (void)NANO_IP_MEMSET(packet->current, 0, size);
    NANO_IP_PACKET_WriteSkipBytes(packet, size);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_PACKET_FUNCS_H */
