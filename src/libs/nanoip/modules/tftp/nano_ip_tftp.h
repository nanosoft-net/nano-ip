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

#ifndef NANO_IP_TFTP_H
#define NANO_IP_TFTP_H

#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_TFTP == 1 )

#include "nano_ip_types.h"
#include "nano_ip_oal.h"
#include "nano_ip_udp.h"



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Standard port used by TFTP */
#define TFTP_STANDARD_PORT       69u

/** \brief Size in bytes of the header of a TFTP data packet */
#define TFTP_DATA_PACKET_HEADER_SIZE    4u

/** \brief Size in bytes of the payload of a TFTP data packet */
#define TFTP_DATA_PACKET_PAYLOAD_SIZE   512u

/** \brief Maximum size in bytes of a TFTP data packet */
#define MAX_TFTP_DATA_PACKET_SIZE       (TFTP_DATA_PACKET_HEADER_SIZE + TFTP_DATA_PACKET_PAYLOAD_SIZE)

/** \brief Size in bytes of a TFTP acknowledge packet */
#define TFTP_ACK_PACKET_SIZE            4u

/** \brief Minimum size of a TFTP error packet */
#define TFTP_MIN_ERROR_PACKET_SIZE      5u

/** \brief Minimum timeout value in milliseconds for the TFTP operations */
#define TFTP_MIN_TIMEOUT_VALUE          100u

/** \brief TFTP transfer mode */
#define TFTP_TRANSFER_MODE              "octet"

/** \brief Size in bytes of the TFTP transfer mode string */
#define TFTP_TRANSFER_MODE_STRING_SIZE  5u

/** \brief Maximum size in bytes of a filename in a TFTP request packet 
           => max packet size - opcode - '/0' - (mode + '/0') */
#define TFTP_MAX_FILENAME_SIZE          (TFTP_DATA_PACKET_PAYLOAD_SIZE - 9u)


/** \brief TFTP request types */
typedef enum _nano_ip_tftp_req_type_t
{
    /** \brief Idle */
    TFTP_REQ_IDLE = 0u,
    /** \brief Read request */
    TFTP_REQ_READ = 1u,
    /** \brief Write request */
    TFTP_REQ_WRITE = 2u,
    /** \brief Data packet */
    TFTP_REQ_DATA = 3u,
    /** \brief Ack packet */
    TFTP_REQ_ACK = 4u,
    /** \brief Error packet */
    TFTP_REQ_ERROR = 5u
} nano_ip_tftp_req_type_t;


/** \brief TFTP error codes */
typedef enum _nano_ip_tftp_error_t
{
    /** \brief Not defined, see error message(if any) */
    TFTP_ERR_UNDEFINED = 0u,
    /** \brief File not found */
    TFTP_ERR_FILE_NOT_FOUND = 1u,
    /** \brief Access violation */
    TFTP_ERR_ACCESS_VIOLATION = 2u,
    /** \brief Disk full or allocation exceeded */
    TFTP_ERR_DISK_FULL = 3u,
    /** \brief Illegal TFTP operation */
    TFTP_ERR_ILLEGAL_OPERATION = 4u,
    /** \brief Unknown transfer ID */
    TFTP_ERR_UNKNOWN_TRANSFER_ID = 5u,
    /** \brief File already exists */
    TFTP_ERR_FILE_EXISTS = 6u,
    /** \brief No such user */
    TFTP_ERR_NO_SUCH_USER = 7u,

    /** \brief Timeout */
    TFTP_ERR_TIMEOUT = 0xFEu,
    /** \brief No error */
    TFTP_ERR_SUCCESS = 0xFFu
} nano_ip_tftp_error_t;




/** \brief Callback called when a TFTP request has been received */
typedef nano_ip_tftp_error_t(*fp_tftp_req_received_callback_t)(void* const user_data, const nano_ip_tftp_req_type_t req_type, const char* filename);

/** \brief Callback called when data has been received */
typedef nano_ip_tftp_error_t(*fp_tftp_data_received_callback_t)(void* const user_data, const uint8_t* const data, const uint16_t size);

/** \brief Callback called when data must be sent  */
typedef nano_ip_tftp_error_t(*fp_tftp_data_to_send_callback_t)(void* const user_data, uint8_t* const data, uint16_t* size);

/** \brief Callback called when an error packet has been received */
typedef void(*fp_tftp_error_received_callback_t)(void* const user_data, const uint16_t error_code, const char* error_string);

/** \brief Callback called at the end of a TFTP transfert */
typedef void(*fp_tftp_eot_callback_t)(void* const user_data, const nano_ip_tftp_error_t tftp_status);



/** \brief TFTP callbacks */
typedef struct _nano_ip_tftp_callbacks_t
{
    /** \brief Request received */
    fp_tftp_req_received_callback_t req_received;
    /** \brief Data received */
    fp_tftp_data_received_callback_t data_received;
    /** \brief Data to send */
    fp_tftp_data_to_send_callback_t data_to_send;
    /** \brief Error received */
    fp_tftp_error_received_callback_t error_received;
    /** \brief End of transfert */
    fp_tftp_eot_callback_t end_of_transfert;
} nano_ip_tftp_callbacks_t;

/** \brief TFTP module internal data */
typedef struct _nano_ip_tftp_t
{
    /** \brief UDP handle */
    nano_ip_udp_handle_t udp_handle;
    /** \brief Listen address */
    uint32_t listen_address;
    /** \brief Listen port */
    uint16_t listen_port;
    /** \brief Destination address */
    uint32_t dest_address;
    /** \brief Destination port */
    uint16_t dest_port;
    /** \brief Current block number */
    uint16_t current_block;
    /** \brief Indicate if a transfert is in progress */
    nano_ip_tftp_req_type_t req_type;
    /** \brief Timer */
    oal_timer_t timer;
    /** \brief Timestamp of the last received packet */
    uint32_t last_rx_packet_timestamp;
    /** \brief Timeout in milliseconds */
    uint32_t timeout;
    /** \brief Last error */
    nano_ip_tftp_error_t last_error;
    /** \brief Callbacks */
    nano_ip_tftp_callbacks_t callbacks;
    /** \brief User data */
    void* user_data;
} nano_ip_tftp_t;


/** \brief Initialize a TFTP instance */
nano_ip_error_t NANO_IP_TFTP_Init(nano_ip_tftp_t* const tftp_module, const uint32_t listen_address, const uint16_t listen_port, const nano_ip_tftp_callbacks_t* const callbacks, void* const user_data, const uint32_t timeout);

/** \brief Start a TFTP instance */
nano_ip_error_t NANO_IP_TFTP_Start(nano_ip_tftp_t* const tftp_module);

/** \brief Stop a TFTP instance */
nano_ip_error_t NANO_IP_TFTP_Stop(nano_ip_tftp_t* const tftp_module);


/** \brief Send a read or write TFTP request */
nano_ip_error_t NANO_IP_TFTP_SendReadWriteRequest(nano_ip_tftp_t* const tftp_module, const uint32_t server_address, const uint16_t server_port, const char* const filename, const uint16_t opcode);

/** \brief Process a received read or write TFTP request */
void NANO_IP_TFTP_ProcessReadWriteRequest(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet, const uint16_t opcode);

/** \brief Process a received TFTP data packet */
void NANO_IP_TFTP_ProcessDataPacket(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet, const nano_ip_tftp_req_type_t req_type);

/** \brief Process a received TFTP acknowledge packet */
void NANO_IP_TFTP_ProcessAckPacket(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet, const nano_ip_tftp_req_type_t req_type);

/** \brief Process a received TFTP error packet */
void NANO_IP_TFTP_ProcessErrorPacket(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_ENABLE_TFTP */

#endif /* NANO_IP_TFTP_H */
