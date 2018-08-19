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

#include "nano_ip_oal_task.h"
#include "nano_ip_tools.h"


/** \brief Size of the memory pool for the stacks in number of stack elements */
#define NANO_IP_OAL_NANO_OS_STACK_POOL_SIZE     1024u

/** \brief Memory pool for the stacks */
static nano_os_stack_t s_nano_os_task_stack_pool[NANO_IP_OAL_NANO_OS_STACK_POOL_SIZE];

/** \brief Memory pool usage */
static uint32_t s_nano_os_task_stack_pool_used;



/** \brief Create a task */
nano_ip_error_t NANO_IP_OAL_TASK_Create(oal_task_t* const task, const char* name, 
                                        void (*task_func)(void*), void* const param, 
                                        const uint8_t priority, const uint32_t stack_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((task != NULL) && 
        (task_func != NULL) &&
        (name != NULL))
    {
        /* Allocate stack */
        const uint32_t stack_size_elem = stack_size / sizeof(nano_os_stack_t);
        if (s_nano_os_task_stack_pool_used + stack_size_elem <= NANO_IP_OAL_NANO_OS_STACK_POOL_SIZE)
        {
            nano_os_error_t os_err;
            nano_os_task_init_data_t task_init_data;
            nano_os_stack_t* stack;

            /* Allocate stack */
            stack = &s_nano_os_task_stack_pool[s_nano_os_task_stack_pool_used];
            s_nano_os_task_stack_pool_used += stack_size_elem;

            /* Fill task info */
            NANO_IP_MEMSET(&task_init_data, 0, sizeof(nano_os_task_init_data_t));
            task_init_data.name = name;
            task_init_data.base_priority = priority;
            task_init_data.stack_origin = stack;
            task_init_data.stack_size = stack_size_elem;
            task_init_data.task_func = NANO_IP_CAST(fp_nano_os_task_func_t, task_func);
            task_init_data.param = param;

            /* Create task */
            os_err = NANO_OS_TASK_Create(task, &task_init_data);
            if (os_err == NOS_ERR_SUCCESS)
            {
                ret = NIP_ERR_SUCCESS;
            }
            else
            {
                /* Release stack */
                s_nano_os_task_stack_pool_used -= stack_size_elem;
                ret = NIP_ERR_FAILURE;
            }
        }
        else
        {
            ret = NIP_ERR_RESOURCE;
        }
    }

    return ret;
}
