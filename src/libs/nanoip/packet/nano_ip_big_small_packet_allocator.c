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

#include "nano_ip_big_small_packet_allocator.h"
#include "nano_ip_tools.h"

/** \brief Allocate a packet */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Allocate(void* const allocator_data, nano_ip_net_packet_t** const packet, const uint16_t size);

/** \brief Release a packet */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Release(void* const allocator_data, nano_ip_net_packet_t* const packet);

/** \brief Allocate a buffer */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_AllocateBuffer(big_small_buffer_desc_t** const free_buffers, big_small_buffer_desc_t** used_buffers, big_small_buffer_desc_t** allocated_buffer);

/** \brief Release a buffer */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_ReleaseBuffer(big_small_buffer_desc_t* allocated_buffer, big_small_buffer_desc_t** const free_buffers, big_small_buffer_desc_t** used_buffers);


/** \brief Initialize the packet allocator */
nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Init(nano_ip_net_packet_allocator_t* const allocator, big_small_packet_allocator_data_t* const allocator_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((allocator != NULL) && (allocator_data != NULL))
    {
        uint16_t i = 0;

        /* 0 init */
        MEMSET(allocator, 0, sizeof(nano_ip_net_packet_allocator_t));
        MEMSET(&allocator_data->internal_data, 0, sizeof(big_small_packet_allocator_internal_data_t));

        /* Initialize allocator */
        allocator->allocate = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Allocate;
        allocator->release = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Release;
        allocator->allocator_data = allocator_data;

        /* Initialize buffers lists */
        for (i = 0u; i < allocator_data->big_buffers_count; i++)
        {
            allocator_data->big_buffers_descs[i].data = &allocator_data->big_buffers[i * allocator_data->big_buffer_size];
            allocator_data->big_buffers_descs[i].next = &allocator_data->big_buffers_descs[i + 1u];
        }
        allocator_data->big_buffers_descs[i - 1u].next = NULL;
        for (i = 0u; i < allocator_data->small_buffers_count; i++)
        {
            allocator_data->small_buffers_descs[i].data = &allocator_data->small_buffers[i * allocator_data->small_buffer_size];
            allocator_data->small_buffers_descs[i].next = &allocator_data->small_buffers_descs[i + 1u];
        }
        allocator_data->small_buffers_descs[i - 1u].next = NULL;
        allocator_data->internal_data.free_big_buffers = allocator_data->big_buffers_descs;
        allocator_data->internal_data.free_small_buffers = allocator_data->small_buffers_descs;

        /* Initialize mutex */
        ret = NANO_IP_OAL_MUTEX_Create(&allocator_data->internal_data.mutex);
    }

    return ret;
}


/** \brief Allocate a packet */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Allocate(void* const allocator_data, nano_ip_net_packet_t** const packet, const uint16_t size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    big_small_packet_allocator_data_t* allocator = NANO_IP_CAST(big_small_packet_allocator_data_t*, allocator_data);

    /* Check parameters */
    if ((allocator != NULL) && (packet != NULL))
    {
        big_small_buffer_desc_t* buffer = NULL;

        /* Lock allocator */
        (void)NANO_IP_OAL_MUTEX_Lock(&allocator->internal_data.mutex);

        /* Check requested size */
        if (size <= allocator->small_buffer_size)
        {
            /* Small buffer */
            ret = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_AllocateBuffer(&allocator->internal_data.free_small_buffers, &allocator->internal_data.used_small_buffers, &buffer);
            if (ret != NIP_ERR_SUCCESS)
            {
                /* Try to allocate a big buffer instead */
                ret = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_AllocateBuffer(&allocator->internal_data.free_big_buffers, &allocator->internal_data.used_big_buffers, &buffer);
            }
        }
        else if (size <= allocator->big_buffer_size)
        {
            /* Big buffer */
            ret = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_AllocateBuffer(&allocator->internal_data.free_big_buffers, &allocator->internal_data.used_big_buffers, &buffer);
        }
        else
        {
            /* Buffer too big to be allocated */
            ret = NIP_ERR_PACKET_TOO_BIG;
        }

        /* Fill packet information */
        if (ret == NIP_ERR_SUCCESS)
        {
            nano_ip_net_packet_t* const pkt = &buffer->packet;
            pkt->data = buffer->data;
            pkt->current = NANO_IP_CAST(uint8_t*, pkt->data);
            pkt->size = size;
            pkt->count = 0u;
            pkt->flags = 0u;
            pkt->allocator_data = buffer;
            pkt->net_if = NULL;
            (*packet) = pkt;
        }

        /* Unlock allocator */
        (void)NANO_IP_OAL_MUTEX_Unlock(&allocator->internal_data.mutex);
    }

    return ret;
}

/** \brief Release a packet */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Release(void* const allocator_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    big_small_packet_allocator_data_t* allocator = NANO_IP_CAST(big_small_packet_allocator_data_t*, allocator_data);
    
    /* Check parameters */
    if ((allocator != NULL) && (packet != NULL) && (packet->data != NULL))
    {
        big_small_buffer_desc_t* buffer = NANO_IP_CAST(big_small_buffer_desc_t*, packet->allocator_data);

        /* Lock allocator */
        (void)NANO_IP_OAL_MUTEX_Lock(&allocator->internal_data.mutex);

        /* Check packet size */
        if (packet->size <= allocator->small_buffer_size)
        {
            /* Small buffer */
            ret = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_ReleaseBuffer(buffer, &allocator->internal_data.free_small_buffers, &allocator->internal_data.used_small_buffers);
            if (ret != NIP_ERR_SUCCESS)
            {
                /* Try to release a big buffer instead */
                ret = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_ReleaseBuffer(buffer, &allocator->internal_data.free_big_buffers, &allocator->internal_data.used_big_buffers);
            }
        }
        else if (packet->size <= allocator->big_buffer_size)
        {
            /* Big buffer */
            ret = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_ReleaseBuffer(buffer, &allocator->internal_data.free_big_buffers, &allocator->internal_data.used_big_buffers);
        }
        else
        {
            /* Buffer too big to be allocated */
            ret = NIP_ERR_PACKET_TOO_BIG;
        }

        /* Unlock allocator */
        (void)NANO_IP_OAL_MUTEX_Unlock(&allocator->internal_data.mutex);
    }

    return ret;
}

/** \brief Allocate a buffer */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_AllocateBuffer(big_small_buffer_desc_t** const free_buffers, big_small_buffer_desc_t** used_buffers, big_small_buffer_desc_t** allocated_buffer)
{
    nano_ip_error_t ret = NIP_ERR_RESOURCE;
    
    /* Check if a buffer is available */
    if ((*free_buffers) != NULL)
    {
        /* Remove the buffer from the free buffers list */
        (*allocated_buffer) = (*free_buffers);
        (*free_buffers) = (*free_buffers)->next;

        /* Add the buffer to the used buffers list */
        (*allocated_buffer)->next = (*used_buffers);
        (*used_buffers) = (*allocated_buffer);

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Release a buffer */
static nano_ip_error_t NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_ReleaseBuffer(big_small_buffer_desc_t* allocated_buffer, big_small_buffer_desc_t** const free_buffers, big_small_buffer_desc_t** used_buffers)
{
    nano_ip_error_t ret = NIP_ERR_PACKET_NOT_FOUND;

    /* Look for the buffer in the used buffers list */
    big_small_buffer_desc_t* used_buffer = (*used_buffers);
    big_small_buffer_desc_t* previous_used_buffer = NULL;
    while ((used_buffer != NULL) && (ret != NIP_ERR_SUCCESS))
    {
        if (used_buffer == allocated_buffer)
        {
            /* Remove the buffer from the used buffers list */
            if (previous_used_buffer != NULL)
            {
                previous_used_buffer->next = allocated_buffer->next;
            }
            else
            {
                (*used_buffers) = allocated_buffer->next;
            }

            /* Add the buffer to the free buffers list */
            allocated_buffer->next = (*free_buffers);
            (*free_buffers) = allocated_buffer;

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            /* Next buffer */
            previous_used_buffer = used_buffer;
            used_buffer = used_buffer->next;
        }
    }

    return ret;
}