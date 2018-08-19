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

#include "nano_ip_log.h"

#if( NANO_IP_ENABLE_LOGS == 1 )

#include "nano_ip_tools.h"
#include "nano_ip_oal.h"

/* Include specific header for the log function */
#include NANO_IP_LOG_FUNC_HEADER

#include <stdarg.h>



/** \brief Size of the log level format string */
#define LOG_LEVEL_FORMAT_STRING_SIZE    11u

/** \brief Nano IP Log function */
void NANO_IP_LOG_Log(const char* const level, const char* const msg, ...)
{
    va_list arg_list;
    size_t timestamp_size;
    char log_buffer[NANO_IP_LOG_MAX_SIZE];

    /* Get the current time */
    const uint32_t timestamp = NANO_IP_OAL_TIME_GetMsCounter();

    /* Initialize the log buffer */
    NANO_IP_ITOA(NANO_IP_CAST(int, timestamp), log_buffer, 10);
    timestamp_size = NANO_IP_STRNLEN(log_buffer, sizeof(log_buffer));
    NANO_IP_STRNCAT(log_buffer, " - [", 5u);
    NANO_IP_STRNCAT(log_buffer, level, 5u);
    NANO_IP_STRNCAT(log_buffer, "] ", 3u);
    NANO_IP_STRNCAT(log_buffer, msg, sizeof(log_buffer) - (2u + timestamp_size + LOG_LEVEL_FORMAT_STRING_SIZE));
    NANO_IP_STRNCAT(log_buffer, "\n", 2u);

    /* Call user defined log function */
    va_start(arg_list, msg);
    NANO_IP_LOG_FUNC(log_buffer, arg_list);
    va_end(arg_list);
}


#endif /* NANO_IP_LOG_LEVEL */
