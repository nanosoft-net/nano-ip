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

#include "systick.h"

#include "chip_lpc43xx.h"


/** \brief Systick counter */
static uint32_t s_sytick_counter;


/** \brief Initialize and start the system tick */
void SYSTICK_Init(void)
{
    /* Configure and start systick */
    static const uint32_t systick_input_freq_hz = 204000000u; /* 204 MHz */
    static const uint32_t systick_rate_hz = 1000u; /* 1 kHz = each millisecond */
    SysTick->LOAD = (systick_input_freq_hz / systick_rate_hz) - 1u;
    SysTick->CTRL = 0x07u;
    s_sytick_counter = 0u;
}

/** \brief Retrieve the current systick counter value */
uint32_t SYSTICK_GetCounter(void)
{
    return s_sytick_counter;
}

/** \brief Systick interrupt handler */
void Systick_Handler(void)
{
    s_sytick_counter++;
}
