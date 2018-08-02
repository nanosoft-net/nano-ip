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
        (*flags) = 0u;
        ret = NIP_ERR_SUCCESS;
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
        (*flags) = 0u;
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
        (*flags) &= ~(flag_mask);
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
        (*flags) |= flag_mask;
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Wait for a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Wait(oal_flags_t* const flags, uint32_t* const flag_mask, const bool reset_flags, const uint32_t timeout) 
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    (void)timeout;

    /* Check parameters */
    if ((flags != NULL) && (flag_mask != NULL))
    {
        const uint32_t active_flags = (*flags) & (*flag_mask);
        if (active_flags == 0u) 
        {
            ret = NIP_ERR_TIMEOUT;
        }
        else
        {
            if (reset_flags)
            {
                (*flags) &= ~(active_flags);
            }

            ret = NIP_ERR_SUCCESS;
        }
        (*flag_mask) = active_flags;
    }

    return ret;
}
