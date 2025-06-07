// SPDX-License-Identifier: GPL-2.0-or-later
/*
*
*
* Copyright (C) 2025 Daniel Wegkamp
*/

#ifndef STM_DEVICE_REGISTERS_H
#define STM_DEVICE_REGISTERS_H

// Serial Wire DEBUG PORT COMMANDS
#define DP_ABORT_CMD    0x81      // 0b10000001
#define DP_IDCODE_CMD   0xA5      // 0b10100101
#define DP_CTRLSTAT_R_CMD 0x8D       // 0b10001101  CTRLSEL 0
#define DP_WIRECTRL_R_CMD 0x8D       //same         CTRLSEL 1
#define DP_CTRLSTAT_W_CMD 0xA9       // 0b10101001  CTRLSEL 0
#define DP_WIRECTRL_W_CMD 0xA9       //same         CTRLSEL 1
#define DP_READRE_CMD   0x95          // 0b10010101
#define DP_SELECT_CMD   0xB1          // 0b10110001
#define DP_READBUF_CMD  0xBD          // 0b10111101
#define DP_ROUTESEL_CMD 0x99            //optional/undefined

//ACCESS PORT COMMANDS
#define AP_READ0_CMD     0x87    // 0b10000111
#define AP_READ1_CMD     0xAF    // 0b10101111
#define AP_READ2_CMD     0xB7    // 0b10110111
#define AP_READ3_CMD     0x9F    // 0b10011111

#define AP_WRITE0_CMD    0xA3    // 0b10100011
#define AP_WRITE1_CMD    0x8B    // 0b10001011
#define AP_WRITE2_CMD    0x93    // 0b10010011
#define AP_WRITE3_CMD    0xBB    // 0b10111011

//DEVICE registers:
//probaly also works on cortex M3 and M1? maybe?
#define M0_M4_DBG_DFSR     0xE000ED30        // from Arm® Cortex®-M4 Processor Technical Reference Manual - Revision: r0p1
#define M0_M4_DBG_DHCSR    0xE000EDF0        // 2. enable the bit0 (C_DEBUGEN) --> HALT
#define M0_M4_DBG_DCRSR    0xE000EDF4
#define M0_M4_DBG_DCRDR    0xE000EDF8
#define M0_M4_DBG_DEMCR    0xE000EDFC         // 1. enable the bit0 (VC_CORRESET)...

#define M0_M3_M4_CPUID        0xE000ED00
#define M0_M3_M4_SCS_AIRCR    0xE000ED0C



//STM32F303
#define STM32F303xE_FLASHBASE  0x40022000
#define STM32F303xE_SYSCFGBASE  0x40010000




#endif
