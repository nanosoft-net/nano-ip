####################################################################################################
# \file makefile
# \brief  Makefile for libpcap-1.8.1 third party library
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

# Locating the root directory
ROOT_DIR := ../../..

# Project name
PROJECT_NAME := pcap

# Build type
BUILD_TYPE := LIB

# Projects that need to be build before the project or containing necessary include paths
PROJECT_DEPENDENCIES := 

			  
# Including common makefile definitions
include $(ROOT_DIR)/build/make/generic_makefile


# Rules for building the library
$(LIBRARY_DIR)/Makefile: 
	@echo "Configuring $(PROJECT_NAME)..."
	$(DISP)cd $(LIBRARY_DIR) && ./configure

$(BIN_DIR)/$(OUTPUT_NAME): $(LIBRARY_DIR)/Makefile
	@echo "Creating archive $(notdir $@)..."
	+$(DISP)$(MAKE) $(SILENT) -C $(LIBRARY_DIR)/  all
	$(DISP)cp $(LIBRARY_DIR)/$(OUTPUT_NAME) $(BIN_DIR)/$(OUTPUT_NAME)

	

