####################################################################################################
# \file makefile.inc
# \brief  Makefile for the include files of nanoip library
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
LIBRARY_DIR := $(ROOT_DIR)/src/libs/nanoip

# Source directories
SOURCE_DIRS := $(LIBRARY_DIR) \
               $(LIBRARY_DIR)/config \
               $(LIBRARY_DIR)/core \
               $(LIBRARY_DIR)/log \
               $(LIBRARY_DIR)/modules \
               $(LIBRARY_DIR)/modules/dhcp \
               $(LIBRARY_DIR)/modules/socket \
               $(LIBRARY_DIR)/modules/tftp \
               $(LIBRARY_DIR)/netif \
               $(LIBRARY_DIR)/netif/phy \
               $(LIBRARY_DIR)/hal \
               $(LIBRARY_DIR)/oal \
               $(LIBRARY_DIR)/oal/$(TARGET_OS) \
               $(LIBRARY_DIR)/packet \
               $(LIBRARY_DIR)/tools
              
# Project specific include directories
PROJECT_INC_DIRS := $(PROJECT_INC_DIRS) \
                    $(foreach LIB_DIR, $(SOURCE_DIRS), $(LIB_DIR))

