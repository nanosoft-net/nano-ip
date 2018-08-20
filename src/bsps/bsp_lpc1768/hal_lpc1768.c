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
#include "LPC17xx.h"

#ifdef NANO_IP_OAL_OS_LESS
#include "systick.h"
#endif /* NANO_IP_OAL_OS_LESS */

/** \brief Initialize the hardware abstraction layer */
nano_ip_error_t NANO_IP_HAL_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    #ifdef NANO_IP_OAL_OS_LESS
    /* Initialize the systick for the millisecond tick counter */
    SYSTICK_Init();
    #endif /* NANO_IP_OAL_OS_LESS */

    /* Turn on power on GPIO */
    LPC_SC->PCONP |= (1u << 15u);

    return ret;
}

#ifdef NANO_IP_OAL_OS_LESS
/** \brief Get the current value of the millisecond tick counter */
uint32_t NANO_IP_HAL_GetMsCounter(void)
{
    return SYSTICK_GetCounter();
}
#endif /* NANO_IP_OAL_OS_LESS */

/** \brief Initialize the clocks of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfClock(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Enable clock */
    LPC_SC->PCONP |= (1u << 30u);

    return NIP_ERR_SUCCESS;
}

/** \brief Initialize the IOs of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfIo(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Pins in RMII mode :
    
        ETH _MDIO : P1_17  FUNC1
        ETH _MDC : P1_16  FUNC1
        ETH_RMII _REF_CLK : P1_15  FUNC1
        ETH_RMII _CRS_DV : P1_8  FUNC1
        ETH _RMII_TX_EN : P1_4  FUNC1
        ETH _RMII_TXD0 : P1_0  FUNC1
        ETH _RMII_TXD1 : P1_1  FUNC1
        ETH_RMII_RXD0 : P1_9  FUNC1
        ETH_RMII_RXD1 : P1_10  FUNC1
    */
    LPC_PINCON->PINSEL2 &= ~((0x03 << 0u) | (0x03 << 2u) | (0x03 << 8u) | (0x03 << 16u) | (0x03 << 18u) | (0x03 << 20u) | (0x03 << 30u));
    LPC_PINCON->PINSEL2 |= ((0x01 << 0u) | (0x01 << 2u) | (0x01 << 8u) | (0x01 << 16u) | (0x01 << 18u) | (0x01 << 20u) | (0x01 << 30u));
    LPC_PINCON->PINSEL3 &= ~((0x03 << 0u) | (0x03 << 2u));
    LPC_PINCON->PINSEL3 |= ((0x01 << 0u) | (0x01 << 2u));

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
