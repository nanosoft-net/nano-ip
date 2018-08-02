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

#ifndef NANO_IP_PHY_DRIVER_H
#define NANO_IP_PHY_DRIVER_H

#include "nano_ip_mdio_driver.h"
#include "nano_ip_net_driver.h"


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */



/** \brief Ethernet PHY driver interface */
typedef struct _nano_ip_phy_driver_t
{
    /** \brief Reset the PHY */
    nano_ip_error_t (*reset)(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address);
    /** \brief Configure the PHY speed and duplex */
    nano_ip_error_t (*configure)(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address, const net_driver_speed_t speed, const net_driver_duplex_t duplex);
    /** \brief Get the PHY link state */
    nano_ip_error_t (*get_link_state)(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address, net_link_state_t* const link_state);
} nano_ip_phy_driver_t;



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* NANO_IP_PHY_DRIVER_H */
