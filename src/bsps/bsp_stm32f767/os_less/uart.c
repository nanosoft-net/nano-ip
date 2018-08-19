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

#include "uart.h"

#include "stm32f767xx.h"




/** \brief Initialize the UART driver */
void UART_Init(void)
{

    /* Enable IOs for Rx and Tx lines (PD9 and PD8, AF7)*/
    GPIOD->AFR[1u] &= ~((0x0Fu << 0u) | (0x0Fu << 4u));
    GPIOD->AFR[1u] |= ((7u << 0u) | (7u << 4u));
    GPIOD->MODER &= ~((3u << 16u) | (3u << 18u));
    GPIOD->MODER |= ((2u << 16u) | (2u << 18u));

    /* Turn on power and clock for the UART => UARTCLK = HSI = 16 MHz */
    RCC->APB1ENR &= ~(1u << 18u);
    RCC->DCKCFGR2 &= ~(3u << 4u);
    RCC->DCKCFGR2 |= (2u << 4u);
    RCC->APB1ENR |= (1u << 18u);

    /* Disable receive and transmit */
    USART3->CR1 = 0x00u;

    /* Configure UART baudrate = UARTCLK/USARTDIV = 16000000 / 139 => 115107bps */
    USART3->BRR = 139u;

    /* Configure transfer : 8bits, 1stop bit, no parity, no flow control */
    USART3->CR2 = 0x00u;
    USART3->CR3 = 0x00u;

    /* Enable receive and transmit */
    USART3->CR1 |= (1u << 0u) | (1u << 2u) | (1u << 3u);
}


/** \brief Send data over the UART */
void UART_Send(const uint8_t* data, uint32_t data_len)
{
    while (data_len != 0)
    {
        while (((USART3->ISR & (1u << 7u)) == 0)) /* TXE bit */
        {}

        /* Send byte */
        USART3->TDR = (*data);

        /* Next byte */
        data++;
        data_len--;
    }

    return;
}
