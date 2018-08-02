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
#include "nano_ip_tools.h"

#include <signal.h>

/** \brief Windows' timer standard callback function */
static void NANO_IP_OAL_TIMER_Callback(sigval_t sigval);


/** \brief Create a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Create(oal_timer_t* const timer, const fp_timer_callback_t callback, void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((timer != NULL) && (callback != NULL))
    {
        /* Create timer */
    	struct sigevent sigev;
    	NANO_IP_memset(&sigev, 0, sizeof(sigev));

    	sigev.sigev_notify = SIGEV_THREAD;
    	sigev.sigev_value.sival_ptr = timer;
    	sigev._sigev_un._sigev_thread._function = NANO_IP_OAL_TIMER_Callback;

    	if (timer_create(CLOCK_MONOTONIC, &sigev, &timer->timer) == 0)
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
    	struct itimerspec timerspec;
    	timerspec.it_value.tv_sec = NANO_IP_CAST(time_t, period / 1000u);
    	timerspec.it_value.tv_nsec = NANO_IP_CAST(long, (period % 1000u) * 1000000u);
    	timerspec.it_interval.tv_sec = timerspec.it_value.tv_sec;
    	timerspec.it_interval.tv_nsec = timerspec.it_value.tv_nsec;

    	if (timer_settime(timer->timer, 0, &timerspec, NULL) == 0)
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

/** \brief Stop a timer */
nano_ip_error_t NANO_IP_OAL_TIMER_Stop(oal_timer_t* const timer)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (timer != NULL)
    {
    	struct itimerspec timerspec;
    	NANO_IP_memset(&timerspec, 0, sizeof(timerspec));

		if (timer_settime(timer->timer, 0, &timerspec, NULL) == 0)
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

/** \brief Windows' timer standard callback function */
static void NANO_IP_OAL_TIMER_Callback(sigval_t sigval)
{
    oal_timer_t* const timer = NANO_IP_CAST(oal_timer_t*, sigval.sival_ptr);
    fp_timer_callback_t callback = NANO_IP_CAST(fp_timer_callback_t, timer->callback);
    callback(timer, timer->user_data);
}
