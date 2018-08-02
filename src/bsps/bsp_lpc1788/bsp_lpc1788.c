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

#include "bsp.h"
#include "lpc177x_8x.h"
#include "uart.h"
#include "nano_ip_tools.h"
#include "nano_ip_lpc_emac.h"
#include "nano_ip_big_small_packet_allocator.h"
#include "nano_ip_generic_phy.h"





/** \brief Buffer sizes for the packet allocator */
#define BIG_BUFFER_SIZE         1536u
#define BIG_BUFFERS_COUNT       10u
#define SMALL_BUFFER_SIZE       256u
#define SMALL_BUFFERS_COUNT     10u

/** \brief Packet allocator instanciation */
STATIC_INSTANCE_BIG_SMALL_PACKET_ALLOCATOR(BIG_BUFFER_SIZE, BIG_BUFFERS_COUNT, SMALL_BUFFER_SIZE, SMALL_BUFFERS_COUNT);

/** \brief LPC EMAC interrupt handler */
static fp_lpc_emac_interrupt_handler_t s_lpc_emac_interrupt_handler;

/** \brief LPC EMAC driver data */
static void* s_lpc_emac_driver_data;



/** \brief Initialize the operating system */
bool NANO_IP_BSP_OSInit()
{
    /* Initialize UART */
    UART_Init();

    return true;
}

/** \brief Start the operating system (should never return on success) */
bool NANO_IP_BSP_OSStart()
{
    /* Schedule tasks */
    while(true)
    {
        NANO_IP_OAL_TASK_Execute();
    }

    return false;
}


/** \brief Instanciate the packet allocator */
bool NANO_IP_BSP_CreatePacketAllocator(nano_ip_net_packet_allocator_t* const packet_allocator)
{
    nano_ip_error_t err;

    BIG_SMALL_PACKET_ALLOCATOR_INIT(BIG_BUFFER_SIZE, BIG_BUFFERS_COUNT, SMALL_BUFFER_SIZE, SMALL_BUFFERS_COUNT);
    err = NANO_IP_BIG_SMALL_PACKET_ALLOCATOR_Init(packet_allocator, &big_small_packet_allocator_data);

    return (err == NIP_ERR_SUCCESS);
}

/** \brief Instanciate the network interface */
bool NANO_IP_BSP_CreateNetIf(nano_ip_net_if_t* const net_if, const char** name, uint32_t* const rx_packet_count, uint32_t* const rx_packet_size)
{
    const nano_ip_lpc_emac_config_t config = {
                                                LPC_EMAC_BASE, /* Base address of LPC EMAC register */
                                                true, /* Indicate if RMII must be use instead of MII */
                                                NANO_IP_GENERIC_PHY_GetDriver(), /* PHY driver to use */
                                                1, /* PHY address */
                                                SPEED_100, /* Network speed */
                                                DUP_AUTO /* Duplex */
                                             };
    const nano_ip_error_t err = NANO_IP_LPC_EMAC_Init(net_if, &s_lpc_emac_interrupt_handler, &s_lpc_emac_driver_data, &config);
    (*name) = "lpc_emac0";
    (*rx_packet_count) = BIG_BUFFERS_COUNT - 2u;
    (*rx_packet_size) = BIG_BUFFER_SIZE;
    return (err == NIP_ERR_SUCCESS);
}


/** \brief Log output function */
void NANO_IP_BSP_Printf(const char* format, va_list arg_list)
{
    static char tmp_buffer[256u];
    int len = NANO_IP_vsnprintf(tmp_buffer, sizeof(tmp_buffer), format, arg_list);
    UART_Send(NANO_IP_CAST(uint8_t*, tmp_buffer), NANO_IP_CAST(uint32_t, len));    
}


/** \brief LPC EMAC interrupt handler */
void Ethernet_Handler(void)
{
    s_lpc_emac_interrupt_handler(s_lpc_emac_driver_data);
}

