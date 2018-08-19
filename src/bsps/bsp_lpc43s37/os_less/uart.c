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

#include "chip_lpc43xx.h"




/** \brief Initialize the UART driver */
void UART_Init(void)
{

    /* Enable IOs for Rx and Tx lines (P2_1 FUNC1 and P6_4 FUNC2)*/
    LPC_SCU->SFSP[6u][4u] = (2u << 0u) | (2u << 3u);
    LPC_SCU->SFSP[2u][1u] = (1u << 0u) | (2u << 3u) | (1u << 6u) | (1u << 7u);

    /* Turn on power and clock for the UART => UARTCLK = IRC clock = 12 MHz */
    LPC_CGU->BASE_CLK[CLK_BASE_UART0] = (1u << 11u) | (1u << 24u);
    LPC_CCU1->CLKCCU[CLK_MX_UART0].CFG = (1u << 0u);
    LPC_CCU2->CLKCCU[CLK_APB0_UART0 - CLK_CCU2_START].CFG = (1u << 0u);

    /* Disable receive and transmit */
    LPC_USART0->FCR = 0x00u;
    LPC_USART0->TER2 = 0x00u;

    /* Configure UART baudrate = UARTCLKPCLK/(16*(256*DLM + DLL)*(1+Div/Mul)) => 115384bps */
    /* DIVADDVAL = 5u, MULVAL = 8 => DLM = 0, DLL = 4 */
    LPC_USART0->LCR = (1u << 7u);
    LPC_USART0->DLM = 0u;
    LPC_USART0->DLL = 4u;
    LPC_USART0->FDR = (5u << 0u) | (8u << 4u);

    /* Configure transfer : 8bits, 1stop bit, no parity, no flow control */
    LPC_USART0->LCR = (3u << 0u);


    /* Reset and configure FIFOs */
    LPC_USART0->FCR = (1u << 0u) | (1u << 1u) | (1u << 2u) | (3u << 6u);

    /* Enable transmit */
    LPC_USART0->TER2 = 0x01u;
}


/** \brief Send data over the UART */
void UART_Send(const uint8_t* data, uint32_t data_len)
{
    while (data_len != 0)
    {
        while((LPC_USART0->LSR & (1u << 5u)) == 0) /* THRE bit */
        {}

        /* Send byte */
        LPC_USART0->THR = (*data);

        /* Next byte */
        data++;
        data_len--;
    }

    return;
}
