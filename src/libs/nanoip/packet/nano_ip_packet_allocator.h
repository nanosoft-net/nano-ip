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

#ifndef NANO_IP_PACKET_ALLOCATOR_H
#define NANO_IP_PACKET_ALLOCATOR_H

#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_packet.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Network packet allocator interface */
typedef struct _nano_ip_net_packet_allocator_t
{
    /** \brief Allocate a packet */
    nano_ip_error_t (*allocate)(void* const allocator_data, nano_ip_net_packet_t** const packet, const uint16_t size);
    /** \brief Release a packet */
    nano_ip_error_t (*release)(void* const allocator_data, nano_ip_net_packet_t* const packet);
    /** \brief Allocator specific data */
    void* allocator_data;
} nano_ip_net_packet_allocator_t;




#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_PACKET_ALLOCATOR_H */
