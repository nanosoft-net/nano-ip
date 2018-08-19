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



/** \brief Create a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Create(oal_timer_t* const timer, const fp_timer_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && (callback != NULL))
    {
        const nano_os_error_t os_err = NANO_OS_TIMER_Create(timer, NANO_IP_CAST(fp_nano_os_timer_callback_func_t, callback), user_data);
        if (os_err == NOS_ERR_SUCCESS)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
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
        const nano_os_error_t os_err = NANO_OS_TIMER_Start(timer, NANO_OS_MS_TO_TICKS(period), NANO_OS_MS_TO_TICKS(period));
        if (os_err == NOS_ERR_SUCCESS)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
        }
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
        const nano_os_error_t os_err = NANO_OS_TIMER_Stop(timer);
        if (os_err == NOS_ERR_SUCCESS)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
        }
    }

    return ret;
}
