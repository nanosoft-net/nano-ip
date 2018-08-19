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

#include "nano_ip_oal_mutex.h"

/** \brief Create a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Create(oal_mutex_t* const mutex)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (mutex != NULL)
    {
        const nano_os_error_t os_err = NANO_OS_MUTEX_Create(mutex, QT_PRIORITY);
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

/** \brief Lock a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Lock(oal_mutex_t* const mutex)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (mutex != NULL)
    {
        const nano_os_error_t os_err = NANO_OS_MUTEX_Lock(mutex, 0xFFFFFFFFu);
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

/** \brief Unlock a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Unlock(oal_mutex_t* const mutex)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (mutex != NULL)
    {
        const nano_os_error_t os_err = NANO_OS_MUTEX_Unlock(mutex);
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
