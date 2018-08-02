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

#include "nano_ip_oal_flags.h"


/** \brief Create a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Create(oal_flags_t* const flags)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (flags != NULL)
    {
    	ret = NIP_ERR_RESOURCE;
        if (pthread_mutex_init(&flags->mutex, NULL) == 0)
        {
        	if (pthread_cond_init(&flags->cond_var, NULL) == 0)
        	{
        		flags->flags = 0u;
				ret = NIP_ERR_SUCCESS;
        	}
        }
    }

    return ret;
}

/** \brief Destroy a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Destroy(oal_flags_t* const flags)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (flags != NULL)
    {
    	pthread_mutex_destroy(&flags->mutex);
        pthread_cond_destroy(&flags->cond_var);
        flags->flags = 0u;
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Reset a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Reset(oal_flags_t* const flags, const uint32_t flag_mask)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (flags != NULL)
    {
		pthread_mutex_lock(&flags->mutex);
        flags->flags &= ~(flag_mask);
        pthread_mutex_unlock(&flags->mutex);
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Set a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Set(oal_flags_t* const flags, const uint32_t flag_mask, const bool from_isr) 
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    (void)from_isr;

    /* Check parameters */
    if (flags != NULL)
    {
    	pthread_mutex_lock(&flags->mutex);
        flags->flags |= flag_mask;
        pthread_cond_broadcast(&flags->cond_var);
        pthread_mutex_unlock(&flags->mutex);
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Wait for a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Wait(oal_flags_t* const flags, uint32_t* const flag_mask, const bool reset_flags, const uint32_t timeout) 
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((flags != NULL) && (flag_mask != NULL))
    {
        struct timespec abstimeout;
        clock_gettime(CLOCK_REALTIME, &abstimeout);
        abstimeout.tv_sec += NANO_IP_CAST(time_t, timeout / 1000u);
        abstimeout.tv_nsec += NANO_IP_CAST(long, (timeout % 1000u) * 1000000u);
        abstimeout.tv_sec += abstimeout.tv_nsec / 1000000;
        abstimeout.tv_nsec = abstimeout.tv_nsec % 1000000;

    	pthread_mutex_lock(&flags->mutex);
        uint32_t active_flags;
        do
        {
            active_flags = flags->flags & (*flag_mask);
            if (active_flags == 0u) 
            {
                if (pthread_cond_timedwait(&flags->cond_var, &flags->mutex, &abstimeout) == 0)
                {
                    ret = NIP_ERR_SUCCESS;
                }
                else
                {
                    ret = NIP_ERR_TIMEOUT;
                }
            }
            else
            {
                ret = NIP_ERR_SUCCESS;
            }
        }
        while ((ret == NIP_ERR_SUCCESS) && (active_flags == 0u));
        if (ret == NIP_ERR_SUCCESS)
        {
            if (reset_flags)
            {
                flags->flags &= ~(active_flags);
            }
        }
        (*flag_mask) = active_flags;
        pthread_mutex_unlock(&flags->mutex);
    }

    return ret;
}
