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

#ifndef NANO_IP_TYPES_H
#define NANO_IP_TYPES_H

#include "nano_ip_cfg.h"

/* Standard integer types*/
#if( USE_STD_INT == 1 )
#include <stdint.h>
#else
typedef signed char             int8_t;
typedef unsigned char           uint8_t;
typedef short int               int16_t;
typedef unsigned short int      uint16_t;
typedef long int                int32_t;
typedef unsigned long int       uint32_t;
typedef long long int           int64_t;
typedef unsigned long long int  uint64_t;
#endif /* USE_STD_INT */

/* Standard bool type definition */
#if( USE_STD_BOOL == 1 )
#include <stdbool.h>
#else
#define bool	uint8_t
#define false	0
#define true	1
#endif /* USE_STD_BOOL */

/* Standard size_t type definition */
#if( USE_STD_SIZE_T == 1 )
#include <stddef.h>
#else
typedef uint32_t size_t;
#endif /* USE_STD_SIZE_T */

/* NULL pointer */
#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void*)0)
#endif /* __cplusplus */
#endif /* NULL */


/** \brief Macro to cast a value or an expression to another type */
#define NANO_IP_CAST(type, expr) ((type)(expr))


#endif /* NANO_IP_TYPES_H */
