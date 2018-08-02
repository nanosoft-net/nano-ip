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
#define DEMO_PING_IP_ADDRESS        "192.168.137.70"

/** \brief MAC address */
static const uint8_t MAC_ADDRESS[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

/** \brief Uncomment the following line to enable non-blocking sockets */
#define USE_NON_BLOCKING_SOCKETS


/************************** UDP demo options ***************************/

/** \brief Enable or disable the UDP echo demo */
#define UDP_ECHO_DEMO_ENABLED                   1u

/** \brief UDP listen port */
#define UDP_ECHO_DEMO_LISTEN_PORT               54321u

/** \brief Maximum size in byte of echoed datagrams */
#define UDP_ECHO_DEMO_DATAGRAM_MAX_SIZE         1024u


/************************** TCP server demo options ***************************/

/** \brief Enable or disable the TCP server echo demo */
#define TCP_SERVER_ECHO_DEMO_ENABLED            1u

/** \brief TCP listen port */
#define TCP_SERVER_ECHO_DEMO_LISTEN_PORT        8765u

/** \brief Maximum number of simultaneously connected clients */
#define TCP_SERVER_ECHO_DEMO_MAX_CLIENTS        3u


/************************** TCP client demo options ***************************/

/** \brief Enable or disable the TCP client echo demo */
#define TCP_CLIENT_ECHO_DEMO_ENABLED            1u

/** \brief TCP client destination address */
#define TCP_CLIENT_ECHO_DEMO_DEST_ADDRESS      "192.168.137.106"

/** \brief TCP client destination port */
#define TCP_CLIENT_ECHO_DEMO_DEST_PORT          4567u






/********************** Demo's local variables *************************/


/** \brief Network interface handle */
static nano_ip_net_if_t s_net_if;

/** \brief Packet allocator handle */
static nano_ip_net_packet_allocator_t s_packet_allocator;


#if (UDP_ECHO_DEMO_ENABLED == 1u)

/** \brief UDP socket */
static uint32_t s_udp_socket;

/** \brief UDP echo task handle */
static oal_task_t s_udp_task_handle;

/** \brief Buffer to receive echoed data */
static uint8_t s_udp_echo_buffer[UDP_ECHO_DEMO_DATAGRAM_MAX_SIZE];

/** \brief UDP echo task */
static void DEMO_UDP_EchoRxTask(void* param);

#endif /* UDP_ECHO_DEMO_ENABLED */




#if (TCP_SERVER_ECHO_DEMO_ENABLED == 1u)

/** \brief TCP server socket */
static uint32_t s_tcp_server_socket;

/** \brief TCP server task handle */
static oal_task_t s_tcp_server_task_handle;

/** \brief TCP server echo task */
static void DEMO_TCP_SERVER_Task(void* param);

#endif /* TCP_SERVER_ECHO_DEMO_ENABLED */





#if (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u)

/** \brief TCP client socket */
static uint32_t s_tcp_client_socket;

/** \brief TCP client task handle */
static oal_task_t s_tcp_client_task_handle;

/** \brief TCP client echo task */
static void DEMO_TCP_CLIENT_EchoTask(void* param);

#endif /* TCP_CLIENT_ECHO_DEMO_ENABLED */






/** \brief Application entry point */
int main(void)
{
    bool ret = false;
    nano_ip_error_t err = NIP_ERR_FAILURE;

    const char* net_if_name = NULL;
    uint32_t rx_packet_size = 0u;
    uint32_t rx_packet_count = 0u;

    /* Initialize operating system */
    ret = NANO_IP_BSP_OSInit();
    if (ret)
    {
        /* Create packet allocator */
        ret = NANO_IP_BSP_CreatePacketAllocator(&s_packet_allocator);
        if (ret)
        {
            /* Create network interface */
            ret = NANO_IP_BSP_CreateNetIf(&s_net_if, &net_if_name, &rx_packet_count, &rx_packet_size);
        }
    }
    if (ret)
    {
        /* Initialize IP stack */
        err = NANO_IP_Init(&s_packet_allocator);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Add the network interface */
            err = NANO_IP_NET_IFACES_AddNetInterface(&s_net_if, net_if_name, rx_packet_count, rx_packet_size);
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
            ipv4_address = NANO_IP_inet_ntoa(DEMO_PING_IP_ADDRESS);
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
        /* Initialize UDP socket */
        err = NANO_IP_SOCKET_Allocate(&s_udp_socket, NIPSOCK_UDP);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Bind socket */
            nano_ip_socket_endpoint_t end_point;
            end_point.address = IPV4_ANY_ADDRESS;
            end_point.port = UDP_ECHO_DEMO_LISTEN_PORT;
            err = NANO_IP_SOCKET_Bind(s_udp_socket, &end_point);
            if (err == NIP_ERR_SUCCESS)
            {
                /* Create UDP task */
                err = NANO_IP_OAL_TASK_Create(&s_udp_task_handle, "UDP demo task", DEMO_UDP_EchoRxTask, &s_udp_socket);
            }

            #ifdef USE_NON_BLOCKING_SOCKETS
            if (err == NIP_ERR_SUCCESS)
            {
                err = NANO_IP_SOCKET_SetNonBlocking(s_udp_socket, true);
            }
            #endif /* USE_NON_BLOCKING_SOCKETS */
        }
    }

    #endif /* UDP_ECHO_DEMO_ENABLED */



    #if (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u)

    if (err == NIP_ERR_SUCCESS)
    {
        /* Initialize TCP socket */
        err = NANO_IP_SOCKET_Allocate(&s_tcp_client_socket, NIPSOCK_TCP);
        if (err == NIP_ERR_SUCCESS)
        {
            /* Create TCP client task */
            err = NANO_IP_OAL_TASK_Create(&s_tcp_client_task_handle, "TCP client demo task", DEMO_TCP_CLIENT_EchoTask, &s_tcp_client_socket);

            #ifdef USE_NON_BLOCKING_SOCKETS
            if (err == NIP_ERR_SUCCESS)
            {
                err = NANO_IP_SOCKET_SetNonBlocking(s_tcp_client_socket, true);
            }
            #endif /* USE_NON_BLOCKING_SOCKETS */
        }
    }

    #endif /* TCP_CLIENT_ECHO_DEMO_ENABLED */



    /* Check initialization errors */
    if (err != NIP_ERR_SUCCESS)
    {
        NANO_IP_LOG_ERROR("main() : Error %d during initalization", NANO_IP_CAST(int32_t, err));
    }

    /* Start the operating system */
    NANO_IP_BSP_OSStart();

    return 0;
}




#if (UDP_ECHO_DEMO_ENABLED == 1u)

/** \brief UDP echo task */
static void DEMO_UDP_EchoRxTask(void* param)
{
    (void)param;

    #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
    while (true)
    #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
    {
        nano_ip_error_t ret;
        size_t received = 0u;
        nano_ip_socket_endpoint_t end_point;

        #ifdef USE_NON_BLOCKING_SOCKETS

        /* Poll on socket to wait for incomming data */
        bool data_ready = false;
        uint32_t poll_count = 0u;
        nano_ip_socket_poll_data_t poll_data;
        MEMSET(&poll_data, 0, sizeof(poll_data));
        poll_data.socket_id = s_udp_socket;
        poll_data.req_events = NIPSOCK_POLLIN | NIPSOCK_POLLERR;
        
        ret = NANO_IP_SOCKET_Poll(&poll_data, 1u, NANO_IP_MAX_TIMEOUT_VALUE, &poll_count); 
        if (ret == NIP_ERR_SUCCESS)
        {
            if ((poll_data.ret_events & NIPSOCK_POLLIN) != 0u)
            {
                data_ready = true;
            }
            if ((poll_data.ret_events & NIPSOCK_POLLERR) != 0u)
            {
                NANO_IP_LOG_ERROR("DEMO_UDP_EchoRxTask() : Error pending on socket");
            }
        }
        else
        {
            if (ret != NIP_ERR_TIMEOUT)
            {
                NANO_IP_LOG_ERROR("DEMO_UDP_EchoRxTask() : Error %d while polling socket", NANO_IP_CAST(int32_t, ret));
            }
        }
        
        if (data_ready)
        {
        #endif /* USE_NON_BLOCKING_SOCKETS */

        /* Receive data from the socket */
        ret = NANO_IP_SOCKET_ReceiveFrom(s_udp_socket, s_udp_echo_buffer, sizeof(s_udp_echo_buffer), &received, &end_point);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Send back received data */
            size_t sent = 0u;
            ret = NANO_IP_SOCKET_SendTo(s_udp_socket, s_udp_echo_buffer, received, &sent, &end_point);
            if (ret != NIP_ERR_SUCCESS)
            {
                if (ret != NIP_ERR_IN_PROGRESS)
                {
                    NANO_IP_LOG_ERROR("DEMO_UDP_EchoRxTask() : Error %d while sending data", NANO_IP_CAST(int32_t, ret));
                }
            }
        }
        else
        {
            if (ret != NIP_ERR_TIMEOUT)
            {
                NANO_IP_LOG_ERROR("DEMO_UDP_EchoRxTask() : Error %d while receiving data", NANO_IP_CAST(int32_t, ret));
            }
        }

        #ifdef USE_NON_BLOCKING_SOCKETS
        }
        #endif /* USE_NON_BLOCKING_SOCKETS */
    }
}

#endif /* UDP_ECHO_DEMO_ENABLED */


#if (TCP_CLIENT_ECHO_DEMO_ENABLED == 1u)


/** \brief Echo data on a TCP socket */
static bool DEMO_TCP_EchoData(const uint32_t socket_id)
{
    nano_ip_error_t ret;
    size_t received = 0u;
    bool disconnected = false;
    uint8_t echo_buffer[16u];

    #ifdef USE_NON_BLOCKING_SOCKETS

    /* Poll on socket to wait for incoming data */
    bool data_ready = false;
    uint32_t poll_count = 0u;
    nano_ip_socket_poll_data_t poll_data;
    MEMSET(&poll_data, 0, sizeof(poll_data));
    poll_data.socket_id = socket_id;
    poll_data.req_events = NIPSOCK_POLLIN | NIPSOCK_POLLERR;
    
    ret = NANO_IP_SOCKET_Poll(&poll_data, 1u, NANO_IP_MAX_TIMEOUT_VALUE, &poll_count); 
    if (ret == NIP_ERR_SUCCESS)
    {
        if ((poll_data.ret_events & NIPSOCK_POLLIN) != 0u)
        {
            data_ready = true;
        }
        if ((poll_data.ret_events & NIPSOCK_POLLERR) != 0u)
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_EchoData() : Error pending on socket");
            disconnected = true;
        }
    }
    else
    {
        if (ret != NIP_ERR_TIMEOUT)
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_EchoData() : Error %d while polling socket", NANO_IP_CAST(int32_t, ret));
            disconnected = true;
        }
    }
    
    if (data_ready)
    {
    #endif /* USE_NON_BLOCKING_SOCKETS */

    /* Receive data from the socket */
    do
    {
        ret = NANO_IP_SOCKET_Receive(socket_id, echo_buffer, sizeof(echo_buffer), &received);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Send back received data */
            size_t sent = 0u;
            ret = NANO_IP_SOCKET_Send(socket_id, echo_buffer, received, &sent);
            if (ret != NIP_ERR_SUCCESS)
            {
                NANO_IP_LOG_ERROR("DEMO_TCP_EchoData() : Error %d while sending data", NANO_IP_CAST(int32_t, ret));
                disconnected = true;
            }
        }
        else if (ret == NIP_ERR_IN_PROGRESS)
        {
            /* No more data available */
        }
        else
        {
            NANO_IP_LOG_ERROR("DEMO_TCP_EchoData() : Error %d while receiving data", NANO_IP_CAST(int32_t, ret));
            disconnected = true;
        }
    }
    while (ret == NIP_ERR_SUCCESS);

    #ifdef USE_NON_BLOCKING_SOCKETS
    }
    #endif /* USE_NON_BLOCKING_SOCKETS */

    return (!disconnected);
}



/** \brief TCP client echo task */
static void DEMO_TCP_CLIENT_EchoTask(void* param)
{
    static bool client_connected = false;
    static bool client_connecting = false;

    (void)param;

    #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
    while (true)
    #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
    {
        nano_ip_error_t ret;

        /* Check if client is connected */
        if (!client_connected)
        {
            if (!client_connecting)
            {
                nano_ip_socket_endpoint_t end_point;

                /* Send connect request */
                end_point.address = NANO_IP_inet_ntoa(TCP_CLIENT_ECHO_DEMO_DEST_ADDRESS);
                end_point.port = TCP_CLIENT_ECHO_DEMO_DEST_PORT;
                ret = NANO_IP_SOCKET_Connect(s_tcp_client_socket, &end_point);
                if ((ret == NIP_ERR_SUCCESS) || (ret == NIP_ERR_IN_PROGRESS))
                {
                    #ifdef USE_NON_BLOCKING_SOCKETS
                    client_connecting = true;
                    #else
                    client_connected = true;
                    NANO_IP_LOG_ERROR("DEMO_TCP_CLIENT_EchoTask() : Connected");
                    #endif /* USE_NON_BLOCKING_SOCKETS */
                }
            }
            else
            {
                #ifdef USE_NON_BLOCKING_SOCKETS

                /* Poll on socket to wait for end of connect data */
                uint32_t poll_count = 0u;
                nano_ip_socket_poll_data_t poll_data;
                MEMSET(&poll_data, 0, sizeof(poll_data));
                poll_data.socket_id = s_tcp_client_socket;
                poll_data.req_events = NIPSOCK_POLLOUT | NIPSOCK_POLLERR;
                
                client_connecting = false;
                ret = NANO_IP_SOCKET_Poll(&poll_data, 1u, NANO_IP_MAX_TIMEOUT_VALUE, &poll_count); 
                if (ret == NIP_ERR_SUCCESS)
                {
                    if ((poll_data.ret_events & NIPSOCK_POLLOUT) != 0u)
                    {
                        client_connected = true;
                        NANO_IP_LOG_ERROR("DEMO_TCP_CLIENT_EchoTask() : Connected");
                    }
                    if ((poll_data.ret_events & NIPSOCK_POLLERR) != 0u)
                    {
                        NANO_IP_LOG_ERROR("DEMO_TCP_CLIENT_EchoTask() : Connect failed");
                    }
                }
                else
                {
                    if (ret != NIP_ERR_TIMEOUT)
                    {
                        NANO_IP_LOG_ERROR("DEMO_TCP_CLIENT_EchoTask() : Error %d while polling socket", NANO_IP_CAST(int32_t, ret));
                    }
                    else
                    {
                        client_connecting = true;
                    }
                }
                
                #endif /* USE_NON_BLOCKING_SOCKETS */
            }
        }
        else
        {
            /* Echo received data */
            if (!DEMO_TCP_EchoData(s_tcp_client_socket))
            {
                NANO_IP_LOG_ERROR("DEMO_TCP_CLIENT_EchoTask() : Disconnected");
                client_connected = false;
                (void)NANO_IP_SOCKET_Release(s_tcp_client_socket);
                (void)NANO_IP_SOCKET_Allocate(&s_tcp_client_socket, NIPSOCK_TCP);
            }
        }
    }
}

#endif /* TCP_CLIENT_ECHO_DEMO_ENABLED */
