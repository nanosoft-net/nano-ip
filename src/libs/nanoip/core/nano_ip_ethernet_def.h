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

#ifndef NANO_IP_ETHERNET_DEF_H
#define NANO_IP_ETHERNET_DEF_H


#include "nano_ip_types.h"


/** \brief Ethernet header size in bytes */
#define ETHERNET_HEADER_SIZE            14u

/** \brief MAC address size in bytes */
#define MAC_ADDRESS_SIZE    6u




#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Ethernet broadcast MAC address */
extern const uint8_t ETHERNET_BROADCAST_MAC_ADDRESS[MAC_ADDRESS_SIZE];

/** \brief Ethernet NULL MAC address */
extern const uint8_t ETHERNET_NULL_MAC_ADDRESS[MAC_ADDRESS_SIZE];


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_ETHERNET_DEF_H */
