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

#include "nano_ip_oal_time.h"


/** \brief Get the current value of the millisecond tick counter */
uint32_t NANO_IP_OAL_TIME_GetMsCounter(void)
{
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
    return (NANO_IP_CAST(uint32_t, tp.tv_sec) * 1000u + NANO_IP_CAST(uint32_t, tp.tv_nsec) / 1000000u);
}
