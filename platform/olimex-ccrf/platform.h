/*
 * Copyright (c) 2015, Swiss Federal Institute of Technology (ETH Zurich).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Author:  Reto Da Forno
 */

/**
 * @addtogroup  Platform
 * @{
 *
 * @defgroup    platform Platform
 * @{
 *
 * @file
 *
 * @brief platform includes and definitions
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

/*
 * include application specific config
 */
#include "config.h"     /* application specific configuration */

/*
 * configuration and definitions (default values, may be overwritten
 * in config.h)
 */

#ifndef FRAM_CONF_ON
#define FRAM_CONF_ON                1
#define FRAM_CONF_SPI               USCI_B0
#define FRAM_CONF_CTRL_PIN          PORT1, PIN7
#ifndef DEBUG_PRINT_CONF_USE_XMEM
#define DEBUG_PRINT_CONF_USE_XMEM   1
#define LWB_USE_XMEM                1
#endif /* DEBUG_PRINT_CONF_USE_XMEM */
#ifndef DEBUG_PRINT_CONF_NUM_MSG
#define DEBUG_PRINT_CONF_NUM_MSG    20
#endif /* DEBUG_PRINT_CONF_NUM_MSG */
#endif /* FRAM_CONF_ON */

#ifndef BOLT_CONF_ON
#define BOLT_CONF_ON                1
#define BOLT_CONF_SPI               USCI_B0
#define BOLT_CONF_IND_PIN           PORT2, PIN0
#define BOLT_CONF_MODE_PIN          PORT2, PIN1
#define BOLT_CONF_REQ_PIN           PORT2, PIN2
#define BOLT_CONF_ACK_PIN           PORT2, PIN3
/* IND pin for the outgoing queue (sent messages) */
#define BOLT_CONF_IND_OUT_PIN       PORT2, PIN4
#define BOLT_CONF_TIMEREQ_PIN       PORT3, PIN3
#endif /* BOLT_CONF_ON */

#ifndef WATCHDOG_CONF_ON
#define WATCHDOG_CONF_ON            0
#endif /* WATCHDOG_CONF_ON */

#ifndef LEDS_CONF_ON 
#define LEDS_CONF_ON                1
#endif /* LEDS_CONF_ON */

#define MCU_TYPE                    "CC430F5137"
#define COMPILER_INFO               "GCC " __VERSION__
#define GCC_VS                      __GNUC__ __GNUC_MINOR__ __GNUC_PATCHLEVEL__
#define COMPILE_DATE                __DATE__
#define SRAM_SIZE                   4096              /* starting at 0x1C00 */

/* specify the number of timer modules */
#if RF_CONF_ON
/* number of (usable) high-frequency timers (there are 5 CCRs in TA0, one of which is used for the radio module) */
#define RTIMER_CONF_NUM_HF          4       
#else
#define RTIMER_CONF_NUM_HF          5
#endif /* RF_CONF_ON */
/* number of (usable) low-frequency timers (there are 3 CCRs in TA1) */
#define RTIMER_CONF_NUM_LF          3       


#ifndef RF_CONF_TX_CH 
#define RF_CONF_TX_CH               5
#endif /* RF_CONF_TX_CH */


/*
 * The application should define the following two macros for better
 * performance (otherwise glossy will disable all active interrupts).
 */
#define GLOSSY_DISABLE_INTERRUPTS
#define GLOSSY_ENABLE_INTERRUPTS

/*
 * pin mapping
 */
#define LED_RED                     PORT1, PIN0
#define LED_0                       LED_RED 
#define LED_STATUS                  LED_RED
#define LED_ERROR                   LED_RED
#define DEBUG_SWITCH                PORT1, PIN1  /* user push-button */
/*
#define FLOCKLAB_LED1               PORT1, PIN0
#define FLOCKLAB_LED2               PORT1, PIN1
#define FLOCKLAB_LED3       
#define FLOCKLAB_INT1               PORT3, PIN6
#define FLOCKLAB_INT2               PORT3, PIN7
*/

#define DEBUG_PRINT_TASK_ACT_PIN    PORT2, PIN0
#define LWB_CONF_TASK_ACT_PIN       PORT2, PIN1
#define GLOSSY_START_PIN            LED_0    /* let LED flash when glossy starts */
#define GLOSSY_RX_PIN               PORT2, PIN3
#define GLOSSY_TX_PIN               PORT2, PIN4
#define RF_GDO0_PIN                 PORT1, PIN2 // default signal for GDO0 is 3-state
#define RF_GDO1_PIN                 PORT1, PIN3 // default signal for GDO1 is 3-state
#define RF_GDO2_PIN                 PORT1, PIN4 // default signal for GDO2 is RF_RDYn = 0x29 (see Table 25-21 in user guide)
/*
 * NOTE: rf1a_init sets the GDO2 signal as follows: Asserts when sync word has
 * been sent or received, and deasserts at the end of the packet. In RX, the 
 * pin deassert when the optional address check fails or the RX FIFO 
 * overflows. In TX the pin deasserts if the TX FIFO underflows.
 */
#define MCLK_PIN                    PORT2, PIN5
//#define ACLK_PIN                    PORT3, PIN3
//#define SMCLK_PIN                   PORT3, PIN1

/*
 * include standard libraries
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <isr_compat.h>

/*
 * include MCU HAL
 */
#include <cc430f5137.h>             /* or simply include <msp430.h> */

/*
 * include MCU specific drivers
 */

#include "rf1a-SmartRF-settings/868MHz-2GFSK-250kbps.h" /* RF1A config */
#include "adc.h"
#include "clock.h"
#include "dma.h"
#include "flash.h"
#include "glossy.h"
#include "gpio.h"
#include "pmm.h"
#include "leds.h"
#include "rf1a.h"
#include "rtimer.h"
#include "spi.h"
#include "uart.h"
#include "usci.h"
#include "watchdog.h"

#endif /* __PLATFORM_H__ */

/**
 * @}
 * @}
 */
