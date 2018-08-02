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

#include "nano_ip_hal.h"
#include "systick.h"
#include "lpc177x_8x.h"

/** \brief Initialize the hardware abstraction layer */
nano_ip_error_t NANO_IP_HAL_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    /* Initialize the systick for the millisecond tick counter */
    SYSTICK_Init();

    /* Turn on power on GPIO */
    LPC_SC->PCONP |= (1u << 15u);

    /* Leds 0,1 => P2-26,27 */
    LPC_IOCON->P2_26 = 0u;
    LPC_IOCON->P2_27 = 0u;
    LPC_GPIO2->DIR = (1u << 26u) | (1u << 27u);

    return ret;
}

/** \brief Get the current value of the millisecond tick counter */
uint32_t NANO_IP_HAL_GetMsCounter(void)
{
    return SYSTICK_GetCounter();
}

/** \brief Initialize the clocks of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfClock(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Enable clock */
    LPC_SC->PCONP |= (1u << 30u);

    /* Reset peripheral */
   /* LPC_SC->RSTCON0 |= (1u << 30u);
    LPC_SC->RSTCON0 &= ~(1u << 30u);
*/
    return NIP_ERR_SUCCESS;
}

/** \brief Initialize the IOs of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfIo(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Pins in RMII mode :
    
        ETH _MDIO : P1_17  IOCON1
        ETH _MDC : P1_16  IOCON1
        ETH_RMII _REF_CLK : P1_15  IOCON1
        ETH_RMII _CRS_DV : P1_8  IOCON1
        ETH _RMII_TX_EN : P1_4  IOCON1
        ETH _RMII_TXD0 : P1_0  IOCON1
        ETH _RMII_TXD1 : P1_1  IOCON1
        ETH_RMII_RXD0 : P1_9  IOCON1
        ETH_RMII_RXD1 : P1_10  IOCON1
    */
    LPC_IOCON->P1_0 &= ~(7u << 0u);
    LPC_IOCON->P1_0 |= (1u << 0u);
    LPC_IOCON->P1_1 &= ~(7u << 0u);
    LPC_IOCON->P1_1 |= (1u << 0u);
    LPC_IOCON->P1_4 &= ~(7u << 0u);
    LPC_IOCON->P1_4 |= (1u << 0u);
    LPC_IOCON->P1_8 &= ~(7u << 0u);
    LPC_IOCON->P1_8 |= (1u << 0u);
    LPC_IOCON->P1_9 &= ~(7u << 0u);
    LPC_IOCON->P1_9 |= (1u << 0u);
    LPC_IOCON->P1_10 &= ~(7u << 0u);
    LPC_IOCON->P1_10 |= (1u << 0u);
    LPC_IOCON->P1_15 &= ~(7u << 0u);
    LPC_IOCON->P1_15 |= (1u << 0u);
    LPC_IOCON->P1_16 &= ~(7u << 0u);
    LPC_IOCON->P1_16 |= (1u << 0u);
    LPC_IOCON->P1_17 &= ~(7u << 0u);
    LPC_IOCON->P1_17 |= (1u << 0u);

    return NIP_ERR_SUCCESS;
}

/** \brief Enable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_EnableNetIfInterrupts(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Enable ethernet interrupts into NVIC */
    NVIC_EnableIRQ(ENET_IRQn);

    return NIP_ERR_SUCCESS;
}

/** \brief Disable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_DisableNetIfInterrupts(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Disable ethernet interrupts into NVIC */
    NVIC_DisableIRQ(ENET_IRQn);

    return NIP_ERR_SUCCESS;
}
