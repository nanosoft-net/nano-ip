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

#ifndef NANO_IP_OAL_FLAGS_H
#define NANO_IP_OAL_FLAGS_H

#include "nano_ip_types.h"
#include "nano_ip_error.h"
#include "nano_ip_oal_types.h"


/** \brief All flags are selected */
#define NANO_IP_OAL_FLAGS_ALL       0xFFFFFFFFu

/** \brief No flags are selected */
#define NANO_IP_OAL_FLAGS_NONE      0x00000000u


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Create a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Create(oal_flags_t* const flags);

/** \brief Destroy a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Destroy(oal_flags_t* const flags);

/** \brief Reset a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Reset(oal_flags_t* const flags, const uint32_t flag_mask);

/** \brief Set a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Set(oal_flags_t* const flags, const uint32_t flag_mask, const bool from_isr);

/** \brief Wait for a synchronization flag */
nano_ip_error_t NANO_IP_OAL_FLAGS_Wait(oal_flags_t* const flags, uint32_t* const flag_mask, const bool reset_flags, const uint32_t timeout);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* NANO_IP_OAL_FLAGS_H */
