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


/** \brief Task */
typedef struct _oal_task_t
{
    /** \brief Name */
    const char* name;
    /** \brief Function */
    void (*task_func)(void*);
    /** \brief Parameter */
    void* param;
    /** \brief Next task */
    struct _oal_task_t* next;
} oal_task_t;

/** \brief Mutex */
typedef uint8_t oal_mutex_t;

/** \brief Flags */
typedef uint32_t oal_flags_t;

/** \brief Timer */
typedef struct _oal_timer_t
{
    /** \brief Period */
    uint32_t period;
    /** \brief Due time */
    uint32_t due_time;
    /** \brief Callback */
    void* callback;
    /** \brief User data */
    void* user_data;
    /** \brief Next timer */
    struct _oal_timer_t* next;
} oal_timer_t;

/** \brief Maximum timeout value */
#define NANO_IP_MAX_TIMEOUT_VALUE   0

/** \brief Indicate that the tasks must not contain infinite loop */
#define NANO_IP_OAL_TASK_NO_INFINITE_LOOP


#endif /* NANO_IP_OAL_TYPES_H */
