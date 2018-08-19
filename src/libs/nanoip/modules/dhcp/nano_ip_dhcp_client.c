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

#include "nano_ip_dhcp_client.h"

#if( (NANO_IP_ENABLE_DHCP == 1) && (NANO_IP_ENABLE_DHCP_CLIENT == 1) )

#include "nano_ip_log.h"
#include "nano_ip_packet_funcs.h"
#include "nano_ip_net_ifaces.h"



/** \brief Send a DHCP discover message */
static bool NANO_IP_DHCP_CLIENT_SendDiscover(nano_ip_dhcp_client_t* const dhcp_client);

/** \brief Send a DHCP request message */
static bool NANO_IP_DHCP_CLIENT_SendRequest(nano_ip_dhcp_client_t* const dhcp_client, const bool send_lease_address);


/** \brief Process an incoming DHCP message */
static void NANO_IP_DHCP_CLIENT_ProcessMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg);

/** \brief Process a DHCP offer message */
static void NANO_IP_DHCP_CLIENT_ProcessOfferMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg);

/** \brief Process a DHCP ack message */
static void NANO_IP_DHCP_CLIENT_ProcessAckMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg);

/** \brief Decode an incoming DHCP message */
static bool NANO_IP_DHCP_CLIENT_DecodeMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg);

/** \brief Parse a DHCP message option */
static bool NANO_IP_DHCP_CLIENT_ParseMessageOption(nano_ip_net_packet_t* const packet, uint8_t* option, uint8_t* option_size, void** option_data);

/** \brief UDP event callback */
static bool NANO_IP_DHCP_CLIENT_UdpEvent(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data);

/** \brief DHCP client timer callback */
static void NANO_IP_DHCP_CLIENT_TimerCallback(oal_timer_t* const timer, void* const user_data);




/** \brief Initialize a DHCP client instance */
nano_ip_error_t NANO_IP_DHCP_CLIENT_Init(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_if_t* const net_if, const uint16_t server_port, const uint16_t client_port, const uint32_t timeout)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((dhcp_client != NULL) && 
        (net_if != NULL) && 
        (timeout != 0u))
    {
        /* 0 init */
        NANO_IP_MEMSET(dhcp_client, 0, sizeof(nano_ip_dhcp_client_t));

        /* Initialize instance */
        dhcp_client->net_if = net_if;
        dhcp_client->server_address = IPV4_BROADCAST_ADDRESS;
        dhcp_client->server_port = server_port;
        dhcp_client->client_port = client_port;
        dhcp_client->timeout = timeout;

        /* Create UDP handle */
        ret = NANO_IP_UDP_InitializeHandle(&dhcp_client->udp_handle, NANO_IP_DHCP_CLIENT_UdpEvent, dhcp_client);

        /* Create timer */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_OAL_TIMER_Create(&dhcp_client->timer, NANO_IP_DHCP_CLIENT_TimerCallback, dhcp_client);
        }
    }

    return ret;
}



/** \brief Start a DHCP client instance */
nano_ip_error_t NANO_IP_DHCP_CLIENT_Start(nano_ip_dhcp_client_t* const dhcp_client)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (dhcp_client != NULL)
    {
        /* Bind UDP socket */
        ret = NANO_IP_UDP_Bind(&dhcp_client->udp_handle, IPV4_ANY_ADDRESS, dhcp_client->client_port);
        if (ret == NIP_ERR_SUCCESS)
        {
            NANO_IP_LOG_INFO("[DCHP Client] Starting on interface %s...", dhcp_client->net_if->name);

            /* Initialize state */
            dhcp_client->state = DCS_SELECTING;

            /* Send first DHCP request */
            dhcp_client->current_timeout = dhcp_client->timeout;
            (void)NANO_IP_DHCP_CLIENT_SendDiscover(dhcp_client);
        }
    }

    return ret;
}

/** \brief Send a DHCP discover message */
static bool NANO_IP_DHCP_CLIENT_SendDiscover(nano_ip_dhcp_client_t* const dhcp_client)
{
    bool ret = false;
    nano_ip_error_t err;
    nano_ip_net_packet_t* packet = NULL;

    /* Stop timer */
    (void)NANO_IP_OAL_TIMER_Stop(&dhcp_client->timer);

    /* Allocate discover packet */
    err = NANO_IP_UDP_AllocatePacket(&packet, DHCP_DISCOVER_MSG_SIZE + (2u + DHCP_PARAMETER_REQUEST_LIST_OPTION_SIZE(5u)));
    if (err == NIP_ERR_SUCCESS)
    {
        /* Fill packet */

        /* Header */
        NANO_IP_PACKET_Write8bits(packet, DHCP_BOOT_REQUEST);
        NANO_IP_PACKET_Write8bits(packet, DHCP_HTYPE_FIELD);
        NANO_IP_PACKET_Write8bits(packet, MAC_ADDRESS_SIZE);
        NANO_IP_PACKET_Write8bits(packet, 0x00u);

        /* Transaction id */
        dhcp_client->transaction_id = NANO_IP_OAL_TIME_GetMsCounter() + 0xF00D0000u;
        NANO_IP_PACKET_Write32bits(packet, dhcp_client->transaction_id);

        /* Flags */
        NANO_IP_PACKET_Write16bits(packet, 0x0000u);
        NANO_IP_PACKET_Write16bits(packet, DHCP_FLAGS_BROADCAST);

        /* IP addresses */
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);

        /* MAC address */
        NANO_IP_PACKET_WriteBuffer(packet, dhcp_client->net_if->mac_address, MAC_ADDRESS_SIZE);
        NANO_IP_PACKET_Write16bits(packet, 0x0000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);

        /* BOOTP padding */
        NANO_IP_PACKET_WriteZeros(packet, DHCP_BOOTP_PADDING_SIZE);

        /* DHCP magic cookie */
        NANO_IP_PACKET_Write32bits(packet, DHCP_MAGIC_COOKIE);

        /* DHCP request type option */
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUEST_TYPE_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUEST_TYPE_OPTION_SIZE);
        NANO_IP_PACKET_Write8bits(packet, DHCP_DISCOVER_MSG_TYPE);

        /* DHCP parameter request list option */
        NANO_IP_PACKET_Write8bits(packet, DHCP_PARAMETER_REQUEST_LIST_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_PARAMETER_REQUEST_LIST_OPTION_SIZE(5u));
        NANO_IP_PACKET_Write8bits(packet, DHCP_SUBNET_MASK_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_ROUTER_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_LEASE_TIME_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_RENEWAL_TIME_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_REBINDING_TIME_OPTION);

        /* DHCP end flag */
        NANO_IP_PACKET_Write8bits(packet, DHCP_END_FLAG_OPTION);

        /* Send packet */
        packet->net_if = dhcp_client->net_if;
        err = NANO_IP_UDP_SendPacket(&dhcp_client->udp_handle, IPV4_BROADCAST_ADDRESS, dhcp_client->server_port, packet);
        if ((err == NIP_ERR_SUCCESS) || (err == NIP_ERR_IN_PROGRESS))
        {
            ret = true;
        }
        else
        {
            /* Release packet */
            (void)NANO_IP_UDP_ReleasePacket(packet);
        }
    }

    /* Start timer */
    (void)NANO_IP_OAL_TIMER_Start(&dhcp_client->timer, dhcp_client->current_timeout);

    return ret;
}

/** \brief Send a DHCP request message */
static bool NANO_IP_DHCP_CLIENT_SendRequest(nano_ip_dhcp_client_t* const dhcp_client, const bool send_lease_address)
{
    bool ret = false;
    nano_ip_error_t err;
    nano_ip_net_packet_t* packet = NULL;

    /* Stop timer */
    (void)NANO_IP_OAL_TIMER_Stop(&dhcp_client->timer);

    /* Allocate discover packet */
    err = NANO_IP_UDP_AllocatePacket(&packet, DHCP_REQUEST_MSG_SIZE);
    if (err == NIP_ERR_SUCCESS)
    {
        /* Fill packet */

        /* Header */
        NANO_IP_PACKET_Write8bits(packet, DHCP_BOOT_REQUEST);
        NANO_IP_PACKET_Write8bits(packet, DHCP_HTYPE_FIELD);
        NANO_IP_PACKET_Write8bits(packet, MAC_ADDRESS_SIZE);
        NANO_IP_PACKET_Write8bits(packet, 0x00u);

        /* Transaction id */
        NANO_IP_PACKET_Write32bits(packet, dhcp_client->transaction_id);

        /* Flags */
        NANO_IP_PACKET_Write16bits(packet, 0x0000u);
        NANO_IP_PACKET_Write16bits(packet, DHCP_FLAGS_BROADCAST);

        /* IP addresses */
        if (send_lease_address)
        {
            NANO_IP_PACKET_Write32bits(packet, dhcp_client->lease_address);
        }
        else
        {
            NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        }
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);

        /* MAC address */
        NANO_IP_PACKET_WriteBuffer(packet, dhcp_client->net_if->mac_address, MAC_ADDRESS_SIZE);
        NANO_IP_PACKET_Write16bits(packet, 0x0000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);
        NANO_IP_PACKET_Write32bits(packet, 0x00000000u);

        /* BOOTP padding */
        NANO_IP_PACKET_WriteZeros(packet, DHCP_BOOTP_PADDING_SIZE);

        /* DHCP magic cookie */
        NANO_IP_PACKET_Write32bits(packet, DHCP_MAGIC_COOKIE);

        /* DHCP request type option */
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUEST_TYPE_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUEST_TYPE_OPTION_SIZE);
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUEST_MSG_TYPE);

        /* Server id option */
        NANO_IP_PACKET_Write8bits(packet, DHCP_SERVER_ID_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_SERVER_ID_OPTION_SIZE);
        NANO_IP_PACKET_Write32bits(packet, dhcp_client->server_address);

        /* Requested address option */
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUESTED_ADDRESS_OPTION);
        NANO_IP_PACKET_Write8bits(packet, DHCP_REQUESTED_ADDRESS_OPTION_SIZE);
        NANO_IP_PACKET_Write32bits(packet, dhcp_client->lease_address);

        /* DHCP end flag */
        NANO_IP_PACKET_Write8bits(packet, DHCP_END_FLAG_OPTION);

        /* Send packet */
        packet->net_if = dhcp_client->net_if;
        err = NANO_IP_UDP_SendPacket(&dhcp_client->udp_handle, IPV4_BROADCAST_ADDRESS, dhcp_client->server_port, packet);
        if ((err == NIP_ERR_SUCCESS) || (err == NIP_ERR_IN_PROGRESS))
        {
            ret = true;
        }
        else
        {
            /* Release packet */
            (void)NANO_IP_UDP_ReleasePacket(packet);
        }
    }

    /* Start timer */
    (void)NANO_IP_OAL_TIMER_Start(&dhcp_client->timer, dhcp_client->current_timeout);

    return ret;
}


/** \brief Process an incoming DHCP message */
static void NANO_IP_DHCP_CLIENT_ProcessMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg)
{
    /* DHCP client state machine */
    switch (dhcp_client->state)
    {
        case DCS_SELECTING:
            /* Waiting for an offer */
            NANO_IP_DHCP_CLIENT_ProcessOfferMessage(dhcp_client, packet, dhcp_msg);
            break;

        case DCS_REQUESTING:
            /* Waiting for an acknowledge */
            
            /* Intended fallthrough */

        case DCS_RENEWING:
            /* Waiting for a renewal acknowledge */
            NANO_IP_DHCP_CLIENT_ProcessAckMessage(dhcp_client, packet, dhcp_msg);
            break;

        default:
            /* Ignore message */
            break;
    }
}

/** \brief Process a DHCP offer message */
static void NANO_IP_DHCP_CLIENT_ProcessOfferMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg)
{
    uint8_t option;
    uint8_t option_size;
    uint8_t* option_data;
    bool end = false;
    uint8_t msg_type = 0u;

    /* Retrieve IP address */
    dhcp_client->lease_address = dhcp_msg->yiaddr;
    dhcp_client->netmask = 0u;
    dhcp_client->gateway = 0u;

    /* Parse options */
    while (NANO_IP_DHCP_CLIENT_ParseMessageOption(packet, &option, &option_size, NANO_IP_CAST(void**, &option_data)))
    {
        /* Handle option */
        switch (option)
        {
            case DHCP_REQUEST_TYPE_OPTION:
                /* Request type */
                if (option_size == DHCP_REQUEST_TYPE_OPTION_SIZE)
                {
                    (void)NANO_IP_MEMCPY(&msg_type, option_data, DHCP_REQUEST_TYPE_OPTION_SIZE);
                }
                break;

            case DHCP_SERVER_ID_OPTION:
                /* Server id */
                if (option_size == DHCP_SERVER_ID_OPTION_SIZE)
                {
                    dhcp_client->server_address = NET_READ_32(option_data);
                }
                break;

            case DHCP_SUBNET_MASK_OPTION:
                /* Netmask */
                if (option_size == DHCP_SUBNET_MASK_OPTION_SIZE)
                {
                    dhcp_client->netmask = NET_READ_32(option_data);
                }
                break;

            case DHCP_ROUTER_OPTION:
                /* Gateway */
                if (option_size == DHCP_ROUTER_OPTION_SIZE)
                {
                    dhcp_client->gateway = NET_READ_32(option_data);
                }
                break;

            case DHCP_LEASE_TIME_OPTION:
                /* Lease time */
                if (option_size == DHCP_LEASE_TIME_OPTION_SIZE)
                {
                    dhcp_client->lease_time = NET_READ_32(option_data);
                }
                break;

            case DHCP_RENEWAL_TIME_OPTION:
                /* Renewal time */
                if (option_size == DHCP_RENEWAL_TIME_OPTION_SIZE)
                {
                    dhcp_client->lease_time_1 = NET_READ_32(option_data);
                }
                break;

            case DHCP_REBINDING_TIME_OPTION:
                /* Rebinding time */
                if (option_size == DHCP_REBINDING_TIME_OPTION_SIZE)
                {
                    dhcp_client->lease_time_2 = NET_READ_32(option_data);
                }
                break;

            case DHCP_END_FLAG_OPTION:
                /* End flag */
                end = true;
                break;

            default:
                /* Skip option */
                break;
        }
    }

    /* Check values */
    if ( (msg_type == DHCP_OFFER_MSG_TYPE) && end && 
         (dhcp_client->lease_address != 0u) &&
        (dhcp_client->netmask != 0u) &&
        (dhcp_client->lease_time != 0u) )
    {
        /* Send corresponding DHCP request */
        const bool ret = NANO_IP_DHCP_CLIENT_SendRequest(dhcp_client, false);
        if (ret)
        {
            /* Next state */
            dhcp_client->state = DCS_REQUESTING;
        }
    }
}

/** \brief Process a DHCP ack message */
static void NANO_IP_DHCP_CLIENT_ProcessAckMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg)

{
    uint8_t option;
    uint8_t option_size;
    void* option_data;
    bool end = false;
    uint8_t msg_type = 0u;

    /* May be used later... */
    (void)dhcp_msg;

    /* Parse options */
    while (NANO_IP_DHCP_CLIENT_ParseMessageOption(packet, &option, &option_size, &option_data))
    {
        /* Handle option */
        switch (option)
        {
            case DHCP_REQUEST_TYPE_OPTION:
                /* Request type */
                if (option_size == DHCP_REQUEST_TYPE_OPTION_SIZE)
                {
                    (void)NANO_IP_MEMCPY(&msg_type, option_data, DHCP_REQUEST_TYPE_OPTION_SIZE);
                }
                break;

            case DHCP_END_FLAG_OPTION:
                /* End flag */
                end = true;
                break;

            default:
                /* Skip option */
                break;
        }
    }

    /* Check values */
    if ((msg_type == DHCP_ACK_MSG_TYPE) && end)
    {
        nano_ip_error_t ret = NIP_ERR_SUCCESS;

        /* Check state */
        if (dhcp_client->state == DCS_REQUESTING)
        {
            /* Apply new IP address */
            ret = NANO_IP_NET_IFACES_SetIpv4Address(dhcp_client->net_if->id, dhcp_client->lease_address, dhcp_client->netmask, dhcp_client->gateway);

            NANO_IP_LOG_INFO("[DCHP Client] Received configuration : { Address : 0x%x - Mask : 0x%x - Gateway : 0x%x }", dhcp_client->lease_address
                                                                                                                       , dhcp_client->netmask
                                                                                                                       , dhcp_client->gateway);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            if (dhcp_client->state == DCS_REQUESTING)
            {
                /* Compute renewing and rebinding times */
                if ((dhcp_client->lease_time_1 == 0u) ||
                    (dhcp_client->lease_time_2 == 0u))
                {
                    dhcp_client->lease_time_1 = (dhcp_client->lease_time * NANO_IP_DHCP_T1_PERCENTAGE) * 10u;
                    dhcp_client->lease_time_2 = (dhcp_client->lease_time * NANO_IP_DHCP_T2_PERCENTAGE) * 10u;
                }
                else
                {
                    dhcp_client->lease_time_1 *= 1000u;
                    dhcp_client->lease_time_2 *= 1000u;
                }
            }

            /* Next state */
            dhcp_client->state = DCS_BOUND;

            /* Restart timer */
            (void)NANO_IP_OAL_TIMER_Stop(&dhcp_client->timer);
            dhcp_client->current_timeout = dhcp_client->lease_time_1;
            (void)NANO_IP_OAL_TIMER_Start(&dhcp_client->timer, dhcp_client->current_timeout);
        }
    }
}

/** \brief Decode an incoming DHCP message */
static bool NANO_IP_DHCP_CLIENT_DecodeMessage(nano_ip_dhcp_client_t* const dhcp_client, nano_ip_net_packet_t* const packet, nano_ip_dhcp_msg_t* const dhcp_msg)
{
    bool ret = false;
    uint32_t magic_cookie = 0u;

    /* May be used later... */
	(void)dhcp_client;
	(void)dhcp_msg;

    /* Check minimum size */
    if (packet->count >= DHCP_MIN_MSG_SIZE)
    {
        /* Header */
        dhcp_msg->op = NANO_IP_PACKET_Read8bits(packet);
        dhcp_msg->htype = NANO_IP_PACKET_Read8bits(packet);
        dhcp_msg->hlen = NANO_IP_PACKET_Read8bits(packet);
        dhcp_msg->hops = NANO_IP_PACKET_Read8bits(packet);

        /* Transaction id */
        dhcp_msg->transaction_id = NANO_IP_PACKET_Read32bits(packet);

        /* Flags */
        dhcp_msg->flags = NANO_IP_PACKET_Read32bits(packet);

        /* IP address */
        dhcp_msg->ciaddr = NANO_IP_PACKET_Read32bits(packet);
        dhcp_msg->yiaddr = NANO_IP_PACKET_Read32bits(packet);
        dhcp_msg->siaddr = NANO_IP_PACKET_Read32bits(packet);
        dhcp_msg->giaddr = NANO_IP_PACKET_Read32bits(packet);

        /* MAC address */
        NANO_IP_PACKET_ReadBuffer(packet, dhcp_msg->chaddr, sizeof(dhcp_msg->chaddr));

        /* BOOTP padding */
        NANO_IP_PACKET_ReadSkipBytes(packet, DHCP_BOOTP_PADDING_SIZE);

        /* DHCP magic cookie */
        magic_cookie = NANO_IP_PACKET_Read32bits(packet);
        if (magic_cookie == DHCP_MAGIC_COOKIE)
        {
            ret = true;
        }
    }

    return ret;
}

/** \brief Parse a DHCP message option */
static bool NANO_IP_DHCP_CLIENT_ParseMessageOption(nano_ip_net_packet_t* const packet, uint8_t* option, uint8_t* option_size, void** option_data)
{
    bool ret = false;

    /* Extract option type */
    if (packet->count >= 1u)
    {
        (*option) = NANO_IP_PACKET_Read8bits(packet);

        /* Extract size */
        if (packet->count >= 1u)
        {
            (*option_size) = NANO_IP_PACKET_Read8bits(packet);

            /* Extract data */
            if (packet->count >= 1u)
            {
                (*option_data) = packet->current;
            }
            else
            {
                (*option_data) = NULL;
            }
        }
        else
        {
            (*option_size) = 0u;
        }

        /* Skip data */
        NANO_IP_PACKET_ReadSkipBytes(packet, (*option_size));

        ret = true;
    }

    return ret;
}

/** \brief UDP event callback */
static bool NANO_IP_DHCP_CLIENT_UdpEvent(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data)
{
    nano_ip_dhcp_client_t* const dhcp_client = NANO_IP_CAST(nano_ip_dhcp_client_t*, user_data);

    /* Check parameters */
    if (dhcp_client != NULL)
    {
        /* Check event */
        switch (event)
        {
            case UDP_EVENT_RX:
            {
                /* Decode packet */
                nano_ip_dhcp_msg_t dhcp_msg;
                const bool ret = NANO_IP_DHCP_CLIENT_DecodeMessage(dhcp_client, event_data->packet, &dhcp_msg);
                if (ret)
                {
                    /* Check if this message is for us */
                    if ((NANO_IP_MEMCMP(dhcp_msg.chaddr, dhcp_client->net_if->mac_address, MAC_ADDRESS_SIZE) == 0) &&
                    (dhcp_msg.transaction_id == dhcp_client->transaction_id) &&
                    (dhcp_msg.op == DHCP_BOOT_REPLY) )
                    {
                        /* Process message */
                        NANO_IP_DHCP_CLIENT_ProcessMessage(dhcp_client, event_data->packet, &dhcp_msg);
                    }
                }
                break;        
            }

            default:
            {
                /* Ignore */
                break;
            }
        }
    }

    /* Always release the packet */
    return true;
}

/** \brief DHCP client timer callback */
static void NANO_IP_DHCP_CLIENT_TimerCallback(oal_timer_t* const timer, void* const user_data)
{
    nano_ip_dhcp_client_t* const dhcp_client = NANO_IP_CAST(nano_ip_dhcp_client_t*, user_data);
    (void)timer;

    /* Check parameters */
    if (dhcp_client != NULL)
    {
        /* Handle corresponding timeout */
        switch (dhcp_client->state)
        {
            case DCS_REBINDING:
                /* Rebinding => unconfigure current IP address and restart DHCP process */
                NANO_IP_NET_IFACES_SetIpv4Address(dhcp_client->net_if->id, 0u, 0u, dhcp_client->gateway);

                /* Intended fallthrough */

            case DCS_REQUESTING:
                /* Requesting => back to selecting state */
                dhcp_client->state = DCS_SELECTING;

                /* Intended fallthrough */

            case DCS_SELECTING:
                /* Selecting => re-send a discover message */
                dhcp_client->current_timeout = dhcp_client->timeout;
                (void)NANO_IP_DHCP_CLIENT_SendDiscover(dhcp_client);
                break;

            case DCS_BOUND:
                /* Bound => send a request message to renew address */
                dhcp_client->state = DCS_RENEWING;
                dhcp_client->current_timeout = dhcp_client->timeout;
                dhcp_client->transaction_id = NANO_IP_OAL_TIME_GetMsCounter() + 0xF00D0000u;
                (void)NANO_IP_DHCP_CLIENT_SendRequest(dhcp_client, true);
                break;

            case DCS_RENEWING:
                /* Renewing => retry until T2 is reached */
                
                /* Rebinding => re-send a discover message */
                dhcp_client->state = DCS_REBINDING;
                (void)NANO_IP_DHCP_CLIENT_SendDiscover(dhcp_client);
                break;

            
            
            default:
                /* Ignore */
                break;
        }
    }
}



#endif /* NANO_IP_ENABLE_DHCP && NANO_IP_ENABLE_DHCP_CLIENT */
