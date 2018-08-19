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
#include "chip_lpc43xx.h"

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
    LPC_CCU1->CLKCCU[CLK_MX_SCU].CFG = (1u << 0u);
    LPC_CCU1->CLKCCU[CLK_MX_GPIO].CFG = (1u << 0u);

    /* Leds 0,1,2 =>
     * P6_9 : GPIO3[5]
     * P6_11 : GPIO3[7]
     * P2_7 : GPIO0[7] */
    LPC_SCU->SFSP[3u][5u] = (1u << 7u);
    LPC_SCU->SFSP[3u][7u] = (1u << 7u);
    LPC_SCU->SFSP[0u][7u] = (1u << 7u);
    LPC_GPIO_PORT->DIR[3u] |= (1u << 5u) | (1u << 7u);
    LPC_GPIO_PORT->DIR[0u] |= (1u << 7u);

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

    /* Configure RMII mode */
    LPC_CCU1->CLKCCU[CLK_MX_CREG].CFG = (1u << 0u);
    LPC_CREG->CREG6 &= ~(0x03u << 0u);
    LPC_CREG->CREG6 |= (0x04u << 0u);

    /* Enable clock */
    LPC_CGU->BASE_CLK[CLK_BASE_PHY_RX] = (1u << 11u) | (3u << 24u);
    LPC_CGU->BASE_CLK[CLK_BASE_PHY_TX] = (1u << 11u) | (3u << 24u);
    LPC_CCU1->CLKCCU[CLK_MX_ETHERNET].CFG = (1u << 0u) | (1u << 1u) | (1u << 2u);

    /* Reset peripheral */
    LPC_RGU->RESET_CTRL[0] = (1u << 22u);

    return NIP_ERR_SUCCESS;
}

/** \brief Initialize the IOs of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfIo(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Pins in RMII mode :
    
        ETH _MDIO : P1_17  FUNC3
        ETH _MDC : P2_0  FUNC7
        ETH_RMII _REF_CLK : P1_19  FUNC0
        ETH_RMII _CRS_DV : P1_16  FUNC7
        ETH _RMII_TX_EN : P0_1  FUNC6
        ETH _RMII_TXD0 : P1_18  FUNC3
        ETH _RMII_TXD1 : P1_20  FUNC3
        ETH_RMII_RXD0 : P1_15  FUNC3
        ETH_RMII_RXD1 : P0_0  FUNC2
    */
    LPC_SCU->SFSP[0u][0u] = (2u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 6u) | (1u << 7u);
    LPC_SCU->SFSP[0u][1u] = (6u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 7u);

    LPC_SCU->SFSP[1u][15u] = (3u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 6u) | (1u << 7u);
    LPC_SCU->SFSP[1u][16u] = (7u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 6u) | (1u << 7u);
    LPC_SCU->SFSP[1u][17u] = (3u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 6u) | (1u << 7u);
    LPC_SCU->SFSP[1u][18u] = (3u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 7u);
    LPC_SCU->SFSP[1u][19u] = (0u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 6u) | (1u << 7u);
    LPC_SCU->SFSP[1u][20u] = (3u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 7u);

    LPC_SCU->SFSP[2u][0u] = (7u << 0u) | (1u << 4u) | (1u << 5u) | (1u << 7u);

    return NIP_ERR_SUCCESS;
}

/** \brief Enable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_EnableNetIfInterrupts(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Enable ethernet interrupts into NVIC */
    NVIC_EnableIRQ(ETHERNET_IRQn);

    return NIP_ERR_SUCCESS;
}

/** \brief Disable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_DisableNetIfInterrupts(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Disable ethernet interrupts into NVIC */
    NVIC_DisableIRQ(ETHERNET_IRQn);

    return NIP_ERR_SUCCESS;
}
