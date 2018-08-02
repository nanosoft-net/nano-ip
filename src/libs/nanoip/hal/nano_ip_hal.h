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

#ifndef NANO_IP_HAL_H
#define NANO_IP_HAL_H

#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_net_if.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Initialize the hardware abstraction layer */
nano_ip_error_t NANO_IP_HAL_Init(void);

/** \brief Get the current value of the millisecond tick counter */
uint32_t NANO_IP_HAL_GetMsCounter(void);

/** \brief Initialize the clocks of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfClock(nano_ip_net_if_t* const net_if);

/** \brief Initialize the IOs of a network interface */
nano_ip_error_t NANO_IP_HAL_InitializeNetIfIo(nano_ip_net_if_t* const net_if);

/** \brief Enable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_EnableNetIfInterrupts(nano_ip_net_if_t* const net_if);

/** \brief Disable the interrupts for a network interface */
nano_ip_error_t NANO_IP_HAL_DisableNetIfInterrupts(nano_ip_net_if_t* const net_if);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_HAL_H */
