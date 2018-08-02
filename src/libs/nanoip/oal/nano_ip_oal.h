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

#ifndef NANO_IP_OAL_H
#define NANO_IP_OAL_H

#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_oal_task.h"
#include "nano_ip_oal_mutex.h"
#include "nano_ip_oal_flags.h"
#include "nano_ip_oal_time.h"
#include "nano_ip_oal_timer.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Initialize the OS abstraction layer */
nano_ip_error_t NANO_IP_OAL_Init(void);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_OAL_H */
