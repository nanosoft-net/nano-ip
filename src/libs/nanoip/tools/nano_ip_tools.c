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

#include "nano_ip_tools.h"
#include "nano_ip_ipv4.h"



/** \brief  Writes a character inside the given string */
static int NANO_IP_PutChar(char *str, char c);

/** \brief  Writes a string inside the given string */
static int NANO_IP_PutString(char *str, const char *source);

/** \brief  Writes an unsigned int inside the given string, using the provided fill &
 *          width parameters */
static int NANO_IP_PutUnsignedInt(char *str, char fill, int width, unsigned int value);

/** \brief  Writes a signed int inside the given string, using the provided fill & width
 *          parameters */
static int NANO_IP_PutSignedInt(char *str, char fill, int width, int value);

/** \brief  Writes an hexadecimal value into a string, using the given fill, width &
 *          capital parameters */
static int NANO_IP_PutHexa(char *str, char fill, int width, unsigned char maj, unsigned int value);











/** \brief Highly portable but non-efficient memset function */
void* NANO_IP_memset(void* const dst, const int val, size_t size)
{
    if (dst != NULL)
    {
        const uint8_t u8_val = NANO_IP_CAST(uint8_t, val);
        uint8_t* u8_dst = NANO_IP_CAST(uint8_t*, dst);
        while (size != 0u)
        {
            (*u8_dst) = u8_val;
            u8_dst++;
            size--;
        }
    }

    return dst;
}

/** \brief Highly portable but non-efficient memcpy function */
void* NANO_IP_memcpy(void* const dst, const void* const src, size_t size)
{
    if ((dst != NULL) && (src != NULL))
    {
        uint8_t* u8_src = NANO_IP_CAST(uint8_t*, src);
        uint8_t* u8_dst = NANO_IP_CAST(uint8_t*, dst);
        while (size != 0u)
        {
            (*u8_dst) = (*u8_src);
            u8_src++;
            u8_dst++;
            size--;
        }
    }

    return dst;
}

/** \brief Highly portable but non-efficient memcmp function */
int NANO_IP_memcmp(const void* const s1, const void* const s2, size_t size)
{
    int ret = 0;

    if ((s1 != NULL) && (s2 != NULL))
    {
        uint8_t* u8_s1 = NANO_IP_CAST(uint8_t*, s1);
        uint8_t* u8_s2 = NANO_IP_CAST(uint8_t*, s2);
        while ((size != 0u) && (ret == 0))
        {
            if ((*u8_s1) > (*u8_s2))
            {
                ret = 1;
            }
            else if ((*u8_s1) < (*u8_s2))
            {
                ret = -1;
            }
            else
            {
                u8_s1++;
                u8_s2++;
                size--;
            }
        }
    }

    return ret;
}

/** \brief Highly portable but non-efficient strncmp function */
int NANO_IP_strncmp(const char* s1, const char* s2, size_t size)
{
    int ret = 0;

    if ((s1 != NULL) && (s2 != NULL))
    {
        while ((size != 0u) && (ret == 0))
        {
            if ((*s1) > (*s2))
            {
                ret = 1;
            }
            else if ((*s1) < (*s2))
            {
                ret = -1;
            }
            else if ((*s1) == 0)
            {
                size = 0;
            }
            else
            {
                s1++;
                s2++;
                size--;
            }
        }
    }

    return ret;
}

/** \brief Highly portable but non-efficient strnlen function */
size_t NANO_IP_strnlen(const char* s, size_t maxlen)
{
    size_t size = 0;

    if (s != NULL)
    {
        while (((*s) != 0) && (size != maxlen))
        {
            s++;
            size++;
        }
    }

    return size;
}

/** \brief Highly portable but non-efficient strncat function */
char* NANO_IP_strncat(char *dest, const char *src, size_t size)
{
    char* ret = dest;

    if ((dest != NULL) && (src != NULL))
    {
        while ((*dest) != 0)
        {
            dest++;
        }

        while ((size != 0) && ((*src) != 0))
        {
            (*dest) = (*src);
            dest++;
            src++;
            size--;
        }
        (*dest) = 0;
    }

    return ret;

}

/** \brief Highly portable but non-efficient vsnprintf function */
int NANO_IP_vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    char          fill;
    unsigned char width;
    signed int    num = 0;
    signed int    current_size = 0;

    /* Clear the string */
    if (str) {

        *str = 0;
    }

    /* Phase string */
    while (*format != 0 && current_size < size) {

        /* Normal character */
        if (*format != '%') {

            *str++ = *format++;
            current_size++;
        }
        /* Escaped '%' */
        else if (*(format+1) == '%') {

            *str++ = '%';
            format += 2;
            current_size++;
        }
        /* Token delimiter */
        else {

            fill = ' ';
            width = 0;
            format++;

            /* Parse filler */
            if (*format == '0') {

                fill = '0';
                format++;
            }

            /* Parse width */
            while ((*format >= '0') && (*format <= '9')) {

                width = (width*10) + *format-'0';
                format++;
            }

            /* Check if there is enough space */
            if (current_size + width > size) {

                width = size - current_size;
            }

            /* Parse type */
            switch (*format) {
            case 'd':
            case 'i': num = NANO_IP_PutSignedInt(str, fill, width, va_arg(ap, signed int)); break;
            case 'u': num = NANO_IP_PutUnsignedInt(str, fill, width, va_arg(ap, unsigned int)); break;
            case 'x': num = NANO_IP_PutHexa(str, fill, width, 0, va_arg(ap, unsigned int)); break;
            case 'X': num = NANO_IP_PutHexa(str, fill, width, 1, va_arg(ap, unsigned int)); break;
            case 's': num = NANO_IP_PutString(str, va_arg(ap, char *)); break;
            case 'c': num = NANO_IP_PutChar(str, va_arg(ap, unsigned int)); break;
            default:
                return -1;
            }

            format++;
            str += num;
            current_size += num;
        }
    }

    /* NULL-terminated (final \0 is not counted) */
    if (current_size < size) {

        *str = 0;
    }
    else {

        *(--str) = 0;
        current_size--;
    }

    return current_size;
}


/** \brief Highly portable but non-efficient snprintf function */
int NANO_IP_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list    ap;
    int rc;

    va_start(ap, format);
    rc = NANO_IP_vsnprintf(str, size, format, ap);
    va_end(ap);

    return rc;
}


/** \brief Highly portable but non-efficient atoi function */
int NANO_IP_atoi(const char* str)
{
    int res = 0;
    bool negative = false;

    if (str != NULL)
    {
        if ((*str) == '-')
        {
            negative = true;
            str++;
        }

        while ((*str) != 0)
        {
            int val = (*str) - '0';
            res = (res * 10u) + val;
            str++;
        }

        if (negative)
        {
            res = -1 * res;
        }
    }

    return res;
}

/** \brief Highly portable but non-efficient itoa function */
char* NANO_IP_itoa(int value, char * str, int base)
{
    static const char ascii_base[] = "0123456789abcdef";
    char tmp;
    int q, r;
    char* inv_str;
    char* ret = str;

    if (str != NULL)
    {
        if (value < 0)
        {
            (*str) = '-';
            str++;
            value = -1 * value;
        }

        inv_str = str;
        while (value >= base)
        {
            q = value / base;
            r = value - q * base;
            (*inv_str) = ascii_base[r];
            inv_str++;
            value = q;
        }
        (*inv_str) = ascii_base[value];
        (*(inv_str + 1u)) = 0;

        while (inv_str > str)
        {
            tmp = (*str);
            (*str) = (*inv_str);
            (*inv_str) = tmp;

            inv_str--;
            str++;
        }
    }

    return ret;
}

/** \brief Convert a string containing an IP address to an integer representation */
uint32_t NANO_IP_inet_ntoa(const char* addr_str)
{
    uint32_t ip_addr = 0u;

    if (addr_str != NULL)
    {
        char part[4u];
        bool error = false;
        uint8_t part_count = 0;        
        uint8_t char_count = 0;
        uint8_t part_index = 0;

        do
        {
            /* Look for the '.' separator */
            while ((addr_str[char_count] != 0u) &&
                   (addr_str[char_count] != '.') &&
                   (part_index < (sizeof(part) - 1u)) &&
                   (char_count < MAX_IPV4_ADDRESS_STRING_SIZE))
            {
                /* Next char */
                part[part_index] = addr_str[char_count];
                part_index++;
                char_count++;
            }

            if ( (char_count < MAX_IPV4_ADDRESS_STRING_SIZE) &&
                 (part_index != 0u) &&
                 ((addr_str[char_count] == '.') || (addr_str[char_count] == 0u)) )
            {
                /* Extract value */
                uint8_t part_value;
                part[part_index] = 0;
                part_value = NANO_IP_ATOI(part);
                ip_addr += (part_value << (8u * (3u - part_count)));
                part_index = 0;
                part_count++;
                char_count++;
            }
            else
            {
                ip_addr = 0;
                error = true;
            }
        }
        while ( (!error) && (part_count != 4u) );
    }

    return ip_addr;
}

/** \brief Compute the internet checksum of a buffer */
uint16_t NANO_IP_ComputeInternetCS(uint8_t* const pseudo_header, uint16_t pseudo_header_size, uint8_t* const buffer, uint16_t size)
{
    uint32_t checksum = 0;
    uint16_t* data = NANO_IP_CAST(uint16_t*, pseudo_header);

    /* Compute sum on pseudo header */
    if (data != NULL)
    {
        while (pseudo_header_size > 1)
        {
            checksum += (*data);
            data++;
            pseudo_header_size -= sizeof(uint16_t);
        }
    }

    /* Compute sum */
    data = NANO_IP_CAST(uint16_t*, buffer);
    while (size > 1)
    {
        checksum += (*data);
        data++;
        size -= sizeof(uint16_t);
    }
    if (size != 0)
    {
        checksum += *NANO_IP_CAST(uint8_t*, data);
    }

     /*  Fold 32-bit sum to 16 bits */
    while ((checksum >> 16u) != 0)
    {
        checksum = (checksum & 0x0000FFFFu) + (checksum >> 16u);
    }

    /* Invert result */
    checksum = ~checksum;

    return NANO_IP_CAST(uint16_t, (checksum & 0x0000FFFFu));
}


/** \brief  Writes a character inside the given string */
static int NANO_IP_PutChar(char *str, char c)
{
    *str = c;
    return 1;
}


/** \brief  Writes a string inside the given string */
static int NANO_IP_PutString(char *str, const char *source)
{
    int num = 0;

    while (*source != 0)
    {
        *str++ = *source++;
        num++;
    }

    return num;
}


/** \brief  Writes an unsigned int inside the given string, using the provided fill &
 *          width parameters */
static int NANO_IP_PutUnsignedInt(char *str, char fill, int width, unsigned int value)
{
    int num = 0;

    /* Take current digit into account when calculating width */
    width--;

    /* Recursively write upper digits */
    if ((value / 10u) > 0)
    {
        num = NANO_IP_PutUnsignedInt(str, fill, width, value / 10u);
        str += num;
    }
    else
    {
    	/* Write filler characters */
        while (width > 0)
        {
        	NANO_IP_PutChar(str, fill);
            str++;
            num++;
            width--;
        }
    }

    /* Write lower digit */
    num += NANO_IP_PutChar(str, (value % 10u) + '0');

    return num;
}


/** \brief  Writes a signed int inside the given string, using the provided fill & width
 *          parameters */
static int NANO_IP_PutSignedInt(char *str, char fill, int width, int value)
{
    int num = 0;
    unsigned int absolute;

    /* Compute absolute value */
    if (value < 0)
    {
        absolute = -value;
    }
    else
    {
        absolute = value;
    }

    /* Take current digit into account when calculating width */
    width--;

    /* Recursively write upper digits */
    if ((absolute / 10u) > 0)
    {
        if (value < 0)
        {
            num = NANO_IP_PutSignedInt(str, fill, width, -(absolute / 10u));
        }
        else
        {
            num = NANO_IP_PutSignedInt(str, fill, width, absolute / 10u);
        }
        str += num;
    }
    else
    {
        /* Reserve space for sign */
        if (value < 0)
        {
            width--;
        }

        /* Write filler characters */
        while (width > 0)
        {
        	NANO_IP_PutChar(str, fill);
            str++;
            num++;
            width--;
        }

        /* Write sign */
        if (value < 0)
        {
            num += NANO_IP_PutChar(str, '-');
            str++;
        }
    }

    /* Write lower digit */
    num += NANO_IP_PutChar(str, (absolute % 10u) + '0');

    return num;
}


/** \brief  Writes an hexadecimal value into a string, using the given fill, width &
 *          capital parameters */
static int NANO_IP_PutHexa(char *str, char fill, int width, unsigned char maj, unsigned int value)
{
    int num = 0;

    /* Decrement width */
    width--;

    /* Recursively output upper digits */
    if ((value >> 4) > 0)
    {
        num += NANO_IP_PutHexa(str, fill, width, maj, value >> 4);
        str += num;
    }
    else
    {
    	/* Write filler chars */
        while (width > 0)
        {
        	NANO_IP_PutChar(str, fill);
            str++;
            num++;
            width--;
        }
    }

    /* Write current digit */
    if ((value & 0x0Fu) < 10u)
    {
    	NANO_IP_PutChar(str, (value & 0x0Fu) + '0');
    }
    else if (maj)
    {
    	NANO_IP_PutChar(str, (value & 0x0Fu) - 10u + 'A');
    }
    else
    {
    	NANO_IP_PutChar(str, (value & 0x0Fu) - 10u + 'a');
    }
    num++;

    return num;
}









