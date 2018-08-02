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

#include "nano_ip_tftp.h"

#if( NANO_IP_ENABLE_TFTP == 1 )

#include "nano_ip_tools.h"
#include "nano_ip_packet_funcs.h"



/** \brief Send an acknowledge packet */
static bool NANO_IP_TFTP_SendAcknowledge(nano_ip_tftp_t* const tftp_module);

/** \brief Send a data packet */
static bool NANO_IP_TFTP_SendData(nano_ip_tftp_t* const tftp_module, uint16_t* const data_size);

/** \brief Send an error packet */
static bool NANO_IP_TFTP_SendError(nano_ip_tftp_t* const tftp_module, const nano_ip_tftp_error_t error);

/** \brief Handle an end of transfer */
static void NANO_IP_TFTP_EndOfTransfer(nano_ip_tftp_t* const tftp_module);

/** \brief TFTP timer callback */
static void NANO_IP_TFTP_TimerCallback(oal_timer_t* const timer, void* const user_data);





/** \brief Initialize a TFTP instance */
nano_ip_error_t NANO_IP_TFTP_Init(nano_ip_tftp_t* const tftp_module, const uint32_t listen_address, const uint16_t listen_port, const nano_ip_tftp_callbacks_t* const callbacks, void* const user_data, const uint32_t timeout)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((tftp_module != NULL) &&
        (callbacks != NULL) && (timeout >= TFTP_MIN_TIMEOUT_VALUE) && 
        ((timeout % TFTP_MIN_TIMEOUT_VALUE) == 0))
    {
        /* 0 init */
        MEMSET(tftp_module, 0, sizeof(nano_ip_tftp_t));

        /* Initialize instance */
        tftp_module->listen_address = listen_address;
        tftp_module->listen_port = listen_port;
        tftp_module->user_data = user_data;
        tftp_module->timeout = timeout;
        MEMCPY(&tftp_module->callbacks, callbacks, sizeof(nano_ip_tftp_callbacks_t));

        /* Create timer */
        ret = NANO_IP_OAL_TIMER_Create(&tftp_module->timer, NANO_IP_TFTP_TimerCallback, tftp_module);
    }

    return ret;
}

/** \brief Start a TFTP instance */
nano_ip_error_t NANO_IP_TFTP_Start(nano_ip_tftp_t* const tftp_module)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (tftp_module != NULL)
    {
        /* Initialize TFTP state */
        tftp_module->req_type = TFTP_REQ_IDLE;

        /* Bind UDP handle */
        ret = NANO_IP_UDP_Bind(&tftp_module->udp_handle, tftp_module->listen_address, tftp_module->listen_port);
    }

    return ret;
}

/** \brief Stop a TFTP instance */
nano_ip_error_t NANO_IP_TFTP_Stop(nano_ip_tftp_t* const tftp_module)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (tftp_module != NULL)
    {
        /* Unbind UDP handle */
        ret = NANO_IP_UDP_Unbind(&tftp_module->udp_handle, tftp_module->listen_address, tftp_module->listen_port);

        /* Stop timer */
        if ((ret == NIP_ERR_SUCCESS) && (tftp_module->req_type != TFTP_REQ_IDLE))
        {
            ret = NANO_IP_OAL_TIMER_Stop(&tftp_module->timer);
        }
    }

    return ret;
}


/** \brief Send a read or write TFTP request */
nano_ip_error_t NANO_IP_TFTP_SendReadWriteRequest(nano_ip_tftp_t* const tftp_module, const uint32_t server_address, const uint16_t server_port, const char* const filename, const uint16_t opcode)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    
    /* Check parameters */
    if ((tftp_module != NULL) && (filename != NULL))
    {
        /* Allocate data packet */
        nano_ip_net_packet_t* packet = NULL;
        ret = NANO_IP_UDP_AllocatePacket(&packet, TFTP_DATA_PACKET_PAYLOAD_SIZE);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Initialize block number */
            tftp_module->current_block = 0u;

            /* Save timestamp */
            tftp_module->last_rx_packet_timestamp = NANO_IP_OAL_TIME_GetMsCounter();

            /* Opcode */
            NANO_IP_PACKET_Write16bits(packet, opcode);

            /* Filename */
            NANO_IP_PACKET_WriteBuffer(packet, filename, NANO_IP_CAST(uint16_t, STRNLEN(filename, TFTP_MAX_FILENAME_SIZE)));
            NANO_IP_PACKET_Write8bits(packet, 0x00u);

            /* Mode */
            NANO_IP_PACKET_WriteBuffer(packet, TFTP_TRANSFER_MODE, TFTP_TRANSFER_MODE_STRING_SIZE);
            NANO_IP_PACKET_Write8bits(packet, 0x00u);

            /* Send packet */
            ret = NANO_IP_UDP_SendPacket(&tftp_module->udp_handle, server_address, server_port, packet);
            if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
            {
                /* Update state */
                tftp_module->req_type = opcode;

                /* Start timer */
                (void)NANO_IP_OAL_TIMER_Start(&tftp_module->timer, TFTP_MIN_TIMEOUT_VALUE);

                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                /* Release packet */
                (void)NANO_IP_UDP_ReleasePacket(packet);
            }
        }
    }
    
    return ret;
}

/** \brief Process a received read or write TFTP request */
void NANO_IP_TFTP_ProcessReadWriteRequest(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet, const uint16_t opcode)
{
    bool ret = false;

    /* Check parameters */
    if ((tftp_module != NULL) && (packet != NULL))
    {
        /* Check server state */
        if (tftp_module->req_type == TFTP_REQ_IDLE)
        {
            /* Extract filename and mode */
            nano_ip_tftp_error_t tftp_error = TFTP_ERR_SUCCESS;
            const char* const filename = NANO_IP_CAST(const char*, packet->current);

            /* Save timestamp */
            tftp_module->last_rx_packet_timestamp = NANO_IP_OAL_TIME_GetMsCounter();

            /* Look for the mode */
            while (((*packet->current) != 0) && (packet->count != 0))
            {
                packet->current++;
                packet->count--;
            }
            if (packet->count != 0)
            {
                const char* mode = NULL;

                /* Skip ending '0' */
                packet->current++;
                packet->count--;

                /* Extract mode */
                mode = NANO_IP_CAST(const char*, packet->current);
                while (((*packet->current) != 0) && (packet->count != 0))
                {
                    packet->current++;
                    packet->count--;
                }
                if (packet->count != 0)
                {
                    /* Check mode */
                    if (STRNCMP(TFTP_TRANSFER_MODE, mode, packet->count) == 0)
                    {
                        /* Start timer */
                        (void)NANO_IP_OAL_TIMER_Start(&tftp_module->timer, TFTP_MIN_TIMEOUT_VALUE);

                        /* Notify start of transfert */
                        tftp_error = tftp_module->callbacks.req_received(tftp_module->user_data, opcode, filename);
                        if (tftp_error == TFTP_ERR_SUCCESS)
                        {
                            /* Initialize transfert */
                            tftp_module->req_type = opcode;
                            tftp_module->current_block = 0;
                            tftp_module->last_error = TFTP_ERR_SUCCESS;

                            /* Send ack or first pack */
                            if (opcode == TFTP_REQ_WRITE)
                            {
                                /* Send acknowledge */
                                (void)NANO_IP_TFTP_SendAcknowledge(tftp_module);
                            }
                            else
                            {
                                /* Read data from the user application */
                                uint16_t data_size = 0;
                                ret = NANO_IP_TFTP_SendData(tftp_module, &data_size);
                                if (ret)
                                {
                                    /* Check end of transfert */
                                    if (data_size < TFTP_DATA_PACKET_PAYLOAD_SIZE)
                                    {
                                        /* End of transfert */
                                        ret = true;
                                    }
                                    else
                                    {
                                        /* Still more data */
                                        ret = false;
                                    }
                                }
                                else
                                {
                                    /* Error */
                                    tftp_error = TFTP_ERR_UNDEFINED;
                                }
                            }
                        }
                    }
                    else
                    {
                        /* Send an error frame */
                        tftp_error = TFTP_ERR_ILLEGAL_OPERATION;
                    }
                }
            }

            /* Check error */
            if (tftp_error != TFTP_ERR_SUCCESS)
            {
                /* Send an error packet */
                (void)NANO_IP_TFTP_SendError(tftp_module, tftp_error);
                ret = true;
            }
        }
    }

    if (ret)
    {
        /* End of transfer */
        NANO_IP_TFTP_EndOfTransfer(tftp_module);
    }

    return;
}

/** \brief Process a received TFTP data packet */
void NANO_IP_TFTP_ProcessDataPacket(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet, const nano_ip_tftp_req_type_t req_type)
{
    bool ret = false;

    /* Check parameters */
    if ((tftp_module != NULL) && (packet != NULL))
    {
        /* Check state */
        if (tftp_module->req_type == req_type)
        {
            /* Extract block number */
            nano_ip_tftp_error_t tftp_error = TFTP_ERR_SUCCESS;
            const uint16_t block_number = NANO_IP_PACKET_Read16bits(packet);
            tftp_module->current_block++;

            /* Save timestamp */
            tftp_module->last_rx_packet_timestamp = NANO_IP_OAL_TIME_GetMsCounter();

            /* Check block number */
            if (block_number == tftp_module->current_block)
            {
                /* Notify data received */
                tftp_error = tftp_module->callbacks.data_received(tftp_module->user_data, packet->current, packet->count);
                if (tftp_error == TFTP_ERR_SUCCESS)
                {
                    /* Send acknowledge */
                    (void)NANO_IP_TFTP_SendAcknowledge(tftp_module);

                    /* Check end of transfert */
                    if (packet->count < TFTP_DATA_PACKET_PAYLOAD_SIZE)
                    {
                        /* End of transfert */
                        ret = true;
                        tftp_module->last_error = TFTP_ERR_SUCCESS;
                    }
                }
            }
            else
            {
                /* Block number error */
                tftp_error = TFTP_ERR_UNKNOWN_TRANSFER_ID;
            }

            /* Check error */
            if (tftp_error != TFTP_ERR_SUCCESS)
            {
                /* Send an error packet */
                (void)NANO_IP_TFTP_SendError(tftp_module, tftp_error);
                ret = true;
            }
        }
    }

    if (ret)
    {
        /* End of transfer */
        NANO_IP_TFTP_EndOfTransfer(tftp_module);
    }

    return;
}

/** \brief Process a received TFTP acknowledge packet */
void NANO_IP_TFTP_ProcessAckPacket(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet, const nano_ip_tftp_req_type_t req_type)
{
    bool ret = false;

    /* Check parameters */
    if ((tftp_module != NULL) && (packet != NULL))
    {
        /* Check state */
        if (tftp_module->req_type == req_type)
        {
            /* Extract block number */
            nano_ip_tftp_error_t tftp_error = TFTP_ERR_SUCCESS;
            const uint16_t block_number = NANO_IP_PACKET_Read16bits(packet);

            /* Save timestamp */
            tftp_module->last_rx_packet_timestamp = NANO_IP_OAL_TIME_GetMsCounter();

            /* Check block number */
            if (block_number == tftp_module->current_block)
            {
                /* Send new data packet */
                uint16_t data_size = 0;
                tftp_module->current_block++;
                ret = NANO_IP_TFTP_SendData(tftp_module, &data_size);
                if (ret)
                {
                    /* Check end of transfert */
                    if (data_size < TFTP_DATA_PACKET_PAYLOAD_SIZE)
                    {
                        /* End of transfert */
                        ret = true;
                        tftp_module->last_error = TFTP_ERR_SUCCESS;
                    }
                    else
                    {
                        /* Still more data */
                        ret = false;
                    }
                }
                else
                {
                    /* Error */
                    tftp_error = TFTP_ERR_UNDEFINED;
                }
            }
            else
            {
                /* Block number error */
                tftp_error = TFTP_ERR_UNKNOWN_TRANSFER_ID;
            }

            /* Check error */
            if (tftp_error != TFTP_ERR_SUCCESS)
            {
                /* Send an error packet */
                (void)NANO_IP_TFTP_SendError(tftp_module, tftp_error);
                ret = true;
            }
        }
    }

    if (ret)
    {
        /* End of transfer */
        NANO_IP_TFTP_EndOfTransfer(tftp_module);
    }

    return;
}

/** \brief Process a received TFTP error packet */
void NANO_IP_TFTP_ProcessErrorPacket(nano_ip_tftp_t* const tftp_module, nano_ip_net_packet_t* const packet)
{
    /* Check parameters */
    if ((tftp_module != NULL) && (packet != NULL))
    {
        /* Check state */
        if (tftp_module->req_type != TFTP_REQ_IDLE)
        {
            /* Extract error number */
            const uint16_t error_number = NANO_IP_PACKET_Read16bits(packet);

            /* Save error number */
            tftp_module->last_error = NANO_IP_CAST(nano_ip_tftp_error_t, error_number);

            /* Notify user */
            tftp_module->callbacks.error_received(tftp_module->user_data, error_number, NANO_IP_CAST(const char*, packet->current));

            /* An error packet automatically triggers an end of transfer */
            NANO_IP_TFTP_EndOfTransfer(tftp_module);
        }
    }

    return;
}


/** \brief Send an acknowledge packet */
static bool NANO_IP_TFTP_SendAcknowledge(nano_ip_tftp_t* const tftp_module)
{
    bool ret = false;

    /* Allocate response packet */
    nano_ip_net_packet_t* packet = NULL;
    nano_ip_error_t err = NANO_IP_UDP_AllocatePacket(&packet, TFTP_ACK_PACKET_SIZE);
    if (err == NIP_ERR_SUCCESS)
    {
        /* Fill packet */
        NANO_IP_PACKET_Write16bits(packet, TFTP_REQ_ACK);
        NANO_IP_PACKET_Write16bits(packet, tftp_module->current_block);

        /* Send packet */
        err = NANO_IP_UDP_SendPacket(&tftp_module->udp_handle, tftp_module->dest_address, tftp_module->dest_port, packet);
        if ((err == NIP_ERR_SUCCESS) || (err == NIP_ERR_IN_PROGRESS))
        {
            ret = true;
        }
        else
        {
            /* Release packet */
            (void)NANO_IP_UDP_ReleasePacket(packet);
        }
    }

    return ret;
}

/** \brief Send a data packet */
static bool NANO_IP_TFTP_SendData(nano_ip_tftp_t* const tftp_module, uint16_t* const data_size)
{
    bool ret = false;

    /* Allocate data packet */
    nano_ip_net_packet_t* packet = NULL;
    nano_ip_error_t err = NANO_IP_UDP_AllocatePacket(&packet, TFTP_DATA_PACKET_PAYLOAD_SIZE);
    if (err == NIP_ERR_SUCCESS)
    {
    	nano_ip_tftp_error_t tftp_err;

        /* Fill packet */
        NANO_IP_PACKET_Write16bits(packet, TFTP_REQ_DATA);
        NANO_IP_PACKET_Write16bits(packet, tftp_module->current_block);

        /* Fill user data */
        tftp_err = tftp_module->callbacks.data_to_send(tftp_module->user_data, packet->current, data_size);
        if (tftp_err == TFTP_ERR_SUCCESS)
        {
            /* Send packet */
            NANO_IP_PACKET_WriteSkipBytes(packet, *data_size);
            err = NANO_IP_UDP_SendPacket(&tftp_module->udp_handle, tftp_module->dest_address, tftp_module->dest_port, packet);
            if ((err == NIP_ERR_SUCCESS) || (err == NIP_ERR_IN_PROGRESS))
            {
                ret = true;
            }
        }

        /* Release packet */
        if (!ret)
        {
            (void)NANO_IP_UDP_ReleasePacket(packet);
        }
    }

    return ret;
}

/** \brief Send an error packet */
static bool NANO_IP_TFTP_SendError(nano_ip_tftp_t* const tftp_module, const nano_ip_tftp_error_t error)
{
    bool ret = false;

    /* Allocate response packet */
    nano_ip_net_packet_t* packet = NULL;
    nano_ip_error_t err = NANO_IP_UDP_AllocatePacket(&packet, TFTP_MIN_ERROR_PACKET_SIZE);
    if (err == NIP_ERR_SUCCESS)
    {
        /* Fill packet */
        NANO_IP_PACKET_Write16bits(packet, TFTP_REQ_ERROR);
        NANO_IP_PACKET_Write16bits(packet, error);
        NANO_IP_PACKET_Write8bits(packet, 0x00u);

        /* Send packet */
        err = NANO_IP_UDP_SendPacket(&tftp_module->udp_handle, tftp_module->dest_address, tftp_module->dest_port, packet);
        if ((err == NIP_ERR_SUCCESS) || (err == NIP_ERR_IN_PROGRESS))
        {
            ret = true;
        }
        else
        {
            /* Release packet */
            (void)NANO_IP_UDP_ReleasePacket(packet);
        }
    }

    /* Save last error */
    tftp_module->last_error = error;

    return ret;
}

/** \brief Handle an end of transfer */
static void NANO_IP_TFTP_EndOfTransfer(nano_ip_tftp_t* const tftp_module)
{
    /* Reset state */
    tftp_module->req_type = TFTP_REQ_IDLE;

    /* Stop timer */
    (void)NANO_IP_OAL_TIMER_Stop(&tftp_module->timer);

    /* Notify user */
    tftp_module->callbacks.end_of_transfert(tftp_module->user_data, tftp_module->last_error);

    return;
}

/** \brief TFTP timer callback */
static void NANO_IP_TFTP_TimerCallback(oal_timer_t* const timer, void* const user_data)
{
    nano_ip_tftp_t* const tftp_module = NANO_IP_CAST(nano_ip_tftp_t*, user_data);
    (void)timer;

    /* Check parameters */
    if ((tftp_module != NULL) && (tftp_module->req_type != TFTP_REQ_IDLE))
    {
        /* Check timeout */
        const uint32_t timestamp = NANO_IP_OAL_TIME_GetMsCounter();
        if ((tftp_module->last_rx_packet_timestamp + tftp_module->timeout) <= timestamp)
        {
            /* Timeout */
            tftp_module->last_error = TFTP_ERR_TIMEOUT;

            /* End of transfer */
            NANO_IP_TFTP_EndOfTransfer(tftp_module);
        }
    }
}



#endif /* NANO_IP_ENABLE_TFTP */
