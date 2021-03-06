####################################################################################################
# \file makefile.inc
# \brief  Makefile for the include files of pcap_netif library
# \author C. Jimenez
# \copyright Copyright(c) 2017 Cedric Jimenez
#
# This file is part of Nano-IP.
#
# Nano-IP is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Nano-IP is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Nano-IP.  If not, see <http://www.gnu.org/licenses/>.
####################################################################################################

# Library directory
LIBRARY_DIR := $(ROOT_DIR)/src/libs/net_if/pcap_netif

# Source directories
SOURCE_DIRS := $(LIBRARY_DIR)
              
# Project specific include directories
PROJECT_INC_DIRS := $(PROJECT_INC_DIRS) \
                    $(LIBRARY_DIR)
