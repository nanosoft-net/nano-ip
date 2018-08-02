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
    	pthread_mutexattr_t mutex_attr;
    	pthread_mutexattr_init(&mutex_attr);
    	pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE_NP);
    	if (pthread_mutex_init(mutex, &mutex_attr) == 0)
    	{
    		ret = NIP_ERR_SUCCESS;
    	}
    	else
    	{
    		ret = NIP_ERR_RESOURCE;
    	}
    	pthread_mutexattr_destroy(&mutex_attr);
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
    	if (pthread_mutex_lock(mutex) == 0)
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

/** \brief Unlock a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Unlock(oal_mutex_t* const mutex)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (mutex != NULL)
    {
    	if (pthread_mutex_unlock(mutex) == 0)
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
