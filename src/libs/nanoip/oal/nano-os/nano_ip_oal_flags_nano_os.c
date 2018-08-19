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
        const nano_os_error_t os_err = NANO_OS_FLAG_SET_Create(flags, 0u, QT_PRIORITY);
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

/** \brief Destroy a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Destroy(oal_flags_t* const flags)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (flags != NULL)
    {
        const nano_os_error_t os_err = NANO_OS_FLAG_SET_Destroy(flags);
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

/** \brief Reset a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Reset(oal_flags_t* const flags, const uint32_t flag_mask)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (flags != NULL)
    {
        const nano_os_error_t os_err = NANO_OS_FLAG_SET_Clear(flags, 0xFFFFFFFFu);
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

/** \brief Set a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Set(oal_flags_t* const flags, const uint32_t flag_mask, const bool from_isr) 
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (flags != NULL)
    {
        nano_os_error_t os_err;
        if (from_isr)
        {
            os_err = NANO_OS_FLAG_SET_SetFromIsr(flags, flag_mask);
        }
        else
        {
            os_err = NANO_OS_FLAG_SET_Set(flags, flag_mask);
        }
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

/** \brief Wait for a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Wait(oal_flags_t* const flags, uint32_t* const flag_mask, const bool reset_flags, const uint32_t timeout) 
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((flags != NULL) && (flag_mask != NULL))
    {
        const uint32_t os_timeout = (timeout == NANO_IP_MAX_TIMEOUT_VALUE) ? NANO_IP_MAX_TIMEOUT_VALUE : NANO_OS_MS_TO_TICKS(timeout);
        const nano_os_error_t os_err = NANO_OS_FLAG_SET_Wait(flags, *flag_mask, false, reset_flags, flag_mask, os_timeout);
        if (os_err == NOS_ERR_SUCCESS)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else if (os_err == NOS_ERR_TIMEOUT)
        {
            ret = NIP_ERR_TIMEOUT;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
        }
    }

    return ret;
}
