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

#include "lpc177x_8x.h"

/** \brief Initialize the UART driver */
void UART_Init(void)
{

    /* Enable IOs for Rx and Tx lines (P02 and P03, IO function 1)*/
    LPC_IOCON->P0_2 = 1u;
    LPC_IOCON->P0_3 = 1u;

    /* Turn on power and clock for the UART => UARTCLK = 60MHz */
    LPC_SC->PCONP |= (1u << 3u);

    /* Disable receive and transmit */
    LPC_UART0->FCR = 0x00u;
    LPC_UART0->TER = 0x00u;

    /* Configure UART baudrate = PCLK/(16*(256*DLM + DLL)*(1+Div/Mul)) => 115131bps */
    LPC_UART0->LCR = (1u << 7u);
    LPC_UART0->DLM = 0x00u;
    LPC_UART0->DLL = 0x13u;
    LPC_UART0->FDR = 0x05u + (0x07u << 4u); /* Div = 5, Mul = 7 */

    /* Configure transfer : 8bits, 1stop bit, no parity, no flow control */
    LPC_UART0->LCR = (3u << 0u);


    /* Reset and configure FIFOs */
    LPC_UART0->FCR = (1u << 0u) | (1u << 1u) | (1u << 2u) | (3u << 6u);

    /* Enable transmit */
    LPC_UART0->TER = (1u << 7u);
}


/** \brief Send data over the UART */
void UART_Send(const uint8_t* data, uint32_t data_len)
{
    while (data_len != 0)
    {
        while((LPC_UART0->LSR & (1u << 5u)) == 0) /* THRE bit */
        {}

        /* Send byte */
        LPC_UART0->THR = (*data);

        /* Next byte */
        data++;
        data_len--;
    }

    return;
}
