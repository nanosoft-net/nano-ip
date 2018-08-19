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

#include <stdio.h>

#ifdef WINDOWS
#define HAVE_REMOTE
#endif /* WINDOWS */
#include <pcap.h>

#include "nano_ip_pcap.h"

#include "nano_ip_tools.h"
#include "nano_ip_oal.h"
#include "nano_ip_log.h"
#include "nano_ip_packet_funcs.h"


/** \brief pcap driver data */
typedef struct _pcap_drv_t
{
    /** \brief Interface's name */
    const char* name;
    /** \brief Callbacks */
    net_driver_callbacks_t callbacks;
    /** \brief List of available rx packets */
    nano_ip_packet_queue_t rx_packets;
    /** \brief List of received packets */
    nano_ip_packet_queue_t received_packets;
    /** \brief List of transmitted packets */
    nano_ip_packet_queue_t transmitted_packets;
    /** \brief Interface's mutex */
    oal_mutex_t mutex;
    /** \brief Task */
    oal_task_t task;
    /** \brief pcap handle */
    pcap_t* pcap;
    /** \brief Network interface driver */
    nano_ip_net_driver_t* driver;
} pcap_drv_t;



/** \brief Init the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks);
/** \brief Start the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvStart(void* const user_data);
/** \brief Stop the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvStop(void* const user_data);
/** \brief Set the MAC address */
static nano_ip_error_t NANO_IP_PCAP_DrvSetMacAddress(void* const user_data, const uint8_t* const mac_address);
/** \brief Set the IPv4 address */
static nano_ip_error_t NANO_IP_PCAP_DrvSetIPv4Address(void* const user_data, const ipv4_address_t ipv4_address, const ipv4_address_t ipv4_netmask);
/** \brief Send a packet on the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Add a packet for reception for the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Get the last received packet on the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvGetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the last sent packet on the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvGetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the link state of the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvGetLinkState(void* const user_data, net_link_state_t* const state);


/** \brief Receive callback */
static void NANO_IP_PCAP_DrvPcapHandler(u_char *user, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data);
/** \brief Receive task */
static void NANO_IP_PCAP_DrvRxTask(void* param);



/** \brief Initialize pcap interface */
nano_ip_error_t NANO_IP_PCAP_Init(nano_ip_net_if_t* const pcap_iface, const char* iface_name)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (pcap_iface != NULL)
    {
        /* Check if interface name has been specified */
        if (iface_name == NULL)
        {
            uint32_t i = 0;
            uint32_t inum = 0;
            char errbuf[PCAP_ERRBUF_SIZE];
            pcap_if_t *alldevs, *d;

            printf("\nNo adapter selected: printing the device list:\n");
            /* The user didn't provide a packet source: Retrieve the local device list */
            if (pcap_findalldevs(&alldevs, errbuf) == -1)
            {
                NANO_IP_LOG_ERROR("Error in pcap_findalldevs_ex: %s\n", errbuf);
                ret = NIP_ERR_FAILURE;
            }
            else
            {
                /* Print the list */
                for (d = alldevs; d; d = d->next)
                {
                    printf("%d. %s\n    ", ++i, d->name);
                    if (d->description)
                    {
                        printf(" (%s)\n", d->description);
                    }
                    else
                    {
                        printf(" (No description available)\n");
                    }
                }
                if (i == 0)
                {
                    NANO_IP_LOG_ERROR("No interfaces found! Exiting.\n");
                    ret = NIP_ERR_RESOURCE;
                }
                else
                {
                    printf("Enter the interface number (1-%d):", i);
                    scanf("%d", &inum);

                    if (inum < 1 || inum > i)
                    {
                        printf("\nInterface number out of range.\n");
                        ret = NIP_ERR_FAILURE;
                    }
                    else
                    {
                        /* Jump to the selected adapter */
                        for (d = alldevs, i = 0; i< inum - 1; d = d->next, i++);

                        /* Save interface name */
                        char* name = NANO_IP_CAST(char*, malloc(strlen(d->name) + 1u));
                        if (name != NULL)
                        {
                            strcpy(name, d->name);
                            iface_name = name;
                        }
                        else
                        {
                            ret = NIP_ERR_RESOURCE;
                        }
                    }
                }

                /* Free the device list */
                pcap_freealldevs(alldevs);
            }
        }

        if (iface_name != NULL)
        {
            /* Allocate a driver instance */
            pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, malloc(sizeof(pcap_drv_t)));
            if (pcap_drv_inst != NULL)
            {
                /* 0 init */
                NANO_IP_MEMSET(pcap_drv_inst, 0, sizeof(pcap_drv_t));

                /* ALlocate driver */
                pcap_drv_inst->driver = NANO_IP_CAST(nano_ip_net_driver_t*, malloc(sizeof(nano_ip_net_driver_t)));
                if (pcap_drv_inst->driver != NULL)
                {
                    /* 0 init */
                    NANO_IP_MEMSET(pcap_drv_inst->driver, 0, sizeof(nano_ip_net_driver_t));

                    /* Init driver interface */
                    pcap_drv_inst->name = iface_name;
                    pcap_drv_inst->driver->user_data = pcap_drv_inst;
                    pcap_drv_inst->driver->caps = NETDRV_CAPS_ETH_MIN_FRAME_SIZE | 
                                                  NETDRV_CAP_ETH_CS_COMPUTATION |
                                                  NETDRV_CAP_ETH_CS_CHECK |
                                                  NETDRV_CAP_ETH_FRAME_PADDING |
                                                  NETDRV_CAP_IPV4_CS_CHECK |
                                                  NETDRV_CAP_UDPIPV4_CS_CHECK |
                                                  NETDRV_CAP_TCPIPV4_CS_CHECK;

                    pcap_drv_inst->driver->init = NANO_IP_PCAP_DrvInit;
                    pcap_drv_inst->driver->start = NANO_IP_PCAP_DrvStart;
                    pcap_drv_inst->driver->stop = NANO_IP_PCAP_DrvStop;
                    pcap_drv_inst->driver->set_mac_address = NANO_IP_PCAP_DrvSetMacAddress;
                    pcap_drv_inst->driver->set_ipv4_address = NANO_IP_PCAP_DrvSetIPv4Address;
                    pcap_drv_inst->driver->send_packet = NANO_IP_PCAP_DrvSendPacket;
                    pcap_drv_inst->driver->add_rx_packet = NANO_IP_PCAP_DrvAddRxPacket;
                    pcap_drv_inst->driver->get_next_rx_packet = NANO_IP_PCAP_DrvGetNextRxPacket;
                    pcap_drv_inst->driver->get_next_tx_packet = NANO_IP_PCAP_DrvGetNextTxPacket;
                    pcap_drv_inst->driver->get_link_state = NANO_IP_PCAP_DrvGetLinkState;
                    pcap_iface->driver = pcap_drv_inst->driver;

                    /* Create the interface's mutex */
                    ret = NANO_IP_OAL_MUTEX_Create(&pcap_drv_inst->mutex);
                }
                else
                {
                    free(pcap_drv_inst);
                    ret = NIP_ERR_RESOURCE;
                }
            }
            else
            {
                ret = NIP_ERR_RESOURCE;
            }
        }
    }

    return ret;
}


/** \brief Init the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if (pcap_drv_inst != NULL)
    {
        (void)NANO_IP_MEMCPY(&pcap_drv_inst->callbacks, callbacks, sizeof(net_driver_callbacks_t));
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Start the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvStart(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if (pcap_drv_inst != NULL)
    {
        /* Initialize pcap driver */
        char errbuf[PCAP_ERRBUF_SIZE];

        #ifdef LINUX
        pcap_drv_inst->pcap = pcap_open_live(pcap_drv_inst->name, 65535, 1, 5, errbuf);
        #else
        pcap_drv_inst->pcap = pcap_open(pcap_drv_inst->name, 65535, PCAP_OPENFLAG_PROMISCUOUS | PCAP_OPENFLAG_NOCAPTURE_LOCAL, 5, NULL, errbuf);
        #endif /* LINUX */

        if (pcap_drv_inst->pcap == NULL)
        {
            NANO_IP_LOG_ERROR("\nError opening adapter => %s\n", errbuf);
            ret = NIP_ERR_FAILURE;
        }
        else
        {
            /* Create receive task */
            ret = NANO_IP_OAL_TASK_Create(&pcap_drv_inst->task, "PCAP Driver Task", NANO_IP_PCAP_DrvRxTask, pcap_drv_inst, 0u, 0u);
            if (ret != NIP_ERR_SUCCESS)
            {
                pcap_close(pcap_drv_inst->pcap);
            }
        }
    }

    return ret;
}

/** \brief Stop the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvStop(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if (pcap_drv_inst != NULL)
    {
        /* Stop packet reception */
        pcap_breakloop(pcap_drv_inst->pcap);

        /* Close pcap driver */
        pcap_close(pcap_drv_inst->pcap);

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Set the MAC address */
static nano_ip_error_t NANO_IP_PCAP_DrvSetMacAddress(void* const user_data, const uint8_t* const mac_address)
{
    /* No need to configure MAC address for libcap */
    (void)user_data;
    (void)mac_address;
    return NIP_ERR_SUCCESS;
}

/** \brief Set the IPv4 address */
static nano_ip_error_t NANO_IP_PCAP_DrvSetIPv4Address(void* const user_data, const ipv4_address_t ipv4_address, const ipv4_address_t ipv4_netmask)
{
    /* No need to configure IPV4 address for libcap */
    (void)user_data;
    (void)ipv4_address;
    (void)ipv4_netmask;
    return NIP_ERR_SUCCESS;
}

/** \brief Send a packet on the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if ((pcap_drv_inst != NULL) && (packet != NULL))
    {
        /* Send packet */
        int pcap_ret = pcap_sendpacket(pcap_drv_inst->pcap, packet->data, packet->count);
        if (pcap_ret == 0)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
        }

        /* Lock interface */
        (void)NANO_IP_OAL_MUTEX_Lock(&pcap_drv_inst->mutex);

        /* Add packet to the transmitted packet list */
        NANO_IP_PACKET_AddToQueue(&pcap_drv_inst->transmitted_packets, packet);

        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);

        /* Notify packet transmission */
        pcap_drv_inst->callbacks.packet_sent(pcap_drv_inst->callbacks.stack_data, false);
    }

    return ret;
}

/** \brief Add a packet for reception for the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if ((pcap_drv_inst != NULL) && (packet != NULL))
    {
        /* Lock interface */
        (void)NANO_IP_OAL_MUTEX_Lock(&pcap_drv_inst->mutex);

        /* Add packet to the list */
        NANO_IP_PACKET_AddToQueue(&pcap_drv_inst->rx_packets, packet);

        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Get the last received packet on the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvGetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if ((pcap_drv_inst != NULL) && (packet != NULL))
    {
        /* Lock interface */
        (void)NANO_IP_OAL_MUTEX_Lock(&pcap_drv_inst->mutex);

        /* Remove last received packet from the list */
        (*packet) = NANO_IP_PACKET_PopFromQueue(&pcap_drv_inst->received_packets);
        if ((*packet) != NULL)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }

        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);
    }

    return ret;
}

/** \brief Get the last sent packet on the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvGetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if ((pcap_drv_inst != NULL) && (packet != NULL))
    {
        /* Lock interface */
        (void)NANO_IP_OAL_MUTEX_Lock(&pcap_drv_inst->mutex);

        /* Remove last transmitted packet from the list */
        (*packet) = NANO_IP_PACKET_PopFromQueue(&pcap_drv_inst->transmitted_packets);
        if ((*packet) != NULL)
        {
            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }

        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);
    }

    return ret;
}


/** \brief Get the link state of the pcap interface driver */
static nano_ip_error_t NANO_IP_PCAP_DrvGetLinkState(void* const user_data, net_link_state_t* const state)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user_data);

    /* Check parameters */
    if ((pcap_drv_inst != NULL) && (state != NULL))
    {
        (*state) = NLS_UP_1000_FD;
        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}



/** \brief Receive callback */
static void NANO_IP_PCAP_DrvPcapHandler(u_char *user, const struct pcap_pkthdr *pkt_header, const u_char *pkt_data)
{
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, user);
    
    /* Lock interface */
    (void)NANO_IP_OAL_MUTEX_Lock(&pcap_drv_inst->mutex);

    /* Get first available rx packet */
    nano_ip_net_packet_t* received_packet = pcap_drv_inst->rx_packets.head;
    if (received_packet != NULL)
    {
        /* Check RX packet size */
        if (received_packet->size >= pkt_header->caplen)
        {
            /* Copy received data to rx packet */
            NANO_IP_MEMCPY(received_packet->data, pkt_data, pkt_header->caplen);
            received_packet->count = pkt_header->caplen;

            /* Remove packet from the available rx packet list */
            (void)NANO_IP_PACKET_PopFromQueue(&pcap_drv_inst->rx_packets);

            /* Add packet to the received packet list */
            NANO_IP_PACKET_AddToQueue(&pcap_drv_inst->received_packets, received_packet);

            /* Unlock interface */
            (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);

            /* Notify packet reception */
            pcap_drv_inst->callbacks.packet_received(pcap_drv_inst->callbacks.stack_data, false);
        }
        else
        {
            /* Unlock interface */
            (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);

            /* Notify packet error */
            pcap_drv_inst->callbacks.net_drv_error(pcap_drv_inst->callbacks.stack_data, false);
        }
    }
    else
    {
        /* Unlock interface */
        (void)NANO_IP_OAL_MUTEX_Unlock(&pcap_drv_inst->mutex);

        /* Notify packet error */
        pcap_drv_inst->callbacks.net_drv_error(pcap_drv_inst->callbacks.stack_data, false);
    }
}


/** \brief Receive task */
static void NANO_IP_PCAP_DrvRxTask(void* param)
{
    pcap_drv_t* pcap_drv_inst = NANO_IP_CAST(pcap_drv_t*, param);

    /* Task loop */
    #ifndef NANO_IP_OAL_TASK_NO_INFINITE_LOOP
    const int ret = pcap_loop(pcap_drv_inst->pcap, -1, NANO_IP_PCAP_DrvPcapHandler, NANO_IP_CAST(u_char*, pcap_drv_inst));
    #else
    const int ret = pcap_dispatch(pcap_drv_inst->pcap, -1, NANO_IP_PCAP_DrvPcapHandler, NANO_IP_CAST(u_char*, pcap_drv_inst));
    #endif /* NANO_IP_OAL_TASK_NO_INFINITE_LOOP */
    if (ret == -1)
    {
        /* Notify error */
        pcap_drv_inst->callbacks.net_drv_error(pcap_drv_inst->callbacks.stack_data, false);
    }
}
