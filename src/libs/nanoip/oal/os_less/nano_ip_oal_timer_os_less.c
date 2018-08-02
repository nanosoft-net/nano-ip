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

#include "nano_ip_oal_timer.h"
#include "nano_ip_oal_time.h"
#include "nano_ip_oal_task.h"
#include "nano_ip_tools.h"



/** \brief Timer list */
static oal_timer_t* s_timer_list = NULL;

/** \brief Timer task */
static oal_task_t s_timer_task;

/** \brief Indicate if the timer task has been created */
static bool s_timer_task_created = false;



/** \brief Timer task function */
static void NANO_IP_OAL_TIMER_OS_LESS_Task(void* const unused);



/** \brief Create a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Create(oal_timer_t* const timer, const fp_timer_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && (callback != NULL))
    {
        /* Fill timer handle */
        (void)MEMSET(timer, 0, sizeof(oal_timer_t));
        timer->callback = callback;
        timer->user_data = user_data;

        /* Create timer task */
        if (!s_timer_task_created)
        {
            ret = NANO_IP_OAL_TASK_Create(&s_timer_task, "TIMER task", NANO_IP_OAL_TIMER_OS_LESS_Task, NULL);
            if (ret == NIP_ERR_SUCCESS)
            {
                s_timer_task_created = true;
            }
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}

/** \brief Start a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Start(oal_timer_t* const timer, const uint32_t period)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && 
        (timer->period == 0u) &&
        (timer->due_time == 0u) &&
        (period != 0u))
    {
        /* Compute due time */
        timer->period = period;
        timer->due_time = NANO_IP_OAL_TIME_GetMsCounter() + period;

        /* Add the timer to the list */
        timer->next = s_timer_list;
        s_timer_list = timer;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Stop a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Stop(oal_timer_t* const timer)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && 
        (timer->period != 0u))
    {
        /* Remove the timer from the list */
        oal_timer_t* previous_timer = NULL;
        oal_timer_t* current_timer = s_timer_list;
        while ((current_timer != NULL) && (current_timer != timer))
        {
            previous_timer = current_timer;
            current_timer = current_timer->next;
        }
        if (current_timer != NULL)
        {
            if (previous_timer != NULL)
            {
                previous_timer->next = timer->next;
            }
            else
            {
                s_timer_list = timer->next;
            }
            timer->period = 0u;
            timer->due_time = 0u;
            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}


/** \brief Timer task function */
static void NANO_IP_OAL_TIMER_OS_LESS_Task(void* const unused)
{
    uint32_t current_time;
    oal_timer_t* current_timer;
    
    (void)unused;

    /* Check all the timers */
    current_timer = s_timer_list;
    current_time = NANO_IP_OAL_TIME_GetMsCounter();
    while (current_timer != NULL)
    {
        if (current_timer->due_time == current_time)
        {
            /* Call timer callback and reset due time */
            fp_timer_callback_t callback = NANO_IP_CAST(fp_timer_callback_t, current_timer->callback);
            callback(current_timer, current_timer->user_data);
            current_timer->due_time += current_timer->period;
        }

        current_timer = current_timer->next;
    }
}