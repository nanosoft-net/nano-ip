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

#ifndef NANO_IP_OAL_MUTEX_H
#define NANO_IP_OAL_MUTEX_H

#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_oal_types.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Create a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Create(oal_mutex_t* const mutex);

/** \brief Lock a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Lock(oal_mutex_t* const mutex);

/** \brief Unlock a mutex */
nano_ip_error_t NANO_IP_OAL_MUTEX_Unlock(oal_mutex_t* const mutex);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_OAL_MUTEX_H */
