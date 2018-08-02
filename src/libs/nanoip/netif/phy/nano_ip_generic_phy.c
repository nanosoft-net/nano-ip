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

#include "nano_ip_generic_phy.h"


/** \brief Basic control register */
#define REG_BASIC_CONTROL       0x00u

/** \brief Basic status register */
#define REG_BASIC_STATUS        0x01u


/** \brief Reset flag */
#define PHY_CTRL_RESET_FLAG             (1u << 15u)

/** \brief 100Mbits flag */
#define PHY_CTRL_100MB_FLAG             (1u << 13u)

/** \brief Auto-negotiation flag */
#define PHY_CTRL_AUTO_NEGO_FLAG         (1u << 12u)

/** \brief Full duplex flag */
#define PHY_CTRL_FULL_DUPLEX_FLAG       (1u << 8u)

/** \brief 1000Mbits flag */
#define PHY_CTRL_1000MB_FLAG            (1u << 6u)


/** \brief Auto-negotiation complete flag */
#define PHY_STATUS_AUTO_NEG_COMPLETED   (1u << 5u)

/** \brief Link up flag */
#define PHY_STATUS_LINK_UP              (1u << 2u)





/** \brief Reset the PHY */
static nano_ip_error_t NANO_IP_GENERIC_PHY_Reset(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address);

/** \brief Configure the PHY speed and duplex */
static nano_ip_error_t NANO_IP_GENERIC_PHY_Configure(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address, const net_driver_speed_t speed, const net_driver_duplex_t duplex);

/** \brief Get the PHY link state */
static nano_ip_error_t NANO_IP_GENERIC_PHY_GetLinkState(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address, net_link_state_t* const link_state);



/** \brief Generic PHY driver interface */
static const nano_ip_phy_driver_t s_generic_phy_driver = {
                                                            NANO_IP_GENERIC_PHY_Reset,
                                                            NANO_IP_GENERIC_PHY_Configure,
                                                            NANO_IP_GENERIC_PHY_GetLinkState
                                                         };



/** \brief Get the PHY driver interface */
const nano_ip_phy_driver_t* NANO_IP_GENERIC_PHY_GetDriver(void)
{
    return &s_generic_phy_driver;
}

/** \brief Reset the PHY */
static nano_ip_error_t NANO_IP_GENERIC_PHY_Reset(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (mdio_driver != NULL)
    {
        /* Set the reset flag */
        ret = mdio_driver->write(mdio_driver->user_data, phy_address, REG_BASIC_CONTROL, PHY_CTRL_RESET_FLAG);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Wait for the PHY to be operational again */
            uint16_t value = 0u;
            uint32_t timeout = 0x100000u;
            do
            {
                ret = mdio_driver->read(mdio_driver->user_data, phy_address, REG_BASIC_CONTROL, &value);
                timeout--;
                if ((timeout == 0u) && ((value & PHY_CTRL_RESET_FLAG) != 0u))
                {
                    ret = NIP_ERR_TIMEOUT;
                    break;
                }
            }
            while ((ret == NIP_ERR_SUCCESS) && ((value & PHY_CTRL_RESET_FLAG) != 0u));
        }
    }

    return ret;
}

/** \brief Configure the PHY speed and duplex */
static nano_ip_error_t NANO_IP_GENERIC_PHY_Configure(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address, const net_driver_speed_t speed, const net_driver_duplex_t duplex)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if (mdio_driver != NULL)
    {
        uint16_t value = 0u;
        ret = NIP_ERR_SUCCESS;

        /* Duplex configuration */
        switch (duplex)
        {
            case DUP_FULL:
                value = PHY_CTRL_FULL_DUPLEX_FLAG;
                break;

            case DUP_HALF:
                value = 0u;
                break;

            case DUP_AUTO:
                value = PHY_CTRL_AUTO_NEGO_FLAG;
                break;

            default:
                ret = NIP_ERR_INVALID_ARG;
                break;
        }
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Speed configuration if not auto-negotiation */
            if (duplex != DUP_AUTO)
            {
                switch (speed)
                {
                    case SPEED_10:
                        break;

                    case SPEED_100:
                        value |= PHY_CTRL_100MB_FLAG;
                        break;

                    case SPEED_1000:
                        value |= PHY_CTRL_1000MB_FLAG;
                        break;

                    default:
                        ret = NIP_ERR_INVALID_ARG;
                        break;
                }
            }
            if (ret == NIP_ERR_SUCCESS)
            {
                /* Apply configuration */
                ret = mdio_driver->write(mdio_driver->user_data, phy_address, REG_BASIC_CONTROL, value);
            }
        }
    }

    return ret;
}

/** \brief Get the PHY link state */
nano_ip_error_t NANO_IP_GENERIC_PHY_GetLinkState(const nano_ip_mdio_driver_t* const mdio_driver, const uint8_t phy_address, net_link_state_t* const link_state)
{
    nano_ip_error_t ret = NIP_ERR_INVALID_ARG;

    /* Check parameters */
    if ((mdio_driver != NULL) && (link_state != NULL))
    {
        /* Get the basic PHY status */
        uint16_t value = 0u;
        ret = mdio_driver->read(mdio_driver->user_data, phy_address, REG_BASIC_STATUS, &value);
        if (ret == NIP_ERR_SUCCESS)
        {
            /* Check link status */
            if ((value & PHY_STATUS_LINK_UP) == 0u)
            {
                /* Link down */
                (*link_state) = NLS_DOWN;
            }
            else
            {
                /* Check auto-negotiation */
                if ((value & PHY_STATUS_AUTO_NEG_COMPLETED) == 0u)
                {
                    /* Auto-negociation in progress */
                    (*link_state) = NLS_AUTO_NEGO;
                }
                else
                {
                    /* Link up -> no generic register to determine speed and duplex */
                    (*link_state) = NLS_UP;
                }
            }
        }
    }

    return ret;
}
