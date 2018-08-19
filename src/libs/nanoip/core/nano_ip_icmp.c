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

#include "nano_ip_icmp.h"

#if( NANO_IP_ENABLE_ICMP == 1 )

#include "nano_ip_data.h"
#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"


/** \brief ICMP protocol id */
#define ICMP_PROTOCOL   0x01u


/** \brief ICMP header size in bytes */
#define ICMP_HEADER_SIZE    0x04u

/** \brief ICMP ping request header in bytes */
#define ICMP_PING_REQ_HEADER_SIZE   0x04u


/** \brief Ping request success flag */
#define PING_REQ_SUCCESS_FLAG    0x01u

/** \brief Ping request cancel flag */
#define PING_REQ_CANCEL_FLAG     0x02u

/** \brief Ping request timeout flag */
#define PING_REQ_TIMEOUT_FLAG    0x04u

/** \brief Ping arp error flag */
#define PING_REQ_ARP_ERROR_FLAG  0x08u

/** \brief Ping failure flag */
#define PING_REQ_FAILURE_FLAG    0x10u


/** \brief Handle an IPv4 error */
static void NANO_IP_ICMP_Ipv4ErrorCallback(void* const user_data, const nano_ip_error_t error);

/** \brief Handle a received ICMP frame */
static nano_ip_error_t NANO_IP_ICMP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet);

/** \brief Handle an ICMP ping request */
static nano_ip_error_t NANO_IP_ICMP_HandlePingRequest(const ipv4_header_t* const ipv4_header, const uint8_t* const start_of_request_data, const uint16_t request_data_size);

#if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

/** \brief Handle an ICMP ping request */
static nano_ip_error_t NANO_IP_ICMP_HandlePingReply(const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet);

/** \brief ICMP periodic task */
static void NANO_IP_ICMP_PeriodicTask(const uint32_t timestamp, void* const user_data);

#endif /* NANO_IP_ENABLE_ICMP_PING_REQ */



/** \brief Initialize the ICMP module */
nano_ip_error_t NANO_IP_ICMP_Init(void)
{
    nano_ip_error_t ret;
    nano_ip_icmp_module_data_t* const icmp_module = &g_nano_ip.icmp_module;

    /* Initialize IPv4 handle */
    ret = NANO_IP_IPV4_InitializeHandle(&icmp_module->ipv4_handle, NULL, NANO_IP_ICMP_Ipv4ErrorCallback);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Register protocol */
        icmp_module->ipv4_protocol.protocol = ICMP_PROTOCOL;
        icmp_module->ipv4_protocol.rx_frame = NANO_IP_ICMP_RxFrame;
        icmp_module->ipv4_protocol.user_data = icmp_module;
        ret = NANO_IP_IPV4_AddProtocol(&icmp_module->ipv4_protocol);

        #if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)
        /* Register periodic callback */
        if (ret == NIP_ERR_SUCCESS)
        {
            icmp_module->ipv4_callback.callback = NANO_IP_ICMP_PeriodicTask;
            icmp_module->ipv4_callback.user_data = icmp_module;
            ret = NANO_IP_IPV4_RegisterPeriodicCallback(&icmp_module->ipv4_callback);
        }
        #endif /* NANO_IP_ENABLE_ICMP_PING_REQ */
    }

    return ret;
}


#if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

/** \brief Initialize an ICMP request handle */
nano_ip_error_t NANO_IP_ICMP_InitRequest(nano_ip_icmp_request_t* const request)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (request != NULL)
    {
        /* 0 init */
        NANO_IP_MEMSET(request, 0, sizeof(nano_ip_icmp_request_t));

        /* Initialize IPv4 handle */
        ret = NANO_IP_IPV4_InitializeHandle(&request->ipv4_handle, request, NANO_IP_ICMP_Ipv4ErrorCallback);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Initialize sync object */
            ret = NANO_IP_OAL_FLAGS_Create(&request->sync_obj);
        }
    }

    return ret;
}

/** \brief Initiate an ICMP ping request */
nano_ip_error_t NANO_IP_ICMP_PingRequest(nano_ip_icmp_request_t* const request, const uint32_t ipv4_address, const uint32_t timeout, const uint8_t data_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (request != NULL)
    {
        /* Reset synchronization object */
        ret = NANO_IP_OAL_FLAGS_Reset(&request->sync_obj, NANO_IP_OAL_FLAGS_ALL);
        if (ret == NIP_ERR_SUCCESS)
        {
            nano_ip_net_packet_t* packet;
            const uint16_t packet_size = ICMP_HEADER_SIZE + ICMP_PING_REQ_HEADER_SIZE + data_size + data_size%2;

            /* Allocate a ping frame */
            ret = NANO_IP_IPV4_AllocatePacket(packet_size, &packet);
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Fill header */
                uint8_t i;
                uint16_t checksum;
                uint8_t* checksum_pos;
                ipv4_header_t header;
                uint8_t* const header_start = packet->current;
                nano_ip_icmp_module_data_t* const icmp_module = &g_nano_ip.icmp_module;
                
                NANO_IP_PACKET_Write8bits(packet, ICMP_ECHO_REQUEST);
                NANO_IP_PACKET_Write8bits(packet, 0x00u);
                checksum_pos = packet->current;
                NANO_IP_PACKET_Write16bits(packet, 0x0000u);

                /* Ping request header */
                request->identifier = NANO_IP_OAL_TIME_GetMsCounter();
                NANO_IP_PACKET_Write32bits(packet, request->identifier);

                /* Request data */
                for (i = 0u; i < (data_size + data_size % 2); i++)
                {
                    NANO_IP_PACKET_Write8bits(packet, i);
                }

                /* Write checksum */
                checksum = NANO_IP_ComputeInternetCS(NULL, 0u, header_start, packet_size);
                checksum_pos[0] = NANO_IP_CAST(uint8_t, (checksum & 0xFFu));
                checksum_pos[1] = NANO_IP_CAST(uint8_t, (checksum >> 8u));

                /* Prepare IPv4 header */
                header.dest_address = ipv4_address;
                header.src_address = 0u;
                header.protocol = ICMP_PROTOCOL;

                /* Send request */
                ret = NANO_IP_IPV4_SendPacket(&icmp_module->ipv4_handle, &header, packet);
                if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
                {
                    /* Update request data */
                    request->ipv4_address = ipv4_address;
                    request->response_time = NANO_IP_OAL_TIME_GetMsCounter();
                    request->timeout = request->response_time + timeout;

                    /* Add request to the list */
                    request->next = icmp_module->requests;
                    icmp_module->requests = request;
                }
                else
                {
                    /* Release packet */
                    (void)NANO_IP_IPV4_ReleasePacket(packet);
                }
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}


/** \brief Wait for an ICMP request */
nano_ip_error_t NANO_IP_ICMP_WaitRequest(nano_ip_icmp_request_t* const request, const uint32_t timeout)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (request != NULL)
    {
        uint32_t flags;

        /* Wait for synchronization object */
        ret = NANO_IP_OAL_FLAGS_Wait(&request->sync_obj, &flags, true, timeout);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Check flags */
            if ((flags & PING_REQ_SUCCESS_FLAG) == PING_REQ_SUCCESS_FLAG)
            {
                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                ret = NIP_ERR_INVALID_PING_REQUEST;
            }
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

/** \brief Cancel an ICMP request */
nano_ip_error_t NANO_IP_ICMP_CancelRequest(nano_ip_icmp_request_t* const request)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    (void)NANO_IP_OAL_MUTEX_Lock(&g_nano_ip.mutex);

    /* Check parameters */
    if (request != NULL)
    {
        /* Remove request from the list */
        nano_ip_icmp_request_t* req;
        nano_ip_icmp_request_t* previous_req = NULL;
        nano_ip_icmp_module_data_t* const icmp_module = &g_nano_ip.icmp_module;

        req = icmp_module->requests;
        while ((req != NULL) && (req != request))
        {
            previous_req = req;
            req = req->next;
        }
        if (req != NULL)
        {
            if (previous_req == NULL)
            {
                icmp_module->requests = NULL;
            }
            else
            {
                previous_req->next = req->next;
            }
        }

        if (req != NULL)
        {
            /* Notify end of request */
            ret = NANO_IP_OAL_FLAGS_Set(&request->sync_obj, PING_REQ_CANCEL_FLAG, false);
        }
    }

    (void)NANO_IP_OAL_MUTEX_Unlock(&g_nano_ip.mutex);

    return ret;
}

#endif /* NANO_IP_ENABLE_ICMP_PING_REQ */


/** \brief Handle an IPv4 error */
static void NANO_IP_ICMP_Ipv4ErrorCallback(void* const user_data, const nano_ip_error_t error)
{
    #if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

    nano_ip_icmp_request_t* const request = NANO_IP_CAST(nano_ip_icmp_request_t*, user_data);

    /* Notify end of request */
    if (request != NULL)
    {
        uint32_t flag = PING_REQ_FAILURE_FLAG;
        if (error == NIP_ERR_ARP_FAILURE)
        {
            flag = PING_REQ_ARP_ERROR_FLAG;
        }
        (void)NANO_IP_OAL_FLAGS_Set(&request->sync_obj, flag, false);
    }

    #else

    (void)user_data;
    (void)error;

    #endif /* NANO_IP_ENABLE_ICMP_PING_REQ */
}

/** \brief Handle a received ICMP frame */
static nano_ip_error_t NANO_IP_ICMP_RxFrame(void* user_data, nano_ip_net_if_t* const net_if, const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    nano_ip_icmp_module_data_t* const icmp_module = NANO_IP_CAST(nano_ip_icmp_module_data_t*, user_data);

    /* Check parameters */
    if ((icmp_module != NULL) && (net_if != NULL) &&
        (ipv4_header != NULL) && (packet != NULL))

    {
        /* Check packet size */
        if (packet->count >= ICMP_HEADER_SIZE)
        {
            /* Decode packet */
            uint8_t type;
            uint16_t null_checksum;
            uint8_t* const header_start = packet->current;
            const uint16_t packet_size = packet->count;
            type = NANO_IP_PACKET_Read8bits(packet);
            NANO_IP_PACKET_ReadSkipBytes(packet, 1u); /* code */
            NANO_IP_PACKET_ReadSkipBytes(packet, 2u); /* checksum */
            
            /* Compute checkum */
            null_checksum = NANO_IP_ComputeInternetCS(NULL, 0u, header_start, packet_size);
            if (null_checksum == 0u)
            {
                if (type == ICMP_ECHO_REQUEST)
                {
                    ret = NANO_IP_ICMP_HandlePingRequest(ipv4_header, packet->current, packet_size);
                }
                #if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)
                else if (type == ICMP_ECHO_REPLY)
                {
                    ret = NANO_IP_ICMP_HandlePingReply(ipv4_header, packet);
                }
                #endif /* NANO_IP_ENABLE_ICMP_PING_REQ */
                else
                {
                    ret = NIP_ERR_IGNORE_PACKET;
                }
            }
            else
            {
                ret = NIP_ERR_INVALID_CS;
            }
        }
        else
        {
            ret = NIPP_ERR_INVALID_PACKET_SIZE;
        }
    }

    return ret;
}


/** \brief Handle an ICMP ping request */
static nano_ip_error_t NANO_IP_ICMP_HandlePingRequest(const ipv4_header_t* const ipv4_header, const uint8_t* const start_of_request_data, const uint16_t request_data_size)
{
    nano_ip_net_packet_t* packet = NULL;
    nano_ip_error_t ret = NIP_ERR_IGNORE_PACKET;

    /* Allocate an IPv4 packet */
    ret = NANO_IP_IPV4_AllocatePacket(request_data_size, &packet);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Fill header */
        uint16_t checksum;
        uint8_t* checksum_pos;
        ipv4_header_t header;
        uint8_t* const header_start = packet->current;
        nano_ip_icmp_module_data_t* const icmp_module = &g_nano_ip.icmp_module;

        NANO_IP_PACKET_Write8bits(packet, ICMP_ECHO_REPLY);
        NANO_IP_PACKET_Write8bits(packet, 0x00u);
        checksum_pos = packet->current;
        NANO_IP_PACKET_Write16bits(packet, 0x0000u);

        /* Copy request data */
        NANO_IP_PACKET_WriteBuffer(packet, start_of_request_data, request_data_size - ICMP_HEADER_SIZE);

        /* Write checksum */
        checksum = NANO_IP_ComputeInternetCS(NULL, 0u, header_start, request_data_size);
        checksum_pos[0] = NANO_IP_CAST(uint8_t, (checksum & 0xFFu));
        checksum_pos[1] = NANO_IP_CAST(uint8_t, (checksum >> 8u));

        /* Prepare IPv4 header */
        header.dest_address = ipv4_header->src_address;
        header.src_address = ipv4_header->dest_address;
        header.protocol = ICMP_PROTOCOL;

        /* Send packet */
        ret = NANO_IP_IPV4_SendPacket(&icmp_module->ipv4_handle, &header, packet);
        if ((ret != NIP_ERR_SUCCESS) && (ret != NIP_ERR_IN_PROGRESS))
        {
            /* Release packet */
            (void)NANO_IP_IPV4_ReleasePacket(packet);
        }
    }

    return ret;
}

#if (NANO_IP_ENABLE_ICMP_PING_REQ == 1)

/** \brief Handle an ICMP ping request */
static nano_ip_error_t NANO_IP_ICMP_HandlePingReply(const ipv4_header_t* const ipv4_header, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_IGNORE_PACKET;
    nano_ip_icmp_request_t* request;
    nano_ip_icmp_request_t* previous = NULL;
    nano_ip_icmp_module_data_t* const icmp_module = &g_nano_ip.icmp_module;

    /* Decode ping request identifier */
    const uint32_t identifier = NANO_IP_PACKET_Read32bits(packet);

    /* May be used later... */
    (void)ipv4_header;

    /* Look for a corresponding request */
    request = icmp_module->requests;
    while ((request != NULL) && 
           (request->identifier != identifier))
    {
        previous = request;
        request = request->next;
    }
    if (request != NULL)
    {
        /* Compute the response time */
        const uint32_t timestamp = NANO_IP_OAL_TIME_GetMsCounter();
        request->response_time = timestamp - request->response_time;

        /* Remove request from the list */
        if (previous == NULL)
        {
            icmp_module->requests = NULL;
        }
        else
        {
            previous->next = request->next;
        }

        /* Notify end of request */
        ret = NANO_IP_OAL_FLAGS_Set(&request->sync_obj, PING_REQ_SUCCESS_FLAG, false);
    }

    return ret;
}


/** \brief ICMP periodic task */
static void NANO_IP_ICMP_PeriodicTask(const uint32_t timestamp, void* const user_data)
{
    nano_ip_icmp_module_data_t* const icmp_module = NANO_IP_CAST(nano_ip_icmp_module_data_t*, user_data);
    if (icmp_module != NULL)
    {
        /* Check requests timeout */
        nano_ip_icmp_request_t* previous_request = NULL;
        nano_ip_icmp_request_t* request = icmp_module->requests;

        while (request != NULL)
        {
            /* Check timeout */
            if (request->timeout < timestamp)
            {
                /* Remove request from list */
                if (previous_request == NULL)
                {
                    icmp_module->requests = request->next;
                }
                else
                {
                    previous_request->next = request->next;
                }

                /* Notify end of request */
                (void)NANO_IP_OAL_FLAGS_Set(&request->sync_obj, PING_REQ_TIMEOUT_FLAG, false);
            }
            else
            {
                previous_request = request;
            }

            /* Next request */
            request = request->next;
        }
    }
}


#endif /* NANO_IP_ENABLE_ICMP_PING_REQ */


#endif /* NANO_IP_ENABLE_ICMP */
