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

#ifndef NANO_IP_H
#define NANO_IP_H

#include "nano_ip_error.h"
#include "nano_ip_types.h"
#include "nano_ip_oal.h"
#include "nano_ip_tools.h"

#include "nano_ip_net_if.h"
#include "nano_ip_net_ifaces.h"
#include "nano_ip_packet_allocator.h"
#include "nano_ip_localhost.h"
#include "nano_ip_ethernet.h"
#include "nano_ip_arp.h"
#include "nano_ip_route.h"
#include "nano_ip_ipv4.h"
#include "nano_ip_icmp.h"
#include "nano_ip_udp.h"
#include "nano_ip_tcp.h"

#include "nano_ip_socket.h"
#include "nano_ip_dhcp_client.h"
#include "nano_ip_tftp_client.h"
#include "nano_ip_tftp_server.h"

#include "nano_ip_log.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Initialize Nano IP stack */
nano_ip_error_t NANO_IP_Init(const nano_ip_net_packet_allocator_t* const packet_allocator);

/** \brief Start Nano IP stack */
nano_ip_error_t NANO_IP_Start(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_H */
