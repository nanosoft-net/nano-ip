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

#ifndef NANO_IP_SYNOPSYS_EMAC_H
#define NANO_IP_SYNOPSYS_EMAC_H

#include "nano_ip_net_if.h"
#include "nano_ip_phy_driver.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/** \brief Interrupt handler function prototype */
typedef void (*fp_synopsys_emac_interrupt_handler_t)(void* const driver_data);


/** \brief SYNOPSYS EMAC driver configuration */
typedef struct _nano_ip_synopsys_emac_config_t
{
    /** \brief Base address of SYNOPSYS EMAC registers */
    uint32_t regs_base_address;
    /** \brief Indicate if RMII must be use instead of MII */
    bool use_rmii;
    /** \brief PHY driver to use */
    const nano_ip_phy_driver_t* phy_driver;
    /** \brief PHY address */
    uint8_t phy_address;
    /** \brief Network speed */
    net_driver_speed_t speed;
    /** \brief Duplex */
    net_driver_duplex_t duplex;
} nano_ip_synopsys_emac_config_t;


/** \brief Initialize SYNOPSYS EMAC interface */
nano_ip_error_t NANO_IP_SYNOPSYS_EMAC_Init(nano_ip_net_if_t* const synopsys_emac_iface, fp_synopsys_emac_interrupt_handler_t* const interrupt_handler, void** const driver_data, const nano_ip_synopsys_emac_config_t* const config);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_SYNOPSYS_EMAC_H */
