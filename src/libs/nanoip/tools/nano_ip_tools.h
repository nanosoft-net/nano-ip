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

#ifndef NANO_IP_TOOLS_H
#define NANO_IP_TOOLS_H

#include "nano_ip_cfg.h"
#include "nano_ip_types.h"

#include <stdarg.h>

#if( (USE_STD_MEMSET == 1) || (USE_STD_MEMCPY == 1) || (USE_STD_MEMCMP == 1) || \
     (USE_STD_STRNCMP == 1) || (USE_STD_STRNLEN == 1) || (USE_STD_STRNCAT == 1) )
#include <string.h>
#endif /* USE_STD_MEMSET */

#if( (USE_STD_ATOI == 1) || (USE_STD_ITOA == 1) )
#include <stdlib.h>
#endif /* USE_STD_ATOI */



#if( USE_STD_MEMSET == 1 )

/** \brief Memset macro definition */
#define NANO_IP_MEMSET(dst, val, size) memset((dst), (val), (size))

#else

/** \brief Memset macro definition */
#define NANO_IP_MEMSET(dst, val, size) NANO_IP_memset((dst), (val), (size))

#endif /* USE_STD_MEMSET */


#if( USE_STD_MEMCPY == 1 )

/** \brief Memcpy macro definition */
#define NANO_IP_MEMCPY(dst, src, size) memcpy((dst), (src), (size))

#else

/** \brief Memcpy macro definition */
#define NANO_IP_MEMCPY(dst, src, size) NANO_IP_memcpy((dst), (src), (size))

#endif /* USE_STD_MEMCPY */


#if( USE_STD_MEMCMP == 1 )

/** \brief Memcmp macro definition */
#define NANO_IP_MEMCMP(s1, s2, size) memcmp((s1), (s2), (size))

#else

/** \brief Memcmp macro definition */
#define NANO_IP_MEMCMP(s1, s2, size) NANO_IP_memcmp((s1), (s2), (size))

#endif /* USE_STD_MEMCMP */


#if( USE_STD_STRNCMP == 1 )

/** \brief Strncmp macro definition */
#define NANO_IP_STRNCMP(s1, s2, size) strncmp((s1), (s2), (size))

#else

/** \brief Strncmp macro definition */
#define NANO_IP_STRNCMP(s1, s2, size) NANO_IP_strncmp((s1), (s2), (size))

#endif /* USE_STD_STRNCMP */


#if( USE_STD_STRNLEN == 1 )

/** \brief Strnlen macro definition */
#define NANO_IP_STRNLEN(s, maxlen) strnlen((s), (maxlen))

#else

/** \brief Strnlen macro definition */
#define NANO_IP_STRNLEN(s, maxlen) NANO_IP_strnlen((s), (maxlen))

#endif /* USE_STD_STRNLEN */


#if( USE_STD_STRNCAT == 1 )

/** \brief Strncat macro definition */
#define NANO_IP_STRNCAT(dest, src, size) strncat((dest), (src), (size))

#else

/** \brief Strncat macro definition */
#define NANO_IP_STRNCAT(dest, src, size) NANO_IP_strncat((dest), (src), (size))

#endif /* USE_STD_STRNCAT */


#if( USE_STD_ATOI == 1 )

/** \brief Atoi macro definition */
#define NANO_IP_ATOI(str) atoi((str))

#else

/** \brief Atoi macro definition */
#define NANO_IP_ATOI(str) NANO_IP_atoi((str))

#endif /* USE_STD_ATOI */


#if( USE_STD_ITOA == 1 )

/** \brief Itoa macro definition */
#define NANO_IP_ITOA(value, str, base) itoa((value), (str), (base))

#else

/** \brief Itoa macro definition */
#define NANO_IP_ITOA(value, str, base) NANO_IP_itoa((value), (str), (base))

#endif /* USE_STD_ITOA */



#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Highly portable but non-efficient memset function */
void* NANO_IP_memset(void* const dst, const int val, size_t size);

/** \brief Highly portable but non-efficient memcpy function */
void* NANO_IP_memcpy(void* const dst, const void* const src, size_t size);

/** \brief Highly portable but non-efficient memcmp function */
int NANO_IP_memcmp(const void* const s1, const void* const s2, size_t size);

/** \brief Highly portable but non-efficient strncmp function */
int NANO_IP_strncmp(const char* s1, const char* s2, size_t size);

/** \brief Highly portable but non-efficient strnlen function */
size_t NANO_IP_strnlen(const char* s, size_t maxlen);

/** \brief Highly portable but non-efficient strncat function */
char* NANO_IP_strncat(char *dest, const char *src, size_t size);

/** \brief Highly portable but non-efficient vsnprintf function */
int NANO_IP_vsnprintf(char *str, size_t size, const char *format, va_list ap);

/** \brief Highly portable but non-efficient snprintf function */
int NANO_IP_snprintf(char *str, size_t size, const char *format, ...);

/** \brief Higly portable but non-efficient atoi function */
int NANO_IP_atoi(const char* str);

/** \brief Higly portable but non-efficient itoa function */
char* NANO_IP_itoa(int value, char * str, int base);



/** \brief Convert a string containing an IP address to an integer representation */
uint32_t NANO_IP_inet_ntoa(const char* addr_str);


/** \brief Compute the internet checksum of a buffer */
uint16_t NANO_IP_ComputeInternetCS(uint8_t* const pseudo_header, uint16_t pseudo_header_size, uint8_t* const buffer, uint16_t size);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_TOOLS_H */
