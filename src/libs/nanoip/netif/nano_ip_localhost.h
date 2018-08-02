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

#ifndef NANO_IP_LOCALHOST_H
#define NANO_IP_LOCALHOST_H

#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_LOCALHOST == 1 )

#include "nano_ip_net_if.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Localhost driver data */
typedef struct _localhost_drv_t
{
    /** \brief Callbacks */
    net_driver_callbacks_t callbacks;
    /** \brief List of received packets */
    nano_ip_packet_queue_t* received_packets;
    /** \brief Interface's mutex */
    oal_mutex_t mutex;
} localhost_drv_t;

/** \brief Localhost module internal data */
typedef struct _nano_ip_localhost_module_data_t
{
    /** \brief Network interface */
    nano_ip_net_if_t net_if;
    /** \brief Driver data */
    localhost_drv_t driver;
} nano_ip_localhost_module_data_t;



/** \brief Initialize localhost interface */
nano_ip_error_t NANO_IP_LOCALHOST_Init(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_LOCALHOST */

#endif /* NANO_IP_LOCALHOST_H */
