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

#ifndef NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_H
#define NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_H

#include "nano_ip_oal.h"
#include "nano_ip_packet_allocator.h"



/** \brief Helper macro to instanciate the big small packet allocator data with section placement and alignment */
#define STATIC_INSTANCE_BIG_SMALL_PACKET_ALLOCATOR_WITH_PLACEMENT(_big_buffer_size, _big_buffer_count, _small_buffer_size, _small_buffer_count, _section, alignment) \
                                    \
                                    static uint8_t big_buffers[(_big_buffer_count) * (_big_buffer_size)] __attribute__ ((section (_section))) __attribute__((aligned(alignment))); \
                                    static uint8_t small_buffers[(_small_buffer_count) * (_small_buffer_size)] __attribute__ ((section (_section))) __attribute__((aligned(alignment))); \
                                    \
                                    static big_small_buffer_desc_t big_buffers_descs[(_big_buffer_count)] __attribute__ ((section (_section))); \
                                    static big_small_buffer_desc_t small_buffers_descs[(_small_buffer_count)] __attribute__ ((section (_section))); \
                                    static big_small_packet_allocator_data_t big_small_packet_allocator_data __attribute__ ((section (_section)))


/** \brief Helper macro to instanciate the big small packet allocator data with placement and alignment*/
#define STATIC_INSTANCE_BIG_SMALL_PACKET_ALLOCATOR(_big_buffer_size, _big_buffer_count, _small_buffer_size, _small_buffer_count) \
                                    \
                                    static uint8_t big_buffers[(_big_buffer_count) * (_big_buffer_size)]; \
                                    static big_small_buffer_desc_t big_buffers_descs[(_big_buffer_count)]; \
                                    \
                                    static uint8_t small_buffers[(_small_buffer_count) * (_small_buffer_size)]; \
                                    static big_small_buffer_desc_t small_buffers_descs[(_small_buffer_count)]; \
                                    static big_small_packet_allocator_data_t big_small_packet_allocator_data




/** \brief Helper macro to initialize packet the big small allocator object */
#define BIG_SMALL_PACKET_ALLOCATOR_INIT(_big_buffer_size, _big_buffer_count, _small_buffer_size, _small_buffer_count) \
                                    \
                                    big_small_packet_allocator_data.big_buffer_size = (_big_buffer_size); \
                                    big_small_packet_allocator_data.big_buffers_count = (_big_buffer_count); \
                                    big_small_packet_allocator_data.big_buffers = big_buffers; \
                                    big_small_packet_allocator_data.big_buffers_descs = big_buffers_descs; \
                                    big_small_packet_allocator_data.small_buffer_size = (_small_buffer_size); \
                                    big_small_packet_allocator_data.small_buffers_count = (_small_buffer_count); \
                                    big_small_packet_allocator_data.small_buffers = small_buffers; \
                                    big_small_packet_allocator_data.small_buffers_descs = small_buffers_descs


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \brief Buffer description */
typedef struct _big_small_buffer_desc_t
{
    /** \brief Data */
    void* data;
    /** \brief Packet */
    nano_ip_net_packet_t packet;
    /** \brief Next buffer */
    struct _big_small_buffer_desc_t* next;
} big_small_buffer_desc_t;


/** \brief Internal data for the big small packet allocator */
typedef struct _big_small_packet_allocator_internal_data_t
{
    /** \brief Free big buffers list */
    big_small_buffer_desc_t* free_big_buffers;
    /** \brief Used big buffers list */
    big_small_buffer_desc_t* used_big_buffers;

    /** \brief Free small buffers list */
    big_small_buffer_desc_t* free_small_buffers;
    /** \brief Used small buffers list */
    big_small_buffer_desc_t* used_small_buffers;

    /** \brief Mutex */
    oal_mutex_t mutex;
} big_small_packet_allocator_internal_data_t;


/** \brief Init data for the big small packet allocator */
typedef struct _big_small_packet_allocator_data_t
{
    /* To be initialized by the application befor calling init() function */

    /** \brief Big buffer size in bytes */
    uint16_t big_buffer_size;
    /** \brief Number of big buffers */
    uint16_t big_buffers_count;
    /** \brief Pointer to the big buffers array */
    uint8_t* big_buffers;
    /** \brief Pointer to the big buffers descriptions array */
    big_small_buffer_desc_t* big_buffers_descs;

    /** \brief Small buffer size in bytes */
    uint16_t small_buffer_size;
    /** \brief Number of small buffers */
    uint16_t small_buffers_count;
    /** \brief Pointer to the small buffers array */
    uint8_t* small_buffers;
    /** \brief Pointer to the small buffers descriptions array */
    big_small_buffer_desc_t* small_buffers_descs;


    /* Initialized by the big small packet buffer allocator */

    /** \brief Internal allocator data */
    big_small_packet_allocator_internal_data_t internal_data;

} big_small_packet_allocator_data_t;




/** \brief Initialize the packet allocator */
nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Init(nano_ip_net_packet_allocator_t* const allocator, big_small_packet_allocator_data_t* const allocator_data);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_H */
