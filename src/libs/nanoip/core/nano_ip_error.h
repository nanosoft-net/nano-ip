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

#ifndef NANO_IP_ERRORS_H
#define NANO_IP_ERRORS_H


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Nano IP error codes */
typedef enum _nano_ip_error_t
{
    /** \brief Success */
    NIP_ERR_SUCCESS = 0,
    /** \brief Generic error */
    NIP_ERR_FAILURE = 1,
    /** \brief Invalid argument */
    NIP_ERR_INVALID_ARG = 2,
    /** \brief Timeout */
    NIP_ERR_TIMEOUT = 3,
    /** \brief Operation in progress */
    NIP_ERR_IN_PROGRESS = 4,
    /** \brief Handle is busy */
    NIP_ERR_BUSY = 5,
    /** \brief No more resources left */
    NIP_ERR_RESOURCE = 6,
    /** \brief Packet is too short to be handled */
    NIP_ERR_PACKET_TOO_SHORT = 7,
    /** \brief Packet is too big to be handled */
    NIP_ERR_PACKET_TOO_BIG = 8,
    /** \brief Packet not found */
    NIP_ERR_PACKET_NOT_FOUND = 9,
    /** \brief Route not found */
    NIP_ERR_ROUTE_NOT_FOUND = 10,
    /** \brief Network interface not found */
    NIP_ERR_NETIF_NOT_FOUND = 11,
    /** \brief Invalid CRC */
    NIP_ERR_INVALID_CRC = 12,
    /** \brief Protocol not found */
    NIP_ERR_PROTOCOL_NOT_FOUND = 13,
    /** \brief Packet is not for us, ignore it */
    NIP_ERR_IGNORE_PACKET = 14,
    /** \brief Packet size is invalid */
    NIPP_ERR_INVALID_PACKET_SIZE = 15,
    /** \brief Invalid ARP frame */
    NIP_ERR_INVALID_ARP_FRAME = 16,
    /** \brief Invalid ARP request */
    NIP_ERR_INVALID_ARP_REQUEST = 17,
    /** \brief ARP failure */
    NIP_ERR_ARP_FAILURE = 18,
    /** \brief Invalid checksum */
    NIP_ERR_INVALID_CS = 19,
    /** \brief Invalid ping request */
    NIP_ERR_INVALID_PING_REQUEST = 20,
    /** \brief Address is already in use */
    NIP_ERR_ADDRESS_IN_USE = 21,
    /** \brief Buffer is to small to copy received data */
    NIP_ERR_BUFFER_TOO_SMALL = 22,
    /** \brief Invalid TCP state for this operation */
    NIP_ERR_INVALID_TCP_STATE = 23,
    /** \brief Connection has been reset by peer */
    NIP_ERR_CONN_RESET = 24
} nano_ip_error_t;


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_ERRORS_H */
