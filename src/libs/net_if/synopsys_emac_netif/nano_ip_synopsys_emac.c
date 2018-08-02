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


#include "nano_ip_synopsys_emac.h"

#include "nano_ip_tools.h"
#include "nano_ip_hal.h"
#include "nano_ip_log.h"
#include "nano_ip_mdio_driver.h"
#include "nano_ip_packet_funcs.h"

/** \brief Number of RX descriptors (must be at least equal to the number of packets allocated to the network interface) */
#define SYNOPSYS_EMAC_NET_IF_RX_DESC_COUNT     10u

/** \brief Number of TX descriptors (must be at least equal to the number of packets allocated to the network interface) */
#define SYNOPSYS_EMAC_NET_IF_TX_DESC_COUNT     10u




/** \brief SYNOPSYS EMAC registers */
typedef struct _synopsys_emac_regs_t
{
    uint32_t MACCR;
    uint32_t MACFFR;
    uint32_t MACHTHR;
    uint32_t MACHTLR;
    uint32_t MACMIIAR;
    uint32_t MACMIIDR;
    uint32_t MACFCR;
    uint32_t MACVLANTR;             /*    8 */
    uint32_t RESERVED0[2];
    uint32_t MACRWUFFR;             /*   11 */
    uint32_t MACPMTCSR;
    uint32_t RESERVED1;
    uint32_t MACDBGR;
    uint32_t MACSR;                 /*   15 */
    uint32_t MACIMR;
    uint32_t MACA0HR;
    uint32_t MACA0LR;
    uint32_t MACA1HR;
    uint32_t MACA1LR;
    uint32_t MACA2HR;
    uint32_t MACA2LR;
    uint32_t MACA3HR;
    uint32_t MACA3LR;               /*   24 */
    uint32_t RESERVED2[40];
    uint32_t MMCCR;                 /*   65 */
    uint32_t MMCRIR;
    uint32_t MMCTIR;
    uint32_t MMCRIMR;
    uint32_t MMCTIMR;               /*   69 */
    uint32_t RESERVED3[14];
    uint32_t MMCTGFSCCR;            /*   84 */
    uint32_t MMCTGFMSCCR;
    uint32_t RESERVED4[5];
    uint32_t MMCTGFCR;
    uint32_t RESERVED5[10];
    uint32_t MMCRFCECR;
    uint32_t MMCRFAECR;
    uint32_t RESERVED6[10];
    uint32_t MMCRGUFCR;
    uint32_t RESERVED7[334];
    uint32_t PTPTSCR;
    uint32_t PTPSSIR;
    uint32_t PTPTSHR;
    uint32_t PTPTSLR;
    uint32_t PTPTSHUR;
    uint32_t PTPTSLUR;
    uint32_t PTPTSAR;
    uint32_t PTPTTHR;
    uint32_t PTPTTLR;
    uint32_t RESERVED8;
    uint32_t PTPTSSR;
    uint32_t RESERVED9[565];
    uint32_t DMABMR;
    uint32_t DMATPDR;
    uint32_t DMARPDR;
    uint32_t DMARDLAR;
    uint32_t DMATDLAR;
    uint32_t DMASR;
    uint32_t DMAOMR;
    uint32_t DMAIER;
    uint32_t DMAMFBOCR;
    uint32_t DMARSWTR;
    uint32_t RESERVED10[8];
    uint32_t DMACHTDR;
    uint32_t DMACHRDR;
    uint32_t DMACHTBAR;
    uint32_t DMACHRBAR;
} synopsys_emac_regs_t;



/** \brief SYNOPSYS EMAC TX descriptor */
typedef struct _synopsys_emac_tx_desc_t
{
    /** \brief Status */
    uint32_t status;
    /** \brief Buffer size */
    uint32_t buffer_size;
    /** \brief Buffer address */
    uint32_t buffer_address;
    /** \brief Next descriptor */
    struct _synopsys_emac_tx_desc_t* next;
} synopsys_emac_tx_desc_t;

/** \brief SYNOPSYS EMAC RX descriptor */
typedef struct _synopsys_emac_rx_desc_t
{
    /** \brief Status */
    uint32_t status;
    /** \brief Buffer size */
    uint32_t buffer_size;
    /** \brief Buffer address */
    uint32_t buffer_address;
    /** \brief Next descriptor */
    struct _synopsys_emac_rx_desc_t* next;
} synopsys_emac_rx_desc_t;




/** \brief SYNOPSYS EMAC driver data */
typedef struct _synopsys_emac_drv_t
{
    /** \brief Indicate if the driver is started */
    bool started;

    /** \brief Network interface */
    nano_ip_net_if_t* net_if;

    /** \brief Configuration */
    nano_ip_synopsys_emac_config_t config;

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

    /** \brief SYNOPSYS EMAC registers */
    volatile synopsys_emac_regs_t* regs;

    /** \brief Tx descriptors */
    synopsys_emac_tx_desc_t tx_descs[SYNOPSYS_EMAC_NET_IF_TX_DESC_COUNT];
    /** \brief Rx descriptors */
    synopsys_emac_rx_desc_t rx_descs[SYNOPSYS_EMAC_NET_IF_RX_DESC_COUNT];
    /** \brief Current Tx descriptor */
    synopsys_emac_tx_desc_t* current_tx_desc;
    /** \brief Current Rx descriptor used by the DMA engine */
    synopsys_emac_tx_desc_t* dma_current_tx_desc;
    /** \brief Current Rx descriptor */
    synopsys_emac_rx_desc_t* current_rx_desc;
    /** \brief Current Rx descriptor used by the DMA engine */
    synopsys_emac_rx_desc_t* dma_current_rx_desc;

    /** \brief Number of packet sent */
    uint32_t tx_packet_count;
    /** \brief Number of packet received */
    uint32_t rx_packet_count;
} synopsys_emac_drv_t;





/** \brief Init the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks);
/** \brief Start the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvStart(void* const user_data);
/** \brief Stop the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvStop(void* const user_data);
/** \brief Set the MAC address */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvSetMacAddress(void* const user_data, const uint8_t* const mac_address);
/** \brief Send a packet on the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Add a packet for reception for the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet);
/** \brief Get the last received packet on the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvGetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the last sent packet on the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvGetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet);
/** \brief Get the link state of the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvGetLinkState(void* const user_data, net_link_state_t* const state);

/** \brief Read data from a PHY register */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvPhyRead(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, uint16_t* const read_data);
/** \brief Write data to a PHY register */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvPhyWrite(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, const uint16_t data);


/** \brief Enable SYNOPSYS EMAC interrupts */
static void NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(volatile synopsys_emac_regs_t* const synopsys_emac_regs);

/** \brief Disable SYNOPSYS EMAC interrupts */
static void NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(volatile synopsys_emac_regs_t* const synopsys_emac_regs);

/** \brief SYNOPSYS EMAC interrupt handler */
static void NANO_IP_SYNOPSYS_EMAC_InterruptHandler(void* const driver_data);


/** \brief SYNOPSYS EMAC driver data */
static synopsys_emac_drv_t s_synopsys_emac_driver_data;

/** \brief SYNOPSYS EMAC interface driver */
static const nano_ip_net_driver_t s_synopsys_emac_driver = {
                                                            NETDRV_CAPS_ETH_MIN_FRAME_SIZE | 
                                                            NETDRV_CAP_ETH_CS_COMPUTATION |
                                                            NETDRV_CAP_ETH_CS_CHECK |
                                                            NETDRV_CAP_ETH_FRAME_PADDING, /* Capabilities */
                                                            &s_synopsys_emac_driver_data, /* User defined data */

                                                            NANO_IP_SYNOPSYS_EMAC_DrvInit,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvStart,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvStop,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvSetMacAddress,
                                                            NULL, /* set_ipv4_address() */
                                                            NANO_IP_SYNOPSYS_EMAC_DrvSendPacket,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvAddRxPacket,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvGetNextRxPacket,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvGetNextTxPacket,
                                                            NANO_IP_SYNOPSYS_EMAC_DrvGetLinkState
                                                        };

/** \brief SYNOPSYS EMAC MDIO driver */
static const nano_ip_mdio_driver_t s_synopsys_emac_mdio_driver = {
                                                                NANO_IP_SYNOPSYS_EMAC_DrvPhyRead,
                                                                NANO_IP_SYNOPSYS_EMAC_DrvPhyWrite,
                                                                &s_synopsys_emac_driver_data
                                                              };

/** \brief Initialize SYNOPSYS EMAC interface */
nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_Init(nano_ip_net_if_t* const synopsys_emac_iface, fp_synopsys_emac_interrupt_handler_t* const interrupt_handler, void** const driver_data, const nano_ip_synopsys_emac_config_t* const config)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((synopsys_emac_iface != NULL) &&
        (interrupt_handler != NULL) && 
        (driver_data != NULL) &&
        (config != NULL) &&
        (config->phy_driver != NULL))
    {
        /* 0 init */
        (void)MEMSET(&s_synopsys_emac_driver_data, 0, sizeof(synopsys_emac_drv_t));

        /* Init driver interface */
        s_synopsys_emac_driver_data.net_if = synopsys_emac_iface;
        (void)MEMCPY(&s_synopsys_emac_driver_data.config, config, sizeof(nano_ip_synopsys_emac_config_t));
        synopsys_emac_iface->driver = &s_synopsys_emac_driver;

        /* Save SYNOPSYS EMAC base address */
        s_synopsys_emac_driver_data.regs = NANO_IP_CAST(volatile synopsys_emac_regs_t*, s_synopsys_emac_driver_data.config.regs_base_address);

        /* Return interrupt handler */
        (*interrupt_handler) = NANO_IP_SYNOPSYS_EMAC_InterruptHandler;
        (*driver_data) = &s_synopsys_emac_driver_data;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Init the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvInit(void* const user_data, net_driver_callbacks_t* const callbacks)
{
    nano_ip_error_t ret = NIP_ERR_SUCCESS;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (callbacks != NULL))
    {
        uint32_t i;
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Save callbacks */
        (void)MEMCPY(&synopsys_emac_driver->callbacks, callbacks, sizeof(net_driver_callbacks_t));

        /* Initialize clocks and IOs */
        ret = NANO_IP_HAL_InitializeNetIfClock(synopsys_emac_driver->net_if);
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = NANO_IP_HAL_InitializeNetIfIo(synopsys_emac_driver->net_if);
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Reset MAC subsystem */
            synopsys_emac_regs->DMABMR = (1u << 0u);
            while ((synopsys_emac_regs->DMABMR & (1u << 0u)) != 0u)
            {}

            /* Configure MAC to 100MBits Full Duplex */
            synopsys_emac_regs->MACCR = (1u << 11u) | (1u << 14u);

            /* Receive perfect filtered frames and all multicast frames */
            /* synopsys_emac_regs->MACFFR = (1u << 4u); */

            /* Disable frame filtering (FIXME find why frame filtering is not working) */
            synopsys_emac_regs->MACFFR = (1u << 0u) | (1u << 31u);

            /* \brief Initialize Tx and Rx descriptors ring lists */
            for (i = 0; i < (SYNOPSYS_EMAC_NET_IF_TX_DESC_COUNT - 1u); i++)
            {
                synopsys_emac_driver->tx_descs[i].next = &synopsys_emac_driver->tx_descs[i + 1u];
            }
            synopsys_emac_driver->tx_descs[i].next = &synopsys_emac_driver->tx_descs[0u];
            
            for (i = 0; i < (SYNOPSYS_EMAC_NET_IF_RX_DESC_COUNT - 1u); i++)
            {
                synopsys_emac_driver->rx_descs[i].next = &synopsys_emac_driver->rx_descs[i + 1u];
            }
            synopsys_emac_driver->rx_descs[i].next = &synopsys_emac_driver->rx_descs[0u];

            /* Reception and transmission on full frame only */
            synopsys_emac_regs->DMAOMR = (1u << 2u) | (1u << 6u) | (1u << 21u) | (1u << 25u);

            /* Enable interrupts at interrupt controller level*/
            ret = NANO_IP_HAL_EnableNetIfInterrupts(synopsys_emac_driver->net_if);
        }

        /* Reset and configure the PHY */
        if (ret == NIP_ERR_SUCCESS)
        {
            ret = synopsys_emac_driver->config.phy_driver->reset(&s_synopsys_emac_mdio_driver, synopsys_emac_driver->config.phy_address);
            if (ret == NIP_ERR_SUCCESS)
            {
                ret = synopsys_emac_driver->config.phy_driver->configure(&s_synopsys_emac_mdio_driver, synopsys_emac_driver->config.phy_address, synopsys_emac_driver->config.speed, synopsys_emac_driver->config.duplex);
            }
        }
    }

    return ret;
}

/** \brief Start the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvStart(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if (synopsys_emac_driver != NULL)
    {
        uint32_t i;
        nano_ip_net_packet_t* packet;
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Reset packet counters */
        synopsys_emac_driver->rx_packet_count = 0u;
        synopsys_emac_driver->tx_packet_count = 0u;

        /* Initialize Tx descriptors list */
        for (i = 0; i < SYNOPSYS_EMAC_NET_IF_TX_DESC_COUNT; i++)
        {
            synopsys_emac_driver->tx_descs[i].status = 0u;
        }
        synopsys_emac_regs->DMATDLAR = NANO_IP_CAST(uint32_t, synopsys_emac_driver->tx_descs);
        synopsys_emac_driver->current_tx_desc = synopsys_emac_driver->tx_descs;
        synopsys_emac_driver->dma_current_tx_desc = synopsys_emac_driver->tx_descs;

        /* Initialize Rx descriptors list */
        synopsys_emac_regs->DMARDLAR = NANO_IP_CAST(uint32_t, synopsys_emac_driver->rx_descs);
        synopsys_emac_driver->current_rx_desc = synopsys_emac_driver->rx_descs;
        synopsys_emac_driver->dma_current_rx_desc = synopsys_emac_driver->rx_descs;
        packet = synopsys_emac_driver->rx_queued_packets.head;
        while ((packet != NULL) && (synopsys_emac_driver->current_rx_desc != NULL))
        {
            volatile synopsys_emac_rx_desc_t* const rx_desc = synopsys_emac_driver->current_rx_desc;

            /* Fill Rx descriptor */
            rx_desc->buffer_size = packet->size | (1u << 14u);
            rx_desc->buffer_address = NANO_IP_CAST(uint32_t, packet->data);
            rx_desc->status = (1u << 31u);

            /* Next packet */
            packet = packet->next;
            synopsys_emac_driver->current_rx_desc = rx_desc->next;
        }

        /* Enable transmit */
        synopsys_emac_regs->MACCR |= (1u << 3u);

        /* Start DMA operations */
        synopsys_emac_regs->DMAOMR |= (1u << 1u) | (1u << 13u);

        /* Enable receive */
        synopsys_emac_regs->MACCR |= (1u << 2u);

        /* Start receive polling */
	    synopsys_emac_regs->DMARPDR = 1u;

        /* Enable interrupts */
        NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(synopsys_emac_regs);

        /* Driver is now started */
        synopsys_emac_driver->started = true;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Stop the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvStop(void* const user_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if (synopsys_emac_driver != NULL)
    {
        nano_ip_net_packet_t* volatile* last_transmitting_packet = &synopsys_emac_driver->transmitting_packets.tail;
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Wait end of last packet transmission */
        while ((*last_transmitting_packet) != NULL)
        {}

        /* Disable receive and transmit */
        synopsys_emac_regs->MACCR &= ~((1u << 2u) | (1u << 3u));

        /* Stop DMA operations */
        synopsys_emac_regs->DMAOMR &= ~((1u << 1u) | (1u << 13u));

        /* Disable interrupts */
        NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(synopsys_emac_regs);

        /* Driver is now stopped */
        synopsys_emac_driver->started = true;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Set the MAC address */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvSetMacAddress(void* const user_data, const uint8_t* const mac_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (mac_address != NULL))
    {
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Set MAC address */
        synopsys_emac_regs->MACA0LR = (mac_address[0] | (mac_address[1] << 8u) | (mac_address[1] << 16u) | (mac_address[3] << 24u));
        synopsys_emac_regs->MACA0HR = (mac_address[4] | (mac_address[5] << 8u));
        synopsys_emac_regs->MACA1LR = synopsys_emac_regs->MACA0LR;
        synopsys_emac_regs->MACA1HR = synopsys_emac_regs->MACA0HR;
        synopsys_emac_regs->MACA2LR = 0u;
        synopsys_emac_regs->MACA2HR = 0u;
        synopsys_emac_regs->MACA3LR = 0u;
        synopsys_emac_regs->MACA3HR = 0u;

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Send a packet on the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvSendPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (packet != NULL))
    {
        /* Check if the driver is started */
        if (synopsys_emac_driver->started)
        {
            volatile synopsys_emac_tx_desc_t* const tx_desc = synopsys_emac_driver->current_tx_desc;
            volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

            /* Wait for Tx descriptor being freed by the DMA engine */
            while ((tx_desc->status & (1u << 31u)) != 0u)
            {}

            /* Disable interrupts */
            NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(synopsys_emac_regs);

            /* Fill Tx descriptor */
            tx_desc->buffer_size = packet->count;
            tx_desc->buffer_address = NANO_IP_CAST(uint32_t, packet->data);
            tx_desc->status = (1u << 31u) | (1u << 30u) | (1u << 29u) | (1u << 28u) | (1u << 20u);

            /* Go to next descriptor */
            synopsys_emac_driver->current_tx_desc = tx_desc->next;

            /* Add packet to the sent packets list */
            NANO_IP_PACKET_AddToQueue(&synopsys_emac_driver->transmitting_packets, packet);

            /* Enable interrupts */
            NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(synopsys_emac_regs);

            /* Notify DMA engine that a descriptor is available for transmission */
            synopsys_emac_regs->DMATPDR = 1u;

            ret = NIP_ERR_SUCCESS;
        }
        else
        {
            ret = NIP_ERR_FAILURE;
        }
    }

    return ret;
}

/** \brief Add a packet for reception for the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvAddRxPacket(void* const user_data, nano_ip_net_packet_t* const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (packet != NULL))
    {
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Check if the driver is started */
        if (synopsys_emac_driver->started)
        {
            volatile synopsys_emac_rx_desc_t* const rx_desc = synopsys_emac_driver->current_rx_desc;

            /* Wait for Rx descriptor being freed by the DMA engine */
            while ((rx_desc->status & (1u << 31u)) != 0u)
            {}

            /* Disable interrupts */
            NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(synopsys_emac_regs);

            /* Fill Rx descriptor */
            rx_desc->buffer_size = packet->size | (1u << 14u);
            rx_desc->buffer_address = NANO_IP_CAST(uint32_t, packet->data);
            rx_desc->status = (1u << 31u);

            /* Go to next descriptor */
            synopsys_emac_driver->current_rx_desc = rx_desc->next;
        }

        /* Add packet to the list of packets queued for reception list */
        NANO_IP_PACKET_AddToQueue(&synopsys_emac_driver->rx_queued_packets, packet);

        if (synopsys_emac_driver->started)
        {
            /* Enable interrupts */
            NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(synopsys_emac_regs);

            /* Notify DMA engine that a descriptor is available for reception */
            synopsys_emac_regs->DMARPDR = 1u;
        }

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Get the last received packet on the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvGetNextRxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (packet != NULL))
    {
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Disable interrupts */
        NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(synopsys_emac_regs);

        /* Check if a packet has been received */
        (*packet) = NANO_IP_PACKET_PopFromQueue(&synopsys_emac_driver->received_packets);
        if ((*packet) == NULL)
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }

        /* Enable interrupts */
        NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(synopsys_emac_regs);
    }

    return ret;
}

/** \brief Get the last sent packet on the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvGetNextTxPacket(void* const user_data, nano_ip_net_packet_t** const packet)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (packet != NULL))
    {
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Disable interrupts */
        NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(synopsys_emac_regs);

        /* Check if a packet has been sent */
        (*packet) = NANO_IP_PACKET_PopFromQueue(&synopsys_emac_driver->transmitted_packets);
        if ((*packet) == NULL)
        {
            ret = NIP_ERR_PACKET_NOT_FOUND;
        }
        else
        {
            ret = NIP_ERR_SUCCESS;
        }

        /* Enable interrupts */
        NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(synopsys_emac_regs);
    }

    return ret;
}

/** \brief Get the link state of the SYNOPSYS EMAC interface driver */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvGetLinkState(void* const user_data, net_link_state_t* const state)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if (synopsys_emac_driver != NULL)
    {
        /* Retrieve the link state from the PHY */
        ret = synopsys_emac_driver->config.phy_driver->get_link_state(&s_synopsys_emac_mdio_driver, synopsys_emac_driver->config.phy_address, state);
    }

    return ret;
}

/** \brief Read data from a PHY register */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvPhyRead(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, uint16_t* const read_data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if ((synopsys_emac_driver != NULL) && (read_data != NULL))
    {
        uint32_t val;
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Wait link ready */
        while ((synopsys_emac_regs->MACMIIAR & (1u << 0u)) != 0u)
        {}

        /* Write PHY register address */
        val = (phy_address << 11u) | (phy_reg << 6u) | (4u << 2u) | (1u << 0u);
        synopsys_emac_regs->MACMIIAR = val;

        /* Wait end of read */
        while ((synopsys_emac_regs->MACMIIAR & (1u << 0u)) != 0u)
        {}

        /* Retrieve data */
        (*read_data) = NANO_IP_CAST(uint16_t, (synopsys_emac_regs->MACMIIDR & 0xFFFFu));

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}

/** \brief Write data to a PHY register */
static nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_DrvPhyWrite(void* const user_data, const uint8_t phy_address, const uint8_t phy_reg, const uint16_t data)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, user_data);

    /* Check parameters */
    if (synopsys_emac_driver != NULL)
    {
        uint32_t val;
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Wait link ready */
        while ((synopsys_emac_regs->MACMIIAR & (1u << 0u)) != 0u)
        {}

        /* Write data */
        synopsys_emac_regs->MACMIIDR = data;

        /* Write PHY register address */
        val = (phy_address << 11u) | (phy_reg << 6u) | (4u << 2u) | (1u << 1u) | (1u << 0u);
        synopsys_emac_regs->MACMIIAR = val;

        /* Wait end of write */
        while ((synopsys_emac_regs->MACMIIAR & (1u << 0u)) != 0u)
        {}

        ret = NIP_ERR_SUCCESS;
    }

    return ret;
}


/** \brief Enable SYNOPSYS EMAC interrupts */
static void NANO_IP_SYNOPSYS_EMAC_EnableInterrupts(volatile synopsys_emac_regs_t* const synopsys_emac_regs)
{
    synopsys_emac_regs->DMAIER = (1u << 0u) | (1u << 6u) | (1u << 15u) | (1u << 16u);
}

/** \brief Disable SYNOPSYS EMAC interrupts */
static void NANO_IP_SYNOPSYS_EMAC_DisableInterrupts(volatile synopsys_emac_regs_t* const synopsys_emac_regs)
{
    synopsys_emac_regs->DMAIER = 0u;
}

/** \brief SYNOPSYS EMAC interrupt handler */
static void NANO_IP_SYNOPSYS_EMAC_InterruptHandler(void* const driver_data)
{
    synopsys_emac_drv_t* synopsys_emac_driver = NANO_IP_CAST(synopsys_emac_drv_t*, driver_data);

    /* Check parameters */
    if (synopsys_emac_driver != NULL)
    {
        bool notify_stack;
        volatile synopsys_emac_regs_t* const synopsys_emac_regs = synopsys_emac_driver->regs;

        /* Handle received packets */
        notify_stack = false;
        while ((synopsys_emac_driver->rx_queued_packets.head != NULL) &&
               (synopsys_emac_regs->DMACHRBAR != NANO_IP_CAST(uint32_t, synopsys_emac_driver->rx_queued_packets.head->data)))
        {
            /* Remove packet from the Rx queued list */
            nano_ip_net_packet_t* packet = NANO_IP_PACKET_PopFromQueue(&synopsys_emac_driver->rx_queued_packets);

            /* Add packet to the received list */
            NANO_IP_PACKET_AddToQueue(&synopsys_emac_driver->received_packets, packet);

            /* Extract packet size */
            packet->count = NANO_IP_CAST(uint16_t, ((synopsys_emac_driver->dma_current_rx_desc->status >> 16u) & 0x3FFFu));
            packet->count -= sizeof(uint32_t); /* CRC32 */

            /* Next descriptor */
            synopsys_emac_driver->dma_current_rx_desc = synopsys_emac_driver->dma_current_rx_desc->next;

            /* Stack must be notified for packet reception */
            notify_stack = true;
            synopsys_emac_driver->rx_packet_count++;
        }

        /* Notify the stack that packets have been received */
        if (notify_stack)
        {
            synopsys_emac_driver->callbacks.packet_received(synopsys_emac_driver->callbacks.stack_data, true);
        }


        /* Handle transmitted packets */
        notify_stack = false;
        while ((synopsys_emac_driver->transmitting_packets.head != NULL) &&
               ((synopsys_emac_regs->DMACHTDR) != NANO_IP_CAST(uint32_t, synopsys_emac_driver->transmitting_packets.head->data)))
        {
            /* Remove packet from the transmitting list */
            nano_ip_net_packet_t* packet = NANO_IP_PACKET_PopFromQueue(&synopsys_emac_driver->transmitting_packets);

            /* Add packet to the transmitted list */
            NANO_IP_PACKET_AddToQueue(&synopsys_emac_driver->transmitted_packets, packet);

            /* Next descriptor */
            synopsys_emac_driver->dma_current_tx_desc = synopsys_emac_driver->dma_current_tx_desc->next;

            /* Stack must be notified for packet transmission */
            notify_stack = true;
            synopsys_emac_driver->tx_packet_count++;
        }

        /* Notify the stack that packets have been transmitted */
        if (notify_stack)
        {
            synopsys_emac_driver->callbacks.packet_sent(synopsys_emac_driver->callbacks.stack_data, true);
        }

        /* Clear interrupts */
        synopsys_emac_regs->DMASR = 0xFFFFFFFFu;
    }
}

