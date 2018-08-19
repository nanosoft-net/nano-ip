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

#ifndef NANO_IP_OAL_TYPES_H
#define NANO_IP_OAL_TYPES_H

/*

    OS abstraction layer types for Nano-OS

*/

#include "nano_os_api.h"


/** \brief Task */
typedef nano_os_task_t oal_task_t;

/** \brief Mutex */
typedef nano_os_mutex_t oal_mutex_t;

/** \brief Flags */
typedef nano_os_flag_set_t oal_flags_t;

/** \brief Timer */
typedef nano_os_timer_t oal_timer_t;

/** \brief Maximum timeout value */
#define NANO_IP_MAX_TIMEOUT_VALUE   0xFFFFFFFFu



#endif /* NANO_IP_OAL_TYPES_H */
