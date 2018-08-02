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

#ifndef NANO_IP_IPV4_DEF_H
#define NANO_IP_IPV4_DEF_H


#include "nano_ip_types.h"


/** \brief IP protocol identifier */
#define IP_PROTOCOL    0x0800u

/** \brief IPv4 broadcast address */
#define IPV4_BROADCAST_ADDRESS          0xFFFFFFFFu

/** \brief IPv4 any address */
#define IPV4_ANY_ADDRESS                0x00000000u

/** \brief IPv4 address size in bytes */
#define IPV4_ADDRESS_SIZE               4u

/** \brief Maximum IPv4 address string size including null char terminator */
#define MAX_IPV4_ADDRESS_STRING_SIZE    16u

/** \brief IPv4 localhost address */
#define IPV4_LOCALHOST_ADDR             "127.0.0.1"

/** \brief IPv4 localhost netmask */
#define IPV4_LOCALHOST_NETMASK          "255.0.0.0"



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief IPv4 address */
typedef uint32_t ipv4_address_t;



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_IPV4_DEF_H */
