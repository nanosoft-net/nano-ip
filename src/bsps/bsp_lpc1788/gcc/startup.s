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

    .syntax unified
    .cpu cortex-m3
    .thumb
    
    .section .startup

    .global _start

    .extern pll_init
    .extern c_init
	.extern cpp_init
    .extern main

    .extern _TOP_OF_STACKS_
    
    
    .weak NMI_Handler
    .weak HardFault_Handler
    .weak MPUFault_Handler
    .weak BusFault_Handler
    .weak UsageFault_Handler
	.weak SVCCall_Handler
	.weak PendSV_Handler
	.weak Systick_Handler
    

    .weak   WatchdogTimer_Handler
    .weak   Timer0_Handler       
    .weak   Timer1_Handler       
    .weak   Timer2_Handler       
    .weak   Timer3_Handler       
    .weak   UART0_Handler        
    .weak   UART1_Handler        
    .weak   UART2_Handler        
    .weak   UART3_Handler        
    .weak   PWM1_Handler         
    .weak   I2C0_Handler         
    .weak   I2C1_Handler         
    .weak   I2C2_Handler         
    .weak   SSP0_Handler         
    .weak   SSP1_Handler         
    .weak   PLL0_Handler         
    .weak   RTC_Handler          
    .weak   ExternalInt0_Handler 
    .weak   ExternalInt1_Handler 
    .weak   ExternalInt2_Handler 
    .weak   ExternalInt3_Handler 
    .weak   ADC_Handler          
    .weak   BOD_Handler          
    .weak   USB_Handler          
    .weak   CAN_Handler          
    .weak   GPDMA_Handler        
    .weak   I2S_Handler          
    .weak   Ethernet_Handler     
    .weak   MCI_IRQHandler
    .weak   MotorControl_Handler 
    .weak   QEI_Handler          
    .weak   PLL1_Handler         
    .weak   USBActivity_Handler  
    .weak   CANActivity_Handler  



_start:
_vectors:
    /* Cortex M3 internal interrupts */
    .include "cm3_internal_interrupts.s.inc"

    /* External Interrupts */
    .long   WatchdogTimer_Handler     /* 16: Watchdog Timer */
    .long   Timer0_Handler            /* 17: Timer0 */
    .long   Timer1_Handler            /* 18: Timer1 */
    .long   Timer2_Handler            /* 19: Timer2 */
    .long   Timer3_Handler            /* 20: Timer3 */
    .long   UART0_Handler             /* 21: UART0 */
    .long   UART1_Handler             /* 22: UART1 */
    .long   UART2_Handler             /* 23: UART2 */
    .long   UART3_Handler             /* 24: UART3 */
    .long   PWM1_Handler              /* 25: PWM1 */
    .long   I2C0_Handler              /* 26: I2C0 */
    .long   I2C1_Handler              /* 27: I2C1 */
    .long   I2C2_Handler              /* 28: I2C2 */
    .long   0                         /* 29: Reserved */
    .long   SSP0_Handler              /* 30: SSP0 */
    .long   SSP1_Handler              /* 31: SSP1 */
    .long   PLL0_Handler              /* 32: PLL0 Lock (Main PLL) */
    .long   RTC_Handler               /* 33: Real Time Clock */
    .long   ExternalInt0_Handler      /* 34: External Interrupt 0 */
    .long   ExternalInt1_Handler      /* 35: External Interrupt 1 */
    .long   ExternalInt2_Handler      /* 36: External Interrupt 2 */
    .long   ExternalInt3_Handler      /* 37: External Interrupt 3 */
    .long   ADC_Handler               /* 38: A/D Converter */
    .long   BOD_Handler               /* 39: Brown-Out Detect */
    .long   USB_Handler               /* 40: USB */
    .long   CAN_Handler               /* 41: CAN */
    .long   GPDMA_Handler             /* 42: General Purpose DMA */
    .long   I2S_Handler               /* 43: I2S */
    .long   Ethernet_Handler          /* 44: Ethernet */
    .long   MCI_IRQHandler            /* 45: SD/MMC card I/F */
    .long   MotorControl_Handler      /* 46: Motor Control PWM */
    .long   QEI_Handler               /* 47: Quadrature Encoder Interface */
    .long   PLL1_Handler              /* 48: PLL1 Lock (USB PLL) */
    .long   USBActivity_Handler       /* 49: USB Activity interrupt to wakeup */
    .long   CANActivity_Handler       /* 50: CAN Activity interrupt to wakeup */


.thumb_func
.type Reset_Handler, %function
Reset_Handler:

    /* PLL init */
    bl pll_init

    /* C vars init */
    bl c_init

    /* C++ init */
    bl cpp_init

    /* Branch to main */
    bl main

    /* To prevent exiting from main */
    b       .
  
.thumb_func
NMI_Handler:
    b       .
    
.thumb_func
HardFault_Handler:
    b       .

.thumb_func
MPUFault_Handler:
    b       .

.thumb_func
BusFault_Handler:
    b       .

.thumb_func
UsageFault_Handler:
    b       .

.thumb_func
SVCCall_Handler:
    b       .

.thumb_func
PendSV_Handler:
    b       .

.thumb_func
Systick_Handler:
    b       .


    

.thumb_func
WatchdogTimer_Handler:
    b       .
    
.thumb_func
Timer0_Handler:
    b       .
   
.thumb_func
Timer1_Handler:
    b       .
   
.thumb_func
Timer2_Handler:
    b       .
   
.thumb_func
Timer3_Handler:
    b       .
  
.thumb_func
UART0_Handler:
    b       .
    
.thumb_func
UART1_Handler:
    b       .
       
.thumb_func
UART2_Handler:
    b       .
   
.thumb_func
UART3_Handler:
    b       .   
     
.thumb_func
PWM1_Handler:
    b       .
        
.thumb_func
I2C0_Handler:
    b       .
        
.thumb_func
I2C1_Handler:
    b       .
       
.thumb_func
I2C2_Handler:
    b       .
       
.thumb_func
SSP0_Handler:
    b       .
      
.thumb_func
SSP1_Handler:
    b       .
        
.thumb_func
PLL0_Handler:
    b       .
     
.thumb_func
RTC_Handler:
    b       .

.thumb_func
ExternalInt0_Handler:
    b       .

.thumb_func
ExternalInt1_Handler:
    b       .

.thumb_func
ExternalInt2_Handler:
    b       .

.thumb_func
ExternalInt3_Handler:
    b       .
     
.thumb_func
ADC_Handler:
    b       .

.thumb_func
BOD_Handler:
    b       .       
        
.thumb_func
USB_Handler:
    b       .
        
.thumb_func
CAN_Handler:
    b       .
      
.thumb_func
GPDMA_Handler:
    b       .
        
.thumb_func
I2S_Handler:
    b       .
   
.thumb_func
Ethernet_Handler:
    b       .
       
.thumb_func
MCI_IRQHandler:
    b       .

.thumb_func
MotorControl_Handler:
    b       .
        
.thumb_func
QEI_Handler:
    b       .
       
.thumb_func
PLL1_Handler:
    b       .
  
.thumb_func
USBActivity_Handler:
    b       .

.thumb_func
CANActivity_Handler:
    b       .
    
    
    
    .end
