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
#include "bsp.h"

/********************* IP address configuration ************************/

/** \brief Uncomment the following line to enable DHCP */
//#define USE_DHCP

/** \brief IP address when DHCP is not enabled */
#define DEMO_IP_ADDRESS        "192.168.0.70"

/** \brief MAC address */
static const uint8_t MAC_ADDRESS[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};


/************************** Demo options ***************************/

/** \brief Priority of the demo task */
#define DEMO_TASK_PRIORITY                  8u

/** \brief Size in bytes of the demo task */
#define DEMO_TASK_STACK_SIZE                1024u



/************************** UDP demo options ***************************/

/** \brief Enable or disable the UDP echo demo */
#define UDP_ECHO_DEMO_ENABLED               1u

/** \brief Enable or disable the UDP echo task */
#define UDP_ECHO_DEMO_TASK_ENABLED          0u

/** \brief Priority of the UDP echo task */
#define UDP_ECHO_TASK_PRIORITY              3u

/** \brief Size in bytes of the UDP echo task */
#define UDP_ECHO_TASK_STACK_SIZE            512u

/** \brief UDP listen port */
#define UDP_ECHO_DEMO_LISTEN_PORT           54321u


/************************** TCP server demo options ***************************/

/** \brief Enable or disable the TCP server echo demo */
#define TCP_SERVER_ECHO_DEMO_ENABLED            1u

/** \brief Enable or disable the TCP server echo task */
#define TCP_SERVER_ECHO_DEMO_TASK_ENABLED       0u

/** \brief Priority of the TCP server echo task */
#define TCP_SERVER_ECHO_TASK_PRIORITY           4u

/** \brief Size in bytes of the TCP server echo task */
#define TCP_SERVER_ECHO_TASK_STACK_SIZE         512u

/** \brief TCP listen port */
#define TCP_SERVER_ECHO_DEMO_LISTEN_PORT        8765u

/** \brief Maximum number of simultaneously connected clients */
#define TCP_SERVER_ECHO_DEMO_MAX_CLIENTS        3u


/************************** TCP client demo options ***************************/

/** \brief Enable or disable the TCP client echo demo */
#define TCP_CLIENT_ECHO_DEMO_ENABLED            1u

/** \brief Enable or disable the TCP client echo task */
#define TCP_CLIENT_ECHO_DEMO_TASK_ENABLED       0u

/** \brief Priority of the TCP client echo task */
#define TCP_CLIENT_ECHO_TASK_PRIORITY           4u

/** \brief Size in bytes of the TCP client echo task */
#define TCP_CLIENT_ECHO_TASK_STACK_SIZE         512u

/** \brief TCP client destination address */
#define TCP_CLIENT_ECHO_DEMO_DEST_ADDRESS       "192.168.0.1"

/** \brief TCP client destination port */
#define TCP_CLIENT_ECHO_DEMO_DEST_PORT          4567u






/********************** Demo's local variables *************************/


/** \brief Network interface handle */
static nano_ip_net_if_t s_net_if;

/** \brief Packet allocator handle */
static nano_ip_net_packet_allocator_t s_packet_allocator;



#if (UDP_ECHO_DEMO_ENABLED == 1u)

/** \brief UDP handle */
static nano_ip_udp_handle_t s_udp_handle;

/** \brief UDP event callback */
static bool DEMO_UDP_EventCallback(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data);



#if (UDP_ECHO_DEMO_TASK_ENABLED == 1u)

/** \brief UDP echo task handle */
static oal_task_t s_udp_task_handle;

/** \brief UDP echo mutex handle */
static oal_mutex_t s_udp_mutex_handle;

/** \brief UDP echo synchronization object */
static oal_flags_t s_udp_sync_object;

/** \brief UDP address to send data */
static ipv4_address_t s_udp_send_address;

/** \brief UDP port to send data */
static uint16_t s_udp_send_port;

/** \brief UDP received packets */
static nano_ip_net_packet_t* s_udp_rx_packets;

/** \brief UDP echo task */
static void DEMO_UDP_EchoRxTask(void* param);

#endif /* UDP_ECHO_DEMO_TASK_ENABLED */

#endif /* UDP_ECHO_DEMO_ENABLED */




#if (TCP_SERVER_ECHO_DEMO_ENABLED == 1u)

/** \brief TCP server handle */
static nano_ip_tcp_handle_t s_tcp_server_handle;

/** \brief TCP server client's handles */
static nano_ip_tcp_handle_t s_tcp_server_client_handles[TCP_SERVER_ECHO_DEMO_MAX_CLIENTS];

/** \brief TCP server event callback */
static bool DEMO_TCP_ServerEventCallback(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data);



#if (TCP_SERVER_ECHO_DEMO_TASK_ENABLED == 1u)

/** \brief UDP echo task handle */
static oal_task_t s_udp_task_handle;

/** \brief UDP echo mutex handle */
static oal_mutex_t s_udp_mutex_handle;

/** \brief UDP echo synchronization object */
static oal_flags_t s_udp_sync_object;

/** \brief UDP address to send data */
static ipv4_address_t s_udp_send_address;

/** \brief UDP port to send data */
static uint16_t s_udp_send_port;

/** \brief UDP received packets */
static nano_ip_net_packet_t* s_udp_rx_packets;

/** \brief TCP server echo task */
static void DEMO_UDP_EchoRxTask(void* param);

#endif /* TCP_SERVER_ECHO_DEMO_TASK_ENABLED */

#endif /* TCP_SERVER_ECHO_DEMO_ENABLED */





#if (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u)

/** \brief TCP client handle */
static nano_ip_tcp_handle_t s_tcp_client_handle;

/** \brief TCP client event callback */
static bool DEMO_TCP_ClientEventCallback(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data);



#if (TCP_CLIENT_ECHO_DEMO_TASK_ENABLED == 1u)

/** \brief UDP echo task handle */
static oal_task_t s_udp_task_handle;

/** \brief UDP echo mutex handle */
static oal_mutex_t s_udp_mutex_handle;

/** \brief UDP echo synchronization object */
static oal_flags_t s_udp_sync_object;

/** \brief UDP address to send data */
static ipv4_address_t s_udp_send_address;

/** \brief UDP port to send data */
static uint16_t s_udp_send_port;

/** \brief UDP received packets */
static nano_ip_net_packet_t* s_udp_rx_packets;

/** \brief TCP server echo task */
static void DEMO_UDP_EchoRxTask(void* param);

#endif /* TCP_CLIENT_ECHO_DEMO_TASK_ENABLED */

#endif /* TCP_CLIENT_ECHO_DEMO_ENABLED */


#ifndef NANO_IP_OAL_OS_LESS
/** \brief Demo task handle */
static oal_task_t s_demo_task;
#endif /* NANO_IP_OAL_OS_LESS */

/** \brief Demo task */
static void DEMO_Task(void* unused);


/** \brief Application entry point */
int main(void)
{
    bool ret;

    /* Initialize operating system */
    ret = NANO_IP_BSP_OSInit();
    if (ret)
    {
        /* Create the Demo task */
        #ifndef NANO_IP_OAL_OS_LESS
        const nano_ip_error_t err = NANO_IP_OAL_TASK_Create(&s_demo_task, "Demo task", DEMO_Task, NULL, DEMO_TASK_PRIORITY, DEMO_TASK_STACK_SIZE);
        if (err != NIP_ERR_SUCCESS)
        {
            ret = false;
        }
        #else
        DEMO_Task(NULL);
        #endif /* NANO_IP_OAL_OS_LESS */
    }

    /* Start the operating system */
    if (ret)
    {
        ret = NANO_IP_BSP_OSStart();
    }
    
    /* If we get here, an error has happened while initializing/starting the operating system */
    while (!ret)
    {}

    return 0;
}


/** \brief Demo task */
static void DEMO_Task(void* unused)
{
    bool ret = false;
    nano_ip_error_t err = NIP_ERR_FAILURE;

    const char* net_if_name = NULL;
    uint32_t rx_packet_size = 0u;
    uint32_t rx_packet_count = 0u;
    uint8_t task_priority = 0u;
    uint32_t task_stack_size = 0u;

    (void)unused;

    /* Create packet allocator */
    ret = NANO_IP_BSP_CreatePacketAllocator(&s_packet_allocator);
    if (ret)
    {
        /* Create network interface */
        ret = NANO_IP_BSP_CreateNetIf(&s_net_if, &net_if_name, &rx_packet_count, &rx_packet_size, &task_priority, &task_stack_size);
    }

    if (ret)
    {
        /* Initialize IP stack */
        err = NANO_IP_Init(&s_packet_allocator);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Add the network interface */
            err = NANO_IP_NET_IFACES_AddNetInterface(&s_net_if, net_if_name, rx_packet_count, rx_packet_size, task_priority, task_stack_size);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Start the IP stack */
                err = NANO_IP_Start();
            }
        }
    }
    
    if (err == NIP_ERR_SUCCESS)
    {
        /* Configure MAC address */
        err = NANO_IP_NET_IFACES_SetMacAddress(s_net_if.id, MAC_ADDRESS);
        if (err == NIP_ERR_SUCCESS)
        {

            #ifndef USE_DHCP
            
            /* Static IP address configuration */
            ipv4_address_t ipv4_address;
            ipv4_address_t ipv4_netmask;
            ipv4_address = NANO_IP_inet_ntoa(DEMO_IP_ADDRESS);
            ipv4_netmask = NANO_IP_inet_ntoa("255.255.255.0");
            err = NANO_IP_NET_IFACES_SetIpv4Address(s_net_if.id, ipv4_address, ipv4_netmask, 0);

            #endif /* USE_DHCP */

            /* Bring up network interface */
            err = NANO_IP_NET_IFACES_Up(s_net_if.id);

            #ifdef USE_DHCP
            
            if (err == NIP_ERR_SUCCESS)
            {
                /* Dynamic IP address configuration */
                static nano_ip_dhcp_client_t dhcp_client;

                err = NANO_IP_DHCP_CLIENT_Init(&dhcp_client, &s_net_if, DHCP_SERVER_STANDARD_PORT, DHCP_CLIENT_STANDARD_PORT, 10000u);
                if (err == NIP_ERR_SUCCESS)
                {
                    err = NANO_IP_DHCP_CLIENT_Start(&dhcp_client);
                }
            }
            
            #endif /* USE_DHCP */
        }
    }


    #if (UDP_ECHO_DEMO_ENABLED == 1u)

    if (err == NIP_ERR_SUCCESS)
    {
        /* Initialize UDP handle */
        err = NANO_IP_UDP_InitializeHandle(&s_udp_handle, DEMO_UDP_EventCallback, &s_udp_handle);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Bind handle */
            err = NANO_IP_UDP_Bind(&s_udp_handle, IPV4_ANY_ADDRESS, UDP_ECHO_DEMO_LISTEN_PORT);
        }

        #if (UDP_ECHO_DEMO_TASK_ENABLED == 1u)
        if (err == NIP_ERR_SUCCESS)
        {
            /* Create synchronization object */
            err = NANO_IP_OAL_FLAGS_Create(&s_udp_sync_object);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Create mutex */
                err = NANO_IP_OAL_MUTEX_Create(&s_udp_mutex_handle);
                if (err == NIP_ERR_SUCCESS)
                {
                    /* Create UDP task */
                    err = NANO_IP_OAL_TASK_Create(&s_udp_task_handle, "UDP demo task", DEMO_UDP_EchoRxTask, &s_udp_handle, UDP_ECHO_TASK_PRIORITY, UDP_ECHO_TASK_STACK_SIZE);
                }
            }
        }
        #endif /* UDP_ECHO_DEMO_TASK_ENABLED */
    }

    #endif /* UDP_ECHO_DEMO_ENABLED */



    #if (TCP_SERVER_ECHO_DEMO_ENABLED == 1u)

    if (err == NIP_ERR_SUCCESS)
    {
        /* Initialize TCP handle */
        err = NANO_IP_TCP_InitializeHandle(&s_tcp_server_handle, DEMO_TCP_ServerEventCallback, &s_tcp_server_handle);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Open handle */
            err = NANO_IP_TCP_Open(&s_tcp_server_handle, TCP_SERVER_ECHO_DEMO_LISTEN_PORT);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Enter listening state */
                err = NANO_IP_TCP_Listen(&s_tcp_server_handle);
                if (err == NIP_ERR_SUCCESS)
                {
                    /* Initialize client handles */
                    uint32_t i;
                    for (i = 0u; i < TCP_SERVER_ECHO_DEMO_MAX_CLIENTS; i++)
                    {
                        err |= NANO_IP_TCP_InitializeHandle(&s_tcp_server_client_handles[i], DEMO_TCP_ServerEventCallback, &s_tcp_server_client_handles[i]);
                    }
                }
            }
        }

        #if (TCP_SERVER_ECHO_DEMO_TASK_ENABLED == 1u)
        if (err == NIP_ERR_SUCCESS)
        {
            /* Create synchronization object */
            err = NANO_IP_OAL_FLAGS_Create(&s_udp_sync_object);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Create mutex */
                err = NANO_IP_OAL_MUTEX_Create(&s_udp_mutex_handle);
                if (err == NIP_ERR_SUCCESS)
                {
                    /* Create UDP task */
                    err = NANO_IP_OAL_TASK_Create(&s_udp_task_handle, "UDP demo task", DEMO_UDP_EchoRxTask, &s_udp_handle, TCP_SERVER_ECHO_TASK_PRIORITY, TCP_SERVER_ECHO_TASK_STACK_SIZE);
                }
            }
        }
        #endif /* TCP_SERVER_ECHO_DEMO_TASK_ENABLED */
    }

    #endif /* TCP_SERVER_ECHO_DEMO_ENABLED */



    #if (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u)

    if (err == NIP_ERR_SUCCESS)
    {
        /* Initialize TCP handle */
        err = NANO_IP_TCP_InitializeHandle(&s_tcp_client_handle, DEMO_TCP_ClientEventCallback, &s_tcp_client_handle);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Open handle */
            err = NANO_IP_TCP_Open(&s_tcp_client_handle, 0u);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Connect */
                err = NANO_IP_TCP_Connect(&s_tcp_client_handle, NANO_IP_inet_ntoa(TCP_CLIENT_ECHO_DEMO_DEST_ADDRESS), TCP_CLIENT_ECHO_DEMO_DEST_PORT);
                if (err == NIP_ERR_IN_PROGRESS)
                {
                    err = NIP_ERR_SUCCESS;
                }
            }
        }

        #if (TCP_CLIENT_ECHO_DEMO_TASK_ENABLED == 1u)
        if (err == NIP_ERR_SUCCESS)
        {
            /* Create synchronization object */
            err = NANO_IP_OAL_FLAGS_Create(&s_udp_sync_object);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Create mutex */
                err = NANO_IP_OAL_MUTEX_Create(&s_udp_mutex_handle);
                if (err == NIP_ERR_SUCCESS)
                {
                    /* Create UDP task */
                    err = NANO_IP_OAL_TASK_Create(&s_udp_task_handle, "UDP demo task", DEMO_UDP_EchoRxTask, &s_udp_handle, TCP_CLIENT_ECHO_TASK_PRIORITY, TCP_CLIENT_ECHO_TASK_STACK_SIZE);
                }
            }
        }
        #endif /* TCP_CLIENT_ECHO_DEMO_TASK_ENABLED */
    }

    #endif /* TCP_CLIENT_ECHO_DEMO_ENABLED */


    /* Check initialization errors */
    if (err != NIP_ERR_SUCCESS)
    {
        NANO_IP_LOG_ERROR("main() : Error %d during initalization", NANO_IP_CAST(int32_t, err));
    }

    #ifndef NANO_IP_OAL_OS_LESS
    /* Init is done, sleep forever */
    while (true)
    {
        NANO_IP_OAL_TASK_Sleep(NANO_IP_MAX_TIMEOUT_VALUE);
    }
    #endif /* NANO_IP_OAL_OS_LESS */

}






#if (UDP_ECHO_DEMO_ENABLED == 1u)

/** \brief Send an UDP message */
static nano_ip_error_t DEMO_UDP_SendMessage(nano_ip_udp_handle_t* const udp_handle, const ipv4_address_t address, const uint16_t port, const void* const data, const uint16_t size)
{
    nano_ip_error_t ret;
    nano_ip_net_packet_t* packet = NULL;

    /* Allocate packet */
    ret = NANO_IP_UDP_AllocatePacket(&packet, size);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Copy data */
        (void)NANO_IP_MEMCPY(packet->current, data, size);
        packet->current += size;
        packet->count = size;

        /* Send packet */
        ret = NANO_IP_UDP_SendPacket(udp_handle, address, port, packet);
        if ((ret != NIP_ERR_SUCCESS) && (ret != NIP_ERR_IN_PROGRESS))
        {
            (void)NANO_IP_UDP_ReleasePacket(packet);
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}

/** \brief UDP event callback */
static bool DEMO_UDP_EventCallback(void* const user_data, const nano_ip_udp_event_t event, const nano_ip_udp_event_data_t* const event_data)
{
    bool release_packet = true;

    /* Check event */
    switch (event)
    {
        case UDP_EVENT_RX:
        {
            udp_header_t* const udp_header = event_data->udp_header;
            nano_ip_net_packet_t* const packet = event_data->packet;

            #if (UDP_ECHO_DEMO_TASK_ENABLED == 1u)
            
            /* Get mutex */
            (void)NANO_IP_OAL_MUTEX_Lock(&s_udp_mutex_handle);

            /* Save received data */
            s_udp_send_address = udp_header->ipv4_header->src_address;
            s_udp_send_port = udp_header->src_port;
            if (s_udp_rx_packets == NULL)
            {
                s_udp_rx_packets = packet;
            }
            else
            {
                nano_ip_net_packet_t* previous_packet = s_udp_rx_packets;
                while (previous_packet->next != NULL)
                {
                    previous_packet = previous_packet->next;
                }
                previous_packet->next = packet;
            }
            packet->next = NULL;

            /* Notify echo task */
            (void)NANO_IP_OAL_FLAGS_Set(&s_udp_sync_object, 1u, false);

            /* Ask IP stack to not deallocate packet */
            release_packet = false;

            /* Release mutex */
            (void)NANO_IP_OAL_MUTEX_Unlock(&s_udp_mutex_handle);
            
            #else

            nano_ip_udp_handle_t* const udp_handle = NANO_IP_CAST(nano_ip_udp_handle_t*, user_data);

            /* Echo received data */
            nano_ip_error_t ret = DEMO_UDP_SendMessage(udp_handle, udp_header->ipv4_header->src_address, udp_header->src_port, packet->current, packet->count);
            if (ret != NIP_ERR_SUCCESS)
            {
                NANO_IP_LOG_ERROR("DEMO_UDP_EventCallback() : Send error %d", NANO_IP_CAST(int32_t, ret));
            }

            #endif /* UDP_ECHO_DEMO_TASK_ENABLED */

            break;
        }

        case UDP_EVENT_TX:
        {
            NANO_IP_LOG_INFO("DEMO_UDP_EventCallback() : transmit complete");
            break;
        }

        default:
        {
            /* Error */
            NANO_IP_LOG_ERROR("DEMO_UDP_EventCallback() : Error %d", NANO_IP_CAST(int32_t, event_data->error));
            break;
        }
    }

    return release_packet;
}


#if (UDP_ECHO_DEMO_TASK_ENABLED == 1u)

/** \brief UDP echo task */
static void DEMO_UDP_EchoRxTask(void* param)
{
    #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
    while (true)
    #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
    {
        nano_ip_error_t ret;
        uint32_t mask = NANO_IP_OAL_FLAGS_ALL;

        /* Wait for a notification */
        ret = NANO_IP_OAL_FLAGS_Wait(&s_udp_sync_object, &mask, true, NANO_IP_MAX_TIMEOUT_VALUE);
        if ((ret != NIP_ERR_TIMEOUT) && (mask != NANO_IP_OAL_FLAGS_NONE))
        {
            /* Get mutex */
            (void)NANO_IP_OAL_MUTEX_Lock(&s_udp_mutex_handle);

            do
            {
                nano_ip_udp_handle_t* const udp_handle = NANO_IP_CAST(nano_ip_udp_handle_t*, param);

                /* Echo received packet */
                ret = DEMO_UDP_SendMessage(udp_handle, s_udp_send_address, s_udp_send_port, s_udp_rx_packets->current, s_udp_rx_packets->count);
                if (ret != NIP_ERR_SUCCESS)
                {
                    NANO_IP_LOG_ERROR("DEMO_UDP_EchoRxTask() : Send error %d", NANO_IP_CAST(int32_t, ret));
                }

                /* Release packet */
                (void)NANO_IP_UDP_ReleasePacket(s_udp_rx_packets);

                /* Next packet */
                s_udp_rx_packets = s_udp_rx_packets->next;
            }
            while (s_udp_rx_packets != NULL);

            /* Release mutex */
            (void)NANO_IP_OAL_MUTEX_Unlock(&s_udp_mutex_handle);
        }
    }
}

#endif /* UDP_ECHO_DEMO_TASK_ENABLED */

#endif /* UDP_ECHO_DEMO_ENABLED */



#if ((TCP_SERVER_ECHO_DEMO_ENABLED == 1u) || (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u))

/** \brief Send a TCP message */
static nano_ip_error_t DEMO_TCP_SendMessage(nano_ip_tcp_handle_t* const tcp_handle, const void* const data, const uint16_t size)
{
    nano_ip_error_t ret;
    nano_ip_net_packet_t* packet = NULL;

    /* Allocate packet */
    ret = NANO_IP_TCP_AllocatePacket(&packet, size);
    if (ret == NIP_ERR_SUCCESS)
    {
        /* Copy data */
        (void)NANO_IP_MEMCPY(packet->current, data, size);
        packet->current += size;
        packet->count = size;

        /* Send packet */
        ret = NANO_IP_TCP_SendPacket(tcp_handle, packet);
        if ((ret != NIP_ERR_SUCCESS) && (ret != NIP_ERR_IN_PROGRESS))
        {
            (void)NANO_IP_TCP_ReleasePacket(packet);
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}

#endif /* TCP_SERVER_ECHO_DEMO_ENABLED || TCP_CLIENT_ECHO_DEMO_ENABLED */



#if (TCP_SERVER_ECHO_DEMO_ENABLED == 1u)

/** \brief TCP server event callback */
static bool DEMO_TCP_ServerEventCallback(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data)
{
    bool release_packet = true;

    switch (event)
    {
        case TCP_EVENT_RX:
        {
            nano_ip_error_t ret;
            nano_ip_net_packet_t* const packet = event_data->packet;
            nano_ip_tcp_handle_t* const tcp_handle = NANO_IP_CAST(nano_ip_tcp_handle_t*, user_data);

            /* Echo received data */
            ret = DEMO_TCP_SendMessage(tcp_handle, packet->current, packet->count);
            if (ret != NIP_ERR_SUCCESS)
            {
                NANO_IP_LOG_ERROR("DEMO_TCP_ServerEventCallback() : Send error %d", NANO_IP_CAST(int32_t, ret));
            }
            break;
        }

        case TCP_EVENT_TX:
        {
            break;
        }

        case TCP_EVENT_TX_FAILED:
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_ServerEventCallback() : Data failed to send");
            break;
        }

        case TCP_EVENT_ACCEPTING:
        {
            uint32_t i;

            NANO_IP_LOG_INFO("DEMO_TCP_ServerEventCallback() : Accepting new client");

            /* Look for a free TCP handle */
            i = 0;
            while ((s_tcp_server_client_handles[i].state != TCP_STATE_CLOSED) && 
                   (i < TCP_SERVER_ECHO_DEMO_MAX_CLIENTS))
            {
                i++;
            }
            if (i < TCP_SERVER_ECHO_DEMO_MAX_CLIENTS)
            {
                /* Open the handle */
                const nano_ip_error_t ret = NANO_IP_TCP_Open(&s_tcp_server_client_handles[i], 0u);
                if (ret == NIP_ERR_SUCCESS)
                {
                    /* Provide handle to the stack */
                    (*event_data->accept_handle) = &s_tcp_server_client_handles[i];
                }
            }
            break;
        }

        case TCP_EVENT_ACCEPTED:
        {
            NANO_IP_LOG_INFO("DEMO_TCP_ServerEventCallback() : New client accepted");
            break;
        }

        case TCP_EVENT_ACCEPT_FAILED:
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_ServerEventCallback() : Failed to accept new client");
            break;
        }

        case TCP_EVENT_CLOSED:
        {
            NANO_IP_LOG_INFO("DEMO_TCP_ServerEventCallback() : Connection closed");
            break;
        }

        default:
        {
            /* Error */
            NANO_IP_LOG_ERROR("DEMO_TCP_ServerEventCallback() : Event %d - Error %d", NANO_IP_CAST(int32_t, event), NANO_IP_CAST(int32_t, event_data->error));
            break;
        }
    }

    return release_packet;
}


#if (TCP_SERVER_ECHO_DEMO_TASK_ENABLED == 1u)


#endif /* TCP_SERVER_ECHO_DEMO_TASK_ENABLED */

#endif /* TCP_SERVER_ECHO_DEMO_ENABLED */



#if (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u)


/** \brief TCP client event callback */
static bool DEMO_TCP_ClientEventCallback(void* const user_data, const nano_ip_tcp_event_t event, const nano_ip_tcp_event_data_t* const event_data)
{
    bool release_packet = true;

    switch (event)
    {
        case TCP_EVENT_RX:
        {
            nano_ip_error_t ret;
            nano_ip_net_packet_t* const packet = event_data->packet;
            nano_ip_tcp_handle_t* const tcp_handle = NANO_IP_CAST(nano_ip_tcp_handle_t*, user_data);

            /* Echo received data */
            ret = DEMO_TCP_SendMessage(tcp_handle, packet->current, packet->count);
            if (ret != NIP_ERR_SUCCESS)
            {
                NANO_IP_LOG_ERROR("DEMO_TCP_ClientEventCallback() : Send error %d", NANO_IP_CAST(int32_t, ret));
            }
            break;
        }

        case TCP_EVENT_TX:
        {
            break;
        }

        case TCP_EVENT_TX_FAILED:
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_ClientEventCallback() : Data failed to send");
            break;
        }

        case TCP_EVENT_CONNECTED:
        {
            NANO_IP_LOG_INFO("DEMO_TCP_ClientEventCallback() : Connected");
            break;
        }

        case TCP_EVENT_CONNECT_TIMEOUT:
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_ClientEventCallback() : Connect timeout");
            break;
        }

        case TCP_EVENT_CLOSED:
        {
            NANO_IP_LOG_INFO("DEMO_TCP_ClientEventCallback() : Connection closed");
            break;
        }

        default:
        {
            /* Error */
            NANO_IP_LOG_ERROR("DEMO_TCP_ClientEventCallback() : Event %d - Error %d", NANO_IP_CAST(int32_t, event), NANO_IP_CAST(int32_t, event_data->error));
            break;
        }
    }

    return release_packet;
}


#if (TCP_CLIENT_ECHO_DEMO_TASK_ENABLED == 1u)


#endif /* TCP_CLIENT_ECHO_DEMO_TASK_ENABLED */

#endif /* TCP_CLIENT_ECHO_DEMO_ENABLED */


