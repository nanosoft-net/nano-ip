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


/* 
    This file defines the common interface to all BSPs.
    Any modification on this BSP interface must be reported on ALL the BSP implementations.
*/

#ifndef BSP_H
#define BSP_H

#include "nano_ip_types.h"
#include "nano_ip_net_if.h"
#include "nano_ip_packet_allocator.h"

#include <stdarg.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Initialize the operating system */
bool NANO_IP_BSP_OSInit();

/** \brief Start the operating system (should never return on success) */
bool NANO_IP_BSP_OSStart();


/** \brief Instanciate the packet allocator */
bool NANO_IP_BSP_CreatePacketAllocator(nano_ip_net_packet_allocator_t* const packet_allocator);

/** \brief Instanciate the network interface */
bool NANO_IP_BSP_CreateNetIf(nano_ip_net_if_t* const net_if, const char** name, 
                             uint32_t* const rx_packet_count, uint32_t* const rx_packet_size,
                             uint8_t* const task_priority, uint32_t* const task_stack_size);


/** \brief Log output function */
void NANO_IP_BSP_Printf(const char* format, va_list arg_list);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BSP_H */
