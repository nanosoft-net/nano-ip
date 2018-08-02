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

#ifndef NANO_IP_CFG_H
#define NANO_IP_CFG_H


/*********************************************************/
/*  Configuration of Nano-IP use of standard libraries    */
/*********************************************************/


/** \brief Indicate if the standard integer datatypes must be defined using <stdint.h> */
#define USE_STD_INT                     1

/** \brief Indicate if the standard boolean datatype must be defined using <stdbool.h> */
#define USE_STD_BOOL                    1

/** \brief Indicate if the standard size_t datatype must be defined using <stddef.h> */
#define USE_STD_SIZE_T                  1

/** \brief Indicate if memset function from <string.h> must be used */
#define USE_STD_MEMSET                  1

/** \brief Indicate if memset function from <string.h> must be used */
#define USE_STD_MEMCPY                  1

/** \brief Indicate if memcmp function from <string.h> must be used */
#define USE_STD_MEMCMP                  1

/** \brief Indicate if strncmp function from <string.h> must be used */
#define USE_STD_STRNCMP                 1

/** \brief Indicate if strnlen function from <string.h> must be used */
#define USE_STD_STRNLEN                 1

/** \brief Indicate if strncat function from <string.h> must be used */
#define USE_STD_STRNCAT                 1

/** \brief Indicate if atoi function from <stdlib.h> must be used */
#define USE_STD_ATOI                    0

/** \brief Indicate if itoa function from <stdlib.h> must be used */
#define USE_STD_ITOA                    0


/*********************************************************/
/*   Configuration of NanoIP core functionalities        */
/*********************************************************/



/** \brief Enable localhost interface */
#define NANO_IP_ENABLE_LOCALHOST                1u


/** \brief Maximum number of network routes (must be at least (2u * NANO_IP_MAX_NET_INTERFACES_COUNT + 2u)) */
#define NANO_IP_MAX_NET_ROUTE_COUNT             ((2u * 2u + 2u) + 0u)

/** \brief Maximum number of ARP table entries */
#define NANO_IP_MAX_ARP_ENTRY_COUNT             10u

/** \brief Validity period in milliseconds of a dynamic entry in the ARP table */
#define NANO_IP_ARP_ENTRY_VALIDITY_PERIOD       600000u /* 10 minutes */

/** \brief ARP request timeout in milliseconds */
#define NANO_IP_ARP_REQUEST_TIMEOUT             500u

/** \brief Enable ICMP protocol */
#define NANO_IP_ENABLE_ICMP                     1u

/** \brief Enable ICMP protocol ping requests */
#define NANO_IP_ENABLE_ICMP_PING_REQ            1u

/** \brief Enable UDP protocol */
#define NANO_IP_ENABLE_UDP                      1u

/** \brief Enable UDP checksum computation and verification */
#define NANO_IP_ENABLE_UDP_CHECKSUM             1u

/** \brief Enable TCP protocol */
#define NANO_IP_ENABLE_TCP                      1u


/*********************************************************/
/*            Configuration of NanoIP log                */
/*********************************************************/

/** \brief Enable logs */
#define NANO_IP_ENABLE_LOGS                     1u

/** \brief Minimal log level, must be one of the following : NANO_IP_DEBUG, NANO_IP_INFO, NANO_IP_ERROR */
#define NANO_IP_LOG_LEVEL                       NANO_IP_DEBUG

/** \brief Function which will output the logs, must have a signature similar to vprintf(const char *format, va_list arg_list) function */
#define NANO_IP_LOG_FUNC(format, arg_list)      NANO_IP_BSP_Printf((format), (arg_list))

/** \brief Header file which exports the above function */
#define NANO_IP_LOG_FUNC_HEADER                 "bsp.h"

/** \brief Maximum log message size in number of ASCII chars (will be allocated on the task stack) */
#define NANO_IP_LOG_MAX_SIZE                    128u



/*********************************************************/
/*          Configuration of NanoIP modules              */
/*********************************************************/

/** \brief Enable socket module */
#define NANO_IP_ENABLE_SOCKET                   1u

/** \brief Maximum number of socket */
#define NANO_IP_SOCKET_MAX_COUNT                10u

/** \brief Enable socket poll() function */
#define NANO_IP_ENABLE_SOCKET_POLL              1u

/** \brief Maximum number of simultaneous calls to socket poll() function */
#define NANO_IP_SOCKET_MAX_POLL_COUNT           3u






/** \brief Enable DHCP module */
#define NANO_IP_ENABLE_DHCP                     1u

/** \brief Enable DHCP server module */
#define NANO_IP_ENABLE_DHCP_SERVER              1u

/** \brief Enable DHCP client module */
#define NANO_IP_ENABLE_DHCP_CLIENT              1u

/** \brief Rebinding time percentage of the total lease time */
#define NANO_IP_DHCP_T1_PERCENTAGE              50u

/** \brief Renewal time percentage of the total lease time */
#define NANO_IP_DHCP_T2_PERCENTAGE              90u



/** \brief Enable TFTP module */
#define NANO_IP_ENABLE_TFTP                     1u

/** \brief Enable TFTP server module */
#define NANO_IP_ENABLE_TFTP_SERVER              1u

/** \brief Create a specific task for the TFTP server module */
#define NANO_IP_ENABLE_TFTP_SERVER_TASK         1u

/** \brief Enable TFTP client module */
#define NANO_IP_ENABLE_TFTP_CLIENT              1u

/** \brief Create a specific task for the TFTP client module */
#define NANO_IP_ENABLE_TFTP_CLIENT_TASK         1u


#endif /* NANO_IP_CFG_H */
