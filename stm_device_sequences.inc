 // SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  partially from cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics-1.pdf
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */
 // the sequence content is partially defined in 
 { 0x446, {
            { 8,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0003 },    //halt
            { 8,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0001 },    //unhalt (0xA05F0000 works too but also leaves debug)
            { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DEMCR, AP_WRITE3_CMD, 0x1, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0001 },    //halt on reset
            { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DEMCR, AP_WRITE3_CMD, 0x0, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0000 },    //unhalt on reset
            { 8,   SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M3_M4_SCS_AIRCR, AP_WRITE3_CMD, 0x05FA0004 },     //reset
            { 8,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_VTOR, AP_WRITE3_CMD, 0x20000000},
            { 0 }
        },
      },
 { 0x417, {
            { 8,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0003 },    //halt
            { 8,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0001 },    //unhalt (0xA05F0000 works too but also leaves debug)

            { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DEMCR, AP_WRITE3_CMD, 0x1, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0001 },
            { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_DEMCR, AP_WRITE3_CMD, 0x0, AP_WRITE1_CMD, M0_M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0000 },
            { 8,   SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M3_M4_SCS_AIRCR, AP_WRITE3_CMD, 0x05FA0004 },
            { 8,  SEQ_AP0_noInc, AP_WRITE1_CMD, M0_M4_DBG_VTOR, AP_WRITE3_CMD, 0x20000000},

            { 0 }
        },
    },
/* EOF */
