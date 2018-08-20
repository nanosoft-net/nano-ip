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

#include "nano_ip.h"
#include "nano_os_api.h"

/* Check if module is enabled */
#if (NANO_OS_CONSOLE_ENABLED == 1u)

/** \brief Handle the 'ifconfig' console command */
static void NANO_OS_NETTOOLS_IfconfigCmdHandler(void* const user_data, const uint32_t command_id, const char* const params);

/** \brief Handle the 'ifup' console command */
static void NANO_OS_NETTOOLS_IfupCmdHandler(void* const user_data, const uint32_t command_id, const char* const params);

/** \brief Handle the 'ifdown' console command */
static void NANO_OS_NETTOOLS_IfdownCmdHandler(void* const user_data, const uint32_t command_id, const char* const params);


/** \brief Console commands */
static const nano_os_console_cmd_desc_t console_commands[] = {
                                                                {"ifconfig", "[netif_id ipaddress netmask gateway] Display or modify network interfaces configuration", NANO_OS_NETTOOLS_IfconfigCmdHandler},
                                                                {"ifup", "[netif_id] Bring up a network interface", NANO_OS_NETTOOLS_IfupCmdHandler},
                                                                {"ifdown", "[netif_id] Bring down a network interface", NANO_OS_NETTOOLS_IfdownCmdHandler}
                                                             };

/** \brief Console commmands group */
static nano_os_console_cmd_group_desc_t console_command_group;


/** \brief Initialize the console commands */
nano_ip_error_t NANO_OS_NETTOOLS_Init(void)
{
    nano_ip_error_t ret;
    nano_os_error_t os_err;
    
    console_command_group.user_data = NULL;
    console_command_group.commands = console_commands;
    console_command_group.command_count = sizeof(console_commands) / sizeof(nano_os_console_cmd_desc_t);
    os_err = NANO_OS_CONSOLE_RegisterCommands(&console_command_group);
    if (os_err == NOS_ERR_SUCCESS)
    {
        ret = NIP_ERR_SUCCESS;
    }
    else
    {
        ret = NIP_ERR_FAILURE;
    }

    return ret;
}

/** \brief Handle the 'ifconfig' console command */
static void NANO_OS_NETTOOLS_IfconfigCmdHandler(void* const user_data, const uint32_t command_id, const char* const params)
{
    nano_ip_error_t err;

    NANO_OS_UNUSED(user_data);
    NANO_OS_UNUSED(command_id);

    /* Check parameters */
    if (params != NULL)
    {
        /* Extract network interface id */
        const uint8_t netif_id = ATOI(params);

        /* Get ip address */
        const char* next_param = NANO_OS_CONSOLE_GetNextParam(params);
        if (next_param != NULL)
        {
            const uint32_t ip_address = NANO_IP_inet_ntoa(next_param);
        
            /* Get netmask */
            next_param = NANO_OS_CONSOLE_GetNextParam(next_param);
            if (next_param != NULL)
            {
                const uint32_t netmask = NANO_IP_inet_ntoa(next_param);
                
                /* Get gateway */
                uint32_t gateway = 0u;
                next_param = NANO_OS_CONSOLE_GetNextParam(next_param);
                if (next_param != NULL)
                {
                    gateway = NANO_IP_inet_ntoa(next_param);
                }

                #if (NANO_IP_ENABLE_LOCALHOST == 1)
                if (netif_id == LOCALHOST_INTERFACE_ID)
                {
                    (void)NANO_OS_USER_ConsoleWriteString("Warning : modifying localhost network interface address may lead to undefined behavior\r\n");
                }
                #endif /* NANO_IP_ENABLE_LOCALHOST */

                /* Configure interface */
                err = NANO_IP_NET_IFACES_SetIpv4Address(netif_id, ip_address, netmask, gateway);
                if (err == NIP_ERR_SUCCESS)
                {
                    (void)NANO_OS_USER_ConsoleWriteString("Success\r\n");
                }
                else
                {
                    char temp_str[10u];
                    (void)NANO_OS_USER_ConsoleWriteString("Error : ");
                    (void)NANO_IP_ITOA(NANO_IP_CAST(int32_t, err), temp_str, 10u);
                    (void)NANO_OS_USER_ConsoleWriteString(temp_str);
                    (void)NANO_OS_USER_ConsoleWriteString("\r\n");
                }
            }
            else
            {
                (void)NANO_OS_USER_ConsoleWriteString("Missing parameter netmask\r\n");    
            }
        }
        else
        {
            (void)NANO_OS_USER_ConsoleWriteString("Missing parameter ip_address\r\n");    
        }
    }
    else
    {
        /* Display network configuration */
        uint8_t netif_id = 0u;
        const char* name;
        ipv4_address_t ip_address;
        ipv4_address_t netmask;
        ipv4_address_t gateway;
        uint8_t mac_address[MAC_ADDRESS_SIZE];
        do
        {
            err = NANO_IP_NET_IFACES_GetInfo(netif_id, &name, &ip_address, &netmask, &gateway, mac_address);
            if (err == NIP_ERR_SUCCESS)
            {
                uint8_t i;
                char temp_str[20u];

                /* Display information */
                NANO_OS_USER_ConsoleWriteString("\r\n");
                NANO_OS_USER_ConsoleWriteString("[");
                NANO_OS_USER_ConsoleWriteString(name);
                NANO_OS_USER_ConsoleWriteString("]\r\n");

                NANO_OS_USER_ConsoleWriteString("Netif id: ");
                (void)NANO_IP_ITOA(NANO_IP_CAST(int32_t, netif_id), temp_str, 10u);
                (void)NANO_OS_USER_ConsoleWriteString(temp_str);
                (void)NANO_OS_USER_ConsoleWriteString("\r\n");

                NANO_OS_USER_ConsoleWriteString("MAC address: ");
                for (i = 0; i < MAC_ADDRESS_SIZE; i++)
                {
                    NANO_IP_snprintf(temp_str, sizeof(temp_str), "0x%x ", mac_address[i]);
                    NANO_OS_USER_ConsoleWriteString(temp_str);
                }
                NANO_OS_USER_ConsoleWriteString("\r\n");

                NANO_OS_USER_ConsoleWriteString("IPv4 address: ");
                NANO_IP_snprintf(temp_str, sizeof(temp_str), "%d.%d.%d.%d ", ((ip_address >> 24u) & 0xFF),
                                                                             ((ip_address >> 16u) & 0xFF),
                                                                             ((ip_address >> 8u) & 0xFF),
                                                                             ((ip_address >> 0u) & 0xFF));
                NANO_OS_USER_ConsoleWriteString(temp_str);

                NANO_OS_USER_ConsoleWriteString("netmask: ");
                NANO_IP_snprintf(temp_str, sizeof(temp_str), "%d.%d.%d.%d ", ((netmask >> 24u) & 0xFF),
                                                                             ((netmask >> 16u) & 0xFF),
                                                                             ((netmask >> 8u) & 0xFF),
                                                                             ((netmask >> 0u) & 0xFF));
                NANO_OS_USER_ConsoleWriteString(temp_str);

                NANO_OS_USER_ConsoleWriteString("gateway: ");
                NANO_IP_snprintf(temp_str, sizeof(temp_str), "%d.%d.%d.%d ", ((gateway >> 24u) & 0xFF),
                                                                             ((gateway >> 16u) & 0xFF),
                                                                             ((gateway >> 8u) & 0xFF),
                                                                             ((gateway >> 0u) & 0xFF));
                NANO_OS_USER_ConsoleWriteString(temp_str);
                NANO_OS_USER_ConsoleWriteString("\r\n");
                
                /* Next interface */
                netif_id++;
            }
        }
        while (err == NIP_ERR_SUCCESS);
    }
}

/** \brief Handle the 'ifup' console command */
static void NANO_OS_NETTOOLS_IfupCmdHandler(void* const user_data, const uint32_t command_id, const char* const params)
{
    NANO_OS_UNUSED(user_data);
    NANO_OS_UNUSED(command_id);

    /* Check parameters */
    if (params != NULL)
    {
        /* Extract network interface id */
        const uint8_t netif_id = ATOI(params);

        /* Bring network interface up */
        const nano_ip_error_t err = NANO_IP_NET_IFACES_Up(netif_id);
        if (err == NIP_ERR_SUCCESS)
        {
            (void)NANO_OS_USER_ConsoleWriteString("Success\r\n");
        }
        else
        {
            char temp_str[10u];
            (void)NANO_OS_USER_ConsoleWriteString("Error : ");
            (void)NANO_IP_ITOA(NANO_IP_CAST(int32_t, err), temp_str, 10u);
            (void)NANO_OS_USER_ConsoleWriteString(temp_str);
            (void)NANO_OS_USER_ConsoleWriteString("\r\n");
        }
    }
    else
    {
        (void)NANO_OS_USER_ConsoleWriteString("Missing parameter netif_id\r\n");
    }
}

/** \brief Handle the 'ifdown' console command */
static void NANO_OS_NETTOOLS_IfdownCmdHandler(void* const user_data, const uint32_t command_id, const char* const params)
{
    NANO_OS_UNUSED(user_data);
    NANO_OS_UNUSED(command_id);

    /* Check parameters */
    if (params != NULL)
    {
        /* Extract network interface id */
        const uint8_t netif_id = ATOI(params);

        /* Bring network interface down */
        const nano_ip_error_t err = NANO_IP_NET_IFACES_Down(netif_id);
        if (err == NIP_ERR_SUCCESS)
        {
            (void)NANO_OS_USER_ConsoleWriteString("Success\r\n");
        }
        else
        {
            char temp_str[10u];
            (void)NANO_OS_USER_ConsoleWriteString("Error : ");
            (void)NANO_IP_ITOA(NANO_IP_CAST(int32_t, err), temp_str, 10u);
            (void)NANO_OS_USER_ConsoleWriteString(temp_str);
            (void)NANO_OS_USER_ConsoleWriteString("\r\n");
        }
    }
    else
    {
        (void)NANO_OS_USER_ConsoleWriteString("Missing parameter netif_id\r\n");
    }
}

#endif /* NANO_OS_CONSOLE_ENABLED */
