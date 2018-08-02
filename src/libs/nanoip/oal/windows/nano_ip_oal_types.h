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

    OS abstraction layer types for windows

*/

#include <Windows.h>


/** \brief Task */
typedef HANDLE oal_task_t;

/** \brief Mutex */
typedef CRITICAL_SECTION oal_mutex_t;

/** \brief Flags */
typedef struct _oal_flags_t
{
    /** \brief Flags */
    uint32_t flags;
    /** \brief Mutex */
    CRITICAL_SECTION mutex;
    /** \brief Condition variable */
    CONDITION_VARIABLE cond_var;
} oal_flags_t;

/** \brief Timer */
typedef struct _oal_timer_t
{
    /** \brief Timer */
    PTP_TIMER timer;
    /** \brief Callback */
    void* callback;
    /** \brief User data */
    void* user_data;
} oal_timer_t;

/** \brief Maximum timeout value */
#define NANO_IP_MAX_TIMEOUT_VALUE   INFINITE



#endif /* NANO_IP_OAL_TYPES_H */
