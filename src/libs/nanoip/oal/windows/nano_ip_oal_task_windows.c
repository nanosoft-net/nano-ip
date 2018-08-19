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

/** \brief Create a task */
nano_ip_error_t NANO_IP_OAL_TASK_Create(oal_task_t* const task, const char* name, 
                                        void (*task_func)(void*), void* const param, 
                                        const uint8_t priority, const uint32_t stack_size)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    (void)name;
	(void)priority;
	(void)stack_size;

    /* Check parameters */
    if ((task != NULL) && (task_func != NULL))
    {
        (*task) = CreateThread(NULL, 0, NANO_IP_CAST(LPTHREAD_START_ROUTINE, task_func), param, 0, NULL);
        if ((*task) != INVALID_HANDLE_VALUE)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_RESOURCE;
        }
    }

    return ret;
}
