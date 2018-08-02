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

#ifndef SYSTICK_H
#define SYSTICK_H


#include "nano_ip_types.h"



/** \brief Initialize and start the system tick */
void SYSTICK_Init(void);

/** \brief Retrieve the current systick counter value */
uint32_t SYSTICK_GetCounter(void);


#endif /* SYSTICK_H */
