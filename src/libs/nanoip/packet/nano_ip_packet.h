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

#ifndef NANO_IP_PACKET_H
#define NANO_IP_PACKET_H

#include "nano_ip_types.h"


/** \brief Flag indicating that the packet is used for reception */
#define NET_IF_PACKET_FLAG_RX                   1u

/** \brief Flag indicating that the packet is used for transmission */
#define NET_IF_PACKET_FLAG_TX                   2u

/** \brief Flag indicating that the packet should not be released on reception */
#define NET_IF_PACKET_FLAG_KEEP_PACKET          4u

/** \brief Flag indicating that the packet transmission or reception has failed */
#define NET_IF_PACKET_FLAG_ERROR                128u



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/* Pre-declaration Network interface */
typedef struct _nano_ip_net_if_t nano_ip_net_if_t;


/** \brief Network packet */
typedef struct _nano_ip_net_packet_t
{
    /** \brief Data */
    uint8_t* data;
    /** \brief Current data pointer */
    uint8_t* current;

    /** \brief Size in bytes */
    uint16_t size;
    /** \brief Number of bytes */
    uint16_t count;

    /** \brief Flags */
    uint32_t flags;

    /** \brief Allocator specific data */
    void* allocator_data;

    /** \brief Network interface which received the packet */
    nano_ip_net_if_t* net_if;

    /** \brief Next packet */
    struct _nano_ip_net_packet_t* next;
} nano_ip_net_packet_t;


/** \brief FIFO packet queue */
typedef struct _nano_ip_packet_queue_t
{
    /** \brief Head of the queue */
    nano_ip_net_packet_t* head;
    /** \brief Tail of the queue */
    nano_ip_net_packet_t* tail;
} nano_ip_packet_queue_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_PACKET_H */
