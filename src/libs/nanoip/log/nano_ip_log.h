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

#ifndef NANO_IP_LOG_H
#define NANO_IP_LOG_H

/* Log levels */

/** \brief Log level debug */
#define NANO_IP_DEBUG   0x01u

/** \brief Log level info */
#define NANO_IP_INFO    0x02u

/** \brief Log level error */
#define NANO_IP_ERROR   0x03u


#include "nano_ip_cfg.h"

#if( NANO_IP_ENABLE_LOGS == 1 )

/** \brief Generic log macro */
#define NANO_IP_LOG(level, msg, ...)    NANO_IP_LOG_Log(level, msg, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/** \brief Nano IP Log function */
void NANO_IP_LOG_Log(const char* const level, const char* const msg, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#else

/* Set an invalid log level to disable log macros */
#undef NANO_IP_LOG_LEVEL
#define NANO_IP_LOG_LEVEL   0xFFu

#endif /* NANO_IP_ENABLE_LOGS */

/* Log macros depending on selected log level */
#if( NANO_IP_LOG_LEVEL == NANO_IP_DEBUG )

/** \brief Log a debug string */
#define NANO_IP_LOG_DEBUG(format, ...)  NANO_IP_LOG("DEBUG", format, ##__VA_ARGS__)

/** \brief Log an information string */
#define NANO_IP_LOG_INFO(format, ...)  NANO_IP_LOG("INFO ", format, ##__VA_ARGS__)

/** \brief Log an error string */
#define NANO_IP_LOG_ERROR(format, ...)  NANO_IP_LOG("ERROR", format, ##__VA_ARGS__)


#elif( NANO_IP_LOG_LEVEL == NANO_IP_INFO )

/** \brief Log a debug string */
#define NANO_IP_LOG_DEBUG(format, ...)  

/** \brief Log an information string */
#define NANO_IP_LOG_INFO(format, ...)  NANO_IP_LOG("INFO ", format, ##__VA_ARGS__)

/** \brief Log an error string */
#define NANO_IP_LOG_ERROR(format, ...)  NANO_IP_LOG("ERROR", format, ##__VA_ARGS__)


#elif( NANO_IP_LOG_LEVEL == NANO_IP_ERROR )

/** \brief Log a debug string */
#define NANO_IP_LOG_DEBUG(format, ...)  

/** \brief Log an information string */
#define NANO_IP_LOG_INFO(format, ...) 

/** \brief Log an error string */
#define NANO_IP_LOG_ERROR(format, ...)  NANO_IP_LOG("ERROR", format, ##__VA_ARGS__)


#else

/** \brief Log a debug string */
#define NANO_IP_LOG_DEBUG(format, ...)  

/** \brief Log an information string */
#define NANO_IP_LOG_INFO(format, ...) 

/** \brief Log an error string */
#define NANO_IP_LOG_ERROR(format, ...)

#endif /* NANO_IP_LOG_LEVEL */

#endif /* NANO_IP_LOG_H */
