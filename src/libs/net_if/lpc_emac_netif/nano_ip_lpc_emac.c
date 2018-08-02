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


#include "nano_ip_lpc_emac.h"

#include "nano_ip_tools.h"
#include "nano_ip_hal.h"
#include "nano_ip_log.h"
#include "nano_ip_mdio_driver.h"
#include "nano_ip_packet_funcs.h"

/** \brief Number of RX descriptors (must be at least equal to the number of packets allocated to the network interface) */
#define LPC_EMAC_NET_IF_RX_DESC_COUNT     10u

/** \brief Number of TX descriptors (must be at least equal to the number of packets allocated to the network interface) */
#define LPC_EMAC_NET_IF_TX_DESC_COUNT     10u




/** \brief LPC EMAC registers */
typedef struct _lpc_emac_regs_t
{
    uint32_t MAC1;                   /* MAC Registers                      */
    uint32_t MAC2;
    uint32_t IPGT;
    uint32_t IPGR;
    uint32_t CLRT;
    uint32_t MAXF;
    uint32_t SUPP;
    uint32_t TEST;
    uint32_t MCFG;
    uint32_t MCMD;
    uint32_t MADR;
    uint32_t MWTD;
    uint32_t MRDD;
    uint32_t MIND;
    uint32_t RESERVED0[2];
    uint32_t SA0;
    uint32_t SA1;
    uint32_t SA2;
    uint32_t RESERVED1[45];
    uint32_t Command;                /* Control Registers                  */
    uint32_t Status;
    uint32_t RxDescriptor;
    uint32_t RxStatus;
    uint32_t RxDescriptorNumber;
    uint32_t RxProduceIndex;
    uint32_t RxConsumeIndex;
    uint32_t TxDescriptor;
    uint32_t TxStatus;
    uint32_t TxDescriptorNumber;
    uint32_t TxProduceIndex;
    uint32_t TxConsumeIndex;
    uint32_t RESERVED2[10];
    uint32_t TSV0;
    uint32_t TSV1;
    uint32_t RSV;
    uint32_t RESERVED3[3];
    uint32_t FlowControlCounter;
    uint32_t FlowControlStatus;
    uint32_t RESERVED4[34];
    uint32_t RxFilterCtrl;           /* Rx Filter Registers                */
    uint32_t RxFilterWoLStatus;
    uint32_t RxFilterWoLClear;
    uint32_t RESERVED5;
    uint32_t HashFilterL;
    uint32_t HashFilterH;
    uint32_t RESERVED6[882];
    uint32_t IntStatus;              /* Module Control Registers           */
    uint32_t IntEnable;
    uint32_t IntClear;
    uint32_t IntSet;
    uint32_t RESERVED7;
    uint32_t PowerDown;
    uint32_t RESERVED8;
    uint32_t Module_ID;
} lpc_emac_regs_t;



/** \brief LPC EMAC TX descriptor */
typedef struct _lpc_emac_tx_desc_t
{
    /** \brief Base address of the data buffer for storing transmit data */
    uint32_t packet;
    /** \brief Control information */
    uint32_t control;
} lpc_emac_tx_desc_t;

/** \brief LPC EMAC TX status descriptor */
typedef struct _lpc_emac_tx_status_desc_t
{
    /** \brief Transmit status return flags */
    uint32_t status_info;
} lpc_emac_tx_status_desc_t;

/** \brief LPC EMAC RX descriptor */
typedef struct _lpc_emac_rx_desc_t
{
    /** \brief Base address of the data buffer for storing receive data */
    uint32_t packet;
    /** \brief Control information */
    uint32_t control;
} lpc_emac_rx_desc_t;

/** \brief LPC EMAC RX status descriptor */
typedef struct _lpc_emac_rx_status_desc_t
{
    /** \brief Receive status return flags */
    uint32_t status_info;
    /** \brief The concatenation of the destination address hash CRC and the source address hash CRC */
    uint32_t status_hash_crc;
} lpc_emac_rx_status_desc_t;



/** \brief LPC EMAC driver data */
typedef struct _lpc_emac_drv_t
{
    /** \brief Indicate if the driver is started */
    bool started;

    /** \brief Network interface */
    nano_ip_net_if_t* net_if;

    /** \brief Configuration */
    nano_ip_lpc_emac_config_t config;

    /** \brief Callbacks */
    net_driver_callbacks_t callbacks;
    
    /** \brief List of received packets */
    nano_ip_packet_queue_t received_packets;

    /** \brief List of packets queued for reception */
    nano_ip_packet_queue_t rx_queued_packets;
    
    /** \brief List of packets being transmitted */
    nano_ip_packet_queue_t transmitting_packets;

    /** \brief List of transmitted packets */
    nano_ip_packet_queue_t transmitted_packets;

    /** \brief LPC EMAC registers */
    volatile lpc_emac_regs_t* regs;

    /** \brief Tx descriptors */
    lpc_emac_tx_desc_t tx_descs[LPC_EMAC_NET_IF_TX_DESC_COUNT];
    /** \brief Tx status descriptors */
    lpc_emac_tx_status_desc_t tx_status_descs[LPC_EMAC_NET_IF_TX_DESC_COUNT];
    /** \brief Rx descriptors */
    lpc_emac_rx_desc_t rx_descs[LPC_EMAC_NET_IF_RX_DESC_COUNT];
    /** \brief Rx status descriptors */
    lpc_emac_rx_status_desc_t rx_status_descs[LPC_EMAC_NET_IF_RX_DESC_COUNT] __attribute__((aligned(8)));
    /** \brief Number of Rx descriptors in use */
    uint32_t rx_descriptor_count;

    /** \brief Index of the Tx descriptor which has been produced */
    uint32_t tx_descriptor_produced_index;
    /** \brief Index of the Rx descriptor to consume */
    uint32_t rx_descriptor_consume_index;

    /** \brief Number of packet sent */
    uint32_t tx_packet_count;
    /** \brief Number of packet received */
    uint32_t rx_packet_count;
} lpc_emac_drv_t;





/** \brief Init the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks);
/** \brief Start the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvStart(void* const user_data);
/** \brief Stop the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvStop(void* const user_data);
/** \brief Set the MAC address */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvSetMacAddress(void* const user_data, const uint8_t* const mac_address);
/** \brief Send a packet on the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Add a packet for reception for the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Get the last received packet on the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvGetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the last sent packet on the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvGetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the link state of the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvGetLinkState(void* const user_data, net_link_state_t* const state);

/** \brief Read data from a PHY register */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvPhyRead(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, uint16_t* const read_data);
/** \brief Write data to a PHY register */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvPhyWrite(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, const uint16_t data);


/** \brief Enable LPC EMAC interrupts */
static void NANO_IP_LPC_EMAC_EnableInterrupts(volatile lpc_emac_regs_t* const lpc_emac_regs);

/** \brief Disable LPC EMAC interrupts */
static void NANO_IP_LPC_EMAC_DisableInterrupts(volatile lpc_emac_regs_t* const lpc_emac_regs);

/** \brief LPC EMAC interrupt handler */
static void NANO_IP_LPC_EMAC_InterruptHandler(void* const driver_data);


/** \brief LPC EMAC driver data */
static lpc_emac_drv_t s_lpc_emac_driver_data;

/** \brief LPC EMAC interface driver */
static const nano_ip_net_driver_t s_lpc_emac_driver = {
                                                            NETDRV_CAPS_ETH_MIN_FRAME_SIZE | 
                                                            NETDRV_CAP_ETH_CS_COMPUTATION |
                                                            NETDRV_CAP_ETH_CS_CHECK |
                                                            NETDRV_CAP_ETH_FRAME_PADDING, /* Capabilities */
                                                            &s_lpc_emac_driver_data, /* User defined data */

                                                            NANO_IP_LPC_EMAC_DrvInit,
                                                            NANO_IP_LPC_EMAC_DrvStart,
                                                            NANO_IP_LPC_EMAC_DrvStop,
                                                            NANO_IP_LPC_EMAC_DrvSetMacAddress,
                                                            NULL, /* set_ipv4_address() */
                                                            NANO_IP_LPC_EMAC_DrvSendPacket,
                                                            NANO_IP_LPC_EMAC_DrvAddRxPacket,
                                                            NANO_IP_LPC_EMAC_DrvGetNextRxPacket,
                                                            NANO_IP_LPC_EMAC_DrvGetNextTxPacket,
                                                            NANO_IP_LPC_EMAC_DrvGetLinkState
                                                        };

/** \brief LPC EMAC MDIO driver */
static const nano_ip_mdio_driver_t s_lpc_emac_mdio_driver = {
                                                                NANO_IP_LPC_EMAC_DrvPhyRead,
                                                                NANO_IP_LPC_EMAC_DrvPhyWrite,
                                                                &s_lpc_emac_driver_data
                                                              };

/** \brief Initialize LPC EMAC interface */
nano_ip_error_t NANO_IP_LPC_EMAC_Init(nano_ip_net_if_t* const lpc_emac_iface, fp_lpc_emac_interrupt_handler_t* const interrupt_handler, void** const driver_data, const nano_ip_lpc_emac_config_t* const config)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((lpc_emac_iface != NULL) &&
        (interrupt_handler != NULL) && 
        (driver_data != NULL) &&
        (config != NULL) &&
        (config->phy_driver != NULL))
    {
        /* 0 init */
        (void)MEMSET(&s_lpc_emac_driver_data, 0, sizeof(lpc_emac_drv_t));

        /* Init driver interface */
        s_lpc_emac_driver_data.net_if = lpc_emac_iface;
        (void)MEMCPY(&s_lpc_emac_driver_data.config, config, sizeof(nano_ip_lpc_emac_config_t));
        lpc_emac_iface->driver = &s_lpc_emac_driver;

        /* Save LPC EMAC base address */
        s_lpc_emac_driver_data.regs = NANO_IP_CAST(volatile lpc_emac_regs_t*, s_lpc_emac_driver_data.config.regs_base_address);

        /* Return interrupt handler */
        (*interrupt_handler) = NANO_IP_LPC_EMAC_InterruptHandler;
        (*driver_data) = &s_lpc_emac_driver_data;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Init the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (callbacks != NULL))
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Save callbacks */
        (void)MEMCPY(&lpc_emac_driver->callbacks, callbacks, sizeof(net_driver_callbacks_t));

        /* Initialize clocks and IOs */
        ret = NANO_IP_HAL_InitializeNetIfClock(lpc_emac_driver->net_if);
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_HAL_InitializeNetIfIo(lpc_emac_driver->net_if);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            uint32_t loop;

            /* Remove soft reset condition */
            lpc_emac_regs->MAC1 = (1u << 8u) | (1u << 9u) | (1u << 10u) | (1u << 11u) | (1u << 14u) | (1u << 5u);
            lpc_emac_regs->Command = (1u << 3u) | (1u << 4u) | (1u << 5u);

            for (loop = 100; loop; loop--);

            lpc_emac_regs->MAC1 = 0u;
            while ((lpc_emac_regs->MAC1 & (1u << 15u)) != 0u)
            {}

            /* Configure MAC : Full-Duplex, Frame length check, Padding and CRC enabled */
            lpc_emac_regs->MAC2 = (1u << 0u) | (1u << 1u) | (1u << 4u) | (1u << 5u);

            /* Accept all broadcast and multicast frames, perfect match for unicast */
            lpc_emac_regs->RxFilterCtrl = (1u << 1u) | (1u << 2u) | (1u << 5u);

            /* Pass runt frames - Disable frame filtering (FIXME find why frame filtering is not working) */
            lpc_emac_regs->Command = (1u << 6u) | (1u << 7u);

            /* Configure MDIO interface */
            lpc_emac_regs->MCFG = (15u << 2u) | (1u << 15u);
            for (loop = 100; loop; loop--);
            lpc_emac_regs->MCFG = (15u << 2u);

            /* Select MII/RMII mode 100Mbps Full-Duplex */
            if (lpc_emac_driver->config.use_rmii)
            {
                lpc_emac_regs->Command |= (1u << 9u);
                lpc_emac_regs->SUPP = (1u << 8u);
            }
            lpc_emac_regs->Command |= (1u << 10u);
            lpc_emac_regs->IPGT = 0x15u;

            /* Enable interrupts at interrupt controller level*/
            ret = NANO_IP_HAL_EnableNetIfInterrupts(lpc_emac_driver->net_if);
        }

        /* Reset and configure the PHY */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = lpc_emac_driver->config.phy_driver->reset(&s_lpc_emac_mdio_driver, lpc_emac_driver->config.phy_address);
            if (ret == NIP_ERR_SUCCESS)
            {
                ret = lpc_emac_driver->config.phy_driver->configure(&s_lpc_emac_mdio_driver, lpc_emac_driver->config.phy_address, lpc_emac_driver->config.speed, lpc_emac_driver->config.duplex);
            }
        }
    }

    return ret;
}

/** \brief Start the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvStart(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if (lpc_emac_driver != NULL)
    {
        nano_ip_net_packet_t* packet;
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Reset packet counters */
        lpc_emac_driver->rx_packet_count = 0u;
        lpc_emac_driver->tx_packet_count = 0u;

        /* Release reset of receive and transmit datapaths */
        lpc_emac_regs->Command &= ~((1u << 4u) | (1u << 5u));

        /* Configure Tx descriptors */
        lpc_emac_regs->TxDescriptorNumber = LPC_EMAC_NET_IF_TX_DESC_COUNT - 1u;
        lpc_emac_regs->TxProduceIndex = 0u;
        lpc_emac_regs->TxDescriptor = NANO_IP_CAST(uint32_t, lpc_emac_driver->tx_descs);
        lpc_emac_regs->TxStatus = NANO_IP_CAST(uint32_t, lpc_emac_driver->tx_status_descs);
        lpc_emac_driver->tx_descriptor_produced_index = 0u;

        /* Configure Rx descriptors */
        lpc_emac_driver->rx_descriptor_count = 0;
        packet = lpc_emac_driver->rx_queued_packets.head;
        while (packet != NULL)
        {
            /* Fill Rx descriptor */
            volatile lpc_emac_rx_desc_t* const rx_desc = &lpc_emac_driver->rx_descs[lpc_emac_driver->rx_descriptor_count];
            volatile lpc_emac_rx_status_desc_t* const rx_status_desc = &lpc_emac_driver->rx_status_descs[lpc_emac_driver->rx_descriptor_count];
            rx_desc->packet = NANO_IP_CAST(uint32_t, packet->data);
            rx_desc->control = (packet->size - 1u) | (1u << 31u);
            rx_status_desc->status_info = 0xFFFFFFFFu;
            rx_status_desc->status_hash_crc = 0xFFFFFFFFu;

            /* Next packet */
            packet = packet->next;
            lpc_emac_driver->rx_descriptor_count++;
        }
        if (lpc_emac_driver->rx_descriptor_count == 0u)
        {
            ret = NIP_ERR_FAILURE;
        }
        else
        {
            lpc_emac_regs->RxDescriptorNumber = lpc_emac_driver->rx_descriptor_count - 1u;
            lpc_emac_regs->RxConsumeIndex = 0u;
            lpc_emac_regs->RxDescriptor = NANO_IP_CAST(uint32_t, lpc_emac_driver->rx_descs);
            lpc_emac_regs->RxStatus = NANO_IP_CAST(uint32_t, lpc_emac_driver->rx_status_descs);
            lpc_emac_driver->rx_descriptor_consume_index = 0u;

            /* Enable receive and transmit data paths */
            lpc_emac_regs->Command |= (1u << 0u) | (1u << 1u);
            lpc_emac_regs->MAC1 = (1u << 0u);
        
            /* Enable interrupts */
            NANO_IP_LPC_EMAC_EnableInterrupts(lpc_emac_regs);

            /* Driver is now started */
            lpc_emac_driver->started = true;

            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}

/** \brief Stop the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvStop(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if (lpc_emac_driver != NULL)
    {
        nano_ip_net_packet_t* volatile* last_transmitting_packet = &lpc_emac_driver->transmitting_packets.tail;
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Wait end of last packet transmission */
        while ((*last_transmitting_packet) != NULL)
        {}

        /* Disable receive and transmit */
        lpc_emac_regs->MAC1 &= ~((1u << 0u) | (1u << 2u));
        lpc_emac_regs->Command &= ~((1u << 0u) | (1u << 1u));

        /* Reset receive and transmit datapaths */
        lpc_emac_regs->Command |= (1u << 4u) | (1u << 5u);
    
        /* Disable interrupts */
        NANO_IP_LPC_EMAC_DisableInterrupts(lpc_emac_regs);

        /* Driver is now stopped */
        lpc_emac_driver->started = false;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Set the MAC address */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvSetMacAddress(void* const user_data, const uint8_t* const mac_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (mac_address != NULL))
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Set MAC address */
        lpc_emac_regs->SA0 = mac_address[1u] | (mac_address[0u] << 8u);
        lpc_emac_regs->SA1 = mac_address[3u] | (mac_address[2u] << 8u);
        lpc_emac_regs->SA2 = mac_address[5u] | (mac_address[4u] << 8u);

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Send a packet on the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (packet != NULL))
    {
        /* Check if the driver is started */
        if (lpc_emac_driver->started)
        {
            uint32_t next_index;
            volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;
            volatile lpc_emac_tx_desc_t* const tx_desc = &lpc_emac_driver->tx_descs[lpc_emac_regs->TxProduceIndex];
            volatile lpc_emac_tx_status_desc_t* const tx_status_desc = &lpc_emac_driver->tx_status_descs[lpc_emac_regs->TxProduceIndex];

            /* Compute next descriptor index */
            next_index = lpc_emac_regs->TxProduceIndex + 1u;
            if (next_index == LPC_EMAC_NET_IF_TX_DESC_COUNT)
            {
                next_index = 0;
            }

            /* Wait for Tx descriptor being freed by the DMA engine */
            while (next_index == lpc_emac_regs->TxConsumeIndex)
            {}
        
            /* Disable interrupts */
            NANO_IP_LPC_EMAC_DisableInterrupts(lpc_emac_regs);

            /* Fill Tx descriptor */
            tx_desc->packet = NANO_IP_CAST(uint32_t, packet->data);
            tx_desc->control = (packet->count - 1u) | (1u << 30u) | (1u << 31u);
            tx_status_desc->status_info = 0xFFFFFFFFu;
    
            /* Go to next descriptor */
            lpc_emac_regs->TxProduceIndex = next_index;

            /* Add packet to the sent packets list */
            NANO_IP_PACKET_AddToQueue(&lpc_emac_driver->transmitting_packets, packet);

            /* Enable interrupts */
            NANO_IP_LPC_EMAC_EnableInterrupts(lpc_emac_regs);

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
        }
    }

    return ret;
}

/** \brief Add a packet for reception for the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (packet != NULL))
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Check if the driver is started */
        if (lpc_emac_driver->started)
        {
            uint32_t next_index;
            volatile lpc_emac_rx_desc_t* const rx_desc = &lpc_emac_driver->rx_descs[lpc_emac_regs->RxConsumeIndex];
            volatile lpc_emac_rx_status_desc_t* const rx_status_desc = &lpc_emac_driver->rx_status_descs[lpc_emac_regs->RxConsumeIndex];

            /* Compute next descriptor index */
            next_index = lpc_emac_regs->RxConsumeIndex + 1u;
            if (next_index == lpc_emac_driver->rx_descriptor_count)
            {
                next_index = 0;
            }
    
            /* Disable interrupts */
            NANO_IP_LPC_EMAC_DisableInterrupts(lpc_emac_regs);

            /* Fill Rx descriptor */
            rx_desc->packet = NANO_IP_CAST(uint32_t, packet->data);
            rx_desc->control = (packet->size - 1u) | (1u << 31u);
            rx_status_desc->status_info = 0xFFFFFFFFu;
            rx_status_desc->status_hash_crc = 0xFFFFFFFFu;
    
            /* Go to next descriptor */
            lpc_emac_regs->RxConsumeIndex = next_index;
        }

        /* Add packet to the list of packets queued for reception */
        NANO_IP_PACKET_AddToQueue(&lpc_emac_driver->rx_queued_packets, packet);

        /* Enable interrupts */
        if (lpc_emac_driver->started)
        {
            NANO_IP_LPC_EMAC_EnableInterrupts(lpc_emac_regs);
        }

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Get the last received packet on the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvGetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (packet != NULL))
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Disable interrupts */
        NANO_IP_LPC_EMAC_DisableInterrupts(lpc_emac_regs);

        /* Check if a packet has been received */
        (*packet) = NANO_IP_PACKET_PopFromQueue(&lpc_emac_driver->received_packets);
        if ((*packet) == NULL)
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }

        /* Enable interrupts */
        NANO_IP_LPC_EMAC_EnableInterrupts(lpc_emac_regs);
    }

    return ret;
}

/** \brief Get the last sent packet on the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvGetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (packet != NULL))
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Disable interrupts */
        NANO_IP_LPC_EMAC_DisableInterrupts(lpc_emac_regs);

        /* Check if a packet has been sent */
        (*packet) = NANO_IP_PACKET_PopFromQueue(&lpc_emac_driver->transmitted_packets);
        if ((*packet) == NULL)
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }

        /* Enable interrupts */
        NANO_IP_LPC_EMAC_EnableInterrupts(lpc_emac_regs);
    }

    return ret;
}

/** \brief Get the link state of the LPC EMAC interface driver */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvGetLinkState(void* const user_data, net_link_state_t* const state)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if (lpc_emac_driver != NULL)
    {
        /* Retrieve the link state from the PHY */
        ret = lpc_emac_driver->config.phy_driver->get_link_state(&s_lpc_emac_mdio_driver, lpc_emac_driver->config.phy_address, state);
    }

    return ret;
}

/** \brief Read data from a PHY register */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvPhyRead(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, uint16_t* const read_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if ((lpc_emac_driver != NULL) && (read_data != NULL))
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Wait link ready */
        while ((lpc_emac_regs->MIND & (1u <<0u)) != 0u)
        {}

        /* Write PHY register address */
        lpc_emac_regs->MADR = phy_reg | (phy_address << 8u);
        
        /* Start read operation */
        lpc_emac_regs->MCMD = (1u << 0u);

        /* Wait end of read */
        while ((lpc_emac_regs->MIND & (1u <<0u)) != 0u)
        {}
        lpc_emac_regs->MCMD = 0u;

        /* Check result */
        if ((lpc_emac_regs->MIND & (1u << 3u)) != 0u)
        {
            /* Error */
            ret = NIP_ERR_FAILURE;
        }
        else
        {
            /* Retrieve data */
            (*read_data) = lpc_emac_regs->MRDD;

            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}

/** \brief Write data to a PHY register */
static nano_ip_error_t NANO_IP_LPC_EMAC_DrvPhyWrite(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, const uint16_t data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, user_data);

    /* Check parameters */
    if (lpc_emac_driver != NULL)
    {
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Wait link ready */
        while ((lpc_emac_regs->MIND & (1u <<0u)) != 0u)
        {}
 
        /* Write PHY register address */
        lpc_emac_regs->MADR = phy_reg | (phy_address << 8u);

        /* Write data */
        lpc_emac_regs->MWTD = data;
 
        /* Wait end of write */
        while ((lpc_emac_regs->MIND & (1u <<0u)) != 0u)
        {}

        /* Check result */
        if ((lpc_emac_regs->MIND & (1u << 3u)) != 0u)
        {
            /* Error */
            ret = NIP_ERR_FAILURE;
        }
        else
        {
            /* Success */
            ret = NIP_ERR_SUCCESS;
        }
    }

    return ret;
}


/** \brief Enable LPC EMAC interrupts */
static void NANO_IP_LPC_EMAC_EnableInterrupts(volatile lpc_emac_regs_t* const lpc_emac_regs)
{
    lpc_emac_regs->IntEnable = (1u << 0u) | (1u << 1u) | (1u << 3u) | (1u << 5u) | (1u << 7u);
}

/** \brief Disable LPC EMAC interrupts */
static void NANO_IP_LPC_EMAC_DisableInterrupts(volatile lpc_emac_regs_t* const lpc_emac_regs)
{
    lpc_emac_regs->IntEnable = 0u;
}

/** \brief LPC EMAC interrupt handler */
static void NANO_IP_LPC_EMAC_InterruptHandler(void* const driver_data)
{
    lpc_emac_drv_t* lpc_emac_driver = NANO_IP_CAST(lpc_emac_drv_t*, driver_data);

    /* Check parameters */
    if (lpc_emac_driver != NULL)
    {
        bool notify_stack;
        volatile lpc_emac_regs_t* const lpc_emac_regs = lpc_emac_driver->regs;

        /* Handle received packets */
        notify_stack = false;
        while ((lpc_emac_driver->rx_queued_packets.head != NULL) &&
               (lpc_emac_regs->RxProduceIndex != lpc_emac_driver->rx_descriptor_consume_index))
        {
            volatile lpc_emac_rx_status_desc_t* rx_status_desc = &lpc_emac_driver->rx_status_descs[lpc_emac_driver->rx_descriptor_consume_index];

            /* Remove packet from the Rx queued list */
            nano_ip_net_packet_t* packet = NANO_IP_PACKET_PopFromQueue(&lpc_emac_driver->rx_queued_packets);

            /* Add packet to the received list */
            NANO_IP_PACKET_AddToQueue(&lpc_emac_driver->received_packets, packet);

            /* Extract packet size (minus FCS) and error flags */
            packet->count = NANO_IP_CAST(uint16_t, ((rx_status_desc->status_info) & 0x3FFu)) + 1u - sizeof(uint32_t);
            if (((rx_status_desc->status_info & ((1u << 23u) | (1u << 25u) | (1u << 27u))) != 0u) ||
                ((rx_status_desc->status_info & (1u << 30u)) == 0u))
            {
                packet->flags |= NET_IF_PACKET_FLAG_ERROR;
            }

            /* Next descriptor */
            lpc_emac_driver->rx_descriptor_consume_index++;
            if (lpc_emac_driver->rx_descriptor_consume_index == lpc_emac_driver->rx_descriptor_count)
            {
                lpc_emac_driver->rx_descriptor_consume_index = 0u;
            }

            /* Stack must be notified for packet reception */
            notify_stack = true;
            lpc_emac_driver->rx_packet_count++;
        }

        /* Notify the stack that packets have been received */
        if (notify_stack)
        {
            lpc_emac_driver->callbacks.packet_received(lpc_emac_driver->callbacks.stack_data, true);
        }


        /* Handle transmitted packets */
        notify_stack = false;
        while ((lpc_emac_driver->transmitting_packets.head != NULL) &&
               (lpc_emac_regs->TxConsumeIndex != lpc_emac_driver->tx_descriptor_produced_index))
        {
            /* Remove packet from the transmitting list */
            nano_ip_net_packet_t* packet = NANO_IP_PACKET_PopFromQueue(&lpc_emac_driver->transmitting_packets);

            /* Add packet to the transmitted list */
            NANO_IP_PACKET_AddToQueue(&lpc_emac_driver->transmitted_packets, packet);

            /* Next descriptor */
            lpc_emac_driver->tx_descriptor_produced_index++;
            if (lpc_emac_driver->tx_descriptor_produced_index == LPC_EMAC_NET_IF_TX_DESC_COUNT)
            {
                lpc_emac_driver->tx_descriptor_produced_index = 0u;
            }

            /* Stack must be notified for packet transmission */
            notify_stack = true;
            lpc_emac_driver->tx_packet_count++;
        }

        /* Notify the stack that packets have been transmitted */
        if (notify_stack)
        {
            lpc_emac_driver->callbacks.packet_sent(lpc_emac_driver->callbacks.stack_data, true);
        }

        /* Clear interrupts */
        lpc_emac_regs->IntClear = 0xFFFFFFFFu;
    }
}

