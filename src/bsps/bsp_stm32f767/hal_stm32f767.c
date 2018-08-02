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
#include "stm32f767xx.h"

/** \brief Initialize the hardware abstraction layer */
nano_ip_error_t NANO_IP_HAL_Init(void)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;

    /* Initialize the systick for the millisecond tick counter */
    SYSTICK_Init();

    /* Turn on power on GPIO */
    RCC->AHB1ENR |= (0x7FFu << 0u);

    /* leds 0,1,2 => PB0, PB7, PB14 */
    GPIOB->MODER &= ~((3u << 0u) | (3u << 14u) | (3u << 28u));
    GPIOB->MODER |= ((1u << 0u) | (1u << 14u) | (1u << 28u));

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

    /* Reset peripheral */
    RCC->AHB1RSTR |= (1u << 25u);

    /* Configure RMII mode */
    RCC->APB2ENR |= (1u << 14u);
    SYSCFG->PMC |= (1u << 23u);

    /* Enable clocks : ETHMACPTPEN - ETHMACEN - ETHMACTXEN - ETHMACRXEN */
    RCC->AHB1ENR |= ((1u << 25u) | (1u << 26u) | (1u << 27u) | (1u << 28u));

    /* Release peripheral reset */
    RCC->AHB1RSTR &= ~(1u << 25u);

    return NIP_ERR_SUCCESS;
}

/** \brief Initialize the IOs of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfIo(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Pins in RMII mode with AF11 :
    
        ETH _MDIO : PA2
        ETH _MDC : PC1
        ETH_RMII _REF_CLK : PA1
        ETH_RMII _CRS_DV : PA7
        ETH _RMII_TX_EN : PG11
        ETH _RMII_TXD0 : PG13
        ETH _RMII_TXD1 : PB13
        ETH_RMII_RXD0 : PC4
        ETH_RMII_RXD1 : PC5
    */
    GPIOA->AFR[0u] &= ~((0x0Fu << 4u) | (0x0Fu << 8u) | (0x0Fu << 28u));
    GPIOA->AFR[0u] |= ((11u << 4u) | (11u << 8u) | (11u << 28u));
    GPIOA->MODER &= ~((3u << 2u) | (3u << 4u) | (3u << 14u));
    GPIOA->MODER |= ((2u << 2u) | (2u << 4u) | (2u << 14u));

    GPIOB->AFR[1u] &= ~(0x0Fu << 20u);
    GPIOB->AFR[1u] |= (11u << 20u);
    GPIOB->MODER &= ~(3u << 26u);
    GPIOB->MODER |= (2u << 26u);

    GPIOC->AFR[0u] &= ~((0x0Fu << 4u) | (0x0Fu << 16u) | (0x0Fu << 20u));
    GPIOC->AFR[0u] |= ((11u << 4u) | (11u << 16u) | (11u << 20u));
    GPIOC->MODER &= ~((3u << 2u) | (3u << 8u) | (3u << 10u));
    GPIOC->MODER |= ((2u << 2u) | (2u << 8u) | (2u << 10u));

    GPIOG->AFR[1u] &= ~((0x0Fu << 12u) | (0x0Fu << 20u));
    GPIOG->AFR[1u] |= ((11u << 12u) | (11u << 20u));
    GPIOG->MODER &= ~((3u << 22u) | (3u << 26u));
    GPIOG->MODER |= ((2u << 22u) | (2u << 26u));

    return NIP_ERR_SUCCESS;
}

/** \brief Enable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_EnableNetIfInterrupts(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Enable ethernet interrupts into NVIC */
    NVIC_EnableIRQ(ETH_IRQn);

    return NIP_ERR_SUCCESS;
}

/** \brief Disable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_DisableNetIfInterrupts(nano_ip_net_if_t* const net_if)
{
    (void)net_if;

    /* Disable ethernet interrupts into NVIC */
    NVIC_DisableIRQ(ETH_IRQn);

    return NIP_ERR_SUCCESS;
}
