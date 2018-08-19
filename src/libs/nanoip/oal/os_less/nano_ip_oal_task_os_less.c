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

/** \brief List of all registered tasks */
static oal_task_t* s_task_list = NULL;


/** \brief Create a task */
nano_ip_error_t NANO_IP_OAL_TASK_Create(oal_task_t* const task, const char* name, 
                                        void (*task_func)(void*), void* const param, 
                                        const uint8_t priority, const uint32_t stack_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
	(void)priority;
	(void)stack_size;

    /* Check parameters */
    if ((task != NULL) && (task_func != NULL))
    {
        /* Fill task handle */
        (void)NANO_IP_MEMSET(task, 0, sizeof(oal_task_t));
        task->name = name;
        task->task_func = task_func;
        task->param = param;

        /* Add task to the list */
        task->next = s_task_list;
        s_task_list = task;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Execute the registered tasks */
void NANO_IP_OAL_TASK_Execute(void)
{
    oal_task_t* task = s_task_list;
    while (task != NULL)
    {
        task->task_func(task->param);
        task = task->next;
    }
}
