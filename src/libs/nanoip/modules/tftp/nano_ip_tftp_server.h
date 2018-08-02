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

#ifndef NANO_IP_TFTP_SERVER_H
#define NANO_IP_TFTP_SERVER_H

#include "nano_ip_cfg.h"

#if( (NANO_IP_ENABLE_TFTP == 1) && (NANO_IP_ENABLE_TFTP_SERVER == 1) )

#include "nano_ip_tftp.h"


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief TFTP server module internal data */
typedef struct _nano_ip_tftp_server_t
{
    /** \brief TFTP module internal data */
    nano_ip_tftp_t tftp_module;

    #if (NANO_IP_ENABLE_TFTP_SERVER_TASK == 1u)
    /** \brief Task */
    oal_task_t task;
    /** \brief Mutex */
    oal_mutex_t mutex;
    /** \brief Synchronization object */
    oal_flags_t sync_flags;
    /** \brief Rx packets */
    nano_ip_net_packet_t* rx_packets;
    #endif /* NANO_IP_ENABLE_TFTP_SERVER_TASK */
} nano_ip_tftp_server_t;



/** \brief Initialize a TFTP server instance */
nano_ip_error_t NANO_IP_TFTP_SERVER_Init(nano_ip_tftp_server_t* const tftp_server, const uint32_t listen_address, const uint16_t listen_port, const nano_ip_tftp_callbacks_t* const callbacks, void* const user_data, const uint32_t timeout);

/** \brief Start a TFTP server instance */
nano_ip_error_t NANO_IP_TFTP_SERVER_Start(nano_ip_tftp_server_t* const tftp_server);

/** \brief Stop a TFTP server instance */
nano_ip_error_t NANO_IP_TFTP_SERVER_Stop(nano_ip_tftp_server_t* const tftp_server);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_TFTP && NANO_IP_ENABLE_TFTP_SERVER */

#endif /* NANO_IP_TFTP_SERVER_H */
