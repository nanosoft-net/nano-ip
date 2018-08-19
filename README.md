# Nano IP
Nano IP is an open source ligthweight IP stack targeting 16 to 32bits microcontrollers.

Its main characteristics are:

* Implementation of the following protocols
  * Ethernet
  * ARP
  * IPv4 (without fragmentation)
  * ICMPv4 (ping requests/responses only)
  * UDPv4
  * TCPv4 (with limitations)
  * DHCPv4 client
  * TFTP (server and client)
* User defined protocols on top of any of the previous protocols can be easily added to the stack
* Support of BSD like socket interface
* Operating System Abstraction Layer (OAL) to ease port on different Operating Systems (can be used without any Operating System)
* Support of localhost network interface
* Simple API to add a new network interface implementation
* Highly portable code (100% written in C language, without endianness issues)
* A unique global variable containing all the internal data

## Supported architectures

Nano IP OAL has been ported to the following operating systems:

* OS less (no Operating System)
* Windows
* Linux
* Nano-OS (https://github.com/nanosoft-net/nano-os)

Nano IP network interfaces implementations:

* localhost
* PCAP (using libpcap and winpcap) for Windows and Linux
* LPC_EMAC for NXP LPC24XX and LPC17XX microcontrollers
* SYNOPSYS_EMAC for NXP LPC43XX, LPC18XX and ST Microelectronics STM32F4XXX, STM32F7XXX

## Building Nano IP

Nano IP source code release uses Nano Build as build system.
Nano Build is a makefile based build system which simplifies cross platform / cross target compilation.
The generated binaries files are in ELF format and are ready to use with a debugger (example : J-link + GdbServer).
If you need to get ihex files or raw binary files you can use the GNU objcopy utility to convert Nano Build output file to the wanted format.
Nano IP demo applications and Nano IP source code can also be built using custom makefile or custom IDE project.

### Building using Nano Build

#### Pre-requisites

The GNU Make 4.1 or greater utility must be installed and must be on the operating system PATH variable.
If using the python build script, Python 2.7 must be installed.

#### Configuring Nano Build

The path to the compilers must be configured in the build/make/compiler directory.
Each compiler files refers to a single compiler and must be written using the Makefile syntax :

* arm-gcc-cortex-m.compiler : GCC compiler for ARM Cortex-M CPUs
* arm-iar.compiler : IAR Systems compiler for ARM CPUs
* arm-keil.compiler : ARM Keil compiler for ARM CPUs
* mingw.compiler : MingGW compiler for Windows
* gnu-gcc.compiler : GNU GCC compiler for Linux

#### Building using the build python script

The build.py script at the root of Nano Build can be use to build multiple project at a time or to build projects without having to navigate in the Makefile directories.


usage: build.py [-h] -t target -p package [package ...] -c command
                [command ...] [-j jobs] [-l] [-v] [-d]

Nano Build tool v1.0

optional arguments:
  -h, --help            show this help message and exit
  -t target             [string] Target name
  -p package [package ...]
                        [string] Packages to build
  -c command [command ...]
                        [string] Build command, ex: all|all+|clean|clean+
  -j jobs               [int] Number of build jobs
  -l                    List all available packages and targets
  -v                    Verbose mode
  -d                    Header dependency check




The following command will list all the available projects and all the available build targets (a target is the association of a compiler and a CPU):

build.py -l

Packages list:
 - 3rdparty.pcap
 - 3rdparty.winpcap
 - apps.demo
 - apps.demo_socket
 - bsps.bsp_linux
 - bsps.bsp_lpc1788
 - bsps.bsp_lpc43s37
 - bsps.bsp_stm32f767
 - bsps.bsp_windows
 - libs.nanoip
 - libs.net_if.lpc_emac_netif
 - libs.net_if.pcap_netif
 - libs.net_if.synopsys_emac_netif

Target list:
 - arm-gcc-freertos-lpc1768
 - arm-gcc-os-less-lpc1788
 - arm-gcc-os-less-lpc43s37
 - arm-gcc-os-less-stm32f767
 - arm-keil-ucos-ii
 - gcc-linux
 - mingw-windows
 
 
 The following command will build the demo application for the LPC1788 CPU using the GCC compiler:

build -t arm-gcc-os-less-lpc1788 -p apps.demo -c all+

The following command will clean the demo_blink application for the LPC1788 CPU using the GCC compiler:

build -t arm-gcc-os-less-lpc1788 -p apps.demo -c clean+

The following command will build all the demo applications for the LPC1788 CPU using the GCC compiler:
 
build -t arm-gcc-os-less-lpc1788 -p apps.* -c all+


The build output can be found in the following folder : build/apps/app_name/bin/target_name

#### Building directly from the Makefiles

To build a project directly from a makefile, you have to open a terminal in the Makefile directory and use the following command lines.

The following command will build the demo application for the LPC1788 CPU using the GCC compiler:

make TARGET=arm-gcc-os-less-lpc1788 all+

The following command will clean the demo application for the LPC1788 CPU using the GCC compiler:

make TARGET=arm-gcc-os-less-lpc1788 clean+

## Demos applications

### Demo environment

The demo applications have been tested on the following boards:

* Embest NXP LPC1768 Evalboard (Cortex-M3, LPC_EMAC network interface)
* Embedded Artists NXP LPC1788 OEM Board (Cortex-M3, LPC_EMAC network interface)
* ST STM32F407 Evalboard (Cortex-M4, SYNOPSYS_EMAC network interface)
* ST NUCLEO STM32F767 (Cortex-M7, SYNOPSYS_EMAC network interface)
* NXP LPCXpresso43S37 - OM13073 board (Cortex-M4, SYNOPSYS_EMAC network interface)

using the following compilers:

* GCC 7.3 for ARM Cortex-M compiler (GNU ARM Embedded Toolchain - https://launchpad.net/gcc-arm-embedded)
* MinGW 7.1 for Windows (MinGW - http://www.mingw.org/)

### Demos

* demo : dhcp, udp and tcp (client + server) communication using the zero copy interface => echo all received data
* demo_socket: dhcp, udp and tcp (client + server) communication using the socket interface => echo all received data
