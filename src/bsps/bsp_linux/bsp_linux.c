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

#include "bsp.h"
#include "nano_ip_pcap.h"
#include "nano_ip_big_small_packet_allocator.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>



/** \brief Buffer sizes for the packet allocator */
#define BIG_BUFFER_SIZE         1536u
#define BIG_BUFFERS_COUNT       1000u
#define SMALL_BUFFER_SIZE       256u
#define SMALL_BUFFERS_COUNT     40u

/** \brief Packet allocator instanciation */
STATIC_INSTANCE_BIG_SMALL_PACKET_ALLOCATOR(BIG_BUFFER_SIZE, BIG_BUFFERS_COUNT, SMALL_BUFFER_SIZE, SMALL_BUFFERS_COUNT);




/** \brief Mutex for the log output function */
static pthread_mutex_t log_output_mutex;


/** \brief Initialize the operating system */
bool NANO_IP_BSP_OSInit()
{
	bool ret = false;

	pthread_mutexattr_t mutex_attr;
	pthread_mutexattr_init(&mutex_attr);
	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
	if (pthread_mutex_init(&log_output_mutex, &mutex_attr) == 0)
	{
		ret = true;
	}
	pthread_mutexattr_destroy(&mutex_attr);

	return ret;
}

/** \brief Start the operating system (should never return on success) */
bool NANO_IP_BSP_OSStart()
{
    usleep(0x7FFFFFFFu);
    return true;
}


/** \brief Instanciate the packet allocator */
bool NANO_IP_BSP_CreatePacketAllocator(nano_ip_net_packet_allocator_t* const packet_allocator)
{
    nano_ip_error_t err;

    BIG_SMALL_PACKET_ALLOCATOR_INIT(BIG_BUFFER_SIZE, BIG_BUFFERS_COUNT, SMALL_BUFFER_SIZE, SMALL_BUFFERS_COUNT);
    err = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Init(packet_allocator, &big_small_packet_allocator_data);

    return (err == NIP_ERR_SUCCESS);
}

/** \brief Instanciate the network interface */
bool NANO_IP_BSP_CreateNetIf(nano_ip_net_if_t* const net_if, uint32_t* const rx_packet_count, uint32_t* const rx_packet_size)
{
    const nano_ip_error_t err = NANO_IP_PCAP_Init(net_if, "pcap0");
    (*rx_packet_count) = 900u;
    (*rx_packet_size) = BIG_BUFFER_SIZE;
    return (err == NIP_ERR_SUCCESS);
}


/** \brief Log output function */
void NANO_IP_BSP_Printf(const char* format, va_list arg_list)
{
    pthread_mutex_lock(&log_output_mutex);
    vprintf(format, arg_list);
    pthread_mutex_unlock(&log_output_mutex);
}




