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

/** \brief Windows' timer standard callback function */
static void CALLBACK NANO_IP_OAL_TIMER_Callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER tim);


/** \brief Create a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Create(oal_timer_t* const timer, const fp_timer_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && (callback != NULL))
    {
        /* Create timer */
        timer->timer = CreateThreadpoolTimer(NANO_IP_OAL_TIMER_Callback, timer, NULL);
        if (timer->timer != NULL)
        {
            /* Save user data */
            timer->callback = callback;
            timer->user_data = user_data;
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_RESOURCE;
        }
    }

    return ret;
}

/** \brief Start a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Start(oal_timer_t* const timer, const uint32_t period)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && (period != 0u))
    {
        LONGLONG due_time_long = -10000 * NANO_IP_CAST(LONGLONG, period);
        SetThreadpoolTimer(timer->timer, NANO_IP_CAST(FILETIME*, &due_time_long), period, 0);

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Stop a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Stop(oal_timer_t* const timer)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (timer != NULL)
    {
        SetThreadpoolTimer(timer->timer, NULL, 0, 0);
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Windows' timer standard callback function */
static void CALLBACK NANO_IP_OAL_TIMER_Callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER tim)
{
    oal_timer_t* const timer = NANO_IP_CAST(oal_timer_t*, context);
    fp_timer_callback_t callback = NANO_IP_CAST(fp_timer_callback_t, timer->callback);
    callback(timer, timer->user_data);
}
