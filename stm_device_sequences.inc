 // SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  partially from cd00167594-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics-1.pdf
 *
 * Copyright (C) 2025 Daniel Wegkamp
 */

 { 0x446, { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M4_DBG_DEMCR, AP_WRITE3_CMD, 0x1, AP_WRITE1_CMD, M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0001 },
          { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M4_DBG_DEMCR, AP_WRITE3_CMD, 0x0, AP_WRITE1_CMD, M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0000 },
          { 8,   SEQ_AP0_noInc, AP_WRITE3_CMD, M3_SCS_AIRCR, AP_WRITE3_CMD, 0x05FA0004 },
          "STM32F302xD(E)/303xD(E) & STM32F398xE",
      },
  { 0x417, { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M4_DBG_DEMCR, AP_WRITE3_CMD, 0x1, AP_WRITE1_CMD, M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0001 },
           { 10,  SEQ_AP0_noInc, AP_WRITE1_CMD, M4_DBG_DEMCR, AP_WRITE3_CMD, 0x0, AP_WRITE1_CMD, M4_DBG_DHCSR, AP_WRITE3_CMD, 0xA05F0000 },
           { 8,   SEQ_AP0_noInc, AP_WRITE3_CMD, M3_SCS_AIRCR, AP_WRITE3_CMD, 0x05FA0004 },
           "STM32L05x?",
       },
/* EOF */
